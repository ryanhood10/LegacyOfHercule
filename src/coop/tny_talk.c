// The tournament's fighter dialogue (docs/tournament-dialogue.md, Ryan's approved plan). A talk is a dialogue
// sequence built in RAM: a short wait, then up to four format entries (mode 3) whose text is a line in ROM with the
// speaker's name already in it ("!GOKU: ..."), the speaker's portrait from the roster (0 = none). Chosen most
// specific first: meeting yourself and rival exchanges (tny_talk_pairs), in 2 v 2 the team-vs-team lines, a rival
// across the teams, then each side's team line or the fighters' own opener / reply. Start skips the rest: the
// sequence's later slots become stop words, so the box on screen is the last.
#include "tny_talk.h"

#define SCENE_RUNNING  (*(volatile u32 *)0x03001FDC)
#define TALK_MAGIC     0x4B4C4154u                 // "TALK"
#define TALK_MAX       4
#define MODE_FORMAT    3
#define OP_END         0x11

typedef struct { u8 head[4]; const char *fmt; u8 script[4]; } TalkEnt;   // u16 next, mode, speaker; format; args script
_Static_assert(sizeof(TalkEnt) == 12, "TalkEnt");

static struct {
    u32 magic;
    u32 action[2];
    const void *seq[TALK_MAX + 2];
    TalkEnt ent[TALK_MAX];
    u8 n, live, wait;                                // live: 1 queued (no scene yet), 2 on screen
} talk TNY_SECTION("talk");

// WaitFrames 12 before the first box (the walk-in's last step settles)
static const u8 talk_wait[] __attribute__((aligned(4))) = { 1, 0, 0, 0x00, 12, 0x02, 0x45, OP_END };

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

static void add(int id, const char *line) {
    if (!line || talk.n >= TALK_MAX) return;
    TalkEnt *e = &talk.ent[talk.n];
    e->head[2] = MODE_FORMAT;
    e->head[3] = id >= 0 && id < TF_COUNT ? tny_fighters[id].portrait : 0;
    e->fmt = line;
    e->script[0] = OP_END;
    talk.n++;
}
static int run(void) {
    if (!talk.n) return 0;
    talk.seq[0] = talk_wait;
    for (int i = 0; i < talk.n; i++) {
        u16 next = (u16)(i + 2);                    // seq[i + 1] is entry i; seq[n + 1] the end
        talk.ent[i].head[0] = (u8)next;
        talk.ent[i].head[1] = (u8)(next >> 8);
        talk.seq[i + 1] = &talk.ent[i];
    }
    talk.seq[talk.n + 1] = 0;
    coop_run_sequence(talk.action, talk.seq);
    talk.live = 1;
    talk.wait = 0;
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
static void add_pair(const TalkPair *p, int a, int b, int swap) {
    for (int k = 0; k < p->n; k++) add((p->spk[k] ^ swap) ? b : a, p->l[k]);
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

int tny_talk_pre(const u8 *opp, int nopp, const u8 *mine, int nmine) {
    start();
    int a0 = nopp > 0 && ok(opp[0]) ? opp[0] : -1, a1 = nopp > 1 && ok(opp[1]) ? opp[1] : -1;
    int b0 = nmine > 0 && ok(mine[0]) ? mine[0] : -1, b1 = nmine > 1 && ok(mine[1]) ? mine[1] : -1;
    if (a0 < 0 || b0 < 0) return 0;
    int swap;
    if (a1 >= 0 && b1 >= 0) {                       // 2 v 2
        const u8 x[2] = { (u8)a0, (u8)a1 }, y[2] = { (u8)b0, (u8)b1 };
        int who[4];
        const TalkTvt *t = tvt_of(x, y, who);
        if (t) {
            for (int k = 0; k < t->n; k++) add(who[t->spk[k]], t->l[k]);
            return run();
        }
        const int as[2] = { a0, a1 }, bs[2] = { b0, b1 };
        for (int i = 0; i < 2; i++)                 // a rival across the ring
            for (int j = 0; j < 2; j++) {
                const TalkPair *p = pair_of(tny_talk_pairs, as[i], bs[j], &swap);
                if (p) { add_pair(p, as[i], bs[j], swap); return run(); }
            }
        const TalkPair *ta = pair_of(tny_talk_teams, a0, a1, &swap);
        if (ta) add_pair(ta, a0, a1, swap);
        else add(a0, voice(a0, (g_FrameCounter & 1) ? TALK_OPEN2 : TALK_OPEN1));
        const TalkPair *tb = pair_of(tny_talk_teams, b0, b1, &swap);
        if (tb) add_pair(tb, b0, b1, swap);
        else add(b0, voice(b0, TALK_REPLY));
        return run();
    }
    const TalkPair *p = pair_of(tny_talk_pairs, a0, b0, &swap);
    if (p) add_pair(p, a0, b0, swap);
    else {
        add(a0, voice(a0, (g_FrameCounter & 1) ? TALK_OPEN2 : TALK_OPEN1));
        add(b0, voice(b0, TALK_REPLY));
    }
    return run();
}

int tny_talk_win(int id) {
    start();
    add(id, voice(id, TALK_WIN));
    return run();
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
    // the box on screen is the last: the game read its `next` when it opened, so the sequence's later slots become
    // stop words (the open box's entry was read already)
    for (int i = 2; i <= talk.n; i++) talk.seq[i] = 0;
}

void tny_talk_test(u32 cmd, u32 arg, u32 arg2) {
    (void)cmd;
    if (arg & 0x80000000u) { tny_talk_win((int)(arg & 0xFF)); return; }
    const u8 a[2] = { (u8)arg, (u8)(arg >> 8) }, b[2] = { (u8)arg2, (u8)(arg2 >> 8) };
    tny_talk_pre(a, a[1] == 0xFF ? 1 : 2, b, b[1] == 0xFF ? 1 : 2);
}
