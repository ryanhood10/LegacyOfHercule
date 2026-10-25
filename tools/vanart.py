"""The ZZTV news van on Papaya Island's street (our own picture, drawn by code; no ROM data): a white van seen from
the front-left in the game's 3/4 view, a blue stripe with "ZZTV", a dark windshield, black wheels and a satellite
dish on the roof, 64 x 48 px. Used by data/maps/papaya_street.json (a `png` op with "draw": "vanart" and an
`art_footprint`), Ryan 2026-10-25: "their van outside there too" (the reporter and his cameraman stand beside it).

usage: vanart.py OUT.png [--x4 PREVIEW.png]
"""
import sys

W, H = 64, 48
OUTLINE = (24, 24, 40)
WHITE = [(173, 181, 198), (214, 222, 231), (247, 247, 255)]
BLUE = [(16, 41, 140), (41, 82, 206)]
GLASS = [(33, 49, 82), (74, 107, 156)]
TYRE = [(16, 16, 24), (66, 66, 74)]
GREY = [(107, 115, 132), (165, 173, 189)]
RED = (206, 33, 33)

# a 3 x 5 pixel font for the logo
FONT = {
    "Z": ["###", "..#", ".#.", "#..", "###"],
    "T": ["###", ".#.", ".#.", ".#.", ".#."],
    "V": ["#.#", "#.#", "#.#", "#.#", ".#."],
}

BODY = (4, 14, 60, 40)        # the box: x0, y0, x1, y1 (the roof's top edge is y0)
ROOF_DEPTH = 6                # the roof seen from above (3/4 view)


def logo_pixels():
    out = set()
    x = 22
    for ch in "ZZTV":
        for ry, row in enumerate(FONT[ch]):
            for rx, c in enumerate(row):
                if c == "#":
                    out.add((x + rx, 27 + ry))
        x += 4
    return out


LOGO = logo_pixels()


def pixel(x, y):
    """The van's colour at (x, y), or None (transparent)."""
    x0, y0, x1, y1 = BODY
    # satellite dish on the roof: a mast and a tilted dish
    if 22 <= x <= 23 and 6 <= y <= y0:
        return GREY[0]
    if 3 <= y <= 8 and 16 <= x <= 29:
        dx, dy = (x - 22.5) / 7.0, (y - 5.5) / 2.6
        if dx * dx + dy * dy <= 1.0:
            return OUTLINE if dx * dx + dy * dy > 0.7 else GREY[1] if dy < 0 else GREY[0]
    # wheels below the body
    for wx in (12, 46):
        if y1 - 2 <= y <= y1 + 5 and wx <= x <= wx + 7:
            edge = x in (wx, wx + 7) or y in (y1 - 2, y1 + 5)
            return OUTLINE if edge else TYRE[1] if (x - wx in (3, 4) and y - y1 in (1, 2)) else TYRE[0]
    if not (x0 <= x <= x1 and y0 <= y <= y1):
        return None
    # the cab's sloped front: the top-left corner cut away diagonally
    slope = (y0 + 14) - y - (x - x0)
    if slope > 0:
        return None
    if slope == 0 or x in (x0, x1) or y in (y0, y1):
        return OUTLINE
    # bumper and headlight at the front's foot
    if y >= y1 - 3 and x <= x0 + 10:
        return GREY[0] if y == y1 - 1 else GREY[1]
    if x0 + 1 <= x <= x0 + 3 and y1 - 7 <= y <= y1 - 5:
        return (255, 239, 132)
    # the roof (top face), lit, behind the cab
    if y < y0 + ROOF_DEPTH and x > x0 + 24:
        return WHITE[2] if y > y0 + 1 else WHITE[1]
    if y == y0 + ROOF_DEPTH and x > x0 + 24:
        return WHITE[0]
    # windshield along the sloped cab, a side window behind it
    if 0 > slope >= -5 and y < 25:
        return GLASS[1] if slope > -2 else GLASS[0]
    if x0 + 14 <= x <= x0 + 22 and y0 + 3 <= y <= y0 + 9:
        return GLASS[0] if y > y0 + 5 else GLASS[1]
    # the blue stripe with the logo
    if 25 <= y <= 33:
        if (x, y) in LOGO:
            return WHITE[2]
        return BLUE[1] if y < 30 else BLUE[0]
    # a red light at the back corner, a door seam
    if x == x1 - 3 and y0 + ROOF_DEPTH + 2 <= y <= y0 + ROOF_DEPTH + 4:
        return RED
    if x == 38 and y > y0 + ROOF_DEPTH:
        return WHITE[0]
    return WHITE[1] if y < y1 - 3 else WHITE[0]


def draw():
    from PIL import Image
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    for y in range(H):
        for x in range(W):
            c = pixel(x, y)
            if c:
                im.putpixel((x, y), c + (255,))
    return im


def footprint():
    """{(tx, ty): 0} blocked tiles: the body and wheels (the dish on top stays walkable behind)."""
    out = {}
    for ty in range(2, H // 8):
        for tx in range(W // 8):
            out[(tx, ty)] = 0
    return out


def main():
    im = draw()
    im.save(sys.argv[1])
    if "--x4" in sys.argv:
        im.resize((W * 4, H * 4)).save(sys.argv[sys.argv.index("--x4") + 1])


if __name__ == "__main__":
    main()
