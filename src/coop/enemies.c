// Co-op difficulty: while a second pad is present, map enemies spawn with scaled stats and a twin
// beside them; twins target the player the original is not after. Bosses (no EXP, >= 1000 HP) keep
// their stats and get a twin of the partner model with its own boss bar. Single-player is untouched:
// without a player-2 source every hook falls through to the original behaviour.
//
// Hooked entries (tools/patch_rom.py): the map-script enemy handler 0x0800E77E, the damage
// calculation 0x0800F910, the two chase-module constructors (0x0801B332, 0x0801B15E) that carry the
// walk speed, and BossHealthBar_Create 0x080104C0.
#include "coop.h"

CoopExtra coop2 __attribute__((section(".coop2"), used));

void *memcpy(void *dst, const void *src, unsigned n);   // util.c
#define COLLISION_MAP ((const void *)g_Ctx[4])

int is_coop_enemy(const Entity *e) { return e && e->vtable == coop2.enemy_vtable; }
static int is_enemy(const Entity *e) { return e && (e->vtable == (u32 *)ENEMY_VTABLE || e->vtable == coop2.enemy_vtable); }

// Bosses: no EXP but real HP (scripted targets such as rocks have no EXP and tiny HP; they are left alone).
static int is_boss(const EnemyStat *st) { return st->exp == 0 && st->max_hp >= 1000; }
static int twinnable(const EnemyStat *st) { return st->exp > 0 || is_boss(st); }

// Partner model for boss twins: stat index -> sprite id (0 = same model). Filled in as the fights are
// identified in mGBA; see docs/coop-design.md "Boss roster".
static int partner_sprite(int stat_index) {
    (void)stat_index;
    return 0;
}

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

static void twin_bar_update(BossBar *bar);

void enemies_init(void) {
    u32 base = (u32)coop2.enemy_vtable;
    const u32 *orig = (const u32 *)ENEMY_VTABLE;
    for (int i = 0; i < 48; i++) coop2.enemy_vtable[i] = (ENEMY_VTABLE + orig[i]) - base;
    coop2.enemy_vtable[0] = (u32)coop_enemy_destroy - base;

    base = (u32)coop2.bar_vtable;
    orig = (const u32 *)BOSSBAR_VTABLE;
    for (int i = 0; i < 24; i++) coop2.bar_vtable[i] = (BOSSBAR_VTABLE + orig[i]) - base;
    coop2.bar_vtable[2] = (u32)twin_bar_update - base;

    if (coop2.magic != COOP2_MAGIC) {
        coop2.magic = COOP2_MAGIC;
        for (int i = 0; i < TWIN_MAX; i++) coop2.twins[i].orig = coop2.twins[i].twin = 0;
        coop2.spawned = coop2.scaled = coop2.bars = 0;
        coop2.off = 0;
    }
}

// ---- spawning -----------------------------------------------------------------------------------
// Replacement for the map loader's enemy record handler (0x0800E77E): Enemy_Create(0, rec, stat).
// In co-op the entity block also carries a private stat copy (scaled HP) and a record copy with the
// shifted spawn point (the constructor registers the collision box at the record's position, so the
// twin must be created where it stands) and, for boss twins, the partner sprite.
static u32 record_size(const EnemyRecord *rec) { return 0x1C + 4 * (u32)rec->behaviour_count; }

static Entity *create_coop_enemy(const EnemyRecord *rec, const EnemyStat *st, int scale, int dx, int sprite) {
    u32 rsize = (record_size(rec) + 3) & ~3u;
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
    if (sprite) r->sprite_id = sprite;
    Entity *e = Enemy_Create(block, r, stat);
    if (!e) { Arena_Free(block); return 0; }
    e->vtable = coop2.enemy_vtable;
    return e;
}

Entity *enemy_spawn_hook(const EnemyRecord *rec) {
    coop_init_once();
    const EnemyStat *st = &ENEMY_STATS[rec->stat_index];
    if (!p2_present() || !twinnable(st)) return Enemy_Create(0, rec, st);

    int boss = is_boss(st);
    Entity *orig = create_coop_enemy(rec, st, !boss, 0, 0);
    if (!orig) return 0;

    // Twin TWIN_OFFSET px to the right when that spot is walkable, else to the left, else on top.
    s32 px[2] = { rec->x + TWIN_OFFSET, rec->y };
    int dx = TWIN_OFFSET;
    if (!Collision_Check(COLLISION_MAP, px, (u8 *)orig + 0x154)) {
        px[0] = rec->x - TWIN_OFFSET;
        dx = (rec->x >= TWIN_OFFSET + 8 && Collision_Check(COLLISION_MAP, px, (u8 *)orig + 0x154)) ? -TWIN_OFFSET : 0;
    }
    Entity *twin = (coop2.off & COOP_OFF_TWINS) ? 0
                 : create_coop_enemy(rec, st, !boss, dx, boss ? partner_sprite(rec->stat_index) : 0);
    if (twin) {
        EntityList_Add(g_SceneTable[1], twin);   // the loader adds the returned original itself
        register_pair(orig, twin);
        coop2.spawned++;
    }
    return orig;
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
    if (is_coop_enemy(e) && !(coop2.off & COOP_OFF_SPEED)) m->speed = m->speed * COOP_SPEED_NUM / COOP_SPEED_DEN;
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
#define BAR_Y_TWIN 20
#define BAR_LIST (*(void **)0x086D60F0)   // -> g_EntityList (the bar's own ObjVram/list cell)

static BossBar *bar_create(void *mem, Entity *e, int lo, int hi, u32 *vtable) {
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
    return bar;
}

// Original slide: y += step until 8 (or removed below -8). The twin's bar settles 12 px lower.
static void twin_bar_update(BossBar *bar) {
    int y = bar->y + bar->step;
    bar->y = (s8)y;
    if (y < -8) EntityList_Remove(BAR_LIST, bar, -8);
    else if (y > BAR_Y_TWIN) { bar->y = BAR_Y_TWIN; bar->step = 0; }
}

BossBar *bossbar_create_hook(void *mem, Entity *e, int lo, int hi) {
    BossBar *bar = bar_create(mem, e, lo, hi, (u32 *)BOSSBAR_VTABLE);
    TwinPair *pr = coop.p2 ? pair_of(e) : 0;
    if (bar && pr && pr->twin && pr->twin != e) {
        BossBar *second = bar_create(0, pr->twin, lo, hi, coop2.bar_vtable);
        if (second) { EntityList_Add(g_SceneTable[1], (Entity *)second); coop2.bars++; }
    }
    return bar;
}
