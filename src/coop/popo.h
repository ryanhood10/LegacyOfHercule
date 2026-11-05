// The Time Chamber's hidden room and Mr. Popo (docs/time-chamber.md 5, 6.3 and 12.7): the room behind the gate (zone 15
// area 25), the scene, his fight, his reward, and the loop (decision 4: he lights the six lights again, every boss a step
// stronger). popo.c; the scripts are data/story/chamber_popo.lscript, the map data/maps/chamber_popo.json.
#pragma once
#include "coop.h"

#define POPO_AREA        25      // zone 15 area of the hidden room
#define POPO_STAT        156     // his stat (bosses.c: popo_stat, BOSS_NO_TWIN: two players get the lone boss's HP)
#define POPO_RATING      500     // on the tournament's rating curve (decision 9): 5200 HP, 40 / 42 / 42
#define CH_LOOP_STEP     50      // rating per time round, every boss of the chamber (Popo and the rooms)
#define CH_FLAG_SCENE    512     // the hidden room's first scene seen (docs/ids.md 510-535)
#define CH_FLAG_POPO     513     // Popo beaten at least once (his big first reward paid)
#define CH_FLAG_FIRST0   514     // 514-519: room n's first win paid (its +3 capsule; later rounds pay a Senzu)
#define CH_FLAG_ROUND0   520     // 520-521: times round (0-3, bit 0 / bit 1)
#define CH_FLAG_ROUND_WON 530    // Popo beaten in this time round: he stands and talks (the fight and the loop on A), no scene
#define CH_ROUND_MAX     3
#ifndef POPO_SPRITE
#define POPO_SPRITE      163     // tools/popoart.py (Makefile POPO_SPRITE)
#endif

int  chamber_round(void);                                      // how many times Popo started the chamber again (0-3)
int  popo_stat(EnemyStat *out);                                // resolve_stat for POPO_STAT (bosses.c)
void popo_tick(void);                                          // chamber_tick on the hidden room's map
int  popo_call(s32 op, s32 a, s32 b, s32 *push);               // CoopCall 36: 1 when a value is pushed
void popo_test(u32 arg, u32 arg2);                             // test command 49 (docs/ids.md)
int  chamber_cut_light(void);                                  // chamber.c: the light of the room whose lever runs
