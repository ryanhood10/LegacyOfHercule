"""Mr. Popo's body for the Time Chamber's hidden room (docs/time-chamber.md 5; docs/sprites.md "Mr. Popo"): ONE sprite record
in Hercule's 21-group player layout (sprite 72's groups and frame counts), so the melee boss AI set 21's groups (0-2, 5-8)
are all there whether or not his AI ever plays them, and the same record is his scene actor (idle 0, walk 1).

usage: popoart.py roms/log2.gba build/gen/popo_sprites_data.c --buu roms/buu.gba|none [--preview-dir DIR]

Sources (nothing from a ROM is committed; everything is derived at build time into build/gen):
  with roms/buu.gba   Buu's Fury record 183: idle (2 frames), walk (4) and anim 12 (2 frames, arms held out: the blast and
                      the teleport's charge), 32 x 32 front / back and 16 x 32 side views, mapped to LoG2's OBJ palette by
                      the nearest colour (buu2log2.nearest_table).
  without it (BUU=0)  LoG2's own sprite 93 (idle 2, walk 4; 16 x 32): the same drawing; his idle stands in for the
                      arms-out pose.
Drawn by code from those frames, as tools/tnysprites.py does for the tournament's fighters: the hit-stun (the idle frame
leaning back: the side views sheared away from the way he faces, the front and back views flinched two rows shorter at
the waist, the stock hit-stun's white marks over the head for its first two frames), the knock-down (the hit-stun, then
lying), the lying pose (the hit-stun turned on its side, tools/lying.py) and, since 2026-11-06, the side views' attacks: a
palm strike (a bare arm straight out, the gold bracelet, the open hand: 6 at chest height, 7 raised, 8 both hands; the arms-out
frames are a front view and showed nothing from the side). Groups: 0 idle, 1 walk, 2 / 11 hit-stun, 3 knock-down, 4 / 5 the arms
out, 6 / 7 / 8 the arms out (front, back) or the palm strikes (sides), 12 lying, 13 / 17 idle, 20 the tumble (hit-stun then
lying); the template's empty groups stay empty.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402
import mapchunks as m  # noqa: E402
import lying as LY  # noqa: E402
from buu2log2 import compress, nearest_table  # noqa: E402

B = 0x08000000
TEMPLATE = 72            # Hercule: the player layout (which groups, frames per group, header)
BUU_POPO = 183           # Buu's Fury: Mr. Popo (idle 0, walk 1, arms out 12)
LOG2_POPO = 93           # LoG2: Mr. Popo (idle 0, walk 1)
HF = 0x10000000
BOX = (-2, -21, 18, 10)  # the hurt box, every facing: LoG2's standard body box (-2, -26, 18, 10) lowered to his height
                         # (Buu's own top is -19, LoG2 93's -19): a punch at arm's length lands as on any 32 x 32 body


def tiles(px, w, h):
    data = bytearray(w * h)
    tw = w // 8
    for ty in range(h // 8):
        for tx in range(tw):
            base = (ty * tw + tx) * 64
            for py in range(8):
                for q in range(8):
                    data[base + py * 8 + q] = px[ty * 8 + py][tx * 8 + q]
    return bytes(data)


def attr_of(w, h):
    shape, size = LY.OBJ_ATTR[(w, h)]
    return 0x2000 | (shape << 14) | (size << 30)          # 8bpp, drawn as stored (no flip)


class Source:
    """frame(anim, f, d) -> (xo, yo, w, h, px): the frame as displayed, LoG2 palette indices, offsets from the position."""
    def __init__(self, log, buu):
        self.l2 = S.Sprites(log)
        self.buu = None
        if buu is not None:
            import buu_sprites as BS
            self.bu = BS.BuuSprites(buu)
            self.rec = self.bu.record(BUU_POPO)
            self.anims = {a: self.bu.anim(p) for a, p in enumerate(self.rec["anims"]) if p}
            self.table = nearest_table(self.bu.pal, self.l2.pal)
            self.buu = True
        else:
            rec = self.l2.record(LOG2_POPO)
            self.groups = self.l2.groups(rec)

    def count(self, anim):
        if self.buu:
            return len(self.anims.get(anim, self.anims[0]))
        g = self.groups[anim] if anim < len(self.groups) else self.groups[0]
        return self.l2.frame_counts(self.groups)[g[0]]

    def frame(self, anim, f, d):
        if self.buu:
            frames = self.anims.get(anim, self.anims[0])
            (x0, y0, w, h), px = self.bu.frame_pixels(frames[f % len(frames)][d], self.rec["remap"])
            px = [[self.table[c][0] if c else 0 for c in row] for row in px]
            return x0, y0, w, h, px
        g = self.groups[anim] if anim < len(self.groups) else self.groups[0]   # 93 has no arms-out group: the idle
        n = self.l2.frame_counts(self.groups)[g[d]]
        xo, yo, w, h, attr, _ = self.l2.descriptor(g[d], f % n)
        _, _, px = self.l2.frame_pixels(g[d], f % n)
        if attr & HF:
            px = [row[::-1] for row in px]
        return xo, yo, w, h, px


def widen(fr, pad):
    """the frame in a canvas `pad` px wider on each side (a 16-wide side view becomes 32 wide)"""
    xo, yo, w, h, px = fr
    return xo - pad, yo, w + 2 * pad, h, [[0] * pad + row + [0] * pad for row in px]


def hurt(fr, d):
    """The hit-stun: the side views lean back (the rows above his feet shifted away from the way he faces, up to 3 px at
    the head), the front and back views flinch two rows shorter at the waist."""
    xo, yo, w, h, px = fr
    rows = [y for y in range(h) if any(px[y])]
    top, bottom = rows[0], rows[-1]
    if d >= 2:
        if w < 32:
            xo, yo, w, h, px = widen(fr, (32 - w) // 2)
        back = 1 if d == 2 else -1                          # facing left he leans right
        out = [[0] * w for _ in range(h)]
        for y in range(h):
            k = round(3 * (bottom - y) / max(1, bottom - top)) * back
            for x in range(w):
                if px[y][x] and 0 <= x + k < w:
                    out[y][x + k] = px[y][x]
        return xo, yo, w, h, out
    waist = top + (bottom - top) * 11 // 20                 # just below the middle: the belt line
    out = [[0] * w for _ in range(h)]
    for y in range(h):
        if waist <= y < waist + 2:
            continue
        ny = y + 2 if y < waist else y                      # everything above the cut drops two rows
        if 0 <= ny < h:
            out[ny] = px[y][:]
    return xo, yo, w, h, out


SPARK = 2            # white: the stock hit-stun frames' marks, the stock attacks' swing arcs
SKIN, GOLD, GOLD2 = 1, 0x25, 0x21


def sparks(fr, d):
    """The stock hit-stun's marks over the head (three short white strokes on the side the blow came from): the front
    and back views' flinch is two rows, too little to read as a hit without them."""
    xo, yo, w, h, px = fr
    out = [row[:] for row in px]
    top = next(y for y in range(h) if any(px[y]))
    xs = [x for x in range(w) if px[top + 2][x]]
    x0 = xs[-1] + 1 if d == 2 else xs[0] - 3        # behind the head in the side views, at its left seen from the front
    for dy, dx in ((-1, 0), (-2, 1), (0, 2), (-3, 2)):
        y, x = top + 1 + dy, x0 + dx if d != 3 else x0 + 2 - dx
        if 0 <= y < h and 0 <= x < w and not out[y][x]:
            out[y][x] = SPARK
    return xo, yo, w, h, out


def palm(fr, d, kind, arc):
    """A side view's palm strike, drawn on the idle frame (32 wide): a bare black arm from the shoulder straight out
    the way he faces, the gold bracelet at the wrist, the open hand upright at its end. kind 0 at chest height, 1
    raised to head height, 2 both hands together (a thicker arm, a taller hand); `arc` adds the stock attacks' white stroke before the hand.
    His front and back views keep Buu's Fury's own arms-out frames."""
    xo, yo, w, h, px = widen(fr, (32 - fr[2]) // 2) if fr[2] < 32 else fr
    out = [row[:] for row in px]
    rows = [y for y in range(h) if any(px[y])]
    top, bottom = rows[0], rows[-1]
    sh = top + (bottom - top) * 13 // 24                     # the shoulder row (his vest's top)
    xs = [x for x in range(w) if px[sh][x]]
    step = -1 if d == 2 else 1
    x0 = (xs[0] if d == 2 else xs[-1]) + step                # the first column in front of the body

    def arm(row, rise, thick=2, reach=2):
        """8 px of arm from (row, x0); rise = rows it climbs over its length; then the hand, `reach` rows up and down."""
        for k in range(8):
            y = row - (rise * k) // 7
            x = x0 + step * k
            for t in range(thick):
                if 0 <= x < w:
                    out[y + t][x] = (GOLD if t == 0 else GOLD2) if k == 5 else SKIN
        hy, hx = row - rise + (thick - 1) // 2, x0 + step * 8
        for t in range(-reach, reach + 1):                     # the open hand, fingers up
            for q in (0, 1):
                if 0 <= hx + step * q < w and not (q == 1 and t == reach):
                    out[hy + t][hx + step * q] = SKIN
        if arc:
            for t in range(-reach - 1, reach + 1):
                q = 2 if t in (-reach - 1, reach) else 3
                if 0 <= hx + step * q < w:
                    out[hy + t][hx + step * q] = SPARK

    if kind == 0:
        arm(sh, 0)
    elif kind == 1:
        arm(sh - 1, 4)
    else:
        arm(sh, 0, thick=3, reach=3)
    return xo, yo, w, h, out


def lie(fr, d):
    xo, yo, w, h, px = fr
    nxo, nyo, ow, oh, npx = LY.lie(px, xo, yo, face_left=(d != 3))
    return nxo, nyo, ow, oh, npx


def build(src, template_counts):
    idle = [[src.frame(0, f, d) for f in range(src.count(0))] for d in range(4)]
    walk = [[src.frame(1, f, d) for f in range(src.count(1))] for d in range(4)]
    arms = [[src.frame(12, f, d) for f in range(src.count(12) if src.buu else 2)] for d in range(4)]
    stun = [hurt(idle[d][0], d) for d in range(4)]
    down = [lie(stun[d], d) for d in range(4)]
    hit = [sparks(stun[d], d) for d in range(4)]
    wide = {d: widen(idle[d][0], (32 - idle[d][0][2]) // 2) if idle[d][0][2] < 32 else idle[d][0] for d in (2, 3)}
    strike = {d: [[wide[d], palm(idle[d][0], d, kind, True), palm(idle[d][0], d, kind, False), wide[d]]
                  for kind in range(3)] for d in (2, 3)}

    def plan(gi, d, k):
        if gi == 1:
            return walk[d][k % len(walk[d])]
        if gi in (2, 11):
            return hit[d] if k < 2 else stun[d]      # the marks for the first two frames, as the stock hit-stun
        if gi == 3:
            return stun[d] if k < 2 else down[d]
        if gi == 20:
            return stun[d] if k < 1 else down[d]
        if gi == 12:
            return down[d]
        if gi in (6, 7, 8) and d >= 2:                # the side views' palm strikes: stand, strike, hold, stand
            return strike[d][gi - 6][k % 4]
        if gi in (4, 5, 6, 7, 8):
            seq = (0, 0, 1) if gi == 5 else (0, 1, 1, 0)   # the charge holds the first, the throws the second
            return arms[d][seq[k % len(seq)] % len(arms[d])]
        return idle[d][k % len(idle[d])]

    groups = []
    for gi, count in enumerate(template_counts):
        if not count:
            groups.append(None)
            continue
        groups.append([[plan(gi, d, k) for k in range(count)] for d in range(4)])
    return groups


def header(log, groups):
    l2 = S.Sprites(log)
    rec = l2.record(TEMPLATE)
    hdr = bytearray(log[rec - B:rec - B + 0x4C])
    hdr[2] = len(groups)
    size = max(f[2] * f[3] for g in groups if g for dirs in g for f in dirs)
    struct.pack_into("<H", hdr, 0, max(size, struct.unpack_from("<H", hdr, 0)[0]))
    for k in range(4):
        hdr[0x0C + 16 * k:0x1C + 16 * k] = struct.pack("<iiii", *BOX)
    return hdr


def write_c(path, hdr, groups, source):
    lines = ["/* GENERATED by tools/popoart.py from the ROMs (" + source + "): Mr. Popo, the Time Chamber's hidden room.",
             " * Build output only; never commit. */",
             '#include "rom.h"', "",
             "typedef struct { signed char xo, yo; u8 w, h; u32 attr; const void *blob; } PopoDesc;",
             '#define ART __attribute__((section(".rodata.art")))', ""]
    blobs = {}
    total = 0
    grp = []
    for gi, g in enumerate(groups):
        if not g:
            grp.append("{0, 0, 0, 0}")
            continue
        arrs = []
        for d, frames in enumerate(g):
            entries = []
            for xo, yo, w, h, px in frames:
                key = (w, h, tuple(tuple(r) for r in px))
                if key not in blobs:
                    raw = tiles(px, w, h)
                    data = compress(raw)
                    assert m.decompress(data, 8, 1)[:w * h] == raw
                    name = f"popo_blob_{len(blobs)}"
                    blobs[key] = name
                    total += len(data)
                    lines.append(f"static const u8 {name}[{len(data)}] ART __attribute__((aligned(4))) = "
                                 f"{{{', '.join(f'0x{b:02x}' for b in data)}}};")
                entries.append(f"{{{xo}, {yo}, {w}, {h}, 0x{attr_of(w, h):08x}u, {blobs[key]}}}")
            an = f"popo_g{gi}_d{d}"
            lines.append(f"static const PopoDesc {an}[{len(frames)}] ART = {{{', '.join(entries)}}};")
            arrs.append(an)
        grp.append("{" + ", ".join(arrs) + "}")
    lines.append(f"typedef struct {{ u8 header[0x4C]; const PopoDesc *groups[{len(groups)}][4]; u32 end; }} PopoRecord;")
    lines.append(f"const PopoRecord popo_record ART = {{{{{', '.join(f'0x{b:02x}' for b in hdr)}}}, {{{', '.join(grp)}}}, 0}};")
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    open(path, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    return len(blobs), total


def write_preview(path, pal, groups, scale=3):
    """Every group x 4 facings x every frame on a common ground line, the hurt box in red on group 0."""
    rows = [(gi, g) for gi, g in enumerate(groups) if g]
    cw, ch = 40, 40
    ox, oy = cw // 2 - 8, ch - 8
    ncol = max(sum(len(dirs) + 1 for dirs in g) for _, g in rows)
    cv = S.Canvas(ncol * cw + 24, len(rows) * ch, (60, 60, 90))
    for r, (gi, g) in enumerate(rows):
        cv.text(2, r * ch + 2, str(gi), (255, 255, 0))
        c = 0
        for d, frames in enumerate(g):
            for k, (xo, yo, w, h, px) in enumerate(frames):
                bx, by = 24 + c * cw + ox, r * ch + oy
                for y in range(h):
                    for x in range(w):
                        if px[y][x]:
                            cv.put(bx + xo + x, by + yo + y, pal[px[y][x]])
                if gi == 0 and k == 0:
                    x1, y1, x2, y2 = BOX
                    for x in range(x1, x2 + 1):
                        cv.put(bx + x, by + y1, (255, 0, 0))
                        cv.put(bx + x, by + y2, (255, 0, 0))
                    for y in range(y1, y2 + 1):
                        cv.put(bx + x1, by + y, (255, 0, 0))
                        cv.put(bx + x2, by + y, (255, 0, 0))
                c += 1
            c += 1
    cv.save(path)
    try:
        from PIL import Image
        im = Image.open(path)
        im.resize((im.width * scale, im.height * scale), Image.NEAREST).save(path.replace(".png", f"_x{scale}.png"))
    except ImportError:
        pass


def main():
    rom_path, out_c = sys.argv[1], sys.argv[2]
    buu_path = sys.argv[sys.argv.index("--buu") + 1] if "--buu" in sys.argv else "none"
    pdir = sys.argv[sys.argv.index("--preview-dir") + 1] if "--preview-dir" in sys.argv else None
    log = open(rom_path, "rb").read()
    buu = open(buu_path, "rb").read() if buu_path != "none" and os.path.exists(buu_path) else None
    l2 = S.Sprites(log)
    trec = l2.record(TEMPLATE)
    tg = l2.groups(trec)
    tc = l2.frame_counts(tg)
    template_counts = [tc[g[0]] if g[0] else 0 for g in tg]
    src = Source(log, buu)
    groups = build(src, template_counts)
    hdr = header(log, groups)
    source = f"Buu's Fury record {BUU_POPO}" if buu is not None else f"LoG2 sprite {LOG2_POPO}, no roms/buu.gba"
    nb, total = write_c(out_c, hdr, groups, source)
    if pdir:
        os.makedirs(pdir, exist_ok=True)
        write_preview(os.path.join(pdir, "popo_preview.png"), l2.pal, groups)
    print(f"wrote {out_c}: Mr. Popo from {source}, {len(groups)} groups, {nb} frame blobs, {total} bytes compressed, "
          f"largest frame {struct.unpack_from('<H', hdr, 0)[0]:#x}")


if __name__ == "__main__":
    main()
