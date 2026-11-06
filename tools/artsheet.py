"""Audit contact sheets of any sprite record of a BUILT ROM (docs/art-audit.md): every group x 4 facings x every frame,
drawn as the engine draws them (h-flip applied, the descriptor's offset on a common ground line, a grey cross at the
entity's position), several ids one under another so a generated body can be read next to the stock one it should match.

usage: python -I tools/artsheet.py BUILT.gba coop.sym OUT.png --ids 165,9 [--scale 4] [--groups 0,1,12] [--first]
                                   [--box] [--side]
  --first   only the first frame of each facing (the overview; a held pose shows frame 0)
  --box     the facing's hurt box in red on group 0
  --side    the ids side by side (default: one under another)
Writes OUT.png at 1x and, with --scale N, OUT_xN.png beside it. Prints one line per id: the groups filled and their
frame counts (facing down).
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402

HF = 0x10000000
NAMES = {0: "idle", 1: "walk", 2: "hitstun", 3: "knockdn", 4: "shout", 5: "charge", 6: "atk1", 7: "atk2", 8: "atk3",
         9: "powerup", 10: "g10", 11: "hurt", 12: "fly/lie", 13: "dash", 14: "fidget", 15: "g15", 16: "special",
         17: "halo", 18: "beam", 19: "beam2", 20: "tumble", 21: "detrans", 22: "g22"}


class Built:
    def __init__(self, rom, sym):
        self.rom, self.sp, self.idx = rom, S.Sprites(rom), sym["sprite_index"]

    def record(self, sid):
        o = self.idx - S.ROM_BASE + 4 * sid
        r = int.from_bytes(self.rom[o:o + 4], "little")
        return r if self.sp.is_ptr(r) else 0

    def groups(self, rec):
        n = self.rom[rec - S.ROM_BASE + 2]
        return [[self.sp.u32(rec + 0x4C + 0x10 * g + 4 * d) for d in range(4)] for g in range(n)]

    def ok(self, a):
        try:
            return self.sp.is_ptr(a) and self.sp.valid_descriptor(a, 0)
        except Exception:  # noqa: BLE001
            return False

    def frames(self, starts, arr):
        n = 1
        while n < 16 and arr + 12 * n not in starts and self.ok(arr + 12 * n):
            n += 1
        return n


def sheet(b, ids, only, first, box, side=False):
    from PIL import Image, ImageDraw
    sp = b.sp
    blocks = []
    for sid in ids:
        rec = b.record(sid)
        if not rec:
            print(f"{sid}: not in the build")
            continue
        groups = b.groups(rec)
        starts = {p for g in groups for p in g if p} | sp.array_starts()
        rows, big, info = [], False, []
        for gi, g in enumerate(groups):
            if only and gi not in only:
                continue
            if not any(g):
                continue
            cells = []
            for d in range(4):
                if not b.ok(g[d]):
                    cells.append([])
                    continue
                n = 1 if first else b.frames(starts, g[d])
                cells.append([(g[d], fi) for fi in range(n)])
                for fi in range(n):
                    _, _, w, h, _, _ = sp.descriptor(g[d], fi)
                    big = big or w > 32 or h > 32
            info.append(f"{gi}:{b.frames(starts, g[0]) if b.ok(g[0]) else 0}")
            rows.append((gi, cells))
        print(f"{sid}: record {rec:#x}, {len(groups)} groups, filled {' '.join(info)}")
        blocks.append((sid, rec, rows, big))
    if not blocks:
        return None
    lab = 54
    width, height, lay = 0, 0, []
    for sid, rec, rows, big in blocks:
        cell = 72 if big else 40
        per = [max(len(c[d]) for _, c in rows) for d in range(4)] if rows else [1] * 4
        w = lab + sum(p * cell + 6 for p in per)
        if side:
            lay.append((cell, per, width, 0, w))
            width, height = width + w + 8, max(height, 12 + len(rows) * cell)
        else:
            lay.append((cell, per, 0, height, w))
            width, height = max(width, w), height + 12 + len(rows) * cell
    im = Image.new("RGB", (width, height), (70, 90, 70))
    dr = ImageDraw.Draw(im)
    for (sid, rec, rows, big), (cell, per, x0, y0, bw_) in zip(blocks, lay):
        dr.rectangle([x0, y0, x0 + bw_ - 1, y0 + 11], fill=(40, 50, 40))
        dr.text((x0 + 2, y0), f"sprite {sid}  d / u / l / r", fill=(255, 255, 0))
        for r, (gi, cells) in enumerate(rows):
            y = y0 + 12 + r * cell
            dr.text((x0 + 2, y + 2), f"{gi}", fill=(255, 255, 0))
            dr.text((x0 + 2, y + 13), NAMES.get(gi, ""), fill=(200, 220, 200))
            x = x0 + lab
            for d in range(4):
                for k, (arr, fi) in enumerate(cells[d]):
                    xo, yo, w, h, attr, _ = sp.descriptor(arr, fi)
                    fp = sp.frame_pixels(arr, fi)
                    ox, oy = x + k * cell + cell // 2, y + cell - (20 if cell > 44 else 8)
                    for q in range(-2, 3):
                        im.putpixel((ox + q, oy), (120, 140, 120))
                        im.putpixel((ox, min(height - 1, oy + q)), (120, 140, 120))
                    if not fp:
                        continue
                    for yy in range(h):
                        for xx in range(w):
                            v = fp[2][yy][w - 1 - xx] if attr & HF else fp[2][yy][xx]
                            px, py = ox + xo + xx, oy + yo + yy
                            if v and 0 <= px < width and 0 <= py < height:
                                im.putpixel((px, py), sp.pal[v])
                    if box and gi == 0 and k == 0:
                        bx, by, bw, bh = struct.unpack_from("<4i", b.rom, rec - S.ROM_BASE + 0xC + 0x10 * d)
                        dr.rectangle([ox + bx, oy + by, ox + bx + bw - 1, oy + by + bh - 1], outline=(255, 0, 0))
                x += per[d] * cell + 6
                dr.line([x - 4, y, x - 4, y + cell - 1], fill=(50, 65, 50))
    return im


def main():
    a = sys.argv
    rom = open(a[1], "rb").read()
    sym = {}
    for line in open(a[2]):
        p = line.split()
        if len(p) == 3:
            sym[p[2]] = int(p[0], 16)
    out = a[3]
    ids = [int(v) for v in a[a.index("--ids") + 1].split(",")]
    only = [int(v) for v in a[a.index("--groups") + 1].split(",")] if "--groups" in a else None
    scale = int(a[a.index("--scale") + 1]) if "--scale" in a else 0
    im = sheet(Built(rom, sym), ids, only, "--first" in a, "--box" in a, "--side" in a)
    if im is None:
        return
    from PIL import Image
    im.save(out)
    if scale > 1:
        im.resize((im.width * scale, im.height * scale), Image.NEAREST).save(out[:-4] + f"_x{scale}.png")
    print(out)


if __name__ == "__main__":
    main()
