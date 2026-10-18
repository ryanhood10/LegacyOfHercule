"""Sprite atlas for Dragon Ball Z: Buu's Fury (US, BG3E): decodes every record in the Buu sprite
index and writes
  build/buu_sprites/atlas.png     one cell per sprite id: first frame of the first animation, Down/Up/Left/Right
  build/buu_sprites/<id>.png      per sprite: first frame of every animation x 4 directions
  build/buu_sprites/report.md     id, record address, animations, frames per animation, max frame box, remap flag

Format (worked out from the ROM, 2026-10-17; see docs/sprites.md "Buu's Fury"):
  sprite index  0x086B6BDC, 297 record pointers (id 0..296), referenced by code at 0x3C0 and 0x1715C
  OBJ palette   0x0805652C, 256 x BGR555 (index 0 transparent, 1 black, 2 white, 3 red, ...)
  record        +0x00 u8 animation count, u8 0, u8 0, u8 flags; +0x04 0; +0x08 0x040A0810 (varies in a
                few records); +0x0C 0; +0x10 four {s32 x, y, w, h} boxes; +0x50 0; +0x54 u32 anim[count]
                (0 = no such pose); an optional 256-byte colour remap table follows the pointers
  animation     u32 frames, then frames x 4 descriptor pointers (down, up, left, right)
  descriptor    u32 parts, {s8 x, s8 y, u8 w, u8 h} frame box, parts x {s8 x, s8 y, u8 w, u8 h,
                u32 attr0 | attr1 << 16, u32 blob} - the 12-byte part is LoG2's descriptor
  blob          {u32 kind (1), u32 size = w*h, stream} for the same IWRAM decompressor as LoG2
                (tools/mapchunks.py), 8-bit 1D-mapped OBJ tiles; right-facing frames reuse the left
                blob with attr1 bit 12 (h-flip)
usage: buu_sprites.py roms/buu.gba [outdir]
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mapchunks as m  # noqa: E402
import sprites as S  # noqa: E402

ROM_BASE = 0x08000000
INDEX = 0x086B6BDC
PALETTE = 0x0805652C
RECORDS_LO, RECORDS_HI = 0x086A8000, INDEX


class BuuSprites:
    def __init__(self, rom):
        self.rom = rom
        self.pal = [S.Sprites.rgb(c) for c in struct.unpack_from("<256H", rom, PALETTE - ROM_BASE)]
        self.blobs = {}
        self.ids = []
        n = 0
        while True:
            v = self.u32(INDEX + 4 * n)
            if not (RECORDS_LO <= v < RECORDS_HI):
                break
            self.ids.append(v)
            n += 1

    def u32(self, a):
        return struct.unpack_from("<I", self.rom, a - ROM_BASE)[0]

    def is_ptr(self, v):
        return ROM_BASE <= v < ROM_BASE + len(self.rom)

    def record(self, sid):
        """{'addr', 'count', 'flags', 'boxes', 'anims' (0 or ROM address), 'remap' (bytes or None)}"""
        r = self.ids[sid]
        end = self.ids[sid + 1] if sid + 1 < len(self.ids) else INDEX
        count = self.u32(r) & 0xFF
        anims = [self.u32(r + 0x54 + 4 * k) for k in range(count)]
        tail = end - (r + 0x54 + 4 * count)
        remap = self.rom[end - ROM_BASE - 256:end - ROM_BASE] if tail == 256 else None
        boxes = [struct.unpack_from("<iiii", self.rom, r - ROM_BASE + 0x10 + 16 * k) for k in range(4)]
        return dict(addr=r, count=count, flags=self.u32(r) >> 24, word8=self.u32(r + 8), boxes=boxes,
                    anims=anims, remap=remap, header=self.rom[r - ROM_BASE:r - ROM_BASE + 0x54])

    def anim(self, a):
        """[frame][direction] -> descriptor address"""
        nf = self.u32(a)
        return [[self.u32(a + 4 + (f * 4 + d) * 4) for d in range(4)] for f in range(nf)]

    def descriptor(self, d):
        """((x, y, w, h) frame box, [(x, y, w, h, attr, blob), ...] parts)"""
        n = self.u32(d)
        box = struct.unpack_from("<bbBB", self.rom, d - ROM_BASE + 4)
        parts = []
        for i in range(n):
            q = d + 8 + 12 * i
            x, y, w, h = struct.unpack_from("<bbBB", self.rom, q - ROM_BASE)
            parts.append((x, y, w, h, self.u32(q + 4), self.u32(q + 8)))
        return box, parts

    def blob(self, b):
        if b not in self.blobs:
            try:
                self.blobs[b] = m.unpack(self.rom, b)
            except Exception:  # noqa: BLE001
                self.blobs[b] = b""
        return self.blobs[b]

    def part_pixels(self, w, h, attr, b):
        """Unflipped tile data -> [h][w] palette indices with the OAM flips applied."""
        data = self.blob(b)
        if len(data) < w * h:
            return None
        hflip, vflip = (attr >> 16) & 0x1000, (attr >> 16) & 0x2000
        px = [[0] * w for _ in range(h)]
        tw = w // 8
        for yy in range(h):
            for xx in range(w):
                ci = data[((yy // 8) * tw + xx // 8) * 64 + (yy % 8) * 8 + xx % 8]
                px[h - 1 - yy if vflip else yy][w - 1 - xx if hflip else xx] = ci
        return px

    def frame_pixels(self, d, remap=None):
        """Composite every part of a descriptor: ((x0, y0, w, h), [h][w] indices), origin-relative."""
        box, parts = self.descriptor(d)
        if not parts:
            return None
        x0 = min(p[0] for p in parts)
        y0 = min(p[1] for p in parts)
        x1 = max(p[0] + p[2] for p in parts)
        y1 = max(p[1] + p[3] for p in parts)
        w, h = x1 - x0, y1 - y0
        out = [[0] * w for _ in range(h)]
        for (x, y, pw, ph, attr, b) in parts:
            px = self.part_pixels(pw, ph, attr, b)
            if px is None:
                continue
            for yy in range(ph):
                for xx in range(pw):
                    ci = px[yy][xx]
                    if ci:
                        out[y - y0 + yy][x - x0 + xx] = remap[ci] if remap else ci
        return (x0, y0, w, h), out

    def draw(self, canvas, ox, oy, d, remap=None, scale=1):
        """Draw a frame with its origin at (ox, oy)."""
        f = self.frame_pixels(d, remap)
        if not f:
            return
        (x0, y0, w, h), px = f
        for yy in range(h):
            for xx in range(w):
                ci = px[yy][xx]
                if ci:
                    c = self.pal[ci]
                    for sy in range(scale):
                        for sx in range(scale):
                            canvas.put(ox + (x0 + xx) * scale + sx, oy + (y0 + yy) * scale + sy, c)


def main():
    rom = open(sys.argv[1], "rb").read()
    out = sys.argv[2] if len(sys.argv) > 2 else "build/buu_sprites"
    os.makedirs(out, exist_ok=True)
    sp = BuuSprites(rom)
    cell, cols = 72, 8
    rows = (len(sp.ids) + cols - 1) // cols
    atlas = S.Canvas(cols * cell * 4 + 8, rows * (cell + 12) + 8)
    report = ["| id | record | flags | anims | frames per anim | max frame | parts>1 | remap |", "|---|---|---|---|---|---|---|---|"]
    for sid in range(len(sp.ids)):
        rec = sp.record(sid)
        anims = [sp.anim(a) if a else [] for a in rec["anims"]]
        maxw = maxh = 0
        multi = 0
        for frames in anims:
            for fr in frames:
                for d in fr:
                    box, parts = sp.descriptor(d)
                    maxw, maxh = max(maxw, box[2]), max(maxh, box[3])
                    multi += len(parts) > 1
        per = " ".join(str(len(f)) for f in anims)
        report.append(f"| {sid} | {rec['addr']:#x} | {rec['flags']:#x} | {rec['count']} | {per} | {maxw}x{maxh} | {multi} | {'yes' if rec['remap'] else ''} |")
        ax = 4 + (sid % cols) * cell * 4
        ay = 4 + (sid // cols) * (cell + 12)
        atlas.text(ax, ay, str(sid), (255, 255, 0), 2)
        first = next((f for f in anims if f), None)
        if first:
            for d in range(4):
                sp.draw(atlas, ax + d * cell + cell // 2, ay + 12 + cell - 8, first[0][d], rec["remap"])
        sheet = S.Canvas(4 * cell + 40, max(1, len(anims)) * cell + 4)
        for gi, frames in enumerate(anims):
            sheet.text(2, gi * cell + 2, str(gi), (255, 255, 0), 2)
            if frames:
                for d in range(4):
                    sp.draw(sheet, 40 + d * cell + cell // 2, gi * cell + cell - 6, frames[0][d], rec["remap"])
        sheet.save(os.path.join(out, f"{sid}.png"))
    atlas.save(os.path.join(out, "atlas.png"))
    open(os.path.join(out, "report.md"), "w", encoding="utf-8").write("\n".join(report) + "\n")
    print(f"{len(sp.ids)} sprites -> {out}/atlas.png, per-sprite sheets, report.md")


if __name__ == "__main__":
    main()
