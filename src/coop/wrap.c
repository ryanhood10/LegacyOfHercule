// Run an original (ROM) player handler for player 2 by temporarily presenting player 2's input as
// the global input block. The handlers only read the block, so swapping its contents around the call
// is safe; the per-player "last direction" globals are swapped the same way.
#include "coop.h"

typedef void (*CtrlFn)(void *state, Entity *e);

static void call_as_player(Entity *e, void *state, CtrlFn fn) {
    if (e != coop.p2) { fn(state, e); return; }
    InputSystem saved = *g_InputSystem;
    u16 saved_dir = g_LastDir;
    u32 saved_frame = g_LastDirFrame;
    static InputSystem no_input;
    *g_InputSystem = p1_controllable() ? coop.input2 : no_input;
    g_LastDir = coop.p2_last_dir;
    g_LastDirFrame = coop.p2_last_dir_frame;
    fn(state, e);
    leash_p2(e);
    coop.p2_last_dir = g_LastDir;
    coop.p2_last_dir_frame = g_LastDirFrame;
    *g_InputSystem = saved;
    g_LastDir = saved_dir;
    g_LastDirFrame = saved_frame;
}

// dash / transformation charge (vtable 0x08025E20 slot 2)
void dash_update_hook(void *state, Entity *e) { call_as_player(e, state, (CtrlFn)0x080083C3); }
// control state 0x080251F4 slot 2
void ctrl_8277_hook(void *state, Entity *e) { call_as_player(e, state, (CtrlFn)0x08008277); }
// control state 0x08025D70 slot 5
void ctrl_85ad_hook(void *state, Entity *e) { call_as_player(e, state, (CtrlFn)0x080085AD); }

// The beam abilities' charge states (slot 2, the update) read B from the global block: Masenko, Big Bang
// and Burning Attack fire on B released, Kamehameha, Special Beam Cannon and Spirit Bomb keep charging
// while B is held. Unwrapped, player 2's beams followed player 1's B: the held ones stopped at once and
// the released ones charged until player 1 let go of B (and a character switch in that state crashed).
void masenko_update_hook(void *state, Entity *e)   { call_as_player(e, state, (CtrlFn)0x08014F89); }
void bigbang_update_hook(void *state, Entity *e)   { call_as_player(e, state, (CtrlFn)0x08015079); }
void burning_update_hook(void *state, Entity *e)   { call_as_player(e, state, (CtrlFn)0x08015177); }
void kame_update_hook(void *state, Entity *e)      { call_as_player(e, state, (CtrlFn)0x08015249); }
void sbc_update_hook(void *state, Entity *e)       { call_as_player(e, state, (CtrlFn)0x0801539B); }
void spiritbomb_update_hook(void *state, Entity *e) { call_as_player(e, state, (CtrlFn)0x080156B7); }
