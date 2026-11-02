// Where the co-op extras sit in OBJ VRAM. The stock allocator (64-byte blocks, best fit) carves every block from the
// top of a free block, and a text box opens only with one contiguous free run of 0x2800 or 0x3800 bytes (0x0800B1E2,
// then 0x0800700E): short of that it takes entities off the list, off-screen ones at round 13 and then, at round 15,
// every one on screen including the players and the HUD, until the box closes. Alone that happens on full maps only;
// player 2's body and panel, created last, landed in the middle of the free run, cut it in two, and West City's
// arrival lost its whole crowd for the first line (docs/perfect-pass.md 11.6). So they sit at the free run's low end
// and stock allocations keep coming down from the top: the room between stays one piece.
//
// When the room still does not fit (a full map), obj_make_room_hook replaces 0x0800700E: first those two blocks settle
// low again, then the stock's rounds as they are
// (round 4 the HUD, 8-9 effects, 13 what is off screen), but its round 15, everything on screen at once, goes one
// entity at a time and stops when the box fits: the body whose block makes the longest free run first, then the rest,
// the players last. Test command 45 holds a block of OBJ VRAM to force this; `perf`'s `drop` and tools/headless/perf_budget.py see it.
#include "coop.h"
#include "objroom.h"

#define BODY_SLOT6   0x080078A3u   // a body's "may I go" (slot 6): off screen from round 13, on screen at round 15
#define NO_VRAM_SLOT7 0x080073B9u  // an entity with nothing to give back (the map's system entities)

ROM_FN(void, Entity_Suspend, (void *list, void *e), 0x08007163);       // slot 7, off the list, onto the list's aside pile (+0x40C)
ROM_FN(void, EntityList_RemoveAt, (s32 *count, int i), 0x08026529);
ROM_FN(void, EntityList_Draw, (void *list), 0x08006AD9);               // every OBJ again, without the ones gone
ROM_FN(void, EntityList_Upload, (void *list), 0x08006AA7);             // the shadow to OAM, the queued frames to VRAM

ObjRoomState obj_room __attribute__((section(".combo.pf")));
_Static_assert(sizeof(ObjRoomState) <= 0x10, ".combo.pf is at most 0x10 bytes");

ROM_FN(int, ObjVram_Largest, (void *list), 0x08006D71);   // largest free block (bytes); 0 when the list is full

// A block at the low end of the largest free block: a block on its top for the rest, then the wanted one, which
// best fit gives what is left of it (or a smaller hole that fits: better still).
u16 obj_alloc_low(int bytes) {
    void *list = g_ObjVramList;
    int largest = ObjVram_Largest(list);
    if (largest <= bytes) return ObjVram_Alloc(list, bytes);
    u16 top = ObjVram_Alloc(list, largest - bytes);
    u16 tile = ObjVram_Alloc(list, bytes);
    if (top != 0xFFFF) ObjVram_Free(list, top);
    if (tile == 0xFFFF) tile = ObjVram_Alloc(list, bytes);   // the allocator's records ran out: anywhere
    return tile;
}

// An entity's body block (+0x0C the tile, +0x0E its size) moved low; +0x20 cleared uploads the frame again.
void obj_entity_low(Entity *e) {
    u16 *tile = (u16 *)((u8 *)e + 0x0C);
    ObjVram_Free(g_ObjVramList, *tile);
    *tile = obj_alloc_low(*(u16 *)((u8 *)e + 0x0E));
    *(u32 *)((u8 *)e + 0x20) = 0;
}

static int listed(const EntityList *l, const Entity *e) {
    for (int i = 0; i < l->count; i++) if (l->items[i] == e) return 1;
    return 0;
}

// A block moved where it cuts nothing: into a hole it fits if there is one (best fit took it from anywhere but the
// largest run), else to the free run's low end.
static u16 settle_block(u16 tile, int bytes) {
    void *list = g_ObjVramList;
    int largest = ObjVram_Largest(list);
    u16 fresh = ObjVram_Alloc(list, bytes);
    if (fresh != 0xFFFF && ObjVram_Largest(list) == largest) {
        ObjVram_Free(list, tile);
        return fresh;
    }
    if (fresh != 0xFFFF) ObjVram_Free(list, fresh);
    ObjVram_Free(list, tile);
    return obj_alloc_low(bytes);
}

// Player 2's body and panel again: since they were placed, what stood beside them may have left (a duplicate NPC, the
// map's name), so this can join two free runs. Nobody sees it: the next draw uploads both again.
static void settle_ours(EntityList *l) {
    if (coop.p2 && listed(l, coop.p2)) {
        u16 *tile = (u16 *)((u8 *)coop.p2 + 0x0C);
        *tile = settle_block(*tile, *(u16 *)((u8 *)coop.p2 + 0x0E));
        *(u32 *)((u8 *)coop.p2 + 0x20) = 0;
    }
    if (coop.hud2.size) {
        coop.hud2.tile = settle_block(coop.hud2.tile, 0x500);
        coop.hud2.hp_px = 0xFF;                  // compose_panel draws it anew
    }
    EntityList_Draw(l);
    EntityList_Upload(l);
}

static int answer(Entity *e, int round) { return ((int (*)(Entity *, int))vslot(e, 6))(e, round); }
static int room_for(void *list, int need) { return ObjVram_Largest(list) >= need; }

// A stock round: every entity that answers leaves (1 destroyed, 2 aside until 0x0800712E brings it back).
static void stock_round(EntityList *l, int round) {
    for (int i = l->count - 1; i >= 0; i--) {
        Entity *e = l->items[i];
        int a = answer(e, round);
        if (a == 1) {
            ((void (*)(Entity *))vslot(e, 1))(e);
            EntityList_RemoveAt(&l->count, i);
        } else if (a == 2) {
            Entity_Suspend(l, e);
        }
    }
    EntityList_Draw(l);
    EntityList_Upload(l);
}

// The free run an entity's block would make (bytes): its block and the free blocks beside it. The allocator's records:
// list + 0x694 + id * 10 = { u8 used, pad, u16 offset, u16 size (64-byte units), u8 next, u8 prev by address, ... },
// list[0xC00 + tile / 2] = the id of the block at that tile.
static int run_if_freed(EntityList *l, const Entity *e) {
    const u8 *list = (const u8 *)l;
    u16 tile = *(const u16 *)((const u8 *)e + 0x0C);
    const u8 *rec = list + 0x694 + list[0xC00 + (tile >> 1)] * 10;
    int size = *(const u16 *)(rec + 4);
    for (int side = 6; side <= 7; side++) {
        if (rec[side] == 0xFF) continue;
        const u8 *nb = list + 0x694 + rec[side] * 10;
        if (!nb[0]) size += *(const u16 *)(nb + 4);
    }
    return size << 6;
}

// Round 15 one at a time: the body on screen whose block makes the longest free run, then the farthest from player 1.
static Entity *next_body(EntityList *l, Entity *p1, Entity *p2) {
    Entity *best = 0;
    int best_run = -1;
    s32 best_d = -1;
    for (int i = 0; i < l->count; i++) {
        Entity *e = l->items[i];
        if (e == p1 || e == p2 || (u32)vslot(e, 6) != BODY_SLOT6 || answer(e, 15) != 2) continue;
        int run = run_if_freed(l, e);
        s32 dx = p1 ? (e->x - p1->x) >> 8 : 0, dy = p1 ? (e->y - p1->y) >> 8 : 0;
        s32 d = dx * dx + dy * dy;
        if (run > best_run || (run == best_run && d > best_d)) { best = e; best_run = run; best_d = d; }
    }
    return best;
}

static void round15(EntityList *l, int need) {
    Entity *p1 = g_PlayerObject, *p2 = coop.p2, *e;
    if (p1 && listed(l, p1)) obj_entity_low(p1);   // no one sees a body move: its old block joins what is free beside it
    while (!room_for(l, need) && (e = next_body(l, p1, p2)) != 0) {
        Entity_Suspend(l, e);
        obj_room.taken++;
    }
    for (int i = l->count - 1; i >= 0 && !room_for(l, need); i--) {   // what is not a body: the decorations
        if (i >= l->count) continue;
        e = l->items[i];
        if (e == p1 || e == p2 || (u32)vslot(e, 7) == NO_VRAM_SLOT7 || answer(e, 15) != 2) continue;
        Entity_Suspend(l, e);
    }
    if (p2 && listed(l, p2) && !room_for(l, need)) { Entity_Suspend(l, p2); obj_room.taken++; }
    if (p1 && listed(l, p1) && !room_for(l, need)) { Entity_Suspend(l, p1); obj_room.taken++; }
    EntityList_Draw(l);
    EntityList_Upload(l);
}

void obj_make_room_hook(EntityList *l, int need) {
    if (room_for(l, need)) return;
    obj_room.calls++;
    obj_room.taken = 0;
    obj_room.round = 0;
    settle_ours(l);
    int round = 0;
    for (; round < 15 && !room_for(l, need); round++) stock_round(l, round);
    if (!room_for(l, need)) round15(l, need);
    for (; round < 64 && !room_for(l, need); round++) stock_round(l, round);   // the stock would spin here for ever
    obj_room.round = (u8)round;
}

// Test command 45: arg bytes of OBJ VRAM held at the largest free block's low end (a map fuller than it is); 0 gives
// them back. The answer: cmd_arg := the largest free block now.
void obj_room_test(u32 arg) {
    if (obj_room.hold) { ObjVram_Free(g_ObjVramList, obj_room.hold - 1); obj_room.hold = 0; }
    if (arg) obj_room.hold = obj_alloc_low((int)arg) + 1;
    coop2.cmd_arg = (u32)ObjVram_Largest(g_ObjVramList);
}
