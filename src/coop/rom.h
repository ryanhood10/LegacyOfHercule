// Addresses and layouts of the original game that the co-op code calls into (LoG2 USA).
#pragma once

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef signed int s32;

typedef struct Entity {
    u32 *vtable;          // +0x000  relative-offset vtable
    u8 _p0[0x0C];
    u8 facing;            // +0x010  0 up? 1 down? 2 left 3 right (see idle handler)
    u8 _p1[7];
    void *state;          // +0x018  current state object (anim table 0x083B5268)
    u8 _p2[0x2C];
    void *sprite_record;  // +0x048
    u8 _p3[0x10C];
    s32 x;                // +0x158  8.8 fixed
    s32 y;                // +0x15C
    u8 _p4[0x20];
    void *behaviour;      // +0x180  display-row behaviour class
    u8 _p5[4];
    s32 speed;            // +0x188
    u8 _p6[0x14];
} Entity;
_Static_assert(sizeof(Entity) == 0x1A0, "Entity size");

typedef struct InputSystem {
    u8 history_head;      // +0x00
    u8 history_armed;     // +0x01
    u16 history[8];       // +0x02
    u16 current;          // +0x12
    u16 pressed;          // +0x14
    u16 released;         // +0x16
} InputSystem;

#define g_PlayerObject   (*(Entity **)0x03001FA8)
#define g_InputSystem    ((InputSystem *)0x0300229C)
#define g_SceneTable     ((void **)0x083ED1B8)       // +4 = entity list used by the map loader
#define PLAYER_VTABLE    ((const u32 *)0x08025604)
#define IDLE_VTABLE      0x08025A84u
#define WALK_VTABLE      0x080256C4u

#define ROM_FN(ret, name, args, addr) static ret (*const name) args = (ret (*) args)(addr)

ROM_FN(Entity *, PlayerEntity_Create, (void *mem, void *sprite_record, const s16 *xy), 0x080086D5);
ROM_FN(void, EntityList_Add, (void *list, Entity *e), 0x080068B9);
ROM_FN(void, Entity_BaseDestroy, (Entity *e, int flag, u32 a, u32 b), 0x08019B49);
ROM_FN(void, Arena_Free, (void *p), 0x0801F111);
ROM_FN(void, IdleState_Update, (void *state, Entity *e), 0x08007F35);
