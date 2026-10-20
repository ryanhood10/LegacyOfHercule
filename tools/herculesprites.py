"""Hercule's Kamehameha pose (group 18 of sprite 72), built from the ROM at build time
(docs/sprites.md "Hercule's beam pose"). Emits C source compiled into the co-op ROM; nothing from the ROM
is committed.

usage: herculesprites.py roms/log2.gba build/gen/hercule_sprites_data.c [--preview-dir build/x]

The engine's beam pose is group 18: the NPCFireKamehameha opcode (script step 137) puts the entity in
control state 30 (vtable 0x08025038), which plays group 18 frame 0 for 8 frames (the charge) and then
frames 1 and 2 alternating every 8 frames for as long as the beam lasts. Goku's (60) and Gohan's (53,
56, 59) group 18 hold 7 frames per facing: frame 0 the charge (hands cupped at the back hip), frame 1
the thrust, frames 2-6 one repeated image (the hold, hair blown up); right is left mirrored. Hercule's
stock record (72, 21 groups) leaves group 18 empty.

herc_record is record 72 with group 18 filled: its header and every other group are 72's own (ROM
addresses, nothing copied), so all users of sprite 72 (player 1 or 2 on display row 21, the actors in
cutscenes) are unchanged; the Makefile repoints id 72 at it in the relocated sprite index
(spriteclone.py --extra 72=herc_record). Group 18 is Hercule's own frames re-posed:
  * left / right: his punch stance (group 7 frame 0, the same lunge as Goku's beam stance on the same
    ground line) with his chambered back arm and front fist removed, and Goku's beam arms of the same
    frame (charge: hands at the back hip; fire: both arms straight out, open palms) pasted in Hercule's
    colours: hands in his skin ramp, everything from the wrist band up in his brown gi sleeve;
  * front / back: his idle frame with the hanging arms removed and Goku's beam arms of that facing pasted
    the same way (front: palms in front of the belly; back: hands at the hip for the charge, arms out
    of sight for the fire);
  * the hold frame (2-6) is the thrust frame with the afro bristling (a ragged black fringe on its top
    and back edge), as Goku's hair rises in his.
Frames are LZ-compressed with buu2log2.compress; the frame boxes and offsets are Goku's (16x32 front /
back, 32x32 sides, right = the left blob h-flipped).
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402
import a16sprites as A  # noqa: E402
import mapchunks as m  # noqa: E402
from buu2log2 import compress  # noqa: E402

HERCULE = 72
GOKU = 60
REFS = (60, 59)              # preview references: Goku, Super Saiyan teen Gohan (the struggle's Gohan)
BEAM_GROUP = 18
FRAMES = 7                   # frames per facing in every stock beam group (0 charge, 1 thrust, 2-6 hold)
HFLIP = 0x10000000           # attr1 bit 12 in the descriptor's attr word
BLACK = 1

# palette indices (shared OBJ palette 0x081DA6C8)
G_SKIN = {0x72: 0x11, 0x73: 0x12, 0x74: 0x13, 0x14: 0x14, 0x17: 0x17}   # Goku skin -> Hercule skin
G_BLUE = {0xD7, 0x50, 0x4F}                                             # Goku wrist bands / undershirt
TO_SLEEVE = {0x72: 0x6F, 0x73: 0x6F, 0x74: 0x70, 0x14: 0x70, 0x17: 0x71,   # forearm skin / band -> gi sleeve
             0x4F: 0x6F, 0x50: 0x70, 0xD7: 0x71}
H_SKIN = {0x11, 0x12, 0x13, 0x14, 0x17}
H_GI = {0x6F, 0x70, 0x71}

# Per facing (in display space: left is the stored left frame, right mirrors it):
#   base     Hercule's (group, frame) the body comes from
#   erase    boxes (row0, row1, col0, col1, colours or None = any) cleared from the base
#   arms     per beam image (0 charge, 1 thrust): Goku's box (row0, row1, col0, col1), shift (dy, dx),
#            wrist column rule ('lt', c): Goku columns < c are hand, the rest sleeve ('gt': > c is hand;
#            None: every skin pixel stays skin)
SIDE = {
    "base": (7, 0),
    "erase": [(10, 17, 26, 31, None), (11, 14, 25, 25, H_SKIN),        # chambered back arm and fist
              (15, 21, 13, 16, H_SKIN)],                               # the front fist at the waist
    "arms": {0: [((17, 22, 17, 28), (0, 0), ("gt", 21))],
             1: [((12, 22, 0, 14), (0, 3), ("lt", 9))]},
}
# Goku's front beam arms are foreshortened skin over his chest, which in Hercule's colours reads as
# a tan block; the front arms are drawn instead (patch rows, see PATCH_COLOURS): charge = the far arm
# across the belly and both hands cupped at the hip on the viewer's left (Goku's side), thrust = elbows
# out in brown sleeves and both open palms over the belt.
FRONT = {
    "base": (0, 0),
    "erase": [(20, 23, 2, 4, H_SKIN), (20, 23, 11, 14, H_SKIN)],
    "arms": {0: [], 1: []},
    "patch": {0: (18, ["................",
                       "kSkS............",
                       "ksSkobbbbbbbo...",
                       "kshkBBBBBBBBBo..",
                       "_kk_ooooooooo...",
                       "................"]),
              1: (19, ["obBokSkSSkSkobBo",
                       "obBoksSssSskobBo",
                       "_obokshhhhskobo_",
                       "__oo_kssssk_oo__",
                       "...o........o..."])},
}
PATCH_COLOURS = {"o": 0x71, "b": 0x70, "B": 0x6F, "k": 0x17, "h": 0x14, "s": 0x12, "S": 0x11, "_": 0}
BACK = {
    "base": (0, 0),
    "erase": [(16, 23, 0, 1, None), (16, 23, 14, 15, None), (20, 23, 2, 2, None), (20, 23, 13, 13, None)],
    "arms": {0: [((16, 23, 11, 15), (0, 0), (None, 0))],
             1: []},
}


def display(sp, sid, gi, d, fi):
    """A frame as it shows on screen: (xo, yo, w, h, attr, pixels with any h-flip applied)."""
    arr = sp.groups(sp.record(sid))[gi][d]
    xo, yo, w, h, attr, _ = sp.descriptor(arr, fi)
    px = sp.frame_pixels(arr, fi)[2]
    return xo, yo, w, h, attr, (A.hflip(px) if attr & HFLIP else [r[:] for r in px])


def arm_colour(c, col, rule):
    """Goku's arm pixel -> Hercule's colour (None: not an arm pixel)."""
    kind, wrist = rule
    if c in G_BLUE:
        return TO_SLEEVE[c]
    if c not in G_SKIN:
        return None
    hand = kind is None or (col < wrist if kind == "lt" else col > wrist)
    return G_SKIN[c] if hand else TO_SLEEVE[c]


def outline(px, rows):
    """Re-close edges opened by the erase: a gi pixel or a skin pixel next to transparency gets the dark
    gi / skin outline."""
    h, w = len(px), len(px[0])
    out = [r[:] for r in px]
    for y in rows:
        for x in range(w):
            c = px[y][x]
            if not c or c == BLACK:
                continue
            open_ = any(not (0 <= yy < h and 0 <= xx < w) or not px[yy][xx]
                        for yy, xx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)))
            if open_ and c in H_GI:
                out[y][x] = 0x71
            elif open_ and c in H_SKIN - {0x17}:
                out[y][x] = 0x17
    return out


def bristle(px, face_row):
    """The hold frame: the afro's top and back edge gets a ragged black fringe (every other column),
    above the face only."""
    h, w = len(px), len(px[0])
    out = [r[:] for r in px]
    for y in range(h):
        for x in range(w):
            if px[y][x] or y >= face_row or (x + y) % 2:
                continue
            if any(0 <= yy < h and 0 <= xx < w and px[yy][xx] == BLACK and yy < face_row
                   for yy, xx in ((y + 1, x), (y, x - 1), (y, x + 1))):
                out[y][x] = BLACK
    return out


def face_row(px):
    """First row holding Hercule's skin (the top of the face; the afro is above it)."""
    for y, r in enumerate(px):
        if any(c in H_SKIN for c in r):
            return y
    return len(px) // 3


def compose(sp, d, spec, image):
    gi, fi = spec["base"]
    xo, yo, w, h, _, base = display(sp, HERCULE, gi, d, fi)
    gxo, gyo, gw, gh, _, goku = display(sp, GOKU, BEAM_GROUP, d, image)
    assert (gw, gh) == (w, h), (d, w, h, gw, gh)
    px = [r[:] for r in base]
    for r0, r1, c0, c1, colours in spec["erase"]:
        for y in range(r0, r1 + 1):
            for x in range(c0, c1 + 1):
                if colours is None or px[y][x] in colours:
                    px[y][x] = 0
    touched = set()
    for (r0, r1, c0, c1), (dy, dx), rule in spec["arms"][image]:
        for y in range(r0, r1 + 1):
            for x in range(c0, c1 + 1):
                c = arm_colour(goku[y][x], x, rule)
                ty, tx = y + dy, x + dx
                if c is not None and 0 <= ty < h and 0 <= tx < w:
                    px[ty][tx] = c
                    touched.add(ty)
    if image in spec.get("patch", {}):
        r0, art = spec["patch"][image]
        for y, line in enumerate(art):
            for x, ch in enumerate(line):
                if ch != ".":
                    px[r0 + y][x] = PATCH_COLOURS[ch]
                    touched.add(r0 + y)
    rows = sorted(touched | {y for r0, r1, *_ in spec["erase"] for y in range(r0, r1 + 1)})
    px = outline(px, rows)
    return (gxo, gyo, w, h), px


def build(sp):
    """Group 18 per facing: 7 (xo, yo, w, h, attr, pixels) frames; pixels are stored (unflipped)."""
    specs = {0: FRONT, 1: BACK, 2: SIDE}
    images = {}
    for d, spec in specs.items():
        box, charge = compose(sp, d, spec, 0)
        _, thrust = compose(sp, d, spec, 1)
        hold = bristle(thrust, face_row(thrust)) if d != 1 else bristle(thrust, 16)
        images[d] = (box, [charge, thrust, hold])
    g = sp.groups(sp.record(GOKU))[BEAM_GROUP]
    dirs = []
    for d in range(4):
        src = 2 if d == 3 else d
        (xo, yo, w, h), imgs = images[src]
        attr = sp.descriptor(g[d], 0)[4]        # Goku's attributes (shape/size; h-flip on the right)
        frames = []
        for fi in range(FRAMES):
            img = imgs[min(fi, 2)]
            frames.append((xo, yo, w, h, attr, A.hflip(img) if attr & HFLIP else img))
        dirs.append(frames)
    return dirs


# ---- output -----------------------------------------------------------------------------------------
def write_c(path, sp, group18):
    rom = sp.rom
    rec = sp.record(HERCULE)
    header = rom[rec - S.ROM_BASE:rec - S.ROM_BASE + 0x4C]
    stock = sp.groups(rec)
    assert len(stock) == header[2] == 21 and not any(stock[BEAM_GROUP]), "record 72 changed?"
    lines = ["/* generated by tools/herculesprites.py from the ROM: Hercule's record 72 with the Kamehameha",
             " * pose (group 18). Build output only; never commit. */",
             '#include "rom.h"', "",
             "typedef struct { signed char xo, yo; u8 w, h; u32 attr; const void *blob; } HcDesc;", ""]
    blobs, nbytes = {}, 0
    arrs = []
    for d, frames in enumerate(group18):
        ents = []
        for xo, yo, w, h, attr, px in frames:
            stored = A.hflip(px) if attr & HFLIP else px
            data = A.tiles(stored, w, h)
            if data not in blobs:
                blob = compress(data)
                assert m.decompress(blob, 8, 1)[:len(data)] == data, "LZ round trip"
                name = f"herc_blob_{len(blobs)}"
                blobs[data] = name
                nbytes += len(blob)
                lines.append(f"static const u8 {name}[{len(blob)}] __attribute__((aligned(4))) = "
                             f"{{{', '.join(f'0x{b:02x}' for b in blob)}}};")
            ents.append(f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {blobs[data]}}}")
        name = f"herc_g18_d{d}"
        lines.append(f"static const HcDesc {name}[{len(frames)}] = {{{', '.join(ents)}}};")
        arrs.append(name)
    grp = []
    for gi, g in enumerate(stock):
        if gi == BEAM_GROUP:
            grp.append("{" + ", ".join(arrs) + "}")
        else:
            grp.append("{" + ", ".join(f"(const HcDesc *)0x{p:08x}u" for p in g) + "}")
    hdr = ", ".join(f"0x{b:02x}" for b in header)
    lines.append(f"typedef struct {{ u8 header[0x4C]; const HcDesc *groups[{len(stock)}][4]; u32 end; }} HcRecord;")
    lines.append(f"const HcRecord herc_record = {{{{{hdr}}}, {{{', '.join(grp)}}}, 0}};")
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    open(path, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    return len(blobs), nbytes


def preview(path, sp, group18):
    """Group 18, 4 facings x 3 images (charge, thrust, hold): Goku, Gohan (59), Hercule (new), and
    Hercule's stock base frames; also a _x4 copy."""
    cell = 36
    rows = []
    for sid in REFS:
        g = sp.groups(sp.record(sid))[BEAM_GROUP]
        rows.append((sid, [[display(sp, sid, BEAM_GROUP, d, fi) for fi in range(3)] for d in range(4)]))
    rows.append((HERCULE, [[(xo, yo, w, h, attr, A.hflip(px) if attr & HFLIP else px)
                            for xo, yo, w, h, attr, px in frames[:3]] for frames in group18]))
    base = [[display(sp, HERCULE, *({0: FRONT, 1: BACK}.get(d, SIDE)["base"][:1]), d, 0)] for d in range(4)]
    rows.append((HERCULE, base))
    cv = S.Canvas(16 + 4 * (3 * cell + 6), len(rows) * (cell + 4) + 4, (60, 60, 90))
    for ri, (sid, dirs) in enumerate(rows):
        y0 = 2 + ri * (cell + 4)
        cv.text(1, y0 + 2, str(sid), (255, 255, 0))
        for d, frames in enumerate(dirs):
            for fi, (xo, yo, w, h, attr, px) in enumerate(frames):
                x0 = 16 + d * (3 * cell + 6) + fi * cell + (8 + xo)
                for y, line in enumerate(px):
                    for x, v in enumerate(line):
                        if v:
                            cv.put(x0 + x, y0 + 2 + y, sp.pal[v])
    cv.save(path)
    try:
        from PIL import Image
        im = Image.open(path)
        im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(path.replace(".png", "_x4.png"))
    except ImportError:
        pass


def main():
    rom = open(sys.argv[1], "rb").read()
    out_c = sys.argv[2]
    pdir = sys.argv[sys.argv.index("--preview-dir") + 1] if "--preview-dir" in sys.argv else None
    sp = S.Sprites(rom)
    g18 = build(sp)
    n, nbytes = write_c(out_c, sp, g18)
    if pdir:
        os.makedirs(pdir, exist_ok=True)
        preview(os.path.join(pdir, "hercule_beam_preview.png"), sp, g18)
    print(f"wrote {out_c}: herc_record (sprite 72 + group 18), {n} frames, {nbytes} bytes")


if __name__ == "__main__":
    main()
