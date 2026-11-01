// The Rumble announcer's calls during a round (docs/tournament.md 24.1; the lines are docs/tournament-dialogue.md 8,
// Ryan's plan, as written). The dialogue box (the engine's say_go, a dialogue sequence) stops the fight while it shows,
// so these are a caption instead: one line at the bottom of the screen for CAP_FRAMES frames, the fight going on
// under it, then gone; up to CAP_MAX queued, shown in turn, never over a scene or behind the curtain.
//
//   a fighter falls        tny_moves.c fallen()'s first frame -> tny_on_fall: "[Name] is down!" / "[Name] hits the
//                          floor!" / "That's it for [Name]!" / "Down goes [Name]!" (a frame-counter pick: never the
//                          engine's rnd(), whose seeded draws the tests rely on)
//   two falls within 60 f  "What a flurry!  Two fighters down!" in place of the first one's line
//   a player goes down     revive.c revive_hit (a co-op Rumble: the partner may revive him) -> tny_on_player_down:
//                          "[Name] is down!  Can the partner hold on?"
//   Super Buu changes form tny_moves.c's MK_TRANSFORM starting on his kit -> tny_on_transform: "He's changing!  Look at
//                          that power!" (his first form), "Not again!  Another form!" (the next)
//
// Only in a Rumble round (tny_rumble_live: the fight, or the pause after its last fall until the result line takes
// over). The team combos' caption (tny_combo_meter.c tny_caption) sits at the same place: these wait while a combo
// plays and CAP_FRAMES after it (its name shows then), and a line that was up when the combo began shows again after.
// Drawn from the HUD's draw hook (hud.c) like the overhead HP bars: the dialogue font through the stock canvas into
// 4bpp OBJ tiles (32 x 16 sprites side by side, a black box behind white text, palette bank 0), allocated while a line
// shows. A line wider than the screen goes on two rows, split where the announcer pauses ("  "). The queue holds a
// line as {line, fighter} and the text is made when it shows, so the state is 16 bytes (`.tny.cap`).
#include "tourney.h"
#include "tny_engine.h"
#include "tny_combo.h"
#include "tny_ui.h"
#include "wide.h"

#define SCENE_RUNNING   (*(volatile u32 *)0x03001FDC)
#define CAP_MAGIC       0x504E5954u            // "TYNP"
#define CAP_FRAMES      90                      // a line's frames on screen
#define CAP_MAX         3
#define FLURRY          60                      // frames: a second fall this soon after the first is a flurry
#define CAP_W_MAX       232                     // px: a wider line goes on two rows
#define C_BLACK         1                       // the game's one OBJ palette (bank 0)
#define C_WHITE         2
#define OBJ_32x16       0x80004000u             // horizontal, size 2, 4bpp
#define OAM_SHADOW      ((u32 *)0x030022BC)     // the frame's OAM entries, 128 x 2 words (tny_ui.h UI_OAM_SHADOW)
#define CAP_OBJS        16                      // 2 rows of 8 sprites at most

enum { L_DOWN, L_FLOOR, L_THATS_IT, L_DOWN_GOES, L_FLURRY, L_PARTNER, L_CHANGING, L_AGAIN, L_COUNT };
// a line: text before the name, after it (0 = no name)
static const char *const lines[L_COUNT][2] = {
    [L_DOWN]      = { "", " is down!" },
    [L_FLOOR]     = { "", " hits the floor!" },
    [L_THATS_IT]  = { "That's it for ", "!" },
    [L_DOWN_GOES] = { "Down goes ", "!" },
    [L_FLURRY]    = { "What a flurry!  Two fighters down!", 0 },
    [L_PARTNER]   = { "", " is down!  Can the partner hold on?" },
    [L_CHANGING]  = { "He's changing!  Look at that power!", 0 },
    [L_AGAIN]     = { "Not again!  Another form!", 0 },
};

typedef struct Cap {        // .tny.cap (tests find it by the magic)
    u32 magic;              // +0x0
    u8 line[CAP_MAX];       // +0x4 the queue (the head shows): L_*
    u8 who[CAP_MAX];        // +0x7 their fighters (TF_*)
    u8 n;                   // +0xA queued
    u8 shown;               // +0xB lines shown (tests: test command 37)
    u8 timer;               // +0xC the head's frames left; with no tiles: frames to wait (after a combo)
    u8 wr;                  // +0xD the head's width / 2 | (rows - 1) << 7
    u16 tile;               // +0xE its OBJ tiles (0 = not drawn)
} Cap;
_Static_assert(sizeof(Cap) == 0x10, ".tny.cap is 0x10 bytes (docs/ids.md)");
static Cap cap TNY_SECTION("cap") __attribute__((used));

int ui_text_width(const char *s);   // tny_ui.c: the dialogue font's width of a line

static void init(void) {
    if (cap.magic == CAP_MAGIC) return;
    memset(&cap, 0, sizeof cap);
    cap.magic = CAP_MAGIC;
}
static void tiles_free(void) {
    if (cap.tile) ObjVram_Free(g_ObjVramList, cap.tile);
    cap.tile = 0;
    cap.wr = 0;
}
static void pop(void) {
    tiles_free();
    cap.timer = 0;
    for (int i = 1; i < CAP_MAX; i++) { cap.line[i - 1] = cap.line[i]; cap.who[i - 1] = cap.who[i]; }
    if (cap.n) cap.n--;
}
static void drop_all(void) {
    tiles_free();
    cap.n = 0;
    cap.timer = 0;
}
static void push(int line, int who) {
    if (cap.n >= CAP_MAX) return;                // a full queue: the newest call is not made
    cap.line[cap.n] = (u8)line;
    cap.who[cap.n++] = (u8)who;
}

// ---- the text -------------------------------------------------------------------------------------------------------
// The line with the name in capitals (the announcer calls names so); returns the split for two rows (the "  " after the
// first sentence, 0 = none)
static char *compose(char *out, int size, int line, int who) {
    char *o = out, *end = out + size - 1, *split = 0;
    const char *parts[3] = { lines[line][0], lines[line][1] ? (who < TF_COUNT ? tny_fighters[who].name : "?") : 0,
                             lines[line][1] };
    for (int p = 0; p < 3; p++)
        for (const char *s = parts[p]; s && *s && o < end; s++) {
            char ch = *s;
            if (p == 1 && ch >= 'a' && ch <= 'z') ch = (char)(ch - 32);
            if (ch == ' ' && s[1] == ' ' && !split) split = o;
            *o++ = ch;
        }
    *o = 0;
    return split;
}

// One row of text into the canvas, then into the 4bpp tiles of sprites [0, n) of row `row` (1D mapping: each sprite's
// top four tiles, then its bottom four)
static void row_out(u8 *cv, u8 *px, const char *s, int w, int n, int row) {
    memset(px, 0, 240 * 16);
    cv[0] = 0;                                  // left aligned at x 4, the glyphs' top row 2
    *(s32 *)(cv + 4) = 4;
    *(s32 *)(cv + 8) = 14;
    cv[0x28] = C_BLACK;
    cv[0x29] = C_WHITE;
    UiCanvas_Text(cv, s);
    volatile u16 *dst = (volatile u16 *)(OBJ_VRAM + (cap.tile + row * n * 8) * 0x20);
    for (int sp = 0; sp < n; sp++)
        for (int ty = 0; ty < 2; ty++)
            for (int tx = 0; tx < 4; tx++)
                for (int r = 0; r < 8; r++) {
                    const u8 *src = px + (ty * 8 + r) * 240 + sp * 32 + tx * 8;
                    for (int i = 0; i < 8; i += 4) {
                        int x0 = sp * 32 + tx * 8 + i;
                        u16 v = 0;
                        for (int q = 0; q < 4; q++) {   // a black box behind the text (the ring's floor is bright)
                            u8 c = x0 + q < w ? (src[i + q] ? src[i + q] & 0xF : C_BLACK) : 0;
                            v |= (u16)(c << (q * 4));
                        }
                        *dst++ = v;
                    }
                }
}

static int render(int line, int who) {
    char text[64], *rows[2];
    char *split = compose(text, sizeof text, line, who);
    int nrows = 1, w = ui_text_width(text) + 8;   // the text and 4 px of the box on each side
    rows[0] = text;
    if (w > CAP_W_MAX && split) {               // two rows: the first sentence, then the rest
        *split = 0;
        rows[1] = split + 2;
        nrows = 2;
        int w0 = ui_text_width(rows[0]) + 8, w1 = ui_text_width(rows[1]) + 8;
        w = w0 > w1 ? w0 : w1;
    }
    if (w > 240) w = 240;
    w &= ~1;
    int n = (w + 31) / 32;
    u16 tile = ObjVram_Alloc(g_ObjVramList, nrows * n * 256);
    if (!tile || tile == 0xFFFF) return 0;
    u8 cv[0x38] __attribute__((aligned(4)));
    memset(cv, 0, sizeof cv);
    UiCanvas_Create(cv, 240, 16);
    u8 *px = *(u8 **)(cv + 0x24);
    if (!px) { ObjVram_Free(g_ObjVramList, tile); return 0; }
    cv[1] = 16;
    *(u32 *)(cv + 0x0C) = UI_FONT;
    *(u32 *)(cv + 0x20) = UI_GLYPH_SHADOW;
    cap.tile = tile;
    for (int r = 0; r < nrows; r++) row_out(cv, px, rows[r], w, n, r);
    UiCanvas_Free(cv, 0);
    cap.wr = (u8)((w >> 1) | (nrows - 1) << 7);
    return 1;
}

// ---- the calls ------------------------------------------------------------------------------------------------------
static int is_fall_line(int line) { return line <= L_DOWN_GOES; }

void tny_on_fall(Entity *e, int id) {
    (void)e;
    init();
    if (!tny_rumble_live() || id < 0 || id >= TF_COUNT) return;
    // a fall line still queued, or up for less than FLURRY frames: the two falls are one call
    for (int i = 0; i < cap.n; i++) {
        if (!is_fall_line(cap.line[i])) continue;
        if (i == 0 && cap.tile && cap.timer <= CAP_FRAMES - FLURRY) continue;
        if (i == 0 && cap.tile) { tiles_free(); cap.timer = 0; }   // the head shows the flurry from now
        cap.line[i] = L_FLURRY;
        return;
    }
    push(L_DOWN + (int)(g_FrameCounter & 3), id);
}

void tny_on_player_down(Entity *e) {
    init();
    if (tny_rumble_live() != 1) return;
    u8 ids[2];
    Entity *ents[2];
    int n = tny_floor_picks(ids, ents);
    for (int i = 0; i < n; i++) if (ents[i] == e) { push(L_PARTNER, ids[i]); return; }
}

void tny_on_transform(Entity *e, int id) {
    init();
    if (id != TF_SUPER_BUU || tny_rumble_live() != 1 || !e) return;
    int first = e->sprite_record == Character_GetSpriteRecord(TS_SUPER_BUU);   // his base body: the first change
    push(first ? L_CHANGING : L_AGAIN, id);
}

// ---- drawing (the HUD's draw hook) ----------------------------------------------------------------------------------
static void obj(u32 **oam, int x, int y, u16 tile) {
    if (x <= -32 || x >= wide_w() || y <= -16 || y >= 160) return;
    u32 *o = *oam;
    u32 xo = (((u32)x & 0x1FF) << 16) | ((((u32)x >> 9) & 1) << 25);
    o[0] = OBJ_32x16 | xo | ((u32)y & 0xFF);   // priority 0, palette bank 0
    o[1] = (o[1] & 0xFFFF0000u) | tile;
    *oam = o + 2;
}

// The caption's entries [first, *oam) moved in front of every other entry: an entity's sprite (a fighter at the bottom
// of the screen) drew over the line. Each entry's second word keeps its high half in place (attr3: the affine matrices
// belong to the OAM index, tny_ui.c writes them there).
static void to_front(u32 *first, u32 *end) {
    u32 *base = OAM_SHADOW;
    int n = (int)(end - first) / 2, before = (int)(first - base) / 2;
    if (n <= 0 || n > CAP_OBJS || before <= 0 || first < base || end > base + 256) return;
    u32 w0[CAP_OBJS];
    u16 w1[CAP_OBJS];
    for (int i = 0; i < n; i++) { w0[i] = first[2 * i]; w1[i] = (u16)first[2 * i + 1]; }
    for (int i = before - 1; i >= 0; i--) {
        u32 *src = base + 2 * i, *dst = base + 2 * (i + n);
        dst[0] = src[0];
        dst[1] = (dst[1] & 0xFFFF0000u) | (src[1] & 0xFFFF);
    }
    for (int i = 0; i < n; i++) { base[2 * i] = w0[i]; base[2 * i + 1] = (base[2 * i + 1] & 0xFFFF0000u) | w1[i]; }
}

void tny_caption_draw(u32 **oam) {
    init();
    int live = tny_rumble_live();
    if (!live || tny_curtain()) { drop_all(); return; }        // the round is over: its calls are stale
    if (tny_combo_busy()) {                                     // a combo plays: its name's caption comes after it
        tiles_free();
        cap.timer = CAP_FRAMES;
        return;
    }
    if (SCENE_RUNNING) {                                        // a line: the fight's waits, the end's takes over
        if (live != 1) drop_all();
        return;
    }
    if (!cap.tile) {
        if (cap.timer) { cap.timer--; return; }                 // after a combo
        if (!cap.n) return;
        if (!render(cap.line[0], cap.who[0])) { pop(); return; }   // no OBJ VRAM for it: the call is not made
        cap.timer = CAP_FRAMES;
        if (cap.shown < 255) cap.shown++;
    }
    int w = (cap.wr & 0x7F) << 1, rows = (cap.wr >> 7) + 1, n = (w + 31) / 32;
    int x = (wide_w() - w) / 2, y = wide_h() - 20 - (rows - 1) * 16;
    u32 *first = *oam;
    for (int r = 0; r < rows; r++)
        for (int i = 0; i < n; i++) obj(oam, x + i * 32, y + r * 16, (u16)(cap.tile + (r * n + i) * 8));
    to_front(first, *oam);
    if (cap.timer) cap.timer--;
    if (!cap.timer) pop();
}

void tny_caption_gone(void) {
    init();
    drop_all();
}

// ---- test command 37 (docs/ids.md) ----------------------------------------------------------------------------------
//   arg 0: the answer into coop2.cmd_arg (0x0203FD0C): the lines shown so far when at least arg2 (else 0: a test's
//          waitnz on the answer times out)
//   arg 1: queue line arg2 & 0xFF (L_*) for fighter arg2 >> 8, in a Rumble round as the calls are
//   arg 2: clear the queue and the count
void tny_caption_test(u32 cmd, u32 arg, u32 arg2) {
    (void)cmd;
    init();
    switch (arg) {
    case 0: coop2.cmd_arg = cap.shown && cap.shown >= arg2 ? cap.shown : 0; break;
    case 1: if (tny_rumble_live() && (arg2 & 0xFF) < L_COUNT) push((int)(arg2 & 0xFF), (int)((arg2 >> 8) & 0xFF)); break;
    case 2: drop_all(); cap.shown = 0; break;
    }
}
