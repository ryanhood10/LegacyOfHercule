// Player 2 picks up dropped food (Ryan, 2026-11-01, the two-player session: "p2 can't grab the health/food that's
// dropped on the ground when you break boulders, so he slowly lost health"). Decompiled: a dropped item whose record
// has no popup and is not counted (the drop scripts' food and senzu: SpawnItem 0x0800ABA4 -> the item entity
// 0x08011802, vtable 0x08024C90) registers a touch box (0x0801177C: {vtable 0x08023F18, s16 x1 - 15, s16 y1 - 7,
// s16 x2, s16 y2, the player object 0x030020BC, the item entity}, 0x14 bytes) in the list at 0x03001FE0 ({count,
// boxes...}, shared with the map's own trigger boxes of other classes). The stock scan fires a box whose rectangle
// holds player 1's position, moving or not, and never looks at player 2 (headless: the box moved onto player 2 alone
// did nothing for 120 frames; onto player 1 alone it was taken within 60). Firing is the class's slot 1 (0x080117C2:
// the item's slot 10 0x0801193E removes the entity and runs the item record's +0x34 pick-up handler, then the box
// leaves the list). Every input poll this tests player 2's position against the item boxes and fires one the same
// way, with the party's active slot and g_PlayerObject swapped to player 2's for the call, so the food lands on his
// character. One a frame: firing changes the list.
#include "coop.h"

#define TOUCH_LIST    ((s32 *)0x03001FE0)
#define ITEM_BOX_VT   0x08023F18u

typedef struct ItemBox { u32 vtable; s16 x1, y1, x2, y2; void *player; Entity *item; } ItemBox;
_Static_assert(sizeof(ItemBox) == 0x14, "ItemBox: the stock touch box");

ROM_FN(int, ItemBox_Fire, (ItemBox *b), 0x080117C3);

void p2_pickup_tick(void) {
    Entity *p2 = coop.p2;
    if (!p2 || companion_active()) return;
    s32 px = p2->x >> 8, py = p2->y >> 8;
    s32 n = TOUCH_LIST[0];
    if (n <= 0 || n > 64) return;
    for (s32 i = 0; i < n; i++) {
        ItemBox *b = (ItemBox *)TOUCH_LIST[1 + i];
        if (!b || b->vtable != ITEM_BOX_VT) continue;
        if (px < b->x1 || px > b->x2 || py < b->y1 || py > b->y2) continue;
        PartyState *p = g_Party;
        u8 slot = p->active_slot;
        Entity *p1 = g_PlayerObject;
        p->active_slot = coop.p2_slot;
        g_PlayerObject = p2;
        ItemBox_Fire(b);
        g_PlayerObject = p1;
        p->active_slot = slot;
        return;
    }
}
