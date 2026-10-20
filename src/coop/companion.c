// Old Hercule walks with Trunks in the future prologue when nobody plays player 2 (Ryan, 2026-10-20: one player
// still sees him, docs/coop-design.md "One player"). In the prologue old Hercule is in the party on party slot 5
// with his old look (display row OLD_HERCULE_ROW, data/story/ch1_opening.json) and is player 2 in co-op. With no
// second controller (1 Player chosen, or no pad), player 2 is still spawned for him, its pad driven by a simple
// follower: walk toward player 1 while farther than COMPANION_NEAR px, and catch up by a jump when stuck far
// behind. His HUD panel is hidden. A real second controller (2 Players) takes him over; once the prologue drops him
// from the party (the present day) the companion ends and player 2 leaves (p2_drop_out).
#include "coop.h"
#include "krillin.h"

#define COMPANION_NEAR   28    // px: close enough, stand still
#define COMPANION_STUCK  90    // frames without getting closer while far ...
#define COMPANION_FAR    96    // ... than this many px: jump to player 1's side

static u16 stuck __attribute__((section(".party.cp")));
static s32 last_d2 __attribute__((section(".party.cp")));

int companion_active(void) {
    const CharacterEntry *c = &g_Party->chars[HERCULE_SLOT];
    if (!(c->flags & 1) || c->display_row != OLD_HERCULE_ROW || g_Party->active_slot == HERCULE_SLOT) return 0;
    return players_one() || !p2_pad_present();
}

// player 2's pad while the companion walks: toward player 1 (both axes, so diagonals work)
u16 companion_pad(void) {
    Entity *p1 = g_PlayerObject, *p2 = coop.p2;
    if (!p1 || !p2 || !p1_in_play()) { stuck = 0; return 0; }
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
