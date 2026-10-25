# Testing recipes

Two emulators, two ROM builds. The stock co-op ROM (`build/log2-coop.gba`) runs in the Windows
mGBA (Qt, Lua scripting works there). The wide ROM (`build/log2-wide.gba`) needs the fork in
`tools/mgba-wide`, built and run inside WSL Ubuntu (Smart App Control blocks it on Windows); the
fork's Lua runtime never fires frame callbacks, so player 2 goes in through a native TCP pad and
inspection goes through the GDB stub.


**Recipes on this page** (pick the first that fits; story work is almost always headless):

| section | use it for |
|---|---|
| Story chapters, headless | any story chapter or scene: scripted input, Map Test with flag presets, screenshots, RAM peeks |
| Gero's computer / Broly chapter / Time Chamber waves and Roshi's sparring | the three ten-day-wait features, end to end |
| Hercule's beam pose | a sprite pose seen in a stock scene, other facings, player 2 in a control state |
| Several fork instances at once (slots) | running the WSL fork beside other sessions |
| WSL fork (wide ROM), Fork helpers | real-time play, the wide view, gdb peeks, pad keys, teleport |
| Boss pairs, Android 16 boss fight, Super Android 13 | fights with the autopilot and hit logs; Super 13's specials, rage, explosion, rates against the twin Cells |
| Krillin / old Hercule tests | the playable-character test triggers |
| New maps (zone 15) | building and warping to the new maps |
| Arctic route (headless) | the post-game arctic: world-map landing and star, every exit, the save pad, OBJ VRAM |
| Tournament grounds (headless) | Papaya Island: the world-map landing, the welcome, both pads, every exit, the board, the director, the waiting room's fighters |
| GDB stub | crash causes ("Jumped to invalid address"), breakpoints, traces |
| Windows mGBA (stock ROM) | the stock-size ROM with Lua helpers |
| Savestates, Reading screenshots, Ghidra headless | supporting tools |

Test commands (warp, spawn, win a fight) and RAM words: `docs/ids.md`.

## Savestates

`build/log2-coop.ss1`: zone 2 area 7 (wolf map, debug Map Test), level-40 party, Piccolo is
player 1 (active slot 1), Gohan is player 2 (slot 0); Hercule is slot 5. `ss2`: prologue Flight
Circle. A state made with build A crashes under build B as soon as player 2 acts (its vtable copy in
EWRAM holds code addresses), so re-make states after rebuilding the same ROM name; the fork reports
"Savestate is for a different version" and still loads it. The debug menu (`DEBUG_MENU=1`) appears
after 30 s idle on the title (1800 frames, checked at 0x08000434), about 75 s from power-on; its Area number is the index inside the zone's entry block.

## Windows mGBA (stock ROM)

```
tools\mgba_run.ps1 [-NoStates]        # restarts mGBA on build\log2-run.gba (mGBA locks the file it runs)
python tools\p2_keys.py               # player 2: IJKL move, O=A, U=B, [ ]=L R, N=Select, M=Start
python tools\mgba_cmd.py "seq('DOWN:4 WAIT:20 A:4')"   # focus-free pad/state/screenshot commands
```

A second instance beside another session's: start `mGBA.exe <your copy>` yourself (never
`mgba_run.ps1`, which kills every mGBA), load `cmd_server.lua` with `cmd_server_port = 7781` set first
(and a frame callback writing `p2_force` to `0x0203FF00` instead of `coop.lua`, whose bridge binds
7777), then `mgba_cmd.py --port=7781`. Lua typed through PowerShell loses double quotes: use single
quotes. `mgba_script.ps1` clicks Tools > Scripting by client coordinates, so the window must be in
front and unobstructed (pin it topmost first); it also fails if `mgba_window.ps1` was dot-sourced
earlier in the same process. The savestate you load may have the Dragon Radar open: `seq('START:4')`.

Console helpers (Tools > Scripting, `dofile(".../tools/mgba/coop.lua")`): `peek()` co-op state,
`ents()` entity list, `p2_force = 0x8001` pad override, `spd()`, `lowhp()`, `actor(n)`. Write Lua
with single quotes only when it goes through PowerShell. Another session may own this window: do not
SendKeys to it; use the TCP command server.

## WSL fork (wide ROM)

```
wsl -d Ubuntu -u root -- bash /mnt/f/MeltyCreations/log2/tools/mgba-wide/build-wsl.sh      # once / after fork edits
wsl -d Ubuntu -u root -- bash -c "cd /root/mgba-build/sdl && MGBA_P2=1 MGBA_P2_PORT=7778 ./mgba -2 -t /mnt/f/MeltyCreations/log2/build/log2-coop.ss1 /mnt/f/MeltyCreations/log2/build/ssj/log2-wide.gba > /root/mgba-runN.log 2>&1"
python tools\p2_keys.py --port 7778                                   # keep one sender running; it reconnects
tools\win_keys.ps1 "N:200 WAIT:1200 M:200 WAIT:500" -Title "mGBA (Ubuntu)"   # keys go to the focused window AND the global sender
tools\win_shot.ps1 build\x.png -Title "mGBA (Ubuntu)"
```

Before launching, stop the stale instance of *your* slot (`runfork.ps1 -Slot n` does): a stale instance
keeps its ports and the new one silently runs without pad or debugger. Never `pkill -x mgba` while other
sessions run forks. The log says `p2pad: sender connected` when the pad is live.
Select cycling for player 2 registers only every few presses; verify the slot instead of counting.

## Several fork instances at once (slots)

`tools/fork/runfork.ps1 <rom> [-Wait s] [-Slot n] [-State /mnt/f/.../x.ssN]` launches instance n with
`MGBA_GDB_PORT` 2345+n, `MGBA_P2_PORT` 7778+n and window title `mGBA-n` (slot 0 keeps 2345 / 7778 /
"mGBA"), stops only that slot's previous instance, installs `tools/fork/*.sh|*.gdb` into `/root`
and attaches the persistent gdb client. It installs the helpers of the checkout it lives in
(`install.sh`, `FORK_TOOLS`): slot 0 into `/root`, slot n into its own `/root/fork-sn`, so another
session reinstalling `/root/*.sh` never swaps scripts under a running slot; slots >= 1 run as
`mgba-sn` and `gdb-sn` (symlinks), out of reach of old helpers' `pkill -x mgba` /
`pkill -INT -x gdb-multiarch`. Every helper takes the slot from `FORK_SLOT` (`tools/fork/slot.sh`:
ports, `/tmp/gdbin-n`, `/root/gdb-n.log`) and finds its siblings next to itself, e.g.
`wsl -d Ubuntu -u root -- bash -c "FORK_SLOT=1 /root/fork-s1/peek.sh"`. Screenshots by title:
`tools\win_shot.ps1 out.png -Title "mGBA-1"` (slot 0: `-Title "mGBA (Ubuntu)"`). Player-2 pad:
`python tools/fork/p2hold.py --port 7779` for slot 1. Claim a slot before using it (check
`ss -ltn | grep 234` in WSL); sessions running in parallel should each use their own.

Finding entities with gdb: the scene list is `g_EntityList` 0x03001120, `count` at +4 and the item
pointers inline from +8; co-op enemies have vtable 0x0203FB04 (`coop2.enemy_vtable`).

**Audio sync stall:** with several forks running, a fork launched with sound can stall (a boot stuck on
the copyright screen for minutes, keys ignored). `runfork.ps1 ... -Mute` passes `-C mute=1 -C
audioSync=0` and runs at full speed (`-Quiet` and `-NoAudio` are the same switch); a state loaded with
sound on crawled at about 1 frame per second. **gdb "Disconnected from target"**: a SIGINT that reaches
gdb while it still waits for the stop reply of the previous one makes it give up and disconnect, and
the stub never accepts a client again for that run. Under load (several forks) the stub answers late,
especially while the CPU idles in the BIOS, so back-to-back `pause`s did it. `gdbsrv.sh pause` now
waits (up to 8 s) for the stop before returning; with that, keys.sh and q.sh cycles run for hours.
gdb's output reaches the log late (the line of one query can arrive after the next command), so
`q.sh` tags its `Q:` line and waits for that tag.

## Boss pairs (fork; `docs/coop-design.md` "Android 13, the Namekian servant and the boss-pair tests")

```
make BUILD=build/bt WIDE=1 DEBUG_MENU=1 A16_TEST=1              # A16_TEST: the autopilot fights any pair
tools\fork\runfork.ps1 /mnt/f/.../build/bt/log2-wide.gba -Slot 3 -Mute -Wait 1
python tools/fork/p2hold.py --port 7781 --cmd build/bt/p2cmd.txt    # keep running: player 2 present
wsl -d Ubuntu -u root -- bash -c "FORK_SLOT=3 /root/fork-s3/bootisland.sh 75"   # boot -> island, level-30 pair placed
tools\win_keys.ps1 "SHIFT+F1:300" -Title "mGBA-3"                  # save; F1 reloads it before each fight
wsl -d Ubuntu -u root -- bash -c "FORK_SLOT=3 /root/fork-s3/bossfight.sh 0x083FD3BC 150 10"   # 19 + 13
```

`bootisland.sh` waits for the debug menu (about 75 s from launch when muted), opens Map Test 11/6
through the pad word, lets player 2 join, writes the level-30 stats of the Android 16 tests (Piccolo
slot 1, Hercule slot 5) and places player 1 at (89, 347) and player 2 at (200, 330), either side of
where a spawned pair stands. `bossfight.sh RECORD [SECS] [STEP]` writes `COOP_CMD_SPAWN_RECORD` (8; 7 is `COOP_CMD_STORY_WIN`) with
the record address (any map's boss record: 19 `0x083FD3BC`, Gero `0x083FF970`, Cooler `0x083FB574`,
17 `0x08406494`), turns on the autopilot (`coop2.a16_bot`, +0x244) and prints both bosses' and both
players' HP every STEP seconds until both are down or a player's HP is 0. `bosstest.sh` peeks the pair,
sets a boss's HP (`hp orig|twin N`: drop both low to watch the both-down order and the defeat
script), `god`, `partner SPRITE` (twin model override), `off BITS`. Runs from the same state are
deterministic (identical numbers), so vary the start to sample. The autopilot walks to its boss and
punches, so results are a lower bound on two people.

## New maps (zone 15, `docs/maps.md`)

```
make BUILD=build/maps WIDE=1 DEBUG_MENU=1                       # MAPS=1 by default: builds data/maps/*.json
tools\fork\runfork.ps1 /mnt/f/.../build/maps/log2-wide.gba -Slot 4 -Mute
python tools/fork/p2hold.py --port 7782 --cmd build/maps/p2cmd.txt   # player 2 present; write "0x40:2" etc. to the file
wsl -d Ubuntu -u root -- bash -c "FORK_SLOT=4 /root/warp.sh 15 1 216 200"   # arctic (2: 312 280 Time Chamber, 3: 276 354 arena, 4: 240 340 crater)
```

The debug Map Test lists zone 15 too (zone field: Right x14 from 1). A savestate taken at the debug
menu (Shift+F1) loads on any rebuild with the same code, so map-data changes do not need a new boot.
Offline check of a built ROM: `python tools/mapexport.py 15 1 --rom build/maps/log2-wide.gba --table
<table address printed by patch_rom.py> --count 330`.

## Arctic route (headless, 2026-10-19)

```
make BUILD=build/arc DEBUG_MENU=1
python tools/headless/st_test.py tools/headless/mkmenu.txt build/arc/log2-coop.gba --out build/arc/hl
python tools/headless/st_test.py tools/headless/arctic_route.txt build/arc/log2-coop.gba build/arc/hl/menu.ss --out build/arc/hl --cols 6 > build/arc/hl/route.out
python tools/headless/objfree.py build/arc/hl/route.out
```

`arctic_route.txt` (co-op from the landing on): flag 381 clear, A on the arctic landing does nothing and
the minimap has no icon there; flag 381 set, the star (shot with the flyer away from it), A lands on the
flight spot of zone 15 area 1; then the cave, both climbs (`ents`: every record and its twin), the ridge's
pad (Save / Switch Characters, "Game Saved!"), the summit, every exit back down, and both flight spots
back to the world map at (1100, 800). Pass: each `waitmap` reached, the `map` lines at the arrival spots
of `docs/maps.md` 9a, no CRASH line. `objfree.py` prints the OBJ VRAM free list (entity list + 0x694:
10-byte nodes, +4 u16 size in 64-byte units, +8 next; head = byte 0 of 0x03001120, `0x08006D71` reads
the same) for each dump: 15.6-15.9 KB and 18.2 KB largest free on the two climbs.

- Warps: `COOP_CMD_WARP` into a field map shows the player about 300 frames later (a `map` line reads
  -1, -1 until then); waiting less makes the next key press or warp go nowhere. The dim frames on
  arrival are the fade under the map-name banner.
- Press-A spots (pad, flight spots) after a warp: step into the rect first (`key 0x40 20`), then A.
- The star with flag 382 set turns into the plain marker (`flags +382`, re-warp to the world map).
- **Continue from the ridge save:** the runner's `reset` power-cycles and keeps the save data: after
  "Game Saved!", `reset`, `key 8 6` twice (intro), Start, A on the slot (the Select Game screen names it
  "THE ARCTIC"), A: the game resumes on the ridge's pad.
- Map previews: `build/arc/maps/arctic*.png` (and `_walk.png`), written by every build.

## Fork helpers (`tools/fork/`, used for the party-slots work)

The stub accepts one client per emulator run: a second `target remote` never completes, and a client
that quits while attached leaves the stub writing to a dead socket (SIGPIPE kills mGBA unless the
launcher ignores it). So keep one gdb attached for the whole run:

```
tools\fork\runfork.ps1 /mnt/f/MeltyCreations/log2/build/log2-coop.gba -Wait 14   # kill old, launch with -g + trap '' PIPE, attach, continue, wait
wsl -d Ubuntu -u root -- bash -c "/root/peek.sh"            # pause, dump party / player / EEPROM slots (peek.gdb), continue
wsl -d Ubuntu -u root -- bash -c "/root/keys.sh 8 0.25"      # hold Start 0.25 s through g_p1_remote (0x0203FF02): A=1 B=2 Sel=4 Start=8 R/L/U/D=0x10/20/40/80 R=0x100 L=0x200
wsl -d Ubuntu -u root -- bash -c "/root/tele.sh 544 215"     # teleport player 1 (pixels)
```

`runfork.ps1` installs the `.sh`/`.gdb` helpers itself (CR stripped; slot n into `/root/fork-sn`). `keys.sh`
pauses, writes the pad word, continues, sleeps, releases: the title, the debug menu, the Select Game
menu and in-game movement take it; the save pad's Save/Switch dialog, the switch screen and the pause
menu do not (they read the pad during a modal loop), so drive those with `win_keys.ps1` (arrows,
X=A, Z=B, A=L, S=R, Enter=Start, Backspace=Select; 250-300 ms holds register, shorter ones drop).
`win_keys.ps1` / `win_shot.ps1` restore a minimised window. `reset.sh` jumps the CPU to the ROM
entry: EWRAM survives, so the co-op state's stale player-2 pointer crashes the next map; prefer a
relaunch (the `.sav` keeps the EEPROM). mGBA starts the EEPROM as a 512-byte device and zero-extends
it on the first wider access, so bytes beyond 0x200 read as zero until written. A `DEBUG_MENU=1` ROM
turns a new game into the debug menu (the patch replaces the intro video). A stale
`tools/p2_keys.py` from another session reconnects to the fork's port 7778 and injects player-2
buttons; kill it before testing.

## Android 16 boss fight (fork, scripted; `docs/coop-design.md` "Android 16 boss")

Build and helpers (merged into main 2026-10-18; the paths below that name the old `log2-a16` worktree now live in this checkout, and its savestate must be re-made with your own build):

```
make BUILD=build/a16 WIDE=1 DEBUG_MENU=1 A16_TEST=1        # Git Bash, toolchain PATH as above
wsl -d Ubuntu -u root -- bash -c "tr -d '\r' < /mnt/f/MeltyCreations/log2/tools/fork/a16install.sh | bash"   # copies a16*.sh/.gdb to /root
python tools/fork/p2hold.py --cmd build/a16/p2cmd.txt      # keep running (background): player 2 present, scripted pad
```

Savestate `build/a16/a16_base.ss1` (made with this build; re-make after a rebuild if player 2
misbehaves): island zone 11 area 6 (debug Map Test 11/6, no triggers, 17/18/16 actors stand
around), Piccolo player 1 (slot 1), Hercule player 2 (slot 5), both at level-30 stats: party
entries 0x0203F028 (Piccolo: hp 554, str 11955, pow 12750, end 10140, exp 382750) and 0x0203F098
(Hercule: hp 245, str 12030, pow 11650, end 16130), and the entity combat bytes written by hand
(player 1 entity +0x160 = 46 49 39, player 2 = 47 45 63; player 2's are otherwise player 1's, a bug).
To re-make it: boot the ROM, leave the title idle ~100 s for the debug menu, Map Test zone 11
area 6 (Down picks the field, Right increments; hold keys 0.3 s), A; start `p2hold.py`, nudge
player 1 so player 2 spawns (Hercule), poke the values above with gdb, then Shift+F1 in the window
(`tools\win_keys.ps1 "SHIFT+F1:300" -Title "mGBA (Ubuntu)"`) -> `build/a16/log2-wide.ss1`.

Run a fight (Git Bash; `MSYS_NO_PATHCONV=1` stops Git Bash rewriting `/mnt/...` and `/root/...`):

```
MSYS_NO_PATHCONV=1 powershell -NoProfile -File tools/fork/runa16.ps1 -State /mnt/f/MeltyCreations/log2/build/a16/a16_base.ss1
MSYS_NO_PATHCONV=1 wsl -d Ubuntu -u root -- bash -c "/root/a16fight.sh 0 1 300 10"   # FORCE BOT SECS STEP
MSYS_NO_PATHCONV=1 wsl -d Ubuntu -u root -- bash -c "/root/a16peek.sh"               # one readout
```

`a16fight.sh` spawns 16 (writes COOP_CMD_A16 = 4 to coop2.cmd at 0x0203FD08), sets the forced
special (0x0203FD40: 1 homing, 2 barrage, 3 bombs, 4 punch, 0 random) and the autopilot
(0x0203FD44), logs every hit a player takes and every hit 16 takes (`a16hits.gdb`, to
`/root/a16hits.log`), and stops at a KO. In game without gdb: player 1 holds L + R and presses Down
(`A16_TEST` builds). Readout fields: coop2 0x0203FB00 + 0x234 a16 entity, +0x238 fired/explosion
sprites, +0x23C blasts that hit/skipped art, +0x244 bot/specials, +0x248/+0x24C spawn/KO frame.

Notes: the fork's p2pad writes 0 to the player-2 pad word while no sender is connected, so a gdb
write of 0x8000 does not keep player 2 present (use `p2hold.py`). P1's Select opens the Dragon Radar.
`tools/fork/a16shots.sh` (pause, screenshot, continue) made the fork exit twice; plain
`win_shot.ps1` captures while running work.

## Super Android 13 (headless and fork; `docs/coop-design.md` "Super Android 13")

```
make BUILD=build/s13t DEBUG_MENU=1 A16_TEST=1                     # A16_TEST: the fight autopilot knows him
python tools/headless/st_test.py tools/headless/mkmenu.txt build/s13t/log2-coop.gba --out build/s13t/hl
python tools/headless/st_test.py tools/headless/super13.txt build/s13t/log2-coop.gba build/s13t/hl/menu.ss --out build/s13t/hl --cols 6
python tools/headless/bossfight.py build/s13t/log2-coop.gba build/s13t/hl/menu.ss s13 --strong --refill --delay 13 --delay 37 --secs 150 --out build/s13t/hl/fight
python tools/headless/bossfight.py build/s13t/log2-coop.gba build/s13t/hl/menu.ss cell --strong --refill --delay 13 --secs 150   # the twin Perfect Cells
```

`super13.txt` (Map Test 147, warp to zone 15 area 3, both players at 9000 HP so standing still is safe)
spawns him with test command 17, forces each special in turn with test command 18 (1 volley, 2 fan, 3
bomber, 4 wave, 5 nova), sets his HP to 3000 of 10000 (`arg` 0x100, `cmd_arg2` 3000: the roar, then rage bombers),
peeks `g_FrameCounter` across 600 frames of rage (equal = no lag), then gives him 1 HP and runs
`COOP_CMD_STORY_WIN` (7): light bursts out of him, explosions, the big blast, then he is gone (`s13.state` 3,
`gone_frame - dead_frame` = 96). The `s13` peek (11 words, found by its magic word 0x53313333 "S133";
its address moves with the `partyram` sections before it): magic, boss, fired / explosion sprites, hits / skipped (OBJ VRAM), specials / combos, force / state / hp% /
dmg%, spawn / dead / gone frames, player attacks received / landed, least largest-free OBJ VRAM block.

`bossfight.py ROM STATE KIND` (KIND `s13`, `cell` = record 0x084090F8, or any record address) runs Map Test,
warps (`--map Z,A,X,Y`, default New Planet Vegeta), spawns, puts player 2 on the boss's far side (the bot walks
straight at its spot and a giant blocks it), turns on the autopilot and logs HP every second. `--strong` gives
both players Cell Games-level stats (Piccolo 1500 HP, 72 / 75 / 62; Hercule 900 HP, 72 / 70 / 84),
`--refill` puts them back to full after every reading (the autopilot never retreats or dies, so the fight
runs on and the drops add up to the damage taken), and the summary is the rates over the active seconds:
players' damage per second on the boss, the estimated time to kill, the boss's damage per second.
`--delay` (frames idled before the spawn) varies the RNG between otherwise identical runs. Reads the `s13`
address from the build's `coop.sym`.

The twin-Cell autopilot fight used to hang in most runs: a crash, player 2 Hercule's held A playing the special
melee he has no frames for (fixed 2026-10-19, `docs/coop-design.md` "The twin-Cell stall"; regression test
`tools/headless/p2_special.txt` below). Since the fix the refilled pair kills the Cells in 37-49 s (6 delays). Fork:
`make BUILD=build/s13w WIDE=1 DEBUG_MENU=1 A16_TEST=1`, `runfork.ps1 ... -Slot n -Mute`, `p2hold.py --port
7778+n`, `bootisland.sh 80` (island, level-30 pair), then `gdbsrv.sh cmd 'set *(unsigned*)0x0203FD08 = 17'`
and `... 0x0203FD44 = 1` (autopilot); the level-30 pair dies to him in seconds (as to the twin Cells), so raise
their stats for longer footage.

### Player 2's special melee (`tools/headless/p2_special.txt`, 2026-10-19)

`python tools/headless/st_test.py tools/headless/p2_special.txt ROM build/<dir>/hl/menu.ss --out build/<dir>/hl` (any
`DEBUG_MENU=1` build): Map Test 147, player 2 (Hercule, no pose group 16) holds A for 600 frames beside player 1
Piccolo (special melee page). Pass: three live entities (vtables `08025604`, `0802408c`, `0203fe00`), the frame
counter +0x3C over 60 frames, player 2's group pointer never `record + 0x14C`, and player 1 holding A reaching it.
Before the fix the game hung within 30 frames.

### Finding a hang or a memory stomp headless (`st_headless.c`, 2026-10-19)

The runner prints `CRASH at frame N regs: ...` and `Jumped to invalid address` when the CPU leaves mapped code; the
game can look merely frozen after it (IRQs keep `g_FrameCounter` going). Save a state a few hundred frames before,
then: `watch ADDR MAX` steps instructions until the word at ADDR changes and prints the pc, lr, registers and stack
(who wrote it); `breakpc PC MAX [HITS]` prints the registers (and the 8 words at r4 + 0x20) each time PC runs; `trace N
FILE` steps N instructions and writes every pc run with its count. Steps are instructions (about 100 000 per frame).

## GDB stub (fork, `-g`, port 2345) — the fastest way to a crash cause

`gdb-multiarch` is installed in WSL. Launch the fork with `-g`, then run a batch script:

```
wsl -d Ubuntu -u root -- bash -c "gdb-multiarch -q -batch -x /mnt/f/MeltyCreations/log2/build/trace.gdb > /root/gdb.log 2>&1"
```

Script skeleton (addresses from the matching `coop.sym`; write the file with the Write tool, not a
heredoc through `wsl.exe`, which strips `$`):

```
set pagination off
set confirm off
set architecture armv4t
set arm force-mode thumb
target remote 127.0.0.1:2345
break *0x08802098            # a coop function
commands
  silent
  printf "ssj_use c=%x\n", $r0
  continue
end
break *0x08802150            # a per-frame hook (wide_update): count frames, dump state at N
commands
  silent
  set $frames = $frames + 1
  if $frames == 150
    x/32wx *(unsigned int*)0x0203ff08
    dump binary memory /root/oam.bin 0x07000000 0x07000400
    detach
    quit
  end
  continue
end
continue
```

Tricks that worked: a jump trace (`stepi` loop printing `$last>$pc` when `$pc` is not `$last+2/+4`)
from a breakpoint pinpointed a crash to the exact `bx` in `install_behaviour`; `tbreak *($lr & ~1)`
inside breakpoint commands catches a function's return; `x/..` reads any address. Quirks: the stub
drops the connection at a fatal "Jumped to invalid address" (no post-mortem; trace before it),
`dump binary memory` returned zeros past 0x06014000 (read VRAM in small `x/` probes instead),
breakpoints on hot functions slow the game to a crawl (arm them from another breakpoint), and every
frame-stop makes emulated time lag real time, so wait longer before reading the log.

## Krillin / old Hercule tests (fork slot 2, `KR_TEST=1`, 2026-10-18)

```
make BUILD=build/kn DEBUG_MENU=1 KR_TEST=1          # standard 240-wide ROM (the wide build's view can be wider than the window)
tools\fork\runfork.ps1 /mnt/f/.../build/kn/log2-coop.gba -Slot 2 -Wait 3 -State /mnt/f/.../dbgmenu.ss3 -Mute   # dbgmenu.ss3: a debug-menu state you make with this build (boot, idle on the title, save)
python tools\fork\p2hold.py --port 7780 --cmd F:/.../build/kn/p2cmd.txt          # background
tools/fork/padq.sh F:/.../build/kn/p2cmd.txt "0x30C:0.3 0:0.5 1:0.4 0:5"           # toggle the pad to player 1, A
```

- **Several forks at once**: with audio sync on, an instance can barely advance (its pad socket's
  Recv-Q grows, keys "don't register"); `-Mute` (= `-C mute=1 -C audioSync=0`) fixes it. Other
  sessions' scripts (`runa16.ps1`, older `runfork.ps1` copies) run `pkill -9 -x mgba` /
  `pkill -x gdb-multiarch`, which kills every instance or its gdb client; a dead gdb client leaves the
  stub halted (relaunch).
- **No window keys needed**: in `KR_TEST` builds player 2's pad pressing L+R+Select+Start toggles the
  fork pad between player 2 and player 1 (the combo itself reaches nobody), so menus with their own
  input loops (pause / status, save pad dialog, switch screen, level-up text) are driven through the
  TCP pad. Player 1's L+R opens the Dragon Radar (Start, B, B closes it). Test triggers on player 2's
  pad holding L+R: **Up** = slot 5 toggles old Hercule (rows 21 / 24), **Right** = Krillin becomes
  player 1, **Left** = Krillin's EXP one short of his next level, **Down** = 1 EXP to Krillin through
  the kill path's EXP event (a level-up on demand); row and player switches are refused inside
  dialogue loops. Party HP is refilled below a third, so idling tests survive.
- Debug-menu savestate: made once at the title-idle debug menu (Map Test row, zone 2 area 7
  selected); it loads under later builds (menu code is stock). Zone 2 area 6 = open field with
  snakes (an intro dialogue first: 6 x A), area 7 = wolves under trees, area 1 = town with the save
  pad at (540, 256).
- Helpers (`FORK_SLOT=n`): `kr_peek.sh` (party entries, active slot, player 2's slot), `kr_ents.gdb`
  (entity list with vtables, positions, sprite records), `kr_tele2.sh X Y` (both players; the
  shared-screen leash pulls one back otherwise), `kr_setidx.sh SLOT INDEX` (selected ability); state
  counters at `kr_state` (`coop.sym`). Screens: `tools\win_burst.ps1` (N frames from one process, start
  it ~1.7 s before the action; check each frame's title bar, another session's window may be on top).

## Tournament grounds (headless, 2026-10-25)

```
make BUILD=build/grounds DEBUG_MENU=1                                  # needs roms/buu.gba (the maps import Buu's Fury's)
python tools/headless/st_test.py tools/headless/mkmenu.txt build/grounds/log2-coop.gba --out build/grounds/hl
python tools/headless/st_test.py tools/headless/tny_grounds.txt build/grounds/log2-coop.gba build/grounds/hl/menu.ss --out build/grounds/hl --cols 5 > build/grounds/hl/grounds.out
```

`tny_grounds.txt` (co-op, from a Map Test of the world map with flag 441 clear): the landing at (1260, 1820) (the
flyer's shadow on a small island), A lands on the plaza's beach (232, 392); the welcome (caption, four announcer
lines; flag 441 set: `FLAGS+0x37` reads 02); the plaza's pad (Save / Switch Characters, "Game Saved!"); the
announcer's line; plaza -> street -> courtyard; the unlock board's fallback line (no ui piece); courtyard ->
infirmary -> waiting room with test command 30 arg 1 (the Rookie stand-in pool: six fighters in one player, five
in co-op when player 2's character is one of them); a fighter's greeting; the director's welcome and fallback
line (no engine piece); the waiting room's pad; the ring door -> the walkway and its "ring is for fighters" line;
then every exit back (ring -> waiting room -> infirmary -> courtyard -> street -> plaza, no second welcome) and the
flight spot to the world map at (1260, 1820); then `reset` and Continue: Select Game names the save "WORLD
TOURNAMENT", the players question (a black screen, A twice), the game resumes on the waiting room's pad (48, 268).
Pass: 14 `waitmap ... reached`, the arrival `map` lines of
`docs/tournament.md` 9b, the shots show each line, no CRASH. Dialogue pages are pressed one A each (an extra A
re-talks to the NPC in front, or flies off the beach's flight spot). Map previews: `build/grounds/maps/papaya_*.png`.

## Story chapters, headless (`tools/headless/`, 2026-10-18)

The fork's window and GDB link were too flaky for long scripted scenes (the stub dropped connections
after pauses, WSLg showed stale frames under other sessions' windows), so chapters are tested in a
headless runner built on the fork's own `libmgba`: no window, no debugger, deterministic frames,
about seven times faster than real time.

```
wsl -d Ubuntu -u root -- bash -c "cd /mnt/f/MeltyCreations/log2 && gcc -O2 -o /root/st_headless tools/headless/st_headless.c -I/root/mgba-build/include -I/mnt/f/MeltyCreations/tools/downloads/mgba/include -L/root/mgba-build -lmgba -Wl,-rpath,/root/mgba-build"
make BUILD=build/sth DEBUG_MENU=1                       # co-op tests (player 2 present with "p2 0")
python tools/headless/st_test.py tools/headless/mkmenu.txt build/sth/log2-coop.gba    # once per build: build/hl/menu.ss
python tools/headless/st_test.py build/hl/x.txt build/sth/log2-coop.gba build/hl/menu.ss --cols 4
```

`tools/headless/mkmenu.txt` boots, skips the intro (`key 8 6` twice), idles 5500 frames into the
debug menu and saves `build/hl/menu.ss` (bare `shot` / `save` / `load` names land in `build/hl/`). Commands (full list in the C file's header): `frames`, `key MASK N`, `keys MASK`,
`p2 MASK|off`, `mash N [F]`, `mashent NAME VTABLE MAX`, `mashmap ZONE AREA MAX`, `waitmap`,
`waitnz ADDR MAX`, `maptest ENTRY` (with the debug menu open: points Map Test at a map entry index
and presses A), `preset +N -N` (story flags Party_Init applies at that Map Test), `flags`, `peek`,
`poke8/16/32`, `find`, `findent`, `deref`, `ents`, `ssave NAME` (a savestate without the cartridge save), `cart PATH` / `cartout PATH` (a .sav into the cartridge, then a reset / the cartridge save out), `entrec RECORD` (how many entities on a sprite record: an actor's sprite id N has its record at the table `*0x0800959C` entry N - 7; sprite 55 = `0x0828FF38`), `map`, `shot NAME`, `save` / `load`, `reset`
(power-cycle the ROM keeping the cartridge save: another build's savestate, then `reset`, boots this ROM
with that build's EEPROM). Predefined variables: `PARTY` (0x0203F000), `SLOTS` (the ROM's party slot
count, from the default-table literal) and **`FLAGS`** (the story-flag bank, `0x0203F00C + SLOTS * 0x1C`:
0x0203F108 with 9 slots, 0x0203F0EC with 8); write `poke32 FLAGS+0x14 ...`, never the bank's address. The
runner prints the registers and frame of the first "Jumped to invalid address" (`CRASH at frame N regs`,
`r14` = the caller).
`st_test.py` resolves bare shot names next to the script and tiles them into one PNG.

Useful words: `coop2.cmd` 0x0203FD08 (7 = `COOP_CMD_STORY_WIN`: every boss fight and story enemy on
the map is won through its own OnHit; 5 = warp, `cmd_arg` 0x0203FD0C = zone | area << 8,
`cmd_arg2` 0x0203FD54 = x | y << 16), `coop2.a16` 0x0203FD34, `coop2.a16_bot` 0x0203FD44,
`coop.p2_slot` 0x0203FF84, party block 0x0203F000 (active slot byte 0, entries at +0xC + 0x1C * slot,
flags at entry +0x14), story flags `FLAGS` + n / 8 (0x0203F108 with the default 9 slots). A Map Test party has everyone in it and
player 1 is Piccolo (`poke8 0x0203F000 0` then a warp makes Gohan player 1). `STORY_TEST=1` builds
(the fork's TCP pad drives player 1, `tools/fork/st_pad.py`; story-flag preset in ROM at 0xC00000,
`tools/fork/st_preset.py`; player 1's L + R + Up wins the fights) are for interactive fork sessions;
`tools/fork/st_run.ps1` launches the fork without the debugger and `st_place.ps1` moves a window out
from under another session's.

## Time Chamber waves and Roshi's sparring (headless, merged 2026-10-19)

Build `make BUILD=build/lk DEBUG_MENU=1` (and `BUILD=build/lkw WIDE=1 DEBUG_MENU=1` for the lag check),
make the menu state per build, then Map Test the Lookout's chamber room (entry 263) with the wait's flags
`preset +143 +144 +148 +149 +151 +154 +157` (fewer flags replay the stock Goku / Vegeta cutscene there).
Mr. Popo stands at (64, 128): test-warp player 1 to (64, 150) (`COOP_CMD_WARP`, `cmd_arg` 0x0309,
`cmd_arg2` 0x00960040), tap Up, press A, and press A once per page **120 frames apart** (the text types
out; an extra A talks to him again). The door wants A while facing up inside (73, 113)-(120, 140): warp
to (96, 160), hold Up, press A, `waitmap 15 2`. "Round 1" shows about 220 frames later; A spawns it.

- `waves` (`src/coop/waves.c`; 0x0203F914 with 9 slots, 0x0203F880 with 8; it sits after `.party.kr` and moves when that grows, so read `$(BUILD)/coop.map`): +0 magic 0xC00B3A7E, +4 round sequence, +8 its action,
  +0x10 round / state (0 off, 1 pause, 2 caption, 3 fight, 4 done) / timer / standing enemies, +0x14
  records spawned, +0x16 the round's smallest largest-free OBJ VRAM block (32-byte units), +0x18
  input polls (game-loop iterations: against `g_FrameCounter`, which ticks in VBlank, they count lag),
  +0x1C the round's four records.
- `poke32 0x0203FD08 13` (`COOP_CMD_WAVE_CLEAR`): every standing enemy dies the regular death (EXP to
  player 1); `poke32 0x0203FD0C N` then `poke32 0x0203FD08 14` (`COOP_CMD_WAVE_SKIP`): the next round
  announced is N (11 = completed). Refill the players between rounds (`poke16 0x0203F00C 999`, slot 1
  at 0x0203F028, Hercule 0x0203F098): an idle pair dies by round 5. Level-up dialogs need extra A.
- Leaving: warp to (308, 230) in the chamber and hold Up (the door rect (272, 200)-(344, 216)).
- Sparring: Map Test 147 with the gym done (`+212 +213 +214`, and `+215 +216 +217` so Roshi skips the
  technique lessons); Roshi at (488, 576): warp to (488, 604), Up, A, four pages. Into the dojo through
  the atrium door: warp to (268, 334), hold Up, `waitmap 6 70`; the mat trigger fires on arrival.
  `0x0203FD5C` = `duel_active`, `duel_winner`. A real final blow: player 2's HP to 1 (its slot from
  `coop.p2_slot` 0x0203FF84; a Map Test's player 2 is Hercule, slot 5), player 1 taps Left and A three
  times 6 frames apart (more A dismisses the winner caption); for player 2's win, player 1's HP to 1 and
  `p2 0x10` then `p2 1`.
- A private runner: build it to `/root/st_headless_lk` and set `ST_HEADLESS=/root/st_headless_lk` (from Git
  Bash also `MSYS2_ENV_CONV_EXCL=ST_HEADLESS`, or the path is rewritten to a Windows one).
- The fork: `tools/fork/st_run.ps1 -Slot 5 -State <headless .ss>` loads a headless savestate of the same
  ROM (the round-3 caption here); `p2hold.py --port 7783` keeps player 2 present.

## Broly chapter (headless + fork slot 6, 2026-10-19)

```
make BUILD=build/broly DEBUG_MENU=1                     # chapter tests; A16_TEST=1 (build/brolyt) for the autopilot fight
python tools/headless/st_test.py tools/headless/mkmenu.txt build/broly/log2-coop.gba --out build/broly/hl
```

- Broadcast: `preset +151 +156`, `maptest 160`; the Map Test spawns player 1 inside the broadcast's
  trigger rect, which fires only on entering: walk out and back in (`key 0x20 30`, `key 0x80 40`,
  `key 0x10 30`), mash; Bulma's three pages follow Cell's last line; flag 400 is bit 0 of `FLAGS+0x32` (0x0203F13A with 9 slots, 0x0203F11E with 8);
  the stock jump to "Meanwhile..." follows.
- Backyard: `p2 0`, `preset +151 +156 +157 +400`, `maptest 165`, walk right (the hint without Vegeta);
  for the scene make Vegeta player 1 (`poke8 0x0203F000 2`, then the test warp `poke32 0x0203FD0C
  0x1506`, `poke32 0x0203FD54 0x01100010`, `poke32 0x0203FD08 5`), walk right, mash about 36 presses
  to the landing zone; save a state there.
- New Planet Vegeta: dismiss the last caption first, then walk **both** pads north (`p2 0x40` with
  `key 0x40 ...`; player 1 alone stops at the co-op leash); about 14 presses through the
  confrontation. `deref B 0x0203FC24` = Broly (pair 0's original); `poke32 0x0203FD08 9` drops Paragus
  only (the rage: B+0x188 HP back to max, B+0x160 attack / defence bytes up), `7` wins the fight;
  then flags `FLAGS+0x32` = 0xff and the party as kept (player 1 0x0203F000, player 2 0x0203FF84).
- Autopilot fight (A16_TEST build): from the fight state `poke16 0x0203FD44 1`; the autopilot never
  presses A, so each cycle pause it, tap A (dialogue), resume; top up the players' HP (slot 2 at
  0x0203F044, slot 3 at 0x0203F060) to see the whole fight. In the fork the same with
  `runfork.ps1 ... -Slot 6 -Mute -State .../fight.ss` (a headless savestate loads in the fork) and a
  gdb loop (from Git Bash set `MSYS_NO_PATHCONV=1` before `wsl ... bash /mnt/...`).
- Single-player: the same scripts without `p2 0` (slower through the dialogue; one boss bar).
- The ship ride, end to end (2026-10-21): `tools/headless/broly_ride.txt` (alone; Paragus' scene, up the ramp
  into the doorway at x 344, the room route around the table to its door, out on New Planet Vegeta at 320,680,
  around the ship to the rim, test command 7, the ride home, out of the backyard ship's door at 344,412). Wait
  about 900 frames after "The ship lands." before walking: the others get off first. Two players: `p2 0`, and
  give player 2's pad the same direction as player 1's on every walk (`p2 0x40` beside `keys 0x40`) or the co-op
  leash stops player 1 halfway (the room's door, the walk to the rim). Player 2 is not drawn during scenes; he
  shows beside player 1 when the scene ends.

## Gero's computer (headless chapter run and the bunker fight in the fork, 2026-10-19)

```
make BUILD=build/gero DEBUG_MENU=1
python tools/headless/st_test.py tools/headless/mkmenu.txt build/gero/log2-coop.gba --out build/gero/hl
python tools/headless/st_test.py tools/headless/ch9_gero.txt build/gero/log2-coop.gba build/gero/hl/menu.ss --out build/gero/hl --cols 6
```

`tools/headless/ch9_gero.txt` Map Tests the workshop with the wait's flags (151, 155-157), plays the
scene, checks the party (active slot 6, `coop.p2_slot` 7), sets flags 0-191, then warps (COOP_CMD_WARP)
to each beat and lets its trigger fire: the world map near the zone 8 landing (the star on the
minimap), the two generators, area 47's top exit, the rubble, the doors 61 -> 64 -> 63, the bunker; it
wins the fight with `COOP_CMD_STORY_WIN`, then the 13 file, the birds and home (active 6, player 2 slot
5, flags 420-425 set). Savestates `g_ws.ss`, `g_fight.ss`, `g_home.ss` land beside the shots.

Fork (bunker pair, Rocket Punch): `make BUILD=build/gerof WIDE=1 DEBUG_MENU=1 A16_TEST=1`, launch a
slot, `p2hold.py`, `bootisland.sh 75`, then `COOP_CMD_A16_PARTY` (write 11 to `coop2.cmd` 0x0203FD08:
the chapter's party without its scene) and `bossfight.sh <bunker record> 120 4 0x0203F0B4 0x0203F0D0`
(Krillin's and 16's entries). The record address is the `map 257 new enemy record 1` line of
`build/<dir>/hercule.patch.json`. Player 2's B (p2hold `--cmd` file, e.g. `0x20:0.2 0:0.2 2:0.1`) fires
the Rocket Punch; `a16p` (coop.sym) counts punches (+8) and hits (+10). Boot fresh for each build: a
state saved after the party switch sometimes left every boss idle.

## Hercule's beam pose (headless, 2026-10-19)

```
make BUILD=build/hb DEBUG_MENU=1
python tools/headless/st_test.py tools/headless/mkmenu.txt build/hb/log2-coop.gba --out build/hb/hl
python tools/headless/st_test.py tools/headless/beam_hercule.txt build/hb/log2-coop.gba build/hb/hl/menu.ss --out build/hb/hl --cols 8
```

`beam_hercule.txt` replays the stock struggle (map 325's record 12 defeat sequence) with Hercule as
player 1 (no player 2): the beam clash (shots 05-08) and `NPCFireKamehameha 0, 140` (41-42); with
`poke8 0x0203F000 0` instead of 5 it is Gohan, for a side-by-side. How the scene is reached without
the earlier fights is in the script's header (spawn record 12 with `COOP_CMD_SPAWN_SCRIPT`, a no-op death
action for record 0's Cell, `COOP_CMD_STORY_WIN`); the run ends on map 326.

- **Other facings with the real opcode:** cut the script after `shot herc_40`, then `findent pl
  0x08025604`, `mash 1` and about 70 x (`poke8 pl+0x10 F`, `frames 1`): the facing byte is read when
  the beam fires 8 frames in (right fires from the palms; down / up fire from the body centre for
  Gohan too: the engine's offset there is 0, 0).
- **Any entity in the pose without a script** (player 2 here; `p2 0`, Map Test 147): `findent p2e
  0x0203FE00` (player 2's vtable copy), `deref rec p2e+0x48`, then `poke8 p2e+0x10 F`, `poke16 p2e+0x12
  0`, `poke16 p2e+0x14 0`, `poke8 p2e+0x8 0x28`, `poke32 p2e+0x4C rec+0x16C`, `poke32 p2e+0x18
  0x03002268` (state 30's enter, by hand); shots 4, 16 and 24 frames later show charge, thrust, hold.
  Move the entity first (`poke32 p2e+0x158 X<<8`, `+0x15C`) if the BG draws over it.
- Map Test 323 (zone 14 area 1): player 2 Hercule's second A press crashes the game ("Jumped to invalid
  address 8B396A16", r14 `0x08006C05`) with the stock record 72 as well; use another map for player-2
  tests until that is understood.

## Super Saiyan 2 Gohan (headless + fork slot 9, 2026-10-19)

```
make BUILD=build/ssj2t DEBUG_MENU=1 SSJ2_TEST=1          # test triggers on player 2's pad (below)
python tools/headless/st_test.py tools/headless/mkmenu.txt build/ssj2t/log2-coop.gba --out build/ssj2t/hl
python tools/headless/st_test.py tools/headless/ssj2_gohan.txt build/ssj2t/log2-coop.gba build/ssj2t/hl/menu.ss --out build/ssj2t/hl --cols 6
python tools/headless/st_test.py tools/headless/ssj2_save.txt build/ssj2t/log2-coop.gba build/ssj2t/hl/menu.ss --out build/ssj2t/hl --cols 4
python tools/headless/st_test.py tools/headless/ssj2_duel.txt build/ssj2t/log2-coop.gba build/ssj2t/hl/menu.ss --out build/ssj2t/hl --cols 2
```

- `SSJ2_TEST=1`: player 2's pad (`p2 MASK`) holding L + R: **Select** = `ssj2_join` (what CoopCall 20 runs),
  **Start** = he becomes player 1 (refused while player 2 plays him), **Up** = player 2 plays him, **Down** =
  his EXP one short of his next level. His entry is at `PARTY+0xEC` (slot 8; ability index at `+0xF5`),
  Gohan's at `PARTY+0x0C`. The ability gate word 0x0300208C is 0 on a Map Test: set it before each B.
  The Kamehameha fires after 24 frames of B held (a tap cancels it); snakes and wolves interrupt
  abilities (hurt state), so expect a missed one now and then in the wolves map.
- `ssj2_gohan.txt`: Map Test 13 (zone 2 area 7) warped to the open path (500, 430), the join, player 2
  then player 1 as him: walks, the A combo, all four abilities (EP drops 2 + 4 + 10 + 2, the held
  Kamehameha drains more). `ssj2_save.txt`: the town's save pad (Map Test 7, zone 2 area 1, pad at 540,
  256; its tutorial and a first save play first, the pad menu opens on the second A, Down = Switch),
  Switch Characters scrolled to him (8 members: the six-wide window scrolls twice), A, Save, soft reset
  (`key 0xF 6`), Select Game (two Starts, Start through the intro, A on START), load: active slot 8, his
  entry and the flags intact. `ssj2_duel.txt`: a clone duel (`COOP_CMD_DUEL`, arg 0x300) with him as player
  1: player 2's slot 7 holds a copy of slot 8, the final blow gives winner 1.
- **CoopCall 20 from a script**: a build whose data patch adds the test-only
  `tools/headless/ssj2_join_test.json` (a dialogue trigger above the Map Test spawn in zone 2 area 1: the
  CoopCall, then the join message):
  ```
  python -c "import json; d=json.load(open('data/hercule.json')); d['include'].append('tools/headless/ssj2_join_test.json'); json.dump(d, open('build/hercule_jointest.json','w'), indent=2)"
  python -I tools/romdata.py build build/hercule_jointest.json build/ssj2j.patch.json roms/log2.gba --party-slots 9
  make BUILD=build/ssj2j DEBUG_MENU=1 DATA_PATCH=build/ssj2j.patch.json
  python tools/headless/st_test.py tools/headless/ssj2_join.txt build/ssj2j/log2-coop.gba build/ssj2j/hl/menu.ss --out build/ssj2j/hl --cols 2
  ```
  (mkmenu for `build/ssj2j` first.) Slot 8 is zero before and filled after the message; player 2's Select
  then cycles Hercule -> Krillin -> him.
- **Old saves**: `ssj2_oldsave1.txt` on an eight-slot build (main before 2026-10-19, or `PARTY_SLOTS=8`)
  saves at the pad and keeps `old_saved.ss`; `ssj2_oldsave2.txt` on the nine-slot build loads that state,
  `reset`s and loads slot 1 twice. Pass: both loads land on the pad with Krillin's entry and the tutorial
  flag (bit 13 of `FLAGS`) intact, slot 8 zero, and the EEPROM mirror (`deref m 0x03003898`) holds the new
  checksum at `m+0x1C8` (0x10 + 0x1B8) while the old one at `m+0x1AC` is gone.
- Status screen: Start opens it on player 1's page; Right / Left step through the members, Up / Down
  through pages. With 8 members it showed garbage beside Gohan's title and crashed stepping onto him
  (main too, with `COOP_CMD_A16_PARTY`) until `status_tabs_hook` (`docs/party-8-slots.md`).
- Fork: `make BUILD=build/ssj2f WIDE=1 DEBUG_MENU=1 SSJ2_TEST=1`, a headless state of the wide ROM (Map
  Test 13, the join), `tools\fork\runfork.ps1 /mnt/f/.../build/ssj2f/log2-wide.gba -Slot 9 -Mute -State
  /mnt/f/.../f_joined.ss`, `p2hold.py --port 7787 --cmd build/ssj2f/p2cmd.txt` (write `MASK:SECONDS` steps
  into the file), player 1 through `FORK_SLOT=9 /root/fork-s9/keys.sh`, the gate word and ability index
  through `gdbsrv.sh pause` / `cmd 'set ...'` / `cmd continue`, `COOP_CMD_WAVE_CLEAR` (13) clears the
  map's enemies first; `win_shot.ps1` needs an absolute output path.

## Haloed Goku (headless, 2026-10-19)

```
make BUILD=build/hb DEBUG_MENU=1
python tools/headless/st_test.py tools/headless/mkmenu.txt build/hb/log2-coop.gba --out build/hb/hl
python tools/headless/st_test.py tools/headless/halo_goku.txt build/hb/log2-coop.gba build/hb/hl/menu.ss --out build/hb/hl --cols 6
```

`halo_goku.txt` Map Tests Roshi's island (entry 147), puts Goku (slot 4, entry at 0x0203F07C) on row 26
(`poke8 0x0203F084 26`; the story does it with `SetCharacterTransformation 4, 176`), warps in place so
the player entity is rebuilt, then walks, punches, transforms (ability index 2 = Transformation B, B)
and transforms back, printing the row and the entity's sprite record (`halo_goku_record` /
`halo_ssj_record` in `coop.sym`) after each. The run must read rows 1a, 1b, 1a with no CRASH line.

- The transformation abilities are ready only while the word at **0x0300208C** is nonzero (0 on a Map
  Test); the script sets it before each B. EP must be >= 10 (entry +4).
- Island 147 is a town (A talks, no attacks) and its floor's bottom edge is an exit: attacks were seen
  on the desert (Map Test 325, warp to (256, 320); Cell's beam interrupts after a few seconds).
- Control: the same script with `poke8 0x0203F084 19` (Goku's stock rows 19 / 20).

## Frame counts (headless, 2026-10-19)

```
make BUILD=build/frames DEBUG_MENU=1
python tools/headless/st_test.py tools/headless/mkmenu.txt build/frames/log2-coop.gba --out build/frames/hl
python tools/headless/st_test.py tools/headless/ssj_tumble.txt build/frames/log2-coop.gba build/frames/hl/menu.ss --out build/frames/hl --cols 6
```

`ssj_tumble.txt` makes Super Saiyan Hercule (slot 5 on row 22) player 1 on Roshi's island and plays
group 20 (tumble) facing right through the detransform state (anim 0x24, state `0x03002180`: frame =
counter / 16 for 64 frames of whatever group quad +0x4C points at, then the row's link back to
Hercule): poke `+0x10` facing, `+0x12` counter, `+0x14` frame, `+0x8` anim, `+0x4C` group quad
(record + 0x4C + 16 * group), `+0x18` state object. Pass: four tumble frames, the party block and entity
list words the same before and after, no CRASH line. Any other record and group: replace
`rec+0x18C` with the address of that group's quad from `coop.sym` (with `paragus_record` group 20 the
pre-fix build crashed with the party block overwritten; a 2-frame group must stop before counter 32).

## Cell Games (headless, 2026-10-19)

```
make BUILD=build/cg DEBUG_MENU=1
python tools/headless/st_test.py tools/headless/mkmenu.txt build/cg/log2-coop.gba --out build/cg/hl
python tools/headless/st_test.py tools/headless/cell_games.txt build/cg/log2-coop.gba build/cg/hl/menu.ss --out build/cg/hl --cols 6
```

`tools/headless/cell_games.txt` plays the whole Games: Map Test 323 with flags 191 and 421, the barrier opened
with A, both players walk up into the ring, then nine fights, each queued by writing its stat + 1 to
`cg_state + 5` (0x0203F9B5 in the current build; read `$(BUILD)/coop.map` for `.party.cg`) and the magic
0xCE11CA3E to `cg_state`; `stepz 0x0203F9B5 140` presses A on both pads every 100 frames until the fight has
won itself (once that stat's enemy stands and player 1 is free to move, so never during a scene). After each
win `cg_state + 7` holds who fought: player 1's slot | solo << 4 | 0x80 when player 2's entity stood (expected
14, 24, 80, 16, 21, 12, 23, 80, 80). The end: `mashmap 6 99`, Goku (slot 4) on row 0x1a (26, the halo). The
script saves `cg_pre8.ss` before the final fight; filming the struggle from it: queue stat 25 (write 26), stepz,
`frames 1500` (the long white fade-in), then shots.

- `COOP_CMD_STORY_WIN` (7) is not usable in the desert: every map reload respawns the always-present stat-22 Cell
  (record 0) until the scene's `DespawnEntity 30`, and a win in that gap replays fight 2's end.
- In a solo-2 fight player 1's pad is ignored, so the title card and the next scene are advanced by player 2's A.
- Savestates break whenever the chapter data or code changes (the enemies' record copies hold sequence
  addresses): rerun from the menu state.

## Post-game on the Lookout (headless, 2026-10-19)

`python tools/headless/st_test.py tools/headless/postgame.txt ROM build/<dir>/hl/menu.ss --out build/<dir>/hl` (a
`DEBUG_MENU=1` build): Map Test 261 with flags 191 and 195, a test warp to just below Dende's record 4 (760, 448), Up,
A. Checks: after the wishes FLAGS+47 = 0x20 (381) and FLAGS+60 bit 0 (480); the reminder; `flags +382` stands in
for the summit's fight; after the finale FLAGS+60 = 0x09 (483); the question answered B then A (No keeps the players
on the Lookout), then A twice: `mashmap 1 2` reaches the epilogue's future city. Never send a test warp while a scene
runs (a warp mid-cutscene crashes, r0 = player 2's vtable): close each dialogue page exactly once and let each stage
settle; FLAGS is the runner's variable (0x0203F108 with 9 party slots).

## The Arctic summit (headless, 2026-10-19)

`tools/headless/arctic_summit.txt`: Map Test 261 with flags 191, 195, 480 and 381, a test warp to the summit's arrival
(15/8, (336, 464)), both pads walk up into the scene's rect, A through the scene, then the boss stat is queued the way
the Cell Games test does it (`cg_state + 5`, stat + 1 = 146 for Super 13's 145; stepz) and FLAGS+47 bit 6 (382)
is checked. `PanCameraToPosition` names the view's top-left corner, not its centre.

## Revive and difficulty (headless, 2026-10-20)

`tools/headless/revive.txt`: Map Test 261, test command 10 downs a player (`arg` 0 / 1); `key 1 200` is player 1
holding A for one revive (a `key` releases for 4 frames afterwards, so two shorter holds never add up), `p2 1` ...
`p2 0` player 2's. Checks `rv` (0x0203F9F8) and the revived HP (30 %), then lets a countdown run out into "You have
died!". `tools/headless/difficulty.txt`: Map Test 0 plays the training scene with the question; B, B answers Hard
(FLAGS+57 = 0x20).

## One player (headless, 2026-10-20)

`p2 off` at the top of a script plays alone: player 2's pad never speaks, so after the 3 s timeout there is no
player 2 (`stepz` presses only player 1's A then). `tools/headless/cell_games_1p.txt` is the Cell Games alone,
`tools/headless/dropout.txt` player 2 leaving and returning. Any chapter test runs alone by changing its first
`p2 0` to `p2 off` and dropping its other `p2` lines (the summit, post-game and Gero chapter pass that way).

## The players question (headless, 2026-10-20)

It only appears after a Select Game, so chapter tests (Map Test) never see it. On the play ROM: power on, Start,
A, A (Select Game, new game), then `stepnz 0x0203FA49 40` (A until `players.asking` is set), then `key 1 6`
(one player) or `key 2 6` (two). `players.mode` is 0x0203FA48; with `p2 0` (a second pad) mode 1 leaves
`coop.p2` 0 and mode 2 brings player 2 in.

## Reading screenshots

`win_shot.ps1` captures the window at 2x. For a region, crop and upscale with PIL (`Image.open(...)
.crop(box).resize(..., Image.NEAREST)`) and view the result; OBJ VRAM / OAM dumps render with
`tools/sprites.py`'s Canvas and the OBJ palette (0x05000200).

## Ghidra headless (decompile before guessing)

```
$env:JAVA_HOME="C:\Program Files\Eclipse Adoptium\jdk-21.0.12.101-hotspot"
& "F:\MeltyCreations\tools\ghidra\ghidra_12.1.4_PUBLIC\support\analyzeHeadless.bat" F:\MeltyCreations\log2\build\ghidra log2 -process log2.gba -noanalysis -scriptPath F:\MeltyCreations\log2\tools\ghidra -postScript DecompileFunctions.java F:\MeltyCreations\log2\build\decomp 0800A8EC 080042BC
```

Output lands in `build/decomp/<addr>_FUN_<addr>.c`. `FindRefs.java` lists references to an address.
Literal pools: search the ROM for a 32-bit value to find every code literal that loads a table
(that is how the 18 display-row literals and the one sprite-index literal were found).

## Drop-in savestates for Ryan (2026-10-20)

`tools/headless/a16_moves.txt` (Android 16 as a player, from a debug-menu state): test command 11 (16 on player 2), player
2's L cycles 0 -> 1 -> 2 -> 3 -> 0 (slot 7's ability index, PARTY+0xD9), each move with B (Rocket Punch 6 EP, Afterimage
Rush 4, Big Bang 10, Ki Blast 2), an older save's entry (flags 0x43, the old numbers) fixed once, then player 1 alone as
16. In Map Test 13 the snakes interrupt moves (hurt state); the same steps in the town (Map Test 7, `mash 60 20`
first) show every EP drop.

`python tools/headless/dropins.py PLAY_ROM DEBUG_ROM MENU_SS SAV OUTDIR` writes `<rom>.ss1`-`.ss7` next to the play
ROM: mGBA loads slot n with Fn (Shift+Fn saves over it). F1 his save on the Northern Wastelands save pad (Krillin),
F2 Goku's landing site as the group flies in, F3 Gero's computer, F4 the Cell Games path (Goku player 1 for the
ring's barrier), F5 the Arctic summit, F6 the post-game Lookout below Dende, F7 the Broly chapter's backyard (Vegeta player 1, flag 400: the ship is there;
walk left into the trigger by the ship). F3 / F4 / F7 are made by Map Test on the
DEBUG_MENU=1 build (the same code but for the 4-byte title patch); F6 and F5 play the Cell Games through alone
(`cell_games_1p.txt`), land at the Lookout's world-map site (the test warp only moves the flyer on the world map,
and the Games' last "Journal Updated!" holds it until A), and F5 makes the wishes (`postgame.txt`), so their party
is the real one (Goku's halo, Super Saiyan 2 Gohan). All are re-saved on the play ROM. Each is saved 300 frames after its warp or Map Test, with the map fully loaded (Ryan, 2026-10-21: F7 saved
30 frames after its warp opened mid-load on the Map Test spot and stayed there in mGBA; check a new one with `load`, `frames 2`, `map`: real player coordinates, not -1,-1). Every one gets the story
before the ten-day wait as done (`EARLY` in dropins.py, worked out from the world map's on-load: no stale stars,
the Lookout landing open) and flag 400 (Bulma's news: the Broly ship waits in the backyard). The states hold no cartridge save, so loading one leaves the .sav alone (an
in-game save inside one still writes it). Savestates break with every rebuild: rerun after each, with mGBA closed.
`build/play/DROP-INS.txt` lists the keys for Ryan.
