# Hercule hero pass (Legacy of Hercule, stage 1)

What this pass changed, how it is built, and what was verified in mGBA (2026-10-17). It covers the
work that needs no new sprites: the ROM data tools, Hercule's stats and growth, his moveset, the
Keep-Goku byte and the first dialogue swap. The Super Saiyan/"Super Human" form waits for the
sprite pipeline (`docs/sprites.md`, in progress in another session).

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
| ability table | `0x1D9900`, 14 x 16 B | literals `0x8024` and `0x8E8C`; relocated to `0x900000` with 18 entries |
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

## 3. Moveset (ability table entries 14-17, `src/coop/hercule.c`)

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

## 6. Verification log

| stage | result |
|---|---|
| 1 data tools | tables print from stock and built ROMs; `make` applies 17 records |
| 2 growth | Map Test party and a live level-up match the new ranges (section 2) |
| 3 moveset | four abilities fire as player 2, EP costs right, no crash (section 3) |
| 4 Keep Goku | byte applied; Cell finale played in mGBA, Goku's slot keeps bit 0 (section 4) |
| 5 dialogue | prologue dump verified; both write paths shown by a new game in mGBA, then reverted per the no-reveal rule (section 5) |

Test states on the run copy: `build/log2-run.ss1` (Z2A7, player 2 = Hercule, this build),
`log2-run.ss3` (debug menu). Driving recipe: `tools/mgba_run.ps1 -NoStates`, then
`python tools/mgba_cmd.py 'state(1)' 'p2seq("DOWN:4")' ...` (see `docs/coop-design.md`).
