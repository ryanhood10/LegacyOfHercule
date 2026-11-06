// The cinematic helpers (docs/cutscenes.md section 5 "Cine"; Ryan, 2026-11-06: "a big bonus for animations like energy
// blasts, a fight thats NOT basic, like a REALLY cool cutscene fight"). Script steps alone make stiff fights, so the
// moves a scene needs are played here, by C, from one bridge call: `CoopCall 37, op, b, c` (cine.h has the ops).
//
// How a move runs (the team combos' recipe, tny_combo_run.c; docs/engine.md "The command modal"):
//   * A blocking op builds a working block on the heap and queues one command (the game's own {vtable, next, done}
//     objects) on a body whose command list is empty: outside a script batch that runs the command modal at once (the
//     script waits, as for a walk), inside a batch it runs with the batch's walks at the commit. The modal sends
//     message 10 to every entity each frame: our command's step is the "director", one frame of the op's timeline.
//   * The cast (body A, body B) is put in a hold control state whose animate does nothing, so the poses written each
//     frame (+0x4C the group, +0x14 the frame) stay. A pose is only ever a group the body's record has (group_of, a
//     fallback list per pose; a missing group is a null descriptor and a heap overwrite). At the end a body goes back to
//     its idle state, or stays held in the pose the op leaves (lying, kneeling) until a rise, CINE_END, or, for a
//     player, the first frames of live play (the hold state's update only runs when the world does).
//   * Effects are the game's own: the animated effect object (0x080135E0: the hit star, the white ring, the fire burst,
//     the dust, the smoke, the yellow rings: 0x400 of OBJ VRAM each while they play, none when OBJ VRAM is short), the
//     stock ki blast and Big Bang frames on the stock projectile class, the Kamehameha beam object (owned by player 1
//     with a hit type nobody takes: it only shows), the camera shake, the palette flash (FlashScreenColor's 0x0800BE14).
//     Everything an op makes is gone when it ends.
//   * Start (either pad) skips the op in progress: the timeline is fast-forwarded without effects, so the bodies end
//     exactly where and how they would have.
// Nothing here runs per frame unless an op plays, a charge is held alongside the script or a flash decays (cine_tick).
#include "cine.h"
#include "revive.h"
#include "tourney.h"

#define CINE_MAGIC      0xC14E0001u
#define VRAM_KEEP       0x800       // OBJ VRAM left alone (bytes of the largest free block) when an effect is made
#define PARK            (1024 << 8) // a hidden body waits this far below its spot (off every view)
#define MAX_FX          12
#define MAX_SHOTS       8
#define KN_FLY          16          // frames of a knocked body's flight
#define KN_SLIDE        8           // ... of its slide after landing
#define DASH_FRAMES     14

#define SFX_LAUNCH      0x0D        // a ki blast's launch (+ 1: its second voice)
#define SFX_BOOM        0x14        // an explosion (+ 1)
#define SFX_HIT         0x22        // an impact (+ 0..2)
#define SFX_WHOOSH      26          // the take-off before every stock flight (PlayAudio 26; FlyToPosition)
#define SFX_POWER       55          // the power-up hum (loops: stopped with Sfx_Stop)
#define SFX_BURST       23          // the transformation's burst

// ---- ROM functions and layouts (decompiled; docs/engine.md "Messages, projectiles and beams", "Cine") ---------------
typedef struct Desc { s8 xo, yo; u8 w, h; u32 attr; const void *blob; } Desc;
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
ROM_FN(void *, Fx_Init, (void *mem, const s32 *pos, const void *frames, int n, int speed, int vram, int prio), 0x080135E1);
ROM_FN(void, Pal_Blend, (int r, int g, int b, int level), 0x0800BE15);      // every colour toward r, g, b (0-255) by level / 256
ROM_FN(void *, Pan_Init, (void *mem, const s32 *view, int speed), 0x0800DF1D);   // PanCameraToPosition's modal
ROM_FN(u32, Coroutine_Run, (void *obj), 0x0801F2F5);
ROM_FN(void, Sfx_Stop, (void *channel, const void *sfx), 0x0801FC23);
#define CMD_BATCH           ((u32 *)0x030020F8)
#define PROJ_VTABLE         0x080259C4u
#define PROJ_DESTROY        0x08027A69u
#define PROJ_MESSAGE        0x08013D69u
#define BEAM_VTABLE         0x08025830u
#define BEAM_UPDATE         0x0801D88Bu
#define BEAM_OFFSETS        ((const s32 *)0x08701DF0)
#define IDLE_STATE_VTABLE   0x08024BA0u
#define BODY_VISIBLE        0x08007A85u     // every body class's vtable slot 3 and 4 (player, actor, NPC, enemy)
#define BODY_OAM            0x08007773u
// the game's effect frames (12-byte descriptors, raw 8bpp tiles; rendered and looked at 2026-11-06)
#define FX_STAR             ((const void *)0x086E53DC)   // 4: a yellow four-point star, growing thin
#define FX_RING             ((const void *)0x086E1590)   // 4: a white dot opening into a ring (the beams' impact)
#define FX_BURST            ((const void *)0x086DBC60)   // 3: an orange fire burst (a ki blast's impact)
#define FX_DUST             ((const void *)0x086AF614)   // 4: the tan dust explosion (the scripts' effect, defeats)
#define FX_SMOKE            ((const void *)0x086FF09C)   // 4: a grey puff of smoke (a broken jar)
#define FX_RINGS            ((const void *)0x086DE634)   // 4: yellow rings spreading (the Masenko's impact)
#define KI_FRAMES           ((const Desc *)0x0840E47C)   // the NPC ki blast: 4 facings x 4 frames, 16 x 16
#define BIGBANG_CHARGE      ((const Desc *)0x086DEB6C)   // the Big Bang in the palm: 3 frames, 32 x 32
#define BIGBANG_FLY         ((const Desc *)0x086E0B90)   // ... flying: 4 facings x 4 frames
#define OBJ_PALETTE         ((volatile const u16 *)0x05000200)
#define E8(e, o)            (*(u8 *)((u8 *)(e) + (o)))
#define E16(e, o)           (*(u16 *)((u8 *)(e) + (o)))
#define E32(e, o)           (*(u32 *)((u8 *)(e) + (o)))
extern const unsigned sprite_index_count;   // build/gen/sprites_data.c: the index has no bounds check

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

// ---- state -------------------------------------------------------------------------------------------------------------
typedef struct CtrlObj { u32 *vtable; } CtrlObj;
// Persistent (cineram, NOLOAD, zeroed at a soft reset): the cast, the hold state bodies may sit in between ops, the
// held charge, and what the tests read (tools/headless/cine.txt finds it by the magic)
typedef struct CineRam {
    u32 magic;              // +0x00
    s16 cast[2];            // +0x04 body codes A, B
    s16 px, py;             // +0x08 the point
    u8 busy, cam, armed, last_op;   // +0x0C busy: an op is queued or plays; cam: follow; armed: the demo's trigger
    u8 last_end, aura_on, aura_lvl, flash_n;   // +0x10 how the last op ended (END_*); the held charge; the flash's frames
    s16 aura_who;           // +0x14 its body code
    u16 aura_t;             // +0x16
    u16 ops, done;          // +0x18 blocking ops started / ended
    u16 frames, skips;      // +0x1C frames of the last op; ops skipped with Start
    u16 fx, fx_short;       // +0x20 effects made; effects left out (OBJ VRAM short)
    u16 shots, quiet;       // +0x24 projectiles and beams made; ops refused quietly (no body, busy)
    u8 flash_t, flash_lvl, pend, _p0;   // +0x28 the flash's frames left, its strength; a test call waits
    s16 pend_b, pend_c;     // +0x2C
    u8 pend_op, _p1;        // +0x30
    u16 mark;               // +0x32 test command 50's OBJ VRAM mark (64-byte blocks)
    void *run;              // +0x34 the working block while an op plays
    u8 *aura_white;         // +0x38 the held charge's white remap (arena)
    void *aura_fx[2];       // +0x3C its smoke puffs
    u32 aura_key[2];        // +0x44
    u32 tick;               // +0x4C g_FrameCounter at the director's last frame
    CtrlObj hold;           // +0x50 the hold control state
    u32 hold_vt[12];        // +0x54
    u8 aura_own, _p2[3];    // +0x84 the held charge plays beside the body's own animation (level + 4): no pose, no hold, no hum
    void *own_fx[4];        // +0x88 its smoke puffs and rings
    u32 own_key[4];         // +0x98
} CineRam;
_Static_assert(sizeof(CineRam) <= 0x100, "cineram is 0x100 bytes (coop.ld)");
static CineRam ram __attribute__((section(".cine"), used));

enum { END_NONE, END_PLAYED, END_SKIPPED, END_LOST, END_TIMEOUT, END_QUIET, END_STUCK };
enum { P_IDLE, P_HURT, P_FALL, P_PUNCH, P_KICK, P_KI, P_BEAM, P_CHARGE, P_DASH, P_LIE, P_KNEEL, P_COUNT };

typedef struct CProj {
    u32 *vtable; s32 sort; u8 prio, _p0[3]; u16 tile, size; const Desc *loaded;
    u8 owner_facing, _p1[3];
    const Desc *desc;     // +0x18
    Entity *owner;        // +0x1C
    s32 x, y;             // +0x20 8.8
    s32 box[4];
    s32 sx, sy;           // the ROM layout ends at +0x40
    s32 vx, vy;
    s16 timer, hold;      // hold: frames it waits where it was made (a big blast in the palm)
    u8 kind, face, _p2[2];
    u32 vt[24];
} CProj;
enum { PK_KI = 1, PK_BIG, PK_GHOST };

typedef struct Cmd { u32 *vtable; void *next; u32 done; } Cmd;
typedef struct Act {
    Entity *e;
    s32 x, y;               // where he is (8.8); the entity is put there every frame
    s32 mx0, my0, mx1, my1; // a move: from, to
    s16 mt, mn, arc, lift;  // its frame, length, arc height (px), the lift now
    s16 pt, pn;             // the pose's timer; pn > 0 frames to run f0..f1 once, 0 hold f0, < 0 loop period
    u8 pose, f0, f1, hidden;
    s16 flash;              // frames of white left
    s16 kt;                 // the knock-down's frame (0 none)
    s16 dt;                 // the dash's frame (0 none)
    s32 dx, dy;             // the dash's spot (8.8); the knock-down's last spot
    u8 kend, endpose, ghost_on, dface;   // how a knock-down ends; the pose he keeps after the op; an afterimage; facing after a dash
    u8 shown, shown_f, shown_face, kn;   // the pose last written (its kind + 1, frame, facing); the fall's frames
    const u32 *kg;          // the fall's group
    CProj *ghost;
} Act;
typedef struct BeamRec { Beam *b; s32 cap; u32 vt[16]; } BeamRec;
typedef struct Run {
    Cmd cmd;
    u32 cmd_vt[6];
    Act act[2];
    void (*step)(struct Run *);
    s32 p[6];               // the op's own numbers
    s32 tx, ty;             // B as a spot (px): his centre, or the point
    s32 cx, cy;             // an exchange: the spot the pair fights round (px)
    u16 t, max;
    u8 op, finished, quiet, started, has_b, phase, hits, hum;
    s16 t0, end;        // a phase's first frame; how an exchange ends
    void *fx[MAX_FX];
    CProj *shot[MAX_SHOTS];
    s16 shot_t[MAX_SHOTS];
    BeamRec beam[2];
    u8 white[256];
} Run;

// ---- helpers --------------------------------------------------------------------------------------------------------------
static void ram_init(void) {
    if (ram.magic == CINE_MAGIC) return;
    memset(&ram, 0, sizeof ram);
    ram.magic = CINE_MAGIC;
    ram.cast[0] = ram.cast[1] = CINE_NOBODY;
}
static int in_list(const void *e) {
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) if (l->items[i] == e) return 1;
    return 0;
}
static void sfx(int id) { Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + id * 12, 0x40, 0); }
static int is_body(const Entity *e) {
    if (!e) return 0;
    u32 v = (u32)e->vtable;
    if (v < 0x02000000u || v >= 0x0A000000u || (v & 3)) return 0;
    return (u32)vslot(e, 3) == BODY_VISIBLE && (u32)vslot(e, 4) == BODY_OAM;
}
// A body by its code (cine.h): never a pointer kept from an earlier call
static Entity *body_of(int code) {
    Entity *p1 = g_PlayerObject, *p2 = coop.p2;
    if (p1 && (!in_list(p1) || !is_body(p1))) p1 = 0;
    if (p2 && (!in_list(p2) || !is_body(p2))) p2 = 0;
    if (code == CINE_P1) return p1;
    if (code == CINE_P2) return p2;
    if (code <= CINE_SLOT0 && code > CINE_SLOT0 - PARTY_SLOTS) {
        int slot = CINE_SLOT0 - code;
        if (p1 && g_Party->active_slot == slot) return p1;
        if (p2 && coop.p2_slot == slot) return p2;
        return 0;
    }
    if (code < 7 || (unsigned)code >= sprite_index_count) return 0;
    const void *rec = Character_GetSpriteRecord(code);
    if (!rec) return 0;
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) {
        Entity *e = l->items[i];
        if (e && e != g_PlayerObject && e != coop.p2 && is_body(e) && e->sprite_record == rec) return e;
    }
    return 0;
}
static void centre(const Entity *e, s32 *x, s32 *y) {
    s32 b[4];
    Entity_GetBox(b, e);
    *x = (b[0] + b[2]) >> 1;
    *y = (b[1] + b[3]) >> 1;
}
static void box_size(const Entity *e, s32 *w, s32 *h) {
    s32 b[4];
    Entity_GetBox(b, e);
    *w = b[2] - b[0];
    *h = b[3] - b[1];
    if (*w < 8) *w = 8;
    if (*h < 8) *h = 8;
}
static int facing_to(s32 dx, s32 dy) {
    s32 ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    if (ax * 2 >= ay) return dx < 0 ? 2 : 3;     // about level: a side facing (the side poses read best)
    return dy < 0 ? 1 : 0;
}
static void face_spot(Entity *e, s32 x, s32 y) {
    s32 ex, ey;
    centre(e, &ex, &ey);
    if (ex == x && ey == y) return;
    e->facing = (u8)facing_to(x - ex, y - ey);
}
static int walkable(Entity *e, s32 x, s32 y) {
    s32 px[2] = { x, y };
    return x > 8 && y > 8 && Collision_Check((const void *)g_Ctx[4], px, (u8 *)e + 0x154);
}
// The whole body in the camera's view (CAMERA_CELL words 3-6: left, top, right, bottom in world px); not when the
// camera follows the cast
static void clamp_view(const Entity *e, s32 *x, s32 *y) {
    if (ram.cam) return;
    const s32 *cam = CAMERA_CELL;
    s32 b[4], ex = e->x >> 8, ey = e->y >> 8;
    Entity_GetBox(b, e);
    if (*x + b[0] - ex < cam[3] + 10) *x = cam[3] + 10 - (b[0] - ex);
    if (*x + b[2] - ex > cam[5] - 10) *x = cam[5] - 10 - (b[2] - ex);
    if (*y + b[1] - ey < cam[4] + 30) *y = cam[4] + 30 - (b[1] - ey);
    if (*y + b[3] - ey > cam[6] - 8) *y = cam[6] - 8 - (b[3] - ey);
}
// A spot (the entity's own x, y in px) he can stand on: in view, on the floor; else pulled back toward (fx, fy), where
// he stands now
static void fit(Entity *e, s32 *x, s32 *y, s32 fx, s32 fy) {
    clamp_view(e, x, y);
    for (int k = 0; k < 6 && !walkable(e, *x, *y); k++) { *x = (*x + fx) / 2; *y = (*y + fy) / 2; }
    if (!walkable(e, *x, *y)) { *x = fx; *y = fy; }
}

// ---- poses (tny_combo_run.c's rules: a group only when the record has it and its frame is drawable) ----------------
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
    for (int f = 0; f < 4; f++) {
        if (grp[f] < 0x08000000u || grp[f] >= 0x0A000000u || (grp[f] & 3)) return 0;
        if (!desc_ok((const Desc *)grp[f])) return 0;
    }
    return grp;
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
// Each pose's group and its stand-ins (docs/sprites.md: 2 hit-stun, 3 the fall, 5 the ki pose, 6-8 attacks, 13 the
// dash, 18 the beam; 11 the players' hurt)
static const s8 kinds[P_COUNT][4] = {
    [P_IDLE] = { 0, -1, -1, -1 },  [P_HURT] = { 2, 11, 0, -1 },  [P_FALL] = { 3, 2, 0, -1 },
    [P_PUNCH] = { 7, 6, 8, 0 },    [P_KICK] = { 8, 6, 7, 0 },    [P_KI] = { 5, 6, 7, 0 },
    [P_BEAM] = { 18, 5, 0, -1 },   [P_CHARGE] = { 5, 0, -1, -1 }, [P_DASH] = { 13, 1, 0, -1 },
    [P_LIE] = { 12, 3, 2, 0 },     [P_KNEEL] = { 3, 2, 0, -1 },
};
static const u32 *kind_group(const Entity *e, int kind) {
    if (kind >= P_COUNT) kind = P_IDLE;
    if (kind == P_CHARGE) {                         // a player layout's group 9: the transformation's first two frames
        const u8 *rec = (const u8 *)e->sprite_record;
        const u32 *g = rec && rec[2] >= 20 ? group_of(e, 9) : 0;
        if (g && frames_in(e, g) >= 2) return g;
    }
    for (int i = 0; i < 4 && kinds[kind][i] >= 0; i++) { const u32 *g = group_of(e, kinds[kind][i]); if (g) return g; }
    return group_of(e, 0);
}
static int has_beam_pose(const Entity *e) { return group_of(e, 18) != 0; }
static void put_pose(Entity *e, const u32 *grp, int f) {
    if (!grp) return;
    int n = frames_in(e, grp);
    if (f >= n) f = n - 1;
    if (f < 0) f = 0;
    if (E32(e, 0x4C) != (u32)grp) E16(e, 0x12) = 0;
    E32(e, 0x4C) = (u32)grp;
    E16(e, 0x14) = (u16)f;
}
static void pose_kind(Entity *e, int kind, int f) {
    if (kind == P_LIE) {                            // the tournament's lying pose (the lying table, else 12 / 3 / 2)
        int lf = 0;
        const u32 *lg = tny_lie_pose(e, &lf);
        if (lg) { put_pose(e, lg, lf); return; }
    }
    const u32 *g = kind_group(e, kind);
    if (kind == P_KNEEL) f = g == group_of(e, 3) ? 1 : 7;      // the fall's second frame (down on a knee), else the hit-stun's last
    if (kind == P_CHARGE && g == group_of(e, 9)) f &= 1;
    put_pose(e, g, f);
}
static void set_tint(Entity *e, const u8 *t) {
    if (E32(e, 0x24) == (u32)t) return;
    E32(e, 0x24) = (u32)t;
    E32(e, 0x20) = 0;                               // drop the cached frame so the next one goes through the remap
}
// White: every colour to the palette's brightest (the entity's +0x24 colour remap)
static void white_build(u8 *w) {
    int best = 1, bv = -1;
    for (int i = 1; i < 256; i++) {
        u16 c = OBJ_PALETTE[i];
        int v = (c & 31) + ((c >> 5) & 31) + ((c >> 10) & 31);
        if (v > bv) { bv = v; best = i; }
    }
    w[0] = 0;
    for (int i = 1; i < 256; i++) w[i] = (u8)best;
}

// ---- the hold state (the kits' stun recipe): animate does nothing, so a written pose stays -------------------------
#define HOLD_FREE 20                                // frames of live play after which a held player stands up
static void hold_enter(CtrlObj *s, Entity *e) { (void)s; E16(e, 0x12) = 0; E8(e, 8) = 0x28; }
static void hold_nop(CtrlObj *s, Entity *e) { (void)s; (void)e; }
// The update only runs when the world does (never inside a scene's modals): a player left lying by a scene that is
// over gets up, so no script can cost him his control
static void hold_update(CtrlObj *s, Entity *e) {
    (void)s;
    if (e != g_PlayerObject && e != coop.p2) return;
    // (an op queued in a batch has played a frame within the second, or its block is lost: then it holds nobody)
    if ((ram.run && (u32)(g_FrameCounter - ram.tick) < 60) || (ram.aura_on && scene_running())) { E16(e, 0x12) = 0; return; }
    if (++E16(e, 0x12) < HOLD_FREE) return;
    set_tint(e, 0);
    Entity_SetAnimation(e, 0);
}
static void hold_init(void) {
    u32 b = (u32)ram.hold_vt;
    const u32 *iv = (const u32 *)IDLE_STATE_VTABLE;
    for (int i = 0; i < 12; i++) ram.hold_vt[i] = (IDLE_STATE_VTABLE + iv[i]) - b;
    ram.hold_vt[0] = (u32)hold_enter - b;
    ram.hold_vt[1] = (u32)hold_nop - b;
    ram.hold_vt[2] = (u32)hold_update - b;
    ram.hold_vt[3] = (u32)hold_nop - b;
    ram.hold.vtable = ram.hold_vt;
}
static int held(const Entity *e) { return e && e->ctrl_state == &ram.hold; }
static void hold(Entity *e) { if (e && !held(e)) { hold_init(); Entity_SetCtrlState(e, &ram.hold); } }
static void release(Entity *e) { if (e && in_list(e) && held(e)) { set_tint(e, 0); Entity_SetAnimation(e, 0); } }

// ---- effects ----------------------------------------------------------------------------------------------------------
static void *fx_make(const void *tab, int n, s32 x, s32 y, int speed) {
    int back = speed < 0;                           // a negative speed: drawn behind the bodies standing there
    if (back) speed = -speed;
    if (ObjVram_Largest(g_EntityList) < 0x400 + VRAM_KEEP) { ram.fx_short++; return 0; }
    s32 pos[2] = { x, y };
    u32 *o = Fx_Init(0, pos, tab, n, speed, 0x400, 0x28);
    if (!o) return 0;
    o[1] = (u32)(back ? y - 24 : y + 48);           // the draw order: in front of the bodies beside it, or behind
    EntityList_Add(g_EntityList, (Entity *)o);
    ram.fx++;
    return o;
}
static void fx(Run *r, const void *tab, int n, s32 x, s32 y, int speed) {
    if (r->quiet) return;
    void *o = fx_make(tab, n, x, y, speed);
    if (!o) return;
    for (int i = 0; i < MAX_FX; i++) if (!r->fx[i] || !in_list(r->fx[i])) { r->fx[i] = o; return; }
}
static void fx_gone(Run *r) {
    for (int i = 0; i < MAX_FX; i++) {
        if (r->fx[i] && in_list(r->fx[i])) EntityList_Remove(g_EntityList, r->fx[i], 0);
        r->fx[i] = 0;
    }
}
static void shake(Run *r, int frames, int strength) { if (!r || !r->quiet) camera_shake_safe(frames, strength); }
static void snd(Run *r, int id) { if (!r->quiet) sfx(id); }
// The screen's white flash (decays in cine_tick, which runs every frame, in the modal too)
static void flash(Run *r, int level, int frames) {
    if (r && r->quiet) return;
    if (level > 255) level = 255;
    ram.flash_lvl = (u8)level;
    ram.flash_t = ram.flash_n = (u8)(frames > 0 ? frames : 1);
}
static void flash_off(void) {
    if (ram.flash_t) Pal_Blend(255, 255, 255, 0);
    ram.flash_t = 0;
}

// ---- projectiles and afterimages (the ROM projectile class with our update, as the combos' runner's) -----------------
static void untrack(CProj *p) {
    Run *r = (Run *)ram.run;
    if (!r || ram.magic != CINE_MAGIC) return;
    for (int i = 0; i < MAX_SHOTS; i++) if (r->shot[i] == p) r->shot[i] = 0;
    for (int i = 0; i < 2; i++) if (r->act[i].ghost == p) r->act[i].ghost = 0;
}
static void proj_destroy(CProj *p, int free_it, u32 a, u32 b) {
    untrack(p);
    ((void (*)(CProj *, int, u32, u32))PROJ_DESTROY)(p, free_it, a, b);
}
static const Desc *proj_frame(const CProj *p) {
    static const u8 seq[4] = { 0, 1, 2, 1 };
    int f = (p->timer >> 2) & 3;
    if (p->kind == PK_BIG) {
        if (p->hold > 0 || (!p->vx && !p->vy)) return &BIGBANG_CHARGE[seq[(p->timer >> 3) & 3]];
        return &BIGBANG_FLY[(p->face & 3) * 4 + seq[(p->timer >> 3) & 3]];
    }
    return &KI_FRAMES[(p->face & 3) * 4 + f];
}
static void proj_update(CProj *p) {
    p->timer++;
    if (p->kind == PK_GHOST) return;
    if (p->hold > 0) p->hold--;
    else { p->x += p->vx; p->y += p->vy; }
    p->desc = proj_frame(p);
    ProjBase_UpdateBox(p);
}
// An afterimage is the body as he was last drawn, in OBJ VRAM of its own (his tiles copied when he vanishes, his frame's
// descriptor kept), shown every other frame where he stood (the entity draw's recipe, 0x08007A84 / 0x08007772)
static int ghost_vis(CProj *p) {
    const Desc *d = p->desc;
    if (!d || !(p->timer & 1)) return 0;
    s32 left = (p->x >> 8) + d->xo, top = (p->y >> 8) + d->yo;
    const s32 *cam = CAMERA_CELL;
    p->sx = left - cam[7];
    p->sy = top - cam[8];
    p->sort = top + d->h - 1;
    return left < cam[5] && cam[3] < left + d->w && top < cam[6] && cam[4] < top + d->h;
}
static int ghost_draw(CProj *p, u32 **oam) {
    const Desc *d = p->desc;
    if (!d) return 0;
    u32 *o = *oam;
    *oam = o + 2;
    o[0] = (d->attr & 0xFE00FFFFu) | (((u32)p->sx & 0x1FF) << 16) | ((u32)p->sy & 0xFF);
    o[1] = (o[1] & 0xFFFFF000u) | ((u32)(p->prio & 0x3F) >> 4) << 10 | (p->tile & 0x3FF);
    return 0;
}
static void ghost_upload(CProj *p) { (void)p; }    // (its tiles were copied once)
static CProj *proj_new(int kind, s32 x, s32 y, int face, int vram);
// the frame his tiles hold (+0x20, the last one the draw loaded), or 0
static const Desc *drawn_frame(const Entity *e) {
    const Desc *d = (const Desc *)E32(e, 0x20);
    u32 v = (u32)d;
    if (v < 0x08000000u || v >= 0x0A000000u || (v & 3) || !desc_ok(d)) return 0;
    return d;
}
static CProj *ghost_new(Entity *e, s32 x, s32 y) {
    const Desc *d = drawn_frame(e);
    if (!d) return 0;
    unsigned n = (unsigned)d->w * d->h;
    CProj *p = proj_new(PK_GHOST, x, y, e->facing, (int)n);
    if (!p) return 0;
    const volatile u32 *s = (const volatile u32 *)(0x06010000u + (u32)E16(e, 0x0C) * 32);
    volatile u32 *t = (volatile u32 *)(0x06010000u + (u32)p->tile * 32);
    for (unsigned i = 0; i < n >> 2; i++) t[i] = s[i];
    p->desc = d;
    p->loaded = d;
    p->prio = E8(e, 8);
    return p;
}
static CProj *proj_new(int kind, s32 x, s32 y, int face, int vram) {
    if (vram < 0x40) vram = 0x40;                   // (the allocator counts 64-byte blocks)
    if (ObjVram_Largest(g_EntityList) < vram + VRAM_KEEP) { ram.fx_short++; return 0; }
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
    if (kind == PK_GHOST) {
        p->vt[3] = (u32)ghost_vis - base;
        p->vt[4] = (u32)ghost_draw - base;
        p->vt[5] = (u32)ghost_upload - base;
    }
    p->vtable = p->vt;
    p->kind = (u8)kind;
    p->face = (u8)face;
    p->x = x << 8;
    p->y = y << 8;
    p->prio = 0x28;
    if (kind != PK_GHOST) { p->desc = proj_frame(p); ProjBase_UpdateBox(p); }
    EntityList_Add(g_EntityList, (Entity *)p);
    return p;
}
static void proj_del(CProj *p) { if (p && in_list(p)) EntityList_Remove(g_EntityList, p, 0); }
static u32 isqrt(u32 v) {
    u32 r = 0, bit = 1u << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; } else r >>= 1;
        bit >>= 2;
    }
    return r;
}
static s32 dist(s32 dx, s32 dy) { return (s32)isqrt((u32)(dx * dx + dy * dy)); }
// A shot from (x, y) that reaches (tx, ty) px in `n` frames, after `hold` frames where it is; returns its slot or -1
static int shoot(Run *r, int kind, s32 x, s32 y, s32 tx, s32 ty, int n, int hold) {
    if (r->quiet) return -1;
    int slot = -1;
    for (int i = 0; i < MAX_SHOTS && slot < 0; i++) if (!r->shot[i]) slot = i;
    if (slot < 0) return -1;
    CProj *p = proj_new(kind, x, y, facing_to(tx - x, ty - y), kind == PK_BIG ? 0x400 : 0x100);
    if (!p) return -1;
    if (n < 1) n = 1;
    p->vx = ((tx - x) << 8) / n;
    p->vy = ((ty - y) << 8) / n;
    p->hold = (s16)hold;
    p->desc = proj_frame(p);
    r->shot[slot] = p;
    r->shot_t[slot] = (s16)(n + hold);
    ram.shots++;
    return slot;
}
// shots fly for the frames they were given, then go (the op's own timeline plays the impact)
static void shots_frame(Run *r) {
    for (int i = 0; i < MAX_SHOTS; i++) {
        CProj *p = r->shot[i];
        if (!p) continue;
        if (!in_list(p)) { r->shot[i] = 0; continue; }
        if (--r->shot_t[i] <= 0) { r->shot[i] = 0; proj_del(p); }
    }
}
static void shots_gone(Run *r) {
    for (int i = 0; i < MAX_SHOTS; i++) { CProj *p = r->shot[i]; r->shot[i] = 0; proj_del(p); }
}

// ---- beams: the Kamehameha object, shown only (cellgames.c's held beam, the combos' cap) ----------------------------
// Owned by player 1 with his hit type made one nobody takes while the beam updates (an actor is too short to hold
// the byte, +0x16C), its length capped where the op says
static void beam_update(Beam *b, int run) {
    Entity *own = b->owner;
    u8 save = 0;
    if (own) { save = E8(own, 0x16C); E8(own, 0x16C) = 3; }
    ((void (*)(Beam *, int))BEAM_UPDATE)(b, run);
    if (own) E8(own, 0x16C) = save;
    Run *r = (Run *)ram.run;
    if (!r || ram.magic != CINE_MAGIC || !in_list(b)) return;
    for (int i = 0; i < 2; i++) {
        BeamRec *br = &r->beam[i];
        if (br->b != b || b->state != 1) continue;
        s32 cap = br->cap < 8 ? 8 : br->cap;
        if (b->len > cap) { b->len = cap; b->impact = 1; }
    }
}
static int beam_fire(Run *r, int i, Entity *e, s32 cap) {
    BeamRec *br = &r->beam[i];
    Entity *p1 = g_PlayerObject;
    br->cap = cap;
    if (r->quiet || br->b || !p1 || ObjVram_Largest(g_EntityList) < 0x400 + VRAM_KEEP) return 0;
    s32 box[4], pos[2];
    Entity_GetBox(box, e);
    pos[0] = (box[0] + box[2]) >> 1;
    pos[1] = (box[1] + box[3]) >> 1;
    Beam *b = Beam_Init(0, pos, 0, p1);
    if (!b) return 0;
    b->mult = 0x400;
    const s32 *vt = (const s32 *)BEAM_VTABLE;
    for (int k = 0; k < 16; k++) br->vt[k] = BEAM_VTABLE + vt[k] - (u32)br->vt;
    br->vt[10] = (u32)beam_update - (u32)br->vt;
    b->vtable = br->vt;
    br->b = b;
    EntityList_Add(g_EntityList, (Entity *)b);
    int f = e->facing & 3;
    Entity_BeamPoint(pos, e);
    pos[0] += BEAM_OFFSETS[f * 2];
    pos[1] += BEAM_OFFSETS[f * 2 + 1];
    Beam_Fire(b, f, pos);
    sfx(SFX_LAUNCH);
    ram.shots++;
    return 1;
}
static void beam_end(Run *r, int i) {              // it slides off and removes itself
    Beam *b = r->beam[i].b;
    if (b && in_list(b) && b->state == 1) Beam_End(b);
}
static int beam_live(Run *r, int i) {
    Beam *b = r->beam[i].b;
    if (b && !in_list(b)) b = r->beam[i].b = 0;
    return b != 0;
}
static void beams_gone(Run *r) {
    for (int i = 0; i < 2; i++) {
        Beam *b = r->beam[i].b;
        r->beam[i].b = 0;
        if (b && in_list(b)) EntityList_Remove(g_EntityList, b, 0);
    }
}
// where a beam of e leaves him (px) and the distance along it to a spot
static s32 beam_reach(Entity *e, s32 tx, s32 ty) {
    s32 pos[2];
    int f = e->facing & 3;
    Entity_BeamPoint(pos, e);
    pos[0] += BEAM_OFFSETS[f * 2];
    pos[1] += BEAM_OFFSETS[f * 2 + 1];
    return f == 2 ? pos[0] - tx : f == 3 ? tx - pos[0] : f == 1 ? pos[1] - ty : ty - pos[1];
}

// ---- actors ---------------------------------------------------------------------------------------------------------------
static void pose(Act *a, int kind, int f0, int f1, int n) {
    a->pose = (u8)kind; a->f0 = (u8)f0; a->f1 = (u8)(f1 > f0 ? f1 : f0); a->pt = 0; a->pn = (s16)n;
}
static void move(Act *a, s32 x1, s32 y1, int n, int arc) {
    if (n < 1) n = 1;
    a->mx0 = a->x; a->my0 = a->y; a->mx1 = x1; a->my1 = y1;
    a->mt = 0; a->mn = (s16)n; a->arc = (s16)arc;
}
static void act_centre(const Act *a, s32 *x, s32 *y) {       // his box's centre where he is (not where a hidden body waits)
    centre(a->e, x, y);
    *x += (a->x - a->e->x) >> 8;
    *y += (a->y - a->e->y) >> 8;
}
static void act_face(Act *a, s32 x, s32 y) {
    s32 cx, cy;
    act_centre(a, &cx, &cy);
    if (cx != x || cy != y) a->e->facing = (u8)facing_to(x - cx, y - cy);
}
static void ghost_del(Act *a) { CProj *g = a->ghost; a->ghost = 0; proj_del(g); }
// The dash: he flickers out where he stands, is at the spot the next frame, an afterimage flickering where he was
static void dash(Run *r, Act *a, s32 x, s32 y, int ghost, int face) {
    a->dx = x; a->dy = y;
    a->dt = 1;
    a->ghost_on = (u8)ghost;
    a->dface = (u8)face;
    a->mn = 0;
    a->flash = 2;
    snd(r, SFX_WHOOSH);
}
// The knock-down: an arc to (x, y), the landing (smoke, a thud, a shake), a short slide, then the pose he keeps
static void knock(Run *r, Act *a, s32 x, s32 y, int endpose) {
    Entity *e = a->e;
    s32 dx = x - a->x, dy = y - a->y, d = dist(dx >> 8, dy >> 8);
    a->dx = x; a->dy = y;
    if (d > KN_SLIDE) { x -= dx * KN_SLIDE / d; y -= dy * KN_SLIDE / d; }
    move(a, x, y, KN_FLY, 14);
    a->kt = 1;
    a->kend = (u8)endpose;
    a->dt = 0;
    a->hidden = 0;
    e->facing = (u8)((e->facing == 1 || e->facing == 3) ? 3 : 2);   // he falls and lies in a side facing
    a->kg = kind_group(e, P_FALL);
    a->kn = (u8)(a->kg ? frames_in(e, a->kg) : 1);
    a->shown = 0;
    (void)r;
}
static void kpose(Act *a, const u32 *g, int f) {     // the fall's frame, written when it changes
    if (!g || (a->shown == 0xFF && a->shown_f == f)) return;
    put_pose(a->e, g, f);
    a->shown = 0xFF; a->shown_f = (u8)f;
}
static void act_frame(Run *r, Act *a) {
    Entity *e = a->e;
    if (!e) return;
    if (a->dt > 0) {
        int k = a->dt++;
        if (k <= 4) a->hidden = (u8)(k & 1);
        else if (k == 5) {
            if (a->ghost_on && !r->quiet && !a->ghost) {
                a->ghost = ghost_new(e, a->x >> 8, a->y >> 8);
            }
            a->x = a->dx; a->y = a->dy;
            a->hidden = 1;
            if (a->dface < 4) e->facing = a->dface;
        } else if (k == 6) { a->hidden = 0; a->flash = 3; }
        else if (k >= DASH_FRAMES) { ghost_del(a); a->dt = 0; }
    }
    if (a->mn > 0) {
        a->mt++;
        a->x = a->mx0 + (a->mx1 - a->mx0) * a->mt / a->mn;
        a->y = a->my0 + (a->my1 - a->my0) * a->mt / a->mn;
        a->lift = (s16)(a->arc * 4 * a->mt * (a->mn - a->mt) / (a->mn * a->mn));
        if (a->mt >= a->mn) { a->mn = 0; a->lift = 0; }
    }
    if (a->kt > 0) {
        int k = a->kt++;
        const u32 *g3 = r->quiet ? 0 : a->kg;       // (a skipped op writes no pose but the last)
        int n = a->kn;
        if (k <= KN_FLY) kpose(a, g3, k * 2 > KN_FLY && n > 1 ? 1 : 0);
        else if (k == KN_FLY + 1) {
            s32 cx, cy;
            act_centre(a, &cx, &cy);
            fx(r, FX_SMOKE, 4, cx - 8, cy + 4, 0x4000);
            fx(r, FX_SMOKE, 4, cx + 10, cy + 2, 0x3000);
            shake(r, 10, 50);
            snd(r, SFX_HIT);
            move(a, a->dx, a->dy, KN_SLIDE, 0);
            kpose(a, g3, n > 2 ? 2 : n - 1);
        } else if (k <= KN_FLY + KN_SLIDE) kpose(a, g3, n - 1);
        else { a->kt = 0; a->endpose = a->kend; pose(a, a->kend, 0, 0, 0); }
    } else {
        int f = a->f0, span = a->f1 - a->f0 + 1;
        if (a->pn > 0) { f = a->f0 + (a->pt * span) / a->pn; if (f > a->f1) f = a->f1; }
        else if (a->pn < 0) f = a->f0 + (a->pt / -a->pn) % span;
        a->pt++;
        // written only when it changes (finding a group walks the record), never in a skipped op
        if (!r->quiet && (a->shown != a->pose + 1 || a->shown_f != f || a->shown_face != e->facing)) {
            pose_kind(e, a->pose, f);
            a->shown = (u8)(a->pose + 1); a->shown_f = (u8)f; a->shown_face = e->facing;
        }
    }
    if (a->flash > 0) { a->flash--; set_tint(e, a->flash > 0 && !r->quiet ? r->white : 0); }
    e->x = a->x;
    e->y = a->y - (a->lift << 8) + (a->hidden ? PARK : 0);
}
static int act_busy(const Act *a) { return a->e && (a->dt > 0 || a->kt > 0 || a->mn > 0); }

// ---- the command and the run's life ---------------------------------------------------------------------------------------
static void finish(Run *r, int how) {
    if (r->finished) return;
    r->finished = 1;
    ram.last_end = (u8)how;
    ram.frames = r->t;
    shots_gone(r);
    beams_gone(r);
    fx_gone(r);
    if (r->hum) { r->hum = 0; Sfx_Stop(g_AudioMixer + 0x5C4, SFX_TABLE + SFX_POWER * 12); }
    for (int i = 0; i < 2; i++) {
        Act *a = &r->act[i];
        Entity *e = a->e;
        ghost_del(a);
        if (!e || !in_list(e)) continue;
        set_tint(e, 0);
        e->x = a->x;
        e->y = a->y;
        if (a->endpose == P_IDLE || !r->started) release(e);
        else if (a->kt > 0) pose_kind(e, a->kend, 0);      // (cut short in the air: he is down)
        else pose_kind(e, a->endpose, 0);
    }
    r->cmd.done = 1;
}
static void frame(Run *r) {
    r->step(r);
    if (r->finished) return;
    shots_frame(r);
    for (int i = 0; i < 2; i++) act_frame(r, &r->act[i]);
    r->t++;
    if (r->t > r->max) finish(r, END_TIMEOUT);
}
static void cam_frame(Run *r) {
    if (!ram.cam) return;
    s32 x, y;
    if (!r->act[0].e) return;
    act_centre(&r->act[0], &x, &y);
    if (r->act[1].e) {                              // the middle of the two, where they are now
        s32 bx, by;
        act_centre(&r->act[1], &bx, &by);
        x = (x + bx) / 2; y = (y + by) / 2;
    } else if (r->has_b) { x = (x + r->tx) / 2; y = (y + r->ty) / 2; }
    camera_aim(x, y);
}
static void director(Run *r) {
    ram.tick = g_FrameCounter;
    if (r->finished) return;
    r->started = 1;
    for (int i = 0; i < 2; i++) {
        Act *a = &r->act[i];
        if (a->e && (!in_list(a->e) || !held(a->e))) { a->e = 0; finish(r, END_LOST); return; }   // gone, or taken by other code
    }
    if (!r->quiet && ((g_InputSystem->pressed | (coop.p2 ? coop.input2.pressed : 0)) & KEY_START)) {
        // skipped: the rest of the timeline at once, without its effects
        r->quiet = 1;
        ram.skips++;
        shots_gone(r);
        beams_gone(r);
        fx_gone(r);
        flash_off();
        for (int i = 0; i < 2; i++) ghost_del(&r->act[i]);
        for (int guard = 0; guard < 3000 && !r->finished; guard++) frame(r);
        if (!r->finished) finish(r, END_TIMEOUT);
        ram.last_end = END_SKIPPED;
        return;
    }
    frame(r);
    if (!r->finished) cam_frame(r);
}
static void run_free(Run *r) {
    if (ram.run == r) { ram.run = 0; ram.busy = 0; }
    Arena_Free(r);
}
// The command's slots ({vtable, next, done}: 0 destroy, 1 start, 2 step, 3 done?, 4 "stay" (0 = pop it)). The pop
// (0x0800E078) calls destroy: the run ends and is freed there, queued in a batch or run at once.
static void cmd_destroy(Cmd *c, int free_it) {
    (void)free_it;
    Run *r = (Run *)((u8 *)c - __builtin_offsetof(Run, cmd));
    if (ram.magic != CINE_MAGIC || ram.run != r) return;
    finish(r, END_STUCK);
    ram.done++;
    run_free(r);
}
static void cmd_start(Cmd *c, Entity *e) { (void)c; (void)e; }
static void cmd_step(Cmd *c, Entity *e) { (void)e; director((Run *)((u8 *)c - __builtin_offsetof(Run, cmd))); }
static int cmd_done(Cmd *c) { return (int)c->done; }
static int cmd_zero(Cmd *c) { (void)c; return 0; }

static Run *run_new(int op) {
    ram_init();
    if (ram.run) {
        if (*CMD_BATCH) { ram.quiet++; ram.last_end = END_QUIET; return 0; }   // one op a batch
        ram.run = 0;                                // a block lost with its map: nothing of it is left to free
    }
    Run *r = Arena_Alloc(sizeof *r);
    if (!r) { ram.quiet++; return 0; }
    memset(r, 0, sizeof *r);
    r->op = (u8)op;
    r->max = 1800;
    u32 base = (u32)r->cmd_vt;
    r->cmd_vt[0] = (u32)cmd_destroy - base;
    r->cmd_vt[1] = (u32)cmd_start - base;
    r->cmd_vt[2] = (u32)cmd_step - base;
    r->cmd_vt[3] = (u32)cmd_done - base;
    r->cmd_vt[4] = (u32)cmd_zero - base;
    r->cmd_vt[5] = (u32)cmd_zero - base;
    r->cmd.vtable = r->cmd_vt;
    return r;
}
static void act_set(Run *r, int i, Entity *e) {
    Act *a = &r->act[i];
    a->e = e;
    if (!e) return;
    a->x = e->x;
    a->y = e->y;
    a->endpose = P_IDLE;
    a->dface = 4;
    pose(a, P_IDLE, 0, 0, 0);
}
// The cast into the run: A must stand; B is a body, the point, or nobody (then has_b is 0)
static int cast(Run *r, int need_b) {
    Entity *a = body_of(ram.cast[0]), *b = ram.cast[1] == CINE_POINT ? 0 : body_of(ram.cast[1]);
    if (b == a) b = 0;
    if (!a || (need_b && !b)) return 0;
    act_set(r, 0, a);
    act_set(r, 1, b);
    if (b) { centre(b, &r->tx, &r->ty); r->has_b = 1; }
    else if (ram.cast[1] == CINE_POINT) { r->tx = ram.px; r->ty = ram.py; r->has_b = 1; }
    return 1;
}
static void unlink_cmd(Entity *e, Cmd *c) {
    void **pp = (void **)((u8 *)e + 0x150);
    while (*pp && *pp != c) pp = &((Cmd *)*pp)->next;
    if (*pp == c) { *pp = c->next; c->next = 0; }
}
// Queue the run's command on a body with no command of its own (any body: the director moves the cast itself)
static void run_go(Run *r) {
    Entity *d = 0, *c[3] = { g_PlayerObject, r->act[0].e, r->act[1].e };
    for (int i = 0; i < 3 && !d; i++) if (c[i] && in_list(c[i]) && is_body(c[i]) && !E32(c[i], 0x150)) d = c[i];
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count && !d; i++) if (is_body(l->items[i]) && !E32(l->items[i], 0x150)) d = l->items[i];
    if (!d) { ram.quiet++; ram.last_end = END_QUIET; Arena_Free(r); return; }
    white_build(r->white);
    for (int i = 0; i < 2; i++) if (r->act[i].e) hold(r->act[i].e);
    ram.run = r;
    ram.busy = 1;
    ram.ops++;
    ram.last_op = r->op;
    ram.last_end = END_NONE;
    ram.tick = g_FrameCounter;
    Cmd_Queue(CMD_BATCH, &r->cmd, d);               // outside a batch: the modal, until the command is done
    if (ram.run == r && !*CMD_BATCH) {              // it never ran (a body whose message does not step commands)
        if (in_list(d)) unlink_cmd(d, &r->cmd);
        finish(r, END_STUCK);
        ram.done++;
        run_free(r);
    }
}

// ---- the ops' timelines (one call a frame, events at r->t) ----------------------------------------------------------------
#define A (&r->act[0])
#define B (&r->act[1])
// where an actor's own position must be for his box's centre to be at (cx, cy) px; 8.8
static void origin_for(const Act *a, s32 cx, s32 cy, s32 *ox, s32 *oy) {
    s32 x, y;
    act_centre(a, &x, &y);
    *ox = a->x + ((cx - x) << 8);
    *oy = a->y + ((cy - y) << 8);
}
// The spot beside a centre (tx, ty; a body `tb` wide and tall, or 0) on a side, his own position (8.8), fitted to the floor
static void beside(Act *a, s32 tx, s32 ty, const Entity *tb, int side, int gap, s32 *ox, s32 *oy) {
    s32 cx, cy, w, h, tw = 8, th = 8;
    act_centre(a, &cx, &cy);
    box_size(a->e, &w, &h);
    if (tb) box_size(tb, &tw, &th);
    s32 rx = gap > 0 ? gap : (w + tw) / 2 + 1, ry = gap > 0 ? gap : (h + th) / 2 + 10;
    if (rx < 17 && gap <= 0) rx = 17;
    if (side == 5 || side == 6) {                   // behind / in front of the body
        int f = tb ? tb->facing & 3 : 0;
        static const u8 front[4] = { 4, 3, 1, 2 }, back[4] = { 3, 4, 2, 1 };
        side = side == 6 ? front[f] : back[f];
    }
    if (side == 0) {                                // his own side: left or right of it, above or below when he is there
        s32 dx = cx - tx, dy = cy - ty, ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
        side = ax * 2 >= ay ? (dx <= 0 ? 1 : 2) : (dy < 0 ? 3 : 4);
    }
    s32 nx = tx, ny = ty;
    if (side == 1) nx = tx - rx; else if (side == 2) nx = tx + rx;
    else if (side == 3) ny = ty - ry; else ny = ty + ry;
    s32 x, y;
    origin_for(a, nx, ny, &x, &y);
    x >>= 8; y >>= 8;
    fit(a->e, &x, &y, a->x >> 8, a->y >> 8);
    *ox = x << 8;
    *oy = y << 8;
}
// a spot `d` px from (fx, fy) away from (ax, ay): where a struck body lands (his own position, 8.8)
static void away(Act *a, s32 ax, s32 ay, int d, s32 *ox, s32 *oy) {
    s32 cx, cy;
    act_centre(a, &cx, &cy);
    s32 dx = cx - ax, dy = cy - ay, len = dist(dx, dy);
    if (len == 0) { dx = 1; dy = 0; len = 1; }
    s32 adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
    if (adx * 2 >= ady) { dy = 0; dx = dx < 0 ? -1 : 1; len = 1; }      // about level: straight back
    s32 x = (a->x >> 8) + dx * d / len, y = (a->y >> 8) + dy * d / len;
    fit(a->e, &x, &y, a->x >> 8, a->y >> 8);
    *ox = x << 8;
    *oy = y << 8;
}
static void contact(Run *r, Act *att, Act *def, s32 *x, s32 *y) {
    s32 ax, ay, bx, by;
    act_centre(att, &ax, &ay);
    act_centre(def, &bx, &by);
    (void)r;
    *x = bx + (ax < bx ? -7 : ax > bx ? 7 : 0);
    *y = by - 2 + (ay < by - 8 ? -8 : ay > by + 8 ? 8 : 0);
}

// One blow of `att` on `def`, from local frame k = 0: the lunge to his side (when he is not there), the swing, the
// impact, and for a heavy one the hit-stop and the knock-down. flags: 1 heavy, 2 a kick, 4 the struck one ends on a
// knee. Returns 1 while it plays.
#define ST_SWING_L  3       // frames from the swing's start to the impact, light / heavy (the wind-up)
#define ST_SWING_H  7
// Flag 8: the blow knocks B out of his form (Ryan, 2026-11-06: a Super Saiyan put down "should turn normal as he falls"): a
// transformed player turns back as the form's own turn-back ends (ssj.c), an actor of a Super Saiyan sprite takes the base
// sprite; a white flash on it, and he falls in base form.
int ssj_turn_back(Entity *e);
static void power_down(Run *r, Act *a) {
    Entity *e = a->e;
    int how = ssj_turn_back(e);
    if (!how) return;
    if (how == 1) hold(e);                          // (the turn-back stood him idle: he is the op's again)
    a->shown = 0;
    flash(r, 200, 10);
    shake(r, 8, 40);
}
static int strike(Run *r, Act *att, Act *def, int k, int flags, int back) {
    int heavy = flags & 1, hit = r->p[4] + (heavy ? ST_SWING_H : ST_SWING_L);
    if (k == 0) {
        s32 x, y, cx, cy, d;
        r->p[4] = 0;
        if (def->e) {
            act_centre(def, &cx, &cy);
            beside(att, cx, cy, def->e, 0, 0, &x, &y);
            d = dist((x - att->x) >> 8, (y - att->y) >> 8);
            if (d > 4) {                            // not beside him: the lunge
                int n = d / 7 + 2;
                if (n > 12) n = 12;
                move(att, x, y, n, 0);
                pose(att, P_DASH, 0, 3, -3);
                r->p[4] = n;
                if (d > 40) snd(r, SFX_WHOOSH);
            }
            act_face(att, cx, cy);
            s32 ax, ay;
            act_centre(att, &ax, &ay);
            act_face(def, ax, ay);
        } else if (r->has_b) act_face(att, r->tx, r->ty);
        return 1;
    }
    if (k == r->p[4] + 1) {
        if (def->e) { s32 cx, cy; act_centre(def, &cx, &cy); act_face(att, cx, cy); }
        if (heavy) pose(att, (flags & 2) ? P_KICK : P_PUNCH, 0, 0, 0);          // wound up
        else pose(att, (flags & 2) ? P_KICK : P_PUNCH, 0, 3, 8);
        if (!def->e) snd(r, SFX_WHOOSH);
    }
    if (heavy && k == hit - 2) pose(att, (flags & 2) ? P_KICK : P_PUNCH, 1, 3, 6);
    if (k == hit && def->e) {
        s32 x, y, ax, ay;
        contact(r, att, def, &x, &y);
        act_centre(att, &ax, &ay);
        act_face(def, ax, ay);
        fx(r, FX_STAR, 4, x, y, heavy ? 0x3000 : 0x5000);
        if (flags & 8) power_down(r, def);       // (skipped too: only its light and shake are left out)
        def->flash = heavy ? 6 : 4;
        pose(def, P_HURT, 0, 0, 0);
        snd(r, SFX_HIT + r->hits % 3);
        r->hits++;
        if (heavy) {
            fx(r, FX_RING, 4, x + (x < ax ? -5 : 5), y, 0x4000);
            flash(r, 150, 7);
            shake(r, 16, 80);
            snd(r, SFX_BOOM);
        } else {
            s32 bx, by;
            shake(r, 6, 35);
            away(def, ax, ay, 6, &bx, &by);
            move(def, bx, by, 5, 0);
        }
    }
    if (!def->e) return k < r->p[4] + 16;
    if (heavy) {
        if (k == hit + 5) {                         // after the hit-stop he flies
            s32 ax, ay, bx, by;
            act_centre(att, &ax, &ay);
            away(def, ax, ay, back > 0 ? back : 56, &bx, &by);
            knock(r, def, bx, by, (flags & 4) ? P_KNEEL : P_LIE);
        }
        if (k == hit + 18) pose(att, P_IDLE, 0, 0, 0);
        return k < hit + 6 || def->kt > 0 || k < hit + 5 + KN_FLY + KN_SLIDE + 12;
    }
    if (k == hit + 12) pose(att, P_IDLE, 0, 0, 0);
    if (k == hit + 14) pose(def, P_IDLE, 0, 0, 0);
    return k < hit + 18;
}

static int face_from(const Act *a, s32 ox, s32 oy, s32 tx, s32 ty) {   // his facing toward a spot once he stands at (ox, oy)
    s32 cx, cy;
    act_centre(a, &cx, &cy);
    cx += (ox - a->x) >> 8;
    cy += (oy - a->y) >> 8;
    return cx == tx && cy == ty ? 4 : facing_to(tx - cx, ty - cy);
}
static void op_dash(Run *r) {
    if (r->t == 0) dash(r, A, r->p[0], r->p[1], r->p[2], r->has_b ? face_from(A, r->p[0], r->p[1], r->tx, r->ty) : 4);
    if (r->t >= DASH_FRAMES + 1 && !act_busy(A)) finish(r, END_PLAYED);
}
static void op_strike(Run *r) {
    if (!strike(r, A, B, r->t, r->p[0], r->p[1])) finish(r, END_PLAYED);
}

// The exchange: beats of a few frames each (who swings, with what, how long: the table, cycled), the two shifting with
// the blows, a clash now and then, and in a long one both vanish and meet again a little way off
typedef struct Beat { u8 kind, len; } Beat;       // kind 0 A swings, 1 B swings, 2 both (a clash), 3 they vanish and meet elsewhere
static const Beat beats[] = {
    { 0, 6 }, { 1, 5 }, { 0, 4 }, { 1, 6 }, { 2, 8 }, { 1, 5 }, { 0, 5 }, { 3, 16 },
    { 1, 4 }, { 0, 6 }, { 0, 4 }, { 1, 5 }, { 2, 8 }, { 0, 5 }, { 1, 4 }, { 1, 6 }, { 3, 16 },
};
#define NBEATS ((int)(sizeof beats / sizeof beats[0]))
static const s8 meet[4][2] = { { 34, -20 }, { -40, 12 }, { 22, 24 }, { -16, -28 } };   // where they meet again, from the first spot
static const s8 jit[6] = { -6, 2, -10, 0, -3, -8 };
// the two spots of the pair round a centre (px): A on the side he is, their centres `gap` apart
static void pair_spots(Run *r, s32 cx, s32 cy, s32 *ax, s32 *ay, s32 *bx, s32 *by) {
    s32 aw, ah, bw, bh, gap;
    box_size(A->e, &aw, &ah);
    box_size(B->e, &bw, &bh);
    gap = (aw + bw) / 2 + 3;
    if (gap < 20) gap = 20;
    int sign = r->p[5];
    origin_for(A, cx + sign * (gap / 2), cy, ax, ay);
    origin_for(B, cx - sign * (gap - gap / 2), cy, bx, by);
    s32 x = *ax >> 8, y = *ay >> 8;
    fit(A->e, &x, &y, A->x >> 8, A->y >> 8);
    *ax = x << 8; *ay = y << 8;
    x = *bx >> 8; y = *by >> 8;
    fit(B->e, &x, &y, B->x >> 8, B->y >> 8);
    *bx = x << 8; *by = y << 8;
}
static void swing(Run *r, Act *att, Act *def, int n, int land) {
    s32 x, y, ax, ay, dx;
    act_centre(att, &ax, &ay);
    contact(r, att, def, &x, &y);
    dx = x < ax ? -1 : 1;
    pose(att, (r->hits & 1) ? P_KICK : P_PUNCH, 1, 3, n > 6 ? 6 : n);
    fx(r, FX_STAR, 4, x + dx * 2, y + jit[r->hits % 6], 0x6000);
    snd(r, SFX_HIT + r->hits % 3);
    if (land) { def->flash = 3; pose(def, P_HURT, 0, 1, 4); shake(r, 4, 30); }
    move(att, att->x + dx * (2 << 8), att->y, 3, 0);
    move(def, def->x + dx * (3 << 8), def->y, 3, 0);
    r->hits++;
}
static void op_exchange(Run *r) {
    int k = r->t - r->t0;
    if (r->phase == 0) {                            // they close in: both vanish and meet in the middle
        if (k == 0) {
            s32 ax, ay, bx, by, cx, cy, ex, ey;
            act_centre(A, &cx, &cy);
            act_centre(B, &ex, &ey);
            r->p[5] = cx <= ex ? -1 : 1;            // A's side of the pair
            r->cx = r->p[2] = (cx + ex) / 2; r->cy = r->p[3] = (cy + ey) / 2;
            pair_spots(r, r->cx, r->cy, &ax, &ay, &bx, &by);
            int far = dist(cx - ex, cy - ey) > 44;
            if (far) { dash(r, A, ax, ay, 1, r->p[5] < 0 ? 3 : 2); dash(r, B, bx, by, 1, r->p[5] < 0 ? 2 : 3); }
            else { move(A, ax, ay, 4, 0); move(B, bx, by, 4, 0); A->e->facing = (u8)(r->p[5] < 0 ? 3 : 2); B->e->facing = (u8)(r->p[5] < 0 ? 2 : 3); }
        }
        if (k >= 6 && !act_busy(A) && !act_busy(B)) { r->phase = 1; r->t0 = (s16)(r->t + 1); r->p[4] = 0; r->p[1] = 0; }
        return;
    }
    if (r->phase == 1) {
        const Beat *bt = &beats[r->p[4] % NBEATS];
        if (k == 0 && bt->kind == 3 && (r->p[0] - r->t < 40 || r->p[0] < 100)) {   // no vanishing in a short one, or near the end
            r->p[4]++;
            bt = &beats[r->p[4] % NBEATS];
        }
        if (bt->kind == 3) {
            if (k == 0) {
                s32 ax, ay, bx, by;
                const s8 *m = meet[r->p[1]++ & 3];
                s32 mx = m[0], my = m[1];
                if (!ram.cam) {                     // toward the middle of the view, never into its edge
                    const s32 *cam = CAMERA_CELL;
                    s32 ox = r->p[2] + mx - (cam[3] + cam[5]) / 2, oy = r->p[3] + my - ((cam[4] + cam[6]) / 2 + 10);
                    if (ox * mx > 0 && (ox > 56 || ox < -56)) mx = -mx;
                    if (oy * my > 0 && (oy > 30 || oy < -30)) my = -my;
                }
                r->cx = r->p[2] + mx; r->cy = r->p[3] + my;
                pair_spots(r, r->cx, r->cy, &ax, &ay, &bx, &by);
                dash(r, A, ax, ay, 1, 4);
                dash(r, B, bx, by, 1, 4);
            }
            if (k == 7) {                           // they are there: the first blow of the new spot is a clash
                s32 x, y;
                contact(r, A, B, &x, &y);
                fx(r, FX_RING, 4, x, y, 0x5000);
                shake(r, 6, 40);
                snd(r, SFX_HIT + 1);
                pose(A, P_PUNCH, 1, 3, 6);
                pose(B, P_KICK, 1, 3, 6);
            }
        } else if (bt->kind == 2) {
            if (k == 0) { pose(A, P_PUNCH, 0, 0, 0); pose(B, P_PUNCH, 0, 0, 0); }
            if (k == 2) {
                s32 x, y;
                contact(r, A, B, &x, &y);
                pose(A, P_PUNCH, 1, 3, 5);
                pose(B, P_PUNCH, 1, 3, 5);
                fx(r, FX_RING, 4, x - r->p[5] * 3, y, 0x5000);
                fx(r, FX_STAR, 4, x - r->p[5] * 3, y - 4, 0x6000);
                shake(r, 8, 50);
                snd(r, SFX_HIT + 2);
                move(A, A->x + r->p[5] * (4 << 8), A->y, 3, 0);
                move(B, B->x - r->p[5] * (4 << 8), B->y, 3, 0);
            }
            if (k == 5) {                           // and back in
                move(A, A->x - r->p[5] * (4 << 8), A->y, 3, 0);
                move(B, B->x + r->p[5] * (4 << 8), B->y, 3, 0);
            }
        } else {
            Act *att = bt->kind ? B : A, *def = bt->kind ? A : B;
            if (k == 0) pose(att, (r->hits & 1) ? P_KICK : P_PUNCH, 0, 0, 0);
            if (k == 1) swing(r, att, def, bt->len - 1, (r->p[4] % 3) == 0);
            if (k == bt->len - 1) {                 // they drift back to the spot between blows
                s32 ax, ay, bx, by;
                pair_spots(r, r->cx, r->cy, &ax, &ay, &bx, &by);
                move(A, ax, ay, 2, 0);
                move(B, bx, by, 2, 0);
            }
        }
        if (k >= bt->len - 1) {
            r->p[4]++;
            r->t0 = (s16)(r->t + 1);
            if (r->t >= r->p[0] && !A->dt && !B->dt) { r->phase = 2; pose(A, P_IDLE, 0, 0, 0); pose(B, P_IDLE, 0, 0, 0); }
        }
        return;
    }
    // the end: they break apart, or one lands the last blow
    if (r->end == 0) {
        if (k == 0) {
            s32 x, y;
            x = (A->x >> 8) + r->p[5] * 24; y = A->y >> 8;
            fit(A->e, &x, &y, A->x >> 8, A->y >> 8);
            move(A, x << 8, y << 8, 9, 7);
            x = (B->x >> 8) - r->p[5] * 24; y = B->y >> 8;
            fit(B->e, &x, &y, B->x >> 8, B->y >> 8);
            move(B, x << 8, y << 8, 9, 7);
            snd(r, SFX_WHOOSH);
        }
        if (k == 10) {
            s32 x, y;
            act_centre(A, &x, &y); fx(r, FX_SMOKE, 4, x, y + 4, 0x5000);
            act_centre(B, &x, &y); fx(r, FX_SMOKE, 4, x, y + 4, 0x5000);
        }
        if (k >= 26) finish(r, END_PLAYED);
        return;
    }
    if (!strike(r, r->end == 1 ? A : B, r->end == 1 ? B : A, k, 1 | ((r->hits & 1) ? 2 : 0), 56)) finish(r, END_PLAYED);
}

// The charge: the power-up pose, white flickers that come faster, a rumble that grows, smoke at his feet, the burst
static void op_charge(Run *r) {
    int n = r->p[0], lvl = r->p[1], t = r->t;
    s32 cx, cy;
    act_centre(A, &cx, &cy);
    if (t == 0) {
        pose(A, P_CHARGE, 0, 1, -8);
        if (!r->quiet) { sfx(SFX_POWER); r->hum = 1; }
        if (lvl >= 1) fx(r, FX_RINGS, 4, cx, cy + 8, -0x3000);
    }
    if (t < n) {
        int period = 12 - 8 * t / n;
        if (t % period == 0) A->flash = 3;
        if ((t & 7) == 0) shake(r, 10, 12 + (25 + 25 * lvl) * t / n);
        if (t % 18 == 6) fx(r, FX_SMOKE, 4, cx + ((t / 18) & 1 ? 15 : -15), cy + 8, -0x5000);
        if (lvl >= 2 && t % 30 == 20) fx(r, FX_RINGS, 4, cx, cy + 8, -0x4000);
    }
    if (t == n) {
        flash(r, 110 + 45 * lvl, 9);
        fx(r, FX_RING, 4, cx, cy - 8, 0x3000);
        shake(r, 14, 70);
        if (r->hum) { r->hum = 0; Sfx_Stop(g_AudioMixer + 0x5C4, SFX_TABLE + SFX_POWER * 12); }
        snd(r, SFX_BURST);
        A->flash = 5;
        pose(A, P_CHARGE, 1, 1, 0);
    }
    if (t == n + 8) pose(A, P_IDLE, 0, 0, 0);
    if (t >= n + 12) finish(r, END_PLAYED);
}

// ---- blasts -----------------------------------------------------------------------------------------------------------------
enum { RE_HURT, RE_BLOCK, RE_THROWN };
// where a shot leaves A (px): in front of his hands
static void muzzle(const Act *a, s32 *x, s32 *y) {
    static const s8 off[4][2] = { { 0, 8 }, { 0, -18 }, { -13, -6 }, { 13, -6 } };
    act_centre(a, x, y);
    *x += off[a->e->facing & 3][0];
    *y += off[a->e->facing & 3][1];
}
static void target_now(Run *r) {                    // B's centre follows him (a pushed blocker)
    if (B->e) { act_centre(B, &r->tx, &r->ty); r->ty -= 8; }
}
// A shot lands: size 0 a small one of several, 1 a ki blast, 2 a big one
static void impact(Run *r, int size, int re, int n) {
    s32 x = r->tx + (size ? 0 : jit[n % 6] / 2), y = r->ty + (size ? 0 : jit[(n + 2) % 6]);
    fx(r, FX_BURST, 3, x, y, size ? 0x3000 : 0x5000);
    snd(r, SFX_HIT + n % 3);
    shake(r, size == 2 ? 20 : size ? 8 : 5, size == 2 ? 90 : 40);
    if (size == 2) {
        fx(r, FX_DUST, 4, x - 10, y + 8, 0x2800);
        fx(r, FX_DUST, 4, x + 12, y - 4, 0x2000);
        fx(r, FX_RING, 4, x, y, 0x4000);
        flash(r, 170, 8);
        snd(r, SFX_BOOM);
    }
    if (!B->e || B->kt) return;
    s32 ax, ay, bx, by;
    act_centre(A, &ax, &ay);
    if (re == RE_BLOCK) {
        pose(B, P_KI, 0, 0, 0);
        fx(r, FX_SMOKE, 4, r->tx + (ax < r->tx ? 8 : -8), r->ty + 14, 0x6000);
    } else {
        B->flash = size ? 5 : 3;
        pose(B, P_HURT, 0, 1, 5);
    }
    away(B, ax, ay, size == 2 ? 8 : size ? 5 : 2, &bx, &by);
    move(B, bx, by, 4, 0);
}
// after the last impact (local frame k from it): the thrown one flies, the others recover; 1 while it plays
static int aftermath(Run *r, int k, int re) {
    if (k == 6) pose(A, P_IDLE, 0, 0, 0);
    if (!B->e) return k < 16;
    if (re == RE_THROWN) {
        if (k == 3) {
            s32 ax, ay, bx, by;
            act_centre(A, &ax, &ay);
            away(B, ax, ay, 56, &bx, &by);
            knock(r, B, bx, by, P_LIE);
        }
        return k < 4 || B->kt > 0 || k < KN_FLY + KN_SLIDE + 14;
    }
    if (k == (re == RE_BLOCK ? 10 : 14)) pose(B, P_IDLE, 0, 0, 0);
    return k < 18;
}
static void op_blast(Run *r) {
    int kind = r->p[0], re = r->p[1], t = r->t;
    s32 mx, my;
    if (t == 0) {
        s32 ax, ay;
        act_face(A, r->tx, r->ty);
        act_centre(A, &ax, &ay);
        if (B->e) { act_face(B, ax, ay); if (re == RE_BLOCK) pose(B, P_KI, 0, 0, 0); }
        target_now(r);
        r->p[3] = -1;
    }
    if (kind == 0) {                                // one ki blast
        if (t == 0) pose(A, P_KI, 0, 0, 0);
        if (t == 10) {
            muzzle(A, &mx, &my);
            int n = dist(r->tx - mx, r->ty - my) / 7;
            if (n < 3) n = 3;
            pose(A, P_KI, 1, 2, 4);
            shoot(r, PK_KI, mx, my, r->tx, r->ty, n, 0);
            snd(r, SFX_LAUNCH);
            r->p[3] = t + n;
        }
        if (t == r->p[3]) { impact(r, 1, re, 0); r->t0 = (s16)t; r->phase = 9; }
    } else if (kind == 1) {                         // a volley: a shot every 6 frames from either hand
        int shots = r->p[2];
        if (t == 0) { pose(A, P_KI, 0, 0, 0); r->p[4] = 0; r->p[5] = 0; }
        if (t >= 8 && (t - 8) % 6 == 0 && r->p[4] < shots) {
            muzzle(A, &mx, &my);
            my += (r->p[4] & 1) ? 6 : -5;
            int n = dist(r->tx - mx, r->ty - my) / 8;
            if (n < 3) n = 3;
            pose(A, P_KI, 1, 2, 5);
            shoot(r, PK_KI, mx, my, r->tx + jit[r->p[4] % 6] / 2, r->ty + jit[(r->p[4] + 2) % 6], n, 0);
            snd(r, SFX_LAUNCH + (r->p[4] & 1));
            if (r->p[4] == 0) r->p[3] = n;          // every shot takes the first one's time: they land 6 frames apart
            r->p[4]++;
        }
        if (r->p[3] > 0 && t >= 8 + r->p[3] && (t - 8 - r->p[3]) % 6 == 0 && r->p[5] < r->p[4] && r->phase != 9) {
            int last = ++r->p[5] == shots;
            impact(r, last ? 2 : 0, re, r->p[5]);
            if (last) { r->t0 = (s16)t; r->phase = 9; }
        }
    } else if (kind == 3) {                         // a big blast: held in the palm, then thrown
        if (t == 0) {
            muzzle(A, &mx, &my);
            int n = dist(r->tx - mx, r->ty - my) / 5;
            if (n < 6) n = 6;
            pose(A, P_KI, 0, 0, 0);
            shoot(r, PK_BIG, mx, my, r->tx, r->ty, n, 24);
            fx(r, FX_RINGS, 4, mx, my + 8, 0x3000);
            snd(r, SFX_BURST);
            r->p[3] = 24 + n;
        }
        if (t < 24 && (t & 7) == 3) { A->flash = 2; shake(r, 8, 20 + t); }
        if (t == 24) { pose(A, P_KI, 1, 2, 4); snd(r, SFX_LAUNCH); }
        if (t == r->p[3]) { impact(r, 2, re, 0); r->t0 = (s16)t; r->phase = 9; }
    } else {                                        // a beam, held on him
        int hold = r->p[2];
        if (t == 0) {                               // level with him first: a beam only goes along his facing
            s32 ax, ay, d;
            act_centre(A, &ax, &ay);
            int f = A->e->facing & 3;
            d = f >= 2 ? r->ty + 8 - ay : r->tx - ax;
            if (d && d >= -40 && d <= 40) {
                s32 x = (A->x >> 8) + (f >= 2 ? 0 : d), y = (A->y >> 8) + (f >= 2 ? d : 0);
                fit(A->e, &x, &y, A->x >> 8, A->y >> 8);
                move(A, x << 8, y << 8, 8, 0);
            }
            pose(A, P_BEAM, 0, 0, 0);
        }
        if (t == 14) {
            s32 cap = beam_reach(A->e, r->tx, r->ty) - 12;
            beam_fire(r, 0, A->e, cap);
            pose(A, P_BEAM, 1, 2, -8);
            shake(r, 8, 30);
            r->p[3] = 14 + (cap > 0 ? cap / 6 : 0) + 2;
            if (hold < r->p[3] - 14 + 24) hold = r->p[2] = r->p[3] - 14 + 24;
        }
        if (t > 14 && r->phase != 9) {
            target_now(r);
            r->beam[0].cap = beam_reach(A->e, r->tx, r->ty) - 12;
            if (t >= r->p[3] && t < 14 + hold) {
                int k = t - r->p[3];
                if (k % 6 == 0) {
                    fx(r, (k / 6) & 1 ? FX_BURST : FX_RING, (k / 6) & 1 ? 3 : 4, r->tx, r->ty, 0x5000);
                    shake(r, 8, 45);
                    if (B->e && re != RE_BLOCK) B->flash = 3;
                }
                if (k == 0 && B->e) pose(B, re == RE_BLOCK ? P_KI : P_HURT, 0, 0, 0);
                if (B->e && k % 3 == 0 && k < 42) {         // he is pushed back, a pixel at a time
                    s32 ax, ay, bx, by;
                    act_centre(A, &ax, &ay);
                    away(B, ax, ay, 1, &bx, &by);
                    B->x = bx; B->y = by;
                }
            }
            if (t == 14 + hold) {
                beam_end(r, 0);
                if (re == RE_THROWN) impact(r, 2, re, 0);
                r->t0 = (s16)t; r->phase = 9;
            }
        }
    }
    if (r->phase == 9) {
        int k = t - r->t0;
        if (!aftermath(r, k, re) && (!beam_live(r, 0) || k > 50)) finish(r, END_PLAYED);
    }
}

// ---- the beam struggle --------------------------------------------------------------------------------------------------
static const s8 sway[16] = { 0, 3, 6, 8, 8, 6, 3, 0, -2, -5, -8, -8, -6, -3, -1, 0 };
static void op_struggle(Run *r) {
    int k = r->t - r->t0, n = r->p[0], win = r->p[1];
    if (r->phase == 0) {                            // apart and level: beams only go along a facing
        if (k == 0) {
            s32 ax, ay, bx, by;
            act_centre(A, &ax, &ay);
            act_centre(B, &bx, &by);
            s32 dx = bx - ax, dy = by - ay, adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
            int side = adx * 2 >= ady, sgn = side ? (dx < 0 ? -1 : 1) : (dy < 0 ? -1 : 1);
            s32 sep = side ? adx : ady, need = sep < 116 ? (116 - sep + 1) / 2 : 0;
            s32 nax = ax - (side ? sgn * need : 0), nay = ay - (side ? 0 : sgn * need);
            s32 nbx = side ? bx + sgn * need : ax, nby = side ? ay : by + sgn * need;
            s32 ox, oy, x, y;
            int fa = side ? (sgn > 0 ? 3 : 2) : (sgn > 0 ? 0 : 1), fb = fa ^ 1;
            origin_for(A, nax, nay, &ox, &oy);
            x = ox >> 8; y = oy >> 8;
            fit(A->e, &x, &y, A->x >> 8, A->y >> 8);
            if (need > 3) dash(r, A, x << 8, y << 8, 1, fa); else A->e->facing = (u8)fa;
            origin_for(B, nbx, nby, &ox, &oy);
            x = ox >> 8; y = oy >> 8;
            fit(B->e, &x, &y, B->x >> 8, B->y >> 8);
            if (need > 3 || (side ? ady : adx) > 3) dash(r, B, x << 8, y << 8, 1, fb); else B->e->facing = (u8)fb;
        }
        if (k >= 2 && !act_busy(A) && !act_busy(B)) { r->phase = 1; r->t0 = (s16)(r->t + 1); }
        return;
    }
    // the whole length between the two beams' starts, and where the clash is along it from A's
    s32 pos[2], full, c, cx, cy;
    int fa = A->e->facing & 3, fb = B->e->facing & 3;
    Entity_BeamPoint(pos, B->e);
    pos[0] += BEAM_OFFSETS[fb * 2]; pos[1] += BEAM_OFFSETS[fb * 2 + 1];
    full = beam_reach(A->e, pos[0], pos[1]);
    if (full < 16) full = 16;
    s32 amp = full / 5 > 26 ? 26 : full / 5;
    if (r->phase == 1) {
        if (k == 0) {
            s32 x, y;
            pose(A, P_BEAM, 0, 0, 0); pose(B, P_BEAM, 0, 0, 0);
            act_centre(A, &x, &y); fx(r, FX_RINGS, 4, x, y + 2, 0x3000);
            act_centre(B, &x, &y); fx(r, FX_RINGS, 4, x, y + 2, 0x3000);
            snd(r, SFX_BURST);
            r->p[2] = full / 2;
        }
        if (k < 16 && (k & 7) == 4) { A->flash = 2; B->flash = 2; shake(r, 8, 25); }
        if (k == 16) {
            beam_fire(r, 0, A->e, full / 2 - 6);
            beam_fire(r, 1, B->e, full - full / 2 - 6);
            pose(A, P_BEAM, 1, 2, -8); pose(B, P_BEAM, 1, 2, -8);
        }
        if (k > 16) r->p[2] = full / 2 + amp * sway[((k - 16) >> 2) & 15] / 8;
        if (k >= 16 + n) { r->phase = 2; r->t0 = (s16)(r->t + 1); r->p[3] = r->p[2]; return; }
    }
    c = r->p[2];
    Entity_BeamPoint(pos, A->e);
    pos[0] += BEAM_OFFSETS[fa * 2]; pos[1] += BEAM_OFFSETS[fa * 2 + 1];
    cx = pos[0] + (fa == 2 ? -c : fa == 3 ? c : 0);
    cy = pos[1] + (fa == 1 ? -c : fa == 0 ? c : 0);
    if (r->phase == 1) {
        if (k > 16 + full / 12 + 2) {               // they have met: the clash
            int j = k - 16;
            if (j % 7 == 0) fx(r, FX_RING, 4, cx, cy, 0x5000);
            if (j % 5 == 2) fx(r, FX_STAR, 4, cx + jit[j % 6], cy + jit[(j + 3) % 6] + 4, 0x6000);
            if ((j & 7) == 0) shake(r, 10, 30 + 40 * j / (n > 0 ? n : 1));
            if (j % 36 == 18) flash(r, 70, 5);
            if (j % 16 == 8) snd(r, SFX_HIT + (j >> 4) % 3);
        }
        r->beam[0].cap = c - 6;
        r->beam[1].cap = full - c - 6;
        return;
    }
    // the end
    if (win == 0) {
        if (k == 0) {
            s32 ax, ay, bx, by, x, y;
            beams_gone(r);
            fx(r, FX_DUST, 4, cx - 10, cy + 6, 0x2400); fx(r, FX_DUST, 4, cx + 12, cy - 6, 0x2000);
            fx(r, FX_RING, 4, cx, cy, 0x3000); fx(r, FX_BURST, 3, cx, cy, 0x3000);
            flash(r, 230, 12);
            shake(r, 26, 100);
            snd(r, SFX_BOOM); snd(r, SFX_BOOM + 1);
            act_centre(A, &ax, &ay); act_centre(B, &bx, &by);
            away(A, bx, by, 14, &x, &y); move(A, x, y, 8, 0);
            away(B, ax, ay, 14, &x, &y); move(B, x, y, 8, 0);
            pose(A, P_HURT, 0, 1, 6); pose(B, P_HURT, 0, 1, 6);
            A->flash = 6; B->flash = 6;
        }
        if (k == 26) { pose(A, P_IDLE, 0, 0, 0); pose(B, P_IDLE, 0, 0, 0); }
        if (k >= 32) finish(r, END_PLAYED);
        return;
    }
    Act *w = win == 1 ? A : B, *l = win == 1 ? B : A;
    int wi = win == 1 ? 0 : 1;
    s32 goal = win == 1 ? full : 0;
    if (k <= 20) {                                  // the clash is driven onto the loser
        c = r->p[2] = r->p[3] + (goal - r->p[3]) * k / 20;
        r->beam[0].cap = c - 6;
        r->beam[1].cap = full - c - 6;
        if (k % 4 == 0) { fx(r, FX_RING, 4, cx, cy, 0x6000); shake(r, 10, 70); }
    }
    if (k == 20) {
        s32 x, y, wx, wy, lx, ly;
        Beam *lb = r->beam[!wi].b;
        r->beam[!wi].b = 0;
        if (lb && in_list(lb)) EntityList_Remove(g_EntityList, lb, 0);
        r->beam[wi].cap = full + 14;
        act_centre(l, &lx, &ly); act_centre(w, &wx, &wy);
        fx(r, FX_DUST, 4, lx - 8, ly - 2, 0x2400); fx(r, FX_BURST, 3, lx, ly - 8, 0x3000);
        fx(r, FX_RING, 4, lx, ly - 8, 0x3000); fx(r, FX_DUST, 4, lx + 10, ly - 14, 0x2000);
        flash(r, 230, 12);
        shake(r, 26, 100);
        snd(r, SFX_BOOM); snd(r, SFX_HIT);
        l->flash = 8;
        pose(l, P_HURT, 0, 0, 0);
        away(l, wx, wy, 60, &x, &y);
        r->p[4] = x; r->p[5] = y;
    }
    if (k == 25) knock(r, l, r->p[4], r->p[5], P_LIE);
    if (k == 34) beam_end(r, wi);
    if (k == 44) pose(w, P_IDLE, 0, 0, 0);
    if (k > 44 && !l->kt && k > 25 + KN_FLY + KN_SLIDE + 12 && (!beam_live(r, wi) || k > 90)) finish(r, END_PLAYED);
}

// ---- rise, kneel, fall (the run's A is the body named) ----------------------------------------------------------------------
static void op_pose(Run *r) {
    Entity *e = A->e;
    int t = r->t;
    const u32 *g3 = group_of(e, 3);
    int n = g3 ? frames_in(e, g3) : 1;
    s32 cx, cy;
    if (r->p[1] == 0) {                             // rise: the fall played backwards from where he is
        if (t == 0) r->p[2] = g3 ? (r->p[3] ? 1 : n - 1) : 0;      // p[3]: he was on a knee
        int len = r->p[2] * 5 + 4;
        if (g3 && t <= len) { int f = r->p[2] - t / 5; pose(A, P_FALL, f < 0 ? 0 : f, 0, 0); }
        if (t == len + 1) pose(A, P_IDLE, 0, 0, 0);
        A->endpose = P_IDLE;
        if (t >= len + 4) finish(r, END_PLAYED);
    } else if (r->p[1] == 1) {                      // down on a knee
        if (t == 0) pose(A, P_FALL, 0, 0, 0);
        if (t == 5) { pose(A, P_KNEEL, 0, 0, 0); A->endpose = P_KNEEL; }
        if (t >= 10) finish(r, END_PLAYED);
    } else {                                        // he falls where he stands and lies
        if (t == 0) { e->facing = (u8)((e->facing == 1 || e->facing == 3) ? 3 : 2); pose(A, P_FALL, 0, n - 1, 14); }
        if (t == 10) {
            act_centre(A, &cx, &cy);
            fx(r, FX_SMOKE, 4, cx, cy + 4, 0x4000);
            snd(r, SFX_HIT);
            shake(r, 6, 30);
        }
        if (t == 16) { pose(A, P_LIE, 0, 0, 0); A->endpose = P_LIE; }
        if (t >= 22) finish(r, END_PLAYED);
    }
}
#undef A
#undef B

// ---- the held charge and the flash (cine_tick: every input poll, in scenes' modals and text boxes too) ------------------
// The charge beside the body's own animation (level + 4: a transformation's stock power-up, `SetEntityAnimation X, 9` on
// an actor or 35 on a player, plays under it): the blocking charge's effects on a 160-frame ramp (the stock power-up's
// length), white flickers coming faster, a rumble growing, smoke at his feet by turns, yellow rings on the ground (level 1
// at the start, level 2 every 30 frames); no pose, no hold and no sound of its own (the script and the game's state play
// the stock's), and the burst at -1 is the light and the shake only.
#define OWN_RAMP 160
static u32 fx_key(void *o);
static void own_fx_add(const void *tab, s32 x, s32 y, int speed) {
    for (int i = 0; i < 4; i++) {
        if (ram.own_fx[i] && in_list(ram.own_fx[i])) continue;
        ram.own_fx[i] = fx_make(tab, 4, x, y, speed);
        if (ram.own_fx[i]) ram.own_key[i] = fx_key(ram.own_fx[i]) + 1;
        return;
    }
}
static void own_tick(Entity *e, int t) {
    int lvl = ram.aura_lvl, k = t < OWN_RAMP ? t : OWN_RAMP;
    int period = 12 - 8 * k / OWN_RAMP;
    set_tint(e, (t % period) < 2 ? ram.aura_white : 0);
    if ((t & 7) == 0) camera_shake_safe(10, 12 + (25 + 25 * lvl) * k / OWN_RAMP);
    s32 cx, cy;
    centre(e, &cx, &cy);
    if (t % 18 == 6) own_fx_add(FX_SMOKE, cx + ((t / 18) & 1 ? 15 : -15), cy + 8, -0x5000);
    if (lvl >= 2 && t % 30 == 20) own_fx_add(FX_RINGS, cx, cy + 8, -0x4000);
    for (int i = 0; i < 4; i++) {                   // animated here when nothing else does (a text box)
        void *o = ram.own_fx[i];
        if (!o) continue;
        if (!in_list(o)) { ram.own_fx[i] = 0; continue; }
        if (fx_key(o) == ram.own_key[i]) {
            ((void (*)(void *))vslot(o, 2))(o);
            if (!in_list(o)) { ram.own_fx[i] = 0; continue; }
        }
        ram.own_key[i] = fx_key(o);
    }
}
static void aura_end(int burst) {
    if (!ram.aura_on) return;
    ram.aura_on = 0;
    if (!ram.aura_own) Sfx_Stop(g_AudioMixer + 0x5C4, SFX_TABLE + SFX_POWER * 12);
    Entity *e = body_of(ram.aura_who);
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) {            // nobody keeps the white table
        Entity *o = l->items[i];
        if (is_body(o) && ram.aura_white && E32(o, 0x24) == (u32)ram.aura_white) set_tint(o, 0);
    }
    for (int i = 0; i < 2; i++) {
        if (ram.aura_fx[i] && in_list(ram.aura_fx[i])) EntityList_Remove(g_EntityList, ram.aura_fx[i], 0);
        ram.aura_fx[i] = 0;
    }
    for (int i = 0; i < 4; i++) {
        if (ram.own_fx[i] && in_list(ram.own_fx[i])) EntityList_Remove(g_EntityList, ram.own_fx[i], 0);
        ram.own_fx[i] = 0;
    }
    if (ram.aura_white) { Arena_Free(ram.aura_white); ram.aura_white = 0; }
    if (e && held(e) && !ram.run) release(e);
    if (burst && e) { flash(0, 130, 9); camera_shake_safe(12, 60); if (!ram.aura_own) sfx(SFX_BURST); }
    ram.aura_own = 0;
}
static u32 fx_key(void *o) { return E32(o, 0x30) + (E32(o, 0x34) << 16) + E32(o, 0x3C); }
static void aura_tick(void) {
    Entity *e = body_of(ram.aura_who);
    if (!e || (!ram.aura_own && !held(e)) || !ram.aura_white) { aura_end(0); return; }
    if (ram.run) return;                            // an op plays (it owns the bodies' poses)
    u16 t = ++ram.aura_t;
    if (ram.aura_own) { own_tick(e, t); return; }
    int period = ram.aura_lvl >= 2 ? 5 : ram.aura_lvl ? 7 : 10;
    set_tint(e, (t % period) < 2 ? ram.aura_white : 0);
    pose_kind(e, P_CHARGE, (t >> 3) & 1);
    if ((t & 15) == 1) camera_shake_safe(16, 12 + 14 * ram.aura_lvl);
    for (int i = 0; i < 2; i++) {                   // the smoke: animated here when nothing else does (a text box)
        void *o = ram.aura_fx[i];
        if (!o) continue;
        if (!in_list(o)) { ram.aura_fx[i] = 0; continue; }
        if (fx_key(o) == ram.aura_key[i]) {
            ((void (*)(void *))vslot(o, 2))(o);
            if (!in_list(o)) { ram.aura_fx[i] = 0; continue; }
        }
        ram.aura_key[i] = fx_key(o);
    }
    if (t % 26 == 6) {
        int i = (t / 26) & 1;
        s32 cx, cy;
        centre(e, &cx, &cy);
        if (!ram.aura_fx[i]) {
            ram.aura_fx[i] = fx_make(FX_SMOKE, 4, cx + (i ? 15 : -15), cy + 8, -0x4000);
            if (ram.aura_fx[i]) ram.aura_key[i] = fx_key(ram.aura_fx[i]) + 1;
        }
    }
}
static void aura_start(int lvl, int own) {
    Entity *e = body_of(ram.cast[0]);
    aura_end(0);
    if (!e || ram.run) { ram.quiet++; return; }
    ram.aura_white = Arena_Alloc(256);
    if (!ram.aura_white) { ram.quiet++; return; }
    white_build(ram.aura_white);
    ram.aura_own = (u8)own;
    if (!own) {
        hold(e);
        pose_kind(e, P_CHARGE, 0);
    }
    ram.aura_who = ram.cast[0];
    ram.aura_lvl = (u8)lvl;
    ram.aura_t = 0;
    ram.aura_on = 1;
    if (!own) sfx(SFX_POWER);
    else if (lvl >= 1) {
        s32 cx, cy;
        centre(e, &cx, &cy);
        own_fx_add(FX_RINGS, cx, cy + 8, -0x3000);
    }
}

void cine_tick(void) {
    if (ram.magic != CINE_MAGIC || !(ram.flash_t | ram.aura_on | ram.pend)) return;
    if (ram.flash_t) {
        ram.flash_t--;
        Pal_Blend(255, 255, 255, ram.flash_n ? ram.flash_lvl * ram.flash_t / ram.flash_n : 0);
    }
    if (ram.aura_on) aura_tick();
    if (ram.pend && !ram.run && !scene_running() && !*CMD_BATCH && g_PlayerObject) {   // test command 50's call
        s32 v = 0;
        ram.pend = 0;
        if (cine_call(ram.pend_op, ram.pend_b, ram.pend_c, &v)) coop2.cmd_arg = (u32)v;
    }
}

// ---- looks, the camera ----------------------------------------------------------------------------------------------------
// a target: a body's centre or the point; 0 = nobody
static int target_of(int code, Entity **e, s32 *x, s32 *y) {
    *e = 0;
    if (code == CINE_POINT) { *x = ram.px; *y = ram.py; return 1; }
    if (code == CINE_NOBODY) return 0;
    *e = body_of(code);
    if (!*e) return 0;
    centre(*e, x, y);
    return 1;
}
static int on_screen(const Entity *e) {
    const s32 *cam = CAMERA_CELL;
    s32 x, y;
    centre(e, &x, &y);
    return x > cam[3] - 16 && x < cam[5] + 16 && y > cam[4] - 16 && y < cam[6] + 24;
}
// every body in view but `except` (and whoever lies or kneels in a pose of ours) turns to (x, y)
static void look_all(const Entity *at, s32 x, s32 y, const Entity *except) {
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) {
        Entity *e = l->items[i];
        if (!is_body(e) || e == at || e == except || held(e) || !on_screen(e)) continue;
        face_spot(e, x, y);
    }
}
static int held_count(void) {
    EntityList *l = g_EntityList;
    int n = 0;
    for (int i = 0; i < l->count; i++) if (is_body(l->items[i]) && held(l->items[i])) n++;
    return n;
}
static void end_all(void) {
    aura_end(0);
    flash_off();
    ram.cam = 0;
    if (ram.run) return;                            // (an op queued in a batch keeps its cast)
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) if (is_body(l->items[i]) && held(l->items[i])) release(l->items[i]);
}

// ---- the bridge (CoopCall 37; vmcall.c) -------------------------------------------------------------------------------------
int cine_call(s32 op, s32 b, s32 c, s32 *push) {
    ram_init();
    Entity *e, *e2;
    s32 x, y, x2, y2;
    Run *r;
    switch (op) {
    case CINE_END: end_all(); return 0;
    case CINE_CAST: ram.cast[0] = (s16)b; ram.cast[1] = (s16)c; return 0;
    case CINE_POINT_SET: ram.px = (s16)b; ram.py = (s16)c; return 0;
    case CINE_CAM: ram.cam = (u8)(b != 0); return 0;
    case CINE_HAS: *push = body_of(b) != 0; return 1;
    case CINE_HELD: *push = held_count(); return 1;
    case CINE_DEMO_COND: *push = ram.armed; return 1;
    case CINE_DEMO_OFF: ram.armed = 0; return 0;
    case CINE_JOLT:
        camera_shake_safe(b > 0 ? b : 10, 30 + 25 * (c < 0 ? 0 : c > 3 ? 3 : c));
        if (c > 0) flash(0, c >= 3 ? 220 : c == 2 ? 150 : 90, 7);
        return 0;
    case CINE_LOOK:                                 // the others to the speaker, the speaker to whom he speaks to
        if (!target_of(b, &e, &x, &y)) return 0;
        look_all(e, x, y, e);
        if (e && !held(e) && target_of(c, &e2, &x2, &y2) && e2 != e) face_spot(e, x2, y2);
        return 0;
    case CINE_LOOKAT:
        if (!target_of(b, &e, &x, &y)) return 0;
        e2 = c == CINE_POINT || c == CINE_NOBODY ? 0 : body_of(c);
        look_all(e, x, y, e2);
        return 0;
    case CINE_PAN: {
        if (!target_of(b, &e, &x, &y) || ram.run || *CMD_BATCH) return 0;
        s32 view[2] = { x - 120, y - 80 };
        void *pan = Pan_Init(0, view, c > 0 ? c : 4);
        if (pan) Coroutine_Run(pan);
        return 0;
    }
    case CINE_CHARGE:
        if (b < 0) { aura_end(1); return 0; }
        if (b == 0) {                               // level + 4: beside the body's own animation
            int own = c >= 4;
            if (own) c -= 4;
            aura_start(c < 0 ? 0 : c > 2 ? 2 : c, own);
            return 0;
        }
        break;
    default: break;
    }
    // the blocking moves
    if (op < CINE_DASH_TO || op > CINE_POSE) return 0;
    if (!(r = run_new(op))) return 0;
    int ok = 0;
    switch (op) {
    case CINE_DASH_TO:
    case CINE_DASH_BY:
        if (!cast(r, 0)) break;
        if (op == CINE_DASH_TO) {
            x = b; y = c;
            r->has_b = r->has_b && r->act[1].e;     // he faces B there (not the point)
        } else {
            if (!r->has_b) break;
            beside(&r->act[0], r->tx, r->ty, r->act[1].e, b & 15, c, &x, &y);
            x >>= 8; y >>= 8;
        }
        fit(r->act[0].e, &x, &y, r->act[0].x >> 8, r->act[0].y >> 8);
        r->p[0] = x << 8; r->p[1] = y << 8;
        r->p[2] = op == CINE_DASH_TO || !(b & 16);
        r->act[1].e = 0;                            // B plays no part: he is not held
        r->step = op_dash;
        ok = 1;
        break;
    case CINE_STRIKE:
        if (!cast(r, 0)) break;
        r->p[0] = b & 15; r->p[1] = c;
        r->step = op_strike;
        ok = 1;
        break;
    case CINE_EXCHANGE:
        if (!cast(r, 1)) break;
        r->p[0] = b > 0 ? (b < 24 ? 24 : b > 900 ? 900 : b) : 120;
        r->end = (s16)(c == 1 || c == 2 ? c : 0);
        r->step = op_exchange;
        ok = 1;
        break;
    case CINE_CHARGE:
        if (!cast(r, 0)) break;
        if (ram.aura_on) aura_end(0);
        r->act[1].e = 0;
        r->p[0] = b < 20 ? 20 : b > 900 ? 900 : b;
        r->p[1] = c < 0 ? 0 : c > 2 ? 2 : c;
        r->step = op_charge;
        ok = 1;
        break;
    case CINE_BLAST:
        if (!cast(r, 0)) break;
        e = r->act[0].e;
        if (!r->has_b) {                            // nobody to fire at: straight ahead
            static const s8 dir[4][2] = { { 0, 1 }, { 0, -1 }, { -1, 0 }, { 1, 0 } };
            centre(e, &x, &y);
            r->tx = x + dir[e->facing & 3][0] * 110;
            r->ty = y + dir[e->facing & 3][1] * 110;
            r->has_b = 1;
        }
        r->p[0] = b & 3;
        r->p[1] = (b & 32) ? RE_THROWN : (b & 16) ? RE_BLOCK : RE_HURT;
        if (r->p[0] == 2 && !has_beam_pose(e)) r->p[0] = 3;         // no beam pose: a big blast
        r->p[2] = r->p[0] == 1 ? (c > 0 ? (c > MAX_SHOTS ? MAX_SHOTS : c) : 5) : (c > 0 ? (c > 600 ? 600 : c) : 60);
        r->step = op_blast;
        ok = 1;
        break;
    case CINE_STRUGGLE:
        if (!cast(r, 1) || !g_PlayerObject) break;
        r->p[0] = b > 0 ? (b > 900 ? 900 : b) : 180;
        r->p[1] = c == 1 || c == 2 ? c : 0;
        r->step = op_struggle;
        ok = 1;
        break;
    case CINE_POSE:
        e = body_of(ram.cast[b ? 1 : 0]);
        if (!e || c < 0 || c > 2) break;
        if (c == 0 && !held(e)) break;              // nothing to rise from
        if (c == 0) {
            const u32 *g3 = group_of(e, 3);
            r->p[3] = g3 && E32(e, 0x4C) == (u32)g3 && E16(e, 0x14) <= 1;
        }
        act_set(r, 0, e);
        r->p[1] = c;
        r->step = op_pose;
        ok = 1;
        break;
    default: break;
    }
    if (!ok) { ram.quiet++; ram.last_op = (u8)op; ram.last_end = END_QUIET; Arena_Free(r); return 0; }
    run_go(r);
    return 0;
}

// Test command 50 (docs/ids.md): arg = op, arg2 = b | c << 16 (two signed 16-bit numbers). The call is made at the next
// input poll outside a scene, as a script would make it; a query's answer replaces the `cmd_arg` word. op 100 arms the
// demo scene's trigger (data/maps/broly_arena.json), 101 wipes the counters, 102 stands an actor: arg2 = sprite | dx << 16
// | dy << 24 (signed bytes: px from player 1), facing down; 103 marks OBJ VRAM's largest free block, 104 answers 1 while it
// is that size again (nothing of ours left in OBJ VRAM).
void cine_test(u32 arg, u32 arg2) {
    ram_init();
    if (arg == 100) { ram.armed = 1; return; }
    if (arg == 103) { ram.mark = (u16)(ObjVram_Largest(g_EntityList) >> 6); return; }
    if (arg == 104) { coop2.cmd_arg = (u32)(ObjVram_Largest(g_EntityList) >> 6) == ram.mark; return; }
    if (arg == 102) {
        static const u32 face_wait[2] = { 0x0800CB0Fu, 0x7D00 };    // NpcBehavior_FaceAndWait: face down
        int sprite = (int)(arg2 & 0xFFFF);
        Entity *p1 = g_PlayerObject;
        ActorRecord *rec = Arena_Alloc(sizeof(ActorRecord));         // (the actor keeps it: it goes with the map)
        if (!rec || !p1 || sprite < 7 || (unsigned)sprite >= sprite_index_count || !Character_GetSpriteRecord(sprite)) return;
        memset(rec, 0, sizeof *rec);
        rec->handler = (void *)0x0800D6DFu;
        rec->x = (s16)((p1->x >> 8) + (s8)(arg2 >> 16));
        rec->y = (s16)((p1->y >> 8) + (s8)(arg2 >> 24));
        rec->sprite_id = sprite;
        rec->behaviour_count = 1;
        rec->behaviours[0] = face_wait;
        Entity *n = MapScript_CreateCharacter(rec);
        if (n) { EntityList_Add(g_SceneTable[1], n); n->facing = 0; }
        return;
    }
    if (arg == 101) { ram.ops = ram.done = ram.frames = ram.skips = ram.fx = ram.fx_short = ram.shots = ram.quiet = 0; ram.last_op = ram.last_end = 0; return; }
    ram.pend_op = (u8)arg;
    ram.pend_b = (s16)(arg2 & 0xFFFF);
    ram.pend_c = (s16)(arg2 >> 16);
    ram.pend = 1;
}
