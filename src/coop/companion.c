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

// the lead when someone else plays (and he is in the party), else -1
static int other_lead(void) {
    int lead = lead_slot();
    return lead >= 0 && lead != g_Party->active_slot && (g_Party->chars[lead].flags & 1) ? lead : -1;
}

// ---- the portrait getter (0x0800BDCC, re-implemented: an entry jump) ----------------------------------
// id 0 none; 1 the active character's (the lead's when someone else plays); 2-7 party slot id - 2; 8+ the
// portrait module table (the getter's literal at 0x0800BE10: word 0 the party, word id + 1 a portrait)
ROM_FN(int, Character_Portrait, (const CharacterEntry *c), 0x08026441);
int portrait_get_hook(int id) {
    if (!id) return 0;
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
//   op 3: the lead's name in capitals (a %s argument: "%s: Hey dad!").
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
    if (op == 3) {
        int lead = lead_slot();
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
