// Krillin as a party member (slot 6, display row 23, sprite 161) and old Hercule (slot 5 on row 24).
//  * status screen: per-slot title, page count and page dispatch (vtable 0x08023B3C slots 13, 23, 24),
//    with the technique pages reading status_module, a copy of the module table 0x081D94D0 that holds
//    per-slot icon pairs and string-block bases for PARTY_SLOTS characters (stock: five slots);
//  * dialogue names: the two VM opcodes that push the active character's name read per-slot blocks;
//  * his four abilities (table entries 19-22, data/hercule.json): Afterimage Rush and Shockwave Punch
//    (melee), Ki Blast (the stock one; its projectile is recoloured blue for Krillin by the spawn hook)
//    and the Destructo Disc, a piercing projectile of the ROM's projectile class;
//  * old Hercule: weak combat bytes while slot 5 shows row 24 (the story switches rows with opcode 73).
// docs/hercule.md "Krillin" / "Old Hercule", docs/party-8-slots.md, docs/sprites.md "Krillin".
#include "coop.h"
#include "krillin.h"
#include "ssj2gohan.h"

typedef struct KrDesc { s8 xo, yo; u8 w, h; u32 attr; const void *blob; } KrDesc;
extern const KrDesc kr_kiblast[16], kr_disc[4];     // build/gen/krillin_sprites_data.c
extern const u8 kr_status_icon_disc[], kr_status_icon_ki[];
extern const char *const ui_strings[];

#define ABILITY_GATE() (*(u32 *)(*(u32 *)0x086DB284u + 0xC))   // the stock isReady gate (hercule.c)
#define E8(e, o)  (*(u8 *)((u8 *)(e) + (o)))
#define E32(e, o) (*(u32 *)((u8 *)(e) + (o)))

ROM_FN(void, StatusPage_Stats, (void *obj, int idx), 0x0800379D);
ROM_FN(void, StatusPage_Melee, (void *obj, int idx), 0x08003685);
ROM_FN(void, StatusPage_Energy, (void *obj, int idx, int which), 0x08003521);
ROM_FN(void *, KiBlast_Create, (void *mem, const s32 *xy, Entity *owner), 0x0800E8B1);
ROM_FN(int, ObjVram_Largest, (void *list), 0x08006D71);
ROM_FN(void *, ProjBase_Init, (void *obj, int vram_bytes, Entity *owner), 0x08013BC1);
ROM_FN(void, ProjBase_UpdateBox, (void *obj), 0x08013BF5);
ROM_FN(int, Proj_InView, (void *obj), 0x08013C23);                     // ahead of it along its facing, still in view
ROM_FN(void, Player_LaunchPoint, (s32 *xy, const Entity *e), 0x08007A27);   // where the ki blast leaves the hand (px)
ROM_FN(void, VM_SetCharacterTransformation, (s32 *stack), 0x0800A8ED);
#ifdef KR_TEST
ROM_FN(void, PlayerEntity_RefreshSprite, (Entity *e), 0x080086A3);
#endif
#define KIBLAST_LIST   (*(void **)0x0840E54C)   // the ki blast module's entity list word (0x0800EAD8)
#define PROJ_VTABLE    0x080259C4u              // the NPC ki blast's vtable (android16.c): copied, slots replaced
#define PROJ_MESSAGE   0x08013D69u              // base projectile message handler (9, 10, 15)
#define SHAKE_HEAD     (*(void **)0x03000FF4)
#define SFX_LAUNCH     0x0D                     // the ki blast's launch (0x0D + rand 2)
#define SFX_KI_HIT     0x22                     // the ki blast's impact (0x22 + rand 3)
#define HURT_VTABLE    0x08025B98u              // player control state while hit (no input)

void p2_refresh_sprite(void);   // player.c

// Attack descriptors {u8 kind (0 melee, 1 ranged), u8 reaction, pad, s32 base power}
static const u32 atk_rush[2]  = { 0x0300, RUSH_POWER };
static const u32 atk_shock[2] = { 0x0400, SHOCK_POWER };
static const u32 atk_disc[2]  = { 0x0301, DISC_POWER };

// Hit message (as the ROM's projectiles build it, android16.c HitMsg)
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

typedef struct KrState {
    u32 magic;
    u32 disc_thrown[PARTY_SLOTS];   // g_FrameCounter of each slot's last throw
    u16 discs, disc_hits;           // debug counters (peek with gdb: kr_state)
    u16 kiblasts, rushes;
    u16 triggers, _pad;            // KR_TEST: frames with L + R held
} KrState;
#define KR_MAGIC 0x4B52494Cu
KrState kr_state __attribute__((section(".party.kr")));   // after g_party_block (coop.ld)

static KrState *state(void) {
    if (kr_state.magic != KR_MAGIC) {
        memset(&kr_state, 0, sizeof kr_state);
        kr_state.magic = KR_MAGIC;
        for (int i = 0; i < PARTY_SLOTS; i++) kr_state.disc_thrown[i] = g_FrameCounter - DISC_COOLDOWN;
    }
    return &kr_state;
}

// Party slot an entity plays (player 1 or player 2), -1 for anything else.
static int entity_slot(const Entity *e) {
    if (!e) return -1;
    if (coop.p2 && e == coop.p2) return coop.p2_slot;
    if (e->vtable == PLAYER_VTABLE) return g_Party->active_slot;
    return -1;
}

static int slot_of(const CharacterEntry *c) { return (int)(c - g_Party->chars); }
static int row_of_slot(int slot) { return slot < 0 ? -1 : g_Party->chars[slot].display_row; }
static void sfx(int id) { Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + id * 12, 0x40, 0); }
static void shake(int frames, int strength) {
    if (SHAKE_HEAD == g_Camera) {          // already shaking: linking it again would loop it on itself
        s32 *left = (s32 *)((u8 *)g_Camera + 0xC);
        if (*left < frames) *left = frames;
        return;
    }
    Camera_Shake(g_Camera, frames, strength);
}

// ---- status screen -------------------------------------------------------------------------------
// The screen object lists the party slots it shows at +0x158 (one word per page index).
static int status_slot(const void *obj, int idx) { return *(const s32 *)((const u8 *)obj + 0x158 + idx * 4); }

// technique icons (18x8, compressed) per slot and page: stock slots 0-4 from the ROM table, Krillin's own
const u32 status_module[13 + 2 * PARTY_SLOTS] __attribute__((aligned(4))) = {
    (u32)&g_party_block, UI_ENERGY1, UI_ENERGY2, UI_DESC1, UI_DESC2,
    0x90, 4, 0xD0, 0x44, 2, 4, 0x1C, 0xF,                                  // blit rects (stock words 5-12)
    0x086FF6D0, 0x086FF68C, 0x086FF7A8, 0x086FF764, 0x086FF5B8,            // slots 0-4 (stock words 13-22)
    0x086FF71C, 0x086FF63C, 0x086FF83C, 0x086FF68C, 0x086FF7E4,
    0, 0,                                                                   // slot 5: Hercule has no technique pages
#if PARTY_SLOTS > KRILLIN_SLOT
    (u32)kr_status_icon_disc, (u32)kr_status_icon_ki,                       // slot 6: Krillin
#endif
};

// Slot 7 is Android 16 on his row 25 (a16player.c) or a duel clone: a clone is named after the
// character whose row it copies (player 1's when none matches).
static int name_slot(int slot) {
    if (slot != DUEL_SLOT || row_of_slot(slot) == 25) return slot;
    for (int s = 0; s < slot; s++) if (row_of_slot(s) == row_of_slot(slot)) return s;
    return g_Party->active_slot;
}

int status_title_hook(void *obj, int idx) { return UI_NAME + name_slot(status_slot(obj, idx)); }

int status_pages_hook(void *obj, int idx) {
    int slot = status_slot(obj, idx);
    if (slot < HERCULE_SLOT) return 4;     // stats, special melee, two energy techniques
    if (slot == KRILLIN_SLOT) return 3;    // stats, Destructo Disc, Ki Blast
    if (slot == SSJ2_SLOT) return 4;       // Gohan's pages
    return 1;
}

void status_draw_hook(void *obj, int idx, int page) {
    int slot = status_slot(obj, idx);
    if (page == 0) StatusPage_Stats(obj, idx);
    else if (slot == SSJ2_SLOT && page <= 3) {
        // Super Saiyan 2 Gohan's technique pages are Gohan's (special melee, Masenko, Kamehameha): the stock
        // pages read the slot from the screen's list (+0x158) for the strings and icons and the slot's flags
        // for "learned", so they are drawn as slot 0 with slot 8's flags for the duration of the call
        s32 *list = (s32 *)((u8 *)obj + 0x158 + idx * 4);
        u32 *gohan = &g_Party->chars[GOHAN_SLOT].flags, kept = *gohan;
        *list = GOHAN_SLOT;
        *gohan = g_Party->chars[SSJ2_SLOT].flags;
        if (page == 1) StatusPage_Melee(obj, idx);
        else StatusPage_Energy(obj, idx, page - 2);
        *gohan = kept;
        *list = SSJ2_SLOT;
    } else if (slot < HERCULE_SLOT) {
        if (page == 1) StatusPage_Melee(obj, idx);
        else StatusPage_Energy(obj, idx, page - 2);
    } else if (slot == KRILLIN_SLOT && page <= 2) StatusPage_Energy(obj, idx, page - 1);
}

// ---- dialogue names (VM opcodes 0x080094B0 / 0x080094CC: push a string pointer) --------------------
static void vm_push(s32 *stack, s32 v) {
    s32 n = stack[0];
    stack[0] = n + 1;
    stack[1 + n] = v;
}
// (Super Saiyan 2 Gohan is "Gohan" in dialogue; "SSJ2 Gohan" is his status-screen title)
static int speaker_slot(void) { int s = g_Party->active_slot; return s == SSJ2_SLOT ? GOHAN_SLOT : s; }
void vm_push_name_hook(s32 *stack) { vm_push(stack, (s32)ui_strings[UI_NAME + speaker_slot()]); }
void vm_push_caps_hook(s32 *stack) { vm_push(stack, (s32)ui_strings[UI_CAPS + speaker_slot()]); }

// ---- ki blast (0x0800EAD8): the stock spawn, Krillin's blasts get the blue frames --------------------
void kiblast_spawn_hook(const s32 *xy, Entity *owner) {
    void *list = KIBLAST_LIST;
    if (ObjVram_Largest(list) < 0x100) return;
    u32 *p = KiBlast_Create(0, xy, owner);
    if (!p) return;
    if (row_of_slot(entity_slot(owner)) == KRILLIN_ROW) {
        const KrDesc *f = &kr_kiblast[(E8(p, 0x14) & 3) * 4];   // frames of its facing (+0x14)
        p[0x14] = (u32)f;    // +0x50 frame base (the update adds frame * 12)
        p[6] = (u32)f;       // +0x18 current frame
        state()->kiblasts++;
    }
    EntityList_Add(list, (Entity *)p);
}

// ---- hits ----------------------------------------------------------------------------------------
static void hit_msg(HitMsg *m, Entity *attacker, void *source, s32 x0, s32 y0, s32 x1, s32 y1, const u32 *attack) {
    memset(m, 0, sizeof *m);
    m->type = (u8)E32(attacker, 0x16C);   // the player side's hit type (projectiles copy it to +0x15)
    m->box[0] = x0; m->box[1] = y0; m->box[2] = x1; m->box[3] = y1;
    m->facing = attacker->facing;
    m->mult = 0x400;
    m->attack = attack;
    m->attacker = attacker;
    m->source = source;
}

void melee_burst(Entity *e, s32 cx, s32 cy, s32 rx, s32 ry, const u32 *attack) {
    HitMsg m;
    hit_msg(&m, e, e, cx - rx, cy - ry, cx + rx, cy + ry, attack);
    EntityList_Broadcast(g_SceneTable[1], 0, &m);
}

// ---- abilities -----------------------------------------------------------------------------------
static int ready(const CharacterEntry *c, int cost) { return ABILITY_GATE() != 0 && (s16)c->ep >= cost; }
typedef struct { s32 dx, dy; } Vec;
#define CTX_VECTORS   ((const Vec *)((const u8 *)g_Ctx + 0xC0))
#define COLLISION_MAP ((const void *)g_Ctx[4])

// 19 Afterimage Rush: dash up to RUSH_DISTANCE px along the facing (stops at walls), then the A attack
// with a stronger hit in front of him
int krillin_rush_ready(const CharacterEntry *c) { return ready(c, RUSH_EP); }
void krillin_rush_use(CharacterEntry *c, Entity *e) {
    c->ep -= RUSH_EP;
    const Vec *v = &CTX_VECTORS[e->facing & 3];
    for (int d = 4; d <= RUSH_DISTANCE; d += 4) {
        s32 px[2] = { (e->x >> 8) + v->dx * 4, (e->y >> 8) + v->dy * 4 };
        if (!Collision_Check(COLLISION_MAP, px, (u8 *)e + 0x154)) break;
        e->x = px[0] << 8;
        e->y = px[1] << 8;
    }
    sfx(SFX_LAUNCH);
    Player_Attack(e);
    s32 cx = (e->x >> 8) + v->dx * 14, cy = (e->y >> 8) - 10 + v->dy * 14;
    melee_burst(e, cx, cy, 14, 14, atk_rush);
    state()->rushes++;
}

// 20 Shockwave Punch: the ki pose (animation 5, group 5) and a burst that knocks back everything around him
int krillin_shock_ready(const CharacterEntry *c) { return ready(c, SHOCK_EP); }
void krillin_shock_use(CharacterEntry *c, Entity *e) {
    c->ep -= SHOCK_EP;
    Entity_SetAnimation(e, 5);
    melee_burst(e, e->x >> 8, (e->y >> 8) - 10, SHOCK_RADIUS, SHOCK_RADIUS - 8, atk_shock);
    shake(8, 50);
    sfx(SFX_KI_HIT);
}

// 21 Ki Blast: the stock ability (its projectile turns blue in kiblast_spawn_hook); data/hercule.json
// points entry 21 at the stock pair with the Ki Blast icons.

// 22 Destructo Disc ---------------------------------------------------------------------------------
typedef struct Disc {
    u32 *vtable;          // +0x00
    s32 sort;             // +0x04
    u8 prio, _p0[3];      // +0x08
    u16 tile, size;       // +0x0C
    const KrDesc *loaded; // +0x10
    u8 facing, type, _p1[2];   // +0x14 (ProjBase_Init: owner's facing and hit type)
    const KrDesc *desc;   // +0x18
    Entity *owner;        // +0x1C (cleared by list message 0xF when the owner goes)
    s32 x, y;             // +0x20 8.8
    s32 box[4];           // +0x28
    s32 sx, sy;           // +0x38
    s32 vx, vy;           // +0x40 8.8 px per frame
    s16 timer, nhit;
    Entity *hit[DISC_MAX_HITS];
    u32 vt[24];
} Disc;

static void disc_remove(Disc *p) { EntityList_Remove(g_SceneTable[1], p, 0); }

static void disc_update(Disc *p) {
    p->timer++;
    p->x += p->vx;
    p->y += p->vy;
    p->desc = &kr_disc[(p->timer >> 1) & 3];
    ProjBase_UpdateBox(p);
    if (!p->owner || p->timer > DISC_LIFE || !Proj_InView(p)) { disc_remove(p); return; }
    s32 x = p->x >> 8, y = p->y >> 8;
    HitMsg m;
    hit_msg(&m, p->owner, p, x - DISC_HALF_W, y - DISC_HALF_H, x + DISC_HALF_W, y + DISC_HALF_H, atk_disc);
    m.type = p->type;
    m.facing = p->facing;
    EntityList *list = g_EntityList;
    Entity *was = coop.acting;
    coop.acting = p->owner;            // EXP for a kill goes to the thrower (exp.c), not the nearer player
    for (int i = list->count - 1; i >= 0 && p->nhit < DISC_MAX_HITS; i--) {
        Entity *t = list->items[i];
        if (!t || t == p->owner || !is_coop_enemy(t)) continue;
        int seen = 0;
        for (int k = 0; k < p->nhit; k++) if (p->hit[k] == t) seen = 1;
        if (seen) continue;
        m.out[0] = m.out[1] = m.out[2] = 0;
        if (((int (*)(Entity *, int, HitMsg *))vslot(t, 9))(t, 0, &m)) {
            p->hit[p->nhit++] = t;      // pierce: keep flying, never hit the same enemy twice
            state()->disc_hits++;
            sfx(SFX_KI_HIT + Rand_Range(3));
        }
    }
    coop.acting = was;
}

static Disc *disc_new(Entity *owner, s32 x, s32 y) {
    if (ObjVram_Largest(g_EntityList) < 0x400 + 0x100) return 0;
    Disc *p = Arena_Alloc(sizeof *p);
    if (!p) return 0;
    memset(p, 0, sizeof *p);
    ProjBase_Init(p, 0x400, owner);
    u32 base = (u32)p->vt;
    const u32 *orig = (const u32 *)PROJ_VTABLE;
    for (int i = 0; i < 24; i++) p->vt[i] = (PROJ_VTABLE + orig[i]) - base;
    p->vt[2] = (u32)disc_update - base;
    p->vt[9] = PROJ_MESSAGE - base;
    p->vt[10] = (u32)disc_update - base;
    p->vtable = p->vt;
    p->x = x << 8;
    p->y = y << 8;
    p->desc = &kr_disc[0];
    p->prio = 0x28;
    const Vec *v = &CTX_VECTORS[p->facing & 3];
    p->vx = v->dx * DISC_SPEED;
    p->vy = v->dy * DISC_SPEED;
    ProjBase_UpdateBox(p);
    EntityList_Add(g_SceneTable[1], (Entity *)p);
    return p;
}

int krillin_disc_ready(const CharacterEntry *c) {
    int slot = slot_of(c);
    return ready(c, DISC_EP) && slot >= 0 && slot < PARTY_SLOTS
        && (u32)(g_FrameCounter - state()->disc_thrown[slot]) >= DISC_COOLDOWN;
}
void krillin_disc_use(CharacterEntry *c, Entity *e) {
    int slot = slot_of(c);
    s32 xy[2];
    Player_LaunchPoint(xy, e);
    if (!disc_new(e, xy[0], xy[1])) return;     // no OBJ VRAM / heap: nothing spent
    c->ep -= DISC_EP;
    state()->disc_thrown[slot] = g_FrameCounter;
    state()->discs++;
    Entity_SetAnimation(e, 5);
    sfx(SFX_LAUNCH + Rand_Range(2));
}

// ---- old Hercule ---------------------------------------------------------------------------------
static void weaken(Entity *e, int slot) {
    if (!e || row_of_slot(slot) != OLD_HERCULE_ROW) return;
    const CharacterEntry *c = &g_Party->chars[slot];
    int s = (c->str >> 8) / OLD_HERCULE_WEAKNESS, p = (c->pow >> 8) / OLD_HERCULE_WEAKNESS;
    e->stat_melee = (u8)(s < 1 ? 1 : s);
    e->stat_ranged = (u8)(p < 1 ? 1 : p);
}

// Switch a slot to a row through the game's own transformation handler (script opcode 73), refreshing
// player 2's entity when it plays that slot. Old Hercule: old_hercule_set(1); back: old_hercule_set(0).
void old_hercule_set(int on) {
    s32 stack[3] = { 2, HERCULE_SLOT, on ? OLD_HERCULE_SPRITE : HERCULE_SPRITE };
    VM_SetCharacterTransformation(stack);
    if (coop.p2 && coop.p2_slot == HERCULE_SLOT) p2_refresh_sprite();
}

// Per frame, from input_poll_hook (player 1's fresh input in `in`).
void krillin_frame(const InputSystem *in) {
    weaken(g_PlayerObject, g_Party->active_slot);
    if (coop.p2) weaken(coop.p2, coop.p2_slot);
#ifdef KR_TEST
    // test builds: nobody dies while a test idles (HP refilled below a third), so the fork can be driven slowly
    for (int i = 0; i < PARTY_SLOTS; i++) {
        CharacterEntry *k = &g_Party->chars[i];
        if (k->max_hp && k->hp < k->max_hp / 3) k->hp = k->max_hp;
    }
    // player 2's pad (the fork's TCP pad, docs/testing.md) holds L + R and presses Up -> slot 5 toggles old
    // Hercule; Right -> Krillin becomes player 1; Left -> Krillin's EXP one short of his next level (the next
    // kill levels him up); Down -> 1 EXP for Krillin through the kill path's EXP event. Player 1's L + R would
    // open the Dragon Radar.
    const InputSystem *t = &coop.input2;
    (void)in;
    if ((t->current & (KEY_L | KEY_R)) == (KEY_L | KEY_R)) {
        Entity *p1 = g_PlayerObject ? g_PlayerObject : coop2.p1_frame;
        state()->triggers++;
        // row and player switches only outside cutscenes and modal dialogue loops (the input hook runs in
        // those too; switching a row inside the level-up dialogue crashed the game when it closed)
        int free = p1_controllable() || (p1 && p1->ctrl_state && *(u32 *)p1->ctrl_state == HURT_VTABLE);
        if ((t->pressed & KEY_UP) && free) old_hercule_set(row_of_slot(HERCULE_SLOT) != OLD_HERCULE_ROW);
        if ((t->pressed & KEY_RIGHT) && free && p1 && g_Party->active_slot != KRILLIN_SLOT) {
            g_Party->active_slot = KRILLIN_SLOT;   // as the save pad's switch does (switch.c confirm)
            p1->sprite_record = Character_GetSpriteRecord(DISPLAY_DATA[KRILLIN_ROW * 0x14 + 1]);
            PlayerEntity_RefreshSprite(p1);
            Entity_SetAnimation(p1, ANIM_IDLE);
        }
        if ((t->pressed & KEY_DOWN) && free) {   // award 1 EXP to Krillin through the kill path's event (exp.c)
            u32 *ev = Arena_Alloc(16);
            if (ev) {
                ev[0] = (u32)coop.exp_vtable; ev[1] = 1; ev[2] = 1; ev[3] = KRILLIN_SLOT;
                g_EntityList->events[g_EntityList->event_count++] = ev;
            }
        }
        if (t->pressed & KEY_LEFT) {
            CharacterEntry *k = &g_Party->chars[KRILLIN_SLOT];
            int lv = Character_GetLevel(k);
            if (lv < 50) k->exp = (u32)Character_LevelExp(lv + 1) - 1;
        }
    }
#else
    (void)in;
#endif
}
