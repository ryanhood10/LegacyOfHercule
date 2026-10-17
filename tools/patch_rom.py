"""Build the co-op ROM: pad the original to 16 MiB, drop the compiled code at 0x800000, rewrite the
vtable slots that redirect into it, and patch function entries with long jumps.

usage: patch_rom.py ORIG.gba coop.bin coop.sym OUT.gba
"""
import struct
import sys

ROM_BASE = 0x08000000
CODE_AT = 0x00800000
ROM_SIZE = 0x01000000

# (vtable address, slot, symbol): slot word becomes (symbol | 1) - vtable
VTABLE_HOOKS = [
    (0x08025A84, 2, "idle_update_hook"),   # idle control state
    (0x080256C4, 2, "walk_update"),        # walk control state
    (0x080258DC, 2, "diag_update"),        # diagonal / free movement control state
    (0x0802408C, 0, "hud_destroy_hook"),   # HUD object
    (0x0802408C, 2, "hud_update_hook"),
    (0x0802408C, 4, "hud_draw_hook"),
]

# (function address, symbol): first 8 bytes become  ldr r3,[pc]; bx r3; .word symbol|1
# The address must be such that (addr + 4) & ~3 == addr + 4, i.e. addr % 4 == 0.
ENTRY_JUMPS = [
    (0x0801EEFC, "input_poll_hook"),       # Input_Poll
]


def load_symbols(path):
    syms = {}
    for line in open(path):
        parts = line.split()
        if len(parts) == 3:
            syms[parts[2]] = int(parts[0], 16)
    return syms


def main():
    orig, blob, symfile, out = sys.argv[1:5]
    rom = bytearray(open(orig, "rb").read())
    code = open(blob, "rb").read()
    syms = load_symbols(symfile)

    if len(rom) > CODE_AT:
        sys.exit(f"original ROM is larger than {CODE_AT:#x}")
    rom += b"\xff" * (ROM_SIZE - len(rom))
    rom[CODE_AT:CODE_AT + len(code)] = code

    for vtable, slot, name in VTABLE_HOOKS:
        target = syms[name] | 1
        rel = (target - vtable) & 0xFFFFFFFF
        off = vtable - ROM_BASE + slot * 4
        old = struct.unpack_from("<I", rom, off)[0]
        struct.pack_into("<I", rom, off, rel)
        print(f"vtable {vtable:08x}[{slot}] {old:08x} -> {rel:08x} ({name} @ {target:08x})")

    for addr, name in ENTRY_JUMPS:
        if addr % 4:
            sys.exit(f"entry jump at {addr:#x} must be 4-byte aligned")
        target = syms[name] | 1
        off = addr - ROM_BASE
        struct.pack_into("<HHI", rom, off, 0x4B00, 0x4718, target)   # ldr r3,[pc,#0]; bx r3; .word
        print(f"entry  {addr:08x} -> {name} @ {target:08x}")

    open(out, "wb").write(rom)
    print(f"wrote {out}: {len(rom)} bytes, code {len(code)} bytes at {ROM_BASE + CODE_AT:#x}")


if __name__ == "__main__":
    main()
