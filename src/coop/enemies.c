// Co-op difficulty: while a second pad is present, map enemies spawn with scaled stats and a twin
// beside them; twins target the player the original is not after. Bosses (no EXP, >= 1000 HP) keep
// their stats and get a twin of the partner model with its own boss bar. Single-player is untouched:
// without a player-2 source every hook falls through to the original behaviour.
//
// Hooked entries (tools/patch_rom.py): the map-script enemy handler 0x0800E77E, the damage
// calculation 0x0800F910, the two chase-module constructors (0x0801B332, 0x0801B15E) that carry the
// walk speed, and BossHealthBar_Create 0x080104C0.
#include "coop.h"
#include "android16.h"
#include "bosses.h"

ROM_FN(void, Enemy_UpdateStates, (Entity *e), 0x0800772B);   // aux + control state; the AI tick follows in 0x0800E5A5
#define ENEMY_UPDATE 0x0800E5A5u

CoopExtra coop2 __attribute__((section(".coop2"), used));

BossBar *bossbar_create_hook(void *mem, Entity *e, int lo, int hi);

void *memcpy(void *dst, const void *src, unsigned n);   // util.c
#define COLLISION_MAP ((const void *)g_Ctx[4])

int is_coop_enemy(const Entity *e) { return e && e->vtable == coop2.enemy_vtable; }

// A co-op enemy's record copy lives behind its stat copy in the entity block (create_coop_enemy).
static EnemyRecord *record_of(Entity *e) { return (EnemyRecord *)((u8 *)e + ENEMY_SIZE + sizeof(EnemyStat)); }
static const EnemyRecord *record_of_const(const Entity *e) { return (const EnemyRecord *)((const u8 *)e + ENEMY_SIZE + sizeof(EnemyStat)); }
static int is_enemy(const Entity *e) { return e && (e->vtable == (u32 *)ENEMY_VTABLE || e->vtable == coop2.enemy_vtable || is_a16(e)); }

// Bosses: real HP and either no EXP (story bosses) or a record that only a script spawns (condition
// word 1: the Cell forms, the tournament opponents). Scripted targets such as rocks (no EXP, tiny HP)
// are left alone.
static int is_boss_rec(const EnemyRecord *rec, const EnemyStat *st) {
    const BossRule *r = boss_rule(rec->stat_index);
    if (r && (r->flags & BOSS_PAIR)) return 1;     // a pair under boss rules whatever its numbers
    return st->max_hp >= 1000 && (st->exp == 0 || rec->flags == 1);
}
static int is_boss(const Entity *e) { return is_boss_rec(record_of_const(e), (const EnemyStat *)e->behaviour); }
static int twinnable(const EnemyRecord *rec, const EnemyStat *st) { return st->exp > 0 || is_boss_rec(rec, st); }

// Partner model for boss twins: stat index -> sprite id (0 = same model). See docs/coop-design.md
// "Boss roster" for how the fights were identified.
// The table lives in bosses.c (boss rules). Cooler (stat 29): King Cold's sprite (74) with Cooler's AI
// set crashes at spawn (so does any other record; Cell's AI takes Cooler's model fine), so Cooler keeps
// the same model until module 0x0801C05B is understood.
static int partner_sprite(int stat_index) {
    if (coop2.partner_override) return (int)coop2.partner_override;
    const BossRule *r = boss_rule(stat_index);
    return r ? r->partner_sprite : 0;
}

// The twin's record copy points its action object here, so a twin's death never fires the
// original's defeat dialogue / script (the pair rules below hand the action to the survivor).
static const u32 no_action[2] = { ENEMY_ACTION_NONE, 0 };

// ---- twin registry ------------------------------------------------------------------------------
static TwinPair *pair_of(const Entity *e) {
    for (int i = 0; i < TWIN_MAX; i++)
        if (coop2.twins[i].orig == e || coop2.twins[i].twin == e) return &coop2.twins[i];
    return 0;
}

static void unregister(Entity *e) {
    for (int i = 0; i < TWIN_MAX; i++) {
        if (coop2.twins[i].orig == e) coop2.twins[i].orig = 0;
        if (coop2.twins[i].twin == e) coop2.twins[i].twin = 0;
    }
}

static TwinPair *register_pair(Entity *orig, Entity *twin) {
    unregister(orig);   // a recycled address must not alias an old pair
    unregister(twin);
    for (int i = 0; i < TWIN_MAX; i++) {
        if (!coop2.twins[i].orig && !coop2.twins[i].twin) {
            coop2.twins[i].orig = orig;
            coop2.twins[i].twin = twin;
            return &coop2.twins[i];
        }
    }
    return 0;
}

// Which player an enemy sees this frame (entities.c sets g_PlayerObject to it): the nearest, except
// that a twin whose original is alive takes the player the original is not nearest to.
Entity *coop_target(Entity *e, Entity *p1, Entity *p2) {
    Entity *near = nearest_player(e, p1, p2);
    if (!is_coop_enemy(e)) return near;
    TwinPair *pr = pair_of(e);
    if (!pr || pr->twin != e || !pr->orig) return near;
    Entity *orig_near = nearest_player(pr->orig, p1, p2);
    return orig_near == p1 ? p2 : p1;
}

// ---- vtable copies ------------------------------------------------------------------------------
static void coop_enemy_destroy(Entity *e, int free_it, u32 a, u32 b) {
    unregister(e);
    e->vtable = (u32 *)ENEMY_VTABLE;   // the original resets it too; keep the list consistent first
    Enemy_Destroy(e, free_it, a, b);
}

// ---- boss pairs ---------------------------------------------------------------------------------

static void set_sprite(Entity *e, int sprite_id) {
    e->sprite_record = Character_GetSpriteRecord(sprite_id);
    ((void (*)(Entity *, int))vslot(e, 10))(e, 0);   // idle control state: frames come from the new record
}

// Remove a boss from the fight without its script: regular-enemy death (no EXP for bosses, popup,
// list removal, dust, SFX) after telling every entity (the bars) that it died.
static void quiet_death(Entity *e) {
    e->hp = 0;
    EntityList_Broadcast(g_SceneTable[1], 0xF, e);
    Enemy_RegularDeath(ENEMY_CLASS_REGULAR, e);
}

// A boss that fell while its partner fights on (both-down rule): HP 0, no action, no AI. The ROM's
// boss death would remove the entity, so a downed boss never reaches it: it takes the blow as a
// non-lethal hit (knock-back, damage popup), then HP 0; its states run DOWN_SETTLE more frames (the
// knock-back finishes) and then it stays frozen where it fell until the map changes.
#define DOWN_SETTLE 90
static s32 *down_timer(EnemyRecord *r) { return record_orig_sprite(r) + 1; }   // record copy word after the original sprite
static int is_downed(const Entity *e) {
    return is_coop_enemy(e) && e->hp <= 0 && record_of_const(e)->action == (void *)no_action;
}

// The knock-down pose: pose group 3 (standing, falling, lying) on the boss models that have it
// (17, Gero, the Cell forms); Android 19's record has no group 3.
#define DOWN_GROUP 3
static const u32 *down_group(const Entity *e) {
    const u32 *g = (const u32 *)((const u8 *)e->sprite_record + 0x4C + 0x10 * DOWN_GROUP);
    return g[0] ? g : 0;
}

// Enemy vtable slot 2 for co-op enemies: a downed boss plays its knock-down and freezes on the last
// frame (or after DOWN_SETTLE frames without one): no states, no AI.
static void coop_enemy_update(Entity *e) {
    if (is_downed(e)) {
        s32 *t = down_timer(record_of(e));
        const u32 *g = down_group(e);
        int lying = g && *(const u32 **)((const u8 *)e + 0x4C) == g && *((const u8 *)e + 0x14) >= 3;
        if (*t < DOWN_SETTLE && !lying) { (*t)++; Enemy_UpdateStates(e); }
        return;
    }
    ((void (*)(Entity *))ENEMY_UPDATE)(e);
}

static void go_down(Entity *e, const u8 *hit) {
    record_of(e)->action = (void *)no_action;
    e->action = (void *)no_action;
    *down_timer(record_of(e)) = 0;
    // the hit sits inside the caller's message (OnHit reads around it), so trim its damage in place
    s32 *dmg = (s32 *)(hit + 8);
    s32 full = *dmg;
    if (e->hp > 1) { *dmg = e->hp - 1; Enemy_OnHit(e, hit); }   // knock-back and popup, not the death
    *dmg = full;
    if (down_group(e)) Entity_SetAnimation(e, DOWN_GROUP);   // control state 3 plays pose group 3
    e->hp = 0;
    EntityList_Broadcast(g_SceneTable[1], 0xF, e);   // "entity died": its boss bar lets go
    coop2.downed++;
}

// Enemy vtable slot 14 (OnHit: HP -= hit[+8], death at 0) for co-op enemies. Boss pairs follow the
// both-down rule: the first of the two to fall hands the original's role (record action, the script's
// model) to the survivor and stays down on the map; the survivor's death then runs the fight's script
// as in single-player.
static int coop_enemy_onhit(Entity *e, const u8 *hit) {
    if (e->hp <= 0) return 1;   // already down: further hits do nothing
    s32 dmg = *(const s32 *)(hit + 8);
    TwinPair *pr = pair_of(e);
    if (pr && is_boss(e) && dmg >= e->hp) {
        Entity *partner = pr->orig == e ? pr->twin : pr->orig;
        if (partner) {
            if (pr->orig == e) {
                // the original falls first: the twin becomes the original (action, script model)
                record_of(partner)->action = e->action;
                partner->action = e->action;
                pr->orig = partner;
                pr->twin = 0;
            } else {
                pr->twin = 0;
            }
            if (coop2.off & COOP_OFF_BOTHDOWN) quiet_death(e);
            else go_down(e, hit);
            return 1;
        }
        // last of the pair: a partner-model twin dies as the original model so the script sees it
        if (record_of(e)->sprite_id != *record_orig_sprite(record_of(e))) set_sprite(e, *record_orig_sprite(record_of(e)));
    }
    return Enemy_OnHit(e, hit);
}

static void coop_bar_update(BossBar *bar);
static int coop_bar_draw(BossBar *bar, u32 **oam);

void enemies_init(void) {
    u32 base = (u32)coop2.enemy_vtable;
    const u32 *orig = (const u32 *)ENEMY_VTABLE;
    for (int i = 0; i < 48; i++) coop2.enemy_vtable[i] = (ENEMY_VTABLE + orig[i]) - base;
    coop2.enemy_vtable[0] = (u32)coop_enemy_destroy - base;
    coop2.enemy_vtable[2] = (u32)coop_enemy_update - base;
    coop2.enemy_vtable[14] = (u32)coop_enemy_onhit - base;

    base = (u32)coop2.bar_vtable;
    orig = (const u32 *)BOSSBAR_VTABLE;
    for (int i = 0; i < 24; i++) coop2.bar_vtable[i] = (BOSSBAR_VTABLE + orig[i]) - base;
    coop2.bar_vtable[2] = (u32)coop_bar_update - base;
    coop2.bar_vtable[4] = (u32)coop_bar_draw - base;

    if (coop2.magic != COOP2_MAGIC) {
        coop2.magic = COOP2_MAGIC;
        for (int i = 0; i < TWIN_MAX; i++) coop2.twins[i].orig = coop2.twins[i].twin = 0;
        coop2.spawned = coop2.scaled = coop2.bars = 0;
        coop2.off = 0;
        coop2.actor_count = 0;
        coop2.partner_override = 0;
        coop2.a16 = 0;
        coop2.a16_force = 0;
        coop2.downed = 0;
        coop2.dupes_removed = 0;
        coop2.duel_active = 0;
        coop2.duel_tint = 0;
        coop2.pad_swap = 0;
    }
}

// ---- spawning -----------------------------------------------------------------------------------
// Replacement for the map loader's enemy record handler (0x0800E77E): Enemy_Create(0, rec, stat).
// In co-op the entity block also carries a private stat copy (scaled HP) and a record copy with the
// shifted spawn point (the constructor registers the collision box at the record's position, so the
// twin must be created where it stands) and, for boss twins, the partner sprite.
static u32 record_size(const EnemyRecord *rec) { return 0x1C + 4 * (u32)rec->behaviour_count; }
static u32 record_copy_size(const EnemyRecord *rec) { return record_size(rec) + 8; }   // + original sprite id, down timer

static Entity *create_coop_enemy(const EnemyRecord *rec, const EnemyStat *st, int scale, int dx, int sprite, int twin, int stat_index) {
    u32 rsize = record_copy_size(rec);
    u8 *block = Arena_Alloc(ENEMY_SIZE + sizeof(EnemyStat) + rsize);
    if (!block) return 0;
    EnemyStat *stat = (EnemyStat *)(block + ENEMY_SIZE);
    EnemyRecord *r = (EnemyRecord *)(block + ENEMY_SIZE + sizeof(EnemyStat));
    *stat = *st;
    if (scale && !(coop2.off & COOP_OFF_HP)) {
        stat->max_hp = st->max_hp * COOP_HP_NUM / COOP_HP_DEN;
        coop2.scaled++;
    }
    memcpy(r, rec, record_size(rec));
    r->x = (s16)(rec->x + dx);
    *record_orig_sprite(r) = rec->sprite_id;
    if (sprite) r->sprite_id = sprite;
    if (stat_index >= 0) r->stat_index = stat_index;   // partner stat: rule lookups follow the twin's own stat
    if (twin) r->action = (void *)no_action;   // twins never fire the record's action
    Entity *e = Enemy_Create(block, r, stat);
    if (!e) { Arena_Free(block); return 0; }
    e->vtable = coop2.enemy_vtable;
    return e;
}

// The stat a record spawns with: the stock entry, an extra stat (bosses.c), and for a boss rule with
// end_pct only the HP above end_pct. Returns the stock pointer when nothing changes (no private copy
// needed in single-player), else fills *buf.
static const EnemyStat *spawn_stat(int stat_index, const BossRule *rule, EnemyStat *buf) {
    const EnemyStat *st;
    if (stat_index < STOCK_STAT_COUNT) {
        st = &ENEMY_STATS[stat_index];
    } else {
        if (!resolve_stat(stat_index, buf)) return 0;
        st = buf;
    }
    if (rule && rule->end_pct) {
        if (st != buf) { *buf = *st; st = buf; }
        buf->max_hp = buf->max_hp * (100 - rule->end_pct) / 100;
    }
    return st;
}

Entity *enemy_spawn_hook(const EnemyRecord *rec) {
    coop_init_once();
    if (rec->stat_index == A16_STAT) return a16_spawn(rec);   // Android 16: single boss, own AI (android16.c)
    const BossRule *rule = boss_rule(rec->stat_index);
    EnemyStat buf;
    const EnemyStat *st = spawn_stat(rec->stat_index, rule, &buf);
    if (!st) return 0;
    int alone = rule && (rule->flags & BOSS_NO_TWIN);
    int pair = rule && (rule->flags & BOSS_PAIR);
    if ((!p2_present() && !pair) || !twinnable(rec, st) || alone)   // a private copy when the stat is not the ROM's
        return st == &buf ? create_coop_enemy(rec, st, 0, 0, 0, 0, -1) : Enemy_Create(0, rec, st);

    int boss = is_boss_rec(rec, st);
    Entity *orig = create_coop_enemy(rec, st, !boss, 0, 0, 0, -1);
    if (!orig) return 0;

    // Twin TWIN_OFFSET px to the right when that spot is walkable, else to the left, else on top.
    s32 px[2] = { rec->x + TWIN_OFFSET, rec->y };
    int dx = TWIN_OFFSET;
    if (!Collision_Check(COLLISION_MAP, px, (u8 *)orig + 0x154)) {
        px[0] = rec->x - TWIN_OFFSET;
        dx = (rec->x >= TWIN_OFFSET + 8 && Collision_Check(COLLISION_MAP, px, (u8 *)orig + 0x154)) ? -TWIN_OFFSET : 0;
    }
    // the twin: the partner's stat when the rule names one (Android 13 beside 19 or Dr. Gero)
    const EnemyStat *tst = st;
    EnemyStat tbuf;
    int tstat = -1;
    if (boss && rule && rule->partner_stat && !(coop2.off & COOP_OFF_PARTNER)) {
        const EnemyStat *p = spawn_stat(rule->partner_stat, boss_rule(rule->partner_stat), &tbuf);
        if (p) { tst = p; tstat = rule->partner_stat; }
    }
    Entity *twin = (coop2.off & COOP_OFF_TWINS) ? 0
                 : create_coop_enemy(rec, tst, !boss, dx, (boss && !(coop2.off & COOP_OFF_PARTNER)) ? partner_sprite(rec->stat_index) : 0, 1, tstat);
    if (twin) {
        EntityList_Add(g_SceneTable[1], twin);   // the loader adds the returned original itself
        register_pair(orig, twin);
        coop2.spawned++;
    }
    return orig;
}

// Chapter tests (COOP_CMD_STORY_WIN, or player 1 L + R + Up in STORY_TEST builds): every boss on the map (story bosses, Android 16) takes a
// blow worth its remaining HP from player 1 through its own OnHit (vtable slot 14), so pair rules,
// end_pct stages and the defeat action all run as if the players had won the fight.
void story_test_finish(void) {
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) {
        Entity *e = l->items[i];
        if (!is_enemy(e) || e->hp <= 0) continue;
        int boss = is_coop_enemy(e) ? is_boss(e) : ((const EnemyStat *)e->behaviour)->max_hp >= 1000;
        const u32 *act = (const u32 *)e->action;     // or any enemy whose death runs a story action
        if (!is_a16(e) && !boss && !(act && act[0] != ENEMY_ACTION_NONE)) continue;
        u8 m[0x40];                                 // a hit message as player attacks send them (android16.c)
        memset(m, 0, sizeof m);
        *(Entity **)(m + 0x20) = g_PlayerObject;   // attacker / source: player 1
        *(Entity **)(m + 0x30) = g_PlayerObject;
        *(s32 *)(m + 0x2C) = e->hp;                 // the applied damage OnHit subtracts
        ((int (*)(Entity *, const u8 *))((u32)e->vtable + e->vtable[14]))(e, m + 0x24);
    }
}

// ---- test commands ------------------------------------------------------------------------------
// enemies.lua writes coop2.cmd / cmd_arg; the entity-list hook runs them at the start of a frame.
void coop_debug_tick(void) {
    u32 cmd = coop2.cmd;
    if (!cmd) return;
    coop2.cmd = 0;
    if (cmd == COOP_CMD_SPAWN_SCRIPT) {
        Map_SpawnScript(g_MapState, (int)coop2.cmd_arg);
    } else if (cmd == COOP_CMD_ACTOR) {
        static const u32 face_wait[2] = { 0x0800CB0Fu, 0x7D00 };   // NpcBehavior_FaceAndWait: face down, 9 min
        ActorRecord *r = &coop2.actor_rec;
        Entity *p1 = g_PlayerObject;
        if (!p1) return;
        r->handler = (void *)0x0800D6DFu;
        r->flags = 0;
        u32 k = coop2.actor_count++;
        r->x = (s16)((p1->x >> 8) - 80 + 40 * (int)(k % 6));
        r->y = (s16)((p1->y >> 8) - 40 + 64 * (int)((k / 6) % 2));
        r->sprite_id = (s32)coop2.cmd_arg;
        r->behaviour_count = 1;
        r->behaviours[0] = face_wait;
        Entity *a = MapScript_CreateCharacter(r);
        if (a) EntityList_Add(g_SceneTable[1], a);
    } else if (cmd == COOP_CMD_A16) {
        a16_test_spawn();
    } else if (cmd == COOP_CMD_DUEL) {
        u32 a = coop2.cmd_arg;
        duel_start((int)(a & 0xFF) - 1, (int)((a >> 8) & 0xFF), (int)((a >> 16) & 0xFFFF) - 1);
    } else if (cmd == COOP_CMD_WARP) {
        // the bytecode WarpToMapNoEntities handler (opcode 39) pops y, x, variation, area, zone
        s32 stack[6] = { 5, (s32)(coop2.cmd_arg & 0xFF), (s32)((coop2.cmd_arg >> 8) & 0xFF), 1,
                         (s32)(coop2.cmd_arg2 & 0xFFFF), (s32)(coop2.cmd_arg2 >> 16) };
        ((void (*)(s32 *))0x08009A89u)(stack);
    } else if (cmd == COOP_CMD_BOSS_BAR) {
        Entity *e = (Entity *)coop2.cmd_arg;
        if (!e) e = coop2.twins[0].orig ? coop2.twins[0].orig : coop2.twins[0].twin;
        if (!e) return;
        BossBar *bar = bossbar_create_hook(0, e, 0, 100);
        if (bar) EntityList_Add(g_SceneTable[1], (Entity *)bar);
    }
}

// ---- walk speed ---------------------------------------------------------------------------------
// The chase modules are 8-byte objects {vtable, speed 8.8} built from a class record whose word 3 is
// the speed. Co-op enemies get a faster copy; everything else (NPC actors, single-player) is unchanged.
typedef struct SpeedModule { u32 *vtable; s32 speed; } SpeedModule;

static SpeedModule *speed_module(const u32 *cls, const Entity *e, u32 vtable) {
    SpeedModule *m = Arena_Alloc(sizeof *m);
    if (!m) return 0;
    m->vtable = (u32 *)vtable;
    m->speed = (s32)cls[3];
    if (coop.p2 && is_coop_enemy(e) && !(coop2.off & COOP_OFF_SPEED)) m->speed = m->speed * COOP_SPEED_NUM / COOP_SPEED_DEN;
    return m;
}
SpeedModule *ai_speed8_create_hook(const u32 *cls, const Entity *e) { return speed_module(cls, e, AI_SPEED8_VTABLE); }
SpeedModule *ai_speed4_create_hook(const u32 *cls, const Entity *e) { return speed_module(cls, e, AI_SPEED4_VTABLE); }

// ---- damage -------------------------------------------------------------------------------------
// Port of 0x0800F910(defender, hit): the same arithmetic and the same two RNG draws in the same
// order (lockstep netplay depends on it), then the co-op multiplier for enemy hits on either player.
void damage_calc_hook(Entity *defender, Hit *hit) {
    const u8 *attack = hit->attack;
    s32 base = *(const s32 *)(attack + 4);
    const Entity *attacker = hit->attacker;
    u32 stat;
    u32 res;
    if (attack[0] == 0) {
        stat = attacker->stat_melee;
        hit->crit = Rand_Chance(stat * 0xA4);
        base = (hit->crit + 1) * base;
        res = defender->res_melee;
    } else {
        stat = attacker->stat_ranged;
        hit->crit = 0;
        res = defender->res_ranged;
    }
    u32 def = defender->defence;
    s32 curve = (s32)(stat * (stat * 0x1A + 0xFFFFFD43u)) + 0x1C3A2;
    s32 r = Rand_Range(0x1A);
    s32 dmg = ((((s32)(stat * curve) >> 8) *
                (s32)(((u32)(((s32)(base * (0x80 - (s32)def)) >> 7) * (r + 0x100))) >> 8)) >> 0x12) * hit->mult >> 10;
    coop2.dmg_calls++;
    int player = defender == g_PlayerObject || defender == coop.p2;
    if (player) coop2.dmg_player++;
    if (coop.p2 && is_enemy(attacker) && player && !(coop2.off & COOP_OFF_DAMAGE)) {
        dmg = dmg * COOP_DMG_NUM / COOP_DMG_DEN;
        coop2.dmg_scaled++;
    }
    hit->dmg = dmg;
    hit->secondary = (s32)(res * dmg) >> 7;
    coop2.last_dmg = dmg;
    coop2.last_secondary = hit->secondary;
    coop2.last_defender = defender;
    coop2.last_attacker = (Entity *)attacker;
}

// ---- boss bars ----------------------------------------------------------------------------------
// The ROM bar sits at x = 160, under player 2's panel (152..232). Co-op bars use a vtable copy whose
// draw and update read the bar's own x / target y: both bars go in the 64 px gap between the panels
// (x = 88), the original at y = 8 and the twin's under it at y = 20.
#define BAR_X_COOP 88
#define BAR_Y_TOP 8
#define BAR_Y_TWIN 20
#define BAR_LIST (*(void **)0x086D60F0)   // -> g_EntityList (the bar's own ObjVram/list cell)

static void dma_copy32(const void *src, void *dst, int words) {
    REG_DMA3SAD = (u32)src;
    REG_DMA3DAD = (u32)dst;
    REG_DMA3CNT_L = (u16)words;
    REG_DMA3CNT_H = 0x8400;
}

// Port of the ROM draw (vtable slot 4, 0x0801055D) with the bar's own x: two 32x8 256-colour sprites,
// fill = lo + hp * range / max_hp pixels of 60, tiles rebuilt from the empty bar when the fill changes.
static int coop_bar_draw(BossBar *bar, u32 **oam) {
    u32 *o = *oam;
    u32 y = (u8)bar->y;
    u32 x = bar->x;
    o[0] = (((0x4000u | x) << 16) | 0x6000u) | y;
    o[1] = (o[1] & 0xFFFFF000u) | (bar->tile & 0x3FF);
    o[2] = (((0x4000u | (x + 32)) << 16) | 0x6000u) | y;
    o[3] = (o[3] & 0xFFFFF000u) | ((bar->tile + 8) & 0x3FF);
    *oam = o + 4;
    u32 fill = bar->lo;
    Entity *e = bar->entity;
    if (e) fill += (u32)(e->hp * bar->range) / (u32)((const EnemyStat *)e->behaviour)->max_hp;
    if (bar->px != fill) {
        bar->px = (u8)fill;
        u8 buf[0x200];
        dma_copy32(BOSSBAR_EMPTY_TILES, buf, 0x80);
        for (u32 i = fill; i < 60; i++) {
            const s32 *t = BOSSBAR_FILL_TABLE + i * 4;
            buf[t[0]] = 1; buf[t[1]] = 1; buf[t[2]] = 1; buf[t[3]] = 1;
        }
        dma_copy32(buf, (void *)(OBJ_VRAM + bar->tile * 0x20), 0x80);
    }
    return 0;
}

static BossBar *bar_create(void *mem, Entity *e, int lo, int hi, u32 *vtable, int x, int y_target) {
    BossBar *bar = mem ? mem : Arena_Alloc(sizeof *bar);
    if (!bar) return 0;
    bar->vtable = (u32 *)BOSSBAR_BASE_VTABLE;
    bar->size = 0x200;
    bar->tile = ObjVram_Alloc(BAR_LIST, 0x200);
    bar->vtable = vtable;
    bar->entity = e;
    bar->px = 0xFF;
    bar->kind = 4;
    bar->f4 = 0;
    bar->y = -8;
    bar->step = 1;
    bar->lo = (u8)(lo * 60 / 100);
    bar->range = (u8)(hi * 60 / 100 - bar->lo);
    bar->x = (u8)x;
    bar->y_target = (s8)y_target;
    return bar;
}

// Original slide: y += step until 8 (or removed below -8); co-op bars stop at their own target.
static void coop_bar_update(BossBar *bar) {
    int y = bar->y + bar->step;
    bar->y = (s8)y;
    if (y < -8) EntityList_Remove(BAR_LIST, bar, -8);
    else if (y > bar->y_target) { bar->y = bar->y_target; bar->step = 0; }
}

// A boss with an end_pct rule has only the HP above end_pct: its bar starts at end_pct.
static int bar_lo(const Entity *e, int lo) {
    if (!is_coop_enemy(e)) return lo;
    const BossRule *r = boss_rule(record_of_const(e)->stat_index);
    return (r && r->end_pct > lo) ? r->end_pct : lo;
}

BossBar *bossbar_create_hook(void *mem, Entity *e, int lo, int hi) {
    if (e) lo = is_a16(e) ? a16_bar_range(e, lo, hi) : bar_lo(e, lo);
    if (!coop.p2) return bar_create(mem, e, lo, hi, (u32 *)BOSSBAR_VTABLE, 160, BAR_Y_TOP);   // single-player: the ROM object
    TwinPair *pr = pair_of(e);
    if (pr && pr->orig && pr->twin) e = pr->orig;   // FindCharacterEntity searches from the end: it finds the twin
    BossBar *bar = bar_create(mem, e, lo, hi, coop2.bar_vtable, BAR_X_COOP, BAR_Y_TOP);
    if (bar && pr && pr->orig && pr->twin) {
        BossBar *second = bar_create(0, pr->twin, lo, hi, coop2.bar_vtable, BAR_X_COOP, BAR_Y_TWIN);
        if (second) { EntityList_Add(g_SceneTable[1], (Entity *)second); coop2.bars++; }
    }
    return bar;
}
