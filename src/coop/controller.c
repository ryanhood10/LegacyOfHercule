// Controller mode: this GBA becomes player 2's pad. Every frame it sends its buttons over the link
// cable (multiplayer mode, slave) and draws them on a bitmap screen. Entered from input_poll_hook
// when L+R are held shortly after boot; never returns.
#include "coop.h"

#define VRAM_BMP ((volatile u16 *)0x06000000)
#define RGB(r, g, b) ((u16)((r) | ((g) << 5) | ((b) << 10)))

static void fill_rect(int x, int y, int w, int h, u16 colour) {
    for (int j = 0; j < h; j++) {
        volatile u16 *row = VRAM_BMP + (y + j) * 240 + x;
        for (int i = 0; i < w; i++) row[i] = colour;
    }
}

static void wait_vblank(void) {
    while (REG_VCOUNT >= 160) { }
    while (REG_VCOUNT < 160) { }
}

static void draw_bits(u16 v, int y) {
    for (int b = 15; b >= 0; b--)
        fill_rect(140 + (15 - b) * 6, y, 5, 6, (v >> b) & 1 ? RGB(31, 31, 31) : RGB(6, 6, 12));
}

typedef struct { u16 key; u8 x, y, w, h; } PadBox;
static const PadBox boxes[] = {
    { KEY_UP,     40,  40, 16, 16 }, { KEY_DOWN,  40,  80, 16, 16 },
    { KEY_LEFT,   20,  60, 16, 16 }, { KEY_RIGHT, 60,  60, 16, 16 },
    { KEY_B,     160,  70, 20, 20 }, { KEY_A,    190,  55, 20, 20 },
    { KEY_SELECT, 95, 120, 20,  8 }, { KEY_START, 125, 120, 20, 8 },
    { KEY_L,      10,  10, 40, 10 }, { KEY_R,    190,  10, 40, 10 },
};

void controller_main(void) {
    REG_DISPCNT = 3 | 0x400;   // mode 3 bitmap, BG2 (interrupts stay as the game left them)

    const u16 bg = RGB(2, 2, 8), off = RGB(8, 8, 16), on = RGB(31, 24, 4), link = RGB(4, 28, 8), nolink = RGB(28, 6, 6);
    fill_rect(0, 0, 240, 160, bg);
    u16 prev = 0xFFFF, sent = 0;
    for (;;) {
        u16 raw = REG_KEYINPUT;
        u16 word = (u16)((raw & 0x3FF) | SIO_MARK);
        if (word != sent) { REG_SIOMLT_SEND = word; sent = word; }   // only touch the port on change
        wait_vblank();
        u16 held = (u16)(~raw & 0x3FF);
        if (held != prev) {
            for (unsigned i = 0; i < sizeof boxes / sizeof boxes[0]; i++)
                fill_rect(boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h, (held & boxes[i].key) ? on : off);
            prev = held;
        }
        // cable indicator: green once the host's marker word has arrived
        fill_rect(110, 145, 20, 8, SIO_VALID(REG_SIOMULTI0) ? link : nolink);
        // diagnostics: bit 15..0 of SIOCNT, SIOMULTI0, SIOMULTI1 as three rows of 16 boxes (left = bit 15)
        draw_bits(REG_SIOCNT, 134);
        draw_bits(REG_SIOMULTI0, 142);
        draw_bits(REG_SIOMULTI1, 150);
    }
}
