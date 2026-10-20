// Reviving a downed partner (docs/coop-design.md "Revive"): at 0 HP a player collapses and waits; the partner
// holds A beside the body for 3 s to get them back up with 30 % HP. The wait runs out after the difficulty's
// time (10 s on Normal), or both players go down: game over.
#ifndef REVIVE_H
#define REVIVE_H
#include "coop.h"

#define REVIVE_HOLD     180   // frames of A held beside the body
#define REVIVE_HP_PCT   30    // HP back, percent of the maximum
#define REVIVE_SAFE     90    // frames of invincibility after getting up
#define REVIVE_REACH    28    // px between the partner and the body (centre to centre)
#define REVIVE_MAGIC    0x5EB1BE00u

typedef struct ReviveState {
    u32 magic;
    Entity *body[2];      // player k's entity while it lies downed (0 = player 1, 1 = player 2)
    u16 left[2];          // frames left on its countdown
    u16 total[2];         // the countdown's length
    u16 hold[2];          // frames of revive done on body k
    u16 safe[2];          // player k's invincibility after getting up
    u16 tile;             // OBJ VRAM of the two bars (0 = none)
    u8 reaching[2];       // player k stands beside the other's body
    u8 a_held[2];         // ... and holds A (taken from the game's input)
    u8 over;              // the game over is queued
    u8 _pad;
    u16 glow_n;           // entries of the glow table built so far (256 = ready)
    u8 *glow;             // the glow table: a colour remap toward white (arena, while someone is down)
    u16 downs, revives, timeouts;   // counters for tests
} ReviveState;
extern ReviveState rv;

void revive_input(u16 *p1, u16 *p2);          // input.c: the revive's A, a downed player's pad
void revive_tick(void);                       // entities.c, once a frame before the entities
int revive_hit(Entity *e, u8 *hit, int k);    // the players' take-damage hooks: 1 = the hit is taken here
void revive_draw(u32 **oam);                  // hud.c: the countdown and revive bars
void revive_hud_gone(void);                   // hud.c: the HUD is destroyed (its VRAM goes with the map)
int revive_is_down(const Entity *e);
void revive_p2_up(void);                      // player 2 gets up now (a save-pad trade of its slot)
void revive_p2_gone(void);                    // player 2 leaves: a downed player 1 is a game over
int scene_running(void);                      // a dialogue sequence runs (0x03001FDC)
void revive_test_down(int k);                 // test command 10: player k goes down
int revive_p1_down(void);                     // player 1 lies downed and no scene runs: player 2 plays on

#endif
