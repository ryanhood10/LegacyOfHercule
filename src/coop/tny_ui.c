// The tournament's screens (ui piece): the screen framework (tny_ui.h), the list box (tny_ui_choose), the pop-up
// (tny_popup_show), CoopCall 28-29 and test commands 28-29. The character select and the unlock board are in
// tny_ui_grid.c, the bracket and the Rumble board in tny_ui_bracket.c.
#include "tny_ui.h"

UiState tny_ui_state TNY_SECTION("ui");

const char *const ui_tier_names[TIER_COUNT] = { "Rookie", "Adept", "Z-Fighter", "Champion", "Legend" };
const char *const ui_div_names[DIV_COUNT] = { "1 v 1", "2 v 2", "Royal Rumble" };

static void state_init(void) {
    if (ui_st.magic == UI_MAGIC) return;
    memset(&ui_st, 0, sizeof ui_st);
    ui_st.magic = UI_MAGIC;
}

// ---- data: the engine's, or the test overrides ----------------------------------------------------------------
int ui_unlocked(int id) {
    if (id < 0 || id >= TF_COUNT) return 0;
    if (ui_st.magic == UI_MAGIC && (ui_st.fake & 1)) return id < 64 && ((ui_st.unlock[id >> 5] >> (id & 31)) & 1);
    return tny_unlocked(id);
}
int ui_tier_open(int tier) {
    if (ui_st.magic == UI_MAGIC && (ui_st.fake & 4)) return (ui_st.tiers >> tier) & 1;
    return tny_tier_open(tier);
}
const TBracket *ui_bracket(void) {
    if (ui_st.magic == UI_MAGIC && (ui_st.fake & 2)) return &ui_st.br;
    return tny_bracket();
}

// ---- sounds ---------------------------------------------------------------------------------------------------
void ui_sfx(int which) {
    static const u16 table[] = {
        [UI_SFX_MOVE] = 0x24C,   // the switch screen's cursor
        [UI_SFX_OK] = 0x6C,      // the switch screen's A / the save pad
        [UI_SFX_BACK] = 0x24C,
        [UI_SFX_BUZZ] = 0x0C,
        [UI_SFX_SLIDE] = 0x24C,
        [UI_SFX_WIN] = 0x30,     // the item pop-up's jingle (index 4)
    };
    Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + table[which], 0x40, 0);
}

// ---- the bitmap layer (BG1: tiles 16..615, 30 x 20, 8bpp) -------------------------------------------------------
#define BM_BASE 0x06000400u
static inline volatile u16 *bm_half(int x, int y) {
    u32 off = (u32)(((y >> 3) * 30 + (x >> 3)) * 64 + (y & 7) * 8 + (x & 6));
    return (volatile u16 *)(BM_BASE + off);
}
static inline void bm_put(int x, int y, int c) {
    if ((unsigned)x >= 240 || (unsigned)y >= 160) return;
    volatile u16 *p = bm_half(x, y);
    if (x & 1) *p = (u16)((*p & 0x00FF) | (c << 8));
    else *p = (u16)((*p & 0xFF00) | c);
}

// DMA fill from a RAM word: with a ROM source the GBA's DMA ignores "fixed" and increments
static void vram_zero(u32 dst, int words) {
    volatile u32 zero = 0;
    REG_DMA3CNT_H = 0;
    REG_DMA3SAD = (u32)&zero;
    REG_DMA3DAD = dst;
    REG_DMA3CNT_L = (u16)words;
    REG_DMA3CNT_H = 0x8500;   // 32-bit, fixed source
}

void bm_clear(void) {
    vram_zero(BM_BASE, 240 * 160 / 4);
}

void bm_fill(int x, int y, int w, int h, int c) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > 240) w = 240 - x;
    if (y + h > 160) h = 160 - y;
    if (w <= 0 || h <= 0) return;
    u16 cc = (u16)(c | (c << 8));
    for (int yy = y; yy < y + h; yy++) {
        int xx = x, end = x + w;
        if (xx & 1) { bm_put(xx, yy, c); xx++; }
        for (; xx + 1 < end; xx += 2) *bm_half(xx, yy) = cc;
        if (xx < end) bm_put(xx, yy, c);
    }
}
void bm_hline(int x, int y, int w, int c) { bm_fill(x, y, w, 1, c); }
void bm_vline(int x, int y, int h, int c) { bm_fill(x, y, 1, h, c); }
void bm_frame(int x, int y, int w, int h, int c) {
    bm_hline(x, y, w, c);
    bm_hline(x, y + h - 1, w, c);
    bm_vline(x, y, h, c);
    bm_vline(x + w - 1, y, h, c);
}

// ---- text (the dialogue font through the stock canvas: +0 align, +4 x, +8 baseline, +0xC font, +0x20 glyph
// drawer, +0x24 pixels, +0x28 shadow colour, +0x29 colour) ---------------------------------------------------------
#define CV_W 240
#define CV_H 16

int ui_text_width(const char *s) {
    const u16 *f = (const u16 *)UI_FONT;
    int w = 0;
    for (; *s && *s != '\n'; s++) w += f[f[(u8)*s]] + 1;
    return w;
}

// Draw a line into the canvas; only the columns [*x0, *x1) are cleared and used (the text's own width).
static void canvas_draw(Ui *u, int x, int align, int colour, int shadow, const char *s, int *x0, int *x1) {
    u8 *c = u->canvas;
    int w = ui_text_width(s) + 3;
    int a = align == 0 ? x : align == 1 ? x - w / 2 - 1 : x - w;
    int b = a + w + 2;
    if (a < 0) a = 0;
    if (b > CV_W) b = CV_W;
    if (b <= a) { *x0 = *x1 = 0; return; }
    for (int r = 0; r < CV_H; r++) memset(u->canvas_buf + r * CV_W + a, 0, (unsigned)(b - a));
    c[0] = (u8)align;
    *(s32 *)(c + 4) = x;
    *(s32 *)(c + 8) = 12;     // the glyphs' top row = baseline - 12
    c[0x28] = (u8)shadow;
    c[0x29] = (u8)colour;
    UiCanvas_Text(c, s);
    *x0 = a;
    *x1 = b;
}

void ui_text(Ui *u, int x, int y, int align, int colour, const char *s) {
    if (!u->canvas_buf || !s) return;
    int a, b;
    canvas_draw(u, x, align, colour, C_BLACK, s, &a, &b);
    const u8 *row = u->canvas_buf;
    for (int r = 0; r < CV_H; r++, row += CV_W)
        for (int i = a; i < b; i++)
            if (row[i]) bm_put(i, y + r, row[i]);
}

// Word wrap into lines of at most w pixels; returns the lines drawn (12 px apart).
int ui_text_wrap(Ui *u, int x, int y, int w, int colour, const char *s, int max_lines) {
    char line[64];
    int n = 0;
    while (*s && n < max_lines) {
        int len = 0, last_space = -1;
        while (s[len] && s[len] != '\n' && len < (int)sizeof line - 1) {
            line[len] = s[len];
            line[len + 1] = 0;
            if (s[len] == ' ') last_space = len;
            if (ui_text_width(line) > w && last_space > 0) { len = last_space; break; }
            len++;
        }
        line[len] = 0;
        ui_text(u, x, y + n * 12, 0, colour, line);
        n++;
        s += len;
        while (*s == ' ' || *s == '\n') s++;
    }
    return n;
}

// A name that fits max_w: "Mercenary Tao" -> "M. Tao", "Goku (Buu Saga)" -> "Goku", else cut.
const char *ui_short_name(const char *name, int max_w, char *buf, int bufsize) {
    if (ui_text_width(name) <= max_w) return name;
    int n = 0;
    for (const char *p = name; *p && *p != '(' && n < bufsize - 1; p++) buf[n++] = *p;
    while (n > 0 && buf[n - 1] == ' ') n--;
    buf[n] = 0;
    if (ui_text_width(buf) <= max_w) return buf;
    char tmp[40];
    int t = 0, word_start = 1;
    int last = 0;
    for (int i = 0; buf[i]; i++) if (buf[i] == ' ') last = i + 1;
    for (int i = 0; buf[i] && t < (int)sizeof tmp - 3; i++) {
        if (i >= last) { tmp[t++] = buf[i]; continue; }
        if (buf[i] == ' ') { word_start = 1; continue; }
        if (word_start) { tmp[t++] = buf[i]; tmp[t++] = '.'; tmp[t++] = ' '; word_start = 0; }
    }
    tmp[t] = 0;
    for (n = 0; tmp[n] && n < bufsize - 1; n++) buf[n] = tmp[n];
    buf[n] = 0;
    while (n > 1 && ui_text_width(buf) > max_w) buf[--n] = 0;
    return buf;
}

// ---- faces (stock Portrait entities) ---------------------------------------------------------------------------
const u8 ui_remap_black[256] = {
    0,
#define B4 OBJ_C_BLACK, OBJ_C_BLACK, OBJ_C_BLACK, OBJ_C_BLACK
#define B16 B4, B4, B4, B4
    OBJ_C_BLACK, OBJ_C_BLACK, OBJ_C_BLACK, B4, B4, B4,
    B16, B16, B16, B16, B16, B16, B16, B16, B16, B16, B16, B16, B16, B16, B16,
#undef B16
#undef B4
};

// A sprite id's record, or 0. tny_fighter_ok() reads Character_GetSpriteRecord, which indexes the relocated
// sprite_index without a bound: an id past its end (art not in this build) returns whatever follows it in ROM.
// So the record is checked for the shape every real one has (docs/sprites.md "Record format").
static int rom_ptr(u32 a) { return a >= 0x08000000u && a < 0x0A000000u && !(a & 3); }
void *ui_sprite_record(int sprite_id) {
    if (sprite_id <= 0) return 0;
    const u8 *rec = Character_GetSpriteRecord(sprite_id);
    if (!rec || !rom_ptr((u32)rec)) return 0;
    u16 bytes = *(const u16 *)rec;
    if (!bytes || bytes > 0x1000 || (bytes & 0x1F)) return 0;
    if (rec[2] == 0 || rec[2] > 40) return 0;
    if (!rom_ptr(*(const u32 *)(rec + 0x4C))) return 0;   // group 0 (idle), facing down
    return (void *)rec;
}
int ui_fighter_ok(int id) {
    return id >= 0 && id < TF_COUNT && tny_fighter_ok(id) && ui_sprite_record(tny_fighters[id].sprite);
}

// The frame box of the face's current group, frame 0, facing down: {xoff, yoff, w, h} from its anchor.
static void face_box(Entity *e, int *dx, int *dy) {
    *dx = -8; *dy = -12;
    const u32 *group = *(const u32 *const *)((const u8 *)e + 0x4C);
    if (!group || !rom_ptr((u32)group)) return;
    const s8 *d = (const s8 *)group[0];
    if (!d || !rom_ptr((u32)d)) return;
    *dx = d[0] + (u8)d[2] / 2;
    *dy = d[1] + (u8)d[3] / 2;
}

// A face whose frame box is centred on (x, y).
Entity *ui_face(int sprite_id, int x, int y) {
    void *rec = ui_sprite_record(sprite_id);
    if (!rec) return 0;
    int bytes = *(const u16 *)rec;
    if (UiObjVram_Largest(g_EntityList) < bytes) { ui_st.heap_fail++; return 0; }
    Entity *e = UiPortrait_Create(0, rec, x, y);
    if (!e) { ui_st.heap_fail++; return 0; }
    u8 *kind = (u8 *)e + 8;
    *kind = (u8)((*kind & ~0x30) | 0x10);     // OBJ priority 1: above the bitmap (2) and the backdrop (3)
    EntityList_Add(g_EntityList, e);
    ui_face_move(e, x, y);
    return e;
}
void ui_face_kill(Entity *e) { if (e) UiEntityList_Delete(g_EntityList, e); }
void ui_face_move(Entity *e, int x, int y) {
    if (!e) return;
    int dx, dy;
    face_box(e, &dx, &dy);
    *(s32 *)((u8 *)e + 0x150) = x - dx;
    *(s32 *)((u8 *)e + 0x154) = y - dy;
}
int ui_face_x(Entity *e) { int dx, dy; if (!e) return 0; face_box(e, &dx, &dy); return *(s32 *)((u8 *)e + 0x150) + dx; }
int ui_face_y(Entity *e) { int dx, dy; if (!e) return 0; face_box(e, &dx, &dy); return *(s32 *)((u8 *)e + 0x154) + dy; }
void ui_face_look(Entity *e, int walking, const u8 *remap) {
    if (!e) return;
    UiEntity_SetCtrlState(e, walking ? UI_STATE_WALK : UI_STATE_IDLE);
    UiEntity_SetRemap(e, remap);
}

// ---- our own OAM entries ---------------------------------------------------------------------------------------
void ui_obj(Ui *u, u32 attr01, u32 attr2) {
    if (u->n_extra >= UI_EXTRA_MAX) return;
    u->extra[u->n_extra * 2] = attr01;
    u->extra[u->n_extra * 2 + 1] = attr2;
    u->n_extra++;
}

// Linear 8bpp pixels -> 8bpp OBJ tiles (1D mapping: tiles row by row), w and h multiples of 8.
static void obj_upload(u16 tile, const u8 *px, int w, int h) {
    volatile u16 *dst = (volatile u16 *)(OBJ_VRAM + tile * 0x20);
    for (int ty = 0; ty < h / 8; ty++)
        for (int tx = 0; tx < w / 8; tx++)
            for (int r = 0; r < 8; r++) {
                const u8 *s = px + (ty * 8 + r) * w + tx * 8;
                for (int i = 0; i < 8; i += 2) *dst++ = (u16)(s[i] | (s[i + 1] << 8));
            }
}

u16 ui_portrait_load(int portrait_id) {
    if (portrait_id <= 0) return 0xFFFF;
    const u32 *res = UiPortrait_Resource(portrait_id);
    if (!res || (u32)res < 0x08000000u || res[1] < 0x1000 || res[1] > 0x1000) return 0xFFFF;
    if (UiObjVram_Largest(g_EntityList) < 0x1000) { ui_st.heap_fail++; return 0xFFFF; }
    u8 *buf = Arena_Alloc(0x1000);
    if (!buf) { ui_st.heap_fail++; return 0xFFFF; }
    UiGfx_Decompress(res, buf);
    u16 tile = ObjVram_Alloc(g_EntityList, 0x1000);
    if (tile != 0xFFFF) obj_upload(tile, buf, 64, 64);
    Arena_Free(buf);
    return tile;
}
void ui_portrait_draw(Ui *u, u16 tile, int x, int y) {
    if (tile == 0xFFFF) return;
    // 64 x 64, 8bpp (attr0 bit 13), square; priority 1
    ui_obj(u, ((u32)y & 0xFF) | 0x2000u | (((u32)x & 0x1FF) << 16) | (3u << 30), tile | (1u << 10));
}

static void qmark_make(Ui *u) {
    u->qmark_tile = 0xFFFF;
    if (!u->canvas_buf || UiObjVram_Largest(g_EntityList) < 0x100) return;
    u16 tile = ObjVram_Alloc(g_EntityList, 0x100);
    if (tile == 0xFFFF) return;
    int a, b;
    memset(u->canvas_buf, 0, CV_W * CV_H);
    canvas_draw(u, 8, 1, OBJ_C_WHITE, OBJ_C_BLACK, "?", &a, &b);
    u8 px[16 * 16];
    for (int r = 0; r < 16; r++)
        for (int i = 0; i < 16; i++) px[r * 16 + i] = u->canvas_buf[r * CV_W + i];
    obj_upload(tile, px, 16, 16);
    u->qmark_tile = tile;
}
void ui_qmark_draw(Ui *u, int x, int y) {
    if (u->qmark_tile == 0xFFFF) return;
    x -= 8; y -= 8;
    // 16 x 16, 8bpp, priority 0 (over the faces)
    ui_obj(u, ((u32)y & 0xFF) | 0x2000u | (((u32)x & 0x1FF) << 16) | (1u << 30), u->qmark_tile);
}

// ---- the screen object -----------------------------------------------------------------------------------------
#define REG_BG1CNT  (*(volatile u16 *)0x0400000A)
#define REG_BG3CNT  (*(volatile u16 *)0x0400000E)
#define REG_BG1HOFS (*(volatile u16 *)0x04000014)
#define REG_BG1VOFS (*(volatile u16 *)0x04000016)
#define REG_BLDCNT  (*(volatile u16 *)0x04000050)
#define REG_BLDALPHA (*(volatile u16 *)0x04000052)
#define REG_BLDY    (*(volatile u16 *)0x04000054)
#define BG_PAL      ((volatile u16 *)0x05000000)
#define OBJ_PAL     ((volatile u16 *)0x05000200)
#define RGB(r, g, b) ((u16)(((r) >> 3) | (((g) >> 3) << 5) | (((b) >> 3) << 10)))

static const u16 ui_colours[16] = {
    RGB(24, 32, 72),    // C_PANEL
    RGB(40, 56, 112),   // C_PANEL2
    RGB(152, 176, 232), // C_BORDER
    RGB(248, 200, 40),  // C_GOLD
    RGB(168, 112, 16),  // C_GOLD_DARK
    RGB(248, 96, 72),   // C_P1
    RGB(96, 160, 248),  // C_P2
    RGB(152, 152, 160), // C_GREY
    RGB(56, 56, 64),    // C_DARK
    RGB(200, 200, 208), // C_LINE
    RGB(96, 208, 96),   // C_GREEN
    RGB(8, 8, 24),      // C_SHADE
    RGB(32, 44, 96),    // C_CELL
    RGB(72, 96, 168),   // C_CELL_HI
    RGB(16, 20, 48),    // C_NAVY
    RGB(120, 24, 24),   // C_RED_DARK
};


static void display_init(Ui *u) {
    REG_BLDCNT = 0x00FF;            // darken everything: the screen fades in from black (u->fade)
    REG_BLDY = 16;
    REG_BLDALPHA = 0;
    // tiles 0..639 clear (blank tile 0, the bitmap 16..615)
    vram_zero(0x06000000u, 0x2800);
    // the backdrop: the switch screen's checker in tiles 1-4, BG3 map at 0xF800
    u8 pat[256];
    UiGfx_Decompress(UI_BACKDROP_IMG, pat);
    volatile u16 *t = (volatile u16 *)0x06000040u;
    for (int ty = 0; ty < 2; ty++)
        for (int tx = 0; tx < 2; tx++)
            for (int r = 0; r < 8; r++)
                for (int i = 0; i < 8; i += 2) {
                    const u8 *s = pat + (ty * 8 + r) * 16 + tx * 8 + i;
                    *t++ = (u16)(s[0] | (s[1] << 8));
                }
    volatile u16 *m3 = (volatile u16 *)0x0600F800u, *m1 = (volatile u16 *)0x0600E800u;
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++) {
            m3[y * 32 + x] = (u16)((y & 1) * 2 + (x & 1) + 1);
            m1[y * 32 + x] = (u16)(y < 20 && x < 30 ? 16 + y * 30 + x : 0);
        }
    for (int i = 0; i < 256; i++) {     // CPU copies: a DMA here left one of our entries overwritten
        BG_PAL[i] = i >= C_PANEL && i < C_PANEL + 16 ? ui_colours[i - C_PANEL] : UI_MENU_PAL[i];
        OBJ_PAL[i] = UI_OBJ_PAL[i];
    }
    REG_BG1CNT = 0x1D82;            // priority 2, 8bpp, char 0, map 0xE800
    REG_BG3CNT = 0x1F83;            // priority 3, 8bpp, char 0, map 0xF800
    REG_BG1HOFS = 0;
    REG_BG1VOFS = 0;
    REG_DISPCNT = 0x1A40;           // mode 0, BG1 + BG3 + OBJ, 1D OBJ mapping
    u->fade = 16;
}

static int slot_nop(Ui *u) { (void)u; return 0; }
static void slot_dtor(Ui *u, int f) { (void)u; (void)f; }

static void slot_open(Ui *u) {
    UiEntityList_Begin(g_EntityList);
    display_init(u);
    UiCanvas_Create(u->canvas, CV_W, CV_H);
    u->canvas_buf = *(u8 **)(u->canvas + 0x24);
    if (u->canvas_buf) {
        u->canvas[1] = 16;
        *(u32 *)(u->canvas + 0x0C) = UI_FONT;
        *(u32 *)(u->canvas + 0x20) = UI_GLYPH_SHADOW;
    } else {
        ui_st.heap_fail++;
    }
    qmark_make(u);
    u->ok = 1;
    if (u->on_open) u->on_open(u);
}

static void slot_close(Ui *u) {
    if (u->on_close) u->on_close(u);
    if (u->canvas_buf) UiCanvas_Free(u->canvas, 0);
    u->canvas_buf = 0;
    UiEntityList_End(g_EntityList);   // the map's entities and OBJ VRAM allocator come back (message 9 re-uploads)
}

void ui_leave(Ui *u) { if (!u->leaving) { u->leaving = 1; u->fade = 0; } }

int ui_dpad(Ui *u) {
    int d = u->keys & KEY_DPAD;
    if (u->held & KEY_DPAD) {
        if (u->hold_frames > 18 && (u->hold_frames % 5) == 0) d |= u->held & KEY_DPAD;
    }
    return d;
}

static void slot_update(Ui *u) {
    u16 keys = 0, held = 0;
    if (u->pads & UI_PAD_P1) { keys |= g_InputSystem->pressed; held |= g_InputSystem->current; }
    if (u->pads & UI_PAD_P2) { keys |= coop.input2.pressed; held |= coop.input2.current; }
    u->hold_frames = (held & KEY_DPAD) ? (u16)(u->hold_frames + 1) : 0;
    u->frame++;
    int busy = 0;
    if (u->leaving) {
        if (u->fade < 16) u->fade += 2;
        if (u->fade >= 16) { u->fade = 16; u->done = 1; }
        busy = 1;
    } else if (u->fade) {
        u->fade -= 2;
        busy = 1;
    }
    REG_BLDY = u->fade;
    u->keys = busy ? 0 : keys;
    u->held = busy ? 0 : held;
    u->n_extra = 0;
    u->idle = 0;
    if (u->on_step) u->on_step(u);
    ui_st.ready = u->idle && !busy;
    EntityList *l = g_EntityList;
    for (int i = l->count - 1; i >= 0; i--) {
        Entity *e = l->items[i];
        ((void (*)(Entity *))vslot(e, 2))(e);
    }
    UiEntityList_Draw(l);
    if (u->n_extra) {
        int k = 0;
        while (k < 128 && UI_OAM_SHADOW[k * 2] != 0x200) k++;
        for (int i = 0; i < u->n_extra && k < 128; i++, k++) {
            UI_OAM_SHADOW[k * 2] = u->extra[i * 2];
            UI_OAM_SHADOW[k * 2 + 1] = u->extra[i * 2 + 1];
        }
    }
}

static int slot_done(Ui *u) {
    UiEntityList_EndDraw(g_EntityList);
    return u->done;
}

static void run_fade(u32 vt, int from, int step) {
    u32 *f = Arena_Alloc(12);
    if (!f) return;
    f[0] = vt;
    f[1] = (u32)from;
    f[2] = (u32)step;
    UiModal_Run(f);           // the fade frees itself when it closes
}

int ui_run(void (*on_open)(Ui *), void (*on_step)(Ui *), void (*on_close)(Ui *), void *ctx, int pads) {
    state_init();
    if (ui_st.busy || !g_EntityList) return 0;
    ui_st.busy = 1;
    ui_st.vram_before = (u16)UiObjVram_Largest(g_EntityList);
    Ui *u = Arena_Alloc(sizeof(Ui));
    if (!u) { ui_st.heap_fail++; ui_st.busy = 0; return 0; }
    memset(u, 0, sizeof *u);
    void *slots[12] = {
        (void *)slot_dtor, (void *)slot_open, (void *)slot_close, (void *)slot_update, (void *)UI_BACKDROP_VBL,
        (void *)slot_done, (void *)slot_nop, (void *)slot_nop, (void *)slot_nop, (void *)slot_nop,
        (void *)slot_nop, (void *)slot_nop,
    };
    for (int i = 0; i < 12; i++) u->vt[i] = (u32)slots[i] - (u32)u->vt;
    u->vtable = u->vt;
    u->on_open = on_open;
    u->on_step = on_step;
    u->on_close = on_close;
    u->ctx = ctx;
    u->pads = (u8)pads;
    u->qmark_tile = 0xFFFF;
    run_fade(UI_FADE_OUT_VT, 0x100, 0x10);
    UiModal_Run(u);
    ui_st.ready = 0;
    Arena_Free(u);
    UiMap_Refresh(UI_WORLD, 1);
    coop.hud2.hp_px = 0xFF;           // player 2's panel tiles were drawn over: compose it again
    run_fade(UI_FADE_IN_VT, 0, 0x10);
    g_InputSystem->pressed = 0;       // the button that closed the screen does nothing on the map
    coop.input2.pressed = 0;
    ui_st.vram_after = (u16)UiObjVram_Largest(g_EntityList);
    ui_st.screens++;
    ui_st.busy = 0;
    return 1;
}

// ---- the list box ----------------------------------------------------------------------------------------------
typedef struct Choose {
    const char *title;
    const char *const *lines;
    int n, cur, result;
    int x, y, w, h;
} Choose;

#define CH_ROW 18
#define CH_TOP 26

static void choose_row(Ui *u, Choose *c, int i) {
    int y = c->y + CH_TOP + i * CH_ROW;
    int on = i == c->cur;
    bm_fill(c->x + 6, y, c->w - 12, CH_ROW - 2, on ? C_GOLD_DARK : C_PANEL);
    if (on) bm_frame(c->x + 6, y, c->w - 12, CH_ROW - 2, C_GOLD);
    ui_text(u, c->x + c->w / 2, y + 2, 1, on ? C_WHITE : C_LINE, c->lines[i]);
}

static void choose_open(Ui *u) {
    Choose *c = u->ctx;
    int w = c->title ? ui_text_width(c->title) : 0;
    for (int i = 0; i < c->n; i++) { int t = ui_text_width(c->lines[i]); if (t > w) w = t; }
    w += 48;
    if (w < 140) w = 140;
    if (w > 224) w = 224;
    c->w = w;
    c->h = CH_TOP + c->n * CH_ROW + 8;
    c->x = (240 - c->w) / 2;
    c->y = (160 - c->h) / 2;
    if (c->y < 0) c->y = 0;
    bm_fill(c->x + 3, c->y + 3, c->w, c->h, C_SHADE);
    bm_fill(c->x, c->y, c->w, c->h, C_PANEL);
    bm_frame(c->x, c->y, c->w, c->h, C_BORDER);
    bm_frame(c->x + 1, c->y + 1, c->w - 2, c->h - 2, C_PANEL2);
    if (c->title) ui_text(u, c->x + c->w / 2, c->y + 5, 1, C_GOLD, c->title);
    bm_hline(c->x + 8, c->y + CH_TOP - 4, c->w - 16, C_PANEL2);
    for (int i = 0; i < c->n; i++) choose_row(u, c, i);
}

static void choose_step(Ui *u) {
    Choose *c = u->ctx;
    u->idle = 1;
    int d = ui_dpad(u);
    int old = c->cur;
    if (d & KEY_UP) c->cur = c->cur > 0 ? c->cur - 1 : c->n - 1;
    if (d & KEY_DOWN) c->cur = c->cur < c->n - 1 ? c->cur + 1 : 0;
    if (c->cur != old) {
        ui_sfx(UI_SFX_MOVE);
        choose_row(u, c, old);
        choose_row(u, c, c->cur);
    }
    if (u->keys & KEY_A) { ui_sfx(UI_SFX_OK); c->result = c->cur; ui_leave(u); }
    else if (u->keys & KEY_B) { ui_sfx(UI_SFX_BACK); c->result = -1; ui_leave(u); }
}

int tny_ui_choose(const char *title, const char *const *lines, int n) {
    if (n <= 0 || !lines) return -1;
    if (n > 7) n = 7;
    Choose c;
    memset(&c, 0, sizeof c);
    c.title = title;
    c.lines = lines;
    c.n = n;
    c.result = -1;
    if (!ui_run(choose_open, choose_step, 0, &c, UI_PAD_BOTH)) return 0;
    ui_st.last_ret = (s8)c.result;
    return c.result;
}

// ---- the pop-up: one narration line in the dialogue box, as the journal's "Journal Updated!" ------------------
void tny_popup_show(const char *line) {
    state_init();
    if (!line || ui_st.busy) return;
    ui_st.busy = 1;
    struct { u8 head[4]; char text[64]; } ent __attribute__((aligned(4)));
    ent.head[0] = 1; ent.head[1] = 0;   // -> entry 1 (the end)
    ent.head[2] = 1;                    // text
    ent.head[3] = 0;                    // narration
    ent.text[0] = '@';
    ent.text[1] = '^';
    int i = 0;
    for (; line[i] && i < (int)sizeof ent.text - 3; i++) ent.text[2 + i] = line[i];
    ent.text[2 + i] = 0;
    const void *seq[2] = { &ent, 0 };
    u32 ctx[8];
    u16 state = 0;
    UiVmDialog_Init(ctx, seq, &state, UI_NO_ACTOR, 0);
    ui_sfx(UI_SFX_WIN);
    u32 *screen = Arena_Alloc(8);
    if (screen) {
        screen[0] = UI_DIALOG_VT;
        screen[1] = (u32)ctx;
        UiModal_Run(screen);
    }
    UiVmDialog_Free(ctx, 0);
    g_InputSystem->pressed = 0;
    coop.input2.pressed = 0;
    ui_st.screens++;
    ui_st.busy = 0;
}

// Every frame (input.c through tourney_hooks.c): the pop-up test (test command 28, 7) runs here, in the same
// context as the engine's pop-up queue.
void tny_ui_frame(void) {
    if (ui_st.magic != UI_MAGIC || !ui_st.popup_test) return;
    int k = ui_st.popup_test - 1;
    ui_st.popup_test = 0;
    tny_popup_show(k == 1 ? "New tournament tier: Champion!" : k == 2 ? "New division: Full bracket!" : "New tournament fighter!");
}

// ---- CoopCall 28-29 (scripts) ----------------------------------------------------------------------------------
// 28 a=1: the unlock board (the courtyard's notice board); a=2: the bracket screen, phase b.
// 29: the character select for player a (0 / 1) at tier b: pushes the fighter id, or -1 when backed out.
int tny_ui_call(int fn, s32 a, s32 b, s32 c, s32 *push) {
    (void)c;
    if (fn == 28) {                       // pushes 1: the grounds' board script (papaya.lscript) ends the talk on it
        if (a == 1) tny_ui_grid(GRID_BOARD, 0, 0, 0);
        else if (a == 2) tny_ui_bracket(b);
        *push = 1;
        return 1;
    }
    if (fn == 29) {
        u8 pick = 0xFF;
        int ok = tny_ui_select(a, b, &pick);
        *push = ok ? pick : -1;
        return 1;
    }
    return 0;
}

// ---- test commands 28-29 (coop2.cmd; docs/testing.md "Tournament screens") -------------------------------------
static const char *const demo_div[3] = { "1 v 1", "2 v 2", "Royal Rumble" };
static const char *const demo_size[2] = { "Quick (8 fighters)", "Full (16 fighters)" };

// A fake bracket from the fighters this build has (the engine's stand-in for the ui's tests): players first.
static void fake_bracket(int size, int coop_players) {
    TBracket *b = &ui_st.br;
    memset(b, 0xFF, sizeof *b);
    b->size = (u8)(size == 16 ? 16 : 8);
    b->rounds = b->size == 16 ? 4 : 3;
    b->round = 0;
    b->match = 0;
    b->tier = TIER_ROOKIE;
    b->division = DIV_1V1;
    int k = 0, id = 0;
    for (int s = 0; s < b->size; s++) {
        if (s == 0 || (coop_players && s == b->size / 2)) continue;   // the players' seats
        while (id < TF_COUNT && (!(tny_fighters[id].role & TR_OPPONENT) || !ui_fighter_ok(id))) id++;
        b->seat[s] = (u8)(id < TF_COUNT ? id : TF_YAMCHA);
        if (id < TF_COUNT) id++;
        k++;
    }
    b->seat[0] = TF_GOHAN;
    b->p1_seat = 0;
    if (coop_players) { b->seat[b->size / 2] = TF_KRILLIN; b->p2_seat = (u8)(b->size / 2); }
    ui_st.fake |= 2;
}

static int match_seat(const TBracket *b, int r, int m, int side) {
    if (r == 0) return m * 2 + side;
    return b->winner[r - 1][m * 2 + side];
}

// Play the bracket's next round: the players' matches go to `players_win`, every other match to its first seat.
static void fake_round(int players_win) {
    TBracket *b = &ui_st.br;
    if (b->round >= b->rounds) return;
    int r = b->round, n = b->size >> (r + 1);
    for (int m = 0; m < n; m++) {
        int s0 = match_seat(b, r, m, 0), s1 = match_seat(b, r, m, 1);
        int pl0 = s0 == b->p1_seat || s0 == b->p2_seat, pl1 = s1 == b->p1_seat || s1 == b->p2_seat;
        int w = s0;
        if (pl1 && !pl0) w = players_win ? s1 : s0;
        else if (pl0 && !pl1) w = players_win ? s0 : s1;
        b->winner[r][m] = (u8)w;
    }
    b->round++;
    b->match = 0;
    if (b->round < b->rounds) {   // the next match shown: the one with player 1 in it, else the first
        int n2 = b->size >> (b->round + 1);
        for (int m = 0; m < n2; m++)
            if (match_seat(b, b->round, m, 0) == b->p1_seat || match_seat(b, b->round, m, 1) == b->p1_seat) b->match = (u8)m;
    }
}

void tny_ui_test(u32 cmd, u32 arg, u32 arg2) {
    state_init();
    if (cmd == 28) {
        switch (arg) {
        case 1: tny_ui_choose("Which division?", demo_div, 3); break;
        case 2: tny_ui_choose("Bracket size", demo_size, 2); break;
        case 3: { u8 p = 0xFF; ui_st.last_ret = (s8)tny_ui_select((int)arg2 & 1, (int)(arg2 >> 8) & 7, &p); ui_st.last_pick[arg2 & 1] = p; break; }
        case 4: tny_ui_bracket((int)arg2); break;
        case 5: {
            static const u8 ids[6] = { TF_GOHAN, TF_YAMCHA, TF_TIEN, TF_FRIEZA, TF_TAO, TF_MECHA_FRIEZA };
            u8 down[6];
            for (int i = 0; i < 6; i++) down[i] = (u8)((arg2 >> i) & 1);
            tny_ui_rumble_board(ids, down, 6);
            break;
        }
        case 6: tny_ui_grid(GRID_BOARD, 0, 0, 0); break;
        case 7: ui_st.popup_test = (u8)(arg2 + 1); break;   // shown from tny_ui_frame (the input poll), as the engine would
        }
    } else if (cmd == 29) {
        switch (arg) {
        case 0: ui_st.fake = 0; ui_st.shown_ok = 0; break;
        case 1: ui_st.unlock[0] = arg2; ui_st.fake |= 1; break;
        case 2: ui_st.unlock[1] = arg2; ui_st.fake |= 1; break;
        case 3: fake_bracket((int)(arg2 & 0xFF), (arg2 >> 8) & 1); break;
        case 4: fake_round((int)arg2); break;
        case 5: ui_st.tiers = (u8)arg2; ui_st.fake |= 4; break;
        }
    }
}
