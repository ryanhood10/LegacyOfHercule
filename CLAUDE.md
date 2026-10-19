# Claude quick-start for this repo (read first, keep current)

**What this is.** A shiftable "decomp" of *Dragon Ball Z: The Legacy of Goku II* (GBA, USA `ALFE`)
turned into a two-player co-op mod, growing into **The Legacy of Hercule** (Hercule as the hero,
Super Saiyan Hercule, co-op difficulty, twin bosses, story edits). Ryan plays it with his brother.
Buu's Fury is a later phase. Nothing from the ROM is committed: code, tools and notes only.

**Map of everything:** `docs/INDEX.md` (wiki home). Engine facts: `docs/engine.md`, `docs/rom-map.md`.
Co-op design and verified findings: `docs/coop-design.md`. Game plan: `docs/plan-legacy-of-hercule.md`.
Story: `docs/story.md`. Testing recipes: `docs/testing.md`. Progress log: `docs/log.md`.

## Rules (Ryan's, standing)

- Commit autonomously and keep working; never stop to ask whether to commit. End commit messages
  with the `Co-Authored-By:` line your session's attribution reminder gives (it names the model).
- Push `main` to `origin` (github.com/ryanhood10/LegacyOfHercule) after committing. Side sessions
  commit on their own branches and never push main; the main session merges them (if the checkout
  is dirty with another session's edits, merge in a temporary worktree and push from there).
- Never commit ROM or asset data: `roms/` and `build/` are ignored; generated sprite/data files
  (`build/gen/*`, `build/*.patch.json`) stay in `build/`. Dialogue *edits* in `data/` are ours.
- The Arefu/Legacy reference clone (`F:\MeltyCreations\tools\downloads\arefu-legacy`, no licence)
  is for facts and names only; never copy code or data from it.
- Never change Windows security settings (Smart App Control is on; it blocks unsigned new binaries
  such as `ld.exe` and any mGBA we build on Windows, which is why the fork lives in WSL).
- Two Claude sessions often share this working tree. Never SendKeys to another session's mGBA
  window; this session only drives the WSL fork window titled `mGBA (Ubuntu)`. Build into a
  private directory (`make BUILD=build/<name> ...`) so object files and `coop.sym` are not swapped
  under each other. Commit only your own files (`git add <paths>`, never `-A`).
- Decompile from the ROM (Ghidra headless) before guessing at engine behaviour; it is the fastest
  way to a fact. Record each new fact in the relevant `docs/*.md`.
- Keep these docs current: when a build flag, tool, address or test recipe changes, update
  `docs/INDEX.md` / the owning doc in the same commit.

## Current phase (2026-10-18): story first

Ryan is designing the story and co-op casting chapter by chapter in chat; `docs/story.md` section 0
is the summary, the standing rules and the list of mechanics the story will need. Do not start
production story code until Ryan says the plan is done; engine pieces he explicitly started run in
side sessions on their own branches. Nothing may reveal Hercule as the hero before the Buu phase.
Facts about the game come from the script dump (`tools/script_dump.py`, full dump in
`build/scripts/`), the quest table (reference notes) and Ghidra; check them before answering a
story question. `docs/log.md` says what landed last.

## Build (Windows, Git Bash)

```
export PATH="/c/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/12.2 mpacbti-rel1/bin:/c/Program Files (x86)/GnuWin32/bin:$PATH"
make                       # build/log2-coop.gba (HERCULE=1 data patch on by default)
make WIDE=1 DEBUG_MENU=1   # build/log2-wide.gba for the WSL fork, debug menu on the title screen
make BUILD=build/ssj SSJ_TEST=1 DEBUG_MENU=1 WIDE=1 DATA_PATCH=   # private dir, P2 Start = transform
make BUILD=build/p6 PARTY_SLOTS=6                                  # stock-sized party (default 8)
```

Flags: `HERCULE=0` plain co-op; `DATA_PATCH=` skips the data patch; `PARTY_SLOTS=N` party size (6..16);
`WIDE_TEST=<w>` forces a view width; `P2_MODE` input transport; `SSJ_TEST=1` test trigger; `BUU=0` leaves out
the sprites converted from Buu's Fury (ids 158 Hercule, 159 Broly; `BUU_SPRITES`, `SSJ_SPRITE=158` for a test
build). `A16_SPRITE` (160) is Android 16's boss sprite; `A16_TEST=1` = player 1's L+R+Down spawns the
Android 16 boss (enemy stat 109) and enables the fight autopilot (`docs/testing.md`). `A16P_SPRITE` (175) is Android 16's player record (row 25, last party slot in the "Gero's computer" chapter, `src/coop/a16player.c`). `KRILLIN_SPRITE` (161) is Krillin's
playable sprite; `KR_TEST=1` = Krillin / old Hercule test triggers on player 2's pad (`docs/testing.md` "Krillin"). `STORY_TEST=1` = chapter-test build (fork TCP pad drives player 1, ROM story-flag preset; `docs/testing.md`). `A13_SPRITE` (165) / `NAMEK_SPRITE` (166) are
Android 13 and Cooler's Namekian servant (`tools/bossmodels.py`; boss rules in `src/coop/bosses.c`). `PARAGUS_SPRITE` (170) / `SHIP_SPRITE` (171) / `PARAGUS_PORTRAIT` (120) are the Broly chapter's art (`tools/brolysprites.py`; Broly 159 and the ship need `roms/buu.gba`). `make` does not run from PowerShell
(GnuWin32 make needs sh) and WSL has no ARM toolchain; use Git Bash with the PATH above.

## Test (see docs/testing.md for the full recipes)

- Stock ROM in Windows mGBA: `tools\mgba_run.ps1` (runs a copy; mGBA locks the file), Lua helpers
  via `tools/mgba/coop.lua`, pad sender `python tools\p2_keys.py`, TCP command server for
  focus-free tests (`tools/mgba_cmd.py`).
- Wide ROM in the WSL fork: `wsl -d Ubuntu -u root -- bash -c "cd /root/mgba-build/sdl && MGBA_P2=1 MGBA_P2_PORT=7778 ./mgba -2 -t <state> <rom>"`,
  sender `python tools\p2_keys.py --port 7778`, keys `tools\win_keys.ps1 "M:200 WAIT:500" -Title "mGBA (Ubuntu)"`,
  screenshot `tools\win_shot.ps1 out.png -Title "mGBA (Ubuntu)"`. Add `-g` and attach
  `gdb-multiarch -batch -x <script>` for breakpoints, traces and memory dumps (works; see testing.md).
- Story chapters (`data/story/`, `docs/scripts.md`) are tested headless: `tools/headless/st_headless.c` on the
  fork's libmgba, `python tools/headless/st_test.py SCRIPT ROM STATE` (`docs/testing.md` "Story chapters, headless").
- `DEBUG_MENU=1` also replaces the new-game intro video with the debug menu, so test new games with
  `DEBUG_MENU=0`. Fork test helpers (persistent GDB client, pad-word keys, teleport): `tools/fork/`.
- Savestates: `build/log2-coop.ss1` = zone 2 area 7 wolf map, level-40 party, Piccolo P1 (slot 1),
  Gohan P2 (slot 0); `ss2` = prologue Flight Circle. Re-make states after every `make` of the same
  ROM name (player 2's vtable copies hold code addresses).

## Key addresses (details in docs/engine.md and src/coop/rom.h)

Party: stock `0x03000E90`, relocated to `g_party_block` `0x0203F000` with `PARTY_SLOTS` (8) x 0x1C
entries at +0xC (entry +8 = display row; `docs/party-8-slots.md`); Character_DisplayData 22 rows x 20
(relocated into the blob as `display_rows`, row 22 = SSJ Hercule, row 23 = Krillin, row 24 = old Hercule, row 25 = Android 16);
sprite index `0x083B4E74` (relocated as `sprite_index`, id 157 = SSJ Hercule, 158+ from Buu's Fury, 160 = Android 16 boss, 161 = Krillin, 165 = Android 13, 166 = Cooler's Namekian, 175 = Android 16 as a player;
Buu ROM: index `0x086B6BDC`, OBJ palette `0x0805652C`, see `docs/sprites.md`); entity list
`0x03001120`; world/camera object `0x03001090`; input `0x0300229C`; co-op state `0x0203FF04`;
player-2 vtable `0x0203FE00`; wide descriptor `0x0203FEC0`; ability table literal `0x08008024`.
