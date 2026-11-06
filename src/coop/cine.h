// The cinematic helpers (docs/cutscenes.md section 5 "Cine"): the moves a cutscene's script steps cannot do well on
// their own, played by C inside the game's own command modal. One bridge call, `CoopCall 37, op, b, c`.
#pragma once
#include "coop.h"

// Body codes (the cast's and every `who` argument): a sprite id (>= 7: the first actor / NPC / enemy showing it, never
// a player), or
#define CINE_P1        0       // player 1's entity
#define CINE_P2        (-1)    // player 2's entity (nobody when it does not stand)
#define CINE_POINT     (-2)    // the point set by CINE_POINT_SET (a target only: nobody stands there)
#define CINE_NOBODY    (-3)
#define CINE_SLOT0     (-10)   // -10 - slot: the player who plays that party slot (nobody when neither does)

enum {
    CINE_END = 0,          // everything of ours ends: a held charge, the flash; every body held in a pose stands idle
    CINE_CAST = 1,         // b = body A, c = body B (the moves below are A's on B)
    CINE_POINT_SET = 2,    // b, c = x, y px: the point
    CINE_DASH_TO = 3,      // A vanishes and reappears at (b, c) px
    CINE_DASH_BY = 4,      // ... beside B: b = side (0 his own side, 1 left, 2 right, 3 above, 4 below, 5 behind B,
                           //     6 in front of B; + 16 no afterimage), c = px between their centres (0 = touching)
    CINE_STRIKE = 5,       // A hits B: b bits: 1 heavy (B is knocked back, lands, lies), 2 a kick, 4 B ends kneeling;
                           //     c = the knock-back in px (0 = 56)
    CINE_EXCHANGE = 6,     // A and B trade a flurry for b frames (0 = 120); c = 0 they break apart, 1 A lands the last
                           //     blow (heavy), 2 B does
    CINE_CHARGE = 7,       // A powers up for b frames (blocking); b = 0 holds it alongside the script until b = -1
                           //     ends it; c = level 0-2
    CINE_BLAST = 8,        // A fires at B: b = 0 one ki blast, 1 a volley, 2 a beam (a big blast without group 18),
                           //     3 a big blast; + 16 B blocks, + 32 B is thrown; c = shots of a volley (0 = 5) /
                           //     frames a beam is held (0 = 60)
    CINE_STRUGGLE = 9,     // A's and B's beams meet for b frames (0 = 180); c = 1 A wins, 2 B wins, 0 it blows up
    CINE_POSE = 10,        // b = 0 A / 1 B; c = 0 rise, 1 kneel, 2 fall and lie
    CINE_LOOK = 11,        // everyone on screen but b faces b; b faces c (a body, the point or nobody)
    CINE_LOOKAT = 12,      // everyone on screen but c faces b (a body or the point)
    CINE_CAM = 13,         // b = 1 the camera follows the cast during the moves, 0 it stays
    CINE_PAN = 14,         // the camera pans to b (a body or the point) at c px a frame (0 = 4), blocking
    CINE_JOLT = 15,        // a shake of b frames (0 = 10) and a white flash of strength c (0-3), not blocking
    CINE_HAS = 16,         // -> 1 when body b stands (pushes)
    CINE_HELD = 17,        // -> how many bodies are held in a pose of ours (pushes; tests)
    CINE_DEMO_OFF = 98,    // the demo's trigger is spent
    CINE_DEMO_COND = 99,   // -> 1 once test command 50 armed the demo (pushes): its trigger's condition
};

int  cine_call(s32 op, s32 b, s32 c, s32 *push);   // CoopCall 37: 1 when a value is pushed
void cine_tick(void);                               // every input poll (nothing while no charge is held, no flash decays)
void cine_test(u32 arg, u32 arg2);                  // test command 50 (docs/ids.md)
void camera_aim(s32 cx, s32 cy);                    // camera.c: one step of the follow camera toward a centre (px)
