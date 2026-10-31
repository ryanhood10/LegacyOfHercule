// The team combos' runner (docs/tournament.md 24.10 the design decided with Ryan, 25.2 what this file does; the
// contract is tny_combo.h). tny_combo_start() takes a pair and a target; the next frame tick (tny_combo_run_frame)
// plays the combo inside the game's own command modal, the one a script's walks and NPC beams run in:
//   * CommitCommandBatch's modal (0x0800DE20, decompiled 2026-10-31) broadcasts message 10 to the entity list every
//     frame and ends when no entity has a command left. An entity with a command steps it, then animates (control
//     state slot 1, aux state slot 1); one without only animates. Nothing's update (slot 2) runs: no AI, no special,
//     no player state reads a pad, no engine fight logic beyond its frame tick. The runner's command on fighter 0
//     (the "director") plays the whole step table, one frame per message.
//   * Projectiles and beams are the exception: their message 10 is their update (the stock projectile message
//     0x08013D68 calls slot 10; beams and the players' ki blasts likewise), so every one in flight is frozen for
//     the combo by a vtable copy whose message swallows 10 (restored at the end).
//   * Every fighter and player (the pair, the target and the bystanders) is put in a hold control state whose
//     animate is empty (the kits' stun recipe), so nobody even animates; at the end each goes back to his idle
//     state where he stands. The target's own reactions never run: the combo's hits go through the game's damage
//     formula (damage_calc_hook, the attacker's own combat bytes) and the body's own OnHit (slot 14) directly, the
//     way a kit's special takes a hit (no AI hit reaction, no knock-back, no poise).
// Damage is a multiplier of the attackers' own moves (Ryan): a step's weight in tenths of one of his punches (melee)
// or ki blasts (ranged), so a stronger story character or a higher tier hits harder; never a share of the target's HP.
// Hercule is the showman: his steps are poses only, and their weight goes to his partner's next blow.
#include "tny_combo_run.h"
#include "tny_engine.h"
#include "revive.h"

#define RUN_MAGIC       0xC0B0A001u
#define RUN_STALE       30          // frames: a combo that stopped ticking this long ago is over (a reset, a load)
#define MAX_SHOTS       8
#define MAX_FROZEN      16
#define MAX_HELD        10
#define RING_MARGIN     20          // px: an entity's position stops about this short of the ring's blocked edges
#define SHOT_SPEED      (6 << 8)    // ki blasts, 8.8 px per frame
#define BALL_SPEED      (4 << 8)
#define DISC_SPEED      (5 << 8)
#define FALL_FRAMES     18          // the knock-down's fall (as tny_moves.c)
#define SFX_LAUNCH      0x0D
#define SFX_BOOM        0x14
#define SFX_HIT         0x22        // the punch / ki impact (0x22 + rand 3)
#define SFX_HERCULE     40          // the stock Hercule punch's voice clip (hercule.c)

// ---- ROM functions and layouts (as tny_moves.c / tny_engine.c use them) --------------------------------------------
typedef struct Desc { s8 xo, yo; u8 w, h; u32 attr; const void *blob; } Desc;
extern const Desc s13_orb_s[3], s13_orb_m[2], s13_orb_l[2];    // build/gen/super13_sprites_data.c
extern const Desc kr_kiblast[16], kr_disc[4];                   // build/gen/krillin_sprites_data.c
extern const Desc a16_boom[4];                                  // build/gen/a16_sprites_data.c

ROM_FN(void *, ProjBase_Init, (void *obj, int vram_bytes, Entity *owner), 0x08013BC1);
ROM_FN(void, ProjBase_UpdateBox, (void *obj), 0x08013BF5);
ROM_FN(int, ObjVram_Largest, (void *list), 0x08006D71);
ROM_FN(void, Entity_SetCtrlState, (Entity *e, void *state), 0x0800763D);
ROM_FN(void, Entity_GetBox, (s32 *out, const Entity *e), 0x080079B9);
ROM_FN(void, Entity_BeamPoint, (s32 *out, Entity *e), 0x08007A27);
ROM_FN(void *, Beam_Init, (void *mem, s32 *pos, int a, Entity *owner), 0x0801D84D);
ROM_FN(void, Beam_Fire, (void *beam, int facing, s32 *pos), 0x08014479);
ROM_FN(void, Beam_End, (void *beam), 0x0801451D);
ROM_FN(void, Cmd_Queue, (u32 *batch, void *cmd, Entity *e), 0x0800E04D);
ROM_FN(void, Cmd_Begin, (u32 *batch), 0x0800E033);
ROM_FN(void, Cmd_Commit, (u32 *batch), 0x0800E039);
#define CMD_BATCH           ((u32 *)0x030020F8)
#define PROJ_VTABLE         0x080259C4u
#define PROJ_DESTROY        0x08027A69u
#define PROJ_MESSAGE        0x08013D69u     // the projectile class's message: 10 = update (slot 10)
#define NPC_KI_MESSAGE      0x08011CA9u     // the stock NPC ki blast's
#define KI_BLAST_MESSAGE    0x0800EA3Bu     // the players' ki blast's (vtable 0x08024CBC)
#define BEAM_MESSAGE        0x0801D91Du     // the beam's (vtable 0x08025830)
#define BEAM_VTABLE         0x08025830u
#define BEAM_UPDATE         0x0801D88Bu
#define BEAM_OFFSETS        ((const s32 *)0x08701DF0)
#define IDLE_STATE_VTABLE   0x08024BA0u
#define OBJ_PALETTE         ((volatile const u16 *)0x05000200)
#define REG_BLDCNT          (*(volatile u16 *)0x04000050)
#define REG_BLDY            (*(volatile u16 *)0x04000054)
#define E8(e, o)            (*(u8 *)((u8 *)(e) + (o)))
#define E16(e, o)           (*(u16 *)((u8 *)(e) + (o)))
#define E32(e, o)           (*(u32 *)((u8 *)(e) + (o)))
void damage_calc_hook(Entity *defender, Hit *hit);       // enemies.c (the game's formula, the co-op multiplier)

typedef struct Beam {
    u32 *vtable; s32 sort_y; u8 prio, _p0[3]; u16 tile, tile_bytes;
    u8 state, impact, alt, _p1;              // +0x10 state 0 charge, 1 out, 2 ending
    s32 fx_time, ticks;
    s32 x, y;                                // +0x1C origin (world px)
    s32 sx, sy;
    s32 dx, dy;                              // +0x2C unit direction
    s32 len;                                 // +0x34 px
    s32 facing, mult;
    const u8 *frame, *frame_set, *frames;
    Entity *owner;                           // +0x4C (hit type = owner's byte +0x16C)
} Beam;
_Static_assert(sizeof(Beam) == 0x50, "Beam size");

// The hit message (krillin.c / tny_moves.c HitMsg): the Hit the formula fills is at +0x14, OnHit's at +0x24
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
static const u32 atk_punch[2] = { 0x0200, 0x400 };   // the players' punch (descriptor 2 of 0x086DB2CC: melee, 0x400)
static const u32 atk_ki[2]    = { 0x0101, 0x400 };   // the ki blast (descriptor 1: ranged, 0x400)

// ---- the runner's own projectiles (the ROM projectile class, as tny_moves.c's) -----------------------------------
enum { PK_SHOT = 1, PK_BALL, PK_DISC, PK_BOOM };
typedef struct CProj {
    u32 *vtable; s32 sort; u8 prio, _p0[3]; u16 tile, size; const Desc *loaded;
    u8 owner_facing, _p1[3];
    const Desc *desc;     // +0x18
    Entity *owner;        // +0x1C
    s32 x, y;             // +0x20 8.8
    s32 box[4];
    s32 sx, sy;           // the ROM layout ends at +0x40
    s32 vx, vy;
    s16 timer, fuse;
    u8 kind, art, from, size_class;
    s16 w10, _p2;
    u32 vt[24];
} CProj;

// ---- state ------------------------------------------------------------------------------------------------------
typedef struct CtrlObj { u32 *vtable; } CtrlObj;
// Persistent (NOLOAD, comboram): what must outlive the combo (the hold state a fallen target may still sit in until
// his kit's next frame swaps it) and what the tests read. Offsets in docs/ids.md (test command 33).
typedef struct RunRam {
    u32 magic;              // +0x00
    u8 busy, combo, side, reason;   // +0x04 busy 1 starting, 2 playing; reason 1 played out, 2 target down, 3 cut short
    u32 tick;               // +0x08 g_FrameCounter at the last frame played
    u16 runs, frames;       // +0x0C combos played; frames of the last
    s32 hp0, hp1;           // +0x10 the target's HP before / after (a player: his party entry's)
    s32 dmg[2];             // +0x18 damage dealt by fighter 0 / 1
    s16 pos[3][2];          // +0x20 fighter 0, 1, the target: x, y after (px)
    u8 hits, held, frozen, moved;   // +0x2C hits landed; bodies held; projectiles frozen; bystanders that moved
    u8 a_id, b_id, fail, starts;    // +0x30 the pair's TF ids; why the last start failed; starts accepted
    Entity *ent[3];         // +0x34 fighter 0, 1, the target
    void *run;              // +0x40 the working block on the heap while a combo plays
    CtrlObj hold;           // +0x44 the hold control state
    u32 hold_vt[12];        // +0x48
    s16 start_pos[3][2];    // +0x78 where they stood at the start
    u16 shots, kd;          // +0x84 projectiles thrown, frames of the knock-down
} RunRam;
_Static_assert(sizeof(RunRam) <= 0x140, ".combo.run is 0x140 bytes");
static RunRam ram COMBO_SECTION("run");

typedef struct Cmd { u32 *vtable; void *next; u32 done; } Cmd;
typedef struct Frozen { u32 vt[24]; Entity *obj; const u32 *orig; } Frozen;   // vt first: obj->vtable points here
typedef struct Actor {
    Entity *e;
    s32 x0, y0, x1, y1;     // a move: from, to (8.8)
    s16 go_t, go_n;
    s16 pose_t, pose_n;     // pose_n: frames to run f0..f1 (0 hold, < 0 loop period)
    s8 sign;                // the fighter's side of the target: -1 left, 1 right
    u8 kind, f0, f1;
    s16 flash;
    s16 knock_t, lie, up;   // the target's knock-down: frame, lying frames, getting-up frames (knock_t 0 = none)
    u8 go_kind, hercule;    // the pose while moving; 1 = Hercule (the showman: poses only)
    s16 bank;               // weight banked from his Hercule partner's steps (tenths)
} Actor;
typedef struct BeamRec { Beam *b; u8 from, done, through, _p; s16 w10, left; u32 vt[16]; } BeamRec;
typedef struct Run {
    Cmd cmd;
    u32 cmd_vt[6];
    Actor act[3];
    const TComboDef *def;
    u16 t, si;
    CProj *shot[MAX_SHOTS];
    BeamRec beam[2];
    u8 white[256];
    Frozen *frz[MAX_FROZEN];
    Entity *held[MAX_HELD];
    s32 hx[MAX_HELD], hy[MAX_HELD];
    s32 fx[MAX_FROZEN], fy[MAX_FROZEN];
    u8 nfrz, nheld, finished, white_ok;
    s16 screen_t, screen_n;
    u16 bldcnt;
} Run;

// ---- helpers ------------------------------------------------------------------------------------------------------
static void ram_init(void) {
    if (ram.magic == RUN_MAGIC) return;
    memset(&ram, 0, sizeof ram);
    ram.magic = RUN_MAGIC;
}
static int in_list(const void *e) {
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) if (l->items[i] == e) return 1;
    return 0;
}
static Entity *p1(void) { return coop2.p1_frame ? coop2.p1_frame : g_PlayerObject; }
static int is_player(const Entity *e) { return e && (e == p1() || e == g_PlayerObject || e == coop.p2); }
static CharacterEntry *entry_of(const Entity *e) {
    if (e == coop.p2) return &g_Party->chars[coop.p2_slot];
    if (e == p1() || e == g_PlayerObject) return &g_Party->chars[g_Party->active_slot];
    return 0;
}
static int is_hercule(int id) { return id == TF_P_HERCULE || id == TF_HERCULE || id == TF_BHERCULE; }
static void sfx(int id) { Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + id * 12, 0x40, 0); }
static void centre(const Entity *e, s32 *x, s32 *y) {
    s32 b[4];
    Entity_GetBox(b, e);
    *x = (b[0] + b[2]) >> 1;
    *y = (b[1] + b[3]) >> 1;
}
static int facing_to(s32 dx, s32 dy) {
    s32 ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    if (ax >= ay) return dx < 0 ? 2 : 3;
    return dy < 0 ? 1 : 0;
}
static void face(Entity *e, const Entity *t) {
    if (!e || !t || e == t) return;
    s32 ex, ey, tx, ty;
    centre(e, &ex, &ey);
    centre(t, &tx, &ty);
    e->facing = (u8)facing_to(tx - ex, ty - ey);
}
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
static int walkable(Entity *e, s32 x, s32 y) {
    s32 px[2] = { x, y };
    return x > 8 && y > 8 && Collision_Check((const void *)g_Ctx[4], px, (u8 *)e + 0x154);
}
// A spot on the ring's floor (in the ring; elsewhere the map's own collision decides)
static void clamp_floor(s32 *x, s32 *y) {
    if (!tny_ring_active()) return;
    if (*x < TNY_RING_X0 + RING_MARGIN) *x = TNY_RING_X0 + RING_MARGIN;
    if (*x > TNY_RING_X1 - RING_MARGIN) *x = TNY_RING_X1 - RING_MARGIN;
    if (*y < TNY_RING_Y0 + RING_MARGIN) *y = TNY_RING_Y0 + RING_MARGIN;
    if (*y > TNY_RING_Y1 - RING_MARGIN) *y = TNY_RING_Y1 - RING_MARGIN;
}
static s32 hp_of(const Entity *e) {
    CharacterEntry *c = entry_of(e);
    return c ? (s32)c->hp : e->hp;
}
static int down(const Entity *e) {
    if (!e || !in_list(e)) return 1;
    if (is_player(e)) { CharacterEntry *c = entry_of(e); return revive_is_down(e) || !c || c->hp == 0; }
    return e->hp <= 0;
}

// ---- poses (any body: a stock enemy set, a player layout, a giant; tny_moves.c's rules) -------------------------------
static int desc_ok(const Desc *d) {
    u32 b = (u32)d->blob;
    if (b < 0x08000000u || b >= 0x0A000000u || (b & 3)) return 0;
    if (!(d->w == 8 || d->w == 16 || d->w == 32 || d->w == 64)) return 0;
    if (!(d->h == 8 || d->h == 16 || d->h == 32 || d->h == 64)) return 0;
    return *(const u32 *)b <= 2;
}
static const u32 *group_of(const Entity *e, int g) {
    const u8 *rec = (const u8 *)e->sprite_record;
    if (!rec || g < 0 || g >= rec[2]) return 0;
    const u32 *grp = (const u32 *)(rec + 0x4C + 0x10 * g);
    return grp[0] && grp[1] && grp[2] && grp[3] && desc_ok((const Desc *)grp[e->facing & 3]) ? grp : 0;
}
// frames in a group's array for his facing: its run of drawable descriptors up to the next array of the record
static int frames_in(const Entity *e, const u32 *grp) {
    const u8 *rec = (const u8 *)e->sprite_record, *d = (const u8 *)grp[e->facing & 3];
    int n = 1;
    for (; n < 8; n++) {
        const u8 *q = d + 12 * n;
        for (int g = 0; g < rec[2]; g++)
            for (int f = 0; f < 4; f++) if (*(const u8 *const *)(rec + 0x4C + 0x10 * g + 4 * f) == q) return n;
        if (!desc_ok((const Desc *)q)) return n;
    }
    return n;
}
// Each pose kind's group and its stand-ins (docs/sprites.md: player layout 21 groups; the enemy sets fewer)
static const s8 kinds[CP_COUNT][4] = {
    [CP_IDLE] = { 0, -1, -1, -1 },   [CP_WALK] = { 1, 0, -1, -1 },    [CP_HURT] = { 2, 0, -1, -1 },
    [CP_FALL] = { 3, 2, 0, -1 },     [CP_SHOUT] = { 4, 14, 5, 0 },    [CP_CHARGE] = { 5, 18, 0, -1 },
    [CP_PUNCH] = { 7, 6, 8, 0 },     [CP_KICK] = { 8, 7, 6, 0 },      [CP_DASH] = { 13, 1, 0, -1 },
    [CP_SPECIAL] = { 16, 8, 7, 6 },  [CP_BEAM] = { 18, 19, 5, 0 },    [CP_BEAM2] = { 19, 18, 5, 0 },
    [CP_KI] = { 5, 6, 7, 0 },        [CP_LIE] = { 12, 3, 2, 0 },
};
static const u32 *kind_group(const Entity *e, int kind) {
    if (kind >= CP_RAW && kind < CP_RAW + 32) { const u32 *g = group_of(e, kind - CP_RAW); return g ? g : group_of(e, 0); }
    if (kind >= CP_COUNT) kind = CP_IDLE;
    for (int i = 0; i < 4 && kinds[kind][i] >= 0; i++) { const u32 *g = group_of(e, kinds[kind][i]); if (g) return g; }
    return group_of(e, 0);
}
static void put_pose(Entity *e, const u32 *grp, int f) {
    if (!grp) return;
    int n = frames_in(e, grp);
    if (f >= n) f = n - 1;
    if (f < 0) f = 0;
    E32(e, 0x4C) = (u32)grp;
    E16(e, 0x14) = (u16)f;
    E16(e, 0x12) = 0;
}
static void pose_kind(Entity *e, int kind, int f) {
    if (kind == CP_LIE) {                           // the engine's lying pose (the lying table, else 12 / 3 / 2)
        int lf = 0;
        const u32 *lg = tny_lie_pose(e, &lf);
        if (lg) { put_pose(e, lg, lf); return; }
    }
    put_pose(e, kind_group(e, kind), f);
}
static void set_tint(Entity *e, const u8 *t) {
    E32(e, 0x24) = (u32)t;
    E32(e, 0x20) = 0;                               // drop the cached frame so the next one goes through the remap
}

// ---- the hold state (the kits' stun recipe: tny_moves.c ctrl_enter / ctrl_nop) -------------------------------------
static void hold_enter(CtrlObj *s, Entity *e) { (void)s; E16(e, 0x12) = 0; E8(e, 8) = 0x28; }
static void hold_nop(CtrlObj *s, Entity *e) { (void)s; (void)e; }
static void hold_init(void) {
    u32 b = (u32)ram.hold_vt;
    const u32 *iv = (const u32 *)IDLE_STATE_VTABLE;
    for (int i = 0; i < 12; i++) ram.hold_vt[i] = (IDLE_STATE_VTABLE + iv[i]) - b;
    ram.hold_vt[0] = (u32)hold_enter - b;
    ram.hold_vt[1] = (u32)hold_nop - b;
    ram.hold_vt[2] = (u32)hold_nop - b;
    ram.hold_vt[3] = (u32)hold_nop - b;
    ram.hold.vtable = ram.hold_vt;
}
static void hold(Entity *e) { if (e && e->ctrl_state != &ram.hold) Entity_SetCtrlState(e, &ram.hold); }
static void release(Entity *e) { if (e && in_list(e) && e->ctrl_state == &ram.hold) Entity_SetAnimation(e, 0); }

// ---- freezing what flies (projectiles, beams: their message 10 is their update) -----------------------------------
static int frozen_msg(Entity *obj, int msg, void *arg) {
    if (msg == 10) return 0;
    const Frozen *f = (const Frozen *)obj->vtable;
    return ((int (*)(Entity *, int, void *))((u32)f->orig + f->orig[9]))(obj, msg, arg);
}
static int flies(const Entity *e) {
    u32 m = (u32)vslot(e, 9);
    return m == PROJ_MESSAGE || m == NPC_KI_MESSAGE || m == KI_BLAST_MESSAGE || m == BEAM_MESSAGE;
}
// what a frozen object's stillness is checked on: a projectile's position (+0x20, +0x24, 8.8), a beam's origin and length
static u32 where_of(const Entity *e, const u32 *vt, int k) {
    if ((u32)vt + vt[9] == BEAM_MESSAGE) return k ? E32(e, 0x34) : E32(e, 0x1C) ^ (E32(e, 0x20) << 16);
    return E32(e, 0x20 + 4 * k);
}
static void freeze(Run *r, Entity *e) {
    if (r->nfrz >= MAX_FROZEN) return;
    Frozen *f = Arena_Alloc(sizeof *f);
    if (!f) return;
    const u32 *ov = e->vtable;
    for (int i = 0; i < 24; i++) f->vt[i] = ((u32)ov + ov[i]) - (u32)f->vt;
    f->vt[9] = (u32)frozen_msg - (u32)f->vt;
    f->obj = e;
    f->orig = ov;
    e->vtable = f->vt;
    r->fx[r->nfrz] = (s32)where_of(e, ov, 0);
    r->fy[r->nfrz] = (s32)where_of(e, ov, 1);
    r->frz[r->nfrz++] = f;
}
static void unfreeze_all(Run *r) {
    for (int i = 0; i < r->nfrz; i++) {
        Frozen *f = r->frz[i];
        if (!f) continue;
        if (in_list(f->obj) && f->obj->vtable == f->vt) {
            if (where_of(f->obj, f->orig, 0) != (u32)r->fx[i] || where_of(f->obj, f->orig, 1) != (u32)r->fy[i]) ram.moved++;
            f->obj->vtable = (u32 *)f->orig;
        }
        Arena_Free(f);
        r->frz[i] = 0;
    }
    r->nfrz = 0;
}

// Everyone else on the floor: players and kit fighters are held where they stand (not the fallen: they lie in their
// kit's own state; not Android 16 / Super 13, whose own code drives their states: they only animate), everything
// that flies is frozen.
static int participant(const Run *r, const Entity *e) { return e == r->act[0].e || e == r->act[1].e || e == r->act[2].e; }
static void freeze_world(Run *r) {
    for (int i = 0; i < 3; i++) hold(r->act[i].e);
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) {
        Entity *e = l->items[i];
        if (!e || participant(r, e)) continue;
        int fid = tny_moves_fighter(e);
        int body = is_player(e) ? !revive_is_down(e) : fid >= 0 && e->hp > 0 && fid != TF_A16 && fid != TF_SUPER13;
        if (body && r->nheld < MAX_HELD) {
            hold(e);
            r->hx[r->nheld] = e->x;
            r->hy[r->nheld] = e->y;
            r->held[r->nheld++] = e;
        }
    }
    for (int i = 0; i < l->count; i++) {            // a held player's state may have ended his beam: then freeze
        Entity *e = l->items[i];
        if (e && !participant(r, e) && flies(e)) freeze(r, e);
    }
    ram.held = r->nheld;
    ram.frozen = r->nfrz;
}
static void thaw_world(Run *r) {
    unfreeze_all(r);
    for (int i = 0; i < r->nheld; i++) {
        Entity *e = r->held[i];
        if (!e || !in_list(e)) continue;
        if (e->x != r->hx[i] || e->y != r->hy[i]) ram.moved++;
        release(e);
    }
    r->nheld = 0;
}

static void pose_set(Actor *a, int kind, int f0, int f1, int n) {
    a->kind = (u8)kind; a->f0 = (u8)f0; a->f1 = (u8)f1; a->pose_t = 0; a->pose_n = (s16)n;
}

// ---- the runner's projectiles ----------------------------------------------------------------------------------------
static const Desc *proj_frame(const CProj *p) {
    switch (p->art) {
    case 0: return &s13_orb_s[(p->timer >> 2) % 3];
    case 1: return &kr_kiblast[((p->vx < 0) ? 2 : 3) * 4 + ((p->timer >> 2) & 3)];   // blue ki: left / right frames
    case 2: return &s13_orb_m[(p->timer >> 2) & 1];
    case 3: return &s13_orb_l[(p->timer >> 2) & 1];
    case 4: return &kr_disc[(p->timer >> 1) & 3];
    default: { int f = p->timer < 4 ? 0 : 1 + (p->timer - 4) / 6; return &a16_boom[f > 3 ? 3 : f]; }
    }
}
static void proj_update(CProj *p) {
    p->timer++;
    if (p->kind == PK_BOOM) {
        if (p->timer >= 22) { EntityList_Remove(g_SceneTable[1], p, 0); return; }
    } else {
        p->x += p->vx;
        p->y += p->vy;
    }
    p->desc = proj_frame(p);
    ProjBase_UpdateBox(p);
}
static void untrack(CProj *p) {
    Run *r = (Run *)ram.run;
    if (r && ram.magic == RUN_MAGIC) for (int i = 0; i < MAX_SHOTS; i++) if (r->shot[i] == p) r->shot[i] = 0;
}
static void proj_destroy(CProj *p, int free_it, u32 a, u32 b) {
    untrack(p);
    ((void (*)(CProj *, int, u32, u32))PROJ_DESTROY)(p, free_it, a, b);
}
static CProj *proj_new(int kind, int art, s32 x, s32 y) {
    static const u16 vram_of[6] = { 0x100, 0x100, 0x400, 0x1000, 0x100, 0x400 };
    int vram = vram_of[art < 6 ? art : 5];
    if (ObjVram_Largest(g_EntityList) < vram + 0x100) return 0;   // keep room for the bodies
    CProj *p = Arena_Alloc(sizeof *p);
    if (!p) return 0;
    memset(p, 0, sizeof *p);
    ProjBase_Init(p, vram, 0);
    u32 base = (u32)p->vt;
    const u32 *orig = (const u32 *)PROJ_VTABLE;
    for (int i = 0; i < 24; i++) p->vt[i] = (PROJ_VTABLE + orig[i]) - base;
    p->vt[0] = (u32)proj_destroy - base;
    p->vt[2] = (u32)proj_update - base;
    p->vt[9] = PROJ_MESSAGE - base;
    p->vt[10] = (u32)proj_update - base;
    p->vtable = p->vt;
    p->kind = (u8)kind;
    p->art = (u8)art;
    p->x = x << 8;
    p->y = y << 8;
    p->desc = proj_frame(p);
    p->prio = 0x28;
    ProjBase_UpdateBox(p);
    EntityList_Add(g_SceneTable[1], (Entity *)p);
    return p;
}
static void boom(s32 x, s32 y) { proj_new(PK_BOOM, 5, x, y); }

// ---- damage ---------------------------------------------------------------------------------------------------------
// One blow of fighter `from` on the target, `w10` tenths of his own punch (melee) or ki blast (ranged): the game's
// damage formula with his combat bytes (players: their party entry's; kits: their tier stat), the defender's
// defence, the co-op multiplier on players, then the target's own OnHit (HP, the number, a player's revive / KO
// rules). Only the target: nothing is broadcast.
static void deal(Run *r, int from, s32 w10, int ranged) {
    Actor *a = &r->act[from], *t = &r->act[2];
    if (a->hercule) { r->act[!from].bank += (s16)w10; return; }   // the showman poses; the work is his partner's
    w10 += a->bank;
    a->bank = 0;
    if (w10 <= 0 || down(t->e)) return;
    Entity *te = t->e, *ae = a->e;
    HitMsg m;
    memset(&m, 0, sizeof m);
    Entity_GetBox(m.box, te);
    m.type = (u8)(ram.side ? 0 : 1);
    m.facing = ae->facing;
    m.mult = w10 * 0x400 / 10;
    m.attack = ranged ? atk_ki : atk_punch;
    m.attacker = ae;
    m.source = ae;
    s32 before = hp_of(te);
    if (is_player(te)) te->facing = (u8)(ae->facing ^ 1);   // as the players' own message handler
    damage_calc_hook(te, (Hit *)((u8 *)&m + 0x14));
    ((int (*)(Entity *, u8 *))vslot(te, 14))(te, (u8 *)&m + 0x24);
    if (in_list(te) && !down(te)) hold(te);          // a player's OnHit starts his hurt state: back to the hold
    ram.dmg[from] += before - hp_of(te);
    ram.hits++;
}

// ---- beams -----------------------------------------------------------------------------------------------------------
// The players' / kits' beam object with the attacker as owner; its update runs with the owner's hit type made one
// nobody takes (3: players take 0 and 2, enemies 1 and 2), so it only shows; the runner deals the damage when its
// tip reaches the target, and caps its length there (or just past him: a beam through a partner)
static void beam_update(Beam *b, int run) {
    Run *r = (Run *)ram.run;
    BeamRec *br = 0;
    if (r) for (int i = 0; i < 2; i++) if (r->beam[i].b == b) br = &r->beam[i];
    u8 save = E8(b->owner, 0x16C);
    if (b->owner) E8(b->owner, 0x16C) = 3;
    ((void (*)(Beam *, int))BEAM_UPDATE)(b, run);
    if (b->owner) E8(b->owner, 0x16C) = save;
    if (!br || !r || b->state != 1) return;
    Entity *t = r->act[2].e;
    s32 tx, ty, d;
    centre(t, &tx, &ty);
    d = b->dx ? (tx - b->x) * b->dx : (ty - b->y) * b->dy;   // along the beam
    s32 cap = d + (br->through ? 48 : 4);
    if (cap < 8) cap = 8;
    if (b->len > cap) { b->len = cap; b->impact = 1; }
    if (!br->done && b->len >= d - 8) {
        br->done = 1;
        deal(r, br->from, br->w10, 1);
        boom(tx, ty);
        camera_shake_safe(16, 70);
        sfx(SFX_BOOM);
    }
}
static void beam_fire(Run *r, int from, s32 w10, int frames, int through) {
    BeamRec *br = !r->beam[0].b ? &r->beam[0] : !r->beam[1].b ? &r->beam[1] : 0;
    Entity *e = r->act[from].e;
    if (!br) return;
    s32 box[4], pos[2];
    Entity_GetBox(box, e);
    pos[0] = (box[0] + box[2]) >> 1;
    pos[1] = (box[1] + box[3]) >> 1;
    Beam *b = Beam_Init(0, pos, 0, e);
    if (!b) { deal(r, from, w10, 1); return; }     // no room for the beam: the blow still lands
    b->mult = 0x400;
    const s32 *vt = (const s32 *)BEAM_VTABLE;
    for (int i = 0; i < 16; i++) br->vt[i] = BEAM_VTABLE + vt[i] - (u32)br->vt;
    br->vt[10] = (u32)beam_update - (u32)br->vt;
    b->vtable = br->vt;
    br->b = b;
    br->from = (u8)from;
    br->done = 0;
    br->through = (u8)through;
    br->w10 = (s16)w10;
    br->left = (s16)frames;
    EntityList_Add(g_EntityList, (Entity *)b);
    int f = e->facing & 3;
    Entity_BeamPoint(pos, e);
    pos[0] += BEAM_OFFSETS[f * 2];
    pos[1] += BEAM_OFFSETS[f * 2 + 1];
    Beam_Fire(b, f, pos);
    sfx(SFX_LAUNCH);
}
static void beams_frame(Run *r) {
    for (int i = 0; i < 2; i++) {
        BeamRec *br = &r->beam[i];
        if (!br->b) continue;
        if (!in_list(br->b)) { br->b = 0; continue; }
        if (br->left > 0 && --br->left == 0) {
            if (!br->done) { br->done = 1; deal(r, br->from, br->w10, 1); }   // it never reached him: it lands anyway
            Beam_End(br->b);
        }
    }
}
// At the end: a beam still ending goes on with the stock vtable (its copy lives in the working block)
static void beams_gone(Run *r) {
    for (int i = 0; i < 2; i++) {
        Beam *b = r->beam[i].b;
        if (b && in_list(b)) {
            if (b->state != 2) Beam_End(b);
            if (b->vtable == r->beam[i].vt) b->vtable = (u32 *)BEAM_VTABLE;
        }
        r->beam[i].b = 0;
    }
}

// ---- shots (ki blasts, orbs, the disc): damage when they reach him ----------------------------------------------------
static void shoot(Run *r, int from, int kind, int art, s32 w10) {
    Entity *e = r->act[from].e, *t = r->act[2].e;
    s32 x, y, tx, ty;
    centre(e, &x, &y);
    centre(t, &tx, &ty);
    x += (e->facing == 2 ? -12 : e->facing == 3 ? 12 : 0);
    CProj *p = 0;
    int slot = -1;
    for (int i = 0; i < MAX_SHOTS && slot < 0; i++) if (!r->shot[i]) slot = i;
    if (slot >= 0) {
        p = proj_new(kind, art, x, y);
        if (!p && art == 3) p = proj_new(kind, art = 2, x, y);   // no room for the large orb: the medium one
    }
    if (!p) { deal(r, from, w10, 1); return; }     // no room: the blow lands without its art
    s32 speed = kind == PK_BALL ? BALL_SPEED : kind == PK_DISC ? DISC_SPEED : SHOT_SPEED;
    set_len((tx - x) << 8, (ty - y) << 8, speed, &p->vx, &p->vy);
    p->from = (u8)from;
    p->w10 = (s16)w10;
    p->fuse = 60;
    p->size_class = (u8)(art == 3 ? 2 : art == 2 ? 1 : 0);
    p->desc = proj_frame(p);
    r->shot[slot] = p;
    ram.shots++;
    sfx(SFX_LAUNCH + Rand_Range(2));
}
static void shots_frame(Run *r) {
    Entity *t = r->act[2].e;
    s32 tx, ty;
    centre(t, &tx, &ty);
    for (int i = 0; i < MAX_SHOTS; i++) {
        CProj *p = r->shot[i];
        if (!p) continue;
        s32 dx = (p->x >> 8) - tx, dy = (p->y >> 8) - ty;
        int reach = p->size_class == 2 ? 14 : p->size_class == 1 ? 10 : 6;
        if (dx * dx + dy * dy > reach * reach && --p->fuse > 0) continue;
        r->shot[i] = 0;
        deal(r, p->from, p->w10, 1);
        boom(p->x >> 8, p->y >> 8);
        if (p->size_class == 2) { boom(tx - 14, ty + 6); boom(tx + 14, ty + 6); boom(tx, ty - 12); camera_shake_safe(24, 100); }
        else if (p->size_class == 1) { boom(tx, ty - 8); camera_shake_safe(12, 70); }
        else camera_shake_safe(4, 30);
        sfx(p->size_class ? SFX_BOOM + Rand_Range(2) : SFX_HIT + Rand_Range(3));
        Entity *from = r->act[p->from].e;
        if (in_list(p)) EntityList_Remove(g_SceneTable[1], p, 0);
        if (!down(t) && r->act[2].knock_t == 0) { face(t, from); pose_set(&r->act[2], CP_HURT, 0, 1, 6); r->act[2].flash = 4; }
    }
}
static void shots_gone(Run *r) {
    for (int i = 0; i < MAX_SHOTS; i++) {
        CProj *p = r->shot[i];
        r->shot[i] = 0;
        if (!p) continue;
        deal(r, p->from, p->w10, 1);                 // still flying at the end: it lands
        if (in_list(p)) EntityList_Remove(g_SceneTable[1], p, 0);
    }
}

// ---- flashes ----------------------------------------------------------------------------------------------------------
// White: every colour of the body to the palette's brightest (the entity's +0x24 colour remap, as revive.c's glow)
static void white_build(Run *r) {
    int best = 1, bv = -1;
    for (int i = 1; i < 256; i++) {
        u16 c = OBJ_PALETTE[i];
        int v = (c & 31) + ((c >> 5) & 31) + ((c >> 10) & 31);
        if (v > bv) { bv = v; best = i; }
    }
    r->white[0] = 0;
    for (int i = 1; i < 256; i++) r->white[i] = (u8)best;
    r->white_ok = 1;
}
static void screen_frame(Run *r) {
    if (r->screen_t <= 0) return;
    r->screen_t--;
    REG_BLDCNT = 0x00BF;                            // brighten every layer
    REG_BLDY = (u16)(r->screen_n ? 16 * r->screen_t / r->screen_n : 0);
    if (!r->screen_t) { REG_BLDCNT = r->bldcnt; REG_BLDY = 0; }
}

// ---- the step table -----------------------------------------------------------------------------------------------------
static void go(Run *r, Actor *a, int i, const TComboStep *s, int instant) {
    Entity *e = a->e, *t = r->act[2].e;
    s32 x, y;
    if (i == 2) {                                   // the target: away from fighter 0
        s32 sx = r->act[0].e->x < e->x ? 1 : -1;
        x = (e->x >> 8) + sx * s->a;
        y = (e->y >> 8) + s->b;
    } else {                                        // from where the target stands (or is being moved to)
        Actor *ta = &r->act[2];
        s32 tx = ta->go_n > 0 ? ta->x1 : t->x, ty = ta->go_n > 0 ? ta->y1 : t->y;
        x = (tx >> 8) + a->sign * s->a;
        y = (ty >> 8) + s->b;
    }
    clamp_floor(&x, &y);
    if (!walkable(e, x, y)) {                       // a rock: no closer than the walkable floor allows
        for (int k = 0; k < 8 && !walkable(e, x, y); k++) { x = (x + (e->x >> 8)) / 2; y = (y + (e->y >> 8)) / 2; }
        if (!walkable(e, x, y)) { x = e->x >> 8; y = e->y >> 8; }
    }
    if (instant || s->c <= 0) {
        s32 cx, cy;
        if (instant) { centre(e, &cx, &cy); boom(cx, cy); }
        e->x = x << 8;
        e->y = y << 8;
        a->go_n = 0;
        if (instant) { centre(e, &cx, &cy); boom(cx, cy); sfx(SFX_LAUNCH); }
        return;
    }
    a->x0 = e->x; a->y0 = e->y;
    a->x1 = x << 8; a->y1 = y << 8;
    a->go_t = 0;
    a->go_n = s->c;
    a->go_kind = (u8)s->d;
    if (s->d != CP_KEEP) { a->kind = (u8)s->d; a->f0 = 0; a->f1 = 3; a->pose_t = 0; a->pose_n = -4; }
    if (x != (e->x >> 8)) e->facing = x < (e->x >> 8) ? 2 : 3;
}

static void do_step(Run *r, const TComboStep *s) {
    if (s->op == CO_SHAKE) { camera_shake_safe(s->a, s->b); return; }
    if (s->op == CO_SOUND) { sfx(s->a); return; }
    if (s->op == CO_SCREEN) { r->screen_t = r->screen_n = s->a; if (!r->screen_t) { REG_BLDCNT = r->bldcnt; REG_BLDY = 0; } return; }
    if (s->op == CO_CAPTION) { tny_caption(tny_combo_name(ram.combo)); return; }
    for (int i = 0; i < 3; i++) {
        if (!(s->who & (1 << i))) continue;
        Actor *a = &r->act[i];
        Entity *e = a->e, *t = r->act[2].e;
        switch (s->op) {
        case CO_GO: go(r, a, i, s, 0); break;
        case CO_TELE: go(r, a, i, s, 1); break;
        case CO_POSE: pose_set(a, s->a, s->b, s->c > s->b ? s->c : s->b, s->d); break;
        case CO_FACE:
            if (i == 2) face(e, r->act[s->a == 1].e);
            else face(e, t);
            if (s->a == 2) e->facing ^= 1;
            break;
        case CO_PUNCH: {
            face(e, t);
            pose_set(a, s->b ? s->b : CP_PUNCH, 0, 3, 8);
            deal(r, i, s->a, 0);
            if (a->hercule) { sfx(SFX_HERCULE); break; }   // the showman's swing: his shout, no blow (banked)
            s32 tx, ty;
            centre(t, &tx, &ty);
            boom(tx - a->sign * 6, ty - 4);
            if (!down(t)) { face(t, e); pose_set(&r->act[2], CP_HURT, 0, 1, 6); r->act[2].flash = 3; }
            camera_shake_safe(4, 30);
            sfx(SFX_HIT + Rand_Range(3));
            break;
        }
        case CO_BLAST:
            face(e, t);
            pose_set(a, CP_KI, 0, 1, 6);
            if (a->hercule) { deal(r, i, s->a, 1); break; }   // banked: the pose only
            shoot(r, i, PK_SHOT, s->b ? 1 : 0, s->a);
            break;
        case CO_BALL:
            face(e, t);
            pose_set(a, CP_KI, 1, 1, 0);
            if (a->hercule) { deal(r, i, s->a, 1); break; }
            shoot(r, i, PK_BALL, s->b >= 2 ? 3 : 2, s->a);
            break;
        case CO_DISC:
            face(e, t);
            pose_set(a, CP_KI, 1, 1, 0);
            if (a->hercule) { deal(r, i, s->a, 1); break; }
            shoot(r, i, PK_DISC, 4, s->a);
            break;
        case CO_BEAM:
            face(e, t);
            pose_set(a, CP_BEAM, 1, 2, -8);
            if (a->hercule) { deal(r, i, s->a, 1); break; }   // Hercule's beam pose: nothing comes out of him
            beam_fire(r, i, s->a, s->b > 0 ? s->b : 30, s->c);
            break;
        case CO_FLASH: a->flash = (s16)(s->a > 0 ? s->a : 4); break;
        case CO_KNOCK:
            if (i != 2 || down(e)) break;
            a->knock_t = 1; a->lie = s->a; a->up = s->b > 0 ? s->b : 12;
            e->facing = (e->facing == 1 || e->facing == 3) ? 3 : 2;   // he falls in a side facing (as at the line)
            sfx(SFX_HIT);
            camera_shake_safe(8, 50);
            break;
        default: break;
        }
    }
}

// One frame of an actor: his move, his pose (or the knock-down), his flash
static void actor_frame(Run *r, Actor *a) {
    Entity *e = a->e;
    if (!e || !in_list(e) || down(e)) return;
    if (a->go_n > 0) {
        a->go_t++;
        e->x = a->x0 + (a->x1 - a->x0) * a->go_t / a->go_n;
        e->y = a->y0 + (a->y1 - a->y0) * a->go_t / a->go_n;
        if (a->go_t >= a->go_n) {
            a->go_n = 0;
            if (a != &r->act[2]) {                  // the target keeps his pose (a push, the hurt)
                if (a->go_kind != CP_KEEP) pose_set(a, CP_IDLE, 0, 0, 0);
                face(e, r->act[2].e);
            }
        }
    }
    if (a->knock_t > 0) {                           // the target: fall, lie, get up
        int k = a->knock_t++;
        const u32 *g3 = kind_group(e, CP_FALL);
        int n = g3 ? frames_in(e, g3) : 1;
        if (k <= FALL_FRAMES) put_pose(e, g3, (k - 1) * n / FALL_FRAMES);
        else if (k <= FALL_FRAMES + a->lie) pose_kind(e, CP_LIE, 0);
        else if (k <= FALL_FRAMES + a->lie + a->up) put_pose(e, g3, (FALL_FRAMES + a->lie + a->up - k) * n / a->up);
        else { a->knock_t = 0; pose_set(a, CP_IDLE, 0, 0, 0); pose_kind(e, CP_IDLE, 0); }
        ram.kd++;
    } else {
        int f = a->f0;
        if (a->pose_n > 0) {
            int span = a->f1 - a->f0 + 1;
            f = a->f0 + (a->pose_t * span) / a->pose_n;
            if (f > a->f1) f = a->f1;
        } else if (a->pose_n < 0) {
            int span = a->f1 - a->f0 + 1;
            f = a->f0 + (a->pose_t / -a->pose_n) % span;
        }
        a->pose_t++;
        pose_kind(e, a->kind, f);
    }
    if (a->flash > 0) {
        a->flash--;
        set_tint(e, a->flash > 0 && r->white_ok ? r->white : 0);
    }
}

static void finish(Run *r, int reason) {
    if (r->finished) return;
    r->finished = 1;
    ram.reason = (u8)reason;
    shots_gone(r);
    beams_gone(r);
    if (r->screen_t) { r->screen_t = 0; REG_BLDCNT = r->bldcnt; REG_BLDY = 0; }
    thaw_world(r);
    for (int i = 0; i < 3; i++) {
        Entity *e = r->act[i].e;
        if (!e || !in_list(e)) continue;
        set_tint(e, 0);
        s32 x = e->x >> 8, y = e->y >> 8;
        clamp_floor(&x, &y);
        if (!walkable(e, x, y)) { x = ram.start_pos[i][0]; y = ram.start_pos[i][1]; }   // never off the floor
        e->x = x << 8;
        e->y = y << 8;
        ram.pos[i][0] = (s16)x;
        ram.pos[i][1] = (s16)y;
        if (!down(e)) { release(e); if (i < 2) face(e, r->act[2].e); }
        // a fallen target keeps the hold until his kit's next frame lays him down (fallen(): the hold lives in comboram)
    }
    ram.hp1 = hp_of(r->act[2].e);
    ram.frames = r->t;
    r->cmd.done = 1;
}

// Room: a target against a wall would leave a fighter no spot on his side; he is first driven (10 frames, hurt) to the
// nearest spot along x with ROOM px of floor on either side (the ring's floor, else the map's collision)
#define ROOM 72
static int room_at(Entity *e, s32 x, s32 y) {
    s32 l = x - ROOM, rr = x + ROOM, yy = y;
    if (tny_ring_active() && (l < TNY_RING_X0 + RING_MARGIN || rr > TNY_RING_X1 - RING_MARGIN)) return 0;
    return walkable(e, l, yy) && walkable(e, rr, yy) && walkable(e, x, yy);
}
static void make_room(Run *r) {
    Actor *a = &r->act[2];
    Entity *e = a->e;
    s32 x = e->x >> 8, y = e->y >> 8;
    if (room_at(e, x, y)) return;
    for (s32 d = 8; d <= 160; d += 8)
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            s32 nx = x + sgn * d;
            if (!room_at(e, nx, y)) continue;
            a->x0 = e->x; a->y0 = e->y; a->x1 = nx << 8; a->y1 = e->y;
            a->go_t = 0; a->go_n = 10; a->go_kind = CP_HURT;
            pose_set(a, CP_HURT, 0, 0, 0);
            return;
        }
}

// The director: one frame of the combo (from fighter 0's command, message 10 of the modal)
static void director(Run *r) {
    ram.tick = g_FrameCounter;
    if (r->finished) return;
    for (int i = 0; i < 2; i++) if (down(r->act[i].e)) { finish(r, 3); return; }
    if (down(r->act[2].e)) { finish(r, 2); return; }
    const TComboDef *d = r->def;
    if (r->t == 0) make_room(r);
    while (r->si < d->n && d->steps[r->si].at <= r->t) {
        const TComboStep *s = &d->steps[r->si++];
        if (s->op == CO_END) { finish(r, 1); return; }
        do_step(r, s);
        if (down(r->act[2].e)) { finish(r, 2); return; }
    }
    shots_frame(r);
    beams_frame(r);
    if (down(r->act[2].e)) { finish(r, 2); return; }
    for (int i = 0; i < 3; i++) actor_frame(r, &r->act[i]);
    screen_frame(r);
    r->t++;
    if (r->t > d->len + 120 || r->t > 1800) finish(r, 1);   // a table without its end row
}

// ---- the command (the game's command objects: {vtable, next, done}; slots 0 destroy, 1 start, 2 step, 3 done?,
// 4 "stay" (0 = pop it), 5 unused) -----------------------------------------------------------------------------------------
static void cmd_destroy(Cmd *c, int free_it) { (void)c; (void)free_it; }      // the working block owns it
static void cmd_start(Cmd *c, Entity *e) { (void)c; (void)e; }
static void cmd_step(Cmd *c, Entity *e) { (void)e; director((Run *)((u8 *)c - __builtin_offsetof(Run, cmd))); }
static int cmd_done(Cmd *c) { return (int)c->done; }
static int cmd_zero(Cmd *c) { (void)c; return 0; }

// ---- the contract (tny_combo.h) --------------------------------------------------------------------------------------------
int tny_combo_busy(void) {
    return ram.magic == RUN_MAGIC && ram.busy && (u32)(g_FrameCounter - ram.tick) < RUN_STALE;
}

const char *tny_combo_name(int combo) {
    if (combo < 0 || combo >= COMBO_COUNT || !tny_combo_defs[combo].name) return "Team combo";
    return tny_combo_defs[combo].name;
}

static int alive_body(const Entity *e) { return e && in_list(e) && !down(e); }

int tny_combo_start(int combo, Entity *a, int a_id, Entity *b, int b_id, Entity *target, int side) {
    ram_init();
    ram.fail = 0;
    if (tny_combo_busy()) { ram.fail = 1; return 0; }
    if (combo < 0 || combo >= COMBO_COUNT || !tny_combo_defs[combo].n) { ram.fail = 2; return 0; }
    if (!alive_body(a) || !alive_body(b) || !alive_body(target) || a == b || a == target || b == target) { ram.fail = 3; return 0; }
    if (scene_running() || *CMD_BATCH) { ram.fail = 4; return 0; }
    if (tny_combo_lead(a_id, b_id)) { Entity *te = a; a = b; b = te; int ti = a_id; a_id = b_id; b_id = ti; }
    Run *r = Arena_Alloc(sizeof *r);
    if (!r) { ram.fail = 5; return 0; }
    memset(r, 0, sizeof *r);
    hold_init();
    ram.run = r;
    ram.busy = 1;
    ram.tick = g_FrameCounter;
    ram.combo = (u8)combo;
    ram.side = (u8)side;
    ram.a_id = (u8)a_id;
    ram.b_id = (u8)b_id;
    ram.ent[0] = a; ram.ent[1] = b; ram.ent[2] = target;
    ram.starts++;
    r->def = &tny_combo_defs[combo];
    Entity *ents[3] = { a, b, target };
    for (int i = 0; i < 3; i++) {
        r->act[i].e = ents[i];
        ram.start_pos[i][0] = (s16)(ents[i]->x >> 8);
        ram.start_pos[i][1] = (s16)(ents[i]->y >> 8);
    }
    // sides: fighter 0 keeps the side of the target he stands on, fighter 1 takes the other
    r->act[0].sign = a->x < target->x ? -1 : 1;
    r->act[1].sign = (s8)-r->act[0].sign;
    r->act[2].sign = (s8)-r->act[0].sign;
    r->act[0].hercule = (u8)is_hercule(a_id);
    r->act[1].hercule = (u8)is_hercule(b_id);
    if (r->act[0].hercule && r->act[1].hercule) r->act[0].hercule = r->act[1].hercule = 0;   // (never: no such pair)
    u32 base = (u32)r->cmd_vt;
    r->cmd_vt[0] = (u32)cmd_destroy - base;
    r->cmd_vt[1] = (u32)cmd_start - base;
    r->cmd_vt[2] = (u32)cmd_step - base;
    r->cmd_vt[3] = (u32)cmd_done - base;
    r->cmd_vt[4] = (u32)cmd_zero - base;
    r->cmd_vt[5] = (u32)cmd_zero - base;
    r->cmd.vtable = r->cmd_vt;
    return 1;
}

// The frame tick (input poll): a started combo runs here, the whole of it inside the command modal (whose own frames
// call this again: busy 2 keeps them out)
void tny_combo_run_frame(void) {
    if (ram.magic != RUN_MAGIC || !ram.busy) return;
    if ((u32)(g_FrameCounter - ram.tick) >= RUN_STALE) { ram.busy = 0; ram.run = 0; return; }   // a reset / a load
    if (ram.busy != 1) return;
    Run *r = (Run *)ram.run;
    ram.busy = 2;
    for (int i = 0; i < 3; i++) if (!alive_body(r->act[i].e)) { ram.busy = 0; ram.run = 0; ram.fail = 6; Arena_Free(r); return; }
    ram.hp0 = hp_of(r->act[2].e);
    ram.dmg[0] = ram.dmg[1] = 0;
    ram.hits = ram.moved = 0;
    ram.shots = ram.kd = 0;
    r->bldcnt = REG_BLDCNT;
    white_build(r);
    freeze_world(r);
    for (int i = 0; i < 2; i++) face(r->act[i].e, r->act[2].e);
    face(r->act[2].e, r->act[0].e);
    if (E32(r->act[0].e, 0x150)) { finish(r, 3); }  // (never in a fight: he already has a script command)
    else {
        Cmd_Begin(CMD_BATCH);
        Cmd_Queue(CMD_BATCH, &r->cmd, r->act[0].e);
        Cmd_Commit(CMD_BATCH);                      // the modal: until the director's command is done
    }
    finish(r, 3);                                   // (already done when it played out)
    ram.runs++;
    ram.run = 0;
    ram.busy = 0;
    Arena_Free(r);
}

// Test command 33 (docs/ids.md): arg = op << 24 | ...
//   op 0  combo | a << 8 | b << 16, arg2 = target | side << 8: start combo `combo` (0xFF: the pair's own, tny_combo_of)
//         with a, b on the target. Who: 0 player 1, 1 player 2, 2 + k the k-th kit fighter in the entity list.
//         The pair's ids: a player's pick on the ring's floor (tny_floor_picks), else his party slot's fighter as the
//         ui's weak default maps them; a kit's fighter id.
//   op 1  clear the counters (runs, starts, the last combo's numbers)
static Entity *who_ent(int w, int *id) {
    *id = -1;
    if (w == 0 || w == 1) {
        Entity *e = w ? coop.p2 : g_PlayerObject;
        u8 ids[2]; Entity *ents[2];
        int n = tny_floor_picks(ids, ents);
        for (int i = 0; i < n; i++) if (ents[i] == e) *id = ids[i];
        if (*id < 0) {
            static const u8 by_slot[9] = { TF_GOHAN, TF_PICCOLO, TF_VEGETA, TF_TRUNKS, TF_GOKU, TF_P_HERCULE, TF_KRILLIN, TF_P_A16, TF_SSJ2_GOHAN };
            int slot = w ? coop.p2_slot : g_Party->active_slot;
            *id = slot >= 0 && slot < 9 ? by_slot[slot] : TF_GOHAN;
        }
        return e;
    }
    EntityList *l = g_EntityList;
    int k = w - 2;
    for (int i = 0; i < l->count; i++) {
        Entity *e = l->items[i];
        int f = e ? tny_moves_fighter(e) : -1;
        if (f < 0) continue;
        if (k-- == 0) { *id = f; return e; }
    }
    return 0;
}
void tny_combo_run_test(u32 cmd, u32 arg, u32 arg2) {
    (void)cmd;
    ram_init();
    int op = (int)(arg >> 24);
    if (op == 0) {
        int ai, bi, ti;
        Entity *a = who_ent((int)((arg >> 8) & 0xFF), &ai), *b = who_ent((int)((arg >> 16) & 0xFF), &bi);
        Entity *t = who_ent((int)(arg2 & 0xFF), &ti);
        int combo = (int)(arg & 0xFF);
        if (combo == 0xFF) combo = tny_combo_of(ai, bi);
        if (combo < 0) { ram.fail = 7; return; }
        tny_combo_start(combo, a, ai, b, bi, t, (int)((arg2 >> 8) & 1));
    } else if (op == 1 && !ram.busy) {              // the counters (the hold state stays: a fallen body may sit in it)
        memset((u8 *)&ram + 4, 0, __builtin_offsetof(RunRam, run) - 4);
        ram.shots = ram.kd = 0;
    }
}
