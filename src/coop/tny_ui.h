// The tournament's screens (ui piece, docs/tournament.md section 7): the shared screen framework of tny_ui.c and
// the screens built on it (tny_ui_grid.c: the character select and the unlock board; tny_ui_bracket.c: the
// bracket and the Rumble board). Internal to the ui piece; the cross-piece API is tourney.h.
//
// A screen is a full-screen modal (the save pad's switch screen recipe, docs/tournament.md 9a "ui"):
//   stock fade-out (screen 0x0802462C) -> our screen object through Modal_Run (0x0801F2F4) -> the map's graphics
//   back (0x08005F1A on the world object) -> stock fade-in (0x08024334).
// Our screen object: a relative vtable built in its own block (slot 1 open, 2 close, 3 update, 4 vblank = the
// switch screen's scrolling backdrop 0x08000BE4, 5 done). EntityList_Begin / End (0x08006F4A / 0x08006FA4) set the
// map's entities and the OBJ VRAM allocator aside while the screen runs and restore them after, so whatever the
// screen allocates goes with it (no leaked OBJ VRAM).
// Display: mode 0, BG3 the stock checker backdrop (tiles 1-4, map 0xF800), BG1 a 240 x 160 8bpp "bitmap" of tiles
// 16..615 (map 0xE800) that the screens draw panels, lines and text into, OBJ faces at priority 1 (stock Portrait
// entities, like the switch screen) and our own OAM entries (the big portrait, "?" marks) after them.
#pragma once
#include "tourney.h"

// ---- ROM ------------------------------------------------------------------------------------------------------
ROM_FN(Entity *, UiPortrait_Create, (void *mem, void *sprite, int x, int y), 0x08007C71);
ROM_FN(void, UiEntity_SetRemap, (Entity *e, const u8 *remap), 0x080076EF);     // +0x28 = remap (0 none); re-decodes
ROM_FN(void, UiEntity_SetCtrlState, (Entity *e, void *state), 0x0800763D);
ROM_FN(void, UiEntityList_Begin, (void *list), 0x08006F4B);                    // set the map's entities aside
ROM_FN(void, UiEntityList_End, (void *list), 0x08006FA5);                      // ... and bring them back (message 9)
ROM_FN(void, UiEntityList_Delete, (void *list, void *obj), 0x080068CB);
ROM_FN(void, UiEntityList_Draw, (void *list), 0x08006AD9);                     // OAM shadow: hidden fill + entities
ROM_FN(void, UiEntityList_EndDraw, (void *list), 0x08006AA7);                  // OAM shadow -> OAM, frame uploads
ROM_FN(int, UiModal_Run, (void *screen), 0x0801F2F5);
ROM_FN(void, UiMap_Refresh, (void *world, int clear), 0x08005F1B);            // the map's tiles, maps, palettes, registers
ROM_FN(void *, UiCanvas_Create, (void *mem, int w, int h), 0x08020E5D);
ROM_FN(void, UiCanvas_Free, (void *canvas, int free_it), 0x08020EAB);
ROM_FN(void, UiCanvas_Text, (void *canvas, const char *s), 0x0801F637);
ROM_FN(void, UiGfx_Decompress, (const void *src, void *dst), 0x0801F395);
ROM_FN(const void *, UiPortrait_Resource, (int id), 0x0800BDCD);              // dialogue portrait image (64 x 64 8bpp)
ROM_FN(int, UiObjVram_Largest, (void *list), 0x08006D71);
ROM_FN(void, UiVmDialog_Init, (void *ctx, const void *seq, void *state, const void *actor, int flags), 0x0800B4AF);
ROM_FN(void, UiVmDialog_Free, (void *ctx, int free_it), 0x0800B4FD);

#define UI_WORLD          ((void *)0x03001090)          // the map / world object (module 0x086D7624 word 2)
#define UI_FADE_OUT_VT    0x0802462Cu                   // {vt, 0x100, 0x10}: fade the map to black
#define UI_FADE_IN_VT     0x08024334u                   // {vt, 0, 0x10}: fade back in
#define UI_DIALOG_VT      0x08023E88u                   // {vt, ctx}: a dialogue context as a modal screen
#define UI_NO_ACTOR       ((const void *)0x086D7634)    // the save pad's "no actor" pair (module + 0x10)
#define UI_BACKDROP_VBL   0x08000BE5u                   // switch screen slot 4: scrolls BG3 diagonally
#define UI_BACKDROP_IMG   ((const void *)0x086D7654)    // 16 x 16 8bpp checker (menu palette)
#define UI_MENU_PAL       ((const u16 *)0x081D4F50)     // menu BG palette
#define UI_OBJ_PAL        ((const u16 *)0x081DA6C8)     // OBJ palette (character sprites, dialogue portraits)
#define UI_FONT           0x08028724u
#define UI_GLYPH_SHADOW   0x08020EC3u                   // glyph drawer: colour +0x29, shadow +0x28
#define UI_STATE_IDLE     ((void *)0x03001F34)          // portrait control states (0x081DA978): standing
#define UI_STATE_WALK     ((void *)0x03001F30)          // ... walking (the switch screen's selected portrait)
#define UI_REMAP_GREY     ((const u8 *)0x086D0DD0)      // the switch screen's greyed portrait
#define UI_REMAP_DIM      ((const u8 *)0x086D0ED0)      // ... and its unselected one
#define UI_OAM_SHADOW     ((u32 *)0x030022BC)           // 128 x 2 words, filled with 0x200 (hidden) each frame

// ---- colours (BG palette: the menu palette plus ours in 224-239) -----------------------------------------------
enum {
    C_CLEAR = 0, C_BLACK = 1, C_WHITE = 2, C_YELLOW = 8,
    C_PANEL = 224, C_PANEL2, C_BORDER, C_GOLD, C_GOLD_DARK, C_P1, C_P2, C_GREY, C_DARK, C_LINE, C_GREEN, C_SHADE,
    C_CELL, C_CELL_HI, C_NAVY, C_RED_DARK,
};
#define OBJ_C_BLACK 1                                   // OBJ palette: black (silhouettes, "?" outline)
#define OBJ_C_WHITE 2

// ---- keys ------------------------------------------------------------------------------------------------------
#define UI_PAD_P1   1
#define UI_PAD_P2   2
#define UI_PAD_BOTH 3

// ---- the screen ------------------------------------------------------------------------------------------------
#define UI_EXTRA_MAX 24
typedef struct Ui Ui;
struct Ui {
    const u32 *vtable;          // our relative vtable (vt below): the object Modal_Run runs
    u32 vt[12];
    void (*on_open)(Ui *u);     // the screen's own setup, after the display is ready
    void (*on_step)(Ui *u);     // every frame: input, animation; set done to leave
    void (*on_close)(Ui *u);    // before the entities are set back
    void *ctx;                  // the screen's own state
    u8 done;                    // leave after the fade-out
    u8 pads;                    // UI_PAD_*: whose buttons drive the screen
    u8 fade;                    // 16 .. 0 fading in, then 0; 16 + leaving
    u8 leaving;
    u16 keys, held;             // this frame's presses / held keys (the pads above)
    u16 hold_frames;            // frames the d-pad has been held (auto-repeat)
    u16 frame;                  // frames since open
    u8 canvas[0x38];            // text canvas (240 x 16)
    u8 *canvas_buf;
    u32 extra[UI_EXTRA_MAX * 2];   // our own OAM entries this frame
    u8 n_extra;
    u8 ok;                      // the display and canvas are ready
    u16 qmark_tile;             // OBJ tiles of the "?" mark (0xFFFF none)
};

int  ui_run(void (*on_open)(Ui *), void (*on_step)(Ui *), void (*on_close)(Ui *), void *ctx, int pads);
void ui_leave(Ui *u);                                   // start the fade-out; the screen closes after it
int  ui_dpad(Ui *u);                                    // pressed or auto-repeated d-pad bits this frame
void ui_sfx(int which);                                 // UI_SFX_*
enum { UI_SFX_MOVE, UI_SFX_OK, UI_SFX_BACK, UI_SFX_BUZZ, UI_SFX_SLIDE, UI_SFX_WIN };

// The bitmap layer (BG1)
void bm_fill(int x, int y, int w, int h, int c);
void bm_frame(int x, int y, int w, int h, int c);       // 1 px outline
void bm_clear(void);
void bm_hline(int x, int y, int w, int c);
void bm_vline(int x, int y, int h, int c);
// Text (the dialogue font, about 12 px tall; y is the top of the line). align 0 left, 1 centre (x = centre), 2 right.
void ui_text(Ui *u, int x, int y, int align, int colour, const char *s);
int  ui_text_width(const char *s);
int  ui_text_wrap(Ui *u, int x, int y, int w, int colour, const char *s, int max_lines);   // lines drawn
const char *ui_short_name(const char *name, int max_w, char *buf, int bufsize);              // abbreviated to fit

// Faces: stock Portrait entities (full-body idle frames, like the switch screen), drawn at OBJ priority 1.
void *ui_sprite_record(int sprite_id);                  // the record, 0 when it is not in this build
int ui_fighter_ok(int id);                              // tny_fighter_ok() and a real record (tny_ui.c)
Entity *ui_face(int sprite_id, int x, int y);           // centred on (x, y); 0 when the record is missing or OBJ VRAM is short
void ui_face_kill(Entity *e);
void ui_face_move(Entity *e, int x, int y);             // x, y: the frame box's centre
void ui_face_look(Entity *e, int walking, const u8 *remap);
int  ui_face_x(Entity *e);
int  ui_face_y(Entity *e);
extern const u8 ui_remap_black[256];
// Our own OAM entries (after the faces): the big portrait, "?" marks.
void ui_obj(Ui *u, u32 attr01, u32 attr2);
u16  ui_portrait_load(int portrait_id);                 // OBJ tile of a 64 x 64 8bpp dialogue portrait, 0xFFFF none
void ui_portrait_draw(Ui *u, u16 tile, int x, int y);
void ui_qmark_draw(Ui *u, int x, int y);                // a 16 x 16 "?" centred on (x, y)

// Data the screens read: the engine's, or the test overrides of test command 29 (tny_ui.c).
int ui_unlocked(int id);
int ui_tier_open(int tier);
const TBracket *ui_bracket(void);
extern const char *const ui_tier_names[TIER_COUNT];
extern const char *const ui_div_names[DIV_COUNT];

// The ui's RAM (.tny.ui, <= 0x100 bytes, NOLOAD: guarded by the magic word)
#define UI_MAGIC 0x7A1C0002u
typedef struct UiState {
    u32 magic;
    u8 fake;                    // test overrides: 1 the unlock mask, 2 the bracket, 4 the tier mask
    u8 busy;                    // a screen is open (re-entrancy guard)
    u8 shown_ok;                // shown_* below hold the bracket as last drawn
    s8 last_ret;                // last screen's answer (tests): choose index / select result
    u8 last_pick[2];            // last picks of player 1 / 2 (tests)
    u8 tiers;                   // fake tier-open mask
    u8 screens;                 // screens opened (tests)
    u32 unlock[2];              // fake unlock bits, fighters 0..63
    TBracket br;                // fake bracket (test command 29)
    u8 shown_seat[16];          // the bracket as last drawn
    u8 shown_win[4][8];
    u16 vram_before, vram_after;   // largest free OBJ VRAM block before / after the last screen (tests)
    u16 heap_fail;              // allocation failures seen (tests)
    u16 anim_frames;            // frames the last bracket animation ran (tests)
} UiState;
extern UiState tny_ui_state;
#define ui_st tny_ui_state

// The screens
int  tny_ui_grid(int mode, int player, int tier, u8 *pick);   // tny_ui_grid.c: mode 0 select, 1 unlock board
#define GRID_SELECT 0
#define GRID_BOARD  1
