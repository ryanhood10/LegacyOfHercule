// The save path for a PARTY_SLOTS-entry party (docs/party-8-slots.md step 3). The stock game copies
// the 0x164-byte party block into a 0x168-byte buffer (block + checksum) and keeps three such slots
// in the 8 KiB EEPROM at 0x10 + slot * 0x168; the save menu keeps a copy of every slot in a 3 x 0x164
// array embedded in its screen object, with six member pointers per slot. All of that is sized by
// the stock block, so these functions replace (entry jumps, PARTY_ENTRY_JUMPS in tools/patch_rom.py):
//   party module  SaveGame 0x08004030, LoadSlot 0x08003F9C, WriteSlot 0x08003FEC
//   save menu     slot reader 0x08001960, the slot queries 0x080019D4 .. 0x08001AE2, erase 0x08001B0E,
//                 start game 0x0800180C
// with a new EEPROM layout (slots of SAVE_STRIDE bytes) and a private slot store in the .party
// region. The menu's screen object keeps its stock size: its embedded array is unused, and its
// six-wide portrait rows show a window of at most six members (SAVE_MENU_MEMBERS) around the slot's
// active character. Old (six-slot) saves fail the checksum and show as empty.
#include "coop.h"

#define SAVE_HEADER       0x10                                   // options + checksum (0x08001E4A), untouched
#define SAVE_STRIDE       ((sizeof(PartyState) + 4 + 7) & ~7u)   // block + u32 checksum, EEPROM writes 8 bytes at a time
#define SAVE_SLOTS        3
#define SAVE_MENU_MEMBERS 6                                      // portraits per slot in the stock save menu
_Static_assert(SAVE_HEADER + SAVE_SLOTS * SAVE_STRIDE <= 0x2000, "saves fit the 8 KiB EEPROM");

typedef struct SaveSlot {
    PartyState party;
    u32 checksum;
    u32 pad[1];           // SAVE_STRIDE rounds up to 8: the read lands here
} SaveSlot;
_Static_assert(sizeof(SaveSlot) >= SAVE_STRIDE, "SaveSlot covers a stride");

// Replaces the save menu's embedded slot array (3 x 0x164 at menu + 0x14, member pointers at + 0x42C).
typedef struct SaveStore {
    SaveSlot slot[SAVE_SLOTS];
    u8 members[SAVE_SLOTS][PARTY_SLOTS];   // in-party slots in order
    u8 count[SAVE_SLOTS];                  // their number
    u8 first[SAVE_SLOTS];                  // first member shown (window of SAVE_MENU_MEMBERS around the active one)
} SaveStore;
static SaveStore store __attribute__((section(".party"), aligned(4)));

// ROM pieces (pool words of the stock functions)
#define PARTY_MODULE_TABLE ((void *const *)0x081D952C)   // [0] EEPROM device, [2] party, [4] map state
#define SAVE_MENU_TABLE    ((void *const *)0x081D4F14)   // [3] party, [4] map state
#define g_SelectedSaveSlot (*(u8 *)0x03000E8C)
#define LOCATION_NAMES     ((const char *const *)0x080E2840)
#define NEW_GAME_ENTRY     ((const u8 *)0x08409410)      // map entry: +0 zone, +1 area
#define FADE_VTABLE        0x08023884u
#define TIMER_VTABLE       0x08023990u
ROM_FN(u32, Save_Checksum, (const void *buf, int len), 0x0802153F);
ROM_FN(void, Eeprom_Read, (void *dev, void *dst, int off, int len), 0x08021527);    // DMA from the EEPROM mirror
ROM_FN(void, Eeprom_Write, (void *dev, int off, const void *src, int len), 0x080214C5);   // mirror + EEPROM, 8-byte blocks
ROM_FN(void, Party_DetransformOthers, (PartyState *p), 0x08003ED5);
ROM_FN(const u8 *, Party_LocationEntry, (const PartyState *p), 0x0800409F);   // map entry of the save position
ROM_FN(u32, Location_NameIndex, (const u8 *entry), 0x0800EBB7);
ROM_FN(int, MapEntry_Variation, (const u8 *entry, int v), 0x0800EB21);
ROM_FN(const u8 *, MapEntry_Sub, (const u8 *entry, int v), 0x0800EB4D);
ROM_FN(u32 *, Map_Enter, (void *mapstate, int zone, int area, const s16 *xy, int variation, int one), 0x0800C585);
ROM_FN(void *, TitleScreen_Create, (void *mem), 0x0800020D);
ROM_FN(void, Scene_Transition, (void), 0x0801F2CF);
ROM_FN(int, Scene_Run, (void), 0x0801F2F5);

void party_init_hook(PartyState *p);   // party.c

// ---- party module ------------------------------------------------------------------------------

static void write_slot(SaveSlot *s, int slot) {
    s->checksum = Save_Checksum(&s->party, sizeof(PartyState));
    Eeprom_Write(PARTY_MODULE_TABLE[0], SAVE_HEADER + slot * SAVE_STRIDE, s, SAVE_STRIDE);
}

// WriteSlot 0x08003FEC(src, slot): used by the menu's erase with one of the store's own slots.
void write_slot_hook(const PartyState *src, int slot) {
    SaveSlot *s = &store.slot[slot];
    if (src != &s->party) memcpy(&s->party, src, sizeof(PartyState));
    write_slot(s, slot);
}

// LoadSlot 0x08003F9C(dst, slot): a slot whose checksum fails reads as a fresh party.
void load_slot_hook(PartyState *dst, int slot) {
    SaveSlot *s = &store.slot[slot];
    party_init_hook(&s->party);
    Eeprom_Read(PARTY_MODULE_TABLE[0], s, SAVE_HEADER + slot * SAVE_STRIDE, SAVE_STRIDE);
    if (Save_Checksum(&s->party, sizeof(PartyState)) != s->checksum) party_init_hook(&s->party);
    if (dst != &s->party) memcpy(dst, &s->party, sizeof(PartyState));
}

// SaveGame 0x08004030(party, xy): the save pad. Records the map and position, then writes the
// selected slot.
void save_game_hook(PartyState *p, const s32 *xy) {
    const u8 *map = PARTY_MODULE_TABLE[4];
    p->zone = map[0];
    p->area = map[1];
    p->variation = map[2];
    p->save_x = (s16)xy[0];
    p->save_y = (s16)xy[1];
    p->flags |= 1;
    write_slot_hook(p, g_SelectedSaveSlot);
}

// ---- save menu ---------------------------------------------------------------------------------
// `handle` is the stock array (menu + 0x14); only the store is used.

static void build_view(int slot) {
    PartyState *p = &store.slot[slot].party;
    int n = 0, pos = 0;
    for (int i = 0; i < PARTY_SLOTS; i++) {
        if (!(p->chars[i].flags & 1)) continue;
        if (i == p->active_slot) pos = n;
        store.members[slot][n++] = (u8)i;
    }
    store.count[slot] = (u8)n;
    int first = n > SAVE_MENU_MEMBERS ? pos - (SAVE_MENU_MEMBERS - 1) : 0;
    if (first < 0) first = 0;
    store.first[slot] = (u8)first;
}

static int view_count(int slot) {
    int n = store.count[slot] - store.first[slot];
    return n > SAVE_MENU_MEMBERS ? SAVE_MENU_MEMBERS : n;
}

static CharacterEntry *view_member(int slot, int i) {
    return &store.slot[slot].party.chars[store.members[slot][store.first[slot] + i]];
}

// 0x08001960(handle): read the three slots and list their members.
void *save_menu_load_hook(void *handle) {
    for (int s = 0; s < SAVE_SLOTS; s++) {
        load_slot_hook(&store.slot[s].party, s);
        build_view(s);
    }
    return handle;
}

// 0x080019D4(handle): any slot saved?
int save_menu_any_hook(void *handle) {
    (void)handle;
    for (int s = 0; s < SAVE_SLOTS; s++)
        if (store.slot[s].party.flags & 1) return 1;
    return 0;
}

// 0x080019F6(handle, slot)
int save_menu_saved_hook(void *handle, int slot) {
    (void)handle;
    return store.slot[slot].party.flags & 1;
}

// 0x08001A0C(handle, slot): the saved location's name
const char *save_menu_location_hook(void *handle, int slot) {
    (void)handle;
    return LOCATION_NAMES[Location_NameIndex(Party_LocationEntry(&store.slot[slot].party))];
}

// 0x08001A26(handle, slot): portraits to draw (0 for an empty slot)
int save_menu_count_hook(void *handle, int slot) {
    (void)handle;
    if (!(store.slot[slot].party.flags & 1)) return 0;
    return view_count(slot);
}

// 0x08001A44 (unreferenced in the ROM) / 0x08001A66(handle, slot, i): member i in party / selectable
int save_menu_member_in_party_hook(void *handle, int slot, int i) {
    (void)handle;
    return view_member(slot, i)->flags & 1;
}
int save_menu_member_selectable_hook(void *handle, int slot, int i) {
    (void)handle;
    return (view_member(slot, i)->flags & 2) != 0;
}

// 0x08001A8A(handle, slot): the active character's position among the portraits
int save_menu_active_pos_hook(void *handle, int slot) {
    (void)handle;
    PartyState *p = &store.slot[slot].party;
    for (int i = 0; i < view_count(slot); i++)
        if (store.members[slot][store.first[slot] + i] == p->active_slot) return i;
    return 0;
}

// 0x08001AB8(handle, slot, i): member i's sprite record (for the portrait)
void *save_menu_member_sprite_hook(void *handle, int slot, int i) {
    (void)handle;
    const u8 *row = DISPLAY_DATA + view_member(slot, i)->display_row * 0x14;
    return Character_GetSpriteRecord(row[1]);
}

// 0x08001AE2(handle, slot, i): start as member i
void save_menu_set_active_hook(void *handle, int slot, int i) {
    (void)handle;
    if (i < 0 || i >= view_count(slot)) return;
    store.slot[slot].party.active_slot = store.members[slot][store.first[slot] + i];
}

// 0x08001B0E(handle, slot): erase
void save_menu_erase_hook(void *handle, int slot) {
    (void)handle;
    party_init_hook(&store.slot[slot].party);
    write_slot(&store.slot[slot], slot);
    build_view(slot);
}

// 0x0800180C(menu): the menu's start action. menu + 0x10 = chosen slot (3 = back to the title).
void save_menu_start_hook(u8 *menu) {
    s32 sel = *(s32 *)(menu + 0x10);
    if (sel == 3) {
        TitleScreen_Create(0);
        Scene_Transition();
        return;
    }
    g_SelectedSaveSlot = (u8)sel;
    PartyState *party = SAVE_MENU_TABLE[3];
    u8 *mapstate = SAVE_MENU_TABLE[4];
    *(u16 *)(mapstate + 8) = 0;
    *(u32 *)(mapstate + 4) = 0;
    if (store.slot[sel].party.flags & 1) {
        memcpy(party, &store.slot[sel].party, sizeof(PartyState));
        Party_DetransformOthers(party);
        const u8 *loc = Party_LocationEntry(party);
        u32 *fade = Arena_Alloc(4);
        if (fade) fade[0] = FADE_VTABLE;
        Scene_Transition();
        Map_Enter(mapstate, loc[0], loc[1], &party->save_x, party->variation, 1);
        // re-run the active character's behaviour class init (display row +0x10 -> IWRAM cell -> vtable slot 2)
        const u8 *row = DISPLAY_DATA + party->chars[party->active_slot].display_row * 0x14;
        u32 *cell = *(u32 **)(row + 0x10);
        const u32 *vt = (const u32 *)*cell;
        ((void (*)(u32 *))((u32)vt + vt[2]))(cell);
    } else {
        party_init_hook(party);
        int v = MapEntry_Variation(NEW_GAME_ENTRY, 0);
        const u8 *sub = MapEntry_Sub(NEW_GAME_ENTRY, v);
        u32 *fade = Arena_Alloc(4);
        if (fade) fade[0] = FADE_VTABLE;
        Scene_Transition();
        u32 *map = Map_Enter(mapstate, NEW_GAME_ENTRY[0], NEW_GAME_ENTRY[1], (const s16 *)(sub + 0x30), v, 1);
        ((void (*)(u32 *, int))vslot(map, 10))(map, 0x26);
    }
    u32 *timer = Arena_Alloc(0xC);
    if (timer) { timer[0] = TIMER_VTABLE; timer[1] = 0; timer[2] = 4; }
    Scene_Run();
}
