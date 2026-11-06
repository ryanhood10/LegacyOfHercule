// The Hyperbolic Time Chamber as a game mode (docs/time-chamber.md; section 12 is the state of the build and how to add a
// room). The open area (zone 15 area 18), its enemies by lights out, the rooms: chamber.c.
#pragma once
#include "coop.h"

#define CH_ZONE         15
#define CH_OPEN_AREA    18      // data/maps/chamber_open.json
#define CH_FLAG_LEVER0  504     // 504-509: a room's lever thrown, one light of the gate out (docs/ids.md 504-535)
#define CH_ROOMS        6
#define CH_SPOT_STAT    155     // a spot record's stat (chamber_open.json): the current tier picks the real one
#define CH_FLAG_RUN     510     // a run is under way: its opening scene has played (cleared when the players leave)
#define CH_FLAG_FIND0   531     // 531-535: the side rooms' finds taken (areas 27-31 in order: docs/time-chamber.md 13.6)
#define CH_FIND_AREA0   27      // zone 15 areas 27-31: the Saiyan ship's bedroom, its store room, Frieza's crystal room, his control
#define CH_FINDS        5       // room, the Saiyan ship's lounge

int  chamber_lights(void);                                     // how many of the six levers are thrown
// A spot record (enemy_spawn_hook): the real record in *out (one behaviour, at most 0x20 bytes) and the kill's EXP
void chamber_spot(const EnemyRecord *in, EnemyRecord *out, s32 *exp);
// tny_resolve_stat: while a room spawns this fighter, his rating (else 0) and his HP in percent
int  chamber_rating(int fighter, int *hp_pct);
void chamber_tick(void);                                       // every input poll (vmcall.c coop_call_tick)
void chamber_input(u16 *p1, u16 *p2);                          // input.c: the opening's skip (Start or B), its boxes closed
int  chamber_call(s32 op, s32 a, s32 b, s32 *push);            // CoopCall 35: 1 when a value is pushed
void chamber_test(u32 arg, u32 arg2);                          // test command 48 (docs/ids.md)
