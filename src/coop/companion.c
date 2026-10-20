// Companions and the story's lead (Ryan, 2026-10-20; docs/coop-design.md "One player").
//
// The lead: the character the story scripts last made active (VM steps 70 SetActiveCharacter / 115 ...NoReset,
// wrapped below; flags 464-467 hold slot + 1, saved with the game; none noted yet reads Gohan while he is in the
// party). Stock scenes are written for the lead: their "player" lines (speaker 1, the active character's box) are
// the lead's ("#GOHAN: how does he know about my father?"). So:
//   * speaker 1 shows the lead's portrait when someone else plays (the portrait getter, below), alone or in co-op;
//   * alone, playing someone else, the lead walks in for the scenes that give him lines: player 2 is spawned for
//     him at the scene's start (again after a map change inside it) and leaves at its end. Nobody is swapped and he
//     does not follow in free play.
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
static struct { u8 standin, lead, _pad[2]; } sc __attribute__((section(".party.cp")));   // a scene's stand-in lead

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

static int usable(int slot) {
    if (slot < 0 || slot >= PARTY_SLOTS || slot == g_Party->active_slot) return 0;
    return (g_Party->chars[slot].flags & 3) == 3;              // in the party and available (not story-locked away)
}

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
    if (sc.standin && usable(sc.lead)) return sc.lead;         // a scene: the lead walks in
    const CharacterEntry *h = &g_Party->chars[HERCULE_SLOT];
    if ((h->flags & 1) && h->display_row == OLD_HERCULE_ROW && g_Party->active_slot != HERCULE_SLOT)
        return HERCULE_SLOT;                                   // the future prologue
    return -1;
}

int companion_active(void) { return companion_slot() >= 0; }

int companion_is(const Entity *e) { return e && e == coop.p2 && companion_active(); }

// ---- scenes: the lead walks in -----------------------------------------------------------------------
// The dialogue sequence start / end (0x0800B4AE / 0x0800B4FC) are re-implemented here (entry jumps,
// tools/patch_rom.py): alone, playing someone else, a sequence with a speaker-1 line spawns player 2 for the lead
// beside player 1 for its length.
#define SCENE_STATE   ((volatile u32 *)0x03001FD8)   // +0 cleared at a start, +4 = a sequence runs

// does the sequence give the player (speaker 1) a line? (entries: u16 next, u8 mode, u8 speaker for text modes 1-4;
// a word whose top 16 bits are 0 stops the scan)
static int has_player_lines(const void *seq) {
    const u32 *e = seq;
    for (int i = 0; i < 160; i++) {
        u32 w = e[i];
        if (!(w >> 16)) return 0;
        const u8 *ent = (const u8 *)w;
        if (ent[2] >= 1 && ent[2] <= 4 && ent[3] == 1) return 1;
    }
    return 0;
}

u32 *scene_begin_hook(u32 *r, const void *seq, u16 *idx, const u32 *pair, u32 arg) {
    if (!r && !(r = Arena_Alloc(0x1C))) return r;
    r[0] = (u32)seq;
    r[1] = (u32)idx;
    r[3] = pair[0];
    r[4] = pair[1];
    r[6] = 0;
    r[5] = arg;
    SCENE_STATE[0] = 0;
    *(u16 *)(r + 2) = *idx;
    SCENE_STATE[1] = 1;
    int lead = other_lead();
    if (alone() && !sc.standin && lead >= 0 && usable(lead) && !coop.p2 && has_player_lines(seq)) {
        sc.lead = (u8)lead;
        sc.standin = 1;
        p2_spawn_now();                                        // the lead walks in beside player 1
    }
    EntityList_Broadcast(g_EntityList, 0xD, 0);                // a cutscene starts
    return r;
}

void scene_end_hook(u32 *r, int free_it) {
    SCENE_STATE[1] = 0;
    EntityList_Broadcast(g_EntityList, 0xE, 0);                // the cutscene ends
    if (free_it) Arena_Free(r);
    if (sc.standin) {
        sc.standin = 0;
        if (coop.p2 && !companion_active()) p2_drop_out();     // the stand-in leaves with the scene
    }
}

// every input poll: a map change inside the scene removed the stand-in: bring him back
void companion_tick(void) {
    if (sc.standin && SCENE_STATE[1] && !coop.p2) p2_spawn_now();
    if (sc.standin && !SCENE_STATE[1]) sc.standin = 0;        // a scene that ended without its end call
}

// player 2's pad while a companion walks: toward player 1 (both axes, so diagonals work); a scene's stand-in
// stands still (the scene stages everyone)
u16 companion_pad(void) {
    Entity *p1 = g_PlayerObject, *p2 = coop.p2;
    if (!p1 || !p2 || !p1_in_play() || sc.standin) { stuck = 0; return 0; }
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
