"""Build the co-op ROM: pad the original to 16 MiB, drop the compiled code at 0x800000 and
rewrite the vtable slots that redirect into it.

usage: patch_rom.py ORIG.gba coop.bin coop.sym OUT.gba
"""
import struct
import sys

ROM_BASE = 0x08000000
CODE_AT = 0x00800000
ROM_SIZE = 0x01000000

# (vtable address, slot, symbol): slot word becomes (symbol | 1) - vtable
HOOKS = [
    (0x08025A84, 2, "idle_update_hook"),
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

    for vtable, slot, name in HOOKS:
        target = syms[name] | 1
        rel = (target - vtable) & 0xFFFFFFFF
        off = vtable - ROM_BASE + slot * 4
        old = struct.unpack_from("<I", rom, off)[0]
        struct.pack_into("<I", rom, off, rel)
        print(f"{name:20s} {target:08x} -> vtable {vtable:08x}[{slot}] word {old:08x} -> {rel:08x}")

    open(out, "wb").write(rom)
    print(f"wrote {out}: {len(rom)} bytes, code {len(code)} bytes at {ROM_BASE + CODE_AT:#x}")


if __name__ == "__main__":
    main()
