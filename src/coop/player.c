// Player control states (idle / walk / diagonal) ported from the ROM with one change: input and the
// party slot come from the entity instead of globals, so a second player entity can run them.
#include "coop.h"

// Both live in the .coop section at the top of EWRAM (reserved from the heap by the ROM patcher);
// the game never zeroes them, hence the magic. Order fixed: remote word at +0, state at +4.
volatile u16 g_p2_remote __attribute__((section(".coop.0"), used));
volatile u16 g_p1_remote __attribute__((section(".coop.0p1"), used)); // +2: netplay override for player 1
CoopState coop __attribute__((section(".coop.1"), used));
u32 g_p2_vtable[PLAYER_VTABLE_SLOTS] __attribute__((section(".coop.vt"), used));

typedef struct { s32 dx, dy; } Vec;
#define CTX_VECTORS   ((const Vec *)((const u8 *)g_Ctx + 0xC0))
#define CTX_FACING    ((const u8 *)g_Ctx + 0x9C)
#define COLLISION_MAP ((const void *)g_Ctx[4])

static int is_p2(const Entity *e) { return e == coop.p2; }

static const u32 PLAYER_CTRL_VTABLES[] = { IDLE_VTABLE, WALK_VTABLE, 0x080258DCu, 0x08025E20u, 0x080251F4u, 0x08025D70u };

int p1_controllable(void) {
    Entity *p1 = g_PlayerObject;
    if (!p1 || !p1->ctrl_state) return 0;
    u32 vt = *(u32 *)p1->ctrl_state;
    for (unsigned i = 0; i < sizeof PLAYER_CTRL_VTABLES / sizeof PLAYER_CTRL_VTABLES[0]; i++)
        if (vt == PLAYER_CTRL_VTABLES[i]) return 1;
    return 0;
}

static InputSystem no_input;   // what player 2 sees during cutscenes

static InputSystem *input_for(const Entity *e) {
    if (!is_p2(e)) return g_InputSystem;
    return p1_controllable() ? &coop.input2 : &no_input;
}
static int slot_for(const Entity *e) { return is_p2(e) ? coop.p2_slot : g_Party->active_slot; }
static u16 *last_dir_for(const Entity *e) { return is_p2(e) ? &coop.p2_last_dir : &g_LastDir; }
static u32 *last_dir_frame_for(const Entity *e) { return is_p2(e) ? &coop.p2_last_dir_frame : &g_LastDirFrame; }

static void play_cant_sfx(void) { Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + 0xE4, 0x40, 0); }

void leash_p2(Entity *e) {
    if (!coop.p2 || (e != coop.p2 && e != g_PlayerObject)) return;   // both players, only in co-op
    const s32 *cam = CAMERA_CELL;
    if (!cam) return;
    s32 left = cam[7], top = cam[8];           // +0x1C, +0x20
    s32 x = e->x >> 8, y = e->y >> 8;
    // the whole visible area minus the sprite's own half-width (the GBA shows 240 x 160 and
    // nothing beyond it; a wider view is an emulator feature, see docs)
    s32 nx = x < left + 4 ? left + 4 : (x > left + 236 ? left + 236 : x);
    s32 ny = y < top + 20 ? top + 20 : (y > top + 158 ? top + 158 : y);
    if (nx != x) e->x = nx << 8;
    if (ny != y) e->y = ny << 8;
}

static void use_ability(Entity *e) {
    CharacterEntry *c = &g_Party->chars[slot_for(e)];
    const AbilityDef *a = &ABILITY_TABLE[c->ability_list[c->ability_index]];
    if (a->is_ready(c)) a->on_use(c, e);
    else play_cant_sfx();
}

static void handle_buttons(Entity *e, const InputSystem *in) {
    if (in->pressed & KEY_A) Player_Attack(e);
    else if (in->pressed & KEY_B) use_ability(e);
}

// ---- idle (vtable 0x08025A84 slot 2, original 0x08007F35) ------------------------------------
typedef struct { u32 *vtable; s32 timer; } IdleState;

void idle_update(IdleState *st, Entity *e) {
    InputSystem *in = input_for(e);
    if (++st->timer > 0x400) {
        const u8 *sprite = e->sprite_record;
        if (sprite[2] < 0xF || *(const u32 *)(sprite + 300) == 0) st->timer = 0;
        else Entity_SetAnimation(e, ANIM_FIDGET);
    }
    if (in->current & KEY_DPAD) {
        if (in->pressed & KEY_LEFT) e->facing = 2;
        if (in->pressed & KEY_RIGHT) e->facing = 3;
        if (in->pressed & KEY_UP) e->facing = 1;
        if (in->pressed & KEY_DOWN) e->facing = 0;
        Entity_SetAnimation(e, ANIM_WALK);
    }
    u16 dir = in->pressed & KEY_DPAD;
    if (dir) {
        u16 *last = last_dir_for(e);
        u32 *last_frame = last_dir_frame_for(e);
        u32 frame = g_FrameCounter;
        if (frame - *last_frame < 20 && (dir & (dir - 1)) == 0 && (*last & dir)) Entity_SetAnimation(e, ANIM_DASH);
        *last = dir;
        *last_frame = frame;
    }
    leash_p2(e);
    handle_buttons(e, in);
}

// ---- walk (vtable 0x080256C4 slot 2, original 0x0800811F) ------------------------------------
void walk_update(void *st, Entity *e) {
    (void)st;
    InputSystem *in = input_for(e);
    u16 cur = in->current;
    if ((*last_dir_for(e) & ~cur) == 0) {
        const Vec *v = &CTX_VECTORS[e->facing];
        s32 nx = e->x + v->dx * e->speed;
        s32 ny = e->y + v->dy * e->speed;
        s32 px[2] = { nx >> 8, ny >> 8 };
        if (!Collision_Check(COLLISION_MAP, px, 0)) {
            Camera_Shake(g_Camera, 8, 100);
            Entity_SetAnimation(e, ANIM_BUMP);
            play_cant_sfx();
        } else {
            e->x = nx;
            e->y = ny;
            if ((cur & KEY_UP) && e->facing != 1) ny -= e->side_speed;
            if ((cur & KEY_DOWN) && e->facing != 0) ny += e->side_speed;
            if ((cur & KEY_LEFT) && e->facing != 2) nx -= e->side_speed;
            if ((cur & KEY_RIGHT) && e->facing != 3) nx += e->side_speed;
            px[0] = nx >> 8;
            px[1] = ny >> 8;
            if (Collision_Check(COLLISION_MAP, px, (u8 *)e + 0x154)) {
                e->x = nx;
                e->y = ny;
            }
        }
    } else {
        Entity_SetAnimation(e, ANIM_IDLE);
    }
    leash_p2(e);
    handle_buttons(e, in);
}

// ---- diagonal / free movement (vtable 0x080258DC slot 2, original 0x08008031) ----------------
void diag_update(void *st, Entity *e) {
    (void)st;
    InputSystem *in = input_for(e);
    u16 cur = in->current;
    if (!(cur & KEY_DPAD)) {
        Entity_SetAnimation(e, ANIM_IDLE);
    } else {
        int k = 0;
        s32 xy[2];
        if (cur & KEY_LEFT) {
            xy[0] = e->x - e->side_speed; xy[1] = e->y;
            Player_StepAxis(e, xy, 2); k = 1;
        } else if (cur & KEY_RIGHT) {
            xy[0] = e->x + e->side_speed; xy[1] = e->y;
            Player_StepAxis(e, xy, 2); k = 2;
        }
        if (cur & KEY_UP) {
            xy[0] = e->x; xy[1] = e->y - e->side_speed;
            Player_StepAxis(e, xy, 1); k += 3;
        } else if (cur & KEY_DOWN) {
            xy[0] = e->x; xy[1] = e->y + e->side_speed;
            Player_StepAxis(e, xy, 1); k += 6;
        }
        e->facing = CTX_FACING[e->facing + k * 4];
    }
    leash_p2(e);
    handle_buttons(e, in);
}

// ---- player 2 lifetime -----------------------------------------------------------------------
static void p2_destroy(Entity *e, int free_it, u32 a, u32 b) {
    e->vtable = g_p2_vtable;   // mirrors the original, which resets its own vtable here
    coop.p2 = 0;
    coop.destroys++;
    Entity_BaseDestroy(e, 0, a, b);
    if (free_it) Arena_Free(e);
}

// ---- player 2 damage routing (private vtable slots 15 / 16) ---------------------------------
static void p2_change_hp(Entity *e, int delta) {
    (void)e;
    CharacterEntry *c = &g_Party->chars[coop.p2_slot];
    int hp = c->hp + delta;
    if (hp > c->max_hp) hp = c->max_hp;
    if (hp < 0) hp = 0;
    c->hp = (u16)hp;
}

ROM_FN(void, Player_Knockback, (Entity *e, void *hit), 0x0800F833);

// Returns 0 (survived) so OnHit plays the hurt animation; at 0 HP player 2 revives beside player 1.
static int p2_take_damage(Entity *e, const u8 *hit) {
    int dmg = *(const s32 *)(hit + 8);
    if (dmg == 0) return 0;
    Player_Knockback(e, (void *)hit);
    CharacterEntry *c = &g_Party->chars[coop.p2_slot];
    if (c->hp > dmg) {
        c->hp = (u16)(c->hp - dmg);
    } else {
        c->hp = (u16)(c->max_hp / 2 ? c->max_hp / 2 : 1);
        Entity *p1 = g_PlayerObject;
        if (p1 && p1 != e) { e->x = p1->x + (16 << 8); e->y = p1->y; }
    }
    return 0;
}

static void build_vtables(void) {
    // All 48 slots: the dash/flight states call slots past 24, and a short copy sent them into
    // whatever followed the table (the "jumped to invalid address" crash on a two-player dash).
    u32 base = (u32)g_p2_vtable;
    for (int i = 0; i < PLAYER_VTABLE_SLOTS; i++) g_p2_vtable[i] = (0x08025604u + PLAYER_VTABLE[i]) - base;
    g_p2_vtable[0] = (u32)p2_destroy - base;
    g_p2_vtable[15] = (u32)p2_change_hp - base;
    g_p2_vtable[16] = (u32)p2_take_damage - base;
    exp_init();
    if (coop.p2) coop.p2->vtable = g_p2_vtable;   // a live player 2 from an older build's savestate
    coop.code_id = (u32)coop_init_once;
}

void coop_init_once(void) {
    if (coop.magic == COOP_MAGIC) {
        // A savestate from another build: the RAM vtables hold that build's code addresses.
        if (coop.code_id != (u32)coop_init_once) build_vtables();
        return;
    }
    coop.magic = COOP_MAGIC;
    coop.p2 = 0;
    coop.control = 0;
    coop.p2_last_dir = 0;
    coop.p2_last_dir_frame = 0;
    coop.input2.history_head = 0;
    coop.input2.history_armed = 0;
    coop.input2.current = coop.input2.pressed = coop.input2.released = 0;
    coop.hud2.tile = 0;
    coop.polls = 0;
    coop.sio_last = 0;
    coop.source_age = P2_SOURCE_TIMEOUT;
    coop.spawns = coop.destroys = 0;
    coop.p2_slot = 0xFF;          // nothing chosen yet: first other party member
    coop.last_active = g_Party->active_slot;
    coop.rebinds = 0;
    // Both roles sit in multiplayer serial mode from the first frame so a linked emulator pair
    // (lockstep) never sees a mode mismatch; the original game does not use the serial port.
    REG_RCNT = 0;
    REG_SIOCNT = SIOCNT_MULTI_115200;
    REG_SIOMLT_SEND = SIO_MARK;
    coop.sio_ready = 1;
    build_vtables();
}

// A slot player 2 may use: in the party, not player 1's. `avail` also requires the game's
// "available" flag (bit 1), the one the save-pad switch screen greys characters out by.
// Hercule (slot 5) is player 2's default: the game fills his entry at new game (level 1, 50 HP)
// but never flags him in the party until late, so he is allowed whenever his entry is filled.
#define HERCULE_SLOT 5
static int slot_ok(int s, int avail) {
    if (s < 0 || s >= 6 || s == g_Party->active_slot) return 0;
    if (s == HERCULE_SLOT) return g_Party->chars[s].max_hp != 0;
    u32 f = g_Party->chars[s].flags;
    return (f & 1) && (!avail || (f & 2));
}

// Player 2's character: whatever it chose before (kept across maps), else Hercule, else the first
// other party member; -1 if none exists, since the players never share a character.
static int pick_p2_slot(void) {
    if (slot_ok(coop.p2_slot, 0)) return coop.p2_slot;
    if (slot_ok(HERCULE_SLOT, 0)) return HERCULE_SLOT;
    for (int i = 0; i < 6; i++)
        if (slot_ok(i, 0)) return i;
    return -1;
}

static void install_behaviour(Entity *e, int slot) {
    const u8 *row = DISPLAY_DATA + g_Party->chars[slot].display_row * 0x14;
    u32 *cell = *(u32 **)(row + 0x10);     // IWRAM cell holding the behaviour vtable pointer
    const u32 *vt = (const u32 *)*cell;
    e->behaviour = cell;
    ((void (*)(void *, Entity *))((u32)vt + vt[0]))(cell, e);
}

// Give the live player-2 entity another party slot: what the save-pad switch does to player 1
// (new sprite record, behaviour re-installed), plus an idle reset so the new sprite's frames apply.
static void p2_bind(Entity *p2, int slot) {
    coop.p2_slot = (u8)slot;
    p2->sprite_record = CharacterEntry_GetSpriteRecord(&g_Party->chars[slot]);
    install_behaviour(p2, slot);
    Entity_SetAnimation(p2, ANIM_IDLE);
    coop.rebinds++;
}

// Called every input poll. When player 1 switches onto player 2's character, player 2 takes
// player 1's previous one (a trade), or failing that the first other party member.
void coop_active_check(void) {
    int active = g_Party->active_slot;
    if (active == coop.last_active) return;
    int old = coop.last_active;
    coop.last_active = (u8)active;
    if (!coop.p2 || active != coop.p2_slot) return;
    int ns = slot_ok(old, 0) ? old : -1;
    for (int i = 0; ns < 0 && i < 6; i++)
        if (slot_ok(i, 0)) ns = i;
    if (ns >= 0) p2_bind(coop.p2, ns);
}

// Player 2's own switch: next available party member after the current one, never player 1's.
void p2_cycle(void) {
    if (!coop.p2) return;
    for (int k = 1; k < 6; k++) {
        int s = (coop.p2_slot + k) % 6;
        if (slot_ok(s, 1)) { p2_bind(coop.p2, s); return; }
    }
    play_cant_sfx();
}

static void ensure_p2(Entity *p1) {
    if (coop.p2 || !p2_present()) return;
    int slot = pick_p2_slot();
    if (slot < 0) return;
    coop.p2_slot = (u8)slot;
    // 12 px to player 1's left when that spot is walkable, else exactly on player 1 (overlapping
    // beats appearing outside the map at an edge spawn)
    s32 px[2] = { (p1->x >> 8) - 12, p1->y >> 8 };
    if (px[0] < 8 || !Collision_Check(COLLISION_MAP, px, (u8 *)p1 + 0x154)) px[0] = p1->x >> 8;   // +0x154 = box w,h
    s16 xy[2] = { (s16)px[0], (s16)px[1] };
    Entity *p2 = PlayerEntity_Create(0, CharacterEntry_GetSpriteRecord(&g_Party->chars[coop.p2_slot]), xy);
    g_PlayerObject = p1;  // the constructor claimed the player slot
    if (!p2) return;
    p2->vtable = g_p2_vtable;
    if (coop.p2_slot != g_Party->active_slot) install_behaviour(p2, coop.p2_slot);
    EntityList_Add(g_SceneTable[1], p2);
    coop.p2 = p2;
    coop.spawns++;
}

// Entry hook kept on the idle slot so P2 is (re)created whenever P1 stands on a map.
void idle_update_hook(IdleState *st, Entity *e) {
    coop_init_once();
    if (e == g_PlayerObject) ensure_p2(e);
    idle_update(st, e);
}
