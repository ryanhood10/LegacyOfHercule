// Midpoint camera: replaces the camera-follow object's update (vtable 0x08025A6C slot 0, original
// 0x0800D9F9). Same smoothing and clamped setter as the original, aimed at the midpoint of the two
// players' bounding boxes when player 2 exists. With both players leashed to the view, neither can
// walk more than a screen away from the other.
#include "coop.h"
#include "wide.h"

ROM_FN(void, Entity_GetBounds, (s32 *rect, Entity *e), 0x0800793B);   // {x1, y1, x2, y2} in px
ROM_FN(int, Camera_Apply, (void *world, s32 *pos), 0x080062E9);        // clamps to the map, sets view
ROM_FN(void, World_SetView, (void *world, s32 *pos), 0x08006079);      // sets view origin + 240x160 cull rect
#define WORLD_CELL (*(s32 **)0x083ED658)                                // -> world object (0x03001090)

void camera_follow_hook(u8 *follow) {
    Entity *target = *(Entity **)(follow + 4);
    s32 r[4];
    Entity_GetBounds(r, target);
    s32 cx = (r[0] + r[2]) / 2, cy = (r[1] + r[3]) / 2;
    Entity *p2 = coop.p2;
    if (p2 && target == g_PlayerObject) {
        s32 q[4];
        Entity_GetBounds(q, p2);
        cx = (cx + (q[0] + q[2]) / 2) / 2;
        cy = (cy + (q[1] + q[3]) / 2) / 2;
    }
    s32 *world = WORLD_CELL;
    s32 curx = world[1], cury = world[2];   // view position in px << 10
    s32 pos[2];
    int w = wide_w(), h = wide_h();
    pos[0] = (((cx - w / 2) * 0x400 - curx) * 3 >> 6) + curx;
    pos[1] = (((cy - h / 2) * 0x400 - cury) * 9 >> 7) + cury;
    if (w == 240 && h == 160) {
        Camera_Apply(world, pos);
        return;
    }
    // the game's clamp (+0x5C/+0x5E = max scroll for 240 x 160) does not know the wide size
    s32 maxx = ((s32) * (u16 *)((u8 *)world + 0x5C) + 240 - w) << 10;
    s32 maxy = ((s32) * (u16 *)((u8 *)world + 0x5E) + 160 - h) << 10;
    if (maxx < 0) maxx = 0;
    if (maxy < 0) maxy = 0;
    if (pos[0] < 0) pos[0] = 0;
    if (pos[1] < 0) pos[1] = 0;
    if (pos[0] > maxx) pos[0] = maxx;
    if (pos[1] > maxy) pos[1] = maxy;
    World_SetView(world, pos);
    world[5] = world[3] + w;   // +0x14/+0x18: cull rect right/bottom (SetView baked 240/160)
    world[6] = world[4] + h;
}
