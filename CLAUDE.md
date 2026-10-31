# Claude quick-start for this repo (read first, keep current)

**What this is.** A shiftable "decomp" of *Dragon Ball Z: The Legacy of Goku II* (GBA, USA `ALFE`)
turned into a two-player co-op mod, growing into **The Legacy of Hercule** (Hercule as the hero,
Super Saiyan Hercule, co-op difficulty, twin bosses, story edits). Ryan plays it with his brother.
Buu's Fury is a later phase. Nothing from the ROM is committed: code, tools and notes only.

## Start here (pick by task)

| task | read first |
|---|---|
| any task: where things are | `docs/INDEX.md` (wiki home, "Where to look for...") |
| taking any new number (flag, stat, sprite, row, portrait, ability, CoopCall, test command, map, RAM) | `docs/ids.md` (the registry; update it in the same commit) |
| doing a common job (chapter, boss, sprite, playable character, map, test, parallel sessions) | `docs/howto.md` |
| story questions or story work | `docs/story.md` section 0 (status) and 4g (latest decisions); stock-game facts in `docs/story-rom.md` and `build/scripts/`; why things changed in `docs/story-archive.md` |
| scripts, dialogue, flags, CoopCall | `docs/scripts.md` |
| testing anything | `docs/testing.md` (headless runner first; fork for real-time and fights) |
| what landed recently | `docs/log.md` (newest first), `git log --oneline -20` |

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
- Several Claude sessions often share this machine. Never send keys to another session's mGBA
  window: drive only your own fork slot (window title `mGBA` for slot 0, `mGBA-n` for slot n). Build
  into a private directory (`make BUILD=build/<name> ...`) so object files and `coop.sym` are not
  swapped under each other. Commit only your own files (`git add <paths>`, never `-A`).
- Decompile from the ROM (Ghidra headless) before guessing at engine behaviour; it is the fastest
  way to a fact. Record each new fact in the relevant `docs/*.md`.
- Keep these docs current: when a build flag, tool, address, id or test recipe changes, update
  `docs/INDEX.md`, `docs/ids.md` and the owning doc in the same commit.

## Current phase (2026-10-25): the tournament is in Ryan's playtest rounds

The World Tournament on Papaya Island is built and merged (`docs/tournament.md`: the plan, 9b-13 the seven-piece
build, 14-23 every playtest round since; 24 the open points and the plan to fix them: **the next session starts
there, after Ryan's playtest notes**; 24.10 is the team combos, decided with him on 2026-10-31: the next thing to build after his playtest notes). Contract: `src/coop/tourney.h`; tests
`tools/headless/tny_*.txt`. Since the build: overhead HP bars in the Rumble (`tny_hpbars.c`), real lying and
knock-down poses, knock-back, the fallen stay down, Energy Steal (ability 24), playables built on the party's best
numbers, your pick never drawn as an opponent, every scene change behind a fade (the "curtain"), and the fighters'
dialogue: `docs/tournament-dialogue.md` is both Ryan's approved plan and the game's data (`tools/tnytalk.py` reads
its tables at build time, so edit a line there to change it in the game; `tny_talk.c` picks and shows them).
Full regression: `bash tools/headless/regfull.sh build/<dir>` (`docs/testing.md` "Full regression"). Drop-ins F8
(Papaya Island, early) and F9 (waiting room, every tier open) carry a powered-up party (`dropins.py` `boost`).
Swapping a build into `build/play`: if mGBA's window lingers after Ryan closes it, close it with
`CloseMainWindow` (never kill it), back up the `.sav` and the `.ss1-.ss9` into `build/play/backup/` first.

## Earlier phase (2026-10-21): Ryan is playtesting; feedback rounds land as they come

Built and merged: every chapter through the ten-day wait, the Cell Games, the post-game (wishes, the
Arctic, Super 13, Peace Times, the ending), the journal entries, reviving a downed partner, the
difficulty (Easy / Normal / Hard at a new game), one-player play and player 2 dropping in and out. All
tested headless in both modes. Ryan plays alone through the drop-in savestates (F1-F7 in mGBA) in
`build/play`; after each change: rebuild (`make` for the play ROM, `make BUILD=build/<dir> DEBUG_MENU=1`
for the debug ROM and its `menu.ss`), rerun `tools/headless/dropins.py`, back up his `.sav` into
`build/play/backup/`, copy the ROM and `.ss1-.ss7` over (mGBA closed; he saves and closes it first),
update `build/play/DROP-INS.txt`, reopen mGBA on the play ROM. Status, the feedback done so far and the
offered-but-not-taken polish: `docs/story.md` section 0. Next: more feedback, then the Buu's Fury phase,
planned in chat. He wants a slow pace, one decision at a time, answers from the
player's point of view grounded in ROM facts, and no suggestions he did not ask for. Do not build a part
of the story until he says its plan is done. Nothing may reveal Hercule as the hero before the Buu phase.
Lines Ryan dictated stay word for word. Facts about the game come from the script dump
(`build/scripts/`), the quest table (reference notes) and Ghidra.

## Build (Windows, Git Bash)

```
export PATH="/c/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/12.2 mpacbti-rel1/bin:/c/Program Files (x86)/GnuWin32/bin:$PATH"
make                                  # build/log2-coop.gba (HERCULE=1 data patch on by default)
make WIDE=1 DEBUG_MENU=1              # build/log2-wide.gba for the WSL fork, debug menu on the title screen
make BUILD=build/<name> DEBUG_MENU=1  # private dir: what every test session should use
```

`make` does not run from PowerShell (GnuWin32 make needs sh) and WSL has no ARM toolchain; use Git
Bash with the PATH above. Python one-offs that read untrusted files run with `python -I`.

| flag | effect |
|---|---|
| `HERCULE=0` / `DATA_PATCH=` | plain co-op / skip the data patch (story, abilities, growth) |
| `PARTY_SLOTS=N` | party size (default 9: slot 7 is the duel clone's, slot 8 Super Saiyan 2 Gohan; 8 still builds without him); everything after the entries (story flags, saves) moves with it; the check allows 6..16, but 16 overflows `partyram` and does not link (`docs/party-8-slots.md`) |
| `WIDE=1`, `WIDE_TEST=<w>` | wide view for the fork / force a view width |
| `DEBUG_MENU=1` | 30 s idle on the title opens the debug menu instead of the intro video (about 75 s from power-on); test a new game's own flow with 0 |
| `BUU=0` | leave out the sprites converted from Buu's Fury (they need `roms/buu.gba`) |
| `P2_MODE` | player 2's input transport |
| `SSJ_TEST=1`, `A16_TEST=1`, `KR_TEST=1`, `SSJ2_TEST=1`, `STORY_TEST=1` | test triggers and autopilots (`docs/testing.md`) |
| `*_SPRITE`, `*_PORTRAIT` | ids of generated art (defaults and owners in `docs/ids.md`) |

## Test (full recipes in docs/testing.md)

- **Headless (story chapters, most checks):** `python tools/headless/st_test.py SCRIPT ROM STATE`
  (`docs/testing.md` "Story chapters, headless"; a whole-chapter example is `tools/headless/ch9_gero.txt`).
- **WSL fork (real time, fights, wide view):** `tools\fork\runfork.ps1 /mnt/f/.../x.gba -Slot n -Mute
  [-State /mnt/f/.../x.ss1]`, GDB on port 2345+n, pad on 7778+n; slot n's helpers install into
  `/root/fork-sn` (`FORK_SLOT=n`); see `tools/fork/` (persistent GDB client, pad keys, teleport).
- **Windows mGBA (stock ROM):** `tools\mgba_run.ps1`, Lua helpers in `tools/mgba/`, TCP command server
  `tools/mgba_cmd.py`.
- **Test commands** (write `coop2.cmd` 0x0203FD08, argument at 0x0203FD0C): warp, spawn a record,
  win the map's fights and more; the full list is in `docs/ids.md`.
- Savestates break after every rebuild of the same ROM name (player 2's vtable copies hold code
  addresses): remake them.

## Key addresses (all of them: docs/ids.md "Memory"; structs: src/coop/rom.h)

Party block `g_party_block` 0x0203F000 (entries at +0xC, 0x1C each; story flags at 0x0203F108 + n/8 with 9 slots,
`0x0203F00C + PARTY_SLOTS * 0x1C`: headless scripts use the runner's `FLAGS`);
co-op state `coop` 0x0203FF04 (`p2` +4, `p2_slot` +0x80); `coop2` 0x0203FB00 (must stay <= 0x2C0
bytes); player-2 vtable 0x0203FE00; wide descriptor 0x0203FEC0; entity list 0x03001120; input
0x0300229C; sprite index 0x083B4E74 (relocated); ability table literal 0x08008024.
