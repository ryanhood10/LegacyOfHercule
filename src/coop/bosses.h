#pragma once
#include "rom.h"

// New enemy stats and per-boss rules for the story (docs/story.md, docs/coop-design.md "Boss rules").
//
// Extra stats: indices >= EXTRA_STAT_BASE name entries of extra_stats[] (bosses.c). Each entry is a
// stock stat with overrides; the spawn resolves it into the enemy's private stat copy, so no ROM
// table is copied into the build. 109 is Android 16 (android16.c, own spawn path).
//
// Boss rules: per stat index (stock or extra), applied in single-player and co-op alike:
//   end_pct        the fight ends when HP reaches end_pct % of the nominal max: the entity gets
//                  (100 - end_pct) % of the HP and the boss bar is drawn over [end_pct, 100]
//   partner_sprite the co-op twin's model (0 = the same model: a Multi-Form duplicate)
//   partner_stat   the co-op twin's stat (0 = the same stat as the original)
//   BOSS_NO_TWIN   the boss fights alone even in co-op
//   BOSS_PAIR      always a pair, single-player too, under the boss rules (partner model and stat,
//                  both-down) whatever the stat's HP and EXP (the forest "spirits", Frieza + Cooler)
//   BOSS_RAGE      on the partner's stat: when this boss falls while the other of the pair still stands,
//                  the survivor rages once (enemies.c): full HP again, BOSS_RAGE_* more attack and defence,
//                  faster chase modules, and the rage dialogue set by CoopCall 9 (Paragus -> Broly)
// Pairs follow the both-down rule (enemies.c): the first boss to fall stays down on the map and only
// the last one's death runs the fight's defeat script.

#define EXTRA_STAT_BASE 110
#define STOCK_STAT_COUNT 109

typedef struct ExtraStat {
    s16 base;            // stock stat index copied first (AI set, class, resistances)
    s16 ai_set;          // -1 = keep the base's
    s32 max_hp;          // 0 = keep
    s32 exp;             // -1 = keep
    u8 melee, ranged, defence, _pad;   // 0 = keep
} ExtraStat;

#define BOSS_NO_TWIN 1
#define BOSS_PAIR    2
#define BOSS_RAGE    4

// Broly's rage when Paragus falls first (docs/story.md 4g); our tuning
#define BOSS_RAGE_MELEE    10     // added to the survivor's melee / ranged attack bytes (capped at 255)
#define BOSS_RAGE_DEFENCE  6      // added to its defence byte
#define BOSS_RAGE_SPEED_NUM 13    // chase speed x 13/10 from the rage on
#define BOSS_RAGE_SPEED_DEN 10

typedef struct BossRule {
    s16 stat;            // stat index this rule applies to; -1 ends the table
    u8 end_pct;
    u8 flags;
    s16 partner_sprite;
    s16 partner_stat;
} BossRule;

// Sprite ids of the new boss models (tools/bossmodels.py; the Makefile passes A13_SPRITE=165 and
// NAMEK_SPRITE=166; 0 = no art, the twin keeps the original's model).
#ifndef A13_SPRITE
#define A13_SPRITE 0
#endif
#ifndef NAMEK_SPRITE
#define NAMEK_SPRITE 0
#endif
#define A13_STAT 110        // Android 13, base form, beside Android 19
#define COOLER_SPIRIT_STAT 111   // the forest spirit's Cooler (docs/story.md 4): Frieza's tutorial numbers
#define COOLER_SPRITE 34
#define NAMEK_STAT 113      // Cooler's Namekian servant (New Namek bonus)
#define A13_GERO_STAT 112   // Android 13 beside Dr. Gero at the lab door
#define A13_ARCTIC_STAT 114 // Android 13 in the arctic (the ten-day wait), a half-health boss
#define BROLY_STAT 115      // Legendary Super Saiyan Broly (Broly side chapter, zone 15 area 3)
#define PARAGUS_STAT 116    // Paragus beside him (BOSS_RAGE: Broly rages when his father falls first)
#define BROLY_SPRITE 159    // Buu's Fury Broly (tools/buu2log2.py)
#ifndef PARAGUS_SPRITE
#define PARAGUS_SPRITE 0    // Vegeta's set made old (tools/brolysprites.py; the Makefile passes 170)
#endif
// "Gero's computer" (docs/story.md 4g) owns stats 120-124, kept in their own table (gero_stats[], bosses.c)
// so other ranges can be added beside it without renumbering
#define GERO_STAT_BASE 120
#define BUNKER19_STAT 120   // the bunker's Android 19 spare body A (the record), BOSS_PAIR with B
#define BUNKER19B_STAT 121  // spare body B (the pair's twin)

extern const ExtraStat extra_stats[];
extern const u32 extra_stat_count;

const BossRule *boss_rule(int stat_index);
// Resolve a stat index (stock or extra) into *out. Returns 0 for an unknown index.
int resolve_stat(int stat_index, EnemyStat *out);
