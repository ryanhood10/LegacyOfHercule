// Dynamic wide view: the view is 240 x 160 while the players are close (or player 2 is absent, or
// player 1 is not in control), and grows up to 720 x 480 as they separate, shrinking back when they
// rejoin. The size is published for the mGBA fork each frame; see wide.h.
#include "coop.h"
#include "wide.h"

ROM_FN(int, Sprite_Emit, (Entity *e, u32 **cursor), 0x08007773);   // base entity draw: 2 OAM words

// Draw slot of every sprite class (13 vtables, see patch_rom.py --wide): after the original has
// written attr0/attr1, add x bit 9 and y bit 8 (OAM attr1 bits 9/10, read by the mGBA fork) for
// non-affine sprites, from the entity's full screen coordinates (+0x40/+0x44).
int sprite_emit_hook(Entity *e, u32 **cursor) {
#if WIDE_VIEW
    u32 *entry = *cursor;
    int r = Sprite_Emit(e, cursor);
    if (*cursor == entry + 2 && !(entry[0] & 0x100)) {
        s32 sx = *(s32 *)((u8 *)e + 0x40), sy = *(s32 *)((u8 *)e + 0x44);
        entry[0] |= (((u32)sx >> 9) & 1) << 25;
        entry[0] |= (((u32)sy >> 8) & 1) << 26;
    }
    return r;
#else
    return Sprite_Emit(e, cursor);
#endif
}


#if WIDE_VIEW
static int iabs(int v) { return v < 0 ? -v : v; }

void wide_update(void) {
    int target = WIDE_MIN_W;
    Entity *p1 = g_PlayerObject, *p2 = coop.p2;
    // a hit or knockback leaves the control states for a moment; only cutscene-length pauses
    // (over a second) pull the view back in
    if (p1 && p1_controllable()) wide.uncontrollable = 0;
    else if (wide.uncontrollable < 0xFFFF) wide.uncontrollable++;
    if (p1 && p2 && wide.uncontrollable < 60) {
        int dx = iabs((p1->x - p2->x) >> 8);
        int dy = iabs((p1->y - p2->y) >> 8);
        // both players plus a 72 px margin each side must fit; height follows at 2:3
        int need_w = dx + 144;
        int need_h = dy + 96;
        if (need_h * 3 / 2 > need_w) need_w = need_h * 3 / 2;
        if (need_w > target) target = need_w;
        if (target > WIDE_MAX_W) target = WIDE_MAX_W;
    }
#ifdef WIDE_TEST
    if (p1) target = WIDE_TEST;   // fixed size for renderer tests
#endif
    if (wide.w < WIDE_MIN_W) wide.w = WIDE_MIN_W;
    wide.target_w = (u16)target;
    // ease towards the target, 1/8 of the gap per frame but at least 4 px, snapping when close
    int diff = target - wide.w;
    int step = diff / 8;
    if (step == 0) step = diff > 0 ? 4 : (diff < 0 ? -4 : 0);
    if (iabs(step) > iabs(diff)) step = diff;
    wide.w = (u16)((wide.w + step) & ~3);
    if (wide.w < WIDE_MIN_W) wide.w = WIDE_MIN_W;
    if (wide.w > WIDE_MAX_W) wide.w = WIDE_MAX_W;
    wide.h = (u16)((wide.w * 2 / 3) & ~1);
    WIDE_DESC[0] = WIDE_MAGIC;
    WIDE_DESC[1] = (u32)wide.w | ((u32)wide.h << 16);
}
#else
void wide_update(void) {}
#endif
