#pragma once
#include "rom.h"

// Dynamic wide view (WIDE_VIEW=1 builds only; needs the mGBA fork in tools/mgba-wide). The ROM
// publishes the view size at 0x0203FEC0 ("WIDE", u16 w, u16 h) and keeps its own camera, cull
// rect, leash and HUD placement in step. Stock mGBA ignores the descriptor and shows 240 x 160.
#ifndef WIDE_VIEW
#define WIDE_VIEW 0
#endif

#define WIDE_DESC   ((volatile u32 *)0x0203FEC0)   // inside the pinned block below the co-op state
#define WIDE_MAGIC  0x45444957u
#define WIDE_MIN_W  240
#define WIDE_MAX_W  720

typedef struct WideState {
    u16 w, h;          // current view size in px (240 x 160 when off)
    u16 target_w;
    u16 uncontrollable;   // frames player 1 has not been in a player-driven state
} WideState;
#define wide (*(volatile WideState *)0x0203FEC8)   // right after the 8-byte descriptor, pinned block

void wide_update(void);        // once per frame (input poll): pick the target size, smooth, publish
static inline int wide_w(void) { return WIDE_VIEW ? wide.w : 240; }
static inline int wide_h(void) { return WIDE_VIEW ? wide.h : 160; }
