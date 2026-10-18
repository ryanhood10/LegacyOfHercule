"""New boss models built from stock sprite records at build time: Android 13 (base form) and Cooler's
Namekian servant. Emits C source (records + LZ-compressed frames) compiled into the co-op ROM; nothing
from the ROM is committed. See docs/sprites.md "New enemy models".

usage: bossmodels.py roms/log2.gba build/gen/boss_models_data.c [--preview-dir build/x]

Android 13 (record a13_record, sprite id 165 by default): Android 16's broad body (sprite 8: idle,
walk, hit-stun, punch, knock-down) plus Android 17's fight frames (sprite 9: knock-back/fall, charge,
two attacks) re-headed with 16's head, the same composition as tools/a16sprites.py, then dressed as 13:
  * the hair (16's orange mohawk) becomes a yellow trucker cap with a black band and a red Red Ribbon
    dot above the forehead, white hair below it;
  * skin -> pale android skin, hands -> brown gloves;
  * the armour / vest -> a dark-yellow sleeveless vest on the torso, bare pale arms beside it, blue
    jeans below the waist (17's trousers are mapped onto the same jeans ramp), dark boots.
Body parts are told apart per frame from geometry: the "up" axis runs from the body's centroid to the
hair's centroid (so lying frames work too), the face is the skin touching the hair, the torso is a band
around the face's line along that axis, the waist sits at 70 % of the body's length from the head.
The record keeps 17's header and group layout (13 groups), so the stock boss AI sets (19's set 29, the
melee set 21) find every group they play, and group 3 (stand, stagger, fall, lie) serves the co-op
both-down rule.

Namekian servant (record namek_record, sprite id 166): Piccolo's capeless complete set (sprite 104, all
23 groups) recoloured: darker teal-green skin, a maroon warrior gi with a gold sash, black boots, the
arm patches pink rather than red so they stand out from the gi. Recoloured frames only, so 104 itself is untouched.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402
import a16sprites as A  # noqa: E402
from buu2log2 import compress  # noqa: E402

HFLIP = A.HFLIP
BLACK = 1

# ---- palette helpers --------------------------------------------------------------------------------
PAL = None


def near(rgb, avoid=(0,)):
    """Index of the shared OBJ palette colour nearest to rgb."""
    best, bi = None, 0
    for i, c in enumerate(PAL):
        if i in avoid:
            continue
        d = sum((a - b) ** 2 for a, b in zip(c, rgb))
        if best is None or d < best:
            best, bi = d, i
    return bi


# ---- Android 13 -------------------------------------------------------------------------------------
A16_SPRITE, A17_SPRITE = 8, 9
GROUPS_FROM_16 = (0, 1, 2, 6, 12)
GROUPS_FROM_17 = (3, 5, 7, 8)
GROUP_COUNT_13 = 13

HAIR = {32, 33, 34, 35}                 # 16's orange mohawk
SKIN = {114, 115, 116, 117, 23}         # 16's skin ramp (+ the brown shade / outline)
ARMOUR = {41, 42, 46, 54}               # 16's armour (17's vest and scarf are mapped onto it first)
TROUSERS = {78, 79, 80, 214, 215}       # 17's blue trousers
# Android 17 -> 16-space before the head swap: vest and scarf -> armour, skin -> 16's skin, shoes ->
# boots; the trousers stay (they become the jeans directly)
PRE17 = {
    15: 42, 14: 42, 13: 41, 12: 41, 11: 54, 10: 46, 9: 46,
    35: 42, 34: 41, 33: 46, 32: 46,
    23: 23, 22: 117, 21: 117, 20: 116, 19: 115, 18: 115, 17: 114, 16: 114,
    58: 15, 57: 13, 50: 15, 203: 42,
}


def a13_colours():
    c = {}
    # brightness rank within each source ramp: 0 = lightest
    c["skin"] = [near(x) for x in ((255, 228, 214), (255, 189, 172), (230, 156, 131), (205, 131, 98), (150, 90, 60))]
    c["hair"] = [near(x) for x in ((255, 255, 255), (222, 222, 222), (197, 197, 197), (148, 148, 148))]
    c["cap"] = [near(x) for x in ((255, 222, 66), (230, 189, 65), (197, 148, 32), (150, 100, 16))]
    c["vest"] = [near(x) for x in ((246, 205, 41), (213, 164, 16), (172, 123, 8), (115, 74, 0))]
    c["jeans"] = [near(x) for x in ((98, 130, 213), (65, 98, 189), (41, 65, 148), (24, 41, 106), (16, 24, 65))]
    c["glove"] = [near(x) for x in ((156, 74, 32), (131, 65, 24), (106, 49, 16), (82, 41, 8))]
    c["red"] = 5
    return c


RANK_HAIR = {32: 0, 33: 1, 34: 2, 35: 3}
RANK_SKIN = {114: 0, 115: 1, 116: 2, 117: 3, 23: 4}
RANK_ARM = {46: 0, 54: 1, 41: 2, 42: 3}
RANK_TROUSER = {214: 0, 78: 1, 79: 2, 80: 3, 215: 4}


def neighbours8(y, x):
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            if dy or dx:
                yield y + dy, x + dx


def dress13(px, w, h, col, facing):
    """Recolour one frame (16-space colours) into Android 13; facing 0 down, 1 up, 2 left, 3 right
    (of the stored, unflipped pixels). Returns new pixels."""
    pts = lambda cls: [(y, x) for y in range(h) for x in range(w) if px[y][x] in cls]  # noqa: E731
    hair, skin, arm, trou = pts(HAIR), pts(SKIN), pts(ARMOUR), pts(TROUSERS)
    opaque = [(y, x) for y in range(h) for x in range(w) if px[y][x]]
    out = [row[:] for row in px]
    if not opaque:
        return out

    def centroid(p):
        return (sum(y for y, _ in p) / len(p), sum(x for _, x in p) / len(p))

    body = arm + trou or opaque
    by, bx = centroid(body)
    if hair:
        hy, hx = centroid(hair)
        uy, ux = hy - by, hx - bx
        n = math.hypot(uy, ux)
        uy, ux = (uy / n, ux / n) if n > 0.5 else (-1.0, 0.0)
    else:
        uy, ux = -1.0, 0.0

    def proj(p):
        return (p[0] - by) * uy + (p[1] - bx) * ux

    def lat(p):
        return (p[1] - bx) * uy - (p[0] - by) * ux

    hairset = set(hair)
    # the face: skin components touching the hair
    skinset = set(skin)
    face, seen = set(), set()
    for p in skin:
        if p in seen:
            continue
        comp, stack = [], [p]
        seen.add(p)
        while stack:
            q = stack.pop()
            comp.append(q)
            for r in neighbours8(*q):
                if r in skinset and r not in seen:
                    seen.add(r)
                    stack.append(r)
        if any(r in hairset for q in comp for r in neighbours8(*q)):
            face.update(comp)
    top = max(proj(p) for p in opaque)
    bottom = min(proj(p) for p in opaque)
    waist = top - 0.70 * (top - bottom)
    ref = list(face) or hair or opaque
    face_lat = sum(lat(p) for p in ref) / len(ref)
    forehead = max((proj(p) for p in face), default=top - 4)
    face_mid = (sum(p[1] for p in face) / len(face)) if face else None

    # cap: hair at or above the forehead line, at most CAP_H deep; anything higher is cut off
    CAP_H = 3.6
    cap = {p for p in hair if proj(p) >= forehead - 0.5}
    cut = {p for p in cap if proj(p) > forehead + CAP_H}
    cap -= cut
    for y, x in cut:
        out[y][x] = 0
    # 16's fringe hangs over his forehead; 13's face is bare under the cap. In views that show more face
    # than hair, hair inside the face's lateral span becomes forehead skin; the rest is white hair.
    below = [p for p in hair if p not in cap and p not in cut]
    span = None
    if face and facing != 1 and len(face) > len(below) // 2:
        lats = [lat(p) for p in face]
        lo, hi = min(lats), max(lats)
        if facing == 0:                      # front: two pixels of white hair at each side
            span = (lo + 2.0, hi - 2.0)
        else:                                # side: white hair behind the face only
            fl = sum(p[1] for p in face) / len(face)
            hl = sum(p[1] for p in below) / len(below) if below else fl
            span = (lo, hi - 2.0) if (hl - fl) * uy > 0 else (lo + 2.0, hi)
    for p in below:
        r = RANK_HAIR[px[p[0]][p[1]]]
        if span and span[0] <= lat(p) <= span[1]:
            out[p[0]][p[1]] = col["skin"][min(4, r + 1)]
            face.add(p)
        else:
            out[p[0]][p[1]] = col["hair"][r]
    for p in cap:
        y, x = p
        d = proj(p) - forehead
        exposed = any((0 <= r[0] < h and 0 <= r[1] < w and (out[r[0]][r[1]] == 0 or r in cut))
                      and (r[0] - by) * uy + (r[1] - bx) * ux > proj(p) for r in neighbours8(y, x)) \
            or not (0 < y < h - 1 and 0 < x < w - 1)
        if d < 0.5:
            out[y][x] = BLACK                     # the band / brim line over the forehead
        elif exposed:
            out[y][x] = BLACK                     # crown outline
        else:
            out[y][x] = col["cap"][min(3, RANK_HAIR[px[y][x]] + (1 if d < 1.5 else 0))]
    # the Red Ribbon dot: front views (the face is centred under the cap), just above the band
    if facing != 1 and face and cap and face_mid is not None:
        cy = [p for p in cap if 0.5 <= proj(p) - forehead < 1.5 and out[p[0]][p[1]] != BLACK]
        cxs = [p[1] for p in cap]
        if cy and abs(face_mid - (min(cxs) + max(cxs)) / 2) < 1.5 and len(face) >= 12:
            p = min(cy, key=lambda q: abs(q[1] - face_mid))
            out[p[0]][p[1]] = col["red"]
    # brim: in side views the cap peak juts out over the face side by two pixels
    if face and cap and face_mid is not None:
        cxs = [p[1] for p in cap]
        mid = (min(cxs) + max(cxs)) / 2
        side = face_mid - mid
        if abs(side) >= 1.5 and abs(uy) > 0.8:
            band = [p for p in cap if out[p[0]][p[1]] == BLACK and proj(p) - forehead < 0.5]
            if band:
                y = band[0][0]
                edge = max(p[1] for p in band) if side > 0 else min(p[1] for p in band)
                step = 1 if side > 0 else -1
                for k in (1, 2):
                    x = edge + step * k
                    if 0 <= x < w and out[y][x] == 0:
                        out[y][x] = BLACK
    for y, x in skin:
        r = RANK_SKIN[px[y][x]]
        out[y][x] = col["skin"][r] if (y, x) in face else col["glove"][min(3, r)]
    # front: white hair at both sides of the face for five rows under the cap; back: the whole head
    # below the cap is hair except the neck (the two lowest rows)
    if facing == 0 and face and abs(uy) > 0.8:
        for y in sorted({p[0] for p in face}):
            d = (y - by) * uy - forehead
            if -5.5 <= d <= -0.5:
                xs = sorted(x for (fy, x) in face if fy == y)
                for x in xs[:2] + xs[-2:]:
                    r = RANK_SKIN.get(px[y][x], RANK_HAIR.get(px[y][x], 1))
                    out[y][x] = col["hair"][min(3, r)]
    if facing == 1 and face:
        neck = min((p[0] - by) * uy + (p[1] - bx) * ux for p in face) + 1.5
        for p in face:
            if proj(p) > neck:
                r = RANK_SKIN.get(px[p[0]][p[1]], RANK_HAIR.get(px[p[0]][p[1]], 1))
                out[p[0]][p[1]] = col["hair"][min(3, r)]
    for p in arm:
        y, x = p
        r = RANK_ARM[px[y][x]]
        if proj(p) < waist:
            out[y][x] = col["jeans"][r + 1]
        elif abs(lat(p) - face_lat) <= 3.6:
            out[y][x] = col["vest"][r]
        else:
            out[y][x] = col["skin"][min(4, r + 1)]
    for y, x in trou:
        out[y][x] = col["jeans"][RANK_TROUSER[px[y][x]]]
    # outline the head (16's head has no outline of its own): transparent pixels next to it -> black
    head = cap | set(face) | set(below)
    for y, x in list(head):
        for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
            if 0 <= ny < h and 0 <= nx < w and out[ny][nx] == 0 and px[ny][nx] == 0:
                out[ny][nx] = BLACK
    return out


def build_a13(sp):
    rom = sp.rom
    rec16, rec17 = sp.record(A16_SPRITE), sp.record(A17_SPRITE)
    g16, g17 = sp.groups(rec16), sp.groups(rec17)
    c16, c17 = sp.frame_counts(g16), sp.frame_counts(g17)
    header = bytearray(rom[rec17 - S.ROM_BASE:rec17 - S.ROM_BASE + 0x4C])
    header[2] = GROUP_COUNT_13
    col = a13_colours()

    heads, chins = [], []
    for d in range(4):
        xo, yo, w, h, attr, _ = sp.descriptor(g16[0][d], 0)
        _, _, px = sp.frame_pixels(g16[0][d], 0)
        if attr & HFLIP:
            px = A.hflip(px)
        heads.append(A.Head(px, w, h))
        xo, yo, w, h, attr, _ = sp.descriptor(g17[0][d], 0)
        _, _, px = sp.frame_pixels(g17[0][d], 0)
        hair, top = A.head_metrics17(px, w, h)
        chin = max(y for y in range(top, top + 16) if any(v in A.SKIN17 for v in px[y][4:w - 4]) or
                   any((y, x) in set(hair) for x in range(w))) - top
        chins.append(min(chin, 13))

    saved = A.RECOLOUR
    A.RECOLOUR = PRE17                    # swap_head recolours 17's body with this table
    try:
        groups = []
        done = {}
        for gi in range(GROUP_COUNT_13):
            src = g16 if gi in GROUPS_FROM_16 else g17 if gi in GROUPS_FROM_17 else None
            if src is None or not any(src[gi]):
                groups.append(None)
                continue
            counts = c16 if src is g16 else c17
            dirs = []
            for d in range(4):
                arr = src[gi][d]
                frames = []
                for fi in range(counts[arr]):
                    xo, yo, w, h, attr, blob = sp.descriptor(arr, fi)
                    if blob not in done:
                        _, _, px = sp.frame_pixels(arr, fi)
                        if src is g17:
                            flip = bool(attr & HFLIP)
                            work = A.hflip(px) if flip else px
                            face = 2 if d == 3 else d
                            work, _ = A.swap_head(work, w, h, heads[face], chins[face])
                            px = A.hflip(work) if flip else work
                        done[blob] = dress13(px, w, h, col, d if not attr & HFLIP else 2)
                    frames.append((xo, yo, w, h, attr, done[blob]))
                dirs.append(frames)
            groups.append(dirs)
    finally:
        A.RECOLOUR = saved
    return header, groups, len(done)


# ---- Namekian servant -------------------------------------------------------------------------------
PICCOLO_CAPELESS = 104


def namek_colours():
    m = {}
    skin = [near(x) for x in ((90, 189, 140), (49, 148, 106), (16, 98, 74))]
    m.update({48: skin[0], 49: skin[1], 50: skin[2]})
    gi = [near(x) for x in ((172, 41, 49), (123, 16, 32), (57, 8, 16))]
    m.update({88: gi[0], 89: gi[1], 211: gi[2]})
    sash = [near(x) for x in ((255, 213, 74), (222, 164, 32), (156, 106, 16))]
    m.update({77: sash[0], 195: sash[1], 72: sash[2]})
    m.update({26: near((230, 98, 123)), 247: near((255, 172, 189))})   # arm patches pink (not red)
    boots = [near(x) for x in ((74, 74, 74), (41, 41, 41))]
    m.update({34: boots[0], 35: boots[1]})
    return m


def build_namek(sp):
    rom = sp.rom
    rec = sp.record(PICCOLO_CAPELESS)
    g = sp.groups(rec)
    counts = sp.frame_counts(g)
    header = bytearray(rom[rec - S.ROM_BASE:rec - S.ROM_BASE + 0x4C])
    remap = namek_colours()
    groups, done = [], {}
    for gi, grp in enumerate(g):
        if not any(grp):
            groups.append(None)
            continue
        dirs = []
        for d in range(4):
            arr = grp[d]
            frames = []
            if arr:
                for fi in range(counts[arr]):
                    xo, yo, w, h, attr, blob = sp.descriptor(arr, fi)
                    if blob not in done:
                        _, _, px = sp.frame_pixels(arr, fi)
                        done[blob] = [[remap.get(v, v) for v in row] for row in px]
                    frames.append((xo, yo, w, h, attr, done[blob]))
            dirs.append(frames)
        groups.append(dirs)
    header[2] = len(groups)
    return header, groups, len(done)


# ---- output -----------------------------------------------------------------------------------------
class CWriter:
    def __init__(self):
        self.lines = ["/* generated by tools/bossmodels.py from the ROM: new boss sprite records (Android 13,",
                      " * Cooler's Namekian servant). Build output only; never commit. */",
                      '#include "rom.h"', "",
                      "typedef struct { signed char xo, yo; u8 w, h; u32 attr; const void *blob; } BmDesc;", ""]
        self.blobs = {}
        self.bytes = 0

    def blob(self, prefix, px, w, h):
        key = (w, h, tuple(tuple(r) for r in px))
        if key not in self.blobs:
            name = f"{prefix}_blob_{len(self.blobs)}"
            data = compress(A.tiles(px, w, h))
            self.bytes += len(data)
            self.blobs[key] = name
            body = ", ".join(f"0x{b:02x}" for b in data)
            self.lines.append(f"static const u8 {name}[{len(data)}] __attribute__((aligned(4))) = {{{body}}};")
        return self.blobs[key]

    def record(self, prefix, header, groups):
        names = []
        for gi, g in enumerate(groups):
            if not g:
                names.append("{0, 0, 0, 0}")
                continue
            arrs = []
            for d, frames in enumerate(g):
                if not frames:
                    arrs.append("0")
                    continue
                entries = [f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {self.blob(prefix, px, w, h)}}}"
                           for xo, yo, w, h, attr, px in frames]
                name = f"{prefix}_g{gi}_d{d}"
                self.lines.append(f"static const BmDesc {name}[{len(frames)}] = {{{', '.join(entries)}}};")
                arrs.append(name)
            names.append("{" + ", ".join(arrs) + "}")
        hdr = ", ".join(f"0x{b:02x}" for b in header)
        t = f"{prefix}_rec_t"
        self.lines.append(f"typedef struct {{ u8 header[0x4C]; const BmDesc *groups[{len(groups)}][4]; u32 end; }} {t};")
        self.lines.append(f"const {t} {prefix}_record = {{{{{hdr}}}, {{{', '.join(names)}}}, 0}};")

    def save(self, path):
        os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
        open(path, "w", encoding="utf-8", newline="\n").write("\n".join(self.lines) + "\n")


def preview(path, sp, groups, ref_groups=None):
    """Every group x 4 facings (all frames), 3x; with ref_groups, the stock frames on the left."""
    cell = 34
    rows = []
    for gi, g in enumerate(groups):
        if not g:
            continue
        row = []
        for frames in g:
            for xo, yo, w, h, attr, px in frames:
                row.append(A.hflip(px) if attr & HFLIP else px)
            row.append(None)
        rows.append((gi, row))
    width = max(len(r) for _, r in rows) * cell + 16
    cv = S.Canvas(width, len(rows) * cell, (60, 60, 90))
    for ri, (gi, row) in enumerate(rows):
        cv.text(1, ri * cell + 2, str(gi), (255, 255, 0))
        for fi, px in enumerate(row):
            if not px:
                continue
            for y, line in enumerate(px):
                for x, v in enumerate(line):
                    if v:
                        cv.put(16 + fi * cell + 1 + x, ri * cell + 1 + y, sp.pal[v])
    cv.save(path)
    try:
        from PIL import Image
        im = Image.open(path)
        im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(path.replace(".png", "_x3.png"))
    except ImportError:
        pass


def main():
    global PAL
    rom = open(sys.argv[1], "rb").read()
    out_c = sys.argv[2]
    pdir = sys.argv[sys.argv.index("--preview-dir") + 1] if "--preview-dir" in sys.argv else None
    sp = S.Sprites(rom)
    PAL = sp.pal
    w = CWriter()
    h13, g13, n13 = build_a13(sp)
    w.record("a13", h13, g13)
    hn, gn, nn = build_namek(sp)
    w.record("namek", hn, gn)
    w.save(out_c)
    if pdir:
        os.makedirs(pdir, exist_ok=True)
        preview(os.path.join(pdir, "a13_preview.png"), sp, g13)
        preview(os.path.join(pdir, "namek_preview.png"), sp, gn)
    print(f"wrote {out_c}: Android 13 {n13} frames, Namekian {nn} frames, {w.bytes} bytes compressed")


if __name__ == "__main__":
    main()
