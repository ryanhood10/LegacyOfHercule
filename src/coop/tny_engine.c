// The tournament engine (docs/tournament.md, "The engine (built)"): the pool, the brackets and their simulation,
// the match loop in every division, the full heal, the 1 HP rule, rewards, the global record on the save chip,
// unlocks, the hidden challengers and the pop-up queue. The other pieces' screens, kits and maps are called
// through tourney.h; with their weak defaults (tourney_hooks.c) the flow runs headless on automatic choices.
//
// Flow (one tournament): CoopCall 23 at the director arms P_DIRECTOR and pushes 1. Every frame
// (tny_engine_frame, from the input poll) decides what comes next; anything that opens a screen, warps, spawns
// or shows a line runs as a "step" in an entity-list event (the context the game's own level-up and defeat
// dialogues run in), one at a time and only while no scene runs and player 1 is free:
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

// ---- ROM ------------------------------------------------------------------------------------------------
ROM_FN(void, Step_SetActiveCharacter, (s32 *st), 0x0800A785);        // VM step 70: player 1 plays party slot st[top]
ROM_FN(void, Eeprom_Read, (void *dev, void *dst, int off, int len), 0x08021527);    // from the 8 KiB RAM mirror
ROM_FN(void, Eeprom_Write, (void *dev, int off, const void *src, int len), 0x080214C5);   // mirror + chip, 8-byte blocks
ROM_FN(u32, Save_Checksum, (const void *buf, int len), 0x0802153F);
ROM_FN(int, ObjVram_Largest, (void *list), 0x08006D71);              // largest free OBJ VRAM block (bytes)
#define EEPROM_DEV          (*(void *const *)0x081D952C)               // the party module's EEPROM device
#define DEATH_EVENT_VTABLE  0x080238FCu                                // event whose fire runs action->func(action)
#define ENEMY_RECORD_HANDLER 0x0800E77Fu

Entity *enemy_spawn_hook(const EnemyRecord *rec);                     // enemies.c
BossBar *bossbar_create_hook(void *mem, Entity *e, int lo, int hi);   // enemies.c
int item_use_hook(int id);                                            // party.c: PickUpItem (popup, count)
int flag_test_hook(PartyState *p, u32 id);                            // party.c
int p2_bind_slot(int slot);                                           // player.c
extern const char *const ui_strings[];

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
    u32 _p1;
    u32 checksum;          // Save_Checksum of the bytes before it
} TRecord;
_Static_assert(sizeof(TRecord) == 64, "TRecord is 64 bytes (8-byte EEPROM blocks)");

// ---- state (.tny.eng, NOLOAD) --------------------------------------------------------------------------
#define ENG_MAGIC     0x7E6E0002u
#define MAX_OPP       6
#define NO            0xFF
enum { P_IDLE, P_DIRECTOR, P_TO_RING, P_NEXT, P_MATCH, P_MATCH_END, P_DUEL, P_TROPHY, P_PRIZE, P_CHALLENGE,
       P_EXIT, P_EXITING };
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
    // a line in a text box (the announcer, the fallback pop-up): one entry in RAM
    const void *say_seq[3];
    u32 say_action[2];
    u32 step_action[2];
    struct { u8 head[4]; char text[72]; } say_entry;
    TRecord rec;                         // the record as last read or written (tests peek it)
} TnyEng;
_Static_assert(sizeof(TnyEng) <= 0x240, ".tny.eng holds 0x240 bytes (docs/ids.md)");
// offsets the tests peek (docs/testing.md "Tournament engine"): eng is at the start of tourneyram, 0x0203EC00
_Static_assert(__builtin_offsetof(TnyEng, phase) == 4 && __builtin_offsetof(TnyEng, pick) == 12 &&
               __builtin_offsetof(TnyEng, rng) == 100 && __builtin_offsetof(TnyEng, br) == 108 &&
               __builtin_offsetof(TnyEng, qn) == 236 && __builtin_offsetof(TnyEng, rec) == 356, "TnyEng layout (tests)");
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
static int fightable(int sprite) {
    return sprite && has_groups(Character_GetSpriteRecord(sprite), ai_groups(11));
}
static int body_of(int id) {
    if (id < 0 || id >= TF_COUNT) return 0;
    const TFighter *f = &tny_fighters[id];
    if (tny_fighter_ok(id) && fightable(f->sprite)) return f->sprite;
    if (f->play_sprite && Character_GetSpriteRecord(f->play_sprite) && fightable(f->play_sprite)) return f->play_sprite;
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
static char *cat(char *o, const char *s) {
    char *end = eng.say_entry.text + sizeof eng.say_entry.text - 1;
    while (s && *s && o < end) *o++ = *s++;
    *o = 0;
    return o;
}
static void say(const char *a, const char *b, const char *c, const char *d, const char *e) {
    eng.say_entry.head[0] = 2; eng.say_entry.head[1] = 0;   // next: the end
    eng.say_entry.head[2] = 1;                               // text
    eng.say_entry.head[3] = 0;                               // narration
    char *o = cat(eng.say_entry.text, "@^");
    o = cat(o, a); o = cat(o, b); o = cat(o, c); o = cat(o, d); cat(o, e);
    eng.say_seq[0] = say_wait;
    eng.say_seq[1] = &eng.say_entry;
    eng.say_seq[2] = 0;
    coop_run_sequence(eng.say_action, eng.say_seq);
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
typedef struct ActionEvent { u32 *vtable; s32 ticks; s32 x, y; const u32 *action; } ActionEvent;
static void run_step(void);
static void step_fire(u32 *action) { (void)action; eng.pending = 0; run_step(); }
static void queue_step(void) {
    ActionEvent *ev = Arena_Alloc(sizeof *ev);
    if (!ev) return;
    eng.step_action[0] = (u32)step_fire;
    eng.step_action[1] = 0;
    ev->vtable = (u32 *)DEATH_EVENT_VTABLE;
    ev->ticks = 1;
    ev->x = g_PlayerObject ? g_PlayerObject->x >> 8 : 0;
    ev->y = g_PlayerObject ? g_PlayerObject->y >> 8 : 0;
    ev->action = eng.step_action;
    EntityList *l = g_EntityList;
    l->events[l->event_count++] = (u32 *)ev;
    eng.pending = 1;
}

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
    const CharacterEntry *lead = entry_of(eng.own[0] < PARTY_SLOTS ? eng.own[0] : g_Party->active_slot);
    if (id == TF_P_A16) { a16_entry(c, lead); return; }
    CharacterEntry out = *c;
    tny_play_entry(id, &out, lead);
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

static void place(Entity *e, int x, int y, int facing) {
    if (!e) return;
    e->x = x << 8;
    e->y = y << 8;
    e->facing = (u8)facing;
}

// Who is on the ring's floor for this fight: player 1 / player 2 alone (the other sits out: the Cell Games' solo
// modes) or both; the fighters' entries are written and the characters set
static void stage_players(int who) {
    if (eng.players == 2 && who != 2) {
        cg_set_solo(who == 0 ? 1 : 2);            // the other player's entity leaves the floor
        apply_entry(who);
        set_active(eng.slot[who]);                 // solo 2: player 2's pad drives player 1's entity as his pick
    } else {
        if (eng.players == 2) {
            cg_set_solo(0);
            apply_entry(0);
            apply_entry(1);
            bind_p2(eng.slot[1]);                  // player 2 first: player 1's change must not trade with him
            set_active(eng.slot[0]);
            if (coop.p2) coop.last_active = g_Party->active_slot;
        } else {
            apply_entry(0);
            set_active(eng.slot[0]);
        }
    }
    heal_all();
    place(g_PlayerObject, TNY_RING_P1_X, TNY_RING_P1_Y, 3);
    if (who == 2) place(coop.p2, TNY_RING_P2_X, TNY_RING_P2_Y, 3);
}

// Back to the players' own characters between fights (and at the end)
static void unstage_players(void) {
    restore_entries();
    if (eng.players == 2) {
        if (coop_solo()) {
            set_active(eng.own[0]);
            coop.p2_slot = eng.own[1] < PARTY_SLOTS ? eng.own[1] : coop.p2_slot;
            cg_set_solo(0);
        } else {
            if (eng.own[1] < PARTY_SLOTS) bind_p2(eng.own[1]);
            set_active(eng.own[0]);
            if (coop.p2) coop.last_active = g_Party->active_slot;
        }
    } else {
        set_active(eng.own[0]);
    }
    heal_all();
}

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
    EntityList_Add(g_SceneTable[1], e);
    e->facing = 2;
    tny_moves_attach(e, id);
    return e;
}

static const s16 opp_spot[MAX_OPP][2] = {
    { 0, 0 }, { 24, 48 }, { 24, -48 }, { 72, 24 }, { 72, -24 }, { -120, 56 },
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
        int x = TNY_RING_OPP_X + opp_spot[i][0], y = TNY_RING_OPP_Y + opp_spot[i][1];
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
        eng.opp[i] = 0;
        eng.nopp++;
    }
    eng.nwait = eng.nopp;
    eng.lone = kind == K_CHALLENGE && n == 1 && who == 2;
    stage_players(who);
    spawn_waiting(opp_budget(), 1);
    eng.lone = 0;
    bars();
    if (kind == K_RUMBLE) tny_rumble_mode(1);
    eng.timer = 0;
    eng.phase = P_MATCH;
}

static int opp_budget(void) { return eng.kind == K_RUMBLE ? 5 : MAX_OPP * 2; }

static void fight_frame(void) {
    if (eng.timer < 0xFFFF) eng.timer++;
    int alive = 0;
    for (int i = 0; i < eng.nopp; i++) {
        if (eng.opp_in[i] != 1) continue;
        Entity *e = eng.opp[i];
        if (!e || !in_list(e) || e->hp <= 0) { eng.opp_in[i] = 2; continue; }
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
    if (eng.magic != ENG_MAGIC || eng.phase != P_MATCH || coop2.duel_active) return 0;
    const CharacterEntry *c = &g_Party->chars[k ? coop.p2_slot : g_Party->active_slot];
    if (dmg < c->hp) return 0;
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
    // fill unit by unit so a bye is a whole unit at the end of the draw (a 2 v 2 team is never half empty)
    for (int u = 0; u < nu; u++)
        for (int i = 0; i < w; i++) {
            int s = u * w + i;
            if (s == b->p1_seat || s == b->p2_seat) continue;
            b->seat[s] = k < n ? pool[k++] : NO;
        }
    eng.results = 0;
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
            if (used(id) || is_headliner(id) || weight(id) != 2) continue;
            out[k++] = (u8)id; use(id); slots -= 2;
            break;
        }
    }
    for (int i = 0; i < n && slots > 0 && k < MAX_OPP; i++) {
        int id = pool[i];
        if (used(id) || is_headliner(id)) continue;
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
        for (int i = 0; i < 2; i++) {
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
    start_fight(K_RUMBLE, eng.players == 2 ? 2 : 0, ids, n);
    char num[2] = { (char)('1' + eng.rb_round), 0 };
    if (eng.rb_round < regular) say("Royal Rumble, round ", num, "!  Last ones standing win!", 0, 0);
    else say("The headliner: ", fname(ids[n - 1]), "!", 0, 0);
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
static int arrived(int zone, int area) {
    if (eng.timer < 0xFFFF) eng.timer++;
    return eng.timer > 20 && on_map(zone, area) && g_PlayerObject && g_PlayerObject != eng.from && !coop2.cmd
           && p1_in_play() && !scene_running();
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
    begin_tournament();
    // where the director stands (the way back) and off to the ring
    const u8 *m = map_state();
    Entity *p1 = g_PlayerObject;
    eng.ret_zone = m ? m[0] : 0;
    eng.ret_area = m ? m[1] : 0;
    eng.ret_x = p1 ? (s16)(p1->x >> 8) : 0;
    eng.ret_y = p1 ? (s16)(p1->y >> 8) : 0;
    warp(TNY_RING_ZONE, TNY_RING_AREA, TNY_RING_P1_X, TNY_RING_P1_Y);
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
        tny_ui_bracket(1);
        eng.results = 0;
        eng.phase = (players_in(champion_seat()) & ~eng.out) ? P_TROPHY : P_EXIT;
        return;
    }
    tny_ui_bracket(eng.results ? 1 : 0);
    eng.results = 0;
    int r = eng.br.round, m = eng.br.match;
    int a = side(r, m, 0), b = side(r, m, 1);
    int pa = players_in(a) & ~eng.out, pb = players_in(b) & ~eng.out;
    static const char *const round_name[4] = { "Round 1: ", "Round 2: ", "Round 3: ", "Round 4: " };
    const char *round = r == eng.br.rounds - 1 ? "The final: " : round_name[r & 3];
    if (eng.division == DIV_1V1 && pa && pb) {       // the co-op final: player 1 against player 2, a duel
        restore_entries();
        if (coop_solo()) coop.p2_slot = eng.own[1] < PARTY_SLOTS ? eng.own[1] : coop.p2_slot;
        cg_set_solo(0);                              // player 2's entity comes back when player 1 stands
        apply_entry(0);
        apply_entry(1);
        set_active(eng.slot[0]);
        heal_all();
        coop2.call_end_seq = 0;
        coop2.call_slot = (s8)eng.slot[1];            // duel_start binds player 2 there
        coop2.call_flags = (u8)(eng.pick[0] == eng.pick[1] ? DUEL_SHADOW : 0);   // the same fighter twice: the shadow
        coop2.duel_winner = 0;
        coop2.call_state = 1;                        // vmcall.c starts it once player 2 stands (the CoopCall 1 path)
        eng.phase = P_DUEL;
        eng.timer = 0;
        say(round, pname(0), " vs. ", pname(1), "!");
        return;
    }
    int lead = pa ? b : a;                           // the opponents' unit
    int who = (pa | pb) == 3 ? 2 : (pa | pb) == 2 ? 1 : 0;
    u8 ids[2];
    int n = 0;
    for (int i = 0; i < unit_w(); i++) if (eng.br.seat[lead + i] != NO) ids[n++] = eng.br.seat[lead + i];
    start_fight(K_MATCH, who, ids, n);
    if (eng.division == DIV_2V2)
        say(round, "your team vs. ", fname(ids[0]), n > 1 ? " & " : "!", n > 1 ? fname(ids[1]) : 0);
    else
        say(round, pname(who), " vs. ", fname(ids[0]), "!");
}

// ---- the end of a fight ---------------------------------------------------------------------------------------
static void step_match_end(void) {
    int won = eng.result == R_WON;
    if (eng.kind == K_RUMBLE) tny_rumble_mode(0);
    if (won) for (int i = 0; i < eng.nopp; i++)
        if (eng.opp_by[i] && (tny_fighters[eng.opp_id[i]].role & TR_BEAT)) unlock(eng.opp_id[i]);
    clear_opps();
    unstage_players();
    if (won) {
        if (eng.fighter == 2 || eng.players == 1) { exp_trickle(exp_slot(0)); if (eng.players == 2) exp_trickle(exp_slot(1)); }
        else exp_trickle(exp_slot(eng.fighter));
    }
    int k = eng.fighter == 2 ? 0 : eng.fighter;
    if (eng.kind == K_CHALLENGE) {
        rec_load();
        if (won) for (int i = 0; i < eng.nchal; i++) unlock(eng.chal[i]);
        else for (int i = 0; i < eng.nchal; i++) {
            u8 *a = &eng.rec.attempts[eng.chal[i] - TF_PUIPUI];
            if (*a < 255) (*a)++;
            rec_save();
        }
        if (won) say(fname(eng.chal[0]), " joins the tournament fighters!", 0, 0, 0);
        else say("Come back when you've won again!", 0, 0, 0, 0);
        eng.phase = P_EXIT;
        return;
    }
    if (eng.kind == K_RUMBLE) {
        if (won) { eng.rb_round++; say("Round cleared!", 0, 0, 0, 0); eng.phase = P_NEXT; }
        else { say(pname(0), eng.players == 2 ? " and partner are out of the Royal Rumble!" : " is out of the Royal Rumble!", 0, 0, 0); eng.phase = P_EXIT; }
        return;
    }
    // a bracket match
    int r = eng.br.round, m = eng.br.match;
    int a = side(r, m, 0), b = side(r, m, 1);
    int pa = players_in(a) & ~eng.out;
    int mine = pa ? a : b, theirs = pa ? b : a;
    set_winner(r, m, won ? mine : theirs);
    if (won) say(eng.division == DIV_2V2 ? "Your team" : pname(k), " wins!", 0, 0, 0);
    else {
        if (eng.division == DIV_2V2) eng.out = 3;
        else eng.out |= (u8)(1 << k);
        say(eng.division == DIV_2V2 ? "Your team" : pname(k), eng.division == DIV_2V2 ? " is out of the tournament!" : " is out of the tournament!", 0, 0, 0);
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
    const char *who = w == 2 ? (eng.players == 2 ? "Your team" : pname(0)) : pname(w);
    say(who, " wins the ", tier_name[eng.tier], eng.division == DIV_RUMBLE ? " Royal Rumble!" : " tournament!", 0);
    eng.phase = P_PRIZE;
}

static int won_as(int id) {
    int w = winner_player();
    for (int k = 0; k < eng.players; k++) if ((w == 2 || w == k) && eng.pick[k] == id) return 1;
    return 0;
}

// The prizes (Ryan, 2026-10-25): the first win of each tournament a +3 capsule (the stat turns with the tier and
// the division: five of each across the fifteen), Full a second one, every later win a Senzu bean (the Rumble a
// second one with no player KO); then the hidden challenger, if he is still locked
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
    // the hidden challenger(s) of this tournament
    eng.nchal = 0;
    for (int id = TF_PUIPUI; id <= TF_VEGITO && eng.nchal < 2; id++) {
        const TFighter *f = &tny_fighters[id];
        if (f->challenger_of != t || rec_bit(id) || !body_of(id)) continue;
        if (id == TF_UUB && !won_as(TF_P_HERCULE)) continue;                       // "won as Hercule"
        if (id == TF_VEGITO && !won_as(TF_GOKU) && !won_as(TF_VEGETA)) continue;   // "won as Goku or Vegeta"
        eng.chal[eng.nchal++] = (u8)id;
    }
    if (!eng.nchal) { eng.phase = P_EXIT; return; }
    say("A challenger appears: ", fname(eng.chal[0]), eng.nchal > 1 ? " and " : "!", eng.nchal > 1 ? fname(eng.chal[1]) : 0, 0);
    eng.phase = P_CHALLENGE;
}

static void step_challenge(void) {
    int w = winner_player();
    start_fight(K_CHALLENGE, w, eng.chal, eng.nchal);
    say(fname(eng.chal[0]), eng.nchal > 1 ? " and partner challenge you!" : " challenges you!", 0, 0, 0);
}

// ---- leaving ---------------------------------------------------------------------------------------------------
static void cleanup(int restore) {
    tny_rumble_mode(0);
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
    switch (eng.phase) {
    case P_IDLE: popup_step(); break;
    case P_DIRECTOR: step_director(); break;
    case P_NEXT: if (!popup_step()) step_next(); break;
    case P_MATCH_END: step_match_end(); break;
    case P_TROPHY: step_trophy(); break;
    case P_PRIZE: step_prize(); break;
    case P_CHALLENGE: step_challenge(); break;
    case P_EXIT: if (!popup_step()) step_exit(); break;
    default: break;
    }
}

// ---- the story watch: new tiers, new fighters, the Full bracket (pop-ups in normal play) -------------------------
static void story_watch(void) {
    if (!g_PlayerObject) { if (eng.absent < 0xFFFF) eng.absent++; return; }
    if (eng.absent > 300) eng.baseline = 0;          // the title, a load: the next look is a silent baseline
    eng.absent = 0;
    if ((g_FrameCounter & 15) != 3) return;
    u16 tiers = 0, story = 0;
    for (int t = 0; t < TIER_COUNT; t++) if (tny_tier_open(t)) tiers |= (u16)(1 << t);
    for (int id = TF_GOHAN; id < TF_COUNT; id++) if (tny_unlocked(id)) story |= (u16)(1 << (id - TF_GOHAN));
    u8 full = 0;
    for (int t = TIER_ADEPT; t < TIER_COUNT; t++) if ((tiers >> t) & 1 && pool_of(t, 0) >= 15) full |= (u8)(1 << t);
    if (eng.baseline) {
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
    story_watch();
    int safe = g_PlayerObject && p1_controllable() && !scene_running();
    switch (eng.phase) {
    case P_TO_RING:
        if (arrived(TNY_RING_ZONE, TNY_RING_AREA)) eng.phase = P_NEXT;
        else if (eng.timer > 1200) { cleanup(0); eng.phase = P_IDLE; }   // the warp never came
        break;
    case P_MATCH:
        if (!in_ring()) break;
        fight_frame();
        break;
    case P_MATCH_END:
        if (eng.timer < 90) eng.timer++;
        break;
    case P_DUEL:
        if (coop2.call_state == 0 && coop2.duel_winner && !coop2.duel_active) {
            duel_clear();
            unstage_players();
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
    // pause and saving are blocked in the ring (and player 2's Select switch)
    if (eng.phase >= P_TO_RING && eng.phase < P_EXITING && in_ring() && p1_in_play()) {
        g_InputSystem->pressed &= (u16)~(KEY_START | KEY_SELECT);
        g_InputSystem->current &= (u16)~KEY_START;
        coop.input2.pressed &= (u16)~(KEY_START | KEY_SELECT);
    }
    // a step when the moment is safe: no scene, player 1 free, nothing queued
    if (eng.pending || !safe) return;
    int want = 0;
    switch (eng.phase) {
    case P_IDLE: want = eng.qn != 0; break;
    case P_DIRECTOR: case P_NEXT: case P_TROPHY: case P_PRIZE: case P_CHALLENGE: case P_EXIT: want = 1; break;
    case P_MATCH_END: want = eng.timer >= 90; break;
    default: break;
    }
    if (want) queue_step();
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
//   21 SEED       arg = the draw's seed for the next tournament
//   22 FORCE      arg 0: every opponent in the fight takes a lethal blow from player 1 (a won fight); 1: the fight is
//                 lost; 2: the running duel ends, arg2 = the winner (1 / 2)
//   23 RECORD     arg 0: wipe the record; 1 / 2: set / clear fighter arg2's unlock bit; 3: reread it into eng.rec
//   24 FORCE_POOL arg = tiers forced open (bits 0-4), arg2 = TFORCE_* (1 ignore story flags, 2 stand-in bodies)
//   25 START      a tournament without the screens: arg = division | full << 4 | tier << 8, arg2 = pick 1 | pick 2 << 8
//                 (0xFF = his own character); from wherever player 1 stands (that is also the way back)
void tny_engine_test(u32 cmd, u32 arg, u32 arg2) {
    state_check();
    switch (cmd) {
    case 21: eng.rng = arg; eng.seeded = 1; break;
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
