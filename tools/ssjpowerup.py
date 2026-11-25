"""Super Saiyan Hercule's power-up, longer and with a drawn aura (2026-11-25, Ryan: "fill in the gap with pixellab and wire it up").

Hercule's own stock frames carry the motion (he braces and shakes, yells, a white burst, the afro turns gold, fists up in flames,
then stands in the aura); the flames and the burst come from two sheets Ryan's PixelLab account drew from his sprite:
roms/ssjh/aura.png and roms/ssjh/burst.png (ignored by git, as the ROMs and roms/stills are: never commit them). Each is a 4 x 4
grid of 64 x 64 cells with Hercule at twice the game's size; PixelLab's bodies are not used (halved, they are smaller and softer
than the game's). Without the sheets tools/ssjhercule.py's four code-drawn frames play by the stock rule, so the build never
needs them.

A cell is halved to the stock 32 x 32 box (2 x 2 -> 1: empty when fewer than two are solid, the outline's black when two are dark,
else the commonest colour), mapped onto the OBJ palette as tools/portraitstills.py does, and only its bright yellow (the burst:
white too) is kept, behind the stock body.

    powerup(sp) -> (frames, seq) or None without the sheets: 32 x 32 index rows, and the frame shown at each 4-frame tick of
                   the 160-frame transform (TICKS entries; src/coop/ssj.c ssj_transform_update plays it)
    STOCK_SEQ   -> the same table for the stock rule (the four code-drawn frames)
    python -I tools/ssjpowerup.py roms/log2.gba OUT.gif        the transform as played (x4), and OUT_strip.png
"""
import os
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402

DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "roms", "ssjh")
AURA, BURST = os.path.join(DIR, "aura.png"), os.path.join(DIR, "burst.png")
CELL, BOX = 64, 32
TICKS = 40                             # 160 frames / 4
STOCK_SEQ = [0] * 4 + [1] * 4 + [2, 2, 2, 2, 3, 3, 3, 3] * 4      # the stock rule: 0, 1 (16 frames each), then 2 / 3 every 16
HERCULE = 72
BRACE, YELL = (2, 2), ((4, 2), (4, 3))  # his stock frames (group, frame), front view


def lab(c):
    r, g, b = c[:3]
    return (0.30 * r + 0.59 * g + 0.11 * b, r - g, (r + g) / 2 - b)


def lum(c):
    return 0.30 * c[0] + 0.59 * c[1] + 0.11 * c[2]


def halve(cell):
    p = cell.load()
    out = [[None] * BOX for _ in range(BOX)]
    for y in range(BOX):
        for x in range(BOX):
            q = [p[2 * x + dx, 2 * y + dy] for dy in (0, 1) for dx in (0, 1)]
            solid = [c for c in q if c[3] >= 128]
            if len(solid) < 2:
                continue
            dark = [c for c in solid if lum(c) < 40]
            if len(dark) >= 2:
                out[y][x] = (0, 0, 0)
            else:
                out[y][x] = Counter(c[:3] for c in solid if lum(c) >= 40).most_common(1)[0][0]
    return out


def cells(path):
    from PIL import Image
    im = Image.open(path).convert("RGBA")
    cs = [im.crop((c * CELL, r * CELL, c * CELL + CELL, r * CELL + CELL))
          for r in range(im.height // CELL) for c in range(im.width // CELL)]
    # the feet onto the cell's bottom edge (the sheet leaves a margin under them, the same in every cell)
    drop = CELL - max(c.getbbox()[3] for c in cs if c.getbbox())
    if drop:
        cs = [Image.new("RGBA", (CELL, CELL), (0, 0, 0, 0)) for _ in cs] and [_shift(c, drop) for c in cs]
    return cs


def _shift(cell, dy):
    from PIL import Image
    out = Image.new("RGBA", (CELL, CELL), (0, 0, 0, 0))
    out.paste(cell, (0, dy))
    return out


def frames(sp, path):
    """A sheet's cells as 32 x 32 index rows."""
    pal = sp.pal
    pl = [lab(c) for c in pal]
    chroma = [(p[1] ** 2 + p[2] ** 2) ** 0.5 for p in pl]
    cache = {(0, 0, 0): 1}

    def near(c):
        if c not in cache:
            l = lab(c)
            cs = (l[1] ** 2 + l[2] ** 2) ** 0.5
            cache[c] = min(range(1, 256), key=lambda i: (pl[i][0] - l[0]) ** 2 * 0.8 + ((pl[i][1] - l[1]) ** 2 + (pl[i][2] - l[2]) ** 2) * 1.5
                           + (4000 if cs > 22 and chroma[i] < 10 else 0))
        return cache[c]

    return [[[near(v) if v else 0 for v in row] for row in halve(c)] for c in cells(path)]


def aura(sp, f, white=False):
    """The aura's pixels of a halved frame: bright yellow (and white, for the burst)."""
    out = [[0] * BOX for _ in range(BOX)]
    for y in range(BOX):
        for x in range(BOX):
            if f[y][x]:
                r, g, b = sp.pal[f[y][x]]
                if (g >= 215 and r >= 215 and b < 190) or (white and min(r, g, b) >= 190):
                    out[y][x] = f[y][x]
    return out


def over(back, body, dy=0):
    out = [row[:] for row in back]
    for y in range(BOX):
        for x in range(BOX):
            if 0 <= y - dy < BOX and body[y - dy][x]:
                out[y][x] = body[y - dy][x]
    return out


def powerup(sp):
    if not (os.path.exists(AURA) and os.path.exists(BURST)):
        return None
    import ssjhercule as H
    a, b = frames(sp, AURA), frames(sp, BURST)
    none = [[0] * BOX for _ in range(BOX)]
    idle, vic = H.frame(sp, HERCULE, *H.IDLE), H.frame(sp, HERCULE, *H.POSE)
    gidle, gvic = H.gold(idle), H.gold(vic)
    brace = H.frame(sp, HERCULE, *BRACE)
    yell0, yell1 = (H.frame(sp, HERCULE, *q) for q in YELL)
    burst = aura(sp, b[7], True)
    small, mid, big, ring, full = aura(sp, a[9]), aura(sp, a[10]), aura(sp, a[11]), aura(sp, a[12]), aura(sp, b[15])
    play = [(idle, 2), (brace, 3), (over(none, brace, 1), 2), (brace, 2), (over(none, brace, 1), 2),
            (yell0, 3), (yell1, 3), (over(burst, yell1), 3), (over(burst, H.gold(yell1)), 2),
            (over(small, gvic), 2), (over(mid, gvic), 2), (over(big, gvic), 2), (over(mid, gvic), 2), (over(big, gvic), 2),
            (over(full, gidle), 3), (over(ring, gidle), 3), (over(full, gidle), 2)]
    out, seq = [], []
    for f, t in play:
        if f not in out:
            out.append(f)
        seq += [out.index(f)] * t
    assert len(seq) == TICKS, len(seq)
    return out, seq


if __name__ == "__main__":
    from PIL import Image
    sp = S.Sprites(open(sys.argv[1], "rb").read())
    got = powerup(sp)
    if not got:
        sys.exit("no roms/ssjh/aura.png and burst.png: the code-drawn frames are used")
    fr, seq = got
    ims = []
    for f in fr:
        im = Image.new("RGB", (BOX, BOX), (72, 120, 72))
        for y in range(BOX):
            for x in range(BOX):
                if f[y][x]:
                    im.putpixel((x, y), tuple(sp.pal[f[y][x]]))
        ims.append(im.resize((BOX * 4, BOX * 4), Image.NEAREST))
    play = [ims[i] for i in seq]
    play[0].save(sys.argv[2], save_all=True, append_images=play[1:], duration=67, loop=0)
    W = Image.new("RGB", (len(ims) * 132, 132), (40, 40, 60))
    for i, im in enumerate(ims):
        W.paste(im, (i * 132 + 2, 2))
    W.save(os.path.splitext(sys.argv[2])[0] + "_strip.png")
    print(len(fr), "frames", seq)
