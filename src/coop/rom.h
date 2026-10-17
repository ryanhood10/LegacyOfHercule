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
    u8 facing;            // +0x010  0 down, 1 up, 2 left, 3 right
    u8 _p1[7];
    void *ctrl_state;     // +0x018  control state object (idle / walk / dash ...; anim table 0x083B5268)
    void *aux_state;      // +0x01C  secondary state (default 0x03001F3C, vtable 0x08024188; hurt/death overrides)
    u8 _p2[0x28];
    void *sprite_record;  // +0x048
    u8 _p3[0x10C];
    s32 x;                // +0x158  8.8 fixed
    s32 y;                // +0x15C
    u8 _p4[0x20];
    void *behaviour;      // +0x180  display-row behaviour class
    u8 _p5[4];
    s32 side_speed;       // +0x188  used when a second direction is held
    s32 speed;            // +0x18C
    u8 _p6[0x10];
} Entity;
_Static_assert(sizeof(Entity) == 0x1A0, "Entity size");

typedef struct InputSystem {
    u8 history_head;      // +0x00
    u8 history_armed;     // +0x01
    u16 history[8];       // +0x02
    u16 current;          // +0x12  active-high
    u16 pressed;          // +0x14
    u16 released;         // +0x16
} InputSystem;

typedef struct CharacterEntry {
    u16 hp, max_hp, ep, max_ep;   // +0x00
    u8 display_row;               // +0x08
    u8 ability_index;             // +0x09
    u16 str, pow, end;            // +0x0A
    u32 exp;                      // +0x10
    u32 flags;                    // +0x14  bit0 = in party
    u8 ability_list[4];           // +0x18
} CharacterEntry;
_Static_assert(sizeof(CharacterEntry) == 0x1C, "CharacterEntry size");

typedef struct PartyState {
    u8 active_slot;               // +0x00
    u8 _p0[0x0B];
    CharacterEntry chars[6];      // +0x0C
} PartyState;

typedef struct AbilityDef {
    void *ready_icon, *cooling_icon;
    int (*is_ready)(CharacterEntry *);
    void (*on_use)(CharacterEntry *, Entity *);
} AbilityDef;

// GBA keys (active-high in InputSystem)
#define KEY_A 1
#define KEY_B 2
#define KEY_SELECT 4
#define KEY_START 8
#define KEY_RIGHT 0x10
#define KEY_LEFT 0x20
#define KEY_UP 0x40
#define KEY_DOWN 0x80
#define KEY_DPAD 0xF0

#define REG_KEYINPUT     (*(volatile u16 *)0x04000130)
#define REG_SIOMULTI0    (*(volatile u16 *)0x04000120)   // data received from GBA 0 (master)
#define REG_SIOMULTI1    (*(volatile u16 *)0x04000122)   // ... from GBA 1
#define REG_SIOCNT       (*(volatile u16 *)0x04000128)
#define REG_SIOMLT_SEND  (*(volatile u16 *)0x0400012A)
#define REG_RCNT         (*(volatile u16 *)0x04000134)
#define REG_DISPCNT      (*(volatile u16 *)0x04000000)
#define REG_VCOUNT       (*(volatile u16 *)0x04000006)
#define REG_IME          (*(volatile u16 *)0x04000208)
#define SIOCNT_MULTI_115200 0x2003   // multiplayer mode, 115200 baud
#define SIOCNT_START        0x0080
#define SIO_MARK            0x4000   // set on every controller word; 0xFFFF (no cable) fails the test
#define SIO_VALID(v)        (((v) & 0xFC00) == SIO_MARK)   // marker set, bits 10-13 and 15 clear

#define g_PlayerObject   (*(Entity **)0x03001FA8)
#define g_InputSystem    ((InputSystem *)0x0300229C)
#define g_FrameCounter   (*(u32 *)0x03000E20)
#define g_LastInputFrame (*(u32 *)0x03002298)
#define g_Party          ((PartyState *)0x03000E90)
#define g_LastDir        (*(u16 *)0x03001F40)   // direction that started the current walk
#define g_LastDirFrame   (*(u32 *)0x03001F60)   // frame of the last d-pad press (double-tap)
#define g_Ctx            ((const u32 *)0x083B50E8) // [4] collision map, +0x9C facing table, +0xC0 vectors
#define g_SceneTable     ((void **)0x083ED1B8)      // [1] entity list used by the map loader
#define g_AudioMixer     ((u8 *)0x030026BC)
#define g_Camera         ((void *)0x030020DC)
#define CAMERA_CELL      (*(const s32 **)0x081DA974)   // ROM cell -> camera object; +0x1C/+0x20 = view origin (world px)
#define SFX_TABLE        ((const u8 *)0x080E15B0)
#define DISPLAY_DATA     ((const u8 *)0x081D967C)   // 22 rows x 0x14; +1 spriteId, +0x10 ppVTable
#define ABILITY_TABLE    ((const AbilityDef *)0x081D9900)
#define PLAYER_VTABLE    ((const u32 *)0x08025604)

#define ANIM_IDLE 0
#define ANIM_WALK 1
#define ANIM_DASH 0x0D
#define ANIM_FIDGET 0x0E
#define ANIM_BUMP 0x1F

typedef struct Hud {
    u32 *vtable;          // +0x00
    s32 f4;               // +0x04
    u8 kind;              // +0x08  4
    u8 _p0[3];
    u16 tile;             // +0x0C  OBJ VRAM tile base (0x500 bytes)
    u16 size;             // +0x0E  0x500
    u8 hp_px, ep_px, xp_px, level;   // +0x10..+0x13 caches
    u8 flash;             // +0x14  level-up flash counter
    u8 meter;             // +0x15
    u8 _p1[2];
    CharacterEntry *entry;// +0x18
    const void *icon;     // +0x1C
    s32 y_vel;            // +0x20
    s32 y;                // +0x24
} Hud;
_Static_assert(sizeof(Hud) == 0x28, "Hud size");

#define IDLE_VTABLE       0x08025A84u   // player control states
#define WALK_VTABLE       0x080256C4u
#define HUD_VTABLE        0x0802408Cu
#define HUD_PANEL_TILES   ((const void *)0x086D7B54)   // 768 bytes, 256-colour
#define HUD_HP_TABLE      ((const s32 *)0x083B5358)    // 0x30 x {a,b,c,pad}: pixel offsets
#define HUD_EP_TABLE      ((const s32 *)0x083B5658)    // 0x31 x {a,b,c,pad}
#define HUD_METER_TABLE   ((const u8 *)0x083B5BE0)     // +meter*4 count; -0x50 offsets; +0x14 flash colours
#define HUD_XP_TABLE      ((const s32 *)0x083B5968)    // 0x45 x {a,b}
#define g_ObjVramList     (*(void **)(0x083B5348 + 8))
#define OBJ_VRAM          0x06010000u
#define REG_DMA3SAD       (*(volatile u32 *)0x040000D4)
#define REG_DMA3DAD       (*(volatile u32 *)0x040000D8)
#define REG_DMA3CNT_L     (*(volatile u16 *)0x040000DC)
#define REG_DMA3CNT_H     (*(volatile u16 *)0x040000DE)
#define KEY_L 0x200
#define KEY_R 0x100

#define ROM_FN(ret, name, args, addr) static ret (*const name) args = (ret (*) args)(addr)

ROM_FN(u16, ObjVram_Alloc, (void *list, int bytes), 0x080067AD);
ROM_FN(void, ObjVram_Free, (void *list, u16 tile), 0x08006665);
ROM_FN(int, Character_GetLevel, (const CharacterEntry *c), 0x08004215);
ROM_FN(int, Character_ExpIntoLevel, (const CharacterEntry *c, int level), 0x0800425B);
ROM_FN(int, Character_LevelExp, (int level), 0x0800426B);
ROM_FN(int, Ability_Cycle, (CharacterEntry *c), 0x08004285);
ROM_FN(void, Hud_Draw, (Hud *h, u32 **oam), 0x08008BEF);
ROM_FN(void, Hud_Update, (Hud *h), 0x08008B89);
ROM_FN(void, Hud_Destroy, (Hud *h, int free_it, u32 a, u32 b), 0x08026C25);

ROM_FN(Entity *, PlayerEntity_Create, (void *mem, void *sprite_record, const s16 *xy), 0x080086D5);
ROM_FN(void, EntityList_Add, (void *list, Entity *e), 0x080068B9);
ROM_FN(void, Entity_BaseDestroy, (Entity *e, int flag, u32 a, u32 b), 0x08019B49);
ROM_FN(void, Arena_Free, (void *p), 0x0801F111);
ROM_FN(void, SoftReset, (void), 0x0802135D);
ROM_FN(void *, CharacterEntry_GetSpriteRecord, (const CharacterEntry *c), 0x0802645D);
ROM_FN(int, Collision_Check, (const void *map, const s32 *xy, void *extra), 0x08006367);
ROM_FN(void, Camera_Shake, (void *cam, int a, int b), 0x0800DD81);
ROM_FN(void, Sfx_Play, (void *channel, const void *sfx, int vol, int pan), 0x0801FC9D);
ROM_FN(void, Player_Attack, (Entity *e), 0x08007DE9);
ROM_FN(void, Player_StepAxis, (Entity *e, const s32 *xy, int axis), 0x08007D25);
ROM_FN(void, IdleState_Update, (void *state, Entity *e), 0x08007F35);

// vtable slot calls: function = table + table[slot]
static inline void *vslot(const void *obj, int slot) {
    const u32 *vt = *(const u32 *const *)obj;
    return (void *)((u32)vt + vt[slot]);
}
static inline void Entity_SetAnimation(Entity *e, int anim) {
    ((void (*)(Entity *, int))vslot(e, 10))(e, anim);
}
