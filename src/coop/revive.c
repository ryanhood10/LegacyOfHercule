// Reviving a downed partner (revive.h, docs/coop-design.md "Revive").
//
// Going down: a lethal hit on either player (both take-damage hooks ask revive_hit first) plays the stock
// death state (message 0x21, the collapse player 1's own death plays) with HP at 0 but queues no game over.
// The body takes no more hits, enemies turn to the partner (entities.c / coop_target), and a downed player's
// pad does nothing. Player 2 keeps playing while player 1 lies down (p1_controllable, player.c).
// Getting up: the partner stands within REVIVE_REACH of the body and holds A for REVIVE_HOLD frames (the A
// is taken from the game, so it does not punch); letting go or taking a hit starts it over. Then HP is
// REVIVE_HP_PCT of the maximum, message 0 (idle) and REVIVE_SAFE frames without damage.
// Game over: the countdown (diff_revive_frames) runs out, or the partner goes down too: player 1 gets the
// stock ending (the death state and the game-over event the stock take-damage queues, 200 frames).
// Not here: duels (duel.c leaves 1 HP), the Cell Games' solo fights (one player), no player 2 (stock death).
// A scene that starts while someone is down (a boss dies, a trigger) gets them up first; the countdown never
// runs during a scene (0x03001FDC: set while a dialogue sequence runs, 0x0800B4AE / 0x0800B4FC).
// Bars: drawn from the HUD's draw hook above the body, 8bpp 32 x 8 sprites in the game's one OBJ palette:
// the countdown (red, seconds in white, blinking for the last 3 s) and, while the partner revives, the
// revive bar (yellow). The body flashes bright while the revive runs (entity +0x24 colour remap).
#include "revive.h"
#include "difficulty.h"
#include "wide.h"
#include "tny_engine.h"

ReviveState rv __attribute__((section(".party.rv"), used));

#define OBJ_PALETTE     ((volatile const u16 *)0x05000200)
#define SCENE_RUNNING   (*(volatile u32 *)0x03001FDC)
#define DEATH_MSG       0x21
#define GAME_OVER_EVENT 0x08023A50u            // the event the stock take-damage queues (200 frames)
#define SFX_DOWN        0x22                   // an impact
#define SFX_UP          55                     // the power-up the transformations use
#define C_BLACK 1
#define C_WHITE 2
#define C_RED   5
#define C_YELLOW 8
#define C_EMPTY 14

ROM_FN(void, Player_Knockback, (Entity *e, void *hit), 0x0800F833);

static void init(void) {
    if (rv.magic == REVIVE_MAGIC) return;
    memset(&rv, 0, sizeof rv);
    rv.magic = REVIVE_MAGIC;
}

static Entity *player(int k) { return k ? coop.p2 : g_PlayerObject; }
static CharacterEntry *entry(int k) { return &g_Party->chars[k ? coop.p2_slot : g_Party->active_slot]; }
static void sfx(int id) { Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + id * 12, 0x40, 0); }
static void set_tint(Entity *e, const u8 *t) {
    *(const u8 **)((u8 *)e + 0x24) = t;
    *(u32 *)((u8 *)e + 0x20) = 0;              // drop the cached frame so the next one goes through the remap
}

static int allowed(void) {
    return coop.p2 && g_PlayerObject && !coop2.duel_active && !coop_solo() && !companion_active();
}

int revive_is_down(const Entity *e) {
    return e && rv.magic == REVIVE_MAGIC && (rv.body[0] == e || rv.body[1] == e);
}

int revive_p1_down(void) {
    return rv.magic == REVIVE_MAGIC && rv.body[0] && rv.body[0] == g_PlayerObject && !SCENE_RUNNING && !rv.over;
}

static void game_over(void) {
    Entity *p1 = g_PlayerObject;
    if (rv.over || !p1) return;
    rv.over = 1;
    rv.timeouts++;
    entry(0)->hp = 0;
    if (rv.body[0] != p1) {                        // standing: it falls the stock way
        *(u32 *)((u8 *)p1 + 0x168) = 0;
        Entity_SetAnimation(p1, DEATH_MSG);
    }
    u32 *ev = Arena_Alloc(8);
    if (!ev) return;
    ev[0] = GAME_OVER_EVENT;
    ev[1] = 200;
    EntityList *l = g_EntityList;
    l->events[l->event_count++] = ev;
}

static void go_down(int k, Entity *e, u8 *hit) {
    if (hit) Player_Knockback(e, hit);
    entry(k)->hp = 0;
    *(u32 *)((u8 *)e + 0x168) = 0;
    Entity_SetAnimation(e, DEATH_MSG);
    rv.body[k] = e;
    rv.left[k] = rv.total[k] = (u16)diff_revive_frames();
    rv.hold[k] = 0;
    rv.glow_n = 0;                                 // the palette may have changed since the last one
    if (!rv.glow) rv.glow = Arena_Alloc(256);
    rv.downs++;
    sfx(SFX_DOWN);
}

static void get_up(int k) {
    Entity *e = rv.body[k];
    CharacterEntry *c = entry(k);
    int hp = c->max_hp * REVIVE_HP_PCT / 100;
    c->hp = (u16)(hp > 0 ? hp : 1);
    rv.body[k] = 0;
    rv.hold[k] = 0;
    rv.safe[k] = REVIVE_SAFE;
    if (e) {
        set_tint(e, 0);
        Entity_SetAnimation(e, 0);
    }
    if (rv.glow && !rv.body[!k]) { Arena_Free(rv.glow); rv.glow = 0; }
    rv.revives++;
    sfx(SFX_UP);
}

int revive_hit(Entity *e, u8 *hit, int k) {
    init();
    if (rv.body[k] == e) return 1;                 // the body takes no more hits
    if (rv.safe[k] || rv.over) return rv.safe[k] != 0;
    s32 dmg = *(s32 *)(hit + 8);
    if (dmg <= 0) return 0;
    rv.hold[!k] = 0;                               // a hit on the partner starts its revive over
    if (!allowed()) return 0;
    if (dmg < entry(k)->hp) return 0;
    if (rv.body[!k]) {                             // the partner is already down: game over
        if (k == 0) { rv.over = 1; rv.timeouts++; return 0; }   // player 1's own stock death does it
        Player_Knockback(e, hit);
        entry(1)->hp = 0;
        game_over();
        return 1;
    }
    go_down(k, e, hit);
    return 1;
}

int scene_running(void) { return SCENE_RUNNING != 0; }

void revive_p2_gone(void) {
    if (rv.magic != REVIVE_MAGIC) return;
    if (rv.body[0] && !rv.over) game_over();      // nobody left to revive player 1
    if (rv.body[1]) {
        set_tint(rv.body[1], 0);
        rv.body[1] = 0;
        rv.hold[1] = 0;
        CharacterEntry *c = entry(1);
        if (!c->hp) c->hp = (u16)(c->max_hp * REVIVE_HP_PCT / 100 ? c->max_hp * REVIVE_HP_PCT / 100 : 1);
    }
    rv.hold[0] = 0;
    if (!rv.body[0] && rv.glow) { Arena_Free(rv.glow); rv.glow = 0; }
}

// Player 1 takes player 2's character at a save pad (player.c coop_active_check) while player 2 is down: it gets up
// first, so the slot changing hands is not one at 0 HP.
void revive_p2_up(void) {
    if (rv.magic == REVIVE_MAGIC && rv.body[1]) get_up(1);
}

void revive_test_down(int k) {
    init();
    Entity *e = player(k);
    if (e && allowed() && !rv.body[k] && !rv.body[!k] && !rv.over) go_down(k, e, 0);
}

static s32 dist2(const Entity *a, const Entity *b) {
    s32 dx = (a->x - b->x) >> 8, dy = (a->y - b->y) >> 8;
    return dx * dx + dy * dy;
}

void revive_input(u16 *p1, u16 *p2) {
    init();
    u16 *pad[2] = { p1, p2 };
    for (int k = 0; k < 2; k++) {
        rv.reaching[k] = rv.a_held[k] = 0;
        if (rv.body[k]) { *pad[k] = 0; continue; }             // a downed player's pad does nothing
        Entity *me = player(k), *body = rv.body[!k];
        if (!me || !body || rv.over || SCENE_RUNNING) continue;
        if (k == 0 && !p1_in_play()) continue;               // a menu (pause, save pad): its A stays its own
        if (dist2(me, body) > REVIVE_REACH * REVIVE_REACH) continue;
        rv.reaching[k] = 1;
        if (*pad[k] & KEY_A) {
            rv.a_held[k] = 1;
            *pad[k] &= (u16)~KEY_A;                            // reviving, not punching
        }
    }
}

// glow: a remap toward white, a slice per frame (a whole table at once would stall a couple of frames)
static void build_glow(void) {
    u8 *glow = rv.glow;
    if (!glow) { rv.glow_n = 0; return; }
    for (int n = 0; n < 16 && rv.glow_n < 256; n++, rv.glow_n++) {
        int i = rv.glow_n;
        if (i == 0) { glow[0] = 0; continue; }
        u16 c = OBJ_PALETTE[i];
        int r = ((c & 31) + 31 * 2) / 3, g = (((c >> 5) & 31) + 31 * 2) / 3, b = (((c >> 10) & 31) + 31 * 2) / 3;
        int best = i, bestd = 0x7FFFFFFF;
        for (int j = 1; j < 256; j++) {
            u16 d = OBJ_PALETTE[j];
            int dr = (d & 31) - r, dg = ((d >> 5) & 31) - g, db = ((d >> 10) & 31) - b;
            int dd = dr * dr + dg * dg + db * db;
            if (dd < bestd) { bestd = dd; best = j; }
        }
        glow[i] = (u8)best;
    }
}

void revive_tick(void) {
    init();
    if (rv.over && !rv.body[0] && !rv.body[1] && entry(0)->hp) rv.over = 0;   // a save loaded after the game over
    for (int k = 0; k < 2; k++) {
        if (rv.safe[k]) rv.safe[k]--;
        Entity *e = rv.body[k];
        if (!e) continue;
        if (player(k) != e) {                      // the body is gone (a map change, player 2 despawned)
            rv.body[k] = 0;
            rv.hold[k] = 0;
            rv.glow = 0;                           // the map's arena went with it
            CharacterEntry *c = entry(k);
            if (!c->hp) c->hp = (u16)(c->max_hp * REVIVE_HP_PCT / 100 ? c->max_hp * REVIVE_HP_PCT / 100 : 1);
            continue;
        }
        if (rv.over) continue;                     // the game over is queued: they stay down
        if (k == 0 && companion_active()) { game_over(); continue; }   // player 2 left for a companion: nobody revives
        if (SCENE_RUNNING || !allowed()) { get_up(k); continue; }   // a scene (or a solo fight) begins
        if (rv.glow_n < 256 && rv.glow) build_glow();
        if (rv.a_held[!k] && rv.reaching[!k] && !rv.body[!k]) {
            if (++rv.hold[k] >= REVIVE_HOLD) { get_up(k); continue; }
        } else {
            rv.hold[k] = 0;
        }
        set_tint(e, rv.hold[k] && rv.glow && rv.glow_n >= 256 && ((rv.hold[k] >> 3) & 1) ? rv.glow : 0);
        if (rv.left[k]) rv.left[k]--;
        if (!rv.left[k] && !tny_ring_active()) game_over();   // in the tournament's ring he stays down: out of the fight
    }
}

// ---- the bars -------------------------------------------------------------------------------------
// 8bpp tiles, 1D mapping: a 32 x 8 sprite is four 64-byte tiles side by side.
static const u8 DIGITS[10][5] = {   // 3 x 5, bit 2 = left column
    { 7, 5, 5, 5, 7 }, { 2, 6, 2, 2, 7 }, { 7, 1, 7, 4, 7 }, { 7, 1, 7, 1, 7 }, { 5, 5, 7, 1, 1 },
    { 7, 4, 7, 1, 7 }, { 7, 4, 7, 5, 7 }, { 7, 1, 1, 1, 1 }, { 7, 5, 7, 5, 7 }, { 7, 5, 7, 1, 7 },
};

static void put(u8 *t, int x, int y, u8 c) { t[(x >> 3) * 64 + y * 8 + (x & 7)] = c; }

static void bar(u8 *t, int filled, u8 colour) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 32; x++)
            put(t, x, y, (y == 0 || y == 7 || x == 0 || x == 31) ? C_BLACK : (x - 1 < filled ? colour : C_EMPTY));
}

static void number(u8 *t, int n) {
    int d[2], nd = 0;
    if (n >= 10) d[nd++] = n / 10 % 10;
    d[nd++] = n % 10;
    int x0 = 16 - (nd * 4 - 1) / 2;
    for (int i = 0; i < nd; i++)
        for (int y = 0; y < 5; y++)
            for (int x = 0; x < 3; x++)
                if (DIGITS[d[i]][y] & (4 >> x)) put(t, x0 + i * 4 + x, 1 + y, C_WHITE);
}

static void emit(u32 **oam, int x, int y, u16 tile) {
    if (x <= -32 || x >= wide_w() || y <= -8 || y >= 160) return;
    u32 *o = *oam;
    u32 xo = (((u32)x & 0x1FF) << 16) | ((((u32)x >> 9) & 1) << 25);
    o[0] = 0x40006000u | xo | ((u32)y & 0xFF);       // 8bpp, horizontal 32 x 8
    o[1] = (o[1] & 0xFFFF0000u) | tile;              // priority 0
    *oam = o + 2;
}

void revive_draw(u32 **oam) {
    if (rv.magic != REVIVE_MAGIC) return;
    int k = rv.body[0] ? 0 : rv.body[1] ? 1 : -1;
    if (k < 0 || rv.over || SCENE_RUNNING) {
        if (rv.tile && k < 0) { ObjVram_Free(g_ObjVramList, rv.tile); rv.tile = 0; }
        return;
    }
    if (!rv.tile) rv.tile = ObjVram_Alloc(g_ObjVramList, 0x200);
    if (!rv.tile) return;
    u8 buf[0x200];
    int secs = (rv.left[k] + 59) / 60;
    int blink = secs <= 3 && ((rv.left[k] >> 3) & 1);
    bar(buf, rv.total[k] ? 30 * rv.left[k] / rv.total[k] : 0, blink ? C_WHITE : C_RED);
    number(buf, secs);
    bar(buf + 0x100, 30 * rv.hold[k] / REVIVE_HOLD, C_YELLOW);
    u8 *vram = (u8 *)(OBJ_VRAM + rv.tile * 0x20);
    for (int i = 0; i < 0x200; i += 4) *(u32 *)(vram + i) = *(u32 *)(buf + i);

    const s32 *cam = CAMERA_CELL;
    const Entity *e = rv.body[k];
    int x = (e->x >> 8) + 8 - 16 - cam[7];
    int y = (e->y >> 8) - 40 - cam[8];
    emit(oam, x, y, rv.tile);
    if (rv.hold[k]) emit(oam, x, y + 10, (u16)(rv.tile + 8));
}

void revive_hud_gone(void) {
    if (rv.magic == REVIVE_MAGIC && rv.tile) { ObjVram_Free(g_ObjVramList, rv.tile); rv.tile = 0; }
}
