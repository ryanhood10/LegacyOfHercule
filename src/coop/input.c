// Replacement for Input_Poll (0x0801EEFC; its first 8 bytes are patched to jump here).
// Same behaviour for player 1, plus player 2's InputSystem filled from the selected source.
#include "coop.h"

#define CONTROLLER_WINDOW 600   // polls (~10 s) during which holding L+R turns this GBA into the P2 pad

void controller_main(void);  // never returns

static void poll_into(InputSystem *in, u16 now) {
    u16 changed = in->current ^ now;
    u16 pressed = now & changed;
    in->pressed = pressed;
    in->released = in->current & changed;
    in->current = now;
    if (pressed) {
        in->history[in->history_head++] = pressed;
        if (in->history_head >= 8) in->history_head = 0;
    }
}

// g_p2_remote (coop.h): an emulator-side script (tools/mgba/p2_input.lua) writes player 2's buttons
// there: bit 15 = valid, bits 0-9 = GBA keys active-high. When valid it overrides the link cable.

int p2_present(void) {
#if P2_MODE == P2_MODE_SIO
    return coop.source_age < P2_SOURCE_TIMEOUT;
#else
    return 1;
#endif
}

#if P2_MODE == P2_MODE_SIO
static u16 sio_poll(void) {
    u16 remote = g_p2_remote;
    if (remote & 0x8000) { coop.source_age = 0; return remote & 0x3FF; }
    if (!coop.sio_ready) {
        REG_RCNT = 0;
        REG_SIOCNT = SIOCNT_MULTI_115200;
        REG_SIOMLT_SEND = SIO_MARK;   // the host sends a marker too, so a controller can see us
        coop.sio_ready = 1;
    }
    u16 got = REG_SIOMULTI1;          // result of the transfer started last frame
    if (!(REG_SIOCNT & SIOCNT_START)) REG_SIOCNT = SIOCNT_MULTI_115200 | SIOCNT_START;
    coop.sio_last = got;
    if (SIO_VALID(got)) { coop.source_age = 0; return (u16)(~got & 0x3FF); }
    if (coop.source_age < 0xFFFF) coop.source_age++;
    return 0;
}
#endif

static u16 p2_source(u16 p1_now, u16 *p1_out) {
#if P2_MODE == P2_MODE_MIRROR
    (void)p1_out;
    u16 p2 = p1_now & ~(u16)(KEY_LEFT | KEY_RIGHT);
    if (p1_now & KEY_LEFT) p2 |= KEY_RIGHT;
    if (p1_now & KEY_RIGHT) p2 |= KEY_LEFT;
    return p2;
#elif P2_MODE == P2_MODE_SWAP
    u16 fresh = p1_now & ~coop.input2.current & ~g_InputSystem->current;
    if (fresh & KEY_SELECT) coop.control ^= 1;
    u16 quiet = p1_now & ~(u16)(KEY_DPAD | KEY_A | KEY_B);
    if (coop.control) { *p1_out = quiet; return p1_now; }
    return quiet;
#else
    (void)p1_now; (void)p1_out;
    return sio_poll();
#endif
}

void input_poll_hook(InputSystem *in) {
    coop_init_once();
    u16 raw = REG_KEYINPUT;
    // Lockstep netplay (tools/mgba/netplay.lua) feeds both pads from EWRAM so the two emulations see
    // identical input; the physical buttons of this machine then never reach the game directly.
    u16 p1_remote = g_p1_remote;
    if (p1_remote & 0x8000) raw = (u16)(~p1_remote & 0x3FF);
    // Every instance publishes its own buttons on the link; a host ignores what it receives from
    // itself, a second game instance thereby doubles as player 2's pad without controller mode.
    REG_SIOMLT_SEND = (u16)((raw & 0x3FF) | SIO_MARK);
#ifndef NO_CONTROLLER_MODE
    if (coop.polls < CONTROLLER_WINDOW) {
        coop.polls++;
        if ((~raw & (KEY_L | KEY_R)) == (KEY_L | KEY_R)) controller_main();
    }
#endif
    u16 now = (u16)~raw;
    u16 p1 = now;
    u16 p2 = p2_source(now, &p1);

    poll_into(in, p1);
    if (in->pressed) g_LastInputFrame = g_FrameCounter;
    if ((~in->current & 0xF) == 0 && in->history_armed && (in->pressed & 0xF)) SoftReset();

    poll_into(&coop.input2, p2);

    coop_active_check();
#if P2_MODE != P2_MODE_SWAP
    if (coop.p2 && (coop.input2.pressed & KEY_SELECT) && p1_controllable()) p2_cycle();
#endif
}
