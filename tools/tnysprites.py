"""The tournament's five fighters built from existing bodies (docs/tournament.md, the art-new piece): King Cold,
Master Roshi, Chi-Chi, Videl and Chiaotzu, each ONE sprite record in the playable 21-group player layout (Hercule's
sprite 72 pattern of filled groups and frame counts) whose groups 0, 1, 2 and 5-8 also satisfy the melee boss AI
set 21 (idle, walk, hit-stun, charge, three attacks), so the same record is an opponent and a player body. Plus
Videl's dialogue portrait (LoG2 has none). Everything is derived from the ROMs at build time into build/gen and
marked TNY_ART (the second ROM blob at 0x08E00000); nothing from a ROM is committed.

usage: tnysprites.py roms/log2.gba build/gen/tny_sprites_data.c --buu roms/buu.gba|none [--preview-dir DIR]
           [--ids 185,186,187,188,189] [--only cold,roshi,...]

Sources (docs/sprites.md "Tournament fighters (art-new)"; ids in src/coop/tourney.h TS_*):
  King Cold  (185) Cooler's 64x64 fight set (LoG2 34) recoloured to Cold's colours (the palette of his cutscene
                   sprite 74: purple skin, white-mint plates, teal dome), Cold's head (horns, dome, face) cut from
                   74 and pasted over Cooler's spiked head (anchored on Cooler's forehead gem), a red cape drawn
                   behind the body; Cooler's attack frames are kept.
  Master Roshi (186) Krillin's body: his NPC set 76 (idle, walk, knock-down, shout, ki pose, punch combo, lying)
                   plus Goku's (60) attacks, hurt, flight, special melee and Kamehameha pose (the krillinsprites.py
                   composition) in the turtle gi, every upright frame re-headed with Roshi's head and beard from his
                   NPC sprite 88 (bald, red sunglasses, white beard), his turtle shell drawn on the back views.
  Chi-Chi    (187) Android 18's fight set (LoG2 11) re-headed with Chi-Chi's head (bun, black hair) from her NPC
                   sprite 33 and recoloured to her LoG2 outfit (orange tabard, white sleeves, dark shoes).
  Videl      (188) Android 18's fight set re-headed with Videl's head from Buu's Fury (record 276, her short hair
                   at the 25th Budokai) and recoloured (white shirt, black shorts / leggings, white shoes).
  Chiaotzu   (189) kid Gohan's complete fight set (LoG2 53, the smallest playable body) re-headed with Chiaotzu's
                   head from his NPC sprite 32 (white face, red cheeks, cap with the red ball) and recoloured to
                   his outfit there (light blue shirt, purple collar, red sash, black trousers).
Frames that show no upright head (lying, tumbling) are recoloured only (18's blonde hair black, kid Gohan's hair
Chiaotzu's cap colour...). Without roms/buu.gba Videl's head is Chi-Chi's without the bun (black bob).
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402
import mapchunks as m  # noqa: E402
from buu2log2 import compress, nearest_table  # noqa: E402

HF = 0x10000000
TEMPLATE = 72                       # Hercule: the player layout (which groups, frames per group)
EXTRA_COUNTS = {16: 7, 18: 7}       # special melee and the beam pose, beyond Hercule's (Goku's counts)
B = 0x08000000
PORTRAIT_TABLE = 0x083EC9D4
BUU_PORTRAIT_MODULE = 0x086F393C
BUU_VIDEL_PORTRAIT = 153            # short-haired Videl (152 has the pigtails)
BUU_VIDEL_RECORD = 276
BLACK = 1
SKIN = {114, 115, 116, 117, 28}     # LoG2 skin ramp (+ 18's cheek pink)

FIGHTERS = ("cold", "roshi", "chichi", "videl", "chiaotzu")
SYMBOL = {f: f"tny_{f}_record" for f in FIGHTERS}
# the test enemy record after each sprite record: (sprite id, stat). Stat 29 is Cooler's (the 64x64 giant on the melee
# boss AI set 21), 4 is Android 18's (a normal body on set 21); docs/testing.md "Tournament fighters (art-new)"
TEST_RECORD = {"tny_cold_record": (185, 29), "tny_roshi_record": (186, 4), "tny_chichi_record": (187, 4),
               "tny_videl_record": (188, 4), "tny_chiaotzu_record": (189, 4)}


# ---- pixels ---------------------------------------------------------------------------------------------------
def hflip(px):
    return [row[::-1] for row in px]


def copy(px):
    return [row[:] for row in px]


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


N4 = ((-1, 0), (1, 0), (0, -1), (0, 1))
N8 = N4 + ((-1, -1), (-1, 1), (1, -1), (1, 1))


def flood(px, seeds, ok, nbr=N4):
    """Pixels reachable from `seeds` through pixels where ok(y, x, v)."""
    h, w = len(px), len(px[0])
    out, stack = set(), [s for s in seeds]
    while stack:
        y, x = stack.pop()
        if (y, x) in out or not (0 <= y < h and 0 <= x < w) or not ok(y, x, px[y][x]):
            continue
        out.add((y, x))
        stack += [(y + dy, x + dx) for dy, dx in nbr]
    return out


def components(px, ok, nbr=N4):
    h, w = len(px), len(px[0])
    seen, out = set(), []
    for y in range(h):
        for x in range(w):
            if (y, x) not in seen and ok(y, x, px[y][x]):
                c = flood(px, [(y, x)], ok, nbr)
                seen |= c
                out.append(c)
    return out


def near(px, y, x, colours, nbr=N8):
    h, w = len(px), len(px[0])
    return any(0 <= y + dy < h and 0 <= x + dx < w and px[y + dy][x + dx] in colours for dy, dx in nbr)


def recolour(px, table):
    return [[table.get(v, v) for v in row] for row in px]


def top_row(px):
    return next((y for y, row in enumerate(px) if any(row)), 0)


# ---- heads ------------------------------------------------------------------------------------------------------
class Head:
    """A donor head: pixels relative to the anchor (chin row, face centre x)."""

    def __init__(self, pts, chin, cx):
        self.pts = {(int(y - chin), int(round(x - cx))): v for (y, x), v in pts.items()}

    def paste(self, px, chin, cx, under=False):
        h, w = len(px), len(px[0])
        cx, chin = int(round(cx)), int(chin)
        for (dy, dx), v in self.pts.items():
            y, x = chin + dy, cx + dx
            if 0 <= y < h and 0 <= x < w and (not under or not px[y][x]):
                px[y][x] = v

    def flipped(self):
        out = Head({}, 0, 0)
        out.pts = {(dy, -dx): v for (dy, dx), v in self.pts.items()}
        return out


def cut_head(px, rows, keep, cx_colours, extra=None):
    """Pixels of `px` in rows [r0, r1] whose colour passes keep(); anchor = (r1, mean x of cx_colours there)."""
    r0, r1 = rows
    pts = {(y, x): px[y][x] for y in range(r0, r1 + 1) for x in range(len(px[0])) if px[y][x] and keep(y, x, px[y][x])}
    if extra:
        pts.update(extra)
    xs = [x for (y, x), v in pts.items() if v in cx_colours and y <= r1]
    return Head(pts, r1, sum(xs) / len(xs))


class Source:
    def __init__(self, rom):
        self.rom = rom
        self.sp = S.Sprites(rom)
        self._g = {}

    def groups(self, sid):
        if sid not in self._g:
            g = self.sp.groups(self.sp.record(sid))
            self._g[sid] = (g, self.sp.frame_counts(g))
        return self._g[sid]

    def frame(self, sid, gi, d, fi):
        """(xo, yo, w, h, attr, blob, stored px) of frame fi of group gi facing d (fi cycles)."""
        g, c = self.groups(sid)
        arr = g[gi][d]
        fi %= c[arr]
        xo, yo, w, h, attr, blob = self.sp.descriptor(arr, fi)
        _, _, px = self.sp.frame_pixels(arr, fi)
        return xo, yo, w, h, attr, blob, px

    def count(self, sid, gi, d):
        g, c = self.groups(sid)
        return c[g[gi][d]] if gi < len(g) and g[gi][d] else 0

    def canon(self, sid, gi, d, fi):
        """The frame as drawn, in canonical orientation: right-facing frames mirrored to face left."""
        xo, yo, w, h, attr, blob, px = self.frame(sid, gi, d, fi)
        disp = hflip(px) if attr & HF else px
        return hflip(disp) if d == 3 else disp


def to_stored(canon, attr, d):
    disp = hflip(canon) if d == 3 else canon
    return hflip(disp) if attr & HF else disp


# ---- the fighters -----------------------------------------------------------------------------------------------
class Fighter:
    """Group plan: gi -> (sprite, source group); a transform(sid, gi, px, f) per frame (f = 0 down, 1 up, 2 side)."""
    header_sprite = 72
    plan = {}

    def __init__(self, src):
        self.src = src
        self.stats = {"frames": 0, "heads": 0}

    def transform(self, sid, gi, px, f):
        return px


# Android 18 (11) as a base for Chi-Chi and Videl ------------------------------------------------------------------
A18 = 11
A18_HAIR = {120, 45, 37, 138, 41, 118}    # gold hair (group 4: a dimmer ramp)
A18_SKIN4 = {16: 114, 17: 114, 18: 115, 19: 116}   # group 4's skin ramp onto the usual one
A18_PLAN = {0: (A18, 0), 1: (A18, 1), 2: (A18, 2), 3: (A18, 12), 4: (A18, 4), 5: (A18, 5), 6: (A18, 6),
            7: (A18, 7), 8: (A18, 8), 11: (A18, 3), 12: (A18, 12), 13: (A18, 1), 16: (A18, 7), 17: (A18, 0),
            18: (A18, 5), 20: (A18, 12)}


class A18Base(Fighter):
    plan = A18_PLAN
    body = {}          # body colour table (applied to everything that is not hair)
    hair = {120: BLACK, 37: BLACK, 138: BLACK, 41: BLACK, 118: BLACK, 45: 15, 2: 14, 23: BLACK}

    def __init__(self, src):
        super().__init__(src)
        self.chin_off = []
        for f in range(3):
            px = src.canon(A18, 0, f, 0)
            top = top_row(px)
            neck = next(y for y in range(top + 6, len(px)) if sum(1 for v in px[y] if v in (79, 80, 81)) >= 2)
            self.chin_off.append(neck - 1 - top)
        self.heads = [self.donor(f) for f in range(3)]

    def hair_mask(self, px):
        core = {(y, x) for y, row in enumerate(px) for x, v in enumerate(row) if v in A18_HAIR}
        mask = set(core)
        for y, row in enumerate(px):
            for x, v in enumerate(row):
                if v in (2, 23) and any((y + dy, x + dx) in core for dy, dx in N8):
                    if v == 2 and near(px, y, x, SKIN | {65, 203}):
                        continue
                    mask.add((y, x))
        return core, mask

    def transform(self, sid, gi, px, f):
        core, mask = self.hair_mask(px)
        px = recolour(px, A18_SKIN4)
        out = [[(self.hair.get(v, v) if (y, x) in mask else self.body.get(v, v)) for x, v in enumerate(row)]
               for y, row in enumerate(px)]
        if not core:
            return out
        h, w = len(px), len(px[0])
        ys = [y for y, _ in core]
        top = min(ys)
        # upright: the hair sits on top of the body (its centroid in the top part of the opaque area)
        body = [(y, x) for y in range(h) for x in range(w) if px[y][x] and (y, x) not in mask]
        if not body or sum(ys) / len(ys) > sum(y for y, _ in body) / len(body) - 4 or gi in (3, 12, 20):
            return out
        chin = min(h - 1, top + self.chin_off[f])
        band = [(y, x) for y in range(top, chin + 1) for x in range(w)]
        face = [x for y, x in band if px[y][x] in SKIN]
        xs = [x for _, x in core]
        cx = (sum(face) / len(face)) if len(face) >= 5 else (sum(xs) / len(xs))
        for y, x in mask:
            out[y][x] = 0
        for y, x in band:
            if abs(x - cx) <= 6 and px[y][x] in SKIN | {65, 203, 2, 1, 10, 9, 20, 23}:
                out[y][x] = 0
        self.heads[f].paste(out, chin, cx)
        self.stats["heads"] += 1
        return out


class ChiChi(A18Base):
    # 18's blue vest -> Chi-Chi's orange tabard, the navy shading -> her blue trousers, brown boots -> dark shoes
    body = {79: 37, 80: 39, 81: 33, 215: 73, 33: 14, 34: 15, 35: BLACK, 65: BLACK, 203: BLACK, 24: 39, 26: 39}

    def donor(self, f):
        px = self.src.canon(33, 0, f, 0)
        # head rows 0..14 (bun, hair, face; row 15 is the neck line), without the shoulders' greys
        keep = lambda y, x, v: v not in (9, 10, 11, 12, 13, 14, 15) or y < 12  # noqa: E731
        return cut_head(px, (0, 14), keep, {114, 115, 116} if f != 1 else {1})


class Videl(A18Base):
    # 18's vest -> a white shirt (her sleeves stay the white-grey stripes, plainer), boots -> white shoes
    body = {79: 2, 80: 9, 81: 11, 215: 13, 12: 10, 13: 11, 33: 10, 34: 12, 35: 14, 24: 9, 26: 9}
    buu = None

    def donor(self, f):
        if self.buu is None:       # no Buu's Fury ROM: Chi-Chi's head without the bun
            px = self.src.canon(33, 0, f, 0)
            keep = lambda y, x, v: y >= 4 and v not in (249, 253) and (v not in (9, 10, 11, 12, 13, 14, 15) or y < 12)  # noqa: E731
            return cut_head(px, (4, 14), keep, {114, 115, 116} if f != 1 else {1})
        bu, table = self.buu
        rec = bu.record(BUU_VIDEL_RECORD)
        fr = bu.anim(rec["anims"][0])
        (x0, y0, w, h), bpx = bu.frame_pixels(fr[0][f], rec["remap"])
        skin = {17: 114, 18: 115, 20: 116}     # Buu's tan face onto LoG2's skin ramp (18's arms)
        px = [[skin.get(table[v][0], table[v][0]) if v else 0 for v in row] for row in bpx]
        # head rows 3..16 (hair to the chin; row 15-16 hold the collar at the sides: dropped)
        keep = lambda y, x, v: not (y >= 15 and v in (112, 33, 37))  # noqa: E731
        return cut_head(px, (3, 16), keep, {114, 115, 116} if f != 1 else {1, 15})


# King Cold from Cooler (34) --------------------------------------------------------------------------------------
COOLER = 34
COLD = 74
COLD_PLAN = {0: (COOLER, 0), 1: (COOLER, 1), 2: (COOLER, 2), 3: (COOLER, 12), 4: (COOLER, 5), 5: (COOLER, 5),
             6: (COOLER, 6), 7: (COOLER, 7), 8: (COOLER, 8), 11: (COOLER, 2), 12: (COOLER, 12), 13: (COOLER, 1),
             16: (COOLER, 7), 17: (COOLER, 0), 18: (COOLER, 5), 20: (COOLER, 12)}
COLD_BODY = {9: 2, 10: 166, 11: 12, 12: 13, 13: 14, 211: 97, 229: 95, 231: 101, 238: 99, 215: 211, 77: 64}
CAPE = (25, 26, 27)                 # red, mid, dark (74's cape)


class KingCold(Fighter):
    header_sprite = COOLER
    plan = COLD_PLAN

    def __init__(self, src):
        super().__init__(src)
        self.heads = self.cold_heads()

    def cold_heads(self):
        """74's head: front from idle facing down, side from idle facing left; the back view is the front with
        the face (eyes, mouth, cheeks) covered by the white-mint skull plate. Anchor = Cold's mouth row and the
        dome's centre, matched to Cooler's mouth row (gem + 6) and gem x."""
        front = self.src.canon(COLD, 0, 0, 0)
        side = self.src.canon(COLD, 0, 2, 0)

        def cut(px, mouth_row):
            keep = {(y, x): v for y in range(0, 18) for x in range(len(px[0])) if (v := px[y][x])}
            # drop the shoulder pads' tips that reach row 17 at the sides
            keep = {(y, x): v for (y, x), v in keep.items() if not (y >= 16 and v in (1, 211))}
            dome = [x for (y, x), v in keep.items() if v in (64, 192)]
            return Head(keep, mouth_row, sum(dome) / len(dome))

        heads = [cut(front, 15), None, cut(side, 15)]
        back = {}
        for (dy, dx), v in heads[0].pts.items():
            if dy >= -5 and v not in (12, 13, 14, 15):      # the face area -> the back of his head (skin)
                v = 1 if v == 1 or abs(dx) >= 7 or dy == 2 else (97 if abs(dx) >= 5 or dy == 1 else (95 if dy >= -1 else 101))
            back[(dy, dx)] = v
        b = Head({}, 0, 0)
        b.pts = back
        heads[1] = b
        return heads

    def transform(self, sid, gi, px, f):
        h, w = len(px), len(px[0])
        gem = sorted((y, x) for y in range(h) for x in range(w) if px[y][x] == 77)
        out = recolour(px, COLD_BODY)
        top = top_row(px)
        if f == 1:      # back view: no gem; the head is the top of the body, anchor from the idle geometry
            if gi in (3, 12, 20):
                return out
            gy = top + 11
            row = [x for x in range(w) if px[gy][x]]
            if not row:
                return out
            gx = (min(row) + max(row)) / 2
        else:
            lying = gi in (3, 12, 20)
            if not gem:
                return out
            gy, gx = gem[0]
            if gy - top > 18 and not lying:
                return out
            if lying and not any(px[y][x] == 25 for y in range(gy + 4, min(h, gy + 8)) for x in range(w)):
                return out          # lying with the face out of sight
        mouth = gy + 6
        lo, hi = (gy - 12, mouth) if gi not in (3, 12, 20) else (gy - 3, mouth + 4)   # lying: the body is above
        head = flood(px, [(y, x) for y in range(max(0, lo), gy + 1) for x in range(w)
                          if px[y][x] and abs(x - gx) <= 12],
                     lambda y, x, v: v != 0 and lo <= y <= hi and abs(x - gx) <= 12, N8)
        for y, x in head:
            out[y][x] = 0
        if gi not in (3, 12, 20):
            self.cape(out, px, f, top, mouth, gx)
        self.heads[f].paste(out, mouth, gx + (0 if f != 2 else 1))
        self.stats["heads"] += 1
        return out

    def cape(self, out, px, f, top, mouth, gx):
        """Cold's red cape: from the shoulders (two rows under the mouth) down to the knees, behind the body
        (only on transparent pixels) for front and side views, over the back for the back view."""
        h, w = len(out), len(out[0])
        y0 = mouth + 2
        bottom = max(y for y in range(h) if any(px[y]))
        y1 = min(h - 1, y0 + int((bottom - y0) * 0.72))
        mask = {}
        for y in range(y0, y1 + 1):
            t = (y - y0) / max(1, y1 - y0)
            if f == 0:
                half = 9 + 6 * t
                xs = range(int(gx - half), int(gx + half) + 1)
            elif f == 1:
                half = 8 + 5 * t
                xs = range(int(gx - half), int(gx + half) + 1)
            else:   # side, facing left: the cape hangs from the back shoulder and trails right
                x0 = gx + 5 - 1 * t
                xs = range(int(x0), int(x0 + 7 + 9 * t) + 1)
            for x in xs:
                if 0 <= x < w:
                    mask[(y, x)] = True
        # a ragged hem: every other 3-column stretch hangs one row longer
        for (y, x) in list(mask):
            if y == y1 and (x // 3) % 2:
                mask.pop((y, x))
        for (y, x) in mask:
            edge = any((y + dy, x + dx) not in mask for dy, dx in N4)
            fold = (x - int(gx)) % 4 == 0 and y > y0 + 2          # vertical folds
            side = abs(x - gx) >= (9 + 6 * (y - y0) / max(1, y1 - y0)) - 2 if f != 2 else False
            c = BLACK if edge else (CAPE[2] if fold else (CAPE[1] if side or y <= y0 + 1 else CAPE[0]))
            if f == 1:
                if out[y][x] in (97, 95, 101, 99) and abs(x - gx) > 6:   # the arms stay over the cape
                    continue
                out[y][x] = c
            elif not out[y][x]:
                out[y][x] = c


# Master Roshi from Krillin's body (76 + Goku 60) ------------------------------------------------------------------
KRILLIN = 76
GOKU = 60
GI = {32, 33, 34, 35, 39}
ROSHI_PLAN = {0: (KRILLIN, 0), 1: (KRILLIN, 1), 2: (KRILLIN, 2), 3: (KRILLIN, 3), 4: (KRILLIN, 4),
              5: (KRILLIN, 5), 6: (KRILLIN, 6), 7: (GOKU, 7), 8: (GOKU, 8), 11: (GOKU, 11), 12: (KRILLIN, 12),
              13: (GOKU, 13), 16: (GOKU, 16), 17: (KRILLIN, 0), 18: (GOKU, 18), 20: (KRILLIN, 3)}
SHELL = (57, 58, 50, 49)            # light, mid, dark green, highlight


class Roshi(Fighter):
    header_sprite = GOKU
    plan = ROSHI_PLAN

    def __init__(self, src):
        super().__init__(src)
        self.chin_off = {}
        for sid in (KRILLIN, GOKU):
            for f in range(3):
                px = src.canon(sid, 0, f, 0)
                top = self.head_top(sid, px)
                neck = next(y for y in range(top + 6, len(px)) if sum(1 for v in px[y] if v in GI) >= 3)
                self.chin_off[(sid, f)] = neck - 1 - top
        self.heads = [self.donor(f) for f in range(3)]

    def donor(self, f):
        px = self.src.canon(88, 0, f, 0)
        # head rows 5..15 without the shirt; the beard (greys under the chin, joined to it) comes along
        head = {(y, x): v for y in range(5, 16) for x in range(16) if (v := px[y][x]) and v not in GI}
        beard = flood(px, [(y, x) for x in range(16) for y in (15, 16) if px[y][x] in (2, 10, 12, 13)],
                      lambda y, x, v: v in (2, 9, 10, 12, 13) and y >= 15)
        if f == 1:
            beard = set()
        extra = {(y, x): px[y][x] for y, x in beard}
        # the beard's outline: grey 13 where it meets the shirt or the air below
        for (y, x) in list(beard):
            for dy, dx in N4:
                q = (y + dy, x + dx)
                if q not in beard and q not in head and q[0] > 15 and 0 <= q[1] < 16:
                    if dy == 1 or px[q[0]][q[1]] in GI:
                        extra.setdefault(q, 13)
        skin = [x for (y, x), v in head.items() if v in (114, 115, 116, 117)]
        h = Head({**head, **extra}, 15, sum(skin) / len(skin))
        return h

    def head_top(self, sid, px):
        if sid == GOKU:
            hair = self.goku_hair(px)
            return min(y for y, _ in hair) if hair else top_row(px)
        return top_row(px)

    @staticmethod
    def goku_hair(px):
        h = len(px)
        comps = [c for c in components([row[:] for row in px[:max(10, int(h * 0.6))]] + [[0] * len(px[0])] * (h - max(10, int(h * 0.6))),
                                       lambda y, x, v: v == BLACK) if len(c) >= 25]
        comps = [c for c in comps if min(y for y, _ in c) <= h * 0.45]
        return max(comps, key=len) if comps else None

    def transform(self, sid, gi, px, f):
        out = copy(px)
        h, w = len(px), len(px[0])
        rows = [y for y in range(h) if any(px[y])]
        if gi == 12 or (gi in (3, 20) and rows[-1] - rows[0] < 22):     # lying / falling: no upright head
            return out
        if sid == GOKU:
            hair = self.goku_hair(px)
            if not hair:
                return out
            top = min(y for y, _ in hair)
            erase = set(hair)
        else:
            top = top_row(px)
            erase = set()
        chin = min(h - 1, top + self.chin_off[(sid, f)])
        band = [(y, x) for y in range(top, chin + 1) for x in range(w)]
        face = [x for y, x in band if px[y][x] in (114, 115, 116)]
        if len(face) < 4 and sid == KRILLIN and f != 1:
            return out
        cx = sum(face) / len(face) if face else sum(x for _, x in erase) / max(1, len(erase))
        for y, x in erase:
            out[y][x] = 0
        # raised hands: skin components touching a wristband (blue) away from the face stay
        keep = set()
        for c in components(px, lambda y, x, v: v in (114, 115, 116, 117)):
            if abs(sum(x for _, x in c) / len(c) - cx) > 4 and any(
                    near(px, y, x, {79, 80, 195, 215}, N4) for y, x in c):
                keep |= flood(px, list(c), lambda y, x, v: v in (114, 115, 116, 117, 23, 1), N4) & set(band)
        r = 9 if f == 2 else 7
        for y, x in band:
            if abs(x - cx) <= r and px[y][x] not in GI | {79, 80, 195, 215} and (y, x) not in keep:
                out[y][x] = 0
        if f == 1:
            self.shell(out, px, f, (chin, cx))
        self.heads[f].paste(out, chin, cx)
        self.stats["heads"] += 1
        return out

    def shell(self, out, px, f, anchor):
        """The turtle shell on his back (back view): an oval from under the neck to the belt, green with a
        darker hexagon pattern and a black rim; the arms (skin) stay in front of it."""
        h, w = len(out), len(out[0])
        if anchor is None:
            return out
        chin, cx = anchor
        cy0, cy1 = chin + 2, chin + 11
        rx = 5.2
        ry = (cy1 - cy0) / 2
        my = (cy0 + cy1) / 2
        pts = {}
        for y in range(cy0, cy1 + 1):
            for x in range(w):
                dx, dy = (x + 0.5 - (cx + 0.5)) / rx, (y + 0.5 - my - 0.5) / ry
                if dx * dx + dy * dy <= 1.0:
                    pts[(y, x)] = dx * dx + dy * dy
        for (y, x), r in pts.items():
            if px[y][x] in (114, 115, 116, 117):       # hands over the shell
                continue
            edge = any((y + dy, x + dx) not in pts for dy, dx in N4)
            if edge:
                c = BLACK
            elif (y - cy0) in (3, 6) or (x == int(round(cx)) - 2 or x == int(round(cx)) + 2) and (y - cy0) not in (3, 6):
                c = SHELL[2] if r > 0.15 else SHELL[1]
            else:
                c = SHELL[3] if (y - cy0) <= 2 and r < 0.5 else SHELL[0]
            out[y][x] = c
        return out


# Chiaotzu from kid Gohan (53) -------------------------------------------------------------------------------------
GOHAN = 53
CHIAOTZU = 32
CH_PLAN = {g: (GOHAN, g) for g in (0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 16, 17, 18, 20)}
GREYS = {2, 9, 10, 11, 12, 13, 14}
CH_SHIRT = {2: 74, 9: 74, 10: 74, 11: 75, 12: 75, 13: 78, 14: 78}
CH_BODY = {80: 15, 81: BLACK, 211: BLACK, 114: 2, 115: 10, 116: 12, 117: 13, 23: 13}   # trousers black, skin white


class Chiaotzu(Fighter):
    header_sprite = GOHAN
    plan = CH_PLAN

    def __init__(self, src):
        super().__init__(src)
        self.chin_off = []
        for f in range(3):
            px = src.canon(GOHAN, 0, f, 0)
            hair = Roshi.goku_hair(px)
            top = min(y for y, _ in hair)
            neck = next(y for y in range(top + 6, len(px)) if sum(1 for v in px[y] if v in (24, 25, 26, 27, 12, 10, 211)) >= 3)
            self.chin_off.append(neck - 1 - top)
        self.heads = [self.donor(f) for f in range(3)]

    def donor(self, f):
        px = self.src.canon(CHIAOTZU, 0, f, 0)
        head = {(y, x): v for y in range(3, 16) for x in range(16) if (v := px[y][x])}
        face = [x for (y, x), v in head.items() if v in (2, 10, 242)] or [x for (y, x), v in head.items()]
        return Head(head, 15, sum(face) / len(face))

    def shirt(self, px):
        """Kid Gohan's white shirt: grey components touching the red sash or the skin, at least 4 pixels, not
        touching his purple trousers (the socks and shoes do)."""
        out = set()
        for c in components(px, lambda y, x, v: v in GREYS):
            if len(c) < 4:
                continue
            touch = {px[y + dy][x + dx] for y, x in c for dy, dx in N4
                     if 0 <= y + dy < len(px) and 0 <= x + dx < len(px[0])}
            if touch & {80, 81} and not touch & {24, 25, 26, 27}:
                continue
            sash = [y for y, row in enumerate(px) for v in row if v in (24, 25, 26, 27)]
            if not sash or sum(y for y, _ in c) / len(c) >= sum(sash) / len(sash):
                continue                             # the shoes and the swing arcs sit below the sash
            if touch & {24, 25, 26, 27, 211}:      # the sash or the shirt's outline
                out |= c
        return out

    def transform(self, sid, gi, px, f):
        h, w = len(px), len(px[0])
        shirt = self.shirt(px)
        out = [[(CH_SHIRT[v] if (y, x) in shirt else CH_BODY.get(v, v)) for x, v in enumerate(row)]
               for y, row in enumerate(px)]
        hair = Roshi.goku_hair(px)
        if gi in (3, 12, 20):
            comps = [c for c in components(px, lambda y, x, v: v == BLACK)
                     if len(c) >= 15 and any(near(px, y, x, {114, 115, 116, 117}, N4) for y, x in c)]
            hair = max(comps, key=len) if comps else None
        if not hair:
            return out
        top = min(y for y, _ in hair)
        if gi in (3, 12, 20):
            # lying / tumbling: his black hair reads as the cap once its spikes are cut back round the face
            face = [(y, x) for y in range(h) for x in range(w) if px[y][x] in (114, 115, 116, 117)
                    and any(abs(y - hy) + abs(x - hx) <= 3 for hy, hx in hair)]
            if face:
                fy = sum(y for y, _ in face) / len(face)
                fx = sum(x for _, x in face) / len(face)
                for y, x in hair:
                    if (y - fy) ** 2 + (x - fx) ** 2 > 36:
                        out[y][x] = 0
                for y, x in hair:   # outline what is left
                    if out[y][x] and any(0 <= y + dy < h and 0 <= x + dx < w and (y + dy, x + dx) in hair
                                         and not out[y + dy][x + dx] for dy, dx in N4):
                        out[y][x] = BLACK
            return out
        chin = min(h - 1, top + self.chin_off[f])
        band = [(y, x) for y in range(top, chin + 1) for x in range(w)]
        face = [x for y, x in band if px[y][x] in (114, 115, 116, 117)]
        xs = [x for _, x in hair]
        cx = (sum(face) / len(face)) if len(face) >= 5 else (sum(xs) / len(xs))
        for y, x in hair:
            out[y][x] = 0
        for y, x in band:
            if abs(x - cx) <= 6 and (px[y][x] in SKIN | {1, 2, 11, 15, 23, 117} and (y, x) not in shirt):
                out[y][x] = 0
        self.heads[f].paste(out, chin, cx)
        self.stats["heads"] += 1
        return out


# ---- record assembly --------------------------------------------------------------------------------------------
def build(fighter, src, template_counts):
    """groups[gi] = None or [4 facings][frames] of (xo, yo, w, h, attr, stored px)."""
    groups = []
    cache = {}
    n = max(len(template_counts), max(fighter.plan) + 1)
    for gi in range(n):
        count = template_counts[gi] if gi < len(template_counts) and template_counts[gi] else EXTRA_COUNTS.get(gi, 0)
        if gi not in fighter.plan or not count:
            groups.append(None)
            continue
        sid, sg = fighter.plan[gi]
        dirs = []
        for d in range(4):
            frames = []
            nsrc = src.count(sid, sg, d)
            assert nsrc, f"{type(fighter).__name__}: sprite {sid} group {sg} facing {d} missing"
            for k in range(count):
                fi = k % nsrc
                xo, yo, w, h, attr, blob, _ = src.frame(sid, sg, d, fi)
                f = 2 if d == 3 else d
                key = (sid, blob, bool(attr & HF), d == 3, gi in (3, 12, 20))
                if key not in cache:
                    canon = src.canon(sid, sg, d, fi)
                    new = fighter.transform(sid, gi, copy(canon), f)
                    cache[key] = to_stored(new, attr, d)
                    fighter.stats["frames"] += 1
                frames.append((xo, yo, w, h, attr, cache[key]))
            dirs.append(frames)
        groups.append(dirs)
    return groups


def header_for(src, fighter, groups):
    rec = src.sp.record(fighter.header_sprite)
    hdr = bytearray(src.rom[rec - B:rec - B + 0x4C])
    hdr[2] = len(groups)
    size = max(f[2] * f[3] for g in groups if g for dirs in g for f in dirs)
    struct.pack_into("<H", hdr, 0, max(size, struct.unpack_from("<H", hdr, 0)[0]))
    return hdr


def videl_portrait(log, buu):
    """Buu's Fury's short-haired Videl portrait (module 153) on LoG2's grey frame, as tools/oldherculeportrait.py
    does for Hercule; without the Buu ROM, LoG2's Chi-Chi portrait (43)."""
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import oldherculeportrait as OH
    ref = m.unpack(log, struct.unpack_from('<I', log, PORTRAIT_TABLE - B + 4 * OH.LOG_HERCULE)[0])[:4096]
    if buu is None:
        return bytes(m.unpack(log, struct.unpack_from('<I', log, PORTRAIT_TABLE - B + 4 * 43)[0])[:4096])
    W = 64
    import buu_sprites as BS
    bpal = [S.Sprites.rgb(c) for c in struct.unpack_from('<256H', buu, BS.PALETTE - B)]
    lpal = S.Sprites(log).pal
    src = m.unpack(buu, struct.unpack_from('<I', buu, BUU_PORTRAIT_MODULE - B + 4 * BUU_VIDEL_PORTRAIT)[0])[:4096]
    frame, stack = set(), [(y, x) for y in range(W) for x in range(W) if y in (0, W - 1) or x in (0, W - 1)]
    while stack:
        y, x = stack.pop()
        if (y, x) in frame or not (0 <= y < W and 0 <= x < W):
            continue
        if src[y * W + x] not in OH.BUU_FRAME or min(y, x, W - 1 - y, W - 1 - x) > 5:
            continue
        frame.add((y, x))
        stack += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    pic = {(y, x): src[y * W + x] for y in range(W) for x in range(W) if (y, x) not in frame}
    while len(pic) < W * W:
        add = {}
        for y in range(W):
            for x in range(W):
                if (y, x) in pic:
                    continue
                for dy, dx in N4:
                    if (y + dy, x + dx) in pic:
                        add[(y, x)] = pic[(y + dy, x + dx)]
                        break
        pic.update(add)
    tbl = nearest_table(bpal, lpal)
    out = bytearray(ref)
    for y in range(3, W - 3):
        for x in range(3, W - 3):
            out[y * W + x] = tbl[pic[(y, x)]][0] or 1
    return bytes(out)


def write_c(path, records, portrait):
    lines = ["/* GENERATED by tools/tnysprites.py from the ROMs: the tournament's five new fighters (King Cold, Master",
             " * Roshi, Chi-Chi, Videl, Chiaotzu) and Videl's dialogue portrait. Build output only; never commit. */",
             '#include "tourney.h"', "",
             "typedef struct { signed char xo, yo; u8 w, h; u32 attr; const void *blob; } TnyDesc;",
             "static const u32 tny_no_action[2] TNY_ART = {0x08010699u, 0};   /* ENEMY_ACTION_NONE */", ""]
    blobs = {}
    total = 0

    def blob_of(px, w, h):
        nonlocal total
        key = (w, h, tuple(tuple(r) for r in px))
        if key not in blobs:
            data = compress(tiles(px, w, h))
            assert m.decompress(data, 8, 1)[:w * h] == tiles(px, w, h)
            name = f"tny_blob_{len(blobs)}"
            blobs[key] = name
            total += len(data)
            lines.append(f"static const u8 {name}[{len(data)}] TNY_ART __attribute__((aligned(4))) = "
                         f"{{{', '.join(f'0x{b:02x}' for b in data)}}};")
        return blobs[key]

    for name, hdr, groups in records:
        grp = []
        for gi, g in enumerate(groups):
            if not g:
                grp.append("{0, 0, 0, 0}")
                continue
            arrs = []
            for d, frames in enumerate(g):
                entries = [f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {blob_of(px, w, h)}}}" for xo, yo, w, h, attr, px in frames]
                an = f"{name}_g{gi}_d{d}"
                lines.append(f"static const TnyDesc {an}[{len(frames)}] TNY_ART = {{{', '.join(entries)}}};")
                arrs.append(an)
            grp.append("{" + ", ".join(arrs) + "}")
        # after the record (the engine reads only header[2] groups): a test enemy record for COOP_CMD_SPAWN_RECORD
        # (8), at record + 0x50 + 16 * groups: handler, flags, stat, x | y, sprite, action, behaviours
        sprite, stat = TEST_RECORD[name]
        lines.append(f"typedef struct {{ u8 header[0x4C]; const TnyDesc *groups[{len(groups)}][4]; u32 end; "
                     f"u32 test[7]; }} {name}_t;")
        lines.append(f"const {name}_t {name} TNY_ART = {{{{{', '.join(f'0x{b:02x}' for b in hdr)}}}, "
                     f"{{{', '.join(grp)}}}, 0, {{0x0800E77Fu, 0, {stat}, 0, {sprite}, (u32)tny_no_action, 0}}}};")
    pb = compress(portrait)
    assert m.decompress(pb, 8, 1)[:4096] == portrait
    total += len(pb)
    lines.append(f"const u8 tny_videl_portrait[{len(pb)}] TNY_ART __attribute__((aligned(4))) = "
                 f"{{{', '.join(f'0x{b:02x}' for b in pb)}}};")
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    open(path, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    return len(blobs), total


def write_preview(path, sp, groups, scale=3):
    """Every group x 4 facings x every frame, drawn at the frames' offsets on a common ground line."""
    rows = [(gi, g) for gi, g in enumerate(groups) if g]
    big = any(f[2] > 32 or f[3] > 32 for _, g in rows for dirs in g for f in dirs)
    cw, ch = (72, 72) if big else (40, 40)
    ox, oy = (cw // 2, ch - 8)
    ncol = max(sum(len(dirs) + 1 for dirs in g) for _, g in rows)
    cv = S.Canvas(ncol * cw + 24, len(rows) * ch, (60, 60, 90))
    for r, (gi, g) in enumerate(rows):
        cv.text(2, r * ch + 2, str(gi), (255, 255, 0))
        c = 0
        for d, frames in enumerate(g):
            for xo, yo, w, h, attr, px in frames:
                disp = hflip(px) if attr & HF else px
                for y in range(h):
                    for x in range(w):
                        if disp[y][x]:
                            cv.put(24 + c * cw + ox + xo + x, r * ch + oy + yo + y, sp.pal[disp[y][x]])
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
    only = sys.argv[sys.argv.index("--only") + 1].split(",") if "--only" in sys.argv else FIGHTERS
    if "--ids" in sys.argv:     # the Makefile's KING_COLD_SPRITE ... CHIAOTZU_SPRITE, in FIGHTERS order
        for name, sid in zip(FIGHTERS, sys.argv[sys.argv.index("--ids") + 1].split(",")):
            TEST_RECORD[SYMBOL[name]] = (int(sid), TEST_RECORD[SYMBOL[name]][1])
    log = open(rom_path, "rb").read()
    buu = open(buu_path, "rb").read() if buu_path != "none" and os.path.exists(buu_path) else None
    src = Source(log)
    tg, tc = src.groups(TEMPLATE)
    template_counts = [tc[g[0]] if g[0] else 0 for g in tg]
    if buu is not None:
        import buu_sprites as BS
        bu = BS.BuuSprites(buu)
        Videl.buu = (bu, nearest_table(bu.pal, src.sp.pal))
    classes = {"cold": KingCold, "roshi": Roshi, "chichi": ChiChi, "videl": Videl, "chiaotzu": Chiaotzu}
    records = []
    for name in FIGHTERS:
        if name not in only:
            continue
        f = classes[name](src)
        groups = build(f, src, template_counts)
        hdr = header_for(src, f, groups)
        records.append((SYMBOL[name], hdr, groups))
        if pdir:
            os.makedirs(pdir, exist_ok=True)
            write_preview(os.path.join(pdir, f"tny_preview_{name}.png"), src.sp, groups)
        print(f"  {name}: {f.stats['frames']} frames, {f.stats['heads']} re-headed, {len(groups)} groups, "
              f"largest frame {struct.unpack_from('<H', hdr, 0)[0]:#x}")
    portrait = videl_portrait(log, buu)
    nb, total = write_c(out_c, records, portrait)
    if pdir:
        cv = S.Canvas(64, 64)
        for i, v in enumerate(portrait):
            cv.put(i % 64, i // 64, src.sp.pal[v])
        cv.save(os.path.join(pdir, "tny_preview_videl_portrait.png"))
    print(f"wrote {out_c}: {len(records)} records, {nb} frame blobs, {total} bytes compressed")


if __name__ == "__main__":
    main()
