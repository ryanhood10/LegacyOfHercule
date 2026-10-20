// Krillin (party slot 6) and old Hercule (slot 5's Buu-saga row): ids shared by src/coop/krillin.c, the
// generated tables (tools/krillinsprites.py, tools/spriteclone.py) and data/hercule.json. docs/hercule.md
// "Krillin" / "Old Hercule", docs/sprites.md "Krillin".
#pragma once

#define KRILLIN_SLOT        6
#define KRILLIN_ROW         23     // display row (tools/spriteclone.py)
#define KRILLIN_SPRITE_ID   161    // sprite index id of krillin_record (Makefile KRILLIN_SPRITE)
#define KRILLIN_STATUS_PORTRAIT 119   // portrait_module id of his status portrait (row +3)
#define HERCULE_SLOT        5
#define HERCULE_ROW         21
#define HERCULE_SPRITE      72
#define OLD_HERCULE_ROW     24     // display row: Hercule's row with the Buu's Fury sprite
#ifndef OLD_HERCULE_SPRITE
#define OLD_HERCULE_SPRITE  158    // Makefile: 72 when the Buu's Fury ROM is absent (BUU=0)
#endif

// Relocated UI string table (ui_strings, tools/krillinsprites.py): 260 stock entries, then blocks of
// UI_BLOCK per-slot entries (stock rows copied for the stock slots).
#define UI_STOCK    260
#define UI_BLOCK    16
#define UI_ENERGY1  (UI_STOCK + 0 * UI_BLOCK)   // status page "energy 1" technique name, by slot
#define UI_ENERGY2  (UI_STOCK + 1 * UI_BLOCK)
#define UI_DESC1    (UI_STOCK + 2 * UI_BLOCK)
#define UI_DESC2    (UI_STOCK + 3 * UI_BLOCK)
#define UI_NAME     (UI_STOCK + 4 * UI_BLOCK)   // "Gohan", ..., "Krillin" (status title, dialogue %s)
#define UI_CAPS     (UI_STOCK + 5 * UI_BLOCK)   // "GOHAN", ..., "KRILLIN"

// Old Hercule is comic relief: his combat bytes (entity +0x160 / +0x161) are divided by this while his
// slot shows OLD_HERCULE_ROW
#define OLD_HERCULE_WEAKNESS 3

// The melee hit of Krillin's abilities, shared with ssj2gohan.c: one hit message {kind, reaction, power} over
// the box (cx +- rx, cy +- ry) to every entity of the scene list
void melee_burst(Entity *e, s32 cx, s32 cy, s32 rx, s32 ry, const u32 *attack);

// Destructo Disc (ability 22)
#define DISC_EP          12
#define DISC_COOLDOWN    150     // frames between throws (per slot)
#define DISC_SPEED       0x400   // 8.8 px per frame (the ki blast flies at 0x300)
#define DISC_LIFE        90      // frames before it fades even if still on screen
#define DISC_POWER       0x800   // attack base (the ki blast's is 0x400)
#define DISC_MAX_HITS    12      // enemies one disc can cut (each once)
#define DISC_HALF_W      12      // hit box half-size around the disc centre (px)
#define DISC_HALF_H      7

// Melee pair (abilities 19, 20)
#define RUSH_EP          4
#define RUSH_DISTANCE    40      // px dashed before the punch
#define RUSH_POWER       0x600
#define SHOCK_EP         6
#define SHOCK_RADIUS     36
#define SHOCK_POWER      0x500
