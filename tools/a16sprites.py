"""Android 16 as a boss: his fighting sprite record (id 160 by default) and the rocket / explosion
projectile frames, emitted as C source compiled into the co-op ROM. Nothing from the ROM is
committed: everything below is derived from roms/log2.gba at build time into build/gen.

usage: a16sprites.py roms/log2.gba build/gen/a16_sprites_data.c [--preview build/a16_preview.png]

Sources (docs/sprites.md "Android 16 boss"):
  * Android 16's own stock record (sprite 8, the cutscene/NPC set the island scenes use) has idle,
    walk, the hit-stun pose, one punch and the knock-down: those groups (0, 1, 2, 6, 12) point at
    the ROM's own descriptor arrays, so the stock sprite 8 stays untouched and is shared, not copied.
  * Android 17's fight set (sprite 9, the island boss) supplies what 16 lacks: the knock-back
    (group 3), the charge pose (5) and two more attacks (7, 8). Each frame is recoloured into 16's
    colours (grey vest and orange scarf -> green armour, blue trousers -> olive, green shoes -> grey
    boots, skin -> 16's skin ramp) and 17's long black hair and face are replaced by 16's head (orange
    mohawk) cut from his own idle frame of the same facing, aligned on the chin and the face centre.
  * Group 13 (new) is the rocket punch: wind-up, then the punch with the forearm gone (a grey socket
    at the elbow) while the fist flies as a projectile.
  * The record keeps 17's group layout (13 groups + the new one), because the boss AI set 21 that
    17 and 18 use addresses groups by index.
Projectiles (raw 8-bit tiles, the format of the ROM's projectile class 0x08013BC0): a small missile
in 8 directions x 2 flame frames (drawn procedurally), the rocket fist in 4 directions, and a fire
explosion recoloured from the ROM's dust-cloud explosion (0x086AF614, 4 frames). The bomb dropped
across the field is the ROM's own (0x086FFBFC, 3 frames), referenced by address from C.

Also emitted: a16p_record (sprite id 175 by default, Makefile A16P_SPRITE), Android 16 as a *player*
for the "Gero's computer" chapter (docs/story.md 4g): a player's 21-group layout with Hercule's (sprite
72) pattern of filled groups, built only from the groups above (nothing new drawn), and the player
header (collision boxes) of Goku's playable record (sprite 60). PLAYER_GROUPS says which group each
player group shows; the ki pose (5, the pose ability animations use) is the rocket punch.
"""
import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402
import mapchunks as m  # noqa: E402
from buu2log2 import compress  # noqa: E402

A16_SPRITE = 8           # Android 16's stock record (idle, walk, hit-stun, punch, knock-down)
A17_SPRITE = 9           # Android 17's fight record (the island boss)
GROUPS_FROM_16 = (0, 1, 2, 6, 12)
GROUPS_FROM_17 = (3, 5, 7, 8)
ROCKET_GROUP = 13
PLAYER_HEADER_SPRITE = 60    # Goku's playable record: the player collision boxes
PLAYER_GROUP_COUNT = 21
# player group -> ("8", group of sprite 8) or ("160", group of the boss record built here); groups not
# listed stay empty, as in Hercule's stock set (9, 10, 14, 15, 16, 18, 19)
PLAYER_GROUPS = {
    0: ("8", 0),       # idle
    1: ("8", 1),       # walk
    2: ("8", 1),       # run (cutscenes): the walk
    3: ("160", 3),     # knock-down (17's knock-back, re-headed)
    4: ("8", 4),       # shout: 16 looking up (also his fidget)
    5: ("160", 13),    # ki pose = the rocket punch (abilities play animation 5)
    6: ("160", 6),     # punch combo (his stock punch)
    7: ("160", 7),     # attacks (17's, re-headed)
    8: ("160", 8),
    11: ("8", 2),      # hurt: the hit-stun
    12: ("8", 12),     # lying down
    13: ("8", 1),      # dash / flight: the walk
    17: ("8", 0),      # halo (never shown for him): idle
    20: ("160", 3),    # tumble: the knock-down
}
DUST_EXPLOSION = 0x086AF614      # 4 raw descriptors {xo, yo, w, h, attr, tiles} (FUN_0800FA1C's)
HFLIP = 0x10000000               # attr1 bit 12 in the descriptor's attr word

BLACK = 1
ARMOUR = {42, 41, 46, 54}        # 16's armour: dark olive, olive, yellow, green
SKIN16 = {117, 115, 114, 116, 23, 19, 20, 18}
HAIR16 = {35, 34, 33, 32}
SKIN17 = {23, 22, 21, 20, 19, 18, 17, 16}
# Android 17 -> Android 16 colours (indices into the shared OBJ palette 0x081DA6C8)
RECOLOUR = {
    15: 42, 14: 42, 13: 41, 12: 41, 11: 54, 10: 46, 9: 46,          # grey vest -> green armour
    215: 42, 80: 42, 79: 41, 78: 41, 214: 54,                        # blue trousers -> olive armour
    35: 42, 34: 41, 33: 46, 32: 46,                                  # orange scarf -> armour collar
    23: 23, 22: 117, 21: 117, 20: 116, 19: 115, 18: 115, 17: 114, 16: 114,   # skin
    58: 15, 57: 13, 50: 15,                                          # green shoes -> grey boots
    203: 42,                                                         # stray eye blue
}


def blank(w, h):
    return [[0] * w for _ in range(h)]


def hflip(px):
    return [row[::-1] for row in px]


def tiles(px, w, h):
    data = bytearray(w * h)
    tw = w // 8
    for ty in range(h // 8):
        for tx in range(tw):
            base = (ty * tw + tx) * 64
            for py in range(8):
                for q in range(8):
                    data[base + py * 8 + q] = px[ty * 8 + py][tx * 8 + q]
    return bytes(data)


def components(px, w, h, colour, limit):
    """4-connected components of `colour` in rows [0, limit)."""
    seen = [[False] * w for _ in range(h)]
    out = []
    for sy in range(limit):
        for sx in range(w):
            if px[sy][sx] != colour or seen[sy][sx]:
                continue
            comp, stack = [], [(sy, sx)]
            seen[sy][sx] = True
            while stack:
                y, x = stack.pop()
                comp.append((y, x))
                for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < limit and 0 <= nx < w and not seen[ny][nx] and px[ny][nx] == colour:
                        seen[ny][nx] = True
                        stack.append((ny, nx))
            out.append(comp)
    return out


class Head:
    """16's head cut from an idle frame: rows from the first opaque row to the row before the first
    row with two armour pixels; anchor = centre x of skin/hair pixels and the chin row."""

    def __init__(self, px, w, h):
        top = next(y for y in range(h) if any(px[y]))
        neck = next(y for y in range(top + 6, h) if sum(1 for v in px[y] if v in ARMOUR) >= 2)
        self.rows = [row[:] for row in px[top:neck]]
        self.h = neck - top
        pts = [x for y in range(top, neck) for x in range(w) if px[y][x] in SKIN16 | HAIR16]
        self.cx = sum(pts) / len(pts)
        self.w = w


def head_metrics17(px, w, h):
    """Android 17's hair blob (largest black component near the top), its top row and the centre
    x of the face (skin pixels in the head band; the hair's centre for back views)."""
    comps = [c for c in components(px, w, h, BLACK, max(10, int(h * 0.6))) if len(c) >= 25]
    comps = [c for c in comps if min(y for y, _ in c) <= h * 0.45]
    if not comps:
        return None
    hair = max(comps, key=len)
    top = min(y for y, _ in hair)
    return hair, top


def swap_head(px, w, h, head, chin17):
    """Remove 17's hair and face, recolour the rest of the body into 16's colours and paste 16's
    head (chin on 17's chin row). Returns (new pixels, swapped?)."""
    out = [row[:] for row in px]
    hm = head_metrics17(px, w, h)
    if hm is None:
        return recolour17(out), False
    hair, top = hm
    chin = top + chin17
    xs = [x for _, x in hair]
    x0, x1 = min(xs), max(xs)
    band = [(y, x) for y in range(top, min(chin + 1, h)) for x in range(x0, x1 + 1)]
    face = [x for y, x in band if px[y][x] in SKIN17 | {203}]
    cx = (sum(face) / len(face)) if len(face) >= 6 else (sum(xs) / len(xs))
    for y, x in hair:
        out[y][x] = 0
    for y, x in band:
        if px[y][x] in SKIN17 | {203, 2, BLACK}:
            out[y][x] = 0
    out = recolour17(out)
    dx = int(round(cx - head.cx))
    y0 = chin - head.h + 1
    for ry, row in enumerate(head.rows):
        for rx, v in enumerate(row):
            ty, tx = y0 + ry, rx + dx
            if v and 0 <= ty < h and 0 <= tx < w:
                out[ty][tx] = v
    return out, True


def recolour17(px):
    return [[RECOLOUR.get(v, v) for v in row] for row in px]


# ---- projectile art ---------------------------------------------------------------------------------
FIST_LEFT = [   # the rocket fist pointing left: knuckles, green forearm, exhaust (16x16)
    "................",
    "................",
    "................",
    "..####..........",
    ".#hhff#.####....",
    "#hffef##kiik#...",
    "#feeff#knnik#yo.",
    "#ffeef#koonk#Woy",
    "#feeff#knnik#yo.",
    "#hffef##kiik#...",
    ".#ffee#.####....",
    "..####..........",
    "................",
    "................",
    "................",
    "................",
]
FIST_COLOURS = {"#": 1, "h": 114, "f": 115, "e": 117, "k": 41, "i": 42, "n": 54, "o": 46, "y": 32, "W": 2}
FLAME_ALT = {"y": 5, "o": 32, "W": 46}   # second flame frame


def parse_art(rows, colours, alt=None):
    px = blank(16, 16)
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch != ".":
                c = colours[ch]
                if alt and ch in alt:
                    c = alt[ch]
                px[y][x] = c
    return px


def rot_cw(px):
    """Rotate a square frame 90 degrees clockwise."""
    n = len(px)
    return [[px[n - 1 - x][y] for x in range(n)] for y in range(n)]


def missile(angle, flame):
    """Small grey missile pointing along `angle` (radians, 0 = right, y down): body 11 px long and 3
    wide, red nose, two fins, a flickering exhaust; outlined in black."""
    ux, uy = math.cos(angle), math.sin(angle)
    px = blank(16, 16)
    body = {}
    for y in range(16):
        for x in range(16):
            rx, ry = x + 0.5 - 8, y + 0.5 - 8
            t = rx * ux + ry * uy          # along the nose direction
            s = -rx * uy + ry * ux         # across
            c = 0
            if -4.5 <= t <= 4.5 and abs(s) <= 1.35 - max(0.0, t - 3.0) * 0.6:
                c = 5 if t > 3.0 else (10 if s < -0.4 else (13 if s < 0.5 else 15))
            elif -4.5 <= t <= -2.0 and abs(s) <= 2.6:
                c = 15                      # fins
            elif -7.5 <= t < -4.5 and abs(s) <= (1.2 if flame else 0.9):
                c = (46 if t > -5.8 else 32) if flame else (2 if t > -5.8 else 46)
            if c:
                body[(y, x)] = c
    for (y, x), c in body.items():
        px[y][x] = c
    for y in range(16):
        for x in range(16):
            if px[y][x] or (y, x) in body:
                continue
            if any((y + dy, x + dx) in body and body[(y + dy, x + dx)] not in (46, 32, 2)
                   for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1))):
                px[y][x] = BLACK
    return px


FIRE = [2, 46, 32, 33, 5, 35]     # white, yellow, orange, dark orange, red, brown-red


def fire_recolour(px, pal):
    """Dust-cloud explosion -> fire: map each colour by brightness onto the fire ramp."""
    used = sorted({v for row in px for v in row if v}, key=lambda i: -sum(pal[i]))
    ramp = {}
    for k, i in enumerate(used):
        ramp[i] = FIRE[min(len(FIRE) - 1, k * len(FIRE) // max(1, len(used)))]
    return [[ramp.get(v, 0) for v in row] for row in px]


# ---- main -------------------------------------------------------------------------------------------
def main():
    rom = open(sys.argv[1], "rb").read()
    out_c = sys.argv[2]
    preview_path = sys.argv[sys.argv.index("--preview") + 1] if "--preview" in sys.argv else None
    sp = S.Sprites(rom)
    u32 = sp.u32

    rec16, rec17 = sp.record(A16_SPRITE), sp.record(A17_SPRITE)
    g16, g17 = sp.groups(rec16), sp.groups(rec17)
    c16, c17 = sp.frame_counts(g16), sp.frame_counts(g17)
    header = bytearray(rom[rec17 - S.ROM_BASE:rec17 - S.ROM_BASE + 0x4C])
    header[2] = ROCKET_GROUP + 1

    # 16's head per facing (down, up, left, right) from his idle group, and 17's chin offset per facing
    heads, chins = [], []
    for d in range(4):
        xo, yo, w, h, attr, _ = sp.descriptor(g16[0][d], 0)
        _, _, px = sp.frame_pixels(g16[0][d], 0)
        if attr & HFLIP:
            px = hflip(px)
        heads.append(Head(px, w, h))
        xo, yo, w, h, attr, _ = sp.descriptor(g17[0][d], 0)
        _, _, px = sp.frame_pixels(g17[0][d], 0)
        hair, top = head_metrics17(px, w, h)
        # chin = last row of the head band: the last row above the shoulders with face/hair pixels
        chin = max(y for y in range(top, top + 16) if any(v in SKIN17 for v in px[y][4:w - 4]) or
                   any((y, x) in set(hair) for x in range(w))) - top
        chins.append(min(chin, 13))

    groups = []              # per group: None, ('rom', [4 addrs]) or ('new', [4 lists of frames])
    stats = {"swapped": 0, "frames": 0}
    for gi in range(ROCKET_GROUP + 1):
        if gi in GROUPS_FROM_16:
            assert all(g16[gi]), f"sprite 8 group {gi} missing"
            groups.append(("rom", g16[gi]))
        elif gi in GROUPS_FROM_17:
            dirs, done = [], {}
            for d in range(4):
                arr = g17[gi][d]
                frames = []
                for fi in range(c17[arr]):
                    xo, yo, w, h, attr, blob = sp.descriptor(arr, fi)
                    if blob not in done:
                        _, _, px = sp.frame_pixels(arr, fi)
                        flip = bool(attr & HFLIP)
                        work = hflip(px) if flip else px          # left-facing art for the right facing
                        face = 2 if d == 3 else d
                        new, ok = swap_head(work, w, h, heads[face], chins[face])
                        stats["swapped"] += ok
                        if flip:
                            new = hflip(new)
                        done[blob] = new
                        stats["frames"] += 1
                    frames.append((xo, yo, w, h, attr, done[blob]))
                dirs.append(frames)
            groups.append(("new", dirs))
        elif gi == ROCKET_GROUP:
            groups.append(("new", rocket_punch(sp, g16, c16)))
        else:
            groups.append(None if not any(g17[gi]) else ("rom", g17[gi]))

    # projectile frames
    proj = {}
    missiles = []
    for k in range(8):                      # 0 = right, then clockwise in 45-degree steps (y down)
        for fl in (0, 1):
            missiles.append(missile(k * math.pi / 4, fl))
    proj["a16_missile"] = [(8, 8, 16, 16, 0x40002000, px) for px in missiles]
    fist = []
    base = [parse_art(FIST_LEFT, FIST_COLOURS), parse_art(FIST_LEFT, FIST_COLOURS, FLAME_ALT)]
    # order: right, down, left, up (matching the missile's 0 / 90 / 180 / 270 degrees) x 2 flames
    for turns in (2, 3, 0, 1):
        for b in base:
            px = b
            for _ in range(turns):
                px = rot_cw(px)
            fist.append(px)
    proj["a16_fist"] = [(8, 8, 16, 16, 0x40002000, px) for px in fist]
    boom = []
    for i in range(4):
        d = DUST_EXPLOSION + 12 * i
        xo, yo, w, h = struct.unpack_from("<bbBB", rom, d - S.ROM_BASE)
        attr, raw = u32(d + 4), u32(d + 8)
        data = rom[raw - S.ROM_BASE:raw - S.ROM_BASE + w * h]
        tw = w // 8
        px = [[data[((y // 8) * tw + x // 8) * 64 + (y % 8) * 8 + x % 8] for x in range(w)] for y in range(h)]
        boom.append((xo, yo, w, h, attr, fire_recolour(px, sp.pal)))
    proj["a16_boom"] = boom

    # the player record: groups of sprite 8 by ROM address, groups of the boss record by reference
    pheader = bytearray(rom[sp.record(PLAYER_HEADER_SPRITE) - S.ROM_BASE:sp.record(PLAYER_HEADER_SPRITE) - S.ROM_BASE + 0x4C])
    pheader[2] = PLAYER_GROUP_COUNT
    pgroups = []
    for gi in range(PLAYER_GROUP_COUNT):
        src = PLAYER_GROUPS.get(gi)
        if not src:
            pgroups.append(None)
        elif src[0] == "8":
            assert all(g16[src[1]]), f"sprite 8 group {src[1]} missing"
            pgroups.append(("rom", g16[src[1]]))
        else:
            assert groups[src[1]], f"boss group {src[1]} missing"
            pgroups.append(("ref", src[1]))

    write_c(out_c, header, groups, proj, pheader, pgroups)
    if preview_path:
        write_preview(preview_path, sp, groups, proj)
        write_preview(preview_path.replace(".png", "_player.png"), sp,
                      [None if not g else groups[g[1]] if g[0] == "ref" else g for g in pgroups], {})
    print(f"wrote {out_c}: {stats['frames']} frames from 17 ({stats['swapped']} head swaps), "
          f"{sum(len(v) for v in proj.values())} projectile frames")


def rocket_punch(sp, g16, c16):
    """Group 13: 16's punch with the forearm detached. Down/up: the wind-up frame, then the same
    frame without the raised fist (socket at the elbow). Left/right: wind-up, then the straight
    punch with the fist and half the forearm cut off and the white swing arc removed."""
    def frame(d, fi):
        xo, yo, w, h, attr, _ = sp.descriptor(g16[6][d], fi)
        _, _, px = sp.frame_pixels(g16[6][d], fi)
        return xo, yo, w, h, attr, [row[:] for row in px]

    def cut(px, x0, x1, y0, y1, socket):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                px[y][x] = 0
        for (y, x), c in socket.items():
            px[y][x] = c
        return px

    down0 = frame(0, 0)
    down_stump = list(down0)
    down_stump[5] = cut([row[:] for row in down0[5]], 0, 6, 13, 17, {(15, 6): 15, (16, 6): 13, (14, 6): 1})
    left0 = frame(2, 0)
    left1 = list(frame(2, 1))
    px = left1[5]
    px = [[0 if v == 2 else v for v in row] for row in px]          # the swing arc
    left1[5] = cut(px, 0, 8, 12, 17, {(13, 8): 1, (14, 8): 15, (15, 8): 13, (16, 8): 1})
    down = [tuple(down0), tuple(down_stump), tuple(down_stump)]
    left = [tuple(left0), tuple(left1), tuple(left1)]
    right = [(xo, yo, w, h, attr | HFLIP, px) for (xo, yo, w, h, attr, px) in left]
    return [down, down, left, right]


def write_c(path, header, groups, proj, pheader=None, pgroups=None):
    lines = ["/* generated by tools/a16sprites.py from the ROM: Android 16's fighting sprite (record",
             " * a16_record) and the rocket / explosion projectile frames. Build output only; never commit. */",
             '#include "rom.h"', "",
             "typedef struct { signed char xo, yo; u8 w, h; u32 attr; const void *blob; } A16Desc;", ""]
    blob_names = {}

    def blob_of(px, w, h, raw):
        key = (raw, w, h, tuple(tuple(r) for r in px))
        if key not in blob_names:
            name = f"a16_{'raw' if raw else 'blob'}_{len(blob_names)}"
            data = tiles(px, w, h)
            if not raw:
                data = compress(data)
            blob_names[key] = name
            body = ", ".join(f"0x{b:02x}" for b in data)
            lines.append(f"static const u8 {name}[{len(data)}] __attribute__((aligned(4))) = {{{body}}};")
        return blob_names[key]

    arr_names = {}
    for gi, g in enumerate(groups):
        if not g or g[0] != "new":
            continue
        for d, frames in enumerate(g[1]):
            entries = []
            for xo, yo, w, h, attr, px in frames:
                entries.append(f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {blob_of(px, w, h, False)}}}")
            name = f"a16_g{gi}_d{d}"
            arr_names[(gi, d)] = name
            lines.append(f"static const A16Desc {name}[{len(frames)}] = {{{', '.join(entries)}}};")
    grp = []
    for gi, g in enumerate(groups):
        if not g:
            grp.append("{0, 0, 0, 0}")
        elif g[0] == "rom":
            grp.append("{" + ", ".join(f"(const A16Desc *)0x{a:08x}u" if a else "0" for a in g[1]) + "}")
        else:
            grp.append("{" + ", ".join(arr_names[(gi, d)] for d in range(4)) + "}")
    hdr = ", ".join(f"0x{b:02x}" for b in header)
    lines.append(f"typedef struct {{ u8 header[0x4C]; const A16Desc *groups[{len(groups)}][4]; u32 end; }} A16Record;")
    lines.append(f"const A16Record a16_record = {{{{{hdr}}}, {{{', '.join(grp)}}}, 0}};")
    if pgroups:   # Android 16 as a player (a16p_record): sprite 8's arrays by address, the boss record's by name
        pg = []
        for g in pgroups:
            if not g:
                pg.append("{0, 0, 0, 0}")
            elif g[0] == "rom":
                pg.append("{" + ", ".join(f"(const A16Desc *)0x{a:08x}u" for a in g[1]) + "}")
            else:
                pg.append(grp[g[1]])
        phdr = ", ".join(f"0x{b:02x}" for b in pheader)
        lines.append(f"typedef struct {{ u8 header[0x4C]; const A16Desc *groups[{len(pgroups)}][4]; u32 end; }} A16PRecord;")
        lines.append(f"const A16PRecord a16p_record = {{{{{phdr}}}, {{{', '.join(pg)}}}, 0}};")
    for name, frames in proj.items():
        entries = [f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {blob_of(px, w, h, True)}}}" for xo, yo, w, h, attr, px in frames]
        lines.append(f"const A16Desc {name}[{len(frames)}] = {{{', '.join(entries)}}};")
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    open(path, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")


def write_preview(path, sp, groups, proj):
    """Every group x 4 facings (all frames), then the projectile frames; 3x."""
    cell = 34
    rows = []
    for gi, g in enumerate(groups):
        if not g:
            continue
        row = []
        for d in range(4):
            if g[0] == "rom":
                arr = g[1][d]
                n = sp.frame_counts([g[1]])[arr]
                for fi in range(n):
                    xo, yo, w, h, attr, _ = sp.descriptor(arr, fi)
                    _, _, px = sp.frame_pixels(arr, fi)
                    row.append(hflip(px) if attr & HFLIP else px)
            else:
                for xo, yo, w, h, attr, px in g[1][d]:
                    row.append(hflip(px) if attr & HFLIP else px)
            row.append(None)
        rows.append(row)
    for frames in proj.values():
        rows.append([f[5] for f in frames])
    width = max(len(r) for r in rows) * cell
    cv = S.Canvas(width, len(rows) * cell, (60, 60, 90))
    for ri, row in enumerate(rows):
        for fi, px in enumerate(row):
            if not px:
                continue
            for y, line in enumerate(px):
                for x, v in enumerate(line):
                    if v:
                        cv.put(fi * cell + 1 + x, ri * cell + 1 + y, sp.pal[v])
    cv.save(path)
    try:  # 3x copy for reading (PIL optional)
        from PIL import Image
        im = Image.open(path)
        im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(path.replace(".png", "_x3.png"))
    except ImportError:
        pass


if __name__ == "__main__":
    main()
