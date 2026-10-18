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

## Fork helpers (`tools/fork/`, used for the party-slots work)

The stub accepts one client per emulator run: a second `target remote` never completes, and a client
that quits while attached leaves the stub writing to a dead socket (SIGPIPE kills mGBA unless the
launcher ignores it). So keep one gdb attached for the whole run:

```
toolsorkunfork.ps1 /mnt/f/MeltyCreations/log2/build/party8/log2-coop.gba 14   # kill old, launch with -g + trap '' PIPE, attach, continue, wait
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
