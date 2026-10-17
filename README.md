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
2. Tools > Scripting, type `dofile("F:/MeltyCreations/log2/tools/mgba/p2_input.lua")` and Run.
3. Run `python tools\p2_keys.py` in a terminal. Player 1 uses mGBA's normal keys; player 2 uses
   IJKL to move, O = A, U = B, [ ] = L R, N = Select, M = Start. Two gamepads: bind one in mGBA,
   adapt `KEYMAP` in `p2_keys.py` for the other. A second player on another PC runs
   `p2_keys.py --host <your ip>` (forward TCP 7777) and watches a screen share.

## Layout

```
asm/     disassembly / split pieces that still link as original binary
src/     decompiled C
tools/   ROM splitting, checking, asset extraction
docs/    reverse-engineering notes (formats, memory map, findings)
build/   build output (ignored)
```
