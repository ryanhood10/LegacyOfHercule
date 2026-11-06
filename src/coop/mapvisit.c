// The map-visit bits, moved out of the party entries (docs/party-8-slots.md "The map-visit bank", docs/log.md
// 2026-11-06 "SS2 Gohan's crash"), and the repair of saves made while they overlapped.
//
// The stock game keeps one "visited" bit per map (297 maps, ids 0-296 in the ROM table at 0x086F6D8C: 68 groups of
// {zone, area, id}) at party + 0xE7 + id / 8, the stock flag bank's upper half (flags 408-704). Three ROM sites
// address it with that Thumb immediate: MapVisited_Set 0x080040EE (every map load, 0x0800C430; VM action 128 at
// 0x0800AFBC), MapVisited_Test 0x080041A6 (VM action 23 at 0x08009646) and the pause menu's area map (0x08017B34,
// the areas drawn as visited). The party block grew to PARTY_SLOTS entries and its tail moved by 0x54 bytes, but
// these three were never moved: with nine slots the bits landed on party + 0xE7 .. + 0x10C, that is slot 7's flags
// top byte and ability list (ids 0-39, zones 2-3), all of slot 8 (Super Saiyan 2 Gohan; ids 40-263) and story
// flags 0-32 (ids 264-296, zones 9 and 11-14). Each map load ORed one bit in; an ability byte of 64 in his list made
// the HUD's isReady read past the ability table (0xFF padding) and jump to 0xFFFFFFFE as soon as it was selected.
//
// The bits now live in their own bank (g_map_visits, .cine.mv in cineram) and the three sites use it: the two functions are
// replaced by C (PARTY_ENTRY_JUMPS in tools/patch_rom.py), the pause map reads it through a shadow copy of its
// module table (VISIT_MAP_* in patch_rom.py). The bank is saved per save slot in its own EEPROM record (0x1D00 +
// slot * 0x30), outside the party block, so the save slots' layout is unchanged; a save without a record (every save
// made before this fix) starts with no map visited. The old bank's place is not read or cleared: the flags it may
// have set are left as they are (party_repair below only stops what crashes).
#include "coop.h"
#include "a16player.h"
#include "ssj2gohan.h"

#define VISIT_TABLE     ((const VisitTable *)0x086F6D8Cu)
#define VISIT_BITS      320                                  // ids 0-296 used (40 bytes)
#define VISIT_MAGIC     0x5456534Du                          // "MSVT"
#define VISIT_EEPROM    0x1D00                               // 3 x 0x30, below the tournament's record at 0x1E00
#define EEPROM_DEV      (*(void *const *)0x081D952Cu)        // the party module's EEPROM device
#define CELL_GAMES_DONE 195                                  // set by cell_games.lscript just before CoopCall 20 (ssj2_join)
#define LEVEL_CAP       50

typedef struct VisitEntry { u8 zone, area; u16 id; } VisitEntry;
typedef struct VisitGroup { u8 _p[20]; s32 count; const VisitEntry *entry[]; } VisitGroup;
typedef struct VisitTable { s32 count; const VisitGroup *const *group; } VisitTable;

typedef struct MapVisits { u8 bits[VISIT_BITS / 8]; } MapVisits;
typedef struct VisitRecord {    // the EEPROM record per save slot
    u32 magic;
    u32 sum;
    MapVisits v;
} VisitRecord;
_Static_assert(sizeof(VisitRecord) == 0x30, "VisitRecord is 0x30 bytes (8-byte EEPROM blocks)");
// In cineram after cine.c's state (coop.ld: partyram is full). patch_rom.py points the pause map at its bits - 0xE7.
MapVisits g_map_visits __attribute__((section(".cine.mv"), aligned(4)));

ROM_FN(u32, Save_Checksum, (const void *buf, int len), 0x0802153F);
ROM_FN(void, Eeprom_Read, (void *dev, void *dst, int off, int len), 0x08021527);
ROM_FN(void, Eeprom_Write, (void *dev, int off, const void *src, int len), 0x080214C5);
int flag_test_hook(PartyState *p, u32 id);   // party.c
const CharacterEntry *party_default_entries(void);

static int visit_id(u32 zone, u32 area) {
    const VisitTable *t = VISIT_TABLE;
    for (int g = 0; g < t->count; g++) {
        const VisitGroup *gr = t->group[g];
        for (int i = 0; i < gr->count; i++) {
            const VisitEntry *e = gr->entry[i];
            if (e->zone == zone && e->area == area) return e->id < VISIT_BITS ? e->id : -1;
        }
    }
    return -1;
}

// MapVisited_Set 0x080040EE(party, zone, area) / MapVisited_Test 0x080041A6(party, zone, area) -> 0 / 1. The party
// argument is the live block (the map state's and the VM's party word); the bank belongs to the game being played.
void map_visit_set_hook(PartyState *p, u32 zone, u32 area) {
    (void)p;
    int id = visit_id(zone, area);
    if (id >= 0) g_map_visits.bits[id >> 3] |= (u8)(1 << (id & 7));
}
int map_visit_test_hook(PartyState *p, u32 zone, u32 area) {
    (void)p;
    int id = visit_id(zone, area);
    return id >= 0 && (g_map_visits.bits[id >> 3] >> (id & 7)) & 1;
}

// ---- the record per save slot (party_save.c) ----------------------------------------------------------------------
void map_visits_clear(void) { memset(&g_map_visits, 0, sizeof g_map_visits); }

void map_visits_load(int slot) {
    VisitRecord r;
    Eeprom_Read(EEPROM_DEV, &r, VISIT_EEPROM + slot * sizeof r, sizeof r);
    if (r.magic == VISIT_MAGIC && r.sum == Save_Checksum(&r.v, sizeof r.v)) g_map_visits = r.v;
    else map_visits_clear();
}

void map_visits_save(int slot) {
    VisitRecord r = { VISIT_MAGIC, Save_Checksum(&g_map_visits, sizeof g_map_visits), g_map_visits };
    Eeprom_Write(EEPROM_DEV, VISIT_EEPROM + slot * sizeof r, &r, sizeof r);
}

void map_visits_erase(int slot) {
    static const VisitRecord none;
    Eeprom_Write(EEPROM_DEV, VISIT_EEPROM + slot * sizeof none, &none, sizeof none);
}

// ---- a save made while the bits overlapped the entries ------------------------------------------------------------
// Only what crashes or cannot be right is changed; story flags are left alone. Runs on every slot a load reads.
static int ability_count(void) {   // entries up to the first whose icon or isReady is not a ROM address (the 0xFF padding)
    const u32 *t = (const u32 *)ABILITY_TABLE;   // 16-byte entries {icon, icon, isReady, onUse}
    int n = 0;
    while (n < 0xFF && (t[n * 4 + 2] & 0xFE000000u) == 0x08000000u && (t[n * 4] & 0xFE000000u) == 0x08000000u) n++;
    return n;
}

static void repair_entry(CharacterEntry *c, const CharacterEntry *def, const u8 *list) {
    int n = ability_count();
    for (int i = 0; i < 4; i++) {
        if (list) c->ability_list[i] = list[i];          // a known kit: every byte of it was in the bank's reach
        else if (c->ability_list[i] >= n)                 // past the table: isReady at 0xFFFFFFFF
            c->ability_list[i] = def->ability_list[i] < n ? def->ability_list[i] : 2;
    }
    if (c->ability_index > 3) c->ability_index = 0;       // the HUD reads ability_list[index]
    if (c->display_row >= display_row_count) c->display_row = def->display_row;
    // The bank only ever added bits, and no real value has the top bit: clearing it gives back the value when that was
    // the only bit added. Character_GetLevel compares EXP signed (a negative EXP walks it off its table); HP and EP are
    // read signed in places (the abilities' EP checks: a negative EP is never enough).
    c->exp &= 0x7FFFFFFFu;
    u32 cap = (u32)Character_LevelExp(LEVEL_CAP);
    if (c->exp > cap) c->exp = cap;
    c->max_hp &= 0x7FFF;
    c->max_ep &= 0x7FFF;
    c->hp &= 0x7FFF;
    c->ep &= 0x7FFF;
    if (c->hp > c->max_hp) c->hp = c->max_hp;
    if (c->ep > c->max_ep) c->ep = c->max_ep;
}

void party_repair(PartyState *p) {
    const CharacterEntry *def = party_default_entries();
    for (int s = 0; s < PARTY_SLOTS; s++) {
        CharacterEntry *c = &p->chars[s];
#if PARTY_SLOTS > SSJ2_SLOT
        if (s == SSJ2_SLOT) {
            if (!flag_test_hook(p, CELL_GAMES_DONE)) {    // he has not joined: everything in the entry is the bank's
                memset(c, 0, sizeof *c);
                continue;
            }
            static const u8 ssj2_list[4] = SSJ2_ABILITIES;
            c->flags = (c->flags & SSJ2_FLAGS) | 1;       // the bank only ever added bits: none beyond the join's 0x7F
            c->display_row = SSJ2_ROW;
            repair_entry(c, &def[s], ssj2_list);
            continue;
        }
#endif
        if (s == A16_SLOT && c->display_row == A16_ROW) {     // Android 16 after his chapter: his flags' top byte and kit
            static const u8 a16_list[4] = { ROCKET_ABILITY, 19, 0, 2 };   // a16player.c a16_join
            c->flags &= 0x00FFFFFFu;
            if (c->flags & 1) repair_entry(c, &def[s], a16_list);
            continue;
        }
        if (c->flags & 1) repair_entry(c, &def[s], 0);
    }
}
