# ROM map (LoG2, USA)

Working notes. Addresses are ROM offsets unless prefixed `0x08`.

## Header / startup

- `0x000000` GBA header, entry `b 0x80000c0`.
- `0x0000c0` ARM-mode crt0, hand-written (no GCC/ADS runtime signatures):
  - sets IRQ stack `0x03007FA0`, user stack `0x03007E00`
  - `WAITCNT (0x04000204) = 0x4014`
  - DMA3 zero-fill BSS: dst `0x03000E7C`, `0x2AA3` words (~10.9 KB)
  - DMA3 copy `.data`: src `0x087FD6BC` -> dst `0x03000000`, `0xE7F` bytes (initialized IWRAM data lives at the very end of the ROM)
  - `lr = 0x08000159` (Thumb), `b 0x0802369C` (main)

## IWRAM image (`0x7FD6BC`, 0xE7F bytes -> `0x03000000`)

Starts with ARM code: a bitstream decoder loop (`adds r6,r6,r6 / bcs`, reloads a 32-bit word every
31 bits via `ldrmi r6,[r1],#4; movmi sl,#31`, helper at `0x0300024C`). This is a custom
LZ/Huffman-style decompressor kept in IWRAM for speed; almost certainly the "everything is
compressed" codec. Later in the image: pointers `0x08020BD9` (Thumb fn), `0x083ED210`, `0x030022B8`,
a block of eight `0x04000000` and a run of `0x08020D25` entries (handler/jump table with a default).

## Language / toolchain

- C++. Assertion strings embed source paths: `K:\Source\CharacterData.cpp`, `K:\Source\AreaEntry.cpp`,
  `K:\Source\ByteCodeInterpreter\ByteCodeFunctionList.cpp`, `...\ByteCodeVariableList.cpp`.
  Buu's Fury has the same files under `T:\Source\` plus `ByteCodeInterpreter.cpp`, `GameData.cpp`.
- `"Assertion Failure!"` at `0x01F984`.
- No `__rt_*` (ADS), no `GCC:`/libgcc/newlib, no scatter-load tables. Compiler still unidentified; a
  non-matching (shiftable) decomp is the plan, so this is low priority.

## main

- `0x02369C` (ARM): `ldr r12,[pc]; bx r12; .word 0x08021D75` -> Thumb `main` at `0x021D74`.
- `0x0236A8`: Thumb->ARM veneer (`bx pc; nop; ldr pc,[pc,#-4]; .word 0x03000000`) into the IWRAM image.

## Regions

Code extent measured by `bl` sites that resolve to functions called from >= 2 places: dense through
`0x027FFF`, zero from `0x028000` on. ~4,900 distinct `bl` targets.

| Range | Content |
|---|---|
| `0x000000 - 0x028000` | Thumb code (160 KB), rodata interleaved (assert strings at `0x42F4`, `0xA0F4`, `0xB148`, `0xEBDC`, `0x1F984`) |
| `0x028000 - ~0x040000` | rodata / string tables (menu text etc.) |
| `0x040000 - ~0x150000` | 8-bit PCM audio samples (voice clips, SFX): smooth signed waveforms, ~1.1 MB |
| `~0x150000 - 0x3B0000` | pointer-rich tables, mixed data |
| `0x1D6241` | credits text (Webfoot Technologies, 2003) |
| `0x3B0000 - 0x400000` | dialogue text + pointer tables (e.g. `0x3B8DEE` "You found the Saiyan Battle Armor!") |
| `0x410000 - 0x4D0000` | no pointers, ~1% zero bytes: compressed assets |
| `0x4E0000 - 0x6B0000` | pointer-rich, mixed |
| `0x6D0000 - 0x700000` | scouter "exhibit" descriptions (`0x6FA044` Dr. Gero ...) |
| `0x700000 - 0x7FD6BC` | compressed assets |
| `0x7FD6BC - 0x7FE53B` | `.data` image copied to IWRAM at boot |

## Buu's Fury overlap

Byte-identical 32-byte chunks of LoG2's code region found in Buu's Fury: ~1% overall, 15% in
`0x020000-0x030000` (likely tables / a shared library). Low byte identity is expected for shifted
Thumb code (`bl` offsets and literal pools move), so shared-source estimates need function-level
matching (Ghidra Version Tracking / BSim) rather than byte comparison.

## Functions (Ghidra, seeded)

Ghidra's auto-discovery on the raw ROM is useless here (69k fake functions in data, 489 real ones), so
`tools/ghidra_import.ps1` seeds it instead: `seed_functions.py` collects `bl` targets in the code
region plus Thumb pointers found in literal pools and pointer tables, `SeedFunctions.java` creates
those functions with a small analyzer whitelist.

- 987 seeds -> **1,020 functions**, 1,011 inside `0x000000-0x028000`, 985 Thumb.
- 76 KB of the 160 KB code region is covered by functions; the rest is interleaved rodata and
  anything only reachable through RAM-resident pointers.
- Median function is 46 bytes (lots of small C++ methods). Largest: `0x022118` (1428 B),
  `0x017ACC`, `0x01204E`, `0x00379C`, `0x0226AC`, `0x019320`.
- Thumb pointer tables into code live at `0x3C0000-0x440000` (~3,800 entries) and
  `0x6C0000-0x700000` (~1,500 entries): callback tables for the script/entity system.

## Open questions

- Which compiler (codegen idioms: division helper calling convention, prologue style, literal pools).
- Bytecode script format (opcode table exists in Arefu/Legacy research repo).
- Asset compression scheme(s).
