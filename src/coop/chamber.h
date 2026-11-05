// The Hyperbolic Time Chamber as a game mode (docs/time-chamber.md; section 12 is the state of the build and how to add a
// room). The open area (zone 15 area 18), its enemies by lights out, the rooms: chamber.c.
#pragma once
#include "coop.h"

#define CH_ZONE         15
#define CH_OPEN_AREA    18      // data/maps/chamber_open.json
#define CH_FLAG_LEVER0  504     // 504-509: a room's lever thrown, one light of the gate out (docs/ids.md 504-535)
#define CH_ROOMS        6
#define CH_SPOT_STAT    155     // a spot record's stat (chamber_open.json): the current tier picks the real one

int  chamber_lights(void);                                     // how many of the six levers are thrown
// A spot record (enemy_spawn_hook): the real record in *out (one behaviour, at most 0x20 bytes) and the kill's EXP
void chamber_spot(const EnemyRecord *in, EnemyRecord *out, s32 *exp);
// tny_resolve_stat: while a room spawns this fighter, his rating (else 0) and his HP in percent
int  chamber_rating(int fighter, int *hp_pct);
void chamber_tick(void);                                       // every input poll (vmcall.c coop_call_tick)
int  chamber_call(s32 op, s32 a, s32 b, s32 *push);            // CoopCall 35: 1 when a value is pushed
void chamber_test(u32 arg, u32 arg2);                          // test command 48 (docs/ids.md)
