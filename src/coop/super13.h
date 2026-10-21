// Super Android 13 (super13.c): the post-game boss (docs/story.md 4i). Sprite 180, enemy stat 145, slow,
// towering, ki-blast combinations with partly homing blasts, explodes when beaten. Tuning knobs live here;
// docs/coop-design.md "Super Android 13" explains each one and how it was tested.
#pragma once
#include "coop.h"

#define S13_STAT          145     // enemy-stat index (docs/ids.md: 145-149 are this boss's); resolved to s13_stat
#ifndef S13_SPRITE
#define S13_SPRITE        180     // tools/super13sprites.py record (Makefile S13_SPRITE)
#endif

// Stat row. The yardstick (docs/story.md 4i: "a little above the twin-Cell fight"): the Cell Games' final
// Perfect Cell (stat 25: 8230 HP, 60 / 59 / 65) fought as a co-op pair = 16460 HP and two attackers, but the
// stock Cells have no poise, so two players stun-lock them (the headless autopilot kills the pair in 22-31 s
// at full HP when it locks them). Super 13 cannot be locked and keeps firing, so his HP is lower: tuned
// headless until the Cell Games-level autopilot pair loses to him narrowly (KO with him at 4-35 % left) where
// it beats the locked Cells (docs/coop-design.md "Super Android 13" has the runs). CoopCall 17 op 0 scales
// HP and blast damage per fight. Single-player gets S13_SOLO_HP_PCT of the HP. Ryan (2026-10-20, after playing
// him alone at 5500 HP): more health, a big towering boss: 10000 -> 14000, alone 55 % -> 70 % (9800, Cell's 8230).
#define S13_HP            14000
#define S13_SOLO_HP_PCT   70
#define S13_MELEE         48
#define S13_RANGED        64
#define S13_DEFENCE       56
#define S13_AI_SET        21      // the melee boss AI (Broly's, Cooler's): approach, punches, dash; the filler
#define S13_SPEED_PCT     55      // his walk / dash speed in percent of the stock chase modules (enemies.c)

// Special-attack pacing (frames, 60 per second)
#define S13_FIRST_DELAY   80
#define S13_COOLDOWN_MIN  130      // between specials (or combos), randomised up to MAX
#define S13_COOLDOWN_MAX  200
#define S13_RAGE_PCT      70      // cooldown percentage once enraged (below a third of his HP)
#define S13_RANGE         230     // a special starts only with a player this close (px, his body centre)
#define S13_CROWD         60      // both players this close: the spinning ki nova is likely
#define S13_RAGE_AT       34      // fight percent below which he roars once and rages

// Ki volley: alternate fists, one small orb at a time, alternately at each player, partly homing
#define S13_VOLLEY_COUNT  4       // 6 enraged
#define S13_VOLLEY_GAP    10
#define S13_VOLLEY_SPEED  0x1E0   // 8.8 px / frame
#define S13_VOLLEY_TURN   26      // steering divisor (lower = sharper)
#define S13_VOLLEY_FUSE   120
#define S13_VOLLEY_RADIUS 12
#define S13_VOLLEY_MULT   0x108   // 10.10 multiplier on the ranged damage formula

// Ki fan: a spread of small orbs at one player, weak homing
#define S13_FAN_COUNT     5       // 7 enraged
#define S13_FAN_SPREAD    0x70
#define S13_FAN_SPEED     0x220
#define S13_FAN_TURN      60
#define S13_FAN_FUSE      100
#define S13_FAN_RADIUS    12
#define S13_FAN_MULT      0xCC 

// Deadly bomber: a giant orb grows over his head and drifts after a player (slow, homing, big blast)
#define S13_BOMB_CHARGE   56      // frames with the arms up while the ball grows
#define S13_BOMB_SPEED    0x0C0
#define S13_BOMB_TURN     18
#define S13_BOMB_FUSE     330
#define S13_BOMB_RADIUS   38
#define S13_BOMB_MULT     0x270

// Ki wave: a crouching power-up, then a slow beam head that partly homes and leaves a trail of ki
#define S13_WAVE_CHARGE   48
#define S13_WAVE_SPEED    0x120
#define S13_WAVE_TURN     30
#define S13_WAVE_FUSE     210
#define S13_WAVE_RADIUS   20
#define S13_WAVE_MULT     0x198
#define S13_TRAIL_GAP     7       // frames between trail orbs
#define S13_TRAIL_LIFE    44
#define S13_TRAIL_RADIUS  9
#define S13_TRAIL_MULT    0xA8 

// Ki nova: he spins in a ki shell, a close blast all round him and a ring of orbs flying out
#define S13_NOVA_RING     8
#define S13_NOVA_SPEED    0x1A0
#define S13_NOVA_FUSE     70
#define S13_NOVA_RADIUS   46      // the close blast round his centre
#define S13_NOVA_MULT     0x150
#define S13_ORB_MULT      0xCC    // each ring orb

// Poise: a hit that stuns him starts S13_POISE frames in which hits only do damage; specials cannot be
// interrupted (as Android 16)
#define S13_POISE         60 

// The death: light bursts out of him (pose group 12), explosions all over, then one big blast and he is gone
#define S13_DEATH_FRAMES  96
#define S13_LIVE          28      // live projectiles tracked

// Pose groups of sprite 180 (tools/super13sprites.py)
#define S13_G_DEATH       12
#define S13_G_KI          21      // ki thrusts (3 frames: wind-up, left fist, right fist)
#define S13_G_BOMB        22      // hunch, arms up, throw
#define S13_G_POWER       23      // crouching power-up (6 frames)
#define S13_G_ROAR        24      // the roar (6 frames)
#define S13_G_NOVA        25      // the spinning shell (8 frames)
#define S13_G_BLANK       26      // empty: he has exploded

// Live state (.party.s13 in partyram, <= 0x40 bytes; docs/ids.md "Memory")
typedef struct S13Global {
    u32 magic;              // S13_MAGIC once initialised (first, so a test can find the block by it)
    Entity *boss;           // the live boss, 0 when none
    u16 fired, booms;       // debug: projectiles launched, explosion sprites
    u16 hits, skipped;      // debug: blasts that hit a player, projectiles skipped (OBJ VRAM / heap)
    u16 specials, combos;   // debug: specials started, of which chained
    u8 force;               // nonzero = every special is this attack (S13_ATK_*; test command 18)
    u8 state;               // 0 none, 1 fighting, 2 exploding, 3 gone (CoopCall 17 op 1 reads it)
    u8 hp_pct, dmg_pct;     // CoopCall 17 op 0: HP and blast damage in percent for the next spawn (0 = 100)
    u32 spawn_frame, dead_frame, gone_frame;   // debug: g_FrameCounter at spawn / HP 0 / explosion over
    u16 msgs, landed;       // debug: player attack messages received / that hit him
    u16 vram_min, _pad;     // debug: the least largest-free OBJ VRAM block seen while he fought (bytes)
} S13Global;
extern S13Global s13;
enum { S13_ATK_NONE, S13_ATK_VOLLEY, S13_ATK_FAN, S13_ATK_BOMB, S13_ATK_WAVE, S13_ATK_NOVA, S13_ATK_ROAR };

Entity *s13_spawn(const EnemyRecord *rec);    // enemy_spawn_hook for stat S13_STAT
int is_s13(const Entity *e);
void s13_test_spawn(void);                    // test command 17 / CoopCall 17 op 2: beside player 1, with a boss bar
void s13_test_drive(u32 arg, u32 arg2);       // test command 18
int s13_threat(const Entity *pl, s32 *ex, s32 *ey);   // test autopilot: a blast about to hit pl? escape vector
s32 s13_call(int op, int a, int b);           // CoopCall 17
