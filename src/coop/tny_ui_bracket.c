// The bracket screen and the Rumble board (ui piece; docs/tournament.md section 7).
//
// Bracket: the engine's TBracket (tny_bracket(), or the test stand-in of test command 29) drawn as a two-sided
// tree: the first round's seats down the left and right edges with their names, the winners one column further in
// per round, the champion at the top centre. Quick (8) is one page; Full (16) has a page per half and a page for
// the final. Phase 0 shows the bracket as it stands and presents the next match: the next match's lines and boxes
// light up, both faces slide to the middle and the banner says "GOHAN vs FRIEZA". Phase 1 first animates every
// result since the screen was last shown (kept in the ui's RAM): the players' match first, the loser greys and the
// winner slides to the next column with a sound, then the round's other matches with quick slides (the page
// turns when a result is on the other half), then presents the next match the same way; when the bracket is over
// the champion slides to the top and the banner names him.
// Rumble board: the round's faces with their names, the fallen ones greying one after another.
#include "tny_ui.h"

#define ROW_Y0 38
#define ROW_DY 30
#define NAME_W 44
#define BANNER_Y 136

typedef struct Br {
    int phase;
    TBracket b;
    u8 shown[4][8];          // winners as drawn so far
    u8 page, pages;          // page drawn: Full 0 / 1 the halves, 2 the final
    Entity *face[16];        // per seat, on the page drawn
    u8 ev_r[16], ev_m[16];   // results still to animate
    u8 n_ev, ev;
    u8 state;
    u16 t, dur;
    s16 sx, sy, tx, ty;      // the slide in progress
    s16 sx2, sy2, tx2, ty2;  // the second face of the "vs" slide
    Entity *mover, *mover2;
    u8 vs_a, vs_b;           // seats of the next match (0xFF none)
    u8 wait;
} Br;

enum { S_START, S_EVENT, S_GREY, S_SLIDE, S_PAUSE, S_NEXT, S_VS, S_WAIT };

static int same_bytes(const u8 *a, const u8 *b, int n) {
    for (int i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}

static int match_seat(const TBracket *b, const u8 (*w)[8], int r, int m, int side) {
    (void)b;
    if (r == 0) return m * 2 + side;
    return w[r - 1][m * 2 + side];
}

static int level_of(const Br *br, int s) {
    int l = 0;
    for (int r = 0; r < br->b.rounds; r++) {
        if (br->shown[r][s >> (r + 1)] == s) l = r + 1;
        else break;
    }
    return l;
}
static int is_out(const Br *br, int s) {
    int l = level_of(br, s);
    if (l >= br->b.rounds) return 0;
    u8 w = br->shown[l][s >> (l + 1)];
    return w != 0xFF && w != s;
}
static int full(const Br *br) { return br->b.size == 16; }
static int page_of_match(const Br *br, int r, int m) {
    if (!full(br)) return 0;
    if (r >= 3) return 2;
    return m >> (2 - r);
}
static int seat_on_page(const Br *br, int s) {
    if (!full(br) || br->page < 2) return !full(br) || (s >> 3) == br->page;
    return level_of(br, s) >= 3;     // the final page: the two half champions
}

// Centre of seat s drawn at level l on the page drawn.
static void seat_pos(const Br *br, int s, int l, int *x, int *y) {
    if (full(br) && br->page == 2) {
        if (l >= 4) { *x = 120; *y = 44; return; }
        *x = (s >> 3) ? 160 : 80;
        *y = 92;
        return;
    }
    int local = s & 7;
    if (l == 0) {
        int side = local >> 2, row = local & 3;
        *x = side ? 226 : 14;
        *y = ROW_Y0 + row * ROW_DY;
    } else if (l == 1) {
        int k = local >> 1, side = k >> 1, pair = k & 1;
        *x = side ? 154 : 86;
        *y = ROW_Y0 + (pair * 2) * ROW_DY + ROW_DY / 2;
    } else if (l == 2) {
        int side = local >> 2;
        *x = side ? 134 : 106;
        *y = ROW_Y0 + ROW_DY + ROW_DY / 2;
    } else {
        *x = 120;
        *y = 44;
    }
}

static const char *round_name(const Br *br, int r) {
    int left = br->b.rounds - r;
    if (left <= 1) return "Final";
    if (left == 2) return "Semifinals";
    if (left == 3) return "Quarterfinals";
    return "First round";
}

static int name_colour(const Br *br, int s) {
    if (s == br->b.p1_seat) return C_P1;
    if (s == br->b.p2_seat) return C_P2;
    return is_out(br, s) ? C_GREY : C_WHITE;
}

// ---- drawing ---------------------------------------------------------------------------------------------------
static void elbow(int x0, int y0, int x1, int y1, int c) {
    int a = x0 < x1 ? x0 : x1, w = x0 < x1 ? x1 - x0 : x0 - x1;
    bm_fill(a, y0, w + 2, 2, c);
    int t = y0 < y1 ? y0 : y1, h = y0 < y1 ? y1 - y0 : y0 - y1;
    bm_fill(x1, t, 2, h + 2, c);
}

static void draw_lines(Br *br) {
    int levels = full(br) && br->page < 2 ? 3 : br->b.rounds;
    int r0 = 0;
    if (full(br) && br->page == 2) r0 = 3, levels = 4;
    for (int r = r0; r < levels; r++) {
        int n = br->b.size >> (r + 1);
        for (int m = 0; m < n; m++) {
            if (page_of_match(br, r, m) != br->page) continue;
            int next = br->vs_a != 0xFF && r == br->b.round && m == br->b.match;
            for (int side = 0; side < 2; side++) {
                int s = match_seat(&br->b, br->shown, r, m, side);
                if (s == 0xFF) continue;
                int x0, y0, x1, y1;
                seat_pos(br, s, r, &x0, &y0);
                seat_pos(br, s, r + 1, &x1, &y1);
                u8 w = br->shown[r][m];
                int c = next ? C_GOLD : w == 0xFF ? C_DARK : w == s ? C_LINE : C_DARK;
                elbow(x0, y0 + 6, x1, y1, c);
            }
        }
    }
}

static void draw_names(Ui *u, Br *br) {
    char sb[24];
    for (int s = 0; s < br->b.size; s++) {
        if (!seat_on_page(br, s) || br->b.seat[s] == 0xFF) continue;
        const char *name = tny_fighters[br->b.seat[s]].name;
        if (full(br) && br->page == 2) {
            int x, y;
            seat_pos(br, s, 3, &x, &y);
            ui_text(u, x, y + 18, 1, name_colour(br, s), ui_short_name(name, 76, sb, sizeof sb));
            continue;
        }
        int x, y;
        seat_pos(br, s, 0, &x, &y);
        int right = (s & 7) >= 4;
        ui_text(u, right ? x - 13 : x + 13, y - 12, right ? 2 : 0, name_colour(br, s), ui_short_name(name, NAME_W, sb, sizeof sb));
    }
}

static void draw_title(Ui *u, Br *br) {
    bm_fill(0, 0, 240, 16, C_PANEL);
    bm_hline(0, 16, 240, C_BORDER);
    const char *tier = br->b.tier < TIER_COUNT ? ui_tier_names[br->b.tier] : "";
    ui_text(u, 4, 2, 0, C_GOLD, tier);
    int x = 4 + ui_text_width(tier) + 6;
    ui_text(u, x, 2, 0, C_WHITE, br->b.size == 16 ? "Full bracket" : "Quick bracket");
    int r = br->b.round < br->b.rounds ? br->b.round : br->b.rounds - 1;
    ui_text(u, 236, 2, 2, C_LINE, br->b.round >= br->b.rounds ? "Champion" : round_name(br, r));
    if (full(br)) {
        static const char *const pages[3] = { "Left half", "Right half", "Final" };
        ui_text(u, 120, 2, 1, C_GREY, pages[br->page]);
    }
}

static void face_state(Br *br, int s) {
    Entity *e = br->face[s];
    if (!e) return;
    int dim = br->state == S_VS || br->state == S_WAIT;
    int in_vs = s == br->vs_a || s == br->vs_b;
    ui_face_look(e, in_vs && dim, is_out(br, s) ? UI_REMAP_GREY : (dim && !in_vs) ? UI_REMAP_DIM : 0);
}

static void build_page(Ui *u, Br *br, int page) {
    for (int s = 0; s < 16; s++) { ui_face_kill(br->face[s]); br->face[s] = 0; }
    br->page = (u8)page;
    bm_clear();
    draw_title(u, br);
    if (br->vs_a != 0xFF && page_of_match(br, br->b.round, br->b.match) == page) {   // the next match's boxes
        for (int k = 0; k < 2; k++) {
            int s = k ? br->vs_b : br->vs_a, x, y;
            seat_pos(br, s, level_of(br, s), &x, &y);
            bm_fill(x - 14, y - 16, 30, 32, C_GOLD_DARK);
            bm_frame(x - 14, y - 16, 30, 32, C_GOLD);
        }
    }
    draw_lines(br);
    draw_names(u, br);
    for (int s = 0; s < br->b.size; s++) {
        if (!seat_on_page(br, s) || br->b.seat[s] == 0xFF) continue;
        int x, y;
        seat_pos(br, s, level_of(br, s), &x, &y);
        br->face[s] = ui_face(tny_fighters[br->b.seat[s]].sprite, x, y);
        face_state(br, s);
    }
}

static void banner(Ui *u, const char *a, const char *mid, const char *b) {
    bm_fill(0, BANNER_Y - 2, 240, 18, C_SHADE);
    bm_hline(0, BANNER_Y - 2, 240, C_GOLD);
    bm_hline(0, BANNER_Y + 15, 240, C_GOLD);
    char sb[2][24];
    const char *na = a ? ui_short_name(a, b ? 90 : 220, sb[0], sizeof sb[0]) : 0;
    const char *nb = b ? ui_short_name(b, 90, sb[1], sizeof sb[1]) : 0;
    if (na && nb) {
        ui_text(u, 104, BANNER_Y, 2, C_WHITE, na);
        ui_text(u, 120, BANNER_Y, 1, C_GOLD, mid);
        ui_text(u, 136, BANNER_Y, 0, C_WHITE, nb);
    } else if (na) {
        ui_text(u, 120, BANNER_Y, 1, C_GOLD, na);
    }
}

// ---- results and the next match -----------------------------------------------------------------------------------
static int is_player(const Br *br, int s) { return s != 0xFF && (s == br->b.p1_seat || s == br->b.p2_seat); }

static void collect_events(Br *br) {
    br->n_ev = br->ev = 0;
    for (int r = 0; r < br->b.rounds && r < 4; r++) {
        int n = br->b.size >> (r + 1);
        for (int pass = 0; pass < 2; pass++)          // the players' matches first, then the others
            for (int m = 0; m < n && m < 8; m++) {
                if (br->b.winner[r][m] == 0xFF || br->shown[r][m] != 0xFF) continue;
                int s0 = match_seat(&br->b, br->b.winner, r, m, 0), s1 = match_seat(&br->b, br->b.winner, r, m, 1);
                int pl = is_player(br, s0) || is_player(br, s1);
                if ((pass == 0) != pl) continue;
                br->ev_r[br->n_ev] = (u8)r;
                br->ev_m[br->n_ev] = (u8)m;
                br->n_ev++;
            }
    }
}

static void find_next(Br *br) {
    br->vs_a = br->vs_b = 0xFF;
    if (br->b.round >= br->b.rounds) return;
    int r = br->b.round, m = br->b.match;
    if (m >= (br->b.size >> (r + 1))) return;
    int a = match_seat(&br->b, br->shown, r, m, 0), b = match_seat(&br->b, br->shown, r, m, 1);
    if (a == 0xFF || b == 0xFF) return;
    br->vs_a = (u8)a;
    br->vs_b = (u8)b;
}

static int champion(const Br *br) {
    int r = br->b.rounds - 1;
    return br->b.round >= br->b.rounds ? br->shown[r][0] : 0xFF;
}

static void start_slide(Br *br, Entity *e, int tx, int ty, int dur) {
    br->mover = e;
    br->sx = (s16)ui_face_x(e);
    br->sy = (s16)ui_face_y(e);
    br->tx = (s16)tx;
    br->ty = (s16)ty;
    br->dur = (u16)dur;
    br->t = 0;
}

static int lerp(int a, int b, int t, int d) {
    // ease out: fast start, slow landing
    int k = d - t;
    return b - (b - a) * k * k / (d * d);
}

static void bracket_open(Ui *u) {
    Br *br = u->ctx;
    br->state = S_START;
    int page = 0;
    if (br->n_ev) page = page_of_match(br, br->ev_r[0], br->ev_m[0]);
    else if (br->b.round < br->b.rounds) page = page_of_match(br, br->b.round, br->b.match);
    else if (full(br)) page = 2;
    find_next(br);
    if (br->n_ev) br->vs_a = br->vs_b = 0xFF;     // the boxes come once the results are in
    build_page(u, br, page);
}

static void bracket_step(Ui *u) {
    Br *br = u->ctx;
    ui_st.anim_frames++;
    switch (br->state) {
    case S_START:
        if (++br->t >= 12) { br->t = 0; br->state = br->ev < br->n_ev ? S_EVENT : S_NEXT; }
        break;
    case S_EVENT: {
        int r = br->ev_r[br->ev], m = br->ev_m[br->ev];
        int pg = page_of_match(br, r, m);
        if (pg != br->page) { build_page(u, br, pg); br->state = S_PAUSE; br->t = 0; br->wait = 20; break; }
        int w = br->b.winner[r][m];
        int s0 = match_seat(&br->b, br->shown, r, m, 0), s1 = match_seat(&br->b, br->shown, r, m, 1);
        int loser = w == s0 ? s1 : s0;
        int slow = is_player(br, s0) || is_player(br, s1);
        if (br->face[loser]) ui_face_look(br->face[loser], 0, UI_REMAP_GREY);   // the loser greys
        if (slow) ui_sfx(UI_SFX_BUZZ);
        if (!br->face[w]) {                        // no face to slide (OBJ VRAM): just the result
            br->shown[r][m] = (u8)w;
            br->ev++;
            br->state = S_PAUSE; br->t = 0; br->wait = slow ? 20 : 4;
            break;
        }
        int tx, ty;
        seat_pos(br, w, r + 1, &tx, &ty);
        start_slide(br, br->face[w], tx, ty, slow ? 32 : 10);
        br->state = S_GREY;
        br->t = 0;
        br->wait = slow ? 16 : 0;
        break;
    }
    case S_GREY:                                   // a beat on the greyed loser, then the slide
        if (br->t++ >= br->wait) { br->t = 0; br->state = S_SLIDE; ui_sfx(UI_SFX_SLIDE); }
        break;
    case S_SLIDE: {
        br->t++;
        ui_face_move(br->mover, lerp(br->sx, br->tx, br->t, br->dur), lerp(br->sy, br->ty, br->t, br->dur));
        if (br->t >= br->dur) {
            int r = br->ev_r[br->ev], m = br->ev_m[br->ev];
            br->shown[r][m] = br->b.winner[r][m];
            ui_sfx(UI_SFX_OK);
            br->ev++;
            int slow = br->dur > 10;
            br->state = S_PAUSE;
            br->t = 0;
            br->wait = slow ? 24 : 2;
            if (br->ev >= br->n_ev) build_page(u, br, br->page);   // lines and names in their new colours
        }
        break;
    }
    case S_PAUSE:
        if (br->t++ >= br->wait) { br->t = 0; br->state = br->ev < br->n_ev ? S_EVENT : S_NEXT; }
        break;
    case S_NEXT: {
        find_next(br);
        int champ = champion(br);
        if (br->vs_a == 0xFF && champ == 0xFF) { br->state = S_WAIT; br->t = 0; break; }
        int pg = champ != 0xFF ? (full(br) ? 2 : 0) : page_of_match(br, br->b.round, br->b.match);
        build_page(u, br, pg);
        if (champ != 0xFF) {
            const char *name = tny_fighters[br->b.seat[champ]].name;
            char line[40];
            int n = 0;
            for (const char *p = name; *p && n < 24; p++) line[n++] = *p;
            const char *tail = " wins!";
            for (const char *p = tail; *p; p++) line[n++] = *p;
            line[n] = 0;
            banner(u, line, 0, 0);
            if (br->face[champ]) {
                br->vs_a = (u8)champ;
                ui_face_look(br->face[champ], 1, 0);
            }
            ui_sfx(UI_SFX_WIN);
            br->state = S_WAIT;
            br->t = 0;
            break;
        }
        Entity *a = br->face[br->vs_a], *b = br->face[br->vs_b];
        br->mover = a;
        br->mover2 = b;
        if (a) { br->sx = (s16)ui_face_x(a); br->sy = (s16)ui_face_y(a); }
        if (b) { br->sx2 = (s16)ui_face_x(b); br->sy2 = (s16)ui_face_y(b); }
        br->tx = 92; br->ty = 106; br->tx2 = 148; br->ty2 = 106;
        br->dur = 24;
        br->t = 0;
        br->state = S_VS;
        for (int s = 0; s < 16; s++) face_state(br, s);
        ui_sfx(UI_SFX_SLIDE);
        break;
    }
    case S_VS:
        br->t++;
        if (br->mover) ui_face_move(br->mover, lerp(br->sx, br->tx, br->t, br->dur), lerp(br->sy, br->ty, br->t, br->dur));
        if (br->mover2) ui_face_move(br->mover2, lerp(br->sx2, br->tx2, br->t, br->dur), lerp(br->sy2, br->ty2, br->t, br->dur));
        if (br->t >= br->dur) {
            banner(u, tny_fighters[br->b.seat[br->vs_a]].name, "vs", tny_fighters[br->b.seat[br->vs_b]].name);
            ui_sfx(UI_SFX_OK);
            br->state = S_WAIT;
            br->t = 0;
        }
        break;
    case S_WAIT:
        br->t++;
        if ((u->keys & (KEY_A | KEY_B | KEY_START)) || br->t > 300) {
            if (u->keys) ui_sfx(UI_SFX_OK);
            ui_leave(u);
        }
        break;
    }
}

void tny_ui_bracket(int phase) {
    const TBracket *src = ui_bracket();
    if (!src || (src->size != 8 && src->size != 16)) return;
    Br *br = Arena_Alloc(sizeof(Br));
    if (!br) { ui_st.heap_fail++; return; }
    memset(br, 0, sizeof *br);
    br->phase = phase;
    br->b = *src;
    if (br->b.rounds > 4) br->b.rounds = 4;
    // what the screen showed last time: the results to animate are the difference
    int same = ui_st.magic == UI_MAGIC && ui_st.shown_ok && same_bytes(ui_st.shown_seat, br->b.seat, 16);
    if (phase == 0 || !same) {
        memset(br->shown, 0xFF, sizeof br->shown);
        if (phase == 0) memcpy(br->shown, br->b.winner, sizeof br->shown);
    } else {
        memcpy(br->shown, ui_st.shown_win, sizeof br->shown);
    }
    collect_events(br);
    ui_st.anim_frames = 0;
    ui_run(bracket_open, bracket_step, 0, br, UI_PAD_BOTH);
    memcpy(ui_st.shown_win, br->b.winner, sizeof ui_st.shown_win);
    memcpy(ui_st.shown_seat, br->b.seat, 16);
    ui_st.shown_ok = 1;
    Arena_Free(br);
}

// ---- the Rumble board ----------------------------------------------------------------------------------------------
typedef struct Board {
    u8 ids[8], down[8];
    u8 n, shown_down, t;
    Entity *face[8];
} Board;

static void board_pos(const Board *bd, int i, int *x, int *y) {
    int cols = bd->n <= 4 ? bd->n : (bd->n + 1) / 2;
    int row = i / cols, col = i % cols;
    int rows = (bd->n + cols - 1) / cols;
    int cw = 56;
    *x = 120 - (cols * cw) / 2 + col * cw + cw / 2;
    *y = (rows == 1 ? 72 : 52) + row * 56;
}

static void board_open(Ui *u) {
    Board *bd = u->ctx;
    bm_fill(0, 0, 240, 16, C_PANEL);
    bm_hline(0, 16, 240, C_BORDER);
    ui_text(u, 120, 2, 1, C_GOLD, "Royal Rumble");
    char sb[24];
    for (int i = 0; i < bd->n; i++) {
        int x, y;
        board_pos(bd, i, &x, &y);
        bm_fill(x - 24, y - 20, 48, 52, C_CELL);
        bm_frame(x - 24, y - 20, 48, 52, C_PANEL2);
        bd->face[i] = ui_face(tny_fighters[bd->ids[i]].sprite, x, y - 2);
        ui_face_look(bd->face[i], 0, 0);
        ui_text(u, x, y + 18, 1, C_WHITE, ui_short_name(tny_fighters[bd->ids[i]].name, 50, sb, sizeof sb));
    }
    int left = 0;
    for (int i = 0; i < bd->n; i++) left += !bd->down[i];
    char line[24] = "Standing: ";
    line[10] = (char)('0' + left);
    line[11] = 0;
    ui_text(u, 120, 140, 1, C_LINE, line);
}

static void board_step(Ui *u) {
    Board *bd = u->ctx;
    bd->t++;
    if (bd->shown_down < bd->n && bd->t >= 16) {   // the fallen grey one after another
        bd->t = 0;
        int i = bd->shown_down++;
        if (bd->down[i]) {
            int x, y;
            board_pos(bd, i, &x, &y);
            ui_face_look(bd->face[i], 0, UI_REMAP_GREY);
            bm_fill(x - 24, y + 17, 48, 14, C_CELL);
            char sb[24];
            ui_text(u, x, y + 18, 1, C_GREY, ui_short_name(tny_fighters[bd->ids[i]].name, 50, sb, sizeof sb));
            bm_frame(x - 24, y - 20, 48, 52, C_RED_DARK);
            ui_sfx(UI_SFX_BUZZ);
        }
        return;
    }
    if (bd->shown_down >= bd->n && ((u->keys & (KEY_A | KEY_B | KEY_START)) || bd->t > 240)) ui_leave(u);
}

void tny_ui_rumble_board(const u8 *ids, const u8 *down, int n) {
    if (!ids || n <= 0) return;
    Board bd;
    memset(&bd, 0, sizeof bd);
    for (int i = 0; i < n && bd.n < 8; i++) {
        if (ids[i] >= TF_COUNT) continue;
        bd.ids[bd.n] = ids[i];
        bd.down[bd.n] = down ? (down[i] != 0) : 0;
        bd.n++;
    }
    if (!bd.n) return;
    ui_run(board_open, board_step, 0, &bd, UI_PAD_BOTH);
}
