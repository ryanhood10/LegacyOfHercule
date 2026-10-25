// Weak defaults for the tournament team's cross-piece functions (tourney.h): main builds and runs with any
// subset of the pieces merged. A piece defines the strong version in its own file; never edit these to add
// behaviour.
#include "tourney.h"

#define WEAK __attribute__((weak))

WEAK int  tny_engine_call(int fn, s32 a, s32 b, s32 c, s32 *push) { (void)fn; (void)a; (void)b; (void)c; (void)push; return 0; }
WEAK void tny_engine_test(u32 cmd, u32 arg, u32 arg2) { (void)cmd; (void)arg; (void)arg2; }
WEAK void tny_engine_frame(void) {}
WEAK int  tny_tier_open(int tier) { return tier == TIER_ROOKIE; }
WEAK int  tny_in_pool(int id) { (void)id; return 0; }
WEAK int  tny_unlocked(int id) { (void)id; return 0; }
WEAK int  tny_challenge_tried(int id) { (void)id; return 0; }
WEAK void tny_notify(const char *line) { (void)line; }

WEAK int  tny_ui_call(int fn, s32 a, s32 b, s32 c, s32 *push) { (void)fn; (void)a; (void)b; (void)c; (void)push; return 0; }
WEAK void tny_ui_test(u32 cmd, u32 arg, u32 arg2) { (void)cmd; (void)arg; (void)arg2; }
WEAK void tny_ui_frame(void) {}
WEAK void tny_popup_show(const char *line) { (void)line; }
// Without the ui piece: the first line, the lead (player 0) or player 2's own character, no bracket screens.
WEAK int  tny_ui_choose(const char *title, const char *const *lines, int n) { (void)title; (void)lines; return n > 0 ? 0 : -1; }
WEAK int  tny_ui_select(int player, int tier, u8 *pick) {
    (void)tier;
    static const u8 by_slot[9] = { TF_GOHAN, TF_PICCOLO, TF_VEGETA, TF_TRUNKS, TF_GOKU, TF_P_HERCULE, TF_KRILLIN, TF_P_A16, TF_SSJ2_GOHAN };
    int slot = player == 0 ? g_Party->active_slot : coop.p2_slot;
    *pick = slot >= 0 && slot < 9 ? by_slot[slot] : TF_GOHAN;
    return 1;
}
WEAK void tny_ui_bracket(int phase) { (void)phase; }
WEAK void tny_ui_rumble_board(const u8 *ids, const u8 *down, int n) { (void)ids; (void)down; (void)n; }
WEAK const TBracket *tny_bracket(void) { return 0; }

WEAK void tny_moves_test(u32 cmd, u32 arg, u32 arg2) { (void)cmd; (void)arg; (void)arg2; }
WEAK void tny_moves_frame(void) {}
WEAK void tny_moves_attach(Entity *e, int fighter_id) { (void)e; (void)fighter_id; }
WEAK void tny_rumble_mode(int on) { (void)on; }

WEAK int  tny_grounds_call(int fn, s32 a, s32 b, s32 c, s32 *push) { (void)fn; (void)a; (void)b; (void)c; (void)push; return 0; }
WEAK void tny_grounds_test(u32 cmd, u32 arg, u32 arg2) { (void)cmd; (void)arg; (void)arg2; }

WEAK int  tny_play_row(int id) { (void)id; return 0; }
WEAK void tny_play_entry(int id, CharacterEntry *out, const CharacterEntry *lead) { (void)id; (void)out; (void)lead; }

// The dispatchers the game calls (vmcall.c, enemies.c, input.c): fixed here, so pieces never touch those files.
int tny_coop_call(int fn, s32 a, s32 b, s32 c, s32 *push) {
    if (fn >= 23 && fn <= 27) return tny_engine_call(fn, a, b, c, push);
    if (fn >= 28 && fn <= 29) return tny_ui_call(fn, a, b, c, push);
    if (fn == 30) return tny_grounds_call(fn, a, b, c, push);
    return 0;
}

void tny_test_cmd(u32 cmd, u32 arg, u32 arg2) {
    if (cmd >= 21 && cmd <= 25) tny_engine_test(cmd, arg, arg2);
    else if (cmd >= 26 && cmd <= 27) tny_moves_test(cmd, arg, arg2);
    else if (cmd >= 28 && cmd <= 29) tny_ui_test(cmd, arg, arg2);
    else if (cmd == 30) tny_grounds_test(cmd, arg, arg2);
}

void tny_frame(void) {
    tny_engine_frame();
    tny_moves_frame();
    tny_ui_frame();
}
