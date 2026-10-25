# How to: recipes for common jobs

Short, ordered recipes that point at the reference implementation for each kind of change. Copy the
newest example of the same kind, not the oldest. Before taking any new number (flag, stat, sprite,
row, portrait, ability, CoopCall, test command, map, memory), read and update `docs/ids.md`.

## Find something fast

| need | fastest route |
|---|---|
| what a stock map does (scenes, NPCs, triggers, flags) | `build/scripts/map_NNN_zZ_aA.txt` (the full script dump; regenerate with `python tools/script_dump.py`) |
| a map's exits, gates, triggers with their flag conditions | `python tools/mapexport.py Z A`, then `build/maps/zZaAv1.txt` |
| a Buu's Fury map: its name, size, exits, a render | `python tools/mapexport.py --buu --list` (all 452), `--buu Z A` (PNGs + .txt); the tournament grounds are `docs/maps.md` section 10 |
| what an engine function does | `build/decomp_all/<addr>_FUN_<addr>.c` (whole-ROM Ghidra corpus; grep it) |
| who calls / references an address | `tools/ghidra/FindRefs.java` (`docs/testing.md` "Ghidra headless") |
| an opcode's meaning | `docs/scripts.md` 2.1; the Arefu reference's `DBZKit/BytecodeVM_OpCodes.md` (facts only) |
| a number already in use | `docs/ids.md` |
| where a feature lives | `docs/INDEX.md` "Where to look for..." |
| what changed recently | `docs/log.md` (newest first), `git log --oneline -20` |

## Add a story chapter or scene

Reference: `data/story/ch9_gero.*` (newest, uses CoopCall, map edits, records, world-map icons).

1. Take a flag range from `docs/ids.md`; record each flag's meaning in `docs/scripts.md` 2.5.
2. Dump every map involved: `python tools/script_asm.py dump map Z A > build/story/zZaA.lscript`.
3. Write `data/story/NAME.lscript` (new lines as `text`; untouched ones as `stock` / `stockseq`) and
   `data/story/NAME.json` (map edits: triggers, records, on-load scripts; format in `docs/scripts.md` 4).
4. Add `data/story/NAME.json` to the `include` list in `data/hercule.json`.
5. `python tools/mapedit.py show data/story/NAME.json` (errors name the line), then build.
6. Test headless (below), then add a row to `docs/scripts.md` 5 and a recipe to `docs/testing.md`.

Rules that bite: one player controls the world map; no Hercule-hero reveal before the Buu phase;
the duplicate-NPC rule hides town NPCs of a character a player controls, but cutscene actors need
per-scene edits; a Map Test party has everyone in it, so check party flags by peeking.

## Call C from a script (CoopCall)

Reference: `src/coop/vmcall.c` (game action 59). Take the next number from `docs/ids.md`, add a
`case` and a header comment line in `vmcall.c`, a row in `docs/scripts.md` 2.2b, and use it in
lscript as `CoopCall N, a, b, c` (always three arguments; pass 0 for unused ones). Results are
pushed on the VM stack (`vm_push`).

## Add a boss, or change one

Reference: `src/coop/bosses.c` (the rules table) and `enemies.c` (both-down, rage, twins).

1. Take an extra stat id (`docs/ids.md`); add it to the stat table in `bosses.c` (stock stat + overrides).
2. Add its rule row: `end_pct`, partner sprite / stat, flags `BOSS_NO_TWIN`, `BOSS_PAIR`, `BOSS_RAGE`.
3. Place it with a map record in the chapter's `.json` (`"records"` / `"add_records"`).
4. Test: `COOP_CMD_SPAWN_RECORD` (8) spawns any record next to player 1; `COOP_CMD_STORY_WIN` (7)
   wins the map's fights; the fork autopilot recipe is in `docs/testing.md` "Boss pairs".

A boss that can fall first in a co-op pair needs pose group 3 (`docs/sprites.md` "Making a new enemy
model"). Android 19 absorbs ranged hits; fists must be melee-kind attacks.

## Add a sprite (enemy model, actor or player body)

Reference: `tools/brolysprites.py` (recolour and edit a stock set), `tools/a16sprites.py` (mix groups
from several sprites), `tools/buu2log2.py` (convert from Buu's Fury).

1. Take a sprite id (`docs/ids.md`). Write the generator under `tools/`; output goes to
   `build/gen/*.c` only (never commit ROM-derived data).
2. Makefile: a `NAME_SPRITE ?= id` variable, a generation rule, `$(NAME_SPRITE)=name_record` in
   `EXTRA_SPRITES`, and `-DNAME_SPRITE=` in `CFLAGS` if C needs the id.
3. Preview every group x 4 facings before testing; `COOP_CMD_ACTOR` (3) stands any sprite next to
   player 1. Document it in `docs/sprites.md`.

## Add a dialogue or status portrait

Portraits go through `tools/krillinsprites.py`'s `portrait_module` table. Another generator passes
its own with `--extra-portrait ID=SYMBOL` (add it to the Makefile's `EXTRA_PORTRAITS`; a change there regenerates the table through
`$(BUILD)/gen/portraits.stamp`, while a stale table shows a garbage portrait). Take the id from
`docs/ids.md`; the generator asserts that no two portraits share an id.

## Add a playable character

References: Super Saiyan 2 Gohan (`src/coop/ssj2gohan.c`, `docs/hercule.md` 5d: a stock sprite, a new slot,
a join function; every per-slot table it needed is listed in `docs/party-8-slots.md`), Krillin
(`src/coop/krillin.c`, `tools/krillinsprites.py`, `docs/hercule.md` 5b) and
Android 16 (`src/coop/a16player.c`, `tools/a16sprites.py` `a16p_record`, `docs/coop-design.md` "Gero's
bunker and Android 16 as a player").

1. A player-grade sprite (all player pose groups), a display row (`tools/spriteclone.py`), a party
   slot (`PARTY_SLOTS` is 9; slot 6 Krillin, slot 8 Super Saiyan 2 Gohan, slot 7 is the duel clone's, borrowed by 16 for his
   chapter), a status portrait, the name in `ui_strings`.
2. Abilities: append to `abilities` in `data/hercule.json` (entries are numbered from 14 in order)
   with `is_ready` / `on_use` C functions.
3. Player 2's class init re-runs when the row changes (`player.c` `p2_sync_combat`); a party member
   must have flag bits 0 and 1 (in party, available) to be bound.

## Add or change a map

Reference: `data/maps/broly_arena.json` (stock basin + new landing zone + a story's records).
Format and tools in `docs/maps.md` (8, 9). New areas live in zone 15; take the area from
`docs/ids.md`. Build previews land in `$(BUILD)/maps/`. Test with `COOP_CMD_WARP` (5).
Stock-map edits that only add triggers, exits or records go in a story `.json` through `mapedit.py`.

## Test

- **Headless (default for story):** `docs/testing.md` "Story chapters, headless". Build with
  `DEBUG_MENU=1` into a private `BUILD=` dir, make the debug-menu state once per build, run scripts
  with `python tools/headless/st_test.py SCRIPT ROM STATE`. Whole-chapter example:
  `tools/headless/ch9_gero.txt`. Test commands: `docs/ids.md` "Test commands".
- **Fork (real-time, fights, wide view):** `tools/fork/runfork.ps1 ROM -Slot n -Mute`; pick a slot no
  other session uses; never send keys to another session's window.
- **Savestates break across builds** (player 2's vtable copies hold code addresses): remake them.

## Work in parallel sessions

1. Before starting sessions, allocate each one a range of every number kind in `docs/ids.md`
   (flags, stats, sprites, rows, portraits, abilities, CoopCall, test commands, zone-15 areas) and
   commit that.
2. Each session builds into its own `BUILD=` dir, uses its own fork slot, commits on its own branch.
3. Merge hotspots (both sides almost always need keeping): `vmcall.c` switch and header, `coop.h`
   test commands, `bosses.c/.h` tables, the Makefile's `CFLAGS` and `EXTRA_SPRITES` lines,
   `data/hercule.json` include list, `tools/mapedit.py`, and the tables in `docs/log.md`,
   `docs/scripts.md`, `docs/testing.md`, `docs/INDEX.md`.
4. After merging, rebuild, then have each session rerun its own tests on the merged main.

## Decompile before guessing

`docs/testing.md` "Ghidra headless". The corpus in `build/decomp_all/` covers the whole ROM; decompile
a new address range with `DecompileFunctions.java`. Record each fact in the owning doc.
