// Android 16 as a player in the "Gero's computer" chapter (docs/story.md 4g, data/story/ch9_gero.*):
//  * CoopCall 11 (a16_join): the chapter's party lock. Every slot but Krillin's and 16's loses its
//    "available" bit (saved in story flags 430 + slot), 16's entry is written into the last party slot
//    (display row 25, sprite 175, Krillin's level with stronger numbers, his abilities) and flag 420 is
//    set. The script then makes Krillin player 1 (SetActiveCharacterNoReset 6) and binds player 2 to 16.
//  * CoopCall 12 (a16_leave): the chapter's end: the saved bits come back, Hercule is in the party and
//    available, flag 421; 16 stays in the party and selectable (Ryan: the players trade Krillin and 16 at a
//    save pad); the script makes Krillin player 1 and Hercule player 2.
//  * CoopCall 12, 1 (a16_gone): the Cell Games' start: 16 leaves the party for good (slot flags 0; he is the
//    stock NPC who dies there); player 2 on him moves to Hercule.
//  * Rocket Punch (ability 22): his forearm flies along his facing and explodes on the first enemy.
// The status screen shows slot 7 as "Android 16" (tools/krillinsprites.py) with his status portrait
// (id 120); krillin.c names a duel clone in that slot after the character it copies.
#include "coop.h"
#include "krillin.h"
#include "a16player.h"

typedef struct A16Desc { s8 xo, yo; u8 w, h; u32 attr; const void *blob; } A16Desc;
extern const A16Desc a16_fist[8], a16_boom[4];     // build/gen/a16_sprites_data.c

int flag_test_hook(PartyState *p, u32 id);          // party.c
void flag_set_hook(PartyState *p, u32 id);
void flag_clear_hook(PartyState *p, u32 id);

#define ABILITY_GATE() (*(u32 *)(*(u32 *)0x086DB284u + 0xC))   // the stock isReady gate (hercule.c)
ROM_FN(int, ObjVram_Largest, (void *list), 0x08006D71);
ROM_FN(void *, ProjBase_Init, (void *obj, int vram_bytes, Entity *owner), 0x08013BC1);
ROM_FN(void, ProjBase_UpdateBox, (void *obj), 0x08013BF5);
ROM_FN(void, Player_LaunchPoint, (s32 *xy, const Entity *e), 0x08007A27);
#define PROJ_VTABLE    0x080259C4u              // the NPC ki blast's vtable (krillin.c): copied, slots replaced
#define PROJ_MESSAGE   0x08013D69u
#define SHAKE_HEAD     (*(void **)0x03000FF4)
#define SFX_LAUNCH     0x0D
#define SFX_BOOM       0x14

static void sfx(int id) { Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + id * 12, 0x40, 0); }
static void shake(int frames, int strength) {
    if (SHAKE_HEAD == g_Camera) {          // already shaking: linking it again would loop it on itself
        s32 *left = (s32 *)((u8 *)g_Camera + 0xC);
        if (*left < frames) *left = frames;
        return;
    }
    Camera_Shake(g_Camera, frames, strength);
}

// ---- the party lock ------------------------------------------------------------------------------
static u16 pct(u32 v, u32 p) { v = v * p / 100; return (u16)(v > 0xFFFF ? 0xFFFF : v); }

void a16_join(void) {
    PartyState *p = g_Party;
    for (int s = 0; s < PARTY_SLOTS; s++) {
        if (s == KRILLIN_SLOT || s == A16_SLOT) continue;
        if (p->chars[s].flags & 2) flag_set_hook(p, A16_FLAG_SAVED + s);
        else flag_clear_hook(p, A16_FLAG_SAVED + s);
        p->chars[s].flags &= ~2u;
    }
    const CharacterEntry *k = &p->chars[KRILLIN_SLOT];
    CharacterEntry *a = &p->chars[A16_SLOT];
    memset(a, 0, sizeof *a);
    a->display_row = A16_ROW;
    a->exp = k->exp;
    a->max_hp = a->hp = pct(k->max_hp, A16_HP_PCT);
    a->max_ep = a->ep = pct(k->max_ep, A16_EP_PCT);
    a->str = pct(k->str, A16_STR_PCT);
    a->pow = pct(k->pow, A16_POW_PCT);
    a->end = pct(k->end, A16_END_PCT);
    a->flags = A16_FLAGS;
    static const u8 abilities[4] = { ROCKET_ABILITY, 19, 0, 2 };   // Rocket Punch, Afterimage Rush, Big Bang, Ki Blast
    for (int i = 0; i < 4; i++) a->ability_list[i] = abilities[i];
    p->chars[KRILLIN_SLOT].flags |= 3;
    flag_set_hook(p, A16_FLAG_ACTIVE);
}

void a16_leave(void) {
    PartyState *p = g_Party;
    for (int s = 0; s < PARTY_SLOTS; s++) {
        if (s == KRILLIN_SLOT || s == A16_SLOT) continue;
        if (flag_test_hook(p, A16_FLAG_SAVED + s)) p->chars[s].flags |= 2;
    }
    if (flag_test_hook(p, A16_FLAG_ACTIVE)) p->chars[A16_SLOT].flags = A16_FLAGS;   // 16 stays selectable
    p->chars[KRILLIN_SLOT].flags |= 3;
    p->chars[HERCULE_SLOT].flags |= 3;
    flag_set_hook(p, A16_FLAG_DONE);
}

// Saves from before 2026-10-21 hold 16 with only Ki Blast learned (flags 0x43) and the old, higher numbers:
// once, the first frame such an entry is seen, he learns all four moves and his numbers are scaled to the
// new percentages (level-ups since he joined are scaled with them)
static u16 rescale(u32 v, u32 now, u32 was) { return pct(v, now * 100 / was); }
void a16_frame(void) {
    CharacterEntry *a = &g_Party->chars[A16_SLOT];
    if (!(a->flags & 1) || a->display_row != A16_ROW || (a->flags & A16_MOVES) == A16_MOVES) return;
    if (!flag_test_hook(g_Party, A16_FLAG_ACTIVE)) return;          // a duel clone on his row: leave it
    a->flags |= A16_MOVES;
    a->max_hp = rescale(a->max_hp, A16_HP_PCT, A16_OLD_HP_PCT);
    if (a->hp > a->max_hp) a->hp = a->max_hp;
    a->str = rescale(a->str, A16_STR_PCT, A16_OLD_STR_PCT);
    a->pow = rescale(a->pow, A16_POW_PCT, A16_OLD_POW_PCT);
    a->end = rescale(a->end, A16_END_PCT, A16_OLD_END_PCT);
}

int p2_bind_slot(int slot);                          // player.c
void a16_gone(void) {
    PartyState *p = g_Party;
    if (!flag_test_hook(p, A16_FLAG_ACTIVE) || p->chars[A16_SLOT].display_row != A16_ROW) return;   // never joined / a duel's
    p->chars[A16_SLOT].flags = 0;
    if (coop.p2 && coop.p2_slot == A16_SLOT) p2_bind_slot(HERCULE_SLOT);
}

// COOP_CMD_A16_PARTY (fork tests): the chapter's party without its scene: 16 joins, Krillin becomes player 1
// through the VM's SetActiveCharacterNoReset (action 115) and player 2 plays 16's slot.
ROM_FN(void, VM_SetActiveCharacterNoReset, (s32 *stack), 0x0800A681);
void a16_test_party(void) {
    a16_join();
    s32 st[2] = { 1, KRILLIN_SLOT };
    VM_SetActiveCharacterNoReset(st);
    p2_bind_slot(A16_SLOT);
}

// ---- Rocket Punch --------------------------------------------------------------------------------
typedef struct HitMsg {      // as the ROM's projectiles build it (krillin.c, android16.c)
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

static const u32 atk_rocket[2] = { 0x0400, ROCKET_POWER };   // melee kind (a fist: Android 19 absorbs ranged hits), heavy knock-back

typedef struct Fist {
    u32 *vtable;          // +0x00
    s32 sort;             // +0x04
    u8 prio, _p0[3];      // +0x08
    u16 tile, size;       // +0x0C
    const A16Desc *loaded;// +0x10
    u8 facing, type, _p1[2];   // +0x14 (ProjBase_Init: owner's facing and hit type)
    const A16Desc *desc;  // +0x18
    Entity *owner;        // +0x1C
    s32 x, y;             // +0x20 8.8
    s32 box[4];           // +0x28
    s32 sx, sy;           // +0x38
    s32 vx, vy;           // +0x40
    s16 timer, boom;      // boom: frames since the explosion started (-1 while flying)
    u8 dir, _p2[3];
    Entity *first;        // the enemy the fist struck (the blast spares it: one hit per punch)
    u32 vt[24];
} Fist;

typedef struct A16State {
    u32 magic;
    u32 thrown;           // g_FrameCounter of the last punch
    u16 punches, hits;    // debug counters
} A16State;
#define A16P_MAGIC 0x41313650u
A16State a16p __attribute__((section(".party.kr")));   // beside krillin.c's state after the party block (coop.ld)

static A16State *state(void) {
    if (a16p.magic != A16P_MAGIC) {
        memset(&a16p, 0, sizeof a16p);
        a16p.magic = A16P_MAGIC;
        a16p.thrown = g_FrameCounter - ROCKET_COOLDOWN;
    }
    return &a16p;
}

static void fist_remove(Fist *p) { EntityList_Remove(g_SceneTable[1], p, 0); }

// One hit message to every enemy whose box meets the rect (but p->first); returns how many took it. With
// first_only it stops at the first and remembers it in p->first.
static int hit_enemies(Fist *p, s32 x0, s32 y0, s32 x1, s32 y1, int first_only) {
    HitMsg m;
    memset(&m, 0, sizeof m);
    m.type = p->type;
    m.box[0] = x0; m.box[1] = y0; m.box[2] = x1; m.box[3] = y1;
    m.facing = p->facing;
    m.mult = 0x400;
    m.attack = atk_rocket;
    m.attacker = p->owner;
    m.source = p;
    EntityList *list = g_EntityList;
    Entity *was = coop.acting;
    coop.acting = p->owner;            // kill EXP to 16 (exp.c)
    int n = 0;
    for (int i = list->count - 1; i >= 0; i--) {
        Entity *t = list->items[i];
        if (!t || t == p->owner || t == p->first || !is_coop_enemy(t)) continue;
        m.out[0] = m.out[1] = m.out[2] = 0;
        if (((int (*)(Entity *, int, HitMsg *))vslot(t, 9))(t, 0, &m)) {
            n++;
            if (first_only) { p->first = t; break; }
        }
    }
    coop.acting = was;
    state()->hits += (u16)n;
    return n;
}

static void fist_explode(Fist *p) {
    s32 x = p->x >> 8, y = p->y >> 8;
    p->boom = 0;
    p->desc = &a16_boom[0];
    ProjBase_UpdateBox(p);
    hit_enemies(p, x - ROCKET_RADIUS, y - ROCKET_RADIUS, x + ROCKET_RADIUS, y + ROCKET_RADIUS, 0);
    shake(10, 60);
    sfx(SFX_BOOM + Rand_Range(2));
}

static void fist_update(Fist *p) {
    p->timer++;
    if (p->boom >= 0) {               // the explosion's four frames, then gone
        p->boom++;
        int f = p->boom < 4 ? 0 : 1 + (p->boom - 4) / 6;
        if (f > 3) { fist_remove(p); return; }
        p->desc = &a16_boom[f];
        ProjBase_UpdateBox(p);
        return;
    }
    if (!p->owner) { fist_remove(p); return; }
    p->x += p->vx;
    p->y += p->vy;
    p->desc = &a16_fist[p->dir * 2 + ((p->timer >> 2) & 1)];
    ProjBase_UpdateBox(p);
    s32 x = p->x >> 8, y = p->y >> 8;
    if (p->timer >= ROCKET_RANGE || hit_enemies(p, x - 7, y - 7, x + 7, y + 7, 1)) fist_explode(p);
}

static Fist *fist_new(Entity *owner, s32 x, s32 y) {
    if (ObjVram_Largest(g_EntityList) < 0x400 + 0x100) return 0;   // room for the 32x32 explosion
    Fist *p = Arena_Alloc(sizeof *p);
    if (!p) return 0;
    memset(p, 0, sizeof *p);
    ProjBase_Init(p, 0x400, owner);
    u32 base = (u32)p->vt;
    const u32 *orig = (const u32 *)PROJ_VTABLE;
    for (int i = 0; i < 24; i++) p->vt[i] = (PROJ_VTABLE + orig[i]) - base;
    p->vt[2] = (u32)fist_update - base;
    p->vt[9] = PROJ_MESSAGE - base;
    p->vt[10] = (u32)fist_update - base;
    p->vtable = p->vt;
    static const u8 fist_dir[4] = { 1, 3, 2, 0 };          // facing down/up/left/right -> a16_fist set
    static const s8 vec[4][2] = { { 0, 1 }, { 0, -1 }, { -1, 0 }, { 1, 0 } };
    int f = p->facing & 3;
    p->dir = fist_dir[f];
    p->vx = vec[f][0] * ROCKET_SPEED;
    p->vy = vec[f][1] * ROCKET_SPEED;
    p->x = x << 8;
    p->y = y << 8;
    p->boom = -1;
    p->desc = &a16_fist[p->dir * 2];
    p->prio = 0x28;
    ProjBase_UpdateBox(p);
    EntityList_Add(g_SceneTable[1], (Entity *)p);
    return p;
}

int a16_rocket_ready(const CharacterEntry *c) {
    return ABILITY_GATE() != 0 && (s16)c->ep >= ROCKET_EP
        && (u32)(g_FrameCounter - state()->thrown) >= ROCKET_COOLDOWN;
}

void a16_rocket_use(CharacterEntry *c, Entity *e) {
    s32 xy[2];
    Player_LaunchPoint(xy, e);
    if (!fist_new(e, xy[0], xy[1])) return;     // no OBJ VRAM / heap: nothing spent
    c->ep -= ROCKET_EP;
    state()->thrown = g_FrameCounter;
    state()->punches++;
    Entity_SetAnimation(e, 5);                  // group 5 of his player record: the rocket punch pose
    sfx(SFX_LAUNCH);
}
