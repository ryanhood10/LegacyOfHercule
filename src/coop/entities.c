// Replacement for EntityList_Update (0x080069A4; entry patched with a long jump). Same work as the
// original, plus: while a non-player entity updates, g_PlayerObject points at whichever player is
// nearest to it, so enemy AI, attacks and hit tests target player 2 when they should.
#include "coop.h"

typedef struct EntityList {
    u32 *vtable;          // +0x000
    s32 count;            // +0x004
    Entity *items[0x182]; // +0x008 .. +0x610
    s32 event_count;      // +0x610
    u32 *events[];        // +0x614  objects: { relative vtable, s32 ticks }
} EntityList;
_Static_assert(__builtin_offsetof(EntityList, event_count) == 0x610, "EntityList layout");

static inline void call_slot(void *obj, int slot) {
    ((void (*)(void *))vslot(obj, slot))(obj);
}

static Entity *nearest_player(const Entity *e, Entity *p1, Entity *p2) {
    s32 dx1 = (e->x - p1->x) >> 8, dy1 = (e->y - p1->y) >> 8;
    s32 dx2 = (e->x - p2->x) >> 8, dy2 = (e->y - p2->y) >> 8;
    return (dx2 * dx2 + dy2 * dy2 < dx1 * dx1 + dy1 * dy1) ? p2 : p1;
}

void entity_list_update_hook(EntityList *list) {
    for (int i = list->event_count - 1; i >= 0; i--) {
        u32 *ev = list->events[i];
        if (--((s32 *)ev)[1] != 0) continue;
        int n = --list->event_count;
        for (int k = i; k < n; k++) list->events[k] = list->events[k + 1];
        call_slot(ev, 3);
        call_slot(ev, 2);
    }

    Entity *p1 = g_PlayerObject;
    Entity *p2 = coop.p2;
    for (int i = list->count - 1; i >= 0; i--) {
        Entity *e = list->items[i];
        if (p1 && p2 && e != p1 && e != p2) {
            g_PlayerObject = nearest_player(e, p1, p2);
            call_slot(e, 2);
            g_PlayerObject = p1;
        } else {
            call_slot(e, 2);
        }
    }
}
