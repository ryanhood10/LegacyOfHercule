#pragma once
#include "rom.h"

// How player 2's buttons are produced (compile-time for now).
#define P2_MODE_SIO    0   // link cable: the other GBA sends its KEYINPUT over serial
#define P2_MODE_MIRROR 1   // P1's input with left/right swapped (test mode, no second controller)
#define P2_MODE_SWAP   2   // Select toggles which player the single controller drives
#ifndef P2_MODE
#define P2_MODE P2_MODE_SIO
#endif

#define COOP_MAGIC 0xC00B0002u

typedef struct CoopState {
    u32 magic;
    Entity *p2;
    u32 p2_vtable[24];
    InputSystem input2;
    u8 p2_slot;        // party slot bound to player 2
    u8 control;        // SWAP mode: 0 = controller drives P1, 1 = drives P2
    u16 p2_last_dir;   // per-player copies of g_LastDir / g_LastDirFrame
    u32 p2_last_dir_frame;
    Hud hud2;          // player 2's panel state (vtable unused); tile == 0 means not allocated
    u16 polls;         // input polls since boot (controller-mode detection window)
    u8 sio_ready;      // serial registers initialised
    u8 _pad;
    u16 sio_last;      // last raw word received from the controller GBA
    u16 source_age;    // frames since a player-2 input source last reported (bridge or cable)
    u16 spawns, destroys;   // debug counters for player 2's lifetime
    Entity *acting;         // player responsible for the entity update in progress (exp.c)
    u32 exp_vtable[6];      // copy of the EXP event vtable with the fire slot replaced
    u16 exp_awards[2];      // debug counters: EXP events credited to [0] player 1, [1] player 2
} CoopState;
_Static_assert(__builtin_offsetof(CoopState, exp_awards) == 0xD8, "CoopState layout (peek.lua)");

#define P2_SOURCE_TIMEOUT 180   // frames without a source before player 2 is considered absent

int p2_present(void);        // a second pad is connected (or a test mode is compiled in)
int p1_controllable(void);   // player 1 is in a player-driven control state (not a cutscene)

#define HUD2_X_OFFSET 144  // panel art spans x = 8..88; place the second copy at 152..232

extern CoopState coop;             // pinned at 0x0203FF04 (section .coop)
extern volatile u16 g_p2_remote;   // pinned at 0x0203FF00: emulator-side player-2 pad word

void coop_init_once(void);
void exp_init(void);         // build the EXP event vtable copy (called from coop_init_once)
void leash_p2(Entity *e);   // keep player 2 inside the camera view (shared screen)
