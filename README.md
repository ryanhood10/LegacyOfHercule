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

## Layout

```
asm/     disassembly / split pieces that still link as original binary
src/     decompiled C
tools/   ROM splitting, checking, asset extraction
docs/    reverse-engineering notes (formats, memory map, findings)
build/   build output (ignored)
```
