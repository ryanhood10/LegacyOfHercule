// New enemy stats and per-boss rules (bosses.h). Numbers here are our own tuning, not ROM data: an
// extra stat names a stock stat to start from and overrides what differs.
#include "bosses.h"

const ExtraStat extra_stats[] = {
    // 110 Android 13, base form (not yet "Super"): Android 19's AI set (29) and boss class. More HP than
    // 19 (2970, 21/14/11) but about his damage: the twin goes for the other player, so 13 usually fights
    // player 2 (Hercule beside Vegeta). 19's co-op partner on the zone 7 battlefield.
    { 5, -1, 3200, -1, 22, 16, 14, 0 },
    // 111 Cooler as the forest spirit shows him to Gohan and Krillin (beside Frieza, stat 41, the stock
    // dream boss): Cooler's own AI set and class, Frieza's tutorial HP, EXP and attack numbers.
    { 29, -1, 130, 20, 3, 2, 3, 0 },
    // 112 Android 13 beside Dr. Gero at the lab door: more HP than Gero (1900, 25/21/17), a little
    // less damage (he backs Gero up; 13 survives the scene and leaves).
    { 5, -1, 2400, -1, 24, 20, 16, 0 },
    // 113 Cooler's Namekian servant: Cooler's melee boss AI set (21) and class; about Cooler's damage
    // but 6000 HP of his 10200 (75/85/79), so the pair is one fight's worth for two players. (At 68/60/64
    // he barely scratched a level-30 Hercule in the fork while Cooler shredded Piccolo.)
    { 29, -1, 6000, -1, 74, 70, 68, 0 },
    // 114 Android 13 in the arctic (the ten-day wait, data/archive/ch8_arctic.lscript): 19's AI set and class,
    // a Cell-saga boss's numbers; the fight ends at half health (rule below). Our tuning, untested in play.
    { 5, -1, 6000, -1, 30, 24, 22, 0 },
    // 115 Legendary Super Saiyan Broly (the Broly side chapter in the ten-day wait, data/story/broly.lscript):
    // Cooler's melee boss AI set (21) and boss class, the strongest numbers of the wait: Cooler's HP
    // class, hits about 1.3 x Perfect Cell's (44/46/50; 1.8 x once enraged). Our tuning: the test
    // autopilot with the Map Test party lost to 58/60/55 + rage in about 20 s.
    { 29, -1, 8000, -1, 52, 54, 50, 0 },
    // 116 Paragus beside him: the same AI set, a fraction of Broly's numbers (he is the weak one); when he
    // falls first Broly rages (BOSS_RAGE below).
    { 29, -1, 2000, -1, 24, 22, 20, 0 },
};
const u32 extra_stat_count = sizeof extra_stats / sizeof extra_stats[0];

// 120-124: the "Gero's computer" side chapter (data/story/ch9_gero.*)
static const ExtraStat gero_stats[] = {
    // 120 / 121 the bunker's two Android 19 spare bodies against Krillin and Android 16 (Cell-saga numbers,
    // each a little above the zone 7 19's 2970 HP, 21/14/11; fork autopilot at the debug party's level 40: see
    // docs/coop-design.md "Gero's bunker")
    { 5, -1, 3600, -1, 23, 17, 16, 0 },
    { 5, -1, 3600, -1, 23, 17, 16, 0 },
};
#define GERO_STAT_COUNT (sizeof gero_stats / sizeof gero_stats[0])

// 140-144: the Cell Games (data/story/cell_games.*)
static const ExtraStat cell_stats[] = {
    // 140 the second Perfect Cell of the final fight: a copy of the stock final stage (stat 25)
    { 25, -1, 0, -1, 0, 0, 0, 0 },
};
#define CELL_STAT_COUNT (sizeof cell_stats / sizeof cell_stats[0])

static const BossRule rules[] = {
    // Androids 17 and 18 pair with each other's model (the road: Piccolo + Vegeta vs 17 + 18)
#ifdef BOSS_RULE_TEST
    { 3,  50, BOSS_NO_TWIN, 0, 0 },   // test build: 17 alone, ends at half health
#else
    { 3,  0, 0, 11, 0 },
#endif
    { 4,  0, 0, 9,  0 },
    // Android 19 + Android 13 (zone 7 area 15), Dr. Gero + Android 13 (the lab door): always a pair, so the
    // story scripts (13's lines, 13 flying off) find 13 in single-player too
    { 5,  0, BOSS_PAIR, A13_SPRITE, A13_STAT },
    { A13_STAT, 0, BOSS_PAIR | BOSS_PARTNER_ONLY, 0, 0 },
    { 7,  0, BOSS_PAIR, A13_SPRITE, A13_GERO_STAT },
    { A13_GERO_STAT, 0, BOSS_PAIR | BOSS_PARTNER_ONLY, 0, 0 },
    // The forest spirit (zone 2 area 8, the stock dream's clearing): Frieza and Cooler, always a pair
    { 41, 0, BOSS_PAIR, COOLER_SPRITE, COOLER_SPIRIT_STAT },
    { COOLER_SPIRIT_STAT, 0, BOSS_PAIR | BOSS_PARTNER_ONLY, 0, 0 },     // the twin's own stat: boss pair rules for it too
    // Cooler (New Namek bonus) + his Namekian servant
    { 29, 0, 0, NAMEK_SPRITE, NAMEK_STAT },
    // Roshi's gym in the ten-day wait: the sparring matches are duos, Yamcha + Tien and Krillin + Hercule
    // (Hercule's sprite with Krillin's numbers); always pairs, so the duos stand in single-player too
    { 106, 0, BOSS_PAIR, 121, 98 },
    { 98,  0, BOSS_PAIR | BOSS_PARTNER_ONLY, 0, 0 },
    { 64,  0, BOSS_PAIR, 72, 0 },
    // Android 13 in the arctic: unused since the encounter was removed 2026-10-18 (docs/story-archive.md H); kept for 13's post-Cell return
    { A13_ARCTIC_STAT, 50, BOSS_NO_TWIN, 0, 0 },
    // Perfect Cell vs Ascended Trunks and Krillin: alone, ends at half health (he laughs it off)
    { 78, 50, BOSS_NO_TWIN, 0, 0 },
    { 79, 50, BOSS_NO_TWIN, 0, 0 },
    // Broly side chapter: Broly and Paragus, always a pair (single-player too); Paragus falling first
    // makes Broly rage once (full HP, stronger, faster; enemies.c)
    { BROLY_STAT, 0, BOSS_PAIR, PARAGUS_SPRITE, PARAGUS_STAT },
    { PARAGUS_STAT, 0, BOSS_PAIR | BOSS_PARTNER_ONLY | BOSS_RAGE, 0, 0 },
    // Gero's bunker: two Android 19 spare bodies at once, single-player too (Ryan: two 19s on screen, no more
    // in co-op): the record's 19 is a BOSS_PAIR whose twin is the second 19; both-down, one defeat dialogue
    { BUNKER19_STAT,  0, BOSS_PAIR, 12, BUNKER19B_STAT },
    { BUNKER19B_STAT, 0, BOSS_PAIR | BOSS_PARTNER_ONLY, 0, 0 },
    // The Cell Games (docs/story.md 4h): one Perfect Cell per fight until the end (stats 21-24 alone, the
    // Juniors (26) alone in their solo fights), then two Perfect Cells in the final fight (stat 25 always a
    // pair with the copy 140, single-player too; both-down)
    { 21, 0, BOSS_NO_TWIN, 0, 0 },
    { 22, 0, BOSS_NO_TWIN, 0, 0 },
    { 23, 0, BOSS_NO_TWIN, 0, 0 },
    { 24, 0, BOSS_NO_TWIN, 0, 0 },
    { 26, 0, BOSS_NO_TWIN, 0, 0 },
    { 25, 0, BOSS_PAIR, CELL2_SPRITE, CELL2_STAT },
    { CELL2_STAT, 0, BOSS_PAIR | BOSS_PARTNER_ONLY, 0, 0 },
    // Imperfect Cell (60) and Semi-Perfect Cell (76) use the default: a same-model Multi-Form twin
    { -1, 0, 0, 0, 0 },
};

const BossRule *boss_rule(int stat_index) {
    for (const BossRule *r = rules; r->stat >= 0; r++)
        if (r->stat == stat_index) return r;
    return 0;
}

int resolve_stat(int stat_index, EnemyStat *out) {
    if (stat_index >= 0 && stat_index < STOCK_STAT_COUNT) {
        *out = ENEMY_STATS[stat_index];
        return 1;
    }
    int i = stat_index - EXTRA_STAT_BASE;
    const ExtraStat *x;
    if (stat_index >= GERO_STAT_BASE && (u32)(stat_index - GERO_STAT_BASE) < GERO_STAT_COUNT)
        x = &gero_stats[stat_index - GERO_STAT_BASE];
    else if (stat_index >= CELL_STAT_BASE && (u32)(stat_index - CELL_STAT_BASE) < CELL_STAT_COUNT)
        x = &cell_stats[stat_index - CELL_STAT_BASE];
    else if (i >= 0 && (u32)i < extra_stat_count)
        x = &extra_stats[i];
    else
        return 0;
    *out = ENEMY_STATS[x->base];
    if (x->ai_set >= 0) out->ai_set = x->ai_set;
    if (x->max_hp) out->max_hp = x->max_hp;
    if (x->exp >= 0) out->exp = x->exp;
    if (x->melee) out->melee = x->melee;
    if (x->ranged) out->ranged = x->ranged;
    if (x->defence) out->defence = x->defence;
    return 1;
}
