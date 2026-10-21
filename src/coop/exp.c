// Replacement for the enemy-death EXP award (0x0800E4D2; entry patched with a long jump).
// The original scales the enemy's EXP to the active party slot's level and queues an event whose
// fire slot calls Party_AddExp(g_Party, active_slot, amount). This version credits the player who
// was acting when the enemy died (player 2 kills go to player 2's slot) and scales to that slot.
// Last hit (Ryan, 2026-10-20: whoever lands the final blow gets the EXP): the damage calculation (enemies.c) notes,
// per enemy, the player behind each hit, a projectile's owner (+0x1C) included; the award credits the player of
// the enemy's last noted hit. Without a note (a scripted kill), the acting player as before: the one updating, or
// while a non-player entity (a ki blast) updates, the player that entity targets.
#include "coop.h"

#define EXP_HITS 12        // enemies remembered at once (a round-robin; a death clears its entry)
typedef struct ExpHit { const Entity *enemy, *player; } ExpHit;
static ExpHit exp_hits[EXP_HITS] __attribute__((section(".party.xp")));
static u8 exp_hit_next __attribute__((section(".party.xp")));

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

// the player behind a hit on a non-player: the attacker itself, or the projectile's owner
void exp_note_hit(const Entity *defender, const Entity *attacker) {
    // player 1 as the update loop saw him: while a non-player entity updates, g_PlayerObject is the player it
    // targets (entities.c), which may be player 2
    const Entity *p1 = coop2.p1_frame, *p2 = coop.p2;
    if (!p2 || !p1 || !attacker || defender == p1 || defender == p2) return;
    const Entity *who = attacker;
    if (who != p1 && who != p2) who = *(const Entity *const *)((const u8 *)attacker + 0x1C);
    if (who != p1 && who != p2) return;
    for (int i = 0; i < EXP_HITS; i++)
        if (exp_hits[i].enemy == defender) { exp_hits[i].player = who; return; }
    exp_hits[exp_hit_next].enemy = defender;
    exp_hits[exp_hit_next].player = who;
    exp_hit_next = (u8)((exp_hit_next + 1) % EXP_HITS);
}

static int credited_slot(const Entity *enemy) {
    if (coop.p2) {
        for (int i = 0; i < EXP_HITS; i++) {
            if (exp_hits[i].enemy != enemy) continue;
            const Entity *who = exp_hits[i].player;
            exp_hits[i].enemy = 0;
            return who == coop.p2 ? coop.p2_slot : g_Party->active_slot;
        }
    }
    if (coop.p2 && coop.acting == coop.p2) return coop.p2_slot;
    return g_Party->active_slot;
}

void exp_award_hook(Entity *enemy) {
    s32 amount = *(const s32 *)enemy->behaviour;   // behaviour word 0 = base EXP
    if (amount == 0) return;
    int slot = credited_slot(enemy);
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
