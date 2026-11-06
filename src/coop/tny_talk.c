// The tournament's fighter dialogue (docs/tournament-dialogue.md, Ryan's approved plan). A talk is a dialogue
// sequence built in RAM: a short wait, then up to four format entries (mode 3) whose text is a line in ROM with the
// speaker's name already in it ("!GOKU: ..."), the speaker's portrait from the roster (0 = none). Chosen most
// specific first: meeting yourself and rival exchanges (tny_talk_pairs), in 2 v 2 the team-vs-team lines, a rival
// across the teams, then each side's team line or the fighters' own opener / reply. Start skips the rest: the
// sequence's later slots become stop words, so the box on screen is the last.
//
// The ring's staging per line (docs/tournament.md 27): every line of a fight's talk is reached through one shared ROM
// script entry (`line_hook`, sequence slot 1) that calls CoopCall 24 (tny_talk_call): the line's speaker and whom he
// speaks to go to the engine (tny_stage_line: the others on the floor turn to the speaker, he turns to whom he
// addresses and strikes his pose), and the call pushes the slot of that line, so the sequence needs no more slots than
// before (7) and no frame is added: the hook runs between two boxes. Each line's seats are kept in the spare bytes of
// its entry's args script (after its END): a seat is side << 4 | index, side 0 the opening side, 1 the answering one.
#include "tny_talk.h"

#define SCENE_RUNNING  (*(volatile u32 *)0x03001FDC)
#define TALK_MAGIC     0x4B4C4154u                 // "TALK"
#define TALK_MAX       4
#define MODE_FORMAT    3
#define OP_END         0x11

typedef struct { u8 head[4]; const char *fmt; u8 script[4]; } TalkEnt;   // u16 next, mode, speaker; format; args script
_Static_assert(sizeof(TalkEnt) == 12, "TalkEnt");
#define ENT_WHO(e)     ((e)->script[1])              // the line's speaker's seat (after the args script's END)
#define ENT_TO(e)      ((e)->script[2])              // whom he speaks to
#define SEAT_OPP       0x00                          // seats: side << 4 | index (the talk's opening side is 0)
#define SEAT_MINE      0x10

static struct {
    u32 magic;
    u32 action[2];
    const void *seq[TALK_MAX + 3];
    TalkEnt ent[TALK_MAX];
    u8 n, live, wait;                                // live: 1 queued (no scene yet), 2 on screen
    u8 line;                                         // the next line the hook stages (talk.n: the talk is over)
} talk TNY_SECTION("talk");
_Static_assert(sizeof(talk) <= 0x5C, ".tny.talk keeps its size (tourneyram is full)");

// WaitFrames 12 before the first box (the walk-in's last step settles)
static const u8 talk_wait[] __attribute__((aligned(4))) = { 1, 0, 0, 0x00, 12, 0x02, 0x45, OP_END };
// The line hook (slot 1): CoopCall 24, 0, 0, 0 pushes the slot of the next line, which overrides the head's next (6, the
// last slot, always the end: what a build without the call does)
#define HOOK_SLOT      1
#define END_SLOT       (TALK_MAX + 2)
static const u8 line_hook[] __attribute__((aligned(4))) = {
    END_SLOT, 0, 0, 0x00, 24, 0x00, 0, 0x00, 0, 0x00, 0, 0x02, 59, OP_END,
};

// The same voice: your party's Hercule and 16 speak as the opponents' ones; Super Saiyan 2 Gohan meets his rivals
// as Gohan (his own lines are his)
static int canon(int id) {
    if (id == TF_P_HERCULE) return TF_HERCULE;
    if (id == TF_P_A16) return TF_A16;
    if (id == TF_SSJ2_GOHAN) return TF_GOHAN;
    return id;
}
static const char *voice(int id, int k) {
    if (id < 0 || id >= TF_COUNT) return 0;
    const char *l = tny_talk_voice[id].l[k];
    return l ? l : tny_talk_voice[canon(id)].l[k];
}
static int same(int table_id, int id, int loose) { return table_id == id || (loose && table_id == canon(id)); }

static void add(int seat, int id, const char *line) {
    if (!line || talk.n >= TALK_MAX) return;
    TalkEnt *e = &talk.ent[talk.n];
    e->head[2] = MODE_FORMAT;
    e->head[3] = id >= 0 && id < TF_COUNT ? tny_fighters[id].portrait : 0;
    e->fmt = line;
    e->script[0] = OP_END;
    ENT_WHO(e) = (u8)seat;
    talk.n++;
}
// Whom each line is said to: the next speaker when he is someone else, else the one before, else the other side's first
static void aim(void) {
    for (int i = 0; i < talk.n; i++) {
        int who = ENT_WHO(&talk.ent[i]), to = (who ^ SEAT_MINE) & 0xF0;
        if (i + 1 < talk.n && ENT_WHO(&talk.ent[i + 1]) != who) to = ENT_WHO(&talk.ent[i + 1]);
        else if (i > 0 && ENT_WHO(&talk.ent[i - 1]) != who) to = ENT_WHO(&talk.ent[i - 1]);
        ENT_TO(&talk.ent[i]) = (u8)to;
    }
}
// [0] the wait, [1] the line hook, [2 .. n + 1] the lines (each goes back to the hook), then the end
static int run(void) {
    if (!talk.n) return 0;
    aim();
    talk.seq[0] = talk_wait;
    talk.seq[HOOK_SLOT] = line_hook;
    for (int i = 0; i < talk.n; i++) {
        talk.ent[i].head[0] = HOOK_SLOT;
        talk.ent[i].head[1] = 0;
        talk.seq[i + 2] = &talk.ent[i];
    }
    for (int i = talk.n + 2; i < TALK_MAX + 3; i++) talk.seq[i] = 0;
    talk.line = 0;
    coop_run_sequence(talk.action, talk.seq);
    talk.live = 1;
    talk.wait = 0;
    return 1;
}
// CoopCall 24 (the line hook): stage line `talk.line` and push its slot; past the last line (or after Start) the end
int tny_talk_call(int fn, s32 a, s32 b, s32 c, s32 *push) {
    (void)fn; (void)a; (void)b; (void)c;
    *push = END_SLOT;
    if (talk.magic != TALK_MAGIC || talk.line >= talk.n) return 1;
    TalkEnt *e = &talk.ent[talk.line];
    // the box opens at the top ("!": tools/tnytalk.py), unless the speaker stands high in the view: then at the bottom
    if (tny_stage_line(ENT_WHO(e), ENT_TO(e)) && e->fmt && e->fmt[0] == '!') e->fmt++;
    *push = 2 + talk.line++;
    return 1;
}
static void start(void) {
    talk.magic = TALK_MAGIC;
    talk.n = 0;
}

// A pair's exchange for a and b (either order): exact ids first, then the shared voices
static const TalkPair *pair_of(const TalkPair *tab, int a, int b, int *swap) {
    for (int loose = 0; loose < 2; loose++)
        for (const TalkPair *p = tab; p->id[0] != 0xFF; p++) {
            if (same(p->id[0], a, loose) && same(p->id[1], b, loose)) { *swap = 0; return p; }
            if (same(p->id[0], b, loose) && same(p->id[1], a, loose)) { *swap = 1; return p; }
        }
    return 0;
}
static void add_pair(const TalkPair *p, int a, int b, int swap, int sa, int sb) {
    for (int k = 0; k < p->n; k++) { int w = p->spk[k] ^ swap; add(w ? sb : sa, w ? b : a, p->l[k]); }
}

static int ok(int id) { return id >= 0 && id < TF_COUNT; }

// Two teams with history: the ids of each side in either order, the sides either way round
static const TalkTvt *tvt_of(const u8 *x, const u8 *y, int *who) {
    for (int loose = 0; loose < 2; loose++)
        for (const TalkTvt *t = tny_talk_tvt; t->id[0] != 0xFF; t++)
            for (int flip = 0; flip < 2; flip++) {
                const u8 *s0 = flip ? y : x, *s1 = flip ? x : y;   // s0 against t->id[0..1], s1 against t->id[2..3]
                for (int o0 = 0; o0 < 2; o0++)
                    for (int o1 = 0; o1 < 2; o1++)
                        if (same(t->id[0], s0[o0], loose) && same(t->id[1], s0[o0 ^ 1], loose) &&
                            same(t->id[2], s1[o1], loose) && same(t->id[3], s1[o1 ^ 1], loose)) {
                            who[0] = s0[o0]; who[1] = s0[o0 ^ 1]; who[2] = s1[o1]; who[3] = s1[o1 ^ 1];
                            return t;
                        }
            }
    return 0;
}

// One side's default lines (no rival across, no team-vs-team history): a known team's own lines, else each member's (the
// opening side its openers, one of each variant so two members never say the same, the answering side its replies; a
// member twice, the same fighter on both pads, speaks once). `max` the boxes this side may take.
static int side_lines(const int *ids, int n, const int *seat, int opening, int max) {
    int swap;
    const TalkPair *t = n == 2 ? pair_of(tny_talk_teams, ids[0], ids[1], &swap) : 0;
    if (t && t->n <= max) {
        add_pair(t, ids[0], ids[1], swap, seat[0], seat[1]);
        return t->n;
    }
    int alt = (int)(g_FrameCounter & 1), k = 0;
    for (int i = 0; i < n && k < max; i++) {
        if (i && canon(ids[i]) == canon(ids[0])) break;
        add(seat[i], ids[i], voice(ids[i], opening ? ((alt ^ i) ? TALK_OPEN2 : TALK_OPEN1) : TALK_REPLY));
        k++;
    }
    return k;
}

// Before a fight (docs/tournament-dialogue.md 2): meeting yourself or a rival exchange (any fighter of one side against any of
// the other's), then in 2 v 2 two teams with history; else each side's own lines. In 2 v 2 every member has a voice (Ryan,
// 2026-11-05: "does it work WELL for 2v2 scenarios too??"): around a rival exchange the other side's partner opens and the
// players' partner has the last word when the four boxes allow, and without one both members of each side speak (a known
// team its two lines, an answer to the other team's). At most TALK_MAX boxes; the players' side always answers last.
int tny_talk_pre(const u8 *opp, int nopp, const u8 *mine, int nmine) {
    start();
    int as[2] = { -1, -1 }, bs[2] = { -1, -1 }, na = 0, nb = 0;
    for (int i = 0; i < nopp && i < 2; i++) if (ok(opp[i])) as[na++] = opp[i];
    for (int i = 0; i < nmine && i < 2; i++) if (ok(mine[i])) bs[nb++] = mine[i];
    if (!na || !nb) return 0;
    // the seats keep the floor's places (side << 4 | index): a fighter's index on his side as the engine passed it
    int sa[2] = { SEAT_OPP, SEAT_OPP + 1 }, sb[2] = { SEAT_MINE, SEAT_MINE + 1 };
    if (nopp > 1 && !ok(opp[0])) sa[0] = SEAT_OPP + 1;
    if (nmine > 1 && !ok(mine[0])) sb[0] = SEAT_MINE + 1;
    int swap;
    if (na == 2 && nb == 2) {                       // two teams with history
        const u8 x[2] = { (u8)as[0], (u8)as[1] }, y[2] = { (u8)bs[0], (u8)bs[1] };
        int who[4];
        const TalkTvt *t = tvt_of(x, y, who);
        if (t) {
            for (int k = 0; k < t->n; k++) {
                int id = who[t->spk[k]];
                add(id == as[0] ? sa[0] : id == as[1] ? sa[1] : id == bs[0] ? sb[0] : sb[1], id, t->l[k]);
            }
            return run();
        }
    }
    for (int i = 0; i < na; i++)                    // a rival across the ring (meeting yourself first: the table's order)
        for (int j = 0; j < nb; j++) {
            const TalkPair *p = pair_of(tny_talk_pairs, as[i], bs[j], &swap);
            if (!p) continue;
            int oa = na == 2 && canon(as[i ^ 1]) != canon(as[i]) ? as[i ^ 1] : -1;   // the partners not in the exchange
            int ob = nb == 2 && canon(bs[j ^ 1]) != canon(bs[j]) ? bs[j ^ 1] : -1;
            int room = TALK_MAX - p->n;
            if (ob >= 0 && room > 0) room--; else ob = -1;
            if (oa >= 0 && room > 0) add(sa[i ^ 1], oa, voice(oa, (g_FrameCounter & 1) ? TALK_OPEN2 : TALK_OPEN1));
            add_pair(p, as[i], bs[j], swap, sa[i], sb[j]);
            if (ob >= 0) add(sb[j ^ 1], ob, voice(ob, TALK_REPLY));
            return run();
        }
    // each side's own: the opening side first (a box always left for the answer), then the answer in the boxes left
    side_lines(as, na, sa, 1, TALK_MAX - 1);
    side_lines(bs, nb, sb, 0, TALK_MAX - talk.n);
    return run();
}

// The winner's line: `seat` his place on the floor (the answering side's first, the players, unless the other side won)
static int talk_win_at(int seat, int id) {
    start();
    add(seat, id, voice(id, TALK_WIN));
    return run();
}
int tny_talk_win(int id) { return talk_win_at(SEAT_MINE, id); }

// The Time Chamber's room scenes (tny_talk.h): seq[0] first, seq[1..n] the lines, seq[n + 1] last (its next is 6), seq[6] the
// stop word it reaches
int tny_talk_scene(const void *first, const u8 *faces, const char *const *lines, int n, const void *last) {
    start();
    for (int i = 0; i < n && i < TALK_MAX; i++) {
        if (!lines[i]) continue;
        TalkEnt *e = &talk.ent[talk.n];
        e->head[2] = MODE_FORMAT;
        e->head[3] = faces[i];
        e->fmt = lines[i];
        e->script[0] = OP_END;
        talk.n++;
    }
    if (!talk.n && !first && !last) return 0;
    talk.seq[0] = first ? first : talk_wait;
    for (int i = 0; i < talk.n; i++) {
        talk.ent[i].head[0] = (u8)(i + 2);
        talk.ent[i].head[1] = 0;
        talk.seq[i + 1] = &talk.ent[i];
    }
    for (int i = talk.n + 1; i < TALK_MAX + 3; i++) talk.seq[i] = 0;
    talk.seq[talk.n + 1] = last;
    coop_run_sequence(talk.action, talk.seq);
    talk.live = 0;                                  // no skip: `last` must run
    return 1;
}

void tny_talk_frame(void) {
    if (talk.magic != TALK_MAGIC || !talk.live) return;
    if (talk.live == 1) {                           // queued: its scene starts within a few frames
        if (SCENE_RUNNING) talk.live = 2;
        else if (++talk.wait > 60) talk.live = 0;
        return;
    }
    if (!SCENE_RUNNING) { talk.live = 0; return; }
    if (!(g_InputSystem->pressed & KEY_START)) return;
    // the box on screen is the last: its `next` is the line hook, which now ends the talk
    talk.line = talk.n;
}

void tny_talk_test(u32 cmd, u32 arg, u32 arg2) {
    (void)cmd;
    if (arg & 0x80000000u) { tny_talk_win((int)(arg & 0xFF)); return; }
    const u8 a[2] = { (u8)arg, (u8)(arg >> 8) }, b[2] = { (u8)arg2, (u8)(arg2 >> 8) };
    tny_talk_pre(a, a[1] == 0xFF ? 1 : 2, b, b[1] == 0xFF ? 1 : 2);
}

// The engine's talk seat (tny_engine.c, docs/tournament.md 23): before a fight the other side opens and the players'
// fighters answer; after it the winner speaks. The Rumble has the announcer only (the engine's own calls).
int tny_talk_hook(const TTalkSeat *s) {
    if (s->kind == TALK_RUMBLE) return 0;
    if (s->when == TALK_BEFORE)
        return tny_talk_pre(s->opp, s->nopp > 2 ? 2 : s->nopp, s->mine, s->nmine > 2 ? 2 : s->nmine);
    int opp_won = s->kind == TALK_FINAL ? s->result == 2 : s->result != 1;
    return talk_win_at(opp_won ? SEAT_OPP : SEAT_MINE, opp_won ? s->opp[0] : s->mine[0]);
}
