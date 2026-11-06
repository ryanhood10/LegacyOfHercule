// The Cell Games (docs/story.md 4h, data/story/cell_games.*): solo fights.
//
// Fights 1-2 and the Cell Junior fights are fought by one player while the other watches. CoopCall 15
// (a) sets the mode: 0 both players, 1 player 1 fights alone, 2 player 2 fights alone. While a mode is
// set, player 2's entity is removed and not respawned, the co-op twins do not spawn, and in mode 2
// player 2's pad drives player 1's entity (the fighter the script made player 1) so a loss is the
// normal game over. The mode only holds in the arena (zone 14 areas 2-4): anywhere else it reads 0
// and is cleared, so a game over or a reload never leaves a player hidden.
#include "coop.h"
#include "krillin.h"
#include "tny_engine.h"
extern const char *const ui_strings[];   // relocated UI strings (krillin.c): UI_NAME + slot = a character's name

#define CG_MAGIC 0xCE11CA3Eu
#define CG_ZONE  14

typedef struct CgState {
    u32 magic;      // partyram is not cleared at boot: the mode only counts with the magic set
    u8 solo;        // 0 both, 1 player 1 alone, 2 player 2 alone (on player 1's entity)
    u8 _pad[3];     // tests: [0] stat to win + 1, [1] frames ready, [2] who fought the last win
    Entity *owner[2];   // CoopCall 16: the entities holding a beam and their beam objects
    void *beam[2];
    u8 choice_armed;    // CoopCall 18: a yes / no question is on screen
    u8 choice;          // 1 yes (A), 2 no (B): the last A or B press on either pad while armed
    u16 choice_prev;    // the A / B buttons held at the previous poll (presses are edges)
} CgState;

CgState cg_state __attribute__((section(".party.cg"), used));

static int in_arena(void) {
    const u8 *m = (const u8 *)g_MapState;   // map state: +0 zone, +1 area
    return m && m[0] == CG_ZONE && m[1] >= 2 && m[1] <= 4;
}

int coop_solo(void) {
    if (cg_state.magic != CG_MAGIC || !cg_state.solo) return 0;
    if (!in_arena() && !tny_ring_active()) { cg_state.solo = 0; cg_state.owner[0] = cg_state.owner[1] = 0; return 0; }
    return cg_state.solo;
}

// COOP_CMD_WIN_STAT (chapter tests): the stat whose fight wins itself once an enemy of it stands and player 1
// is free to move (the fight has started, not a scene that only shows it); cg_state's spare bytes hold it
u8 *cg_test_slot(void) {
    if (cg_state.magic != CG_MAGIC) { cg_state.magic = CG_MAGIC; cg_state.solo = 0; cg_state._pad[0] = cg_state._pad[1] = cg_state._pad[2] = 0;
                                     cg_state.owner[0] = cg_state.owner[1] = 0; cg_state.beam[0] = cg_state.beam[1] = 0; }
    return cg_state._pad;
}

// The test win's record (the Cell Juniors' check, docs/testing.md "Cell Juniors"): at every COOP_CMD_WIN_STAT win,
// how many enemies stood and where the first two were (map px). Tests find it by its magic word ("JRWN").
typedef struct WinNote {
    u32 magic;
    u8 standing, stat, _p[2];
    s16 xy[2][2];
} WinNote;
_Static_assert(sizeof(WinNote) == 0x10, "WinNote: .party.pp holds 0x10");
static WinNote win_note __attribute__((section(".party.pp"), used));
void cg_note_win(int stat, int standing, const s16 xy[2][2]) {
    win_note.magic = 0x4E57524Au;
    win_note.stat = (u8)stat;
    win_note.standing = (u8)standing;
    for (int i = 0; i < 2; i++) { win_note.xy[i][0] = xy[i][0]; win_note.xy[i][1] = xy[i][1]; }
}

// CoopCall 16 Beam (sprite, on): a Kamehameha held by the entity showing that sprite, driven from C the way the
// beam command of script step 137 (NPCFireKamehameha, 0x0801D788 / 0x0801D7DC) drives it, but without the
// command chain: step 137 and batch commits wait for the entity's commands, so a beam held through the dialogue
// would stall the scene. on 0 ends that entity's beam. The beam struggle gives Hercule and the second Cell
// their beams this way (the stock clash between Gohan and Cell is background art of map 326: BG0's strip, BG1's
// starburst, its knot at x 428..453, y about 387).
// any other on: the beam stops where the clash meets (Ryan: Hercule behind Gohan and the second Cell behind Cell add to
// the clash, their beams stop in the middle); on 2: no cap (the finale's push through Cell).
// The beam object (0x50 bytes, decompiled 2026-10-20): +0x10 state (0 charge, 1 out, 2 ending), +0x11 head
// blocked (impact frames), +0x1C / +0x20 origin, +0x2C / +0x30 unit direction, +0x34 length in px (the head;
// the drawn tip is 16 px beyond), +0x4C owner. It grows up to 6 px a frame until a projectile-mask wall, the view's
// edge, a hit or an opposing beam's head stops it, so a camera pan let the held beams run on past the middle.
// The cap wraps the update (vtable slot 10) in a RAM copy of the vtable and clamps the length after the stock
// update, so the drawing (it reads +0x34) never sees it longer. The hit type is the owner's byte +0x16C, past a
// script actor's 0x16C-byte allocation (a heap header byte): the owner becomes player 1 (type 1: enemies only;
// the scene has none), so the beam can neither hurt nor be stopped by a player.
ROM_FN(Entity *, Vm_FindCharEntity, (int id), 0x08009371);   // 0 = player 1, 1-6 party slots, else a sprite id
ROM_FN(void, Entity_GetBox, (s32 *out, Entity *e), 0x080079B9);
ROM_FN(void, Entity_BeamPoint, (s32 *out, Entity *e), 0x08007A27);
ROM_FN(void *, Beam_Init, (void *mem, s32 *pos, int a, Entity *owner), 0x0801D84D);
ROM_FN(void, Beam_Fire, (void *beam, int facing, s32 *pos), 0x08014479);
ROM_FN(void, Beam_End, (void *beam), 0x0801451D);
#define BEAM_OFFSETS ((const s32 *)0x08701DF0)   // per facing {dx, dy}: down, up, left (-9, 2), right (10, 0)

typedef struct Beam {
    u32 *vtable; s32 sort_y; u8 prio, _p0[3]; u16 tile, tile_bytes;
    u8 state, impact, alt, _p1;              // +0x10
    s32 fx_time, ticks;                      // +0x14
    s32 x, y;                                // +0x1C origin (world px)
    s32 sx, sy;                              // +0x24 screen
    s32 dx, dy;                              // +0x2C unit direction
    s32 len;                                 // +0x34 px
    s32 facing, mult;                        // +0x38
    const u8 *frame, *frame_set, *frames;    // +0x40
    Entity *owner;                           // +0x4C
} Beam;
_Static_assert(sizeof(Beam) == 0x50, "Beam size");
#define BEAM_VTABLE   0x08025830u            // relative offsets (Beam_Init's subclass); slot 10 = update
#define BEAM_VT_SLOTS 14                     // the 14 known slots
#define CG_CLASH_X    440                    // map 326: the middle of the stock clash
ROM_FN(void, Beam_Update, (Beam *b, int run), 0x0801D88B);
// The capped beams' vtable copy and the struggle's scene state (CoopCall 16 on 3 / 4), in the 0x40 bytes the 16-word copy had
// (partyram is full, and the tests peek cg_state, s13, rv by address: nothing before this moves).
typedef struct CgBeamRam {
    u32 vt[BEAM_VT_SLOTS];
    s16 clash_x, clash_to;   // where the held beams meet now (map px) and where the script sent it; it moves `step` px a frame
    u8 step;
    u8 spirit;               // a body shown as a spirit: 0 none, 1 flickering (two frames in four), 2 whole, 3 hidden
    u8 spirit_id;            // its sprite id
    u8 seen;                 // CoopCall 16 on 5 / 6: CG_SEEN once the summit's scene has played in full since power-on (a retry plays its short form)
} CgBeamRam;
#define CG_SEEN 0x5A         // the mark; up to CG_SEEN + CG_SEEN_HOLD - 1 while it counts polls spent off the Arctic (cg_choice_input)
#define CG_SEEN_HOLD 60
_Static_assert(sizeof(CgBeamRam) == 0x40, "CgBeamRam: .party.bv holds 0x40");
static CgBeamRam cgb __attribute__((section(".party.bv")));
static int cg_seen(void) { return cgb.seen >= CG_SEEN && cgb.seen < CG_SEEN + CG_SEEN_HOLD; }
#define cg_beam_vt cgb.vt
static const u8 cg_clear_remap[256];         // every colour transparent (the Rumble's blink, tny_engine.c)

// The spirit's body through the transparent remap or its own colours (entity +0x24 the remap, +0x20 the decoded frame)
static void cg_spirit_show(int whole) {
    Entity *e = Vm_FindCharEntity(cgb.spirit_id);
    if (!e || e == g_PlayerObject || e == coop.p2) return;
    const u8 *want = whole ? 0 : cg_clear_remap, **slot = (const u8 **)((u8 *)e + 0x24);
    if (*slot == want || (*slot && *slot != cg_clear_remap)) return;   // never over another remap
    *slot = want;
    *(u32 *)((u8 *)e + 0x20) = 0;            // the next frame is decoded through it
}

// Every input poll in the arena: the clash walks to where the script sent it (not from the beams' own update: a text
// box may have them away, or one of them gone).
static void cg_scene_step(void) {
    if (cgb.clash_x < 64 || cgb.clash_x > 2000) cgb.clash_x = cgb.clash_to = CG_CLASH_X;   // partyram is not cleared at boot
    int d = cgb.clash_to - cgb.clash_x, s = cgb.step ? cgb.step : 1;
    if (d > s) d = s;
    if (d < -s) d = -s;
    cgb.clash_x = (s16)(cgb.clash_x + d);
}

static void cg_beam_update(Beam *b, int run) {
    Beam_Update(b, run);
    if (b->state != 1 || !b->dx) return;
    if (b->ticks > 600) b->ticks -= 480;     // a held beam is held: it faded on its own after a while (the script refired it at every step)
    s32 cap = (cgb.clash_x - b->x) * b->dx - 16;   // the drawn tip on the clash's middle
    if (cap < 8) cap = 8;
    if (b->len > cap) { b->len = cap; b->impact = 1; }
}

static void cg_beam_capped(Beam *b) {
    const s32 *vt = (const s32 *)BEAM_VTABLE;
    for (int i = 0; i < BEAM_VT_SLOTS; i++) cg_beam_vt[i] = BEAM_VTABLE + vt[i] - (u32)cg_beam_vt;
    cg_beam_vt[10] = (u32)cg_beam_update - (u32)cg_beam_vt;
    b->vtable = cg_beam_vt;
}

// A held beam can be destroyed under us: a text box that needs OBJ VRAM evicts effect objects first (docs/engine.md "OBJ VRAM
// and the text box"), and the beam's memory is then free for the next allocation. A slot's beam counts only while it is on the
// entity list with a beam's vtable; a dead one is forgotten at the next poll (cg_beams_sweep) and fired again by the script's
// next `on` (the struggle re-asserts its two beams at every script entry).
static int cg_beam_alive(const void *b) {
    if (!b) return 0;
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++)
        if ((const void *)l->items[i] == b) { u32 vt = *(const u32 *)b; return vt == (u32)cg_beam_vt || vt == BEAM_VTABLE; }
    return 0;
}
static void cg_beams_sweep(void) {
    for (int k = 0; k < 2; k++) if (cg_state.beam[k] && !cg_beam_alive(cg_state.beam[k])) cg_state.beam[k] = 0;
    if (cg_state.beam[0] && cg_state.beam[0] == cg_state.beam[1]) cg_state.beam[1] = 0;   // never one object in both slots
}

int cg_beam(int id, int on, int c) {
    if (on == 5) { cgb.seen = c ? CG_SEEN : 0; return 0; }   // the summit's scene (arctic_summit.lscript): seen in full / forget
    if (on == 6) return cg_seen();                           // -> 1 on a retry in the same session (pushed by vmcall.c)
    if (on == 3) {                           // the clash goes to x = c px at id px a frame (0 = 1; 99 = at once)
        cgb.clash_to = (s16)c;
        cgb.step = (u8)(id == 99 ? 0 : id);
        if (id == 99) cgb.clash_x = (s16)c;
        return 0;
    }
    if (on == 4) {                           // sprite id as a spirit: c = 1 flickering, 2 whole, 3 hidden, 0 as any body again
        cgb.spirit_id = (u8)id;
        cgb.spirit = (u8)c;
        cg_spirit_show(c == 0 || c == 2);
        return 0;
    }
    Entity *e = Vm_FindCharEntity(id);
    if (!e) return 0;
    int k;
    cg_beams_sweep();
    for (k = 0; k < 2 && cg_state.owner[k] != e; k++) {}
    if (!on) {
        if (k < 2) {
            if (cg_state.beam[k]) Beam_End(cg_state.beam[k]);
            cg_state.owner[k] = 0; cg_state.beam[k] = 0; Entity_SetAnimation(e, 0);
        }
        return 0;
    }
    if (k < 2 && cg_state.beam[k]) return 0;   // already holding one, and it is alive
    if (k == 2) for (k = 0; k < 2 && cg_state.owner[k]; k++) {}
    if (k == 2) return 0;
    s32 box[4], pos[2];
    Entity_GetBox(box, e);
    pos[0] = (box[0] + box[2]) >> 1;
    pos[1] = (box[1] + box[3]) >> 1;
    void *beam = Beam_Init(0, pos, 0, e);
    if (!beam) return 0;
    if (g_PlayerObject) ((Beam *)beam)->owner = g_PlayerObject;   // a known hit type (see above)
    if (on != 2) cg_beam_capped(beam);   // 2: the finale's push through Cell
    EntityList_Add(g_EntityList, (Entity *)beam);
    Entity_SetAnimation(e, 30);              // the beam pose (group 18)
    int f = e->facing & 3;
    Entity_BeamPoint(pos, e);
    pos[0] += BEAM_OFFSETS[f * 2];
    pos[1] += BEAM_OFFSETS[f * 2 + 1];
    Beam_Fire(beam, f, pos);
    cg_state.magic = CG_MAGIC;
    cg_state.owner[k] = e;
    cg_state.beam[k] = beam;
    return 0;
}

// CoopCall 18 Choice (op): a yes / no question in an ordinary text box. op 1 arms it right before the
// question's text entry; while armed, input.c calls cg_choice_input every poll: the last A or B press on either
// pad is the answer, and B is turned into A so either button closes the box. op 0 (after the text entry) pushes
// the answer, 1 = yes, 2 = no, and disarms. (Post-game: Dende's "start the Buu's Fury campaign?", docs/story.md 4i.)
void cg_choice_arm(void) {
    (void)cg_test_slot();                    // the state's magic
    cg_state.choice_armed = 1;
    cg_state.choice = 0;
    cg_state.choice_prev = 0xFFFF;           // a button still held from the last box is not a press
}

int cg_choice_take(void) {
    int c = cg_state.choice ? cg_state.choice : 2;
    cg_state.choice_armed = 0;
    cg_state.choice = 0;
    return c;
}

void cg_choice_input(u16 *p1, u16 *p2) {
    // The summit's short form (CoopCall 16 on 5 / 6) is for a retry from the ridge pad: a game over leaves EWRAM alone (seen: "You have
    // died!" on zone 1 area 7, the title, the load), a soft reset clears it. A second on any map but the Arctic's (zone 15 areas 1,
    // 5-8), the death screen's and the title (no map) drops the mark, so another save or a new game in this session sees the scene in
    // full; shorter than that is a map load's half-written zone / area (seen: it dropped the mark on the way to the death screen).
    if (cg_seen()) {
        const u8 *m = (const u8 *)g_MapState;
        int off = m && (m[0] || m[1]) && !(m[0] == 1 && m[1] == 7) && !(m[0] == 15 && (m[1] == 1 || (m[1] >= 5 && m[1] <= 8)));
        if (!off) cgb.seen = CG_SEEN;
        else if (++cgb.seen >= CG_SEEN + CG_SEEN_HOLD) cgb.seen = 0;
    }
    if (cg_state.magic == CG_MAGIC && in_arena()) {
        if (cg_state.owner[0] || cg_state.owner[1]) { cg_beams_sweep(); cg_scene_step(); }   // while a beam is held
        if (cgb.spirit && cgb.spirit <= 3) cg_spirit_show(cgb.spirit == 2 || (cgb.spirit == 1 && ((g_FrameCounter >> 1) & 1)));   // the spirit flickers
    } else if (cgb.spirit) cgb.spirit = 0;   // partyram is not cleared at boot; nothing of it outside the arena
    if (cg_state.magic != CG_MAGIC || !cg_state.choice_armed) return;
    u16 held = (u16)((*p1 | *p2) & (KEY_A | KEY_B));
    u16 edge = (u16)(held & ~cg_state.choice_prev);
    cg_state.choice_prev = held;
    if (edge & KEY_B) cg_state.choice = 2;
    else if (edge & KEY_A) cg_state.choice = 1;
    if (*p1 & KEY_B) *p1 = (u16)((*p1 & ~KEY_B) | KEY_A);   // B closes the box too
    if (*p2 & KEY_B) *p2 = (u16)((*p2 & ~KEY_B) | KEY_A);
}

// Title cards (CoopCall 14 op 3, kind 0 player 1 / 1 player 2 / 2 both): with two players they say who fights;
// alone, the one player fights them all, so the card names the character ("Goku fights!").
static char card_buf[24] __attribute__((section(".party.ui")));
const char *cg_card(int kind) {
    static const char *const two[3] = { "@^Player 1 fights!", "@^Player 2 fights!", "@^Players 1 & 2 fight!" };
    if (kind < 0 || kind > 2) kind = 0;
    if (p2_coming() || (kind == 1 && coop_solo() == 2 && p2_present())) return two[kind];
    const char *name = ui_strings[UI_NAME + g_Party->active_slot];
    char *o = card_buf;
    *o++ = '@'; *o++ = '^';
    while (*name && o < card_buf + 14) *o++ = *name++;
    const char *tail = " fights!";
    while (*tail) *o++ = *tail++;
    *o = 0;
    return card_buf;
}

void cg_set_solo(int mode) {
    if (mode < 0 || mode > 2) mode = 0;
    cg_state.magic = CG_MAGIC;
    cg_state.solo = (u8)mode;
    if (mode && coop.p2) EntityList_Remove(g_SceneTable[1], coop.p2, 0);   // its destroy clears coop.p2
}
