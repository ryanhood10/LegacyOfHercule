// Second HUD panel for player 2, drawn right after player 1's by hooking the HUD object's vtable
// (draw / update / destroy). The panel composition is a port of the original draw routine.
#include "coop.h"
#include "wide.h"

static void dma_copy32(const void *src, void *dst, int words) {
    REG_DMA3SAD = (u32)src;
    REG_DMA3DAD = (u32)dst;
    REG_DMA3CNT_L = (u16)words;
    REG_DMA3CNT_H = 0x8400;
}

static void emit_sprites(const Hud *h, u32 **oam, int x_offset) {
    u32 *o = *oam;
    u32 y = h->y & 0xFF;
    // attr1 x is 9 bits; the wide-view fork reads bit 9 of x from attr1 bit 9 (word bit 25)
    u32 xo = (((u32)x_offset & 0x1FF) << 16) | ((((u32)x_offset >> 9) & 1) << 25);
    o[0] = (0x80086000u + xo) | y;
    o[1] = (o[1] & 0xFFFFFC00u) | (h->tile & 0x3FF);
    o[2] = (0x40282000u + xo) | y;
    o[3] = (o[3] & 0xFFFFFC00u) | ((h->tile + 0x10) & 0x3FF);
    o[4] = (0x80386000u + xo) | y;
    o[5] = (o[5] & 0xFFFFFC00u) | ((h->tile + 0x18) & 0x3FF);
    *oam = o + 6;
}

static void compose_panel(Hud *h, CharacterEntry *c) {
    u8 hp_px = c->max_hp ? (u8)((c->hp * 0x30) / c->max_hp) : 0;
    u8 ep_px = c->max_ep ? (u8)((c->ep * 0x31) / c->max_ep) : 0;
    int level = Character_GetLevel(c);
    u8 xp_px;
    if (level == 0x32) {
        xp_px = 0x45;
    } else {
        int into = Character_ExpIntoLevel(c, level);
        int span = Character_LevelExp(level + 1) - Character_LevelExp(level);
        xp_px = span ? (u8)((into * 0x45) / span) : 0;
    }
    if (h->entry == c && h->level != level) h->flash = 0x24;
    h->level = (u8)level;
    h->entry = c;
    if (h->flash) h->flash--;

    const AbilityDef *a = &ABILITY_TABLE[c->ability_list[c->ability_index]];
    const void *icon = a->is_ready(c) ? a->ready_icon : a->cooling_icon;

    int meter = *(const s32 *)((const u8 *)g_Party + 8) >> 8;
    if (!(c->flags & 0x20) || meter > 4) meter = 4;

    if ((h->flash & 3) != 3 && h->hp_px == hp_px && h->ep_px == ep_px && h->xp_px == xp_px &&
        h->icon == icon && h->meter == meter)
        return;
    h->hp_px = hp_px; h->ep_px = ep_px; h->xp_px = xp_px; h->icon = icon; h->meter = (u8)meter;

    u8 buf[0x500];
    dma_copy32(HUD_PANEL_TILES, buf, 0xC0);
    dma_copy32(icon, buf + 0x300, 0x80);
    for (int i = hp_px; i < 0x30; i++) {
        const s32 *t = HUD_HP_TABLE + i * 4;
        buf[t[0]] = 1; buf[t[1]] = 1; buf[t[2]] = 1;
    }
    for (int i = ep_px; i < 0x31; i++) {
        const s32 *t = HUD_EP_TABLE + i * 4;
        buf[t[0]] = 1; buf[t[1]] = 1; buf[t[2]] = 1;
    }
    int n = *(const s32 *)(HUD_METER_TABLE + meter * 4);
    for (int j = 0; j < n; j++) buf[*(const s32 *)(HUD_METER_TABLE - 0x50 + j * 4)] = 1;
    u8 colour = h->flash ? HUD_METER_TABLE[0x14 + (h->flash >> 2)] : 1;
    for (int i = xp_px; i < 0x45; i++) {
        const s32 *t = HUD_XP_TABLE + i * 2;
        buf[t[0]] = colour; buf[t[1]] = colour;
    }
    dma_copy32(buf, (void *)(OBJ_VRAM + h->tile * 0x20), 0x140);
}

static void hud2_ensure_tiles(void) {
    if (coop.hud2.tile) return;
    coop.hud2.tile = ObjVram_Alloc(g_ObjVramList, 0x500);
    coop.hud2.size = 0x500;
    coop.hud2.hp_px = 0xFF;
    coop.hud2.flash = 0;
    coop.hud2.entry = 0;
    coop.hud2.icon = 0;
}

void hud_draw_hook(Hud *h, u32 **oam) {
    Hud_Draw(h, oam);
    if (!coop.p2) return;
    hud2_ensure_tiles();
    coop.hud2.y = h->y;
    emit_sprites(&coop.hud2, oam, wide_w() - 96);   // 144 at 240 wide (HUD2_X_OFFSET)
    compose_panel(&coop.hud2, &g_Party->chars[coop.p2_slot]);
}

void hud_update_hook(Hud *h) {
    Hud_Update(h);
    if (coop.p2 && (coop.input2.pressed & KEY_L)) Ability_Cycle(&g_Party->chars[coop.p2_slot]);
}

void hud_destroy_hook(Hud *h, int free_it, u32 a, u32 b) {
    if (coop.hud2.tile) {
        ObjVram_Free(g_ObjVramList, coop.hud2.tile);
        coop.hud2.tile = 0;
    }
    Hud_Destroy(h, free_it, a, b);
}
