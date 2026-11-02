// What a character gets back with time (Ryan, 2026-11-01 and 2026-11-02; docs/perfect-pass.md 11.1, 11.2). The base
// classes' update (ssj_base_update) comes here once per player entity.
//
// Resting heals ("if you get away from enemies and you aren't hit, your health will regen after a certain amount of
// time to 2/3rds; the last 1/3rd the player will have to get the traditional way"). Each player's own slot: once
// REGEN_DELAY frames pass with no hit on that player and no enemy within REGEN_NEAR px, HP rises REGEN_PCT % of the
// maximum every REGEN_STEP frames until REGEN_CAP_NUM / REGEN_CAP_DEN of it.
//
// Ki ("it regenerates only up to 1/3rd ... independent from the health mechanic: we can take damage, hit enemies, be
// near enemies"; "start it after 30 s but have it take 40 s to reach 1/3"): KI_DELAY frames with no ki move by that
// player, then EP rises at a rate that takes KI_RISE frames from empty to 1 / KI_CAP_DEN of the maximum. Never lowers EP.
//
// The stock trickle (the base class's update, stock 0x08017028: every 596 frames or so 5 % HP and EP): once a frame,
// not per player, and only for the characters nobody is playing, who keep the stock game's heal to full ("yes keep
// stock full"). The party meter's decrement goes with it: per player it halved the Super Saiyan cool-down. The stock's
// 3 % EP crumb for the character being played is gone (REGEN_PLAYED_CRUMB brings it back); "We do not have ki
// regeneration".
//
// A transformed player runs another class and gets none of this, as in the stock game. No resting heal and no ki rule
// in a duel or the tournament's ring: a fight must not outlast its ki and HP. The timers live in our EWRAM (zeroed with
// it at a soft reset).
#include "coop.h"
#include "tny_engine.h"

#define REGEN_DELAY    300   // frames of quiet before the healing starts (5 s)
#define REGEN_STEP     20    // frames between ticks (3 ticks a second)
#define REGEN_PCT      1     // % of the maximum a tick: 0 to 2/3 in about 22 s
#define REGEN_NEAR     128   // px: an enemy this close (either axis) keeps a player from resting
#define REGEN_CAP_NUM  2
#define REGEN_CAP_DEN  3

#define KI_DELAY       1800  // frames with no ki move before the ki starts to rise (30 s)
#define KI_RISE        2400  // frames from empty to the cap (40 s)
#define KI_CAP_DEN     3     // the cap: 1/3 of the maximum

#define TRICKLE_STEP   0x6E  // stock: 16.16 accumulator at the entity's +0x184, an overflow about every 596 frames
#define TRICKLE_RATE   0xCCD // stock: 5 % (16.16) of HP and EP an overflow, for the characters nobody plays
#define CRUMB_RATE     0x7AE // stock: 3 % of EP an overflow, for the character being played
#define REGEN_PLAYED_CRUMB 0 // 1: the played characters get the crumb again (Ryan did not answer; the lead's call, reversible)

#define CLASS_BASE     0x030021E0u   // the standard class
#define CLASS_SSJ2     0x030021E4u   // the Super Saiyan 2 class (rows 4 and 28): same rules, its stock update is replaced

// .combo.rg (docs/ids.md: at most 0x10 bytes)
static u16 quiet[2] __attribute__((section(".combo.rg")));     // frames since player k was last hit or had an enemy near
static u16 ki_idle[2] __attribute__((section(".combo.rg")));   // frames since player k's last ki move, up to KI_DELAY
static u16 ki_frac[2] __attribute__((section(".combo.rg")));   // the rise's fraction of an EP, in 1 / KI_RISE

void regen_hit(int k) { quiet[k] = 0; }

// Player k spent ki on a move (player.c use_ability), or a form began or ended (ssj.c apply). A hit does not count.
void regen_ki_used(int k) { ki_idle[k] = 0; ki_frac[k] = 0; }

// Test command 44: both ki timers at `frames` (0: as if both had just used a ki move).
void regen_test(u32 frames) {
    for (int k = 0; k < 2; k++) { ki_idle[k] = (u16)(frames > KI_DELAY ? KI_DELAY : frames); ki_frac[k] = 0; }
}

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

static void ki_tick(CharacterEntry *c, int k) {
    if (ki_idle[k] < KI_DELAY) { ki_idle[k]++; return; }
    s32 cap = (s16)c->max_ep / KI_CAP_DEN, ep = (s16)c->ep;
    if (ep >= cap) { ki_frac[k] = 0; return; }   // never lowers, never pushes past the cap
    ki_frac[k] = (u16)(ki_frac[k] + cap);
    if (ki_frac[k] < KI_RISE) return;
    ki_frac[k] = (u16)(ki_frac[k] - KI_RISE);
    c->ep = (u16)(ep + 1);
}

// e is the player's entity, c its slot, k 0 for player 1 and 1 for player 2.
static void regen_tick(const Entity *e, CharacterEntry *c, int k) {
    if (coop2.duel_active || tny_ring_active()) return;
    if ((s16)c->hp > 0) ki_tick(c, k);     // downed (revive.c): nothing until he is up
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

// a slot's row is on a class that comes here (not a form's)
static int rests(int slot) {
    u32 cell = *(const u32 *)(DISPLAY_DATA + g_Party->chars[slot].display_row * 0x14 + 0x10);
    return cell == CLASS_BASE || cell == CLASS_SSJ2;
}

// Player 2 is somebody's character: on the map, or a pad waiting for the next map to build him. The companion (the
// prologue alone, companion.c) is nobody's: his slot is benched.
static int p2_plays(int slot) {
    if (companion_active() || slot != coop.p2_slot) return 0;
    return coop.p2 || (p2_present() && !coop_solo());
}

// No trickle for a slot on a transformed row (a form's class runs its drain).
static void trickle(CharacterEntry *c, int ep_rate, int hp_rate) {
    if (DISPLAY_DATA[c->display_row * 0x14] & 1) return;
    s32 ep = (s16)c->ep + (((s16)c->max_ep * ep_rate) >> 16);
    c->ep = (u16)(ep > (s16)c->max_ep ? (s16)c->max_ep : ep);
    if (!hp_rate) return;
    s32 hp = (s16)c->hp + (((s16)c->max_hp * hp_rate) >> 16);
    c->hp = (u16)(hp > (s16)c->max_hp ? (s16)c->max_hp : hp);
}

// Once a frame. Slots 6 and 8 (Krillin, Super Saiyan 2 Gohan) are characters too when they are in the party; the stock
// loop stops at slot 5. Slot 7 is the duel clone's and Android 16's, borrowed: not healed on the bench.
static void party_frame(Entity *e) {
    PartyState *p = g_Party;
    if (p->meter) p->meter--;
    u32 *acc = (u32 *)((u8 *)e + 0x184);
    *acc += TRICKLE_STEP;
    if (*acc >> 16) {
        for (int i = 0; i < PARTY_SLOTS; i++) {
            CharacterEntry *c = &p->chars[i];
            if (i == p->active_slot || p2_plays(i)) {
                if (REGEN_PLAYED_CRUMB) trickle(c, CRUMB_RATE, 0);
                continue;
            }
            if (i >= PARTY_SLOTS_STOCK && (i == DUEL_SLOT || !(c->flags & 1))) continue;
            trickle(c, TRICKLE_RATE, TRICKLE_RATE);
        }
    }
    *acc &= 0xFFFF;
}

// The base classes' update, once per player entity. The shared part runs from player 1's entity, or from player 2's
// while player 1 is on a form's class (which has none of this: both on forms, no decay and no trickle, as stock alone).
void regen_update(Entity *e) {
    PartyState *p = g_Party;
    int is_p2 = coop.p2 && e == coop.p2;
    if (!is_p2 || !rests(p->active_slot)) party_frame(e);
    if (is_p2) { if (!companion_active()) regen_tick(e, &p->chars[coop.p2_slot], 1); }
    else regen_tick(e, &p->chars[p->active_slot], 0);
}
