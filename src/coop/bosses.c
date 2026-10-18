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
};
const u32 extra_stat_count = sizeof extra_stats / sizeof extra_stats[0];

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
    { A13_STAT, 0, BOSS_PAIR, 0, 0 },
    { 7,  0, BOSS_PAIR, A13_SPRITE, A13_GERO_STAT },
    { A13_GERO_STAT, 0, BOSS_PAIR, 0, 0 },
    // The forest spirit (zone 2 area 8, the stock dream's clearing): Frieza and Cooler, always a pair
    { 41, 0, BOSS_PAIR, COOLER_SPRITE, COOLER_SPIRIT_STAT },
    { COOLER_SPIRIT_STAT, 0, BOSS_PAIR, 0, 0 },     // the twin's own stat: boss pair rules for it too
    // Cooler (New Namek bonus) + his Namekian servant
    { 29, 0, 0, NAMEK_SPRITE, NAMEK_STAT },
    // Roshi's gym in the ten-day wait: the sparring matches are duos, Yamcha + Tien and Krillin + Hercule
    // (Hercule's sprite with Krillin's numbers); always pairs, so the duos stand in single-player too
    { 106, 0, BOSS_PAIR, 121, 98 },
    { 98,  0, BOSS_PAIR, 0, 0 },
    { 64,  0, BOSS_PAIR, 72, 0 },
    // Perfect Cell vs Ascended Trunks and Krillin: alone, ends at half health (he laughs it off)
    { 78, 50, BOSS_NO_TWIN, 0, 0 },
    { 79, 50, BOSS_NO_TWIN, 0, 0 },
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
    if (i < 0 || (u32)i >= extra_stat_count) return 0;
    const ExtraStat *x = &extra_stats[i];
    *out = ENEMY_STATS[x->base];
    if (x->ai_set >= 0) out->ai_set = x->ai_set;
    if (x->max_hp) out->max_hp = x->max_hp;
    if (x->exp >= 0) out->exp = x->exp;
    if (x->melee) out->melee = x->melee;
    if (x->ranged) out->ranged = x->ranged;
    if (x->defence) out->defence = x->defence;
    return 1;
}
