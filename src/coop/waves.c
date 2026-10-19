// Hyperbolic Time Chamber training waves (docs/story.md 4g, docs/scripts.md "CoopCall"). From Cell's
// announcement on, the Lookout's chamber door (zone 9 area 3) warps the players into the chamber (zone
// 15 area 2) and calls WaveStart with the round sequence. Ten rounds of stock enemies follow, fixed per
// round and harder as they go; in co-op every regular enemy brings its twin (enemies.c), so four records
// per round put eight on screen. Between rounds the chamber script shows the round (a caption) and asks
// for the spawn; after round 10 it says "Training completed!" and nothing more spawns. Leaving the map
// (the door, a game over) switches the controller off; the Lookout door starts it again from round 1.
//
//   WS_CLEAR    the last round is cleared (or the training just started): WAVE_PAUSE frames, then the
//               round number goes up and the round sequence is queued (as an enemy's defeat dialogue)
//   WS_ANNOUNCE the sequence shows "Round N" and calls WaveSpawn (CoopCall 14, 1)
//   WS_FIGHT    the round's enemies are on the map; when none is left standing -> WS_CLEAR
//   WS_DONE     past round 10: the sequence said "Training completed!"; nothing more happens
#include "coop.h"

#define WAVE_ZONE   15
#define WAVE_AREA   2
#define WAVE_ROUNDS 10
#define WAVE_MAX    4          // records per round (doubled by the co-op twins; WaveState.recs)
#define WAVE_PAUSE  75         // frames between the last kill and the next round's caption
#define WAVE_GRACE  20         // frames after a spawn before the room is counted
#define WAVE_MAGIC  0xC00B3A7Eu

enum { WS_OFF, WS_CLEAR, WS_ANNOUNCE, WS_FIGHT, WS_DONE };

// A round's records, filled in RAM at the spawn (around the players): the enemy keeps a pointer to its
// record's behaviour list (Enemy_Create 0x0800E280 -> 0x0800D138), so they live until the next round.
typedef struct WaveRec {
    u32 handler, flags;
    s32 stat;
    s16 x, y;
    s32 sprite;
    const u32 *action;
    s32 count;
    u32 beh[1];
} WaveRec;

typedef struct WaveState {
    u32 magic;
    const void *seq;       // the chamber's round sequence (lscript seq:lk_tc_round), from WaveStart
    u32 action[2];         // {EnemyAction_RunDialog, seq}: the event that shows it
    u8 round;              // the round announced / fought (0 before the first, WAVE_ROUNDS + 1 when done)
    u8 state;              // WS_*
    u8 timer;              // frames left in WS_CLEAR / the spawn grace
    u8 alive;              // debug: enemies standing at the last count
    u16 spawned;           // debug: records spawned since boot
    u16 obj_low;           // debug: smallest largest-free OBJ VRAM block seen in this round's fight (bytes / 32)
    u16 ticks;             // debug: input polls (game-loop iterations): against the frame count it shows lag
    u16 _pad;
    WaveRec recs[4];       // the round being fought (WAVE_MAX)
} WaveState;
_Static_assert(sizeof(WaveState) == 28 + 4 * 0x20, "WaveState layout (docs/testing.md peeks)");
WaveState waves __attribute__((section(".party.wv"), used));   // after the .party.kr states (0x0203F880 since the merge; the address moves: read coop.map)

ROM_FN(int, ObjVram_Largest, (void *list), 0x08006D71);       // largest free OBJ VRAM block (bytes)
Entity *enemy_spawn_hook(const EnemyRecord *rec);             // enemies.c: twins, co-op scaling

// ---- the rounds -----------------------------------------------------------------------------------
// One wandering behaviour for every record: the stock wild-wolf wander (zone 2 area 7 record 0's
// {0x0800D0AF, 0x8C}); the chase AI takes over when a player comes near.
#define WANDER 0x083EE430u
static const u32 no_action[2] = { ENEMY_ACTION_NONE, 0 };

// {stat, sprite}: stock enemies (stat table 0x086D56AC, bestiary 0x086F8E40), easier to harder by HP.
typedef struct WaveFoe { u8 stat, sprite; } WaveFoe;
static const WaveFoe rounds[WAVE_ROUNDS][WAVE_MAX] = {
    { {57, 148}, {107, 64}, {57, 148}, {107, 64} },     //  1 wolf 675 HP, hawk 700
    { {92, 108}, {70, 91},  {92, 108}, {70, 91} },      //  2 sabre-toothed tiger 750, Kuma mercenary 940
    { {75, 96},  {80, 105}, {75, 96},  {80, 105} },     //  3 ninja 900, pterodactyl 900
    { {12, 142}, {56, 145}, {12, 142}, {56, 145} },     //  4 Warlord's henchman 900, warthog 900
    { {84, 65},  {104, 149},{84, 65},  {104, 149} },    //  5 hawk 1000, wolf 1000
    { {51, 143}, {10, 19},  {51, 143}, {10, 19} },      //  6 henchman 1100, tiger bandit 1120
    { {40, 47},  {88, 106}, {40, 47},  {88, 106} },     //  7 Eggbot 1130, pterodactyl 1200
    { {86, 98},  {39, 46},  {86, 98},  {39, 46} },      //  8 ninja 1400, Eggbot 1349
    { {89, 110}, {47, 112}, {45, 97},  {47, 112} },     //  9 sabre-toothed tiger 1750, scorpion 2000, ninja 1600
    { {48, 117}, {50, 147}, {32, 38},  {50, 147} },     // 10 snake 3530, wolf 3980, Destroyer 4200 (one: big frames)
};

// Spawn spots: a ring around the players' midpoint (the twin stands TWIN_OFFSET px right of its
// original), clamped to the arena: the void floor inside the border, below the living quarters.
static const s16 ring[WAVE_MAX][2] = { { -96, -40 }, { 72, -40 }, { -72, 56 }, { 48, 56 } };
#define ARENA_X0   24
#define ARENA_X1   (640 - 24 - 24 - 16)   // room for the twin on the right
#define ARENA_Y0   24
#define ARENA_Y1   (480 - 32)
#define QUARTERS_X0 184                   // the living quarters' block and canopy (time_chamber.json)
#define QUARTERS_X1 448
#define QUARTERS_Y1 232

// ---- helpers --------------------------------------------------------------------------------------
static int in_chamber(void) {
    const u8 *m = (const u8 *)g_MapState;   // map state: +0 zone, +1 area (the PushCurrentWorld / Area opcodes)
    return m && m[0] == WAVE_ZONE && m[1] == WAVE_AREA;
}

static int enemies_standing(void) {
    EntityList *l = g_EntityList;
    int n = 0;
    for (int i = 0; i < l->count; i++) {
        Entity *e = l->items[i];
        if (e && (e->vtable == (u32 *)ENEMY_VTABLE || e->vtable == coop2.enemy_vtable) && e->hp > 0) n++;
    }
    return n;
}

static void state_check(void) {
    if (waves.magic == WAVE_MAGIC) return;
    memset(&waves, 0, sizeof waves);
    waves.magic = WAVE_MAGIC;
    waves.obj_low = 0xFFFF;
}

// ---- script calls (vmcall.c) ----------------------------------------------------------------------
// CoopCall 13, seq: start (or restart) the training at round 1 with this round sequence.
void wave_start(const void *seq, int first_round) {
    state_check();
    waves.seq = seq;
    waves.round = (u8)(first_round > 1 && first_round <= WAVE_ROUNDS ? first_round - 1 : 0);
    waves.state = WS_CLEAR;
    waves.timer = 30;
}

// CoopCall 14, 0: the round being announced or fought (WAVE_ROUNDS + 1 once completed).
int wave_round(void) { state_check(); return waves.round; }

// CoopCall 14, 1: spawn the announced round's enemies.
void wave_spawn(void) {
    state_check();
    if (waves.state != WS_ANNOUNCE || waves.round < 1 || waves.round > WAVE_ROUNDS || !in_chamber()) return;
    const WaveFoe *f = rounds[waves.round - 1];
    Entity *p1 = g_PlayerObject;
    s32 cx = p1 ? p1->x >> 8 : 312, cy = p1 ? p1->y >> 8 : 320;
    if (p1 && coop.p2) { cx = (cx + (coop.p2->x >> 8)) / 2; cy = (cy + (coop.p2->y >> 8)) / 2; }
    for (int i = 0; i < WAVE_MAX; i++) {
        WaveRec *r = &waves.recs[i];
        s32 x = cx + ring[i][0], y = cy + ring[i][1];
        if (x < ARENA_X0) x = ARENA_X0;
        if (x > ARENA_X1) x = ARENA_X1;
        if (y < ARENA_Y0) y = ARENA_Y0;
        if (y > ARENA_Y1) y = ARENA_Y1;
        if (x + TWIN_OFFSET >= QUARTERS_X0 && x <= QUARTERS_X1 && y < QUARTERS_Y1) y = QUARTERS_Y1;
        r->handler = 0x0800E77Fu;
        r->flags = 0;
        r->stat = f[i].stat;
        r->x = (s16)x;
        r->y = (s16)y;
        r->sprite = f[i].sprite;
        r->action = no_action;
        r->count = 1;
        r->beh[0] = WANDER;
        Entity *e = enemy_spawn_hook((const EnemyRecord *)r);
        if (!e) continue;
        EntityList_Add(g_SceneTable[1], e);   // the loader's add; the hook adds the twin itself
        waves.spawned++;
    }
    waves.state = WS_FIGHT;
    waves.timer = WAVE_GRACE;
    waves.obj_low = 0xFFFF;
}

// Test command (COOP_CMD_WAVE_CLEAR): every standing enemy on the map dies the regular death (EXP to
// player 1, popup, removal; one blow through OnHit left fliers and dodgers standing).
void wave_clear_all(void) {
    EntityList *l = g_EntityList;
    for (int i = l->count - 1; i >= 0; i--) {
        Entity *e = l->items[i];
        if (!e || !(e->vtable == (u32 *)ENEMY_VTABLE || e->vtable == coop2.enemy_vtable) || e->hp <= 0) continue;
        e->hp = 0;
        EntityList_Broadcast(g_SceneTable[1], 0xF, e);
        Enemy_RegularDeath(ENEMY_CLASS_REGULAR, e);
    }
}

// ---- every input poll (coop_call_tick) ------------------------------------------------------------
void waves_tick(void) {
    state_check();
    waves.ticks++;
    if (waves.state == WS_OFF) return;
    if (!in_chamber()) { waves.state = WS_OFF; return; }   // out of the door (or a game over)
    switch (waves.state) {
    case WS_CLEAR:
        if (waves.timer) { waves.timer--; break; }
        waves.round++;
        waves.state = waves.round > WAVE_ROUNDS ? WS_DONE : WS_ANNOUNCE;
        coop_run_sequence(waves.action, waves.seq);
        break;
    case WS_FIGHT: {
        if (waves.timer) { waves.timer--; break; }
        int largest = ObjVram_Largest(g_EntityList) >> 5;
        if (largest < waves.obj_low) waves.obj_low = (u16)largest;
        int n = enemies_standing();
        waves.alive = (u8)n;
        if (!n) { waves.state = WS_CLEAR; waves.timer = WAVE_PAUSE; }
        break;
    }
    default:
        break;
    }
}

// Test command (COOP_CMD_WAVE_SKIP): the next round announced is `round` (the current one is cleared
// first by COOP_CMD_WAVE_CLEAR).
void wave_skip_to(int round) {
    state_check();
    if (waves.state == WS_OFF || round < 1 || round > WAVE_ROUNDS + 1) return;
    waves.round = (u8)(round - 1);
}
