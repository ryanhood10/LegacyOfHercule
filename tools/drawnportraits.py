"""Hand-drawn dialogue portraits (Ryan, 2026-11-12: Android 13's and Super 13's were stock faces repainted, Paragus' was Vegeta's with a
beard: "is it possible for us to get anime-accurate portraits", "in akira toriyama's style"): Android 13 (169), Super Android 13 (122) and
Paragus (120), each drawn here pixel by pixel from the way the movies draw them, in LoG2's OBJ palette and the stock portraits' manner
(black line, flat colour, one shade, the stock gradient behind, the 3 px grey frame). Nothing is copied from a picture: every shape below
is a polygon, a line or a row of pixels of ours. The ROM gives only the frame and the background gradient of a stock portrait.

    a13(rom), s13(rom), paragus(rom) -> 4096 palette indices (64 x 64), as ppportraits.py / super13sprites.py / brolysprites.py emit them
    python -I tools/drawnportraits.py roms/log2.gba SHEET.png      a sheet of the three (x6) for looking at

Coordinates are the portrait's own (x right, y down, 3..60 inside the frame). `Pic.tf` enlarges a drawing about a point."""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mapchunks as m  # noqa: E402
import sprites as S  # noqa: E402
from PIL import Image, ImageDraw  # noqa: E402

LOG_TABLE, B = 0x083EC9D4, 0x08000000      # Portrait_Table (oldherculeportrait.py)
W = 64
CX = 31.5
_ROM = None


def stock(i):
    return bytearray(m.unpack(_ROM, struct.unpack_from("<I", _ROM, LOG_TABLE - B + 4 * i)[0])[:4096])


def flood(pic, colours, seeds):
    seen, stack = set(), list(seeds)
    while stack:
        y, x = stack.pop()
        if (y, x) in seen or not (3 <= y < 61 and 3 <= x < 61) or pic[y * W + x] not in colours:
            continue
        seen.add((y, x))
        stack += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    return seen


def background(sources, colours):
    """The stock gradient behind the given stock portraits: each pixel from whichever shows background there, the rest from the nearest known."""
    bg = [None] * 4096
    for s in sources:
        pic = stock(s)
        seen = flood(pic, set(colours), [(3, x) for x in range(3, 61)] + [(y, 3) for y in range(3, 61)] + [(y, 60) for y in range(3, 61)])
        for (y, x) in seen:
            if bg[y * W + x] is None:
                bg[y * W + x] = pic[y * W + x]
    known = [(i // W, i % W) for i in range(4096) if bg[i] is not None]
    for y in range(3, 61):
        for x in range(3, 61):
            if bg[y * W + x] is None:
                # the same diagonal first (the gradients run along them), else the nearest
                best = min(known, key=lambda k: (abs((k[0] + k[1]) - (y + x)) * 3 + abs(k[0] - y) + abs(k[1] - x), (k[0] + k[1] + y + x) & 1))
                bg[y * W + x] = bg[best[0] * W + best[1]]
    frame = stock(sources[0])
    for i in range(4096):
        y, x = divmod(i, W)
        if not (3 <= y < 61 and 3 <= x < 61):
            bg[i] = frame[i]
    return bg


class Pic:
    def __init__(self, bg):
        self.im = Image.new("P", (W, W))
        self.im.putdata(bg)
        self.d = ImageDraw.Draw(self.im)
        self.frame = list(bg)
        self.tf = None                 # (cx, cy, scale, dx, dy): the drawing's coordinates enlarged about a point

    def q(self, pt):
        if not self.tf:
            return pt
        cx, cy, k, dx, dy = self.tf
        return (round(cx + (pt[0] - cx) * k + dx), round(cy + (pt[1] - cy) * k + dy))

    def poly(self, pts, fill, outline=None):
        self.d.polygon([self.q(t) for t in pts], fill=fill, outline=outline)

    def mpoly(self, pts, fill, outline=None):
        """the polygon and its mirror about the centre line"""
        self.poly(pts, fill, outline)
        self.poly([(2 * CX - x, y) for x, y in pts], fill, outline)

    def line(self, pts, c, w=1):
        self.d.line([self.q(t) for t in pts], fill=c, width=w)

    def mline(self, pts, c, w=1):
        self.line(pts, c, w)
        self.line([(2 * CX - x, y) for x, y in pts], c, w)

    def px(self, x, y, c):
        x, y = self.q((x, y))
        self.im.putpixel((int(x), int(y)), c)

    def mpx(self, x, y, c):
        self.px(x, y, c)
        self.px(2 * CX - x, y, c)

    def rows(self, x0, y0, art, key):
        """ASCII art: key maps a character to a colour; '.' leaves the pixel"""
        for j, row in enumerate(art):
            for i, ch in enumerate(row):
                if ch != "." and ch != " ":
                    self.px(x0 + i, y0 + j, key[ch])

    def get(self, x, y):
        return self.im.getpixel((int(x), int(y)))

    def data(self):
        out = list(self.im.tobytes())
        for i in range(4096):
            y, x = divmod(i, W)
            if not (3 <= y < 61 and 3 <= x < 61):
                out[i] = self.frame[i]
        return out


def png(pics, path, k=4, cols=None):
    pal = S.Sprites(_ROM).pal
    cols = cols or len(pics)
    rows = (len(pics) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * (64 * k + 8), rows * (64 * k + 8)), (40, 40, 40))
    for n, p in enumerate(pics):
        im = Image.new("RGB", (64, 64))
        im.putdata([tuple(pal[c]) for c in p[:4096]])
        sheet.paste(im.resize((64 * k, 64 * k), Image.NEAREST), ((n % cols) * (64 * k + 8) + 4, (n // cols) * (64 * k + 8) + 4))
    sheet.save(path)


def a13(rom):
    """Android 13 as the movie draws him, three-quarter: the trucker cap low over his eyes (a dark mesh back, the mustard front with its black
patch, here reading 13, the long black bill), wispy white hair blown back, the long jaw and the smirk, the olive vest."""
    global _ROM
    _ROM = rom
    K, OUT = 1, 113
    S1, S2, S3, S4 = 114, 115, 116, 117   # skin: light, base, shade, dark
    Y1, Y2, Y3 = 38, 120, 33              # the cap's front: mustard, its shade
    M1, M2 = 42, 167                      # the mesh back: dark olive, dark green
    H1, H2, H3, H4 = 2, 9, 10, 12         # white hair
    V1, V2, V3 = 133, 41, 42              # the vest's olive
    p = Pic(background((28, 29, 40), {25, 252, 31, 30, 29}))
    # ---- the hair: long, wispy, blown back to the left, and a few locks behind the neck on the right
    hair = [(21, 22), (14, 24), (8, 28), (3, 31), (8, 32), (3, 38), (9, 37), (4, 44), (11, 42), (8, 49), (15, 45), (15, 51), (20, 47), (23, 50), (25, 42),
            (25, 28)]
    p.poly(hair, H2, K)
    for a, b, c in (((20, 24), (9, 30), H1), ((20, 27), (8, 35), H1), ((21, 31), (9, 41), H1), ((22, 35), (12, 46), H1), ((23, 40), (18, 48), H1),
                    ((21, 25), (12, 29), H3), ((21, 29), (11, 36), H3), ((22, 33), (12, 42), H4), ((23, 38), (16, 46), H3), ((24, 42), (21, 48), H4)):
        p.line([a, b], c)
    p.poly(hair, None, K)
    # ---- the neck, the shoulder, the vest
    p.poly([(3, 60), (3, 56), (11, 53), (22, 47), (41, 50), (45, 54), (48, 60)], S2, OUT)
    p.poly([(26, 46), (41, 50), (44, 55), (37, 57), (28, 52)], S3)            # under the jaw
    p.poly([(31, 51), (41, 51), (43, 55), (37, 56)], S4)
    p.line([(28, 53), (32, 60)], OUT)
    p.line([(38, 57), (39, 60)], OUT)
    p.poly([(3, 60), (3, 55), (10, 52), (17, 51), (21, 55), (23, 60)], V1, K)
    p.line([(5, 56), (11, 54), (16, 54)], 129)
    p.line([(17, 53), (20, 57), (21, 59)], V2)
    p.line([(4, 59), (13, 57)], V3)
    # ---- the head: the long face, the far cheek, the heavy jaw
    face = [(24, 22), (50, 22), (50, 27), (48, 30), (49, 34), (48, 39), (46, 45), (43, 50), (39, 53), (33, 53), (28, 49), (25, 43), (24, 35)]
    p.poly(face, S1, OUT)
    p.poly([(25, 22), (50, 22), (50, 27), (48, 29), (40, 30), (31, 29), (25, 28)], S2)        # the bill's shadow over the eyes
    p.poly([(25, 36), (28, 45), (33, 51), (39, 52), (34, 47), (30, 41), (28, 34)], S2)        # the jaw
    p.poly([(26, 40), (29, 47), (33, 52), (31, 47), (28, 41)], S3)
    p.poly(face, None, OUT)
    # ---- the ear with its ring, a lock in front of it
    p.poly([(20, 28), (25, 27), (26, 37), (23, 39), (20, 34)], S1, OUT)
    p.line([(22, 30), (23, 33), (22, 35)], S3)
    p.line([(22, 40), (22, 42)], 40)
    p.px(23, 42, 133)
    p.line([(25, 23), (26, 27)], H3)
    # ---- brows and eyes: narrow, sharp, in the cap's shadow
    p.poly([(30, 27), (33, 27), (41, 29), (42, 31), (34, 29), (30, 28)], K)
    p.line([(32, 31), (40, 32)], K)
    p.line([(33, 32), (40, 33)], K)
    p.line([(34, 33), (36, 33)], 2)
    p.px(37, 33, 76)
    p.px(38, 33, K)
    p.line([(34, 34), (38, 34)], OUT)
    p.poly([(44, 29), (48, 28), (48, 29), (45, 30)], K)
    p.line([(44, 31), (47, 31)], K)
    p.px(44, 32, 76)
    p.px(45, 32, K)
    p.px(46, 32, 2)
    # ---- the nose, the smirk
    p.line([(43, 31), (44, 34), (46, 38)], OUT)
    p.line([(46, 39), (45, 40), (42, 40)], OUT)
    p.line([(41, 35), (41, 39)], S2)
    p.line([(36, 44), (38, 45), (43, 44), (45, 43)], OUT)
    p.px(35, 43, OUT)
    p.px(46, 42, OUT)
    p.line([(39, 47), (43, 47)], S2)
    p.line([(40, 40), (38, 43)], S2)
    p.line([(31, 31), (32, 35)], S2)
    # ---- the cap
    crown = [(12, 24), (11, 15), (14, 8), (21, 4), (30, 3), (40, 4), (47, 8), (50, 14), (51, 22)]
    p.poly(crown, Y1, K)
    p.poly([(12, 24), (11, 15), (14, 8), (21, 4), (26, 3), (25, 23)], M1)     # the mesh back
    for y in range(4, 24):
        for x in range(11, 26):
            if (x + y) & 1 and p.get(x, y) == M1:
                p.px(x, y, M2)
    p.line([(26, 3), (25, 23)], K)
    p.poly([(27, 5), (31, 4), (30, 21), (27, 22)], Y2)                        # the front panel's turn
    p.line([(44, 7), (48, 11), (50, 16)], Y2)
    p.poly([(32, 8), (47, 9), (48, 18), (32, 18)], K)                         # the patch
    p.rows(34, 10, ["..#..###.", ".##.....#", "..#.....#", "..#...##.", "..#.....#", "..#.....#", ".###.###."], {"#": H1})
    p.poly(crown, None, K)
    p.poly([(12, 23), (51, 21), (51, 24), (12, 25)], K)                       # the band
    p.poly([(30, 23), (51, 21), (57, 24), (61, 29), (60, 31), (52, 28), (42, 26), (30, 26)], K)   # the bill
    p.line([(36, 23), (51, 22), (56, 25), (59, 28)], 15)
    p.line([(44, 25), (52, 27), (58, 30)], 14)
    return p.data()


def s13(rom):
    """Super Android 13 as the movie draws him, close on the face: royal blue skin, the furrowed brow under the orange flame of hair, slanted
blank yellow eyes in a dark mask, gritted teeth, the ear rings."""
    global _ROM
    _ROM = rom
    K, OUT = 1, 211
    B1, B2, B3 = 77, 72, 215              # the blue skin: light, base, shade
    O1, O2, O3 = 32, 33, 26               # the hair: orange, its mid, the red of its shadowed side
    EYE, TOOTH = 46, 2
    p = Pic([O2] * 4096)
    p.frame = stock(28)
    # ---- the hair fills the frame behind the head: red on his right, orange on his left, in upward locks
    p.poly([(3, 3), (30, 3), (30, 30), (3, 30)], O3)
    p.poly([(32, 3), (60, 3), (60, 30), (32, 30)], O1)
    for lock, c in (([(3, 3), (9, 3), (14, 26), (8, 30), (3, 22)], 119), ([(14, 3), (19, 3), (20, 14), (16, 22)], 119), ([(24, 3), (28, 3), (27, 12)], 119),
                    ([(36, 3), (40, 3), (37, 12)], O2), ([(46, 3), (51, 3), (48, 18), (44, 10)], O2), ([(55, 3), (60, 3), (60, 24), (54, 30), (51, 20)], O2)):
        p.poly(lock, c)
    for a, b in (((9, 3), (13, 24)), ((19, 3), (17, 16)), ((28, 3), (28, 10)), ((36, 3), (36, 10)), ((45, 3), (47, 16)), ((55, 3), (52, 22))):
        p.line([a, b], K)
    # ---- the shoulders under the jaw
    p.poly([(3, 60), (3, 44), (12, 44), (20, 60)], B3, K)
    p.poly([(60, 60), (60, 44), (51, 44), (43, 60)], B3, K)
    p.line([(4, 50), (10, 47)], B2)
    p.line([(59, 50), (53, 47)], B2)
    # ---- the ears with their rings
    p.mpoly([(6, 27), (11, 25), (13, 44), (10, 46), (6, 39)], B2, K)
    p.mline([(8, 30), (9, 37), (10, 41)], B3)
    p.mline([(10, 45), (10, 48)], TOOTH)
    p.mpx(11, 48, 11)
    # ---- the head: the wide skull, the hairline's deep peak, the heavy jaw
    head = [(12, 26), (12, 13), (15, 8), (20, 5), (27, 5), (31, 9), (32, 9), (36, 5), (43, 5), (48, 8), (51, 13), (51, 26), (50, 38), (48, 47), (44, 56), (41, 60), (22, 60),
            (19, 56), (15, 47), (13, 38)]
    p.poly(head, B2, K)
    # the dark mask round the eyes and down the nose, the hollows of the cheeks, the dark round the mouth
    p.poly([(13, 29), (21, 27), (29, 31), (34, 31), (42, 27), (50, 29), (49, 37), (42, 39), (37, 43), (26, 43), (21, 39), (14, 37)], B3)
    p.poly([(14, 40), (18, 42), (21, 50), (20, 56), (16, 47)], B3)
    p.poly([(49, 40), (45, 42), (42, 50), (43, 56), (47, 47)], B3)
    p.poly([(21, 44), (42, 44), (44, 52), (41, 57), (22, 57), (19, 52)], B3)
    p.poly([(26, 54), (37, 54), (38, 59), (25, 59)], B2)                       # the chin catches the light
    p.poly([(29, 32), (34, 32), (35, 40), (28, 40)], B2)                       # and the bridge of the nose
    # lights: the cheekbones (the brow is one broad plane: lights on it read as horns)
    p.mpoly([(14, 38), (18, 39), (20, 42), (16, 42)], B1)
    p.poly(head, None, K)
    # ---- the furrows of the brow
    p.line([(31, 9), (31, 16)], K)                                            # the peak's crease, fading down the brow
    p.line([(32, 9), (32, 16)], K)
    p.line([(31, 17), (31, 23)], OUT)
    p.line([(32, 24), (32, 30)], OUT)
    p.mline([(25, 23), (28, 27), (29, 31)], K)
    p.mline([(17, 22), (21, 20), (25, 21)], B3)                               # the brow's wrinkles
    p.mline([(18, 25), (24, 26)], OUT)
    # ---- the eyes: blank yellow slits under black ridges, slanted down to the nose
    p.mpoly([(14, 27), (21, 28), (29, 32), (30, 35), (22, 31), (14, 30)], K)
    p.mpoly([(17, 31), (22, 32), (28, 36), (27, 38), (21, 35), (17, 33)], EYE)
    p.mline([(16, 31), (17, 34), (21, 36), (27, 39)], K)
    # ---- the nose and the gritted teeth
    p.mline([(27, 41), (29, 42), (30, 41)], K)
    p.poly([(23, 46), (27, 45), (31, 46), (32, 46), (36, 45), (40, 46), (41, 50), (39, 53), (24, 53), (22, 50)], TOOTH, K)
    p.line([(23, 49), (40, 49)], 13)
    for x in (26, 29, 33, 37):
        p.line([(x, 46), (x, 52)], 11)
    p.mline([(20, 43), (22, 46)], K)
    p.mline([(22, 54), (24, 57)], K)
    p.line([(28, 56), (35, 56)], B3)
    return p.data()


def paragus(rom):
    """Paragus as the Broly movie draws him, three-quarter: black upswept hair with the peak on the brow, heavy brows, his left eye scarred shut,
the big moustache, the white scarf over his armour."""
    global _ROM
    _ROM = rom
    K, OUT = 1, 113
    S1, S2, S3, S4 = 17, 18, 19, 20       # a tan Saiyan's skin: light, base, shade, dark
    W1, W2, W3, W4 = 2, 9, 184, 187       # the scarf: white, its grey, blue-grey folds
    G = 15
    p = Pic(background((106, 65), {92, 225, 91, 233}))
    p.tf = (31, 28, 1.14, 1, 3)
    # ---- the hair: black, swept up and back in broad locks
    hair = [(15, 24), (13, 14), (9, 8), (14, 9), (13, 0), (20, 4), (24, -2), (30, 3), (36, -2), (40, 4), (47, 0), (47, 9), (53, 6), (50, 14), (49, 31), (45, 31)]
    p.poly(hair, K, K)
    for a, b in (((16, 4), (18, 12)), ((24, 2), (25, 11)), ((33, 2), (33, 9)), ((41, 3), (40, 10)), ((48, 8), (46, 15)), ((12, 10), (15, 15))):
        p.line([a, b], G)
    # ---- the armour's shoulder and the scarf
    p.poly([(38, 44), (50, 36), (58, 40), (58, 58), (36, 58)], 211, K)
    p.line([(46, 44), (57, 50)], 40)
    p.line([(46, 45), (57, 51)], 133)
    p.line([(44, 47), (55, 56)], 15)
    p.poly([(18, 30), (44, 26), (47, 40), (40, 48), (24, 48), (19, 40)], S4, OUT)   # the neck in the scarf's shadow
    scarf = [(1, 58), (1, 30), (6, 24), (14, 25), (18, 32), (21, 40), (27, 44), (34, 45), (39, 42), (44, 34), (48, 30), (51, 35), (47, 43), (41, 50),
             (38, 58)]
    p.poly(scarf, W1, K)
    p.poly([(44, 34), (48, 30), (51, 35), (47, 43), (42, 48), (43, 41)], W3)
    p.poly([(20, 46), (28, 49), (36, 48), (41, 50), (38, 58), (24, 58)], W2)
    p.poly([(2, 34), (8, 30), (12, 38), (16, 47), (22, 54), (20, 58), (12, 52), (6, 44)], W3)          # the big fold over his shoulder
    p.poly([(4, 38), (8, 35), (11, 42), (15, 50), (12, 50), (7, 44)], W1)
    for a, b, c in (((6, 26), (10, 36), W4), ((10, 36), (15, 46), W4), ((15, 46), (23, 55), W4), ((2, 46), (8, 56), W4),
                    ((22, 43), (28, 47), W4), ((28, 47), (35, 47), W4), ((35, 47), (40, 44), W4), ((29, 51), (36, 53), W4), ((24, 51), (27, 57), W4)):
        p.line([a, b], c)
    p.poly(scarf, None, K)
    # ---- the ear
    p.poly([(42, 20), (46, 18), (47, 24), (45, 29), (42, 28)], S2, OUT)
    p.line([(44, 21), (44, 26)], S3)
    # ---- the face
    face = [(16, 17), (21, 17), (30, 22), (37, 12), (43, 12), (44, 20), (44, 29), (41, 36), (37, 42), (33, 44), (26, 43), (22, 40), (20, 35), (18, 30),
            (16, 24)]
    p.poly(face, S2, OUT)
    p.poly([(38, 28), (43, 26), (43, 30), (40, 37), (36, 42), (33, 43), (36, 38)], S3)        # the jaw's shade
    p.poly([(17, 25), (19, 31), (21, 36), (23, 40), (21, 33), (19, 26)], S3)
    p.poly([(36, 14), (42, 13), (42, 18), (37, 19)], S1)                                      # the brow's light
    p.poly([(31, 28), (37, 28), (36, 31), (32, 31)], S1)
    p.poly(face, None, OUT)
    # ---- the brows
    p.poly([(16, 20), (19, 21), (25, 24), (25, 26), (20, 24), (16, 22)], K)
    p.poly([(29, 25), (33, 22), (40, 18), (41, 20), (35, 24), (30, 27)], K)
    # ---- his right eye, open: the Saiyan's black pupil; his left, scarred shut
    p.line([(18, 24), (21, 25), (24, 27)], K)
    p.poly([(19, 26), (23, 27), (22, 28), (20, 28)], 2)
    p.line([(21, 26), (21, 28)], K)
    p.line([(20, 29), (23, 29)], OUT)
    p.line([(31, 27), (34, 26), (38, 23)], K)
    p.line([(32, 28), (35, 27), (38, 24)], K)
    p.line([(37, 16), (36, 21), (37, 26), (36, 31)], 27)                      # the scar
    # ---- the nose
    p.line([(27, 25), (26, 29), (25, 31)], OUT)
    p.line([(25, 32), (28, 32), (29, 31)], OUT)
    p.line([(28, 27), (28, 30)], S3)
    # ---- the moustache and the mouth
    p.poly([(21, 33), (24, 32), (27, 33), (30, 33), (34, 32), (38, 34), (39, 37), (36, 36), (33, 36), (29, 37), (25, 36), (23, 38), (22, 41), (20, 38)], K)
    p.line([(27, 39), (32, 39)], OUT)
    p.line([(28, 40), (31, 40)], 242)
    p.line([(27, 42), (33, 42)], S3)
    return p.data()


def main():
    global _ROM
    _ROM = open(sys.argv[1], "rb").read()
    png([a13(_ROM), s13(_ROM), paragus(_ROM)], sys.argv[2], k=6)
    print("wrote", sys.argv[2])


if __name__ == "__main__":
    main()
