// The party block and the ROM functions that address it past the entries, re-implemented for
// PARTY_SLOTS entries (docs/party-8-slots.md). The stock block (g_PartyState, 0x03000E90, six entries)
// is followed by other IWRAM globals, so the build pins this one in EWRAM (section .party) and
// tools/patch_rom.py repoints the 27 ROM words that held its address; the static initialiser at
// 0x08022EE0 then runs Party_Init on it at boot. Every function below replaces a ROM function through
// an entry jump (PARTY_ENTRY_JUMPS in patch_rom.py) because the original adds the stock tail offsets
// (+0xB4 flags, +0x11A seen bits, +0x12B counters) as Thumb immediates.
#include "coop.h"

PartyState g_party_block __attribute__((section(".party"), aligned(4)));

// Default party table: tools/patch_rom.py rebuilds it with PARTY_SLOTS entries (stock rows 0-5, then
// data/hercule.json's rows) and repoints the code literal at 0x08003EA0 so that literal + 0xA8 is
// still where the level-up code 0x08003CA0 reads the growth rows.
#define PARTY_DEFAULTS_LITERAL (*(const u32 *)0x08003EA0)
#define PARTY_GROWTH_ROWS      ((const u8 *)(PARTY_DEFAULTS_LITERAL + 0xA8))
#define PARTY_DEFAULTS         ((const CharacterEntry *)(PARTY_GROWTH_ROWS - PARTY_SLOTS * sizeof(CharacterEntry)))
const CharacterEntry *party_default_entries(void) { return PARTY_DEFAULTS; }   // party_save.c (old saves)

ROM_FN(void, VM_RunCondition, (u32 *ctx, const void *code, void *extra), 0x080092E5);   // bytecode condition -> ctx[1]
ROM_FN(int, EntityList_CountKind, (void *list, int kind), 0x08006E65);                // live entities of a kind
ROM_FN(int, Bestiary_IndexOfRecord, (void *sprite_record), 0x08019103);               // -1 if none
ROM_FN(void *, ItemPopup_Create, (void *mem, const void *item), 0x0800FB75);
ROM_FN(int, Modal_Run, (void), 0x0801F2F5);                                           // runs the pushed screen
#define SCRIPT_ENTITY_LIST (*(void **)0x083B5C80)   // the script module's entity list word (its table + 8)
#define ITEM_TABLE         ((const u8 *)0x086ADE24)  // 0x38 bytes per item; +0 usable, +2 popup, +3 max count, +0x30 handler
#define PARTY_FLAG_BANK_ZEROED 50     // Party_Init zeroes 25 halfwords of each bank
#define PARTY_WAIT_FLAG_BYTES 13      // story flags 400-503 (the ten-day wait's side chapters, the post-game) start clear too
#define PARTY_COUNTER_BANK_START (0x12A + PARTY_TAIL_SHIFT)   // the stock DMA target 0x12B rounds down

// Party_Init 0x08003C24: new-game defaults. The stock code DMAs 6 x 0x1C from the default table and
// clears 50 bytes at +0xB4 and at +0x12A; the rest of the block is left as it was.
void flag_set_hook(PartyState *p, u32 id);
void flag_clear_hook(PartyState *p, u32 id);

void party_init_hook(PartyState *p) {
    p->flags = 0;
    p->meter = 0;
    p->active_slot = 3;
    memcpy(p->chars, PARTY_DEFAULTS, PARTY_SLOTS * sizeof(CharacterEntry));
    memset(p->event_flags, 0, PARTY_FLAG_BANK_ZEROED);
    memset(p->event_flags + PARTY_FLAG_BANK_ZEROED, 0, PARTY_WAIT_FLAG_BYTES);   // the wait's chapter flags 400-479
    memset((u8 *)p + PARTY_COUNTER_BANK_START, 0, PARTY_FLAG_BANK_ZEROED);
    const StoryTest *t = STORY_TEST_RAM;      // chapter-test preset (coop.h)
#ifdef STORY_TEST
    if (STORY_PRESET->magic == STORY_TEST_MAGIC) t = STORY_PRESET;
#endif
    if (t->magic == STORY_TEST_MAGIC) {
        for (int i = 0; i < 16 && t->set[i] != 0xFFFF; i++) flag_set_hook(p, t->set[i]);
        for (int i = 0; i < 8 && t->clear[i] != 0xFFFF; i++) flag_clear_hook(p, t->clear[i]);
    }
}

// Party_AddExp 0x08003D58 (the EXP event's fire, exp.c): the stock code reaches its module words (SFX
// mixer, entity list, map state, the VM's "no actor" pair) at negative offsets from the default-table
// literal 0x08003EA0 (literal - 0x1C .. - 0x8 = the module table 0x081D952C in the stock ROM). The
// relocated table moved that literal, so a level-up read party data as pointers and its message-0x15
// broadcast jumped to garbage ("Jumped to invalid address" when the level-up dialogue closed). Same
// steps, module words read from the stock table.
#define LEVELUP_MODULE  ((void *const *)0x081D952C)   // [1] mixer, [2] party (relocated), [3] entity list, [4] map state
#define LEVELUP_NO_ACTOR ((const void *)0x081D9540)   // the stock literal - 8 (two -1 words)
#define LEVELUP_SEQ     (*(const void *const *)0x08003EC0)   // "%s has achieved Level %d!" + Attribute Bonus
#define LEVELUP_SFX     (*(const void *const *)0x08003EC4)
#define LEVELUP_SFX_CH  (*(const u32 *)0x08003EC8)            // 0x5C4
#define LEVELUP_SCREEN_VT (*(const u32 *)0x08003ECC)         // modal screen running the dialogue context
ROM_FN(void, Party_LevelUp, (PartyState *p, int slot, int from, int to), 0x08003CA1);   // growth draws
ROM_FN(void, VmDialog_Init, (void *ctx, const void *seq, void *state, const void *actor, int flags), 0x0800B4AF);
ROM_FN(void, VmDialog_Free, (void *ctx, int free_it), 0x0800B4FD);
ROM_FN(void, Map_RefreshItems, (void *map_state), 0x0800C35F);
ROM_FN(void, PlayerEntity_RefreshSprite, (Entity *e), 0x080086A3);

void party_add_exp_hook(PartyState *p, int slot, int amount) {
    CharacterEntry *c = &p->chars[slot];
    if (c->hp == 0) return;
    int from = Character_GetLevel(c);
    c->exp += (u32)amount;
    int to = Character_GetLevel(c);
    if (from == to) return;
    Party_LevelUp(p, slot, from, to);
    u32 ctx[8];        // the stock frame gives the dialogue context 28 bytes
    u16 state = 0;
    VmDialog_Init(ctx, LEVELUP_SEQ, &state, LEVELUP_NO_ACTOR, 0);
    Sfx_Play((u8 *)LEVELUP_MODULE[1] + LEVELUP_SFX_CH, LEVELUP_SFX, 0x40, 0);
    u32 *screen = Arena_Alloc(8);
    if (screen) { screen[0] = LEVELUP_SCREEN_VT; screen[1] = (u32)ctx; }
    ((int (*)(void *))0x0801F2F5)(screen);   // Modal_Run with the screen pushed in r0
    if (g_PlayerObject) PlayerEntity_RefreshSprite(g_PlayerObject);
    EntityList_Broadcast(LEVELUP_MODULE[3], 0x15, 0);
    Map_RefreshItems(LEVELUP_MODULE[4]);
    VmDialog_Free(ctx, 0);
}

// Flag bank 1 (+0xB4): 0x08003E42 / 0x08003E5E / 0x08003E74, called by the script VM with the party pointer.
int flag_test_hook(PartyState *p, u32 id) {
    return (p->event_flags[id >> 3] >> (id & 7)) & 1;
}
void flag_set_hook(PartyState *p, u32 id) {
    p->event_flags[id >> 3] |= (u8)(1 << (id & 7));
}
void flag_clear_hook(PartyState *p, u32 id) {
    p->event_flags[id >> 3] &= (u8)~(1 << (id & 7));
}

// 0x08003F06: a map script's condition word. 0 = always; low half = flag id, high half = mode
// (1 set, 2 clear, 3 set-and-consume); a ROM address (high half >= 4) = bytecode condition.
int flag_cond_hook(u32 cond) {
    if (cond == 0) return 1;
    u32 mode = cond >> 16;
    if (mode == 0) return 0;
    if (mode <= 3) {
        u8 *b = &g_Party->event_flags[(cond & 0xFFFF) >> 3];
        u8 bit = (u8)(1 << (cond & 7));
        if (mode == 1) return (*b & bit) != 0;
        if (mode == 2) return (*b & bit) == 0;
        if (!(*b & bit)) return 0;
        *b &= (u8)~bit;
        return 1;
    }
    u32 ctx[0x12];
    ctx[0] = 0;
    ctx[0x11] = 0;
    VM_RunCondition(ctx, (const void *)cond, 0);
    return ctx[1] != 0;
}

// Script VM counter opcodes (stack[0] = top index, stack[top] = top value), 0x080095E4 / 96CA / 96E8.
void vm_counter_get_hook(s32 *stack) {
    s32 top = stack[0];
    s32 idx = stack[top];
    stack[top] = g_Party->counters[idx] + EntityList_CountKind(SCRIPT_ENTITY_LIST, 0x17);
}
void vm_counter_dec_hook(s32 *stack) {
    s32 top = stack[0]--;
    g_Party->counters[stack[top]]--;
}
void vm_counter_sub_hook(s32 *stack) {
    s32 top = stack[0];
    s32 amount = stack[top];
    s32 idx = stack[top - 1];
    stack[0] = top - 2;
    g_Party->counters[idx] = (u8)(g_Party->counters[idx] - amount);
}

// 0x0800F3F0: picking up / using item `id`: refuse at the item's max count, show its popup, count it
// or run its handler.
int item_use_hook(int id) {
    const u8 *item = ITEM_TABLE + id * 0x38;
    u8 *count = &g_Party->counters[id];
    if (item[3] <= *count) return 0;
    if (item[2]) {
        ItemPopup_Create(0, item);
        Modal_Run();
    }
    if (item[0]) {
        (*count)++;
        return 1;
    }
    return ((int (*)(int))*(const u32 *)(item + 0x30))(id);
}

// 0x0800B00C: VM opcode marking the bestiary entry of a sprite id as seen (bank 2, +0x11A).
void vm_seen_hook(s32 *stack) {
    s32 top = stack[0]--;
    void *rec = Character_GetSpriteRecord(stack[top]);
    int idx = Bestiary_IndexOfRecord(rec);
    if (idx != -1) g_Party->seen_bits[idx >> 3] |= (u8)(1 << (idx & 7));
}
