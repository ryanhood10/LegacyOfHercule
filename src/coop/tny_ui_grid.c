// The character select and the unlock board (ui piece; docs/tournament.md section 7). One grid screen in two
// modes:
//   select  four tabs (Z Fighters / Tournament / Villains / Buu Saga), a face grid of the fighters a player can
//           ever pick under each (story playables, beat-to-unlock, challengers), locked ones as black silhouettes
//           with "?", the big dialogue portrait and the name at the side; A picks an unlocked fighter, B backs out.
//   board   the courtyard's notice board (CoopCall 28 a=1): every beat-to-unlock and challenger fighter, black
//           with "?" until unlocked; the cursor shows only the condition (unlock_text) until then, afterwards the
//           face in colour, the portrait and the name.
// Fighters whose sprite record is missing from the build (tny_fighter_ok) are left out. The faces are stock
// Portrait entities like the save pad's switch screen; eight to a page (4 x 2), more pages scroll with the d-pad.
#include "tny_ui.h"

ROM_FN(void, UiObjVram_Free, (void *list, u16 tile), 0x08006665);

#define COLS 4
#define ROWS 2
#define PAGE (COLS * ROWS)
#define CELL_X0 4
#define CELL_Y0 20
#define CELL_W 36
#define CELL_H 58
#define FACE_DY 28           // the face's centre below the cell's top
#define PANEL_X 152
#define PANEL_Y 18
#define PANEL_W 86
#define PANEL_H 120
#define PORT_X 163
#define PORT_Y 22
#define BAR_Y 140

typedef struct Grid {
    int mode, player, tier, result;
    u8 pick;
    u8 n, tab, page, cur;    // fighters listed, tab, page, cursor within the page
    u8 confirm;              // frames left of the pick's flash
    u8 ids[TF_COUNT];
    Entity *face[PAGE];
    u16 crop[PAGE];          // a giant's face: a 32 x 32 crop of his head (ui_face_crop), 0xFFFF none
    Entity *panel_face;
    u16 port_tile;
    u8 locked[PAGE];         // drawn as a silhouette
} Grid;

static int listed(int mode, int id) {
    const TFighter *f = &tny_fighters[id];
    if (!ui_fighter_ok(id)) return 0;
    if (mode == GRID_BOARD) return (f->role & (TR_BEAT | TR_CHALLENGER)) != 0;
    return (f->role & (TR_STORY | TR_BEAT | TR_CHALLENGER)) != 0;
}

static int pickable(Grid *g, int id) {
    if (!ui_unlocked(id)) return 0;
    return g->mode == GRID_BOARD || ui_tier_open(tny_fighters[id].tier);
}

static void build_list(Grid *g) {
    g->n = 0;
    for (int id = 0; id < TF_COUNT; id++) {
        if (!listed(g->mode, id)) continue;
        if (g->mode == GRID_SELECT && tny_fighters[id].tab != g->tab) continue;
        g->ids[g->n++] = (u8)id;
    }
}

static int pages(Grid *g) { return g->n ? (g->n + PAGE - 1) / PAGE : 1; }
static int on_page(Grid *g) { int left = g->n - g->page * PAGE; return left > PAGE ? PAGE : (left < 0 ? 0 : left); }
static int cur_id(Grid *g) { return on_page(g) ? g->ids[g->page * PAGE + g->cur] : -1; }
static int cell_x(int k) { return CELL_X0 + (k % COLS) * CELL_W; }
static int cell_y(int k) { return CELL_Y0 + (k / COLS) * CELL_H; }
static int cursor_colour(Grid *g) { return g->mode == GRID_SELECT && g->player == 1 ? C_P2 : C_GOLD; }

// ---- drawing ---------------------------------------------------------------------------------------------------
static void draw_cell(Grid *g, int k) {
    int x = cell_x(k), y = cell_y(k);
    int on = k == g->cur && k < on_page(g);
    bm_fill(x, y, CELL_W - 2, CELL_H - 2, k < on_page(g) ? (on ? C_CELL_HI : C_CELL) : C_NAVY);
    if (on) {
        bm_frame(x, y, CELL_W - 2, CELL_H - 2, cursor_colour(g));
        bm_frame(x + 1, y + 1, CELL_W - 4, CELL_H - 4, cursor_colour(g));
    }
}

static void draw_tabs(Ui *u, Grid *g) {
    static const char *const names[TAB_COUNT] = { "Z Fighters", "Tourney", "Villains", "Buu Saga" };
    if (g->mode == GRID_BOARD) {
        bm_fill(0, 0, 240, 16, C_PANEL);
        bm_hline(0, 16, 240, C_BORDER);
        int have = 0, all = 0;
        for (int id = 0; id < TF_COUNT; id++) if (listed(GRID_BOARD, id)) { all++; have += ui_unlocked(id); }
        ui_text(u, 6, 2, 0, C_GOLD, "Tournament Fighters");
        char buf[16];
        int n = 0;
        if (have >= 10) buf[n++] = (char)('0' + have / 10);
        buf[n++] = (char)('0' + have % 10);
        buf[n++] = ' '; buf[n++] = '/'; buf[n++] = ' ';
        if (all >= 10) buf[n++] = (char)('0' + all / 10);
        buf[n++] = (char)('0' + all % 10);
        buf[n] = 0;
        ui_text(u, 234, 2, 2, C_WHITE, buf);
        return;
    }
    for (int t = 0; t < TAB_COUNT; t++) {
        int x = t * 60, on = t == g->tab;
        bm_fill(x, 0, 59, 16, on ? C_GOLD_DARK : C_PANEL);
        if (on) bm_hline(x, 0, 59, C_GOLD);
        ui_text(u, x + 30, 2, 1, on ? C_WHITE : C_GREY, names[t]);
    }
    bm_hline(0, 16, 240, C_BORDER);
}

static void draw_bar(Ui *u, Grid *g) {
    bm_fill(0, BAR_Y, 240, 20, C_PANEL);
    bm_hline(0, BAR_Y, 240, C_BORDER);
    int x = 4;
    if (g->mode == GRID_SELECT && (g->player == 1 || coop.p2 || p2_present())) {
        ui_text(u, x, BAR_Y + 4, 0, g->player ? C_P2 : C_P1, g->player ? "Player 2" : "Player 1");
        x += 52;
    }
    if (pages(g) > 1) {
        char buf[8] = { (char)('1' + g->page), '/', (char)('0' + pages(g)), 0 };
        ui_text(u, x, BAR_Y + 4, 0, C_GREY, buf);
    }
    ui_text(u, 236, BAR_Y + 4, 2, C_LINE, g->mode == GRID_SELECT ? "L R tab  A pick  B back" : "A B close");
}

static void panel_clear(Grid *g) {
    if (g->panel_face) { ui_face_kill(g->panel_face); g->panel_face = 0; }
    if (g->port_tile != 0xFFFF) { UiObjVram_Free(g_EntityList, g->port_tile); g->port_tile = 0xFFFF; }
}

static void draw_panel(Ui *u, Grid *g) {
    panel_clear(g);
    bm_fill(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, C_PANEL);
    bm_frame(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, C_BORDER);
    bm_fill(PORT_X - 1, PORT_Y - 1, 66, 66, C_NAVY);
    int id = cur_id(g);
    if (id < 0) {
        ui_text(u, PANEL_X + PANEL_W / 2, 92, 1, C_GREY, "Nobody yet");
        return;
    }
    const TFighter *f = &tny_fighters[id];
    int cx = PANEL_X + PANEL_W / 2;
    char sb[24];
    if (pickable(g, id)) {
        g->port_tile = ui_portrait_load(f->portrait);
        if (g->port_tile == 0xFFFF) {            // no dialogue portrait: the body, standing
            g->panel_face = ui_face(f->sprite, cx, PORT_Y + 32);
            ui_face_look(g->panel_face, 0, 0);
        }
        ui_text(u, cx, 87, 1, C_WHITE, ui_short_name(f->name, PANEL_W - 4, sb, sizeof sb));
        if (g->mode == GRID_BOARD) ui_text(u, cx, 100, 1, C_GREEN, "Unlocked!");
        else ui_text(u, cx, 100, 1, C_GREY, ui_tier_names[f->tier]);
        return;
    }
    ui_text(u, cx, PORT_Y + 26, 1, C_GREY, "?");
    ui_text(u, cx, 87, 1, C_GREY, "? ? ?");
    const char *why;
    if (g->mode == GRID_SELECT && ui_unlocked(id)) why = 0;
    else why = f->unlock_text;
    if (!why) {
        if (g->mode == GRID_SELECT && ui_unlocked(id)) {
            ui_text_wrap(u, PANEL_X + 4, 100, PANEL_W - 8, C_LINE, "Opens with a higher tier", 3);
            return;
        }
        why = (f->role & TR_STORY) ? "Joins with the story" : "Not playable";
    }
    int lines = ui_text_wrap(u, PANEL_X + 4, 100, PANEL_W - 8, C_LINE, why, 3);
    if ((f->role & TR_CHALLENGER) && tny_challenge_tried(id) && lines < 3)
        ui_text_wrap(u, PANEL_X + 4, 100 + lines * 12, PANEL_W - 8, C_P1, "Lost once: win again", 3 - lines);
}

static void crops_free(Grid *g) {
    for (int k = 0; k < PAGE; k++) {
        if (g->crop[k] != 0xFFFF) UiObjVram_Free(g_EntityList, g->crop[k]);
        g->crop[k] = 0xFFFF;
    }
}
static int face_cx(int k) { return cell_x(k) + (CELL_W - 2) / 2; }
static int face_cy(int k) { return cell_y(k) + FACE_DY; }

static void face_look(Grid *g, int k) {
    Entity *e = g->face[k];
    if (!e) return;
    if (g->locked[k]) ui_face_look(e, 0, ui_remap_black);
    else ui_face_look(e, k == g->cur, 0);
}

static void build_page(Ui *u, Grid *g) {
    for (int k = 0; k < PAGE; k++) { ui_face_kill(g->face[k]); g->face[k] = 0; }
    crops_free(g);
    panel_clear(g);
    if (g->cur >= on_page(g)) g->cur = on_page(g) ? (u8)(on_page(g) - 1) : 0;
    draw_tabs(u, g);
    bm_fill(0, 17, PANEL_X, BAR_Y - 17, C_CLEAR);
    for (int k = 0; k < PAGE; k++) draw_cell(g, k);
    draw_panel(u, g);                         // the big portrait takes its OBJ VRAM before the faces
    for (int k = 0; k < on_page(g); k++) {
        int id = g->ids[g->page * PAGE + k];
        g->locked[k] = !pickable(g, id);
        int sprite = tny_fighters[id].sprite;
        // a giant (64 x 64) or a tall body overflows the 34 x 56 cell: his head, cropped to 32 x 32 (polish)
        if (ui_face_big(sprite)) g->crop[k] = ui_face_crop(sprite, g->locked[k] ? ui_remap_black : 0);
        if (g->crop[k] == 0xFFFF) g->face[k] = ui_face(sprite, face_cx(k), face_cy(k));
        face_look(g, k);
    }
    draw_bar(u, g);
    if (!on_page(g)) ui_text(u, PANEL_X / 2, 70, 1, C_GREY, "No fighters yet");
}

static void move_cursor(Ui *u, Grid *g, int k) {
    if (k == g->cur) return;
    int old = g->cur;
    g->cur = (u8)k;
    draw_cell(g, old);
    draw_cell(g, k);
    face_look(g, old);
    face_look(g, k);
    draw_panel(u, g);
    ui_sfx(UI_SFX_MOVE);
}

// ---- the screen ------------------------------------------------------------------------------------------------
static void grid_open(Ui *u) {
    Grid *g = u->ctx;
    g->port_tile = 0xFFFF;
    for (int k = 0; k < PAGE; k++) g->crop[k] = 0xFFFF;
    build_list(g);
    if (g->mode == GRID_SELECT) {             // the cursor starts on the player's own character
        int slot = g->player ? coop.p2_slot : g_Party->active_slot;
        for (int i = 0; i < g->n; i++)
            if (tny_fighters[g->ids[i]].story_slot == slot) { g->page = (u8)(i / PAGE); g->cur = (u8)(i % PAGE); break; }
    }
    build_page(u, g);
}

static void grid_step(Ui *u) {
    Grid *g = u->ctx;
    for (int k = 0; k < on_page(g); k++) {
        if (g->crop[k] != 0xFFFF) {
            ui_crop_draw(u, g->crop[k], face_cx(k), face_cy(k));
            if (g->locked[k]) ui_qmark_draw(u, face_cx(k), face_cy(k));
        } else if (g->locked[k] && g->face[k]) ui_qmark_draw(u, ui_face_x(g->face[k]), ui_face_y(g->face[k]));
    }
    if (g->port_tile != 0xFFFF) ui_portrait_draw(u, g->port_tile, PORT_X, PORT_Y);
    if (g->confirm) {
        if (--g->confirm == 0) ui_leave(u);
        int on = (g->confirm >> 2) & 1;
        bm_frame(cell_x(g->cur), cell_y(g->cur), CELL_W - 2, CELL_H - 2, on ? C_WHITE : cursor_colour(g));
        return;
    }
    u->idle = 1;
    int n = on_page(g), d = ui_dpad(u), col = g->cur % COLS, row = g->cur / COLS;
    if (d & KEY_RIGHT) {
        if (col < COLS - 1 && g->cur + 1 < n) move_cursor(u, g, g->cur + 1);
        else if (g->page + 1 < pages(g)) {
            g->page++;
            g->cur = (u8)(row * COLS);
            if (g->cur >= on_page(g)) g->cur = 0;
            build_page(u, g);
            ui_sfx(UI_SFX_MOVE);
        }
    } else if (d & KEY_LEFT) {
        if (col > 0) move_cursor(u, g, g->cur - 1);
        else if (g->page > 0) {
            g->page--;
            g->cur = (u8)(row * COLS + COLS - 1);
            build_page(u, g);
            ui_sfx(UI_SFX_MOVE);
        }
    } else if ((d & KEY_UP) && row > 0) {
        move_cursor(u, g, g->cur - COLS);
    } else if ((d & KEY_DOWN) && row < ROWS - 1 && g->cur + COLS < n) {
        move_cursor(u, g, g->cur + COLS);
    }
    if (g->mode == GRID_SELECT && (u->keys & (KEY_L | KEY_R))) {
        g->tab = (u8)((g->tab + ((u->keys & KEY_R) ? 1 : TAB_COUNT - 1)) % TAB_COUNT);
        g->page = 0;
        g->cur = 0;
        build_list(g);
        build_page(u, g);
        ui_sfx(UI_SFX_MOVE);
    }
    if (u->keys & KEY_A) {
        int id = cur_id(g);
        if (g->mode == GRID_SELECT && id >= 0 && pickable(g, id)) {
            ui_sfx(UI_SFX_OK);
            g->pick = (u8)id;
            g->result = 1;
            g->confirm = 24;
        } else if (g->mode == GRID_SELECT) {
            ui_sfx(UI_SFX_BUZZ);
        } else {                                  // the board: A closes it as B does
            ui_sfx(UI_SFX_BACK);
            ui_leave(u);
        }
    } else if (u->keys & KEY_B) {
        ui_sfx(UI_SFX_BACK);
        g->result = 0;
        ui_leave(u);
    }
}

static void grid_close(Ui *u) {
    Grid *g = u->ctx;
    crops_free(g);
    panel_clear(g);
}

int tny_ui_grid(int mode, int player, int tier, u8 *pick) {
    Grid *g = Arena_Alloc(sizeof(Grid));
    if (!g) { ui_st.heap_fail++; return 0; }
    memset(g, 0, sizeof *g);
    g->mode = mode;
    g->player = player ? 1 : 0;
    g->tier = tier;
    g->pick = 0xFF;
    int pads = mode == GRID_BOARD ? UI_PAD_BOTH : (g->player ? UI_PAD_P2 : UI_PAD_P1);
    int ran = ui_run(grid_open, grid_step, grid_close, g, pads);
    int result = ran ? g->result : 0;
    if (pick && result) *pick = g->pick;
    if (mode == GRID_SELECT) { ui_st.last_ret = (s8)result; ui_st.last_pick[g->player] = g->pick; }
    Arena_Free(g);
    return result;
}

int tny_ui_select(int player, int tier, u8 *pick) { return tny_ui_grid(GRID_SELECT, player, tier, pick); }
