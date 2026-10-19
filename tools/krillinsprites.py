"""Krillin as a playable character: his fighting sprite record (id 161 by default), his status-screen
portrait, his recoloured ki blast, the Destructo Disc projectile and its HUD icons, emitted as C
source compiled into the co-op ROM. Nothing from the ROM is committed: everything below is derived
from roms/log2.gba at build time into build/gen.

usage: krillinsprites.py roms/log2.gba build/gen/krillin_sprites_data.c [--preview build/krillin_preview.png]

Sources (docs/sprites.md "Krillin"):
  * Krillin's stock record (sprite 76, the NPC / cutscene set) has idle 0, walk 1, run 2, the
    knock-down 3, the shout 4, the ki pose 5, a punch combo 6 and lying down 12: those groups point
    at the ROM's own descriptor arrays (shared, sprite 76 stays untouched for the cutscenes).
  * Goku's playable record (sprite 60) wears the same turtle gi in the same colours and the same
    16x32 / 32x32 frame boxes (shoulders on row 15 in both idle frames), so it supplies the groups
    a player row needs that 76 lacks: attacks 7 and 8, hurt 11, the flying lean 13, the fidget 14,
    the special melee 16 (the charged A attack, which the debug party unlocks for every slot but
    Hercule's), the halo 17 and the Kamehameha pose 18. Each frame's hair (the largest black component near
    the top) and face are cleared and Krillin's bald head, cut from his idle frame of the same
    facing, is pasted with its chin on Goku's chin row and its face centre on Goku's.
  * Group 20 (tumble) reuses Krillin's own knock-down (group 3): Goku's tumble frames are rotated
    bodies with no head on top to swap. Goku's transformation (9) and Spirit Bomb (19) stay empty.
  * The record copies Goku's header (collision boxes) and his 21-group layout, so every animation
    that addresses a group by index finds it where a player's record has it.
Also emitted:
  * krillin_status_portrait: the ROM's Krillin dialogue portrait (Portrait_Table 73, OBJ palette)
    remapped to the menu BG palette (0x081D4F50) that the status screen's portraits (8..25) use,
    compressed like the stock ones (64x64 linear 8bpp).
  * kr_kiblast[16]: the player ki blast's 4 facings x 4 frames (0x0840E47C) recoloured blue-white.
  * kr_disc[4]: the Destructo Disc, a spinning yellow disc (32x32 raw tiles, drawn procedurally).
  * kr_disc_icon_0/1: HUD icons (ready / cooling, 32x16) built from the Ki Blast icon pair, and
    kr_status_icon_disc / _ki: the status screen's 18x8 technique icons (menu BG palette, compressed).
  * ui_strings: the UI string table (0x080E2840, 260 entries, 20 code literals repointed by
    patch_rom.py) copied with per-slot blocks appended for PARTY_SLOTS characters: the status
    screen's two energy-technique names and descriptions, and the mixed-case / capitals names
    (UI_* indices, mirrored in src/coop/krillin.h). Stock rows point at the ROM's own strings.
  * portrait_module: the portrait module table (0x083EC9D0: word 0 the party, word id + 1 =
    Portrait_Table[id]) copied with Krillin's status portrait appended as id KRILLIN_STATUS_PORTRAIT,
    and Android 16's (his dialogue portrait 26, same remap) as id A16_STATUS_PORTRAIT (party slot 7 in the
    "Gero's computer" chapter, display row 25; src/coop/a16player.c).
"""
import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402
import mapchunks as m  # noqa: E402
from buu2log2 import compress  # noqa: E402

KRILLIN_SPRITE = 76
GOKU_SPRITE = 60
GROUPS_FROM_76 = (0, 1, 2, 3, 4, 5, 6, 12)
GROUPS_FROM_GOKU = (7, 8, 11, 13, 14, 16, 17, 18)
GROUP_ALIASES = {20: 3}             # tumble <- Krillin's own knock-down
GROUP_COUNT = 21
HFLIP = 0x10000000
BLACK = 1
GI = {32, 33, 34, 35, 39}           # orange gi (both sprites)
SKIN = {114, 115, 116, 23, 20, 19}  # skin ramp and outlines
FACE = SKIN | {BLACK, 2, 11, 14}    # what may be cleared in Goku's head band (eyes, whites, mouth)
PORTRAIT_TABLE = 0x083EC9D4
KRILLIN_PORTRAIT = 73
MENU_BG_PALETTE = 0x081D4F50       # 256 BGR555, the status screen's BG palette (raw in the ROM)
KIBLAST_FRAMES = 0x0840E47C         # 4 facings x 4 descriptors {xo, yo, w, h, attr, raw tiles}
ABILITY_TABLE = 0x081D9900
KIBLAST_ABILITY = 2
UI_STRINGS = 0x080E2840
UI_STOCK = 260
PORTRAIT_MODULE = 0x083EC9D0
PORTRAIT_MODULE_COPY = 120          # words copied verbatim (ids 0..118; word 116 is the entity list)
KRILLIN_STATUS_PORTRAIT = 119       # display row +3 of Krillin's row (src/coop/krillin.h)
A16_PORTRAIT = 26                   # Android 16's dialogue portrait (speaker 26)
A16_STATUS_PORTRAIT = 120           # display row +3 of Android 16's player row 25 (tools/spriteclone.py)
SLOTS = 16                          # per-slot string blocks cover PARTY_SLOTS up to 16
# appended UI string blocks (indices from UI_STOCK; mirrored in src/coop/krillin.h)
UI_BLOCKS = ("energy1", "energy2", "desc1", "desc2", "name", "caps")
STOCK_BLOCK = {"energy1": 0xA7, "energy2": 0xB1, "desc1": 0xAC, "desc2": 0xB6, "name": 0x30, "caps": 0x36}
STOCK_SLOTS = {"energy1": 5, "energy2": 5, "desc1": 5, "desc2": 5, "name": 6, "caps": 6}
NEW_STRINGS = {   # slot -> block -> text (ours, not ROM data)
    6: {"energy1": "Destructo Disc",
        "desc1": "Krillin hurls a spinning disc of energy that slices through every enemy in its path. "
                 "It needs a few seconds to recharge between throws.",
        "energy2": "Ki Blast",
        "desc2": "A quick blast of blue energy. Holding down the B Button will keep firing.",
        "name": "Krillin", "caps": "KRILLIN"},
    7: {"name": "Android 16", "caps": "ANDROID 16"},   # slot 7 while 16 plays it (src/coop/a16player.c)
}


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


def untile(data, w, h):
    tw = w // 8
    return [[data[((y // 8) * tw + x // 8) * 64 + (y % 8) * 8 + x % 8] for x in range(w)] for y in range(h)]


def components(px, w, h, colour, limit):
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
    """Krillin's head cut from an idle frame: from the first opaque row to the row before the first
    row with three gi pixels (the shoulders); anchor = centre x of the skin pixels, chin = last row."""

    def __init__(self, px, w, h):
        top = next(y for y in range(h) if any(px[y]))
        neck = next(y for y in range(top + 6, h) if sum(1 for v in px[y] if v in GI) >= 3)
        self.rows = [row[:] for row in px[top:neck]]
        self.h = neck - top
        pts = [x for y in range(top, neck) for x in range(w) if px[y][x] in (114, 115, 116)]
        self.cx = sum(pts) / len(pts)


def hair_of(px, w, h):
    """Goku's hair: the largest black component starting in the top 45 % of the frame."""
    comps = [c for c in components(px, w, h, BLACK, max(10, int(h * 0.6))) if len(c) >= 25]
    comps = [c for c in comps if min(y for y, _ in c) <= h * 0.45]
    if not comps:
        return None
    return max(comps, key=len)


def swap_head(px, w, h, head, chin_off):
    out = [row[:] for row in px]
    hair = hair_of(px, w, h)
    if hair is None:
        return out, False
    top = min(y for y, _ in hair)
    chin = min(h - 1, top + chin_off)
    xs = [x for _, x in hair]
    x0, x1 = max(0, min(xs) - 1), min(w - 1, max(xs) + 1)
    band = [(y, x) for y in range(top, chin + 1) for x in range(x0, x1 + 1)]
    face = [x for y, x in band if px[y][x] in (114, 115, 116)]
    cx = (sum(face) / len(face)) if len(face) >= 6 else (sum(xs) / len(xs))
    for y, x in hair:
        out[y][x] = 0
    for y, x in band:
        if px[y][x] in FACE:
            out[y][x] = 0
    dx = int(round(cx - head.cx))
    y0 = chin - head.h + 1
    for ry, row in enumerate(head.rows):
        for rx, v in enumerate(row):
            ty, tx = y0 + ry, rx + dx
            if v and 0 <= ty < h and 0 <= tx < w:
                out[ty][tx] = v
    # stray hair tufts outside the head (small black blobs left in the band's corners)
    for c in components(out, w, h, BLACK, min(h, chin + 1)):
        if len(c) <= 6 and all(y < y0 for y, _ in c):
            for y, x in c:
                out[y][x] = 0
    return out, True


def nearest(pal, rgb, exclude=(0,)):
    best, bi = None, 0
    for i, c in enumerate(pal):
        if i in exclude:
            continue
        d = sum((a - b) ** 2 for a, b in zip(c, rgb))
        if best is None or d < best:
            best, bi = d, i
    return bi


def bgr555(rom, addr, n=256):
    return [S.Sprites.rgb(struct.unpack_from("<H", rom, addr - S.ROM_BASE + 2 * i)[0]) for i in range(n)]


# ---- projectile art ------------------------------------------------------------------------------
def disc_frame(pal, k):
    """A flat spinning disc seen at a low angle: an ellipse (rx 13, ry 5) with a saw-toothed rim,
    a white-hot centre line and four spokes rotating with the frame (k = 0..3)."""
    white, light, yellow, gold, orange = (nearest(pal, c) for c in
                                          ((255, 255, 255), (255, 255, 160), (255, 230, 40), (240, 180, 0), (230, 110, 0)))
    px = [[0] * 32 for _ in range(32)]
    cx, cy, rx, ry = 15.5, 15.5, 13.0, 5.0
    phase = k * math.pi / 8
    for y in range(32):
        for x in range(32):
            dx, dy = (x + 0.5 - 16) / rx, (y + 0.5 - 16) / ry
            r = math.hypot(dx, dy)
            ang = math.atan2(dy, dx)
            teeth = 0.08 * (1 if math.sin(12 * ang + 2 * phase * 4) > 0 else 0)
            if r > 1.0 + teeth:
                continue
            if r > 0.86:
                c = orange
            elif r > 0.66:
                c = gold
            else:
                c = yellow
            spoke = math.cos(2 * (ang - phase * 4))
            if r < 0.86 and spoke > 0.92:
                c = light
            if abs(dy) < 0.22 and r < 0.6:
                c = white if r < 0.35 else light
            px[y][x] = c
    return px


def recolour_ki(px, pal):
    """Yellow/orange ki blast -> blue-white, by brightness rank onto a blue ramp."""
    ramp_rgb = [(255, 255, 255), (200, 240, 255), (120, 200, 255), (60, 140, 255), (40, 80, 230), (30, 40, 170)]
    ramp = [nearest(pal, c) for c in ramp_rgb]
    used = sorted({v for row in px for v in row if v}, key=lambda i: -sum(pal[i]))
    mp = {i: ramp[min(len(ramp) - 1, k * len(ramp) // max(1, len(used)))] for k, i in enumerate(used)}
    return [[mp.get(v, 0) for v in row] for row in px]


def disc_icons(rom, pal):
    """The Ki Blast icon pair (32x16, 8bpp tiles) with its picture area (rows 2-8, columns 13-28)
    redrawn as a small disc with speed lines; the cooling icon in the stock icons' greys."""
    out = []
    u32 = lambda a: struct.unpack_from("<I", rom, a - S.ROM_BASE)[0]
    for j, colours in enumerate(((46, 2, 32, 38), (42, 14, 113, 113))):
        ic = u32(ABILITY_TABLE + 16 * KIBLAST_ABILITY + 4 * j)
        px = untile(rom[ic - S.ROM_BASE:ic - S.ROM_BASE + 0x200], 32, 16)
        for y in range(2, 9):
            for x in range(13, 29):
                px[y][x] = BLACK
        body, hot, rim, lines = colours
        for y in range(2, 9):
            for x in range(13, 29):
                dx, dy = (x + 0.5 - 23) / 5.5, (y + 0.5 - 5.5) / 2.2
                r = math.hypot(dx, dy)
                if r <= 1.0:
                    px[y][x] = rim if r > 0.75 else (hot if abs(dy) < 0.35 and r < 0.5 else body)
        for y, x0, x1 in ((4, 13, 16), (6, 14, 17), (5, 13, 15)):
            for x in range(x0, x1 + 1):
                px[y][x] = lines
        out.append(tiles(px, 32, 16))
    return out


# ---- main ----------------------------------------------------------------------------------------
def main():
    rom = open(sys.argv[1], "rb").read()
    out_c = sys.argv[2]
    preview_path = sys.argv[sys.argv.index("--preview") + 1] if "--preview" in sys.argv else None
    sp = S.Sprites(rom)
    u32 = sp.u32

    rk, rg = sp.record(KRILLIN_SPRITE), sp.record(GOKU_SPRITE)
    gk, gg = sp.groups(rk), sp.groups(rg)
    ck, cg = sp.frame_counts(gk), sp.frame_counts(gg)
    header = bytearray(rom[rg - S.ROM_BASE:rg - S.ROM_BASE + 0x4C])
    header[2] = GROUP_COUNT

    heads, chins = [], []
    for d in range(4):
        xo, yo, w, h, attr, _ = sp.descriptor(gk[0][d], 0)
        _, _, px = sp.frame_pixels(gk[0][d], 0)
        if attr & HFLIP:
            px = hflip(px)
        heads.append(Head(px, w, h))
        xo, yo, w, h, attr, _ = sp.descriptor(gg[0][d], 0)
        _, _, px = sp.frame_pixels(gg[0][d], 0)
        if attr & HFLIP:
            px = hflip(px)
        hair = hair_of(px, w, h)
        top = min(y for y, _ in hair)
        neck = next(y for y in range(top + 6, h) if sum(1 for v in px[y] if v in GI) >= 3)
        chins.append(neck - 1 - top)

    groups = []
    stats = {"swapped": 0, "frames": 0, "kept": 0}
    for gi in range(GROUP_COUNT):
        if gi in GROUPS_FROM_76:
            assert all(gk[gi]), f"sprite 76 group {gi} missing"
            groups.append(("rom", gk[gi]))
        elif gi in GROUP_ALIASES:
            groups.append(("rom", gk[GROUP_ALIASES[gi]]))
        elif gi in GROUPS_FROM_GOKU:
            dirs, done = [], {}
            for d in range(4):
                arr = gg[gi][d]
                frames = []
                for fi in range(cg[arr]):
                    xo, yo, w, h, attr, blob = sp.descriptor(arr, fi)
                    if blob not in done:
                        _, _, px = sp.frame_pixels(arr, fi)
                        flip = bool(attr & HFLIP)
                        work = hflip(px) if flip else px
                        face = 2 if d == 3 else d
                        new, ok = swap_head(work, w, h, heads[face], chins[face])
                        stats["swapped" if ok else "kept"] += 1
                        if flip:
                            new = hflip(new)
                        done[blob] = new
                        stats["frames"] += 1
                    frames.append((xo, yo, w, h, attr, done[blob]))
                dirs.append(frames)
            groups.append(("new", dirs))
        else:
            groups.append(None)

    pal = sp.pal
    # status portrait: Krillin's dialogue portrait in the menu BG palette
    bg = bgr555(rom, MENU_BG_PALETTE)

    def status_portrait(pid):
        raw = m.unpack(rom, u32(PORTRAIT_TABLE + 4 * pid))
        remap = {0: 0}
        for v in set(raw):
            if v not in remap:
                remap[v] = nearest(bg, pal[v])
        img = bytes(remap[v] for v in raw)
        blob = compress(img)
        assert m.decompress(blob, 8, 1)[:len(img)] == img
        return img, blob
    portrait, portrait_blob = status_portrait(KRILLIN_PORTRAIT)
    _, a16_portrait_blob = status_portrait(A16_PORTRAIT)

    # ki blast recolour (same descriptors, new raw tiles)
    ki = []
    ki_cache = {}
    for i in range(16):
        d = KIBLAST_FRAMES + 12 * i
        xo, yo, w, h = struct.unpack_from("<bbBB", rom, d - S.ROM_BASE)
        attr, rawp = u32(d + 4), u32(d + 8)
        if rawp not in ki_cache:
            px = untile(rom[rawp - S.ROM_BASE:rawp - S.ROM_BASE + w * h], w, h)
            ki_cache[rawp] = recolour_ki(px, pal)
        ki.append((xo, yo, w, h, attr, ki_cache[rawp]))
    disc = [(16, 16, 32, 32, 0x80002000, disc_frame(pal, k)) for k in range(4)]
    icons = disc_icons(rom, pal)
    status_icons = {"kr_status_icon_disc": compress(status_icon(bg, "disc")),
                    "kr_status_icon_ki": compress(status_icon(bg, "ki"))}
    ui = ui_table(rom)
    pmod = [u32(PORTRAIT_MODULE + 4 * i) for i in range(PORTRAIT_MODULE_COPY)]

    write_c(out_c, header, groups, {"kr_kiblast": ki, "kr_disc": disc}, portrait_blob, icons, status_icons, ui, pmod,
            a16_portrait_blob)
    if preview_path:
        write_preview(preview_path, sp, groups, {"kr_kiblast": ki, "kr_disc": disc}, portrait, bg)
    print(f"wrote {out_c}: {stats['frames']} frames from Goku ({stats['swapped']} head swaps, {stats['kept']} kept), "
          f"portrait {len(portrait_blob)} bytes, {len(ki) + len(disc)} projectile frames")


def status_icon(bg, kind):
    """18x8 status-screen technique icon in the menu BG palette: corners 0, black field (1), a small
    yellow disc with speed lines or a blue ki ball."""
    px = [[1] * 18 for _ in range(8)]
    for y, x in ((0, 0), (0, 17), (7, 0), (7, 17)):
        px[y][x] = 0
    if kind == "disc":
        body, hot, rim, line = (nearest(bg, c, (0, 1)) for c in ((255, 230, 40), (255, 255, 230), (230, 120, 0), (200, 200, 120)))
        cx, cy, rx, ry = 11.0, 4.0, 5.5, 2.2
        for x0, x1, y in ((1, 4, 3), (2, 5, 5), (1, 3, 4)):
            for x in range(x0, x1 + 1):
                px[y][x] = line
    else:
        body, hot, rim = (nearest(bg, c, (0, 1)) for c in ((90, 160, 255), (240, 250, 255), (40, 60, 200)))
        cx, cy, rx, ry = 9.0, 4.0, 3.2, 3.2
    for y in range(1, 7):
        for x in range(1, 17):
            r = math.hypot((x + 0.5 - cx) / rx, (y + 0.5 - cy) / ry)
            if r <= 1.0:
                px[y][x] = rim if r > 0.72 else (hot if r < 0.4 else body)
    return bytes(v for row in px for v in row)


def ui_table(rom):
    """Entries of the relocated UI string table: ("rom", address) or ("str", text)."""
    out = [("rom", struct.unpack_from("<I", rom, UI_STRINGS - S.ROM_BASE + 4 * i)[0]) for i in range(UI_STOCK)]
    for b in UI_BLOCKS:
        for slot in range(SLOTS):
            if slot < STOCK_SLOTS[b]:
                out.append(out[STOCK_BLOCK[b] + slot])
            else:
                out.append(("str", NEW_STRINGS.get(slot, {}).get(b, "")))
    return out


def write_c(path, header, groups, proj, portrait_blob, icons, status_icons, ui, pmod, a16_portrait_blob):
    lines = ["/* generated by tools/krillinsprites.py from the ROM: Krillin's playable sprite (krillin_record),",
             " * status portrait, ki blast, Destructo Disc and its icons. Build output only; never commit. */",
             '#include "rom.h"', "",
             "typedef struct { signed char xo, yo; u8 w, h; u32 attr; const void *blob; } KrDesc;", ""]
    blob_names = {}

    def blob_of(px, w, h, raw):
        key = (raw, w, h, tuple(tuple(r) for r in px))
        if key not in blob_names:
            name = f"kr_{'raw' if raw else 'blob'}_{len(blob_names)}"
            data = tiles(px, w, h)
            if not raw:
                data = compress(data)
            blob_names[key] = name
            lines.append(f"static const u8 {name}[{len(data)}] __attribute__((aligned(4))) = {{{', '.join(f'0x{b:02x}' for b in data)}}};")
        return blob_names[key]

    arr_names = {}
    for gi, g in enumerate(groups):
        if not g or g[0] != "new":
            continue
        for d, frames in enumerate(g[1]):
            entries = [f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {blob_of(px, w, h, False)}}}" for xo, yo, w, h, attr, px in frames]
            name = f"kr_g{gi}_d{d}"
            arr_names[(gi, d)] = name
            lines.append(f"static const KrDesc {name}[{len(frames)}] = {{{', '.join(entries)}}};")
    grp = []
    for gi, g in enumerate(groups):
        if not g:
            grp.append("{0, 0, 0, 0}")
        elif g[0] == "rom":
            grp.append("{" + ", ".join(f"(const KrDesc *)0x{a:08x}u" if a else "0" for a in g[1]) + "}")
        else:
            grp.append("{" + ", ".join(arr_names[(gi, d)] for d in range(4)) + "}")
    hdr = ", ".join(f"0x{b:02x}" for b in header)
    lines.append(f"typedef struct {{ u8 header[0x4C]; const KrDesc *groups[{len(groups)}][4]; u32 end; }} KrRecord;")
    lines.append(f"const KrRecord krillin_record = {{{{{hdr}}}, {{{', '.join(grp)}}}, 0}};")
    for name, frames in proj.items():
        entries = [f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {blob_of(px, w, h, True)}}}" for xo, yo, w, h, attr, px in frames]
        lines.append(f"const KrDesc {name}[{len(frames)}] = {{{', '.join(entries)}}};")
    lines.append(f"const u8 krillin_status_portrait[{len(portrait_blob)}] __attribute__((aligned(4))) = "
                 f"{{{', '.join(f'0x{b:02x}' for b in portrait_blob)}}};")
    lines.append(f"const u8 a16_status_portrait[{len(a16_portrait_blob)}] __attribute__((aligned(4))) = "
                 f"{{{', '.join(f'0x{b:02x}' for b in a16_portrait_blob)}}};")
    for i, ic in enumerate(icons):
        lines.append(f"const u8 kr_disc_icon_{i}[0x200] __attribute__((aligned(4))) = {{{', '.join(f'0x{b:02x}' for b in ic)}}};")
    for name, blob in status_icons.items():
        lines.append(f"const u8 {name}[{len(blob)}] __attribute__((aligned(4))) = {{{', '.join(f'0x{b:02x}' for b in blob)}}};")
    strs = {}
    for kind, v in ui:
        if kind == "str" and v not in strs:
            strs[v] = f"kr_str_{len(strs)}"
            esc = v.replace("\\", "\\\\").replace('"', '\\"')
            lines.append(f'static const char {strs[v]}[] = "{esc}";')
    ents = [f"(const char *)0x{v:08x}u" if kind == "rom" else strs[v] for kind, v in ui]
    lines.append(f"const char *const ui_strings[{len(ui)}] __attribute__((aligned(4))) = {{{', '.join(ents)}}};")
    lines.append("extern PartyState g_party_block;")
    pw = ["(u32)&g_party_block"] + [f"0x{v:08x}u" for v in pmod[1:]]
    pw += ["0"] * (KRILLIN_STATUS_PORTRAIT + 1 - len(pw)) + ["(u32)krillin_status_portrait"]
    pw += ["0"] * (A16_STATUS_PORTRAIT + 1 - len(pw)) + ["(u32)a16_status_portrait"]
    lines.append(f"const u32 portrait_module[{len(pw)}] __attribute__((aligned(4))) = {{{', '.join(pw)}}};")
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    open(path, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")


def write_preview(path, sp, groups, proj, portrait, bg):
    """Every group x 4 facings (all frames), the projectile frames, then the status portrait; 3x copy."""
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
    width = max(max(len(r) for r in rows) * cell, 70)
    height = len(rows) * cell + 70
    cv = S.Canvas(width, height, (60, 60, 90))
    for ri, row in enumerate(rows):
        for fi, px in enumerate(row):
            if not px:
                continue
            for y, line in enumerate(px):
                for x, v in enumerate(line):
                    if v:
                        cv.put(fi * cell + 1 + x, ri * cell + 1 + y, sp.pal[v])
    y0 = len(rows) * cell + 3
    for y in range(64):
        for x in range(64):
            cv.put(2 + x, y0 + y, bg[portrait[y * 64 + x]])
    cv.save(path)
    try:
        from PIL import Image
        im = Image.open(path)
        im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(path.replace(".png", "_x3.png"))
    except ImportError:
        pass


if __name__ == "__main__":
    main()
