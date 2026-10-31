// The team combos' meter (docs/tournament.md 24.10 the design decided with Ryan, 25.1 what was built; contract
// tny_combo.h). Owns: the shared-hit tracking, the players' bar under both HP panels, full at a fight's start, the
// 90-second cooldown, the button, the AI pairs' looser trigger and tny_caption (a line at the bottom that never
// pauses the fight).
//
// The players' bar (a co-op fight whose two picks are a listed pair, both on the floor):
//   READY  full at the start of every fight (each Rumble round is a fight); flashes yellow / white; Select on either pad
//          fires it: tny_combo_start(combo, lead, partner, target, 0). The target is the opponent they last hit
//          together (alive), else the last one either hit, else the kit fighter nearest the presser. If the runner
//          refuses (0) the bar stays full.
//   COOL   hidden for COOLDOWN frames, counted only while a fight runs, no combo plays and no scene holds the fight.
//   FILL   hidden while empty; a shared hit (one player hits the opponent the other hit within SHARE_WINDOW frames)
//          adds P_STEP; DRAIN_DELAY frames without one (or a split: they hit different opponents) and it drains by
//          DRAIN a frame; full -> READY.
// The AI pairs (two living kit fighters whose ids are a listed pair; greedy, up to SLOTS a fight): an undrawn bar
// that starts empty, fills AI_STEP from either one's hits on either player, never drains, and when full fires by
// itself as soon as both are within AI_REACH px of one player (no tell): tny_combo_start(..., player, 1). Refused:
// retried AI_RETRY frames later. Fired: the same cooldown.
// The button is Select, not R: player 1's R opens the stock area map in the field (headless, 2026-10-31; his
// control states never read it: idle / walk / diagonal are ours in player.c, A punches and B uses the ability, the
// stock dash, charge and attack states 0x080083C3 / 0x08008277 test A, B and the D-pad, the HUD's update 0x08008B89
// cycles the ability on L; the map is opened elsewhere). Every button has a field use (L the ability, Select the
// scouter, Start the pause menu; player 2's L his ability, Select his character), but the ring already takes Start
// and Select from both pads during a fight (tny_engine.c's lock), so Select is the one button free there.
#include "tny_combo.h"
#include "tny_engine.h"
#include "revive.h"
#include "wide.h"
#include "tny_ui.h"

#define SCENE_RUNNING   (*(volatile u32 *)0x03001FDC)
#define METER_MAGIC     0x524D4354u            // "TCMR"
#define CAP_MAGIC       0x50414354u            // "TCAP"

// ---- the numbers (25.1) ---------------------------------------------------------------------------------------------
#define FULL            960     // a bar's units
#define P_STEP          88      // a shared hit: 11 fill the players' bar (each player lands ~6 on one target)
#define AI_STEP         96      // an AI pair's hit on either player: 10 fill it
#define SHARE_WINDOW    90      // frames: the partner's last hit on the same opponent at most 1.5 s ago
#define DRAIN_DELAY     60      // frames without a shared hit before the bar drains
#define DRAIN           3       // units a frame: a full bar empties in about 5 s
#define COOLDOWN        5400    // 90 s
#define AI_REACH        80      // px: both of an AI pair this close to one player
#define AI_RETRY        30      // frames before a refused AI combo is tried again
#define SLOTS           3       // AI pairs tracked a fight
#define KITS_MAX        10
#define CAP_FRAMES      90
#define CAP_MAX         3

// colours: the game's one OBJ palette (bank 0): 1 black, 2 white, 6 cyan, 8 yellow, 14 dark grey
#define C_BLACK 1
#define C_WHITE 2
#define C_CYAN  6
#define C_YEL   8
#define C_EMPTY 14

enum { PS_READY, PS_COOL, PS_FILL };           // the players' bar
enum { AS_FREE, AS_FILL, AS_READY, AS_COOL };   // an AI pair's bar

typedef struct ASlot {      // 0x10
    u8 ida, idb, combo, state;
    s16 fill;
    u16 cool;               // AS_COOL: the cooldown; AS_READY: the retry delay
    u16 tries, starts;      // tny_combo_start calls / the ones that returned 1
    u16 hits;               // the pair's hits on the players
    u16 _p;
} ASlot;

// .combo.meter: fixed offsets (docs/ids.md test command 32; tests peek them at 0x0203EA00 + offset)
typedef struct Meter {
    u32 magic;              // +0x00
    u8 on, pstate, pcombo, hold;   // +0x04 the fight was on last frame; PS_*; the players' combo (0xFF none); bit 0 AI never fires
    s16 pfill;              // +0x08
    u16 pcool;              // +0x0A frames left
    u16 ptries, pstarts;    // +0x0C button presses that called tny_combo_start / that it accepted
    u16 hits, shared;       // +0x10 players' hits on kit fighters seen / of them shared
    u16 splits, khits;      // +0x14 split hits / kit hits on the players seen
    u16 last_f[2];          // +0x18 each player's last hit (frame, low 16 bits)
    Entity *last_t[2];      // +0x1C each player's last target
    Entity *ptarget;        // +0x24 the last shared target
    u16 last_shared, fights;   // +0x28 the last shared hit's frame / fight starts seen
    u8 press, drawn, nslot, nkits;  // +0x2C a test press pending (1 + player); bit 0 the bars drawn; AI slots used; kits seen
    u16 tile, notarget;     // +0x30 the bars' OBJ tiles; presses with no living opponent
    Entity *call_a, *call_b, *call_t;   // +0x34 the last tny_combo_start call: lead, partner, target
    u8 call_combo, call_side, call_ret, _p0;   // +0x40
    ASlot slot[SLOTS];      // +0x44 .. +0x74
    u16 drained;            // +0x74 frames the players' bar drained
    u8 sel, _p1;            // +0x76 Select held last frame (bit 0 player 1, bit 1 player 2)
    u16 _p2[4];
} Meter;
_Static_assert(sizeof(Meter) == 0x80, ".combo.meter is 0x80 bytes");
static Meter M COMBO_SECTION("meter");

typedef struct Caption {    // .combo.cap (0x0203EA80)
    u32 magic;              // +0x00
    const char *q[CAP_MAX]; // +0x04 the queue (the head shows)
    u8 n, sprites;          // +0x10 queued; the head's 32 x 16 sprites
    u16 timer;              // +0x12 the head's frames left
    u16 tile, shown;        // +0x14 its OBJ tiles; captions shown
    u16 fails, drops;       // +0x18 no heap / VRAM for one; dropped (queue full)
    u16 frames, width;      // +0x1C frames drawn in all; the head's text width (px)
    u16 hidden;             // +0x20 frames a caption waited behind a scene
    u8 _p[0x0E];
} Caption;
_Static_assert(sizeof(Caption) == 0x30, ".combo.cap is 0x30 bytes");
static Caption cap COMBO_SECTION("cap");

int ui_text_width(const char *s);   // tny_ui.c: the dialogue font's width of a line

static void m_init(void) {
    if (M.magic == METER_MAGIC) return;
    memset(&M, 0, sizeof M);
    M.magic = METER_MAGIC;
    M.pcombo = 0xFF;
    M.pfill = FULL;
}
static void cap_init(void) {
    if (cap.magic == CAP_MAGIC) return;
    memset(&cap, 0, sizeof cap);
    cap.magic = CAP_MAGIC;
}

// ---- helpers --------------------------------------------------------------------------------------------------------
static u16 now(void) { return (u16)g_FrameCounter; }
static int ram_ptr(const void *p) {
    u32 a = (u32)p;
    return !(a & 3) && ((a >= 0x02000000u && a < 0x02040000u) || (a >= 0x03000000u && a < 0x03008000u));
}
static int in_list(const Entity *e) {
    const EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) if (l->items[i] == e) return 1;
    return 0;
}
// a kit fighter standing in the fight
static int alive_kit(const Entity *e) { return e && in_list(e) && tny_moves_fighter(e) >= 0 && e->hp > 0; }
static s32 dist2(const Entity *a, const Entity *b) {
    s32 dx = (a->x - b->x) >> 8, dy = (a->y - b->y) >> 8;
    return dx * dx + dy * dy;
}
// the fight's living kit fighters (entity list order)
static int kits(Entity **out, u8 *ids) {
    const EntityList *l = g_EntityList;
    int n = 0;
    for (int i = 0; i < l->count && n < KITS_MAX; i++) {
        Entity *e = l->items[i];
        if (!e || e->hp <= 0) continue;
        int id = tny_moves_fighter(e);
        if (id < 0) continue;
        out[n] = e;
        ids[n++] = (u8)id;
    }
    return n;
}
// The players' pair: both on the floor in a co-op fight (never the players' own duel), else 0
static int floor_pair(u8 *ids, Entity **ents) {
    if (coop2.duel_active || tny_floor_picks(ids, ents) != 2 || !ents[0] || !ents[1] || ents[0] == ents[1]) return 0;
    return 1;
}
static int held(void) { return tny_combo_busy() || SCENE_RUNNING || tny_curtain(); }

// ---- the fight's start ----------------------------------------------------------------------------------------------
static void fight_start(void) {
    M.pstate = PS_READY;                        // full and ready at the start of every fight
    M.pfill = FULL;
    M.pcool = 0;
    M.last_t[0] = M.last_t[1] = 0;
    M.ptarget = 0;
    M.press = 0;
    M.nslot = 0;
    memset(M.slot, 0, sizeof M.slot);           // the AI's bars start empty
    M.fights++;
}

// ---- hits -----------------------------------------------------------------------------------------------------------
static void players_hit(int k, Entity *t) {
    if (tny_moves_fighter(t) < 0) return;       // only hits on kit fighters (the opponents)
    u16 f = now();
    int o = !k;
    int recent = M.last_t[o] && (u16)(f - M.last_f[o]) <= SHARE_WINDOW;
    M.hits++;
    M.last_t[k] = t;
    M.last_f[k] = f;
    if (recent && M.last_t[o] == t) {           // the same opponent, close together: a shared hit
        M.shared++;
        M.ptarget = t;
        M.last_shared = f;
        if (M.pstate == PS_FILL) {
            M.pfill += P_STEP;
            if (M.pfill >= FULL) { M.pfill = FULL; M.pstate = PS_READY; }
        }
    } else if (recent) {                        // they split up: the bar drains from now
        M.splits++;
        M.last_shared = (u16)(f - DRAIN_DELAY - 1);
    }
}

static void ai_hit(int id) {
    M.khits++;
    for (int i = 0; i < M.nslot; i++) {
        ASlot *s = &M.slot[i];
        if (s->ida != id && s->idb != id) continue;
        s->hits++;
        if (s->state != AS_FILL) continue;
        s->fill += AI_STEP;
        if (s->fill >= FULL) { s->fill = FULL; s->state = AS_READY; s->cool = 0; }
    }
}

// Every hit that lands: tny_moves.c kit_message (a player's hit on a kit fighter), the players' take-damage hooks
// (duel.c player 1, player.c player 2: the attacker word before the hit, a projectile's owner at +0x1C)
void tny_combo_on_hit(Entity *target, Entity *attacker) {
    if (!target || !ram_ptr(attacker) || tny_combo_busy()) return;   // a combo's own blows fill no bar
    m_init();
    u8 ids[2];
    Entity *ents[2];
    if (!tny_fight_on() || !floor_pair(ids, ents)) return;   // co-op fights only
    int k = attacker == ents[0] ? 0 : attacker == ents[1] ? 1 : -1;
    if (k >= 0) { players_hit(k, target); return; }
    if (target != ents[0] && target != ents[1]) return;
    int id = tny_moves_fighter(attacker);
    if (id < 0) {
        Entity *owner = *(Entity **)((u8 *)attacker + 0x1C);
        if (ram_ptr(owner)) id = tny_moves_fighter(owner);
    }
    if (id >= 0) ai_hit(id);
}

// ---- firing ---------------------------------------------------------------------------------------------------------
static int start(int combo, Entity *a, int a_id, Entity *b, int b_id, Entity *t, int side) {
    M.call_a = a; M.call_b = b; M.call_t = t;
    M.call_combo = (u8)combo; M.call_side = (u8)side;
    int lead = tny_combo_lead(a_id, b_id);
    int r = lead ? tny_combo_start(combo, b, b_id, a, a_id, t, side) : tny_combo_start(combo, a, a_id, b, b_id, t, side);
    if (lead) { M.call_a = b; M.call_b = a; }
    M.call_ret = (u8)r;
    return r;
}

static Entity *players_target(int k, Entity **ents) {
    if (alive_kit(M.ptarget)) return M.ptarget;                 // the one they have been hitting together
    if (alive_kit(M.last_t[k])) return M.last_t[k];
    if (alive_kit(M.last_t[!k])) return M.last_t[!k];
    Entity *ks[KITS_MAX], *best = 0;
    u8 kid[KITS_MAX];
    int n = kits(ks, kid);
    for (int i = 0; i < n; i++) if (!best || dist2(ks[i], ents[k]) < dist2(best, ents[k])) best = ks[i];
    return best;
}

static void players_fire(int k, u8 *ids, Entity **ents) {
    Entity *t = players_target(k, ents);
    M.ptries++;
    if (!t) { M.notarget++; return; }
    if (start(M.pcombo, ents[0], ids[0], ents[1], ids[1], t, 0)) {
        M.pstarts++;
        M.pstate = PS_COOL;
        M.pcool = COOLDOWN;
        M.pfill = 0;
    }
}

// ---- the AI pairs ---------------------------------------------------------------------------------------------------
static Entity *nearest_of(int id, const Entity *p, Entity **ks, const u8 *kid, int n, const Entity *not) {
    Entity *best = 0;
    for (int i = 0; i < n; i++)
        if (kid[i] == id && ks[i] != not && (!best || dist2(ks[i], p) < dist2(best, p))) best = ks[i];
    return best;
}

static void ai_frame(Entity **ents, int frozen) {
    Entity *ks[KITS_MAX];
    u8 kid[KITS_MAX], claimed[KITS_MAX];
    int n = kits(ks, kid);
    M.nkits = (u8)n;
    // a fighter whose id already has a slot is claimed; the rest pair up greedily (list order)
    for (int i = 0; i < n; i++) {
        claimed[i] = 0;
        for (int j = 0; j < M.nslot; j++) if (M.slot[j].ida == kid[i] || M.slot[j].idb == kid[i]) claimed[i] = 1;
    }
    for (int i = 0; i < n && M.nslot < SLOTS; i++) {
        if (claimed[i]) continue;
        for (int j = i + 1; j < n; j++) {
            int c = claimed[j] ? -1 : tny_combo_of(kid[i], kid[j]);
            if (c < 0) continue;
            ASlot *s = &M.slot[M.nslot++];
            memset(s, 0, sizeof *s);
            s->ida = kid[i]; s->idb = kid[j]; s->combo = (u8)c; s->state = AS_FILL;
            claimed[i] = claimed[j] = 1;
            break;
        }
    }
    if (frozen) return;
    for (int i = 0; i < M.nslot; i++) {
        ASlot *s = &M.slot[i];
        if (s->state == AS_COOL) {
            if (s->cool) s->cool--;
            if (!s->cool) { s->state = AS_FILL; s->fill = 0; }
            continue;
        }
        if (s->state != AS_READY) continue;
        if (s->cool) { s->cool--; continue; }
        if (M.hold & 1) continue;
        for (int k = 0; k < 2; k++) {           // both of them in reach of one player (standing)
            Entity *p = ents[k];
            if (revive_is_down(p)) continue;
            Entity *a = nearest_of(s->ida, p, ks, kid, n, 0);
            Entity *b = a ? nearest_of(s->idb, p, ks, kid, n, a) : 0;
            if (!b || dist2(a, p) > AI_REACH * AI_REACH || dist2(b, p) > AI_REACH * AI_REACH) continue;
            s->tries++;
            if (start(s->combo, a, s->ida, b, s->idb, p, 1)) {
                s->starts++;
                s->state = AS_COOL;
                s->cool = COOLDOWN;
                s->fill = 0;
            } else {
                s->cool = AI_RETRY;
            }
            break;
        }
    }
}

// ---- per frame (tny_frame) ------------------------------------------------------------------------------------------
void tny_combo_meter_frame(void) {
    m_init();
    int on = tny_fight_on();
    if (on && !M.on) fight_start();
    M.on = (u8)on;
    u8 ids[2];
    Entity *ents[2];
    if (!on || !floor_pair(ids, ents)) { M.pcombo = 0xFF; M.press = 0; M.sel = 3; return; }   // sel 3: a held Select is no press
    int c = tny_combo_of(ids[0], ids[1]);
    M.pcombo = c < 0 ? 0xFF : (u8)c;
    int frozen = held();
    if (c >= 0) {
        if (!frozen) {
            if (M.pstate == PS_COOL) {
                if (M.pcool) M.pcool--;
                if (!M.pcool) { M.pstate = PS_FILL; M.pfill = 0; }
            } else if (M.pstate == PS_FILL && M.pfill > 0 && (u16)(now() - M.last_shared) > DRAIN_DELAY) {
                M.pfill = M.pfill > DRAIN ? M.pfill - DRAIN : 0;
                M.drained++;
            }
        }
        // one button, either player: Select. The engine's ring lock (tny_engine.c, before this in tny_frame) takes
        // Select out of `pressed` and leaves it in `current`, so the press is the edge of `current` here
        u8 sel = (u8)(((g_InputSystem->current & KEY_SELECT) ? 1 : 0) | ((coop.input2.current & KEY_SELECT) ? 2 : 0));
        u8 edge = sel & ~M.sel;
        M.sel = sel;
        int k = -1;
        if (edge & 1) k = 0;
        else if (edge & 2) k = 1;
        else if (M.press) k = (M.press - 1) & 1;
        M.press = 0;
        if (k >= 0 && !frozen && M.pstate == PS_READY && !revive_is_down(ents[0]) && !revive_is_down(ents[1]))
            players_fire(k, ids, ents);
    }
    ai_frame(ents, frozen);
}

// ---- drawing (the HUD's draw hook) ----------------------------------------------------------------------------------
// A 48 x 8 strip of 4bpp tiles (a 32 x 8 and a 16 x 8 sprite): rows 0-4 a black frame round a 46 x 3 fill
static void put4(u8 *t, int x, int y, u8 c) {
    u8 *b = t + (x >> 3) * 32 + y * 4 + ((x & 7) >> 1);
    *b = (x & 1) ? (u8)((*b & 0x0F) | (c << 4)) : (u8)((*b & 0xF0) | c);
}
static void bar(u8 *t, int fill, u8 colour) {
    int px = fill * 46 / FULL;
    if (fill > 0 && px < 1) px = 1;
    memset(t, 0, 192);
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 48; x++)
            put4(t, x, y, (y == 0 || y == 4 || x == 0 || x == 47) ? C_BLACK : (x - 1 < px ? colour : C_EMPTY));
}
static void obj(u32 **oam, int x, int y, u32 shape_size, u16 tile) {
    if (x <= -32 || x >= wide_w() || y <= -16 || y >= 160) return;
    u32 *o = *oam;
    u32 xo = (((u32)x & 0x1FF) << 16) | ((((u32)x >> 9) & 1) << 25);
    o[0] = shape_size | xo | ((u32)y & 0xFF);   // 4bpp, priority 0, palette bank 0
    o[1] = (o[1] & 0xFFFF0000u) | tile;
    *oam = o + 2;
}
#define OBJ_32x8   0x40004000u                  // horizontal, size 1
#define OBJ_16x8   0x00004000u                  // horizontal, size 0
#define OBJ_32x16  0x80004000u                  // horizontal, size 2

static void bars_free(void) {
    if (M.tile) ObjVram_Free(g_ObjVramList, M.tile);
    M.tile = 0;
}

static void bars_draw(u32 **oam) {
    u8 ids[2];
    Entity *ents[2];
    int show = M.on && M.pcombo != 0xFF && !held() && floor_pair(ids, ents)
        && !revive_is_down(ents[0]) && !revive_is_down(ents[1])
        && (M.pstate == PS_READY || (M.pstate == PS_FILL && M.pfill > 0));
    M.drawn = (u8)show;
    if (!show) { bars_free(); return; }
    if (!M.tile) {
        u16 t = ObjVram_Alloc(g_ObjVramList, 192);
        if (!t || t == 0xFFFF) return;
        M.tile = t;
    }
    u8 buf[192];
    u8 colour = M.pstate == PS_READY ? (((g_FrameCounter >> 3) & 1) ? C_WHITE : C_YEL) : C_CYAN;
    bar(buf, M.pfill, colour);
    u8 *vram = (u8 *)(OBJ_VRAM + M.tile * 0x20);
    for (int j = 0; j < 192; j += 4) *(u32 *)(vram + j) = *(u32 *)(buf + j);
    int y = coop.hud2.y + 17;                   // just under the panels (the HUD's y, as hud.c sets it)
    for (int k = 0; k < 2; k++) {
        int x = k ? wide_w() - 96 + 12 : 12;    // under the HP bar of player 1's panel / player 2's
        obj(oam, x, y, OBJ_32x8, M.tile);
        obj(oam, x + 32, y, OBJ_16x8, (u16)(M.tile + 4));
    }
}

// ---- the caption ----------------------------------------------------------------------------------------------------
static void cap_free(void) {
    if (cap.tile) ObjVram_Free(g_ObjVramList, cap.tile);
    cap.tile = 0;
}
static void cap_pop(void) {
    cap_free();
    for (int i = 1; i < CAP_MAX; i++) cap.q[i - 1] = cap.q[i];
    cap.q[CAP_MAX - 1] = 0;
    if (cap.n) cap.n--;
}

// The head line through the stock canvas (the ui's text: dialogue font, white with a black shadow) into 4bpp OBJ
// tiles, 32 x 16 sprites side by side (1D mapping: each sprite's top four tiles, then its bottom four)
static int cap_render(const char *s) {
    int w = ui_text_width(s) + 8;               // the text and 4 px of the black box each side
    if (w > 240) w = 240;
    int n = (w + 31) / 32;
    u16 tile = ObjVram_Alloc(g_ObjVramList, n * 256);
    if (!tile || tile == 0xFFFF) return 0;
    u8 cv[0x38] __attribute__((aligned(4)));
    memset(cv, 0, sizeof cv);
    UiCanvas_Create(cv, 240, 16);
    u8 *px = *(u8 **)(cv + 0x24);
    if (!px) { ObjVram_Free(g_ObjVramList, tile); return 0; }
    cv[1] = 16;
    *(u32 *)(cv + 0x0C) = UI_FONT;
    *(u32 *)(cv + 0x20) = UI_GLYPH_SHADOW;
    memset(px, 0, 240 * 16);
    cv[0] = 0;                                  // left aligned at x 4, glyphs' top row 2
    *(s32 *)(cv + 4) = 4;
    *(s32 *)(cv + 8) = 14;
    cv[0x28] = C_BLACK;
    cv[0x29] = C_WHITE;
    UiCanvas_Text(cv, s);
    volatile u16 *dst = (volatile u16 *)(OBJ_VRAM + tile * 0x20);
    for (int sp = 0; sp < n; sp++)
        for (int ty = 0; ty < 2; ty++)
            for (int tx = 0; tx < 4; tx++)
                for (int r = 0; r < 8; r++) {
                    const u8 *row = px + (ty * 8 + r) * 240 + sp * 32 + tx * 8;
                    for (int i = 0; i < 8; i += 4) {
                        int x0 = sp * 32 + tx * 8 + i;
                        u16 v = 0;
                        for (int q = 0; q < 4; q++) {   // a black box behind the text (the floors are bright)
                            u8 c = x0 + q < w ? (row[i + q] ? row[i + q] & 0xF : C_BLACK) : 0;
                            v |= (u16)(c << (q * 4));
                        }
                        *dst++ = v;
                    }
                }
    UiCanvas_Free(cv, 0);
    cap.tile = tile;
    cap.sprites = (u8)n;
    cap.width = (u16)w;
    return 1;
}

void tny_caption(const char *line) {
    cap_init();
    if (!line || !*line) return;
    if (cap.n >= CAP_MAX) { cap.drops++; return; }
    cap.q[cap.n++] = line;
}

static void cap_draw(u32 **oam) {
    if (!cap.n) { cap_free(); return; }
    if ((SCENE_RUNNING && !tny_combo_busy()) || tny_curtain()) { cap.hidden++; return; }   // never over a scene
    if (!cap.tile) {
        if (!cap_render(cap.q[0])) { cap.fails++; cap_pop(); return; }
        cap.timer = CAP_FRAMES;
        cap.shown++;
    }
    int x = (wide_w() - cap.width) / 2, y = wide_h() - 20;
    for (int i = 0; i < cap.sprites; i++) obj(oam, x + i * 32, y, OBJ_32x16, (u16)(cap.tile + i * 8));
    cap.frames++;
    if (cap.timer) cap.timer--;
    if (!cap.timer) cap_pop();
}

void tny_combo_draw(u32 **oam) {
    m_init();
    cap_init();
    bars_draw(oam);
    cap_draw(oam);
}

void tny_combo_hud_gone(void) {
    m_init();
    cap_init();
    bars_free();
    cap_free();
    cap.n = 0;
    for (int i = 0; i < CAP_MAX; i++) cap.q[i] = 0;
}

// ---- test command 32 (docs/ids.md) ----------------------------------------------------------------------------------
static const char *const test_lines[] = { "Team combo test", "Kamehameha!", "A third caption" };

void tny_combo_meter_test(u32 cmd, u32 arg, u32 arg2) {
    (void)cmd;
    m_init();
    cap_init();
    switch (arg) {
    case 0: fight_start(); break;                                         // as at a fight's start
    case 1: M.pstate = PS_READY; M.pfill = FULL; M.pcool = 0; break;      // the players' bar full
    case 2: M.pstate = PS_FILL; M.pfill = 0; M.pcool = 0; break;          // empty, cooldown over
    case 3: if (M.pstate == PS_COOL) { M.pstate = PS_FILL; M.pfill = 0; M.pcool = 0; } break;   // end the cooldown
    case 4: M.pstate = PS_COOL; M.pcool = (u16)arg2; M.pfill = 0; break;  // cooldown of arg2 frames
    case 5:                                                               // AI slot arg2 (0xFF all) full
        for (int i = 0; i < M.nslot; i++)
            if (arg2 == 0xFF || arg2 == (u32)i) { M.slot[i].state = AS_READY; M.slot[i].fill = FULL; M.slot[i].cool = 0; }
        break;
    case 6: M.press = (u8)(1 + (arg2 & 1)); break;                       // the button, player arg2, next frame
    case 7:                                                               // a caption
        tny_caption(arg2 < 3 ? test_lines[arg2] : tny_combo_name(M.pcombo == 0xFF ? COMBO_GENERIC : M.pcombo));
        break;
    case 8: M.pstate = PS_FILL; M.pfill = (s16)(arg2 > FULL ? FULL : arg2); M.last_shared = now(); break;
    case 9: M.hold = (u8)arg2; break;                                     // bit 0: the AI never fires by itself
    case 10: {                                                            // player (arg2 & 1) punches kit (arg2 >> 8 & 0xFF)
        u8 ids[2];
        Entity *ents[2];
        if (!floor_pair(ids, ents)) break;
        Entity *ks[KITS_MAX];
        u8 kid[KITS_MAX];
        int n = kits(ks, kid), i = (arg2 >> 8) & 0xFF, gap = (arg2 >> 16) & 0xFF;
        if (i >= n) break;
        Entity *p = ents[arg2 & 1], *t = ks[i];
        int side = (arg2 >> 24) & 1;                                      // 0 on his left, 1 on his right
        p->x = t->x + (side ? 1 : -1) * ((gap ? gap : 20) << 8);
        p->y = t->y;
        p->facing = side ? 2 : 3;
        Player_Attack(p);                                                     // the A button's punch (0x08007DE9)
        break;
    }
    case 11: {                                                            // kit (arg2 & 0xFF) stands arg2 >> 8 px right of player (arg2 >> 24)
        u8 ids[2];
        Entity *ents[2];
        if (!floor_pair(ids, ents)) break;
        Entity *ks[KITS_MAX];
        u8 kid[KITS_MAX];
        int n = kits(ks, kid), i = arg2 & 0xFF;
        if (i >= n) break;
        Entity *p = ents[(arg2 >> 24) & 1];
        ks[i]->x = p->x + (s32)(((arg2 >> 8) & 0xFFFF) << 8);
        ks[i]->y = p->y;
        break;
    }
    }
}
