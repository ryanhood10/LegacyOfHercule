// The tournament's ring and grounds as the engine stages them (grounds piece, docs/tournament.md "The grounds").
// Maps: data/maps/papaya_*.json (zone 15 areas 11-16, Buu's Fury's zone 7 imported). All coordinates are map
// pixels (entity positions: x, y >> 8).
#pragma once

// The ring with the stands (Buu's Fury 7/6, 720 x 720): data/maps/papaya_ring.json.
#define TNY_RING_ZONE       15
#define TNY_RING_AREA       16
// The platform: the walkable floor, closed on every side by the map's collision (no ring-outs; the walkway
// from the stadium door ends one tile short of it, so nobody walks on or off: fighters are warped onto it).
// Walkable tiles x 232..487, y 256..511; an entity's position stops about 16-20 px short of a blocked edge.
#define TNY_RING_X0         232
#define TNY_RING_Y0         256
#define TNY_RING_X1         488     // exclusive
#define TNY_RING_Y1         512     // exclusive
// Entry points: the players on the left half facing right, the opponents on the right half facing left;
// "walking in from opposite sides" = place them here and walk them towards the centre.
#define TNY_RING_P1_X       280
#define TNY_RING_P1_Y       360
#define TNY_RING_P2_X       280     // player 2 (2v2, the Rumble; a co-op 1v1's other player waits here too)
#define TNY_RING_P2_Y       416
#define TNY_RING_OPP_X      440     // the opponent (a second opponent / Rumble entrants: step +56 in y)
#define TNY_RING_OPP_Y      384
#define TNY_RING_CENTER_X   360     // camera centre: the view's top-left is centre - (120, 80)
#define TNY_RING_CENTER_Y   384
#define TNY_RING_WARP_X     360     // where a warp into the ring map lands by default (the map's spawn)
#define TNY_RING_WARP_Y     384

// The fighters' waiting room (the hub: the director, the save pad, today's fighters): where the players go
// back after a tournament, in front of the director (he stands at (168, 328), the ring door is (96..144, 330..360)).
#define TNY_WAIT_ZONE       15
#define TNY_WAIT_AREA       15
#define TNY_WAIT_X          200
#define TNY_WAIT_Y          300

// The plaza (the world-map landing arrives on its beach) and the other grounds maps.
#define TNY_PLAZA_AREA      11
#define TNY_STREET_AREA     12
#define TNY_COURTYARD_AREA  13
#define TNY_INFIRMARY_AREA  14

// Story flags (grounds 441-445): 441 the first visit's arrival scene on the plaza has played.
#define TNY_FLAG_ARRIVED    441
