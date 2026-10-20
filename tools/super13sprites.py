"""Super Android 13 (the post-game boss, docs/story.md 4i): his sprite record (id 180 by default), his
ki-blast projectile frames and his dialogue portrait, emitted as C source compiled into the co-op ROM.
Nothing from either ROM is committed: everything is derived at build time into build/gen.

usage: super13sprites.py roms/log2.gba build/gen/super13_sprites_data.c --buu roms/buu.gba|none
           [--preview-dir build/<dir>]

The body (docs/sprites.md "Super Android 13"): Legendary Super Saiyan Broly from Buu's Fury (Buu record
28), a 64 px tall muscle giant twice a normal LoG2 fighter's height, converted to LoG2's record format
with tools/buu2log2.py's converter and recoloured per body part, per frame, from geometry:
  * skin -> Super 13's blue skin ramp;
  * yellow components above the skin's centroid (the hair) -> white hair; below it (the trousers)
    -> red-brown trousers;
  * the largest red component (the sash) -> a charcoal belt; red below the trousers (boot soles) -> black;
  * silver and gem pixels (necklace, arm cuffs) above the trousers -> skin (bare chest and forearms);
    below them (the boots) -> black boots;
  * everything else (white light, the orange fire of the power-up, the mouth) keeps its nearest colour.
Groups (LoG2 layout = Hercule's 21-group template, as buu2log2's Broly 159, so the melee boss AI set 21
finds 0-2 and 5-8) plus the poses super13.c plays itself (S13_G_* in src/coop/super13.h):
  2 hit-stun, 3 knock-back, 11 hurt <- Broly's hurt (Buu 9); 12 the death: light bursting out of him
  (Buu 17, 8 frames); 21 ki thrusts (Buu 3); 22 the overhead ball (Buu 18 hunch, arms up, Buu 3 throw);
  23 the crouching power-up with fire at his feet (Buu 20); 24 the roar (Buu 21); 25 the spinning shell
  (Buu 19); 26 an empty 8x8 frame (shown once he has exploded).
Without roms/buu.gba (BUU=0) the record aliases Perfect Cell's stock groups (sprite 30) so the build and
the AI still work (a stand-in, not his look).

Projectiles (raw 8-bit tiles, the ROM projectile class's format, as a16sprites.py): red-pink ki orbs drawn
procedurally, 16x16 (s13_orb_s, 3 flicker frames), 32x32 (s13_orb_m, 2), 64x64 (s13_orb_l, 2, the big
overhead ball) and the 16x16 beam trail (s13_trail, 2).

Portrait (s13_portrait, portrait id 122 through krillinsprites.py --extra-portrait): Android 16's
dialogue portrait (Portrait_Table 26) with blue skin, white hair, bare blue shoulders where the armour
was and red eyes.
"""
import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402
import mapchunks as m  # noqa: E402
import buu2log2 as BL  # noqa: E402
from buu2log2 import compress  # noqa: E402

BUU_BROLY = 28
TEMPLATE = 72                 # Hercule's 21-group layout (the converter's default, as Broly 159)
FALLBACK = 30                 # Perfect Cell, the stand-in without the Buu ROM
PORTRAIT_TABLE = 0x083EC9D4
A16_PORTRAIT = 26
GROUP_COUNT = 27

# LoG2 group -> list of (Buu animation, frame); one list for all four facings. Groups not listed keep
# buu2log2's map (cycled to the template's frame counts). "front" = the Buu animation draws the same
# frame for every facing.
GROUPS = {
    0: [(0, 0), (0, 1)],
    1: [(1, 0), (1, 1), (1, 2), (1, 3)],
    2: [(9, 0), (9, 1), (9, 2), (9, 1)],
    3: [(9, 0), (9, 1), (9, 2), (9, 2)],
    11: [(9, 1), (9, 2)],
    12: [(17, f) for f in range(8)],
    21: [(3, 0), (3, 1), (3, 2)],
    22: [(18, 0), (18, 1), (3, 1)],
    23: [(20, f) for f in range(6)],
    24: [(21, f) for f in range(6)],
    25: [(19, f) for f in range(8)],
}
BLANK_GROUP = 26
# {x1, y1, x2, y2} around his position (Entity_GetBox 0x080079B8 adds the position): his body is drawn from x
# (32 px frames) or x - 16 (64 px frames), y - 48 to y + 16, centred on x + 16
HURT_BOX = (-6, -46, 38, 16)
# fallback (Perfect Cell) group for each of ours
FALLBACK_GROUPS = {4: 5, 21: 5, 22: 5, 23: 5, 24: 2, 25: 5, 11: 2, 13: 0, 14: 0, 17: 0, 20: 3}

# Buu palette indices (after Broly's own remap table)
B_SKIN = {200, 201, 202, 203, 205, 16, 18, 66, 67}
B_WARM = {28, 110, 5, 29, 30, 2, 4, 109}          # hair and trousers (yellow, lit white, orange shading)
B_YELLOW = {28, 110, 5, 29, 30}
B_RED = {104, 105, 31, 32, 33, 34}
B_SILVER = {117, 118, 119, 120, 87, 97, 164, 178, 162, 183, 132}

ARC = {210: 10, 211: 9}        # Broly's brown swing-arc colours -> light grey (the arcs read white)
PAL = None


def near(rgb, avoid=(0,)):
    best, bi = None, 0
    for i, c in enumerate(PAL):
        if i in avoid:
            continue
        d = sum((a - b) ** 2 for a, b in zip(c, rgb))
        if best is None or d < best:
            best, bi = d, i
    return bi


def ramps():
    """Buu index -> LoG2 index per body part (LoG2 OBJ palette 0x081DA6C8 indices)."""
    skin = {67: 184, 66: 186, 200: 186, 201: 189, 203: 187, 202: 76, 18: 187, 205: 193, 16: 192}
    hair = {30: 2, 29: 2, 2: 2, 28: 9, 5: 9, 110: 10, 109: 11, 4: 12}
    pants = {30: 117, 29: 117, 2: 117, 28: 110, 5: 110, 110: 111, 109: 112, 4: 112}
    belt = {105: 14, 104: 15, 34: 14, 31: 1, 32: 15, 33: 15}
    sole = {k: 1 for k in B_RED}
    cuff = {28: 189, 5: 189, 110: 187, 109: 187, 4: 193, 30: 186, 29: 186, 2: 184, 120: 184, 119: 186, 118: 189, 117: 187, 87: 187, 97: 187, 164: 187, 178: 187, 162: 187, 183: 187, 132: 187}
    boot = {120: 12, 119: 13, 118: 14, 117: 15, 87: 15, 97: 15, 164: 15, 178: 15, 162: 15, 183: 15, 132: 15}
    return skin, hair, pants, belt, sole, cuff, boot


def comps(px, w, h, cols):
    """8-connected components of pixels whose colour is in cols."""
    seen = set()
    out = []
    for sy in range(h):
        for sx in range(w):
            if px[sy][sx] not in cols or (sy, sx) in seen:
                continue
            comp, stack = [], [(sy, sx)]
            seen.add((sy, sx))
            while stack:
                y, x = stack.pop()
                comp.append((y, x))
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        ny, nx = y + dy, x + dx
                        if 0 <= ny < h and 0 <= nx < w and (ny, nx) not in seen and px[ny][nx] in cols:
                            seen.add((ny, nx))
                            stack.append((ny, nx))
            out.append(comp)
    return out


def cy(comp):
    return sum(p[0] for p in comp) / len(comp)


def dress13(px, w, h, table, death=False):
    """Buu-index frame (remap applied) -> LoG2-index frame in Super 13's colours. death: the frames of
    light bursting out of him (Buu animation 17), whose yellow patches on the torso stay light."""
    skin, hair, pants, belt, sole, cuff, boot = RAMPS
    out = [[(ARC.get(c) or table[c][0]) if c else 0 for c in row] for row in px]
    sk = [(y, x) for y in range(h) for x in range(w) if px[y][x] in B_SKIN]
    if not sk:
        return out
    skin_y = cy(sk)
    for y, x in sk:
        out[y][x] = skin[px[y][x]]
    reds = sorted(comps(px, w, h, B_RED), key=len, reverse=True)
    belt_y = cy(reds[0]) if reds and len(reds[0]) >= 12 else None
    pants_px = []
    hair_px = set()
    for comp in comps(px, w, h, B_WARM):
        if not any(px[y][x] in B_YELLOW for y, x in comp):
            continue                                        # white light, sparks: unchanged
        c = cy(comp)
        if c < skin_y:
            part = hair
            hair_px.update(comp)
        elif death and belt_y is not None and c < belt_y:
            continue                                        # light bursting out of the torso
        elif len(comp) >= 30:
            # trousers: the pixels with no transparent 4-neighbour and their 8-neighbours; the rest of
            # the component is a swing arc or streak touching them (white)
            opaque = lambda y, x: 0 <= y < h and 0 <= x < w and px[y][x] != 0   # noqa: E731
            core = {(y, x) for y, x in comp if all(opaque(y + dy, x + dx) for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)))}
            for y, x in comp:
                if any((y + dy, x + dx) in core for dy in (-1, 0, 1) for dx in (-1, 0, 1)):
                    out[y][x] = pants[px[y][x]]
                    pants_px.append((y, x))
                else:
                    out[y][x] = hair[px[y][x]]
            continue
        else:
            part = cuff                                     # the gold necklace and arm cuffs
        for y, x in comp:
            out[y][x] = part.get(px[y][x], cuff[118])
    # Broly's hair is shaded with the skin ramp: skin pixels mostly surrounded by hair are hair (grown
    # from the yellow strands three times, 5x5 window), shaded grey
    shade = {67: 9, 66: 9, 200: 10, 201: 10, 203: 11, 18: 11, 202: 12, 205: 13, 16: 13}
    rest = set(sk)
    for _ in range(3):
        grown = [(y, x) for y, x in rest
                 if sum((y + dy, x + dx) in hair_px for dy in range(-2, 3) for dx in range(-2, 3)) >= 9]
        for p in grown:
            hair_px.add(p)
            rest.discard(p)
            out[p[0]][p[1]] = shade[px[p[0]][p[1]]]
    for y, x in [p for p in rest if px[p[0]][p[1]] in (202, 205, 16)]:    # the hair's dark outline
        if sum((y + dy, x + dx) in hair_px for dy in (-1, 0, 1) for dx in (-1, 0, 1)) >= 3:
            out[y][x] = 14
    pants_y = cy(pants_px) if pants_px else None
    for k, comp in enumerate(reds):
        if belt_y is not None and len(comp) >= 8 and abs(cy(comp) - belt_y) <= 6:   # the sash, front and back
            for y, x in comp:
                out[y][x] = belt[px[y][x]]
        elif pants_y is not None and cy(comp) > pants_y:
            for y, x in comp:
                out[y][x] = sole[px[y][x]]
    for comp in comps(px, w, h, B_SILVER):
        part = boot if pants_y is not None and cy(comp) > pants_y + 2 else cuff
        for y, x in comp:
            out[y][x] = part[px[y][x]]
    return out


RAMPS = None


class S13Converter(BL.Converter):
    """buu2log2's converter with Super 13's recolour instead of the plain nearest-colour map."""
    death = False

    def encode(self, px, w, h, ow, oh, remap):
        src = [[(remap[c] if remap and c else c) for c in row] for row in px]
        dressed = dress13(src, w, h, self.table, self.death)
        out = [[0] * ow for _ in range(oh)]
        for yy in range(h):
            for xx in range(w):
                out[yy][xx] = dressed[yy][xx]
        raw = BL.tile_bytes(out, ow, oh)
        return (ow, oh, compress(raw), raw)


def build_record(conv):
    """(header, groups): groups[g] = None or [frames x 4] of converted frames (xo, yo, w, h, attr, blob, raw)."""
    rec = conv.bu.record(BUU_BROLY)
    anims = [conv.bu.anim(a) if a else [] for a in rec["anims"]]
    header = bytearray(conv.t_header)
    header[2] = GROUP_COUNT
    for k in range(4):   # one hurt box for every facing (Broly's side boxes are 29 px, narrower than his body)
        header[0x0C + 16 * k:0x1C + 16 * k] = struct.pack("<iiii", *HURT_BOX)
    groups = []
    for gi in range(GROUP_COUNT):
        if gi in GROUPS:
            spec = GROUPS[gi]
        elif gi < len(conv.t_counts) and conv.t_counts[gi]:
            src = BL.GROUP_MAP.get(gi, 0)
            fr = anims[src] if src < len(anims) and anims[src] else anims[0]
            spec = [(src if anims[src] else 0, k % len(fr)) for k in range(conv.t_counts[gi])]
        else:
            groups.append(None)
            continue
        dirs = []
        for d in range(4):
            frames = []
            for a, f in spec:
                conv.death = a == 17
                frames.append(conv.convert_frame(anims[a][f][d], rec["remap"], BUU_BROLY))
            dirs.append(frames)
        groups.append(dirs)
    blank = (0, 0, 8, 8, 0x00002000, compress(bytes(64)), bytes(64))
    groups[BLANK_GROUP] = [[blank] for _ in range(4)]
    size = max(f[2] * f[3] for g in groups if g for dirs in g for f in dirs)
    struct.pack_into("<H", header, 0, size)
    if size >= 0x1000:
        header[3] = max(header[3], 0x18)
    return header, groups


# ---- projectiles ------------------------------------------------------------------------------------
def orb(size, r, frame, trail=False):
    """A red-pink ki orb: white core, pink, red, dark red rim with a flickering spiky edge."""
    if trail:
        ramp = [near((255, 160, 200)), near((230, 60, 110)), near((170, 20, 60)), near((110, 0, 30))]
    else:
        ramp = [2, near((255, 190, 220)), near((255, 90, 140)), near((220, 20, 70)), near((140, 0, 40))]
    px = [[0] * size for _ in range(size)]
    c = size / 2
    spikes = 9 if size >= 32 else 6
    for y in range(size):
        for x in range(size):
            dx, dy = x + 0.5 - c, y + 0.5 - c
            d = math.hypot(dx, dy)
            a = math.atan2(dy, dx)
            edge = r * (1.0 + 0.16 * max(0.0, math.sin(spikes * a + frame * 1.9)) ** 3 + 0.05 * math.sin(3 * a - frame))
            if d > edge:
                continue
            t = d / edge
            k = min(len(ramp) - 1, int(t * len(ramp) * (0.9 if trail else 1.0)))
            px[y][x] = ramp[k]
    return px


def build_projectiles():
    out = {}
    out["s13_orb_s"] = [(8, 8, 16, 16, 0x40002000, orb(16, 5.6, f)) for f in range(3)]
    out["s13_orb_m"] = [(16, 16, 32, 32, 0x80002000, orb(32, 11.5, f)) for f in range(2)]
    out["s13_orb_l"] = [(32, 32, 64, 64, 0xC0002000, orb(64, 25.0, f)) for f in range(2)]
    out["s13_trail"] = [(8, 8, 16, 16, 0x40002000, orb(16, 5.0, f, trail=True)) for f in range(2)]
    return out


# ---- portrait ---------------------------------------------------------------------------------------
P_SKIN = {114: 184, 115: 189, 116: 187, 117: 193, 112: 192, 111: 193, 110: 193}
P_HAIR = {119: 9, 34: 11, 26: 12, 35: 13, 25: 12, 27: 13}
P_ARMOUR = {141: 189, 137: 187, 155: 193, 49: 193, 130: 186, 140: 186, 143: 187}
P_EYE = {204: 25}
P_EARRING = {37: 115, 38: 115}
P_OUTLINE = 113


def build_portrait(rom):
    pt = struct.unpack_from("<I", rom, PORTRAIT_TABLE - S.ROM_BASE + 4 * A16_PORTRAIT)[0]
    raw = list(m.unpack(rom, pt))[:4096]
    W = 64
    px = [raw[y * W:(y + 1) * W] for y in range(W)]
    out = [row[:] for row in px]
    hair = {(y, x) for y in range(W) for x in range(W) if px[y][x] in P_HAIR and y < 34}
    for y in range(W):
        for x in range(W):
            c = px[y][x]
            if (y, x) in hair:
                out[y][x] = P_HAIR[c]
            elif c in P_EARRING:
                out[y][x] = P_SKIN[P_EARRING[c]]
            elif c in P_SKIN and not (y < 4 or x < 3 or x > 60):
                out[y][x] = P_SKIN[c]
            elif c in P_ARMOUR:
                out[y][x] = P_ARMOUR[c]
            elif c in P_EYE:
                out[y][x] = P_EYE[c]
            elif c == P_OUTLINE:
                by_hair = any((y + dy, x + dx) in hair for dy in (-1, 0, 1) for dx in (-1, 0, 1))
                out[y][x] = 14 if by_hair else 211
    return bytes(v for row in out for v in row)


# ---- output -----------------------------------------------------------------------------------------
class CWriter:
    def __init__(self):
        self.lines = ["/* generated by tools/super13sprites.py from the ROMs: Super Android 13 (record s13_record),",
                      " * his ki orbs and his dialogue portrait. Build output only; never commit. */",
                      '#include "rom.h"', "",
                      "typedef struct { signed char xo, yo; u8 w, h; u32 attr; const void *blob; } S13Desc;", ""]
        self.blobs = {}
        self.bytes = 0

    def raw_blob(self, data):
        if data not in self.blobs:
            name = f"s13_blob_{len(self.blobs)}"
            self.bytes += len(data)
            self.blobs[data] = name
            self.lines.append(f"static const u8 {name}[{len(data)}] __attribute__((aligned(4))) = "
                              f"{{{', '.join(f'0x{b:02x}' for b in data)}}};")
        return self.blobs[data]

    def record(self, header, groups):
        names = []
        for gi, g in enumerate(groups):
            if not g:
                names.append("{0, 0, 0, 0}")
                continue
            arrs = []
            for d, frames in enumerate(g):
                ents = [f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {self.raw_blob(blob)}}}" for xo, yo, w, h, attr, blob, _ in frames]
                name = f"s13_g{gi}_d{d}"
                self.lines.append(f"static const S13Desc {name}[{len(frames)}] = {{{', '.join(ents)}}};")
                arrs.append(name)
            names.append("{" + ", ".join(arrs) + "}")
        self.finish_record(header, names)

    def finish_record(self, header, names):
        hdr = ", ".join(f"0x{b:02x}" for b in header)
        self.lines.append(f"typedef struct {{ u8 header[0x4C]; const S13Desc *groups[{GROUP_COUNT}][4]; u32 end; }} S13Record;")
        self.lines.append(f"const S13Record s13_record = {{{{{hdr}}}, {{{', '.join(names)}}}, 0}};")

    def fallback_record(self, rom, sp):
        rec = sp.record(FALLBACK)
        g = sp.groups(rec)
        header = bytearray(rom[rec - S.ROM_BASE:rec - S.ROM_BASE + 0x4C])
        header[2] = GROUP_COUNT
        blank = self.raw_blob(compress(bytes(64)))
        self.lines.append(f"static const S13Desc s13_blank[1] = {{{{0, 0, 8, 8, 0x00002000u, {blank}}}}};")
        names = []
        for gi in range(GROUP_COUNT):
            src = gi if gi < len(g) and any(g[gi]) else FALLBACK_GROUPS.get(gi)
            if gi == BLANK_GROUP:
                names.append("{s13_blank, s13_blank, s13_blank, s13_blank}")
            elif src is None or src >= len(g) or not any(g[src]):
                names.append("{0, 0, 0, 0}")
            else:
                names.append("{" + ", ".join(f"(const S13Desc *)0x{p:08x}u" for p in g[src]) + "}")
        self.finish_record(header, names)

    def projectiles(self, proj):
        for name, frames in proj.items():
            ents = [f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {self.raw_blob(BL.tile_bytes(px, w, h))}}}"
                    for xo, yo, w, h, attr, px in frames]
            self.lines.append(f"const S13Desc {name}[{len(frames)}] = {{{', '.join(ents)}}};")

    def save(self, path):
        os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
        open(path, "w", encoding="utf-8", newline="\n").write("\n".join(self.lines) + "\n")


def untile(raw, w, h):
    return [[raw[((y // 8) * (w // 8) + x // 8) * 64 + (y % 8) * 8 + x % 8] for x in range(w)] for y in range(h)]


def preview(path, groups, proj, portrait, ref_portrait, scale=1):
    """Every group x 4 facings x every frame; projectiles and both portraits at the bottom."""
    cell = 68
    rows = [(gi, g) for gi, g in enumerate(groups) if g]
    maxf = max(len(g[0]) for _, g in rows)
    cw = 24 + 4 * maxf * cell
    ch = len(rows) * cell + 150
    cv = S.Canvas(cw * scale, ch * scale)

    def put(x, y, c):
        for a in range(scale):
            for b in range(scale):
                cv.put(x * scale + b, y * scale + a, c)

    for r, (gi, g) in enumerate(rows):
        y0 = r * cell
        cv.text(2 * scale, (y0 + 4) * scale, str(gi), (255, 255, 0), scale)
        for d in range(4):
            for fi, (xo, yo, w, h, attr, blob, raw) in enumerate(g[d]):
                ox = 24 + (d * maxf + fi) * cell + 20
                oy = y0 + 50
                img = untile(raw, w, h)
                for yy in range(h):
                    for xx in range(w):
                        ci = img[yy][xx]
                        if ci:
                            sx = w - 1 - xx if attr & 0x10000000 else xx
                            put(ox + xo + sx, oy + yo + yy, PAL[ci])
    y0 = len(rows) * cell + 8
    x = 24
    for name, frames in proj.items():
        for xo, yo, w, h, attr, px in frames:
            for yy in range(h):
                for xx in range(w):
                    if px[yy][xx]:
                        put(x + xx, y0 + yy, PAL[px[yy][xx]])
            x += w + 6
    x += 20
    for img in (ref_portrait, portrait):
        for yy in range(64):
            for xx in range(64):
                put(x + xx, y0 + yy, PAL[img[yy * 64 + xx]] if img[yy * 64 + xx] else (40, 40, 40))
        x += 72
    cv.save(path)


def main():
    global PAL, RAMPS
    rom = open(sys.argv[1], "rb").read()
    out_c = sys.argv[2]
    opt = lambda k: sys.argv[sys.argv.index(k) + 1] if k in sys.argv else None   # noqa: E731
    buu_path = opt("--buu")
    pdir = opt("--preview-dir")
    buu = open(buu_path, "rb").read() if buu_path and buu_path != "none" and os.path.exists(buu_path) else None
    sp = S.Sprites(rom)
    PAL = sp.pal
    RAMPS = ramps()
    w = CWriter()
    groups = None
    if buu:
        conv = S13Converter(rom, buu, TEMPLATE)
        header, groups = build_record(conv)
        w.record(header, groups)
    else:
        w.fallback_record(rom, sp)
    proj = build_projectiles()
    w.projectiles(proj)
    portrait = build_portrait(rom)
    pblob = compress(portrait)
    assert m.decompress(pblob, 8, 1)[:len(portrait)] == portrait
    w.lines.append(f"const u8 s13_portrait[{len(pblob)}] __attribute__((aligned(4))) = "
                   f"{{{', '.join(f'0x{b:02x}' for b in pblob)}}};")
    w.save(out_c)
    if pdir and groups:
        os.makedirs(pdir, exist_ok=True)
        pt = struct.unpack_from("<I", rom, PORTRAIT_TABLE - S.ROM_BASE + 4 * A16_PORTRAIT)[0]
        ref = bytes(m.unpack(rom, pt))[:4096]
        preview(os.path.join(pdir, "super13_preview.png"), groups, proj, portrait, ref)
        preview(os.path.join(pdir, "super13_preview_x2.png"), groups, proj, portrait, ref, 2)
    nf = len({f[5] for g in groups if g for dirs in g for f in dirs}) if groups else 0
    print(f"wrote {out_c}: Super 13 {'Buu record 28' if buu else 'fallback (Perfect Cell 30)'}, {nf} frame blobs, "
          f"{w.bytes} bytes, portrait {len(pblob)} bytes")


if __name__ == "__main__":
    main()
