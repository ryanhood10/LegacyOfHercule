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
// A pose group with a frame array in every facing: the draw (0x08007772) of a null frame reads its size
// from address 0 and DMAs that many bytes from the staging buffer into the entity's tile buffer.
static int has_group(const Entity *e, int g) {
    const u8 *rec = e->sprite_record;
    if (!rec || g >= rec[2]) return 0;
    const u32 *dirs = (const u32 *)(rec + 0x4C + g * 0x10);
    return dirs[0] && dirs[1] && dirs[2] && dirs[3];
}

// Control state 0x08025D70 slot 5, the end of an attack: the ROM's 0x080085AD sends message 0x27 (the
// special melee, pose group 16) while the state's +8 is set, A is held and the *active* party slot's
// flags have bit 2 (the special melee page), else message 0 (back to idle). Rewritten so player 2 reads
// its own slot and so a sprite without group 16 never gets it: player 2 Hercule (no group 16) beside a
// player 1 with the page played a null frame whose garbage size (0x5416 bytes) overran its 0x400-byte
// tile buffer, wiping both players and the HUD; the next draw jumped through a zeroed vtable and the
// game hung (seen first as the twin-Cell autopilot "stall", docs/coop-design.md "Super Android 13").
static void attack_end(void *state, Entity *e) {
    int slot = e == coop.p2 ? coop.p2_slot : g_Party->active_slot;
    int special = *(s32 *)((u8 *)state + 8) && (g_InputSystem->current & KEY_A) && slot < PARTY_SLOTS
                  && (g_Party->chars[slot].flags & 4) && has_group(e, 16);
    Entity_SetAnimation(e, special ? 0x27 : 0);
}
void ctrl_85ad_hook(void *state, Entity *e) { call_as_player(e, state, attack_end); }

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
