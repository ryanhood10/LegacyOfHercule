// Companions and the story's lead (Ryan, 2026-10-20; docs/coop-design.md "One player").
//
// The lead: the character the story scripts last made active (VM steps 70 SetActiveCharacter / 115 ...NoReset,
// wrapped below; flags 464-467 hold slot + 1, saved with the game; none noted yet reads Gohan while he is in the
// party). Stock scenes are written for the lead: their "player" lines (speaker 1, the active character's box) are
// the lead's ("#GOHAN: how does he know about my father?"). So:
//   * speaker 1 shows the lead's portrait when someone else plays (the portrait getter, below), alone or in co-op;
//   * when nobody plays the lead, our scene scripts stage him as an actor walking and flying in with the others
//     (CoopCall 22 guards those steps). Nobody is swapped and he does not follow in free play.
// Old Hercule in the future prologue (party slot 5 on his old look, display row OLD_HERCULE_ROW) is player 2 there
// in co-op; alone he walks along with player 1 (a follower pad).
// A companion is not a player for the rules: no twins or co-op scaling (p2_coming), no revive, enemies ignore him
// and he takes no damage, scenes see one player (CoopCall 5) and duels are skipped. His HUD panel is hidden. A real
// second controller takes over (2 Players).
#include "coop.h"
#include "krillin.h"

#define COMPANION_NEAR   28    // px: close enough, stand still
#define COMPANION_STUCK  90    // frames without getting closer while far ...
#define COMPANION_FAR    96    // ... than this many px: jump to player 1's side
#define LEAD_FLAG        464   // flags 464-467: the story's lead, slot + 1 (0 = none yet)

static u16 stuck __attribute__((section(".party.cp")));
static s32 last_d2 __attribute__((section(".party.cp")));

// ---- the story's lead -------------------------------------------------------------------------------
static int lead_slot(void) {
    const u8 *f = g_Party->event_flags;
    int v = 0;
    for (int i = 0; i < 4; i++) v |= ((f[(LEAD_FLAG + i) >> 3] >> ((LEAD_FLAG + i) & 7)) & 1) << i;
    if (v) return v - 1;
    // none noted yet (a save from before the lead was noted): Gohan, the stock game's lead, while he is in the party;
    // the next SetActiveCharacter of the story notes the real one
    return (g_Party->chars[0].flags & 3) == 3 ? 0 : -1;
}

static void lead_set(s32 slot) {
    if (slot < 0 || slot >= PARTY_SLOTS) return;
    u8 *f = g_Party->event_flags;
    for (int i = 0; i < 4; i++) {
        int n = LEAD_FLAG + i, bit = 1 << (n & 7);
        if ((slot + 1) & (1 << i)) f[n >> 3] |= (u8)bit;
        else f[n >> 3] &= (u8)~bit;
    }
}

// The VM's SetActiveCharacter (step 70) and SetActiveCharacterNoReset (115), repointed here by data/hercule.json:
// note the slot (the top of the stack), then the stock step.
ROM_FN(void, Step_SetActiveCharacter, (s32 *st), 0x0800A785);
ROM_FN(void, Step_SetActiveCharacterNoReset, (s32 *st), 0x0800A681);
void vm_set_active_hook(s32 *st) { lead_set(st[st[0]]); Step_SetActiveCharacter(st); }
void vm_set_active_noreset_hook(s32 *st) { lead_set(st[st[0]]); Step_SetActiveCharacterNoReset(st); }

static int alone(void);

// Is the lead in the scene: player 2 plays him, or an actor of his stands (the perfecting pass, Ryan 2026-11-01:
// alone as Krillin, the player's own lines showed Gohan). Actor sprites by party slot (0 = none further).
static const u8 slot_actor_sprites[9][6] = {
    { 53, 55, 56, 57, 59, 0 },        // Gohan (not 54, Future Gohan)
    { 73, 103, 104, 0 },              // Piccolo
    { 137, 138, 139, 140, 0 },        // Vegeta
    { 128, 132, 134, 135, 0 },        // Trunks
    { 60, 61, 62, 176, 177, 0 },      // Goku (and haloed)
    { 72, 158, 0 },                   // Hercule, old Hercule
    { 76, 161, 0 },                   // Krillin
    { 8, 175, 0 },                    // Android 16
    { 58, 0 },                        // Super Saiyan 2 Gohan
};
static int lead_present(int lead) {
    if (coop.p2_slot == lead && !alone()) return 1;           // player 2 (even while a warp has his entity away)
    if (lead >= 9) return 0;
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) {
        Entity *e = l->items[i];
        if (!e || e == g_PlayerObject || e == coop.p2) continue;
        for (const u8 *sp = slot_actor_sprites[lead]; *sp; sp++)
            if (e->sprite_record == Character_GetSpriteRecord(*sp)) return 1;
    }
    return 0;
}

// the lead when someone else plays, he is in the party and he is in the scene, else -1 (then speaker 1 is player 1's
// own character, as in the stock game: lines like "%s: Hey, what happened to you?" outside his scenes)
static int other_lead(void) {
    int lead = lead_slot();
    return lead >= 0 && lead != g_Party->active_slot && (g_Party->chars[lead].flags & 1) && lead_present(lead) ? lead : -1;
}

// ---- the portrait getter (0x0800BDCC, re-implemented: an entry jump) ----------------------------------
// id 0 none; 1 the active character's (the lead's when someone else plays and the lead is in the scene); 250 player 1's
// own character always; 2-7 party slot id - 2; 8+ the
// portrait module table (the getter's literal at 0x0800BE10: word 0 the party, word id + 1 a portrait)
ROM_FN(int, Character_Portrait, (const CharacterEntry *c), 0x08026441);
#define PLAYER1_PORTRAIT 250   // speaker id: player 1's own character, never the lead (docs/ids.md "Portraits")
int portrait_get_hook(int id) {
    if (!id) return 0;
    if (id == PLAYER1_PORTRAIT) return Character_Portrait(&g_Party->chars[g_Party->active_slot]);
    if (id == 1) {
        int lead = other_lead();
        return Character_Portrait(&g_Party->chars[lead >= 0 ? lead : g_Party->active_slot]);
    }
    if (id < 8) return Character_Portrait(&g_Party->chars[id - 2]);
    return (*(const s32 *const *)0x0800BE10)[id + 1];
}

// ---- who walks along ---------------------------------------------------------------------------------
static int alone(void) { return (players_one() || !p2_pad_present()) && !coop_solo() && !coop2.duel_active; }

int companion_slot(void) {
    if (!alone()) return -1;                                   // a second controller plays player 2; a solo fight; a duel
    const CharacterEntry *h = &g_Party->chars[HERCULE_SLOT];
    if ((h->flags & 1) && h->display_row == OLD_HERCULE_ROW && g_Party->active_slot != HERCULE_SLOT)
        return HERCULE_SLOT;                                   // the future prologue
    return -1;
}

int companion_active(void) { return companion_slot() >= 0; }

int companion_is(const Entity *e) { return e && e == coop.p2 && companion_active(); }

// ---- scenes: the lead's actor ------------------------------------------------------------------------
// Stock scenes stage the player as the lead. When nobody plays him, our scene scripts stage him as an actor of
// his own (walking and flying in with the others, data/story/ch1_opening): CoopCall 22 guards those steps.
//   op 1, a = party slot: 1 when that character is in the party and neither player plays him (spawn his actor);
//         player 2 plays him while a second player is in (not alone), whether or not his entity stands yet;
//   op 2, a = sprite id: 1 when an actor (not a player) of that sprite stands on the map (move it);
//   op 3: the name in capitals of whoever speaker 1 shows (the lead in the scene, else player 1; a %s argument: "%s: Hey dad!");
//   op 4, a = party slot: 1 when player 1 plays him, or player 2 does while a second player is in: he is in the scene
//         (Ryan, 2026-11-05: a party member nobody plays is not in the scene; docs/howto.md "Stage a scene" rule 2).
extern const char *const ui_strings[];   // relocated UI strings (krillin.c): UI_CAPS + slot = a name in capitals
s32 companion_call(int op, int a) {
    if (op == 1) {
        if (a < 0 || a >= PARTY_SLOTS || !(g_Party->chars[a].flags & 1)) return 0;
        // player 2's character counts while a second player plays, even before his entity respawns after a warp
        return a != g_Party->active_slot && !(coop.p2_slot == a && !alone());
    }
    if (op == 2) {
        const void *rec = Character_GetSpriteRecord(a);
        EntityList *l = g_EntityList;
        for (int i = 0; i < l->count; i++) {
            Entity *e = l->items[i];
            if (e && e != g_PlayerObject && e != coop.p2 && e->sprite_record == rec) return 1;
        }
        return 0;
    }
    if (op == 4) return a >= 0 && a < PARTY_SLOTS && (a == g_Party->active_slot || (coop.p2_slot == a && !alone()));
    if (op == 3) {   // the name speaker 1 shows: the lead's while he is in the scene, else player 1's (Ryan, 2026-11-05)
        int lead = other_lead();
        return (s32)ui_strings[UI_CAPS + (lead >= 0 ? lead : g_Party->active_slot)];
    }
    return 0;
}

// player 2's pad while a companion walks: toward player 1 (both axes, so diagonals work)
u16 companion_pad(void) {
    Entity *p1 = g_PlayerObject, *p2 = coop.p2;
    if (!p1 || !p2 || !p1_in_play()) { stuck = 0; return 0; }
    int slot = companion_slot();
    if (slot >= 0 && coop.p2_slot != slot) p2_bind_slot(slot);   // the companion is this character now
    s32 dx = (p1->x - p2->x) >> 8, dy = (p1->y - p2->y) >> 8;
    s32 d2 = dx * dx + dy * dy;
    if (d2 <= COMPANION_NEAR * COMPANION_NEAR) { stuck = 0; last_d2 = d2; return 0; }
    if (d2 > COMPANION_FAR * COMPANION_FAR && d2 >= last_d2) {
        if (++stuck > COMPANION_STUCK) {                   // walled off: appear beside player 1
            p2->x = p1->x - (16 << 8);
            p2->y = p1->y;
            stuck = 0;
        }
    } else {
        stuck = 0;
    }
    last_d2 = d2;
    u16 pad = 0;
    if (dx > 8) pad |= KEY_RIGHT;
    else if (dx < -8) pad |= KEY_LEFT;
    if (dy > 8) pad |= KEY_DOWN;
    else if (dy < -8) pad |= KEY_UP;
    return pad;
}

// ---- scenes: player 2 handed to an actor (CoopCall 31) -------------------------------------------------
// Player 2's entity takes no part in scenes (it cannot be walked by a script, and the companion's pad is not
// read while one runs), so a scene that should move him stands an actor of his sprite on his spot and removes the
// entity (Ryan, 2026-11-01: old Hercule walking into Gohan's death scene instead of stopping, then popping):
//   op 0 -> 1 when player 2's entity stands; op 1 / 2 -> its x / y in px; op 3: remove it (the usual respawn beside
//   player 1 brings it back once he is free; nothing is pushed).
s32 p2_actor_call(int op) {
    Entity *p2 = coop.p2;
    if (op == 0) return p2 != 0;
    if (!p2) return 0;
    if (op == 1) return p2->x >> 8;
    if (op == 2) return p2->y >> 8;
    if (op == 3) EntityList_Remove(g_SceneTable[1], p2, 0);   // its destroy clears coop.p2 (cg_set_solo does the same)
    return 0;
}
