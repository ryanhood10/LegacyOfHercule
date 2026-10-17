// Shadow-Gohan proof: a second player entity that shares the first one's input.
// The idle state's update slot is redirected to idle_update_hook (see tools/patch_rom.py).
#include "rom.h"

#define COOP_MAGIC 0xC00B0001u

typedef struct CoopState {
    u32 magic;
    Entity *p2;
    u32 p2_vtable[16];
} CoopState;

static CoopState coop;  // EWRAM, placed by the linker script; not zeroed by the game, hence the magic

static void p2_destroy(Entity *e, int free_it, u32 a, u32 b) {
    e->vtable = coop.p2_vtable;
    coop.p2 = 0;
    Entity_BaseDestroy(e, 0, a, b);
    if (free_it) Arena_Free(e);
}

static void build_p2_vtable(void) {
    u32 base = (u32)coop.p2_vtable;
    for (int i = 0; i < 16; i++) coop.p2_vtable[i] = (0x08025604u + PLAYER_VTABLE[i]) - base;
    coop.p2_vtable[0] = (u32)p2_destroy - base;
}

static void ensure_p2(Entity *p1) {
    if (coop.magic != COOP_MAGIC) {
        coop.magic = COOP_MAGIC;
        coop.p2 = 0;
        build_p2_vtable();
    }
    if (coop.p2) return;

    s16 xy[2] = { (s16)((p1->x >> 8) + 24), (s16)(p1->y >> 8) };
    Entity *p2 = PlayerEntity_Create(0, p1->sprite_record, xy);
    g_PlayerObject = p1;  // the constructor claimed the player slot
    if (!p2) return;
    p2->vtable = coop.p2_vtable;
    EntityList_Add(g_SceneTable[1], p2);
    coop.p2 = p2;
}

void idle_update_hook(void *state, Entity *e) {
    if (e == g_PlayerObject) ensure_p2(e);
    IdleState_Update(state, e);
}
