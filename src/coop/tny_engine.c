// The tournament engine (docs/tournament.md, "The engine (built)"): the pool, the brackets and their simulation,
// the match loop in every division, the full heal, the 1 HP rule, rewards, the global record on the save chip,
// unlocks, the hidden challengers and the pop-up queue. The other pieces' screens, kits and maps are called
// through tourney.h; with their weak defaults (tourney_hooks.c) the flow runs headless on automatic choices.
//
// Flow (one tournament): CoopCall 23 at the director arms P_DIRECTOR and pushes 1. Every frame
// (tny_engine_frame, from the input poll) decides what comes next; anything that opens a screen, warps, spawns
// or shows a line runs as a "step" from that tick (where the ui's screens expect their callers), one at a time
// and only while no scene runs and player 1 is free; warps go through the test-warp command:
//   P_DIRECTOR   the choices (tier, division, Quick / Full), the select screens, the draw; warp to the ring
//   P_TO_RING    wait for the ring map
//   P_NEXT       the next thing to play: a bracket match (NPC matches resolve off-screen first), a Rumble
//                round, the duel final, or the end (trophy / out)
//   P_MATCH      a fight runs: watched every frame (opponents down = won; a player out = lost)
//   P_MATCH_END  a short pause, then the step: bodies cleared, entries restored, unlocks, EXP, the result line
//   P_DUEL       the co-op final (the CoopCall 1 path: duel.c, vmcall.c's call_state)
//   P_TROPHY / P_PRIZE / P_CHALLENGE   the winner's line, the prizes, the hidden challenger's entrance and fight
//   P_EXIT / P_EXITING   everything restored, back to where the director was
// A map change away from the ring (a game over, a reload) drops the tournament: entries are restored only while
// the same game is still running (player 1 present), never into a freshly loaded party.
#include "tny_engine.h"
#include "revive.h"
#include "super13.h"
#include "difficulty.h"

// ---- ROM ------------------------------------------------------------------------------------------------
ROM_FN(void, Step_SetActiveCharacter, (s32 *st), 0x0800A785);        // VM step 70: player 1 plays party slot st[top]
ROM_FN(void, Eeprom_Read, (void *dev, void *dst, int off, int len), 0x08021527);    // from the 8 KiB RAM mirror
ROM_FN(void, Eeprom_Write, (void *dev, int off, const void *src, int len), 0x080214C5);   // mirror + chip, 8-byte blocks
ROM_FN(u32, Save_Checksum, (const void *buf, int len), 0x0802153F);
ROM_FN(int, ObjVram_Largest, (void *list), 0x08006D71);              // largest free OBJ VRAM block (bytes)
#define EEPROM_DEV          (*(void *const *)0x081D952C)               // the party module's EEPROM device
#define ENEMY_RECORD_HANDLER 0x0800E77Fu

Entity *enemy_spawn_hook(const EnemyRecord *rec);                     // enemies.c
BossBar *bossbar_create_hook(void *mem, Entity *e, int lo, int hi);   // enemies.c
int item_use_hook(int id);                                            // party.c: PickUpItem (popup, count)
int flag_test_hook(PartyState *p, u32 id);                            // party.c
int p2_bind_slot(int slot);                                           // player.c
extern const char *const ui_strings[];
// the moves piece (tny_moves.c): who hit a kit fighter last (the Rumble's NPCs hit each other); weak, so the
// engine links without it and keeps its own note from the damage calculation (tny_note_hit)
Entity *tny_moves_last_hitter(const Entity *e) __attribute__((weak));

#define A16_STAT_ID   109     // android16.c's own spawn path (rockets, barrage, bombs, punch)
#define S13_STAT_ID   145     // super13.c's own spawn path
#define A16_FLAG_DONE 421     // a16player.h: Gero's computer is over (16 playable from then on)
#define A16_ROW       25
#define A16_PLAYER_FLAGS 0x7B // in party, available, his four moves (a16player.h A16_FLAGS)
#define HERCULE_PARTY_SLOT 5
#define KRILLIN_PARTY_SLOT 6

// Items (stock item table 0x086ADE24; names from the UI strings): +3 capsules and the Senzu bean
#define ITEM_SENZU    0
#define ITEM_STR3     3
#define ITEM_PWR3     6
#define ITEM_END3     9

// ---- the global record (save chip, shared by the three save files) -------------------------------------
// Offset 0x1E00 of the 8 KiB EEPROM: the save slots end at 0x550 with 9 party slots (0x670 with the 15 the
// party check allows), so the record stays clear of them at any PARTY_SLOTS.
#define TREC_OFF      0x1E00
#define TREC_MAGIC    0x3159544Eu     // "NTY1"
#define TREC_VERSION  1
typedef struct TRecord {
    u32 magic;
    u8 version, _p0[3];
    u8 unlocked[8];        // bit per fighter id (TF_*): beaten (TR_BEAT) or challenger won (TR_CHALLENGER)
    u8 wins[15];           // wins per tournament (tier * 3 + division), any size, capped at 255
    u8 full_wins[5];       // 1 v 1 Full wins per tier
    u8 attempts[16];       // challenger attempts lost (id - TF_PUIPUI)
    u32 paid;              // bit t: tournament t's first-win capsule paid; bit 16 + tier: the Full capsule
    u32 extra;             // bit 0: Super Buu's final form beaten (the Legend Rumble's hidden fight, the record's last line)
    u32 checksum;          // Save_Checksum of the bytes before it
} TRecord;
_Static_assert(sizeof(TRecord) == 64, "TRecord is 64 bytes (8-byte EEPROM blocks)");

// ---- state (.tny.eng, NOLOAD) --------------------------------------------------------------------------
#define ENG_MAGIC     0x7E6E0003u     // 3: the presentation fields (polish)
#define MAX_OPP       6
#define NO            0xFF
enum { P_IDLE, P_DIRECTOR, P_TO_RING, P_NEXT, P_MATCH, P_MATCH_END, P_DUEL, P_TROPHY, P_PRIZE, P_CHALLENGE,
       P_INTRO, P_RESTAGE, P_EXIT, P_EXITING };
enum { K_MATCH, K_RUMBLE, K_CHALLENGE };
enum { R_NONE, R_WON, R_LOST };

typedef struct TnyEng {
    u32 magic;
    u8 phase, pending, kind, result;     // phase P_*, a step event queued, the fight's kind K_*, R_*
    u8 division, tier, players, full;    // the tournament; players 1 / 2
    u8 pick[2];                          // the players' fighters (TF_*)
    u8 slot[2];                          // the party slots they fight on (story slot or a carrier)
    u8 own[2];                           // their own slots before the tournament (player 1 active, player 2's)
    u8 fighter;                          // who fights this fight: 0 player 1, 1 player 2, 2 both
    u8 out;                              // bit k: player k is out of the tournament (1 v 1)
    u8 ko;                               // bit k: player k is out of this fight
    u8 koed;                             // a player went down in this tournament (the Rumble's no-KO Senzu)
    u16 timer;
    u8 nopp, nwait;                      // opponents in this fight, of which still waiting to enter
    u8 opp_id[MAX_OPP];
    u8 opp_by[MAX_OPP];                  // the last hit on him came from a player (beat-to-unlock)
    u8 opp_in[MAX_OPP];                  // 0 waiting, 1 in the ring, 2 down
    Entity *opp[MAX_OPP];
    u8 played[4];                        // bit m: match m of round r resolved
    u8 results;                          // results since the bracket screen last showed
    u8 lone;                             // one challenger against two players: x1.8 HP (the lone-boss rule)
    u8 backed;                           // bit k: bslot[k] holds a backup
    u8 bslot[2];
    u8 rb_round, rb_rounds;              // the Rumble: this round, the rounds (headliner's included)
    u8 used[8];                          // fighters already drawn in this Rumble
    u8 chal[2], nchal;                   // the hidden challenger(s) of this tournament
    u8 seeded;                           // test command 21: the next draw uses rng as it is
    u8 ret_zone, ret_area;               // where the director was
    s16 ret_x, ret_y;
    u32 rng;
    Entity *from;                        // player 1's entity when a warp was asked for (arrived())
    TBracket br;
    CharacterEntry backup[2];
    // pop-ups
    const char *queue[4];
    u8 qn, shown;                        // queued, shown since boot (tests)
    u8 force_tiers, force_flags;         // test command 24: tiers forced open, TFORCE_* bits
    u16 seen_tiers, seen_story;          // story watch: what was open / in the party last time
    u16 absent;                          // frames without player 1 (title, menus): the next baseline is silent
    u8 baseline, full_seen;              // story watch armed; tiers whose Full bracket was announced
    u8 preset;                           // test command 25: choices given, skip the screens
    u8 preset_pick[2];
    u8 refresh;                          // the active slot's entry was rewritten: set_active must reload it
    u8 t_nofight, t_busy;                // tests (stepz): 0 = a fight runs and no line is up / nothing runs
    u8 back;                             // P_RESTAGE: the phase whose step runs again once the ring has reloaded
    u8 say_hold;                         // frames without a scene before the next step (a line's event fires late)
    // a line in a text box (the announcer, the fallback pop-up): one entry in RAM
    const void *say_seq[3];
    u32 say_action[2];
    u8 stepping;                         // a step runs (its screens' nested input polls tick again: kept out)
    u8 _p3[3];
    u32 step_sp;                         // the stack at the step's tick: a deeper tick is nested, a shallower one
                                         // finds a stale flag (a reset or a state loaded mid-step) and clears it
    // the ring presentation (76 bytes, where the first say entry was)
    Entity *pose_e[4];                   // held poses (the loser lying, the winner's pose) while a line shows
    u8 pose_g[4], pose_f[4];             // their group and frame
    struct { u32 handler, flags; s16 x, y; s32 sprite, count; const void *beh; } ann_rec;   // the announcer's
                                         // actor record (MapScript_CreateCharacter; the actor keeps pointing at it)
    Entity *ann;                         // the announcer on the ring's edge (between fights only)
    u8 pres;                             // P_INTRO / P_MATCH_END: 0 the presentation's first part, 1 its line ran
    u8 force_opp;                        // test command 21 arg2: the players' next opponent / headliner (NO = none)
    u8 npose;                            // poses held (pose_e)
    u8 chal_final;                       // the hidden fight is Super Buu's final form (the Legend Rumble's)
    u8 _p4[20];
    TRecord rec;                         // the record as last read or written (tests peek it)
    u8 down_t[MAX_OPP];                  // frames a Rumble fighter has lain down (his body leaves at 60)
    u8 br_shown;                         // the bracket screen has shown this draw (then phase 1 with the results)
    u8 *tint;                            // the same fighter twice on the floor: player 2's shadow remap (arena)
    struct { u8 head[4]; char text[100]; } say_entry;   // a line (the announcer's, the fallback pop-up)
} TnyEng;
_Static_assert(sizeof(TnyEng) <= 0x240, ".tny.eng holds 0x240 bytes (docs/ids.md)");
_Static_assert(__builtin_offsetof(TnyEng, rec) == 360 && __builtin_offsetof(TnyEng, pose_e) == 284, "the presentation fields fill the old line's 76 bytes");
// offsets the tests peek (docs/testing.md "Tournament engine"): eng is at the start of tourneyram, 0x0203EC00
_Static_assert(__builtin_offsetof(TnyEng, phase) == 4 && __builtin_offsetof(TnyEng, pick) == 12 &&
               __builtin_offsetof(TnyEng, rng) == 100 && __builtin_offsetof(TnyEng, br) == 108 &&
               __builtin_offsetof(TnyEng, qn) == 236 && __builtin_offsetof(TnyEng, t_nofight) == 252 && __builtin_offsetof(TnyEng, rec) == 360, "TnyEng layout (tests)");
static TnyEng eng TNY_SECTION("eng") __attribute__((used));

#define TFORCE_POOL_FLAGS 1      // ignore the fighters' story flags (pool_flag)
#define TFORCE_STANDIN    2      // a fighter whose art is not in this build fights with a stock stand-in body
#define STANDIN_SPRITE    121    // Tien's body (AI set 98)

// ---- opponents' numbers (stats 160-219) ------------------------------------------------------------------
// Fixed per tier (the tournament's, not the fighter's own): a tier base times the fighter's power, then the
// difficulty on top in enemy_spawn_hook. Our tuning, to be adjusted in play; references: the Android 19 fight
// (2970 HP, 21/14/11) for Adept, Imperfect Cell (5210, 32/32/37) for Z-Fighter, the Cell Games (6500,
// 50/55/60) for Champion, Super 13 (10000, 48/64/56) for Legend.
typedef struct TierBase { s32 hp; u8 melee, ranged, defence, _p; } TierBase;
static const TierBase tier_base[TIER_COUNT] = {
    { 1400, 12, 12, 10, 0 },    // Rookie: early game (levels about 1-10)
    { 2800, 22, 20, 18, 0 },    // Adept
    { 4200, 33, 33, 34, 0 },    // Z-Fighter
    { 5800, 47, 50, 52, 0 },    // Champion
    { 8000, 58, 62, 58, 0 },    // Legend
};
// power (percent of the tier base: HP x p, attack and defence x (100 + p) / 200) and the AI set that fits his
// body (docs/sprites.md "AI sets": 21 the melee boss set for most; the Cells' own; 98 the dojo bodies; 29
// the androids; 129 Tao's robot set); a body without the set's pose groups falls back (ai_for)
typedef struct TOpp { u8 power, ai, base; } TOpp;
#define B_BOSS 29                // stock stat copied first: the boss class (death without a script)
#define B_JR   26                // Cell Jr's own class
static const TOpp opp_tab[TF_GOHAN] = {
    [TF_YAMCHA] = { 85, 21, B_BOSS },  [TF_TIEN] = { 95, 98, B_BOSS },     [TF_FRIEZA] = { 110, 21, B_BOSS },
    [TF_MECHA_FRIEZA] = { 105, 21, B_BOSS }, [TF_KING_COLD] = { 115, 21, B_BOSS }, [TF_HERCULE] = { 75, 98, B_BOSS },
    [TF_ROSHI] = { 100, 21, B_BOSS },  [TF_TAO] = { 95, 129, B_BOSS },     [TF_SPOPOVICH] = { 95, 21, B_BOSS },
    [TF_YAMU] = { 85, 21, B_BOSS },    [TF_CHICHI] = { 80, 21, B_BOSS },   [TF_VIDEL] = { 85, 21, B_BOSS },
    [TF_CHIAOTZU] = { 78, 21, B_BOSS },
    [TF_A19] = { 95, 29, B_BOSS },     [TF_GERO] = { 90, 29, B_BOSS },     [TF_A13] = { 105, 29, B_BOSS },
    [TF_A17] = { 110, 21, B_BOSS },    [TF_A18] = { 105, 21, B_BOSS },
    [TF_IMPERFECT_CELL] = { 105, 80, B_BOSS }, [TF_A16] = { 110, 21, B_BOSS },
    [TF_SEMI_CELL] = { 108, 116, B_BOSS }, [TF_PERFECT_CELL] = { 120, 148, B_BOSS }, [TF_CELL_JR] = { 85, 21, B_JR },
    [TF_COOLER] = { 120, 21, B_BOSS }, [TF_NAMEKIAN] = { 95, 21, B_BOSS }, [TF_BROLY] = { 130, 21, B_BOSS },
    [TF_PARAGUS] = { 90, 21, B_BOSS },
    [TF_SUPER_PERFECT_CELL] = { 125, 138, B_BOSS }, [TF_SUPER13] = { 130, 21, B_BOSS }, [TF_BGOKU] = { 120, 21, B_BOSS },
    [TF_MAJIN_VEGETA] = { 120, 21, B_BOSS }, [TF_MYSTIC_GOHAN] = { 125, 21, B_BOSS }, [TF_SAIYAMAN] = { 100, 21, B_BOSS },
    [TF_BHERCULE] = { 80, 21, B_BOSS }, [TF_BPICCOLO] = { 105, 21, B_BOSS }, [TF_SUPER_BUU] = { 135, 21, B_BOSS },
    [TF_PUIPUI] = { 110, 21, B_BOSS }, [TF_GOTEN] = { 100, 21, B_BOSS },   [TF_KID_TRUNKS] = { 100, 21, B_BOSS },
    [TF_YAKON] = { 120, 21, B_BOSS },  [TF_PIKKON] = { 115, 21, B_BOSS },  [TF_DABURA] = { 120, 21, B_BOSS },
    [TF_EVIL_BUU] = { 115, 21, B_BOSS }, [TF_KIBITO] = { 110, 21, B_BOSS }, [TF_FAT_BUU] = { 120, 21, B_BOSS },
    [TF_JANEMBA] = { 125, 21, B_BOSS }, [TF_KID_BUU] = { 130, 21, B_BOSS }, [TF_GOTENKS] = { 125, 21, B_BOSS },
    [TF_UUB] = { 125, 21, B_BOSS },    [TF_VEGITO] = { 140, 21, B_BOSS },
};

// Pose groups each AI set plays (docs/sprites.md "AI sets"): a body must have them or the AI reads past the record
static u32 ai_groups(int ai) {
    switch (ai) {
    case 21: return 0x1E7;                       // 0, 1, 2, 5, 6, 7, 8
    case 29: return 0x11E7;                      // + 12
    case 98: return 0x107F;                      // 0-6, 12
    case 129: return 0x67;                       // 0, 1, 2, 5, 6
    case 80: case 116: return 0xC17EF;           // 0-3, 5-10, 12, 18, 19
    case 138: case 148: return 0x411EF;          // 0-3, 5-8, 12, 18
    default: return 0x47;                        // regular enemies: 0, 1, 2, 6
    }
}
static int has_groups(const u8 *rec, u32 need) {
    if (!rec) return 0;
    int count = rec[2];
    for (int g = 0; g < 20; g++) {
        if (!(need & (1u << g))) continue;
        if (g >= count || !*(const u32 *)(rec + 0x4C + 0x10 * g)) return 0;
    }
    return 1;
}

static int body_of(int id);
static int ai_for(int id) {
    const u8 *rec = Character_GetSpriteRecord(body_of(id));
    int want = opp_tab[id].ai;
    static const u8 fallback[] = { 21, 98, 129, 11 };
    if (has_groups(rec, ai_groups(want))) return want;
    for (unsigned i = 0; i < sizeof fallback; i++)
        if (has_groups(rec, ai_groups(fallback[i]))) return fallback[i];
    return 11;
}

static int cap(int v, int hi) { return v < 1 ? 1 : v > hi ? hi : v; }

static int scale_tier(int id) {
    if (eng.magic == ENG_MAGIC && eng.phase != P_IDLE) return eng.tier;
    return tny_fighters[id].tier;
}

int tny_resolve_stat(int stat_index, EnemyStat *out) {
    int id = stat_index - TNY_STAT0;
    if (id < 0 || id >= TF_GOHAN) return 0;
    const TOpp *o = &opp_tab[id];
    const TierBase *t = &tier_base[scale_tier(id)];
    int p = o->power ? o->power : 100;
    *out = ENEMY_STATS[o->base];
    out->exp = 0;                               // no kill EXP: the engine pays a trickle per match
    out->ai_set = ai_for(id);
    out->max_hp = t->hp * p / 100;
    if (eng.magic == ENG_MAGIC && eng.lone) out->max_hp = out->max_hp * COOP_LONE_BOSS_HP_PCT / 100;
    out->melee = (u8)cap(t->melee * (100 + p) / 200, 255);
    out->ranged = (u8)cap(t->ranged * (100 + p) / 200, 255);
    out->defence = (u8)cap(t->defence * (100 + p) / 200, 110);
    return 1;
}

// Every tournament opponent: never twinned, no lone-boss HP (the engine scales a lone challenger itself)
static const BossRule tny_rule = { TNY_STAT0, 0, BOSS_NO_TWIN | BOSS_FLAT, 0, 0, 0, 0, 0 };
const BossRule *tny_boss_rule(int stat_index) {
    return stat_index >= TNY_STAT0 && stat_index < TNY_STAT0 + TF_GOHAN ? &tny_rule : 0;
}

// ---- small helpers --------------------------------------------------------------------------------------
static int flag(u32 n) { return flag_test_hook(g_Party, n); }
static const u8 *map_state(void) { return (const u8 *)g_MapState; }
static int on_map(int zone, int area) { const u8 *m = map_state(); return m && m[0] == zone && m[1] == area; }
static int in_ring(void) { return on_map(TNY_RING_ZONE, TNY_RING_AREA); }
static int in_party(int slot) { return slot >= 0 && slot < PARTY_SLOTS && (g_Party->chars[slot].flags & 1); }

static u32 rnd(u32 n) {
    eng.rng = eng.rng * 1103515245u + 12345u;
    return n ? ((eng.rng >> 16) & 0x7FFF) % n : 0;
}

static void state_check(void) {
    if (eng.magic == ENG_MAGIC) return;
    memset(&eng, 0, sizeof eng);
    eng.magic = ENG_MAGIC;
    eng.rng = 0x1234567u;
    memset(&eng.br, NO, sizeof eng.br);
    eng.br.size = 0;
}

// ---- the record -------------------------------------------------------------------------------------------
static void rec_load(void) {
    Eeprom_Read(EEPROM_DEV, &eng.rec, TREC_OFF, sizeof eng.rec);
    if (eng.rec.magic != TREC_MAGIC || eng.rec.checksum != Save_Checksum(&eng.rec, sizeof eng.rec - 4)) {
        memset(&eng.rec, 0, sizeof eng.rec);
        eng.rec.magic = TREC_MAGIC;
        eng.rec.version = TREC_VERSION;
    }
}
static void rec_save(void) {
    eng.rec.magic = TREC_MAGIC;
    eng.rec.version = TREC_VERSION;
    eng.rec.checksum = Save_Checksum(&eng.rec, sizeof eng.rec - 4);
    Eeprom_Write(EEPROM_DEV, TREC_OFF, &eng.rec, sizeof eng.rec);
}
static int rec_bit(int id) { return (eng.rec.unlocked[id >> 3] >> (id & 7)) & 1; }

// ---- the pool -----------------------------------------------------------------------------------------------
int tny_tier_open(int tier) {
    if (tier < 0 || tier >= TIER_COUNT) return 0;
    if (eng.magic == ENG_MAGIC && (eng.force_tiers >> tier) & 1) return 1;
    return !tny_tier_flag[tier] || flag(tny_tier_flag[tier]);
}

// 16 is the players' from Gero's computer on (in the party, then "the tournament copy stays after the Games")
static int a16_ours(void) {
    const CharacterEntry *c = &g_Party->chars[DUEL_SLOT];
    return flag(A16_FLAG_DONE) || ((c->flags & 1) && c->display_row == A16_ROW);
}

// His body as an opponent: his sprite when its record is in this build and has the pose groups an AI set needs
// (idle, walk, hit-stun, an attack at least: Mecha Frieza's cutscene body 49 has none of the fight poses), else his
// player-layout record (play_sprite, the playables / art pieces), else the test stand-in when it is on, else none
extern const unsigned sprite_index_count;   // build/gen/sprites_data.c: the index has no bounds check
static int fightable(int sprite) {
    return sprite >= 7 && (unsigned)sprite < sprite_index_count && has_groups(Character_GetSpriteRecord(sprite), ai_groups(11));
}
static int body_of(int id) {
    if (id < 0 || id >= TF_COUNT) return 0;
    const TFighter *f = &tny_fighters[id];
    if (tny_fighter_ok(id) && fightable(f->sprite)) return f->sprite;
    if (fightable(f->play_sprite)) return f->play_sprite;
    if (eng.magic == ENG_MAGIC && (eng.force_flags & TFORCE_STANDIN)) return STANDIN_SPRITE;
    return 0;
}

int tny_in_pool(int id) {
    if (id < 0 || id >= TF_COUNT) return 0;
    const TFighter *f = &tny_fighters[id];
    if (!(f->role & TR_OPPONENT) || !tny_tier_open(f->tier) || !body_of(id)) return 0;
    if (f->pool_flag && !flag(f->pool_flag) && !(eng.magic == ENG_MAGIC && (eng.force_flags & TFORCE_POOL_FLAGS))) return 0;
    // no mirror matches: your own party never appears as opponents
    if (id == TF_HERCULE && in_party(HERCULE_PARTY_SLOT)) return 0;
    if (id == TF_A16 && a16_ours()) return 0;
    return 1;
}

// The pool of a tier: everyone in the pool whose own tier is at or below it
static int pool_of(int tier, u8 *out) {
    int n = 0;
    for (int id = 0; id < TF_GOHAN; id++)
        if (tny_fighters[id].tier <= tier && tny_in_pool(id)) { if (out) out[n] = (u8)id; n++; }
    return n;
}

int tny_unlocked(int id) {
    if (id < 0 || id >= TF_COUNT) return 0;
    const TFighter *f = &tny_fighters[id];
    if (f->role & TR_STORY) {
        if (id == TF_P_A16) return a16_ours();
        if (f->pool_flag && !flag(f->pool_flag)) return 0;
        return in_party(f->story_slot);
    }
    if (!(f->role & (TR_BEAT | TR_CHALLENGER))) return 0;
    state_check();
    rec_load();
    return rec_bit(id);
}

int tny_challenge_tried(int id) {
    if (id < TF_PUIPUI || id > TF_VEGITO) return 0;
    state_check();
    rec_load();
    return eng.rec.attempts[id - TF_PUIPUI] != 0;
}

const TBracket *tny_bracket(void) {
    if (eng.magic != ENG_MAGIC || eng.phase == P_IDLE || !eng.br.size) return 0;
    return &eng.br;
}

// ---- lines in a text box ---------------------------------------------------------------------------------
static const u8 say_wait[] __attribute__((aligned(4))) = { 1, 0, 0, 0x00, 0x14, 0x02, 0x45, 0x11 };   // WaitFrames 20
static char *catu(char *o, const char *s, int upper) {   // upper: in capitals (the announcer calls names so)
    char *end = eng.say_entry.text + sizeof eng.say_entry.text - 1;
    while (s && *s && o < end) { char ch = *s++; *o++ = upper && ch >= 'a' && ch <= 'z' ? (char)(ch - 32) : ch; }
    *o = 0;
    return o;
}
static char *cat(char *o, const char *s) { return catu(o, s, 0); }
// A line: built now (say_prep), shown by the next list update (say_go: a dialogue event; the fight frame and the
// steps wait for it). Speaker 0 = narration ("@^" caption), else a portrait (the announcer's 140)
static char *say_open(int speaker) {
    eng.say_entry.head[0] = 2; eng.say_entry.head[1] = 0;   // next: the end
    eng.say_entry.head[2] = 1;                               // text
    eng.say_entry.head[3] = (u8)speaker;
    return cat(eng.say_entry.text, speaker ? "ANNOUNCER: " : "@^");
}
static void say_go(void) {
    eng.say_seq[0] = say_wait;
    eng.say_seq[1] = &eng.say_entry;
    eng.say_seq[2] = 0;
    coop_run_sequence(eng.say_action, eng.say_seq);
    eng.say_hold = 8;
}
static void say(const char *a, const char *b, const char *c, const char *d, const char *e) {
    char *o = say_open(0);
    o = cat(o, a); o = cat(o, b); o = cat(o, c); o = cat(o, d); cat(o, e);
    say_go();
}
// The announcer's line: plain parts and NAMES (upper case) alternate, starting with plain text
static int ann_speaker(void);
static void ann_prep(const char *a, const char *n1, const char *b, const char *n2, const char *c) {
    char *o = say_open(ann_speaker());
    o = cat(o, a);
    o = catu(o, n1, 1);
    o = cat(o, b);
    o = catu(o, n2, 1);
    cat(o, c);
}

static const char *const tier_name[TIER_COUNT] = { "Rookie", "Adept", "Z-Fighter", "Champion", "Legend" };
static const char *const tier_notice[TIER_COUNT] = {
    "New tournament tier: Rookie!", "New tournament tier: Adept!", "New tournament tier: Z-Fighter!",
    "New tournament tier: Champion!", "New tournament tier: Legend!",
};
static const char new_fighter[] = "New tournament fighter!";
static const char new_full[] = "New division: Full bracket!";

void tny_notify(const char *line) {
    state_check();
    if (!line) return;
    for (int i = 0; i < eng.qn; i++) if (eng.queue[i] == line) return;
    if (eng.qn < 4) eng.queue[eng.qn++] = line;
}

// The ui's tny_popup_show, unless only the weak default (an empty function: bx lr) is linked: then the line
// shows in a text box, the stock journal pop-up's way
static int popup_is_stub(void) { return *(const u16 *)((u32)tny_popup_show & ~1u) == 0x4770; }

// ---- steps (entity-list events) ----------------------------------------------------------------------------
// Steps run from the frame tick; a step that opens a modal screen runs the game loop inside it, and that loop's
// input polls call the tick again: `stepping` keeps those nested ticks out
#define stepping eng.stepping
static void run_step(void);

// ---- players --------------------------------------------------------------------------------------------
static CharacterEntry *entry_of(int slot) { return &g_Party->chars[slot]; }
static void set_active(int slot) {
    s32 st[2] = { 1, slot };
    if (slot < 0 || slot >= PARTY_SLOTS || !g_PlayerObject) return;
    if (slot == g_Party->active_slot && !eng.refresh) return;
    eng.refresh = 0;
    Step_SetActiveCharacter(st);
}
static void bind_p2(int slot) {
    if (slot < 0 || slot >= PARTY_SLOTS) return;
    if (coop.p2) p2_bind_slot(slot); else coop.p2_slot = (u8)slot;
}
static void heal_slot(int slot) {
    if (slot < 0 || slot >= PARTY_SLOTS) return;
    CharacterEntry *c = entry_of(slot);
    c->hp = c->max_hp;
    c->ep = c->max_ep;
}
static void heal_all(void) {
    heal_slot(g_Party->active_slot);
    if (eng.players == 2) heal_slot(coop.p2_slot);
    for (int k = 0; k < 2; k++) heal_slot(eng.slot[k]);
}

// A fighter the player can fight as: a story playable he has, a tournament playable with a display row, 16
static int pick_playable(int id) {
    if (id < 0 || id >= TF_COUNT || !tny_unlocked(id)) return 0;
    const TFighter *f = &tny_fighters[id];
    if (f->role & TR_STORY) return 1;
    return tny_play_row(id) != 0;
}

static const u8 by_slot[9] = { TF_GOHAN, TF_PICCOLO, TF_VEGETA, TF_TRUNKS, TF_GOKU, TF_P_HERCULE, TF_KRILLIN, TF_P_A16, TF_SSJ2_GOHAN };
static int own_pick(int slot) { return slot >= 0 && slot < 9 ? by_slot[slot] : TF_GOHAN; }

// The slot a pick plays from: his story slot while he is in the party, else a carrier slot that gets his entry
// for the match (the player's own slot when free, else the duel clone's slot when nobody is in it)
static int carrier_for(int k, int used) {
    int own = eng.own[k];
    if (own < PARTY_SLOTS && !(used & (1 << own))) return own;
    if (!(used & (1 << DUEL_SLOT)) && !in_party(DUEL_SLOT)) return DUEL_SLOT;
    return -1;
}
static void resolve_slots(void) {
    int used = 0;
    for (int k = 0; k < eng.players; k++) eng.slot[k] = NO;
    for (int k = 0; k < eng.players; k++) {           // story picks first: they have fixed slots
        int id = eng.pick[k];
        const TFighter *f = &tny_fighters[id];
        if ((f->role & TR_STORY) && id != TF_P_A16 && !(used & (1 << f->story_slot))) {
            eng.slot[k] = f->story_slot;
            used |= 1 << f->story_slot;
        }
        if (id == TF_P_A16 && in_party(DUEL_SLOT) && entry_of(DUEL_SLOT)->display_row == A16_ROW && !(used & (1 << DUEL_SLOT))) {
            eng.slot[k] = DUEL_SLOT;
            used |= 1 << DUEL_SLOT;
        }
    }
    for (int k = 0; k < eng.players; k++) {
        if (eng.slot[k] != NO) continue;
        int c = carrier_for(k, used);
        if (c < 0) {                                   // nowhere to put him: the player's own character
            eng.pick[k] = (u8)own_pick(eng.own[k]);
            c = eng.own[k] < PARTY_SLOTS && !(used & (1 << eng.own[k])) ? eng.own[k] : -1;
            for (int s = 0; c < 0 && s < PARTY_SLOTS; s++) if (in_party(s) && !(used & (1 << s))) c = s;
            if (c < 0) c = 0;
        }
        eng.slot[k] = (u8)c;
        used |= 1 << c;
    }
}

// 16's tournament copy (the a16_join recipe at the lead's level) for a carrier slot
static void a16_entry(CharacterEntry *a, const CharacterEntry *k) {
    memset(a, 0, sizeof *a);
    a->display_row = A16_ROW;
    a->exp = k->exp;
    a->max_hp = a->hp = (u16)(k->max_hp * 140 / 100);
    a->max_ep = a->ep = (u16)(k->max_ep * 110 / 100);
    a->str = (u16)(k->str * 130 / 100);
    a->pow = k->pow;
    a->end = (u16)(k->end * 130 / 100);
    a->flags = A16_PLAYER_FLAGS;
    static const u8 ab[4] = { 22, 19, 0, 2 };
    for (int i = 0; i < 4; i++) a->ability_list[i] = ab[i];
}

// The lead a tournament playable is built from: the party's best (story slots 0-6 in the party, each read as it was
// before the tournament: a carrier already holding the other player's pick reads from its backup), number by number:
// the highest EXP (his level), max HP, max ki, strength, power and endurance of any of them. It was player 1's own
// slot, so a weak or low-level player-1 character (Hercule's HP is the party's lowest) gave a weak playable (Ryan:
// Spopovich / Yamu "like he got a default level 1"), and player 2's pick was built on player 1's already-written
// pick (found 2026-10-25). The fighters' percentages (tny_play.c) then apply to this best-of-the-party.
static const CharacterEntry *orig_entry(int s) {
    for (int k = 0; k < 2; k++) if ((eng.backed & (1 << k)) && eng.bslot[k] == s) return &eng.backup[k];
    return entry_of(s);
}
static void lead_entry(CharacterEntry *out) {
    *out = *orig_entry(eng.own[0] < PARTY_SLOTS ? eng.own[0] : g_Party->active_slot);
    for (int s = 0; s < PARTY_SLOTS && s < DUEL_SLOT; s++) {
        const CharacterEntry *c = orig_entry(s);
        if (!(c->flags & 1) || !c->max_hp) continue;
        if (c->exp > out->exp) out->exp = c->exp;
        if (c->max_hp > out->max_hp) out->max_hp = c->max_hp;
        if (c->max_ep > out->max_ep) out->max_ep = c->max_ep;
        if (c->str > out->str) out->str = c->str;
        if (c->pow > out->pow) out->pow = c->pow;
        if (c->end > out->end) out->end = c->end;
    }
    out->hp = out->max_hp;
    out->ep = out->max_ep;
}

// Write the picks' entries into their carriers (backed up first) for the fight
static void apply_entry(int k) {
    int id = eng.pick[k], s = eng.slot[k];
    const TFighter *f = &tny_fighters[id];
    if (s >= PARTY_SLOTS) return;
    int story = (f->role & TR_STORY) && id != TF_P_A16;
    if (story && s == f->story_slot) return;                                        // plays from his own slot
    if (id == TF_P_A16 && s == DUEL_SLOT && entry_of(s)->display_row == A16_ROW) return;   // 16 in the party
    if (eng.backed & (1 << k)) return;
    CharacterEntry *c = entry_of(s);
    if (s == g_Party->active_slot) eng.refresh = 1;
    eng.bslot[k] = (u8)s;
    eng.backup[k] = *c;
    eng.backed |= (u8)(1 << k);
    if (story) { *c = *entry_of(f->story_slot); c->flags |= 3; return; }       // the same character twice: a copy
    CharacterEntry lead;
    lead_entry(&lead);
    if (id == TF_P_A16) { a16_entry(c, &lead); return; }
    CharacterEntry out = *c;
    tny_play_entry(id, &out, &lead);
    out.flags |= 3;
    *c = out;
}
static void restore_entries(void) {
    for (int k = 0; k < 2; k++) {
        if (!(eng.backed & (1 << k))) continue;
        if (eng.bslot[k] == g_Party->active_slot) eng.refresh = 1;
        *entry_of(eng.bslot[k]) = eng.backup[k];
    }
    eng.backed = 0;
}

// Where a fight starts: both sides a little off the centre, so the camera (on player 1) shows the opponent
#define FIGHT_P_X   (TNY_RING_CENTER_X - 40)
#define FIGHT_O_X   (TNY_RING_CENTER_X + 40)
static void place(Entity *e, int x, int y, int facing) {
    if (!e) return;
    e->x = x << 8;
    e->y = y << 8;
    e->facing = (u8)facing;
}

// Who is on the ring's floor for this fight: player 1 / player 2 alone (the other sits out: the Cell Games' solo
// modes) or both; the fighters' entries are written and the characters set
// The fighter on player 1's entity for a fight: his pick, or in a co-op 1 v 1 player 2's pick on player 2's own
// matches (solo 2: player 2's pad drives player 1's entity). A body change of player 1 goes through a reload of
// the ring map (the playables piece: a changed body in place keeps the old OBJ VRAM slot, a 64 x 64 one overflows it)
static int p1_slot_for(int who) { return eng.players == 2 && who == 1 ? eng.slot[1] : eng.slot[0]; }
static int needs_reload(int who) { return p1_slot_for(who) != g_Party->active_slot; }
static void warp_ring(void);
// Who is on the ring's floor: player 1 / player 2 alone (the other sits out: the Cell Games' solo modes) or both.
// Returns 1 when player 1's body changed: the ring reloads (P_RESTAGE) and the caller's step runs again on arrival.
static int stage_players(int who, int x) {
    int mode = eng.players == 2 && who != 2 ? (who == 0 ? 1 : 2) : 0;
    if (coop_solo() != mode) {
        if (!mode && eng.slot[1] < PARTY_SLOTS) coop.p2_slot = eng.slot[1];   // player 2 comes back on his pick
        cg_set_solo(mode);                         // 1 / 2: the other player's entity leaves the floor
    }
    int target = p1_slot_for(who);
    if (target != g_Party->active_slot) {
        set_active(target);
        coop.last_active = g_Party->active_slot;
        warp_ring();
        return 1;
    }
    if (mode == 0 && eng.players == 2 && coop.p2 && coop.p2_slot != eng.slot[1]) bind_p2(eng.slot[1]);
    heal_all();
    place(g_PlayerObject, x, TNY_RING_P1_Y, 3);
    if (who == 2) place(coop.p2, x, TNY_RING_P2_Y, 3);
    return 0;
}

// The tournament's characters, set up once before the warp into the ring: the picks' entries in their slots,
// player 1 on his pick (the warp's map load builds his body), player 2's slot (he respawns there)
static void enter_players(void) {
    apply_entry(0);
    if (eng.players == 2) apply_entry(1);
    set_active(eng.slot[0]);
    coop.last_active = g_Party->active_slot;
    if (eng.players == 2 && eng.slot[1] < PARTY_SLOTS) {
        if (coop.p2) bind_p2(eng.slot[1]); else coop.p2_slot = eng.slot[1];
    }
}

// Back to the players' own characters (the end, or a tournament dropped): entries restored, both slots back;
// the warp back (a map load) gives player 1 his own body again
static void unstage_players(void) {
    if (eng.players == 2 && coop_solo()) cg_set_solo(0);
    restore_entries();
    if (eng.players == 2 && eng.own[1] < PARTY_SLOTS) {
        if (coop.p2) bind_p2(eng.own[1]); else coop.p2_slot = eng.own[1];
    }
    set_active(eng.own[0]);
    coop.last_active = g_Party->active_slot;
    heal_all();
}

// The same fighter twice on the floor (2 v 2, the Rumble): player 2 drawn through a dark violet remap, the duel's
// shadow (duel.c builds the same for its clone); the hit flash clears the field, so the fight frame puts it back
#define OBJ_PAL ((volatile const u16 *)0x05000200)
static void shadow_on(void) {
    if (!eng.tint) {
        u8 *t = Arena_Alloc(256);
        if (!t) return;
        t[0] = 0;
        for (int i = 1; i < 256; i++) {
            u16 c = OBJ_PAL[i];
            int r = (c & 31) * 45 / 100, g = ((c >> 5) & 31) * 40 / 100, b = ((c >> 10) & 31) * 55 / 100 + 2;
            int best = i, bestd = 0x7FFFFFFF;
            for (int j = 1; j < 256; j++) {
                u16 d = OBJ_PAL[j];
                int dr = (d & 31) - r, dg = ((d >> 5) & 31) - g, db = ((d >> 10) & 31) - b;
                int dist = dr * dr + dg * dg + db * db;
                if (dist < bestd) { bestd = dist; best = j; }
            }
            t[i] = (u8)best;
        }
        eng.tint = t;
    }
    if (coop.p2 && *(u8 **)((u8 *)coop.p2 + 0x24) != eng.tint) {
        *(u8 **)((u8 *)coop.p2 + 0x24) = eng.tint;
        *(u32 *)((u8 *)coop.p2 + 0x20) = 0;          // the next frame is decoded through the remap
    }
}
static void shadow_off(void) {
    if (coop.p2 && eng.tint && *(u8 **)((u8 *)coop.p2 + 0x24) == eng.tint) {
        *(u8 **)((u8 *)coop.p2 + 0x24) = 0;
        *(u32 *)((u8 *)coop.p2 + 0x20) = 0;
    }
    if (eng.tint) Arena_Free(eng.tint);
    eng.tint = 0;
}
static int twins_on_floor(void) { return eng.players == 2 && eng.fighter == 2 && eng.pick[0] == eng.pick[1]; }

// ---- opponents --------------------------------------------------------------------------------------------
static const u32 no_action[2] = { ENEMY_ACTION_NONE, 0 };
static int in_list(const Entity *e) {
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) if (l->items[i] == e) return 1;
    return 0;
}
static int weight(int id) { return (tny_fighters[id].role & TR_GIANT) ? 2 : 1; }

static Entity *spawn_opp(int id, int x, int y) {
    int sprite = body_of(id);
    if (!sprite) return 0;
    s32 stat = id == TF_A16 ? A16_STAT_ID : id == TF_SUPER13 ? S13_STAT_ID : TNY_STAT0 + id;
    u32 buf[8];
    EnemyRecord *r = (EnemyRecord *)buf;
    memset(buf, 0, sizeof buf);
    r->handler = (void *)ENEMY_RECORD_HANDLER;
    r->flags = 1;                                   // script-spawned: boss rules
    r->stat_index = stat;
    r->x = (s16)x;
    r->y = (s16)y;
    r->sprite_id = sprite;
    r->action = (void *)no_action;
    r->behaviour_count = 0;
    Entity *e = enemy_spawn_hook(r);               // the co-op scaling and the difficulty; never a twin (tny_rule)
    if (!e) return 0;
    EnemyStat t;
    if (stat != TNY_STAT0 + id && tny_resolve_stat(TNY_STAT0 + id, &t)) {
        // 16 and Super 13 spawn on their own paths (android16.c / super13.c: their stats 109 / 145, their own AI and
        // specials) with their story numbers: the tournament's tier numbers replace them, the difficulty on top
        EnemyStat *st = (EnemyStat *)e->behaviour;
        st->max_hp = t.max_hp * diff_boss_hp_pct() / 100;
        e->hp = st->max_hp;
        st->melee = e->stat_melee = t.melee;
        st->ranged = e->stat_ranged = t.ranged;
        st->defence = e->defence = t.defence;
    }
    EntityList_Add(g_SceneTable[1], e);
    e->facing = 2;
    tny_moves_attach(e, id);
    return e;
}

static const s16 opp_spot[MAX_OPP][2] = {
    { 0, 0 }, { 0, 56 }, { 0, -56 }, { 48, 28 }, { 48, -28 }, { -96, 80 },
};
// Spawn waiting entrants while the ring has room (a giant counts double) and OBJ VRAM has a block for the body;
// `alone` (nobody of the fight stands): the next one comes in whatever the VRAM says, so a fight never stalls
static int spawn_waiting(int budget, int alone) {
    int load = 0, spawned = 0;
    for (int i = 0; i < eng.nopp; i++) if (eng.opp_in[i] == 1) load += weight(eng.opp_id[i]);
    for (int i = 0; i < eng.nopp; i++) {
        if (eng.opp_in[i] != 0) continue;
        int w = weight(eng.opp_id[i]);
        if (load && load + w > budget) break;
        if (!(alone && !load) && ObjVram_Largest(g_EntityList) < (w == 2 ? 0x1400 : 0x800)) break;   // wait for a free block
        // a bracket match's opponents enter on their side of the platform and walk in (step_intro); the Rumble's
        // fighters and later entrants appear on their marks
        int x0 = eng.kind != K_RUMBLE && eng.phase != P_MATCH ? TNY_RING_OPP_X : FIGHT_O_X;
        int x = x0 + opp_spot[i][0], y = TNY_RING_OPP_Y + opp_spot[i][1];
        if (x > TNY_RING_X1 - 16) x = TNY_RING_X1 - 16;
        if (y < TNY_RING_Y0 + 8) y = TNY_RING_Y0 + 8;
        if (y > TNY_RING_Y1 - 8) y = TNY_RING_Y1 - 8;
        Entity *e = spawn_opp(eng.opp_id[i], x, y);
        eng.opp[i] = e;
        eng.opp_in[i] = e ? 1 : 2;                  // no body (heap): counts as down
        if (e) { load += w; spawned++; eng.nwait--; }
        else eng.nwait--;
    }
    return spawned;
}

static void bars(void) {
    if (eng.kind == K_RUMBLE) return;               // a Rumble has no bars (too many fighters)
    int nbar = 0;
    for (int i = 0; i < eng.nopp && nbar < 2; i++) {
        Entity *e = eng.opp[i];
        if (!e || eng.opp_in[i] != 1) continue;
        BossBar *b = bossbar_create_hook(0, e, 0, 100);
        if (!b) continue;
        if (nbar == 1) b->y_target = 20;            // the second opponent's bar under the first (co-op bars)
        EntityList_Add(g_SceneTable[1], (Entity *)b);
        nbar++;
    }
}

static void clear_opps(void) {
    for (int i = 0; i < eng.nopp; i++) {
        Entity *e = eng.opp[i];
        eng.opp[i] = 0;
        if (!e || !in_list(e)) continue;
        if (e->hp > 0) e->hp = 0;
        EntityList_Broadcast(g_SceneTable[1], 0xF, e);   // "entity died": its bar lets go
        EntityList_Remove(g_SceneTable[1], e, 0);
    }
    // the fight's boss bars (bound to nobody now): off the screen before the next fight's
    EntityList *l = g_EntityList;
    for (int i = l->count - 1; i >= 0; i--) {
        Entity *b = l->items[i];
        if (b && (b->vtable == (u32 *)BOSSBAR_VTABLE || b->vtable == coop2.bar_vtable)) EntityList_Remove(g_SceneTable[1], b, -8);
    }
}

// Who hit an opponent last (damage calc): the beat-to-unlock needs a player's last hit (the Rumble's NPCs hit
// each other too)
void tny_note_hit(const Entity *defender, const Entity *attacker) {
    if (eng.magic != ENG_MAGIC || eng.phase != P_MATCH || !attacker) return;
    for (int i = 0; i < eng.nopp; i++) {
        if (eng.opp[i] != defender) continue;
        const Entity *p1 = coop2.p1_frame ? coop2.p1_frame : g_PlayerObject, *p2 = coop.p2;
        const Entity *who = attacker;
        if (who != p1 && (!p2 || who != p2)) who = *(const Entity *const *)((const u8 *)attacker + 0x1C);
        eng.opp_by[i] = (u8)(who && (who == p1 || who == p2));
        return;
    }
}

// a blow worth his HP through his own OnHit (test command 22): as story_test_finish does
static void lethal_hit(Entity *e) {
    u8 m[0x40];
    memset(m, 0, sizeof m);
    *(Entity **)(m + 0x20) = g_PlayerObject;
    *(Entity **)(m + 0x30) = g_PlayerObject;
    *(s32 *)(m + 0x2C) = e->hp;
    ((int (*)(Entity *, const u8 *))((u32)e->vtable + e->vtable[14]))(e, m + 0x24);
}

// ---- the ring presentation: the announcer at the ring's edge, the walk-in, the end poses -----------------------
// The walk is the stock script command (WalkToPosition's object, decompiled 2026-10-25: 0x08009D4A queues
// 0x08026620's command on the entity's command list +0x150 through 0x0800E04C; BeginCommandBatch sets the batch word
// 0x030020F8, CommitCommandBatch 0x0800E038 runs the command modal (0x0800DE20: message 0xB starts every queue, each
// frame message 10 steps them, entities without commands are only animated and drawn) until every queue is done).
// While a dialogue line shows the world is frozen but drawn (the dialogue modal's update, 0x08004EA8, draws the
// entity list without updating it), so a pose written into an entity's frame fields holds for the whole line.
ROM_FN(void *, Cmd_Walk, (void *mem, const s32 *xy), 0x08026621);
ROM_FN(void, Cmd_Queue, (u32 *batch, void *cmd, Entity *e), 0x0800E04D);
ROM_FN(void, Cmd_Begin, (u32 *batch), 0x0800E033);
ROM_FN(void, Cmd_Commit, (u32 *batch), 0x0800E039);
#define CMD_BATCH           ((u32 *)0x030020F8)
#define ACTOR_HANDLER       0x0800D6DFu
#define ANN_STANDIN         99                      // the grounds' stand-in for the announcer (a stock townsman)
#define ANN_PORTRAIT        140                     // art-buu's announcer portrait (Buu's Fury portrait 10)
#define ANN_X               TNY_RING_CENTER_X       // at the ring's top edge, between the fighters
#define ANN_Y               (TNY_RING_Y0 + 52)
#define E8(e, o)            (*(u8 *)((u8 *)(e) + (o)))
#define E16(e, o)           (*(u16 *)((u8 *)(e) + (o)))
#define E32(e, o)           (*(u32 *)((u8 *)(e) + (o)))

static int sprite_ok(int s) { return s >= 7 && (unsigned)s < sprite_index_count && Character_GetSpriteRecord(s) != 0; }
static int ann_sprite(void) { return sprite_ok(TS_ANNOUNCER) ? TS_ANNOUNCER : ANN_STANDIN; }
static int ann_speaker(void) { return ann_sprite() == TS_ANNOUNCER ? ANN_PORTRAIT : 0; }

// The announcer between fights (never during one: no OBJ VRAM taken from a fight). An actor (the stock character
// record handler) facing down; the line runs without him when OBJ VRAM is short.
static void ann_show(void) {
    static const u32 face_down[2] = { 0x0800CB0Fu, 0x7D00 };   // NpcBehavior_FaceAndWait: face down, ~9 min
    if (eng.ann && in_list(eng.ann)) return;
    eng.ann = 0;
    if (!in_ring() || ObjVram_Largest(g_EntityList) < 0x800) return;
    eng.ann_rec.handler = ACTOR_HANDLER;
    eng.ann_rec.flags = 0;
    eng.ann_rec.x = ANN_X;
    eng.ann_rec.y = ANN_Y;
    eng.ann_rec.sprite = ann_sprite();
    eng.ann_rec.count = 1;
    eng.ann_rec.beh = face_down;
    Entity *a = MapScript_CreateCharacter((const ActorRecord *)&eng.ann_rec);
    if (!a) return;
    EntityList_Add(g_SceneTable[1], a);
    eng.ann = a;
}
static void ann_hide(void) {
    Entity *a = eng.ann;
    eng.ann = 0;
    if (a && in_list(a) && a->sprite_record == Character_GetSpriteRecord(eng.ann_rec.sprite))
        EntityList_Remove(g_SceneTable[1], a, 0);
}

// Pose groups (a record's group = 4 facing arrays of 12-byte frames): a frame's descriptor is drawable, and an
// array's length is its run of drawable descriptors up to the next array of the record (docs/sprites.md "Frame counts")
static int desc_ok(const u8 *d) {
    u32 b = *(const u32 *)(d + 8);
    if (b < 0x08000000u || b >= 0x0A000000u || (b & 3)) return 0;
    if (!(d[2] == 8 || d[2] == 16 || d[2] == 32 || d[2] == 64) || !(d[3] == 8 || d[3] == 16 || d[3] == 32 || d[3] == 64)) return 0;
    return *(const u32 *)b <= 2;
}
static const u32 *grp_of(const Entity *e, int g) {
    const u8 *rec = (const u8 *)e->sprite_record;
    if (!rec || g >= rec[2]) return 0;
    const u32 *grp = (const u32 *)(rec + 0x4C + 0x10 * g);
    return grp[0] && grp[1] && grp[2] && grp[3] && desc_ok((const u8 *)grp[e->facing & 3]) ? grp : 0;
}
static int frames_of(const Entity *e, const u32 *grp) {
    const u8 *rec = (const u8 *)e->sprite_record, *d = (const u8 *)grp[e->facing & 3];
    int n = 1;
    for (; n < 8; n++) {
        const u8 *p = d + 12 * n;
        for (int g = 0; g < rec[2]; g++)
            for (int f = 0; f < 4; f++) if (*(const u8 *const *)(rec + 0x4C + 0x10 * g + 4 * f) == p) return n;
        if (!desc_ok(p)) break;
    }
    return n;
}
// Hold e in a pose for the line: lying (the loser) or the victory pose (the winner, facing the crowd)
static void pose_hold(Entity *e, int win) {
    if (!e || !in_list(e) || eng.npose >= 4) return;
    if (win) e->facing = 0;
    static const s8 lie[][2] = { { 12, -1 }, { 3, -1 }, { 2, 0 } };     // group, frame (-1 = the last)
    static const s8 cheer[][2] = { { 4, -1 }, { 14, 0 }, { 5, 0 } };
    const s8 (*tab)[2] = win ? cheer : lie;
    int g = -1, f = 0;
    if (!win && is_s13(e)) { g = S13_G_POWER; f = 0; }          // Super 13's group 12 is his death light: he kneels
    for (int i = 0; g < 0 && i < 3; i++) {
        const u32 *grp = grp_of(e, tab[i][0]);
        if (!grp) continue;
        g = tab[i][0];
        f = tab[i][1] < 0 ? frames_of(e, grp) - 1 : tab[i][1];
    }
    if (g < 0 || !grp_of(e, g)) return;
    int k = eng.npose++;
    eng.pose_e[k] = e;
    eng.pose_g[k] = (u8)g;
    eng.pose_f[k] = (u8)f;
}
static void poses_apply(int still) {
    for (int k = 0; k < eng.npose; k++) {
        Entity *e = eng.pose_e[k];
        const u32 *grp = e && in_list(e) ? grp_of(e, eng.pose_g[k]) : 0;
        if (!grp) continue;
        E32(e, 0x4C) = (u32)grp;
        E16(e, 0x14) = eng.pose_f[k];
        E16(e, 0x12) = 0;
    }
    // nobody moves while they are held (the frames before the line's freeze; never during it: A closes the line)
    if (!still) return;
    g_InputSystem->current = g_InputSystem->pressed = 0;
    coop.input2.current = coop.input2.pressed = 0;
}
static void poses_release(void) {
    for (int k = 0; k < eng.npose; k++) {
        Entity *e = eng.pose_e[k];
        if (e && in_list(e) && (e == g_PlayerObject || e == coop.p2)) Entity_SetAnimation(e, 0);
        eng.pose_e[k] = 0;
    }
    eng.npose = 0;
}

// The walk-in: the fighters from their sides of the platform to their marks (the command modal: nothing else
// moves meanwhile), then everyone faces his opponent
static void walk_cmd(Entity *e, int x, int y) {
    if (!e || !in_list(e) || E32(e, 0x150)) return;
    s32 xy[2] = { x, y };
    void *c = Cmd_Walk(0, xy);
    if (c) Cmd_Queue(CMD_BATCH, c, e);
}
static void walk_in(void) {
    Entity *p1 = g_PlayerObject, *p2 = eng.fighter == 2 && !coop_solo() ? coop.p2 : 0;
    Cmd_Begin(CMD_BATCH);
    walk_cmd(p1, FIGHT_P_X, TNY_RING_P1_Y);
    if (p2) walk_cmd(p2, FIGHT_P_X, TNY_RING_P2_Y);
    for (int i = 0; i < eng.nopp; i++)
        if (eng.opp_in[i] == 1 && eng.opp[i]) walk_cmd(eng.opp[i], FIGHT_O_X + opp_spot[i][0], eng.opp[i]->y >> 8);
    Cmd_Commit(CMD_BATCH);
    if (p1) p1->facing = 3;
    if (p2 && in_list(p2)) p2->facing = 3;
    for (int i = 0; i < eng.nopp; i++) if (eng.opp_in[i] == 1 && eng.opp[i] && in_list(eng.opp[i])) eng.opp[i]->facing = 2;
}

// ---- the fight ---------------------------------------------------------------------------------------------
static const char *pname(int k) { return tny_fighters[eng.pick[k]].name; }
static const char *fname(int id) { return id < TF_COUNT ? tny_fighters[id].name : "?"; }

static int opp_budget(void);
static void start_fight(int kind, int who, const u8 *ids, int n) {
    eng.kind = (u8)kind;
    eng.fighter = (u8)who;
    eng.result = R_NONE;
    eng.ko = 0;
    eng.nopp = 0;
    for (int i = 0; i < n && i < MAX_OPP; i++) {
        eng.opp_id[i] = ids[i];
        eng.opp_by[i] = kind != K_RUMBLE;           // 1 v 1 / 2 v 2: only players hit him
        eng.opp_in[i] = 0;
        eng.down_t[i] = 0;
        eng.opp[i] = 0;
        eng.nopp++;
    }
    eng.nwait = eng.nopp;
    eng.lone = kind == K_CHALLENGE && n == 1 && who == 2;
    eng.tint = 0;                                    // an arena block of an earlier map is gone
    // the caller checked needs_reload(who) first. A bracket match / a challenge: everyone on his side of the
    // platform, the walk-in and the announcer's call come in step_intro; a Rumble round: the players on their marks,
    // the pack comes in after the announcer's line (he never shares the floor with a pack)
    stage_players(who, kind == K_RUMBLE ? FIGHT_P_X : TNY_RING_P1_X);
    if (kind != K_RUMBLE) {
        spawn_waiting(opp_budget(), 1);
        if (eng.chal_final && kind == K_CHALLENGE)
            for (int i = 0; i < eng.nopp; i++) if (eng.opp[i]) tny_moves_final_form(eng.opp[i]);
    }
    eng.lone = 0;
    eng.timer = 0;
    eng.pres = 0;
    eng.phase = P_INTRO;
}

// The walk-in and the announcer's call (the line was prepared by the caller: ann_prep), then the fight
static void step_intro(void) {
    if (eng.pres == 0) {
        ann_show();
        if (eng.kind != K_RUMBLE) walk_in();
        say_go();
        eng.pres = 1;
        return;
    }
    eng.pres = 0;
    ann_hide();
    if (eng.kind == K_RUMBLE) spawn_waiting(opp_budget(), 1);
    bars();
    if (eng.kind == K_RUMBLE) tny_rumble_mode(1);
    eng.timer = 0;
    eng.phase = P_MATCH;
}

static int opp_budget(void) { return eng.kind == K_RUMBLE ? 5 : MAX_OPP * 2; }

static void fight_frame(void) {
    if (eng.timer < 0xFFFF) eng.timer++;
    if (twins_on_floor()) shadow_on();
    int alive = 0;
    for (int i = 0; i < eng.nopp; i++) {
        if (eng.opp_in[i] == 2 && eng.kind == K_RUMBLE && eng.opp[i]) {   // a fallen Rumble fighter's body leaves: its
            Entity *d = eng.opp[i];                                       // OBJ VRAM goes to the next entrant
            if (!in_list(d)) eng.opp[i] = 0;
            else if (++eng.down_t[i] >= 60) {
                EntityList_Broadcast(g_SceneTable[1], 0xF, d);
                EntityList_Remove(g_SceneTable[1], d, 0);
                eng.opp[i] = 0;
            }
            continue;
        }
        if (eng.opp_in[i] != 1) continue;
        Entity *e = eng.opp[i];
        if (!e || !in_list(e) || e->hp <= 0) {
            if (e && in_list(e) && tny_moves_last_hitter) {      // the kit's own note of the last hitter
                const Entity *h = tny_moves_last_hitter(e);
                if (h) eng.opp_by[i] = (u8)(h == g_PlayerObject || h == coop.p2);
            }
            eng.opp_in[i] = 2;
            continue;
        }
        alive++;
    }
    if (eng.nwait && eng.timer > 30 && (eng.timer & 15) == 0) spawn_waiting(opp_budget(), !alive);
    if (eng.result == R_NONE) {
        if (!alive && !eng.nwait && eng.timer > 10) eng.result = R_WON;
    }
    if (eng.result != R_NONE) {
        eng.phase = P_MATCH_END;
        eng.timer = 0;
    }
}

int tny_ring_active(void) {
    return eng.magic == ENG_MAGIC && eng.phase >= P_TO_RING && eng.phase < P_EXITING && in_ring();
}

int tny_ko_check(int k, s32 dmg) {
    if (eng.magic != ENG_MAGIC || coop2.duel_active || !tny_ring_active()) return 0;
    const CharacterEntry *c = &g_Party->chars[k ? coop.p2_slot : g_Party->active_slot];
    if (dmg < c->hp) return 0;
    if (eng.phase != P_MATCH) return 1;              // after the fight's end (a blast still flying): 1 HP, no more
    eng.koed = 1;
    // a fight of both players: while the partner stands the revive takes the KO (revive.c)
    if (eng.fighter == 2 && eng.players == 2 && coop.p2 && g_PlayerObject && !coop_solo()) {
        Entity *partner = k ? g_PlayerObject : coop.p2;
        if (!revive_is_down(partner) && !(eng.ko & (1 << !k))) return 0;
    }
    eng.ko |= (u8)(1 << k);
    eng.result = R_LOST;
    return 1;
}

// ---- the bracket ----------------------------------------------------------------------------------------------
static int unit_w(void) { return eng.division == DIV_2V2 ? 2 : 1; }
static int units(void) { return eng.br.size / unit_w(); }
static int side(int r, int m, int s) {
    if (r == 0) {
        int lead = (2 * m + s) * unit_w();
        return eng.br.seat[lead] == NO ? NO : lead;
    }
    return eng.br.winner[r - 1][2 * m + s];
}
// which players sit in a unit: bit 0 player 1, bit 1 player 2
static int players_in(int lead) {
    if (lead == NO) return 0;
    int v = 0;
    if (eng.br.p1_seat != NO && (eng.br.p1_seat == lead || (unit_w() == 2 && eng.br.p1_seat == lead + 1))) v |= 1;
    if (eng.br.p2_seat != NO && (eng.br.p2_seat == lead || (unit_w() == 2 && eng.br.p2_seat == lead + 1))) v |= 2;
    return v;
}
static u32 power_of(int lead) {
    u32 p = 0;
    for (int i = 0; i < unit_w(); i++) {
        int id = eng.br.seat[lead + i];
        if (id < TF_GOHAN) p += opp_tab[id].power;
        else if (id != NO) p += 100;
    }
    return p ? p : 1;
}
static int simulate(int a, int b) {
    u32 pa = power_of(a), pb = power_of(b);
    pa *= pa; pb *= pb;
    return rnd(pa + pb) < pa ? a : b;        // stronger wins mostly: HP x attack ~ power^2, a little luck
}
static void set_winner(int r, int m, int w) {
    eng.br.winner[r][m] = (u8)w;
    eng.played[r] |= (u8)(1 << m);
    eng.results = 1;
}

// The next match a player plays (round, match into br.round / br.match); NPC matches before it resolve
static int find_next(void) {
    for (int r = eng.br.round == NO ? 0 : eng.br.round; r < eng.br.rounds; r++) {
        int nm = units() >> (r + 1);
        for (int m = 0; m < nm; m++) {
            if (eng.played[r] & (1 << m)) continue;
            int a = side(r, m, 0), b = side(r, m, 1);
            int pa = players_in(a) & ~eng.out, pb = players_in(b) & ~eng.out;
            if ((pa || pb) && a != NO && b != NO) { eng.br.round = (u8)r; eng.br.match = (u8)m; return 1; }
            set_winner(r, m, a == NO ? b : b == NO ? a : simulate(a, b));
        }
        eng.br.round = (u8)(r + 1);
    }
    eng.br.match = 0;
    return 0;
}

static void shuffle(u8 *a, int n) {
    for (int i = n - 1; i > 0; i--) { int j = (int)rnd((u32)i + 1); u8 t = a[i]; a[i] = a[j]; a[j] = t; }
}

static void draw_bracket(void) {
    TBracket *b = &eng.br;
    memset(b, NO, sizeof *b);
    b->size = (u8)(eng.division == DIV_2V2 ? 8 : eng.full ? 16 : 8);
    b->rounds = (u8)(eng.division == DIV_2V2 ? 2 : eng.full ? 4 : 3);
    b->round = 0;
    b->match = 0;
    b->tier = eng.tier;
    b->division = eng.division;
    memset(eng.played, 0, sizeof eng.played);
    int w = unit_w(), nu = b->size / w;
    if (eng.division == DIV_2V2) {                  // the players are one team
        int t = (int)rnd((u32)nu);
        b->p1_seat = (u8)(t * 2);
        b->p2_seat = (u8)(t * 2 + 1);
    } else if (eng.players == 2) {                  // opposite halves: they can only meet in the final
        b->p1_seat = (u8)rnd((u32)b->size / 2);
        b->p2_seat = (u8)(b->size / 2 + rnd((u32)b->size / 2));
    } else {
        b->p1_seat = (u8)rnd(b->size);
    }
    b->seat[b->p1_seat] = eng.pick[0];
    if (b->p2_seat != NO) b->seat[b->p2_seat] = eng.pick[1];
    u8 pool[TF_GOHAN];
    int n = pool_of(eng.tier, pool), k = 0;
    shuffle(pool, n);
    if (eng.force_opp != NO) {                      // test command 21: him first, into the seat across from player 1
        int j = 0;
        while (j < n && pool[j] != eng.force_opp) j++;
        if (j == n && n < TF_GOHAN) n++;
        for (; j > 0; j--) pool[j] = pool[j - 1];
        pool[0] = eng.force_opp;
    }
    // fill unit by unit so a bye is a whole unit at the end of the draw (a 2 v 2 team is never half empty)
    for (int u = 0; u < nu; u++)
        for (int i = 0; i < w; i++) {
            int s = u * w + i;
            if (s == b->p1_seat || s == b->p2_seat) continue;
            b->seat[s] = k < n ? pool[k++] : NO;
        }
    if (eng.force_opp != NO) {                      // the forced fighter sits across from player 1
        int across = w == 2 ? ((b->p1_seat >> 1) ^ 1) * 2 : b->p1_seat ^ 1;
        for (int s = 0; s < b->size; s++)
            if (b->seat[s] == eng.force_opp && s != across && s != b->p1_seat && s != b->p2_seat) {
                b->seat[s] = b->seat[across];
                b->seat[across] = eng.force_opp;
                break;
            }
    }
    eng.results = 0;
    eng.br_shown = 0;
}

// ---- the Rumble ----------------------------------------------------------------------------------------------
// Rounds per tier (Ryan, 2026-10-25): fighters in the ring at once, players included; then the headliner with two
// helpers. Giants: the Z-Fighter's last round has one, the Legend's rounds from the second on may.
static const u8 rumble_counts[TIER_COUNT][4] = { { 3, 3 }, { 3, 4, 4 }, { 4, 4, 4 }, { 4, 4, 4, 4 }, { 4, 4, 4, 4 } };
static const u8 rumble_rounds[TIER_COUNT] = { 2, 3, 3, 4, 4 };
static const u8 headliners[TIER_COUNT][2] = {
    { TF_KING_COLD, NO }, { TF_A17, TF_A18 }, { TF_A16, NO }, { TF_BROLY, NO }, { TF_SUPER_BUU, NO } };

static int used(int id) { return (eng.used[id >> 3] >> (id & 7)) & 1; }
static void use(int id) { eng.used[id >> 3] |= (u8)(1 << (id & 7)); }
static int is_headliner(int id) {
    for (int i = 0; i < 2; i++) if (headliners[eng.tier][i] == id) return 1;
    return 0;
}

// Draw `slots` worth of fighters (a giant counts double) from the pool, none twice in this Rumble
static int draw_pack(u8 *out, int slots, int giant) {
    u8 pool[TF_GOHAN];
    int n = pool_of(eng.tier, pool), k = 0;
    shuffle(pool, n);
    if (giant >= 0) {                                // a giant first when the round wants one
        for (int i = 0; i < n && slots >= 2; i++) {
            int id = pool[i];
            if (used(id) || is_headliner(id) || id == eng.force_opp || weight(id) != 2) continue;
            out[k++] = (u8)id; use(id); slots -= 2;
            break;
        }
    }
    for (int i = 0; i < n && slots > 0 && k < MAX_OPP; i++) {
        int id = pool[i];
        if (used(id) || is_headliner(id) || id == eng.force_opp) continue;
        if (weight(id) == 2 && (giant < 0 || slots < 2)) continue;
        out[k++] = (u8)id; use(id); slots -= weight(id);
    }
    return k;
}

static void rumble_round(void) {
    u8 ids[MAX_OPP];
    int n;
    int regular = rumble_rounds[eng.tier];
    if (eng.rb_round < regular) {
        int slots = rumble_counts[eng.tier][eng.rb_round] - eng.players;
        if (slots < 1) slots = 1;
        int giant = -1;
        if (eng.tier == TIER_ZFIGHTER && eng.rb_round == regular - 1) giant = 1;
        if (eng.tier == TIER_LEGEND && eng.rb_round >= 1) giant = 1;
        if (giant > 0 && slots < 2) slots = 2;      // a giant needs the room of two
        n = draw_pack(ids, slots, giant);
    } else {                                        // the headliner(s) with helpers: three fighters
        n = 0;
        if (eng.force_opp != NO) { ids[n++] = eng.force_opp; use(eng.force_opp); }   // test command 21
        for (int i = 0; n == 0 && i < 2; i++) {
            int id = headliners[eng.tier][i];
            if (id != NO && tny_in_pool(id) && !used(id)) { ids[n++] = (u8)id; use(id); }
        }
        if (!n) {                                   // his art or his story flag is missing: the strongest left
            u8 pool[TF_GOHAN];
            int np = pool_of(eng.tier, pool), best = -1;
            for (int i = 0; i < np; i++)
                if (!used(pool[i]) && (best < 0 || opp_tab[pool[i]].power > opp_tab[best].power)) best = pool[i];
            if (best >= 0) { ids[n++] = (u8)best; use(best); }
        }
        u8 help[MAX_OPP];
        int h = draw_pack(help, 3 - n, -1);
        // the headliner enters last: helpers first in the list (spawn order)
        u8 all[MAX_OPP];
        int a = 0;
        for (int i = 0; i < h; i++) all[a++] = help[i];
        for (int i = 0; i < n; i++) all[a++] = ids[i];
        n = a;
        for (int i = 0; i < n; i++) ids[i] = all[i];
    }
    u8 down[MAX_OPP] = { 0 };
    tny_ui_rumble_board(ids, down, n);
    if (!n) { eng.result = R_WON; eng.kind = K_RUMBLE; eng.phase = P_MATCH_END; eng.timer = 90; return; }
    char num[2] = { (char)('1' + eng.rb_round), 0 };
    if (eng.rb_round < regular) ann_prep("Royal Rumble, round ", num, "!  Last ones standing win!", 0, 0);
    else ann_prep("And now, the headliner: ", fname(ids[n - 1]), "!  Begin!", 0, 0);
    start_fight(K_RUMBLE, eng.players == 2 ? 2 : 0, ids, n);
}

// ---- prizes, unlocks ----------------------------------------------------------------------------------------
static void exp_trickle(int slot) {
    if (slot < 0 || slot >= PARTY_SLOTS || !in_party(slot)) return;
    CharacterEntry *c = entry_of(slot);
    int lv = Character_GetLevel(c);
    if (lv >= 50) return;
    s32 span = Character_LevelExp(lv + 1) - Character_LevelExp(lv);
    s32 amount = span / 16;                          // a Quick bracket: about a fifth of a level
    if (amount < 1) amount = 1;
    u32 *ev = Arena_Alloc(16);
    if (!ev) return;
    ev[0] = (u32)coop.exp_vtable; ev[1] = 1; ev[2] = (u32)amount; ev[3] = (u32)slot;   // exp.c's event
    g_EntityList->events[g_EntityList->event_count++] = ev;
}

static void unlock(int id) {
    if (id >= TF_COUNT || !(tny_fighters[id].role & (TR_BEAT | TR_CHALLENGER))) return;
    rec_load();
    if (rec_bit(id)) return;
    eng.rec.unlocked[id >> 3] |= (u8)(1 << (id & 7));
    rec_save();
    tny_notify(new_fighter);
}

// The EXP goes to the player's story slot, or to his own character when he fought as a tournament fighter
static int exp_slot(int k) {
    const TFighter *f = &tny_fighters[eng.pick[k]];
    return (f->role & TR_STORY) ? eng.slot[k] : eng.own[k];
}

// ---- director ----------------------------------------------------------------------------------------------
// A warp (the WarpToMapNoEntities opcode) through the test-warp command, so it runs where that one does: at the
// start of the entity-list update, before the events (a map change inside an event fire would free the arena
// under the event loop). The arrival: player 1 is a new entity on the target map (eng.from = the old one).
static void warp(int zone, int area, int x, int y) {
    coop2.cmd_arg = (u32)(zone | area << 8);
    coop2.cmd_arg2 = (u32)(x | y << 16);
    coop2.cmd = COOP_CMD_WARP;
    eng.from = g_PlayerObject;
    eng.timer = 0;
}
static void warp_ring(void) { warp(TNY_RING_ZONE, TNY_RING_AREA, FIGHT_P_X, TNY_RING_P1_Y); }
static void restage(int back) { eng.back = (u8)back; eng.phase = P_RESTAGE; }
static int arrived(int zone, int area) {
    if (eng.timer < 0xFFFF) eng.timer++;
    // a ring-to-ring reload may give the new player 1 the old one's address: then 90 frames settle it
    int in = eng.timer > 20 && on_map(zone, area) && g_PlayerObject && (g_PlayerObject != eng.from || eng.timer > 90)
             && !coop2.cmd && p1_in_play() && !scene_running();
    if (in) { eng.ann = 0; eng.npose = 0; }          // a new map: the announcer and the posed bodies are gone
    return in;
}

static void begin_tournament(void) {
    eng.out = 0;
    eng.koed = 0;
    eng.rb_round = 0;
    memset(eng.used, 0, sizeof eng.used);
    eng.nchal = 0;
    if (eng.seeded) eng.seeded = 0;
    else eng.rng ^= g_FrameCounter * 2654435761u;
    eng.br.size = 0;
    if (eng.division == DIV_RUMBLE) {
        eng.rb_rounds = (u8)(rumble_rounds[eng.tier] + 1);
        memset(&eng.br, NO, sizeof eng.br);
        eng.br.size = 0;
        eng.br.tier = eng.tier;
        eng.br.division = eng.division;
    } else {
        draw_bracket();
    }
}

static void step_director(void) {
    int players = (coop.p2 || p2_present()) && !companion_active() ? 2 : 1;
    eng.players = (u8)players;
    if (!eng.preset) {
        // the tier (when more than one is open), the division, Quick / Full
        const char *lines[TIER_COUNT];
        u8 tiers[TIER_COUNT];
        int n = 0;
        for (int t = 0; t < TIER_COUNT; t++) if (tny_tier_open(t)) { tiers[n] = (u8)t; lines[n++] = tier_name[t]; }
        int c = n > 1 ? tny_ui_choose("Which tournament?", lines, n) : 0;
        if (c < 0 || c >= n) { eng.phase = P_IDLE; return; }
        eng.tier = tiers[c];
        static const char *const div2[3] = { "1 v 1", "2 v 2", "Royal Rumble" };
        static const char *const div1[2] = { "1 v 1", "Royal Rumble" };
        c = players == 2 ? tny_ui_choose("Which division?", div2, 3) : tny_ui_choose("Which division?", div1, 2);
        if (c < 0) { eng.phase = P_IDLE; return; }
        eng.division = (u8)(players == 2 ? c : c == 1 ? DIV_RUMBLE : DIV_1V1);
        eng.full = 0;
        if (eng.division == DIV_1V1 && eng.tier != TIER_ROOKIE && pool_of(eng.tier, 0) >= 15) {
            static const char *const size[2] = { "Quick (8 fighters)", "Full (16 fighters)" };
            c = tny_ui_choose("Which bracket?", size, 2);
            if (c < 0) { eng.phase = P_IDLE; return; }
            eng.full = (u8)c;
        }
    }
    if (eng.division == DIV_2V2 && players < 2) eng.division = DIV_1V1;   // 2 v 2 is co-op only
    if (eng.full && pool_of(eng.tier, 0) < 15) eng.full = 0;
    eng.own[0] = g_Party->active_slot;
    eng.own[1] = players == 2 ? coop.p2_slot : NO;
    for (int k = 0; k < players; k++) {
        u8 p = NO;
        if (eng.preset) p = eng.preset_pick[k] != NO ? eng.preset_pick[k] : (u8)own_pick(eng.own[k]);
        else if (!tny_ui_select(k, eng.tier, &p)) { eng.phase = P_IDLE; eng.preset = 0; return; }
        if (!pick_playable(p)) p = (u8)own_pick(eng.own[k]);
        eng.pick[k] = p;
    }
    eng.preset = 0;
    resolve_slots();
    enter_players();
    begin_tournament();
    // where the director stands (the way back) and off to the ring
    const u8 *m = map_state();
    Entity *p1 = g_PlayerObject;
    eng.ret_zone = m ? m[0] : 0;
    eng.ret_area = m ? m[1] : 0;
    eng.ret_x = p1 ? (s16)(p1->x >> 8) : 0;
    eng.ret_y = p1 ? (s16)(p1->y >> 8) : 0;
    warp_ring();
    eng.phase = P_TO_RING;
}

// ---- next ---------------------------------------------------------------------------------------------------
static int champion_seat(void) { return eng.br.rounds ? eng.br.winner[eng.br.rounds - 1][0] : NO; }

static void step_next(void) {
    if (eng.division == DIV_RUMBLE) {
        if (eng.rb_round >= eng.rb_rounds) { eng.phase = P_TROPHY; return; }
        rumble_round();
        return;
    }
    if (!find_next()) {                              // the bracket is played out
        tny_ui_bracket(eng.br_shown ? 1 : 0);
        eng.br_shown = 1;
        eng.results = 0;
        eng.phase = (players_in(champion_seat()) & ~eng.out) ? P_TROPHY : P_EXIT;
        return;
    }
    int r = eng.br.round, m = eng.br.match;
    int a = side(r, m, 0), b = side(r, m, 1);
    int pa = players_in(a) & ~eng.out, pb = players_in(b) & ~eng.out;
    int duel = eng.division == DIV_1V1 && pa && pb;
    int who = duel ? 2 : (pa | pb) == 3 ? 2 : (pa | pb) == 2 ? 1 : 0;
    if (needs_reload(who)) { stage_players(who, FIGHT_P_X); restage(P_NEXT); return; }   // this step again after the reload
    tny_ui_bracket(eng.br_shown ? 1 : 0);            // the draw once (0), then the results since (1)
    eng.br_shown = 1;
    eng.results = 0;
    static const char *const round_name[4] = { "Round 1: ", "Round 2: ", "Round 3: ", "Round 4: " };
    const char *round = r == eng.br.rounds - 1 ? "The final: " : round_name[r & 3];
    if (duel) {                                      // the co-op final: player 1 against player 2, a duel
        stage_players(2, FIGHT_P_X);                 // player 2's entity comes back on his pick when player 1 stands
        coop2.call_end_seq = 0;
        coop2.call_slot = (s8)eng.slot[1];            // duel_start binds player 2 there
        coop2.call_flags = (u8)(eng.pick[0] == eng.pick[1] ? DUEL_SHADOW : 0);   // the same fighter twice: the shadow
        coop2.duel_winner = 0;
        coop2.call_state = 1;                        // vmcall.c starts it once player 2 stands (the CoopCall 1 path)
        eng.phase = P_DUEL;
        eng.timer = 0;
        ann_show();                                  // he leaves when the duel starts (the frame tick)
        ann_prep(round, pname(0), " versus ", pname(1), "!  Begin!");
        say_go();
        return;
    }
    int lead = pa ? b : a;                           // the opponents' unit
    u8 ids[2];
    int n = 0;
    for (int i = 0; i < unit_w(); i++) if (eng.br.seat[lead + i] != NO) ids[n++] = eng.br.seat[lead + i];
    if (eng.division == DIV_2V2) {
        char *o = say_open(ann_speaker());
        o = cat(o, round);
        o = cat(o, "your team versus ");
        o = catu(o, fname(ids[0]), 1);
        if (n > 1) { o = cat(o, " & "); o = catu(o, fname(ids[1]), 1); }
        cat(o, "!  Begin!");
    } else {
        ann_prep(round, pname(who), " versus ", fname(ids[0]), "!  Begin!");
    }
    start_fight(K_MATCH, who, ids, n);
}

// ---- the end of a fight ---------------------------------------------------------------------------------------
static const char *chal_name(int i);
static Entity *floor_p2(void) { return eng.fighter == 2 && coop.p2 && !coop_solo() ? coop.p2 : 0; }

// The end's presentation: the loser lies down, the winner shows his pose, the announcer calls it; the poses hold
// while the line shows (the step after it releases them)
static void end_show(void) {
    int won = eng.result == R_WON, k = eng.fighter == 2 ? 0 : eng.fighter;
    const char *team = eng.division == DIV_2V2 || (eng.fighter == 2 && eng.players == 2) ? "your team" : 0;
    const char *me = team ? team : pname(k);
    if (eng.kind == K_RUMBLE && won) clear_opps();  // a cleared round: the fallen leave before he comes in
    Entity *pl[2] = { g_PlayerObject, floor_p2() };
    for (int i = 0; i < 2; i++) if (pl[i]) pose_hold(pl[i], won);
    for (int i = 0; i < eng.nopp; i++) {
        Entity *e = eng.opp[i];
        if (!e || !in_list(e)) continue;
        if (won) pose_hold(e, 0);
        else if (e->hp > 0) pose_hold(e, 1);
    }
    ann_show();
    if (eng.kind == K_CHALLENGE) {
        if (won) ann_prep("", me, " defeats ", chal_name(0), eng.nchal > 1 ? " and partner!" : "!");
        else ann_prep("", chal_name(0), eng.nchal > 1 ? " and partner win!" : " wins!", 0, 0);
    } else if (eng.kind == K_RUMBLE) {
        if (won) ann_prep(eng.rb_round + 1 < eng.rb_rounds ? "Round cleared!  " : "The headliner is down!  ", me,
                          team ? " stand tall!" : " stands tall!", 0, 0);
        else ann_prep("", me, team ? " are out of the Royal Rumble!" : " is out of the Royal Rumble!", 0, 0);
    } else {
        if (won) ann_prep("The winner...  ", me, "!", 0, 0);
        else ann_prep("", eng.nopp > 1 ? "your opponents" : fname(eng.opp_id[0]), eng.nopp > 1 ? " win!  " : " wins!  ",
                      me, team ? " are out of the tournament!" : " is out of the tournament!");
    }
    say_go();
    eng.pres = 1;
}

static void step_match_end(void) {
    if (eng.pres == 0) { end_show(); return; }
    eng.pres = 0;
    int won = eng.result == R_WON;
    if (eng.kind == K_RUMBLE) tny_rumble_mode(0);
    if (won) for (int i = 0; i < eng.nopp; i++)
        if (eng.opp_by[i] && (tny_fighters[eng.opp_id[i]].role & TR_BEAT)) unlock(eng.opp_id[i]);
    clear_opps();
    shadow_off();
    heal_all();
    if (won) {
        if (eng.fighter == 2 || eng.players == 1) { exp_trickle(exp_slot(0)); if (eng.players == 2) exp_trickle(exp_slot(1)); }
        else exp_trickle(exp_slot(eng.fighter));
    }
    int k = eng.fighter == 2 ? 0 : eng.fighter;
    if (eng.kind == K_CHALLENGE) {
        rec_load();
        if (won && eng.chal_final) { eng.rec.extra |= 1; rec_save(); }   // the final form beaten: the record's last line
        if (won) for (int i = 0; i < eng.nchal; i++) unlock(eng.chal[i]);
        else for (int i = 0; i < eng.nchal; i++) {
            if (eng.chal[i] < TF_PUIPUI || eng.chal[i] > TF_VEGITO) continue;   // an alternative: no attempt line
            u8 *a = &eng.rec.attempts[eng.chal[i] - TF_PUIPUI];
            if (*a < 255) (*a)++;
            rec_save();
        }
        if (won && eng.chal_final) say("Super Buu's final form is beaten!  The Legend Royal Rumble is yours!", 0, 0, 0, 0);
        else if (won) say(fname(eng.chal[0]), eng.nchal > 1 ? " and partner join the tournament fighters!" : " joins the tournament fighters!", 0, 0, 0);
        else say("Come back when you've won again!", 0, 0, 0, 0);
        eng.phase = P_EXIT;
        return;
    }
    if (eng.kind == K_RUMBLE) {
        if (won) { eng.rb_round++; eng.phase = P_NEXT; }
        else eng.phase = P_EXIT;
        return;
    }
    // a bracket match
    int r = eng.br.round, m = eng.br.match;
    int a = side(r, m, 0), b = side(r, m, 1);
    int pa = players_in(a) & ~eng.out;
    int mine = pa ? a : b, theirs = pa ? b : a;
    set_winner(r, m, won ? mine : theirs);
    if (!won) {
        if (eng.division == DIV_2V2) eng.out = 3;
        else eng.out |= (u8)(1 << k);
    }
    eng.phase = P_NEXT;
}

// ---- the winner -------------------------------------------------------------------------------------------------
static int winner_player(void) {                     // 0 / 1, or 2 = both (team, Rumble)
    if (eng.division != DIV_1V1) return 2;
    int p = players_in(champion_seat()) & ~eng.out;
    return p == 2 ? 1 : 0;
}

static void step_trophy(void) {
    int t = tny_tourney_index(eng.tier, eng.division);
    rec_load();
    if (eng.rec.wins[t] < 255) eng.rec.wins[t]++;
    if (eng.full && eng.rec.full_wins[eng.tier] < 255) eng.rec.full_wins[eng.tier]++;
    rec_save();
    int w = winner_player();
    const char *who = w == 2 ? (eng.players == 2 ? "your team" : pname(0)) : pname(w);
    pose_hold(g_PlayerObject, 1);
    if (coop.p2 && !coop_solo()) pose_hold(coop.p2, 1);
    ann_show();
    char *o = say_open(ann_speaker());
    o = cat(o, "Ladies and gentlemen...  ");
    o = catu(o, who, 1);
    o = cat(o, w == 2 && eng.players == 2 ? " win the " : " wins the ");
    o = cat(o, tier_name[eng.tier]);
    cat(o, eng.division == DIV_RUMBLE ? " Royal Rumble!" : eng.division == DIV_2V2 ? " tournament (2 v 2)!" : " tournament!");
    say_go();
    eng.phase = P_PRIZE;
}

static int won_as(int id) {
    int w = winner_player();
    for (int k = 0; k < eng.players; k++) if ((w == 2 || w == k) && eng.pick[k] == id) return 1;
    return 0;
}

// The hidden fight after a win (docs/tournament.md section 6, the rule decided 2026-10-25): the tournament's own
// challenger(s) when still locked, in the build and his condition met; otherwise the tournament's *alternative*
// when still locked: Champion Rumble -> Majin Vegeta (not won as Hercule: no Uub), Legend 1 v 1 -> Goku (Buu saga,
// the SSJ3 path; not won as Goku or Vegeta: no Vegito), Legend 2 v 2 -> Goku (Buu saga) and Majin Vegeta (the forms),
// Legend Rumble -> Super Buu in his final form (Gohan absorbed). Alternatives are beat-to-unlock fighters: winning
// the challenge unlocks them as beating them anywhere does; a lost one records no attempt (that line is the
// challengers'). The final form is the exception to "still locked": the Rumble's headliner is Super Buu himself (beaten
// there, he is unlocked already), so his final form comes after every Legend Rumble win until he is beaten in it once
// (the record's `extra` bit 0: the record's last line).
static const struct { u8 t, a, b, final; } alt_chal[] = {
    { TIER_CHAMPION * DIV_COUNT + DIV_RUMBLE, TF_MAJIN_VEGETA, NO, 0 },
    { TIER_LEGEND * DIV_COUNT + DIV_1V1, TF_BGOKU, NO, 0 },
    { TIER_LEGEND * DIV_COUNT + DIV_2V2, TF_BGOKU, TF_MAJIN_VEGETA, 0 },
    { TIER_LEGEND * DIV_COUNT + DIV_RUMBLE, TF_SUPER_BUU, NO, 1 },
};
static const char *chal_name(int i) {
    if (i == 0 && eng.chal_final) return "Super Buu, final form";
    return fname(eng.chal[i]);
}
static int pick_challengers(int t) {
    eng.nchal = 0;
    eng.chal_final = 0;
    for (int id = TF_PUIPUI; id <= TF_VEGITO && eng.nchal < 2; id++) {
        const TFighter *f = &tny_fighters[id];
        if (f->challenger_of != t || rec_bit(id) || !body_of(id)) continue;
        if (id == TF_UUB && !won_as(TF_P_HERCULE)) continue;                       // "won as Hercule"
        if (id == TF_VEGITO && !won_as(TF_GOKU) && !won_as(TF_VEGETA)) continue;   // "won as Goku or Vegeta"
        eng.chal[eng.nchal++] = (u8)id;
    }
    for (unsigned i = 0; !eng.nchal && i < sizeof alt_chal / sizeof alt_chal[0]; i++) {
        if (alt_chal[i].t != t) continue;
        const u8 ids[2] = { alt_chal[i].a, alt_chal[i].b };
        int open = alt_chal[i].final ? !(eng.rec.extra & 1) : 1;
        for (int j = 0; j < 2 && open; j++)
            if (ids[j] != NO && (alt_chal[i].final || !rec_bit(ids[j])) && body_of(ids[j])) eng.chal[eng.nchal++] = ids[j];
        eng.chal_final = (u8)(eng.nchal && alt_chal[i].final);
    }
    return eng.nchal;
}

// The prizes (Ryan, 2026-10-25): the first win of each tournament a +3 capsule (the stat turns with the tier and
// the division: five of each across the fifteen), Full a second one, every later win a Senzu bean (the Rumble a
// second one with no player KO); then the hidden fight, if one is still locked
static void step_prize(void) {
    int t = tny_tourney_index(eng.tier, eng.division);
    static const u8 capsule[3] = { ITEM_STR3, ITEM_PWR3, ITEM_END3 };
    rec_load();
    int paid = 0;
    if (!(eng.rec.paid & (1u << t))) { eng.rec.paid |= 1u << t; paid = 1; item_use_hook(capsule[(eng.tier + eng.division) % 3]); }
    if (eng.full && !(eng.rec.paid & (1u << (16 + eng.tier)))) { eng.rec.paid |= 1u << (16 + eng.tier); paid = 1; item_use_hook(capsule[(eng.tier + 1) % 3]); }
    if (paid) rec_save();
    else item_use_hook(ITEM_SENZU);
    if (eng.division == DIV_RUMBLE && !eng.koed) item_use_hook(ITEM_SENZU);
    if (!pick_challengers(t)) { eng.phase = P_EXIT; return; }
    ann_show();
    ann_prep("Wait...  a challenger appears: ", chal_name(0), eng.nchal > 1 ? " and " : "!", eng.nchal > 1 ? chal_name(1) : 0,
             eng.nchal > 1 ? "!" : 0);
    say_go();
    eng.phase = P_CHALLENGE;
}

static void step_challenge(void) {
    int w = winner_player();
    if (needs_reload(w)) { stage_players(w, FIGHT_P_X); restage(P_CHALLENGE); return; }
    ann_prep("", chal_name(0), eng.nchal > 1 ? " and " : " challenges you!  Begin!", eng.nchal > 1 ? chal_name(1) : 0,
             eng.nchal > 1 ? " challenge you!  Begin!" : 0);
    start_fight(K_CHALLENGE, w, eng.chal, eng.nchal);
}

// ---- leaving ---------------------------------------------------------------------------------------------------
static void cleanup(int restore) {
    tny_rumble_mode(0);
    if (restore) { poses_release(); ann_hide(); } else { eng.npose = 0; eng.ann = 0; }
    eng.pres = 0;
    if (restore) shadow_off(); else eng.tint = 0;
    if (restore) {
        clear_opps();
        unstage_players();
    } else {
        eng.backed = 0;
        for (int i = 0; i < MAX_OPP; i++) eng.opp[i] = 0;
    }
    if (coop2.call_state && eng.phase == P_DUEL) coop2.call_state = 0;
    eng.nopp = 0;
    eng.kind = K_MATCH;
}

static void step_exit(void) {
    cleanup(1);
    if (eng.ret_zone && !on_map(eng.ret_zone, eng.ret_area)) {
        warp(eng.ret_zone, eng.ret_area, eng.ret_x, eng.ret_y);
        eng.phase = P_EXITING;
    } else {
        eng.phase = P_IDLE;
    }
}

static int popup_step(void) {
    if (!eng.qn) return 0;
    const char *line = eng.queue[0];
    for (int i = 1; i < eng.qn; i++) eng.queue[i - 1] = eng.queue[i];
    eng.qn--;
    eng.shown++;
    if (popup_is_stub()) say(line, 0, 0, 0, 0);
    else tny_popup_show(line);
    return 1;
}

static void run_step(void) {
    if (eng.npose) poses_release();                  // the poses held for the last line
    switch (eng.phase) {
    case P_IDLE: popup_step(); break;
    case P_DIRECTOR: step_director(); break;
    case P_NEXT: if (!(popup_is_stub() && popup_step())) step_next(); break;
    case P_MATCH_END: step_match_end(); break;
    case P_TROPHY: step_trophy(); break;
    case P_PRIZE: step_prize(); break;
    case P_CHALLENGE: step_challenge(); break;
    case P_INTRO: step_intro(); break;
    case P_EXIT: if (!(popup_is_stub() && popup_step())) step_exit(); break;
    default: break;
    }
}

// ---- the story watch: new tiers, new fighters, the Full bracket (pop-ups in normal play) -------------------------
static void story_watch(void) {
    if (!g_PlayerObject) { if (eng.absent < 0xFFFF) eng.absent++; return; }
    if (eng.absent > 300) eng.baseline = 0;          // the title, a load: the next look is a silent baseline
    eng.absent = 0;
    if ((g_FrameCounter & 15) != 3) return;
    if (eng.force_tiers || eng.force_flags) return;   // tests force the pool: no notices from that
    u16 tiers = 0, story = 0;
    for (int t = 0; t < TIER_COUNT; t++) if (tny_tier_open(t)) tiers |= (u16)(1 << t);
    for (int id = TF_GOHAN; id < TF_COUNT; id++) if (tny_unlocked(id)) story |= (u16)(1 << (id - TF_GOHAN));
    u8 full = 0;
    for (int t = TIER_ADEPT; t < TIER_COUNT; t++) if ((tiers >> t) & 1 && pool_of(t, 0) >= 15) full |= (u8)(1 << t);
    // before the first visit to Papaya Island (the grounds' arrival flag) the tournament means nothing to the
    // players yet: no notices, the baseline only follows the story (our call, 2026-10-25)
    if (eng.baseline && flag(TNY_FLAG_ARRIVED)) {
        for (int t = 0; t < TIER_COUNT; t++) if ((tiers & ~eng.seen_tiers) >> t & 1) tny_notify(tier_notice[t]);
        if (story & ~eng.seen_story) tny_notify(new_fighter);
        if (full & ~eng.full_seen) tny_notify(new_full);
    }
    eng.seen_tiers = tiers;
    eng.seen_story = story;
    eng.full_seen = full;
    eng.baseline = 1;
}

// ---- every frame (input poll) --------------------------------------------------------------------------------------
void tny_engine_frame(void) {
    state_check();
    if (stepping) {                                  // a step's screen is open: its nested input polls stay out
        if ((u32)__builtin_frame_address(0) < eng.step_sp) return;
        stepping = 0;
    }
    story_watch();
    if (eng.say_hold && !scene_running()) eng.say_hold--;
    int safe = g_PlayerObject && p1_controllable() && !scene_running() && !eng.say_hold;
    switch (eng.phase) {
    case P_RESTAGE:
        if (arrived(TNY_RING_ZONE, TNY_RING_AREA)) eng.phase = eng.back;
        else if (eng.timer > 1200) eng.phase = eng.back;
        break;
    case P_TO_RING:
        if (arrived(TNY_RING_ZONE, TNY_RING_AREA)) eng.phase = P_NEXT;
        else if (eng.timer > 1200) { cleanup(0); eng.phase = P_IDLE; }   // the warp never came
        break;
    case P_MATCH:
        if (!in_ring() || scene_running()) break;   // the fight waits for the announcer's line
        fight_frame();
        break;
    case P_MATCH_END:
        if (eng.timer < 90) eng.timer++;
        break;
    case P_DUEL:
        if (eng.ann && coop2.duel_active) ann_hide();   // the announcer leaves when the duel begins
        if (coop2.call_state == 0 && coop2.duel_winner && !coop2.duel_active) {
            duel_clear();
            heal_all();
            int r = eng.br.round, m = eng.br.match;
            int a = side(r, m, 0), b = side(r, m, 1);
            int p1side = (players_in(a) & 1) ? a : b;
            set_winner(r, m, coop2.duel_winner == 1 ? p1side : (p1side == a ? b : a));
            eng.out |= (u8)(coop2.duel_winner == 1 ? 2 : 1);
            exp_trickle(exp_slot(coop2.duel_winner - 1));
            eng.phase = P_NEXT;
        }
        break;
    case P_EXITING:
        if (arrived(eng.ret_zone, eng.ret_area) || eng.timer > 1200) eng.phase = P_IDLE;
        break;
    default:
        break;
    }
    // a tournament phase off the ring: a game over or a reload; restore only into the running game
    if (eng.phase >= P_NEXT && eng.phase <= P_EXIT && !in_ring()) {
        cleanup(g_PlayerObject != 0 && !rv.over);
        eng.phase = P_IDLE;
        eng.pending = 0;
    }
    eng.t_nofight = !((eng.phase == P_MATCH && eng.timer > 20) || (eng.phase == P_DUEL && coop2.duel_active)) || scene_running();
    // pause and saving are blocked in the ring (and player 2's Select switch); between fights (the walk-in, the
    // announcer's lines, the poses, the bracket's turn) nobody moves or attacks either: the tournament runs as
    // scenes from the first match to the trophy (Ryan, 2026-10-25). A stays free to close the lines.
    if (eng.phase >= P_TO_RING && eng.phase < P_EXITING && in_ring() && p1_in_play()) {
        u16 lock = KEY_START | KEY_SELECT;
        if (eng.t_nofight) lock |= KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_B | KEY_L | KEY_R;
        g_InputSystem->pressed &= (u16)~lock;
        g_InputSystem->current &= (u16)~(lock & ~KEY_SELECT);
        coop.input2.pressed &= (u16)~lock;
        coop.input2.current &= (u16)~(lock & ~KEY_SELECT);
    }
    if (eng.npose) poses_apply(!scene_running());   // the end's poses hold while the announcer speaks
    eng.t_busy = eng.phase != P_IDLE || scene_running() || eng.qn || eng.pending || eng.say_hold;
    // a step when the moment is safe: no scene, player 1 free, nothing queued
    if (eng.pending || !safe) return;
    int want = 0;
    switch (eng.phase) {
    case P_IDLE: want = eng.qn != 0; break;
    case P_DIRECTOR: case P_NEXT: case P_TROPHY: case P_PRIZE: case P_CHALLENGE: case P_EXIT: case P_INTRO: want = 1; break;
    case P_MATCH_END: want = eng.timer >= 90; break;
    default: break;
    }
    // the pop-up queue: shown here, in the frame tick, at the safe moment (the ui's tny_popup_show runs its own
    // modal line), between the tournament's steps (idle, or before the next match / the way out)
    if (eng.qn && (eng.phase == P_IDLE || eng.phase == P_NEXT || eng.phase == P_EXIT) && !popup_is_stub()) {
        eng.step_sp = (u32)__builtin_frame_address(0);
        stepping = 1;
        popup_step();
        stepping = 0;
        return;
    }
    // the step runs here, in the frame tick (the ui's screens are made to be opened from it; one opened from an
    // entity-list event lost the next event across its set-aside of the map's entities)
    if (want) { eng.pending = 0; eng.step_sp = (u32)__builtin_frame_address(0); stepping = 1; run_step(); stepping = 0; }
}

// ---- CoopCall 23-27 ------------------------------------------------------------------------------------------------
//   23 Director (op): op 0 = the director's entry: the whole tournament from here (push 1 when the engine took over,
//      0 when it cannot start now); op 1 -> 1 while a tournament runs
int tny_engine_call(int fn, s32 a, s32 b, s32 c, s32 *push) {
    (void)b; (void)c;
    state_check();
    if (fn != 23) return 0;
    if (a == 1) { *push = eng.phase != P_IDLE; return 1; }
    if (eng.phase != P_IDLE) { *push = 1; return 1; }
    if (!g_PlayerObject) { *push = 0; return 1; }
    eng.preset = 0;
    eng.phase = P_DIRECTOR;
    *push = 1;
    return 1;
}

// ---- test commands 21-25 (docs/ids.md) ------------------------------------------------------------------------------
//   21 SEED       arg = the draw's seed for the next tournament; arg2 = 0x7E000000 | fighter: he is the players' first
//                 opponent (a bracket: the seat across from player 1, out of the pool's rules) or the Rumble's headliner
//   22 FORCE      arg 0: every opponent in the fight takes a lethal blow from player 1 (a won fight); 1: the fight is
//                 lost; 2: the running duel ends, arg2 = the winner (1 / 2)
//   23 RECORD     arg 0: wipe the record; 1 / 2: set / clear fighter arg2's unlock bit; 3: reread it into eng.rec
//   24 FORCE_POOL arg = tiers forced open (bits 0-4), arg2 = TFORCE_* (1 ignore story flags, 2 stand-in bodies)
//   25 START      a tournament without the screens: arg = division | full << 4 | tier << 8, arg2 = pick 1 | pick 2 << 8
//                 (0xFF = his own character); from wherever player 1 stands (that is also the way back)
void tny_engine_test(u32 cmd, u32 arg, u32 arg2) {
    state_check();
    switch (cmd) {
    case 21:
        eng.rng = arg;
        eng.seeded = 1;
        eng.force_opp = (arg2 >> 24) == 0x7E && (arg2 & 0xFF) < TF_GOHAN ? (u8)arg2 : NO;   // 0x7E0000NN: fighter NN
        break;
    case 22:
        if (arg == 0 && eng.phase == P_MATCH) {
            for (int i = 0; i < eng.nopp; i++) {
                Entity *e = eng.opp[i];
                if (eng.opp_in[i] == 1 && e && in_list(e) && e->hp > 0) { eng.opp_by[i] = 1; lethal_hit(e); }
            }
            if (eng.nwait) { eng.nwait = 0; for (int i = 0; i < eng.nopp; i++) if (!eng.opp_in[i]) eng.opp_in[i] = 2; }
        } else if (arg == 1 && eng.phase == P_MATCH) {
            int k = eng.fighter == 1 ? 1 : 0;
            CharacterEntry *ce = &g_Party->chars[k && coop.p2 ? coop.p2_slot : g_Party->active_slot];
            ce->hp = 1;
            eng.ko |= (u8)(1 << k);
            eng.result = R_LOST;
        } else if (arg == 2 && coop2.duel_active) {
            coop2.duel_active = 0;
            coop2.duel_winner = (u8)(arg2 == 2 ? 2 : 1);
            heal_all();
        }
        break;
    case 23:
        rec_load();
        if (arg == 0) { memset(&eng.rec, 0, sizeof eng.rec); rec_save(); }
        else if ((arg == 1 || arg == 2) && arg2 < TF_COUNT) {
            if (arg == 1) eng.rec.unlocked[arg2 >> 3] |= (u8)(1 << (arg2 & 7));
            else eng.rec.unlocked[arg2 >> 3] &= (u8)~(1 << (arg2 & 7));
            rec_save();
        }
        break;
    case 24: eng.force_tiers = (u8)arg; eng.force_flags = (u8)arg2; break;
    case 25:
        if (eng.phase != P_IDLE || !g_PlayerObject) break;
        eng.division = (u8)(arg & 3);
        eng.full = (u8)((arg >> 4) & 1);
        eng.tier = (u8)((arg >> 8) & 7);
        if (eng.tier >= TIER_COUNT) eng.tier = 0;
        eng.preset_pick[0] = (u8)arg2;
        eng.preset_pick[1] = (u8)(arg2 >> 8);
        eng.preset = 1;
        eng.phase = P_DIRECTOR;
        break;
    default: break;
    }
}
