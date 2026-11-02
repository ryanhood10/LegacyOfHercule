// Where the co-op extras sit in OBJ VRAM. The stock allocator (64-byte blocks, best fit) carves every block from the
// top of a free block, and a text box opens only with one contiguous free run of 0x2800 or 0x3800 bytes (0x0800B1E2,
// then 0x0800700E): short of that it takes entities off the list, off-screen ones at round 13 and then, at round 15,
// every one on screen including the players and the HUD, until the box closes. Alone that happens on full maps only;
// player 2's body and panel, created last, landed in the middle of the free run, cut it in two, and West City's
// arrival lost its whole crowd for the first line (docs/perfect-pass.md 11.6). So they sit at the free run's low end
// and stock allocations keep coming down from the top: the room between stays one piece.
#include "coop.h"

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
