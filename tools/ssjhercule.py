"""Super Saiyan Hercule's transformation art (docs/sprites.md "Super Saiyan Hercule: the transformation"),
built from the ROM at build time; nothing from the ROM is committed. A library: tools/herculesprites.py and
tools/spriteclone.py import it.

The stock transformation (docs/engine.md "Super Saiyan transformation") plays two pose groups that
Hercule's records lacked:
  * group 9 of the *base* record while the transform state (anim 0x23, ability 12; 0x25, ability 11)
    runs: frames 0 and 1 for 16 frames each, then 2 and 3 alternating every 16 frames up to frame 160.
    Goku's (60) is a front-facing flex, black hair on 0 / 1, gold on 2 / 3; teen Gohan's (59) adds a
    yellow outline on 2 / 3. All four facings show the front view. `powerup_frames` builds Hercule's for
    herc_record (sprite 72's record with the beam pose, herculesprites.py): his victory pose (group 11
    frame 1, both fists up), black afro with a dim orange outline, then the gold afro with a yellow
    aura that flickers between a tight and a wide ring.
  * group 21 of the *transformed* record while the detransform state (anim 0x24) runs: frame =
    counter / 16 for 64 frames. Super Saiyan Goku's (61) is his front idle with the hair fading gold ->
    light brown -> brown -> black. `detransform_frames` builds Hercule's for ssj_record (sprite 157,
    spriteclone.py): his front idle with the afro gold -> dark gold -> brown -> black. Record 157 had 21
    groups, so the detransform read past its group table (a null frame's DMA wipes EWRAM).
Frames are 32 x 32 at (-8, -24) with Goku's group 9 attributes, the box the stock groups use.

The HUD icon pair for ability 18 (`icons`): the stock Transformation icon (ability 12's ready / cooling,
32 x 16) with its picture recoloured through ICON_READY / ICON_COOLING (Hercule's gold afro and brown gi).
To swap the design: edit those two tables (palette index -> palette index, shared OBJ palette), or point
ability 18 at another icon in data/hercule.json ("icons": a stock ability id, or two "sym:" names).
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402

HERCULE, GOKU = 72, 60
POWERUP_GROUP, DETRANSFORM_GROUP = 9, 21
POSE = (11, 1)                    # Hercule's victory pose: group 11 frame 1, facing down (both fists up)
IDLE = (0, 0)                     # his front idle
BOX = (-8, -24, 32, 32)           # xo, yo, w, h of every frame (the stock transform / detransform box)
BLACK = 1
HAIR_GOLD, HAIR_LIGHT = 120, 46   # SSJ Goku's hair (spriteclone.recolour)
# detransform fade, frame by frame: (hair, highlight) or None = the stock black afro
FADE = [(HAIR_GOLD, HAIR_LIGHT), (20, HAIR_GOLD), (23, 20), None]
# aura colours (teen Gohan's group 9 outline: 46 yellow, 38 orange, 37 amber, 2 white)
AURA_DIM, AURA_IN, AURA_OUT, AURA_HOT = 38, 46, 37, 2

ABILITY_TABLE = 0x081D9900
TRANSFORM_ABILITY = 12
ICON_PICTURE = (2, 9, 13, 29)     # rows / columns of the icon's picture (inside the black field)
# ready: the pale yellow "Z" -> Hercule's gold afro, its olive edge -> his brown gi
ICON_READY = {44: 17, 45: 18, 46: 120, 8: 46, 36: 120, 42: 111, 139: 19, 133: 20, 128: 19, 13: 112}
ICON_COOLING = {42: 112, 23: 113}


def frame(sp, sid, group, fi, d=0):
    """A stored frame's pixels pasted into the 32 x 32 box (front view, no flip)."""
    arr = sp.groups(sp.record(sid))[group][d]
    xo, yo, w, h, _, _ = sp.descriptor(arr, fi)
    px = sp.frame_pixels(arr, fi)[2]
    bx, by, bw, bh = BOX
    out = [[0] * bw for _ in range(bh)]
    for y in range(h):
        for x in range(w):
            if px[y][x]:
                out[yo - by + y][xo - bx + x] = px[y][x]
    return out


def gold(px, hair=HAIR_GOLD, light=HAIR_LIGHT):
    """spriteclone.recolour (the afro's interior gold, its brown highlights yellow) with other colours."""
    from spriteclone import recolour
    h, w = len(px), len(px[0])
    out = recolour(px, w, h)
    swap = {HAIR_GOLD: hair, HAIR_LIGHT: light}
    return [[swap.get(v, v) if v != px[y][x] else v for x, v in enumerate(row)] for y, row in enumerate(out)]


def ring(px, colour, every=1, phase=0):
    """Transparent pixels next to the body (4-neighbours) take `colour`; every=2 keeps a checkerboard."""
    h, w = len(px), len(px[0])
    out = [r[:] for r in px]
    for y in range(h):
        for x in range(w):
            if px[y][x] or (every > 1 and (x + y + phase) % every):
                continue
            if any(0 <= y + dy < h and 0 <= x + dx < w and px[y + dy][x + dx]
                   for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1))):
                out[y][x] = colour
    return out


def attr(sp):
    return sp.descriptor(sp.groups(sp.record(GOKU))[POWERUP_GROUP][0], 0)[4]


def powerup_frames(sp):
    """Group 9 for herc_record: four (xo, yo, w, h, attr, px) frames, the same for every facing."""
    base = frame(sp, HERCULE, *POSE)
    ssj = gold(base)
    images = [base,
              ring(base, AURA_DIM, 2),
              ring(ssj, AURA_IN),
              ring(ring(ring(ssj, AURA_HOT), AURA_IN), AURA_OUT, 2)]
    a = attr(sp)
    return [(BOX[0], BOX[1], BOX[2], BOX[3], a, img) for img in images]


def detransform_frames(sp):
    """Group 21 for ssj_record: four frames, the afro fading gold -> black, the same for every facing."""
    base = frame(sp, HERCULE, *IDLE)
    a = attr(sp)
    return [(BOX[0], BOX[1], BOX[2], BOX[3], a, gold(base, *f) if f else base) for f in FADE]


def untile(data, w, h):
    tw = w // 8
    return [[data[((y // 8) * tw + x // 8) * 64 + (y % 8) * 8 + x % 8] for x in range(w)] for y in range(h)]


def tiles(px, w, h):
    tw = w // 8
    data = bytearray(w * h)
    for y in range(h):
        for x in range(w):
            data[((y // 8) * tw + x // 8) * 64 + (y % 8) * 8 + x % 8] = px[y][x]
    return bytes(data)


def icons(rom):
    """Ability 18's HUD icons (ready, cooling): 0x200 bytes each, 32 x 16 8bpp tiles."""
    out = []
    r0, r1, c0, c1 = ICON_PICTURE
    for j, remap in enumerate((ICON_READY, ICON_COOLING)):
        a = struct.unpack_from("<I", rom, ABILITY_TABLE + 16 * TRANSFORM_ABILITY + 4 * j - S.ROM_BASE)[0]
        px = untile(rom[a - S.ROM_BASE:a - S.ROM_BASE + 0x200], 32, 16)
        for y in range(r0, r1):
            for x in range(c0, c1):
                px[y][x] = remap.get(px[y][x], px[y][x])
        out.append(tiles(px, 32, 16))
    return out


GUN_PICTURE = [                   # right-facing pistol in the icon's picture area: # steel, + its shadow
    "................",
    ".##############.",
    ".++++++++++++++.",
    ".####.#.........",
    ".###..##........",
    ".###............",
    ".+++............",
]


def gun_icons(rom):
    """Ability 25's HUD icons (ready, cooling), on the Transformation icons' frame: 0x200 bytes each, 32 x 16 8bpp tiles."""
    out = []
    r0, r1, c0, c1 = ICON_PICTURE
    for j, colours in enumerate(({"#": 2, "+": 46}, {"#": 112, "+": 113})):
        a = struct.unpack_from("<I", rom, ABILITY_TABLE + 16 * TRANSFORM_ABILITY + 4 * j - S.ROM_BASE)[0]
        px = untile(rom[a - S.ROM_BASE:a - S.ROM_BASE + 0x200], 32, 16)
        field = px[r0][c0]
        for y in range(r0, r1):
            for x in range(c0, c1):
                px[y][x] = colours.get(GUN_PICTURE[y - r0][x - c0], field)
        out.append(tiles(px, 32, 16))
    return out


def preview(path, rom):
    """Both groups (Goku's / Gohan's for reference above Hercule's) and the icon pair, x4."""
    sp = S.Sprites(rom)
    rows = [[frame(sp, GOKU, POWERUP_GROUP, fi) for fi in range(4)],
            [f[5] for f in powerup_frames(sp)],
            [frame(sp, 61, DETRANSFORM_GROUP, fi) for fi in range(4)],
            [f[5] for f in detransform_frames(sp)]]
    cv = S.Canvas(4 * 36 + 8, len(rows) * 36 + 24, (60, 60, 90))
    for r, imgs in enumerate(rows):
        for i, px in enumerate(imgs):
            for y, line in enumerate(px):
                for x, v in enumerate(line):
                    if v:
                        cv.put(4 + i * 36 + x, 4 + r * 36 + y, sp.pal[v])
    for i, ic in enumerate(icons(rom)):
        px = untile(ic, 32, 16)
        for y in range(16):
            for x in range(32):
                if px[y][x]:
                    cv.put(4 + i * 36 + x, 4 + len(rows) * 36 + y, sp.pal[px[y][x]])
    cv.save(path)
    try:
        from PIL import Image
        im = Image.open(path)
        im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(path.replace(".png", "_x4.png"))
    except ImportError:
        pass


if __name__ == "__main__":
    preview(sys.argv[2], open(sys.argv[1], "rb").read())
