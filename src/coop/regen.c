// Resting heals (Ryan, 2026-11-01, from the two-player session: "if you get away from enemies and you aren't hit,
// your health will regen after a certain amount of time to 2/3rds; the last 1/3rd the player will have to get the
// traditional way"). Each player's own slot: once REGEN_DELAY frames pass with no hit on that player and no enemy
// within REGEN_NEAR px, HP rises REGEN_PCT % of the maximum every REGEN_STEP frames until REGEN_CAP_NUM /
// REGEN_CAP_DEN of it; food, senzu beans and the stock 5 % trickle for the slots not being played are unchanged.
// Runs from the base class's update (ssj.c ssj_base_update), so a transformed player (the Super Saiyan class)
// never rests, as in the stock game. The timers live in our EWRAM (zeroed with it at a soft reset).
#include "coop.h"

#define REGEN_DELAY    300   // frames of quiet before the healing starts (5 s)
#define REGEN_STEP     20    // frames between ticks (3 ticks a second)
#define REGEN_PCT      1     // % of the maximum a tick: 0 to 2/3 in about 22 s
#define REGEN_NEAR     128   // px: an enemy this close (either axis) keeps a player from resting
#define REGEN_CAP_NUM  2
#define REGEN_CAP_DEN  3

static u16 quiet[2] __attribute__((section(".combo.rg")));   // frames since player k was last hit or had an enemy near

void regen_hit(int k) { quiet[k] = 0; }

static int enemy_near(const Entity *e) {
    const EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) {
        const Entity *o = l->items[i];
        if (!is_any_enemy(o)) continue;
        s32 dx = (o->x - e->x) >> 8, dy = (o->y - e->y) >> 8;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        if (dx < REGEN_NEAR && dy < REGEN_NEAR) return 1;
    }
    return 0;
}

// Every frame of the base class's update: e is the player's entity, c its slot, k 0 for player 1 and 1 for player 2.
void regen_tick(const Entity *e, CharacterEntry *c, int k) {
    if (quiet[k] < 0xFFFF) quiet[k]++;
    if (quiet[k] % REGEN_STEP) return;
    if (enemy_near(e)) { quiet[k] = 0; return; }
    if (quiet[k] < REGEN_DELAY) return;
    s32 max = (s16)c->max_hp, hp = (s16)c->hp, cap = max * REGEN_CAP_NUM / REGEN_CAP_DEN;
    if (hp <= 0 || hp >= cap) return;      // downed (revive.c), or the rest of the way is the player's
    s32 step = max * REGEN_PCT / 100;
    if (step < 1) step = 1;
    hp += step;
    c->hp = (u16)(hp > cap ? cap : hp);
}
