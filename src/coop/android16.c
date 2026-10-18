// Android 16 boss: sprite 160 (tools/a16sprites.py), enemy stat 109 (a16_stat below), spawned from a
// map enemy record through enemy_spawn_hook. He is an ordinary enemy entity (class 0x080252A4) with the
// stock boss AI set 21 as his melee filler, plus a private vtable whose update runs a special-attack
// scheduler between the AI ticks:
//   * heat-seeking rocket: a missile that locks onto the nearer player, steers after them and
//     detonates on contact or when its three-second fuse runs out;
//   * rocket barrage: a fan of fast missiles with weak homing and a short fuse;
//   * area explosives: bombs (the ROM's fused bomb) dropped around both players and across the view;
//   * rocket punch: his forearm detaches (sprite group 13) and flies straight along his facing.
// Every blast is a type-0 hit message (the kind only players accept, enemy message 0 takes types 1
// and 2), so nothing he fires can hurt him. Projectiles are objects of the ROM's projectile class
// (base init 0x08013BC0: OBJ tiles, cull, OAM and DMA slots) with their own vtable copy inside the
// object, so a projectile never points at memory the boss frees. docs/coop-design.md "Android 16 boss".
#include "android16.h"

typedef struct A16Desc { s8 xo, yo; u8 w, h; u32 attr; const void *blob; } A16Desc;
extern const A16Desc a16_missile[16], a16_fist[8], a16_boom[4];   // build/gen/a16_sprites_data.c
#define BOMB_FRAMES  ((const A16Desc *)0x086FFBFC)                  // the ROM's fused bomb, 3 frames
#define DEBRIS_VEL   ((const s32 *)0x086FF8DC)                       // 4 {vx, vy} pairs (FUN_0801CD32)

ROM_FN(void *, ProjBase_Init, (void *obj, int vram_bytes, Entity *owner), 0x08013BC1);
ROM_FN(void, ProjBase_UpdateBox, (void *obj), 0x08013BF5);
ROM_FN(int, ObjVram_Largest, (void *list), 0x08006D71);              // largest free OBJ VRAM block (bytes)
ROM_FN(void *, Debris_Create, (void *mem, const s32 *xy, const s32 *vel), 0x0800EC09);
ROM_FN(void, Entity_SetCtrlState, (Entity *e, void *state), 0x0800763D);
ROM_FN(void, Entity_BaseUpdate, (Entity *e), 0x0800772B);           // aux + control state update
ROM_FN(void, Ai_Tick, (void *ai), 0x0801AB09);                       // entity +0x19C
ROM_FN(void, Entity_GetBox, (s32 *out, const Entity *e), 0x080079B9);
#define PROJ_VTABLE      0x080259C4u     // the NPC ki blast's vtable (FUN_08011BC4): copied, 3 slots replaced
#define PROJ_DESTROY     0x08027A69u     // its slot 0: frees the OBJ tiles, then the object
#define PROJ_MESSAGE     0x08013D69u     // the base projectile message handler (9, 10, 15)
#define ENEMY_STATES     ((void *const *)0x0840DB00)   // enemy control states by index (0 idle, 1 walk)
#define IDLE_STATE_VTABLE 0x08024BA0u   // vtable of control state 0 (idle): enter, update, 2, exit, 4.. common
#define BOSS_CLASS       ((const u32 *)0x086FF3C4)
#define SHAKE_HEAD       (*(void **)0x03000FF4)        // camera controller chain head (Camera_Shake links in)
#define E8(e, o)         (*(u8 *)((u8 *)(e) + (o)))
#define E16(e, o)        (*(u16 *)((u8 *)(e) + (o)))
#define E32(e, o)        (*(u32 *)((u8 *)(e) + (o)))

// Stat row 109 (EnemyStat layout): 0 EXP like every story boss, the boss death class (the record's
// action fires, the entity stays for the script).
static const EnemyStat a16_stat = {
    0, A16_HP, A16_AI_SET, A16_MELEE, A16_RANGED, A16_DEFENCE, 128, 128, BOSS_CLASS,
};
// Attack descriptors {u8 kind (1 = ranged), u8 reaction, pad, s32 base power}; the stock bomb uses
// {1, 1, .., 0x400}, 17's strong melee {1, 3, .., 0x400}.
static const u32 atk_missile[2] = { 0x0101, 0x400 };
static const u32 atk_heavy[2]   = { 0x0301, 0x400 };

enum { K_MISSILE = 1, K_FIST, K_BOMB, K_BOOM };
enum { ATK_NONE, ATK_HOMING, ATK_BARRAGE, ATK_BOMBS, ATK_PUNCH };

struct A16;
typedef struct Proj {
    u32 *vtable;          // +0x00
    s32 sort;             // +0x04 (bottom y, set by ProjBase_UpdateBox)
    u8 prio;              // +0x08
    u8 _p0[3];
    u16 tile, size;       // +0x0C
    const A16Desc *loaded;   // +0x10
    u8 owner_facing, _p1[3]; // +0x14
    const A16Desc *desc;  // +0x18 frame drawn (DMA'd when it differs from +0x10)
    Entity *owner;        // +0x1C (cleared by list message 0xF when the owner dies)
    s32 x, y;             // +0x20 8.8 anchor
    s32 box[4];           // +0x28
    s32 sx, sy;           // +0x38 screen position (cull slot)
    struct A16 *boss;     // +0x40 0 = orphan (the boss is gone): no damage
    u8 kind, dir, flame, big;
    s16 timer, fuse;
    s32 vx, vy, speed;    // 8.8 px/frame
    Entity *target;
    s32 gy, z;            // bombs: ground y (8.8) and height above it (px)
    s16 turn, radius;
    s32 mult;
    const u32 *attack;
    u32 vt[24];           // private vtable copy
} Proj;

typedef struct CtrlObj { u32 *vtable; } CtrlObj;

typedef struct A16 {
    u32 vt[48];           // the boss's vtable: ENEMY_VTABLE with destroy and update replaced
    u32 ctrl_vt[12];      // control state used while a special plays its own frames (idle state's vtable,
                          // slots 0-3 replaced; the engine calls slots 4+ on whatever state is current)
    CtrlObj ctrl;
    Entity *e;
    u8 attack, step, group, frame;
    s16 timer, cooldown;
    s16 shots, fired;
    s16 poise;            // frames of stun immunity left (A16_POISE)
    s16 _pad;
    Entity *target;
    Proj *live[A16_LIVE];
} A16;

static Entity *real_player(int i) { return i == 0 ? coop2.p1_frame : coop.p2; }

// ---- small helpers ------------------------------------------------------------------------------
static u32 isqrt(u32 v) {
    u32 r = 0, bit = 1u << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; } else r >>= 1;
        bit >>= 2;
    }
    return r;
}

// Scale (dx, dy) to length `len` (8.8 result); dx, dy in pixels or 8.8, any scale.
static void set_len(s32 dx, s32 dy, s32 len, s32 *ox, s32 *oy) {
    while (dx > 30000 || dx < -30000 || dy > 30000 || dy < -30000) { dx >>= 1; dy >>= 1; }
    s32 d = (s32)isqrt((u32)(dx * dx + dy * dy));
    if (d == 0) { *ox = len; *oy = 0; return; }
    *ox = dx * len / d;
    *oy = dy * len / d;
}

static void body_centre(const Entity *e, s32 *x, s32 *y) { *x = e->x >> 8; *y = (e->y >> 8) - 12; }

static Entity *nearer_player(const Entity *from) {
    Entity *p1 = real_player(0), *p2 = real_player(1);
    if (!p1) return p2;
    if (!p2) return p1;
    return nearest_player(from, p1, p2);
}

static s32 dist2_px(const Entity *a, const Entity *b) {
    s32 dx = (a->x - b->x) >> 8, dy = (a->y - b->y) >> 8;
    return dx * dx + dy * dy;
}

static int facing_to(s32 dx, s32 dy) {
    s32 ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    if (ax > ay) return dx < 0 ? 2 : 3;
    return dy < 0 ? 1 : 0;
}

// 0 right, 1 down-right, 2 down, 3 down-left, 4 left, 5 up-left, 6 up, 7 up-right (y down)
static int octant(s32 vx, s32 vy) {
    s32 ax = vx < 0 ? -vx : vx, ay = vy < 0 ? -vy : vy;
    if (ay * 5 < ax * 2) return vx < 0 ? 4 : 0;
    if (ax * 5 < ay * 2) return vy < 0 ? 6 : 2;
    if (vx > 0) return vy > 0 ? 1 : 7;
    return vy > 0 ? 3 : 5;
}

static void shake(int frames, int strength) {
    if (SHAKE_HEAD == g_Camera) {                  // already shaking: linking it again would loop it on itself
        s32 *left = (s32 *)((u8 *)g_Camera + 0xC);
        if (*left < frames) *left = frames;
        return;
    }
    Camera_Shake(g_Camera, frames, strength);
}

static void sfx(int id) { Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + id * 12, 0x40, 0); }
#define SFX_LAUNCH  0x0D   // the ki blast's launch (0x0D + rand 2)
#define SFX_BOOM    0x14   // the bomb's explosion (0x14 + rand 2)

// ---- hits ---------------------------------------------------------------------------------------
// The message the ROM's bomb broadcasts on landing (FUN_0801CDFC), with type 0: only players react.
typedef struct HitMsg {
    u8 type; u8 _p0[3];
    s32 box[4];           // x1, y1, x2, y2 (world px)
    u8 facing; u8 _p1[3]; // the attacker's facing (the player turns to face it)
    s32 mult;             // 10.10
    const u32 *attack;
    Entity *attacker;
    s32 out[3];           // crit / damage / secondary, filled by the receiver
    void *source;
    s32 _p2[3];
} HitMsg;

static void blast(Entity *boss, s32 x, s32 y, int r, const u32 *attack, s32 mult) {
    if (!boss || boss->hp <= 0) return;
    HitMsg m;
    memset(&m, 0, sizeof m);
    m.type = 0;
    m.box[0] = x - r; m.box[1] = y - r; m.box[2] = x + r; m.box[3] = y + r;
    m.facing = boss->facing;
    m.mult = mult;
    m.attack = attack;
    m.attacker = boss;
    m.source = boss;
    if (EntityList_Broadcast(g_SceneTable[1], 0, &m)) coop2.a16_hits++;
}

// ---- projectiles --------------------------------------------------------------------------------
static void proj_update(Proj *p);
static void proj_destroy(Proj *p, int free_it, u32 a, u32 b);

static void track(A16 *a, Proj *p) {
    for (int i = 0; i < A16_LIVE; i++) if (!a->live[i]) { a->live[i] = p; p->boss = a; return; }
    p->boss = a;   // untracked: still fine while the boss lives (the boss frees nothing a projectile uses)
}

static void untrack(Proj *p) {
    A16 *a = p->boss;
    if (!a) return;
    for (int i = 0; i < A16_LIVE; i++) if (a->live[i] == p) a->live[i] = 0;
    p->boss = 0;
}

static void proj_destroy(Proj *p, int free_it, u32 a, u32 b) {
    untrack(p);
    ((void (*)(Proj *, int, u32, u32))PROJ_DESTROY)(p, free_it, a, b);
}

// Creates a projectile at (x, y) px with a frame descriptor; vram = bytes of its largest frame.
static Proj *proj_new(A16 *a, int kind, s32 x, s32 y, const A16Desc *desc, int vram) {
    if (ObjVram_Largest(g_EntityList) < vram + 0x100) { coop2.a16_skipped++; return 0; }   // keep room for others
    Proj *p = Arena_Alloc(sizeof *p);
    if (!p) { coop2.a16_skipped++; return 0; }
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
    Proj *b = proj_new(0, K_BOOM, x, y, &a16_boom[0], 0x400);
    if (b) coop2.a16_booms++;
}

static void explode(Proj *p, int damage) {
    s32 x = p->x >> 8, y = p->y >> 8;
    Entity *boss = (damage && p->boss) ? p->boss->e : 0;
    spawn_boom(x, y);
    if (p->big) {
        spawn_boom(x - 12, y + 6);
        spawn_boom(x + 12, y + 6);
        if (ObjVram_Largest(g_EntityList) >= 0x200) {
            s32 xy[2] = { x, y };
            for (int i = 0; i < 4; i++) {
                void *d = Debris_Create(0, xy, DEBRIS_VEL + 2 * i);
                if (d) EntityList_Add(g_SceneTable[1], d);
            }
        }
        shake(24, 100);
    } else {
        shake(10, 60);
    }
    sfx(SFX_BOOM + Rand_Range(2));
    blast(boss, x, y, p->radius, p->attack, p->mult);
    proj_remove(p);
}

static int touches_player(const Proj *p) {
    s32 x = p->x >> 8, y = p->y >> 8;
    for (int i = 0; i < 2; i++) {
        Entity *pl = real_player(i);
        if (!pl) continue;
        s32 b[4];
        Entity_GetBox(b, pl);
        if (x > b[0] - 4 && x < b[2] + 4 && y > b[1] - 10 && y < b[3] + 4) return 1;
    }
    return 0;
}

static void proj_update(Proj *p) {
    p->timer++;
    if (p->kind == K_BOOM) {
        int f = p->timer < 4 ? 0 : 1 + (p->timer - 4) / 6;
        if (f > 3) { proj_remove(p); return; }
        p->desc = &a16_boom[f];
        ProjBase_UpdateBox(p);
        return;
    }
    int alive = p->boss && p->boss->e && p->boss->e->hp > 0;
    if (p->kind == K_BOMB) {
        if (p->z > 0) {
            p->z -= 2 + p->timer / 6;
            if (p->z <= 0) { p->z = 0; shake(4, 40); }
        } else if (--p->fuse <= 0) {
            explode(p, alive);
            return;
        }
        int f = p->z > 0 ? 0 : (p->fuse < 24 ? (p->timer >> 1) % 3 : (p->timer >> 3) % 3);
        p->desc = &BOMB_FRAMES[f];
        p->y = p->gy - (p->z << 8);
        ProjBase_UpdateBox(p);
        return;
    }
    if (!alive) { explode(p, 0); return; }
    if (p->kind == K_MISSILE && p->target) {
        s32 tx, ty, dx, dy;
        body_centre(p->target, &tx, &ty);
        set_len((tx << 8) - p->x, (ty << 8) - p->y, p->speed, &dx, &dy);
        p->vx += (dx - p->vx) / p->turn;
        p->vy += (dy - p->vy) / p->turn;
        set_len(p->vx, p->vy, p->speed, &p->vx, &p->vy);
    }
    p->x += p->vx;
    p->y += p->vy;
    if ((p->timer & 3) == 0) p->flame ^= 1;
    if (p->kind == K_MISSILE) p->desc = &a16_missile[octant(p->vx, p->vy) * 2 + p->flame];
    else p->desc = &a16_fist[p->dir * 2 + p->flame];
    ProjBase_UpdateBox(p);
    if (--p->fuse <= 0 || touches_player(p)) explode(p, 1);
}

static void launch_point(const Entity *e, s32 *x, s32 *y) {
    static const s8 off[4][2] = { { 0, 6 }, { 0, -10 }, { -12, 0 }, { 12, 0 } };
    body_centre(e, x, y);
    *x += off[e->facing & 3][0];
    *y += off[e->facing & 3][1];
}

static void fire_missile(A16 *a, Entity *target, s32 aim_x, s32 aim_y, s32 speed, int turn, int fuse, int radius, s32 mult) {
    s32 x, y;
    launch_point(a->e, &x, &y);
    Proj *p = proj_new(a, K_MISSILE, x, y, &a16_missile[0], 0x100);
    if (!p) return;
    set_len(aim_x, aim_y, speed, &p->vx, &p->vy);
    p->speed = speed;
    p->turn = (s16)turn;
    p->fuse = (s16)fuse;
    p->target = target;
    p->radius = (s16)radius;
    p->mult = mult;
    p->attack = atk_missile;
    p->desc = &a16_missile[octant(p->vx, p->vy) * 2];
    coop2.a16_fired++;
    sfx(SFX_LAUNCH + Rand_Range(2));
}

static void fire_fist(A16 *a) {
    static const u8 fist_dir[4] = { 1, 3, 2, 0 };              // facing down/up/left/right -> frame set
    static const s8 vec[4][2] = { { 0, 1 }, { 0, -1 }, { -1, 0 }, { 1, 0 } };
    s32 x, y;
    launch_point(a->e, &x, &y);
    int f = a->e->facing & 3;
    Proj *p = proj_new(a, K_FIST, x, y, &a16_fist[fist_dir[f] * 2], 0x100);
    if (!p) return;
    p->dir = fist_dir[f];
    p->vx = vec[f][0] * A16_PUNCH_SPEED;
    p->vy = vec[f][1] * A16_PUNCH_SPEED;
    p->fuse = A16_PUNCH_RANGE;
    p->radius = A16_PUNCH_RADIUS;
    p->mult = A16_PUNCH_MULT;
    p->attack = atk_heavy;
    coop2.a16_fired++;
    sfx(SFX_LAUNCH);
}

// A bomb lands at (x, y) after falling A16_BOMB_DROP px; inside the camera view.
static void drop_bomb(A16 *a, s32 x, s32 y) {
    const s32 *cam = CAMERA_CELL;
    s32 left = cam[7], top = cam[8];
    if (x < left + 12) x = left + 12;
    if (x > left + 228) x = left + 228;
    if (y < top + 24) y = top + 24;
    if (y > top + 150) y = top + 150;
    Proj *p = proj_new(a, K_BOMB, x, y, &BOMB_FRAMES[0], 0x100);
    if (!p) return;
    p->gy = y << 8;
    p->z = A16_BOMB_DROP;
    p->y = p->gy - (p->z << 8);
    p->fuse = A16_BOMB_FUSE;
    p->radius = A16_BOMB_RADIUS;
    p->mult = A16_BOMB_MULT;
    p->attack = atk_heavy;
    p->big = 1;
    coop2.a16_fired++;
}

// ---- the boss -----------------------------------------------------------------------------------
static A16 *state_of(Entity *e) {
    u8 *blk = (u8 *)e + ENEMY_SIZE + sizeof(EnemyStat);
    const EnemyRecord *r = (const EnemyRecord *)blk;
    u32 rs = (0x1C + 4 * (u32)r->behaviour_count + 3) & ~3u;
    return (A16 *)(blk + rs);
}

int is_a16(const Entity *e) { return e && coop2.a16 == e; }

static void set_frames(A16 *a, int group, int frame) {
    Entity *e = a->e;
    a->group = (u8)group;
    a->frame = (u8)frame;
    E32(e, 0x4C) = (u32)e->sprite_record + 0x4C + 0x10 * (u32)group;
    E16(e, 0x14) = (u16)frame;
}

static void ctrl_enter(CtrlObj *s, Entity *e) {
    (void)s;
    E16(e, 0x12) = 0;
    E8(e, 8) = 0x28;
}
static void ctrl_nop(CtrlObj *s, Entity *e) { (void)s; (void)e; }

static void begin(A16 *a, int attack) {
    Entity *e = a->e;
    a->attack = (u8)attack;
    a->step = 0;
    a->timer = 0;
    a->fired = 0;
    s32 dx = (a->target->x - e->x) >> 8, dy = (a->target->y - e->y) >> 8;
    e->facing = (u8)facing_to(dx, dy);
    Entity_SetCtrlState(e, &a->ctrl);
    set_frames(a, attack == ATK_PUNCH ? 13 : 5, 0);
}

static void finish(A16 *a) {
    Entity *e = a->e;
    a->attack = ATK_NONE;
    int pct = e->hp * 100 / ((const EnemyStat *)e->behaviour)->max_hp;
    a->cooldown = (s16)(A16_COOLDOWN_MIN + Rand_Range(A16_COOLDOWN_MAX - A16_COOLDOWN_MIN));
    if (pct < 34) a->cooldown = (s16)(a->cooldown * A16_RAGE_PCT / 100);
    if (e->ctrl_state == &a->ctrl) Entity_SetAnimation(e, 0);
}

static int choose(A16 *a) {
    Entity *e = a->e;
    int pct = e->hp * 100 / ((const EnemyStat *)e->behaviour)->max_hp;
    // weights: homing, barrage, bombs, punch
    int w[4];
    if (pct > 66)      { w[0] = 40; w[1] = 20; w[2] = 0;  w[3] = 40; }
    else if (pct > 33) { w[0] = 25; w[1] = 25; w[2] = 25; w[3] = 25; }
    else               { w[0] = 25; w[1] = 25; w[2] = 35; w[3] = 15; }
    if (coop2.a16_force) { int f = coop2.a16_force; return f; }
    int r = Rand_Range(100);
    for (int i = 0; i < 4; i++) { if (r < w[i]) return ATK_HOMING + i; r -= w[i]; }
    return ATK_HOMING;
}

static void step_attack(A16 *a) {
    Entity *e = a->e;
    int pct = e->hp * 100 / ((const EnemyStat *)e->behaviour)->max_hp;
    int rage = pct < 34;
    a->timer++;
    switch (a->attack) {
    case ATK_HOMING:   // charge 30 frames, fire (one per player when raging), hold 20
        set_frames(a, 5, a->timer < 30 ? (a->timer >> 3) % 3 : 2);
        if (a->timer == 30) {
            Entity *t = nearer_player(e);
            s32 tx, ty, x, y;
            body_centre(t, &tx, &ty);
            launch_point(e, &x, &y);
            fire_missile(a, t, tx - x, ty - y, A16_HOMING_SPEED, A16_HOMING_TURN, A16_HOMING_FUSE, A16_HOMING_RADIUS, A16_HOMING_MULT);
            Entity *other = t == real_player(0) ? real_player(1) : real_player(0);
            if (rage && other) {
                body_centre(other, &tx, &ty);
                fire_missile(a, other, tx - x, ty - y, A16_HOMING_SPEED, A16_HOMING_TURN, A16_HOMING_FUSE, A16_HOMING_RADIUS, A16_HOMING_MULT);
            }
        }
        if (a->timer >= 50) finish(a);
        break;
    case ATK_BARRAGE: {   // charge 24, then one missile every 4 frames across the fan
        int n = rage ? A16_BARRAGE_COUNT + 2 : A16_BARRAGE_COUNT;
        set_frames(a, 5, a->timer < 24 ? (a->timer >> 3) % 3 : 2);
        if (a->timer >= 24 && ((a->timer - 24) & 3) == 0 && a->fired < n) {
            Entity *t = a->target;
            s32 tx, ty, x, y, ax, ay;
            body_centre(t, &tx, &ty);
            launch_point(e, &x, &y);
            set_len(tx - x, ty - y, 0x100, &ax, &ay);
            s32 k = (2 * a->fired - (n - 1)) * A16_BARRAGE_SPREAD / (n - 1);   // -spread .. +spread
            fire_missile(a, t, ax - (ay * k >> 8), ay + (ax * k >> 8), A16_BARRAGE_SPEED, A16_BARRAGE_TURN,
                         A16_BARRAGE_FUSE, A16_BARRAGE_RADIUS, A16_BARRAGE_MULT);
            a->fired++;
        }
        if (a->fired >= n && a->timer >= 24 + 4 * n + 16) finish(a);
        break;
    }
    case ATK_BOMBS: {   // arms up 30, then a bomb every 6 frames: alternately at each player, then anywhere
        int n = rage ? A16_BOMB_COUNT + 2 : A16_BOMB_COUNT;
        set_frames(a, 5, a->timer < 30 ? (a->timer >> 3) % 3 : 2);
        if (a->timer >= 30 && ((a->timer - 30) % 6) == 0 && a->fired < n) {
            Entity *pl = real_player(a->fired & 1);
            if (!pl) pl = a->target;
            s32 x, y;
            if (a->fired < 4) {
                body_centre(pl, &x, &y);
                x += Rand_Range(64) - 32;
                y += Rand_Range(48) - 12;
            } else {
                const s32 *cam = CAMERA_CELL;
                x = cam[7] + 16 + Rand_Range(208);
                y = cam[8] + 28 + Rand_Range(118);
            }
            drop_bomb(a, x, y);
            a->fired++;
        }
        if (a->fired >= n && a->timer >= 30 + 6 * n + 20) finish(a);
        break;
    }
    case ATK_PUNCH:     // wind-up 18 (frame 0), forearm off (frame 1) while the fist flies, regrow
        if (a->timer < 18) set_frames(a, 13, 0);
        else set_frames(a, 13, 1);
        if (a->timer == 18) fire_fist(a);
        if (a->timer >= 18 + 36) finish(a);
        break;
    default:
        finish(a);
    }
}

// Enemy vtable slot 2 for the boss: control/aux states as usual, then either the stock AI or the
// special in progress.
static void a16_update(Entity *e) {
    A16 *a = state_of(e);
    coop2.a16 = e;
    if (e->hp <= 0) {              // defeated: the script owns him now; show the knock-down and stop
        if (!coop2.a16_dead_frame) coop2.a16_dead_frame = g_FrameCounter;
        if (e->ctrl_state != &a->ctrl) Entity_SetCtrlState(e, &a->ctrl);
        set_frames(a, 12, 0);
        a->attack = ATK_NONE;
        Entity_BaseUpdate(e);
        return;
    }
    Entity_BaseUpdate(e);
    if (a->poise > 0) a->poise--;
    if (a->attack) {
        if (e->ctrl_state != &a->ctrl) { a->attack = ATK_NONE; a->cooldown = 60; }   // knocked out of it
        else { step_attack(a); return; }
    }
    Ai_Tick((u8 *)e + 0x19C);
    if (a->cooldown > 0) { a->cooldown--; return; }
    void *st = e->ctrl_state;
    if (st != ENEMY_STATES[0] && st != ENEMY_STATES[1]) return;   // only from idle or walking
    Entity *t = nearer_player(e);
    if (!t || dist2_px(e, t) > A16_RANGE * A16_RANGE) return;
    a->target = t;
    coop2.a16_specials++;
    begin(a, choose(a));
}

// Enemy vtable slot 9 for the boss (messages). A player's attack (message 0, types 1 and 2) during
// poise or a special is resolved here without the AI's hit reaction: the stock box test, the damage
// calculation and OnHit (HP, popup, flash, death) only. Everything else goes to the stock handler,
// and a stun it causes starts the poise window.
void damage_calc_hook(Entity *defender, Hit *hit);   // enemies.c
#define ENEMY_MESSAGE 0x0800E5B9u
static int a16_message(Entity *e, int msg, u8 *m) {
    A16 *a = state_of(e);
    if (msg == 0 && (m[0] == 1 || m[0] == 2) && e->hp > 0) {
        if (a->poise > 0 || a->attack) {
            if (!E32(e, 0x168)) return 0;              // not hittable right now (as the stock test)
            s32 b[4];
            const s32 *mb = (const s32 *)(m + 4);
            Entity_GetBox(b, e);
            if (!(mb[0] < b[2] && b[0] < mb[2] && mb[1] < b[3] && b[1] < mb[3])) return 0;
            damage_calc_hook(e, (Hit *)(m + 0x14));
            Enemy_OnHit(e, m + 0x24);
            return 1;
        }
        int r = ((int (*)(Entity *, int, u8 *))ENEMY_MESSAGE)(e, msg, m);
        if (r) a->poise = A16_POISE;
        return r;
    }
    return ((int (*)(Entity *, int, u8 *))ENEMY_MESSAGE)(e, msg, m);
}

// Test autopilot helper: is a landed (or nearly landed) bomb or a close missile about to catch pl?
// Returns 1 and an escape direction (px) away from the nearest threat.
int a16_threat(const Entity *pl, s32 *ex, s32 *ey) {
    Entity *e = coop2.a16;
    if (!e) return 0;
    A16 *a = state_of(e);
    s32 px = pl->x >> 8, py = (pl->y >> 8) - 8;
    s32 best = 0x7FFFFFFF;
    void *st = e->ctrl_state;                      // a melee swing in progress (control states 6, 7, 8, 10)
    if (st == ENEMY_STATES[6] || st == ENEMY_STATES[7] || st == ENEMY_STATES[8] || st == ENEMY_STATES[10]) {
        s32 dx = px - (e->x >> 8), dy = (pl->y - e->y) >> 8;
        if (dx > -34 && dx < 34 && dy > -20 && dy < 20) { *ex = dx ? dx : 1; *ey = 0; return 1; }
    }
    for (int i = 0; i < A16_LIVE; i++) {
        Proj *p = a->live[i];
        if (!p) continue;
        s32 qx = p->x >> 8, qy = (p->kind == K_BOMB ? p->gy : p->y) >> 8;
        s32 reach = p->kind == K_BOMB ? (p->z < 48 ? p->radius + 22 : 0) : 34;
        s32 dx = px - qx, dy = py - qy, d2 = dx * dx + dy * dy;
        if (reach && d2 < reach * reach && d2 < best) {
            best = d2;
            *ex = dx ? dx : 1;
            *ey = dy;
        }
    }
    return best != 0x7FFFFFFF;
}

static void a16_destroy(Entity *e, int free_it, u32 x, u32 y) {
    A16 *a = state_of(e);
    for (int i = 0; i < A16_LIVE; i++) {
        if (a->live[i]) { a->live[i]->boss = 0; a->live[i] = 0; }   // orphans fizzle without damage
    }
    if (coop2.a16 == e) coop2.a16 = 0;
    e->vtable = (u32 *)ENEMY_VTABLE;
    Enemy_Destroy(e, free_it, x, y);
}

// enemy_spawn_hook for stat A16_STAT: block = entity + stat copy + record copy + boss state.
Entity *a16_spawn(const EnemyRecord *rec) {
    u32 rs = (0x1C + 4 * (u32)rec->behaviour_count + 3) & ~3u;
    u8 *block = Arena_Alloc(ENEMY_SIZE + sizeof(EnemyStat) + rs + sizeof(A16));
    if (!block) return 0;
    EnemyStat *stat = (EnemyStat *)(block + ENEMY_SIZE);
    EnemyRecord *r = (EnemyRecord *)(block + ENEMY_SIZE + sizeof(EnemyStat));
    A16 *a = (A16 *)((u8 *)r + rs);
    memset(a, 0, sizeof *a);
    *stat = a16_stat;
    if (!p2_present()) stat->max_hp = A16_HP * A16_SOLO_HP_PCT / 100;
    memcpy(r, rec, 0x1C + 4 * (u32)rec->behaviour_count);
    Entity *e = Enemy_Create(block, r, stat);
    if (!e) { Arena_Free(block); return 0; }
    a->e = e;
    u32 base = (u32)a->vt;
    const u32 *orig = (const u32 *)ENEMY_VTABLE;
    for (int i = 0; i < 48; i++) a->vt[i] = (ENEMY_VTABLE + orig[i]) - base;
    a->vt[0] = (u32)a16_destroy - base;
    a->vt[2] = (u32)a16_update - base;
    a->vt[9] = (u32)a16_message - base;
    base = (u32)a->ctrl_vt;
    orig = (const u32 *)IDLE_STATE_VTABLE;
    for (int i = 0; i < 12; i++) a->ctrl_vt[i] = (IDLE_STATE_VTABLE + orig[i]) - base;
    a->ctrl_vt[0] = (u32)ctrl_enter - base;
    a->ctrl_vt[1] = (u32)ctrl_nop - base;
    a->ctrl_vt[2] = (u32)ctrl_nop - base;
    a->ctrl_vt[3] = (u32)ctrl_nop - base;
    a->ctrl.vtable = a->ctrl_vt;
    a->cooldown = A16_FIRST_DELAY;
    e->vtable = a->vt;
    coop2.a16_spawn_frame = g_FrameCounter;
    coop2.a16_dead_frame = 0;
    coop2.a16 = e;
    return e;
}

// Test placement (COOP_CMD_A16, A16_TEST builds: player 1 holds L + R and presses Down): an enemy record
// for stat 109 / sprite 160 64 px to the right of player 1, spawned the way TriggerMapEvent spawns a
// boss record, plus the boss bar the island script draws for 17.
void a16_test_spawn(void) {
    static const u32 face_left[2] = { 0x0800CB0Fu, 0x00027D00u };   // NpcBehavior_FaceAndWait, as 17's record
    Entity *p1 = g_PlayerObject;
    if (!p1 || coop2.a16) return;
    struct { EnemyRecord r; const void *beh[1]; } rec;
    memset(&rec, 0, sizeof rec);
    rec.r.handler = (void *)0x0800E77Fu;
    rec.r.flags = 0;
    rec.r.stat_index = A16_STAT;
    rec.r.x = (s16)((p1->x >> 8) + 64);
    rec.r.y = (s16)(p1->y >> 8);
    rec.r.sprite_id = A16_SPRITE;
    rec.r.action = (void *)0;
    static const u32 no_action[2] = { ENEMY_ACTION_NONE, 0 };
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
