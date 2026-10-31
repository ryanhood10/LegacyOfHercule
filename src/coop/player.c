// Player control states (idle / walk / diagonal) ported from the ROM with one change: input and the
// party slot come from the entity instead of globals, so a second player entity can run them.
#include "coop.h"
#include "revive.h"
#include "wide.h"
#include "tny_engine.h"

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

// Player 1 states in which player 2 keeps its own input (anything else = a cutscene / script holds
// player 1, and player 2 freezes with it). 0x08025B98 is the hurt state: without it player 2 froze
// whenever a boss pair was hitting player 1 (found in the Android 19 + 13 tests).
static const u32 PLAYER_CTRL_VTABLES[] = { IDLE_VTABLE, WALK_VTABLE, 0x080258DCu, 0x08025E20u, 0x080251F4u, 0x08025D70u, 0x08025B98u };

// Player 1's entity is in a player-driven control state (not a cutscene, menu or scripted walk).
int p1_in_play(void) {
    Entity *p1 = g_PlayerObject;
    if (!p1 || !p1->ctrl_state) return 0;
    u32 vt = *(u32 *)p1->ctrl_state;
    for (unsigned i = 0; i < sizeof PLAYER_CTRL_VTABLES / sizeof PLAYER_CTRL_VTABLES[0]; i++)
        if (vt == PLAYER_CTRL_VTABLES[i]) return 1;
    return 0;
}

int p1_controllable(void) {
    return revive_p1_down() || p1_in_play();   // a downed player 1: player 2 plays on (revive.c)
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
    s32 right = left + wide_w() - 4, bottom = top + wide_h() - 2;
    s32 nx = x < left + 4 ? left + 4 : (x > right ? right : x);
    // the 20 px head room only below a scrolled view: at the map's top edge (top 0) the players walk to
    // y 0 as in single-player, or exits along the top edge (zone 14 area 1's ring door, (232, 0)-(281, 17);
    // Gero's lab area 47) could never be reached in co-op
    s32 min_y = top > 0 ? top + 20 : top;
    s32 ny = y < min_y ? min_y : (y > bottom ? bottom : y);
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

// Camera_Shake links the camera into the controller chain headed at 0x03000FF4; calling it again while
// it shakes links it to itself (an endless loop). Extend the running shake instead.
#define SHAKE_HEAD (*(void **)0x03000FF4)
static void install_behaviour(Entity *e, int slot);
static void p2_bind(Entity *p2, int slot);

void camera_shake_safe(int frames, int strength) {
    if (SHAKE_HEAD == g_Camera) {
        s32 *left = (s32 *)((u8 *)g_Camera + 0xC);
        if (*left < frames) *left = frames;
        return;
    }
    Camera_Shake(g_Camera, frames, strength);
}

// Player 2's combat bytes and speeds come from its class init run as its own slot (install_behaviour).
// When its stats or form change (level-up, capsule, transformation) the init runs again; player 1's
// refresh paths only ever touch the active player's entity.
void p2_sync_combat(void) {
    Entity *p2 = coop.p2;
    if (!p2) return;
    int slot = coop.p2_slot;
    const CharacterEntry *c = &g_Party->chars[slot];
    u32 key = (u32)c->str ^ ((u32)c->pow << 8) ^ ((u32)c->end << 16) ^ ((u32)c->display_row << 24) ^ (u32)slot;
    if (key == coop2.p2_class_key || revive_is_down(p2)) return;   // a downed body keeps its pose
    // a script changed the form (SetCharacterTransformation only refreshes the active player's
    // entity): take the new row's sprite too; otherwise only the class (stats, speeds) is re-run
    if (c->display_row != coop2.p2_class_row) p2_bind(p2, slot);
    else install_behaviour(p2, slot);
}

// ---- walk (vtable 0x080256C4 slot 2, original 0x0800811F): the 4-way walk the sprint uses (anim 0xD,
// state 0x03001F48); plain walking is the diagonal state below (anim 1, 0x03001F44) ----------------
void walk_update(void *st, Entity *e) {
    (void)st;
    InputSystem *in = input_for(e);
    u16 cur = in->current;
    if ((*last_dir_for(e) & ~cur) == 0) {
        const Vec *v = &CTX_VECTORS[e->facing];
        s32 nx = e->x + v->dx * e->speed;
        s32 ny = e->y + v->dy * e->speed;
        s32 px[2] = { nx >> 8, ny >> 8 };
        if (!Collision_Check(COLLISION_MAP, px, (u8 *)e + 0x154)) {   // the stock passes the entity's box (+0x154)
                                                                    // here too: 0 read a box from address 0 and the sprint bumped
            camera_shake_safe(8, 100);
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

// Player 2's controller went away (no input for the source timeout): its entity leaves, as in the Cell Games'
// solo fights; it comes back through ensure_p2 when the pad does. A downed player is settled first (revive.c).
void p2_drop_out(void) {
    if (!coop.p2) return;
    revive_p2_gone();
    duel_p2_left();                       // duel.c: a duel in progress ends as player 1's win
    EntityList_Remove(g_SceneTable[1], coop.p2, 0);   // its destroy clears coop.p2
}

// Player 2 is there or about to be: its entity, or a live pad and a party member it can play. Twins, scaling and
// bars go by this (a warp despawns the entity until player 1 is free, so the pad counts).
static int pick_p2_slot(void);
int p2_coming(void) {
    if (companion_active()) return 0;     // a companion (companion.c) is not a player for the rules
    return coop.p2 || (p2_present() && pick_p2_slot() >= 0);
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

void tny_combo_on_hit(Entity *target, Entity *attacker);   // tny_combo_meter.c

// Returns 0 (survived) so OnHit plays the hurt animation. A lethal hit downs player 2 (revive.c); where
// that does not apply (a duel, no player 1) player 2 gets up at half HP beside player 1 as before.
static int p2_take_damage(Entity *e, const u8 *hit) {
    int dmg = *(const s32 *)(hit + 8);
    if (dmg == 0) return 0;
    if (companion_active()) return 1;            // the companion takes no hits (companion.c)
    if (dmg > 0) tny_combo_on_hit(e, *(Entity *const *)(hit - 4));   // team combos (tny_combo.h): the attacker word before the hit
    if (tny_ko_check(1, dmg)) {                  // a tournament fight: 1 HP and out (tny_engine.c)
        Player_Knockback(e, (void *)hit);
        g_Party->chars[coop.p2_slot].hp = 1;
        return 0;
    }
    if (revive_hit(e, (u8 *)hit, 1)) return 1;   // 1: no hurt state for the body
    Player_Knockback(e, (void *)hit);
    if (duel_p2_final_blow(e, dmg)) return 0;   // a duel's final blow: 1 HP, the duel ends
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
    enemies_init();
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
// The story scripts choose player 2 through the party flags (docs/story.md 0): old Hercule joins for
// the future prologue, Krillin when he visits Gohan, Hercule for good in West City (data/story/).
// Hercule (slot 5) stays the first choice whenever he is in the party.
#define HERCULE_SLOT 5
static int slot_ok(int s, int avail) {
    if (s < 0 || s >= PARTY_SLOTS || s == g_Party->active_slot) return 0;
    u32 f = g_Party->chars[s].flags;
    return (f & 1) && (!avail || (f & 2));
}

// Player 2's character: whatever it chose before (kept across maps), else Hercule, else the first
// other party member; -1 if none exists, since the players never share a character. Only available
// members count (bit 1, what the stock scripts clear to lock a chapter to some characters), so the
// story's locks apply to player 2 at the next map load.
static int pick_p2_slot(void) {
    int cs = companion_slot();            // one player: the companion's character (companion.c)
    if (cs >= 0) return cs;
    if (slot_ok(coop.p2_slot, 1)) return coop.p2_slot;
    if (slot_ok(HERCULE_SLOT, 1)) return HERCULE_SLOT;
    for (int i = 0; i < PARTY_SLOTS; i++)
        if (slot_ok(i, 1)) return i;
    return -1;
}

// The class init (slot 0) writes the entity's combat bytes (+0x160..+0x164, with the form's bonus:
// the Super Saiyan classes add e.g. +15/+10/+15) and speeds (+0x188/+0x18C) from the ACTIVE party
// slot (0x08016FD8, 0x0801718C), so it runs with the active slot set to `slot` for the duration.
static void install_behaviour(Entity *e, int slot) {
    const u8 *row = DISPLAY_DATA + g_Party->chars[slot].display_row * 0x14;
    u32 *cell = *(u32 **)(row + 0x10);     // IWRAM cell holding the behaviour vtable pointer
    const u32 *vt = (const u32 *)*cell;
    e->behaviour = cell;
    u8 active = g_Party->active_slot;
    g_Party->active_slot = (u8)slot;
    ((void (*)(void *, Entity *))((u32)vt + vt[0]))(cell, e);
    g_Party->active_slot = active;
    const CharacterEntry *c = &g_Party->chars[slot];
    coop2.p2_class_key = (u32)c->str ^ ((u32)c->pow << 8) ^ ((u32)c->end << 16) ^ ((u32)c->display_row << 24) ^ (u32)slot;
    coop2.p2_class_row = c->display_row;
}

// Give the live player-2 entity another party slot: what the save-pad switch does to player 1
// (new sprite record, behaviour re-installed), plus an idle reset so the new sprite's frames apply.
static void p2_bind(Entity *p2, int slot) {
    coop.p2_slot = (u8)slot;
    void *rec = CharacterEntry_GetSpriteRecord(&g_Party->chars[slot]);
    // A body of another OBJ size (a 64 x 64 tournament fighter, tools/tnyplayables.py): the entity's VRAM slot
    // (+0x0E) and frame buffer (+0x30) were sized from the old record's first half-word at construction, and a
    // bigger frame overruns them (heap corruption, a jump to garbage). The entity leaves instead and ensure_p2
    // builds a new one on this slot on player 1's next idle frame.
    if (rec && *(const u16 *)rec != *(const u16 *)((const u8 *)p2 + 0x0E)) {
        EntityList_Remove(g_SceneTable[1], p2, 0);   // its destroy clears coop.p2
        return;
    }
    p2->sprite_record = rec;
    install_behaviour(p2, slot);
    Entity_SetAnimation(p2, ANIM_IDLE);
    coop.rebinds++;
}

// duel.c: bind player 2 to any slot (the duel's clone slot is not a selectable party member).
int p2_bind_slot(int slot) {
    if (!coop.p2 || slot < 0 || slot >= PARTY_SLOTS) return 0;
    p2_bind(coop.p2, slot);
    return 1;
}

// Player 2's slot changed display row (a transformation): re-read sprite and behaviour, stay idle.
void p2_refresh_sprite(void) {
    if (coop.p2) p2_bind(coop.p2, coop.p2_slot);
}

// Called every input poll. When player 1 switches onto player 2's character, player 2 takes
// player 1's previous one (a trade), or failing that the first other party member.
void coop_active_check(void) {
    int active = g_Party->active_slot;
    if (active == coop.last_active) return;
    int old = coop.last_active;
    coop.last_active = (u8)active;
    if (!coop.p2 || active != coop.p2_slot) return;
    revive_p2_up();                       // a downed player 2's slot changes hands: up first (revive.c)
    int ns = slot_ok(old, 0) ? old : -1;
    for (int i = 0; ns < 0 && i < PARTY_SLOTS; i++)
        if (slot_ok(i, 0)) ns = i;
    if (ns >= 0) p2_bind(coop.p2, ns);
}

// Player 2's own switch: next available party member after the current one, never player 1's. Only
// while player 2 stands or walks: leaving an ability's state by force (the idle reset of p2_bind)
// freed a held Masenko ball that the scene list still held, and the game jumped to garbage.
static int p2_switchable(void) {
    u32 vt = coop.p2->ctrl_state ? *(u32 *)coop.p2->ctrl_state : 0;
    return vt == IDLE_VTABLE || vt == WALK_VTABLE || vt == 0x080258DCu;
}
void p2_cycle(void) {
    if (!coop.p2) return;
    if (!p2_switchable()) { play_cant_sfx(); return; }
    for (int k = 1; k < PARTY_SLOTS; k++) {
        int s = (coop.p2_slot + k) % PARTY_SLOTS;
        if (slot_ok(s, 1)) { p2_bind(coop.p2, s); return; }
    }
    play_cant_sfx();
}

static void ensure_p2(Entity *p1) {
    if (coop.p2 || !p2_present() || coop_solo()) return;
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
