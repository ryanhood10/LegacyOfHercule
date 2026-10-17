// Midpoint camera: replaces the camera-follow object's update (vtable 0x08025A6C slot 0, original
// 0x0800D9F9). Same smoothing and clamped setter as the original, aimed at the midpoint of the two
// players' bounding boxes when player 2 exists. With both players leashed to the view, neither can
// walk more than a screen away from the other.
#include "coop.h"

ROM_FN(void, Entity_GetBounds, (s32 *rect, Entity *e), 0x0800793B);   // {x1, y1, x2, y2} in px
ROM_FN(int, Camera_Apply, (void *world, s32 *pos), 0x080062E9);        // clamps to the map, sets view
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
    pos[0] = (((cx - 0x78) * 0x400 - curx) * 3 >> 6) + curx;
    pos[1] = (((cy - 0x54) * 0x400 - cury) * 9 >> 7) + cury;
    Camera_Apply(world, pos);
}
