# Legacy of Goku II — decomp + co-op

Decompilation of *Dragon Ball Z: The Legacy of Goku II* (GBA, USA, `ALFE`) with the goal of
adding shared-screen co-op (link cable as second controller) and later merging *Buu's Fury*.

This repo contains code and notes only. No ROM data or game assets are committed.

## ROMs

Drop dumps of your own cartridges here (git-ignored):

```
roms/log2.gba     Dragon Ball Z: The Legacy of Goku II (USA)   game code ALFE
roms/buu.gba      Dragon Ball Z: Buu's Fury (USA)
```

Then check them:

```
python tools/romcheck.py roms/log2.gba roms/buu.gba
```

## Ghidra

Builds a headless Ghidra project in `build/ghidra/` with the GBA memory map, functions seeded from
`bl` targets and pointer tables (Ghidra's own code finder is useless on this ROM), and a function
list in `build/log2.functions.csv`:

```
tools\ghidra_import.ps1 roms\log2.gba
```

Open `build\ghidra\log2.gpr` in the Ghidra GUI afterwards. Findings go in `docs/rom-map.md`.

## Building the co-op ROM

`src/coop/` is compiled with `arm-none-eabi-gcc` (Thumb, arm7tdmi) into a blob linked at
`0x08800000`, and `tools/patch_rom.py` pads the original to 16 MiB, drops the blob in, and rewrites
the vtable slots that redirect into it:

```
make
```

Output: `build/log2-coop.gba`. Run it in mGBA. For a boot smoke test over the GDB stub:
`mgba-sdl -g build\log2-coop.gba`, then `arm-none-eabi-gdb -batch -x tools\gdb\check_boot.gdb`.

## Playing two-player on one PC

1. Open `build\log2-coop.gba` in mGBA (the Qt `mGBA.exe`, not `mgba-sdl`).
2. Tools > Scripting, type `dofile("F:/MeltyCreations/log2/tools/mgba/coop.lua")` and Run. That
   loads the player-2 bridge plus the console helpers `peek()`, `ents()` and `p2_force` (mGBA 0.10.5
   has no command-line script option, so this step is manual).
3. Run `python tools\p2_keys.py` in a terminal. Player 1 uses mGBA's normal keys; player 2 uses
   IJKL to move, O = A, U = B, [ ] = L R, N = Select, M = Start. Two gamepads: bind one in mGBA,
   adapt `KEYMAP` in `p2_keys.py` for the other. (A second player on another PC can also run
   `p2_keys.py --host <your ip>` with TCP 7777 forwarded and watch a screen share, but see the next
   section for real netplay.)

Player 2 appears as soon as its pad sends anything, bound to the first other party member, with its
own HUD panel on the right, its own damage and EXP (kills go to whoever made them). Player 2 presses
Select to switch to the next available party member; the two players never share a character (if
player 1 picks player 2's character at a save pad, player 2 takes player 1's old one). With no second
pad the game plays exactly as before. Testing tip: `make DEBUG_MENU=1`, idle ~75 s on the title for
the debug menu's Map Test, and keep an mGBA savestate (`Shift+F1`) on a map; `mGBA.exe -t
build\log2-coop.ss1 build\log2-coop.gba` starts straight from it.

## Playing online (lockstep netplay)

Both players run the same `build\log2-coop.gba` in their own mGBA, and `tools/mgba/netplay.lua`
keeps the two emulations identical: every frame each side sends its pad word and waits for the
other's before the frame runs. The host is player 1 on mGBA's normal keys, the guest is player 2
on *its* mGBA's normal keys. No screen share: each player watches their own window. A tiny Python
relay pairs the two connections, so only the relay's machine needs a forwarded port.

1. **Relay** (on the host's PC, or any machine both can reach):
   `python tools\netplay_relay.py` listens on TCP 7778. Forward that port on the router of
   whoever runs it (or run it on a VPS); the players connect outbound only.
2. **Host**: open the ROM in mGBA, get to where you want to start (fresh boot, or load a savestate
   made with this ROM build), then in Tools > Scripting run
   `NETPLAY = { role = "host", relay = "127.0.0.1", port = 7778, delay = 2 }  dofile("F:/MeltyCreations/log2/tools/mgba/netplay.lua")`
   (relay = the relay's address as seen from the host).
3. **Guest**: open the ROM in mGBA (any state; it is replaced) and run
   `NETPLAY = { role = "guest", relay = "<relay ip>", port = 7778 }  dofile("<path>/tools/mgba/netplay.lua")`.
   The console on both sides should show "peer connected", the snapshot transfer (~400 KB) and
   "lockstep running"; from then on both windows show the same game and player 2 appears.
4. Every second both sides compare a hash of the game state; a mismatch is logged as **DESYNC**
   and the host re-sends its snapshot. `netplay_status()` prints frame, stalls and checks;
   `netplay_stop()` leaves netplay (the game continues locally as single player).

`delay` is the input lag in frames, the same for both players: inputs pressed at frame N apply at
frame N+delay, so the game never waits as long as one-way latency stays under (delay-1) frames
(16.7 ms each). Use 2 on a LAN, 3-4 across the internet; the guest adopts the host's value.
The host's snapshot includes its save data, so the guest's `.sav` is overwritten by the host's.
Do not load `p2_input.lua`/`coop.lua` in the same mGBA as `netplay.lua`: both write player 2's pad.

Testing on one PC: start two mGBA instances, use `tools\mgba_script.ps1 '<lua>' -Window N` to type
into each one's Scripting console (it opens Tools > Scripting itself), `tools\mgba_input.ps1` to
drive each window, and `tools\mgba_shot.ps1 out.png -Title 'Scripting*' -Of N` for instance N's
console. Make savestates with the current build: player 2's vtable copies in EWRAM hold code
addresses, so a state from an older build crashes (`Jumped to invalid address`) as soon as
player 2 acts.

## Layout

```
asm/     disassembly / split pieces that still link as original binary
src/     decompiled C
tools/   ROM splitting, checking, asset extraction
docs/    reverse-engineering notes (formats, memory map, findings)
build/   build output (ignored)
```
