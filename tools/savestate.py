"""Read an mGBA GBA savestate (raw or PNG-wrapped) and dump memory regions or words.

usage: savestate.py STATE.ss1 dump iwram|ewram|vram|oam|pram|io OUT.bin
       savestate.py STATE.ss1 words ADDR COUNT        (hex ADDR in GBA address space)
"""
import struct
import sys
import zlib

# GBASerializedState layout (mgba/src/gba/serialize.h)
REGIONS = {
    "io":    (0x00200, 0x400),
    "pram":  (0x00600, 0x400),
    "oam":   (0x00A00, 0x400),
    "vram":  (0x00E00, 0x18000),
    "iwram": (0x18E00, 0x8000),
    "ewram": (0x20E00, 0x40000),
}
BASES = {0x02000000: "ewram", 0x03000000: "iwram", 0x04000000: "io", 0x05000000: "pram", 0x06000000: "vram", 0x07000000: "oam"}


def load(path):
    data = open(path, "rb").read()
    if data[:8] == b"\x89PNG\r\n\x1a\n":
        pos = 8
        while pos < len(data):
            length, ctype = struct.unpack_from(">I4s", data, pos)
            body = data[pos + 8:pos + 8 + length]
            if ctype == b"gbAs":
                return zlib.decompress(body)
            pos += 12 + length
        sys.exit("no gbAs chunk in PNG savestate")
    return data


def region(state, name):
    off, size = REGIONS[name]
    return state[off:off + size]


def read_words(state, addr, count):
    base = addr & 0xFF000000
    name = BASES[base]
    mem = region(state, name)
    off = addr - base
    if name == "iwram":
        off &= 0x7FFF
    return struct.unpack_from("<%dI" % count, mem, off)


def main():
    state = load(sys.argv[1])
    cmd = sys.argv[2]
    if cmd == "dump":
        open(sys.argv[4], "wb").write(region(state, sys.argv[3]))
        print(f"wrote {sys.argv[3]} -> {sys.argv[4]}")
    elif cmd == "words":
        addr, count = int(sys.argv[3], 16), int(sys.argv[4])
        for i, w in enumerate(read_words(state, addr, count)):
            print(f"{addr + i*4:08x}: {w:08x}")


if __name__ == "__main__":
    main()
