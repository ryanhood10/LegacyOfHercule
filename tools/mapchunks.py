"""Map chunk tools for the wide-view work: a Python port of the game's IWRAM decompressor
(ROM 0x087FD6BC -> IWRAM 0x03000000, entry 0x03000000) and a savestate reader that finds the
background layer objects, decompresses their map chunks from the ROM and checks the result against
the 2 KB chunk caches the game keeps in RAM.

usage: mapchunks.py roms/log2.gba build/log2-coop.ss1
"""
import struct
import sys
import zlib

ROM_BASE = 0x08000000


class BitReader:
    """32-bit little-endian words, bits consumed MSB first (mirrors the ARM code's r6/sl)."""

    def __init__(self, data, pos):
        self.data, self.pos = data, pos
        self.buf, self.left = 0, 0

    def _load(self):
        self.buf = struct.unpack_from("<I", self.data, self.pos)[0]
        self.pos += 4

    def bit(self):
        self.left -= 1
        if self.left < 0:
            self._load()
            self.left = 31
        b = (self.buf >> 31) & 1
        self.buf = (self.buf << 1) & 0xFFFFFFFF
        return b

    def bits(self, n):
        if n == 0:
            return 0
        if self.left == 0:
            self._load()
            self.left = 32
        take = min(n, self.left)
        v = self.buf >> (32 - take)
        self.buf = (self.buf << take) & 0xFFFFFFFF
        self.left -= take
        n -= take
        if n:
            self._load()
            self.left = 32 - n
            v = (v << n) | (self.buf >> self.left)
            self.buf = (self.buf << n) & 0xFFFFFFFF
        return v

    def gamma(self):
        v = 1
        while True:
            v = (v << 1) | self.bit()
            if not self.bit():
                return v


def decompress(src, pos, lit_bits, want_end=False):
    """Port of the ARM routine at 0x03000000. src[pos:] is the bitstream (header + 8),
    lit_bits is r3 at entry (the header's word 0). Returns the output bytes, or (bytes, end offset
    of the last 32-bit word read) with want_end (the stream's size in the ROM, for copying it)."""
    out = bytearray()
    r = BitReader(src, pos)
    off_bits = 8      # r5
    last_off = 1      # ip
    base = 0          # r4

    def copy(dist, n):
        for _ in range(n):
            out.append(out[-dist])

    while True:
        if r.bit():                                   # literal
            out.append((r.bits(lit_bits) + base) & 0xFF)
            continue
        if r.bit():                                   # gamma-coded match
            g = r.gamma()
            if g == 2:                                # repeat last offset
                n = r.gamma()
                copy(last_off, n)
                continue
            hi = g - 3
            lo = r.bits(off_bits)
            last_off = lo + (hi << off_bits)
            n = r.gamma()
            if last_off >= 0x10000:
                n += 3
            elif last_off >= 0x37FF:
                n += 2
            elif last_off >= 0x27F:
                n += 1
            elif last_off <= 127:
                n += 4
            copy(last_off, n)
            continue
        if r.bit():                                   # short codes
            v = r.bits(4) - 1
            if v == 0:
                out.append(0)
            elif v > 0:
                out.append(out[-v])                   # 1 byte from distance v
            else:                                     # v == -1
                if r.bit():
                    while True:                       # runs of 256 raw bytes
                        for _ in range(256):
                            out.append(r.bits(8))
                        if not r.bit():
                            break
                else:
                    base = 0
                    lit_bits = 7 + r.bit()
                    if lit_bits != 8:
                        base = r.bits(8)
            continue
        # bit == 0: short match or control
        o = r.bits(7)
        if o != 0:
            last_off = o
            n = r.bits(2) + 2
            copy(last_off, n)
            continue
        k = r.bits(2)
        if k == 0:
            return (bytes(out), r.pos) if want_end else bytes(out)   # end of stream
        off_bits = r.bits(k + 3)


def unpack(rom, addr):
    """FUN_0801F394: header word 0 = 0 plain copy (word 1 = size, data at +8), 1/2 = compressed."""
    off = addr - ROM_BASE
    kind, size = struct.unpack_from("<II", rom, off)
    if kind == 0:
        return rom[off + 8:off + 8 + size]
    if kind in (1, 2):
        return decompress(rom, off + 8, kind)
    raise ValueError(f"unknown chunk kind {kind} at {addr:#x}")


# ---- savestate ------------------------------------------------------------------------------
OFF_PRAM, OFF_OAM, OFF_VRAM, OFF_IWRAM, OFF_WRAM = 0x800, 0xC00, 0x1000, 0x19000, 0x21000


def load_state(path):
    data = open(path, "rb").read()
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    pos = 8
    while pos < len(data):
        (length,) = struct.unpack_from(">I", data, pos)
        name = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + length]
        if name == b"gbAs":
            state = zlib.decompress(body)
            assert len(state) == 0x61000, len(state)
            return state
        pos += 12 + length
    raise ValueError("no gbAs chunk")


class Mem:
    def __init__(self, rom, state):
        self.rom, self.state = rom, state

    def read(self, addr, n):
        if 0x02000000 <= addr < 0x02040000:
            o = OFF_WRAM + (addr - 0x02000000)
            return self.state[o:o + n]
        if 0x03000000 <= addr < 0x03008000:
            o = OFF_IWRAM + (addr - 0x03000000)
            return self.state[o:o + n]
        if 0x06000000 <= addr < 0x06018000:
            o = OFF_VRAM + (addr - 0x06000000)
            return self.state[o:o + n]
        if ROM_BASE <= addr < ROM_BASE + len(self.rom):
            o = addr - ROM_BASE
            return self.rom[o:o + n]
        raise ValueError(f"unmapped {addr:#x}")

    def u32(self, addr):
        return struct.unpack("<I", self.read(addr, 4))[0]

    def u8(self, addr):
        return self.read(addr, 1)[0]


LAYER_TABLE = 0x0300107C   # four BG layer object pointers (one per hardware background)


def find_layers(mem):
    """Layer object: +0 vtable 0x08023868, +4 BG index (low byte), +8 map header (ROM: +4/+8 scale
    x/y in 1/1024, +0xC/+0x10 base offset, +0x14/+0x15 chunk grid w/h, +0x18 inline chunk pointer table),
    +0xC..+0x18 the four chunk cache pointers, +0x1C four chunk ids (TL TR BL BR), +0x20/+0x24
    chunk window x/y, +0x28 four 2 KB caches."""
    return [mem.u32(LAYER_TABLE + 4 * i) for i in range(4)]


def main():
    rom = open(sys.argv[1], "rb").read()
    state = load_state(sys.argv[2])
    mem = Mem(rom, state)
    ok = bad = 0
    for layer in find_layers(mem):
        bg = mem.u8(layer + 4)
        if not (0x02000000 <= layer < 0x02040000):
            continue
        hdr = mem.u32(layer + 8)
        if not (ROM_BASE <= hdr < ROM_BASE + len(rom)):
            continue
        w, h = mem.u8(hdr + 0x14), mem.u8(hdr + 0x15)
        table = hdr + 0x18   # inline pointer table: chunk id -> chunk data address
        ids = list(mem.read(layer + 0x1C, 4))
        cx, cy = mem.u32(layer + 0x20), mem.u32(layer + 0x24)
        print(f"layer @{layer:08x} bg{bg} map {hdr:08x} {w}x{h} chunks, table {table:08x}, window chunk ({cx},{cy}) ids {ids}")
        for slot, cid in enumerate(ids):
            if cid == 0xFF:
                continue
            caddr = mem.u32(table + cid * 4)
            cache = mem.read(layer + 0x28 + slot * 0x800, 0x800)
            try:
                data = unpack(rom, caddr)
            except Exception as e:  # noqa: BLE001
                print(f"  chunk {cid} @{caddr:08x}: decode error {e}")
                bad += 1
                continue
            match = data[:0x800] == cache
            ok += match
            bad += not match
            kind = struct.unpack_from("<I", rom, caddr - ROM_BASE)[0]
            print(f"  chunk {cid:3d} @{caddr:08x} kind {kind} -> {len(data)} bytes, cache match: {match}")
    print(f"chunks ok={ok} bad={bad}")


if __name__ == "__main__":
    main()
