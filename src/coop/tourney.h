// The tournament mode (docs/tournament.md): the shared contract of the build team (2026-10-25).
//
// Every session of the team codes against this header. It fixes:
//   * who owns which numbers (flags, stats, sprites, rows, portraits, CoopCalls, test commands, maps, RAM),
//   * the fighter ids and the roster table's shape (tourney_data.c holds the table itself),
//   * the functions each piece provides to the others. Every cross-piece function has a weak default in
//     tourney_hooks.c, so main always builds; a piece defines the strong version in its own file.
//
// Owners (each works on its own branch, builds into its own BUILD dir, merges through the main session):
//   grounds   Papaya Island maps, the world-map landing, pads, crowd, director, board object, waiting-room actors
//   art-new   the five fighters built from existing bodies: King Cold, Master Roshi, Chi-Chi, Videl, Chiaotzu
//   art-buu   every Buu's Fury import (fighters, the announcer) and their portraits
//   playables player-layout records for the LoG2 bodies, display rows, the playable kits and multipliers
//   engine    tourney_data.c's numbers, the pool, brackets and simulation, the match loop in every division,
//             heal, rewards, the global record on the save chip, unlocks, challengers, pop-ups
//   moves     the move library's new kinds and every fighter's kit as an opponent; NPC-vs-NPC hits and the
//             Rumble's target weights; the transform rule
//   ui        the list box, the character select, the bracket screen and its animation, the Rumble board,
//             the unlock board, the pop-up's look
#pragma once
#include "coop.h"

// ---- numbers (also in docs/ids.md "Allocations for the tournament build") --------------------------------
// Story flags (cleared at a new game: 400-503 are): grounds 441-445, engine 446-459. Unlocks are NOT flags: they
// live in the global record on the save chip (engine).
#define TNY_FLAG_GROUNDS0   441
#define TNY_FLAG_ENGINE0    446
// Stats: engine 160-219 (one per fighter as an opponent; tier scaling in code). moves may add rows 220-229.
#define TNY_STAT0           160
#define TNY_AB_STEAL        24      // ability: Energy Steal (Spopovich, Yamu as players; tny_play.c, data/hercule.json)
// CoopCall: engine 23-27, ui 28-29, grounds 30.
// Test commands (coop2.cmd): engine 21-25, moves 26-27, ui 28-29, grounds 30.
// Zone 15 areas 11-17: grounds. UI strings 357-379: grounds (place names, banner), 380-399: ui.
// Display rows 29-79: playables (raise the row limit at file 0x42D6 and EXTRA_ROWS in spriteclone.py).
// Abilities 24-40: playables. Portraits 127-139 art-new, 140-189 art-buu, 190-199 ui.

// Sprite ids. A fighter whose record is missing from a build (art not merged yet, BUU=0) has sprite 0 there
// and every piece skips him (tny_fighter_ok()).
// art-new: player-layout 21-group records (they also run under AI set 21 as opponents)
#define TS_KING_COLD        185
#define TS_ROSHI            186
#define TS_CHICHI           187
#define TS_VIDEL            188
#define TS_CHIAOTZU         189
// art-buu: Buu's Fury imports through tools/buu2log2.py (player layout already; data/tny_buu.txt, records checked by eye)
#define TS_SPOPOVICH        195     // Buu r.242
#define TS_YAMU             196     // Buu r.293
#define TS_PUIPUI           197     // Buu r.224
#define TS_YAKON            198     // Buu r.290
#define TS_KIBITO           199     // Buu r.148 (NPC set; r.32 is Evil Buu)
#define TS_PIKKON           200     // Buu r.214 (r.132 is a capped soldier)
#define TS_DABURA           201     // Buu r.63
#define TS_FAT_BUU          202     // Buu r.33
#define TS_EVIL_BUU         203     // Buu r.32 (grey, caped; r.62 is a capped soldier)
#define TS_KID_BUU          204     // Buu r.37
#define TS_SUPER_BUU        205     // Buu r.40 (base)
#define TS_SUPER_BUU_G      206     // Buu r.41 (Gotenks absorbed: the vest)
#define TS_SUPER_BUU_GH     207     // Buu r.43 (Gohan absorbed)
#define TS_JANEMBA          208     // Buu r.144
#define TS_GOTEN            209     // Buu r.114 (SSJ; r.92 is Vegito)
#define TS_KID_TRUNKS       210     // Buu r.260 (SSJ)
#define TS_GOTENKS          211     // Buu r.116 (r.277 is a red-haired kid)
#define TS_GOTENKS_SSJ      212     // Buu r.119
#define TS_GOTENKS_SSJ3     213     // Buu r.120
#define TS_VEGITO           214     // Buu r.92 (SSJ; r.93 is the same frames)
#define TS_UUB              215     // Buu r.263
#define TS_BGOKU            216     // Buu r.105 (base)
#define TS_BGOKU_SSJ        217     // Buu r.108
#define TS_BGOKU_SSJ3       218     // Buu r.110
#define TS_MAJIN_VEGETA     219     // Buu r.274
#define TS_MYSTIC_GOHAN     220     // Buu r.100
#define TS_SAIYAMAN         221     // Buu r.101
#define TS_BPICCOLO         222     // Buu r.212
#define TS_ANNOUNCER        230     // Buu r.8 (NPC; r.207 is the Ox-King)
// playables: player-layout versions of LoG2 bodies (the a16sprites.py regrouping)
#define TS_P_YAMCHA         235
#define TS_P_TIEN           236
#define TS_P_FRIEZA         237
#define TS_P_MECHA          238
#define TS_P_TAO            239
#define TS_P_A13            240
#define TS_P_A17            241
#define TS_P_A18            242
#define TS_P_PCELL          243
#define TS_P_COOLER         244

// RAM: the tournament region `tourneyram` (coop.ld, 0x0203EC00, 0x400 bytes; the heap cap ends there). Each
// piece keeps its state in its own section: engine `.tny.eng` (<= 0x240), ui `.tny.ui` (<= 0x100), moves
// `.tny.mv` (<= 0x80), grounds `.tny.gr` (<= 0x40). State is NOLOAD: guard it with a magic word.
#define TNY_SECTION(name)   __attribute__((section(".tny." name)))
// ROM: big generated art goes in the second blob `rom2` (0x08E00000, 2 MiB): mark frame data with TNY_ART.
#define TNY_ART             __attribute__((section(".rodata.art")))

// ---- the roster ---------------------------------------------------------------------------------------------
typedef enum {
    // Rookie pool
    TF_YAMCHA, TF_TIEN, TF_FRIEZA, TF_MECHA_FRIEZA, TF_KING_COLD, TF_HERCULE, TF_ROSHI, TF_TAO, TF_SPOPOVICH,
    TF_YAMU, TF_CHICHI, TF_VIDEL, TF_CHIAOTZU,
    // Adept
    TF_A19, TF_GERO, TF_A13, TF_A17, TF_A18,
    // Z-Fighter
    TF_IMPERFECT_CELL, TF_A16,
    // Champion
    TF_SEMI_CELL, TF_PERFECT_CELL, TF_CELL_JR, TF_COOLER, TF_NAMEKIAN, TF_BROLY, TF_PARAGUS,
    // Legend
    TF_SUPER_PERFECT_CELL, TF_SUPER13, TF_BGOKU, TF_MAJIN_VEGETA, TF_MYSTIC_GOHAN, TF_SAIYAMAN, TF_BHERCULE,
    TF_BPICCOLO, TF_SUPER_BUU,
    // hidden challengers (never drawn into a pool)
    TF_PUIPUI, TF_GOTEN, TF_KID_TRUNKS, TF_YAKON, TF_PIKKON, TF_DABURA, TF_EVIL_BUU, TF_KIBITO, TF_FAT_BUU,
    TF_JANEMBA, TF_KID_BUU, TF_GOTENKS, TF_UUB, TF_VEGITO,
    // the story's playables as tournament picks (never opponents: no mirror matches)
    TF_GOHAN, TF_KRILLIN, TF_PICCOLO, TF_VEGETA, TF_TRUNKS, TF_GOKU, TF_P_HERCULE, TF_P_A16, TF_SSJ2_GOHAN,
    TF_COUNT
} TFighterId;

enum { TIER_ROOKIE, TIER_ADEPT, TIER_ZFIGHTER, TIER_CHAMPION, TIER_LEGEND, TIER_COUNT };
enum { TAB_ZFIGHTERS, TAB_TOURNAMENT, TAB_VILLAINS, TAB_BUU, TAB_COUNT };
enum { DIV_1V1, DIV_2V2, DIV_RUMBLE, DIV_COUNT };

// role bits
#define TR_OPPONENT   0x01    // drawn into brackets / Rumble packs once his pool condition holds
#define TR_BEAT       0x02    // playable once a player beats him in a match (global record)
#define TR_CHALLENGER 0x04    // a hidden challenger: only met after winning his tournament (global record)
#define TR_STORY      0x08    // a story playable: pickable while the party has him (the party flags)
#define TR_GIANT      0x10    // 64 x 64 body: counts double against the ring's sprite budget
#define TR_HEADLINER  0x20    // a Rumble headliner (enters last with two helpers)

typedef struct TFighter {
    const char *name;         // shown under the cursor, on the bracket, by the announcer
    u16 sprite;               // body as an opponent (and on the select grid)
    u16 play_sprite;          // body as a playable (0 = none; the story's playables use their party row)
    u8 portrait;              // dialogue portrait id (0 = draw the face from the sprite)
    u8 tier;                  // the tier he joins the pool at (TIER_*)
    u8 tab;                   // select-screen tab (TAB_*)
    u8 role;                  // TR_* bits
    u16 pool_flag;            // story flag that must be set for him to be in the pool (0 = none beyond the tier)
    u8 story_slot;            // TR_STORY: the party slot he plays from (0xFF otherwise)
    u8 challenger_of;         // TR_CHALLENGER: tournament index tier * 3 + division, 0xFF otherwise
    const char *unlock_text;  // the unlock board's condition line (the name stays hidden until unlocked)
} TFighter;

extern const TFighter tny_fighters[TF_COUNT];      // tourney_data.c
// The story flag that opens each tier (TIER_ROOKIE: 0 = from the start).
extern const u16 tny_tier_flag[TIER_COUNT];

// The end poses' lying table (generated by tools/tnyplayables.py, build/gen/tny_play_sprites_data.c): a body whose
// group 12 is not a lying pose lies in group `group` frame `frame` (-1 = the last) of its own record, or in `grp` (four
// facing arrays) when set. Ends with sprite 0.
typedef struct { u16 sprite; s8 group, frame; const void *const *grp; } TnyLie;
extern const TnyLie tny_lie[];
const u32 *tny_lie_pose(const Entity *e, int *frame);   // tny_engine.c: a body's lying pose (group arrays) and frame
int tny_fighter_ok(int id);          // his sprite record exists in this build (tourney_data.c)
static inline int tny_tourney_index(int tier, int div) { return tier * DIV_COUNT + div; }

// ---- cross-piece API (weak defaults in tourney_hooks.c) -----------------------------------------------------
// engine
int  tny_engine_call(int fn, s32 a, s32 b, s32 c, s32 *push);   // CoopCall 23-27; returns 1 when *push is set
void tny_engine_test(u32 cmd, u32 arg, u32 arg2);                // test commands 21-25
void tny_engine_frame(void);                                     // every frame (input.c): pop-up queue, match loop
int  tny_tier_open(int tier);                                    // this save's story has opened the tier
int  tny_in_pool(int id);                                        // in this save's pool right now
int  tny_unlocked(int id);                                       // pickable (story / beaten / challenger won)
int  tny_challenge_tried(int id);                                // the record notes an attempt
void tny_notify(const char *line);                               // queue a pop-up for the next safe frame
// The curtain (tny_engine.c, docs/tournament.md 23): 1 while the engine holds the screen black to change the ring's
// scene. A ui screen opened then starts from black (no fade-out of the map) and ends black (no fade-in): the
// engine stages the next scene behind it and fades it in itself.
int  tny_curtain(void);
// The fighters' dialogue seat (docs/tournament.md 23): called with the scene staged and lit, once after the walk-in
// (before the announcer's call: TALK_BEFORE) and once after the end pose and the announcer's result line
// (TALK_AFTER). Returns 1 when it started a scene of its own (a dialogue sequence: the engine waits until no scene
// runs before going on), 0 when it showed nothing (the weak default). Nothing is written for it yet.
enum { TALK_BEFORE, TALK_AFTER };
enum { TALK_MATCH, TALK_RUMBLE, TALK_CHALLENGE, TALK_FINAL };       // TTalkSeat.kind (TALK_FINAL: the co-op duel)
typedef struct TTalkSeat {
    u8 when;            // TALK_BEFORE / TALK_AFTER
    u8 division, tier;  // DIV_*, TIER_*
    u8 kind;            // TALK_MATCH a bracket match, TALK_RUMBLE a Rumble round, TALK_CHALLENGE the hidden fight,
                        // TALK_FINAL the co-op 1 v 1 final (player 1 against player 2)
    u8 mine[2], nmine;  // the players' fighters on the floor (TF_*: their picks)
    u8 opp[6], nopp;    // the other side (TF_*); TALK_FINAL: player 2's pick
    u8 round;           // the bracket round (0-based; the final is rounds - 1) or the Rumble round (0-based)
    u8 last;            // 1: the final / the headliner round
    u8 result;          // TALK_AFTER: 1 the players won, 2 they lost (TALK_FINAL: the winner, player 1 / 2)
    u8 _p;
} TTalkSeat;
int  tny_talk_hook(const TTalkSeat *seat);
// The bracket the engine runs and the ui draws. seat[] holds fighter ids (TF_*), 0xFF = empty; a player sits in
// a seat with his pick's id and p1_seat / p2_seat say which (0xFF = none). winner[r][m] is the winning seat of
// match m in round r (0xFF = not played yet). Quick: size 8, 3 rounds; Full: size 16, 4 rounds.
typedef struct TBracket {
    u8 size, rounds, round, match;          // the next match to play / show
    u8 seat[16];
    u8 winner[4][8];
    u8 p1_seat, p2_seat;
    u8 tier, division;
} TBracket;
const TBracket *tny_bracket(void);                               // engine: the current bracket (0 = none)

// ui
int  tny_ui_call(int fn, s32 a, s32 b, s32 c, s32 *push);       // CoopCall 28-29 (the screens, run from scripts)
int  tny_ui_choose(const char *title, const char *const *lines, int n);   // modal list box: index or -1 (B)
int  tny_ui_select(int player, int tier, u8 *pick);              // modal select for player 0 / 1: 1 picked, 0 backed out
void tny_ui_bracket(int phase);                                  // modal bracket screen: 0 show the next match,
                                                                 // 1 animate the last results, then show the next
void tny_ui_rumble_board(const u8 *ids, const u8 *down, int n);  // modal: the round's faces, fallen ones greyed
void tny_ui_test(u32 cmd, u32 arg, u32 arg2);                    // test commands 28-29
void tny_ui_frame(void);                                         // every frame
void tny_popup_show(const char *line);                           // the pop-up's look (engine calls it when safe)
// moves
void tny_moves_test(u32 cmd, u32 arg, u32 arg2);                 // test commands 26-27
void tny_moves_frame(void);                                      // every frame: kits, transforms, statuses
void tny_moves_attach(Entity *e, int fighter_id);                // give a spawned opponent his kit
void tny_rumble_mode(int on);                                    // NPCs hit each other, target weights on / off
void tny_moves_final_form(Entity *e);                            // every transform of his kit now (a final-form fight)
// grounds
int  tny_grounds_call(int fn, s32 a, s32 b, s32 c, s32 *push);  // CoopCall 30
void tny_grounds_test(u32 cmd, u32 arg, u32 arg2);               // test command 30
void tny_talk_test(u32 cmd, u32 arg, u32 arg2);                  // test command 31 (tny_talk.c)
void tny_talk_frame(void);                                       // tny_talk.c: Start skips a talk
void tny_grounds_frame(void);                                    // every frame (held poses on the grounds)
// playables
int  tny_play_row(int id);                                       // display row of a tournament playable (0 = none)
void tny_play_entry(int id, CharacterEntry *out, const CharacterEntry *lead);   // his entry at the lead's level
