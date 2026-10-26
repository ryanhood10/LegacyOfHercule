// Overhead HP bars in the tournament ring (Ryan, 2026-10-25: "should we display the hp of the other fighters
// during the royal rumble?" -> a small bar over each head). Every kit fighter standing in the ring that has no boss
// bar of its own (all of a Royal Rumble, a 2 v 2 partner) gets a thin 16 x 8 sprite over the head: green, yellow
// under half, red under a quarter. A fighter at 0 HP loses his bar. 1 v 1 and the 2 v 2 opponents keep the big boss
// bars at the top (tny_engine.c bars()), so nothing changes there.
// Drawn from the HUD's draw hook (hud.c) like the revive bars: 4bpp tiles in palette bank 0 (the game's one OBJ
// palette: 1 black, 4 green, 5 red, 8 yellow, 14 dark grey), one 64-byte pair of tiles per fighter.
#include "coop.h"
#include "tourney.h"
#include "tny_engine.h"
#include "wide.h"

#define SCENE_RUNNING   (*(volatile u32 *)0x03001FDC)
#define HB_MAGIC        0x48504221u            // "!BPH"
#define HB_MAX          8
#define HB_BYTES        (HB_MAX * 64)
#define C_BLACK 1
#define C_GREEN 4
#define C_RED   5
#define C_YELLOW 8
#define C_EMPTY 14

const u32 *tny_moves_base(const Entity *e);   // tny_moves.c: the vtable a kit replaced (0 = not a kit fighter)

static struct { u32 magic; u16 tile; u16 _pad; } hb TNY_SECTION("hp");

static void put(u8 *t, int x, int y, u8 c) {
    u8 *b = t + (x >> 3) * 32 + y * 4 + ((x & 7) >> 1);
    *b = (x & 1) ? (u8)((*b & 0x0F) | (c << 4)) : (u8)((*b & 0xF0) | c);
}

// rows 0-3 of a 16 x 8 sprite: a black frame round a 14 x 2 fill
static void bar(u8 *t, s32 hp, s32 max) {
    int filled = max > 0 ? (int)(14 * hp / max) : 0;
    if (filled < 1) filled = 1;                     // alive: at least a sliver
    if (filled > 14) filled = 14;
    u8 colour = hp * 4 < max ? C_RED : hp * 2 < max ? C_YELLOW : C_GREEN;
    for (int i = 0; i < 64; i++) t[i] = 0;
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 16; x++)
            put(t, x, y, (y == 0 || y == 3 || x == 0 || x == 15) ? C_BLACK : (x - 1 < filled ? colour : C_EMPTY));
}

static void emit(u32 **oam, int x, int y, u16 tile) {
    if (x <= -16 || x >= wide_w() || y <= -8 || y >= 160) return;
    u32 *o = *oam;
    u32 xo = (((u32)x & 0x1FF) << 16) | ((((u32)x >> 9) & 1) << 25);
    o[0] = 0x00004000u | xo | ((u32)y & 0xFF);      // 4bpp, horizontal 16 x 8
    o[1] = (o[1] & 0xFFFF0000u) | tile;             // priority 0, palette bank 0
    *oam = o + 2;
}

static void release(void) {
    if (hb.magic == HB_MAGIC && hb.tile) ObjVram_Free(g_ObjVramList, hb.tile);
    hb.magic = HB_MAGIC;
    hb.tile = 0;
}

static int has_boss_bar(const Entity *e) {
    const EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) {
        const BossBar *b = (const BossBar *)l->items[i];
        if (b && (b->vtable == (u32 *)BOSSBAR_VTABLE || b->vtable == coop2.bar_vtable) && b->entity == e) return 1;
    }
    return 0;
}

void tny_hpbars_draw(u32 **oam) {
    if (hb.magic != HB_MAGIC) { hb.magic = HB_MAGIC; hb.tile = 0; }
    Entity *who[HB_MAX];
    int n = 0;
    if (tny_fight_on() && !SCENE_RUNNING) {   // never over the walk-in, a line or the end poses (polish2)
        const EntityList *l = g_EntityList;
        for (int i = 0; i < l->count && n < HB_MAX; i++) {
            Entity *e = l->items[i];
            if (!e || e->hp <= 0 || !e->behaviour || !tny_moves_base(e) || has_boss_bar(e)) continue;
            who[n++] = e;
        }
    }
    if (!n) { if (hb.tile) release(); return; }
    if (!hb.tile) hb.tile = ObjVram_Alloc(g_ObjVramList, HB_BYTES);
    if (!hb.tile) return;
    u8 buf[64];
    const s32 *cam = CAMERA_CELL;
    for (int i = 0; i < n; i++) {
        const Entity *e = who[i];
        bar(buf, e->hp, ((const EnemyStat *)e->behaviour)->max_hp);
        u8 *vram = (u8 *)(OBJ_VRAM + (hb.tile + i * 2) * 0x20);
        for (int j = 0; j < 64; j += 4) *(u32 *)(vram + j) = *(u32 *)(buf + j);
        emit(oam, (e->x >> 8) - cam[7], (e->y >> 8) - 36 - cam[8], (u16)(hb.tile + i * 2));
    }
}

void tny_hpbars_gone(void) { release(); }
