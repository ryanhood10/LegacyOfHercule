"""Haloed Goku: his playable records with the dead pose's halo on every frame, built from the ROM at build
time (docs/sprites.md "Haloed Goku"). Emits C source compiled into the co-op ROM; nothing from the ROM is
committed.

usage: halosprites.py roms/log2.gba build/gen/halo_sprites_data.c [--preview-dir build/x]

After the Cell Games Goku stays playable but dead (docs/story.md 4h). Every LoG2 player record has a
"dead" pose, group 17 (2 frames): the character standing with a halo, which the game draws as a pale
yellow ring (palette 0x24 / 0x25 / 0x2C) round the top of the hair, its top arc one row above the hair
and its front arc over it. Goku's (sprite 60, group 17 frame 0, front view) ring is 13 x 5 px.

Records (all 21 / 22 groups of the source, every frame decoded, haloed and LZ-compressed with
buu2log2.compress; headers, frame boxes, offsets and attributes unchanged):
  halo_goku_record  sprite 60 (Goku, display row 19) -> id 176, display row 26
  halo_ssj_record   sprite 61 (Super Saiyan Goku, row 20) -> id 177, display row 27
Per frame the head is the hair: black (0x01) or Super Saiyan yellow (0x2E, 0x78; Goku's own group 9,
the transformation, turns his hair gold half way), taken as the 8-connected component of hair pixels
holding the topmost one (the ground shadow and the eyes are other components); the head counts as
Super Saiyan when it has more yellow than black. The ring goes on at the hair's top row - 1 (row 0 at
least), centred on the hair's columns in its top 4 rows, as in the stock dead pose; ring pixels outside
the frame are dropped. On Super Saiyan hair, which is the halo's own yellow, the ring is drawn in white / the halo's palest
yellow (0x02 / 0x2C) with a black line under its front arc where that lies on hair. Group 17 (the
stock dead pose, already haloed) is kept as it is.
Buu's Fury has no haloed Goku (ids 97-120 checked); its floating halos are on NPCs only (30 Bubbles,
125 Gregory).
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402
import a16sprites as A  # noqa: E402
from brolysprites import CWriter  # noqa: E402

GOKU, SSJ_GOKU = 60, 61
HALO_SRC = (GOKU, 17, 0, 0)          # sprite, group, facing, frame of the stock halo
HALO = {0x24, 0x25, 0x2C}
DEAD_GROUP = 17
BLACK = 1
GOLD = {0x2E, 0x78}                  # Super Saiyan hair (61, and 60's own transform frames in group 9)
HAIR = {BLACK} | GOLD
HFLIP = 0x10000000
RECORDS = ((GOKU, "halo_goku"), (SSJ_GOKU, "halo_ssj"))


def halo_template(sp):
    """[(dy, dx, colour)] relative to (ring top row, ring centre column)."""
    sid, gi, d, fi = HALO_SRC
    arr = sp.groups(sp.record(sid))[gi][d]
    _, _, px = sp.frame_pixels(arr, fi)
    pts = [(y, x, c) for y, r in enumerate(px) for x, c in enumerate(r) if c in HALO]
    y0 = min(y for y, _, _ in pts)
    x0, x1 = min(x for _, x, _ in pts), max(x for _, x, _ in pts)
    cx = (x0 + x1) / 2
    return [(y - y0, x - cx, c) for y, x, c in pts]


def hair_component(px, hair):
    """The 8-connected component of hair pixels that holds the topmost one; None when the frame has none."""
    h, w = len(px), len(px[0])
    seeds = [(y, x) for y in range(h) for x in range(w) if px[y][x] in hair]
    if not seeds:
        return None
    top = min(seeds)
    ok = hair
    seen, todo = {top}, [top]
    while todo:
        y, x = todo.pop()
        for yy in (y - 1, y, y + 1):
            for xx in (x - 1, x, x + 1):
                if 0 <= yy < h and 0 <= xx < w and (yy, xx) not in seen and px[yy][xx] in ok:
                    seen.add((yy, xx))
                    todo.append((yy, xx))
    return seen


SSJ_RING = {0x24: 0x2C, 0x25: 0x02, 0x2C: 0x02}   # on yellow hair the ring is white / pale


def add_halo(px, ring):
    comp = hair_component(px, HAIR)
    if not comp:
        return px, False
    ssj = sum(px[y][x] in GOLD for y, x in comp) > sum(px[y][x] == BLACK for y, x in comp)
    h, w = len(px), len(px[0])
    t = min(y for y, _ in comp)
    crown = [x for y, x in comp if y < t + 4]
    cx = sum(crown) / len(crown)
    top = max(0, t - 1)
    out = [r[:] for r in px]
    bottom = max(dy for dy, _, _ in ring)
    for dy, dx, c in ring:
        y, x = top + dy, int(round(cx + dx))
        if 0 <= y < h and 0 <= x < w:
            out[y][x] = SSJ_RING[c] if ssj else c
            if ssj and dy == bottom and y + 1 < h and px[y + 1][x] in GOLD:
                out[y + 1][x] = BLACK
    return out, True


def frame_counts(sp, groups):
    """Frames per descriptor array: the run of valid descriptors (a GBA OBJ size and a ROM blob whose
    header kind is 0-2) up to the next array of the record. sprites.frame_counts guesses from the gap to
    the next array or blob, which cuts Super Saiyan Goku's group 21 (4 frames, the detransformation
    the engine plays with animation 0x24) to 1 frame: a copy built from that guess makes the engine read
    descriptors past the array and decompress garbage over EWRAM."""
    starts = {p for g in groups for p in g if p}
    out = {}
    for arr in starts:
        n = 0
        while n < 32 and (n == 0 or arr + 12 * n not in starts):
            xo, yo, w, h, attr, blob = sp.descriptor(arr, n)
            if w not in (8, 16, 32, 64) or h not in (8, 16, 32, 64) or not sp.is_ptr(blob) or sp.u32(blob) > 2:
                break
            n += 1
        out[arr] = n
    return out


def build(sp, sid, ring):
    rec = sp.record(sid)
    g = sp.groups(rec)
    counts = frame_counts(sp, g)
    guess = sp.frame_counts(g)
    for gi, grp in enumerate(g):
        for d, arr in enumerate(grp):
            if arr and counts[arr] != guess[arr]:
                print(f"  sprite {sid} group {gi} facing {d}: {counts[arr]} frames (gap guess {guess[arr]})")
    header = bytearray(sp.rom[rec - S.ROM_BASE:rec - S.ROM_BASE + 0x4C])
    groups, done, haloed = [], {}, 0
    for gi, grp in enumerate(g):
        if not any(grp):
            groups.append(None)
            continue
        dirs = []
        for arr in grp:
            frames = []
            if arr:
                for fi in range(counts[arr]):
                    xo, yo, w, h, attr, blob = sp.descriptor(arr, fi)
                    if blob not in done:
                        _, _, px = sp.frame_pixels(arr, fi)
                        if gi == DEAD_GROUP:      # the stock dead pose already wears the halo
                            done[blob], hit = px, False
                        else:
                            done[blob], hit = add_halo(px, ring)
                        haloed += hit
                    frames.append((xo, yo, w, h, attr, done[blob]))
            dirs.append(frames)
        groups.append(dirs)
    assert header[2] == len(groups)
    return header, groups, len(done), haloed


def preview(path, sp, built):
    """Per record: every group x 4 facings, first frame, stock above haloed; plus a _x3 copy."""
    cell = 34
    rows = []
    for sid, (header, groups) in built:
        stock = sp.groups(sp.record(sid))
        for gi, g in enumerate(groups):
            if not g:
                continue
            row = []
            for d in range(4):
                xo, yo, w, h, attr, px = g[d][0] if g[d] else (0, 0, 0, 0, 0, None)
                if px is None:
                    row += [None, None]
                    continue
                spx = sp.frame_pixels(stock[gi][d], 0)[2]
                row += [(xo, A.hflip(spx) if attr & HFLIP else spx), (xo, A.hflip(px) if attr & HFLIP else px)]
            rows.append((f"{sid}", gi, row))
    cv = S.Canvas(30 + 8 * cell + 4 * 6, len(rows) * cell + 4, (60, 60, 90))
    for ri, (sid, gi, row) in enumerate(rows):
        y0 = 2 + ri * cell
        cv.text(1, y0 + 2, sid, (255, 255, 0))
        cv.text(1, y0 + 10, str(gi), (0, 255, 255))
        for k, item in enumerate(row):
            if not item:
                continue
            xo, px = item
            x0 = 30 + (k // 2) * (2 * cell + 6) + (k % 2) * cell + (8 + xo if xo < 0 else 8)
            for y, line in enumerate(px):
                for x, v in enumerate(line):
                    if v:
                        cv.put(x0 + x, y0 + y, sp.pal[v])
    cv.save(path)
    try:
        from PIL import Image
        im = Image.open(path)
        im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(path.replace(".png", "_x3.png"))
    except ImportError:
        pass


def main():
    rom = open(sys.argv[1], "rb").read()
    out_c = sys.argv[2]
    pdir = sys.argv[sys.argv.index("--preview-dir") + 1] if "--preview-dir" in sys.argv else None
    sp = S.Sprites(rom)
    ring = halo_template(sp)
    w = CWriter()
    w.lines[0] = "/* generated by tools/halosprites.py from the ROM: Goku's playable records with a halo on every"
    w.lines[1] = " * frame (haloed Goku, rows 26 / 27). Build output only; never commit. */"
    built, report = [], []
    for sid, prefix in RECORDS:
        header, groups, n, haloed = build(sp, sid, ring)
        w.record(prefix, header, groups)
        built.append((sid, (header, groups)))
        report.append(f"{prefix} {n} frames ({haloed} haloed)")
    w.save(out_c)
    if pdir:
        os.makedirs(pdir, exist_ok=True)
        preview(os.path.join(pdir, "halo_goku_preview.png"), sp, built)
    print(f"wrote {out_c}: {', '.join(report)}, {w.bytes} bytes of frames")


if __name__ == "__main__":
    main()
