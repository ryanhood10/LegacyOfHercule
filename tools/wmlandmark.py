"""World-map landmarks (docs/maps.md 9e): the 3D-view sprites the world map draws over each landing site.

The world map's variation (zone 6 area 99, 0x085C0814) lists 13 decorations (0x18 bytes each, +0x28) on one
sprite sheet (+0x2C, a compressed resource of 8bpp 32 x 64 frames, 0x40 tiles of 32 bytes per frame); a
decoration's index is the id the on-load's `FindCharacterEntity N` shows (and `FindEntityByType N` hides).
This module draws the new frames from the stock ones (no ROM data in the repo) for tools/mapedit.py's
"add_decorations" ("art": NAME).

Frames on the stock sheet: 0 Goku's house, 1 a rock pillar, 2 a city, 3 Capsule Corp's dome, 4 a snowy
mountain, 5 the Lookout, 6 Kame House, 7 a palm island, 8 the Cell Games arena. Every frame has the
landmark in rows 0-31 (its foot on row 31) and the same purple cone (the site's pin) in rows 44-63.

usage: wmlandmark.py ROM OUT.png [ART]     preview the stock sheet and the new frames (OBJ palette, x4)
"""
import struct
import sys

FRAME = 2048                 # one 32 x 64 8bpp frame
W, H = 32, 64
OBJ_PALETTE = 0x1DA6C8       # file offset of the OBJ palette (docs/maps.md 5)

# OBJ palette indices used by the stock frames (palm island 7, Cell Games arena 8)
SAND, SAND_RIM, GRASS, LEAF, LEAF_DARK, LEAF_SHADE = 19, 20, 58, 49, 50, 167
TRUNK = (33, 34, 35)
WHITE, STONE_HI, STONE, STONE_MID, STONE_LO = 2, 9, 10, 11, 12
ROOF = (24, 25, 26)          # Kame House's roof reds: light, mid, dark


def get(frame, x, y):
    t = (y // 8) * 4 + x // 8
    return frame[t * 64 + (y % 8) * 8 + x % 8]


def put(frame, x, y, v):
    if 0 <= x < W and 0 <= y < H:
        t = (y // 8) * 4 + x // 8
        frame[t * 64 + (y % 8) * 8 + x % 8] = v


def frames(sheet):
    return [bytes(sheet[k * FRAME:(k + 1) * FRAME]) for k in range(len(sheet) // FRAME)]


def papaya(sheet):
    """Papaya Island: the World Tournament's stone ring on a sandy island, the hall's red roof behind it, a palm
    tree on the left (the palm island's own), the stock cone below."""
    st = frames(sheet)
    palm, out = st[7], bytearray(FRAME)
    for y in range(32, H):                      # the pin
        for x in range(W):
            put(out, x, y, get(palm, x, y))
    # the island: an ellipse of sand with a darker rim and a few grass tufts, rows 18-31
    cx, cy, rx, ry = 15.5, 25.0, 16.0, 6.5
    for y in range(18, 32):
        for x in range(W):
            d = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
            if d <= 1.0:
                rim = ((x - cx) / (rx - 1.3)) ** 2 + ((y - cy) / (ry - 1.3)) ** 2 > 1.0
                v = SAND_RIM if rim else SAND
                if not rim and (x * 7 + y * 3) % 11 == 0 and (x < 9 or y > 28):
                    v = GRASS
                put(out, x, y, v)
    # the hall behind the ring: a red pagoda roof (Kame House's reds) over white walls and red pillars, x 12-31
    for y in range(2, 10):
        half = 3 + (y - 2) * 1.35 + (1 if y >= 8 else 0)     # the roof widens downwards, its eaves flare out
        for x in range(10, W):
            if abs(x - 21.5) <= half:
                v = ROOF[0] if y == 2 or (x + 2 * y) % 6 == 0 else ROOF[1]
                put(out, x, y, ROOF[2] if y == 9 else v)
    for y in range(10, 17):
        for x in range(13, 31):
            v = WHITE
            if x in (13, 30) or (x - 13) % 4 == 1:
                v = ROOF[1]                                     # pillars
            elif 19 <= x <= 23 and y >= 12:
                v = STONE_LO                                    # the doorway
            put(out, x, y, v)
    # the palm (frame 7's leaves and trunk, shifted left by 9 and up by 2): beside the hall, behind the ring
    for y in range(4, 25):
        for x in range(W):
            v = get(palm, x, y)
            if v in (LEAF, LEAF_DARK, LEAF_SHADE) + TRUNK and not (y >= 23 and v == LEAF_SHADE):
                put(out, x - 9, y - 2, v)
    # the ring: a flat stone square, top face rows 16-23 with faint tile lines, front face rows 24-26
    x0, x1 = 7, 30
    for y in range(16, 27):
        for x in range(x0, x1 + 1):
            if y <= 23:
                if y == 16 or x in (x0, x1):
                    v = STONE_MID
                elif y == 23:
                    v = WHITE
                else:
                    v = STONE_HI if (x - x0) % 6 == 0 or (y - 16) % 3 == 0 else STONE
            else:
                v = STONE_LO if x in (x0, x1) or y == 26 else STONE_MID
            put(out, x, y, v)
    for y in (24, 25, 26):                                      # the steps up the front
        for x in range(16, 22):
            put(out, x, y, STONE_HI if y != 25 else STONE)
    return bytes(out)


ART = {"papaya": papaya}


def palette(rom):
    out = []
    for i in range(256):
        c = struct.unpack_from("<H", rom, OBJ_PALETTE + 2 * i)[0]
        out.append(((c & 31) * 8, ((c >> 5) & 31) * 8, ((c >> 10) & 31) * 8))
    return out


def preview(rom, sheet, path, scale=4):
    from PIL import Image
    pal, fr = palette(rom), frames(sheet)
    img = Image.new("RGB", (len(fr) * (W + 4), H), (40, 40, 60))
    for k, f in enumerate(fr):
        for y in range(H):
            for x in range(W):
                v = get(f, x, y)
                if v:
                    img.putpixel((k * (W + 4) + x, y), pal[v])
    img.resize((img.width * scale, img.height * scale), Image.NEAREST).save(path)


def stock_sheet(rom):
    import os
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from mapchunks import decompress
    var = 0x5C0814                               # the world map's variation (file offset)
    sh = struct.unpack_from("<I", rom, var + 0x2C)[0] - 0x08000000
    mode, size = struct.unpack_from("<II", rom, sh)
    return bytes(decompress(rom, sh + 8, mode)[:size])


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    rom = open(sys.argv[1], "rb").read()
    sheet = stock_sheet(rom)
    names = sys.argv[3:] or list(ART)
    preview(rom, sheet + b"".join(ART[n](sheet) for n in names), sys.argv[2])
    print(f"{sys.argv[2]}: {len(sheet) // FRAME} stock frames + {', '.join(names)}")


if __name__ == "__main__":
    main()
