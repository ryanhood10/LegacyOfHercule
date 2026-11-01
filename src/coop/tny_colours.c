// The energy colours, shared by the team combos (tny_combo_run.c) and the kits' own specials outside them (tny_moves.c).
// Their one home is docs/tournament-combos.md section 4: the colours' five palette entries and the kits' table (which
// colour a fighter's own move takes); tools/tnycombo.py builds from it a remap per colour (tny_combo_tints), the
// Kamehameha beam's frame sets drawn in each colour a beam uses (tny_combo_beams) and the kits' list (tny_kit_colours).
// docs/tournament.md 25.4 (how the colours work), 25.5 (the kits' beams).
#include "tny_combo_run.h"

#define OBJ_VRAM 0x06010000u
#define E16(e, o) (*(u16 *)((u8 *)(e) + (o)))
#define E32(e, o) (*(u32 *)((u8 *)(e) + (o)))

// A beam in a colour: Beam_Fire (0x08014478) took the facing's frame set from the ROM (+0x44 / +0x48, the frame +0x40)
// and put its three blobs up (tile + 0, 0x100, 0x200: descriptors 0, 2, 1); the same from the set built in that colour,
// so every later upload (the tip's flicker, the end) reads the coloured tiles too. Called right after Beam_Fire.
void tny_beam_paint(void *beam, int col) {
    const TBeamDesc *sets = col > 0 && col < TCOL_MAX ? tny_combo_beams[col] : 0;
    if (!beam || !sets) return;
    const TBeamDesc *set = sets + (E32(beam, 0x38) & 3) * 8;   // +0x38 the facing
    E32(beam, 0x44) = E32(beam, 0x48) = (u32)set;
    E32(beam, 0x40) = (u32)(set + 1);
    static const u16 at[3] = { 0, 0x100, 0x200 }, k[3] = { 0, 2, 1 };
    for (int i = 0; i < 3; i++) {
        const u32 *s = (const u32 *)set[k[i]].src;
        volatile u32 *v = (volatile u32 *)(OBJ_VRAM + (u32)E16(beam, 0x0C) * 32 + at[i]);
        for (int w = 0; w < 0x40; w++) v[w] = s[w];
    }
}

// A projectile's upload (vtable slot 5; the ROM's 0x08013D40 DMAs the frame's tiles) through a colour's remap: the
// ProjBase layout (+0x0C its tile, +0x10 the frame loaded, +0x18 the frame to show; frames {xo, yo, w, h, attr, tiles})
void tny_tint_tiles(void *obj, const u8 *t) {
    const u8 *d = (const u8 *)E32(obj, 0x18);
    const u8 *src = *(const u8 *const *)(d + 8);
    volatile u16 *v = (volatile u16 *)(OBJ_VRAM + (u32)E16(obj, 0x0C) * 32);
    E32(obj, 0x10) = (u32)d;
    for (int i = 0, n = d[2] * d[3] >> 1; i < n; i++) v[i] = (u16)(t[src[2 * i]] | t[src[2 * i + 1]] << 8);
}

// The colour of kit fighter `fighter`'s own move in slot `slot` (0-5 of his row in tny_kits), 0 = the art's own
int tny_kit_colour(int fighter, int slot) {
    for (const TKitColour *c = tny_kit_colours; c->fighter != 0xFF; c++)
        if (c->fighter == fighter && c->slot == slot) return c->col;
    return 0;
}
