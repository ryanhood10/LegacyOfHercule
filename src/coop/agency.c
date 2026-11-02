// Player 2 is not a co-pilot (docs/perfect-pass.md 11.3; Ryan, 2026-11-02: "only P1 can guide the gameplay experience").
// Decompiled (docs/engine.md "Trigger boxes"): the stock scan 0x0800BFC0 runs once a world update, from the top of the
// trigger list {count, boxes...} at 0x03001FE0 down, for player 1's position, and stops at the first box that fires. A box
// is {vtable, s16 x1, y1, x2, y2 (x2 and y2 outside), activation object}; the activation's slot 1 asks "does it fire now?"
// (A pressed, A pressed facing a way, a direction held, on entering) and calls the box's slot 1, its fire: a door, a
// walk-off, a script, a dialogue or the save pad, each modal for as long as it runs. This file replaces the scan: player
// 1's half is the stock one, and then the same boxes are tried for player 2's position and pad.
//   * a save pad, a door and a walk-off get his entity, as they would player 1's (the save's spot, the arrival's offset);
//   * a script and a dialogue walked into, written for a player standing on the box, have player 1 put on his spot first
//     (the scene's staging and player 1's character stay what the scripts were written for);
//   * whatever he fires, his pad is the stock input until it ends (input.c, agency_pads): the save menu, the switch screen,
//     a map viewed with a wait for A, and the world map when he took off.
// Character barriers ("Only Piccolo"): both the punchable item and the refusal line depend on the active character only
// through a condition tested at the map's load. With two players the item and the line are made for any character who
// can open it, and the condition is tested again for the player who punched or pressed A (agency_item_msg, gate()).
// None of this runs alone: no player 2 (or a companion), the stock scan and the stock barriers.
#include "coop.h"
#include "agency.h"
#include "revive.h"
#include "tny_engine.h"

#define TRIGGER_LIST  ((s32 *)0x03001FE0)   // {count, boxes...}; [0x21] scans since the last reset, [0x22] resets (map loads)

// the box classes (their slot 1 fires) and the activation classes (their slot 1 tests, then fires)
#define BOX_DOOR      0x080240E4u    // 0x0800C233: fires while the key +0x11 is held, warps to the payload at +0x14
#define BOX_WALKOFF   0x08023E38u    // 0x0800D9C3: the same, the arrival keeps the offset into the rect
#define BOX_SCRIPT    0x08023BC0u    // 0x0800C2C3
#define BOX_SAVE      0x08023BCCu    // 0x08010AC7
#define BOX_DIALOGUE  0x08023E44u    // 0x0800D7E1
#define ACT_ENTER     0x080238E4u    // 0x03002094: fires, then the box leaves the list (unless the fire reset it)
#define ACT_A         0x08023E50u    // 0x03002098: A pressed inside
#define ACT_FACING    0x08025188u    // 0x0300209C..B4: A pressed while facing +4
#define ACT_HELD      0x0802498Cu    // 0x03002154..: the key +4 held, then as ACT_ENTER (door animations)
#define ACT_EDGE      0x08024980u    // its own object: only when not inside at the last scan (+4 the scan's number)
#define ACT_ALWAYS    0x080240F0u    // 0x030020BC: the box's own test decides (exits, dropped items)

typedef struct Box { u32 vtable; s16 x1, y1, x2, y2; u32 *act; u8 facing, key; } Box;
#define ITEM_VT       0x08025720u    // a map item on its decoration ({vt, ..., +0x24 the record, +0x28 the decoration})
#define ITEM_SCRIPT   0x0800F9A3u    // an item record's handler that runs its script (barriers, rocks with a script)

ROM_FN(void, Box_Remove, (s32 *list, Box *b), 0x0800BF6F);
ROM_FN(void, Item_Attach, (const void *record), 0x0800F035);
ROM_FN(int, Item_Message, (void *item, int msg, void *data, u32 x), 0x0800F2B7);
ROM_FN(void, Cmd_Queue, (u32 *batch, void *cmd, Entity *e), 0x0800E04D);
ROM_FN(void, Cmd_Begin, (u32 *batch), 0x0800E033);
ROM_FN(void, Cmd_Commit, (u32 *batch), 0x0800E039);
ROM_FN(void, Step_FlyTo, (s32 *st), 0x08009E31);
ROM_FN(void, Step_Warp, (s32 *st), 0x080099D3);
ROM_FN(void *, Mover_Walk, (const void *go, Entity *e), 0x0800D585);
#define CMD_BATCH     ((u32 *)0x030020F8)
#define FLY_CMD_VT    0x08024864u    // FlyToPosition's command: {vt, next, done, mover, x, y}, 2 px a frame, animation 12
#define HERCULE_SLOT  5
#define WORLD_ZONE    6
#define WORLD_AREA    99

typedef struct AgState {
    u8 modal;            // 1: player 2 fired what runs now: his pad is the stock input (input.c)
    u8 world;            // he took off: his pad drives the world map until the flyer lands (1 on the way, 2 on it)
    u8 pad;              // the box firing is a flight pad's kind (a script on A): 1 for player 1, 2 for player 2
    u8 quiet;            // frames left of a press's "no second press from the other pad" (a line closed once)
    u8 quiet_by;         // 1 / 2: whose press it was
    u8 pending;          // a slot + 1 chosen on the switch screen for a body of another size: bound when it ends
    u16 prev2;           // player 2's buttons at the last poll
    Entity *attacker;    // the player inside Player_Attack
    s32 map_gen;         // the trigger list's reset count the barriers were last made for
    u32 run_vt[6];       // the run command's vtable (relative words: it lives here)
} AgState;
static AgState ag __attribute__((section(".combo.ag")));
_Static_assert(sizeof(AgState) <= 0x40, "docs/ids.md: .combo.ag at most 0x40 bytes");

static int fire_box(Box *b, Entity *who) {
    const u32 *vt = (const u32 *)b->vtable;
    return ((int (*)(Box *, Entity *))((u32)vt + vt[1]))(b, who);
}

// ---- who may fire ---------------------------------------------------------------------------------------------------
static int two_players(void) { return coop.p2 && !companion_active(); }

// Player 2 is free to lead: no scene, no duel, no solo fight, nobody down, player 1 on his feet in a control state and
// player 2 standing, walking or dashing. A ring's fight has its own exits.
static int p2_free(void) {
    Entity *p2 = coop.p2;
    if (!two_players() || !p2_pad_present() || coop_solo() || coop2.duel_active || coop2.call_state) return 0;
    if (scene_running() || !p1_in_play() || revive_is_down(p2) || revive_p1_down() || tny_ring_active()) return 0;
    if (!g_Party->chars[coop.p2_slot].hp || !g_Party->chars[g_Party->active_slot].hp) return 0;
    return p2_in_control();
}

static int slot_of(const Entity *e) { return e == coop.p2 ? coop.p2_slot : g_Party->active_slot; }

// ---- character conditions ------------------------------------------------------------------------------------------
// A condition script that reads the active character (Step 11 PushActiveCharIndex, bytes 02 0B): every barrier's item and
// refusal line in the ROM, and no other trigger or item (docs/engine.md "Character barriers").
static int reads_character(u32 cond) {
    if (cond < 0x08000000u) return 0;
    const u8 *s = (const u8 *)cond;
    for (int i = 0; i < 48 && s[i] != 0x11; i++)
        if (s[i] == 0x02 && s[i + 1] == 0x0B) return 1;
    return 0;
}

static int cond_for(u32 cond, int slot) {
    u8 active = g_Party->active_slot;
    g_Party->active_slot = (u8)slot;
    int r = flag_cond_hook(cond);
    g_Party->active_slot = active;
    return r;
}

typedef struct MapTrigger { u32 cond; const u32 *data; } MapTrigger;   // data: create, activation getter, s16 rect, payload
static const u8 *map_entry(void) { return *(const u8 **)((u8 *)g_MapState + 4); }

// The condition of a live box, if it is one of those (found by rect in the map's list; only asked on a press)
static u32 box_condition(const Box *b) {
    const u8 *m = map_entry();
    const MapTrigger *t = *(const MapTrigger *const *)(m + 0x10);
    for (int i = 0; i < m[3]; i++) {
        const s16 *r = (const s16 *)&t[i].data[2];
        if (r[0] == b->x1 && r[1] == b->y1 && r[2] == b->x2 && r[3] == b->y2 && reads_character(t[i].cond)) return t[i].cond;
    }
    return 0;
}

// A press on a character-conditioned box counts for the character of the player who pressed
static int gate(const Box *b, const Entity *who) {
    if (*b->act != ACT_A && *b->act != ACT_FACING) return 1;
    u32 cond = box_condition(b);
    return !cond || cond_for(cond, slot_of(who));
}

static int any_slot(u32 cond) {
    for (int s = 0; s < PARTY_SLOTS; s++)
        if ((g_Party->chars[s].flags & 1) && cond_for(cond, s)) return 1;
    return 0;
}

static int item_there(const u32 *rec) {
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) {
        const u32 *e = (const u32 *)l->items[i];
        if (e && e[0] == ITEM_VT && (const u32 *)e[9] == rec) return 1;
    }
    return 0;
}

static int box_there(const u32 *data) {
    const s32 *list = TRIGGER_LIST;
    const s16 *r = (const s16 *)&data[2];
    for (int i = 1; i <= list[0]; i++) {
        const Box *b = (const Box *)list[i];
        if (r[0] == b->x1 && r[1] == b->y1 && r[2] == b->x2 && r[3] == b->y2) return 1;
    }
    return 0;
}

// Once a map, when player 2 stands on it: the barrier pieces player 1's character did not get at the load. The stock
// tested each condition for the active character only.
static void barriers_for_two(void) {
    const u8 *m = map_entry();
    const u32 *const *items = *(const u32 *const *const *)(m + 0x18);
    for (int i = 0; i < m[5]; i++) {
        const u32 *rec = items[i];
        if (rec[2] == ITEM_SCRIPT && reads_character(rec[0]) && any_slot(rec[0]) && !item_there(rec)) Item_Attach(rec);
    }
    const MapTrigger *t = *(const MapTrigger *const *)(m + 0x10);
    s32 *list = TRIGGER_LIST;
    for (int i = 0; i < m[3]; i++) {
        if (!reads_character(t[i].cond) || !any_slot(t[i].cond) || box_there(t[i].data)) continue;
        void *(*create)(const u32 *) = (void *(*)(const u32 *))t[i].data[0];
        void *box = create(t[i].data);
        if (box && list[0] < 0x1F) list[++list[0]] = (s32)box;
    }
}

void agency_map_tick(void) {
    s32 gen = TRIGGER_LIST[0x22];
    if (gen == ag.map_gen || !two_players() || !p1_in_play()) return;
    ag.map_gen = gen;
    barriers_for_two();
}

// The item's message (vtable 0x08025720 slot 9): a hit (message 0, the attacker at +0x20: a punch's player, or a
// projectile with its owner at +0x1C) on a barrier counts only for the character of the player behind it.
int agency_item_msg(u32 *item, int msg, u8 *data, u32 x) {
    if (msg == 0) {                  // alone too: a barrier made for player 2, who has left since, stays closed to the wrong one
        const u32 *rec = (const u32 *)item[9];
        if (rec[2] == ITEM_SCRIPT && reads_character(rec[0])) {
            const Entity *who = *(const Entity *const *)(data + 0x20);
            if (who && who != g_PlayerObject && who != coop.p2) who = *(const Entity *const *)((const u8 *)who + 0x1C);
            if (!cond_for(rec[0], who == coop.p2 ? coop.p2_slot : g_Party->active_slot)) return 0;
        }
    }
    return Item_Message(item, msg, data, x);
}

// ---- the scan ---------------------------------------------------------------------------------------------------------
// Player 1 put on player 2's spot: a scene he walked into stages player 1, as written, where the box is
static void bring_p1(const Entity *p2) {
    Entity *p1 = g_PlayerObject;
    p1->x = p2->x;
    p1->y = p2->y;
    p1->facing = p2->facing;
}

// Does this box fire for player 2 now? The activation's own test on his entity and pad, then the fire.
static int p2_try_box(Box *b, s32 *list) {
    Entity *p2 = coop.p2;
    const InputSystem *in = &coop.input2;
    u32 av = *b->act;
    if (b->vtable == BOX_SCRIPT && av == ACT_EDGE) return 0;   // stock: only the bridges' layer swaps, which follow player 1
    switch (av) {
    case ACT_A: if (!(in->pressed & KEY_A)) return 0; break;
    case ACT_FACING: if (!(in->pressed & KEY_A) || (u8)b->act[1] != p2->facing) return 0; break;
    case ACT_HELD: if (!(in->current & b->act[1])) return 0; break;
    case ACT_EDGE: {
        s32 now = list[0x21], was = (s32)b->act[1];
        b->act[1] = (u32)now;
        if (was == now || was + 1 == now) return 0;
        break;
    }
    case ACT_ENTER: case ACT_ALWAYS: break;
    default: return 0;
    }
    if (!gate(b, p2)) return 0;
    Entity *who = g_PlayerObject;
    switch (b->vtable) {
    case BOX_DOOR: case BOX_WALKOFF:                           // his key, his spot; both go, as when player 1 walks out
        if (!(in->current & b->key)) return 0;
        g_InputSystem->current |= b->key;
        who = p2;
        break;
    case BOX_SAVE: who = p2; break;                            // his spot is saved, his menu
    case BOX_SCRIPT:
        if (av == ACT_A) ag.pad = 2;
        if (av != ACT_HELD) bring_p1(p2);
        break;
    case BOX_DIALOGUE:
        if (av == ACT_ENTER) bring_p1(p2);
        break;
    default: return 0;                                         // dropped items are pickup.c's
    }
    s32 gen = list[0x22];
    ag.modal = 1;
    int fired = fire_box(b, who);
    ag.modal = 0;
    ag.pad = 0;
    if (fired && (av == ACT_ENTER || av == ACT_HELD) && list[0x22] == gen) Box_Remove(list, b);
    if (ag.pending) {                                          // the switch screen is gone: a bigger body can be rebuilt
        p2_bind_slot(ag.pending - 1);
        ag.pending = 0;
    }
    return fired;
}

// 0x0800BFC0 (entry jump): the stock scan for player 1, then the same boxes for player 2.
void trigger_scan_hook(s32 *list) {
    Entity *p1 = g_PlayerObject;
    if (!p1 || !g_Party->chars[g_Party->active_slot].hp) return;
    s32 x = p1->x >> 8, y = p1->y >> 8, gen = list[0x22];
    int a = g_InputSystem->pressed & KEY_A;
    list[0x21]++;
    list[0x23] = 1;
    int fired = 0;
    for (s32 i = list[0]; i > 0 && !fired; i--) {
        Box *b = (Box *)list[i];
        if (x < b->x1 || x >= b->x2 || y < b->y1 || y >= b->y2) continue;
        const u32 *vt = (const u32 *)*b->act;
        if (a && !gate(b, p1)) continue;
        if (two_players() && b->vtable == BOX_SCRIPT && *b->act == ACT_A) ag.pad = 1;
        fired = ((int (*)(u32 *, Box *, Entity *))((u32)vt + vt[1]))(b->act, b, p1);
        ag.pad = 0;
    }
    list[0x23] = 0;
    if (fired || list[0x22] != gen || !p2_free()) return;
    Entity *p2 = coop.p2;
    x = p2->x >> 8;
    y = p2->y >> 8;
    for (s32 i = list[0]; i > 0; i--) {
        Box *b = (Box *)list[i];
        if (x < b->x1 || x >= b->x2 || y < b->y1 || y >= b->y2) continue;
        if (p2_try_box(b, list)) return;
    }
}

// 0x0800C06E (entry jump), in Player_Attack when the punch hit nothing: a box that would take this A press: no swing.
// The stock asks for player 1; the player punching is asked for here.
int trigger_takes_a_hook(s32 *list) {
    Entity *who = ag.attacker ? ag.attacker : g_PlayerObject;
    if (!who) return 0;
    if (who == coop.p2 && !p2_free()) return 0;
    s32 x = who->x >> 8, y = who->y >> 8;
    for (s32 i = list[0]; i > 0; i--) {
        Box *b = (Box *)list[i];
        if (x < b->x1 || x >= b->x2 || y < b->y1 || y >= b->y2) continue;
        const u32 *vt = (const u32 *)*b->act;
        if (((int (*)(u32 *, Entity *))((u32)vt + vt[2]))(b->act, who) && gate(b, who)) return 1;
    }
    return 0;
}

void agency_attack(Entity *e) {
    ag.attacker = e;
    Player_Attack(e);
    ag.attacker = 0;
}

// ---- pads -------------------------------------------------------------------------------------------------------------
static int on_world_map(void) {
    const u8 *m = (const u8 *)g_MapState;
    return m && m[0] == WORLD_ZONE && m[1] == WORLD_AREA;
}

// Every poll, before the stock input is filled: what player 2 fired runs on his pad (a dialogue inside it goes by
// agency_scene_press as any other). The world map he took off to, until the flyer lands (Ryan's rule: whoever stepped on
// the sign).
void agency_pads(u16 *p1, u16 *p2) {
    if (ag.world) {
        if (on_world_map()) ag.world = 2;
        else if (ag.world == 2) ag.world = 0;
    }
    if (!p2_pad_present() || players_one()) { ag.world = 0; return; }
    if ((ag.modal || ag.world == 2) && !scene_running()) {
        *p1 = *p2;
        *p2 = 0;
    }
}

// Scenes: player 2's A and B go into the stock pressed bits (a press is an edge on his own pad, whatever the solo mode
// or the revive made of it). A press of one pad shuts the other out for a moment, so that two people pressing for the
// same line close it once.
#define PRESS_QUIET 10
void agency_scene_press(InputSystem *in, u16 pad2) {
    u16 edge = (u16)(pad2 & ~ag.prev2 & (KEY_A | KEY_B));
    ag.prev2 = pad2;
    if (!scene_running() || companion_active() || !p2_pad_present() || players_one()) { ag.quiet = 0; return; }
    u16 mine = (u16)(in->pressed & (KEY_A | KEY_B));
    if (ag.quiet) {
        ag.quiet--;
        if (ag.quiet_by == 1) edge = 0;
        else { in->pressed &= (u16)~(KEY_A | KEY_B); mine = 0; }
    }
    if (mine) { ag.quiet = PRESS_QUIET; ag.quiet_by = 1; }
    else if (edge) { in->pressed |= edge; ag.quiet = PRESS_QUIET; ag.quiet_by = 2; }
}

// ---- the save pad's switch screen (switch.c) ----------------------------------------------------------------------------
int agency_menu_is_p2(void) { return ag.modal && coop.p2; }

void agency_menu_choose(int slot) {
    if (slot == coop.p2_slot) return;
    if (p2_body_fits(slot)) p2_bind_slot(slot);
    else ag.pending = (u8)(slot + 1);                  // the entity list is swapped out under this screen: after it
}

// ---- flight pads: both go -----------------------------------------------------------------------------------------------
// Everyone flies but Hercule (party slot 5, in every form: no flying pose, docs/hercule.md): he runs along, on a command
// that is the fly command but for its start (private vtable: the walking mover at the flight's speed, the dash's pose).
static void run_start(u32 *cmd, Entity *e) {
    struct { u32 zero; s16 x, y, speed; } go = { 0, (s16)cmd[4], (s16)cmd[5], 0x200 };
    ((void (*)(Entity *))vslot(e, 12))(e);
    cmd[3] = (u32)Mover_Walk(&go, e);
    Entity_SetAnimation(e, ANIM_DASH);
}

static void go_cmd(Entity *e, int slot, s32 x, s32 y) {
    u32 *c = Arena_Alloc(0x18);
    if (!c) return;
    c[0] = FLY_CMD_VT;
    if (slot == HERCULE_SLOT) {
        u32 base = (u32)ag.run_vt;
        const u32 *fly = (const u32 *)FLY_CMD_VT;
        for (int i = 0; i < 6; i++) ag.run_vt[i] = (FLY_CMD_VT + fly[i]) - base;
        ag.run_vt[1] = (u32)run_start - base;
        c[0] = base;
    }
    c[1] = c[2] = c[3] = 0;
    c[4] = (u32)x;
    c[5] = (u32)y;
    Cmd_Queue(CMD_BATCH, c, e);
}

static void p2_goes_along(s32 x, s32 y) {
    Entity *p2 = coop.p2;
    // standing, walking or dashing only: forcing him out of an ability's state frees what it holds (p2_cycle's crash)
    if (!p2 || !p2_in_control() || revive_is_down(p2) || *(u32 *)((u8 *)p2 + 0x150)) return;
    go_cmd(p2, coop.p2_slot, x - 16, y);
}

// VM step 54 FlyToPosition (data/hercule.json): a flight circle's flight of the player takes player 2 along
void vm_fly_hook(s32 *st) {
    s32 n = st[0];
    if (!ag.pad || !two_players() || st[n - 2] != 0 || *CMD_BATCH) { Step_FlyTo(st); return; }
    s32 x = st[n - 1], y = st[n];
    Cmd_Begin(CMD_BATCH);
    if (g_Party->active_slot == HERCULE_SLOT) {
        st[0] = n - 3;
        go_cmd(g_PlayerObject, HERCULE_SLOT, x, y);
    } else {
        Step_FlyTo(st);
    }
    p2_goes_along(x, y);
    Cmd_Commit(CMD_BATCH);
}

// VM step 38 WarpToMapWithEntities: a world-map pad flies both players off the screen first; with player 2's take-off his
// pad drives the flyer
void vm_warp_hook(s32 *st) {
    s32 n = st[0];
    int to_world = ag.pad && st[n - 4] == WORLD_ZONE && st[n - 3] == WORLD_AREA;
    if (to_world && two_players()) {
        const s32 *cam = CAMERA_CELL;
        Entity *p1 = g_PlayerObject;
        s32 top = cam ? cam[8] - 48 : (p1->y >> 8) - 160;
        Cmd_Begin(CMD_BATCH);
        go_cmd(p1, g_Party->active_slot, p1->x >> 8, top);
        p2_goes_along(p1->x >> 8, top);
        Cmd_Commit(CMD_BATCH);
    }
    if (to_world && ag.pad == 2) ag.world = 1;
    Step_Warp(st);
}
