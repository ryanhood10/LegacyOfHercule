"""The injured fighter in the first bed of Papaya Island's infirmary (our own picture, drawn by code; no ROM data):
a man lying under the blanket, his head on the pillow with a bandage round it and closed eyes, an arm in a white
cast over the blanket, 48 x 56 px, drawn over the bed on BG0 (data/maps/papaya_infirmary.json, a `png` op with
"draw": "patientart"; the bed's own collision blocks him). Ryan, 2026-10-25: "a random npc man laying down in one
of the beds" (a fan's husband entered the Rookie division and is in the infirmary).

usage: patientart.py OUT.png [--x4 PREVIEW.png]
"""
import sys

W, H = 48, 56
OUTLINE = (49, 33, 24)
HAIR = [(82, 49, 24), (123, 74, 41)]
SKIN = [(206, 140, 99), (239, 181, 140), (255, 214, 173)]
BANDAGE = [(206, 206, 214), (247, 247, 255)]
BLANKET = [(140, 140, 148), (173, 173, 181), (206, 206, 214)]
BRUISE = (148, 99, 156)

# the bed in the map (x 41-82, the pillow at x 51-67, y 56-65, the blanket from y 66) relative to the art at (40, 48)
HEAD = (19, 14, 7.0, 6.5)          # centre x, y, radii
BODY = (21, 34, 15.0, 9.0)         # the hump under the blanket


def ell(x, y, e):
    cx, cy, rx, ry = e
    return ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2


def pixel(x, y):
    # the arm in a cast lying over the blanket (from the shoulder, down and across)
    if 24 <= y <= 27 and 27 <= x <= 40:
        if y in (24, 27) or x == 40:
            return OUTLINE
        return BANDAGE[1] if x < 37 else SKIN[1]
    # the hump of his body under the blanket
    d = ell(x, y, BODY)
    if d <= 1.0 and y >= 24:
        if d > 0.86:
            return BLANKET[0]
        return BLANKET[2] if y < 31 else BLANKET[1]
    # the head on the pillow
    d = ell(x, y, HEAD)
    if d <= 1.0:
        if d > 0.8:
            return OUTLINE
        dy = y - HEAD[1]
        if dy < -2.5:
            return HAIR[1] if (x + y) % 3 else HAIR[0]
        if -2.5 <= dy < -0.5:                      # the bandage round the forehead
            return BANDAGE[1] if x % 4 else BANDAGE[0]
        if dy in (1, 1.5) or (abs(dy - 1) < 0.6 and x in (16, 17, 21, 22)):
            return OUTLINE if x in (16, 17, 21, 22) else SKIN[1]   # closed eyes
        if x == 23 and dy > 2:
            return BRUISE
        return SKIN[2] if x < HEAD[0] else SKIN[1]
    return None


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
    return {}


def main():
    im = draw()
    im.save(sys.argv[1])
    if "--x4" in sys.argv:
        im.resize((W * 4, H * 4)).save(sys.argv[sys.argv.index("--x4") + 1])


if __name__ == "__main__":
    main()
