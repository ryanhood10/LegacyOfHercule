"""The Broly side chapter's art, built from the ROMs at build time (docs/sprites.md "Paragus and the
Saiyan ship"): Paragus' sprite and dialogue portrait (Vegeta made old) and the Saiyan ship. Emits C
source compiled into the co-op ROM; nothing from either ROM is committed.

usage: brolysprites.py roms/log2.gba build/gen/broly_sprites_data.c [--buu roms/buu.gba] [--preview-dir build/x]

Paragus (record paragus_record, sprite id 170 by default): Vegeta's playable set in Saiyan armour
(sprite 137, all 21 groups) with every frame aged:
  * the hair (Vegeta's black flame, told apart per frame: black pixels above the shoulders that are not
    the eyes, i.e. not between face-skin pixels in the face's rows) becomes grey: a black outline where
    it meets anything else, mid grey inside, light grey for Vegeta's grey hair highlights;
  * a grey goatee on the chin: the lowest face-skin rows at the chin's centre in front views, at the
    chin's front edge in side views (the side the stored frame faces); none in back views.
The record keeps 137's header and group layout, so the melee boss AI set 21 finds every group it plays
(0, 1, 2, 5-8) and group 3 (knock-down) serves the both-down rule.

Paragus' dialogue portrait (paragus_portrait, portrait id 120 through krillinsprites.py
--extra-portrait): Vegeta's dialogue portrait (Portrait_Table 106) with the same hair rule (the hair is
the black outside the face box) and a grey moustache and chin beard drawn over the skin around the
mouth.

The Saiyan ship (ship_record, sprite id 171): Buu's Fury record 1 (a purple Saiyan-style ship, 64x32,
two animations of two frames x 4 facings) converted to LoG2's palette by tools/buu2log2.py's converter,
as a two-group actor record (idle, walk). Without the Buu ROM the ship falls back to stock sprite 43's
groups (a stand-in so the chapter still loads; the chapter needs the Buu ROM for Broly anyway).
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402
import a16sprites as A  # noqa: E402
import mapchunks as m  # noqa: E402
from buu2log2 import compress  # noqa: E402

VEGETA = 137
PORTRAIT_TABLE = 0x083EC9D4
VEGETA_PORTRAIT = 106
BUU_SHIP = 1
FALLBACK_SHIP = 43
BLACK = 1
HFLIP = 0x10000000            # attr1 bit 12 (h-flip) in the descriptor's attr word
SKIN = {16, 17, 18, 19, 20, 21, 22, 23}       # Vegeta's face ramp (sprite and portrait)
ARMOUR = {2, 40, 41, 42, 79, 80, 81}          # white chest plate, gold shoulders, blue suit
HAIR_HI = 13                                   # grey highlight inside Vegeta's black hair
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


def neighbours4(y, x):
    return ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1))


# ---- Paragus' sprite --------------------------------------------------------------------------------
def age_frame(px, w, h, facing):
    """facing in stored pixel space: 'front', 'back', 'left', 'right'. Returns a new pixel grid."""
    out = [row[:] for row in px]
    # Vegeta's skin shows only on the face (white gloves, blue suit), so skin = face
    face = [(y, x) for y in range(h) for x in range(w) if px[y][x] in SKIN]
    fy0 = min((y for y, _ in face), default=h)
    fy1 = max((y for y, _ in face), default=-1)

    def is_eye(y, x):
        row = px[y]
        left = any(row[k] in SKIN for k in range(max(0, x - 4), x))
        right = any(row[k] in SKIN for k in range(x + 1, min(w, x + 5)))
        if left and right:
            return True
        up = any(px[k][x] in SKIN for k in range(max(0, y - 3), y))
        down = any(px[k][x] in SKIN for k in range(y + 1, min(h, y + 4)))
        return up and down
    # black outside the eyes, in 4-connected components; hair = the components touching the face (8-way)
    # plus, in back views (no face), the component holding the topmost black pixel. The ground shadow
    # under the boots touches no skin and lies below the head.
    black = [[px[y][x] == BLACK and not is_eye(y, x) for x in range(w)] for y in range(h)]
    seen, comps = set(), []
    for y in range(h):
        for x in range(w):
            if black[y][x] and (y, x) not in seen:
                comp, stack = [], [(y, x)]
                seen.add((y, x))
                while stack:
                    cy, cx = stack.pop()
                    comp.append((cy, cx))
                    for ny, nx in neighbours4(cy, cx):
                        if 0 <= ny < h and 0 <= nx < w and black[ny][nx] and (ny, nx) not in seen:
                            seen.add((ny, nx))
                            stack.append((ny, nx))
                comps.append(comp)

    def touches_skin(comp):
        for y, x in comp:
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < h and 0 <= nx < w and px[ny][nx] in SKIN:
                        return True
        return False
    hair = set()
    for comp in comps:
        if len(comp) >= 3 and touches_skin(comp):
            hair.update(comp)
    if not hair and comps:
        top = min(comps, key=lambda c: min(y for y, _ in c))
        if len(top) >= 10:
            hair.update(top)
    # Vegeta's grey highlights inside the hair (13 is also his boots: only where hair surrounds it)
    for y in range(h):
        for x in range(w):
            if px[y][x] == HAIR_HI and sum((ny, nx) in hair for ny, nx in neighbours4(y, x)) >= 2:
                hair.add((y, x))
    grey_lo, grey_mid, grey_hi = near((90, 90, 98)), near((128, 128, 136)), near((180, 180, 188))
    upright = bool(face) and bool(hair) and (sum(y for y, _ in hair) / len(hair)) < (sum(y for y, _ in face) / len(face))
    for y, x in hair:
        v = px[y][x]
        if v == HAIR_HI:
            out[y][x] = grey_hi
            continue
        edge = False
        for ny, nx in neighbours4(y, x):
            if not (0 <= ny < h and 0 <= nx < w) or (ny, nx) not in hair:
                edge = True
                break
        out[y][x] = BLACK if edge else grey_mid
    # a darker band right above the face so the forehead line stays readable
    for y, x in hair:
        if out[y][x] == grey_mid and y + 1 < h and px[y + 1][x] in SKIN:
            out[y][x] = grey_lo
    # goatee
    if face and facing != "back" and upright:
        chin = fy1
        xs = [x for y, x in face if y == chin]
        if facing == "front":
            cx = (min(xs) + max(xs)) // 2 if xs else w // 2
            spots = [(chin, cx - 1), (chin, cx), (chin, cx + 1), (chin - 1, cx), (chin + 1, cx)]
        else:
            front = min(xs) if facing == "left" else max(xs)
            step = 1 if facing == "left" else -1
            spots = [(chin, front), (chin, front + step), (chin - 1, front), (chin + 1, front + step)]
        for y, x in spots:
            if 0 <= y < h and 0 <= x < w and (px[y][x] in SKIN or (y == chin + 1 and px[y][x] != 0)):
                out[y][x] = grey_mid if y <= chin else grey_lo
    return out


def facing_of(d, attr):
    if d == 0:
        return "front"
    if d == 1:
        return "back"
    flipped = bool(attr & HFLIP)
    left = (d == 2) != flipped
    return "left" if left else "right"


def build_paragus(sp):
    rom = sp.rom
    rec = sp.record(VEGETA)
    g = sp.groups(rec)
    counts = sp.frame_counts(g)
    header = bytearray(rom[rec - S.ROM_BASE:rec - S.ROM_BASE + 0x4C])
    groups, done = [], {}
    for grp in g:
        if not any(grp):
            groups.append(None)
            continue
        dirs = []
        for d in range(4):
            arr = grp[d]
            frames = []
            if arr:
                for fi in range(counts[arr]):
                    xo, yo, w, h, attr, blob = sp.descriptor(arr, fi)
                    key = (blob, facing_of(d, attr))
                    if key not in done:
                        _, _, px = sp.frame_pixels(arr, fi)
                        done[key] = age_frame(px, w, h, key[1])
                    frames.append((xo, yo, w, h, attr, done[key]))
            dirs.append(frames)
        groups.append(dirs)
    header[2] = len(groups)
    return header, groups, len(done)


# ---- Paragus' portrait ------------------------------------------------------------------------------
def build_portrait(rom):
    pt = struct.unpack_from("<I", rom, PORTRAIT_TABLE - S.ROM_BASE + 4 * VEGETA_PORTRAIT)[0]
    raw = list(m.unpack(rom, pt))[:4096]
    W = 64
    px = [raw[y * W:(y + 1) * W] for y in range(W)]
    out = [row[:] for row in px]

    def in_face(y, x):   # the eyes, brows and the face's own black lines (read off the portrait)
        return (29 <= y <= 38 and 14 <= x <= 43) or (y >= 44 and x < 44) or y >= 50
    hair = {(y, x) for y in range(3, 50) for x in range(3, 61) if px[y][x] in (BLACK, 211) and not in_face(y, x)}
    dark, mid, light = near((80, 80, 88)), near((128, 128, 136)), near((176, 176, 184))
    for y, x in hair:
        edge = any((ny, nx) not in hair for ny, nx in neighbours4(y, x))
        if edge:
            out[y][x] = BLACK
            continue
        deep = all((y + dy, x + dx) in hair for dy in (-2, 0, 2) for dx in (-2, 0, 2))
        out[y][x] = light if px[y][x] == 211 else (mid if deep else dark)
    # moustache over the upper lip and a pointed chin beard (grey, darker rim)
    beard = set()
    for y in (42, 43):
        for x in range(23, 35):
            beard.add((y, x))
    for y in range(46, 51):
        half = 6 - (y - 46)
        for x in range(30 - half, 31 + half):
            beard.add((y, x))
    for y, x in beard:
        if px[y][x] in SKIN or px[y][x] == 113:
            rim = any((ny, nx) not in beard for ny, nx in neighbours4(y, x))
            out[y][x] = dark if rim else mid
    return bytes(v for row in out for v in row)


# ---- the ship ---------------------------------------------------------------------------------------
def build_ship(sp, buu_rom):
    """Returns (header, groups) with groups as [[frames x 4]] of (xo, yo, w, h, attr, px or None, blob)."""
    if buu_rom is None:
        return None
    import buu2log2 as BL
    conv = BL.Converter(sp.rom, buu_rom, 23)
    rec = conv.bu.record(BUU_SHIP)
    anims = [conv.bu.anim(a) if a else [] for a in rec["anims"]]
    groups = []
    for ai in range(2):
        frames = anims[ai] if ai < len(anims) and anims[ai] else anims[0]
        dirs = []
        for d in range(4):
            fl = []
            for fi in range(len(frames)):
                xo, yo, w, h, attr, blob, raw = conv.convert_frame(frames[fi][d], rec["remap"], BUU_SHIP)
                fl.append((xo, yo, w, h, attr, blob))
            dirs.append(fl)
        groups.append(dirs)
    size = max(f[2] * f[3] for g in groups for dl in g for f in dl)
    header = bytearray(conv.t_header)
    struct.pack_into("<H", header, 0, size)
    header[2] = len(groups)
    for k in range(4):
        header[0x0C + 16 * k:0x1C + 16 * k] = struct.pack("<iiii", *rec["boxes"][k])
    return header, groups


# ---- output -----------------------------------------------------------------------------------------
class CWriter:
    def __init__(self):
        self.lines = ["/* generated by tools/brolysprites.py from the ROMs: Paragus (sprite + portrait) and the",
                      " * Saiyan ship. Build output only; never commit. */",
                      '#include "rom.h"', "",
                      "typedef struct { signed char xo, yo; u8 w, h; u32 attr; const void *blob; } BrDesc;", ""]
        self.blobs = {}
        self.bytes = 0

    def raw_blob(self, prefix, data):
        if data not in self.blobs:
            name = f"{prefix}_blob_{len(self.blobs)}"
            self.bytes += len(data)
            self.blobs[data] = name
            self.lines.append(f"static const u8 {name}[{len(data)}] __attribute__((aligned(4))) = "
                              f"{{{', '.join(f'0x{b:02x}' for b in data)}}};")
        return self.blobs[data]

    def blob(self, prefix, px, w, h):
        return self.raw_blob(prefix, compress(A.tiles(px, w, h)))

    def record(self, prefix, header, groups, precompressed=False):
        names = []
        for gi, g in enumerate(groups):
            if not g:
                names.append("{0, 0, 0, 0}")
                continue
            arrs = []
            for d, frames in enumerate(g):
                if not frames:
                    arrs.append("0")
                    continue
                ents = []
                for xo, yo, w, h, attr, px in frames:
                    b = self.raw_blob(prefix, px) if precompressed else self.blob(prefix, px, w, h)
                    ents.append(f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {b}}}")
                name = f"{prefix}_g{gi}_d{d}"
                self.lines.append(f"static const BrDesc {name}[{len(frames)}] = {{{', '.join(ents)}}};")
                arrs.append(name)
            names.append("{" + ", ".join(arrs) + "}")
        hdr = ", ".join(f"0x{b:02x}" for b in header)
        t = f"{prefix}_rec_t"
        self.lines.append(f"typedef struct {{ u8 header[0x4C]; const BrDesc *groups[{len(groups)}][4]; u32 end; }} {t};")
        self.lines.append(f"const {t} {prefix}_record = {{{{{hdr}}}, {{{', '.join(names)}}}, 0}};")

    def alias_record(self, prefix, rom_record, ngroups):
        """A record whose header and group pointers are a stock record's (ROM addresses)."""
        sp_hdr = ", ".join(f"0x{b:02x}" for b in rom_record[0])
        grp = ", ".join("{" + ", ".join(f"(const BrDesc *)0x{p:08x}u" for p in g) + "}" for g in rom_record[1])
        t = f"{prefix}_rec_t"
        self.lines.append(f"typedef struct {{ u8 header[0x4C]; const BrDesc *groups[{ngroups}][4]; u32 end; }} {t};")
        self.lines.append(f"const {t} {prefix}_record = {{{{{sp_hdr}}}, {{{grp}}}, 0}};")

    def save(self, path):
        os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
        open(path, "w", encoding="utf-8", newline="\n").write("\n".join(self.lines) + "\n")


def preview(path, sp, groups, portrait=None, ref=None):
    """Every group x 4 facings (all frames), 3x copy; the portrait (and the stock one) at the bottom."""
    cell = 34
    rows = []
    for gi, g in enumerate(groups):
        if not g:
            continue
        row = []
        for frames in g:
            for xo, yo, w, h, attr, px in frames:
                row.append(A.hflip(px) if attr & HFLIP else px)
            row.append(None)
        rows.append((gi, row))
    width = max(max(len(r) for _, r in rows) * cell + 16, 140)
    height = len(rows) * cell + (68 if portrait else 0)
    cv = S.Canvas(width, height, (60, 60, 90))
    for ri, (gi, row) in enumerate(rows):
        cv.text(1, ri * cell + 2, str(gi), (255, 255, 0))
        for fi, px in enumerate(row):
            if not px:
                continue
            for y, line in enumerate(px):
                for x, v in enumerate(line):
                    if v:
                        cv.put(16 + fi * cell + 1 + x, ri * cell + 1 + y, sp.pal[v])
    if portrait:
        y0 = len(rows) * cell + 2
        for k, img in enumerate([p for p in (ref, portrait) if p]):
            for i, v in enumerate(img):
                if v:
                    cv.put(2 + k * 68 + i % 64, y0 + i // 64, sp.pal[v])
    cv.save(path)
    try:
        from PIL import Image
        im = Image.open(path)
        im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(path.replace(".png", "_x3.png"))
    except ImportError:
        pass


def main():
    global PAL
    rom = open(sys.argv[1], "rb").read()
    out_c = sys.argv[2]
    opt = lambda k: sys.argv[sys.argv.index(k) + 1] if k in sys.argv else None   # noqa: E731
    buu_path = opt("--buu")
    pdir = opt("--preview-dir")
    buu = open(buu_path, "rb").read() if buu_path and buu_path != "none" and os.path.exists(buu_path) else None
    sp = S.Sprites(rom)
    PAL = sp.pal
    w = CWriter()
    hp, gp, n = build_paragus(sp)
    w.record("paragus", hp, gp)
    portrait = build_portrait(rom)
    pblob = compress(portrait)
    assert m.decompress(pblob, 8, 1)[:len(portrait)] == portrait
    w.lines.append(f"const u8 paragus_portrait[{len(pblob)}] __attribute__((aligned(4))) = "
                   f"{{{', '.join(f'0x{b:02x}' for b in pblob)}}};")
    ship = build_ship(sp, buu)
    if ship:
        hs, gs = ship
        w.record("ship", hs, gs, precompressed=True)
    else:
        rec = sp.record(FALLBACK_SHIP)
        g = sp.groups(rec)
        w.alias_record("ship", (rom[rec - S.ROM_BASE:rec - S.ROM_BASE + 0x4C], g), len(g))
    w.save(out_c)
    if pdir:
        os.makedirs(pdir, exist_ok=True)
        pt = struct.unpack_from("<I", rom, PORTRAIT_TABLE - S.ROM_BASE + 4 * VEGETA_PORTRAIT)[0]
        preview(os.path.join(pdir, "paragus_preview.png"), sp, gp, portrait, bytes(m.unpack(rom, pt))[:4096])
    print(f"wrote {out_c}: Paragus {n} frames, portrait {len(pblob)} bytes, ship {'Buu record 1' if ship else 'fallback 43'}, "
          f"{w.bytes} bytes of frames")


if __name__ == "__main__":
    main()
