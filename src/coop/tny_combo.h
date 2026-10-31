// The tournament's team combos (docs/tournament.md 24.10 the design decided with Ryan, 25 what was built).
// Contract between the pieces (2026-10-31): the lead (this header, tny_combo.c's pair table, the hooks, the draw),
// the meter (tny_combo_meter.c: shared-hit tracking, the bars, the cooldown, the button, the AI's trigger, the caption)
// and the runner (tny_combo_run.c: the frozen fight, the step tables, damage, the knockdown). Every function a piece
// does not own has a weak default in tourney_hooks.c, so each piece builds and tests alone.
#pragma once
#include "tourney.h"

// ---- RAM: `comboram` (coop.ld, 0x0203EA00, 0x200 bytes, NOLOAD; the heap cap ends there). Each piece in its own
// section, guarded by a magic word: meter `.combo.meter` (<= 0x80), caption `.combo.cap` (<= 0x30), runner
// `.combo.run` (<= 0x140).
#define COMBO_SECTION(name)  __attribute__((section(".combo." name)))

// ---- the pairs (lead; tny_combo.c, from docs/tournament.md 24.10; the runner's generator may take it over) -------
enum {
    COMBO_GENERIC,          // the barrage of ki blasts from both sides (the androids' pairs, Spopovich & Yamu)
    COMBO_GOHAN_KRILLIN,    // Krillin's Destructo Disc pins him; Gohan's Masenko finishes
    COMBO_PICCOLO_HERCULE,  // Piccolo's Special Beam Cannon pose holds him; Hercule's Dynamite Kick, the credit
    COMBO_GOKU_VEGETA,      // both dash in trading punches; a joint Kamehameha / Big Bang
    COMBO_GOKU_PICCOLO,     // Goku grabs him from behind; Piccolo's Special Beam Cannon through both (only he is hurt)
    COMBO_FATHER_SON,       // the father-son Kamehameha (Goku & Gohan; Hercule & SSJ2 Gohan: Hercule poses, no beam)
    COMBO_FRIEZA_COOLER,    // Death Beam volley from both sides; Cooler's Supernova
    COMBO_BROLY_PARAGUS,    // Paragus's control flash freezes him; Broly's Eraser Cannon
    COMBO_MECHA_COLD,       // Death Beams from both (Mecha Frieza & King Cold)
    COMBO_CELL_JR,          // the Juniors swarm him; Cell's Kamehameha
    COMBO_COUNT
};
// The combo of fighters a and b (TF_*, either order) or -1: no combo, no bar. Story picks count by person (Super
// Saiyan 2 Gohan as Gohan, but Hercule & SSJ2 Gohan is its own entry). A listed pair has it on either side.
int  tny_combo_of(int a, int b);
// With `lead` set: which of a / b leads the combo (the step tables' fighter 0): 0 = a, 1 = b
int  tny_combo_lead(int a, int b);

// ---- the meter (meter piece; tny_combo_meter.c) --------------------------------------------------------------------
void tny_combo_on_hit(Entity *target, Entity *attacker);   // every hit that lands (players on opponents, kits on players)
void tny_combo_meter_frame(void);                          // per frame (tny_frame): fight start / end, bars, the button
void tny_combo_draw(u32 **oam);                            // the HUD draw hook: the bars and the caption
void tny_combo_hud_gone(void);                             // the HUD goes: free the tiles
void tny_caption(const char *line);                        // a one-line caption at the bottom that never pauses (queued)
void tny_combo_meter_test(u32 cmd, u32 arg, u32 arg2);     // test command 32 (TNY_COMBO_METER)

// ---- the runner (runner piece; tny_combo_run.c) --------------------------------------------------------------------
// Play combo `combo` by the pair a, b (entities, ids TF_*) on `target`; side 0 = the players' pair, 1 = an opponent
// pair (the target is a player). 1 = started (the fight freezes until it ends).
int  tny_combo_start(int combo, Entity *a, int a_id, Entity *b, int b_id, Entity *target, int side);
int  tny_combo_busy(void);                                 // 1 while a combo plays (the fight is frozen)
void tny_combo_run_frame(void);                            // per frame (tny_frame)
const char *tny_combo_name(int combo);                     // the move's name (the caption)
void tny_combo_run_test(u32 cmd, u32 arg, u32 arg2);       // test command 33 (TNY_COMBO_RUN)

// ---- accessors the pieces share (lead) -----------------------------------------------------------------------------
int  tny_floor_picks(u8 *ids, Entity **ents);   // the players on the ring's floor in a fight: count (0-2), their picks and bodies
int  tny_moves_fighter(const Entity *e);        // tny_moves.c: a kit fighter's TF_* id, or -1
