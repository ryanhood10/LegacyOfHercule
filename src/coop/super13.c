// Super Android 13: sprite 180 (tools/super13sprites.py), enemy stat 145 (s13_stat below), spawned from a
// map enemy record through enemy_spawn_hook. Built on the Android 16 boss (android16.c): an ordinary enemy
// entity (class 0x080252A4) with the stock melee boss AI set 21 as filler (walking at S13_SPEED_PCT of the
// stock speed: enemies.c scales his chase modules), plus a private vtable whose update runs a
// special-attack scheduler between the AI ticks. His specials:
//   * ki volley: alternate fists throw small orbs, one at a time, alternately at each player; each orb
//     steers after its player (partly: a turn divisor, so stepping aside works);
//   * ki fan: a spread of small orbs at one player, weakly homing;
//   * deadly bomber: arms up while a giant orb grows over his head, then it drifts after a player, slow
//     and homing, and explodes big;
//   * ki wave: a crouching power-up, then a slow beam head that homes partly and leaves a trail of ki
//     that bursts on whoever touches it;
//   * ki nova: he spins in a ki shell (when both players crowd him), a blast all round him and a ring of
//     orbs flying out;
//   * the roar: once, when he falls below a third of his HP; from then on he rages (more orbs, shorter
//     cooldowns, two bombers at once) and chains specials into combinations more often.
// Specials chain into combinations (volley -> fan, fan -> wave, wave -> volley...). He cannot be stunned
// out of a special, and a stun opens a poise window (hits only do damage), so two players cannot lock him.
// Every blast is a type-0 hit message (only players accept it). When his HP reaches 0 the record's action
// fires (the boss death class) and he explodes: light bursts out of him, explosions cover him, one big
// blast, and he is gone (an empty pose; the entity stays for the script). docs/coop-design.md
// "Super Android 13".
#include "super13.h"

typedef struct S13Desc { s8 xo, yo; u8 w, h; u32 attr; const void *blob; } S13Desc;
extern const S13Desc s13_orb_s[3], s13_orb_m[2], s13_orb_l[2], s13_trail[2];   // build/gen/super13_sprites_data.c
typedef struct A16Desc { s8 xo, yo; u8 w, h; u32 attr; const void *blob; } A16Desc;
extern const A16Desc a16_boom[4];                                              // build/gen/a16_sprites_data.c (fire)
#define DEBRIS_VEL   ((const s32 *)0x086FF8DC)

ROM_FN(void *, ProjBase_Init, (void *obj, int vram_bytes, Entity *owner), 0x08013BC1);
ROM_FN(void, ProjBase_UpdateBox, (void *obj), 0x08013BF5);
ROM_FN(int, ObjVram_Largest, (void *list), 0x08006D71);
ROM_FN(void *, Debris_Create, (void *mem, const s32 *xy, const s32 *vel), 0x0800EC09);
ROM_FN(void, Entity_SetCtrlState, (Entity *e, void *state), 0x0800763D);
ROM_FN(void, Entity_BaseUpdate, (Entity *e), 0x0800772B);
ROM_FN(void, Ai_Tick, (void *ai), 0x0801AB09);
ROM_FN(void, Entity_GetBox, (s32 *out, const Entity *e), 0x080079B9);
#define PROJ_VTABLE      0x080259C4u
#define PROJ_DESTROY     0x08027A69u
#define PROJ_MESSAGE     0x08013D69u
#define IDLE_STATE_VTABLE 0x08024BA0u
#define BOSS_CLASS       ((const u32 *)0x086FF3C4)
#define ENEMY_MESSAGE    0x0800E5B9u
#define E8(e, o)         (*(u8 *)((u8 *)(e) + (o)))
#define E16(e, o)        (*(u16 *)((u8 *)(e) + (o)))
#define E32(e, o)        (*(u32 *)((u8 *)(e) + (o)))
#define S13_MAGIC        0x53313333u   // "S133"

S13Global s13 __attribute__((section(".party.s13")));

static const EnemyStat s13_stat = {
    0, S13_HP, S13_AI_SET, S13_MELEE, S13_RANGED, S13_DEFENCE, 128, 128, BOSS_CLASS,
};
static const u32 atk_ki[2]    = { 0x0101, 0x400 };   // {kind 1 = ranged, reaction 1}, base
static const u32 atk_heavy[2] = { 0x0301, 0x400 };   // reaction 3: the heavy knock-back

enum { K_ORB = 1, K_TRAIL, K_HELD, K_BOOM };

struct S13;
typedef struct Proj {
    u32 *vtable;          // +0x00 (the ROM projectile layout up to +0x40, as android16.c)
    s32 sort;
    u8 prio;
    u8 _p0[3];
    u16 tile, size;
    const S13Desc *loaded;
    u8 owner_facing, _p1[3];
    const S13Desc *desc;  // +0x18
    Entity *owner;        // +0x1C
    s32 x, y;             // +0x20 8.8
    s32 box[4];
    s32 sx, sy;
    struct S13 *boss;     // +0x40
    u8 kind, size_class, wave, _p2;   // size_class 0 small, 1 medium, 2 large
    s16 timer, fuse;
    s32 vx, vy, speed;
    Entity *target;
    s16 turn, radius, reach, trail_t;
    s32 mult;
    const u32 *attack;
    u32 vt[24];
} Proj;

typedef struct CtrlObj { u32 *vtable; } CtrlObj;

typedef struct S13 {
    u32 vt[48];
    u32 ctrl_vt[12];
    CtrlObj ctrl;
    Entity *e;
    u8 attack, chain, depth, frame;
    u8 group, raged, down, gone;
    s16 timer, cooldown;
    s16 fired, poise;
    s16 death_t, _p;
    u16 dmg_pct, _p2;
    Entity *target;
    Proj *held[2];        // bombers growing over his head
    Proj *live[S13_LIVE];
} S13;

static Entity *real_player(int i) { return i == 0 ? coop2.p1_frame : coop.p2; }

// ---- helpers ------------------------------------------------------------------------------------
static u32 isqrt(u32 v) {
    u32 r = 0, bit = 1u << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; } else r >>= 1;
        bit >>= 2;
    }
    return r;
}

static void set_len(s32 dx, s32 dy, s32 len, s32 *ox, s32 *oy) {
    while (dx > 30000 || dx < -30000 || dy > 30000 || dy < -30000) { dx >>= 1; dy >>= 1; }
    s32 d = (s32)isqrt((u32)(dx * dx + dy * dy));
    if (d == 0) { *ox = len; *oy = 0; return; }
    *ox = dx * len / d;
    *oy = dy * len / d;
}

// Body centres (px). His frames are drawn from x (32 wide) or x - 16 (64 wide), y - 48 to y + 16, so his
// centre is 16 px right of and 16 px above his position; a player's 16 px frame centres 8 px right.
static void his_centre(const Entity *e, s32 *x, s32 *y) { *x = (e->x >> 8) + 16; *y = (e->y >> 8) - 16; }
static void pl_centre(const Entity *e, s32 *x, s32 *y) { *x = (e->x >> 8) + 8; *y = (e->y >> 8) - 12; }

static s32 dist2_to(const Entity *boss, const Entity *pl) {
    s32 bx, by, px, py;
    his_centre(boss, &bx, &by);
    pl_centre(pl, &px, &py);
    return (px - bx) * (px - bx) + (py - by) * (py - by);
}

static Entity *nearer_player(const Entity *b) {
    Entity *p1 = real_player(0), *p2 = real_player(1);
    if (!p1) return p2;
    if (!p2) return p1;
    return dist2_to(b, p1) <= dist2_to(b, p2) ? p1 : p2;
}

static Entity *other_player(Entity *t) {
    Entity *o = t == real_player(0) ? real_player(1) : real_player(0);
    return o ? o : t;
}

static int facing_to(s32 dx, s32 dy) {
    s32 ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    if (ax > ay) return dx < 0 ? 2 : 3;
    return dy < 0 ? 1 : 0;
}

static void shake(int frames, int strength) { camera_shake_safe(frames, strength); }
static void sfx(int id) { Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + id * 12, 0x40, 0); }
#define SFX_LAUNCH  0x0D
#define SFX_BOOM    0x14

// ---- hits (the bomb's message, type 0: only players react; android16.c) ----------------------------
typedef struct HitMsg {
    u8 type; u8 _p0[3];
    s32 box[4];
    u8 facing; u8 _p1[3];
    s32 mult;
    const u32 *attack;
    Entity *attacker;
    s32 out[3];
    void *source;
    s32 _p2[3];
} HitMsg;

static void blast(S13 *a, s32 x, s32 y, int r, const u32 *attack, s32 mult) {
    Entity *boss = a ? a->e : 0;
    if (!boss || boss->hp <= 0) return;
    HitMsg m;
    memset(&m, 0, sizeof m);
    m.box[0] = x - r; m.box[1] = y - r; m.box[2] = x + r; m.box[3] = y + r;
    m.facing = boss->facing;
    m.mult = a->dmg_pct ? mult * a->dmg_pct / 100 : mult;
    m.attack = attack;
    m.attacker = boss;
    m.source = boss;
    if (EntityList_Broadcast(g_SceneTable[1], 0, &m)) s13.hits++;
}

// ---- projectiles --------------------------------------------------------------------------------
static void proj_update(Proj *p);
static void proj_destroy(Proj *p, int free_it, u32 a, u32 b);

static void track(S13 *a, Proj *p) {
    p->boss = a;
    for (int i = 0; i < S13_LIVE; i++) if (!a->live[i]) { a->live[i] = p; return; }
}

static void untrack(Proj *p) {
    S13 *a = p->boss;
    if (!a) return;
    for (int i = 0; i < S13_LIVE; i++) if (a->live[i] == p) a->live[i] = 0;
    for (int i = 0; i < 2; i++) if (a->held[i] == p) a->held[i] = 0;
    p->boss = 0;
}

static void proj_destroy(Proj *p, int free_it, u32 a, u32 b) {
    untrack(p);
    ((void (*)(Proj *, int, u32, u32))PROJ_DESTROY)(p, free_it, a, b);
}

static Proj *proj_new(S13 *a, int kind, s32 x, s32 y, const S13Desc *desc, int vram) {
    if (ObjVram_Largest(g_EntityList) < vram + 0x100) { s13.skipped++; return 0; }
    Proj *p = Arena_Alloc(sizeof *p);
    if (!p) { s13.skipped++; return 0; }
    memset(p, 0, sizeof *p);
    ProjBase_Init(p, vram, a ? a->e : 0);
    u32 base = (u32)p->vt;
    const u32 *orig = (const u32 *)PROJ_VTABLE;
    for (int i = 0; i < 24; i++) p->vt[i] = (PROJ_VTABLE + orig[i]) - base;
    p->vt[0] = (u32)proj_destroy - base;
    p->vt[2] = (u32)proj_update - base;
    p->vt[9] = PROJ_MESSAGE - base;
    p->vt[10] = (u32)proj_update - base;
    p->vtable = p->vt;
    p->kind = (u8)kind;
    p->x = x << 8;
    p->y = y << 8;
    p->desc = desc;
    p->prio = 0x28;
    ProjBase_UpdateBox(p);
    if (a) track(a, p);
    EntityList_Add(g_SceneTable[1], (Entity *)p);
    return p;
}

static void proj_remove(Proj *p) {
    untrack(p);
    EntityList_Remove(g_SceneTable[1], p, 0);
}

static void spawn_boom(s32 x, s32 y) {
    if (proj_new(0, K_BOOM, x, y, (const S13Desc *)&a16_boom[0], 0x400)) s13.booms++;
}

static void debris(s32 x, s32 y) {
    if (ObjVram_Largest(g_EntityList) < 0x200) return;
    s32 xy[2] = { x, y };
    for (int i = 0; i < 4; i++) {
        void *d = Debris_Create(0, xy, DEBRIS_VEL + 2 * i);
        if (d) EntityList_Add(g_SceneTable[1], d);
    }
}

static void explode(Proj *p, int damage) {
    s32 x = p->x >> 8, y = p->y >> 8;
    S13 *a = damage ? p->boss : 0;
    spawn_boom(x, y);
    if (p->size_class == 2) {
        spawn_boom(x - 16, y + 8);
        spawn_boom(x + 16, y + 8);
        spawn_boom(x, y - 14);
        debris(x, y);
        shake(28, 100);
    } else if (p->size_class == 1) {
        spawn_boom(x - 10, y + 6);
        spawn_boom(x + 10, y + 6);
        shake(14, 70);
    } else if (p->kind == K_ORB) {
        shake(6, 40);
    }
    sfx(SFX_BOOM + Rand_Range(2));
    blast(a, x, y, p->radius, p->attack, p->mult);
    proj_remove(p);
}

static int touches_player(const Proj *p) {
    s32 x = p->x >> 8, y = p->y >> 8, r = p->reach;
    for (int i = 0; i < 2; i++) {
        Entity *pl = real_player(i);
        if (!pl) continue;
        s32 b[4];
        Entity_GetBox(b, pl);
        if (x > b[0] - r && x < b[2] + r && y > b[1] - r - 6 && y < b[3] + r) return 1;
    }
    return 0;
}

static const S13Desc *orb_frames(int size_class) {
    return size_class == 2 ? s13_orb_l : size_class == 1 ? s13_orb_m : s13_orb_s;
}

static void spawn_trail(Proj *head);

static void proj_update(Proj *p) {
    p->timer++;
    if (p->kind == K_BOOM) {
        int f = p->timer < 4 ? 0 : 1 + (p->timer - 4) / 6;
        if (f > 3) { proj_remove(p); return; }
        p->desc = (const S13Desc *)&a16_boom[f];
        ProjBase_UpdateBox(p);
        return;
    }
    int alive = p->boss && p->boss->e && p->boss->e->hp > 0;
    if (!alive) { explode(p, 0); return; }
    if (p->kind == K_HELD) {                       // growing over his head: the boss positions it
        int big = p->timer > S13_BOMB_CHARGE / 3 && p->size_class == 2;
        p->desc = big ? &s13_orb_l[(p->timer >> 2) & 1] : &s13_orb_m[(p->timer >> 2) & 1];
        ProjBase_UpdateBox(p);
        return;
    }
    if (p->kind == K_TRAIL) {
        p->desc = &s13_trail[(p->timer >> 2) & 1];
        ProjBase_UpdateBox(p);
        if (touches_player(p)) { explode(p, 1); return; }
        if (--p->fuse <= 0) proj_remove(p);
        return;
    }
    if (p->target && p->turn) {                    // partial homing: steer a fraction of the way each frame
        s32 tx, ty, dx, dy;
        pl_centre(p->target, &tx, &ty);
        set_len((tx << 8) - p->x, (ty << 8) - p->y, p->speed, &dx, &dy);
        p->vx += (dx - p->vx) / p->turn;
        p->vy += (dy - p->vy) / p->turn;
        set_len(p->vx, p->vy, p->speed, &p->vx, &p->vy);
    }
    p->x += p->vx;
    p->y += p->vy;
    const S13Desc *f = orb_frames(p->size_class);
    p->desc = &f[(p->timer >> 2) % (p->size_class ? 2 : 3)];
    ProjBase_UpdateBox(p);
    if (p->wave && ++p->trail_t >= S13_TRAIL_GAP) { p->trail_t = 0; spawn_trail(p); }
    if (--p->fuse <= 0 || touches_player(p)) explode(p, 1);
}

static void spawn_trail(Proj *head) {
    int n = 0;
    S13 *a = head->boss;
    for (int i = 0; i < S13_LIVE; i++) if (a->live[i] && a->live[i]->kind == K_TRAIL) n++;
    if (n >= 8) return;
    Proj *t = proj_new(a, K_TRAIL, head->x >> 8, head->y >> 8, &s13_trail[0], 0x100);
    if (!t) return;
    t->fuse = S13_TRAIL_LIFE;
    t->radius = S13_TRAIL_RADIUS;
    t->reach = 2;
    t->mult = S13_TRAIL_MULT;
    t->attack = atk_ki;
}

static void launch_point(const Entity *e, s32 *x, s32 *y) {
    static const s8 off[4][2] = { { 0, 10 }, { 0, -16 }, { -24, -4 }, { 24, -4 } };
    his_centre(e, x, y);
    *x += off[e->facing & 3][0];
    *y += off[e->facing & 3][1];
}

static Proj *fire_orb(S13 *a, int size_class, s32 x, s32 y, Entity *target, s32 aim_x, s32 aim_y,
                      s32 speed, int turn, int fuse, int radius, s32 mult, const u32 *attack) {
    static const u16 vram[3] = { 0x100, 0x400, 0x1000 };
    const S13Desc *f = orb_frames(size_class);
    Proj *p = proj_new(a, K_ORB, x, y, f, vram[size_class]);
    if (!p && size_class == 2) { size_class = 1; f = s13_orb_m; p = proj_new(a, K_ORB, x, y, f, 0x400); }
    if (!p) return 0;
    p->size_class = (u8)size_class;
    set_len(aim_x, aim_y, speed, &p->vx, &p->vy);
    p->speed = speed;
    p->turn = (s16)turn;
    p->fuse = (s16)fuse;
    p->target = target;
    p->radius = (s16)radius;
    p->reach = (s16)(size_class == 2 ? 18 : size_class == 1 ? 9 : 3);
    p->mult = mult;
    p->attack = attack;
    s13.fired++;
    sfx(SFX_LAUNCH + Rand_Range(2));
    return p;
}

static Proj *fire_at(S13 *a, int size_class, Entity *t, s32 speed, int turn, int fuse, int radius, s32 mult, const u32 *atk) {
    s32 x, y, tx, ty;
    launch_point(a->e, &x, &y);
    pl_centre(t, &tx, &ty);
    return fire_orb(a, size_class, x, y, t, tx - x, ty - y, speed, turn, fuse, radius, mult, atk);
}

// ---- the boss -----------------------------------------------------------------------------------
static S13 *state_of(Entity *e) {
    u8 *blk = (u8 *)e + ENEMY_SIZE + sizeof(EnemyStat);
    const EnemyRecord *r = (const EnemyRecord *)blk;
    u32 rs = (0x1C + 4 * (u32)r->behaviour_count + 3) & ~3u;
    return (S13 *)(blk + rs);
}

int is_s13(const Entity *e) { return e && s13.magic == S13_MAGIC && s13.boss == e; }

static void set_frames(S13 *a, int group, int frame) {
    Entity *e = a->e;
    a->group = (u8)group;
    a->frame = (u8)frame;
    E32(e, 0x4C) = (u32)e->sprite_record + 0x4C + 0x10 * (u32)group;
    E16(e, 0x14) = (u16)frame;
}

static void ctrl_enter(CtrlObj *s, Entity *e) { (void)s; E16(e, 0x12) = 0; E8(e, 8) = 0x28; }
static void ctrl_nop(CtrlObj *s, Entity *e) { (void)s; (void)e; }

static void face(S13 *a, const Entity *t) {
    s32 bx, by, px, py;
    his_centre(a->e, &bx, &by);
    pl_centre(t, &px, &py);
    a->e->facing = (u8)facing_to(px - bx, py - by);
}

static void begin(S13 *a, int attack) {
    a->attack = (u8)attack;
    a->timer = 0;
    a->fired = 0;
    face(a, a->target);
    Entity_SetCtrlState(a->e, &a->ctrl);
    static const u8 first_group[7] = { 0, S13_G_KI, S13_G_KI, S13_G_BOMB, S13_G_POWER, S13_G_NOVA, S13_G_ROAR };
    set_frames(a, first_group[attack], 0);
}

// Health left over the fight, percent of the nominal maximum.
static int fight_pct(const S13 *a) {
    s32 max = ((const EnemyStat *)a->e->behaviour)->max_hp;
    return max > 0 ? a->e->hp * 100 / max : 0;
}

static void finish(S13 *a) {
    Entity *e = a->e;
    int next = a->chain;
    a->attack = S13_ATK_NONE;
    a->chain = 0;
    if (next && a->depth < 2 && a->target) {      // a combination: the next special at once
        a->depth++;
        s13.combos++;
        a->target = Rand_Range(100) < 50 ? a->target : other_player(a->target);
        begin(a, next);
        return;
    }
    a->depth = 0;
    a->cooldown = (s16)(S13_COOLDOWN_MIN + Rand_Range(S13_COOLDOWN_MAX - S13_COOLDOWN_MIN));
    if (a->raged) a->cooldown = (s16)(a->cooldown * S13_RAGE_PCT / 100);
    if (e->ctrl_state == &a->ctrl) Entity_SetAnimation(e, 0);
}

static int crowded(const S13 *a) {
    Entity *p1 = real_player(0), *p2 = real_player(1);
    s32 r2 = S13_CROWD * S13_CROWD;
    if (!p1 || dist2_to(a->e, p1) > r2) return 0;
    return !p2 || dist2_to(a->e, p2) <= r2;
}

static int choose(S13 *a) {
    if (s13.force) return s13.force;
    int pct = fight_pct(a);
    if (!a->raged && pct < S13_RAGE_AT) return S13_ATK_ROAR;
    int w[5];   // volley, fan, bomber, wave, nova
    if (pct > 66)      { w[0] = 35; w[1] = 20; w[2] = 15; w[3] = 20; w[4] = 10; }
    else if (pct > 33) { w[0] = 25; w[1] = 20; w[2] = 20; w[3] = 25; w[4] = 10; }
    else               { w[0] = 20; w[1] = 15; w[2] = 25; w[3] = 30; w[4] = 10; }
    if (crowded(a)) w[4] += 60;
    int sum = w[0] + w[1] + w[2] + w[3] + w[4];
    int r = Rand_Range(sum);
    for (int i = 0; i < 5; i++) { if (r < w[i]) return S13_ATK_VOLLEY + i; r -= w[i]; }
    return S13_ATK_VOLLEY;
}

// What follows a special in a combination (0 = nothing): more often once he rages.
static int chain_after(S13 *a, int attack) {
    int r = Rand_Range(100), rage = a->raged;
    switch (attack) {
    case S13_ATK_VOLLEY: return r < (rage ? 60 : 40) ? S13_ATK_FAN : (rage && r < 80 ? S13_ATK_BOMB : 0);
    case S13_ATK_FAN:    return r < (rage ? 45 : 20) ? S13_ATK_WAVE : 0;
    case S13_ATK_WAVE:   return r < (rage ? 35 : 15) ? S13_ATK_VOLLEY : 0;
    case S13_ATK_ROAR:   return S13_ATK_BOMB;
    default:             return 0;
    }
}

static void place_held(S13 *a) {
    s32 x, y;
    his_centre(a->e, &x, &y);
    for (int i = 0; i < 2; i++) {
        Proj *p = a->held[i];
        if (!p) continue;
        p->x = (x + (i ? 26 : (a->held[1] ? -26 : 0))) << 8;
        p->y = (y - 46) << 8;
    }
}

static void step_attack(S13 *a) {
    Entity *e = a->e;
    int rage = a->raged;
    a->timer++;
    switch (a->attack) {
    case S13_ATK_VOLLEY: {      // wind-up 12, then a fist every S13_VOLLEY_GAP frames, alternating players
        int n = rage ? S13_VOLLEY_COUNT + 2 : S13_VOLLEY_COUNT;
        int t = a->timer - 12;
        if (t < 0) { set_frames(a, S13_G_KI, 0); break; }
        if (t % S13_VOLLEY_GAP == 0 && a->fired < n) {
            Entity *pl = (a->fired & 1) ? other_player(a->target) : a->target;
            face(a, pl);
            set_frames(a, S13_G_KI, 1 + (a->fired & 1));
            fire_at(a, 0, pl, S13_VOLLEY_SPEED, rage ? S13_VOLLEY_TURN * 3 / 4 : S13_VOLLEY_TURN, S13_VOLLEY_FUSE,
                    S13_VOLLEY_RADIUS, S13_VOLLEY_MULT, atk_ki);
            a->fired++;
        } else if (t % S13_VOLLEY_GAP == S13_VOLLEY_GAP / 2) {
            set_frames(a, S13_G_KI, 0);
        }
        if (a->fired >= n && t >= n * S13_VOLLEY_GAP + 10) { a->chain = (u8)chain_after(a, a->attack); finish(a); }
        break;
    }
    case S13_ATK_FAN: {         // wind-up 22, then the fan, two frames apart
        int n = rage ? S13_FAN_COUNT + 2 : S13_FAN_COUNT;
        set_frames(a, S13_G_KI, a->timer < 22 ? 0 : 1);
        if (a->timer >= 22 && ((a->timer - 22) & 1) == 0 && a->fired < n) {
            Entity *t = a->target;
            s32 x, y, tx, ty, ax, ay;
            launch_point(e, &x, &y);
            pl_centre(t, &tx, &ty);
            set_len(tx - x, ty - y, 0x100, &ax, &ay);
            s32 k = (2 * a->fired - (n - 1)) * S13_FAN_SPREAD / (n - 1);
            fire_orb(a, 0, x, y, t, ax - (ay * k >> 8), ay + (ax * k >> 8), S13_FAN_SPEED, S13_FAN_TURN,
                     S13_FAN_FUSE, S13_FAN_RADIUS, S13_FAN_MULT, atk_ki);
            a->fired++;
        }
        if (a->fired >= n && a->timer >= 22 + 2 * n + 18) { a->chain = (u8)chain_after(a, a->attack); finish(a); }
        break;
    }
    case S13_ATK_BOMB: {        // hunch 10, arms up while the ball grows, throw (one per player when raging)
        int hold = 10 + S13_BOMB_CHARGE;
        if (a->timer < 10) { set_frames(a, S13_G_BOMB, 0); break; }
        if (a->timer == 10) {
            s32 x, y;
            his_centre(e, &x, &y);
            int two = rage && real_player(1);
            for (int i = 0; i < (two ? 2 : 1); i++) {
                int big = ObjVram_Largest(g_EntityList) >= 0x1100;
                Proj *p = proj_new(a, K_HELD, x, y - 46, &s13_orb_m[0], big ? 0x1000 : 0x400);
                if (!p) break;
                p->size_class = (u8)(big ? 2 : 1);
                a->held[i] = p;
            }
            shake(S13_BOMB_CHARGE, 30);
        }
        if (a->timer < hold) { set_frames(a, S13_G_BOMB, 1); place_held(a); break; }
        if (a->timer == hold) {
            set_frames(a, S13_G_BOMB, 2);
            Entity *far = a->target;                       // the first ball goes after the farther player
            Entity *o = other_player(far);
            if (dist2_to(e, o) > dist2_to(e, far)) far = o;
            for (int i = 0; i < 2; i++) {
                Proj *p = a->held[i];
                if (!p) continue;
                Entity *t = i ? other_player(far) : far;
                s32 tx, ty;
                pl_centre(t, &tx, &ty);
                p->kind = K_ORB;
                set_len((tx << 8) - p->x, (ty << 8) - p->y, S13_BOMB_SPEED, &p->vx, &p->vy);
                p->speed = S13_BOMB_SPEED;
                p->turn = S13_BOMB_TURN;
                p->fuse = S13_BOMB_FUSE;
                p->target = t;
                p->radius = p->size_class == 2 ? S13_BOMB_RADIUS : S13_BOMB_RADIUS * 2 / 3;
                p->reach = p->size_class == 2 ? 18 : 9;
                p->mult = p->size_class == 2 ? S13_BOMB_MULT : S13_BOMB_MULT * 3 / 4;
                p->attack = atk_heavy;
                a->held[i] = 0;
                s13.fired++;
            }
            sfx(SFX_LAUNCH);
        }
        if (a->timer >= hold + 24) { a->chain = (u8)chain_after(a, a->attack); finish(a); }
        break;
    }
    case S13_ATK_WAVE: {        // the crouching power-up (6 frames), then the beam, held for 40 frames
        int per = S13_WAVE_CHARGE / 6;
        int f = a->timer / per;
        set_frames(a, S13_G_POWER, f > 5 ? 5 : f);
        if (a->timer == 8) shake(S13_WAVE_CHARGE - 8, 40);
        if (a->timer == S13_WAVE_CHARGE) {
            face(a, a->target);
            set_frames(a, S13_G_KI, 1);
            Proj *p = fire_at(a, 1, a->target, S13_WAVE_SPEED, rage ? S13_WAVE_TURN * 2 / 3 : S13_WAVE_TURN,
                              S13_WAVE_FUSE, S13_WAVE_RADIUS, S13_WAVE_MULT, atk_heavy);
            if (p) p->wave = 1;
        }
        if (a->timer > S13_WAVE_CHARGE) set_frames(a, S13_G_KI, 1);
        if (a->timer >= S13_WAVE_CHARGE + 40) { a->chain = (u8)chain_after(a, a->attack); finish(a); }
        break;
    }
    case S13_ATK_NOVA: {        // the spinning shell: 8 frames x 6; the blast at frame 4
        int f = a->timer / 6;
        set_frames(a, S13_G_NOVA, f > 7 ? 7 : f);
        if (a->timer == 24) {
            s32 x, y;
            his_centre(e, &x, &y);
            spawn_boom(x - 18, y + 4);
            spawn_boom(x + 18, y + 4);
            shake(16, 80);
            sfx(SFX_BOOM);
            blast(a, x, y, S13_NOVA_RADIUS, atk_heavy, S13_NOVA_MULT);
            static const s8 dir[8][2] = { { 16, 0 }, { 11, 11 }, { 0, 16 }, { -11, 11 }, { -16, 0 }, { -11, -11 }, { 0, -16 }, { 11, -11 } };
            for (int i = 0; i < S13_NOVA_RING; i++)
                fire_orb(a, 0, x + dir[i][0], y + dir[i][1], 0, dir[i][0], dir[i][1], S13_NOVA_SPEED, 0,
                         S13_NOVA_FUSE, S13_FAN_RADIUS, S13_ORB_MULT, atk_ki);
        }
        if (a->timer >= 48) finish(a);
        break;
    }
    case S13_ATK_ROAR: {        // once: the roar (6 frames x 8), then he rages and throws bombers
        int f = a->timer / 8;
        set_frames(a, S13_G_ROAR, f > 5 ? 5 : f);
        if (a->timer == 1) { shake(48, 90); a->raged = 1; }
        if ((a->timer & 15) == 0) sfx(SFX_BOOM);
        if (a->timer >= 48) { a->chain = (u8)chain_after(a, a->attack); a->depth = 0; finish(a); }
        break;
    }
    default:
        finish(a);
    }
}

// The death: pose group 12 (light bursting out of him) while explosions cover him, then a big blast and
// the empty pose.
static void dying(S13 *a) {
    Entity *e = a->e;
    s32 x, y;
    his_centre(e, &x, &y);
    a->death_t++;
    if (a->gone) return;
    int t = a->death_t;
    if (t < S13_DEATH_FRAMES) {
        set_frames(a, S13_G_DEATH, t * 8 / S13_DEATH_FRAMES);
        if ((t % 7) == 3) {
            spawn_boom(x - 20 + Rand_Range(40), y - 26 + Rand_Range(52));
            sfx(SFX_BOOM + Rand_Range(2));
        }
        if ((t % 24) == 1) shake(24, 50 + t / 2);
        return;
    }
    // the big blast
    spawn_boom(x, y);
    spawn_boom(x - 22, y - 12);
    spawn_boom(x + 22, y - 12);
    spawn_boom(x - 16, y + 16);
    spawn_boom(x + 16, y + 16);
    debris(x, y);
    shake(40, 100);
    sfx(SFX_BOOM);
    set_frames(a, S13_G_BLANK, 0);
    a->gone = 1;
    s13.state = 3;
    s13.gone_frame = g_FrameCounter;
}

static void s13_update(Entity *e) {
    S13 *a = state_of(e);
    s13.boss = e;
    if (e->hp <= 0) {              // beaten: the explosion; the record's action runs the story
        if (!a->down) {
            a->down = 1;
            a->attack = S13_ATK_NONE;
            for (int i = 0; i < 2; i++) if (a->held[i]) { a->held[i]->kind = K_ORB; a->held[i] = 0; }   // fizzle
            s13.state = 2;
            s13.dead_frame = g_FrameCounter;
            Entity_SetCtrlState(e, &a->ctrl);
        }
        if (e->ctrl_state != &a->ctrl) Entity_SetCtrlState(e, &a->ctrl);   // a script animation stays out
        Entity_BaseUpdate(e);
        dying(a);
        return;
    }
    Entity_BaseUpdate(e);
    int vram = ObjVram_Largest(g_EntityList);
    if (vram < s13.vram_min) s13.vram_min = (u16)vram;
    if (a->poise > 0) a->poise--;
    if (a->attack) {
        if (e->ctrl_state != &a->ctrl) { a->attack = S13_ATK_NONE; a->chain = 0; a->cooldown = 40; }
        else { step_attack(a); return; }
    }
    Ai_Tick((u8 *)e + 0x19C);
    if (a->cooldown > 0) { a->cooldown--; return; }
    void *st = e->ctrl_state;
    if (st != ENEMY_STATES[0] && st != ENEMY_STATES[1]) return;   // only from idle or walking
    Entity *t = nearer_player(e);
    if (!t || dist2_to(e, t) > S13_RANGE * S13_RANGE) return;
    a->target = Rand_Range(100) < 65 ? t : other_player(t);        // either player, the nearer more often
    s13.specials++;
    begin(a, choose(a));
}

// Messages: as android16.c. A player's hit during poise or a special only does damage.
void damage_calc_hook(Entity *defender, Hit *hit);   // enemies.c
static int s13_message(Entity *e, int msg, u8 *m) {
    S13 *a = state_of(e);
    if (msg == 0 && (m[0] == 1 || m[0] == 2)) {
        if (e->hp <= 0) return 0;
        s13.msgs++;
        if (a->poise > 0 || a->attack) {
            if (!E32(e, 0x168)) return 0;
            s32 b[4];
            const s32 *mb = (const s32 *)(m + 4);
            Entity_GetBox(b, e);
            if (!(mb[0] < b[2] && b[0] < mb[2] && mb[1] < b[3] && b[1] < mb[3])) return 0;
            damage_calc_hook(e, (Hit *)(m + 0x14));
            Enemy_OnHit(e, m + 0x24);
            s13.landed++;
            return 1;
        }
        int r = ((int (*)(Entity *, int, u8 *))ENEMY_MESSAGE)(e, msg, m);
        if (r) { a->poise = S13_POISE; s13.landed++; }
        return r;
    }
    return ((int (*)(Entity *, int, u8 *))ENEMY_MESSAGE)(e, msg, m);
}

// Test autopilot helper: a swing, an orb, a trail or a bomber about to catch pl? Escape vector (px).
int s13_threat(const Entity *pl, s32 *ex, s32 *ey) {
    Entity *e = s13.boss;
    if (!is_s13(e) || e->hp <= 0) return 0;
    S13 *a = state_of(e);
    s32 px, py, bx, by;
    pl_centre(pl, &px, &py);
    his_centre(e, &bx, &by);
    s32 best = 0x7FFFFFFF;
    void *st = e->ctrl_state;
    if (st == ENEMY_STATES[6] || st == ENEMY_STATES[7] || st == ENEMY_STATES[8] || st == ENEMY_STATES[10] ||
        (a->attack == S13_ATK_NOVA && a->timer < 26)) {
        s32 dx = px - bx, dy = py - by;
        if (dx > -50 && dx < 50 && dy > -40 && dy < 40) { *ex = dx ? dx : 1; *ey = dy; return 1; }
    }
    for (int i = 0; i < S13_LIVE; i++) {   // an orb closing in (or a trail right here)
        Proj *p = a->live[i];
        if (!p || p->kind == K_HELD || p->kind == K_BOOM) continue;
        s32 qx = p->x >> 8, qy = p->y >> 8;
        s32 dx = px - qx, dy = py - qy, d2 = dx * dx + dy * dy;
        s32 reach = p->kind == K_TRAIL ? p->radius + 8 : p->radius + p->reach + 14;
        if (p->kind == K_ORB && (dx * p->vx + dy * p->vy) < 0) continue;   // moving away
        if (d2 < reach * reach && d2 < best) { best = d2; *ex = dx ? dx : 1; *ey = dy; }
    }
    return best != 0x7FFFFFFF;
}

static void s13_destroy(Entity *e, int free_it, u32 x, u32 y) {
    S13 *a = state_of(e);
    for (int i = 0; i < S13_LIVE; i++) if (a->live[i]) { a->live[i]->boss = 0; a->live[i] = 0; }
    for (int i = 0; i < 2; i++) a->held[i] = 0;
    if (s13.boss == e) { s13.boss = 0; if (s13.state < 3) s13.state = 0; }
    e->vtable = (u32 *)ENEMY_VTABLE;
    Enemy_Destroy(e, free_it, x, y);
}

static void s13_globals(void) {
    if (s13.magic != S13_MAGIC) {
        memset(&s13, 0, sizeof s13);
        s13.magic = S13_MAGIC;
    }
}

Entity *s13_spawn(const EnemyRecord *rec) {
    s13_globals();
    u32 rs = (0x1C + 4 * (u32)rec->behaviour_count + 3) & ~3u;
    u8 *block = Arena_Alloc(ENEMY_SIZE + sizeof(EnemyStat) + rs + sizeof(S13));
    if (!block) return 0;
    EnemyStat *stat = (EnemyStat *)(block + ENEMY_SIZE);
    EnemyRecord *r = (EnemyRecord *)(block + ENEMY_SIZE + sizeof(EnemyStat));
    S13 *a = (S13 *)((u8 *)r + rs);
    memset(a, 0, sizeof *a);
    *stat = s13_stat;
    if (s13.hp_pct) stat->max_hp = stat->max_hp * s13.hp_pct / 100;
    if (!p2_present()) stat->max_hp = stat->max_hp * S13_SOLO_HP_PCT / 100;
    a->dmg_pct = s13.dmg_pct;
    memcpy(r, rec, 0x1C + 4 * (u32)rec->behaviour_count);
    Entity *e = Enemy_Create(block, r, stat);
    if (!e) { Arena_Free(block); return 0; }
    a->e = e;
    u32 base = (u32)a->vt;
    const u32 *orig = (const u32 *)ENEMY_VTABLE;
    for (int i = 0; i < 48; i++) a->vt[i] = (ENEMY_VTABLE + orig[i]) - base;
    a->vt[0] = (u32)s13_destroy - base;
    a->vt[2] = (u32)s13_update - base;
    a->vt[9] = (u32)s13_message - base;
    base = (u32)a->ctrl_vt;
    orig = (const u32 *)IDLE_STATE_VTABLE;
    for (int i = 0; i < 12; i++) a->ctrl_vt[i] = (IDLE_STATE_VTABLE + orig[i]) - base;
    a->ctrl_vt[0] = (u32)ctrl_enter - base;
    a->ctrl_vt[1] = (u32)ctrl_nop - base;
    a->ctrl_vt[2] = (u32)ctrl_nop - base;
    a->ctrl_vt[3] = (u32)ctrl_nop - base;
    a->ctrl.vtable = a->ctrl_vt;
    a->cooldown = S13_FIRST_DELAY;
    e->vtable = a->vt;
    s13.boss = e;
    s13.state = 1;
    s13.spawn_frame = g_FrameCounter;
    s13.dead_frame = s13.gone_frame = 0;
    s13.fired = s13.booms = s13.hits = s13.skipped = s13.specials = s13.combos = 0;
    s13.msgs = s13.landed = 0;
    s13.vram_min = 0xFFFF;
    return e;
}

// Test placement (test command 17, CoopCall 17 op 2): an enemy record for stat 145 / sprite 180 72 px to
// the right of player 1, spawned the way TriggerMapEvent spawns a boss record, with a boss bar.
void s13_test_spawn(void) {
    static const u32 face_left[2] = { 0x0800CB0Fu, 0x00027D00u };   // NpcBehavior_FaceAndWait
    static const u32 no_action[2] = { ENEMY_ACTION_NONE, 0 };
    Entity *p1 = g_PlayerObject;
    s13_globals();
    if (!p1 || (s13.boss && s13.boss->hp > 0)) return;
    struct { EnemyRecord r; const void *beh[1]; } rec;
    memset(&rec, 0, sizeof rec);
    rec.r.handler = (void *)0x0800E77Fu;
    rec.r.stat_index = S13_STAT;
    rec.r.x = (s16)((p1->x >> 8) + 72);
    rec.r.y = (s16)(p1->y >> 8);
    rec.r.sprite_id = S13_SPRITE;
    rec.r.action = (void *)no_action;
    rec.r.behaviour_count = 1;
    rec.beh[0] = face_left;
    extern Entity *enemy_spawn_hook(const EnemyRecord *rec);
    Entity *e = enemy_spawn_hook(&rec.r);
    if (!e) return;
    EntityList_Add(g_SceneTable[1], e);
    extern BossBar *bossbar_create_hook(void *mem, Entity *e, int lo, int hi);
    BossBar *bar = bossbar_create_hook(0, e, 0, 100);
    if (bar) EntityList_Add(g_SceneTable[1], (Entity *)bar);
}

// Test command 18: arg low byte = forced special (S13_ATK_*, 0 = his own choice; a forced one starts as soon as
// he is free); arg bit 8 = set his HP to
// arg2 (rage and death tests); arg bit 9 = toggle the test autopilot (A16_TEST builds' coop2.a16_bot).
void s13_test_drive(u32 arg, u32 arg2) {
    s13_globals();
    s13.force = (u8)(arg & 0xFF) <= S13_ATK_ROAR ? (u8)(arg & 0xFF) : 0;
    Entity *e = s13.boss;
    if (s13.force && is_s13(e) && e->hp > 0) state_of(e)->cooldown = 0;   // the forced special starts at once
    if ((arg & 0x100) && is_s13(e) && e->hp > 0) e->hp = (s32)arg2 > 0 ? (s32)arg2 : 1;
    if (arg & 0x200) coop2.a16_bot ^= 1;
}

// CoopCall 17 (op, a, b): op 0 = tune the next spawn (a = HP percent, b = blast damage percent, 0 = 100);
// op 1 -> state (0 none, 1 fighting, 2 exploding, 3 gone: a script waits for 3 before it goes on);
// op 2 = spawn him beside player 1 with a boss bar (scenes without a map record).
s32 s13_call(int op, int a, int b) {
    s13_globals();
    if (op == 0) { s13.hp_pct = (u8)a; s13.dmg_pct = (u8)b; return 0; }
    if (op == 1) return s13.state;
    if (op == 2) { s13_test_spawn(); return 0; }
    return 0;
}
