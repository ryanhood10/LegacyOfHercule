"""The tournament's unlock board as map art (our own picture, drawn by code; no ROM data): a wooden notice
board on two posts, a red header strip, pinned papers, 48 x 40 px. Used by data/maps/papaya_courtyard.json
(a `png` op with "draw": "boardart" and an `art_footprint`), docs/tournament.md "Grounds".

usage: boardart.py OUT.png [--x4 PREVIEW.png]
"""
import sys

W, H = 48, 40
OUTLINE = (49, 24, 8)
WOOD = [(99, 49, 16), (140, 74, 33), (181, 107, 57), (214, 148, 90)]
CORK = [(189, 140, 82), (214, 173, 115)]
RED = [(165, 16, 16), (222, 49, 41)]
PAPER = [(255, 255, 255), (222, 222, 214), (123, 123, 140)]
PIN = (49, 82, 214)

# papers on the panel: (x, y, w, h)
PAPERS = [(8, 13, 9, 11), (19, 12, 10, 9), (31, 14, 9, 10), (20, 23, 8, 6), (9, 26, 7, 4), (31, 26, 8, 4)]


def pixel(x, y):
    """The board's colour at (x, y), or None (transparent)."""
    # posts under the board
    if 32 <= y < H:
        for px in (8, 37):
            if px <= x < px + 4:
                return WOOD[1] if x < px + 2 else WOOD[0]
        return None
    if not (2 <= x < W - 2 and 2 <= y < 34):
        return None
    # frame
    if x < 5 or x >= W - 5 or y < 5 or y >= 31:
        if y < 9 and 5 <= x < W - 5:
            pass
        else:
            return WOOD[3] if (y == 2 or x == 2) else WOOD[2] if (y < 4 or x < 4) else WOOD[1]
    # header strip
    if 5 <= y < 10:
        return RED[1] if y < 7 else RED[0]
    for (pxx, pyy, pw, ph) in PAPERS:
        if pxx <= x < pxx + pw and pyy <= y < pyy + ph:
            if (x, y) == (pxx + pw // 2, pyy):
                return PIN
            if y > pyy + 1 and (y - pyy) % 2 == 0 and pxx + 1 <= x < pxx + pw - 1:
                return PAPER[2]
            return PAPER[0] if x < pxx + pw - 1 else PAPER[1]
    return CORK[1] if ((x * 7 + y * 3) % 11) else CORK[0]


def draw():
    """The board as an RGBA PIL image, outlined."""
    from PIL import Image
    px = [[pixel(x, y) for x in range(W)] for y in range(H)]
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    for y in range(H):
        for x in range(W):
            c = px[y][x]
            if c is None:
                continue
            edge = any(not (0 <= x + ox < W and 0 <= y + oy < H) or px[y + oy][x + ox] is None
                       for ox, oy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            im.putpixel((x, y), (OUTLINE if edge else c) + (255,))
    return im


def footprint():
    """Collision in 8-px tiles relative to the picture's top-left: the board's lower half and its posts
    block (the top rows stay open so a player can stand right under it and read it)."""
    out = {}
    for tx in range(W // 8):
        for ty in range(2, H // 8):
            out[(tx, ty)] = 0
    return out


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    im = draw()
    im.save(sys.argv[1])
    if "--x4" in sys.argv:
        from PIL import Image
        bg = Image.new("RGBA", im.size, (230, 230, 220, 255))
        bg.alpha_composite(im)
        bg.resize((W * 4, H * 4), Image.NEAREST).save(sys.argv[sys.argv.index("--x4") + 1])


if __name__ == "__main__":
    main()
