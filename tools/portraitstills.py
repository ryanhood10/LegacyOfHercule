"""Dialogue portraits digitized from stills Ryan picked (2026-11-24: "pick portraits from the anime and digitize them so they can show on
gba"): Android 13 (169), Super Android 13 (122), Paragus (120); and Super Saiyan Hercule (174, 2026-11-25, tools/ppportraits.py ssjherc) and Cell Jr. (170, the same day: it was Perfect Cell's 40 repainted). The stills are his files in roms/stills/<name>.png (ignored by git, as the
ROMs are: never commit them); without one, tools/drawnportraits.py's drawing of that character is used, so the build never needs them.

A still is cropped square on the face (CROP, the still's own pixels), shrunk to the 58 x 58 inside LoG2's 3 px frame and mapped
onto the OBJ palette: nearest by lightness and hue, a coloured pixel never onto a grey, a lone speck taken back to its neighbours' colour.

    portrait(rom, name) -> 4096 palette indices, or None without the still
    python -I tools/portraitstills.py roms/log2.gba SHEET.png      the three (x6) for looking at
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402

STILLS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "roms", "stills")
CROP = {"a13": (160, 0, 580, 420), "s13": (30, 25, 410, 405), "paragus": (85, 0, 395, 310), "ssjherc": (415, 35, 595, 215),
        "celljr": (95, 90, 255, 250)}
SAT = {"a13": 1.35, "s13": 1.2, "paragus": 1.2, "ssjherc": 1.2, "celljr": 1.25}
# a cut-out or a still on plain white: the background becomes this colour (Cell Jr.: the red behind the stock Cells)
BACK = {"celljr": (213, 16, 41)}
FRAME_FROM = 28                        # a stock portrait whose grey frame is copied


def lab(c):
    r, g, b = c
    return (0.30 * r + 0.59 * g + 0.11 * b, r - g, (r + g) / 2 - b)


def portrait(rom, name):
    path = os.path.join(STILLS, name + ".png")
    if not os.path.exists(path):
        return None
    from PIL import Image, ImageEnhance
    import drawnportraits as D
    D._ROM = rom
    pal = S.Sprites(rom).pal
    pl = [lab(c) for c in pal]
    chroma = [(p[1] ** 2 + p[2] ** 2) ** 0.5 for p in pl]

    def near(c):
        l = lab(c)
        cs = (l[1] ** 2 + l[2] ** 2) ** 0.5
        return min(range(1, 256), key=lambda i: (pl[i][0] - l[0]) ** 2 * 0.8 + ((pl[i][1] - l[1]) ** 2 + (pl[i][2] - l[2]) ** 2) * 1.5
                   + (4000 if cs > 22 and chroma[i] < 10 else 0))

    im = Image.open(path)
    if name in BACK:
        # a cut-out (transparent) or a still on plain white: the background becomes BACK's colour
        from PIL import ImageDraw
        rgba = im.convert("RGBA")
        im = Image.new("RGB", rgba.size, BACK[name])
        im.paste(rgba, (0, 0), rgba)
        w0, h0 = im.size
        for seed in [(x, y) for x in range(0, w0, 8) for y in (0, h0 - 1)] + [(x, y) for y in range(0, h0, 8) for x in (0, w0 - 1)]:
            if min(im.getpixel(seed)) >= 235:
                ImageDraw.floodfill(im, seed, BACK[name], thresh=40)
    im = im.convert("RGB")
    im = im.crop(CROP[name]).resize((232, 232), Image.LANCZOS)
    im = ImageEnhance.Contrast(im).enhance(1.12)
    im = ImageEnhance.Color(im).enhance(SAT[name])
    im = im.resize((58, 58), Image.LANCZOS)
    im = ImageEnhance.Sharpness(im).enhance(1.8)
    cache, px = {}, im.load()
    g = [[0] * 58 for _ in range(58)]
    for y in range(58):
        for x in range(58):
            c = px[x, y]
            if c not in cache:
                cache[c] = near(c)
            g[y][x] = cache[c]
    for _ in range(1):                 # lone specks
        for y in range(58):
            for x in range(58):
                nb = [g[y + dy][x + dx] for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)) if 0 <= y + dy < 58 and 0 <= x + dx < 58]
                if g[y][x] not in nb:
                    best = max(set(nb), key=nb.count)
                    if nb.count(best) >= 3:
                        g[y][x] = best
    out = list(D.stock(FRAME_FROM))
    for y in range(58):
        for x in range(58):
            out[(y + 3) * 64 + x + 3] = g[y][x]
    return out


if __name__ == "__main__":
    import drawnportraits as D
    rom = open(sys.argv[1], "rb").read()
    D._ROM = rom
    D.png([D.a13(rom), D.s13(rom), D.paragus(rom)], sys.argv[2], k=6)
    print("wrote", sys.argv[2])
