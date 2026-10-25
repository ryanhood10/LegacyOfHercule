// The tournament's playables (docs/tournament.md 4c, 9a "playables"): every TR_BEAT / TR_CHALLENGER fighter of
// the roster (tourney_data.c) plays on a display row of his own (29-79, tools/tnyplayables.py: the rows and the
// generated fighter -> row map tny_play_rows) with a kit of four abilities from the ability table and numbers
// derived from the match's lead at his level (the Android 16 recipe, a16player.c a16_join).
//
//   tny_play_row(id)               his display row, 0 for a fighter who is not a tournament playable (the story's
//                                  playables play from their own party slot)
//   tny_play_entry(id, out, lead)  the CharacterEntry the engine writes into the borrowed party slot for a match:
//                                  his row, the lead's EXP (so the lead's level), HP / EP / STR / POW / END as the
//                                  fighter's percentages of the lead's, full HP and EP, in party + available + all
//                                  four moves learned (0x7B), his four abilities, ability index 0
//
// Kits: only abilities whose animations play groups the body has (docs/tournament.md "The playables"): the beam
// abilities play the beam groups 18 / 19 (the Kamehameha 3 only on Yamcha, Perfect Cell and Cooler, whose records
// have a group 18; Special Beam Cannon 9, Spirit Bomb 10 and Sword Blast 13 on nobody) and Energy Punch 7 crashes
// on Hercule's layout. Ability ids: 0 Big Bang, 1 Burning Attack, 2 Ki Blast, 3 Kamehameha, 4 Masenko,
// 8 Scatter Shot, 14-17 Hercule's (Rolling Hercule Punch, Dynamite Kick, Megaton Punch, Taunt), 19 Afterimage
// Rush, 20 Shockwave Punch, 21 Destructo Disc, 23 Super Saiyan 2 Burst.
#include "tourney.h"

extern const u8 tny_play_rows[TF_COUNT];       // build/gen/tny_play_sprites_data.c (tools/tnyplayables.py)

typedef struct TKit {
    u8 ab[4];                    // ability_list
    u8 hp, ep, str, pow, end;    // percent of the lead's numbers
} TKit;

// Big Bang, Burning Attack, Ki Blast, Kamehameha (beam bodies only), Masenko, Scatter Shot; Hercule's punch, kick, Megaton Punch, Taunt;
// Afterimage Rush, Shockwave Punch, Destructo Disc, Super Saiyan 2 Burst
enum { BB = 0, BURN = 1, KI = 2, KAME = 3, MAS = 4, SCAT = 8, HPUNCH = 14, HKICK = 15, MEGA = 16, TAUNT = 17,
       RUSH = 19, SHOCK = 20, DISC = 21, BURST = 23 };

// Numbers: the tournament regulars about the lead's own (100), villains and the Buu era stronger, the champions'
// circle (Chi-Chi, Chiaotzu, Roshi) weaker; giants trade speed (the body) for HP. Tuned in play.
static const TKit kits[TF_COUNT] = {
    //                    abilities                      hp   ep  str  pow  end
    [TF_YAMCHA]       = { { RUSH, KAME, BB, KI },       100, 100, 100,  95, 100 },   // Wolf Fang Fist, Kamehameha, Spirit Ball
    [TF_TIEN]         = { { BURN, SHOCK, BB, KI },      105, 100, 100, 105, 105 },   // Tri-Beam, Solar Flare stand-in
    [TF_FRIEZA]       = { { KI, BB, SCAT, RUSH },       120, 120, 115, 125, 115 },   // Death Beam, Death Ball
    [TF_MECHA_FRIEZA] = { { KI, RUSH, BB, SHOCK },      125, 110, 120, 115, 125 },   // Death Beam, the rush
    [TF_KING_COLD]    = { { MEGA, KI, BB, SHOCK },      140, 100, 125, 105, 125 },   // the giant's blows, Death Beam
    [TF_ROSHI]        = { { MAS, RUSH, SHOCK, KI },      85, 110,  85,  95,  85 },   // his Kamehameha wave, Drunken Fist, Thunder Shock
    [TF_TAO]          = { { KI, RUSH, SHOCK, BB },      100, 100, 105, 100, 100 },   // Dodon Ray, Crane rush
    [TF_SPOPOVICH]    = { { HPUNCH, MEGA, SHOCK, TAUNT }, 130, 80, 115,  80, 130 },  // a brawler
    [TF_YAMU]         = { { RUSH, HPUNCH, KI, SHOCK },  100,  90, 105,  90, 100 },
    [TF_CHICHI]       = { { HPUNCH, HKICK, RUSH, TAUNT }, 80, 70,  90,  70,  80 },   // no ki: the frying pan, the combo
    [TF_VIDEL]        = { { RUSH, HKICK, HPUNCH, KI },   85,  80,  95,  80,  85 },   // Hawk Arrow, the combo
    [TF_CHIAOTZU]     = { { KI, SHOCK, MAS, BB },        75, 110,  70, 105,  75 },   // Dodon Ray, psychic hold stand-in
    [TF_A13]          = { { BB, RUSH, KI, SHOCK },      125, 115, 120, 115, 125 },   // S.S. Deadly Bomber
    [TF_A17]          = { { BB, SHOCK, RUSH, KI },      120, 120, 120, 120, 115 },   // Power Blitz, Energy Barrier
    [TF_A18]          = { { BB, KI, SCAT, RUSH },       115, 120, 115, 120, 110 },   // Power Blitz, Finger Beam, Infinity Bullet
    [TF_PERFECT_CELL] = { { KAME, SHOCK, BB, KI },      145, 130, 140, 140, 140 },   // Kamehameha, Solar Flare stand-in
    [TF_COOLER]       = { { KAME, BB, SCAT, KI },       140, 125, 135, 135, 135 },   // Death Flash (the beam), Supernova
    [TF_BROLY]        = { { BB, SCAT, BURST, RUSH },    160, 120, 150, 135, 150 },   // Eraser Cannon, Blaster Shell, Gigantic Meteor
    [TF_SUPER13]      = { { BURST, BB, SCAT, KI },      155, 120, 145, 140, 145 },   // the nova, the bomber, the volley
    [TF_BGOKU]        = { { BURN, RUSH, BURST, KI },    140, 140, 140, 145, 135 },   // Kamehameha stand-in, Dragon Fist
    [TF_MAJIN_VEGETA] = { { BB, BURN, BURST, KI },      140, 135, 145, 140, 135 },   // Big Bang, Final Flash, Final Explosion
    [TF_MYSTIC_GOHAN] = { { MAS, BURN, RUSH, KI },      140, 140, 145, 145, 135 },
    [TF_SAIYAMAN]     = { { MAS, HKICK, TAUNT, KI },    110, 110, 105, 105, 105 },   // justice poses
    [TF_SUPER_BUU]    = { { SCAT, SHOCK, BB, BURST },   155, 140, 150, 150, 145 },   // Human Extinction, Vice Shout
    [TF_PUIPUI]       = { { SCAT, HPUNCH, KI, RUSH },   100, 100, 100, 100, 100 },
    [TF_GOTEN]        = { { RUSH, MAS, KI, HKICK },     105, 110, 110, 110, 100 },   // small, fast
    [TF_KID_TRUNKS]   = { { BB, BURN, RUSH, KI },       105, 110, 110, 110, 100 },   // Big Tree Cannon, Galick-style beam
    [TF_YAKON]        = { { HPUNCH, MEGA, SHOCK, RUSH }, 130, 90, 120,  90, 125 },   // claws
    [TF_PIKKON]       = { { BURN, SHOCK, BB, KI },      125, 120, 125, 120, 120 },   // Thunder Flash, Hyper Tornado, Burning Shoot
    [TF_DABURA]       = { { SCAT, KI, MEGA, SHOCK },    135, 125, 130, 130, 130 },   // Evil Flame, Stone Spit stand-in
    [TF_EVIL_BUU]     = { { BURN, BB, RUSH, KI },       130, 125, 130, 130, 125 },   // mouth blast, Vanishing Ball
    [TF_KIBITO]       = { { HPUNCH, SHOCK, KI, RUSH },  115, 110, 110, 110, 115 },
    [TF_FAT_BUU]      = { { KI, BURST, BURN, MEGA },    150, 120, 135, 125, 140 },   // Candy Beam, Angry Explosion
    [TF_JANEMBA]      = { { RUSH, BB, SCAT, HPUNCH },   145, 130, 140, 135, 140 },   // Rakshasa's Claw, Hell's Gate
    [TF_KID_BUU]      = { { BB, SCAT, RUSH, BURST },    145, 140, 145, 150, 135 },   // Planet Burst, mad rush
    [TF_GOTENKS]      = { { DISC, BB, RUSH, KI },       130, 130, 135, 135, 125 },   // Galactic Donut, the ghosts
    [TF_UUB]          = { { SCAT, RUSH, KI, SHOCK },    130, 125, 135, 130, 125 },
    [TF_VEGITO]       = { { BURN, RUSH, BB, BURST },    160, 150, 155, 155, 150 },   // Final Kamehameha, Spirit Sword
};

#define TNY_PLAY_FLAGS 0x7B      // in party (1), available (2), all four moves learned (0x78)

int tny_play_row(int id) {
    if (id < 0 || id >= TF_COUNT) return 0;
    if (!(tny_fighters[id].role & (TR_BEAT | TR_CHALLENGER))) return 0;
    return tny_play_rows[id];
}

static u16 pct(u32 v, u32 p) { v = v * p / 100; return (u16)(v > 0xFFFF ? 0xFFFF : v < 1 ? 1 : v); }

void tny_play_entry(int id, CharacterEntry *out, const CharacterEntry *lead) {
    int row = tny_play_row(id);
    if (!row) {                  // not a tournament playable: the lead's own entry, nothing invented
        if (out != lead) memcpy(out, lead, sizeof *out);
        return;
    }
    const TKit *k = &kits[id];
    static const TKit plain = { { RUSH, BB, KI, SHOCK }, 100, 100, 100, 100, 100 };
    if (!k->hp) k = &plain;      // a playable added to the table without a kit here
    CharacterEntry c;
    memset(&c, 0, sizeof c);
    c.display_row = (u8)row;
    c.exp = lead->exp;
    c.max_hp = c.hp = pct(lead->max_hp, k->hp);
    c.max_ep = c.ep = pct(lead->max_ep, k->ep);
    c.str = pct(lead->str, k->str);
    c.pow = pct(lead->pow, k->pow);
    c.end = pct(lead->end, k->end);
    c.flags = TNY_PLAY_FLAGS;
    for (int i = 0; i < 4; i++) c.ability_list[i] = k->ab[i];
    *out = c;
}
