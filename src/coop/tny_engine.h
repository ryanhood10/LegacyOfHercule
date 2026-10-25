// The tournament engine's hooks into the game's own code (docs/tournament.md "The engine"). The cross-piece API
// is tourney.h; these are the few calls other files of the build make into src/coop/tny_engine.c.
#pragma once
#include "tourney.h"
#include "bosses.h"

// The ring the matches are staged in: the grounds piece's src/coop/tny_ring.h when it is in the build, else the
// Hyperbolic Time Chamber (zone 15 area 2) as a stand-in: its walkable floor below the living quarters.
#if defined(__has_include)
#if __has_include("tny_ring.h")
#include "tny_ring.h"
#endif
#endif
#ifndef TNY_RING_ZONE
#define TNY_RING_ZONE       15
#define TNY_RING_AREA       2
#define TNY_RING_X0         40
#define TNY_RING_Y0         248
#define TNY_RING_X1         600
#define TNY_RING_Y1         448
#define TNY_RING_P1_X       264
#define TNY_RING_P1_Y       344
#define TNY_RING_P2_X       264
#define TNY_RING_P2_Y       392
#define TNY_RING_OPP_X      352
#define TNY_RING_OPP_Y      360
#define TNY_RING_CENTER_X   320
#define TNY_RING_CENTER_Y   360
#endif

// Engine flags (446-459, cleared at a new game): none taken yet; the global record (unlocks, wins, attempts,
// prizes paid) lives on the save chip, the tournament in progress in `.tny.eng`.

// Extra enemy stats 160-219: one per fighter as an opponent (stat = TNY_STAT0 + fighter id), resolved at the
// spawn from the fighter's power and AI set and the running tournament's tier (bosses.c resolve_stat / boss_rule)
#define TNY_STAT_COUNT      60
#define BOSS_FLAT           16      // bosses.h rule flag: no x1.8 "lone boss against two players" HP (enemies.c)
int tny_resolve_stat(int stat_index, EnemyStat *out);
const BossRule *tny_boss_rule(int stat_index);

int  tny_ring_active(void);                                     // a tournament runs and the players are in the ring
int  tny_ko_check(int k, s32 dmg);                              // duel.c / player.c: player k (0 / 1) takes a
                                                                // lethal hit in a tournament fight: 1 = he stays
                                                                // at 1 HP and is out (the caller trims the hit)
void tny_note_hit(const Entity *defender, const Entity *attacker);   // enemies.c damage calc: who hit an opponent
