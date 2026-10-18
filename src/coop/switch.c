// The save pad's Switch Characters screen for a PARTY_SLOTS-entry party (docs/party-8-slots.md step 4).
// The stock screen object (0xD8 bytes, module 0x08010B08..0x080112D4) lists at most six portraits
// (pairs {slot, entity} at +0x0C) and reads the party through its module table (0x086D763C, word 1).
// Rather than porting the whole screen, the build points that table word at a private "view" party
// (tools/patch_rom.py PARTY_SHADOWS): a compacted copy holding the window of up to six members
// currently shown, so the stock open routine 0x08010BCE lays the portraits out exactly as before.
// The functions below replace (entry jumps) the constructor (bigger object: the member list and the
// window live after the stock fields), the prev/next queries, the selection and arrow updates and the
// input handler, which scrolls the window: portraits are rebuilt when the selection leaves it, and
// small arrow tiles at the frame's edges show that more characters exist on that side.
#include "coop.h"

#define SWITCH_OBJ_SIZE   0xD8
#define SWITCH_WINDOW     6          // portraits the stock screen shows at once
#define SWITCH_VTABLE     0x080247A8u
#define BUTTON_VTABLE     0x08023BD8u
#define PORTRAIT_Y        0x53
#define PORTRAIT_FRAME_OK 0x086D0DD0u   // portrait +0x28 value that needs no refresh after greying
#define SWITCH_TABLE      ((void *const *)0x086D763C)   // [0] audio mixer, [1] party (shadowed -> switch_view), [2] entity list
#define SWITCH_BG_MAP     ((volatile u16 *)0x0600E800)  // tilemap of the frame / labels layer
#define FRAME_X1 4
#define FRAME_Y1 6
#define FRAME_X2 26
#define FRAME_TILE0 8                                   // 0x08010BCE fills the frame rect with tiles 8, 9, 10, ...
#define ARROW_TILES       0x06004600u                   // after the title's tiles (0x06004080 + 22 x 0x40): two 8x8 tiles
#define ARROW_TILE_INDEX  (ARROW_TILES - 0x06000000) / 0x40
#define ARROW_Y           9                             // tile rows 9-10 (px 72-88): the portrait row
#define ARROW_COLOUR      2                             // the CANCEL / SWITCH label colour

typedef struct SwitchExt {
    u8 members[PARTY_SLOTS];   // in-party slots in order
    u8 count;
    u8 first;                  // first member shown
    u8 wsize;                  // members shown (<= SWITCH_WINDOW)
    u8 drawn;                  // indicator tiles rendered
} SwitchExt;
#define EXT(o) ((SwitchExt *)((u8 *)(o) + SWITCH_OBJ_SIZE))
#define F32(o, off) (*(s32 *)((u8 *)(o) + (off)))
#define FPTR(o, off) (*(void **)((u8 *)(o) + (off)))
#define PAIR_SLOT(o, i) F32(o, 0x0C + (i) * 8)
#define PAIR_ENTITY(o, i) (*(Entity **)((u8 *)(o) + 0x10 + (i) * 8))

PartyState switch_view __attribute__((section(".party"), aligned(4)));

ROM_FN(Entity *, Portrait_Create, (void *mem, void *sprite, int x, int y), 0x08007C71);
ROM_FN(void, Portrait_Refresh, (Entity *e), 0x080076EF);
ROM_FN(void, EntityList_Begin, (void *list), 0x08006F4B);
ROM_FN(void, EntityList_Delete, (void *list, void *obj), 0x080068CB);   // unlinks and runs the object's destructor
ROM_FN(void *, Canvas_Create, (void *mem, int w, int h), 0x08020E5D);
ROM_FN(void, Canvas_Upload, (void *canvas, u32 vram), 0x080236B5);
ROM_FN(void, Canvas_Free, (void *canvas, int keep), 0x08020EAB);
ROM_FN(int, Button_Pressed, (void *button, u8 *state), 0x08002DDB);
ROM_FN(void, Button_Animate, (void *button, u8 *state), 0x08002E29);
ROM_FN(void, Button_Enable, (void *button), 0x08002DB7);
ROM_FN(void, Button_Disable, (void *button), 0x08002DC9);
ROM_FN(void, NameBox_Move, (void *box, const s32 *xy), 0x080184AF);
ROM_FN(void, PlayerEntity_RefreshSprite, (Entity *e), 0x080086A3);
ROM_FN(void, Party_DetransformOthers2, (PartyState *p), 0x08003ED5);

void p2_refresh_sprite(void);   // player.c

// ---- the view -----------------------------------------------------------------------------------

static void build_view(void *obj) {
    SwitchExt *x = EXT(obj);
    int n = 0, pos = 0;
    for (int i = 0; i < PARTY_SLOTS; i++) {
        if (!(g_Party->chars[i].flags & 1)) continue;
        if (i == g_Party->active_slot) pos = n;
        x->members[n++] = (u8)i;
    }
    x->count = (u8)n;
    int first = 0;
    if (n > SWITCH_WINDOW) {
        first = pos - (SWITCH_WINDOW - 1);
        if (first < 0) first = 0;
        if (first > n - SWITCH_WINDOW) first = n - SWITCH_WINDOW;
    }
    x->first = (u8)first;
    x->wsize = (u8)(n - first > SWITCH_WINDOW ? SWITCH_WINDOW : n - first);
    memset(&switch_view, 0, sizeof switch_view);
    for (int k = 0; k < x->wsize; k++) switch_view.chars[k] = g_Party->chars[x->members[first + k]];
    switch_view.active_slot = (u8)(pos - first);
}

static int member_selectable(SwitchExt *x, int j) {
    return (g_Party->chars[x->members[j]].flags & 2) != 0;
}

// ---- portraits and indicators ---------------------------------------------------------------------

static int row_spacing(int n) {
    int s = 0x90 / n;
    return s > 0x30 ? 0x30 : s;
}

static Entity *make_portrait(int slot, int px) {
    const u8 *row = DISPLAY_DATA + g_Party->chars[slot].display_row * 0x14;
    Entity *e = Portrait_Create(0, Character_GetSpriteRecord(row[1]), px, PORTRAIT_Y);
    if (!(g_Party->chars[slot].flags & 2)) {
        *((u8 *)e + 0x10) = 1;          // greyed: not selectable
        if (*(u32 *)((u8 *)e + 0x28) != PORTRAIT_FRAME_OK) Portrait_Refresh(e);
    }
    return e;
}

// Replace the screen's portraits by the current window (same layout rule as 0x08010BCE).
static void rebuild_portraits(void *obj) {
    SwitchExt *x = EXT(obj);
    void *list = SWITCH_TABLE[2];
    int n = F32(obj, 0x08);
    for (int k = 0; k < n; k++) EntityList_Delete(list, PAIR_ENTITY(obj, k));
    int spacing = row_spacing(x->wsize);
    int px = 0x70 - ((x->wsize - 1) * spacing >> 1);
    for (int k = 0; k < x->wsize; k++) {
        int slot = x->members[x->first + k];
        PAIR_SLOT(obj, k) = slot;
        PAIR_ENTITY(obj, k) = make_portrait(slot, px);
        EntityList_Add(list, PAIR_ENTITY(obj, k));
        px += spacing;
    }
    F32(obj, 0x08) = x->wsize;
    for (int k = 0; k < x->wsize; k++) switch_view.chars[k] = g_Party->chars[x->members[x->first + k]];
}

// Two 8x8 tiles: a left-pointing arrow (top and bottom halves of a 16-row shape); the right one is the
// same tiles with the tilemap's horizontal flip.
static void render_arrow_tiles(void) {
    u8 *c = Canvas_Create(0, 8, 16);
    if (!c) return;
    u8 *buf = *(u8 **)(c + 0x24);
    memset(buf, 0, 8 * 16);
    for (int r = 2; r < 14; r++) {
        int d = r < 8 ? 7 - r : r - 8;    // 0 at the middle rows, 5 at the ends
        for (int px = 1 + d; px < 7; px++) buf[r * 8 + px] = ARROW_COLOUR;
    }
    Canvas_Upload(c, ARROW_TILES);
    Canvas_Free(c, 0);
}

static void put_frame_tile(int tx, int ty, int tile) {
    SWITCH_BG_MAP[ty * 32 + tx] = (u16)tile;
}

static void draw_indicators(void *obj) {
    SwitchExt *x = EXT(obj);
    if (!x->drawn) { render_arrow_tiles(); x->drawn = 1; }
    int more_left = x->first > 0, more_right = x->first + x->wsize < x->count;
    for (int i = 0; i < 2; i++) {
        int ty = ARROW_Y + i;
        int frame_l = FRAME_TILE0 + (ty - FRAME_Y1) * (FRAME_X2 - FRAME_X1) + (FRAME_X1 - FRAME_X1);
        int frame_r = FRAME_TILE0 + (ty - FRAME_Y1) * (FRAME_X2 - FRAME_X1) + (FRAME_X2 - 1 - FRAME_X1);
        put_frame_tile(FRAME_X1, ty, more_left ? (int)(ARROW_TILE_INDEX + i) : frame_l);
        put_frame_tile(FRAME_X2 - 1, ty, more_right ? (int)((ARROW_TILE_INDEX + i) | 0x400) : frame_r);
    }
}

// ---- replaced ROM functions ---------------------------------------------------------------------

// 0x08010B08(mem): constructor (the pad handler passes 0)
void *switch_create_hook(u8 *obj) {
    if (!obj) {
        obj = Arena_Alloc(SWITCH_OBJ_SIZE + sizeof(SwitchExt));
        if (!obj) return 0;
    }
    obj[4] = 0;
    obj[5] = 0;
    *(u32 *)obj = SWITCH_VTABLE;
    F32(obj, 0x08) = 0;
    F32(obj, 0x3C) = 0;
    Canvas_Create(obj + 0x50, 0x20, 0x20);
    *(u32 *)(obj + 0x88) = BUTTON_VTABLE;
    *(u32 *)(obj + 0x9C) = BUTTON_VTABLE;
    *(u32 *)(obj + 0xB0) = BUTTON_VTABLE;
    *(u32 *)(obj + 0xC4) = BUTTON_VTABLE;
    FPTR(obj, 0x4C) = g_PlayerObject;
    EntityList_Begin(SWITCH_TABLE[2]);
    obj[0x49] = 0;
    obj[0x48] = 0;
    memset(EXT(obj), 0, sizeof(SwitchExt));
    build_view(obj);
    return obj;
}

// 0x08010B60 / 0x08010B9A: next / previous selectable member after the selection, over the whole
// party (index into the member list), -1 if none. The stock callers only compare with -1.
int switch_next_hook(void *obj) {
    SwitchExt *x = EXT(obj);
    for (int j = x->first + F32(obj, 0x3C) + 1; j < x->count; j++)
        if (member_selectable(x, j)) return j;
    return -1;
}
int switch_prev_hook(void *obj) {
    SwitchExt *x = EXT(obj);
    for (int j = x->first + F32(obj, 0x3C) - 1; j >= 0; j--)
        if (member_selectable(x, j)) return j;
    return -1;
}

static void *sfx_channel(void) { return (u8 *)SWITCH_TABLE[0] + 0x5C4; }

// 0x0801110C: move the highlight and the name box to the selected portrait
void switch_select_hook(void *obj) {
    Sfx_Play(sfx_channel(), SFX_TABLE + 0x24C, 0x40, 0);
    Entity *old = FPTR(obj, 0x40);
    if (old) ((void (*)(Entity *, int))vslot(old, 10))(old, 0);
    Entity *sel = PAIR_ENTITY(obj, F32(obj, 0x3C));
    FPTR(obj, 0x40) = sel;
    ((void (*)(Entity *, int))vslot(sel, 10))(sel, 1);
    NameBox_Move(FPTR(obj, 0x44), (const s32 *)((u8 *)sel + 0x40));
}

// 0x080110BC: enable the arrow buttons that lead somewhere
void switch_arrows_hook(void *obj) {
    if (switch_prev_hook(obj) == -1) Button_Disable((u8 *)obj + 0xB0); else Button_Enable((u8 *)obj + 0xB0);
    if (switch_next_hook(obj) == -1) Button_Disable((u8 *)obj + 0xC4); else Button_Enable((u8 *)obj + 0xC4);
}

static void move_selection(void *obj, int j) {
    SwitchExt *x = EXT(obj);
    if (j < 0) return;
    if (j < x->first || j >= x->first + x->wsize) {
        int first = j > x->first ? j - (SWITCH_WINDOW - 1) : j;
        if (first < 0) first = 0;
        if (first > x->count - SWITCH_WINDOW) first = x->count - SWITCH_WINDOW;
        x->first = (u8)first;
        x->wsize = (u8)(x->count - first > SWITCH_WINDOW ? SWITCH_WINDOW : x->count - first);
        FPTR(obj, 0x40) = 0;       // the old highlight is gone with its portrait
        rebuild_portraits(obj);
    }
    F32(obj, 0x3C) = j - x->first;
    switch_view.active_slot = (u8)(j - x->first);
    switch_select_hook(obj);
    switch_arrows_hook(obj);
    draw_indicators(obj);
}

// 0x08011172: per-frame input. +0x49 = 0 while the buttons slide in.
int switch_update_hook(u8 *obj) {
    u8 *state = obj + 0x49;
    if (*state == 0) {
        Button_Animate(obj + 0x88, state);
        Button_Animate(obj + 0x9C, state);
        Button_Animate(obj + 0xB0, state);
        Button_Animate(obj + 0xC4, state);
        draw_indicators(obj);
        return 0;
    }
    if (Button_Pressed(obj + 0x88, state)) {           // A: switch
        Sfx_Play(sfx_channel(), SFX_TABLE + 0x6C, 0x40, 0);
        SwitchExt *x = EXT(obj);
        int slot = x->members[x->first + F32(obj, 0x3C)];
        g_Party->active_slot = (u8)slot;
        Entity *player = FPTR(obj, 0x4C);
        const u8 *row = DISPLAY_DATA + g_Party->chars[slot].display_row * 0x14;
        player->sprite_record = Character_GetSpriteRecord(row[1]);
        PlayerEntity_RefreshSprite(player);
        Party_DetransformOthers2(g_Party);
        obj[5] = 2;
        obj[0x48] = 1;
    }
    if (Button_Pressed(obj + 0x9C, state)) obj[5] = 2;   // B: cancel
    if (Button_Pressed(obj + 0xB0, state)) move_selection(obj, switch_prev_hook(obj));
    if (Button_Pressed(obj + 0xC4, state)) move_selection(obj, switch_next_hook(obj));
    return 0;
}
