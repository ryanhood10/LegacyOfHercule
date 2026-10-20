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
| Boss pairs, Android 16 boss fight | fights with the autopilot and hit logs |
| Krillin / old Hercule tests | the playable-character test triggers |
| New maps (zone 15) | building and warping to the new maps |
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
`poke8/16/32`, `find`, `findent`, `deref`, `ents`, `map`, `shot NAME`, `save` / `load`. The runner
prints the registers of the first "Jumped to invalid address" (`CRASH regs`, `r14` = the caller).
`st_test.py` resolves bare shot names next to the script and tiles them into one PNG.

Useful words: `coop2.cmd` 0x0203FD08 (7 = `COOP_CMD_STORY_WIN`: every boss fight and story enemy on
the map is won through its own OnHit; 5 = warp, `cmd_arg` 0x0203FD0C = zone | area << 8,
`cmd_arg2` 0x0203FD54 = x | y << 16), `coop2.a16` 0x0203FD34, `coop2.a16_bot` 0x0203FD44,
`coop.p2_slot` 0x0203FF84, party block 0x0203F000 (active slot byte 0, entries at +0xC + 0x1C * slot,
flags at entry +0x14), story flags 0x0203F0EC + n / 8. A Map Test party has everyone in it and
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

- `waves` (`src/coop/waves.c`; 0x0203F880 on main; it sits after `.party.kr` and moves when that grows, so read `$(BUILD)/coop.map`): +0 magic 0xC00B3A7E, +4 round sequence, +8 its action,
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
  `key 0x10 30`), mash; Bulma's three pages follow Cell's last line; flag 400 is bit 0 of 0x0203F11E;
  the stock jump to "Meanwhile..." follows.
- Backyard: `p2 0`, `preset +151 +156 +157 +400`, `maptest 165`, walk right (the hint without Vegeta);
  for the scene make Vegeta player 1 (`poke8 0x0203F000 2`, then the test warp `poke32 0x0203FD0C
  0x1506`, `poke32 0x0203FD54 0x01100010`, `poke32 0x0203FD08 5`), walk right, mash about 36 presses
  to the landing zone; save a state there.
- New Planet Vegeta: dismiss the last caption first, then walk **both** pads north (`p2 0x40` with
  `key 0x40 ...`; player 1 alone stops at the co-op leash); about 14 presses through the
  confrontation. `deref B 0x0203FC24` = Broly (pair 0's original); `poke32 0x0203FD08 9` drops Paragus
  only (the rage: B+0x188 HP back to max, B+0x160 attack / defence bytes up), `7` wins the fight;
  then flags 0x0203F11E = 0xff and the party as kept (player 1 0x0203F000, player 2 0x0203FF84).
- Autopilot fight (A16_TEST build): from the fight state `poke16 0x0203FD44 1`; the autopilot never
  presses A, so each cycle pause it, tap A (dialogue), resume; top up the players' HP (slot 2 at
  0x0203F044, slot 3 at 0x0203F060) to see the whole fight. In the fork the same with
  `runfork.ps1 ... -Slot 6 -Mute -State .../fight.ss` (a headless savestate loads in the fork) and a
  gdb loop (from Git Bash set `MSYS_NO_PATHCONV=1` before `wsl ... bash /mnt/...`).
- Single-player: the same scripts without `p2 0` (slower through the dialogue; one boss bar).

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
