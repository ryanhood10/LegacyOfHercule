"""Sprite atlas for Legacy of Goku II (US): decodes every sprite record in g_CharacterSpriteIndex
and writes
  build/sprites/atlas.png      one row per sprite id: first frame of group 0, facing Down/Up/Left/Right
  build/sprites/<id>.png       per sprite: first frame of every group x 4 directions (poses)
  build/sprites/report.md      id, record address, groups, frames per group, max frame size
Facts used: index at 0x083B4E74 (ids 3..156), record = 0x4C header + 16-byte groups of 4 direction
pointers (ends at the first word that is not a ROM pointer), descriptor = {s8 xoff, s8 yoff, u8 w, u8 h,
u32 attr0|attr1<<16, u32 blob}, blob = {u32 kind, u32 size, stream} decoded by the game's IWRAM
decompressor (tools/mapchunks.py port), frames are 8-bit 1D-mapped OBJ tiles, shared OBJ palette at
0x081DA6C8.  usage: sprites.py roms/log2.gba [outdir]
"""
import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mapchunks as m  # noqa: E402

ROM_BASE = 0x08000000
INDEX = 0x083B4E74
PALETTE = 0x081DA6C8
MAX_GROUPS = 24

FONT = {  # 3x5 digits for labels
    "0": ["111", "101", "101", "101", "111"], "1": ["010", "110", "010", "010", "111"],
    "2": ["111", "001", "111", "100", "111"], "3": ["111", "001", "111", "001", "111"],
    "4": ["101", "101", "111", "001", "001"], "5": ["111", "100", "111", "001", "111"],
    "6": ["111", "100", "111", "101", "111"], "7": ["111", "001", "001", "001", "001"],
    "8": ["111", "101", "111", "101", "111"], "9": ["111", "101", "111", "001", "111"],
}


class Canvas:
    def __init__(self, w, h, bg=(40, 40, 40)):
        self.w, self.h = w, h
        self.px = bytearray(bg * (w * h))

    def put(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            i = (y * self.w + x) * 3
            self.px[i:i + 3] = bytes(c)

    def text(self, x, y, s, c=(255, 255, 255), scale=1):
        for ch in s:
            g = FONT.get(ch)
            if g:
                for r, row in enumerate(g):
                    for q, bit in enumerate(row):
                        if bit == "1":
                            for sy in range(scale):
                                for sx in range(scale):
                                    self.put(x + q * scale + sx, y + r * scale + sy, c)
            x += 4 * scale

    def save(self, path):
        raw = b"".join(b"\x00" + bytes(self.px[y * self.w * 3:(y + 1) * self.w * 3]) for y in range(self.h))

        def chunk(t, d):
            c = t + d
            return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)
        open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", self.w, self.h, 8, 2, 0, 0, 0))
                               + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


class Sprites:
    def __init__(self, rom):
        self.rom = rom
        self.pal = [self.rgb(struct.unpack_from("<H", rom, PALETTE - ROM_BASE + i * 2)[0]) for i in range(256)]
        self.blobs = {}

    @staticmethod
    def rgb(c):
        return ((c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31)

    def u32(self, a):
        return struct.unpack_from("<I", self.rom, a - ROM_BASE)[0]

    def is_ptr(self, v):
        return ROM_BASE <= v < ROM_BASE + len(self.rom)

    def record(self, sid):
        r = self.u32(INDEX + sid * 4)
        return r if self.is_ptr(r) else 0

    def groups(self, rec):
        out = []
        a = rec + 0x4C
        for _ in range(MAX_GROUPS):
            ptrs = [self.u32(a + 4 * d) for d in range(4)]
            # a direction slot may be empty (0); the record ends at the first word that is neither
            if not any(self.is_ptr(p) for p in ptrs) or not all(p == 0 or self.is_ptr(p) for p in ptrs):
                break
            out.append(ptrs)
            a += 16
        return out

    def frame_counts(self, groups):
        """Descriptor arrays are back to back; an array's length is the gap to the next one (max 16)."""
        arrs = sorted({p for g in groups for p in g if p})
        blobs = []
        for g in groups:
            for p in g:
                if not p:
                    continue
                b = self.u32(p + 8)
                if self.is_ptr(b):
                    blobs.append(b)
        end = min(blobs) if blobs else arrs[-1] + 12
        counts = {}
        for i, a in enumerate(arrs):
            nxt = arrs[i + 1] if i + 1 < len(arrs) else end
            n = max(1, min(16, (nxt - a) // 12))
            counts[a] = n
        return counts

    def descriptor(self, arr, fi):
        xo, yo, w, h = struct.unpack_from("<bbBB", self.rom, arr - ROM_BASE + fi * 12)
        attr = self.u32(arr + fi * 12 + 4)
        blob = self.u32(arr + fi * 12 + 8)
        return xo, yo, w, h, attr, blob

    def frame_pixels(self, arr, fi):
        if not arr:
            return None
        xo, yo, w, h, attr, blob = self.descriptor(arr, fi)
        if not self.is_ptr(blob) or w == 0 or h == 0 or w > 64 or h > 64:
            return None
        if blob not in self.blobs:
            try:
                self.blobs[blob] = m.unpack(self.rom, blob)
            except Exception:  # noqa: BLE001
                self.blobs[blob] = b""
        data = self.blobs[blob]
        tw, th = w // 8, h // 8
        px = [[0] * w for _ in range(h)]
        for ty in range(th):
            for tx in range(tw):
                base = (ty * tw + tx) * 64
                for py in range(8):
                    row = data[base + py * 8:base + py * 8 + 8]
                    for q, ci in enumerate(row):
                        px[ty * 8 + py][tx * 8 + q] = ci
        return w, h, px

    def draw(self, canvas, x, y, arr, fi):
        f = self.frame_pixels(arr, fi)
        if not f:
            return
        w, h, px = f
        for yy in range(h):
            for xx in range(w):
                ci = px[yy][xx]
                if ci:
                    canvas.put(x + xx, y + yy, self.pal[ci])


def main():
    rom = open(sys.argv[1], "rb").read()
    out = sys.argv[2] if len(sys.argv) > 2 else "build/sprites"
    os.makedirs(out, exist_ok=True)
    sp = Sprites(rom)
    ids = [sid for sid in range(3, 160) if sp.record(sid)]
    cell, cols = 66, 8
    rows = (len(ids) + cols - 1) // cols
    atlas = Canvas(cols * cell * 4 + 8, rows * (cell + 12) + 8)
    report = ["| id | record | groups | frames per group (dir 0) | max frame |", "|---|---|---|---|---|"]
    for n, sid in enumerate(ids):
        rec = sp.record(sid)
        groups = sp.groups(rec)
        counts = sp.frame_counts(groups) if groups else {}
        maxw = maxh = 0
        for g in groups:
            for p in g:
                if not p:
                    continue
                for fi in range(counts.get(p, 1)):
                    _, _, w, h, _, b = sp.descriptor(p, fi)
                    if sp.is_ptr(b):
                        maxw, maxh = max(maxw, w), max(maxh, h)
        per = " ".join(str(counts.get(g[0], 0) if g[0] else 0) for g in groups)
        report.append(f"| {sid} | {rec:#x} | {len(groups)} | {per} | {maxw}x{maxh} |")
        # atlas row: group 0, four directions
        ax = 4 + (n % cols) * cell * 4
        ay = 4 + (n // cols) * (cell + 12)
        atlas.text(ax, ay, str(sid), (255, 255, 0), 2)
        if groups:
            for d in range(4):
                sp.draw(atlas, ax + d * cell, ay + 12, groups[0][d], 0)
        # per-sprite sheet: every group x 4 directions, first frame
        if groups:
            sheet = Canvas(4 * cell + 40, len(groups) * cell + 4)
            for gi, g in enumerate(groups):
                sheet.text(2, gi * cell + 2, str(gi), (255, 255, 0), 2)
                for d in range(4):
                    sp.draw(sheet, 40 + d * cell, gi * cell + 2, g[d], 0)
            sheet.save(os.path.join(out, f"{sid}.png"))
    atlas.save(os.path.join(out, "atlas.png"))
    open(os.path.join(out, "report.md"), "w", encoding="utf-8").write("\n".join(report) + "\n")
    print(f"{len(ids)} sprites -> {out}/atlas.png, per-sprite sheets, report.md")


if __name__ == "__main__":
    main()
