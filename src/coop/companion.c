// A companion for one player (Ryan, 2026-10-20; docs/coop-design.md "One player"): when nobody plays player 2
// (1 Player chosen, or no second controller), player 2 is still spawned for one character, its pad driven by a
// simple follower (walk toward player 1 beyond COMPANION_NEAR px; jump beside him when walled off far behind):
//   * the story's lead, when player 1 plays someone else: the lead is the character the story scripts last made
//     active (SetActiveCharacter / ...NoReset, wrapped below; flags 464-467 hold slot + 1, saved with the game),
//     so Gohan walks with a player who switched to Krillin, and his scenes have him;
//   * old Hercule in the future prologue (party slot 5 on his old look, display row OLD_HERCULE_ROW), who is player
//     2 there in co-op.
// The companion is not a player for the rules: no twins or co-op scaling (p2_coming), no revive, enemies ignore
// him and he takes no damage, scenes see one player (CoopCall 5) and duels are skipped. His HUD panel is hidden.
// A real second controller takes over (2 Players); when the reason ends (player 1 plays the lead again, the
// prologue drops old Hercule) he leaves (p2_drop_out).
#include "coop.h"
#include "krillin.h"

#define COMPANION_NEAR   28    // px: close enough, stand still
#define COMPANION_STUCK  90    // frames without getting closer while far ...
#define COMPANION_FAR    96    // ... than this many px: jump to player 1's side
#define LEAD_FLAG        464   // flags 464-467: the story's lead, slot + 1 (0 = none yet)

static u16 stuck __attribute__((section(".party.cp")));
static s32 last_d2 __attribute__((section(".party.cp")));
static struct { u8 swapped, player, lead, p2; } sw __attribute__((section(".party.cp")));   // a scene's swap (p2: player 2's slot before it)

// ---- the story's lead -------------------------------------------------------------------------------
static int lead_slot(void) {
    const u8 *f = g_Party->event_flags;
    int v = 0;
    for (int i = 0; i < 4; i++) v |= ((f[(LEAD_FLAG + i) >> 3] >> ((LEAD_FLAG + i) & 7)) & 1) << i;
    return v ? v - 1 : -1;
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

// ---- who follows -------------------------------------------------------------------------------------
static int usable(int slot) {
    if (slot < 0 || slot >= PARTY_SLOTS || slot == g_Party->active_slot) return 0;
    return (g_Party->chars[slot].flags & 3) == 3;              // in the party and available (not story-locked away)
}

static int alone(void) { return (players_one() || !p2_pad_present()) && !coop_solo() && !coop2.duel_active; }

int companion_slot(void) {
    if (!alone()) return -1;                                   // a second controller plays player 2; a solo fight; a duel
    if (sw.swapped && usable(sw.player)) return sw.player;     // a scene: the player's own character beside the lead
    const CharacterEntry *h = &g_Party->chars[HERCULE_SLOT];
    if ((h->flags & 1) && h->display_row == OLD_HERCULE_ROW && g_Party->active_slot != HERCULE_SLOT)
        return HERCULE_SLOT;                                   // the future prologue
    int lead = lead_slot();
    return usable(lead) ? lead : -1;
}

int companion_active(void) { return companion_slot() >= 0; }

// ---- scenes: the lead plays them --------------------------------------------------------------------
// Stock scenes give the player's lines and place to whoever plays (written for the story's lead: "GOHAN: how does
// he know my father?"). So a dialogue sequence starts with the lead as player 1 when player 1 plays someone else:
// alone, the player's own character becomes the companion (the co-op staging); with two players, a player 2 who
// plays the lead trades with player 1 for the scene, and otherwise player 2 stays (only two characters can stand
// in). The end puts everyone back (unless the scene itself changed who plays): coop_active_check trades player 2. The sequence start / end are
// 0x0800B4AE / 0x0800B4FC, re-implemented here (entry jumps, tools/patch_rom.py) with the swap added.
#define SCENE_STATE   ((volatile u32 *)0x03001FD8)   // +0 cleared at a start, +4 = a sequence runs

static void set_active(int slot) { s32 st[2] = { 1, slot }; Step_SetActiveCharacter(st); }

static void scene_swap_in(void) {
    if (sw.swapped || coop_solo() || coop2.duel_active || !g_PlayerObject) return;   // alone or with player 2
    int lead = lead_slot(), act = g_Party->active_slot;
    if (lead < 0 || lead == act || !usable(lead)) return;
    sw.player = (u8)act;
    sw.lead = (u8)lead;
    sw.p2 = coop.p2_slot;
    sw.swapped = 1;
    set_active(lead);
    if (coop.p2 && (alone() || coop.p2_slot == lead)) p2_bind_slot(act);   // the companion, or player 2 who played the lead
}

static void scene_swap_out(void) {
    if (!sw.swapped) return;
    sw.swapped = 0;
    if (g_Party->active_slot != sw.lead || !usable(sw.player)) return;   // the scene changed who plays
    set_active(sw.player);
    // player 2 back to its character (a map reload in the scene may have despawned it: then its respawn picks it)
    if (sw.p2 < PARTY_SLOTS && sw.p2 != sw.player) {
        if (coop.p2) p2_bind_slot(sw.p2);
        else coop.p2_slot = sw.p2;
    }
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
    scene_swap_in();
    EntityList_Broadcast(g_EntityList, 0xD, 0);                // a cutscene starts
    return r;
}

void scene_end_hook(u32 *r, int free_it) {
    SCENE_STATE[1] = 0;
    EntityList_Broadcast(g_EntityList, 0xE, 0);                // the cutscene ends
    if (free_it) Arena_Free(r);
    scene_swap_out();
}

int companion_is(const Entity *e) { return e && e == coop.p2 && companion_active(); }

// player 2's pad while the companion walks: toward player 1 (both axes, so diagonals work)
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
