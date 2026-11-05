// The bracket screen and the Rumble board (ui piece; docs/tournament.md section 7, 12 and 26).
//
// Bracket: the engine's TBracket (tny_bracket(), or the test stand-in of test command 29).
// 8 seats (a Quick bracket, 4 teams): one page, a two-sided tree: the first round's seats down the left and right edges
// with their names, the winners one column further in per round, the champion at the top centre.
// 16 seats (a Full bracket, 8 teams): one half at a time. A half page is one-sided (docs/tournament.md 26): all eight names
// in a column on its outer edge, each on its own row, their faces zigzag in two columns beside them on the line, the tree
// runs inwards to the "Final" box on the inner edge; the right half is the left half mirrored; the final has its own page.
// 32 seats (16 teams, a 2 v 2's alone; docs/tournament.md 26): four "block" pages, each as an 8-team half page (four teams, their two rounds, the box they lead
// to), then the "Final four" page, the 4-team tree (the block winners, the semifinals, the final). Never more than 8 faces on a page, as on every other page.
// The pages slide (S_PAN): BG1 scrolls out, the page is drawn while off screen, it scrolls in from the other side.
// Phase 0 shows the bracket as it stands and presents the next match: with 16 seats first a look at each half, with 32 at each block; the next
// match's lines (and boxes) light up, then both faces leave the tree for the ends of the bar at the bottom and the bar says
// "GOHAN vs FRIEZA". Phase 1 first animates every result since the screen was last shown (kept in the ui's RAM): the players'
// match first, the loser greys and the winner walks to the next column with a sound, then the page's other matches in one
// staggered sweep (a page turn when a result is on the other half), then presents the next match the same way; when the
// bracket is over the champion walks to the top and the bar names him. The champion's own screen (tny_ui_champion, the
// engine shows it after this one) is at the end of this file.
// The players' road (draw_players): from the seat to the final in the player's colour, until he loses.
// A 2 v 2 (docs/tournament.md 13.3; the engine's unit is a team, seats 2k and 2k + 1, rounds one fewer): a team's seats
// join and move together, winner[r][m] is the winning team's first seat, and the banner names both.
// Rumble board: the round's faces with their names, the fallen ones greying one after another.
#include "tny_ui.h"

#define ROW_Y0 32
#define ROW_DY 28
#define NAME_W 44
#define CHAMP_Y 36
#define BAR_Y 132            // the bar at the bottom, to the screen's end; the tree's faces stop above it
#define BAR_H 28
#define BAR_FACE_Y 146

// A half page, as the left half (the right half: 239 - x): the rows, the columns
#define HR(i) (29 + 13 * (i))
#define H_MID 74             // between rows 3 and 4
#define H_NAME_X 3
#define H_NAME_W 44
#define H_LINE_X 48          // the seat's line starts where the longest name ends
#define H_COL_A 60           // the first-round faces: seats 0, 2, 4, 6 ...
#define H_COL_B 82           // ... and 1, 3, 5, 7, so that no two touch (26 px apart in a column)
#define H_TEAM_J 96          // a team's two lines join
#define H_BOX_X 206          // the "Final" box, to the inner edge
#define H_BOX_W 32
#define F_BOX_Y 26           // the final page: the champion's box
#define F_BOX_H 36

// The animation's frames: the players' matches, the others' sweep, the next match. Docs: docs/tournament.md 26.
#define T_START 24
#define T_GREY 24            // the players' match: a beat on the greyed loser, the winner's walk, a beat on the result
#define T_SLIDE 40
#define T_PAUSE 32
#define T_GAP_Q 8            // the others: one after another, each starting this long after the last
#define T_GREY_Q 6
#define T_SLIDE_Q 22
#define T_PAUSE_Q 14
#define T_PAGE 12            // a beat after a page has slid in, before its results
#define T_PAN 20             // a page's slide: out, then the next one in (240 px each)
#define T_TOUR 90            // the first look at each half
#define T_TOUR_B 60          // ... at each of 16 teams' four blocks
#define T_SPOT 40            // the next match lit on the tree, before the two leave it
#define T_VS 30
#define T_WAIT 240
#define T_WAIT_CHAMP 150     // the champion's own screen follows

_Static_assert(__builtin_offsetof(UiState, vs) == 141, "tny_coop.txt peeks the banner's seats");
#define ALL_SEATS 32         // "the rest of the page": no page has more
static u8 ui_shown_seat[16] TNY_SECTION("us") __attribute__((used));   // the bracket's first seats as last drawn: the same draw? (UiState.shown_ok)

// One result being animated: the winner (and a team's partner) on his way
typedef struct Walk {
    Entity *e[2];
    s16 sx[2], sy[2], tx[2], ty[2];
} Walk;
#define WALKS 4

typedef struct Br {
    int phase;
    TBracket b;
    u8 shown[4][8];          // winners as drawn so far
    u8 page;                 // page drawn: 16 seats 0 / 1 the halves, 2 the final; 32 seats 0-3 the blocks, 4 the final four
    Entity *face[ALL_SEATS]; // per seat, on the page drawn
    u8 ev_r[16], ev_m[16];   // results still to animate
    u8 n_ev, ev;
    u8 state;
    u16 t, dur;
    s16 sx, sy, tx, ty;      // the slide to the bar in progress
    s16 sx2, sy2, tx2, ty2;  // the second face
    Entity *mover, *mover2;
    struct { Entity *e; s16 sx, sy, tx, ty; } mate[2];   // a 2 v 2: the partners moving with the slide's faces
    Walk walk[WALKS];        // the results in progress (S_WALK): events ev .. ev + n_walk - 1
    u8 n_walk, slow;         // slow: the players' match, alone
    u8 vs_a, vs_b;           // seats of the next match (0xFF none)
    u8 wait;
    u8 skip;                 // A during the animation: the rest plays at once
    u8 pan_to, pan_after, pan_in;   // the page's slide (S_PAN): the page coming, the state after, in its second half
    s8 pan_dir;              // the pages move: -1 to the left
    s16 pan_off;             // the page's x offset now (the faces have it added)
    u8 name_i;               // the page that slides in: its names, one a frame (seats from here on still to draw)
    u32 face_done;           // ... and its faces, two a frame (seats done)
    u8 dim_i;                // S_SPOT: the faces take their looks a few a frame (seats from here on)
    u8 titled;               // the title is drawn (it stays through the page turns, but for "Left half")
} Br;

enum { S_START, S_EVENT, S_WALK, S_PAUSE, S_NEXT, S_SPOT, S_VS, S_WAIT, S_PAN, S_TOUR };

static int same_bytes(const u8 *a, const u8 *b, int n) {
    for (int i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}

// A 2 v 2's bracket: teams of two seats (the engine's unit), the first seat speaks for the team
static int is_team(const TBracket *b) { return b->division == DIV_2V2; }
// 16 seats: 16 fighters or 8 teams, a page per half
static int halves(const TBracket *b) { return b->size == 16; }
// 32 seats: 16 teams, a page per block of four teams
static int blocks(const TBracket *b) { return b->size == 32; }
static int paged(const TBracket *b) { return b->size != 8; }
static int final_page(const TBracket *b) { return blocks(b) ? 4 : 2; }
// A page of the one-sided kind: a half or a block
static int on_side_page(const Br *br) { return paged(&br->b) && br->page < final_page(&br->b); }
// The 16-seat bracket's own final page
static int on_final2(const Br *br) { return halves(&br->b) && br->page == 2; }
// A page of the two-sided tree: the 8-seat bracket's, or 32 seats' last, which is that same picture of 4 teams
static int on_tree(const Br *br) { return !paged(&br->b) || (blocks(&br->b) && br->page == 4); }
// The round that page's seats stand in, and a seat's place in its 8-seat picture
static int tree_r0(const Br *br) { return blocks(&br->b) ? 2 : 0; }
static int tree_seat(const Br *br, int s) { return blocks(&br->b) ? ((s >> 3) << 1) | (s & 1) : s; }
// On the right of the floor plan (a half page and a block mirror there)
static int seat_right(const Br *br, int s) { return blocks(&br->b) ? s >> 4 : s >> 3; }
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
static int page_of_match(const Br *br, int r, int m) {
    if (!paged(&br->b)) return 0;
    if (blocks(&br->b)) return r >= 2 ? 4 : r ? m : m >> 1;   // eight matches, then four: two on a block, then its winner's; the rest on the last page
    if (r >= br->b.rounds - 1) return 2;
    return m >> (br->b.rounds - 2 - r);
}
static int seat_on_page(const Br *br, int s) {
    if (!paged(&br->b)) return 1;
    if (br->page < final_page(&br->b)) return (s >> 3) == br->page;
    return level_of(br, s) >= br->b.rounds - (blocks(&br->b) ? 2 : 1);   // the last page: the half / block champions
}

static int h_x(const Br *br, int s, int x) { return seat_right(br, s) ? 239 - x : x; }

// A half page or the final page: centre of seat s at level l
static void half_pos(const Br *br, int s, int l, int *x, int *y) {
    int team = is_team(&br->b), local = s & 7, second = team ? s & 1 : 0;
    if (on_final2(br)) {
        int side = s >> 3;
        if (team) {
            if (l >= 3) { *x = second ? 134 : 106; *y = 44; }
            else { *x = (side ? 160 : 52) + second * 28; *y = 84; }
        } else if (l >= 4) { *x = 120; *y = 44; }
        else { *x = side ? 168 : 72; *y = 84; }
        return;
    }
    int xx, yy;
    if (l == 0) { xx = local & 1 ? H_COL_B : H_COL_A; yy = HR(local); }
    else if (team) {
        xx = l == 1 ? 116 : 172;
        yy = (l == 1 ? (HR(local & 4) + HR((local & 4) + 3)) / 2 : H_MID) + (second ? 13 : -13);
    }
    else if (l == 1) { xx = 112; yy = (HR(local & 6) + HR((local & 6) + 1)) / 2; }
    else if (l == 2) { xx = 146; yy = (HR(local & 4) + HR((local & 4) + 3)) / 2; }
    else { xx = 180; yy = H_MID; }
    *x = h_x(br, s, xx);
    *y = yy;
}

// Centre of seat s drawn at level l on the page drawn.
static void seat_pos(const Br *br, int s, int l, int *x, int *y) {
    if (!on_tree(br)) { half_pos(br, s, l, x, y); return; }
    s = tree_seat(br, s);
    l -= tree_r0(br);
    if (l < 0) l = 0;
    if (is_team(&br->b) && l > 0) {   // a team: the partners one above the other, then side by side
        int side = s >> 2, second = s & 1;
        if (l == 1) { *x = side ? 156 : 84; *y = ROW_Y0 + ROW_DY + ROW_DY / 2 + (second ? 14 : -14); }
        else { *x = second ? 134 : 106; *y = CHAMP_Y; }
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

// The line of a seat (a team's first seat) from level r to level r + 1
static void seat_line(Br *br, int s, int r, int c) {
    int x0, y0, x1, y1;
    seat_pos(br, s, r, &x0, &y0);
    seat_pos(br, s, r + 1, &x1, &y1);
    int team = is_team(&br->b), tree = on_tree(br), rt = tree ? r - tree_r0(br) : r;
    if (team && (rt > 0 || !tree)) {                // from the team's middle to the team's middle
        int xb, yb;
        seat_pos(br, s + 1, r, &xb, &yb);
        x0 = (x0 + xb) / 2; y0 = (y0 + yb) / 2;
        seat_pos(br, s + 1, r + 1, &xb, &yb);
        x1 = (x1 + xb) / 2; y1 = (y1 + yb) / 2;
    }
    if (!tree) {
        if (on_final2(br)) y1 = F_BOX_Y + F_BOX_H;  // up to the champion's box
        if (r == 0) {
            x0 = h_x(br, s, H_LINE_X);
            if (team) {                             // the two seats' lines join, the team's runs on from there
                int xj = h_x(br, s, H_TEAM_J), ya = HR(s & 7), yb = HR((s & 7) + 1);
                int a = x0 < xj ? x0 : xj, w = x0 < xj ? xj - x0 : x0 - xj;
                bm_fill(a, ya, w + 2, 2, c);
                bm_fill(a, yb, w + 2, 2, c);
                bm_fill(xj, ya, 2, yb - ya + 2, c);
                x0 = xj;
                y0 = (ya + yb) / 2;
            }
        }
        elbow(x0, y0, x1, y1, c);
        return;
    }
    if (team && rt == 0) {                          // the two seats join, then the team's line runs in
        int xb, yb, xj = (tree_seat(br, s) >> 2) ? 166 : 74;
        seat_pos(br, s + 1, r, &xb, &yb);
        int ax = x0 < xj ? x0 : xj, w0 = x0 < xj ? xj - x0 : x0 - xj;
        bm_fill(ax, y0 + 6, w0 + 2, 2, c);
        bm_fill(ax, yb + 6, w0 + 2, 2, c);
        bm_fill(xj, y0 + 6, 2, yb - y0 + 2, c);
        elbow(xj, (y0 + yb) / 2 + 6, x1, ROW_Y0 + ROW_DY + ROW_DY / 2, c);
        return;
    }
    elbow(x0, rt ? y0 : y0 + 6, x1, y1, c);
}

static void draw_match(Br *br, int r, int m) {
    int next = br->vs_a != 0xFF && r == br->b.round && m == br->b.match;
    for (int side = 0; side < 2; side++) {
        int s = match_seat(&br->b, br->shown, r, m, side);
        u8 w = br->shown[r][m];
        // nobody there yet: the tree's line all the same, from the place the first seat under it would stand
        if (s == 0xFF) seat_line(br, ((m * 2 + side) << r) * (is_team(&br->b) ? 2 : 1), r, C_DARK);
        else seat_line(br, s, r, next ? C_GOLD : w == s ? C_LINE : C_DARK);
    }
}

// A half page or a block: the line from its last column to the box it leads to
static void final_stub(Br *br, int c) {
    int s = br->page << 3;
    int xs = is_team(&br->b) ? 172 : 180;
    elbow(h_x(br, s, xs), H_MID, h_x(br, s, H_BOX_X), H_MID, c);
}

// The players' road: what they have won and what is ahead of them, in their colours, until they lose
static void draw_players(Br *br) {
    for (int k = 0; k < (is_team(&br->b) ? 1 : 2); k++) {
        int p = k ? br->b.p2_seat : br->b.p1_seat;
        if (p == 0xFF) continue;
        int lead = lead_of(br, p), l = level_of(br, lead), out = is_out(br, lead), c = k ? C_P2 : C_P1;
        for (int r = 0; r < br->b.rounds; r++) {
            if (r >= l && out) break;
            if (page_of_match(br, r, match_of(br, lead, r)) == br->page) seat_line(br, lead, r, c);
        }
        if (!out && on_side_page(br) && (lead >> 3) == br->page) final_stub(br, c);
    }
}

// A player has lost at round r: the road he had ahead is taken up (draw_lines puts back what is under it)
static void erase_road(Br *br, int lead, int r) {
    for (r++; r < br->b.rounds; r++)
        if (page_of_match(br, r, match_of(br, lead, r)) == br->page) seat_line(br, lead, r, C_CLEAR);
    if (on_side_page(br) && (lead >> 3) == br->page) final_stub(br, C_CLEAR);
}

static void draw_lines(Br *br) {
    if (on_side_page(br)) final_stub(br, br->shown[blocks(&br->b) ? 1 : br->b.rounds - 2][br->page] == 0xFF ? C_DARK : C_LINE);
    for (int r = 0; r < br->b.rounds; r++) {
        int n = matches(&br->b, r);
        for (int m = 0; m < n && m < 8; m++)
            if (page_of_match(br, r, m) == br->page) draw_match(br, r, m);
    }
    draw_players(br);
    if (br->vs_a != 0xFF && br->b.round < br->b.rounds && page_of_match(br, br->b.round, br->b.match) == br->page)
        draw_match(br, br->b.round, br->b.match);
}

static void draw_name(Ui *u, Br *br, int s, int colour) {
    char sb[24];
    const char *name = tny_fighters[br->b.seat[s]].name;
    int x, y;
    if (!on_tree(br)) {
        if (on_final2(br)) {
            if (is_team(&br->b)) ui_text(u, (s >> 3) ? 174 : 66, 102 + (s & 1) * 12, 1, colour, ui_short_name(name, 76, sb, sizeof sb));
            else { seat_pos(br, s, br->b.rounds - 1, &x, &y); ui_text(u, x, y + 18, 1, colour, ui_short_name(name, 76, sb, sizeof sb)); }
            return;
        }
        ui_text(u, h_x(br, s, H_NAME_X), HR(s & 7) - 6, seat_right(br, s) ? 2 : 0, colour, ui_short_name(name, H_NAME_W, sb, sizeof sb));
        return;
    }
    seat_pos(br, s, tree_r0(br), &x, &y);
    int right = tree_seat(br, s) >= 4;
    ui_text(u, right ? x - 13 : x + 13, y - 12, right ? 2 : 0, colour, ui_short_name(name, NAME_W, sb, sizeof sb));
}

// The next n names of the page drawn
static void names_step(Ui *u, Br *br, int n) {
    for (; br->name_i < br->b.size && n > 0; br->name_i++) {
        int s = br->name_i;
        if (seat_on_page(br, s) && br->b.seat[s] != 0xFF) { draw_name(u, br, s, name_colour(br, s)); n--; }
    }
}

// The names of the next match, lit (the rest stand as they are)
static void vs_names(Ui *u, Br *br) {
    for (int k = 0; k < 2; k++) {
        int s = k ? br->vs_b : br->vs_a;
        if (s == 0xFF) continue;
        for (int i = 0; i < (is_team(&br->b) ? 2 : 1); i++)
            if (seat_on_page(br, s + i) && br->b.seat[s + i] != 0xFF) draw_name(u, br, s + i, name_colour(br, s + i));
    }
}

static void draw_page_label(Ui *u, Br *br) {
    static const char *const block[5] = { "Block A", "Block B", "Block C", "Block D", "Final four" };
    if (blocks(&br->b)) ui_text(u, 130, 2, 1, C_GREY, block[br->page]);
    else if (on_side_page(br)) ui_text(u, 130, 2, 1, C_GREY, br->page ? "Right half" : "Left half");
}

static void draw_title(Ui *u, Br *br) {
    bm_fill(0, 0, 240, 16, C_PANEL);
    const char *tier = br->b.tier < TIER_COUNT ? ui_tier_names[br->b.tier] : "";
    ui_text(u, 4, 2, 0, C_GOLD, tier);
    int x = 4 + ui_text_width(tier) + 6;
    ui_text(u, x, 2, 0, C_WHITE, is_team(&br->b) ? "2 v 2" : br->b.size == 16 ? "Full" : "Quick");
    int r = br->b.round < br->b.rounds ? br->b.round : br->b.rounds - 1;
    ui_text(u, 236, 2, 2, C_LINE, br->b.round >= br->b.rounds ? "Champion" : round_name(br, r));
}

static void face_state(Br *br, int s) {
    Entity *e = br->face[s];
    if (!e) return;
    int dim = br->state == S_SPOT || br->state == S_VS || br->state == S_WAIT;
    int in_vs = lead_of(br, s) == br->vs_a || lead_of(br, s) == br->vs_b;
    ui_face_look(e, in_vs && dim, is_out(br, s) ? UI_REMAP_GREY : (dim && !in_vs) ? UI_REMAP_DIM : 0);
}

// The next match's boxes: lit under the two faces, gone (fill 0) once they have left for the bar: the lines stay gold.
// None in a half page's first round: its faces stand too close, the gold names and lines say it
static void vs_boxes(Br *br, int page, int fill) {
    if (br->vs_a != 0xFF && page_of_match(br, br->b.round, br->b.match) == page) {
        for (int k = 0; k < 2; k++) {
            int s = k ? br->vs_b : br->vs_a, x, y;
            for (int i = 0; i < (is_team(&br->b) ? 2 : 1); i++) {   // a team: both seats
                int l = level_of(br, s + i);
                if (l == 0 && !on_tree(br)) continue;
                seat_pos(br, s + i, l, &x, &y);
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

// The next n faces of the page drawn, those nearest the final first (they come on screen first when the page slides in)
static void faces_step(Br *br, int n) {
    for (; n > 0; n--) {
        int best = -1, best_l = -1;
        for (int s = 0; s < br->b.size; s++) {
            if ((br->face_done >> s) & 1) continue;
            if (!seat_on_page(br, s) || br->b.seat[s] == 0xFF) continue;
            int l = level_of(br, s);
            if (l > best_l) { best = s; best_l = l; }
        }
        if (best < 0) return;
        br->face_done |= 1u << best;
        int x, y;
        seat_pos(br, best, best_l, &x, &y);
        br->face[best] = ui_face(tny_fighters[br->b.seat[best]].sprite, x + br->pan_off, y);
        face_state(br, best);
    }
}

// The whole page but for what build_later leaves to the slide: a name is a third of a frame's work, eight new faces two
// frames' on their first update, and the page's outer edge is the last to come on screen: drawn a little a frame while
// it is still off screen, the slide never stops. (Opened from black: all at once.)
static void build_page(Ui *u, Br *br, int page, int later) {
    for (int s = 0; s < ALL_SEATS; s++) { ui_face_kill(br->face[s]); br->face[s] = 0; }
    br->face_done = 0;
    br->name_i = 0;
    br->page = (u8)page;
    if (br->titled) bm_clear_below_title();
    else { bm_clear(); draw_title(u, br); br->titled = 1; }
    bm_hline(0, 16, 240, C_BORDER);
    bm_fill(102, 0, 58, 16, C_PANEL);               // the page's label: when it lands
    if (!later) draw_page_label(u, br);
    bar_clear();
    if (on_side_page(br)) {                         // the names' column; the box the half or the block leads to
        int s = page << 3, right = seat_right(br, s);
        bm_fill(right ? 239 - H_LINE_X - 1 : 0, 17, H_LINE_X + 2, BAR_Y - 17, C_NAVY);
        int bx = right ? 239 - H_BOX_X - H_BOX_W + 1 : H_BOX_X;
        bm_fill(bx, H_MID - 8, H_BOX_W, 18, C_PANEL);
        bm_frame(bx, H_MID - 8, H_BOX_W, 18, C_GOLD);
        ui_text(u, h_x(br, s, H_BOX_X + H_BOX_W / 2), H_MID - 6, 1, C_GOLD, blocks(&br->b) ? "Semis" : "Final");
    } else if (on_final2(br)) {                     // the champion's box, the finalists come to it
        int w = is_team(&br->b) ? 60 : 32;
        bm_fill(120 - w / 2, F_BOX_Y, w, F_BOX_H, C_PANEL);
        bm_frame(120 - w / 2, F_BOX_Y, w, F_BOX_H, C_GOLD);
    }
    vs_boxes(br, page, C_GOLD_DARK);
    draw_lines(br);
    if (!later) { names_step(u, br, ALL_SEATS); faces_step(br, ALL_SEATS); }
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
        for (int pass = 0; pass <= final_page(&br->b) + 1; pass++) {   // pass 0: last_page; then pages 0, 1, 2 ... (others)
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

// Ease in and out: a gentle start, a gentle landing (d <= 64: no overflow at 240 px)
static int ease(int a, int b, int t, int d) {
    if (t >= d) return b;
    return a + (b - a) * (t * t * (3 * d - 2 * t)) / (d * d * d);
}

// ---- the page's slide -----------------------------------------------------------------------------------------------
// The whole bitmap and the faces on it move: left half <- final <- right half is the order on the "floor", so going
// from the left half to the right half the page leaves to the left and the next arrives from the right. Nothing heavy
// runs while one moves: the register (written in the vblank, tny_ui.c slot_done) and up to 8 faces' positions; the page
// is drawn once, off screen, in the frame between.
static void pan_set(Ui *u, Br *br, int off) {
    int d = off - br->pan_off;
    if (d)
        for (int s = 0; s < ALL_SEATS; s++)
            if (br->face[s]) ui_face_move(br->face[s], ui_face_x(br->face[s]) + d, ui_face_y(br->face[s]));
    br->pan_off = (s16)off;
    u->bg1_x = (u16)(-off & 511);
}

static void start_pan(Br *br, int page, int after) {
    static const u8 place[2][5] = { { 0, 2, 1 },     // left half, right half, final: their order on the floor
                                    { 0, 1, 3, 4, 2 } };   // blocks A, B, C, D, the final four: A B, the final four, C D
    const u8 *floor = place[blocks(&br->b)];
    br->pan_to = (u8)page;
    br->pan_after = (u8)after;
    br->pan_in = 0;
    br->pan_dir = floor[page] > floor[br->page] ? -1 : 1;
    br->state = S_PAN;
    br->t = 0;
    ui_sfx(UI_SFX_SLIDE);
}

// The first look at a half or a block: whose it is
static void tour_banner(Ui *u, Br *br) {
    int p = br->b.p1_seat, q = br->b.p2_seat;
    int s = p != 0xFF && (p >> 3) == br->page ? p : q != 0xFF && (q >> 3) == br->page ? q : 0xFF;
    if (s == 0xFF) banner(u, blocks(&br->b) ? "Another block" : "The other half", 0, 0, 0);
    else banner_unit(u, br, lead_of(br, s), " starts here", " start here");
}

// ---- a result -------------------------------------------------------------------------------------------------------
static int loser_of(const Br *br, int r, int m) {
    int s0 = match_seat(&br->b, br->shown, r, m, 0), s1 = match_seat(&br->b, br->shown, r, m, 1);
    return br->b.winner[r][m] == s0 ? s1 : s0;
}

// The loser greys, his name with him
static void grey_loser(Ui *u, Br *br, int loser) {
    if (loser == 0xFF) return;
    for (int i = 0; i < (is_team(&br->b) ? 2 : 1); i++) {
        if (br->face[loser + i]) ui_face_look(br->face[loser + i], 0, UI_REMAP_GREY);
        if (!is_player(br, loser + i) && seat_on_page(br, loser + i) && br->b.seat[loser + i] != 0xFF) draw_name(u, br, loser + i, C_GREY);
    }
}

// The result is in: the winner stands where he walked to, his way here lights up
static void land_result(Br *br, int r, int m) {
    int w = br->b.winner[r][m], loser = loser_of(br, r, m);
    for (int i = 0; i < (is_team(&br->b) ? 2 : 1); i++) ui_face_look(br->face[w + i], 0, 0);
    br->shown[r][m] = (u8)w;
    draw_match(br, r, m);
    if (is_player(br, w)) draw_players(br);
    if (is_player(br, loser)) { erase_road(br, loser, r); draw_lines(br); }
}

static void walk_start(Ui *u, Br *br, int i) {
    int r = br->ev_r[br->ev + i], m = br->ev_m[br->ev + i], w = br->b.winner[r][m];
    grey_loser(u, br, loser_of(br, r, m));
    if (br->slow) ui_sfx(UI_SFX_BUZZ);
    Walk *k = &br->walk[i];
    for (int j = 0; j < 2; j++) {
        k->e[j] = 0;
        if (j && !is_team(&br->b)) break;
        Entity *e = br->face[w + j];
        if (!e) continue;
        int tx, ty;
        seat_pos(br, w + j, r + 1, &tx, &ty);
        k->e[j] = e;
        k->sx[j] = (s16)ui_face_x(e); k->sy[j] = (s16)ui_face_y(e);
        k->tx[j] = (s16)tx; k->ty[j] = (s16)ty;
    }
}

// The next events: the players' match alone, or the page's others in this round together
static void walks_begin(Br *br) {
    int r = br->ev_r[br->ev], m = br->ev_m[br->ev];
    int s0 = match_seat(&br->b, br->shown, r, m, 0), s1 = match_seat(&br->b, br->shown, r, m, 1);
    br->slow = (u8)(is_player(br, s0) || is_player(br, s1));
    br->n_walk = 1;
    if (!br->slow) {
        int pg = page_of_match(br, r, m);
        while (br->n_walk < WALKS && br->ev + br->n_walk < br->n_ev) {
            int r2 = br->ev_r[br->ev + br->n_walk], m2 = br->ev_m[br->ev + br->n_walk];
            if (r2 != r || page_of_match(br, r2, m2) != pg || players_match(br, r2, m2)) break;
            br->n_walk++;
        }
    }
    br->state = S_WALK;
    br->t = 0;
}

static void walks_step(Ui *u, Br *br) {
    int grey = br->slow ? T_GREY : T_GREY_Q, slide = br->slow ? T_SLIDE : T_SLIDE_Q;
    int hop = br->slow ? 8 : 4, t = br->t;
    for (int i = 0; i < br->n_walk; i++) {
        int t0 = i * T_GAP_Q, go = t0 + grey, end = go + slide;
        if (t == t0) walk_start(u, br, i);
        Walk *k = &br->walk[i];
        if (t == go) {
            ui_face_look(k->e[0], 1, 0);
            ui_face_look(k->e[1], 1, 0);
            ui_sfx(UI_SFX_SLIDE);
        }
        if (t > go && t <= end) {
            int d = t - go, h = hop * 4 * d * (slide - d) / (slide * slide);
            for (int j = 0; j < 2; j++)
                if (k->e[j]) ui_face_move(k->e[j], ease(k->sx[j], k->tx[j], d, slide), ease(k->sy[j], k->ty[j], d, slide) - h);
        }
        if (t == end) {
            int r = br->ev_r[br->ev + i], m = br->ev_m[br->ev + i];
            land_result(br, r, m);
            if (br->slow && r < br->b.rounds - 1) banner_unit(u, br, br->b.winner[r][m], " advances!", " advance!");
        }
    }
    br->t++;
    if (t >= (br->n_walk - 1) * T_GAP_Q + grey + slide) {
        ui_sfx(UI_SFX_OK);
        br->ev += br->n_walk;
        br->state = S_PAUSE;
        br->t = 0;
        br->wait = br->slow ? T_PAUSE : T_PAUSE_Q;
    }
}

// A: every result left is in, at once, drawn in place (nothing clears the screen); those on another page are drawn with it
static void finish_all(Ui *u, Br *br) {
    for (; br->ev < br->n_ev; br->ev++) {
        int r = br->ev_r[br->ev], m = br->ev_m[br->ev], w = br->b.winner[r][m];
        if (page_of_match(br, r, m) != br->page) { br->shown[r][m] = (u8)w; continue; }
        grey_loser(u, br, loser_of(br, r, m));
        for (int i = 0; i < (is_team(&br->b) ? 2 : 1); i++) {
            int x, y;
            seat_pos(br, w + i, r + 1, &x, &y);
            ui_face_move(br->face[w + i], x, y);
        }
        land_result(br, r, m);
    }
}

static void bracket_open(Ui *u) {
    Br *br = u->ctx;
    u->shrink = 1;                                // giants' faces at half size (tny_ui.c shrink_giants)
    int tour = br->phase == 0 && paged(&br->b) && !br->n_ev;
    if (paged(&br->b)) ui_bg1_wide();
    br->state = tour ? S_TOUR : S_START;
    int page = 0;
    if (tour) page = 0;
    else if (br->n_ev) page = page_of_match(br, br->ev_r[0], br->ev_m[0]);
    else if (br->b.round < br->b.rounds) page = page_of_match(br, br->b.round, br->b.match);
    else if (paged(&br->b)) page = final_page(&br->b);
    find_next(br);
    if (br->n_ev || tour) br->vs_a = br->vs_b = 0xFF;   // the boxes come once the results are in
    build_page(u, br, page, 0);
    if (tour) tour_banner(u, br);
}

static void bracket_step(Ui *u) {
    Br *br = u->ctx;
    ui_st.anim_frames++;
    ui_st.dbg[0] = br->state; ui_st.dbg[1] = br->ev; ui_st.dbg[2] = br->n_ev; ui_st.dbg[3] = br->page;
    if (br->state != S_WAIT && (u->keys & (KEY_A | KEY_B | KEY_START))) br->skip = 1;
    if (br->skip) {                                   // waits end now, slides land now
        switch (br->state) {
        case S_START: case S_TOUR: case S_EVENT: case S_WALK: case S_PAUSE:
            finish_all(u, br);
            br->state = S_NEXT;
            br->t = 0;
            break;
        case S_SPOT: br->t = br->wait; break;
        case S_VS: br->t = br->dur - 1; break;
        }
    }
    switch (br->state) {
    case S_START:
        if (++br->t >= T_START) { br->t = 0; br->state = br->ev < br->n_ev ? S_EVENT : S_NEXT; }
        break;
    case S_TOUR:                                   // a look at each half, left then right; at each block, in order
        if (++br->t >= (blocks(&br->b) ? T_TOUR_B : T_TOUR)) {
            if (br->page < final_page(&br->b) - 1) start_pan(br, br->page + 1, S_TOUR);
            else { br->t = 0; br->state = S_NEXT; }
        }
        break;
    case S_PAN: {
        br->t += br->skip ? 3 : 1;
        int d = T_PAN, t = br->t > d ? d : br->t, k = d - t;
        if (!br->pan_in) {                         // out, faster and faster
            if (br->t > d) {                       // gone (the frame before): the next page, off screen
                pan_set(u, br, br->pan_dir * 240);
                build_page(u, br, br->pan_to, 1);
                br->pan_off = 0;
                pan_set(u, br, -br->pan_dir * 240);
                br->pan_in = 1;
                br->t = 0;
            } else {
                pan_set(u, br, br->pan_dir * 240 * t * t / (d * d));
            }
        } else {                                   // in, slower and slower
            pan_set(u, br, -br->pan_dir * 240 * k * k / (d * d));
            faces_step(br, t >= d ? ALL_SEATS : 2);
            if (t >= 3 || t >= d) names_step(u, br, t >= d ? ALL_SEATS : 1);   // the name column: on screen last, past frame 10
            if (t >= d) {
                pan_set(u, br, 0);
                draw_page_label(u, br);
                if (br->pan_after == S_TOUR) tour_banner(u, br);
                br->state = br->pan_after;
                br->t = 0;
                br->wait = T_PAGE;
            }
        }
        break;
    }
    case S_EVENT: {
        int pg = page_of_match(br, br->ev_r[br->ev], br->ev_m[br->ev]);
        if (pg != br->page) { start_pan(br, pg, S_PAUSE); break; }
        walks_begin(br);
        walks_step(u, br);
        break;
    }
    case S_WALK:
        walks_step(u, br);
        break;
    case S_PAUSE:
        if (br->t++ >= br->wait) { br->t = 0; br->state = br->ev < br->n_ev ? S_EVENT : S_NEXT; }
        break;
    case S_NEXT: {
        find_next(br);
        int champ = champion(br);
        if (br->vs_a == 0xFF && champ == 0xFF) { br->state = S_WAIT; br->t = 0; break; }
        int pg = champ != 0xFF ? (paged(&br->b) ? final_page(&br->b) : 0) : page_of_match(br, br->b.round, br->b.match);
        if (pg != br->page) { start_pan(br, pg, S_NEXT); break; }
        // the boxes are drawn in place (polish2: a full rebuild cleared the bitmap and the screen showed one frame
        // without names or lines), and only what changes: a name is a third of a frame's work
        bar_clear(); vs_boxes(br, pg, C_GOLD_DARK); draw_lines(br); vs_names(u, br);
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
        br->dim_i = 0;
        ui_sfx(UI_SFX_OK);
        break;
    }
    case S_SPOT: {                                 // the next match lit on the tree, then the two come down to the bar
        for (int n = 0; n < 2 && br->dim_i < ALL_SEATS; br->dim_i++)   // a look re-decodes the frame: a few a frame
            if (br->face[br->dim_i]) { face_state(br, br->dim_i); n++; }
        if (br->t++ < br->wait) break;
        for (; br->dim_i < ALL_SEATS; br->dim_i++) face_state(br, br->dim_i);
        vs_boxes(br, br->page, C_CLEAR);
        draw_lines(br);
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
        if (br->mover) ui_face_move(br->mover, ease(br->sx, br->tx, br->t, br->dur), ease(br->sy, br->ty, br->t, br->dur));
        if (br->mover2) ui_face_move(br->mover2, ease(br->sx2, br->tx2, br->t, br->dur), ease(br->sy2, br->ty2, br->t, br->dur));
        for (int k = 0; k < 2; k++)
            if (br->mate[k].e) ui_face_move(br->mate[k].e, ease(br->mate[k].sx, br->mate[k].tx, br->t, br->dur), ease(br->mate[k].sy, br->mate[k].ty, br->t, br->dur));
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
    if (!src || (src->size != 8 && src->size != 16 && src->size != 32)) return;
    Br *br = Arena_Alloc(sizeof(Br));
    if (!br) { ui_st.heap_fail++; return; }
    memset(br, 0, sizeof *br);
    br->phase = phase;
    br->b = *src;
    if (br->b.rounds > 4) br->b.rounds = 4;
    // what the screen showed last time: the results to animate are the difference
    int same = ui_st.magic == UI_MAGIC && ui_st.shown_ok && same_bytes(ui_shown_seat, br->b.seat, sizeof ui_shown_seat);
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
    memcpy(ui_shown_seat, br->b.seat, sizeof ui_shown_seat);
    ui_st.shown_ok = 1;
    Arena_Free(br);
}

// ---- the Rumble board ----------------------------------------------------------------------------------------------
typedef struct Board {
    u8 ids[8], down[8];
    u8 n, shown_down, t, shown;
    u8 teams;                // two players: their team on top, the pairs below
    u8 round, rounds;
    Entity *face[8];
} Board;

// The next round's screen (Ryan, 2026-11-04: a Rumble had no real screen between rounds): which round, and with two players the three teams,
// yours on top, the two others below, every fighter arriving in turn.
#define BOARD_ARRIVE 14
static void board_pos(const Board *bd, int i, int *x, int *y) {
    if (bd->teams) {
        // bd's first two places are the players', then the pairs
        if (i < 2) { *x = i ? 150 : 90; *y = 42; return; }
        int k = i - 2;
        *x = (k < 2 ? 30 : 150) + (k & 1) * 60;
        *y = 106;
        return;
    }
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
    char head[32] = "Royal Rumble";
    if (bd->rounds) {
        char *o = head + 12;
        const char *t = bd->round + 1 >= bd->rounds ? ":  the final round" : ":  round ";
        while (*t) *o++ = *t++;
        if (bd->round + 1 < bd->rounds) *o++ = (char)('1' + bd->round);
        *o = 0;
    }
    ui_text(u, 120, 2, 1, C_GOLD, head);
    if (bd->teams) {
        ui_text(u, 120, 76, 1, C_WHITE, "VS");
        ui_text(u, 120, 112, 1, C_WHITE, "VS");
    }
    bd->shown = 0;
    bd->t = BOARD_ARRIVE;
}

// the next fighter's cell, his face and his name
static void board_arrive(Ui *u, Board *bd, int i) {
    char sb[24];
    int x, y;
    board_pos(bd, i, &x, &y);
    bm_fill(x - 24, y - 20, 48, 52, C_CELL);
    bm_frame(x - 24, y - 20, 48, 52, C_PANEL2);
    bd->face[i] = ui_face(tny_fighters[bd->ids[i]].sprite, x, y - 2);
    ui_face_look(bd->face[i], 0, 0);
    ui_text(u, x, y + 18, 1, C_WHITE, ui_short_name(tny_fighters[bd->ids[i]].name, 50, sb, sizeof sb));
}

static void board_step(Ui *u) {
    Board *bd = u->ctx;
    bd->t++;
    if (bd->shown < bd->n) {                       // they arrive in turn; A shows the rest at once
        if (bd->t < BOARD_ARRIVE && !(u->keys & (KEY_A | KEY_B | KEY_START))) return;
        bd->t = 0;
        do board_arrive(u, bd, bd->shown++); while ((u->keys & (KEY_A | KEY_B | KEY_START)) && bd->shown < bd->n);
        ui_sfx(UI_SFX_MOVE);
        return;
    }
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
    int round, rounds;
    const u8 *picks = tny_rumble_info(&round, &rounds);
    bd.round = (u8)round;
    bd.rounds = (u8)rounds;
    if (picks && n <= 4 && !down[0]) {              // two players: their team first
        bd.teams = 1;
        for (int i = 0; i < 2; i++) bd.ids[bd.n++] = picks[i];
    }
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
    if (c->n == 1 && (c->ids[0] == TF_HERCULE || c->ids[0] == TF_P_HERCULE)) {   // he never lets anyone forget it
        PUT("The people's champ does it again!");
        line[n] = 0;
        banner(u, line, 0, 0, 0);
        return;
    }
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
