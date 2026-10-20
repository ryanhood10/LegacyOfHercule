// Replacement for EntityList_Update (0x080069A4; entry patched with a long jump). Same work as the
// original, plus: while a non-player entity updates, g_PlayerObject points at whichever player is
// nearest to it, so enemy AI, attacks and hit tests target player 2 when they should. coop.acting
// records the player responsible for the current update (for EXP crediting, see exp.c).
#include "coop.h"
#include "revive.h"

static inline void call_slot(void *obj, int slot) {
    ((void (*)(void *))vslot(obj, slot))(obj);
}

Entity *nearest_player(const Entity *e, Entity *p1, Entity *p2) {
    s32 dx1 = (e->x - p1->x) >> 8, dy1 = (e->y - p1->y) >> 8;
    s32 dx2 = (e->x - p2->x) >> 8, dy2 = (e->y - p2->y) >> 8;
    return (dx2 * dx2 + dy2 * dy2 < dx1 * dx1 + dy1 * dy1) ? p2 : p1;
}

void entity_list_update_hook(EntityList *list) {
    coop.acting = g_PlayerObject;
    coop2.p1_frame = g_PlayerObject;
    coop_debug_tick();
    revive_tick();
    actors_tick();
    for (int i = list->event_count - 1; i >= 0; i--) {
        u32 *ev = list->events[i];
        if (--((s32 *)ev)[1] != 0) continue;
        int n = --list->event_count;
        for (int k = i; k < n; k++) list->events[k] = list->events[k + 1];
        call_slot(ev, 3);
        call_slot(ev, 2);
    }

    for (int i = list->count - 1; i >= 0; i--) {
        // Re-read both players every entity: a cutscene or map change inside an update replaces
        // the player entity (and destroys player 2), and the old pointers must not be restored.
        Entity *p1 = g_PlayerObject;
        Entity *p2 = coop.p2;
        Entity *e = list->items[i];
        if (p1 && p2 && e != p1 && e != p2) {
            Entity *near = coop_target(e, p1, p2);
            g_PlayerObject = near;
            coop.acting = near;
            call_slot(e, 2);
            if (g_PlayerObject == near) g_PlayerObject = p1;
        } else {
            coop.acting = (p2 && e == p2) ? p2 : p1;
            call_slot(e, 2);
        }
        if (i > list->count) i = list->count;   // the list shrank under us
    }
    coop.acting = g_PlayerObject;
}
