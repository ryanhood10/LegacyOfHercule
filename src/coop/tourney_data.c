// The tournament roster (docs/tournament.md section 4, tourney.h): one row per fighter. Owned by the engine
// session after the team starts; other pieces read it. Portrait ids are the dialogue portraits where the game
// has one (art sessions fill 0s); pool flags are the story flags named in docs/tournament.md section 3 (the
// engine confirms each against the chapter scripts).
#include "tourney.h"

#define NO 0xFF

// The tiers open on these story flags (Rookie from the start).
const u16 tny_tier_flag[TIER_COUNT] = {
    0,      // Rookie: from the first flight
    86,     // Adept: the Android 19 + 13 fight won (zone 8's landing opens; engine to confirm)
    136,    // Z-Fighter: Gingertown done (engine to confirm)
    191,    // Champion: the ten-day wait (the Cell Games landing opens)
    382,    // Legend: Super 13 beaten
};

#define OPP     TR_OPPONENT
#define BEAT    (TR_OPPONENT | TR_BEAT)
#define CHAL    TR_CHALLENGER
#define STORY   TR_STORY
#define T(tier, div) ((tier) * DIV_COUNT + (div))

const TFighter tny_fighters[TF_COUNT] = {
    // name, sprite, play_sprite, portrait, tier, tab, role, pool_flag, story_slot, challenger_of, unlock text
    [TF_YAMCHA]        = { "Yamcha",          156, TS_P_YAMCHA,   114, TIER_ROOKIE,   TAB_TOURNAMENT, BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_TIEN]          = { "Tien",            121, TS_P_TIEN,      95, TIER_ROOKIE,   TAB_TOURNAMENT, BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_FRIEZA]        = { "Frieza",           48, TS_P_FRIEZA,    49, TIER_ROOKIE,   TAB_VILLAINS,   BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_MECHA_FRIEZA]  = { "Mecha Frieza",     49, TS_P_MECHA,     49, TIER_ROOKIE,   TAB_VILLAINS,   BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_KING_COLD]     = { "King Cold",   TS_KING_COLD, TS_KING_COLD, 70, TIER_ROOKIE, TAB_VILLAINS,  BEAT | TR_GIANT | TR_HEADLINER, 0, NO, NO, "Beat this fighter in a match" },
    [TF_HERCULE]       = { "Hercule",          72, 0,              67, TIER_ROOKIE,   TAB_TOURNAMENT, OPP,  0,   NO, NO, 0 },
    [TF_ROSHI]         = { "Master Roshi", TS_ROSHI,  TS_ROSHI,    80, TIER_ROOKIE,   TAB_TOURNAMENT, BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_TAO]           = { "Mercenary Tao",    50, TS_P_TAO,       53, TIER_ROOKIE,   TAB_TOURNAMENT, BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_SPOPOVICH]     = { "Spopovich",  TS_SPOPOVICH, TS_SPOPOVICH, 142, TIER_ROOKIE,  TAB_TOURNAMENT, BEAT | TR_GIANT, 0, NO, NO, "Beat this fighter in a match" },
    [TF_YAMU]          = { "Yamu",       TS_YAMU,     TS_YAMU,      143, TIER_ROOKIE,   TAB_TOURNAMENT, BEAT | TR_GIANT, 0, NO, NO, "Beat this fighter in a match" },
    [TF_CHICHI]        = { "Chi-Chi",    TS_CHICHI,   TS_CHICHI,   43, TIER_ROOKIE,   TAB_TOURNAMENT, BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_VIDEL]         = { "Videl",      TS_VIDEL,    TS_VIDEL,   127, TIER_ROOKIE,   TAB_TOURNAMENT, BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_CHIAOTZU]      = { "Chiaotzu",   TS_CHIAOTZU, TS_CHIAOTZU, 42, TIER_ROOKIE,   TAB_TOURNAMENT, BEAT, 0,   NO, NO, "Beat this fighter in a match" },

    [TF_A19]           = { "Android 19",       12, 0,              30, TIER_ADEPT,    TAB_VILLAINS,   OPP,  0,   NO, NO, 0 },
    [TF_GERO]          = { "Dr. Gero",         14, 0,              31, TIER_ADEPT,    TAB_VILLAINS,   OPP,  0,   NO, NO, 0 },
    [TF_A13]           = { "Android 13",      165, TS_P_A13,        0, TIER_ADEPT,    TAB_VILLAINS,   BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_A17]           = { "Android 17",        9, TS_P_A17,       28, TIER_ADEPT,    TAB_VILLAINS,   BEAT | TR_HEADLINER, 127, NO, NO, "Beat this fighter in a match" },
    [TF_A18]           = { "Android 18",       11, TS_P_A18,       29, TIER_ADEPT,    TAB_VILLAINS,   BEAT | TR_HEADLINER, 127, NO, NO, "Beat this fighter in a match" },

    [TF_IMPERFECT_CELL]= { "Imperfect Cell",   28, 0,              38, TIER_ZFIGHTER, TAB_VILLAINS,   OPP,  0,   NO, NO, 0 },
    [TF_A16]           = { "Android 16",      160, 0,              26, TIER_ZFIGHTER, TAB_VILLAINS,   OPP | TR_HEADLINER, 146, NO, NO, 0 },

    [TF_SEMI_CELL]     = { "Semi-Perfect Cell",29, 0,              39, TIER_CHAMPION, TAB_VILLAINS,   OPP,  0,   NO, NO, 0 },
    [TF_PERFECT_CELL]  = { "Perfect Cell",     30, TS_P_PCELL,     40, TIER_CHAMPION, TAB_VILLAINS,   BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_CELL_JR]       = { "Cell Jr.",         31, 0,               0, TIER_CHAMPION, TAB_VILLAINS,   OPP,  0,   NO, NO, 0 },
    [TF_COOLER]        = { "Cooler",           34, TS_P_COOLER,    44, TIER_CHAMPION, TAB_VILLAINS,   BEAT | TR_GIANT, 0, NO, NO, "Beat this fighter in a match" },
    [TF_NAMEKIAN]      = { "Namekian Warrior",166, 0,               0, TIER_CHAMPION, TAB_VILLAINS,   OPP,  0,   NO, NO, 0 },
    [TF_BROLY]         = { "Broly",           159, 159,             141, TIER_CHAMPION, TAB_VILLAINS,   BEAT | TR_GIANT | TR_HEADLINER, 402, NO, NO, "Beat this fighter in a match" },
    [TF_PARAGUS]       = { "Paragus",         170, 0,             120, TIER_CHAMPION, TAB_VILLAINS,   OPP,  402, NO, NO, 0 },

    [TF_SUPER_PERFECT_CELL] = { "Super Perfect Cell", 30, 0,       40, TIER_LEGEND,   TAB_VILLAINS,   OPP,  0,   NO, NO, 0 },
    [TF_SUPER13]       = { "Super Android 13",180, 180,           122, TIER_LEGEND,   TAB_VILLAINS,   BEAT | TR_GIANT, 0, NO, NO, "Beat this fighter in a match" },
    [TF_BGOKU]         = { "Goku (Buu Saga)", TS_BGOKU, TS_BGOKU,   162, TIER_LEGEND,   TAB_BUU,        BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_MAJIN_VEGETA]  = { "Majin Vegeta", TS_MAJIN_VEGETA, TS_MAJIN_VEGETA, 165, TIER_LEGEND, TAB_BUU, BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_MYSTIC_GOHAN]  = { "Mystic Gohan", TS_MYSTIC_GOHAN, TS_MYSTIC_GOHAN, 166, TIER_LEGEND, TAB_BUU, BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_SAIYAMAN]      = { "Great Saiyaman", TS_SAIYAMAN, TS_SAIYAMAN, 167, TIER_LEGEND,  TAB_BUU,        BEAT, 0,   NO, NO, "Beat this fighter in a match" },
    [TF_BHERCULE]      = { "Hercule (Buu Saga)", 158, 0,            126, TIER_LEGEND,  TAB_BUU,        OPP,  0,   NO, NO, 0 },
    [TF_BPICCOLO]      = { "Piccolo (Buu Saga)", TS_BPICCOLO, 0,   168, TIER_LEGEND,   TAB_BUU,        OPP,  0,   NO, NO, 0 },
    [TF_SUPER_BUU]     = { "Super Buu",  TS_SUPER_BUU, TS_SUPER_BUU, 151, TIER_LEGEND,  TAB_BUU,        BEAT | TR_GIANT | TR_HEADLINER, 0, NO, NO, "Beat this fighter in a match" },

    [TF_PUIPUI]        = { "Pui Pui",    TS_PUIPUI,   TS_PUIPUI,    144, TIER_ROOKIE,   TAB_BUU,        CHAL, 0,   NO, T(TIER_ROOKIE, DIV_1V1),      "Win the Rookie tournament (1 v 1)" },
    [TF_GOTEN]         = { "Goten",      TS_GOTEN,    TS_GOTEN,     155, TIER_ROOKIE,   TAB_BUU,        CHAL, 0,   NO, T(TIER_ROOKIE, DIV_2V2),      "Win the Rookie tournament (2 v 2)" },
    [TF_KID_TRUNKS]    = { "Kid Trunks", TS_KID_TRUNKS, TS_KID_TRUNKS, 156, TIER_ROOKIE, TAB_BUU,        CHAL, 0,   NO, T(TIER_ROOKIE, DIV_2V2),      "Win the Rookie tournament (2 v 2)" },
    [TF_YAKON]         = { "Yakon",      TS_YAKON,    TS_YAKON,     145, TIER_ROOKIE,   TAB_BUU,        CHAL | TR_GIANT, 0, NO, T(TIER_ROOKIE, DIV_RUMBLE), "Win the Rookie Royal Rumble" },
    [TF_PIKKON]        = { "Pikkon",     TS_PIKKON,   TS_PIKKON,    147, TIER_ADEPT,    TAB_BUU,        CHAL, 0,   NO, T(TIER_ADEPT, DIV_1V1),       "Win the Adept tournament (1 v 1)" },
    [TF_DABURA]        = { "Dabura",     TS_DABURA,   TS_DABURA,    148, TIER_ADEPT,    TAB_BUU,        CHAL | TR_GIANT, 0, NO, T(TIER_ADEPT, DIV_2V2), "Win the Adept tournament (2 v 2)" },
    [TF_EVIL_BUU]      = { "Evil Buu",   TS_EVIL_BUU, TS_EVIL_BUU,  0, TIER_ADEPT,    TAB_BUU,        CHAL | TR_GIANT, 0, NO, T(TIER_ADEPT, DIV_RUMBLE),    "Win the Adept Royal Rumble" },
    [TF_KIBITO]        = { "Kibito",     TS_KIBITO,   TS_KIBITO,    146, TIER_ZFIGHTER, TAB_BUU,        CHAL, 0, NO, T(TIER_ZFIGHTER, DIV_1V1), "Win the Z-Fighter tournament (1 v 1)" },
    [TF_FAT_BUU]       = { "Fat Buu",    TS_FAT_BUU,  TS_FAT_BUU,   149, TIER_ZFIGHTER, TAB_BUU,        CHAL | TR_GIANT, 0, NO, T(TIER_ZFIGHTER, DIV_2V2), "Win the Z-Fighter tournament (2 v 2)" },
    [TF_JANEMBA]       = { "Super Janemba", TS_JANEMBA, TS_JANEMBA, 154, TIER_ZFIGHTER, TAB_BUU,        CHAL | TR_GIANT, 0, NO, T(TIER_ZFIGHTER, DIV_RUMBLE), "Win the Z-Fighter Royal Rumble" },
    [TF_KID_BUU]       = { "Kid Buu",    TS_KID_BUU,  TS_KID_BUU,   150, TIER_CHAMPION, TAB_BUU,        CHAL, 0,   NO, T(TIER_CHAMPION, DIV_1V1),    "Win the Champion tournament (1 v 1)" },
    [TF_GOTENKS]       = { "Gotenks",    TS_GOTENKS,  TS_GOTENKS,   157, TIER_CHAMPION, TAB_BUU,        CHAL, 0,   NO, T(TIER_CHAMPION, DIV_2V2),    "Win the Champion tournament (2 v 2)" },
    [TF_UUB]           = { "Uub",        TS_UUB,      TS_UUB,       161, TIER_CHAMPION, TAB_BUU,        CHAL, 0,   NO, T(TIER_CHAMPION, DIV_RUMBLE), "Win the Champion Royal Rumble as Hercule" },
    [TF_VEGITO]        = { "Vegito",     TS_VEGITO,   TS_VEGITO,    160, TIER_LEGEND,   TAB_BUU,        CHAL, 0,   NO, T(TIER_LEGEND, DIV_1V1),      "Win the Legend tournament (1 v 1) as Goku or Vegeta" },

    [TF_GOHAN]         = { "Gohan",            57, 0,              60, TIER_ROOKIE,   TAB_ZFIGHTERS,  STORY, 0,  0, NO, 0 },
    [TF_KRILLIN]       = { "Krillin",         161, 0,              73, TIER_ROOKIE,   TAB_ZFIGHTERS,  STORY, 0,  6, NO, 0 },
    [TF_PICCOLO]       = { "Piccolo",         103, 0,              90, TIER_ROOKIE,   TAB_ZFIGHTERS,  STORY, 0,  1, NO, 0 },
    [TF_VEGETA]        = { "Vegeta",          137, 0,             106, TIER_ROOKIE,   TAB_ZFIGHTERS,  STORY, 0,  2, NO, 0 },
    [TF_TRUNKS]        = { "Trunks",          135, 0,             103, TIER_ROOKIE,   TAB_ZFIGHTERS,  STORY, 0,  3, NO, 0 },
    [TF_GOKU]          = { "Goku",             60, 0,              63, TIER_ROOKIE,   TAB_ZFIGHTERS,  STORY, 0,  4, NO, 0 },
    [TF_P_HERCULE]     = { "Hercule",          72, 0,              67, TIER_ROOKIE,   TAB_ZFIGHTERS,  STORY, 0,  5, NO, 0 },
    [TF_P_A16]         = { "Android 16",      175, 0,              26, TIER_ROOKIE,   TAB_ZFIGHTERS,  STORY, 421, 7, NO, 0 },
    [TF_SSJ2_GOHAN]    = { "SSJ2 Gohan",       58, 0,              61, TIER_ROOKIE,   TAB_ZFIGHTERS,  STORY, 0,  8, NO, 0 },
};

extern const unsigned sprite_index_count;   // build/gen/sprites_data.c (tools/spriteclone.py)

// The sprite index has no bounds check: an id past this build's table (art not merged, BUU=0) reads garbage.
int tny_fighter_ok(int id) {
    if (id < 0 || id >= TF_COUNT) return 0;
    const TFighter *f = &tny_fighters[id];
    return f->sprite >= 7 && (unsigned)f->sprite < sprite_index_count && Character_GetSpriteRecord(f->sprite) != 0;
}
