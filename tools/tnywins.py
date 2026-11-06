"""Win celebrations the tournament's bodies lack, drawn by code from the ROM's own frames (docs/tournament.md 28,
docs/sprites.md "Win poses"). Nothing from the ROM is committed: the frames are made at build time into build/gen.

Why groups of their own: a held pose shows *frame 0* of the group it is given. The engine writes the frame index (+0x14)
every frame, but each body's control state recomputes it from its timer (+0x12, which the poses reset) before the draw:
frame 0 (docs/tournament.md 28, found headless: +0x14 always read 0). So a pose that is not a group's first frame needs
a group whose first frame it is: a one-frame array per facing, here.

Android 16 (the player body 175 and the opponent 160 share sprite 8's idle, docs/sprites.md "Android 16 as a player")
has no victory pose: his group 4 is sprite 8's startled look round (Ryan, 2026-11-05: "he didnt celebrate, he just
looked left and right"), his 5 the rocket punch. Drawn here from his idle (sprite 8, group 0, facing down, frame 0): the
right arm (the viewer's left) raised, bent at the elbow, the fist above his head. The frame is 32 x 32 (his OBJ VRAM slot
and frame buffer are 0x400: 32 x 32 at 8 bpp), the body in the middle 16 columns as his 16 x 32 idle stands (x offset -8,
y -24). The same array for all four facings: the pose faces the crowd and keeps it when he turns to the loser.

Gohan (57) and Krillin (161, sprite 76's shout): their cheer is group 4's frame 3 (arms up; frame 0 is the arms down).
Their own descriptors for that frame in each facing, copied (12 bytes each, pointing at the ROM's own frame blobs).

Out (C): `tny_win_a16`, `tny_win_gohan`, `tny_win_krillin` (four descriptor-array pointers each: a group the engine's
pose table points at, tny_engine.c `tny_wins`); every array is its frame and a zero descriptor (the engine's frame count
stops there). `--preview PNG` draws 16's frame x 6.

usage: python tools/tnywins.py roms/log2.gba OUT.c [--preview PNG]
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402
from buu2log2 import compress  # noqa: E402
from a16sprites import tiles  # noqa: E402

A16_SPRITE = 8
W = H = 32
OX = 8                         # the 16-wide idle's column 0 in the 32-wide frame
ATTR_32 = 0x80002000           # attr0 8bpp square, attr1 size 2 (32 x 32)

# Colours (the shared OBJ palette 0x081DA6C8): his armour and skin ramps as his idle draws them
COL = {"#": 0x01, "o": 0x17, "h": 0x72, "f": 0x73, "e": 0x74, "d": 0x75,
       "k": 0x29, "K": 0x2A, "y": 0x2E, "g": 0x36}

# The raised arm over the frame's columns 0-13 (ARM_LOW: the fist at the shoulder, drawn first and kept for a two-frame
# pose if the engine ever plays one), rows 0-19 ("." leaves the pixel as it is). Shoulder pad at columns 9-12
# rows 13-14 (the body's), the upper arm out to the left at rows 12-17, the forearm up from the elbow, the fist on top.
ARM_HIGH = [
    "..............",
    "...oooo.......",
    "..ohhffo......",
    "..ohffeo......",
    "..ofeedo......",
    "..oededo......",
    "...oooo.......",
    "...#Kk#.......",
    "...#kg#.......",
    "...#kgK#......",
    "...#kgK#......",
    "...#kgK#......",
    "...#kgK#......",
    "...#kgK#####..",
    "...#kgkkkkk#..",
    "...#kggggyk#..",
    "...#KKKKKKK#..",
    "....#######...",
]
ARM_LOW = [
    "..............",
    "..............",
    "..............",
    "..............",
    "..............",
    "...oooo.......",
    "..ohhffo......",
    "..ohffeo......",
    "..ofeedo......",
    "..oededo......",
    "...oooo.......",
    "...#Kk#.......",
    "...#kgK#......",
    "...#kgK#####..",
    "...#kgkkkkk#..",
    "...#kggggyk#..",
    "...#KKKKKKK#..",
    "....#######...",
]
SKIN = {0x17, 0x72, 0x73, 0x74, 0x75}


def idle(sp):
    rec = sp.record(A16_SPRITE)
    arr = sp.groups(rec)[0][0]
    xo, yo, w, h, attr, _ = sp.descriptor(arr, 0)
    assert (xo, yo, w, h) == (0, -24, 16, 32) and not attr & 0x10000000, (xo, yo, w, h, hex(attr))
    return sp.frame_pixels(arr, 0)[2]


def frame(body, arm):
    px = [[0] * W for _ in range(H)]
    for y in range(32):
        for x in range(16):
            px[y][OX + x] = body[y][x]
    # the hanging right arm goes: the body's columns 0-3 from row 15 down (and the hand's outline in column 4)
    for y in range(15, 25):
        for x in range(0, 5):
            v = body[y][x]
            if x < 4 or (y >= 22 and v in SKIN):
                px[y][OX + x] = 0
    for y, row in enumerate(arm):
        for x, c in enumerate(row):
            if c != ".":
                px[y][x] = COL[c]
    for y in (13, 14):                         # the shoulder pad stays over the arm
        for x in range(1, 5):
            if body[y][x]:
                px[y][OX + x] = body[y][x]
    return px


# (symbol, stock sprite, group, frame): a stock frame as its own one-frame group, per facing
CHEERS = [("tny_win_gohan", 57, 4, 3), ("tny_win_krillin", 76, 4, 3)]


def cheer(sp, sid, group, fi):
    """The four facings' descriptors of frame `fi` of a stock group (12 bytes each, the ROM's own blob pointers)."""
    arrs = sp.groups(sp.record(sid))[group]
    counts = sp.frame_counts([arrs])
    out = []
    for arr in arrs:
        f = min(fi, counts[arr] - 1)
        xo, yo, w, h, attr, blob = sp.descriptor(arr, f)
        assert sp.valid_descriptor(arr, f), (sid, group, f)
        out.append((xo, yo, w, h, attr, blob))
    return out


def write_c(path, frames, cheers):
    lines = ["/* generated by tools/tnywins.py from the ROM: the win poses the tournament's bodies lack (Android 16's",
             " * raised fist). Build output only; never commit. */", '#include "rom.h"', '#include "tourney.h"', "",
             "typedef struct { signed char xo, yo; u8 w, h; u32 attr; const void *blob; } WinDesc;"]
    ents = []
    for i, px in enumerate(frames):
        data = compress(tiles(px, W, H))
        body = ", ".join(f"0x{b:02x}" for b in data)
        lines.append(f"static const u8 win_a16_{i}[{len(data)}] TNY_ART __attribute__((aligned(4))) = {{{body}}};")
        ents.append(f"{{-8, -24, {W}, {H}, 0x{ATTR_32:08x}u, win_a16_{i}}}")
    ents.append("{0, 0, 0, 0, 0, 0}")
    lines.append(f"static const WinDesc win_a16_d[{len(ents)}] TNY_ART = {{{', '.join(ents)}}};")
    lines.append("const WinDesc *const tny_win_a16[4] TNY_ART = { win_a16_d, win_a16_d, win_a16_d, win_a16_d };")
    for name, descs in cheers.items():
        arrs = []
        for d, (xo, yo, w, h, attr, blob) in enumerate(descs):
            lines.append(f"static const WinDesc {name}_{d}[2] TNY_ART = {{{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, "
                         f"(const void *)0x{blob:08x}u}}, {{0, 0, 0, 0, 0, 0}}}};")
            arrs.append(f"{name}_{d}")
        lines.append(f"const WinDesc *const {name}[4] TNY_ART = {{ {', '.join(arrs)} }};")
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    open(path, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")


def preview(path, sp, frames):
    from PIL import Image
    im = Image.new("RGB", (len(frames) * (W + 8), H + 8), (70, 90, 70))
    for i, px in enumerate(frames):
        for y in range(H):
            for x in range(W):
                if px[y][x]:
                    im.putpixel((4 + i * (W + 8) + x, 4 + y), sp.pal[px[y][x]])
    im.resize((im.width * 6, im.height * 6), Image.NEAREST).save(path)


def main():
    rom = open(sys.argv[1], "rb").read()
    out = sys.argv[2]
    sp = S.Sprites(rom)
    body = idle(sp)
    frames = [frame(body, ARM_HIGH)]
    cheers = {name: cheer(sp, sid, group, fi) for name, sid, group, fi in CHEERS}
    write_c(out, frames, cheers)
    if "--preview" in sys.argv:
        preview(sys.argv[sys.argv.index("--preview") + 1], sp, frames)
    print(f"{out}: Android 16's raised fist; {', '.join(cheers)}")


if __name__ == "__main__":
    main()
