# PARTY_SLOTS: a party of N entries (built 2026-10-17, default 8)

Ryan's decision: Krillin (and later others) are real party members with their own stats and saves,
so the party grows. This page records what the build does, every ROM site that assumed six (found
by decompiling every function, `build/decomp_all/`, and grepping), how each is patched, what was
verified in mGBA and what is still placeholder. `make PARTY_SLOTS=N` (default 8, code written for
6..16; see "Limits") builds it; `tools/patch_rom.py --party-slots N` applies the patches and asserts
every patched byte's stock value.

## The party block

Stock: `g_PartyState` at `0x03000E90`, 0x164 bytes of IWRAM BSS followed by other globals, so it
cannot grow in place. The build pins `g_party_block` (`src/coop/party.c`, section `.party`) at
`0x0203F000` and repoints the 27 ROM words that hold the stock address (`PARTY_PTR_WORDS` in
`patch_rom.py`; the static initialiser's literal at file 0x22EE8 plus the per-module pointer tables).
The arena cap drops to 0xF9FE words: the arena hands out `0x02000808 + 4 * cap`, so the stock 0xFDFE
ended exactly at 0x02040000 and the earlier 0xFCC0 let the first allocation (the 8 KiB EEPROM mirror,
`0x08021454`) run 8 bytes past 0x0203FB00 into `coop2`. Layout (`PartyState` in `src/coop/rom.h`;
offsets for N = 6, everything after the entries moves by `(N - 6) * 0x1C`):

```
+0x00  u8   active slot              +0x01 u8 flags: bit0 saved once, bit1 meter shown, bit2 debug party
+0x02  u8   zone, area, variation    (save position's map, SaveGame)
+0x08  s32  meter (24.8)
+0x0C  CharacterEntry[N] x 0x1C      hp/ep/row/stats/exp/flags/abilities
+0xB4  u8[0x66] event flags          Flag_Test/Set/Clear 0x08003E42/5E/74, condition word 0x08003F06
+0x11A u8[0x10] seen bits            journal / bestiary (0x0800B00C, 0x08018A38, 0x08019204)
+0x12B u8[0x32] item counters        0x08004348, 0x080095E4, 0x080096CA, 0x080096E8, 0x0800F3F0
+0x160 s16 x, +0x162 s16 y           save position (pixels)
```

`Party_Init` fills the entries from the default table and zeroes 50 bytes at +0xB4 and at +0x12A.
The region `partyram` (`coop.ld`, 0x0203F000..0x0203FB00) also holds, in link order: the save menu's
slot store (`party_save.c`) and the switch screen's view party (`switch.c`) in `.party`; then
`.party.kr` (`a16p` from `a16player.c`, `kr_state` from `krillin.c`); then `.party.wv` (`waves`, the
Time Chamber state from `waves.c`, `0x0203F880` in the merged build). `coop2` keeps 0x0203FB00.
Addresses after the party block move with `PARTY_SLOTS` and with code changes; read the build's
`coop.map`. (The three sites `0x0800831C/8330/833E`
listed by the earlier inventory touch an Entity's resistance bytes, not the party.)

## Every site that assumed six, and its patch

**Pointer words (27)** -> `LITERAL_RELOCATIONS` to `g_party_block`.

**Tail offsets, re-implemented in C** (entry jumps, `PARTY_ENTRY_JUMPS`; all generic in
`PARTY_SLOTS`): `Party_Init` 0x08003C24, `Flag_Test/Set/Clear` 0x08003E42/5E/74, the script condition
word 0x08003F06 (`src/coop/party.c`); VM counter opcodes 0x080095E4/96CA/96E8, item use 0x0800F3F0,
bestiary "seen" opcode 0x0800B00C. (0x08003E8A is an unreferenced flag toggle.)

**Tail offsets in large constructors -> shadow tables** (`PARTY_SHADOWS`): 0x08004348 (status item
list, counters), 0x08018A38 (journal page, seen bit), 0x08019204 (bestiary list, seen bits) load the
party through a private pool word -> pointer table -> party word. The pool word is repointed to a
0x200-byte copy of the table at file 0xA01000+ whose party word is `g_party_block + shift`, so their
unchanged `adds #0xFF; adds #0x11; ldrb [r, #10]` instructions land on the moved banks for any N.
The same trick gives the switch screen its view party (below).

**Count constants** (`PARTY_COUNT_PATCHES`, imm8): 0x08003E00 / 0x08003E1E / 0x08003ED4 loops,
the status list builder 0x08003480 (loop and object size 0x170 + 4 per slot, also allocated by
0x08004CAC), the VM character-id range `(id - 2) <= 5` in 0x0800B1E2 (scripts name characters as
slot + 2: Krillin = 8), the debug party loop 0x08005C0A (fills only the *defined* slots: entries
with a max HP after the data patch). The switch screen's own `< 6` (0x08010C90) stays: its view
never holds more than six.

**Default party and growth rows**: rebuilt with N entries at file 0xA00000 (`romdata.PARTY_TABLE_AT`:
stock rows 0-5, zero rows above, growth rows after the entries) and the literal at 0x3EA0 repointed
to `table + N * 0x1C - 0xA8` so the level-up code 0x08003CA0 still reads growth at literal + 0xA8.
`tools/romdata.py build --party-slots N` writes `data/hercule.json`'s party/growth records into that
table (the Makefile passes it); `romdata.py party build/<dir>/log2-coop.gba` prints the built one.

**Save path** (`src/coop/party_save.c`, entry jumps): `SaveGame` 0x08004030, `LoadSlot` 0x08003F9C,
`WriteSlot` 0x08003FEC and the save menu's 0x08001960 (read slots), 0x080019D4/19F6 (saved),
0x08001A0C (location name), 0x08001A26 (portrait count), 0x08001A44/1A66 (member in party /
selectable), 0x08001A8A (active position), 0x08001AB8 (member sprite), 0x08001AE2 (start as member),
0x08001B0E (erase), 0x0800180C (start game, returns 0 like the original or the menu falls through to
the intro video). EEPROM layout: header 0x10 unchanged, slots of `SAVE_STRIDE = (sizeof PartyState +
4 + 7) & ~7` bytes (0x1A0 for N = 8) at `0x10 + slot * stride`, block then u32 checksum
(`0x0802153E`), written through the ROM's own mirror + EEPROM routines (`0x080214C4`, 8-byte
blocks). Three slots use 0x4F0 of the 8 KiB (0x790 for N = 16). Old six-slot saves fail the checksum
and read as empty. The menu's screen object keeps its stock size: the three 0x164-byte buffers inside
it are unused, the slots live in a private store (`SaveStore` in `.party`), and its six-wide portrait
rows show a window of at most six members around the slot's active character (start-as-member picks
among those). The menu has no slot-copy feature (start / erase only).

**Switch Characters screen** (`src/coop/switch.c`): the stock screen object (0xD8 bytes) holds six
portrait pairs and reads the party through its module table (0x086D763C word 1). That word is
shadowed to `switch_view`, a compacted party holding the window of up to six members currently shown,
so the stock open routine 0x08010BCE lays the portraits out exactly as before. Replaced: the
constructor 0x08010B08 (allocates 0xD8 + the member list / window, builds the view), prev/next
0x08010B60/0x08010B9A (queries over the whole party), select 0x0801110C, arrow-button state
0x080110BC and the input handler 0x08011172, which rebuilds the portraits (delete + create, same
layout rule) when the selection leaves the window and draws arrow tiles (two 8x8 tiles rendered into
VRAM 0x06004600, placed on the frame's edge columns of the tilemap at 0x0600E800, rows 9-10; the right
one is the horizontal flip) while more characters exist on that side. Confirm writes the real party's
active slot and refreshes player 1 as the original did.

**Status screen** (0x0800379C, the pause menu): its list object is sized for N (above) and shows one
character per page set. In the pause menu **Right / Left step through the characters and Up / Down
through a character's pages** (stats, then techniques); the corner arrows switch Journal / Status /
Items. Per-slot titles, page counts and technique pages: "Krillin (slot 6)" below.

**Co-op code**: `PartyState chars[PARTY_SLOTS]`, `player.c` slot loops and `p2_cycle`, `ssj.c`,
`tools/mgba/peek.lua` / `enemies.lua` read the block through the `g_party_block` symbol.

## Krillin (slot 6) and slot 7

Slot 6 is Krillin for real since 2026-10-18 (`docs/hercule.md` 5b, `src/coop/krillin.c`): display row
23 (Gohan's standard row with sprite 161, dialogue portrait 73, status portrait 119), his own stats,
growth and abilities 21 / 19 / 20 / 2. The per-slot tables that had six entries are grown for
PARTY_SLOTS (16 rows in the generated tables, `UI_BLOCK`):

| six-slot site | now |
|---|---|
| status title `slot + 0x30` (`0x08003B7E`, vtable slot 13 of `0x08023B3C` / `0x080248CC`) | `status_title_hook`: `ui_strings[UI_NAME + slot]` |
| status page count (`0x0800350A`: Hercule 1, others 4) | `status_pages_hook`: slots 0-4 four, Hercule one, Krillin three, others one |
| status page dispatch (`0x08003B44`) | `status_draw_hook`: Krillin's pages 1-2 = the stock energy page `0x08003520` (p 0 / 1) |
| technique names / descriptions `X[1..4] + slot`, icons `X[13 + 2 slot + p]` (module table `0x081D94D0`, literal 0x377C) | `status_module`: per-slot icon pairs for PARTY_SLOTS, string bases = the `UI_ENERGY1/2`, `UI_DESC1/2` blocks |
| special melee page `0x9D/0xA2 + slot` (`0x08003684`) | not shown for slots 5+ (page count) |
| dialogue `%s` name `0x30/0x36 + active` (`0x080094B0/CC`) | `vm_push_name_hook` / `vm_push_caps_hook`: the `UI_NAME` / `UI_CAPS` blocks |
| status portrait (row +3 -> `Portrait_Table`, ids 8-25) | appended to `portrait_module` (`tools/krillinsprites.py`): 119 Krillin's status portrait, 120 Paragus' dialogue portrait (`tools/brolysprites.py`), 121 Android 16's status portrait (row 25 +3) |
| level-up victory poses (script `0x083B6524`, slots 0-5 by `PushActiveCharIndex`) | none for slot 6: he keeps standing (harmless) |

Slot 7 is the duel clone's slot (`duel.c` copies player 1's entry into it for a clone duel) and,
during the "Gero's computer" chapter only, **Android 16** (`src/coop/a16player.c`, 2026-10-19): no duel
runs in that chapter, so `CoopCall 11` writes his entry there (row 25, Krillin's EXP, his stats, flags
0x43, abilities 22 / 19 / 0 / 2) and `CoopCall 12` clears its flags when he leaves (a later clone duel
overwrites the slot as before). Growing `PARTY_SLOTS` was not needed. Growth row 7 (`data/hercule.json`)
is 16's level-up range. The status screen names slot 7 "Android 16" on row 25 and names a clone after
the character whose row it copies (`krillin.c name_slot`).

## Level-ups (fixed 2026-10-18)

`Party_AddExp` 0x08003D58 (the EXP event's fire) reaches its module words (SFX mixer, entity list,
map state, the dialogue's "no actor" pair) at **negative offsets from the default-table literal
0x3EA0** (`literal - 0x1C .. - 0x8`, the module table `0x081D952C` in the stock ROM). Relocating the
table moved the literal (`table + N * 0x1C - 0xA8`), so in every PARTY_SLOTS build a level-up read
party data as those pointers and its message-0x15 broadcast jumped to `0x526F526E` when the level-up
dialogue closed (seen in the fork with Krillin; any character would crash). `party_add_exp_hook`
(`src/coop/party.c`, entry jump) re-implements it with the words read from the stock table; other
users of the literal (`Party_Init`, replaced; the growth code, `literal + 0xA8`) were checked in the
disassembly. Verified: level 40 -> 41 for Krillin as player 1, dialogue closed, play continued.

## Limits

- `PARTY_SLOTS` 6..16 (static assert). The count patches are imm8; the object-size patches reach
  N = 16; the shadow tables and C code have no limit. `partyram` (region 0xB00) holds the party, the
  save store (3 slots + 8 bytes each), the view, `.party.kr` and `.party.wv`: 0x91C bytes for N = 8
  (merged build, 2026-10-19). N = 16 does not fit: a test link on 2026-10-19 needed 0xDB4 bytes and
  failed with "region `partyram' overflowed by 692 bytes". It needs the region start and the heap cap
  lowered by at least 0x2B4 (0x300 keeps alignment: region at 0x0203ED00, cap
  (0x0203ED00 - 0x02000808) / 4 = 0xF93E). (The earlier figures, 0x87C for N = 8 and 0xCC4 for N = 16
  with a 0x200 shortfall, predate `.party.kr` and `.party.wv`.)
- The save menu shows at most six members per slot; the switch screen shows six at a time.
- The debug party (Map Test) fills slots 0..defined-1 with level-40 characters (flags 0x3F for every
  slot but Hercule's, so Krillin shows both technique pages there).

## Verified (WSL fork, `build/party8`, PARTY_SLOTS=8, docs/testing.md "fork helpers")

Boot: 8-entry defaults in the block, the stock block stays zero. Map Test party of seven, zone 2
area 1: the save pad tutorial flag lands in the moved bank (+0xEC), Save writes slot 0 (checksum,
zone/area/position), the Select Game menu shows the slot's six-portrait window and "East District
439", loading restores all eight entries and lands on the pad; a new game runs through the hooks.
Switch Characters: six portraits, right indicator, Right scrolls to Piccolo..Krillin with the left
indicator, A makes Krillin player 1 (active slot 6), the pause menu shows his stats. Erase: the menu's R
mode, Delete Game? Yes, and the slot reads as a new game (active 3, flags 0). Not exercised: the
journal with seen bits set, a save from the real story (the fork's EEPROM starts as a
512-byte device and zero-extends; slots beyond it read as empty until written).

## Follow-ups

- Save-menu portrait row wider than six (the menu object's 3 x 6 entity arrays).
- The 6 x 0x1C bytes still reserved for the party inside the save-menu object and the scroll's
  rebuilt portraits (deleted through the entity list, so no leak was seen) are the only loose ends.
