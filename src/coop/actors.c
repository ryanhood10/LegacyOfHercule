// Duplicate-actor rule (docs/story.md section 0): a town NPC that shows a character one of the
// players is controlling is removed from the scene, so Krillin as player 2 never stands next to
// Krillin the NPC. Town NPCs (map records with handler 0x0800B711, vtable 0x080255CC) are dialogue
// characters that scripts do not move; cutscene actors (handler 0x0800D6DF, bytecode spawns) are left
// alone because scripts address them by sprite (the story-scripts work removes those per scene).
//
// "Same person": the NPC's sprite is the sprite of a display row reachable from the player's current
// row through the transformation links (row +4 / +5), or an NPC-only alias of that slot's character.
// Runs at the start of every entity-list update (entities.c), before anything is drawn, so a removed
// NPC never appears; switching characters at a pad or with Select removes the new duplicate at once.
#include "coop.h"

#define NPC_VTABLE 0x080255CCu

// NPC-only sprite ids of party characters (sprites that are not any display row's).
static const struct { s16 sprite; s16 slot; } aliases[] = {
    { 76, 6 },   // Krillin's NPC set -> Krillin (slot 6)
    { 60, 4 },   // Goku and Super Saiyan Goku -> Goku (slot 4) while he wears the halo (rows 26 / 27,
    { 61, 4 },   //   which link to each other, not to his stock rows 19 / 20)
};

static int row_sprite(int row) { return DISPLAY_DATA[row * 0x14 + 1]; }

// Is `target` reachable from `from` through transformation links (a slot's forms)?
static int same_family(int from, int target) {
    int r = from;
    for (int i = 0; i < 8; i++) {   // the stock chains are 1-3 rows long
        if (r == target) return 1;
        int next = DISPLAY_DATA[r * 0x14 + 4];
        if (next == r) next = DISPLAY_DATA[r * 0x14 + 5];
        if (next == r || next == from || (u32)next >= display_row_count) break;
        r = next;
    }
    return 0;
}

// Does a sprite id show the character in party slot `slot`?
static int shows_slot(int sprite_id, int slot) {
    if (slot < 0 || slot >= PARTY_SLOTS) return 0;
    for (u32 i = 0; i < sizeof aliases / sizeof aliases[0]; i++)
        if (aliases[i].sprite == sprite_id && aliases[i].slot == slot) return 1;
    int row = g_Party->chars[slot].display_row;
    for (u32 r = 0; r < display_row_count; r++)
        if (row_sprite((int)r) == sprite_id && same_family(row, (int)r)) return 1;
    return 0;
}

// The sprite records that show the character in `slot`: its aliases, then its current row's chain of forms (the rows
// same_family reaches, walked directly). Each player's chain is 1-3 rows, so the check below costs a few comparisons
// an NPC; testing all 29 rows per NPC per frame was half the frame in a town with a crowd, twice over with player 2
// (the probe, docs/perfect-pass.md 11.6: 150 % of a frame in West City).
static int slot_records(int slot, const void **out, int n) {
    if (slot < 0 || slot >= PARTY_SLOTS) return n;
    for (u32 i = 0; i < sizeof aliases / sizeof aliases[0]; i++)
        if (aliases[i].slot == slot) out[n++] = Character_GetSpriteRecord(aliases[i].sprite);
    int from = g_Party->chars[slot].display_row, r = from;
    for (int i = 0; i < 8 && (u32)r < display_row_count; i++) {
        out[n++] = Character_GetSpriteRecord(row_sprite(r));
        int next = DISPLAY_DATA[r * 0x14 + 4];
        if (next == r) next = DISPLAY_DATA[r * 0x14 + 5];
        if (next == r || next == from) break;
        r = next;
    }
    return n;
}

void actors_tick(void) {
    if (coop2.off & COOP_OFF_DUPES) return;
    EntityList *list = g_EntityList;
    int s1 = g_PlayerObject ? g_Party->active_slot : -1;
    int s2 = coop.p2 ? coop.p2_slot : -1;
    if (s1 < 0 && s2 < 0) return;
    const void *recs[2 * (8 + sizeof aliases / sizeof aliases[0])];
    int nrecs = -1;
    for (int i = list->count - 1; i >= 0; i--) {
        Entity *e = list->items[i];
        if (!e || e->vtable != (u32 *)NPC_VTABLE) continue;
        if (nrecs < 0) nrecs = slot_records(s2, recs, slot_records(s1, recs, 0));   // the first NPC of the frame
        for (int k = 0; k < nrecs; k++) {
            if (e->sprite_record != recs[k]) continue;
            EntityList_Remove(list, e, 0);   // the list's own removal (vtable slot 1 tears it down)
            coop2.dupes_removed++;
            break;
        }
    }
}

// Unused for now: id-level test, kept for the map tools (a record's sprite id against a slot).
int sprite_shows_slot(int sprite_id, int slot) { return shows_slot(sprite_id, slot); }
