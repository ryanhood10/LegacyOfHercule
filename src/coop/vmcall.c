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
//   5 P2Present () -> 0 / 1             is player 2 there (its entity, or its pad: the twins spawn by the pad)
//   6 P2Bind (slot)                     player 2 plays this party slot now (or at its respawn after a warp)
//   7 PadSwap ()                        the two pads trade players (coop2.pad_swap, as a duel won by player 2)
//   8 P2Slot () -> slot / -1            player 2's party slot (the last bound one while a warp has the entity
//                                       despawned), -1 without player 2
//  15 Solo (mode)                     the Cell Games (cellgames.c): 0 both players, 1 player 1 fights alone,
//                                       2 player 2 fights alone on player 1's entity (the twins stay away)
//  16 Beam (sprite, on)               the Cell Games: a Kamehameha held by that entity, the script does not wait;
//                                       on 0 ends it
//  18 Choice (op) [-> 1 / 2]          post-game: op 1 arms a yes / no question before its text entry (B closes
//                                       the box and answers no); op 0 after it pushes 1 yes / 2 no
//  13 WaveStart (seq, round)            Time Chamber training (waves.c): rounds from `round` (0/1 = the first),
//                                       seq = the round sequence shown before each round and after the last
//  14 Wave (op) -> ...                  op 0 -> the round (11 = completed); op 1 spawn the announced round
//                                       (no result); op 2 -> player 2's character name (a %s argument)
//   9 Stage (op, b, c)                  the Broly chapter's helpers: op 0 = b is the dialogue a BOSS_RAGE survivor
//                                       runs when it rages (enemies.c: Paragus falls first; kept in call_end_seq,
//                                       which only a duel also uses, and a duel start overwrites it); op 1 =
//                                       player 2 stands at (b, c) px, facing up (scene staging)
//  10 PartyKeep (op) [-> slot]          the Broly chapter's trip: op 0 keeps the party's selectable bits
//                                       (slots 0-7), Gohan's in-party bit and who player 1 / player 2 are in
//                                       story flags 404-419; op 1 puts the bits back and pushes player 1's
//                                       slot for a SetActiveCharacter step; op 2 (after that step) binds
//                                       player 2 to its kept slot again
//  11 A16Join ()                        "Gero's computer": 16 joins in the last party slot, the party locks to
//                                       Krillin + 16 (a16player.c; the script then sets player 1 and 2)
//  12 A16Leave ()                       16 leaves the party for good, the lock is undone, Hercule available
//  20 Ssj2Join ()                       Super Saiyan 2 Gohan joins (party slot 8, ssj2gohan.c) at Gohan's level;
//                                       the script then shows "Super Saiyan 2 Gohan is now an available
//                                       character!" (number proposed by the SSJ2 Gohan piece; the Cell Games
//                                       chapter calls it at the end of the Games)
//  17 Super13 (op, a, b) [-> state]     the post-game boss (super13.c): op 0 tunes the next spawn (a = HP %, b = blast
//                                       damage %, 0 = 100); op 1 pushes his state (0 none, 1 fighting, 2 exploding,
//                                       3 gone: wait for 3 after his defeat before the scene goes on); op 2 spawns him
//                                       beside player 1 with a boss bar
#include "coop.h"
#include "a16player.h"
#include "krillin.h"
#include "ssj2gohan.h"
#include "super13.h"

extern const char *const ui_strings[];   // relocated UI strings (krillin.c): UI_NAME + slot = a character's name

void hercule_awaken(void);                 // ssj.c
int p2_bind_slot(int slot);                // player.c
void *memcpy(void *dst, const void *src, unsigned n);

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
// fired at the next list update, outside whatever code noticed the duel's end. `action` (two words) must
// live until the event fires.
void coop_run_sequence(u32 *action, const void *seq) {
    if (!seq) return;
    action[0] = ACTION_RUN_DIALOG;
    action[1] = (u32)seq;
    ActionEvent *ev = Arena_Alloc(sizeof *ev);
    if (!ev) return;
    Entity *p1 = g_PlayerObject;
    ev->vtable = (u32 *)DEATH_EVENT_VTABLE;
    ev->ticks = 1;
    ev->x = p1 ? p1->x >> 8 : 0;
    ev->y = p1 ? p1->y >> 8 : 0;
    ev->action = action;
    EntityList *list = g_EntityList;
    list->events[list->event_count++] = (u32 *)ev;
}
static void run_sequence_soon(const void *seq) { coop_run_sequence(coop2.call_action, seq); }

// PartyKeep (CoopCall 10): story flags 404-411 = slots 0-7 selectable (party flags bit 1), 412-414 = player
// 1's slot, 415-418 = player 2's slot + 1 (0 = none), 419 = Gohan (slot 0) in the party (bit 0). Flags
// 400-419 are the Broly chapter's (docs/scripts.md 2.5). Slot 8 (Super Saiyan 2 Gohan) has no kept bit (the
// trip leaves it as it is) and player 1 on slot 8 comes back as slot 0 (three bits); player 2's four bits
// hold it.
#define KEEP_BITS 404
#define KEEP_P1 412
#define KEEP_P2 415
#define KEEP_GOHAN 419
void flag_set_hook(PartyState *p, u32 id);
void flag_clear_hook(PartyState *p, u32 id);
int flag_test_hook(PartyState *p, u32 id);
static void keep_flag(u32 id, int v) { if (v) flag_set_hook(g_Party, id); else flag_clear_hook(g_Party, id); }
static void keep_field(u32 id, int bits, int v) { for (int i = 0; i < bits; i++) keep_flag(id + i, (v >> i) & 1); }
static int kept_field(u32 id, int bits) { int v = 0; for (int i = 0; i < bits; i++) v |= flag_test_hook(g_Party, id + i) << i; return v; }

static void party_keep(s32 *st, int op) {
    int n = PARTY_SLOTS < 8 ? PARTY_SLOTS : 8;
    if (op == 0) {
        for (int s = 0; s < n; s++) keep_flag(KEEP_BITS + s, (g_Party->chars[s].flags >> 1) & 1);
        keep_field(KEEP_P1, 3, g_Party->active_slot);
        int p2 = (coop.p2 || p2_present()) ? coop.p2_slot + 1 : 0;
        keep_field(KEEP_P2, 4, p2 > 15 ? 0 : p2);
        keep_flag(KEEP_GOHAN, g_Party->chars[0].flags & 1);
        return;
    }
    for (int s = 0; s < n; s++) {
        if (flag_test_hook(g_Party, KEEP_BITS + s)) g_Party->chars[s].flags |= 2;
        else g_Party->chars[s].flags &= ~2;
    }
    if (op == 2) {
        int p2 = kept_field(KEEP_P2, 4);
        if (p2) { if (coop.p2) p2_bind_slot(p2 - 1); else coop.p2_slot = (u8)(p2 - 1); }
        return;
    }
    if (!flag_test_hook(g_Party, KEEP_GOHAN)) g_Party->chars[0].flags &= ~1;
    vm_push(st, kept_field(KEEP_P1, 3));
}

static void call_duel_start(int slot, int flags, const void *end_seq) {
    coop2.call_end_seq = end_seq;
    if (!p2_present() || companion_active()) {   // single-player (or a companion): player 1 wins, the story goes on
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
    case 5: vm_push(st, (coop.p2 != 0 || p2_present()) && !companion_active()); break;   // a companion: one player   // the pad: a warp despawns the entity until player 1 is free
    case 6:   // without an entity (a warp despawned it until player 1 is free again) the respawn picks the slot
        if (a >= 0 && a < PARTY_SLOTS) { if (coop.p2) p2_bind_slot(a); else coop.p2_slot = (u8)a; }
        break;
    case 7: coop2.pad_swap ^= 1; break;   // input.c swaps the pads while player 2's pad is present
    case 8: vm_push(st, (coop.p2 || p2_present()) ? (s32)coop.p2_slot : -1); break;
    case 15: cg_set_solo(a); break;       // the Cell Games' solo fights (cellgames.c)
    case 16: cg_beam(a, b); break;
    case 18: if (a) cg_choice_arm(); else vm_push(st, cg_choice_take()); break;   // yes / no (cellgames.c)
    case 13: wave_start((const void *)a, b); break;
    case 14:
        if (a == 0) vm_push(st, wave_round());
        else if (a == 1) wave_spawn();
        else if (a == 2) {                 // player 2's name; alone (or no valid slot) player 1's
            int s = (coop.p2 || p2_present()) && coop.p2_slot < PARTY_SLOTS ? coop.p2_slot : g_Party->active_slot;
            vm_push(st, (s32)ui_strings[UI_NAME + s]);
        } else if (a == 3) vm_push(st, (s32)cg_card(b));   // the Cell Games' title card (cellgames.c)
        break;
    case 9:
        if (a == 0) coop2.call_end_seq = (const void *)b;
        else if (a == 1 && coop.p2) { coop.p2->x = b << 8; coop.p2->y = c << 8; coop.p2->facing = 1; }
        break;
    case 10: party_keep(st, a); break;
    case 11: a16_join(); break;
    case 12: a16_leave(); break;
#if PARTY_SLOTS > SSJ2_SLOT
    case 20: ssj2_join(); break;
#endif
    case 21: vm_push(st, players_take()); break;   // the 1 / 2 players answer (players.c): pushes the next entry
    case 17:
        if (a == 1) vm_push(st, s13_call(1, 0, 0));
        else s13_call(a, b, c);
        break;
    default: break;
    }
}

// Every input poll: start a pending duel once player 2 exists, run the end sequence once it is over.
void coop_call_tick(void) {
    if (coop2.call_state == 1 && coop.p2) {
        duel_start(coop2.call_slot, coop2.call_flags, -1);
        coop2.call_state = coop2.duel_active ? 2 : 1;
        if (coop2.call_state == 1) coop2.call_state = 3;   // no duel after all: skip it below
    }
    if ((coop2.call_state == 1 && !p2_coming()) || coop2.call_state == 3) {   // player 2 left before the duel
        coop2.duel_winner = 1;
        coop2.call_state = 0;
        run_sequence_soon(coop2.call_end_seq);
    } else if (coop2.call_state == 2 && !coop2.duel_active) {
        coop2.call_state = 0;
        run_sequence_soon(coop2.call_end_seq);
    }
    waves_tick();
}
