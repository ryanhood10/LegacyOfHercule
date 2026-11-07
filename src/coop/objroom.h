// objroom.c: OBJ VRAM's room for the text box in co-op (docs/perfect-pass.md 11.6, docs/engine.md "OBJ VRAM and the text box").
#pragma once
#include "coop.h"

typedef struct ObjRoomState {   // .combo.pf, at most 0x10 bytes (docs/ids.md)
    u16 hold;                   // +0: test command 45's block (its tile + 1; 0 none)
    u8 round;                   // +2: the last eviction's last round
    u8 taken;                   // +3: on-screen bodies its round 15 took (the crowd's vanishing, 0 when none)
    u32 calls;                  // +4: evictions so far (a text box or an effect that did not fit)
    u8 decor;                   // +8: the last eviction set the map's decorations aside while none was in the view (1)
    u8 moved;                   // +9: bodies the last eviction gave a new block to join the free run (compact)
} ObjRoomState;
extern ObjRoomState obj_room;

u16 obj_alloc_low(int bytes);
void obj_entity_low(Entity *e);
void obj_make_room_hook(EntityList *l, int need);   // entry of 0x0800700E
void obj_room_test(u32 arg);
