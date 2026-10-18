// Script -> C bridge (docs/scripts.md "CoopCall"). Game action 59 (Deprecated_CharacterSpecialAttack,
// an assert no stock script calls) is repointed here by data/hercule.json: `CoopCall fn, a, b, c` pops four values and runs one of
// the functions below; the queries push one result, the rest push nothing (a value left on the stack
// at the end of a dialogue script entry would be read as the next entry's index).
//
//   1 DuelStart (slot, flags, end_seq)  player-vs-player duel (duel.c): slot = player 2's party slot, -1 =
//                                       a clone of player 1; flags = DUEL_*; end_seq = dialogue sequence
//                                       (lscript seq:NAME) run when the duel ends. Player 2 is spawned
//                                       first when it does not exist yet (the clone slot is made
//                                       available). Without a player 2 the duel is skipped: player 1
//                                       wins and end_seq runs at once.
//   2 DuelClear ()                      after the end scene: player 2's slot and colours back (duel_clear)
//   3 DuelWinner () -> 1 / 2            winner of the last duel (0 before any)
//   4 HerculeAwaken ()                  Super Hercule into Hercule's abilities (ssj.c; Buu phase)
//   5 P2Present () -> 0 / 1             is there a player 2 entity
//   6 P2Bind (slot)                     player 2 plays this party slot now
#include "coop.h"

void hercule_awaken(void);                 // ssj.c
int p2_bind_slot(int slot);                // player.c
void *memcpy(void *dst, const void *src, unsigned n);

#define DUEL_SLOT          (PARTY_SLOTS - 1)
#define DEATH_EVENT_VTABLE 0x080238FCu     // the enemy-death event: its fire slot runs action->func(action)
#define ACTION_RUN_DIALOG  0x080106B3u     // EnemyAction_RunDialog {func, sequence}: runs a dialogue modally

typedef struct ActionEvent {
    u32 *vtable;        // relative vtable
    s32 ticks;          // frames until it fires
    s32 x, y;           // copied to the "last death position" globals
    const u32 *action;  // {func, payload}
} ActionEvent;

static s32 vm_pop(s32 *st) { s32 n = st[0]; st[0] = n - 1; return st[n]; }
static void vm_push(s32 *st, s32 v) { s32 n = st[0] + 1; st[0] = n; st[n] = v; }

// Run a dialogue sequence the way an enemy's defeat dialogue starts: an event in the entity list's queue,
// fired at the next list update, outside whatever code noticed the duel's end.
static void run_sequence_soon(const void *seq) {
    if (!seq) return;
    coop2.call_action[0] = ACTION_RUN_DIALOG;
    coop2.call_action[1] = (u32)seq;
    ActionEvent *ev = Arena_Alloc(sizeof *ev);
    if (!ev) return;
    Entity *p1 = g_PlayerObject;
    ev->vtable = (u32 *)DEATH_EVENT_VTABLE;
    ev->ticks = 1;
    ev->x = p1 ? p1->x >> 8 : 0;
    ev->y = p1 ? p1->y >> 8 : 0;
    ev->action = coop2.call_action;
    EntityList *list = g_EntityList;
    list->events[list->event_count++] = (u32 *)ev;
}

static void call_duel_start(int slot, int flags, const void *end_seq) {
    coop2.call_end_seq = end_seq;
    if (!p2_present()) {                   // single-player: player 1 wins, the story goes on
        coop2.duel_winner = 1;
        coop2.call_state = 0;
        run_sequence_soon(end_seq);
        return;
    }
    if (slot < 0) {                        // the clone's slot exists before player 2 spawns into it
        memcpy(&g_Party->chars[DUEL_SLOT], &g_Party->chars[g_Party->active_slot], sizeof(CharacterEntry));
        g_Party->chars[DUEL_SLOT].flags |= 3;
    }
    coop2.call_slot = (s8)slot;
    coop2.call_flags = (u8)flags;
    coop2.call_state = 1;
    coop_call_tick();
}

void vm_coop_call(s32 *st) {
    s32 c = vm_pop(st), b = vm_pop(st), a = vm_pop(st), fn = vm_pop(st);
    switch (fn) {
    case 1: call_duel_start(a, b, (const void *)c); break;
    case 2: duel_clear(); coop2.call_state = 0; break;
    case 3: vm_push(st, coop2.duel_winner); break;
    case 4: hercule_awaken(); break;
    case 5: vm_push(st, coop.p2 != 0); break;
    case 6: if (coop.p2 && a >= 0 && a < PARTY_SLOTS) p2_bind_slot(a); break;
    default: (void)b; break;
    }
}

// Every input poll: start a pending duel once player 2 exists, run the end sequence once it is over.
void coop_call_tick(void) {
    if (coop2.call_state == 1 && coop.p2) {
        duel_start(coop2.call_slot, coop2.call_flags, -1);
        coop2.call_state = coop2.duel_active ? 2 : 0;
    } else if (coop2.call_state == 2 && !coop2.duel_active) {
        coop2.call_state = 0;
        run_sequence_soon(coop2.call_end_seq);
    }
}
