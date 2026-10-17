#pragma once
#include "rom.h"

// How player 2's buttons are produced (compile-time for now).
#define P2_MODE_SIO    0   // link cable: the other GBA sends its KEYINPUT over serial
#define P2_MODE_MIRROR 1   // P1's input with left/right swapped (test mode, no second controller)
#define P2_MODE_SWAP   2   // Select toggles which player the single controller drives
#ifndef P2_MODE
#define P2_MODE P2_MODE_SIO
#endif

#define COOP_MAGIC 0xC00B0002u

typedef struct CoopState {
    u32 magic;
    Entity *p2;
    u32 p2_vtable_unused[24];   // superseded by g_p2_vtable (48 slots); kept so offsets stay put
    InputSystem input2;
    u8 p2_slot;        // party slot bound to player 2
    u8 control;        // SWAP mode: 0 = controller drives P1, 1 = drives P2
    u16 p2_last_dir;   // per-player copies of g_LastDir / g_LastDirFrame
    u32 p2_last_dir_frame;
    Hud hud2;          // player 2's panel state (vtable unused); tile == 0 means not allocated
    u16 polls;         // input polls since boot (controller-mode detection window)
    u8 sio_ready;      // serial registers initialised
    u8 _pad;
    u16 sio_last;      // last raw word received from the controller GBA
    u16 source_age;    // frames since a player-2 input source last reported (bridge or cable)
    u16 spawns, destroys;   // debug counters for player 2's lifetime
    Entity *acting;         // player responsible for the entity update in progress (exp.c)
    u32 exp_vtable[6];      // copy of the EXP event vtable with the fire slot replaced
    u16 exp_awards[2];      // debug counters: EXP events credited to [0] player 1, [1] player 2
    u8 last_active;         // active slot seen last frame (detects save-pad character switches)
    u8 _pad2;
    u16 rebinds;            // debug counter: player 2 character changes
    u32 code_id;            // address of coop_init_once in the build that filled the vtables
} CoopState;
_Static_assert(__builtin_offsetof(CoopState, exp_awards) == 0xD8, "CoopState layout (peek.lua)");

#define P2_SOURCE_TIMEOUT 180   // frames without a source before player 2 is considered absent

int p2_present(void);        // a second pad is connected (or a test mode is compiled in)
int p1_controllable(void);   // player 1 is in a player-driven control state (not a cutscene)

#define HUD2_X_OFFSET 144  // panel art spans x = 8..88; place the second copy at 152..232

extern CoopState coop;             // pinned at 0x0203FF04 (section .coop)

// Co-op difficulty state (enemies.c), pinned at 0x0203FB00 (section .coop2, below the player-2 vtable block).
#define COOP2_MAGIC 0xC00B0D1Fu
#define TWIN_MAX 24
typedef struct TwinPair { Entity *orig, *twin; } TwinPair;
typedef struct CoopExtra {
    u32 magic;
    u32 enemy_vtable[48];   // copy of the enemy class vtable (all slots, see g_p2_vtable): destroy unregisters the pair
    u32 bar_vtable[24];     // copy of the boss-bar vtable for the twin's bar (settles lower)
    TwinPair twins[TWIN_MAX];
    u16 spawned;            // debug: twins created since boot
    u16 scaled;             // debug: enemies given scaled stats
    u16 bars;               // debug: twin boss bars created
    u16 dmg_calls;          // debug: damage calculations (any defender)
    u16 off;                // tuning switches (set from the console): COOP_OFF_* bits
    u16 _pad;
    u16 dmg_calls_unused;   // (keeps the debug words below word-aligned)
    u16 dmg_player;         // debug: ... with a player as the defender
    u16 dmg_scaled;         // debug: ... that got the co-op multiplier
    s32 last_dmg;           // debug: last calculation's dmg / secondary / defender / attacker
    s32 last_secondary;
    Entity *last_defender;
    Entity *last_attacker;
    u32 cmd, cmd_arg;       // test commands from the console (enemies.lua): COOP_CMD_*
    ActorRecord actor_rec;  // scratch record for COOP_CMD_ACTOR
    u32 actor_count;        // COOP_CMD_ACTOR places successive actors on a 6 x 2 grid around player 1
    u32 partner_override;   // console: nonzero = every boss twin uses this sprite id (model experiments)
} CoopExtra;
#define COOP_CMD_SPAWN_SCRIPT 1   // arg = mapScripts index: spawn it like the TriggerMapEvent opcode (bosses)
#define COOP_CMD_BOSS_BAR     2   // arg = entity (0 = pair 0's original): the DrawBossHealthBar path
#define COOP_CMD_ACTOR        3   // arg = sprite id: stand a script actor of that sprite 32 px right of player 1 (model lookup)
void coop_debug_tick(void);       // enemies.c, from the entity-list hook
extern CoopExtra coop2;

// Tuning (regular enemies while player 2 is present; bosses keep their stats and get a twin)
#define COOP_HP_NUM 3        // HP x 1.5
#define COOP_HP_DEN 2
#define COOP_DMG_NUM 5       // damage dealt to players x 1.25
#define COOP_DMG_DEN 4
#define COOP_SPEED_NUM 6     // walk speed x 1.2
#define COOP_SPEED_DEN 5
#define TWIN_OFFSET 24       // px beside the original
#define COOP_OFF_DAMAGE 1    // coop2.off bits: disable the damage multiplier / twins / speed / HP scaling
#define COOP_OFF_TWINS  2
#define COOP_OFF_SPEED  4
#define COOP_OFF_HP     8
#define COOP_OFF_PARTNER 16  // boss twins keep the original's model

void enemies_init(void);                                   // build the vtable copies (from coop_init_once)
Entity *coop_target(Entity *e, Entity *p1, Entity *p2);    // which player an enemy should see this frame
int is_coop_enemy(const Entity *e);
Entity *nearest_player(const Entity *e, Entity *p1, Entity *p2);   // entities.c
extern volatile u16 g_p2_remote;   // pinned at 0x0203FF00: emulator-side player-2 pad word
#define PLAYER_VTABLE_SLOTS 48     // 0x08025604 .. 0x080256C4
extern u32 g_p2_vtable[PLAYER_VTABLE_SLOTS];   // player 2's private player vtable (section .coop.vt)
extern volatile u16 g_p1_remote;   // pinned at 0x0203FF02: netplay replaces KEYINPUT for player 1 (same format)

void coop_init_once(void);
void exp_init(void);         // build the EXP event vtable copy (called from coop_init_once)
void leash_p2(Entity *e);   // keep player 2 inside the camera view (shared screen)
void coop_active_check(void);   // player 1 switched character: keep player 2 on a different one
void p2_cycle(void);            // player 2 picks the next available character (Select)
