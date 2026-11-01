// The team combos' step tables (runner piece; docs/tournament-combos.md is their source, tools/tnycombo.py turns it into
// build/gen/tny_combo_data.c at build time). docs/tournament.md 25.2 describes the runner.
#pragma once
#include "tny_combo.h"

// who a step is for (bits): fighter 0 (the pair's lead, tny_combo_lead), fighter 1, the target
enum { CW_A = 1, CW_B = 2, CW_T = 4 };

// what a step does (a, b, c, d per op; the doc's format section names them)
enum {
    CO_END,         // the combo ends here: control back, everyone where they stand (clamped to the ring's floor)
    CO_GO,          // a = px from the target (on the mover's own side; the target: away from fighter 0), b = px down,
                    // c = frames, d = pose kind while moving (CP_*; 0xFF keep the pose)
    CO_TELE,        // the same at once, with a flash at both ends (an afterimage)
    CO_POSE,        // a = pose kind, b = first frame, c = last frame, d = frames to run b..c (0 = hold b; < 0 = loop
                    // every -d frames)
    CO_FACE,        // a = 0 the target (the target: fighter 0), 1 the target faces fighter 1, 2 turn away
    CO_PUNCH,       // a melee hit: a = weight (tenths of one of his own punches), b = pose kind (punch / kick / special)
    CO_BLAST,       // a ki blast flying at the target: a = weight (tenths of his own ki blast), b = art (0 orb, 1 ki)
    CO_BALL,        // a big orb thrown slowly: a = weight, b = size (1 medium, 2 large, 3 the game's Big Bang), c = frames
                    // held first (over his head, or d = 1 in front of his hands) before it flies
    CO_BEAM,        // his beam: a = weight, b = frames held, c = 1 it goes through to the far side (else it ends on him)
                    // (CO_BLAST, CO_BALL, CO_BEAM: the step's col, a colour of the doc's section 4; 0 = the art's own)
    CO_DISC,        // a Destructo Disc: a = weight, b = frames held over his head first, c = 1 it pins him (stays
                    // spinning on him until the partner's next blow lands)
    CO_SHAKE,       // a = frames, b = strength
    CO_SOUND,       // a = sound id (the game's effect table)
    CO_FLASH,       // whoever: a white flash a frames long
    CO_SCREEN,      // the whole screen flashes white, fading over a frames
    CO_KNOCK,       // the target falls (pose group 3), lies a frames, gets up over b frames
    CO_CAPTION,     // the move's name at the bottom (tny_caption)
    CO_SBC,         // Piccolo's Special Beam Cannon (the game's own object): a = weight, b = frames held once fired,
                    // c = 1 through (as CO_BEAM), d = frames of charge (the spark at his fingers) before it fires
    CO_COPY,        // a copy of the fighter's body (it shows his pose, frame for frame): a = copy 1-3 (0 = all), b / c =
                    // the spot (as CO_GO), d = 1 the copy vanishes
    CO_COUNT
};
// pose kinds: a group and its stand-ins for bodies that lack it (tny_combo_run.c `kinds`)
enum { CP_IDLE, CP_WALK, CP_HURT, CP_FALL, CP_SHOUT, CP_CHARGE, CP_PUNCH, CP_KICK, CP_DASH, CP_SPECIAL, CP_BEAM,
       CP_BEAM2, CP_KI, CP_LIE, CP_COUNT, CP_RAW = 0x40, CP_KEEP = 0xFF };   // CP_RAW + g: group g as it is

typedef struct TComboStep { u16 at; u8 who, op; s16 a, b, c, d; u8 col; } TComboStep;
typedef struct TComboDef { const char *name; const TComboStep *steps; u16 n, len; } TComboDef;
extern const TComboDef tny_combo_defs[COMBO_COUNT];     // build/gen/tny_combo_data.c

// Colours (the doc's section 4; tools/tnycombo.py): one shared 256-colour OBJ palette, so a colour is a remap of the
// palette indices the energy arts use (the runner's orbs and ki, the Kamehameha beam) to five of another ramp.
// tny_combo_tints[c]: colour c's remap (c 0: none); tny_combo_beams[c]: the Kamehameha beam's four facings' frame sets
// (8 descriptors each, as the ROM's at 0x086E3458) drawn in colour c, or 0 (no beam in that colour, or no ROM at build)
#define TCOL_MAX 12
typedef struct TBeamDesc { u32 geom, attr; const u8 *src; } TBeamDesc;
extern const u8 *const tny_combo_tints[TCOL_MAX];
extern const TBeamDesc *const tny_combo_beams[TCOL_MAX];
extern const u8 *const tny_combo_spark;                  // the colour `white` (where a body vanishes: a spark)

// The kits' own energy outside the combos (the doc's section 4, its kits' table): fighter (TF_*), slot (0-5 in his
// tny_kits row), colour; ends with fighter 0xFF. tny_colours.c: the shared painting.
typedef struct TKitColour { u8 fighter, slot, col; } TKitColour;
extern const TKitColour tny_kit_colours[];
void tny_beam_paint(void *beam, int col);                 // a fired beam (right after Beam_Fire) redrawn in colour col
void tny_tint_tiles(void *obj, const u8 *tint);          // a projectile's upload (slot 5) through a remap
int  tny_kit_colour(int fighter, int slot);               // the colour of a kit fighter's own move, 0 = its own
