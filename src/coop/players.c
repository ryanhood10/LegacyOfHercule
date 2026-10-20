// 1 Player / 2 Players (docs/coop-design.md "One player"): asked at every game start. The Select Game screen
// (party_save.c save_menu_load_hook, which every new game and every loaded save passes) arms the question; the
// first moment player 1 can move, a dialogue asks "How many players?" (a CoopCall 18 yes / no box: A one, B
// two). One player keeps player 2 out even with a second controller connected (p2_present() reads 0); two players
// is the pad's rule (player 2 is there while its controller sends input). Not saved: each session picks again.
// Before any Select Game (power-on, the debug menu's Map Test, the headless tests) nothing is asked and the pad
// decides, as before.
#include "coop.h"
#include "revive.h"

#define PLAYERS_MAGIC 0x91A7E250u

typedef struct PlayersState {
    u32 magic;            // set by the Select Game screen: partyram is not cleared at power-on
    u8 mode;              // 0 not asked yet, 1 one player, 2 two players
    u8 asking;            // the question is queued or showing
    u8 _pad[2];
    u32 action[2];        // the queued sequence's action object (coop_run_sequence: two live words)
} PlayersState;
static PlayersState players __attribute__((section(".party.ui")));

// The question, a dialogue sequence built here (entry: u16 next, u8 mode, then bytecode or u8 speaker + text;
// docs/scripts.md 2.2). CoopCall bytecode from tools/script_asm.py: PushMultiVarint [fn, a, b, c] + Step 59 + END.
static const u8 q_arm[] __attribute__((aligned(4))) = {
    1, 0, 0,                                                 // -> 1, script
    0x1D, 0x04, 0x24, 0x02, 0x00, 0x00, 0x02, 0x3B, 0x11,    // CoopCall 18, 1, 0, 0 (arm the yes / no)
};
static const struct { u8 head[4]; char text[52]; } q_text __attribute__((aligned(4))) = {
    { 2, 0, 1, 0 }, "@^How many players?  (A: 1 Player  B: 2 Players)",    // -> 2, text, narration
};
static const u8 q_take[] __attribute__((aligned(4))) = {
    3, 0, 0,                                                 // script: CoopCall 21 pushes the next entry (3 / 4)
    0x1D, 0x04, 0x2A, 0x00, 0x00, 0x00, 0x02, 0x3B, 0x11,    // CoopCall 21, 0, 0, 0
};
static const struct { u8 head[4]; char text[16]; } q_one __attribute__((aligned(4))) = {
    { 5, 0, 1, 0 }, "@^One player.",
};
static const struct { u8 head[4]; char text[64]; } q_two __attribute__((aligned(4))) = {
    { 5, 0, 1, 0 }, "@^Two players.  Player 2 joins when their controller is on.",
};
static const void *const q_seq[] = { q_arm, &q_text, q_take, &q_one, &q_two, 0 };

void players_reset(void) {
    players.magic = PLAYERS_MAGIC;
    players.mode = 0;
    players.asking = 0;
}

int players_one(void) { return players.magic == PLAYERS_MAGIC && players.mode == 1; }

// CoopCall 21: the answer (A one player, B two); pushes the entry that confirms it
int players_take(void) {
    int one = cg_choice_take() == 1;
    players.mode = one ? 1 : 2;
    players.asking = 0;
    return one ? 3 : 4;
}

// Every input poll: ask once player 1 is free to move after a Select Game
void players_tick(void) {
    if (players.magic != PLAYERS_MAGIC || players.mode || players.asking) return;
    if (!g_PlayerObject || !p1_in_play() || scene_running()) return;
    players.asking = 1;
    coop_run_sequence(players.action, q_seq);
}
