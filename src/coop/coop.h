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
    Entity *p1_frame;       // player 1 at the start of this frame's entity update (g_PlayerObject is swapped per entity)
    Entity *a16;            // the live Android 16 boss (android16.c), 0 when none
    u16 a16_fired, a16_booms;   // debug: projectiles launched, explosion sprites spawned
    u16 a16_hits, a16_skipped;  // debug: blasts that hit a player, projectiles skipped (OBJ VRAM / heap)
    u32 a16_force;          // console/GDB: nonzero = every special is this attack (1 homing, 2 barrage, 3 bombs, 4 punch)
    u16 a16_bot;            // A16_TEST builds: nonzero = both players are driven by the test autopilot (input.c)
    u16 a16_specials;       // debug: specials started
    u32 a16_spawn_frame;    // debug: g_FrameCounter when he spawned / when his HP reached 0
    u32 a16_dead_frame;
    u16 downed;             // debug: bosses left downed by the both-down rule (enemies.c)
    u16 dupes_removed;      // debug: town NPCs removed because a player controls that character (actors.c)
    u32 cmd_arg2;           // second argument of a test command (COOP_CMD_WARP: x | y << 16)
    u32 p2_class_key;       // stats/row/slot player 2's class init last ran with (player.c)
    u8 duel_active;         // duel.c: player-vs-player hits on, final blow ends the duel
    u8 duel_winner;         // 1 or 2 after a duel (0 before)
    u8 duel_flags;          // DUEL_* below
    u8 duel_saved_slot;     // player 2's slot before a clone duel
    s16 duel_end_script;    // mapScripts index run when the duel ends (-1 none)
    u16 duel_hits;          // debug: player-vs-player hits landed
    u8 *duel_tint;          // the clone's shadow remap table (Arena)
    u8 pad_swap;            // 1 = player 1's pad drives player 2's entity and the other way round
    u8 p2_class_row;        // display row player 2's class init last ran with (player.c)
    u8 _pad5[2];
    u32 boss_rec[8];        // scratch record for COOP_CMD_SPAWN_RECORD (0x1C bytes, no behaviours)
    // script -> C bridge (vmcall.c, game action 59 "CoopCall")
    const void *call_end_seq;   // dialogue sequence run when the bridged duel ends (0 none)
    u32 call_action[2];         // {EnemyAction_RunDialog, sequence}: what the end event fires
    s8 call_slot;               // pending duel: player 2's slot (-1 = clone of player 1)
    u8 call_flags;              // pending duel: DUEL_* flags
    u8 call_state;              // 0 idle, 1 waiting for player 2, 2 duel running
    u8 _pad6;
} CoopExtra;
_Static_assert(sizeof(CoopExtra) <= 0x2C0, "CoopExtra must fit 0x0203FB00..0x0203FDC0 (StoryTest above it)");

// Story test preset: Party_Init (party.c) applies it after resetting the party, so a debug Map Test
// starts a chapter with its story flags. STORY_TEST builds read it from ROM (STORY_PRESET, file offset
// 0xC00000, written by tools/fork/st_preset.py); a debugger may also write it to EWRAM (STORY_TEST_RAM, used
// when the ROM copy has no magic). Inert unless magic == STORY_TEST_MAGIC; the game never writes either.
typedef struct StoryTest {
    u32 magic;
    u16 set[16];            // story flags to set (0xFFFF ends the list)
    u16 clear[8];           // story flags to clear (0xFFFF ends the list)
} StoryTest;
#define STORY_TEST_RAM ((StoryTest *)0x0203FDC0)
#define STORY_PRESET ((const StoryTest *)0x08C00000)
#define STORY_TEST_MAGIC 0x53545259u   // "STRY"
_Static_assert(sizeof(StoryTest) <= 0x40, "StoryTest must fit 0x0203FDC0..0x0203FE00");
#define COOP_CMD_SPAWN_SCRIPT 1   // arg = mapScripts index: spawn it like the TriggerMapEvent opcode (bosses)
#define COOP_CMD_BOSS_BAR     2   // arg = entity (0 = pair 0's original): the DrawBossHealthBar path
#define COOP_CMD_ACTOR        3   // arg = sprite id: stand a script actor of that sprite 32 px right of player 1 (model lookup)
#define COOP_CMD_A16          4   // spawn the Android 16 boss 64 px right of player 1, with a boss bar (android16.c)
#define COOP_CMD_STORY_WIN    7   // chapter tests: win every boss fight on the map (story_test_finish, input.c)
#define COOP_CMD_DUEL         6   // arg = (p2_slot + 1, 0 = clone) | flags << 8 | (end_script + 1) << 16: duel_start
#define COOP_CMD_WARP         5   // arg = zone | area << 8, arg2 = x | y << 16 (pixels): the WarpToMap opcode (39)
void camera_shake_safe(int frames, int strength);   // player.c: Camera_Shake that never re-links a shaking camera
void p2_sync_combat(void);                          // player.c: player 2 entity combat bytes from its own slot
#define DUEL_CLONE          1   // player 2 plays a copy of player 1's character (last party slot)
#define DUEL_SHADOW         2   // draw player 2 through a darkening colour remap
#define DUEL_SWAP_ON_P2_WIN 4   // when player 2 wins, the pads swap from then on
#define DUEL_FULL_EP        8   // both players' EP stays full (no Super Saiyan drain)
void duel_start(int p2_slot, int flags, int end_script);   // duel.c
void duel_clear(void);
void duel_tick(void);
void coop_call_tick(void);                           // vmcall.c, every input poll
int duel_p2_final_blow(Entity *e, int dmg);
void actors_tick(void);                             // actors.c: remove town NPCs that duplicate a player's character
#define COOP_CMD_SPAWN_RECORD 8   // arg = enemy record address (any map's): spawn it 48 px right of player 1, plus its boss bar
void coop_debug_tick(void);       // enemies.c, from the entity-list hook
void story_test_finish(void);     // enemies.c: win every boss fight on the map (chapter tests)
extern CoopExtra coop2;

// Time Chamber training waves (waves.c; state `waves` in .party.wv, 24 bytes at 0x0203F874)
#define COOP_CMD_WAVE_CLEAR 13    // every standing enemy on the map takes a blow worth its HP (wave tests)
#define COOP_CMD_WAVE_SKIP  14    // arg = round: the next round announced is arg (wave tests; the training must be running)
void coop_run_sequence(u32 *action, const void *seq);   // vmcall.c: queue a dialogue sequence (action = 2 live words)
void wave_start(const void *seq, int first_round);
int wave_round(void);
void wave_spawn(void);
void wave_clear_all(void);
void wave_skip_to(int round);
void waves_tick(void);

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
#define COOP_OFF_DUPES 64     // keep town NPCs that show a player's character
#define COOP_OFF_BOTHDOWN 32 // the first boss of a pair to fall is removed (pre-2026-10-18 behaviour) instead of staying down

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
void krillin_frame(const InputSystem *in);   // krillin.c, every input poll
void old_hercule_set(int on);   // krillin.c: slot 5 -> old Hercule's row (1) or Hercule's (0)
