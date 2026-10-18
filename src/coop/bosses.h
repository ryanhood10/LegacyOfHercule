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

typedef struct BossRule {
    s16 stat;            // stat index this rule applies to; -1 ends the table
    u8 end_pct;
    u8 flags;
    s16 partner_sprite;
    s16 partner_stat;
} BossRule;

// Sprite ids of the new boss models (0 until the art exists = the original's model).
#ifndef A13_SPRITE
#define A13_SPRITE 0
#endif
#define A13_STAT 110        // Android 13, base form

extern const ExtraStat extra_stats[];
extern const u32 extra_stat_count;

const BossRule *boss_rule(int stat_index);
// Resolve a stat index (stock or extra) into *out. Returns 0 for an unknown index.
int resolve_stat(int stat_index, EnemyStat *out);
