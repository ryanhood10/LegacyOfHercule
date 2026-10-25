// The tournament's moves (docs/tournament.md sections 4, 5 and "Moves (built)"): how tournament opponents fight.
//
// tny_moves_attach(e, fighter) gives a spawned opponent (an enemy entity: the ROM's enemy vtable, the co-op
// copy, or Android 16's / Super 13's private one) his kit from tny_kits[] below. The entity gets a private vtable
// copy of whatever it had (destroy, update and messages wrapped) and a private stat copy (transforms raise it,
// NPC-vs-NPC kills zero its EXP). Between the stock AI's ticks a special-attack scheduler runs his moves, each
// written once in the move library:
//   beam      the players' Kamehameha beam object (Beam_Init / Beam_Fire / Beam_End, cellgames.c) with the
//             fighter as owner: an enemy's hit type byte (+0x16C) is 0, so it hurts players only (thin: capped
//             length; stun: freezes whoever it touches; cost: HP)
//   ball      an orb of the projectile class (super13.c's orbs): small / medium fast, large held over the head
//             and thrown slowly homing (the Deadly Bomber)
//   homing    Android 16's heat-seeking rocket class (missiles, orbs or blue ki blasts)
//   volley    Super 13's ki volley (a fan of small shots; converge = Hellzone Grenade)
//   wave      Super 13's ki wave (a slow head leaving a trail)
//   nova      Super 13's ki nova (a blast round him, a ring of orbs); self-KO = Final Explosion / Chiaotzu
//   rush      dash at the target (map collision checked) and N melee hits; stun = a grab or a frying pan
//   teleport  vanish with a flash and strike from behind the target (or hop away: a dodge)
//   stun      freeze the target N frames: a flash round him (Solar Flare), a held target (psychic hold) or a
//             projectile (Stone Spit, Galactic Donut); players' pads go dead, a fighter's AI stops
//   heal      restore the most hurt fighter of his side (Kibito)
//   drain     passive: his melee hits heal him (HP) or empty the victim's ki (EP)
//   regen     passive: HP ticks back every second
//   transform at N % HP: power-up pose, the new sprite record (Frieza -> Mecha Frieza, the Buu chain...) and
//             more attack / defence; same sprite = a stat boost (Roshi's MAX Power, Broly's rage)
//   summon    spawn helpers beside him (Cell Jr.) or, at N % HP, a copy of himself (Multi-Form)
// Every special is a type-0 hit message (only players accept those) except in Rumble mode, where a fighter
// accepts type-0 hits from a fighter of another side (tny_rumble_mode). Enemies have no EP: cooldowns pace the
// specials; damage comes from the fighter's stat through the stock damage calculation. A special cannot be
// stunned out of, and a hit during one only does damage (no AI hit reaction); KF_POISE fighters also get a
// poise window after every stun (the Android 16 rule).
//
// Rumble mode: fighters target the nearest player 70 % of the time and the nearest fighter of another side
// 30 %, re-rolled every RUMBLE_REROLL frames, always a player within RUMBLE_CLOSE px. A fighter's update points
// g_PlayerObject at its target while the stock AI runs (entities.c does the same for player 2), so the stock
// AI chases and swings at another fighter. NPC-vs-NPC kills give nobody EXP.
//
// Test commands 26 / 27 (tny_moves_test, docs/ids.md): spawn a fighter with his kit beside player 1; force a
// move, Rumble mode, HP. State `mv` in .tny.mv (find it by TNY_MV_MAGIC); kit state on the heap per fighter.
#include "tourney.h"
#include "revive.h"
#include "bosses.h"
#include "android16.h"
#include "super13.h"

// ---- tuning ---------------------------------------------------------------------------------------------------
#define FIRST_DELAY     90      // frames after the attach before the first special
#define RUMBLE_REROLL   180     // frames between Rumble target rolls
#define RUMBLE_PLAYER   70      // percent: the nearest player (else the nearest other fighter)
#define RUMBLE_CLOSE    40      // px: a player this close is always the target
#define POISE_FRAMES    75      // KF_POISE: hits only do damage this long after a stun
#define RUSH_SPEED      5       // px per frame
#define BEAM_THIN_LEN   112     // px: a thin beam's longest
#define STUN_FX_LIVE    1       // a stunned target shows small orbs over his head
#define KIT_LIVE        16      // projectiles tracked per fighter
#define KIT_MAX         8       // fighters with a kit at once (the ring holds 2 players + 4 normal bodies)
#define TNY_MV_MAGIC    0x4D4F5645u   // "MOVE"

// ---- ROM functions and layouts -----------------------------------------------------------------------------------
typedef struct Desc { s8 xo, yo; u8 w, h; u32 attr; const void *blob; } Desc;
extern const Desc a16_missile[16], a16_boom[4];                                // build/gen/a16_sprites_data.c
extern const Desc s13_orb_s[3], s13_orb_m[2], s13_orb_l[2], s13_trail[2];      // build/gen/super13_sprites_data.c
extern const Desc kr_kiblast[16];                                              // build/gen/krillin_sprites_data.c

ROM_FN(void *, ProjBase_Init, (void *obj, int vram_bytes, Entity *owner), 0x08013BC1);
ROM_FN(void, ProjBase_UpdateBox, (void *obj), 0x08013BF5);
ROM_FN(int, ObjVram_Largest, (void *list), 0x08006D71);
ROM_FN(void, Entity_SetCtrlState, (Entity *e, void *state), 0x0800763D);
ROM_FN(void, Entity_BaseUpdate, (Entity *e), 0x0800772B);
ROM_FN(void, Entity_GetBox, (s32 *out, const Entity *e), 0x080079B9);
ROM_FN(void, Entity_BeamPoint, (s32 *out, Entity *e), 0x08007A27);
ROM_FN(void *, Beam_Init, (void *mem, s32 *pos, int a, Entity *owner), 0x0801D84D);
ROM_FN(void, Beam_Fire, (void *beam, int facing, s32 *pos), 0x08014479);
ROM_FN(void, Beam_End, (void *beam), 0x0801451D);
#define PROJ_VTABLE      0x080259C4u
#define PROJ_DESTROY     0x08027A69u
#define PROJ_MESSAGE     0x08013D69u
#define IDLE_STATE_VTABLE 0x08024BA0u
#define BEAM_VTABLE      0x08025830u
#define BEAM_UPDATE      0x0801D88Bu
#define BEAM_OFFSETS     ((const s32 *)0x08701DF0)
#define ENEMY_MESSAGE    0x0800E5B9u
#define COLLISION_MAP    ((const void *)g_Ctx[4])
#define E8(e, o)         (*(u8 *)((u8 *)(e) + (o)))
#define E16(e, o)        (*(u16 *)((u8 *)(e) + (o)))
#define E32(e, o)        (*(u32 *)((u8 *)(e) + (o)))
void damage_calc_hook(Entity *defender, Hit *hit);          // enemies.c
Entity *enemy_spawn_hook(const EnemyRecord *rec);           // enemies.c
BossBar *bossbar_create_hook(void *mem, Entity *e, int lo, int hi);

typedef struct Beam {
    u32 *vtable; s32 sort_y; u8 prio, _p0[3]; u16 tile, tile_bytes;
    u8 state, impact, alt, _p1;              // +0x10 state 0 charge, 1 out, 2 ending
    s32 fx_time, ticks;
    s32 x, y;                                // +0x1C origin (world px)
    s32 sx, sy;
    s32 dx, dy;                              // +0x2C unit direction
    s32 len;                                 // +0x34 px
    s32 facing, mult;                        // +0x38, +0x3C damage multiplier (10.10)
    const u8 *frame, *frame_set, *frames;
    Entity *owner;                           // +0x4C (hit type = owner's byte +0x16C)
} Beam;
_Static_assert(sizeof(Beam) == 0x50, "Beam size");
#define BEAM_VT_SLOTS 16

// The hit message (the ROM bomb's, android16.c): type 0 = only players accept it.
typedef struct HitMsg {
    u8 type; u8 _p0[3];
    s32 box[4];
    u8 facing; u8 _p1[3];
    s32 mult;
    const u32 *attack;
    Entity *attacker;
    s32 out[3];
    void *source;
    s32 _p2[3];
} HitMsg;
static const u32 atk_ki[2]    = { 0x0101, 0x400 };   // {kind 1 ranged, reaction 1}, base power
static const u32 atk_heavy[2] = { 0x0301, 0x400 };   // ranged, reaction 3 (the heavy knock-back)
static const u32 atk_melee[2] = { 0x0100, 0x400 };   // melee (crits), reaction 1
static const u32 atk_smash[2] = { 0x0300, 0x400 };   // melee, heavy knock-back

// ---- the move library's kinds and the kit table ---------------------------------------------------------------
enum { MK_NONE, MK_BEAM, MK_BALL, MK_HOMING, MK_VOLLEY, MK_WAVE, MK_NOVA, MK_RUSH, MK_TELEPORT, MK_STUN,
       MK_HEAL, MK_DRAIN, MK_REGEN, MK_TRANSFORM, MK_SUMMON, MK_COUNT };
// move flags
#define MF_PASSIVE  0x01    // never scheduled (drain, regen)
#define MF_TRIGGER  0x02    // once, when his HP falls to cd percent (transform, Multi-Form, self-destruct)
#define MF_STUN     0x04    // beam / rush: freezes whoever it hits
#define MF_THIN     0x08    // beam: short; volley / homing: blue ki art
#define MF_COST     0x10    // beam: costs him HP (Tri-Beam)
#define MF_HOME     0x20    // ball: homes weakly; volley: converges (Hellzone Grenade)
#define MF_SELFKO   0x40    // nova: costs him the match
#define MF_TEAMONLY 0x80    // summon: only in a Rumble or 2v2 (the plan: Cell Jr. helpers)
#define MF_PROJ     MF_HOME // stun: a projectile (Stone Spit, Galactic Donut)
#define MF_TARGET   MF_THIN // stun: the target only, at range (psychic hold)
#define MF_KI       MF_THIN // volley / homing: blue ki blasts
#define MF_DODGE    MF_THIN // teleport: hop away instead of striking

typedef struct TMove {
    u8 kind, a, b, flags;   // a / b per kind (below)
    u16 power;              // damage multiplier, 10.10 (0x400 = the plain formula); transform: sprite id
    u16 cd;                 // cooldown in frames; MF_TRIGGER: the HP percent that fires it
} TMove;
// a / b:  beam a = hold frames, b = charge frames (0 = 24) | ball a = size 0-2, b = speed / 16 |
//         homing a = count, b = art 0 missile 1 orb | volley a = count, b = frames apart | wave - | nova a = radius,
//         b = ring orbs | rush a = distance px, b = hits | teleport - | stun a = frames / 4, b = flash radius px |
//         heal a = percent of max | drain a = percent of the damage healed, b = EP drained | regen a = permille of
//         max per second | transform a = attack added, b = defence added | summon a = count, b = fighter (0xFF self)
#define BEAM(p, hold, fl, cd)           { MK_BEAM, hold, 0, fl, p, cd }
#define BEAMC(p, hold, ch, fl, cd)      { MK_BEAM, hold, ch, fl, p, cd }
#define BALL(p, size, spd, fl, cd)      { MK_BALL, size, spd, fl, p, cd }
#define HOMING(p, n, art, fl, cd)       { MK_HOMING, n, art, fl, p, cd }
#define VOLLEY(p, n, gap, fl, cd)       { MK_VOLLEY, n, gap, fl, p, cd }
#define WAVE(p, cd)                     { MK_WAVE, 0, 0, 0, p, cd }
#define NOVA(p, r, ring, fl, cd)        { MK_NOVA, r, ring, fl, p, cd }
#define RUSH(p, dist, hits, fl, cd)     { MK_RUSH, dist, hits, fl, p, cd }
#define TELE(p, fl, cd)                 { MK_TELEPORT, 0, 0, fl, p, cd }
#define STUN(f4, r, fl, cd)             { MK_STUN, f4, r, fl, 0x200, cd }
#define STUNP(p, f4, fl, cd)            { MK_STUN, f4, 0, fl, p, cd }
#define HEAL(pct, cd)                   { MK_HEAL, pct, 0, 0, 0, cd }
#define DRAIN(hp, ep)                   { MK_DRAIN, hp, ep, MF_PASSIVE, 0, 0 }
#define REGEN(pm)                       { MK_REGEN, pm, 0, MF_PASSIVE, 0, 0 }
#define XFORM(pct, sprite, atk, def)    { MK_TRANSFORM, atk, def, MF_TRIGGER, sprite, pct }
#define SUMMON(n, who, fl, cd)          { MK_SUMMON, n, who, fl, 0, cd }
#define SELF 0xFF

// kit flags
#define KF_NATIVE 0x01      // his own AI does his specials (Android 16, Super 13): only Rumble hits and targeting
#define KF_POISE  0x02      // a stun opens a poise window (giants, bruisers)
#define KIT_MOVES 6
typedef struct TKitDef {
    u8 flags;
    u8 range;               // px: a special starts only with the target this close (0 = 170)
    u8 pace;                // frames between specials / 2 (0 = 75: 150 frames)
    u8 _pad;
    TMove m[KIT_MOVES];
} TKitDef;

// One row per fighter (docs/tournament.md 4a / 4b; numbers ours, tuned in play). Adding a fighter is one row.
const TKitDef tny_kits[TF_COUNT] = {
    // ---- Rookie
    [TF_YAMCHA]   = { 0, 0, 0, 0, { RUSH(0x300, 80, 3, 0, 300), HOMING(0x300, 1, 1, 0, 420) } },   // Wolf Fang Fist, Spirit Ball
    [TF_TIEN]     = { 0, 0, 0, 0, { BEAMC(0x480, 40, 36, MF_COST, 420), STUN(22, 72, 0, 540),     // Tri-Beam, Solar Flare,
                                    SUMMON(1, SELF, MF_TRIGGER, 50) } },                              // Multi-Form at 50 %
    [TF_FRIEZA]   = { 0, 0, 0, 0, { BEAM(0x280, 24, MF_THIN, 300), BALL(0x500, 2, 12, MF_HOME, 600),   // Death Beam, Death Ball,
                                    XFORM(40, 49, 8, 4) } },                                          // Mecha Frieza at 40 %
    [TF_MECHA_FRIEZA] = { 0, 0, 0, 0, { BEAM(0x2C0, 24, MF_THIN, 300), RUSH(0x340, 72, 2, 0, 300) } },
    [TF_KING_COLD] = { KF_POISE, 0, 0, 0, { RUSH(0x400, 28, 1, MF_STUN, 360), BEAM(0x280, 24, MF_THIN, 360) } },   // the throw, Death Beam
    [TF_HERCULE]  = { 0, 0, 90, 0, { RUSH(0x380, 64, 1, 0, 300), RUSH(0x400, 16, 1, MF_STUN, 420) } },   // Dynamite Kick, Megaton Punch
    [TF_ROSHI]    = { 0, 0, 0, 0, { BEAMC(0x500, 60, 48, 0, 540), STUN(14, 56, 0, 480),                // Kamehameha (slow, strong), Thunder Shock,
                                    RUSH(0x2C0, 56, 3, 0, 300), XFORM(50, 0, 12, 0) } },              // Drunken Fist, MAX Power at 50 %
    [TF_TAO]      = { 0, 0, 0, 0, { BEAM(0x2C0, 28, MF_THIN, 360), RUSH(0x300, 72, 2, 0, 300),         // Dodon Ray, Crane rush,
                                    RUSH(0x600, 12, 1, 0, 720) } },                                   // the hidden blade
    [TF_SPOPOVICH] = { KF_POISE, 0, 0, 0, { RUSH(0x400, 16, 1, MF_STUN, 420) } },                    // bear hug
    [TF_YAMU]     = { 0, 0, 0, 0, { DRAIN(0, 12), RUSH(0x2C0, 48, 2, 0, 360) } },                    // energy steal
    [TF_CHICHI]   = { 0, 0, 0, 0, { RUSH(0x480, 12, 1, MF_STUN, 420), RUSH(0x220, 48, 4, 0, 300) } },  // frying pan, combo
    [TF_VIDEL]    = { 0, 0, 0, 0, { RUSH(0x220, 48, 4, 0, 300), RUSH(0x300, 96, 1, 0, 360), TELE(0, MF_DODGE, 480) } },   // Hawk Arrow, flight dodge
    [TF_CHIAOTZU] = { 0, 0, 0, 0, { BEAM(0x260, 24, MF_THIN, 300), STUN(20, 0, MF_TARGET, 540),     // Dodon Ray, psychic hold,
                                    NOVA(0x700, 56, 0, MF_TRIGGER | MF_SELFKO, 10) } },               // self-destruct at 10 %
    // ---- Adept
    [TF_A19]      = { 0, 0, 0, 0, { DRAIN(30, 0), BEAM(0x240, 20, MF_THIN, 300) } },               // absorb, eye beams
    [TF_GERO]     = { 0, 0, 0, 0, { DRAIN(40, 0), BEAM(0x240, 20, MF_THIN, 300) } },
    [TF_A13]      = { 0, 0, 0, 0, { BALL(0x480, 2, 12, MF_HOME, 540), RUSH(0x300, 72, 2, 0, 300) } },    // S.S. Deadly Bomber
    [TF_A17]      = { KF_POISE, 0, 0, 0, { BALL(0x380, 1, 32, 0, 360), NOVA(0x300, 40, 0, 0, 480),   // Power Blitz, Energy Barrier,
                                           TELE(0x300, 0, 420) } },                                   // afterimage
    [TF_A18]      = { 0, 0, 0, 0, { BALL(0x380, 1, 32, 0, 360), BEAM(0x260, 24, MF_THIN, 300),       // Power Blitz, Finger Beam,
                                    VOLLEY(0x180, 6, 6, MF_KI, 420) } },                              // Infinity Bullet
    // ---- Z-Fighter
    [TF_IMPERFECT_CELL] = { 0, 0, 0, 0, { BEAM(0x400, 48, 0, 420), DRAIN(40, 0), SUMMON(1, SELF, MF_TRIGGER, 50) } },
    [TF_A16]      = { KF_NATIVE | KF_POISE, 0, 0, 0, { { 0 } } },                                    // android16.c's rockets
    // ---- Champion
    [TF_SEMI_CELL] = { 0, 0, 0, 0, { BEAM(0x440, 48, 0, 420), SUMMON(1, SELF, MF_TRIGGER, 50) } },
    [TF_PERFECT_CELL] = { 0, 0, 0, 0, { BEAM(0x480, 56, 0, 420), STUN(22, 72, 0, 540),              // Kamehameha, Solar Flare,
                                        SUMMON(2, TF_CELL_JR, MF_TEAMONLY, 900), XFORM(30, 0, 10, 6) } },   // Cell Jr., Super Perfect
    [TF_CELL_JR]  = { 0, 0, 0, 0, { BEAM(0x200, 24, MF_THIN, 360), RUSH(0x280, 64, 2, 0, 300) } },
    [TF_COOLER]   = { KF_POISE, 0, 0, 0, { BEAM(0x400, 32, MF_THIN, 360), BALL(0x600, 2, 12, MF_HOME, 660) } },   // Death Flash, Supernova
    [TF_NAMEKIAN] = { 0, 0, 0, 0, { BEAM(0x3C0, 24, MF_THIN, 420), RUSH(0x280, 48, 2, 0, 300) } },   // Special Beam Cannon
    [TF_BROLY]    = { KF_POISE, 0, 0, 0, { BALL(0x500, 1, 24, 0, 420), BALL(0x700, 2, 10, MF_HOME, 720),   // Eraser Cannon, Gigantic Meteor,
                                           VOLLEY(0x200, 6, 8, 0, 480), XFORM(50, 0, 10, 4) } },           // Blaster Shell, the rage
    [TF_PARAGUS]  = { 0, 0, 0, 0, { VOLLEY(0x180, 2, 10, MF_KI, 300), RUSH(0x280, 32, 1, 0, 360) } },
    // ---- Legend
    [TF_SUPER_PERFECT_CELL] = { KF_POISE, 0, 0, 0, { BEAM(0x500, 60, 0, 360), STUN(22, 72, 0, 540),
                                                     TELE(0x380, 0, 420), VOLLEY(0x200, 5, 6, 0, 480) } },
    [TF_SUPER13]  = { KF_NATIVE | KF_POISE, 0, 0, 0, { { 0 } } },                                    // super13.c's specials
    [TF_BGOKU]    = { 0, 0, 0, 0, { XFORM(60, TS_BGOKU_SSJ, 8, 4), XFORM(25, TS_BGOKU_SSJ3, 10, 6), BEAM(0x480, 56, 0, 420),
                                    TELE(0x380, 0, 420), RUSH(0x400, 80, 1, 0, 480) } },              // Kamehameha, IT, Dragon Fist
    [TF_MAJIN_VEGETA] = { 0, 0, 0, 0, { BALL(0x480, 1, 32, 0, 360), BEAMC(0x500, 56, 40, 0, 480),   // Big Bang, Final Flash,
                                        NOVA(0x900, 64, 8, MF_TRIGGER | MF_SELFKO, 10) } },           // Final Explosion at 10 %
    [TF_MYSTIC_GOHAN] = { 0, 0, 0, 0, { BEAM(0x400, 24, 0, 300), BEAM(0x500, 56, 0, 420) } },       // Masenko, Kamehameha
    [TF_SAIYAMAN] = { 0, 0, 0, 0, { BEAM(0x300, 24, 0, 360), RUSH(0x280, 56, 2, 0, 300) } },
    [TF_BHERCULE] = { 0, 0, 90, 0, { RUSH(0x380, 64, 1, 0, 300), RUSH(0x400, 16, 1, MF_STUN, 420) } },
    [TF_BPICCOLO] = { 0, 0, 0, 0, { BEAM(0x480, 40, MF_THIN, 420), VOLLEY(0x1A0, 8, 4, MF_HOME, 480) } },   // SBC, Hellzone Grenade
    [TF_SUPER_BUU] = { KF_POISE, 0, 0, 0, { VOLLEY(0x200, 8, 4, 0, 480), NOVA(0x300, 48, 0, 0, 480), DRAIN(30, 0),   // Human Extinction,
                                            XFORM(60, TS_SUPER_BUU_G, 8, 4), XFORM(30, TS_SUPER_BUU_GH, 10, 6) } },     // Vice Shout, absorb
    // ---- hidden challengers
    [TF_PUIPUI]   = { 0, 0, 0, 0, { VOLLEY(0x180, 4, 8, MF_KI, 300), RUSH(0x280, 48, 2, 0, 360) } },
    [TF_GOTEN]    = { 0, 0, 0, 0, { BEAM(0x300, 24, MF_THIN, 360), RUSH(0x280, 72, 2, 0, 300) } },  // Kamekameha
    [TF_KID_TRUNKS] = { 0, 0, 0, 0, { BALL(0x380, 1, 32, 0, 360), BEAM(0x2C0, 32, 0, 420) } },      // Big Tree Cannon
    [TF_YAKON]    = { KF_POISE, 0, 0, 0, { DRAIN(0, 16), RUSH(0x380, 40, 2, 0, 300) } },            // Darkness Eater, claws
    [TF_PIKKON]   = { 0, 0, 0, 0, { BEAM(0x400, 40, 0, 420), NOVA(0x300, 36, 0, 0, 420), BALL(0x400, 1, 34, 0, 360) } },   // Thunder Flash, Hyper Tornado, Burning Shoot
    [TF_DABURA]   = { KF_POISE, 0, 0, 0, { WAVE(0x200, 420), STUNP(0x200, 45, MF_PROJ, 600), RUSH(0x400, 32, 1, 0, 360) } },   // Evil Flame, Stone Spit, sword
    [TF_EVIL_BUU] = { 0, 0, 0, 0, { WAVE(0x200, 420), BALL(0x480, 1, 36, 0, 420), REGEN(8) } },     // mouth blast, Vanishing Ball
    [TF_KIBITO]   = { KF_POISE, 0, 0, 0, { RUSH(0x300, 40, 2, 0, 300), HEAL(20, 600) } },
    [TF_FAT_BUU]  = { KF_POISE, 0, 0, 0, { BEAM(0x200, 28, MF_THIN | MF_STUN, 480), NOVA(0x400, 52, 6, 0, 540),   // Candy Beam, Angry Explosion,
                                           REGEN(10), WAVE(0x200, 420) } },                                    // Mouth Blast
    [TF_JANEMBA]  = { KF_POISE, 0, 0, 0, { RUSH(0x400, 40, 1, 0, 300), TELE(0x380, 0, 420), BALL(0x400, 2, 12, MF_HOME, 600) } },   // Dimension Sword, Rakshasa's Claw, Hell's Gate
    [TF_KID_BUU]  = { 0, 0, 0, 0, { BALL(0x700, 2, 12, MF_HOME, 720), BALL(0x400, 1, 38, 0, 360), RUSH(0x300, 96, 3, 0, 300), REGEN(8) } },
    [TF_GOTENKS]  = { 0, 0, 0, 0, { HOMING(0x300, 3, 1, 0, 540), STUNP(0x180, 20, MF_PROJ, 480), XFORM(40, TS_GOTENKS_SSJ3, 10, 6) } },   // ghosts, Galactic Donut
    [TF_UUB]      = { 0, 0, 0, 0, { VOLLEY(0x1C0, 6, 6, MF_KI, 420), RUSH(0x300, 80, 3, 0, 300) } },
    [TF_VEGITO]   = { KF_POISE, 0, 0, 0, { BEAMC(0x600, 64, 40, 0, 480), RUSH(0x480, 32, 2, 0, 300), BALL(0x500, 1, 38, 0, 420), TELE(0x380, 0, 480) } },
};

// Library defaults (test command 27 forcing a kind on any fighter).
static const TMove lib_default[MK_COUNT] = {
    [MK_BEAM] = BEAM(0x400, 40, 0, 0), [MK_BALL] = BALL(0x400, 1, 32, 0, 0), [MK_HOMING] = HOMING(0x300, 2, 0, 0, 0),
    [MK_VOLLEY] = VOLLEY(0x180, 5, 6, 0, 0), [MK_WAVE] = WAVE(0x200, 0), [MK_NOVA] = NOVA(0x300, 44, 8, 0, 0),
    [MK_RUSH] = RUSH(0x300, 80, 3, 0, 0), [MK_TELEPORT] = TELE(0x380, 0, 0), [MK_STUN] = STUN(22, 72, 0, 0),
    [MK_HEAL] = HEAL(25, 0), [MK_DRAIN] = DRAIN(40, 12), [MK_REGEN] = REGEN(10),
    [MK_TRANSFORM] = XFORM(100, 0, 10, 6), [MK_SUMMON] = SUMMON(2, TF_CELL_JR, 0, 0),
};
static const TMove lib_ball_big = BALL(0x500, 2, 12, MF_HOME, 0);
static const TMove lib_stun_proj = STUNP(0x200, 45, MF_PROJ, 0);
static const TMove lib_beam_stun = BEAM(0x200, 32, MF_THIN | MF_STUN, 0);
static const TMove lib_self_ko = NOVA(0x700, 56, 8, MF_SELFKO, 0);
static const TMove lib_summon_self = SUMMON(1, SELF, 0, 0);

// ---- state ---------------------------------------------------------------------------------------------------------
struct TKit;
typedef struct Proj {
    u32 *vtable;          // +0x00 (the ROM projectile layout up to +0x40, as android16.c)
    s32 sort;
    u8 prio, _p0[3];
    u16 tile, size;
    const Desc *loaded;
    u8 owner_facing, _p1[3];
    const Desc *desc;     // +0x18
    Entity *owner;        // +0x1C
    s32 x, y;             // +0x20 8.8
    s32 box[4];
    s32 sx, sy;
    struct TKit *kit;     // +0x40 0 = orphan (the fighter is gone): no damage
    u8 kind, art, size_class, flags;
    s16 timer, fuse;
    s32 vx, vy, speed;
    Entity *target, *follow;
    s16 turn, radius, reach, trail_t;
    s16 home_at, stun;    // frames before steering starts; frames of stun on hit
    s32 mult;
    const u32 *attack;
    u32 vt[24];
} Proj;
enum { K_SHOT = 1, K_HELD, K_TRAIL, K_BOOM, K_FX, K_FLASH };
enum { ART_ORB_S, ART_ORB_M, ART_ORB_L, ART_KI, ART_MISSILE };
#define PF_STUN 1
#define PF_DRAIN 2

typedef struct CtrlObj { u32 *vtable; } CtrlObj;

typedef struct TKit {
    u32 vt[48];           // first: kit_of(e) = (TKit *)e->vtable
    u32 ctrl_vt[12];      // control state while a special plays its own frames (android16.c)
    u32 beam_vt[BEAM_VT_SLOTS];
    CtrlObj ctrl;
    const u32 *base;      // the vtable he had
    Entity *e;
    const TKitDef *def;
    EnemyStat stat;       // private copy (e->behaviour)
    const TMove *cur;     // the special in progress (0 = none)
    u8 fighter, team, copy, native;
    u8 step, done, group, hit_mask;   // done: MF_TRIGGER moves fired (bit per move)
    s16 timer, gcd, poise, stun;
    s16 cd[KIT_MOVES];
    s16 fired, regen_t, retarget, beam_left;
    s16 rush_left, _p;
    Entity *target;       // the special's target
    Entity *rtarget;      // the Rumble target (re-rolled)
    Entity *last_hitter;  // the last accepted hit's player or fighter (tny_moves_last_hitter)
    const TMove *extra;   // test command 27 op 5: a passive from the library on top of his kit
    Beam *beam;
    Proj *held;
    Proj *live[KIT_LIVE];
} TKit;

typedef struct MvGlobal {
    u32 magic;
    u8 rumble, force_move, force_reg, team_seq;   // force_move: 1-6 kit slot, 0x80 | kind library kind
    TKit *reg[KIT_MAX];
    Entity *stun_e[2];    // stunned players (their pads are dead while stun_t runs)
    s16 stun_t[2];
    u16 specials, fired, hits, rumble_hits;
    u16 npc_kos, transforms, summons, stuns;
    u16 drains, attached;
    TKit *dying;          // set by a fighter's destroy (a hit that killed him: do not touch his state after)
    u32 spawn_frame;
    u16 spawn_fail, vram_min;   // test command 26: 1 no player, 2 no art, 3 no stat / entity, 4 body lacks the AI set's groups; least free OBJ VRAM seen
} MvGlobal;
_Static_assert(sizeof(MvGlobal) <= 0x80, ".tny.mv is 0x80 bytes");
static MvGlobal mv TNY_SECTION("mv");

static void mv_init(void) {
    if (mv.magic == TNY_MV_MAGIC) return;
    memset(&mv, 0, sizeof mv);
    mv.magic = TNY_MV_MAGIC;
}

static TKit *kit_of(const Entity *e) {
    if (!e || mv.magic != TNY_MV_MAGIC) return 0;
    for (int i = 0; i < KIT_MAX; i++) if (mv.reg[i] && e->vtable == mv.reg[i]->vt) return mv.reg[i];
    return 0;
}
// enemies.c: the vtable a kit fighter had (is_coop_enemy, is_enemy keep counting him), 0 for others
const u32 *tny_moves_base(const Entity *e) { TKit *k = kit_of(e); return k ? k->base : 0; }
Entity *tny_moves_last_hitter(const Entity *e) { TKit *k = kit_of(e); return k ? k->last_hitter : 0; }

static int base_call9(TKit *k, Entity *e, int msg, u8 *m) {
    const u32 *vt = k->base;
    return ((int (*)(Entity *, int, u8 *))((u32)vt + vt[9]))(e, msg, m);
}

// ---- helpers ---------------------------------------------------------------------------------------------------------
static u32 isqrt(u32 v) {
    u32 r = 0, bit = 1u << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; } else r >>= 1;
        bit >>= 2;
    }
    return r;
}
static void set_len(s32 dx, s32 dy, s32 len, s32 *ox, s32 *oy) {
    while (dx > 30000 || dx < -30000 || dy > 30000 || dy < -30000) { dx >>= 1; dy >>= 1; }
    s32 d = (s32)isqrt((u32)(dx * dx + dy * dy));
    if (d == 0) { *ox = len; *oy = 0; return; }
    *ox = dx * len / d;
    *oy = dy * len / d;
}
static void centre(const Entity *e, s32 *x, s32 *y) {
    s32 b[4];
    Entity_GetBox(b, e);
    *x = (b[0] + b[2]) >> 1;
    *y = (b[1] + b[3]) >> 1;
}
static s32 dist2(const Entity *a, const Entity *b) {
    s32 ax, ay, bx, by;
    centre(a, &ax, &ay);
    centre(b, &bx, &by);
    return (ax - bx) * (ax - bx) + (ay - by) * (ay - by);
}
static int facing_to(s32 dx, s32 dy) {
    s32 ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    if (ax > ay) return dx < 0 ? 2 : 3;
    return dy < 0 ? 1 : 0;
}
static int octant(s32 vx, s32 vy) {
    s32 ax = vx < 0 ? -vx : vx, ay = vy < 0 ? -vy : vy;
    if (ay * 5 < ax * 2) return vx < 0 ? 4 : 0;
    if (ax * 5 < ay * 2) return vy < 0 ? 6 : 2;
    if (vx > 0) return vy > 0 ? 1 : 7;
    return vy > 0 ? 3 : 5;
}
static void face(Entity *e, const Entity *t) {
    s32 ex, ey, tx, ty;
    if (!t) return;
    centre(e, &ex, &ey);
    centre(t, &tx, &ty);
    e->facing = (u8)facing_to(tx - ex, ty - ey);
}
static void sfx(int id) { Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + id * 12, 0x40, 0); }
#define SFX_LAUNCH  0x0D
#define SFX_BOOM    0x14
static void shake(int f, int s) { camera_shake_safe(f, s); }

static Entity *real_player(int i) {
    Entity *p = i == 0 ? (coop2.p1_frame ? coop2.p1_frame : g_PlayerObject) : coop.p2;
    return (p && !revive_is_down(p)) ? p : 0;
}
static int is_player(const Entity *e) { return e && (e == real_player(0) || e == real_player(1) || e == g_PlayerObject || e == coop.p2); }
static int alive_kit(const TKit *k) { return k && k->e && k->e->hp > 0; }

// a fighter of another side (Rumble), alive
static int foe_kit(const TKit *k, const TKit *o) { return o && o != k && alive_kit(o) && o->team != k->team; }

static Entity *nearest_player_of(const Entity *e) {
    Entity *p1 = real_player(0), *p2 = real_player(1);
    if (!p1) return p2;
    if (!p2) return p1;
    return dist2(e, p2) < dist2(e, p1) ? p2 : p1;
}
static Entity *nearest_foe(const TKit *k) {
    Entity *best = 0;
    s32 bd = 0x7FFFFFFF;
    for (int i = 0; i < KIT_MAX; i++) {
        TKit *o = mv.reg[i];
        if (!foe_kit(k, o)) continue;
        s32 d = dist2(k->e, o->e);
        if (d < bd) { bd = d; best = o->e; }
    }
    return best;
}

// The target of this frame: the stock rule (g_PlayerObject as entities.c set it) outside the Rumble.
static Entity *rumble_target(TKit *k) {
    Entity *pl = nearest_player_of(k->e);
    if (pl && dist2(k->e, pl) <= RUMBLE_CLOSE * RUMBLE_CLOSE) return pl;
    TKit *rt = kit_of(k->rtarget);
    int valid = k->rtarget && (k->rtarget == pl || (rt && foe_kit(k, rt)) || is_player(k->rtarget));
    if (k->retarget <= 0 || !valid) {
        k->retarget = RUMBLE_REROLL;
        Entity *f = nearest_foe(k);
        k->rtarget = (f && (!pl || Rand_Range(100) >= RUMBLE_PLAYER)) ? f : pl;
    }
    return k->rtarget ? k->rtarget : pl;
}

// ---- sprite groups (any body: a stock enemy set, a player layout, a giant) -------------------------------------------
static const u32 *group_of(const Entity *e, int g) {
    const u8 *rec = (const u8 *)e->sprite_record;
    if (!rec || g >= rec[2]) return 0;
    const u32 *grp = (const u32 *)(rec + 0x4C + 0x10 * g);
    return (grp[0] && grp[1] && grp[2] && grp[3]) ? grp : 0;
}
// A frame descriptor the game can draw: an OBJ size and a blob whose header kind is 0-2 (tools/sprites.py's rule).
static int desc_ok(const Desc *d) {
    u32 b = (u32)d->blob;
    if (b < 0x08000000u || b >= 0x0A000000u || (b & 3)) return 0;
    if (!(d->w == 8 || d->w == 16 || d->w == 32 || d->w == 64)) return 0;
    if (!(d->h == 8 || d->h == 16 || d->h == 32 || d->h == 64)) return 0;
    return *(const u32 *)b <= 2;
}
static int frame_ok(const Entity *e, const u32 *grp, int f) {
    const Desc *d = (const Desc *)grp[e->facing & 3];
    for (int i = 0; i <= f; i++) if (!desc_ok(&d[i])) return 0;
    return 1;
}
static int pick_group(const Entity *e, int a, int b, int c) {
    if (group_of(e, a)) return a;
    if (group_of(e, b)) return b;
    return c;
}
static void pose(TKit *k, int g, int f) {
    Entity *e = k->e;
    const u32 *grp = group_of(e, g);
    if (!grp) { g = 0; grp = group_of(e, 0); if (!grp) return; }
    if (!frame_ok(e, grp, f)) f = 0;
    k->group = (u8)g;
    E32(e, 0x4C) = (u32)grp;
    E16(e, 0x14) = (u16)f;
}
#define G_CHARGE(k) pick_group((k)->e, 5, 18, 0)
#define G_BEAM(k)   pick_group((k)->e, 18, 5, 0)
#define G_SWING(k)  pick_group((k)->e, 6, 7, 0)
#define G_HURT(k)   pick_group((k)->e, 2, 0, 0)

static void ctrl_enter(CtrlObj *s, Entity *e) { (void)s; E16(e, 0x12) = 0; E8(e, 8) = 0x28; }
static void ctrl_nop(CtrlObj *s, Entity *e) { (void)s; (void)e; }

// ---- hits -------------------------------------------------------------------------------------------------------------
// Who a blast can touch: players always; in the Rumble the fighters of other sides too.
typedef struct Victims { Entity *e[2 + KIT_MAX]; int n; } Victims;
static void victims(const TKit *k, Victims *v) {
    v->n = 0;
    for (int i = 0; i < 2; i++) { Entity *p = real_player(i); if (p) v->e[v->n++] = p; }
    if (!mv.rumble || !k) return;
    for (int i = 0; i < KIT_MAX; i++) if (foe_kit(k, mv.reg[i])) v->e[v->n++] = mv.reg[i]->e;
}
static int box_hits(const Entity *t, s32 x1, s32 y1, s32 x2, s32 y2) {
    s32 b[4];
    Entity_GetBox(b, t);
    return x1 < b[2] && b[0] < x2 && y1 < b[3] && b[1] < y2;
}

static void fx_on(Entity *t, int frames);
static void stun_entity(Entity *t, int frames) {
    if (!t || frames <= 0) return;
    TKit *o = kit_of(t);
    int fresh = 0;
    if (o) {
        if (o->cur) return;                        // a special cannot be stunned out of
        fresh = o->stun <= 0;
        if (o->stun < frames) o->stun = (s16)frames;
    } else {
        for (int i = 0; i < 2; i++) {
            if (t != real_player(i)) continue;
            fresh = mv.stun_t[i] <= 0 || mv.stun_e[i] != t;
            mv.stun_e[i] = t;
            if (mv.stun_t[i] < frames || fresh) mv.stun_t[i] = (s16)frames;
        }
    }
    if (!fresh) return;
    mv.stuns++;
    fx_on(t, frames);                               // one ring of orbs over his head while it lasts
}

static CharacterEntry *entry_of(const Entity *p) {
    if (p == real_player(0)) return &g_Party->chars[g_Party->active_slot];
    if (p == coop.p2) return &g_Party->chars[coop.p2_slot];
    return 0;
}

// A hit landed on t (damage dmg): drain moves (HP to him, EP from a player).
static void on_hit_effects(TKit *k, Entity *t, s32 dmg) {
    if (!alive_kit(k)) return;
    for (int i = 0; i <= KIT_MOVES; i++) {
        const TMove *m = i < KIT_MOVES ? &k->def->m[i] : k->extra;
        if (!m || m->kind != MK_DRAIN) continue;
        Entity *e = k->e;
        if (m->a && dmg > 0) {
            s32 hp = e->hp + dmg * m->a / 100;
            e->hp = hp > k->stat.max_hp ? k->stat.max_hp : hp;
        }
        if (m->b) {
            CharacterEntry *c = entry_of(t);
            if (c) c->ep = (u16)((s16)c->ep > m->b ? c->ep - m->b : 0);
        }
        mv.drains++;
    }
}

// Broadcast a type-0 hit (players; the Rumble's other sides through kit_message). Effects go to the victims
// whose box overlaps when someone took it.
static int blast(TKit *k, s32 x, s32 y, int rx, int ry, const u32 *attack, s32 mult, int stun, int effects) {
    if (!alive_kit(k)) return 0;
    Entity *e = k->e;
    HitMsg m;
    memset(&m, 0, sizeof m);
    m.box[0] = x - rx; m.box[1] = y - ry; m.box[2] = x + rx; m.box[3] = y + ry;
    m.facing = e->facing;
    m.mult = mult;
    m.attack = attack;
    m.attacker = e;
    m.source = e;
    u16 calls = coop2.dmg_calls;
    TKit *keep = k;
    mv.dying = 0;
    u32 r = EntityList_Broadcast(g_SceneTable[1], 0, &m);
    if (mv.dying == keep) return r != 0;            // he is gone (a self-destruct): nothing more to do
    if (!r) return 0;
    mv.hits++;
    if (!stun && !effects) return 1;
    Victims v;
    victims(k, &v);
    for (int i = 0; i < v.n; i++) {
        if (!box_hits(v.e[i], m.box[0], m.box[1], m.box[2], m.box[3])) continue;
        if (stun) stun_entity(v.e[i], stun);
        if (effects && coop2.dmg_calls != calls) on_hit_effects(k, v.e[i], coop2.last_dmg);
    }
    return 1;
}

// ---- projectiles ---------------------------------------------------------------------------------------------------
static void proj_update(Proj *p);
static void track(TKit *k, Proj *p) {
    p->kit = k;
    if (!k) return;
    for (int i = 0; i < KIT_LIVE; i++) if (!k->live[i]) { k->live[i] = p; return; }
}
static void untrack(Proj *p) {
    TKit *k = p->kit;
    if (!k) return;
    for (int i = 0; i < KIT_LIVE; i++) if (k->live[i] == p) k->live[i] = 0;
    if (k->held == p) k->held = 0;
    p->kit = 0;
}
static void proj_destroy(Proj *p, int free_it, u32 a, u32 b) {
    untrack(p);
    ((void (*)(Proj *, int, u32, u32))PROJ_DESTROY)(p, free_it, a, b);
}
static const Desc *art_frame(const Proj *p) {
    switch (p->art) {
    case ART_ORB_S: return &s13_orb_s[(p->timer >> 2) % 3];
    case ART_ORB_M: return &s13_orb_m[(p->timer >> 2) & 1];
    case ART_ORB_L: return &s13_orb_l[(p->timer >> 2) & 1];
    case ART_KI: {
        static const u8 dir_of[8] = { 3, 0, 0, 2, 2, 1, 1, 3 };   // octant -> the blast's facing (down, up, left, right)
        return &kr_kiblast[dir_of[octant(p->vx, p->vy)] * 4 + ((p->timer >> 2) & 3)];
    }
    case ART_MISSILE: return &a16_missile[octant(p->vx, p->vy) * 2 + ((p->timer >> 2) & 1)];
    default: return &a16_boom[0];
    }
}
static Proj *proj_new(TKit *k, int kind, int art, s32 x, s32 y) {
    static const u16 vram_of[5] = { 0x100, 0x400, 0x1000, 0x100, 0x100 };
    int vram = kind == K_BOOM ? 0x400 : vram_of[art];
    if (ObjVram_Largest(g_EntityList) < vram + 0x100) return 0;   // keep room for the bodies
    Proj *p = Arena_Alloc(sizeof *p);
    if (!p) return 0;
    memset(p, 0, sizeof *p);
    ProjBase_Init(p, vram, k ? k->e : 0);
    u32 base = (u32)p->vt;
    const u32 *orig = (const u32 *)PROJ_VTABLE;
    for (int i = 0; i < 24; i++) p->vt[i] = (PROJ_VTABLE + orig[i]) - base;
    p->vt[0] = (u32)proj_destroy - base;
    p->vt[2] = (u32)proj_update - base;
    p->vt[9] = PROJ_MESSAGE - base;
    p->vt[10] = (u32)proj_update - base;
    p->vtable = p->vt;
    p->kind = (u8)kind;
    p->art = (u8)art;
    p->x = x << 8;
    p->y = y << 8;
    p->desc = kind == K_BOOM ? &a16_boom[0] : art_frame(p);
    p->prio = 0x28;
    ProjBase_UpdateBox(p);
    if (k) track(k, p);
    EntityList_Add(g_SceneTable[1], (Entity *)p);
    return p;
}
static void proj_remove(Proj *p) {
    untrack(p);
    EntityList_Remove(g_SceneTable[1], p, 0);
}
static void boom(s32 x, s32 y) { proj_new(0, K_BOOM, ART_ORB_S, x, y); }
static void flash(s32 x, s32 y, int art, int frames) {
    Proj *p = proj_new(0, K_FLASH, art, x, y);
    if (p) p->fuse = (s16)frames;
}
static void fx_on(Entity *t, int frames) {
    s32 x, y;
    centre(t, &x, &y);
    Proj *p = proj_new(0, K_FX, ART_ORB_S, x, y - 20);
    if (!p) return;
    p->follow = t;
    p->fuse = (s16)(frames < 240 ? frames : 240);
}

static void explode(Proj *p, int damage) {
    s32 x = p->x >> 8, y = p->y >> 8;
    TKit *k = damage ? p->kit : 0;
    boom(x, y);
    if (p->size_class == 2) {
        boom(x - 16, y + 8);
        boom(x + 16, y + 8);
        boom(x, y - 14);
        shake(24, 100);
    } else if (p->size_class == 1) {
        boom(x - 10, y + 6);
        boom(x + 10, y + 6);
        shake(12, 70);
    } else {
        shake(6, 40);
    }
    sfx(SFX_BOOM + Rand_Range(2));
    blast(k, x, y, p->radius, p->radius, p->attack, p->mult, (p->flags & PF_STUN) ? p->stun : 0, 0);
    proj_remove(p);
}

static int touches_victim(const Proj *p) {
    s32 x = p->x >> 8, y = p->y >> 8, r = p->reach;
    Victims v;
    victims(p->kit, &v);
    for (int i = 0; i < v.n; i++) {
        if (v.e[i] == (p->kit ? p->kit->e : 0)) continue;
        if (box_hits(v.e[i], x - r, y - r - 6, x + r, y + r)) return 1;
    }
    return 0;
}

static void spawn_trail(Proj *head) {
    TKit *k = head->kit;
    int n = 0;
    for (int i = 0; i < KIT_LIVE; i++) if (k->live[i] && k->live[i]->kind == K_TRAIL) n++;
    if (n >= 6) return;
    Proj *t = proj_new(k, K_TRAIL, ART_ORB_S, head->x >> 8, head->y >> 8);
    if (!t) return;
    t->fuse = 44;
    t->radius = 9;
    t->reach = 2;
    t->mult = head->mult / 3;
    t->attack = atk_ki;
}

static void proj_update(Proj *p) {
    p->timer++;
    if (p->kind == K_BOOM) {
        int f = p->timer < 4 ? 0 : 1 + (p->timer - 4) / 6;
        if (f > 3) { proj_remove(p); return; }
        p->desc = &a16_boom[f];
        ProjBase_UpdateBox(p);
        return;
    }
    if (p->kind == K_FX || p->kind == K_FLASH) {   // stun stars over a head / a flash: no damage
        if (--p->fuse <= 0) { proj_remove(p); return; }
        if (p->kind == K_FX && p->follow) {
            if (!is_player(p->follow) && !kit_of(p->follow)) { proj_remove(p); return; }   // he is gone
            s32 x, y;
            centre(p->follow, &x, &y);
            static const s8 orbit[8][2] = { { 8, 0 }, { 6, 3 }, { 0, 4 }, { -6, 3 }, { -8, 0 }, { -6, -3 }, { 0, -4 }, { 6, -3 } };
            int o = (p->timer >> 2) & 7;
            p->x = (x + orbit[o][0]) << 8;
            p->y = (y - 22 + orbit[o][1]) << 8;
        }
        p->desc = art_frame(p);
        ProjBase_UpdateBox(p);
        return;
    }
    int alive = alive_kit(p->kit);
    if (!alive) { explode(p, 0); return; }
    if (p->kind == K_HELD) {                       // growing over his head: the fighter places it
        p->desc = art_frame(p);
        ProjBase_UpdateBox(p);
        return;
    }
    if (p->kind == K_TRAIL) {
        p->desc = &s13_trail[(p->timer >> 2) & 1];
        ProjBase_UpdateBox(p);
        if (touches_victim(p)) { explode(p, 1); return; }
        if (--p->fuse <= 0) proj_remove(p);
        return;
    }
    if (p->target && p->turn && p->timer >= p->home_at) {   // partial homing
        int ok = is_player(p->target) || alive_kit(kit_of(p->target));
        if (!ok) p->target = 0;
        else {
            s32 tx, ty, dx, dy;
            centre(p->target, &tx, &ty);
            set_len((tx << 8) - p->x, (ty << 8) - p->y, p->speed, &dx, &dy);
            p->vx += (dx - p->vx) / p->turn;
            p->vy += (dy - p->vy) / p->turn;
            set_len(p->vx, p->vy, p->speed, &p->vx, &p->vy);
        }
    }
    p->x += p->vx;
    p->y += p->vy;
    p->desc = art_frame(p);
    ProjBase_UpdateBox(p);
    if (p->trail_t >= 0 && p->flags & 0x80) {     // a wave's head leaves a trail
        if (++p->trail_t >= 7) { p->trail_t = 0; spawn_trail(p); }
    }
    if (--p->fuse <= 0 || touches_victim(p)) explode(p, 1);
}

static void launch_point(const Entity *e, s32 *x, s32 *y) {
    static const s8 off[4][2] = { { 0, 8 }, { 0, -12 }, { -14, -2 }, { 14, -2 } };
    centre(e, x, y);
    *x += off[e->facing & 3][0];
    *y += off[e->facing & 3][1];
}

static Proj *fire(TKit *k, int art, int size_class, Entity *t, s32 ax, s32 ay, s32 speed, int turn, int fuse,
                  int radius, s32 mult, const u32 *attack) {
    s32 x, y;
    launch_point(k->e, &x, &y);
    Proj *p = proj_new(k, K_SHOT, art, x, y);
    if (!p && art == ART_ORB_L) { art = ART_ORB_M; size_class = 1; p = proj_new(k, K_SHOT, art, x, y); }
    if (!p) return 0;
    p->size_class = (u8)size_class;
    set_len(ax, ay, speed, &p->vx, &p->vy);
    p->speed = speed;
    p->turn = (s16)turn;
    p->fuse = (s16)fuse;
    p->target = t;
    p->radius = (s16)radius;
    p->reach = (s16)(size_class == 2 ? 18 : size_class == 1 ? 9 : 3);
    p->mult = mult;
    p->attack = attack;
    p->trail_t = -1;
    mv.fired++;
    sfx(SFX_LAUNCH + Rand_Range(2));
    return p;
}
static Proj *fire_at(TKit *k, int art, int size_class, Entity *t, s32 speed, int turn, int fuse, int radius, s32 mult, const u32 *atk) {
    s32 x, y, tx, ty;
    launch_point(k->e, &x, &y);
    centre(t, &tx, &ty);
    return fire(k, art, size_class, t, tx - x, ty - y, speed, turn, fuse, radius, mult, atk);
}

// ---- beams ----------------------------------------------------------------------------------------------------------
// A thin beam's length is capped in its own vtable copy's update (cellgames.c's capped beams): the drawing reads
// +0x34, so it never shows longer.
static void kit_beam_update(Beam *b, int run) {
    ((void (*)(Beam *, int))BEAM_UPDATE)(b, run);
    if (b->state == 1 && b->len > BEAM_THIN_LEN) { b->len = BEAM_THIN_LEN; b->impact = 1; }
}
static void beam_end(TKit *k) {
    if (!k->beam) return;
    Beam_End(k->beam);
    k->beam = 0;
}
static void beam_fire(TKit *k, const TMove *m) {
    Entity *e = k->e;
    s32 box[4], pos[2];
    Entity_GetBox(box, e);
    pos[0] = (box[0] + box[2]) >> 1;
    pos[1] = (box[1] + box[3]) >> 1;
    Beam *b = Beam_Init(0, pos, 0, e);
    if (!b) return;
    b->mult = m->power;
    if (m->flags & MF_THIN) {
        const s32 *vt = (const s32 *)BEAM_VTABLE;
        for (int i = 0; i < BEAM_VT_SLOTS; i++) k->beam_vt[i] = BEAM_VTABLE + vt[i] - (u32)k->beam_vt;
        k->beam_vt[10] = (u32)kit_beam_update - (u32)k->beam_vt;
        b->vtable = k->beam_vt;
    }
    EntityList_Add(g_EntityList, (Entity *)b);
    int f = e->facing & 3;
    Entity_BeamPoint(pos, e);
    pos[0] += BEAM_OFFSETS[f * 2];
    pos[1] += BEAM_OFFSETS[f * 2 + 1];
    Beam_Fire(b, f, pos);
    k->beam = b;
    mv.fired++;
    if (m->flags & MF_COST) {                      // Tri-Beam: it costs him
        s32 cost = k->stat.max_hp / 20;
        e->hp = e->hp > cost ? e->hp - cost : 1;
    }
}
// Candy Beam: whoever the beam's length touches is frozen (once per beam)
static void beam_stun(TKit *k, int frames) {
    Beam *b = k->beam;
    if (!b || b->state != 1) return;
    s32 x1 = b->x, y1 = b->y, x2 = b->x + b->dx * b->len, y2 = b->y + b->dy * b->len;
    if (x2 < x1) { s32 t = x1; x1 = x2; x2 = t; }
    if (y2 < y1) { s32 t = y1; y1 = y2; y2 = t; }
    Victims v;
    victims(k, &v);
    for (int i = 0; i < v.n && i < 8; i++) {
        if (k->hit_mask & (1 << i)) continue;
        if (!box_hits(v.e[i], x1 - 6, y1 - 6, x2 + 6, y2 + 6)) continue;
        k->hit_mask |= (u8)(1 << i);
        stun_entity(v.e[i], frames);
    }
}

// ---- spawning (test command 26, summons) -----------------------------------------------------------------------------
// The stat a fighter spawns with outside the engine: the engine's row (TNY_STAT0 + id, when that build has it), else
// the stock stat of his body (a stand-in for tests and summons), else Yamcha's set-21 row.
static const u8 stand_in_stat[TF_COUNT] = {
    [TF_YAMCHA] = 106, [TF_TIEN] = 98, [TF_FRIEZA] = 41, [TF_MECHA_FRIEZA] = 41, [TF_HERCULE] = 64, [TF_TAO] = 44,
    [TF_A19] = 5, [TF_GERO] = 6, [TF_A13] = 110, [TF_A17] = 3, [TF_A18] = 4, [TF_IMPERFECT_CELL] = 60, [TF_A16] = 109,
    [TF_SEMI_CELL] = 76, [TF_PERFECT_CELL] = 78, [TF_CELL_JR] = 26, [TF_COOLER] = 29, [TF_NAMEKIAN] = 113,
    [TF_BROLY] = 115, [TF_PARAGUS] = 116, [TF_SUPER_PERFECT_CELL] = 78, [TF_SUPER13] = 145, [TF_BHERCULE] = 64,
};
// Pose groups an AI set plays (docs/sprites.md "rules for new enemy models": the groups every stock sprite of
// that set has). A body missing one crashes at the first move that plays it (Mecha Frieza's sprite 49 has only
// 0, 1, 3, 5: under Frieza's set 21 the first swing jumps to C002E560).
static u32 groups_needed(int ai_set) {
    switch (ai_set) {
    case 21:  return 0x1E7;                       // 0-2, 5-8
    case 29:  return 0x11E7;                      // + 12
    case 45: case 52: return 0x1C7;               // 0-2, 6-8
    case 91:  return 0xC7;                        // 0-2, 6, 7
    case 80: case 116: return 0xC17EF;            // 0-3, 5-10, 12, 18, 19
    case 98:  return 0x107F;                      // 0-6, 12
    case 129: return 0x67;                        // 0-2, 5, 6
    case 138: case 148: return 0x411EF;           // 0-3, 5-8, 12, 18
    default:  return 0x47;                        // 0-2, 6 (regular sets)
    }
}
static int record_fits(const u8 *rec, u32 need) {
    if (!rec) return 0;
    for (int g = 0; g < 32; g++) {
        if (!(need & (1u << g))) continue;
        if (g >= rec[2]) return 0;
        const u32 *grp = (const u32 *)(rec + 0x4C + 0x10 * g);
        if (!grp[0] || !grp[1] || !grp[2] || !grp[3]) return 0;
    }
    return 1;
}
// The engine may ask before spawning: does this body have every pose group that AI set plays?
int tny_moves_body_fits(int sprite, int ai_set) {
    return sprite > 0 && record_fits((const u8 *)Character_GetSpriteRecord(sprite), groups_needed(ai_set));
}
// A transform's body for this AI set: the sprite itself, or the play body of the fighter shown by that sprite
// (Frieza -> Mecha Frieza: sprite 49 lacks the attack groups, the playables' 238 has them); 0 = keep his body.
static int transform_body(int sprite, int ai_set) {
    if (tny_moves_body_fits(sprite, ai_set)) return sprite;
    for (int i = 0; i < TF_COUNT; i++)
        if (tny_fighters[i].sprite == sprite && tny_fighters[i].play_sprite && tny_moves_body_fits(tny_fighters[i].play_sprite, ai_set))
            return tny_fighters[i].play_sprite;
    return 0;
}

static int stat_for(int id) {
    EnemyStat tmp;
    if (resolve_stat(TNY_STAT0 + id, &tmp)) return TNY_STAT0 + id;
    return stand_in_stat[id] ? stand_in_stat[id] : 106;
}

static Entity *spawn_fighter(int id, s32 x, s32 y, int stat, int team) {
    static const u32 face_wait[2] = { 0x0800CB0Fu, 0x00027D00u };   // NpcBehavior_FaceAndWait
    static const u32 no_action[2] = { ENEMY_ACTION_NONE, 0 };
    if (!tny_fighter_ok(id)) { mv.spawn_fail = 2; return 0; }
    if (!stat) stat = stat_for(id);
    EnemyStat st;
    if (!resolve_stat(stat, &st) && stat != A16_STAT && stat != S13_STAT) { mv.spawn_fail = 3; return 0; }
    int sprite = tny_fighters[id].sprite;
    if (stat != A16_STAT && stat != S13_STAT && !tny_moves_body_fits(sprite, st.ai_set)) {
        sprite = tny_fighters[id].play_sprite;     // his play body, when that build has it
        if (!tny_moves_body_fits(sprite, st.ai_set)) { mv.spawn_fail = 4; return 0; }
    }
    struct { EnemyRecord r; const void *beh[1]; } rec;
    memset(&rec, 0, sizeof rec);
    rec.r.handler = (void *)0x0800E77Fu;
    rec.r.stat_index = stat;
    rec.r.x = (s16)x;
    rec.r.y = (s16)y;
    rec.r.sprite_id = sprite;
    rec.r.action = (void *)no_action;
    rec.r.behaviour_count = 1;
    rec.beh[0] = face_wait;
    u16 off = coop2.off;
    coop2.off |= COOP_OFF_TWINS;                   // never twin a tournament fighter
    Entity *e = enemy_spawn_hook(&rec.r);
    coop2.off = off;
    if (!e) { mv.spawn_fail = 3; return 0; }
    EntityList_Add(g_SceneTable[1], e);
    tny_moves_attach(e, id);
    TKit *k = kit_of(e);
    if (k && team) k->team = (u8)team;
    return e;
}

// ---- the specials ----------------------------------------------------------------------------------------------------
static void finish(TKit *k) {
    Entity *e = k->e;
    beam_end(k);
    if (k->held) { k->held->kind = K_SHOT; k->held->fuse = 1; k->held = 0; }   // fizzles
    k->cur = 0;
    k->hit_mask = 0;
    int pace = k->def->pace ? k->def->pace * 2 : 150;
    k->gcd = (s16)(pace + Rand_Range(pace / 2));
    if (e->ctrl_state == &k->ctrl) Entity_SetAnimation(e, 0);
}

static int foe_ok(const TKit *k, const Entity *t) {
    if (!t) return 0;
    if (is_player(t)) return !revive_is_down(t);
    return mv.rumble && foe_kit(k, kit_of(t));
}

// Stun everybody of the other side within r px of him (Solar Flare)
static void stun_around(TKit *k, int r, int frames) {
    Victims v;
    victims(k, &v);
    for (int i = 0; i < v.n; i++) if (dist2(k->e, v.e[i]) <= r * r) stun_entity(v.e[i], frames);
}

static int walkable(Entity *e, s32 x, s32 y) {
    s32 px[2] = { x, y };
    return x > 8 && y > 8 && Collision_Check(COLLISION_MAP, px, (u8 *)e + 0x154);
}

static void melee_burst(TKit *k, const u32 *atk, s32 mult, int stun) {
    Entity *e = k->e;
    static const s8 vec[4][2] = { { 0, 1 }, { 0, -1 }, { -1, 0 }, { 1, 0 } };
    s32 x, y;
    centre(e, &x, &y);
    x += vec[e->facing & 3][0] * 14;
    y += vec[e->facing & 3][1] * 14;
    blast(k, x, y, 16, 16, atk, mult, stun, 1);
}

static void heal_side(TKit *k, int pct) {
    TKit *best = k;
    s32 best_pct = k->e->hp * 100 / (k->stat.max_hp ? k->stat.max_hp : 1);
    for (int i = 0; i < KIT_MAX; i++) {
        TKit *o = mv.reg[i];
        if (!alive_kit(o) || o == k || (mv.rumble && o->team != k->team)) continue;
        s32 p = o->e->hp * 100 / (o->stat.max_hp ? o->stat.max_hp : 1);
        if (p < best_pct) { best = o; best_pct = p; }
    }
    s32 hp = best->e->hp + best->stat.max_hp * pct / 100;
    best->e->hp = hp > best->stat.max_hp ? best->stat.max_hp : hp;
    s32 x, y;
    centre(best->e, &x, &y);
    flash(x, y - 8, ART_ORB_M, 20);
}

static void summon(TKit *k, const TMove *m) {
    int who = m->b == SELF ? k->fighter : m->b;
    Entity *e = k->e;
    for (int i = 0; i < m->a; i++) {
        s32 x = (e->x >> 8) + (i & 1 ? -28 : 28), y = (e->y >> 8) + (i >> 1) * 20;
        if (!walkable(e, x, y)) x = e->x >> 8;
        Entity *h = spawn_fighter(who, x, y, 0, k->team);
        TKit *hk = kit_of(h);
        if (!hk) continue;
        hk->copy = 1;                                  // helpers and copies never summon or transform
        if (m->b == SELF) {                            // Multi-Form: the copy has half his health
            hk->stat.max_hp = k->stat.max_hp / 2;
            h->hp = hk->stat.max_hp;
            h->sprite_record = e->sprite_record;
        }
        s32 cx, cy;
        centre(h, &cx, &cy);
        flash(cx, cy, ART_ORB_M, 16);
        mv.summons++;
    }
}

static void transform(TKit *k, const TMove *m) {
    Entity *e = k->e;
    int body = m->power ? transform_body(m->power, k->stat.ai_set) : 0;
    if (body) e->sprite_record = Character_GetSpriteRecord(body);   // else a stat boost on his own body
    e->stat_melee = (u8)(e->stat_melee + m->a > 255 ? 255 : e->stat_melee + m->a);
    e->stat_ranged = (u8)(e->stat_ranged + m->a > 255 ? 255 : e->stat_ranged + m->a);
    e->defence = (u8)(e->defence + m->b > 120 ? 120 : e->defence + m->b);
    k->stat.melee = e->stat_melee;
    k->stat.ranged = e->stat_ranged;
    k->stat.defence = e->defence;
    mv.transforms++;
}

static void self_ko(TKit *k) {
    Entity *e = k->e;
    u8 m[0x40];
    memset(m, 0, sizeof m);
    *(Entity **)(m + 0x20) = e;
    *(Entity **)(m + 0x30) = e;
    *(s32 *)(m + 0x2C) = e->hp;
    mv.dying = 0;
    ((int (*)(Entity *, const u8 *))((u32)e->vtable + e->vtable[14]))(e, m + 0x24);
}

static void begin(TKit *k, const TMove *m, Entity *t) {
    k->cur = m;
    k->timer = 0;
    k->fired = 0;
    k->hit_mask = 0;
    k->target = t;
    k->rush_left = m->kind == MK_RUSH ? m->a : 0;
    face(k->e, t);
    Entity_SetCtrlState(k->e, &k->ctrl);
    pose(k, G_CHARGE(k), 0);
    mv.specials++;
}

// One frame of the special in progress (k->cur is cleared when it ends).
static void step(TKit *k) {
    const TMove *m = k->cur;
    Entity *e = k->e, *t = k->target;
    int tt = ++k->timer;
    if (!foe_ok(k, t) && m->kind != MK_TRANSFORM && m->kind != MK_SUMMON && m->kind != MK_HEAL && m->kind != MK_NOVA) {
        t = k->target = mv.rumble ? rumble_target(k) : nearest_player_of(e);
        if (!t) { finish(k); return; }
    }
    switch (m->kind) {
    case MK_BEAM: {
        int charge = m->b ? m->b : 24;
        int g = G_BEAM(k);
        if (tt < charge) { pose(k, g, 0); if (tt == charge / 2) face(e, t); break; }
        if (tt == charge) { face(e, t); pose(k, g, 1); beam_fire(k, m); if (!k->beam) { finish(k); break; } }
        pose(k, g, (tt >> 3) & 1 ? 2 : 1);
        if (m->flags & MF_STUN) beam_stun(k, 90);
        if (tt >= charge + m->a) finish(k);
        break;
    }
    case MK_BALL: {
        if (m->a < 2) {
            if (tt < 18) { pose(k, G_CHARGE(k), 0); break; }
            if (tt == 18) {
                face(e, t);
                pose(k, G_SWING(k), 0);
                fire_at(k, m->a ? ART_ORB_M : ART_ORB_S, m->a, t, m->b * 16, (m->flags & MF_HOME) ? 40 : 0, 110,
                        m->a ? 22 : 14, m->power, atk_heavy);
            }
            if (tt >= 36) finish(k);
            break;
        }
        // the large one: held over his head while it grows, then thrown slowly after the target
        if (tt < 8) { pose(k, G_CHARGE(k), 0); break; }
        s32 x, y, bx[4];
        centre(e, &x, &y);
        Entity_GetBox(bx, e);
        if (tt == 8) {
            int big = ObjVram_Largest(g_EntityList) >= 0x1100;
            k->held = proj_new(k, K_HELD, big ? ART_ORB_L : ART_ORB_M, x, bx[1] - 18);
            if (k->held) k->held->size_class = (u8)(big ? 2 : 1);
            shake(48, 30);
        }
        if (tt < 56) {
            pose(k, G_CHARGE(k), 1);
            if (k->held) { k->held->x = x << 8; k->held->y = (bx[1] - 18) << 8; }
            break;
        }
        if (tt == 56 && k->held) {
            Proj *p = k->held;
            k->held = 0;
            s32 tx, ty;
            centre(t, &tx, &ty);
            p->kind = K_SHOT;
            set_len((tx << 8) - p->x, (ty << 8) - p->y, m->b * 16, &p->vx, &p->vy);
            p->speed = m->b * 16;
            p->turn = (m->flags & MF_HOME) ? 18 : 0;
            p->fuse = 300;
            p->target = t;
            p->radius = p->size_class == 2 ? 38 : 26;
            p->reach = p->size_class == 2 ? 18 : 9;
            p->mult = m->power;
            p->attack = atk_heavy;
            p->trail_t = -1;
            mv.fired++;
            pose(k, G_SWING(k), 0);
            sfx(SFX_LAUNCH);
        }
        if (tt >= 80) finish(k);
        break;
    }
    case MK_HOMING: {   // charge 24, one shot every 10 frames
        pose(k, G_CHARGE(k), tt < 24 ? 0 : 1);
        if (tt >= 24 && ((tt - 24) % 10) == 0 && k->fired < m->a) {
            int art = (m->flags & MF_KI) ? ART_KI : m->b ? ART_ORB_M : ART_MISSILE;
            s32 x, y, tx, ty, ax, ay;
            launch_point(e, &x, &y);
            centre(t, &tx, &ty);
            set_len(tx - x, ty - y, 0x100, &ax, &ay);
            s32 kk = m->a > 1 ? (2 * k->fired - (m->a - 1)) * 0x50 / (m->a - 1) : 0;
            fire(k, art, art == ART_ORB_M, t, ax - (ay * kk >> 8), ay + (ax * kk >> 8), 0x1A0, 10, 180, 20, m->power, atk_ki);
            k->fired++;
        }
        if (k->fired >= m->a && tt >= 24 + 10 * m->a + 12) finish(k);
        break;
    }
    case MK_VOLLEY: {   // wind-up 12, then a shot every b frames with a small spread
        int gap = m->b ? m->b : 6;
        int s = tt - 12;
        if (s < 0) { pose(k, G_CHARGE(k), 0); break; }
        if (s % gap == 0 && k->fired < m->a) {
            face(e, t);
            pose(k, G_SWING(k), k->fired & 1);
            s32 x, y, tx, ty, ax, ay;
            launch_point(e, &x, &y);
            centre(t, &tx, &ty);
            set_len(tx - x, ty - y, 0x100, &ax, &ay);
            s32 kk = (s32)Rand_Range(0x60) - 0x30;
            if (m->flags & MF_HOME) kk = (k->fired & 1 ? 1 : -1) * (0x90 + (k->fired >> 1) * 0x20);   // Hellzone: out wide, then in
            Proj *p = fire(k, (m->flags & MF_KI) ? ART_KI : ART_ORB_S, 0, t, ax - (ay * kk >> 8), ay + (ax * kk >> 8),
                           (m->flags & MF_HOME) ? 0x140 : 0x200, (m->flags & MF_HOME) ? 8 : 30, 120, 12, m->power, atk_ki);
            if (p && (m->flags & MF_HOME)) p->home_at = 20;
            k->fired++;
        }
        if (k->fired >= m->a && s >= m->a * gap + 10) finish(k);
        break;
    }
    case MK_WAVE: {     // a crouching power-up, then a slow head that homes partly and leaves a trail
        pose(k, G_CHARGE(k), tt < 40 ? 0 : 1);
        if (tt == 8) shake(32, 40);
        if (tt == 40) {
            face(e, t);
            Proj *p = fire_at(k, ART_ORB_M, 1, t, 0x120, 30, 210, 20, m->power, atk_heavy);
            if (p) { p->flags |= 0x80; p->trail_t = 0; }
        }
        if (tt >= 80) finish(k);
        break;
    }
    case MK_NOVA: {     // a shell round him: the blast at 24, a ring of orbs
        pose(k, G_CHARGE(k), tt < 24 ? 0 : 1);
        if (tt == 4) shake(20, 60);
        if (tt == 24) {
            s32 x, y;
            centre(e, &x, &y);
            boom(x - 18, y + 4);
            boom(x + 18, y + 4);
            boom(x, y - 16);
            if (m->a > 50) { boom(x - 30, y - 10); boom(x + 30, y - 10); boom(x, y + 20); }
            shake(16, 90);
            sfx(SFX_BOOM);
            blast(k, x, y, m->a, m->a, atk_heavy, m->power, 0, 0);
            if (mv.dying == k) return;                 // (never: the blast is type 0, he does not take it)
            static const s8 dir[8][2] = { { 16, 0 }, { 11, 11 }, { 0, 16 }, { -11, 11 }, { -16, 0 }, { -11, -11 }, { 0, -16 }, { 11, -11 } };
            for (int i = 0; i < m->b && i < 8; i++) {
                Proj *p = fire(k, ART_ORB_S, 0, 0, dir[i][0], dir[i][1], 0x1A0, 0, 70, 12, m->power / 2, atk_ki);
                if (p) { p->x = (x + dir[i][0]) << 8; p->y = (y + dir[i][1]) << 8; }
            }
            if (m->flags & MF_SELFKO) { k->cur = 0; self_ko(k); return; }   // the match is his loss
        }
        if (tt >= 48) finish(k);
        break;
    }
    case MK_RUSH: {     // a crouch, the dash (map collision), then b hits 8 frames apart
        if (tt < 10) { pose(k, G_CHARGE(k), 0); break; }
        if (k->rush_left > 0) {
            s32 ex, ey, tx, ty, vx, vy;
            centre(e, &ex, &ey);
            centre(t, &tx, &ty);
            if ((tx - ex) * (tx - ex) + (ty - ey) * (ty - ey) <= 18 * 18) { k->rush_left = 0; }
            else {
                set_len(tx - ex, ty - ey, RUSH_SPEED << 8, &vx, &vy);
                s32 nx = e->x + vx, ny = e->y + vy;
                if (!walkable(e, nx >> 8, ny >> 8)) k->rush_left = 0;
                else { e->x = nx; e->y = ny; k->rush_left -= RUSH_SPEED; }
                e->facing = (u8)facing_to(tx - ex, ty - ey);
                pose(k, 1, (tt >> 2) & 1);                 // the walk at a run
            }
            k->step = (u8)tt;                          // the frame the dash ended
            break;
        }
        int s = tt - k->step;
        int hits = m->b ? m->b : 1;
        pose(k, G_SWING(k), (s >> 2) & 1);
        if (s % 8 == 1 && k->fired < hits) {
            face(e, t);
            int last = k->fired == hits - 1;
            melee_burst(k, last ? atk_smash : atk_melee, m->power, (last && (m->flags & MF_STUN)) ? 50 : 0);
            if (mv.dying == k) return;
            k->fired++;
        }
        if (k->fired >= hits && s >= 8 * hits + 8) finish(k);
        break;
    }
    case MK_TELEPORT: { // vanish with a flash, appear behind the target, strike (or hop away: a dodge)
        s32 x, y;
        centre(e, &x, &y);
        if (tt == 1) {
            flash(x, y, ART_ORB_M, 10);
            static const s8 behind[4][2] = { { 0, -22 }, { 0, 22 }, { 24, 0 }, { -24, 0 } };   // behind a target facing down/up/left/right
            s32 nx, ny;
            if (m->flags & MF_DODGE) {
                s32 tx, ty;
                centre(t, &tx, &ty);
                set_len(x - tx, y - ty, 56, &nx, &ny);
                nx += e->x >> 8; ny += e->y >> 8;
            } else {
                nx = (t->x >> 8) + behind[t->facing & 3][0];
                ny = (t->y >> 8) + behind[t->facing & 3][1];
            }
            if (!walkable(e, nx, ny)) { nx = (t->x >> 8) - behind[t->facing & 3][0]; ny = (t->y >> 8) - behind[t->facing & 3][1]; }
            if (walkable(e, nx, ny)) { e->x = nx << 8; e->y = ny << 8; }
            centre(e, &x, &y);
            flash(x, y, ART_ORB_M, 10);
            sfx(SFX_LAUNCH);
            face(e, t);
        }
        pose(k, tt < 10 ? G_CHARGE(k) : G_SWING(k), tt < 10 ? 0 : (tt >> 2) & 1);
        if (tt == 10 && !(m->flags & MF_DODGE)) { face(e, t); melee_burst(k, atk_smash, m->power, 0); if (mv.dying == k) return; }
        if (tt >= 28) finish(k);
        break;
    }
    case MK_STUN: {
        pose(k, G_CHARGE(k), tt < 20 ? 0 : 1);
        if (tt == 20) {
            s32 x, y;
            centre(e, &x, &y);
            int frames = m->a * 4;
            if (m->flags & MF_PROJ) {                  // Stone Spit / Galactic Donut: a projectile that freezes
                face(e, t);
                Proj *p = fire_at(k, ART_ORB_M, 1, t, 0x240, 20, 120, 14, m->power, atk_ki);
                if (p) { p->flags |= PF_STUN; p->stun = (s16)frames; p->size_class = 0; }
            } else if (m->flags & MF_TARGET) {        // psychic hold: the target alone, at range
                stun_entity(t, frames);
                flash(x, y - 18, ART_ORB_S, 20);
            } else {                                   // Solar Flare: a flash, everyone near him
                flash(x, y - 16, ART_ORB_L, 14);
                shake(10, 40);
                stun_around(k, m->b, frames);
            }
            sfx(SFX_LAUNCH);
        }
        if (tt >= 40) finish(k);
        break;
    }
    case MK_HEAL:
        pose(k, G_CHARGE(k), tt < 30 ? 0 : 1);
        if (tt == 30) heal_side(k, m->a);
        if (tt >= 44) finish(k);
        break;
    case MK_SUMMON:
        pose(k, G_CHARGE(k), (tt >> 3) & 1);
        if (tt == 6) shake(24, 50);
        if (tt == 30) summon(k, m);
        if (tt >= 50) finish(k);
        break;
    case MK_TRANSFORM: {
        pose(k, G_CHARGE(k), (tt >> 3) & 1);
        s32 x, y;
        centre(e, &x, &y);
        if (tt == 2) shake(40, 60);
        if ((tt & 7) == 4) { boom(x - 16 + Rand_Range(32), y - 16 + Rand_Range(32)); sfx(SFX_BOOM); }
        if (tt == 40) { transform(k, m); flash(x, y, ART_ORB_L, 12); }
        if (tt >= 56) finish(k);
        break;
    }
    default:
        finish(k);
    }
}

// ---- the scheduler ---------------------------------------------------------------------------------------------------
static int allowed(const TKit *k, const TMove *m) {
    if (m->kind == MK_SUMMON && (m->flags & MF_TEAMONLY)) {
        if (mv.rumble) return 1;
        const TBracket *b = tny_bracket();
        return b && b->division == DIV_2V2;
    }
    if ((m->kind == MK_SUMMON || m->kind == MK_TRANSFORM) && k->copy) return 0;
    return 1;
}
static int aligned(const Entity *e, const Entity *t) {
    s32 ex, ey, tx, ty;
    centre(e, &ex, &ey);
    centre(t, &tx, &ty);
    s32 dx = tx - ex, dy = ty - ey;
    return (dy > -14 && dy < 14) || (dx > -12 && dx < 12);
}

static const TMove *forced_move(TKit *k) {
    if (!mv.force_move || mv.force_reg >= KIT_MAX || mv.reg[mv.force_reg] != k) return 0;
    u8 f = mv.force_move;
    mv.force_move = 0;
    if (f & 0x80) {
        int kind = f & 0x7F;
        if (kind == 0x7F) return &lib_ball_big;
        if (kind == 0x7E) return &lib_stun_proj;
        if (kind == 0x7D) return &lib_beam_stun;
        if (kind == 0x7C) return &lib_self_ko;
        if (kind == 0x7B) return &lib_summon_self;
        return kind < MK_COUNT && kind != MK_DRAIN && kind != MK_REGEN ? &lib_default[kind] : 0;
    }
    return f <= KIT_MOVES && k->def->m[f - 1].kind ? &k->def->m[f - 1] : 0;
}

static void schedule(TKit *k, Entity *t) {
    Entity *e = k->e;
    s32 max = k->stat.max_hp ? k->stat.max_hp : 1;
    int pct = e->hp * 100 / max;
    const TMove *f = forced_move(k);
    if (f) { begin(k, f, t ? t : nearest_player_of(e)); return; }
    // HP triggers first (transform, Multi-Form, self-destruct): they ignore the cooldowns
    for (int i = 0; i < KIT_MOVES; i++) {
        const TMove *m = &k->def->m[i];
        if (!m->kind || !(m->flags & MF_TRIGGER) || (k->done & (1 << i)) || pct > m->cd) continue;
        k->done |= (u8)(1 << i);
        if (!allowed(k, m)) continue;
        begin(k, m, t);
        return;
    }
    if (k->gcd > 0 || !t) return;
    void *st = e->ctrl_state;
    if (st != ENEMY_STATES[0] && st != ENEMY_STATES[1]) return;   // only from idle or walking
    int range = k->def->range ? k->def->range : 170;
    if (dist2(e, t) > range * range) return;
    const TMove *pick[KIT_MOVES];
    int n = 0;
    for (int i = 0; i < KIT_MOVES; i++) {
        const TMove *m = &k->def->m[i];
        if (!m->kind || (m->flags & (MF_PASSIVE | MF_TRIGGER)) || k->cd[i] > 0 || !allowed(k, m)) continue;
        if (m->kind == MK_BEAM && !aligned(e, t)) continue;
        if (m->kind == MK_RUSH && m->a < 24 && dist2(e, t) > 40 * 40) continue;    // a grab / a pan wants him close
        if (m->kind == MK_HEAL && e->hp * 100 / max > 70) continue;
        pick[n++] = m;
    }
    if (!n) return;
    const TMove *m = pick[Rand_Range(n)];
    k->cd[m - k->def->m] = (s16)m->cd;
    begin(k, m, t);
}

// ---- the vtable wrappers ----------------------------------------------------------------------------------------------
static void unregister(TKit *k) {
    for (int i = 0; i < KIT_MAX; i++) if (mv.reg[i] == k) mv.reg[i] = 0;
}

static void kit_destroy(Entity *e, int free_it, u32 a, u32 b) {
    TKit *k = (TKit *)e->vtable;
    const u32 *base = k->base;
    beam_end(k);
    for (int i = 0; i < KIT_LIVE; i++) if (k->live[i]) { k->live[i]->kit = 0; k->live[i] = 0; }
    if (k->held) { k->held->kit = 0; k->held = 0; }
    unregister(k);
    mv.dying = k;
    e->vtable = (u32 *)base;
    ((void (*)(Entity *, int, u32, u32))((u32)base + base[0]))(e, free_it, a, b);
    Arena_Free(k);
}

static void kit_update(Entity *e) {
    TKit *k = (TKit *)e->vtable;
    const u32 *base = k->base;
    void (*base_update)(Entity *) = (void (*)(Entity *))((u32)base + base[2]);
    if (e->hp <= 0 || k->native) {
        if (e->hp <= 0 && k->cur) { beam_end(k); k->cur = 0; }
        base_update(e);                             // natives (Android 16, Super 13) aim at players themselves
        return;
    }
    Entity *saved = g_PlayerObject;
    Entity *t = g_PlayerObject;
    if (mv.rumble) {
        if (k->retarget > 0) k->retarget--;
        t = rumble_target(k);
        if (t) g_PlayerObject = t;
    } else if (!t || revive_is_down(t) || kit_of(t)) {
        t = nearest_player_of(e);
    }
    // passives
    if (k->poise > 0) k->poise--;
    for (int i = 0; i < KIT_MOVES; i++) if (k->cd[i] > 0) k->cd[i]--;
    if (k->gcd > 0) k->gcd--;
    if (++k->regen_t >= 60) {
        k->regen_t = 0;
        for (int i = 0; i <= KIT_MOVES; i++) {
            const TMove *m = i < KIT_MOVES ? &k->def->m[i] : k->extra;
            if (!m || m->kind != MK_REGEN) continue;
            s32 hp = e->hp + k->stat.max_hp * m->a / 1000;
            e->hp = hp > k->stat.max_hp ? k->stat.max_hp : hp;
        }
    }
    if (k->stun > 0) {                              // frozen: states only (the hurt pose), no AI, no specials
        k->stun--;
        if (k->cur) { beam_end(k); k->cur = 0; }
        if (e->ctrl_state != &k->ctrl) Entity_SetCtrlState(e, &k->ctrl);
        pose(k, G_HURT(k), 0);
        Entity_BaseUpdate(e);
        if (!k->stun) Entity_SetAnimation(e, 0);
    } else if (k->cur) {
        if (e->ctrl_state != &k->ctrl) { beam_end(k); k->cur = 0; k->gcd = 40; }   // knocked out of it (a script)
        else {
            Entity_BaseUpdate(e);
            mv.dying = 0;
            step(k);
            if (mv.dying == k) { if (g_PlayerObject == t) g_PlayerObject = saved; return; }   // self-destruct: e is gone
        }
    } else {
        u16 calls = coop2.dmg_calls;
        base_update(e);                             // the stock AI (chase, swings) at this frame's target
        if (coop2.dmg_calls != calls && coop2.last_attacker == e && coop2.last_defender != e)
            on_hit_effects(k, coop2.last_defender, coop2.last_dmg);   // drain on his melee hits
        if (!k->cur && e->hp > 0) schedule(k, t);
    }
    if (g_PlayerObject == t) g_PlayerObject = saved;
}

// Messages. Rumble: a type-0 hit from a fighter of another side counts as a player's (type 1) for him. A hit
// during a special or poise only does damage (the stock box test, the damage calculation and OnHit).
static int kit_message(Entity *e, int msg, u8 *m) {
    TKit *k = (TKit *)e->vtable;
    if (msg != 0 || e->hp <= 0) return base_call9(k, e, msg, m);
    Entity *att = *(Entity **)(m + 0x20);
    TKit *ak = kit_of(att);
    int conv = m[0] == 0 && mv.rumble && foe_kit(k, ak);
    if (m[0] == 0 && !conv) return base_call9(k, e, msg, m);
    if (conv) m[0] = 1;
    s32 exp = k->stat.exp;
    if (conv) k->stat.exp = 0;                      // an NPC's kill gives nobody EXP
    int r;
    mv.dying = 0;
    if (!k->native && (m[0] == 1 || m[0] == 2) && (k->cur || k->poise > 0)) {
        r = 0;
        if (E32(e, 0x168)) {                        // hittable right now (as the stock test)
            s32 b[4];
            const s32 *mb = (const s32 *)(m + 4);
            Entity_GetBox(b, e);
            if (mb[0] < b[2] && b[0] < mb[2] && mb[1] < b[3] && b[1] < mb[3]) {
                damage_calc_hook(e, (Hit *)(m + 0x14));
                Enemy_OnHit(e, m + 0x24);
                r = 1;
            }
        }
    } else {
        r = base_call9(k, e, msg, m);
        if (r && mv.dying != k && (k->def->flags & KF_POISE) && !k->native) k->poise = POISE_FRAMES;
    }
    if (conv) m[0] = 0;
    if (mv.dying == k) {                            // he fell to it and is gone
        if (conv) mv.npc_kos++;
        return r;
    }
    if (conv) k->stat.exp = exp;
    if (r) {
        if (conv) { mv.rumble_hits++; if (e->hp <= 0) mv.npc_kos++; }
        Entity *who = att;
        if (who && !is_player(who) && !ak) who = *(Entity **)((u8 *)att + 0x1C);   // a projectile: its owner
        k->last_hitter = who;
    }
    return r;
}

// ---- the contract (tourney.h) -------------------------------------------------------------------------------------------
void tny_moves_attach(Entity *e, int fighter_id) {
    mv_init();
    if (!e || fighter_id < 0 || fighter_id >= TF_COUNT) return;
    TKit *old = kit_of(e);
    if (old) {                                      // a new kit on the same body
        old->fighter = (u8)fighter_id;
        old->def = &tny_kits[fighter_id];
        old->native = (old->def->flags & KF_NATIVE) != 0;
        return;
    }
    const u32 *vt = e->vtable;
    if (!(vt == (u32 *)ENEMY_VTABLE || vt == coop2.enemy_vtable || is_a16(e) || is_s13(e))) return;   // enemies only
    int slot = -1;
    for (int i = 0; i < KIT_MAX; i++) if (!mv.reg[i]) { slot = i; break; }
    if (slot < 0) return;
    TKit *k = Arena_Alloc(sizeof *k);
    if (!k) return;
    memset(k, 0, sizeof *k);
    k->e = e;
    k->base = vt;
    k->fighter = (u8)fighter_id;
    k->def = &tny_kits[fighter_id];
    k->native = (k->def->flags & KF_NATIVE) || is_a16(e) || is_s13(e);
    k->team = ++mv.team_seq ? mv.team_seq : ++mv.team_seq;
    k->gcd = FIRST_DELAY;
    k->stat = *(const EnemyStat *)e->behaviour;
    e->behaviour = &k->stat;
    u32 b = (u32)k->vt;
    for (int i = 0; i < 48; i++) k->vt[i] = ((u32)vt + vt[i]) - b;
    k->vt[0] = (u32)kit_destroy - b;
    k->vt[2] = (u32)kit_update - b;
    k->vt[9] = (u32)kit_message - b;
    b = (u32)k->ctrl_vt;
    const u32 *iv = (const u32 *)IDLE_STATE_VTABLE;
    for (int i = 0; i < 12; i++) k->ctrl_vt[i] = (IDLE_STATE_VTABLE + iv[i]) - b;
    k->ctrl_vt[0] = (u32)ctrl_enter - b;
    k->ctrl_vt[1] = (u32)ctrl_nop - b;
    k->ctrl_vt[2] = (u32)ctrl_nop - b;
    k->ctrl_vt[3] = (u32)ctrl_nop - b;
    k->ctrl.vtable = k->ctrl_vt;
    e->vtable = k->vt;
    mv.reg[slot] = k;
    mv.attached++;
}

// His final form at once (the engine's final-form challenge: Super Buu, Gohan absorbed, after the Legend Rumble):
// every transform of his kit applied now, in table order, and marked done so none fires again
void tny_moves_final_form(Entity *e) {
    TKit *k = kit_of(e);
    if (!k) return;
    for (int i = 0; i < KIT_MOVES; i++) {
        const TMove *m = &k->def->m[i];
        if (m->kind != MK_TRANSFORM || (k->done & (1 << i))) continue;
        k->done |= (u8)(1 << i);
        transform(k, m);
    }
}

void tny_rumble_mode(int on) {
    mv_init();
    mv.rumble = on ? 1 : 0;
    for (int i = 0; i < KIT_MAX; i++) if (mv.reg[i]) { mv.reg[i]->retarget = 0; mv.reg[i]->rtarget = 0; }
}

// Every frame (input poll): drop kits whose fighter left the list (a map change frees the heap without the
// destroys), and keep a stunned player's pad dead.
void tny_moves_frame(void) {
    if (mv.magic != TNY_MV_MAGIC) return;
    int vram = ObjVram_Largest(g_EntityList);
    if (!mv.vram_min || vram < mv.vram_min) mv.vram_min = (u16)vram;
    EntityList *l = g_EntityList;
    for (int i = 0; i < KIT_MAX; i++) {
        TKit *k = mv.reg[i];
        if (!k) continue;
        int found = 0;
        for (int j = 0; j < l->count && !found; j++) found = l->items[j] == k->e && l->items[j]->vtable == (u32 *)k;
        if (!found) mv.reg[i] = 0;
    }
    for (int i = 0; i < 2; i++) {
        if (mv.stun_t[i] <= 0) continue;
        mv.stun_t[i]--;
        Entity *p = mv.stun_e[i];
        InputSystem *in = 0;
        if (p && p == (coop2.p1_frame ? coop2.p1_frame : g_PlayerObject)) in = g_InputSystem;
        else if (p && p == coop.p2) in = &coop.input2;
        if (!in || revive_is_down(p)) { mv.stun_t[i] = 0; continue; }
        in->current = in->pressed = 0;
    }
}

// Test commands (docs/ids.md):
//   26 arg = fighter | place << 8 | flags << 16, arg2 = stat (0 or >= 1000 = the engine's row or a stand-in): spawn him with his
//      kit at spot `place` around player 1 (0 = 64 px right; 1-7 round him); flags bit 0 = a boss bar, bit 1 = the
//      same side as the last spawn (Rumble teams), bit 2 = no specials for 600 frames
//   27 arg = op << 24 | ...:
//      op 0  reg << 8 | move: force a special on the reg-th kit (move 1-6 = his kit's slot; 0x80 | kind = a
//            library kind with default numbers: 0xFF big ball, 0xFE stun projectile, 0xFD stun beam, 0xFC self-KO
//            nova, 0xFB Multi-Form); it starts as soon as he is free, ignoring cooldowns and range
//      op 1  Rumble mode = arg & 1
//      op 2  reg << 8: his HP = arg2 (0x800000NN: NN percent of his max; transform / trigger tests)
//      op 3  reg << 8: no specials for arg2 frames (his stock AI still runs)
//      op 4  every kit fighter leaves, Rumble off
//      op 5  reg << 8 | kind: a passive on top of his kit (11 drain, 12 regen; 0 = none)
void tny_moves_test(u32 cmd, u32 arg, u32 arg2) {
    mv_init();
    Entity *p1 = g_PlayerObject;
    if (cmd == 26) {
        mv.spawn_fail = 1;
        if (!p1) return;
        static const s8 spot[8][2] = { { 64, 0 }, { 56, -40 }, { 56, 40 }, { -56, -40 }, { -56, 40 }, { 0, -56 }, { 0, 56 }, { -64, 0 } };
        int place = (arg >> 8) & 7, fl = (arg >> 16) & 0xFF;
        s32 x = (p1->x >> 8) + spot[place][0], y = (p1->y >> 8) + spot[place][1];
        for (int i = 1; i < 8 && !walkable(p1, x, y); i++) {   // a rock there: the next spot round him
            x = (p1->x >> 8) + spot[(place + i) & 7][0];
            y = (p1->y >> 8) + spot[(place + i) & 7][1];
        }
        int team = (fl & 2) ? mv.team_seq : 0;
        Entity *e = spawn_fighter((int)(arg & 0xFF), x, y, arg2 < 1000 ? (int)arg2 : 0, team);
        if (!e) return;
        mv.spawn_fail = 0;
        mv.spawn_frame = g_FrameCounter;
        if (fl & 1) {
            BossBar *bar = bossbar_create_hook(0, e, 0, 100);
            if (bar) EntityList_Add(g_SceneTable[1], (Entity *)bar);
        }
        TKit *k = kit_of(e);
        if (k && (fl & 4)) k->gcd = 600;
        return;
    }
    if (cmd != 27) return;
    int op = (int)(arg >> 24), reg = (int)((arg >> 8) & 0xFF);
    TKit *k = reg < KIT_MAX ? mv.reg[reg] : 0;
    if (op == 0) {
        mv.force_reg = (u8)reg;
        mv.force_move = (u8)(arg & 0xFF);
        if (k) k->gcd = 0;
    } else if (op == 1) {
        tny_rumble_mode((int)(arg & 1));
    } else if (op == 2 && k && k->e->hp > 0) {
        s32 hp = (arg2 & 0x80000000u) ? k->stat.max_hp * (s32)(arg2 & 0xFF) / 100 : (s32)arg2;   // bit 31: percent of max
        k->e->hp = hp > 0 ? hp : 1;
    } else if (op == 3 && k) {
        k->gcd = (s16)arg2;
    } else if (op == 5 && k) {
        int kind = (int)(arg & 0xFF);
        k->extra = (kind == MK_DRAIN || kind == MK_REGEN) ? &lib_default[kind] : 0;
    } else if (op == 4) {                           // every kit fighter leaves (the next test starts clean)
        tny_rumble_mode(0);
        for (int i = 0; i < KIT_MAX; i++) {
            TKit *o = mv.reg[i];
            if (!o) continue;
            Entity *e = o->e;
            EntityList_Broadcast(g_SceneTable[1], 0xF, e);   // "entity died": a boss bar lets go
            EntityList_Remove(g_SceneTable[1], e, 0);
            mv.reg[i] = 0;
        }
        mv.team_seq = 0;
    }
}
