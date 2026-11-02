// The bracket screen and the Rumble board (ui piece; docs/tournament.md section 7).
//
// Bracket: the engine's TBracket (tny_bracket(), or the test stand-in of test command 29) drawn as a two-sided
// tree: the first round's seats down the left and right edges with their names, the winners one column further in
// per round, the champion at the top centre. Quick (8) is one page; Full (16) has a page per half and a page for
// the final. Phase 0 shows the bracket as it stands and presents the next match: the next match's lines and boxes
// light up, then both faces leave the tree for the ends of the bar at the bottom and the bar says "GOHAN vs FRIEZA"
// (the tree's columns are spaced so that no two faces overlap, and nothing but the two ever crosses them). Phase 1
// first animates every result since the screen was last shown (kept in the ui's RAM): the players' match first, the
// loser greys and the winner walks to the next column with a sound, then the round's other matches a little quicker
// (the page turns when a result is on the other half), then presents the next match the same way; when the bracket
// is over the champion walks to the top and the bar names him. The champion's own screen (tny_ui_champion, the engine
// shows it after this one) is at the end of this file.
// A 2 v 2 (docs/tournament.md 13.3; the engine's unit is a team, seats 2k and 2k + 1, rounds 2): a team's seats sit side
// by side on the edge and move together, winner[r][m] is the winning team's first seat, and the banner names both.
// Rumble board: the round's faces with their names, the fallen ones greying one after another.
#include "tny_ui.h"

#define ROW_Y0 32
#define ROW_DY 28
#define NAME_W 44
#define CHAMP_Y 36
#define BAR_Y 132            // the bar at the bottom, to the screen's end; the tree's faces stop above it
#define BAR_H 28
#define BAR_FACE_Y 146

// The animation's frames: the players' matches, the others', the next match
#define T_START 30
#define T_GREY 30
#define T_SLIDE 44
#define T_PAUSE 40
#define T_GREY_Q 8
#define T_SLIDE_Q 20
#define T_PAUSE_Q 12
#define T_PAGE 30
#define T_SPOT 45            // the next match lit on the tree, before the two leave it
#define T_VS 36
#define T_WAIT 300
#define T_WAIT_CHAMP 150     // the champion's own screen follows

_Static_assert(__builtin_offsetof(UiState, vs) == 141, "tny_coop.txt peeks the banner's seats");

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
    struct { Entity *e; s16 sx, sy, tx, ty; } mate[2];   // a 2 v 2: the partners moving with the slide's faces
    u8 vs_a, vs_b;           // seats of the next match (0xFF none)
    u8 wait;
    u8 skip;                 // A during the animation: the rest plays at once
    u8 hop;                  // the walking winner's little jump, px
} Br;

enum { S_START, S_EVENT, S_GREY, S_SLIDE, S_PAUSE, S_NEXT, S_SPOT, S_VS, S_WAIT };

static int same_bytes(const u8 *a, const u8 *b, int n) {
    for (int i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}

// A 2 v 2's bracket: teams of two seats (the engine's unit), the first seat speaks for the team
static int is_team(const TBracket *b) { return b->division == DIV_2V2 && b->size == 8; }
static int lead_of(const Br *br, int s) { return is_team(&br->b) ? s & ~1 : s; }
static int matches(const TBracket *b, int r) { return (is_team(b) ? b->size / 2 : b->size) >> (r + 1); }
static int match_of(const Br *br, int s, int r) { return (is_team(&br->b) ? s >> 1 : s) >> (r + 1); }

static int match_seat(const TBracket *b, const u8 (*w)[8], int r, int m, int side) {
    if (r == 0) return (m * 2 + side) * (is_team(b) ? 2 : 1);
    return w[r - 1][m * 2 + side];
}

static int level_of(const Br *br, int s) {
    int l = 0, lead = lead_of(br, s);
    for (int r = 0; r < br->b.rounds; r++) {
        if (br->shown[r][match_of(br, lead, r)] == lead) l = r + 1;
        else break;
    }
    return l;
}
static int is_out(const Br *br, int s) {
    int l = level_of(br, s), lead = lead_of(br, s);
    if (l >= br->b.rounds) return 0;
    u8 w = br->shown[l][match_of(br, lead, l)];
    return w != 0xFF && w != lead;
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
    if (is_team(&br->b) && l > 0) {                  // a team: the partners one above the other, then side by side
        int side = s >> 2, second = s & 1;
        if (l == 1) { *x = side ? 156 : 84; *y = ROW_Y0 + ROW_DY + ROW_DY / 2 + (second ? 14 : -14); }
        else { *x = second ? 134 : 106; *y = CHAMP_Y; }
        return;
    }
    if (full(br) && br->page == 2) {
        if (l >= 4) { *x = 120; *y = 44; return; }
        *x = (s >> 3) ? 164 : 76;
        *y = 84;
        return;
    }
    int local = s & 7;
    if (l == 0) {
        int side = local >> 2, row = local & 3;
        *x = side ? 226 : 14;
        *y = ROW_Y0 + row * ROW_DY;
    } else if (l == 1) {
        int k = local >> 1, side = k >> 1, pair = k & 1;
        *x = side ? 156 : 84;
        *y = ROW_Y0 + (pair * 2) * ROW_DY + ROW_DY / 2;
    } else if (l == 2) {
        int side = local >> 2;
        *x = side ? 132 : 108;
        *y = ROW_Y0 + ROW_DY + ROW_DY / 2;
    } else {
        *x = 120;
        *y = CHAMP_Y;
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
    if (is_out(br, s)) return C_GREY;
    return lead_of(br, s) == br->vs_a || lead_of(br, s) == br->vs_b ? C_GOLD : C_WHITE;
}

// ---- drawing ---------------------------------------------------------------------------------------------------
static void elbow(int x0, int y0, int x1, int y1, int c) {
    int a = x0 < x1 ? x0 : x1, w = x0 < x1 ? x1 - x0 : x0 - x1;
    bm_fill(a, y0, w + 2, 2, c);
    int t = y0 < y1 ? y0 : y1, h = y0 < y1 ? y1 - y0 : y0 - y1;
    bm_fill(x1, t, 2, h + 2, c);
}

static void draw_match(Br *br, int r, int m) {
    int next = br->vs_a != 0xFF && r == br->b.round && m == br->b.match;
    for (int side = 0; side < 2; side++) {
        int s = match_seat(&br->b, br->shown, r, m, side);
        if (s == 0xFF) continue;
        int x0, y0, x1, y1;
        seat_pos(br, s, r, &x0, &y0);
        seat_pos(br, s, r + 1, &x1, &y1);
        u8 w = br->shown[r][m];
        int c = next ? C_GOLD : w == 0xFF ? C_DARK : w == s ? C_LINE : C_DARK;
        if (is_team(&br->b) && r == 0) {     // the two seats join, then the team's line runs in
            int xb, yb, xj = (s >> 2) ? 166 : 74;
            seat_pos(br, s + 1, 0, &xb, &yb);
            int ax = x0 < xj ? x0 : xj, w0 = x0 < xj ? xj - x0 : x0 - xj;
            bm_fill(ax, y0 + 6, w0 + 2, 2, c);
            bm_fill(ax, yb + 6, w0 + 2, 2, c);
            bm_fill(xj, y0 + 6, 2, yb - y0 + 2, c);
            elbow(xj, (y0 + yb) / 2 + 6, x1, ROW_Y0 + ROW_DY + ROW_DY / 2, c);
            continue;
        }
        elbow(x0, y0 + 6, x1, y1, c);
    }
}

static void draw_lines(Br *br) {
    int levels = full(br) && br->page < 2 ? 3 : br->b.rounds;
    int r0 = 0;
    if (full(br) && br->page == 2) r0 = 3, levels = 4;
    for (int r = r0; r < levels; r++) {
        int n = matches(&br->b, r);
        for (int m = 0; m < n; m++)
            if (page_of_match(br, r, m) == br->page) draw_match(br, r, m);
    }
}

static void draw_name(Ui *u, Br *br, int s, int colour) {
    char sb[24];
    const char *name = tny_fighters[br->b.seat[s]].name;
    int x, y;
    if (full(br) && br->page == 2) {
        seat_pos(br, s, 3, &x, &y);
        ui_text(u, x, y + 18, 1, colour, ui_short_name(name, 76, sb, sizeof sb));
        return;
    }
    seat_pos(br, s, 0, &x, &y);
    int right = (s & 7) >= 4;
    ui_text(u, right ? x - 13 : x + 13, y - 12, right ? 2 : 0, colour, ui_short_name(name, NAME_W, sb, sizeof sb));
}

static void draw_names(Ui *u, Br *br) {
    for (int s = 0; s < br->b.size; s++)
        if (seat_on_page(br, s) && br->b.seat[s] != 0xFF) draw_name(u, br, s, name_colour(br, s));
}

static void draw_title(Ui *u, Br *br) {
    bm_fill(0, 0, 240, 16, C_PANEL);
    bm_hline(0, 16, 240, C_BORDER);
    const char *tier = br->b.tier < TIER_COUNT ? ui_tier_names[br->b.tier] : "";
    ui_text(u, 4, 2, 0, C_GOLD, tier);
    int x = 4 + ui_text_width(tier) + 6;
    ui_text(u, x, 2, 0, C_WHITE, is_team(&br->b) ? "2 v 2" : br->b.size == 16 ? "Full" : "Quick");
    int r = br->b.round < br->b.rounds ? br->b.round : br->b.rounds - 1;
    ui_text(u, 236, 2, 2, C_LINE, br->b.round >= br->b.rounds ? "Champion" : round_name(br, r));
    if (full(br)) {
        static const char *const pages[3] = { "Left half", "Right half", "Final" };
        ui_text(u, 124, 2, 1, C_GREY, pages[br->page]);
    }
}

static void face_state(Br *br, int s) {
    Entity *e = br->face[s];
    if (!e) return;
    int dim = br->state == S_SPOT || br->state == S_VS || br->state == S_WAIT;
    int in_vs = lead_of(br, s) == br->vs_a || lead_of(br, s) == br->vs_b;
    ui_face_look(e, in_vs && dim, is_out(br, s) ? UI_REMAP_GREY : (dim && !in_vs) ? UI_REMAP_DIM : 0);
}

// The next match's boxes: lit under the two faces, gone (fill 0) once they have left for the bar: the lines stay gold
static void vs_boxes(Br *br, int page, int fill) {
    if (br->vs_a != 0xFF && page_of_match(br, br->b.round, br->b.match) == page) {
        for (int k = 0; k < 2; k++) {
            int s = k ? br->vs_b : br->vs_a, x, y;
            for (int i = 0; i < (is_team(&br->b) ? 2 : 1); i++) {   // a team: both seats
                seat_pos(br, s + i, level_of(br, s + i), &x, &y);
                bm_fill(x - 12, y - 13, 24, 26, fill);
                if (fill) bm_frame(x - 12, y - 13, 24, 26, C_GOLD);
            }
        }
    }
}

static void bar_clear(void) {
    bm_fill(0, BAR_Y, 240, BAR_H, C_SHADE);
    bm_hline(0, BAR_Y, 240, C_GOLD);
    bm_hline(0, BAR_Y + BAR_H - 1, 240, C_GOLD);
}

static void build_page(Ui *u, Br *br, int page) {
    for (int s = 0; s < 16; s++) { ui_face_kill(br->face[s]); br->face[s] = 0; }
    br->page = (u8)page;
    bm_clear();
    draw_title(u, br);
    bar_clear();
    vs_boxes(br, page, C_GOLD_DARK);
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

// One line in the bar; a match between its two faces (two: the teams' longer names, one above the other)
static void banner(Ui *u, const char *a, const char *mid, const char *b, int two) {
    bar_clear();
    char sb[2][40];
    const char *na = a ? ui_short_name(a, b && !two ? 68 : 220, sb[0], sizeof sb[0]) : 0;
    const char *nb = b ? ui_short_name(b, two ? 104 : 68, sb[1], sizeof sb[1]) : 0;
    if (na && nb && two) {
        ui_text(u, 120, BAR_Y + 1, 1, C_WHITE, na);
        int x = 120 - (ui_text_width(nb) + 18) / 2;
        ui_text(u, x, BAR_Y + 14, 0, C_GOLD, mid);
        ui_text(u, x + 18, BAR_Y + 14, 0, C_WHITE, nb);
    } else if (na && nb) {
        ui_text(u, 104, BAR_Y + 8, 2, C_WHITE, na);
        ui_text(u, 120, BAR_Y + 8, 1, C_GOLD, mid);
        ui_text(u, 136, BAR_Y + 8, 0, C_WHITE, nb);
    } else if (na) {
        ui_text(u, 120, BAR_Y + 8, 1, C_GOLD, na);
    }
}

// ---- results and the next match -----------------------------------------------------------------------------------
static int is_player(const Br *br, int s) { return s != 0xFF && (s == br->b.p1_seat || s == br->b.p2_seat); }

// The results to animate: round by round, the players' matches first, then the others grouped by page (the
// page the players' last result is on first), so a Full bracket turns its pages as few times as it can.
static void add_event(Br *br, int r, int m) {
    br->ev_r[br->n_ev] = (u8)r;
    br->ev_m[br->n_ev] = (u8)m;
    br->n_ev++;
}
static int pending(const Br *br, int r, int m) { return br->b.winner[r][m] != 0xFF && br->shown[r][m] == 0xFF; }
static int players_match(const Br *br, int r, int m) {
    return is_player(br, match_seat(&br->b, br->b.winner, r, m, 0)) || is_player(br, match_seat(&br->b, br->b.winner, r, m, 1));
}
static void collect_events(Br *br) {
    br->n_ev = br->ev = 0;
    int last_page = -1;
    for (int r = 0; r < br->b.rounds && r < 4; r++) {
        int n = matches(&br->b, r);
        for (int m = 0; m < n && m < 8; m++)
            if (pending(br, r, m) && players_match(br, r, m)) { add_event(br, r, m); last_page = page_of_match(br, r, m); }
        for (int pass = 0; pass < 4; pass++) {          // pass 0: last_page; passes 1-3: pages 0, 1, 2 (others)
            for (int m = 0; m < n && m < 8; m++) {
                if (!pending(br, r, m) || players_match(br, r, m)) continue;
                int pg = page_of_match(br, r, m);
                if (pass == 0 ? pg != last_page : (pg != pass - 1 || pg == last_page)) continue;
                add_event(br, r, m);
            }
        }
        if (br->n_ev) last_page = page_of_match(br, br->ev_r[br->n_ev - 1], br->ev_m[br->n_ev - 1]);
    }
}

static void find_next(Br *br) {
    br->vs_a = br->vs_b = 0xFF;
    if (br->b.round >= br->b.rounds) return;
    int r = br->b.round, m = br->b.match;
    if (m >= matches(&br->b, r)) return;
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
    br->mate[0].e = br->mate[1].e = 0;
}
// A team's partner moves along: mate k from where he stands to (tx, ty)
static void mate_slide(Br *br, int k, Entity *e, int tx, int ty) {
    br->mate[k].e = e;
    if (!e) return;
    br->mate[k].sx = (s16)ui_face_x(e);
    br->mate[k].sy = (s16)ui_face_y(e);
    br->mate[k].tx = (s16)tx;
    br->mate[k].ty = (s16)ty;
}
// "Krillin & Gohan" for a team, the fighter's name otherwise
static const char *unit_name(const Br *br, int s, char *buf, int size) {
    const char *a = tny_fighters[br->b.seat[s]].name;
    if (!is_team(&br->b) || br->b.seat[s + 1] == 0xFF) return a;
    const char *b = tny_fighters[br->b.seat[s + 1]].name;
    char sa[24], sb[24];
    a = ui_short_name(a, 48, sa, sizeof sa);
    b = ui_short_name(b, 48, sb, sizeof sb);
    int n = 0;
    for (const char *p = a; *p && n < size - 5; p++) buf[n++] = *p;
    buf[n++] = ' '; buf[n++] = '&'; buf[n++] = ' ';
    for (const char *p = b; *p && n < size - 1; p++) buf[n++] = *p;
    buf[n] = 0;
    return buf;
}

// "GOHAN wins!", "KRILLIN & GOHAN advance!"
static void banner_unit(Ui *u, const Br *br, int s, const char *one, const char *team) {
    char tb[40], line[48];
    const char *name = unit_name(br, s, tb, sizeof tb);
    int n = 0;
    for (const char *p = name; *p && n < 36; p++) line[n++] = *p;
    for (const char *p = is_team(&br->b) ? team : one; *p; p++) line[n++] = *p;
    line[n] = 0;
    banner(u, line, 0, 0, 0);
}

static int lerp(int a, int b, int t, int d) {
    // ease out: fast start, slow landing
    int k = d - t;
    return b - (b - a) * k * k / (d * d);
}

static void bracket_open(Ui *u) {
    Br *br = u->ctx;
    u->shrink = 1;                                // giants' faces at half size (tny_ui.c shrink_giants)
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
    ui_st.dbg[0] = br->state; ui_st.dbg[1] = br->ev; ui_st.dbg[2] = br->n_ev; ui_st.dbg[3] = br->page;
    if (br->state != S_WAIT && (u->keys & (KEY_A | KEY_B | KEY_START))) br->skip = 1;
    if (br->skip && br->state != S_WAIT) {            // waits end now, slides land now
        if (br->t < br->wait) br->t = br->wait;
        if (br->state == S_SLIDE || br->state == S_VS) br->t = br->dur - 1;
        if (br->state == S_START) br->t = T_START;
    }
    switch (br->state) {
    case S_START:
        if (++br->t >= T_START) { br->t = 0; br->state = br->ev < br->n_ev ? S_EVENT : S_NEXT; }
        break;
    case S_EVENT: {
        int r = br->ev_r[br->ev], m = br->ev_m[br->ev];
        int pg = page_of_match(br, r, m);
        if (pg != br->page) { build_page(u, br, pg); br->state = S_PAUSE; br->t = 0; br->wait = T_PAGE; break; }
        int w = br->b.winner[r][m];
        int s0 = match_seat(&br->b, br->shown, r, m, 0), s1 = match_seat(&br->b, br->shown, r, m, 1);
        int loser = w == s0 ? s1 : s0;
        int slow = is_player(br, s0) || is_player(br, s1);
        for (int i = 0; i < (is_team(&br->b) ? 2 : 1); i++) {       // the loser greys, his name with him
            if (br->face[loser + i]) ui_face_look(br->face[loser + i], 0, UI_REMAP_GREY);
            if (!is_player(br, loser + i) && seat_on_page(br, loser + i)) draw_name(u, br, loser + i, C_GREY);
        }
        if (slow) ui_sfx(UI_SFX_BUZZ);
        if (!br->face[w]) {                        // no face to slide (OBJ VRAM): just the result
            br->shown[r][m] = (u8)w;
            br->ev++;
            br->state = S_PAUSE; br->t = 0; br->wait = slow ? T_PAUSE : T_PAUSE_Q;
            break;
        }
        int tx, ty;
        seat_pos(br, w, r + 1, &tx, &ty);
        start_slide(br, br->face[w], tx, ty, slow ? T_SLIDE : T_SLIDE_Q);
        br->hop = slow ? 8 : 4;
        if (is_team(&br->b)) {
            int tx2, ty2;
            seat_pos(br, w + 1, r + 1, &tx2, &ty2);
            mate_slide(br, 0, br->face[w + 1], tx2, ty2);
        }
        br->state = S_GREY;
        br->t = 0;
        br->wait = slow ? T_GREY : T_GREY_Q;
        break;
    }
    case S_GREY:                                   // a beat on the greyed loser, then the winner walks on
        if (br->t++ >= br->wait) {
            br->t = 0;
            br->state = S_SLIDE;
            ui_face_look(br->mover, 1, 0);
            ui_face_look(br->mate[0].e, 1, 0);
            ui_sfx(UI_SFX_SLIDE);
        }
        break;
    case S_SLIDE: {
        br->t++;
        int hop = br->hop * 4 * br->t * (br->dur - br->t) / (br->dur * br->dur);
        ui_face_move(br->mover, lerp(br->sx, br->tx, br->t, br->dur), lerp(br->sy, br->ty, br->t, br->dur) - hop);
        if (br->mate[0].e) ui_face_move(br->mate[0].e, lerp(br->mate[0].sx, br->mate[0].tx, br->t, br->dur), lerp(br->mate[0].sy, br->mate[0].ty, br->t, br->dur) - hop);
        if (br->t >= br->dur) {
            int r = br->ev_r[br->ev], m = br->ev_m[br->ev];
            br->shown[r][m] = br->b.winner[r][m];
            ui_face_look(br->mover, 0, 0);
            ui_face_look(br->mate[0].e, 0, 0);
            draw_match(br, r, m);                  // his way here lights up
            if (r < br->b.rounds - 1) banner_unit(u, br, br->b.winner[r][m], " advances!", " advance!");
            ui_sfx(UI_SFX_OK);
            br->ev++;
            br->state = S_PAUSE;
            br->t = 0;
            br->wait = br->dur == T_SLIDE ? T_PAUSE : T_PAUSE_Q;
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
        // the page already shown gets the boxes drawn in place (polish2: a full rebuild cleared the bitmap and the
        // screen showed one frame without names or lines); another page is built anew
        if (pg == br->page) { bar_clear(); vs_boxes(br, pg, C_GOLD_DARK); draw_lines(br); draw_names(u, br); }
        else build_page(u, br, pg);
        if (champ != 0xFF) {
            banner_unit(u, br, champ, " wins!", " win!");
            if (br->face[champ]) {
                br->vs_a = (u8)champ;
                ui_face_look(br->face[champ], 1, 0);
                if (is_team(&br->b) && br->face[champ + 1]) ui_face_look(br->face[champ + 1], 1, 0);
            }
            ui_sfx(UI_SFX_WIN);
            br->state = S_WAIT;
            br->t = 0;
            break;
        }
        br->state = S_SPOT;
        br->t = 0;
        br->wait = T_SPOT;
        for (int s = 0; s < 16; s++) face_state(br, s);
        ui_sfx(UI_SFX_OK);
        break;
    }
    case S_SPOT: {                                 // the next match lit on the tree, then the two come down to the bar
        if (br->t++ < br->wait) break;
        vs_boxes(br, br->page, C_CLEAR);
        draw_lines(br);
        draw_names(u, br);
        Entity *a = br->face[br->vs_a], *b = br->face[br->vs_b];
        br->mover = a;
        br->mover2 = b;
        if (a) { br->sx = (s16)ui_face_x(a); br->sy = (s16)ui_face_y(a); }
        if (b) { br->sx2 = (s16)ui_face_x(b); br->sy2 = (s16)ui_face_y(b); }
        br->ty = br->ty2 = BAR_FACE_Y;
        br->mate[0].e = br->mate[1].e = 0;
        if (is_team(&br->b)) {                       // the teams at the bar's ends, side by side
            br->tx = 14; br->tx2 = 202;
            mate_slide(br, 0, br->face[br->vs_a + 1], 38, BAR_FACE_Y);
            mate_slide(br, 1, br->face[br->vs_b + 1], 226, BAR_FACE_Y);
        }
        else { br->tx = 18; br->tx2 = 222; }
        br->dur = T_VS;
        br->t = 0;
        br->state = S_VS;
        ui_sfx(UI_SFX_SLIDE);
        break;
    }
    case S_VS:
        br->t++;
        if (br->mover) ui_face_move(br->mover, lerp(br->sx, br->tx, br->t, br->dur), lerp(br->sy, br->ty, br->t, br->dur));
        if (br->mover2) ui_face_move(br->mover2, lerp(br->sx2, br->tx2, br->t, br->dur), lerp(br->sy2, br->ty2, br->t, br->dur));
        for (int k = 0; k < 2; k++)
            if (br->mate[k].e) ui_face_move(br->mate[k].e, lerp(br->mate[k].sx, br->mate[k].tx, br->t, br->dur), lerp(br->mate[k].sy, br->mate[k].ty, br->t, br->dur));
        if (br->t >= br->dur) {
            char ta[40], tb[40];
            banner(u, unit_name(br, br->vs_a, ta, sizeof ta), "vs", unit_name(br, br->vs_b, tb, sizeof tb), is_team(&br->b));
            ui_st.vs[0] = br->vs_a; ui_st.vs[1] = br->vs_b;
            ui_sfx(UI_SFX_OK);
            br->state = S_WAIT;
            br->t = 0;
        }
        break;
    case S_WAIT:
        u->idle = 1;
        br->t++;
        if ((u->keys & (KEY_A | KEY_B | KEY_START)) || br->t > (champion(br) != 0xFF ? T_WAIT_CHAMP : T_WAIT)) {
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
    u->shrink = 1;
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
    if (bd->shown_down < bd->n && bd->t >= 24) {   // the fallen grey one after another
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
    if (bd->shown_down >= bd->n) u->idle = 1;
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

// ---- the champion's own screen -------------------------------------------------------------------------------------
// After the last result (and after a Rumble's headliner): the tier and the division on top, the champion's dialogue
// portrait (or his standing body, when he has none) rising into a gold frame, "CHAMPION" letter by letter, then the
// jingle, the bar's line and confetti at the sides. A once finishes the show, then closes; it closes itself after 10 s.
#define CH_RISE 44
#define CH_LETTER 7
#define CH_WAIT 600
#define CH_FLAKES 18
#define CH_PANEL_X 44
#define CH_PANEL_W 152
#define CH_BOX_Y 42
#define CH_LINE_T (CH_RISE + 8 * CH_LETTER + 12)

typedef struct Champ {
    u8 ids[2], n, tier, division, full;
    u16 tile[2];             // the portraits, 0xFFFF none
    Entity *body[2];         // or the body
    u16 t, left;
    u8 fx[CH_FLAKES], fy[CH_FLAKES];
} Champ;

static int champ_x(const Champ *c, int i) { return c->n > 1 ? (i ? 128 : 48) : 88; }

static void champ_letters(Ui *u, int upto) {
    static const char word[] = "CHAMPION";
    char one[2] = { 0, 0 };
    for (int i = 0; i < upto && word[i]; i++) {
        one[0] = word[i];
        ui_text(u, 120 - 49 + i * 14, 24, 1, C_GOLD, one);
    }
}

static void champ_line(Ui *u, const Champ *c) {
    char sa[24], sb[24], line[72];
    int n = 0;
    #define PUT(str) for (const char *p = (str); *p && n < (int)sizeof line - 1; p++) line[n++] = *p
    PUT(ui_short_name(tny_fighters[c->ids[0]].name, 76, sa, sizeof sa));
    if (c->n > 1) { PUT(" & "); PUT(ui_short_name(tny_fighters[c->ids[1]].name, 76, sb, sizeof sb)); }
    PUT(c->n > 1 ? " win the " : " wins the ");
    PUT(c->tier < TIER_COUNT ? ui_tier_names[c->tier] : "");
    PUT(c->division == DIV_RUMBLE ? " Royal Rumble!" : c->division == DIV_2V2 ? " 2 v 2!" : " tournament!");
    #undef PUT
    line[n] = 0;
    banner(u, line, 0, 0, 0);
}

static void champ_open(Ui *u) {
    Champ *c = u->ctx;
    bm_fill(0, 0, 240, 16, C_PANEL);
    bm_hline(0, 16, 240, C_BORDER);
    const char *tier = c->tier < TIER_COUNT ? ui_tier_names[c->tier] : "";
    ui_text(u, 4, 2, 0, C_GOLD, tier);
    const char *div = c->division == DIV_RUMBLE ? "Royal Rumble" : c->division == DIV_2V2 ? "2 v 2" : c->full ? "Full" : "Quick";
    ui_text(u, 4 + ui_text_width(tier) + 6, 2, 0, C_WHITE, div);
    bm_fill(CH_PANEL_X + 3, 22, CH_PANEL_W, 106, C_SHADE);
    bm_fill(CH_PANEL_X, 19, CH_PANEL_W, 106, C_PANEL);
    bm_frame(CH_PANEL_X, 19, CH_PANEL_W, 106, C_GOLD);
    bm_frame(CH_PANEL_X + 1, 20, CH_PANEL_W - 2, 104, C_GOLD_DARK);
    bar_clear();
    char sb[24];
    for (int i = 0; i < c->n; i++) {
        const TFighter *f = &tny_fighters[c->ids[i]];
        int x = champ_x(c, i);
        bm_fill(x - 1, CH_BOX_Y - 1, 66, 66, C_NAVY);
        bm_frame(x - 2, CH_BOX_Y - 2, 68, 68, C_GOLD);
        ui_text(u, x + 32, CH_BOX_Y + 66, 1, C_WHITE, ui_short_name(f->name, 76, sb, sizeof sb));
        c->tile[i] = ui_portrait_load(f->portrait);
        if (c->tile[i] == 0xFFFF) c->body[i] = ui_face(f->sprite, x + 32, 160 + 32);
    }
    for (int i = 0; i < CH_FLAKES; i++) { c->fx[i] = 0xFF; c->fy[i] = (u8)(18 + i * 61 % 108); }
}

static const u8 flake_colour[5] = { C_GOLD, C_P1, C_P2, C_GREEN, C_WHITE };

// In the columns beside the panel, where the bitmap has nothing else to lose
static void champ_flakes(Champ *c) {
    for (int i = 0; i < CH_FLAKES; i++) {
        if (c->fx[i] != 0xFF) bm_fill(c->fx[i], c->fy[i], 2, 3, C_CLEAR);
        int y = c->fy[i] + 1 + (i & 1);
        if (y >= BAR_Y - 4) y = 18;
        int col = (i * 7 + (c->t >> 3)) % 36;
        int x = (i & 1) ? 202 + col : 2 + col;
        c->fx[i] = (u8)x;
        c->fy[i] = (u8)y;
        bm_fill(x, y, 2, 3, flake_colour[i % 5]);
    }
}

static void champ_step(Ui *u) {
    Champ *c = u->ctx;
    int pressed = u->keys & (KEY_A | KEY_B | KEY_START);
    if (pressed && c->t < CH_LINE_T) {             // the show ends at once
        champ_letters(u, 8);
        c->t = CH_LINE_T - 1;
        pressed = 0;
    }
    if (c->t < 0xFFFF) c->t++;
    if (c->t >= CH_RISE && c->t < CH_LINE_T) {     // the letters
        int k = (c->t - CH_RISE) / CH_LETTER;
        if ((c->t - CH_RISE) % CH_LETTER == 0 && k < 8) { champ_letters(u, k + 1); ui_sfx(UI_SFX_MOVE); }
    }
    if (c->t == CH_LINE_T) { champ_line(u, c); ui_sfx(UI_SFX_WIN); }
    int d = CH_RISE - (c->t < CH_RISE ? c->t : CH_RISE);
    int y = CH_BOX_Y + (160 - CH_BOX_Y) * d * d / (CH_RISE * CH_RISE);   // ease out: fast start, slow landing
    for (int i = 0; i < c->n; i++) {
        if (c->tile[i] != 0xFFFF) ui_portrait_draw(u, c->tile[i], champ_x(c, i), y);
        else ui_face_move(c->body[i], champ_x(c, i) + 32, y + 32);
    }
    if (c->t < CH_LINE_T) return;
    if (!(c->t & 1)) champ_flakes(c);
    u->idle = 1;
    c->left++;
    if (pressed || c->left > CH_WAIT) {
        if (pressed) ui_sfx(UI_SFX_OK);
        ui_leave(u);
    }
}

void tny_ui_champion(const u8 *ids, int n, int tier, int division, int full) {
    if (!ids || n <= 0) return;
    Champ c;
    memset(&c, 0, sizeof c);
    for (int i = 0; i < n && c.n < 2; i++) if (ids[i] < TF_COUNT) c.ids[c.n++] = ids[i];
    if (!c.n) return;
    c.tier = (u8)tier;
    c.division = (u8)division;
    c.full = (u8)full;
    ui_run(champ_open, champ_step, 0, &c, UI_PAD_BOTH);
}
