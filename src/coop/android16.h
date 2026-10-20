// Android 16 boss (android16.c): sprite 160, enemy stat 109, rocket attacks. Tuning knobs live here;
// docs/coop-design.md "Android 16 boss" explains each one and how they were tested.
#pragma once
#include "coop.h"

#define A16_STAT          109     // enemy-stat index (the stock table has 0..108); resolved to a16_stat
#ifndef A16_SPRITE
#define A16_SPRITE        160     // tools/a16sprites.py record (Makefile A16_SPRITE)
#endif

// Stat row (designed for two players at level 25-30; single-player gets A16_SOLO_HP_PCT of it).
// A16_HP is the HP the players take off him over the whole island fight; the fight ends when he is
// down to A16_END_PCT % of his nominal health (A16_HP * 100 / (100 - A16_END_PCT)): the defeat action
// fires and he stays standing (docs/story.md 4f: he survives for later). A boss bar drawn for him with
// a percent range (DrawBossHealthBarRange 160, lo, hi) gives that stage the slice [max(lo, END), hi]
// of the nominal health, so the island's two stages (75-100 in area 4, 50-75 in area 6) end at half.
#define A16_HP            4000
#define A16_END_PCT       50
#define A16_SOLO_HP_PCT   45
#define A16_MELEE         12      // stock 17: 32 melee, 39 ranged, 34 defence (tuned down: see docs)
#define A16_RANGED        28
#define A16_DEFENCE       38
#define A16_AI_SET        21      // 17's boss AI (stock melee, approach, dash) = the filler between rockets

// Special-attack pacing (frames, 60 per second)
#define A16_FIRST_DELAY   90      // after spawning
#define A16_COOLDOWN_MIN  150     // between specials, randomised up to MAX
#define A16_COOLDOWN_MAX  240
#define A16_RAGE_PCT      75      // cooldown percentage below a third of his HP
#define A16_RANGE         170     // a special starts only with a player this close (px)

// Heat-seeking rocket: locks onto the nearer player, detonates on contact or after the fuse
#define A16_HOMING_FUSE   180     // three seconds
#define A16_HOMING_SPEED  0x1A0   // 8.8 px/frame (1.63)
#define A16_HOMING_TURN   10      // steering divisor (lower = sharper)
#define A16_HOMING_RADIUS 20      // blast half-size (px)
#define A16_HOMING_MULT   0x400   // 10.10 damage multiplier on the ranged formula

// Barrage: a fan of missiles with weak homing and a short fuse
#define A16_BARRAGE_COUNT 5       // 7 below a third of his HP
#define A16_BARRAGE_SPREAD 0x60   // fan half-width (8.8 fraction of the aim vector)
#define A16_BARRAGE_SPEED 0x260
#define A16_BARRAGE_TURN  48
#define A16_BARRAGE_FUSE  80
#define A16_BARRAGE_RADIUS 16
#define A16_BARRAGE_MULT  0x300

// Area explosives: bombs dropped around both players and across the view
#define A16_BOMB_COUNT    5       // 7 below a third of his HP
#define A16_BOMB_FUSE     72      // after landing
#define A16_BOMB_DROP     96      // fall height (px)
#define A16_BOMB_RADIUS   28
#define A16_BOMB_MULT     0x400

// Rocket punch: the forearm flies straight along his facing
#define A16_PUNCH_SPEED   0x380
#define A16_PUNCH_RANGE   60      // frames of flight
#define A16_PUNCH_RADIUS  18
#define A16_PUNCH_MULT    0x400

// Poise: a hit that stuns him starts A16_POISE frames in which further hits only do damage (no stun,
// no knock-back), and he cannot be knocked out of a special; two players cannot stun-lock him.
#define A16_POISE         90

#define A16_LIVE          24      // live projectiles tracked per boss

Entity *a16_spawn(const EnemyRecord *rec);   // enemy_spawn_hook for stat A16_STAT
int is_a16(const Entity *e);
void a16_test_spawn(void);                    // COOP_CMD_A16: next to player 1, with a boss bar
int a16_threat(const Entity *pl, s32 *ex, s32 *ey);   // test autopilot: a blast about to hit pl? escape vector
int a16_bar_range(Entity *e, int lo, int hi);    // bossbar_create_hook: slice his HP to [lo, hi], returns the bar's lo
