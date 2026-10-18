# The Legacy of Hercule: build plan

Ryan's direction (2026-10-17): keep Legacy of Goku II's story backbone (it is good and already built),
but retell it with Hercule as the true hero who grows through the Android and Cell sagas, keep every
beloved character in a role, add playable characters, new scenes, enemies, levels, movesets and silly
side quests (animal transformations), and carry it into Buu's Fury where Hercule, not Goku, finishes
Majin Buu. Co-op and the wide view stay the foundation.

## 1. What the engine lets us do (grounded in the decompile and the reference notes)

| Want | How the ROM supports it | Confidence |
|---|---|---|
| New playable characters / forms | `Character_DisplayData` rows (22 today, relocatable, limit byte at file 0x42D6 up to 255); each row = sprite id + portrait + behaviour class. Party stays 6 *slots*; who sits in a slot is swapped by the existing script opcode (`SetCharacterInParty` 79 / `ClearCharacterInParty` 91, `SetCharacterTransformation` 73). | high |
| Sprites for them | `g_CharacterSpriteIndex` (ids 3..156, relocatable); records = header + per-direction frame arrays, frames are JCALG1 streams (our decoder is ported both ways: `tools/mapchunks.py`, the fork). NPC and enemy sprites (Krillin, Yamcha, Tien, Mecha Frieza, Cooler, Androids, animals) exist as sprite records; a playable needs the walk/attack frame groups a player row expects. | high for playables, medium for promoting NPCs |
| Movesets | `g_AbilityTable` (14 entries, relocatable): icon pair, isReady, onUse (EP cost + animation). Abilities belong to the *slot* (4 per slot); making them per-character is a small patch (store the list in the row's spare word and read it in `Player_UseAbility` / HUD). | high |
| Transformations (Super Human Hercule) | A new display row (recoloured copy of a sprite) plus a transformation ability that targets it; the stock Transformation reverts rows flagged as transformed. | high (documented pattern), untested by us |
| Story and dialogue | Maps carry `mapScripts[]`: NPC (talkable), actor (scripted), enemy records; dialogue sequences and condition scripts run on the bytecode VM (145 opcodes mapped in the reference `OpCodes.csv`, warp opcode pops zone/area/variation/x/y); 43 quests decoded with their flags. Text lives in dialogue sequences we can rewrite or add. | high for reading, medium for authoring (we need our own assembler) |
| New maps and scenes | `g_MapEntries` (327 x 0x38, count and 4 literals to patch to extend); maps = chunk tilemaps + tileset, already decoded by the wide-view work, so a map *editor/importer* is within reach. | medium |
| Enemies and bosses | Enemy records (stat index, sprite, action object incl. "run dialogue on defeat"); the co-op session already scales and twins them. | high |
| Keep Goku after Cell | One byte at file 0x3EC738 (91 -> 79); applied by `data/hercule.json`. | done |
| Co-op, wide view | Done (this repo). | done |

Hard limits: 6 active party slots (the save format bakes them in; widen the pool instead), 128
sprites on screen, 240x160 native (the fork widens it), SRAM save layout (new flags must fit the
existing story-flag bits or reuse spare ones).

## 2. Pillars and milestones

**A. Playable roster (Hercule first).**
1. Sprite atlas: dump every sprite record's first frames to a PNG sheet with ids (our tool, from the
   decoder we already have), so we can *see* who exists (Krillin, Yamcha, Tien, Chiaotzu, Bulma,
   Androids 16-20, Cell forms, Mecha Frieza, Cooler, King Cold, animals) and which have full walk,
   attack and hurt groups.
2. Promote characters: add display rows for Krillin (and others whose records have the needed
   groups); give them abilities (Destructo Disc = recoloured Scatter Shot, etc.); make abilities
   per-character.
3. Hercule's growth (done, `docs/hercule.md`): stat table and growth rows tuned, moveset of four
   reskinned animations (Rolling Hercule Punch, Dynamite Kick, Megaton Punch, Taunt); still to do: a
   late transformation row "Super Human Hercule" (recolour + aura) via a transformation ability.
4. Unlocks as story rewards: Mecha Frieza playable after Cooler, Androids 16/18 after their arcs
   (enemy sprite -> playable row where the frame groups allow; otherwise a new sprite record).

**B. Story rewrite ("don't deviate too hard").**
1. Dialogue pass: a script disassembler/assembler for the VM so we can edit sequences and add
   scenes; start with the opening (Hercule's point of view), the Cell Games (Hercule's "win"), and
   the post-Cell flag so Goku stays.
2. New scenes using actor records and behaviours (walk-to, face-and-wait): cutscenes without new
   engine code.
3. Quest log: add Hercule side quests through the decoded quest table (43 entries, flag-based).

**C. Silly side quests (animal forms).**
- A "squirrel" playable row: an animal sprite with the walk groups, behaviour class copied from a
  base row, abilities empty, and a scripted quest that transforms the active character (opcode 73),
  warps to a forest map with enemies that chase, and transforms back at the exit. Co-op: both
  players become animals (player 2 rebinds the same way).

**D. Enemies and levels.**
- Twin bosses and scaled enemies (co-op session, in progress) plus new enemy placements per map
  record; later a map importer for new areas built from existing tilesets.

**E. Buu's Fury continuation.**
- Facts: Buu's Fury is a separate cartridge on the same engine family; it starts seven years after
  Cell with Goku in Other World and Gohan at school. There is no in-cart transition; both ROMs expect
  to run at 0x08000000, so a single merged cartridge is impractical.
- Our transition: a new LoG2 ending scene (Hercule's arc closes), then the emulator loads Buu's Fury
  with a converted save (the fork can swap ROMs in-process; stock mGBA via a Lua script). Buu's Fury
  needs its own decompile pass (party, display rows, saves) before "Super Human Hercule beats Buu"
  can be written; treat it as phase 2 of this plan, reusing all tools.

**F. Co-op and wide view polish.** Real save-pad run with two players, Lua scripts in the fork (or
the netplay reader ported to C like the pad reader), HUD at wide corners for dialogue boxes, more
than 128 sprites is a no.

## 3. Order of work (each step ships something playable)

1. **Sprite atlas + roster report** (1 session, in progress): the PNG sheet and a table of candidates
   with their frame groups; the Super Saiyan Hercule sprite as a new sprite id and display row
   (`docs/sprites.md`). Ryan picks the roster.
2. **Hercule hero pass, no new sprites** (done 2026-10-17, `docs/hercule.md`): ROM data tools
   (`tools/romdata.py`, `patch_rom.py --data-patch`, `data/hercule.json`), stats and growth curve,
   four abilities from existing animations (table entries 14-17), the Keep-Goku byte, the dialogue
   dumper (`tools/script_dump.py`) and the first two opening-text swaps.
3. **Hercule hero pass, sprite half** (next, after 1): the "Super Human" transformation entry
   pointing at the new display row, real icons for the four abilities, a HUD portrait.
4. **Script tooling** (1-2 sessions): a dialogue/script *assembler* on top of the dumper (new
   sequences and scripts in free space, not only in-place swaps), verified by round-tripping the
   stock scripts; then the scene edits of pillar B (the Cell Games as Hercule's win).
5. **New playables** (per character, ~1 session each): Krillin first.
6. **Animal quest** (1-2 sessions).
7. **Buu's Fury phase 2** (later): decompile pass, save conversion, handoff.

## 4. Rules we keep

- ROM and asset data never committed; code and tools only.
- The Arefu/Legacy reference is used for facts (addresses, formats), never copied.
- Every change tested in mGBA from a savestate; the wide build and the stock build stay in step.
- Story edits keep the original beats; Hercule gains, nobody else loses their moment.
