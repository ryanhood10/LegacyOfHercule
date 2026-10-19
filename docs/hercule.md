# Hercule hero pass (Legacy of Hercule, stage 1)

What this pass changed, how it is built, and what was verified in mGBA (2026-10-17). It covers the
work that needs no new sprites: the ROM data tools, Hercule's stats and growth, his moveset, the
Keep-Goku byte and the first dialogue swap. The Super Saiyan form was built later (sprite 157, row 22, ability 18; `docs/sprites.md`).

## 1. Build and tools

- `make` now applies `data/hercule.json` by default (`HERCULE=0` builds the plain co-op ROM).
  `tools/romdata.py build` turns that spec into `build/hercule.patch.json`, a list of generic
  `{offset, bytes | word}` records, and `tools/patch_rom.py --data-patch` applies them after the code
  and hooks. Words may be `"fn:<symbol>"` (address | 1 from `coop.sym`) so data tables can point at
  functions in `src/coop`. Record files written by other tools are pulled in through the spec's
  `include` list (`data/hercule-text.json` holds the dialogue records).
- `tools/romdata.py party|abilities|exp [ROM]` prints the three character tables; on a built ROM it
  follows the relocated ability table through the code literal. `tools/script_dump.py` dumps a map's
  dialogue and scripts (section 5) and writes text records.
- `tools/mgba_run.ps1` restarts mGBA on a *copy* of the build (`build/log2-run.gba`), because mGBA
  locks the ROM it runs and `make` cannot overwrite it; `-NoStates` keeps the copy's own savestates.
  The copy's `log2-run.ss1` is zone 2 area 7 with player 2 bound to Hercule (this build).

Facts used (file offsets, all checked on the ROM and in the emulator; see `tools/romdata.py`):

| what | where | note |
|---|---|---|
| default party | `0x1D9548`, 6 x 28 B | one code literal (`0x3EA0`), also used by the level-up code |
| growth rows | `0x1D95F0`, 6 x 3 x {u16 min, u16 max} | per level-up: stat += rand(max-min+1) + min, one draw for all levels gained, cap 25600 |
| HP / EP growth | code `0x08003CA0` | max HP x (1 + 0x0E14..0x1333 / 65536) per level (5.5..7.5 %), EP + 2..4; shared by every slot |
| level EXP table | `0x1D9834`, 50 words | literal `0x42EC`; level 50 = 22 965 105 |
| ability table | `0x1D9900`, 14 x 16 B | literals `0x8024` and `0x8E8C`; relocated by `romdata.py` into free ROM, now 23 entries (0-22; `docs/ids.md` "Ability table") |
| player combat bytes | code `0x08016FE0` | entity +0x160/161/162 = str/pow/end >> 8; damage taken x (128 - end/256) / 128 |
| Keep Goku | `0x3EC738` | Cell's defeat dialogue, entry 53: `ClearCharacterInPartyFlag(4)` -> `SetCharacterInPartyFlag(4)` |

## 2. Growth design

Stock Hercule: 50 HP, 20 EP, str 8.0 / pow 1.0 / end 6.0, growth 64..192 on every stat (a quarter
of the others' pace), four copies of his punch. He starts tanky-but-slow and never catches up.

New Hercule (slot 5, `data/hercule.json`): 50 HP, 30 EP, str 2.0 / pow 0.5 / end 1.5, growth
str 360..560, pow 300..500, end 450..650 (Gohan: 318..518 / 302..502 / 256..456 from 3 / 5 / 3).

Why these numbers. HP growth is a shared multiplicative constant, so a 50-HP character keeps about
59 % of Gohan's HP at every level; the per-slot lever is END, which is the defence byte. With END
reaching ~85 by level 40 (Gohan ~55) his damage taken is x43/128 against Gohan's x73/128, so his
effective HP (HP x 128 / (128 - END)) matches around the Cell Games (the debug party's level 40),
while he is clearly the weakest party member for the first ten levels. STR is his specialty (he has
no beams); POW drives his projectile reskins and stays a little under the others.

Expected values (means; the game draws one uniform random per stat):

| level | HP H / G | STR H / G | POW H / G | END H / G | effective HP H / G |
|---|---|---|---|---|---|
| 1 | 50 / 85 | 2.0 / 3.0 | 0.5 / 5.0 | 1.5 / 3.0 | 50 / 87 |
| 10 | 86 / 147 | 18 / 18 | 14 / 19 | 21 / 15 | 103 / 166 |
| 20 | 165 / 281 | 36 / 34 | 30 / 35 | 42 / 29 | 245 / 363 |
| 30 | 309 / 526 | 54 / 50 | 45 / 51 | 63 / 43 | 609 / 792 |
| 40 | 586 / 1000 | 72 / 67 | 61 / 66 | 85 / 55 | 1744 / 1753 |

Verified in mGBA: the Map Test party built from the patched table gives slot 5 abilities 14-17,
HP 487, str 17966 / pow 13088 / end 19395 at level 40 (inside the new ranges). A live level-up
(EXP set to 3 335 400, one wolf kill by Hercule as player 2, +4747 EXP) raised him to 41 with
str +529, pow +482, end +489 and HP +5.5 % (`build/hc6_levelup.png`).

## 3. Moveset (ability table entries 14-18, `src/coop/hercule.c`, `ssj.c`; Krillin 19-21, Android 16 22)

Abilities are {ready icon, cooling icon, isReady, onUse}; onUse spends EP and runs an *animation*,
which is a control state from the table at `0x083B5268` that reads a fixed frame group of the
character's sprite record. Hercule's sprite (72) has the group-5 "ki pose" frames but not the beam
groups 18/19 or the fidget group 14, which decides what he can use. Tested on his sprite in mGBA:
animations 5, 34, 44, 45, 46, 47 run; 43 (Energy Punch) crashes ("Jumped to invalid address");
48-51 and 14 are unsafe by the frame-group rule.

| idx | name (HUD icon) | animation | EP | how |
|---|---|---|---|---|
| 14 | Rolling Hercule Punch (Hercule icon) | 5, his stock punch: pose, shockwave object, grunt (SFX 40) | 4 | stock onUse `0x0801327D`, 2 EP refunded |
| 15 | Dynamite Kick (Ki Blast icon) | 34, Ki Blast: one quick projectile | 3 | stock onUse `0x080131E1`, 1 EP extra |
| 16 | Megaton Punch (Big Bang icon) | 46, Big Bang: the big slow projectile | 12 | stock onUse `0x08013179`, 2 EP extra |
| 17 | Hercule Taunt (Transformation icon) | 31, the wall-bump recoil, plus SFX 40 | restores 8 | own code |
| 18 | Super Hercule (Hercule icon for now) | switches the slot to display row 22 (the recoloured clone of sprite 72, built by the parent session's `tools/spriteclone.py`) | 50 | `ssj_ready` / `ssj_use` in `src/coop/ssj.c` |

Hercule's starting list is **14, 15, 16, 17** (punch, kick, Megaton Punch, taunt). Super Hercule (18)
is kept out of it by Ryan's no-reveal rule (2026-10-18): the late-story script calls
`hercule_awaken()` (src/coop/ssj.c), which puts 18 into the third slot in place of the Megaton
Punch. (It was 14, 15, 18, 17 until then.) Verified as player 2 from `log2-run.ss1`: row 21 ->
22, EP 100 -> 50, the entity's sprite record moved to the cloned sprite (`build/hc10_ssj.png`).

The isReady gate copies the stock one (`[[0x086DB284] + 0xC] != 0`, the game's "abilities allowed"
flag) plus the EP check. The stock 14 entries are untouched; the table is a relocated copy with the
four new rows and the two code literals repointed. Verified as player 2 from `log2-run.ss1`: EP
50 -> 46 / 47 / 38 / 58 and the control states 0x08024810 (punch), 0x08025D48 (ki blast),
0x080258B4 (big bang), 0x080258C8 (bump) each returned to idle (`build/hc5_*.png`).

Later: proper icons (a 0x200-byte 4bpp tile pair each, a golden one for 18) once the sprite pipeline
can draw them.

## 4. Keep Goku after Cell

Applied as the one-byte record in `data/hercule.json` (`expect 5b -> 4f`). `tools/script_dump.py
--rom build/log2-coop.gba map 14 3` shows Cell's defeat dialogue (record 12, stat 25) with entry 53
now reading `PushByte 4; Step 79 SetCharacterInPartyFlag` where the stock ROM has `Step 91
ClearCharacterInPartyFlag`.

Verified in mGBA (Map Test Z14 A3 with player 2 present): Cell spawned with `spawnrec(12)`, both of
the pair set to 1 HP and beaten with ki blasts; the finale ran (beam struggle, Vegeta, the 500 000
EXP award of entry ~27, then entry 53's warp to Z6 A99 at (1832,1088), where player 1 stood at
(1824,1084) afterwards). Entry 53's party step comes right after that warp, and slot 4's flags stayed
`0x7f` through the whole dialogue (the stock step would have cleared bit 0 to `0x7e`). Screens:
`build/hc8b_dlg60.png`, `build/hc8c_end.png`.

## 5. Dialogue tooling and the first text swap

`tools/script_dump.py` walks a map entry (`0x08409368 + 0x38 i`): the music / map-name scripts, the
on-load script, triggers (dialogue when the condition is `0x0800D7E1`, script when `0x0800C2C3`),
town NPC dialogues (record +0xC), enemy defeat actions (script or dialogue) and prints dialogue
entries by mode (0 script, 1 text, 2 JCALG1 text via `tools/mapchunks.py`, 3/4 formatted text with
the argument script, 5 jump) and scripts with the 30 main opcodes and the 145 Step actions. One
correction to the reference notes: the varint of `PushVarint`/`PushMultiVarint` is 7-bit groups
MSB first with the game's own sign fold (even -> v/2, odd -> -(v/2)), read from
`BytecodeVM_PushVarint` at `0x08008F5B`; the LEB128 reading gave negative map coordinates.

The prologue (zone 1 area 4, map entry 3) is one screen-wide trigger with a five-entry sequence at
`0x083B8340`: text, compressed text, a script that loads Z1A3 at (120,104) with music 23, compressed
text, and a script that sets Trunks (slot 3) to level 6, makes him active and warps to Z1A1 at
(200,664).

Two write paths, both emitted by the tool as records (`text` in place, must fit the old string, NUL
padded; `entry` writes a new plain-text entry in free space that keeps the old entry's next-index and
speaker and repoints the sequence slot). They were proven on the prologue (entries 0 and 3 of the
sequence at `0x083B8340`, shown by a new game in mGBA: `build/hc9_t3.png`, `build/hc9_o5.png`) and
then **removed again** (commit e78e78e): Ryan's rule is that nothing reveals Hercule as the hero until
much later, so `data/hercule-text.json` is empty and the stock opening plays. Dialogue edits from here
follow `docs/story.md`; the tool and the write path stay.

## 5b. Krillin (party slot 6, display row 23, sprite 161; `src/coop/krillin.c`, 2026-10-18)

Krillin is a real party member: his own row, sprite (`docs/sprites.md` "Krillin"), portraits, names,
stats, growth and four abilities. Ids are in `src/coop/krillin.h`.

**Stats** (`data/hercule.json` slot 6; Ryan's rule: weaker than the Z fighters, like Hercule): 60 HP,
40 EP, str 1.5 / pow 2.5 / end 1.5, growth str 280..480, pow 330..530, end 300..500 per level (Gohan
318..518 / 302..502 / 256..456), flags 0x58 (0x40 like every stock row, plus bits 3 and 4 = his two
status-screen technique pages). At the debug party's level 40 he lands near str 60 / pow 70 / end 60
with about 60 % of Gohan's HP. A live level-up 40 -> 41 in the fork gave HP +7 %, EP +4, str +479,
pow +513, end +303 (inside the ranges) and showed "Krillin has achieved Level 41!".

**Abilities** (list `[21, 19, 20, 2]`):

| idx | name (HUD icon) | what | EP |
|---|---|---|---|
| 21 | Destructo Disc (new disc icons) | animation 5 (the ki pose) and a disc of the ROM's projectile class flying along his facing at 4 px / frame for up to 90 frames or until it leaves the view; every frame it sends a type-1 hit message (attack `{1, 3, 0x800}`, twice the ki blast's base) with a 24x14 box to each enemy it has not hit yet (up to 12): it **pierces** and never hits the same enemy twice; walls do not stop it; players ignore it. Cooldown 150 frames per slot; while thrown, `coop.acting` = the thrower so kill EXP goes to him | 12 |
| 19 | Afterimage Rush (Energy Punch icon) | dashes up to 40 px along his facing (4 px steps, stops at walls), then the A attack plus a melee hit `{0, 3, 0x600}` in front of him | 4 |
| 20 | Shockwave Punch (Hercule punch icon) | animation 5 and a melee burst `{0, 4, 0x500}` 36 px around him, small camera shake | 6 |
| 2 | Ki Blast (stock) | the stock ability; `kiblast_spawn_hook` (entry jump on `0x0800EAD8`) gives every ki blast whose owner plays row 23 the blue frames | 2 |

Verified in the fork (zone 2 areas 6 and 7, as player 1 and as player 2): every ability fired
(`kr_state` counters `discs, disc_hits, kiblasts, rushes` at 0x0203F86C in `build/kn`); one disc cut
through two snakes standing on Hercule and flew on (Hercule untouched), others killed snakes and the
EXP landed on slot 6; screenshots in `build/kr/shots/`.

**Status screen** (pause menu; `docs/party-8-slots.md`): the title is `ui_strings[UI_NAME + slot]`
(vtable slot 13 of both status objects, `0x08023B3C` and the pause menu's `0x080248CC`); page count
(slot 23): 4 for slots 0-4, 1 for Hercule, 3 for Krillin (stats, Destructo Disc, Ki Blast); page
dispatch (slot 24) maps Krillin's pages onto the stock energy-technique page `0x08003520`, which
reads `status_module` (a copy of the module table `0x081D94D0` with per-slot icon pairs and the
string-block bases; literal file 0x377C repointed). Shows KRILLIN, his portrait (status id 119) and
both technique pages with their icons and texts.

**Names**: `ui_strings` is the UI string table (0x080E2840, 260 entries: menu labels, names at
0x30/0x36 + slot, technique names 0x9D.. / 0xA7.. / 0xB1.. + slot, item and place names) copied by
`tools/krillinsprites.py` with six 16-entry per-slot blocks appended (energy 1 / 2 names and
descriptions, mixed-case and capital names; stock rows copied for the stock slots); `patch_rom.py`
repoints all 20 literals. The dialogue opcodes that push the active character's name (`0x080094B0`,
`0x080094CC`, "%s has achieved Level %d!" and every "%s:" line) are re-implemented to read the
blocks. The switch screen shows sprites only (no names); the save menu shows portraits.

## 5c. Old Hercule (slot 5 on display row 24, sprite 158)

For the future prologue (`docs/story.md` section 0): Hercule as comic relief in his Buu-saga look.
Row 24 is Hercule's row 21 with the Buu's Fury Hercule sprite (158; `OLD_HERCULE_SPRITE`, 72 when
`BUU=0`), no transformation links. The story switches with the game's own opcode:

- **on**: `SetCharacterTransformation(5, 158)` (opcode 73: the first display row with sprite 158 is
  row 24 in the default build; a test build with `SSJ_SPRITE=158` would match row 22 first);
- **off**: `SetCharacterTransformation(5, 72)` (row 21, the first row with sprite 72).

Abilities stay slot 5's (14-17: Rolling Hercule Punch, Dynamite Kick, Megaton Punch, Taunt), tuned weak:
while slot 5 shows row 24, `krillin_frame` (every input poll) divides his entity's combat bytes
(+0x160 melee, +0x161 ranged) by `OLD_HERCULE_WEAKNESS` (3) for whichever player plays him, and
`ssj_ready` refuses Super Hercule on row 24. C code can call `old_hercule_set(1 / 0)`. Verified in the
fork as player 2: row 21 -> 24, entity sprite record -> `buu_record_158`, combat bytes 85/73 -> 28/24,
Super Hercule refused (row and EP unchanged), and back to row 21 with the stock record.

## 6. Verification log

| stage | result |
|---|---|
| 1 data tools | tables print from stock and built ROMs; `make` applies 17 records |
| 2 growth | Map Test party and a live level-up match the new ranges (section 2) |
| 3 moveset | four abilities fire as player 2, EP costs right, no crash (section 3) |
| 4 Keep Goku | byte applied; Cell finale played in mGBA, Goku's slot keeps bit 0 (section 4) |
| 5 dialogue | prologue dump verified; both write paths shown by a new game in mGBA, then reverted per the no-reveal rule (section 5) |
| 5b Krillin | status page, technique pages, switch screen, all four abilities as player 1 and 2, disc piercing and kills, level-up (fork, section 5b) |
| 5c old Hercule | row 24 on and off as player 2, weak combat bytes, Super Hercule refused (section 5c) |

Test states on the run copy: `build/log2-run.ss1` (Z2A7, player 2 = Hercule, this build),
`log2-run.ss3` (debug menu). Driving recipe: `tools/mgba_run.ps1 -NoStates`, then
`python tools/mgba_cmd.py 'state(1)' 'p2seq("DOWN:4")' ...` (see `docs/coop-design.md`).
