// Replacement for the enemy-death EXP award (0x0800E4D2; entry patched with a long jump).
// The original scales the enemy's EXP to the active party slot's level and queues an event whose
// fire slot calls Party_AddExp(g_Party, active_slot, amount). This version credits the player who
// was acting when the enemy died (player 2 kills go to player 2's slot) and scales to that slot.
#include "coop.h"

#define EXP_EVENT_VTABLE 0x080245B0u   // original event: slot 2 = destroy, slot 3 = fire

typedef struct ExpEvent {
    u32 *vtable;    // +0  relative vtable
    s32 ticks;      // +4  frames until it fires (1 = next EntityList_Update)
    s32 amount;     // +8
    s32 slot;       // +C  (new) party slot credited
} ExpEvent;

static void exp_event_fire(ExpEvent *ev) {
    coop.exp_awards[coop.p2 && ev->slot == coop.p2_slot]++;
    Party_AddExp(g_Party, ev->slot, ev->amount);
}

void exp_init(void) {
    u32 base = (u32)coop.exp_vtable;
    const u32 *orig = (const u32 *)EXP_EVENT_VTABLE;
    for (int i = 0; i < 6; i++) coop.exp_vtable[i] = (EXP_EVENT_VTABLE + orig[i]) - base;
    coop.exp_vtable[3] = (u32)exp_event_fire - base;
}

static int credited_slot(void) {
    if (coop.p2 && coop.acting == coop.p2) return coop.p2_slot;
    return g_Party->active_slot;
}

void exp_award_hook(Entity *enemy) {
    s32 amount = *(const s32 *)enemy->behaviour;   // behaviour word 0 = base EXP
    if (amount == 0) return;
    int slot = credited_slot();
    if (amount > 5) {
        int level = Character_GetLevel(&g_Party->chars[slot]);
        if (level == 50) return;
        s32 span = Character_LevelExp(level + 1) - Character_LevelExp(level);
        s32 lo = span >> 7, hi = span >> 1;
        if (amount < lo) amount = lo;
        else if (amount > hi) amount = hi;
    }
    ExpEvent *ev = Arena_Alloc(sizeof *ev);
    if (!ev) return;
    ev->vtable = coop.exp_vtable;
    ev->ticks = 1;
    ev->amount = amount;
    ev->slot = slot;
    EntityList *list = g_EntityList;
    list->events[list->event_count++] = (u32 *)ev;
}
