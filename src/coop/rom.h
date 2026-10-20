// Addresses and layouts of the original game that the co-op code calls into (LoG2 USA).
#pragma once

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
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
    u8 stat_melee;        // +0x160  combat bytes (enemies: copied from EnemyStat +0xC..+0x10 by 0x0800F7CC)
    u8 stat_ranged;       // +0x161  attacker stat for ranged hits
    u8 defence;           // +0x162  damage taken scales by (128 - defence) / 128
    u8 res_melee;         // +0x163  hit->secondary = dmg * res / 128 (melee)
    u8 res_ranged;        // +0x164  same for ranged hits
    u8 _p4[0x1B];
    void *behaviour;      // +0x180  player: display-row behaviour class; enemy: EnemyStat *
    void *action;         // +0x184  enemy: record's action object {func, payload}, fired on death
    union {
        s32 side_speed;   // +0x188  player: used when a second direction is held
        s32 hp;           // +0x188  enemy: current HP (max = stat->max_hp)
    };
    s32 speed;            // +0x18C  player speed; enemy: +0x18C/+0x18D flag bytes
    u8 _p6[0x10];
} Entity;
_Static_assert(sizeof(Entity) == 0x1A0, "Entity size");
_Static_assert(__builtin_offsetof(Entity, hp) == 0x188, "Entity hp offset");
#define ENEMY_SIZE 0x1F0   // enemy entities are larger: +0x190 behaviour-list state, +0x19C AI state

// Enemy stat database (0x086D56AC, 24 bytes per entry; MapScript enemy record +8 indexes it).
typedef struct EnemyStat {
    s32 exp;              // +0x00  base EXP (0 for bosses and scripted targets)
    s32 max_hp;           // +0x04
    s32 ai_set;           // +0x08  index into the AI set table 0x086D5194 (8-byte {class, param})
    u8 melee, ranged, defence, res_melee;   // +0x0C..+0x0F -> entity +0x160..+0x163
    s32 res_ranged;       // +0x10  -> entity +0x164 (byte)
    const u32 *cls;       // +0x14  class object: word 2 = death handler (EXP award, popup, removal)
} EnemyStat;
_Static_assert(sizeof(EnemyStat) == 0x18, "EnemyStat size");
#define ENEMY_STATS ((const EnemyStat *)0x086D56AC)

// MapEntry.mapScripts[] record handled by 0x0800E77F (MapScript_CreateEnemy). Size 0x1C + 4*count.
typedef struct EnemyRecord {
    void *handler;        // +0x00  0x0800E77F
    u32 flags;            // +0x04  spawn condition (already evaluated by the map loader)
    s32 stat_index;       // +0x08
    s16 x, y;             // +0x0C  pixels
    s32 sprite_id;        // +0x10  Character_GetSpriteId space (>= 7)
    void *action;         // +0x14  {func, payload}: 0x0801069B bytecode, 0x080106B3 dialog, 0x08010699 none
    s32 behaviour_count;  // +0x18  behaviour list (waypoints, waits) ticked by 0x0800D156
    const void *behaviours[];   // +0x1C
} EnemyRecord;
// Co-op record copies (enemies.c) are allocated with one extra word after the behaviour pointers
// holding the record's original sprite id (partner-model twins change sprite_id).
static inline s32 *record_orig_sprite(EnemyRecord *r) { return (s32 *)&r->behaviours[r->behaviour_count]; }

// Hit descriptor filled by the damage calculation 0x0800F910(defender, hit).
typedef struct Hit {
    s32 kind;             // +0x00  0 = melee popup style (player side)
    s32 mult;             // +0x04  10.10 fixed multiplier (0x400 = 1.0)
    const u8 *attack;     // +0x08  attack descriptor: byte 0 = 0 melee / else ranged, word +4 = base power
    Entity *attacker;     // +0x0C
    s32 crit;             // +0x10  set by the calculation (melee only)
    s32 dmg;              // +0x14  result
    s32 secondary;        // +0x18  dmg * defender resistance / 128
} Hit;

// Boss health bar object (0x1C bytes, vtable 0x08025748), created by BossHealthBar_Create 0x080104C1
// from the DrawBossHealthBar opcodes; two 32x8 sprites at x = 160, y slides from -8 to 8.
typedef struct BossBar {
    u32 *vtable;          // +0x00
    s32 f4;               // +0x04
    u8 kind;              // +0x08  4
    u8 _p0[3];
    u16 tile;             // +0x0C  OBJ VRAM tile base (0x200 bytes)
    u16 size;             // +0x0E  0x200
    Entity *entity;       // +0x10  bound enemy (cleared by list message 0xF when it dies)
    u8 px;                // +0x14  cached fill width (0xFF = redraw)
    u8 lo;                // +0x15  fill pixels at 0 % (lo% * 60 / 100)
    u8 range;             // +0x16  fill pixels spanned by 100 %
    s8 step;              // +0x17  y velocity (1 slide in, -1 slide out)
    s8 y;                 // +0x18
    u8 x;                 // +0x19  (co-op bars only) left edge; the ROM draw uses 160
    s8 y_target;          // +0x1A  (co-op bars only) where the slide-in stops; the ROM update uses 8
    u8 _p1;
} BossBar;
_Static_assert(sizeof(BossBar) == 0x1C, "BossBar size");
#define BOSSBAR_VTABLE      0x08025748u
#define BOSSBAR_BASE_VTABLE 0x08023A1Cu
#define BOSSBAR_FILL_TABLE  ((const s32 *)0x086D60F4)   // 60 x {4 byte offsets} into the 0x200-byte tile block
#define BOSSBAR_EMPTY_TILES ((const void *)0x086D64B4) // 0x200 bytes: the empty bar
#define ENEMY_VTABLE        0x080252A4u
#define ENEMY_STATES        ((void *const *)0x0840DB00)   // enemy control states by index (0 idle, 1 walk, 6/7/8/10 attack groups)
#define AI_SPEED8_VTABLE    0x08023BE0u   // 8-direction chase module {vtable, speed 8.8} (ctor 0x0801B333)
#define AI_SPEED4_VTABLE    0x08023D88u   // axis-aligned approach module (ctor 0x0801B15F)

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

// The party block (stock: g_PartyState at 0x03000E90, 0x164 bytes with six entries). The build relocates
// it into EWRAM (g_party_block, section .party) by repointing the 27 pointer words that hold its
// address, and sizes it for PARTY_SLOTS entries; everything after the entries keeps its stock layout,
// shifted by (PARTY_SLOTS - 6) * 0x1C (tools/patch_rom.py --party-slots patches the ROM's offsets).
#ifndef PARTY_SLOTS
#define PARTY_SLOTS 8
#endif
_Static_assert(PARTY_SLOTS >= 6 && PARTY_SLOTS <= 16, "PARTY_SLOTS range");
#define PARTY_SLOTS_STOCK 6
#define PARTY_TAIL_SHIFT ((PARTY_SLOTS - PARTY_SLOTS_STOCK) * 0x1C)
// Slot 7 is the duel clone's (duel.c; Android 16 borrows it during his chapter, a16player.c); slots after
// it are real party members (8 = Super Saiyan 2 Gohan, ssj2gohan.h). Builds of fewer than 8 slots use the last.
#define DUEL_SLOT (PARTY_SLOTS < 8 ? PARTY_SLOTS - 1 : 7)
typedef struct PartyState {
    u8 active_slot;               // +0x00
    u8 flags;                     // +0x01  bit0 saved once, bit1 meter shown (0x080040AA), bit2 debug party
    u8 zone, area, variation;     // +0x02  save position (map), written by SaveGame
    u8 _p0[3];
    s32 meter;                    // +0x08  HUD meter (24.8)
    CharacterEntry chars[PARTY_SLOTS];   // +0x0C
    u8 event_flags[0x66];         // +0xB4  bit bank 1 (Flag_Test/Set/Clear 0x08003E42/5E/74, 0x08003F06); Party_Init zeroes 50 bytes
    u8 seen_bits[0x10];           // +0x11A bit bank 2: journal / bestiary "seen" bits (0x0800B00C, 0x08018A38, 0x08019204)
    u8 _p1;                       // +0x12A
    u8 counters[0x32];            // +0x12B 50 item / quest counters (0x08004348, 0x080095E4, 0x080096CA, 0x080096E8, 0x0800F3EE)
    u8 _p2[3];                    // +0x15D
    s16 save_x, save_y;           // +0x160 save position (pixels), SaveGame 0x08004030
} PartyState;
_Static_assert(__builtin_offsetof(PartyState, event_flags) == 0xB4 + PARTY_TAIL_SHIFT, "PartyState tail");
_Static_assert(__builtin_offsetof(PartyState, counters) == 0x12B + PARTY_TAIL_SHIFT, "PartyState counters");
_Static_assert(__builtin_offsetof(PartyState, save_x) == 0x160 + PARTY_TAIL_SHIFT, "PartyState save position");
_Static_assert(sizeof(PartyState) == 0x164 + PARTY_TAIL_SHIFT, "PartyState size");
#define PARTY_BYTES sizeof(PartyState)
extern PartyState g_party_block;      // party.c, pinned in the .party EWRAM region (coop.ld)

typedef struct EntityList {
    u32 *vtable;          // +0x000
    s32 count;            // +0x004
    Entity *items[0x182]; // +0x008 .. +0x610
    s32 event_count;      // +0x610
    u32 *events[];        // +0x614  objects: { relative vtable, s32 ticks, ... }; slot 3 fires, slot 2 destroys
} EntityList;
_Static_assert(__builtin_offsetof(EntityList, event_count) == 0x610, "EntityList layout");

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
#define g_Party          (&g_party_block)       // stock 0x03000E90; relocated (see PartyState)
#define g_LastDir        (*(u16 *)0x03001F40)   // direction that started the current walk
#define g_LastDirFrame   (*(u32 *)0x03001F60)   // frame of the last d-pad press (double-tap)
#define g_Ctx            ((const u32 *)0x083B50E8) // [4] collision map, +0x9C facing table, +0xC0 vectors
#define g_SceneTable     ((void **)0x083ED1B8)      // [1] entity list used by the map loader
#define g_EntityList     ((EntityList *)0x03001120)  // == g_SceneTable[1]
#define g_AudioMixer     ((u8 *)0x030026BC)
#define g_Camera         ((void *)0x030020DC)
#define CAMERA_CELL      (*(const s32 **)0x081DA974)   // ROM cell -> camera object; +0x1C/+0x20 = view origin (world px)
#define SFX_TABLE        ((const u8 *)0x080E15B0)
// Character_DisplayData rows (0x14 bytes: +0 flags bit0 transformed, +1 spriteId, +4/+5 transformed/
// detransformed row, +0x10 ppVTable). The build relocates the table into the co-op blob (generated by
// tools/spriteclone.py, stock rows plus Super Saiyan Hercule) and repoints every ROM literal at it, so
// co-op code must read the relocated copy too: the stock table at 0x081D967C has no row 22.
extern const u8 display_rows[];
extern const u32 display_row_count;
#define DISPLAY_DATA     display_rows
#define ABILITY_TABLE    (*(const AbilityDef *const *)0x08008024)   // code literal (0x081D9900 stock; relocated by data/hercule.json)
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

void *memcpy(void *dst, const void *src, unsigned n);   // util.c (freestanding)
void *memset(void *dst, int c, unsigned n);

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
ROM_FN(void *, Arena_Alloc, (int bytes), 0x0801F08D);
ROM_FN(void, Party_AddExp, (PartyState *party, int slot, int amount), 0x08003D59);   // handles level-ups
ROM_FN(Entity *, Enemy_Create, (void *mem, const EnemyRecord *rec, const EnemyStat *stat), 0x0800E281);
ROM_FN(void, Enemy_Destroy, (Entity *e, int free_it, u32 a, u32 b), 0x0800E33B);   // enemy vtable slot 0
ROM_FN(int, Rand_Range, (int n), 0x0802141D);        // n * rand16 >> 16
ROM_FN(int, Rand_Chance, (u32 p), 0x0802142F);       // rand16 < p
ROM_FN(void, EntityList_Remove, (void *list, void *obj, int flag), 0x080068CB);
ROM_FN(void *, Character_GetSpriteRecord, (int sprite_id), 0x08009325);      // 0 player, 1-6 party, >= 7 NPC sprites
ROM_FN(int, Enemy_OnHit, (Entity *e, const u8 *hit), 0x0800E485);           // enemy vtable slot 14
ROM_FN(void, Enemy_RegularDeath, (const u32 *cls, Entity *e), 0x0801A74D);  // EXP, popup + action, list removal, dust, SFX
ROM_FN(u32, EntityList_Broadcast, (void *list, int msg, void *arg), 0x08006DA7);   // every entity's slot 9
#define ENEMY_CLASS_REGULAR ((const u32 *)0x086D2B6C)
#define g_MapState (*(void **)0x083B5C88)   // -> map state (+4 = MapEntry *)
typedef struct ActorRecord {   // MapScript_CreateCharacter record (handler 0x0800D6DF)
    void *handler; u32 flags; s16 x, y; s32 sprite_id; s32 behaviour_count; const void *behaviours[1];
} ActorRecord;
ROM_FN(Entity *, MapScript_CreateCharacter, (const ActorRecord *rec), 0x0800D6DF);
ROM_FN(void, Map_SpawnScript, (void *map, int index), 0x0800C73B);   // TriggerMapEvent opcode 84: mapScripts[index] without its condition
#define ENEMY_ACTION_NONE   0x08010699u

// vtable slot calls: function = table + table[slot]
static inline void *vslot(const void *obj, int slot) {
    const u32 *vt = *(const u32 *const *)obj;
    return (void *)((u32)vt + vt[slot]);
}
static inline void Entity_SetAnimation(Entity *e, int anim) {
    ((void (*)(Entity *, int))vslot(e, 10))(e, anim);
}
