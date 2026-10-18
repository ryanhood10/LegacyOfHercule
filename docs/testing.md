# Testing recipes

Two emulators, two ROM builds. The stock co-op ROM (`build/log2-coop.gba`) runs in the Windows
mGBA (Qt, Lua scripting works there). The wide ROM (`build/log2-wide.gba`) needs the fork in
`tools/mgba-wide`, built and run inside WSL Ubuntu (Smart App Control blocks it on Windows); the
fork's Lua runtime never fires frame callbacks, so player 2 goes in through a native TCP pad and
inspection goes through the GDB stub.

## Savestates

`build/log2-coop.ss1`: zone 2 area 7 (wolf map, debug Map Test), level-40 party, Piccolo is
player 1 (active slot 1), Gohan is player 2 (slot 0); Hercule is slot 5. `ss2`: prologue Flight
Circle. A state made with build A crashes under build B as soon as player 2 acts (its vtable copy in
EWRAM holds code addresses), so re-make states after rebuilding the same ROM name; the fork reports
"Savestate is for a different version" and still loads it. The debug menu (`DEBUG_MENU=1`) appears
after ~75 s idle on the title; its Area number is the index inside the zone's entry block.

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

Always `pkill -9 -x mgba` before launching: a stale instance keeps ports 7778/2345 and the new one
silently runs without pad or debugger. The log says `p2pad: sender connected` when the pad is live.
Select cycling for player 2 registers only every few presses; verify the slot instead of counting.

## Several fork instances at once (slots)

`tools/fork/runfork.ps1 <rom> [-Wait s] [-Slot n] [-State /mnt/f/.../x.ssN]` launches instance n with
`MGBA_GDB_PORT` 2345+n, `MGBA_P2_PORT` 7778+n and window title `mGBA-n` (slot 0 keeps 2345 / 7778 /
"mGBA"), stops only that slot's previous instance, installs `tools/fork/*.sh|*.gdb` into `/root`
(`install.sh`) and attaches the persistent gdb client. Every helper takes the slot from `FORK_SLOT`
(`tools/fork/slot.sh`: ports, `/tmp/gdbin-n`, `/root/gdb-n.log`), e.g.
`wsl -d Ubuntu -u root -- bash -c "FORK_SLOT=1 /root/peek.sh"`. Screenshots by title:
`tools\win_shot.ps1 out.png -Title "mGBA-1"` (slot 0: `-Title "mGBA (Ubuntu)"`). Player-2 pad:
`python tools/fork/p2hold.py --port 7779` for slot 1. Claim a slot before using it (check
`ss -ltn | grep 234` in WSL); sessions running in parallel should each use their own.

Finding entities with gdb: the scene list is `g_EntityList` 0x03001120, `count` at +4 and the item
pointers inline from +8; co-op enemies have vtable 0x0203FB04 (`coop2.enemy_vtable`).

## Fork helpers (`tools/fork/`, used for the party-slots work)

The stub accepts one client per emulator run: a second `target remote` never completes, and a client
that quits while attached leaves the stub writing to a dead socket (SIGPIPE kills mGBA unless the
launcher ignores it). So keep one gdb attached for the whole run:

```
tools\fork\runfork.ps1 /mnt/f/MeltyCreations/log2/build/party8/log2-coop.gba 14   # kill old, launch with -g + trap '' PIPE, attach, continue, wait
wsl -d Ubuntu -u root -- bash -c "/root/peek.sh"            # pause, dump party / player / EEPROM slots (peek.gdb), continue
wsl -d Ubuntu -u root -- bash -c "/root/keys.sh 8 0.25"      # hold Start 0.25 s through g_p1_remote (0x0203FF02): A=1 B=2 Sel=4 Start=8 R/L/U/D=0x10/20/40/80 R=0x100 L=0x200
wsl -d Ubuntu -u root -- bash -c "/root/tele.sh 544 215"     # teleport player 1 (pixels)
```

Copy the `.sh`/`.gdb` files to `/root` first (`runfork.ps1` expects them there; strip CR). `keys.sh`
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

Build and helpers (from the `android16-boss` worktree, `F:\MeltyCreations\log2-a16`):

```
make BUILD=build/a16 WIDE=1 DEBUG_MENU=1 A16_TEST=1        # Git Bash, toolchain PATH as above
wsl -d Ubuntu -u root -- bash -c "tr -d '\r' < /mnt/f/MeltyCreations/log2-a16/tools/fork/a16install.sh | bash"   # copies a16*.sh/.gdb to /root
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
MSYS_NO_PATHCONV=1 powershell -NoProfile -File tools/fork/runa16.ps1 -State /mnt/f/MeltyCreations/log2-a16/build/a16/a16_base.ss1
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
