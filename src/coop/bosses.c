// New enemy stats and per-boss rules (bosses.h). Numbers here are our own tuning, not ROM data: an
// extra stat names a stock stat to start from and overrides what differs.
#include "bosses.h"

const ExtraStat extra_stats[] = {
    // 110 Android 13, base form (not yet "Super"): Android 19's AI set (29) and boss class. More HP than
    // 19 (2970, 21/14/11) but about his damage: the twin goes for the other player, so 13 usually fights
    // player 2 (Hercule beside Vegeta). 19's co-op partner on the zone 7 battlefield.
    { 5, -1, 3200, -1, 22, 16, 14, 0 },
    // 111 Cooler's Namekian servant: Cooler's melee boss AI set (21) and class, well below Cooler
    // (10200 HP, 75/85/79) so the pair is one fight's worth for two players.
    { 29, -1, 6000, -1, 68, 60, 64, 0 },
    // 112 Android 13 beside Dr. Gero at the lab door: more HP than Gero (1900, 25/21/17), a little
    // less damage (he backs Gero up; 13 survives the scene and leaves).
    { 5, -1, 2400, -1, 24, 20, 16, 0 },
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
    // Android 19 + Android 13 (zone 7 area 15), Dr. Gero + Android 13 (the lab door)
    { 5,  0, 0, A13_SPRITE, A13_STAT },
    { 7,  0, 0, A13_SPRITE, A13_GERO_STAT },
    // Cooler (New Namek bonus) + his Namekian servant
    { 29, 0, 0, NAMEK_SPRITE, NAMEK_STAT },
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
