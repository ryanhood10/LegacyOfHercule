// The Time Chamber's hidden room and Mr. Popo (docs/time-chamber.md 5, 6.3, decisions 4 and 9; the state of the build:
// section 12.7). Ryan: "when you go into that hidden room it custscenes, its shaking and mysterious, and then you have a
// boss fight with Mr Popo. he teleports to keep distance from you and shoots blasts, and he has decent dodge. we can put a
// couple jars in this room that give big reward on food/energy."
//
// The room (data/maps/chamber_popo.json, zone 15 area 25) lies behind the gate: its door is the way between the pillars,
// open once all six lights are out (chamber_open.json). Its on-load (ch_popo_onload) hands this file the scenes; when the
// load's fade is over and player 1 is free, the first visit's scene runs (ch_popo_intro: the shaking, the figure with his
// back turned, a line by player 1's character and a reply by player 2's, Popo walks down, a flash and he is behind them),
// later visits one line (ch_popo_again). The scene's last part seals the door; its end swaps the actor for the fighter
// (tny_moves.c tny_moves_popo_spawn: his own kit, far hops, the dodge, no stock AI). At 0 HP he does not fall (KF_YIELD):
// the arms out, then ch_popo_end: a flash, he stands at the far end, his line, the reward (the first time a Senzu and a
// +3 capsule of each, later a Senzu), the offer to start again (the loop: the six lights back on, the tier back to 0,
// every boss CH_LOOP_STEP stronger, a room's first-win capsule then a Senzu), the door opens.
//
// The states:
//   P_IDLE   nothing (not loaded through ch_popo_onload, or done)
//   P_WAIT   loaded: the scene starts once player 1 is free (20 frames after the fade)
//   P_SCENE  the scene runs; its op POPO_FIGHT seals the door -> P_ARMED
//   P_ARMED  the scene ends: the actor becomes the fighter, the boss bar -> P_FIGHT
//   P_FIGHT  his HP at 0 -> P_YIELD (the bar slides out)
//   P_YIELD  YIELD_FRAMES of the arms-out pose, then ch_popo_end -> P_OUTRO
//   P_OUTRO  the end scene; its op POPO_OPEN opens the door -> P_DONE
// Every line is Claude's draft (DRAFT), in data/story/chamber_popo.lscript.
#include "popo.h"
#include "chamber.h"
#include "revive.h"
#include "tny_engine.h"

#define PO_MAGIC        0x4F504F50u   // "POPO"
#define WAIT_FRAMES     20            // after the load's fade, before the scene
#define SCENE_FAILSAFE  90            // frames of no scene before P_SCENE gives up (the door was never sealed)
#define YIELD_FRAMES    40            // his arms out before the end scene's flash
#define FAR_X           244           // the far end, where he stands at a load and after the fight
#define FAR_Y           72
#define MARK_Y          300           // where the players walk to (the arrival is 340)
#define MARK_DX         14            // ... each this far from the middle with two players
#define BEHIND_DY       40            // the flash puts him this far below the players
#define SHAKE_FRAMES    900           // the scene's slow shake, stopped when he moves
#define SHAKE_STRENGTH  4
#define LOOK_UP         1
#define LOOK_DOWN       0
#define LOOK_LEFT       2
#define LOOK_RIGHT      3
#define ITEM_SENZU      0
#define ITEM_STR3       3             // +3 capsules (tny_engine.c's prize)
#define ITEM_PWR3       6
#define ITEM_END3       9
#define ITEM_FOOD30     13            // the big jar: 30 % food and 30 % energy (docs/time-chamber.md 2.4)
#define ITEM_ENERGY30   16
#define DECO_SEAL       0             // the map's decorations: 0 the gate's beam across the door, 1-4 the jars
#define STEP_FLAG_SHOW  0x08009741u   // step 29: decoration N shows (id << 1 | 1 to every entity)
#define STEP_FLAG_HIDE  0x0800975Fu   // step 30: ... hides
#define STEP_WALK_RECT  0x0800A7A1u   // step 71 SetCollisionRect: walkable
#define STEP_BLOCK_RECT 0x0800A7F9u   // step 72 ClearCollisionRect: blocked
#define STEP_SPAWN_ITEM 0x0800ABA5u   // step 87 SpawnItem: at the last break's spot
#define SHAKE_HEAD      (*(void **)0x03000FF4)   // the camera shake chain (player.c camera_shake_safe)
#define CMD_BATCH       ((u32 *)0x030020F8)
ROM_FN(void *, Cmd_Walk, (void *mem, const s32 *xy), 0x08026621);
ROM_FN(void, Cmd_Queue, (u32 *batch, void *cmd, Entity *e), 0x0800E04D);

static const s16 seal[4] = { 224, 360, 264, 376 };   // the door's strip, blocked during the fight (the exit is below it)
static const s16 arena[4] = { 24, 40, 456, 344 };    // his far hops stay inside (px), above the door's strip

BossBar *bossbar_create_hook(void *mem, Entity *e, int lo, int hi);
int flag_test_hook(PartyState *p, u32 id);                      // party.c
void flag_set_hook(PartyState *p, u32 id);
void flag_clear_hook(PartyState *p, u32 id);
int is_any_enemy(const Entity *e);                              // enemies.c
Entity *tny_moves_popo_spawn(s32 x, s32 y, int stat, int sprite, const s16 *arena);   // tny_moves.c
u32 tny_moves_popo_counts(const Entity *e);

enum { P_IDLE, P_WAIT, P_SCENE, P_ARMED, P_FIGHT, P_YIELD, P_OUTRO, P_DONE };
typedef struct PopoState {
    u32 magic;
    u8 state, timer, bot, first;   // bot: the test autopilot (test command 49 op 2); first: this win is the first ever
    Entity *boss;                  // the fighter
    const void *seq[3];            // ch_popo_intro, ch_popo_again, ch_popo_end (the on-load)
    u32 action[2];                 // the queued scene's action object (coop_run_sequence)
    u16 frames;                    // the fight's frames so far (tests, the balance runs)
    u16 pad[2];                    // the autopilot's last pads
} PopoState;
_Static_assert(sizeof(PopoState) <= 0x30, "docs/ids.md: .combo.po at most 0x30 bytes");
static PopoState po __attribute__((section(".combo.po")));

static void state_check(void) {
    if (po.magic == PO_MAGIC) return;
    memset(&po, 0, sizeof po);
    po.magic = PO_MAGIC;
}

static int flag(u32 n) { return flag_test_hook(g_Party, n); }
static int in_list(const Entity *e) {
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) if (l->items[i] == e) return 1;
    return 0;
}
static int here(void) { const u8 *m = (const u8 *)g_MapState; return m && m[0] == CH_ZONE && m[1] == POPO_AREA; }
static void step4(u32 fn, s32 a, s32 b, s32 c, s32 d) {
    s32 st[6] = { 4, a, b, c, d, 0 };
    ((void (*)(s32 *))fn)(st);
}
static void step1(u32 fn, s32 a) {
    s32 st[3] = { 1, a, 0 };
    ((void (*)(s32 *))fn)(st);
}

int chamber_round(void) {
    return flag(CH_FLAG_ROUND0) | flag(CH_FLAG_ROUND0 + 1) << 1;
}
static void set_round(int n) {
    for (int b = 0; b < 2; b++) {
        if ((n >> b) & 1) flag_set_hook(g_Party, CH_FLAG_ROUND0 + b);
        else flag_clear_hook(g_Party, CH_FLAG_ROUND0 + b);
    }
}

int popo_stat(EnemyStat *out) {
    return tny_rating_stat(POPO_RATING + CH_LOOP_STEP * chamber_round(), out);
}

// The actor (the map's record, or the one the end scene stands at the far end): his sprite, not a player, not the fighter
static Entity *actor(void) {
    const void *rec = Character_GetSpriteRecord(POPO_SPRITE);
    if (!rec) return 0;
    EntityList *l = g_EntityList;
    for (int i = l->count - 1; i >= 0; i--) {
        Entity *e = l->items[i];
        if (e && e != g_PlayerObject && e != coop.p2 && e->sprite_record == rec && !is_any_enemy(e)) return e;
    }
    return 0;
}
static Entity *p2_here(void) { return coop.p2 && in_list(coop.p2) ? coop.p2 : 0; }

static void walk(Entity *e, s32 x, s32 y) {        // inside the script's command batch: it walks with the others
    if (!e || !in_list(e) || *(u32 *)((u8 *)e + 0x150)) return;
    s32 xy[2] = { x, y };
    void *c = Cmd_Walk(0, xy);
    if (c) Cmd_Queue(CMD_BATCH, c, e);
}

static void bars_out(void) {
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) {
        BossBar *b = (BossBar *)l->items[i];
        if (b && (b->vtable == (u32 *)BOSSBAR_VTABLE || b->vtable == coop2.bar_vtable)) b->step = -1;
    }
}

static void door(int open) {
    if (open) { step4(STEP_WALK_RECT, seal[0], seal[1], seal[2], seal[3]); step1(STEP_FLAG_HIDE, DECO_SEAL); }
    else { step4(STEP_BLOCK_RECT, seal[0], seal[1], seal[2], seal[3]); step1(STEP_FLAG_SHOW, DECO_SEAL); }
}

static void shake(int on) {
    if (on) { camera_shake_safe(SHAKE_FRAMES, SHAKE_STRENGTH); return; }
    if (SHAKE_HEAD == g_Camera) {                   // the shake runs out on the next frame (its own unlink)
        s32 *left = (s32 *)((u8 *)g_Camera + 0xC);
        if (*left > 1) *left = 1;
    }
}

static void fight_begin(void) {
    Entity *a = actor();
    s32 x = a ? a->x >> 8 : FAR_X, y = a ? a->y >> 8 : FAR_Y;
    int facing = a ? a->facing : LOOK_UP;
    if (a) EntityList_Remove(g_SceneTable[1], a, 0);
    po.boss = tny_moves_popo_spawn(x, y, POPO_STAT, POPO_SPRITE, arena);
    po.frames = 0;
    if (!po.boss) { door(1); po.state = P_DONE; return; }   // no heap for him (never seen): the door opens, no fight
    po.boss->facing = (u8)facing;
    BossBar *bar = bossbar_create_hook(0, po.boss, 0, 100);
    if (bar) EntityList_Add(g_SceneTable[1], (Entity *)bar);
    po.state = P_FIGHT;
}

// ---- the test autopilot (test command 49 op 2): both players walk up beside him and punch; no dodging, no abilities,
// so its numbers are a lower bound on two people's (input.c's bot_pad does the same for Android 16)
static u16 bot_pad(const Entity *pl, const Entity *boss, int side) {
    s32 tx = (boss->x >> 8) + side * 18, ty = boss->y >> 8;
    s32 ex = tx - (pl->x >> 8), ey = ty - (pl->y >> 8);
    u16 pad = 0;
    if (ex > 3 || ex < -3 || ey > 3 || ey < -3) {
        if (ex > 3) pad |= KEY_RIGHT;
        if (ex < -3) pad |= KEY_LEFT;
        if (ey > 3) pad |= KEY_DOWN;
        if (ey < -3) pad |= KEY_UP;
        return pad;
    }
    u32 t = g_FrameCounter;
    if ((t % 20) == 0) return side < 0 ? KEY_RIGHT : KEY_LEFT;   // face him
    return (t % 10) < 2 ? KEY_A : 0;
}
static void bot_input(InputSystem *in, u16 pad, u16 *last) {
    in->current = pad;
    in->pressed = (u16)(pad & ~*last);
    *last = pad;
}
static void bot_tick(void) {
    if (!po.bot || po.state != P_FIGHT || !po.boss || po.boss->hp <= 0 || scene_running()) return;
    Entity *p1 = g_PlayerObject, *p2 = p2_here();
    if (p1 && !revive_is_down(p1)) bot_input(g_InputSystem, bot_pad(p1, po.boss, -1), &po.pad[0]);
    if (p2 && !revive_is_down(p2)) bot_input(&coop.input2, bot_pad(p2, po.boss, 1), &po.pad[1]);
}

void popo_tick(void) {
    state_check();
    if (!here() || !g_PlayerObject || !in_list(g_PlayerObject)) return;
    switch (po.state) {
    case P_WAIT: {
        if (scene_running() || !p1_controllable()) { po.timer = 0; break; }
        if (++po.timer < WAIT_FRAMES) break;
        const void *seq = flag(CH_FLAG_SCENE) && po.seq[1] ? po.seq[1] : po.seq[0];
        po.timer = 0;
        po.state = seq ? P_SCENE : P_IDLE;
        if (seq) coop_run_sequence(po.action, seq);
        break;
    }
    case P_SCENE:                                   // queued, then on screen; never sealed: give up (the door is open)
        if (scene_running()) po.timer = 0;
        else if (++po.timer > SCENE_FAILSAFE) po.state = P_IDLE;
        break;
    case P_ARMED:
        if (!scene_running()) fight_begin();
        break;
    case P_FIGHT:
        if (!po.boss || !in_list(po.boss)) { door(1); po.state = P_DONE; break; }   // gone some other way: never trap them
        if (po.frames < 0xFFFF) po.frames++;
        if (po.boss->hp > 0) { bot_tick(); break; }
        bars_out();
        po.state = P_YIELD;
        po.timer = 0;
        break;
    case P_YIELD:
        if (scene_running() || ++po.timer < YIELD_FRAMES) break;
        po.state = P_OUTRO;
        if (po.seq[2]) coop_run_sequence(po.action, po.seq[2]);
        else { door(1); po.state = P_DONE; }
        break;
    default:
        break;
    }
}

// ---- the scripts' calls (CoopCall 36, data/story/chamber_popo.lscript) -------------------------------------------------
enum {
    POPO_LOAD = 1,      // the on-load: a = seq:ch_popo_intro, b = seq:ch_popo_again; the state from scratch
    POPO_LOAD_END,      // a = seq:ch_popo_end
    POPO_STAGE,         // the players face the far end; player 2 beside player 1
    POPO_MARKS,         // (in a command batch) the players walk in to their marks
    POPO_LOOK,          // a = the players' facing (4: towards each other; alone, up)
    POPO_P1_LINE,       // -> the intro's entry of player 1's line (by his party slot; 10 the fallback)
    POPO_P2_LINE,       // -> the intro's entry of player 2's reply (21 the fallback, 22 alone: none)
    POPO_SHAKE,         // a = 1 the slow shake, 0 it stops
    POPO_TURN,          // a = his facing
    POPO_WALK,          // (in a command batch) he walks to (a, b)
    POPO_BEHIND,        // the flash's other side: he stands behind the players, they turn round to him
    POPO_FIGHT,         // the door seals; the fight starts when the scene ends
    POPO_AWAY,          // the end scene's flash: the fighter's body goes, he stands at the far end facing them
    POPO_WIN,           // -> the end's entry: 1 the first win ever, 3 a later one
    POPO_PAID,          // his first reward is paid (flag 513)
    POPO_LOOP,          // the loop: the six lights back on, one time round more (at most CH_ROUND_MAX)
    POPO_OPEN,          // the door opens
    POPO_JAR,           // a big jar's drop
    POPO_ROOM_OWED,     // (a room's lever, chamber.lscript) -> 1 when its win pays: the first win, or any win once round
    POPO_ROOM_PRIZE,    // -> the item: the room's first win a +3 capsule (flag 514 + light), later a Senzu
    POPO_CAM_X, POPO_CAM_Y,   // -> the view's top-left with player 1 in the middle (PanCameraToPosition)
};
_Static_assert(POPO_ROOM_OWED == 19 && POPO_CAM_Y == 22, "the op numbers data/story/chamber_popo.lscript and chamber.lscript use");

// the intro's entries (data/story/chamber_popo.lscript ch_popo_intro, `[n]` there)
#define E_P1_LINE0      1             // 1-9 player 1's line by party slot, 10 the fallback
#define E_P1_FALLBACK   10
#define E_P2_LINE0      12            // 12-20 player 2's reply by party slot, 21 the fallback
#define E_P2_FALLBACK   21
#define E_POPO_TURNS    22
#define LINE_SLOTS      9

static int bar_short(const CharacterEntry *c, int ep) { return ep ? c->ep < c->max_ep : c->hp < c->max_hp; }
static void big_jar(void) {            // both players' bars (the stock drop reads player 1's): each item while either needs it
    const CharacterEntry *a = &g_Party->chars[g_Party->active_slot];
    const CharacterEntry *b = (coop.p2 && coop.p2_slot < PARTY_SLOTS) ? &g_Party->chars[coop.p2_slot] : a;
    if (bar_short(a, 0) || bar_short(b, 0)) step1(STEP_SPAWN_ITEM, ITEM_FOOD30);
    if (bar_short(a, 1) || bar_short(b, 1)) step1(STEP_SPAWN_ITEM, ITEM_ENERGY30);
}

static s32 view(s32 at, s32 half, s32 size) {
    s32 v = at - half;
    return v < 0 ? 0 : v > size - 2 * half ? size - 2 * half : v;
}

int popo_call(s32 op, s32 a, s32 b, s32 *push) {
    state_check();
    Entity *p1 = g_PlayerObject, *p2 = p2_here();
    switch (op) {
    case POPO_LOAD:
        po.state = P_WAIT;
        po.timer = 0;
        po.boss = 0;
        po.bot = 0;
        po.seq[0] = (const void *)a;
        po.seq[1] = (const void *)b;
        return 0;
    case POPO_LOAD_END: po.seq[2] = (const void *)a; return 0;
    case POPO_STAGE:
        if (p1) p1->facing = LOOK_UP;
        if (p1 && p2) {                             // he walks in beside player 1 (a scene would leave him behind)
            s32 dx = (p2->x >> 8) - (p1->x >> 8), dy = (p2->y >> 8) - (p1->y >> 8);
            if (dx > 40 || dx < -40 || dy > 24 || dy < -24) { p2->x = p1->x + (16 << 8); p2->y = p1->y; }
            p2->facing = LOOK_UP;
        }
        return 0;
    case POPO_MARKS: {
        s32 cx = FAR_X;
        walk(p1, p2 ? cx - MARK_DX : cx, MARK_Y);
        walk(p2, cx + MARK_DX, MARK_Y);
        return 0;
    }
    case POPO_LOOK:
        if (a == 4) {
            if (p1) p1->facing = p2 ? LOOK_RIGHT : LOOK_UP;
            if (p2) p2->facing = LOOK_LEFT;
        } else {
            if (p1) p1->facing = (u8)a;
            if (p2) p2->facing = (u8)a;
        }
        return 0;
    case POPO_P1_LINE: {
        int slot = g_Party->active_slot;
        *push = slot < LINE_SLOTS ? E_P1_LINE0 + slot : E_P1_FALLBACK;
        return 1;
    }
    case POPO_P2_LINE:
        if (!p2 || coop_solo()) *push = E_POPO_TURNS;
        else *push = coop.p2_slot < LINE_SLOTS ? E_P2_LINE0 + coop.p2_slot : E_P2_FALLBACK;
        return 1;
    case POPO_SHAKE: shake(a); return 0;
    case POPO_TURN: { Entity *e = actor(); if (e) e->facing = (u8)a; return 0; }
    case POPO_WALK: walk(actor(), a, b); return 0;
    case POPO_BEHIND: {
        Entity *e = actor();
        if (!e || !p1) return 0;
        s32 x = p1->x >> 8, y = p1->y >> 8;
        if (p2) { x = (x + (p2->x >> 8)) / 2; y = (y + (p2->y >> 8)) / 2; }
        y += BEHIND_DY;
        if (y > arena[3]) y = arena[3];
        e->x = x << 8;
        e->y = y << 8;
        e->facing = LOOK_UP;
        p1->facing = LOOK_DOWN;
        if (p2) p2->facing = LOOK_DOWN;
        return 0;
    }
    case POPO_FIGHT:
        if (!here()) return 0;
        door(0);
        flag_set_hook(g_Party, CH_FLAG_SCENE);
        po.first = !flag(CH_FLAG_POPO);
        po.state = P_ARMED;
        return 0;
    case POPO_AWAY: {
        Entity *e = po.boss;
        po.boss = 0;
        if (e && in_list(e)) {
            EntityList_Broadcast(g_SceneTable[1], 0xF, e);   // "entity died": a bar lets go
            EntityList_Remove(g_SceneTable[1], e, 0);
        }
        if (!actor()) {
            static const u32 face_wait[2] = { 0x0800CB0Fu, 0x7D00 };   // NpcBehavior_FaceAndWait: face down
            static const ActorRecord far = { (void *)0x0800D6DFu, 0, FAR_X, FAR_Y, POPO_SPRITE, 1, { face_wait } };
            Entity *n = MapScript_CreateCharacter(&far);
            if (n) { EntityList_Add(g_SceneTable[1], n); n->facing = LOOK_DOWN; }
        }
        return 0;
    }
    case POPO_WIN: *push = po.first ? 1 : 3; return 1;
    case POPO_PAID: flag_set_hook(g_Party, CH_FLAG_POPO); return 0;
    case POPO_LOOP: {
        for (int i = 0; i < CH_ROOMS; i++) flag_clear_hook(g_Party, CH_FLAG_LEVER0 + i);
        int n = chamber_round();
        if (n < CH_ROUND_MAX) set_round(n + 1);
        return 0;
    }
    case POPO_OPEN:
        if (here()) { door(1); po.state = P_DONE; }
        return 0;
    case POPO_JAR: big_jar(); return 0;
    case POPO_ROOM_OWED: {
        int light = chamber_cut_light();
        *push = !flag(CH_FLAG_FIRST0 + light) || chamber_round() > 0;
        return 1;
    }
    case POPO_ROOM_PRIZE: {
        int light = chamber_cut_light();
        static const u8 capsule[3] = { ITEM_STR3, ITEM_PWR3, ITEM_END3 };
        if (!flag(CH_FLAG_FIRST0 + light)) { flag_set_hook(g_Party, CH_FLAG_FIRST0 + light); *push = capsule[light % 3]; }
        else *push = ITEM_SENZU;
        return 1;
    }
    case POPO_CAM_X: *push = p1 ? view(p1->x >> 8, 120, 480) : 0; return 1;
    case POPO_CAM_Y: *push = p1 ? view(p1->y >> 8, 80, 400) : 0; return 1;
    default:
        return 0;
    }
}

// ---- test command 49 (docs/ids.md) ------------------------------------------------------------------------------------
void popo_test(u32 arg, u32 arg2) {
    state_check();
    switch (arg) {
    case 0:                                         // state | round << 8 | flag 512 << 16 | 513 << 17 | first << 18
        coop2.cmd_arg = (u32)po.state | (u32)chamber_round() << 8 | (u32)flag(CH_FLAG_SCENE) << 16
                      | (u32)flag(CH_FLAG_POPO) << 17 | (u32)po.first << 18;
        break;
    case 1:                                         // his HP := arg2 (bit 31: arg2 & 0xFF percent of his max)
        if (po.boss && in_list(po.boss) && po.boss->hp > 0) {
            s32 max = ((const EnemyStat *)po.boss->behaviour)->max_hp;
            s32 hp = (arg2 & 0x80000000u) ? max * (s32)(arg2 & 0xFF) / 100 : (s32)arg2;
            po.boss->hp = hp > 0 ? hp : 1;
        }
        break;
    case 2: po.bot = (u8)arg2; po.pad[0] = po.pad[1] = 0; break;   // the autopilot on / off
    case 3: coop2.cmd_arg = po.boss ? tny_moves_popo_counts(po.boss) : 0; break;   // far hops | dodges << 16
    case 4: set_round((int)arg2 > CH_ROUND_MAX ? CH_ROUND_MAX : (int)arg2); break;
    case 5:                                         // the fight's frames | his HP << 16 (0 when he is gone)
        coop2.cmd_arg = (u32)po.frames | (u32)(po.boss && in_list(po.boss) && po.boss->hp > 0 ? po.boss->hp : 0) << 16;
        break;
    case 6: coop2.cmd_arg = po.boss && in_list(po.boss) ? (u32)((const EnemyStat *)po.boss->behaviour)->max_hp : 0; break;
    case 7: {                                       // his distance (px, feet to feet) from the nearer player
        u32 best = 0xFFFF;
        Entity *pl[2] = { g_PlayerObject, p2_here() };
        for (int i = 0; i < 2 && po.boss && in_list(po.boss); i++) {
            if (!pl[i]) continue;
            s32 dx = (pl[i]->x - po.boss->x) >> 8, dy = (pl[i]->y - po.boss->y) >> 8;
            u32 d = 0;
            while ((d + 1) * (d + 1) <= (u32)(dx * dx + dy * dy)) d++;
            if (d < best) best = d;
        }
        coop2.cmd_arg = best;
        break;
    }
    case 8: {                                       // the intro's entries for this lineup: player 1's | player 2's << 8
        s32 a = 0, b = 0;
        popo_call(POPO_P1_LINE, 0, 0, &a);
        popo_call(POPO_P2_LINE, 0, 0, &b);
        coop2.cmd_arg = (u32)a | (u32)b << 8;
        break;
    }
    default: break;
    }
}
