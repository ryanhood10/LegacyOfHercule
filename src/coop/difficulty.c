// Difficulty: the numbers behind Easy / Normal / Hard (difficulty.h) and the drop-roll wrapper.
#include "difficulty.h"

static int flag(int n) { return (g_Party->event_flags[n >> 3] >> (n & 7)) & 1; }

int difficulty(void) {
    if (flag(DIFF_FLAG_EASY)) return DIFF_EASY;
    if (flag(DIFF_FLAG_HARD)) return DIFF_HARD;
    return DIFF_NORMAL;
}

static const u8 enemy_hp[3] = DIFF_ENEMY_HP, damage[3] = DIFF_DAMAGE, speed[3] = DIFF_SPEED;
static const u8 boss_hp[3] = DIFF_BOSS_HP, revive_s[3] = DIFF_REVIVE_S;

int diff_enemy_hp_pct(void) { return enemy_hp[difficulty()]; }
int diff_damage_pct(void) { return damage[difficulty()]; }
int diff_speed_pct(void) { return speed[difficulty()]; }
int diff_boss_hp_pct(void) { return boss_hp[difficulty()]; }
int diff_revive_frames(void) { return revive_s[difficulty()] * 60; }
int diff_regular_twins(void) { return difficulty() != DIFF_EASY; }

// ---- drops ----------------------------------------------------------------------------------------
// A record action is {func, payload}; the bytecode action 0x0801069B runs the script at +4. The stock drop
// rolls (StackRand 100 against thresholds, then SpawnItem; no story steps) are these five scripts, shared by
// every regular enemy record (tools: the defeat scripts of all maps, classified 2026-10-20).
#define ACTION_BYTECODE 0x0801069Bu
ROM_FN(void, Action_RunBytecode, (const u32 *action), 0x0801069B);
static const u32 DROP_SCRIPTS[] = { 0x083EE120, 0x083EEDE4, 0x083FEB28, 0x083FFAF4, 0x083FE538 };
#define N_DROPS (sizeof DROP_SCRIPTS / sizeof DROP_SCRIPTS[0])

static void drop_action(const u32 *action) {
    int d = difficulty();
    if (d == DIFF_HARD && Rand_Range(DIFF_HARD_SKIP) == 0) return;
    int before = g_EntityList->count;
    Action_RunBytecode(action);
    if (d == DIFF_EASY && g_EntityList->count == before) Action_RunBytecode(action);   // nothing dropped: roll again
}

static const u32 drop_actions[N_DROPS][2] = {
    { (u32)drop_action, 0x083EE120 }, { (u32)drop_action, 0x083EEDE4 }, { (u32)drop_action, 0x083FEB28 },
    { (u32)drop_action, 0x083FFAF4 }, { (u32)drop_action, 0x083FE538 },
};

void *diff_drop_action(void *action) {
    const u32 *a = action;
    if (!a || a[0] != ACTION_BYTECODE) return action;
    for (unsigned i = 0; i < N_DROPS; i++)
        if (a[1] == DROP_SCRIPTS[i]) return (void *)drop_actions[i];
    return action;
}
