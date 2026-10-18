# Growing the party from 6 to 8 slots (plan, 2026-10-17)

Ryan's decision: Krillin (and later others) are real party members with their own stats and saves,
so the party grows. This page is the inventory of everything in the ROM that assumes six, found by
decompiling every function (`build/decomp_all/`, `DecompileFunctions.java ... all`) and grepping,
and the plan to change it. No code written yet.

## What the party block is

`g_PartyState` at `0x03000E90`, 0x164 bytes, BSS (not in the IWRAM image); the next global starts
at `0x03000FF4`, so it cannot grow in place. Layout (from `Party_Init` `0x08003C24`, the flag
accessors and the save code):

```
+0x00  u8   active slot
+0x01  u8   flags (bit0 "saved", bit2 ...)
+0x02  u8[3] zone, area, variation of the save position
+0x04  u32  ?
+0x08  s32  meter (HUD, 24.8)
+0x0C  CharacterEntry[6] x 0x1C   (name/hp/ep/stats/exp/flags/abilities; +0x20 flags: bit0 in party, bit1 selectable, bit5.. unlocked moves)
+0xB4  u8[100] event-flag bank 1  (Flag_Test/Set/Clear 0x08003E42/5E/74, 0x08003F06; Party_Init descriptor 0x19 x 4)
+0x11A u8[]   "seen" bit bank    (0x0800B00C, 0x08018A38, 0x08019204: journal / bestiary bits)
+0x12B        bank 2 descriptor base in Party_Init (0x19 x 4)
+0x160 s16 x, +0x162 s16 y  save position (SaveGame 0x08004030); +0x163 also used as a byte flag
                                (0x0800831C/8330/833E set it to 0 / 0x80 around character changes)
```

The game never holds the party address in code literals except once (static init, file 0x22EE8);
everything else reads it from 26 pointer tables in data (per-module globals blocks such as
`0x083B50DC+0x20`, the script module's blocks, `0x081D94D0`). Only two data words point *inside*
the block and both are unrelated graphics bytes, so the block can move freely.

## Every site that assumes six (grouped by the patch it needs)

**A. Pointer words (27)**: file 0x22EE8, 0x1D4F20, 0x1D94D0, 0x1D9534, 0x1D9C10, 0x1D9CA8,
0x1DA8D8, 0x3B50FC, 0x3B526C, 0x3B5354, 0x3B5C98, 0x3B5F6C, 0x3EC838, 0x3EC9D0, 0x3ED1A4,
0x3ED1C0, 0x40DAF8, 0x6ADE1C, 0x6D60EC, 0x6D7628, 0x6D7640, 0x6DB288, 0x6E3620, 0x6F7E54,
0x6F8078, 0x6F88CC, 0x6FD86C. All become the new block (LITERAL_RELOCATIONS already does this
kind of edit).

**B. Tail offsets (+0x38 each)**: Party_Init 0x08003C24 (+0xB4, +0x12B descriptors); flag bank 1
accessors 0x08003E42, 0x08003E5E, 0x08003E74, 0x08003F06 (+0xB4); bank 2 users 0x0800B00C,
0x08018A38, 0x08019204 (+0x11A); save position / end byte 0x08004030 (+0x160/+0x162),
0x0800831C, 0x08008330, 0x0800833E (+0x163). Thumb immediates: 0xB4->0xEC, 0x11A->0x152,
0x12B->0x163 fit `add #imm8`; the 0x160-0x163 sites need per-site inspection (likely
`mov+lsl` or a pool literal). About 15 instructions.

**C. Slot-count constants (`< 6`)**: 0x08003480 (switch-screen list builder; object 0x170 ->
0x178), 0x08003E00, 0x08003E28, 0x08003ED4 (detransform others), 0x08005C0A (debug party; its
`slot != 5` Hercule special case too), VM character-id checks `id - 2 < 6` in 0x0800B1E2 and
0x0800B296 (scripts name characters as slot + 2, so Krillin = 8, Hercule stays 7). About 8 bytes.

**D. Default party and growth rows**: the default table `0x081D9548` (6 x 0x1C, literal file
0x3EA0) is followed by the growth rows at table+0xA8 (`0x08003CA0` adds 0xA8). Relocate the table
to our data (8 entries + 8 x 3 growth pairs) and change 0xA8 -> 0xE0. `data/hercule.json` already
rewrites these rows through `tools/romdata.py`, so the 8-entry table can be emitted the same way.

**E. Save path (0x164 / 0x168 everywhere)**: SaveGame 0x08004030, LoadSlot 0x08003F9C, WriteSlot
0x08003FEC (each copies the block into a 0x164-byte stack buffer, checksums it and reads/writes
SRAM at `0x10 + slot * 0x168` through 0x08021526 / 0x080214C4), and the save-menu module
0x08001960, 0x080019D4, 0x080019F6, 0x08001A0C, 0x08001A26, 0x08001A8A, 0x08001AE2, 0x08001B0E,
0x0800180C (slot buffer with stride 0x164, copy / erase / validity / load). Patching stack frames
and strides by hand is fragile, so: **re-implement the three party-module save functions and the
menu's SRAM reader, slot copy and slot erase in C** (entry jumps, like `exp.c`), with a new SRAM
layout: header 0x10, slots of 0x19C + 4 checksum at stride 0x1A0 (SRAM is 32 KiB; the stock game
uses 0x448 bytes). Old saves are not loadable; the menu shows them as empty.

**F. Screens**: the save-pad switch screen (0x08010BCE / 0x08011172, list from 0x08003480) and the
status screen (0x0800379C, slot list at object+0x158) lay out portraits for up to six; check what
they do with eight (spacing or paging) once the data side works.

**G. Our own code and data**: `CharacterEntry chars[6]` in `src/coop/rom.h`, slot loops in
`player.c` (`slot_ok`, `p2_cycle % 6`, `pick_p2_slot`), `hud.c`, `exp.c`, `tools/mgba/peek.lua`
offsets, `data/hercule.json` party rows (add Krillin and old Hercule), `docs/engine.md`.

## Where the block goes

EWRAM, just below the co-op difficulty state: lower the arena cap (the existing `WORD_PATCHES`
Arena_Init heap size) by 0x200 and pin `PartyState8` (0x19C bytes) at `0x0203F900`. IWRAM would
be faster but its free space is not mapped; the party is not hot.

## Order of work (each step testable in mGBA from a savestate)

1. Relocate as-is (A + the block at 0x0203F900, still 6 entries): the game must play and save
   exactly as before. Proves the pointer inventory is complete.
2. Grow: B, C, D with 8 entries; new game, level-ups, flags, switching, journal all as before.
3. Save path E in C; save, load, copy, erase slots; `tools/savestate.py` to inspect SRAM.
4. Screens F for eight portraits.
5. G, then Krillin's row/abilities (`docs/sprites.md`) and old Hercule's row.

Estimate: steps 1-3 about two days of work with testing, 4-5 another day plus Krillin's own art and
abilities (separate plan).
