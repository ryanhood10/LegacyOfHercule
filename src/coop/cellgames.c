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
    if (!in_arena()) { cg_state.solo = 0; cg_state.owner[0] = cg_state.owner[1] = 0; return 0; }
    return cg_state.solo;
}

// COOP_CMD_WIN_STAT (chapter tests): the stat whose fight wins itself once an enemy of it stands and player 1
// is free to move (the fight has started, not a scene that only shows it); cg_state's spare bytes hold it
u8 *cg_test_slot(void) {
    if (cg_state.magic != CG_MAGIC) { cg_state.magic = CG_MAGIC; cg_state.solo = 0; cg_state._pad[0] = cg_state._pad[1] = cg_state._pad[2] = 0;
                                     cg_state.owner[0] = cg_state.owner[1] = 0; cg_state.beam[0] = cg_state.beam[1] = 0; }
    return cg_state._pad;
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
#define BEAM_VT_SLOTS 16                     // 14 known slots (partyram is tight: 16 words)
#define CG_CLASH_X    440                    // map 326: the middle of the stock clash
ROM_FN(void, Beam_Update, (Beam *b, int run), 0x0801D88B);
static u32 cg_beam_vt[BEAM_VT_SLOTS] __attribute__((section(".party.bv")));   // last in partyram: the tests peek cg_state, s13, rv by address

static void cg_beam_update(Beam *b, int run) {
    Beam_Update(b, run);
    if (b->state != 1 || !b->dx) return;
    s32 cap = (CG_CLASH_X - b->x) * b->dx - 16;   // the drawn tip on the clash's middle
    if (cap < 8) cap = 8;
    if (b->len > cap) { b->len = cap; b->impact = 1; }
}

static void cg_beam_capped(Beam *b) {
    const s32 *vt = (const s32 *)BEAM_VTABLE;
    for (int i = 0; i < BEAM_VT_SLOTS; i++) cg_beam_vt[i] = BEAM_VTABLE + vt[i] - (u32)cg_beam_vt;
    cg_beam_vt[10] = (u32)cg_beam_update - (u32)cg_beam_vt;
    b->vtable = cg_beam_vt;
}

void cg_beam(int id, int on) {
    Entity *e = Vm_FindCharEntity(id);
    if (!e) return;
    int k;
    for (k = 0; k < 2 && cg_state.owner[k] != e; k++) {}
    if (!on) {
        if (k < 2) { Beam_End(cg_state.beam[k]); cg_state.owner[k] = 0; cg_state.beam[k] = 0; Entity_SetAnimation(e, 0); }
        return;
    }
    if (k < 2) return;                       // already holding one
    for (k = 0; k < 2 && cg_state.owner[k]; k++) {}
    if (k == 2) return;
    s32 box[4], pos[2];
    Entity_GetBox(box, e);
    pos[0] = (box[0] + box[2]) >> 1;
    pos[1] = (box[1] + box[3]) >> 1;
    void *beam = Beam_Init(0, pos, 0, e);
    if (!beam) return;
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
