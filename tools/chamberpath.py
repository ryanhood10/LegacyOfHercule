"""The Time Chamber open area's way-finding art (docs/time-chamber.md 13, Ryan's first play): our own art drawn by code, no
ROM data, in the family of tools/chamberart.py. draw(NAME) is an RGBA picture painted by mapbuild's
{"op": "png", "draw": "chamberpath", "arg": NAME, "at": [col, row]}; obj(NAME) is an 8bpp decoration frame for an
"objects" part {"draw": "chamberpath", "arg": NAME} (mapbuild converts it to the OBJ palette):

  carpet   the whole 2048 x 2048 map: a red carpet 32 px wide with gold trim from the quarters out to the six entrances'
           doors and up to the gate (SEGMENTS; the trim follows the union's edge, so junctions open into each other)
  wall     2048 x 104 at y 296: a white stone wall across the whole map that the gate's two pillars stand in (they stay
           transparent here: the stamped pillars show), the gate's opening between them a sealed portal (a dark stone
           ring, its swirl dim): nothing walks round the gate any more
  door     32 x 24: the quarters' door home, a dark doorway in a gold frame on the canopy's front (BG1)
  sign     40 x 32: an EXIT sign on two posts beside the door (BG2)
  portal_open  40 x 72: the hidden room's way back, the same ring lit (a picture: the room's door)
  obj portal / portal2  32 x 64 decoration frames: the open portal's swirl (two phases, shown in turn by chamber.c when all
           six lights are out)

usage: chamberpath.py OUT.png      the pictures side by side on the white floor (4x), the carpet's map at 1/4
"""
import math
import sys

W = H = 2048
CARPET = 32
# The carpet's runs, px rects (x1, y1, x2, y2), each 32 px across (docs/time-chamber.md 13.2's map). The entrances' doors are
# 32 px wide at +32 of the 96 x 80 pictures, their bottoms at +80: Namek (416, 560), Frieza (1600, 560), Saiyan (224, 1136),
# pirate (1792, 1136), lab (416, 1744), dojo (1600, 1744). The quarters' door (976-1048, 1224-1256); the gate's way x 1012-1044.
SEGMENTS = [
    (416, 600, 1632, 632),     # H0: the northern crossing under the gate, from the Namek line to the Frieza line
    (416, 560, 448, 1320),     # the west line: Namek's door down to H1
    (1600, 560, 1632, 1320),   # the east line: Frieza's door down to H1
    (224, 1288, 1824, 1320),   # H1: the avenue below the quarters, Saiyan line to pirate line
    (224, 1136, 256, 1320),    # the Saiyan ship's door
    (1792, 1136, 1824, 1320),  # the pirate airship's door
    (984, 1240, 1048, 1288),   # the mat in front of the quarters' door
    (1000, 1320, 1032, 1808),  # the south line: the quarters down to H3
    (416, 1776, 1632, 1808),   # H3: under the southern entrances
    (416, 1744, 448, 1808),    # the lab's door
    (1600, 1744, 1632, 1808),  # the dojo's door
    (864, 984, 896, 1320),     # round the quarters, west
    (1144, 984, 1176, 1320),   # round the quarters, east
    (864, 984, 1176, 1016),    # behind the quarters
    (1012, 400, 1044, 1016),   # the gate's line: from behind the quarters up to the gate
]

RED = [(123, 0, 0), (189, 0, 0), (255, 0, 0)]
GOLD = [(123, 123, 0), (222, 222, 0), (255, 255, 115)]
GREY = [(0, 0, 0), (41, 41, 41), (65, 65, 65), (90, 90, 90), (123, 123, 123), (156, 156, 156),
        (189, 189, 189), (222, 222, 222), (255, 255, 255)]
PURPLE = [(57, 0, 123), (90, 0, 189), (123, 0, 255), (156, 57, 255), (189, 115, 255), (222, 180, 255)]
GREEN = [(24, 65, 0), (0, 123, 0), (0, 156, 0), (0, 189, 0)]
WOOD = [(98, 57, 32), (139, 82, 41), (189, 106, 41)]
CLEAR = (0, 0, 0, 0)

_CARPET = None


def carpet():
    """The whole map's carpet: by its depth inside the union of SEGMENTS (Chebyshev, so an inner corner keeps its trim
    round): 0 dark red, 1-2 gold, 3 dark red, deeper the red."""
    global _CARPET
    if _CARPET is not None:
        return _CARPET
    from PIL import Image, ImageDraw, ImageFilter
    mask = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(mask)
    for x1, y1, x2, y2 in SEGMENTS:
        d.rectangle((x1, y1, x2 - 1, y2 - 1), fill=255)
    levels = [mask]
    for _ in range(4):
        levels.append(levels[-1].filter(ImageFilter.MinFilter(3)))
    cols = [RED[0], GOLD[1], GOLD[1], RED[0], RED[1]]
    im = Image.new("RGBA", (W, H), CLEAR)
    for k in range(5):                       # paint each depth's ring: level k minus level k + 1
        ring = levels[k] if k == 4 else Image.composite(levels[k], Image.new("L", (W, H), 0),
                                                          levels[k + 1].point(lambda v: 255 - v))
        im.paste(cols[k] + (255,), (0, 0), ring)
    _CARPET = im
    return im


def wall():
    """2048 x 104 from y 296: a cap (296-304), the face of white stone blocks (304-392), a shadow row (392-400). The pillars'
    columns (x 984-1008, 1048-1072, from y 320) stay clear; between them, under the lintel, the sealed portal."""
    from PIL import Image
    w, h, top = W, 104, 296
    im = Image.new("RGBA", (w, h), CLEAR)
    px = im.load()
    for y in range(h):
        gy = top + y
        for x in range(w):
            pillar = (984 <= x < 1008 or 1048 <= x < 1072) and gy >= 320
            gap = 1008 <= x < 1048 and gy >= 320
            if pillar or gap:
                continue
            if gy < 298:
                c = GREY[4]
            elif gy < 304:
                c = GREY[8] if gy < 302 else GREY[6]
            elif gy >= 392:
                c = GREY[6] if gy < 396 else GREY[7]
            else:
                row = (gy - 304) // 16
                bx = (x + (16 if row & 1 else 0)) % 32
                by = (gy - 304) % 16
                if by == 15 or bx == 31:
                    c = GREY[5]
                elif by == 0 or bx == 0:
                    c = GREY[8]
                else:
                    c = GREY[7]
            px[x, y] = c + (255,)
    ring = portal_ring(lit=False)
    im.alpha_composite(ring, (1008, 320 - top))
    return im


def portal_ring(lit):
    """40 x 72: the gate's opening, a stone ring (grey, a dark rim) round an oval; sealed, a dark violet with a faint
    swirl; lit, a bright one (the hidden room's way back)."""
    from PIL import Image
    im = Image.new("RGBA", (40, 72), CLEAR)
    px = im.load()
    cx, cy = 19.5, 36.0
    for y in range(72):
        for x in range(40):
            dx, dy = (x - cx) / 19.5, (y - cy) / 36.0
            r = math.hypot(dx, dy)
            if r > 1.0:
                c = GREY[7]                                    # the wall's stone behind the ring's corners
            elif r > 0.86:
                c = GREY[3] if r > 0.95 else GREY[5]
            elif r > 0.80:
                c = GREY[2]
            else:
                a = math.atan2(dy, dx * 0.55)
                arm = (math.sin(3 * a + 9 * r) + 1) / 2
                if lit:
                    v = (1 - r) * 0.8 + arm * 0.35
                    c = PURPLE[min(5, int(v * 6))] if v < 0.95 else (255, 255, 255)
                else:
                    c = PURPLE[0] if arm < 0.6 else PURPLE[1] if arm < 0.85 or r > 0.5 else PURPLE[2]
                    if r < 0.12:
                        c = PURPLE[1]
            px[x, y] = c + (255,)
    return im


def door():
    """32 x 24: a doorway, dark inside, a gold frame and arch."""
    from PIL import Image
    im = Image.new("RGBA", (32, 24), CLEAR)
    px = im.load()
    for y in range(24):
        for x in range(32):
            ax = abs(x - 15.5)
            arch = y < 8 and math.hypot(ax / 15.5, (8 - y) / 8.0) > 1.0
            if arch:
                continue
            inner = ax < 11.5 and (y >= 8 or math.hypot(ax / 11.5, (8 - y) / 5.0) <= 1.0) and y >= 3
            if not inner:
                c = GOLD[1] if ax < 14.5 and y > 0 else GOLD[0]
            else:
                c = GREY[1] if y < 20 else GREY[2]
                if ax < 1 and 6 < y:
                    c = GREY[3]                                # the two leaves' seam
            px[x, y] = c + (255,)
    return im


FONT = {   # 5 x 7
    "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    "X": ["10001", "10001", "01010", "00100", "01010", "10001", "10001"],
    "I": ["11111", "00100", "00100", "00100", "00100", "00100", "11111"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
}


def sign():
    """40 x 32: a green EXIT board (white letters, a dark rim) on two wooden posts."""
    from PIL import Image
    im = Image.new("RGBA", (40, 32), CLEAR)
    px = im.load()
    for y in range(32):
        for x in range(40):
            if y < 15:
                rim = x in (0, 39) or y in (0, 14)
                px[x, y] = (GREY[1] if rim else GREEN[2] if y > 1 else GREEN[3]) + (255,)
            elif x in (6, 7, 32, 33):
                px[x, y] = (WOOD[1] if x in (6, 32) else WOOD[0]) + (255,)
    x0 = 6
    for ch in "EXIT":
        for r, row in enumerate(FONT[ch]):
            for k, b in enumerate(row):
                if b == "1":
                    px[x0 + k, 4 + r] = (255, 255, 255, 255)
        x0 += 7
    for x in range(1, 39):                                     # a ground shadow under the posts
        if 4 <= x <= 9 or 30 <= x <= 35:
            px[x, 31] = GREY[6] + (255,)
    return im


def portal_obj(phase):
    """32 x 64 RGBA: the open portal's swirl, three arms turning (phase 0 / 1 half a step apart), bright at the core,
    transparent outside the oval. A decoration frame (chamber.c shows the two in turn)."""
    from PIL import Image
    im = Image.new("RGBA", (32, 64), CLEAR)
    px = im.load()
    cx, cy = 15.5, 33.0
    ramp = [(49, 16, 172), (90, 49, 255), (115, 74, 255), (156, 131, 255), (180, 164, 255), (172, 255, 255), (255, 255, 255)]
    for y in range(64):
        for x in range(32):
            dx, dy = (x - cx) / 15.5, (y - cy) / 30.0
            r = math.hypot(dx, dy)
            if r > 1.0:
                continue
            a = math.atan2(dy, dx * 0.5)
            arm = (math.sin(3 * a + 10 * r + phase * math.pi / 3) + 1) / 2
            v = (1 - r) * 0.7 + arm * 0.45
            if r > 0.9:
                v = min(v, 0.3)
            px[x, y] = ramp[max(0, min(6, int(v * 6.5)))] + (255,)
    return im


def draw(name="carpet"):
    if name == "carpet":
        return carpet()
    if name == "wall":
        return wall()
    if name == "door":
        return door()
    if name == "sign":
        return sign()
    if name == "portal_open":
        return portal_ring(lit=True)
    raise ValueError(name)


def obj(name):
    """An RGBA decoration frame (mapbuild converts it to 8bpp OBJ tiles in the OBJ palette)."""
    if name == "portal":
        return portal_obj(0)
    if name == "portal2":
        return portal_obj(1)
    raise ValueError(name)


def footprint():
    return {}


if __name__ == "__main__":
    from PIL import Image
    pics = [door(), sign(), portal_ring(False), portal_ring(True), portal_obj(0), portal_obj(1)]
    wd = sum(p.width for p in pics) + 8 * len(pics) + 8
    out = Image.new("RGBA", (wd, 80), (252, 252, 252, 255))
    x = 8
    for p in pics:
        out.alpha_composite(p, (x, 4))
        x += p.width + 8
    out = out.resize((out.width * 4, out.height * 4), Image.NEAREST)
    out.save(sys.argv[1] if len(sys.argv) > 1 else "chamberpath.png")
    small = Image.new("RGBA", (W, H), (252, 252, 252, 255))
    small.alpha_composite(carpet())
    small.alpha_composite(wall(), (0, 296))
    small.resize((512, 512)).save((sys.argv[1] if len(sys.argv) > 1 else "chamberpath.png").replace(".png", "_map.png"))
