// Replacement for Input_Poll (0x0801EEFC; its first 8 bytes are patched to jump here).
// Same behaviour for player 1, plus player 2's InputSystem filled from the selected source.
#include "coop.h"

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
    return (u16)~(REG_SIODATA32 & 0x3FF);
#endif
}

void input_poll_hook(InputSystem *in) {
    coop_init_once();
    u16 now = (u16)~REG_KEYINPUT;
    u16 p1 = now;
    u16 p2 = p2_source(now, &p1);

    poll_into(in, p1);
    if (in->pressed) g_LastInputFrame = g_FrameCounter;
    if ((~in->current & 0xF) == 0 && in->history_armed && (in->pressed & 0xF)) SoftReset();

    poll_into(&coop.input2, p2);
}
