"""Hercule's Kamehameha pose (group 18 of sprite 72), built from the ROM at build time
(docs/sprites.md "Hercule's beam pose"). Emits C source compiled into the co-op ROM; nothing from the ROM
is committed.

usage: herculesprites.py roms/log2.gba build/gen/hercule_sprites_data.c [--buu roms/buu.gba|none] [--preview-dir build/x]

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
Groups 22 and 23 are the Gun's and the Ki Blast's poses (hercule.c points the ki blast's state at them): the Gun from
Buu's Fury's Hercule (record 128 anim 3, the same drawing as 72; --buu, else group 5's peace sign), the Ki Blast a push
built from his own frames (see "the Gun" below; docs/sprites.md "Hercule's shooting poses"); the record has 24 groups.
Group 9 (the Super Saiyan transform's power-up, tools/ssjhercule.py) and ability 18's HUD icons
(herc_ssj_icon_0 / _1) are emitted here too (docs/sprites.md "Super Saiyan Hercule: the transformation").
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
import ssjhercule as SSJ  # noqa: E402

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


def arm_colour(c, col, rule, extra=None):
    """Goku's arm pixel -> Hercule's colour (None: not an arm pixel). `extra`: more Goku colours,
    {colour: (as hand, as sleeve)} (the ki pose's arms use his darker skin outline and the bright band)."""
    kind, wrist = rule
    if extra and c in extra:
        hand = kind is None or (col < wrist if kind == "lt" else col > wrist)
        return extra[c][0 if hand else 1]
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


def compose(sp, d, spec, image, group=BEAM_GROUP):
    gi, fi = spec["base"]
    xo, yo, w, h, _, base = display(sp, HERCULE, gi, d, fi)
    gxo, gyo, gw, gh, _, goku = display(sp, GOKU, group, d, image)
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
                c = arm_colour(goku[y][x], x, rule, spec.get("extra"))
                ty, tx = y + dy, x + dx
                if c is not None and 0 <= ty < h and 0 <= tx < w:
                    px[ty][tx] = c
                    touched.add(ty)
    for r0, r1, c0, c1, colours in spec.get("erase_after", {}).get(image, []):
        for y in range(r0, r1 + 1):
            for x in range(c0, c1 + 1):
                if colours is None or px[y][x] in colours:
                    px[y][x] = 0
                    touched.add(y)
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


# ---- the flight, seen from the side (group 12) -------------------------------------------------------
FLY_GROUP = 12
# FlyToPosition (script step 54, world-map pads, any scene that flies player 1) plays group 12 (docs/engine.md). The
# stock record 72 draws it from the front and the back only (head, shoulders and fists toward the viewer, the soles of
# his boots glinting from behind) and shows the front view when he flies left or right. The side view is built from
# his own frames, as Goku's (60) is laid out: his lying frame (the knock-down's last, on his front, arms out ahead)
# lifted to Goku's flying height, its head (turned down into the ground) replaced by his standing side head looking
# where he flies, and the stock frames' glints trailing from his boots.
FLY_LIFT = 8                     # rows the lying body is raised (Goku's flight sits rows 7-23 of the 32 x 32 frame)
FLY_HEAD = (14, 27, 6, 21)       # the lying frame's head: rows, columns (afro and the face under it)
FLY_FACE = (22, 27, 8, 14)
FLY_HEAD_ROWS = 14               # the idle side frame's head: rows 0-13 (the chin is row 13)
FLY_HEAD_AT = (14, 5)            # where it goes on the lying frame: row, column shift
GLINTS = [                       # per frame: (row, column, colour) on the lifted frame; the stock frames' own
    [(15, 29, 0x24), (14, 29, 0x24), (16, 29, 0x24), (15, 28, 0x24), (15, 30, 0x24),
     (12, 25, 0x24), (11, 25, 0x24), (13, 25, 0x24), (12, 24, 0x24), (12, 26, 0x24)],
    [(14, 30, 0x24), (13, 30, 0x27), (15, 30, 0x27), (14, 29, 0x27), (14, 31, 0x27),
     (11, 26, 0x24), (10, 26, 0x27), (12, 26, 0x27), (11, 25, 0x27), (11, 27, 0x27)],
    [(12, 31, 0x21), (9, 27, 0x21)],
    [(10, 31, 0x23), (7, 28, 0x23)],
]


def fly_side(sp):
    """Group 12 facing left: 4 frames of display pixels (32 x 32)."""
    _, _, w, h, _, lying = display(sp, HERCULE, 3, 2, 3)
    _, _, hw, _, _, idle = display(sp, HERCULE, 0, 2, 0)
    assert (w, h, hw) == (32, 32, 16), "record 72's frames changed?"
    px = [r[:] for r in lying]
    r0, r1, c0, c1 = FLY_HEAD
    for y in range(r0, r1 + 1):                      # the afro, and the face turned into the ground
        for x in range(c0, c1 + 1):
            if px[y][x] == BLACK:
                px[y][x] = 0
    r0, r1, c0, c1 = FLY_FACE
    for y in range(r0, r1 + 1):
        for x in range(c0, c1 + 1):
            if px[y][x] not in H_GI:
                px[y][x] = 0
    hy, hx = FLY_HEAD_AT
    for y in range(FLY_HEAD_ROWS):
        for x in range(hw):
            if idle[y][x]:
                px[hy + y][hx + x] = idle[y][x]
    px = px[FLY_LIFT:] + [[0] * w for _ in range(FLY_LIFT)]
    assert not any(any(r) for r in lying[:FLY_LIFT]), "the lying frame reaches the lifted rows"
    frames = []
    for glints in GLINTS:
        f = [r[:] for r in px]
        for y, x, c in glints:
            if not f[y][x]:
                f[y][x] = c
        frames.append(f)
    return frames


# ---- the Gun (group 22) and the Ki Blast's push (group 23) ----------------------------------------------
# Both abilities run the ki blast's control state (animation 34, hercule.c): its enter points the entity at group 5
# (record + 0x9C, the peace sign on 72 / 157) and its update shows frame 0 for 8 frames, frame 1 from the shot (the
# blast leaves 8 frames in) and, for each extra shot of a held fire, frames 1 and 2 in turn. hercule.c then points the
# entity at group 22 (the Gun) or 23 (the Ki Blast) when the record has it. Frames: 0 the wind-up, 1 the shot, 2 the
# next shot.
GUN_GROUP, KI_GROUP = 22, 23
GROUP_COUNT = 24                 # herc_record / ssj_record's group count (21 stays empty in herc_record)
BUU_HERCULE, BUU_GUN_ANIM = 128, 3   # Buu's Fury Hercule (record 128, the same drawing as 72's); anim 3 = the gun:
#                                  frame 0 the hand at the holster, 1 aimed, 2 fired (the gun kicks up, the flash)
FLASH_NEXT = {0x24: 0x25, 0x25: 0x26, 0x26: 0x27, 0x77: 0x24}   # the muzzle flash one step hotter: the next shot
SHADOW = 1


def add_shadow(px):
    """LoG2's ground shadow under the feet (Buu's Fury draws none; every frame of 72 has one): a black band from 3 px
    outside the feet, over the boots' last rows and one row under them, only on transparent pixels."""
    h, w = len(px), len(px[0])
    bot = max(y for y in range(h) if any(px[y]))
    cols = [x for y in range(max(0, bot - 2), bot + 1) for x in range(w) if px[y][x]]
    left, right = min(cols) - 3, max(cols) + 3
    out = [r[:] for r in px]
    for y, inset in ((bot - 2, 3), (bot - 1, 1), (bot, 0), (bot + 1, 1)):
        if 0 <= y < h:
            for x in range(max(0, left + inset), min(w, right - inset + 1)):
                if not out[y][x]:
                    out[y][x] = SHADOW
    return out


def gun_frames(sp, buu_rom, shadow=True):
    """Group 22 per facing (down, up, left, right): 3 frames (xo, yo, w, h, attr, display pixels) from Buu's Fury's
    Hercule gun anim in LoG2's palette: 0 aimed, 1 fired, 2 fired with the flash one step hotter. Right is the left
    blob h-flipped, as LoG2's. `shadow`: add LoG2's ground shadow (72 and 157; old Hercule's 158 has none)."""
    import buu_sprites as B
    from buu2log2 import nearest_table
    bu = B.BuuSprites(buu_rom)
    rec = bu.record(BUU_HERCULE)
    anim = bu.anim(rec["anims"][BUU_GUN_ANIM])
    assert len(anim) == 3, "Buu's Fury Hercule's gun anim changed?"
    table = nearest_table(bu.pal, sp.pal)
    dirs = []
    for d in range(3):
        imgs = []
        for f in (1, 2):
            box, parts = bu.descriptor(anim[f][d])
            assert len(parts) == 1 and not parts[0][4] & 0x30000000, "a one-part, unflipped frame"
            x, y, w, h, attr, _ = parts[0]
            (_, _, _, _), raw = bu.frame_pixels(anim[f][d], rec["remap"])
            px = [[table[c][0] if c else 0 for c in r] for r in raw]
            imgs.append((x, y, w, h, attr, add_shadow(px) if shadow else px))
        hot = [[FLASH_NEXT.get(c, c) for c in r] for r in imgs[1][5]]
        dirs.append([imgs[0], imgs[1], imgs[1][:5] + (hot,)])
    dirs.append([(x, y, w, h, attr | HFLIP, A.hflip(px)) for x, y, w, h, attr, px in dirs[2]])
    return dirs


# The Ki Blast: a push. Seen from the side, Goku's ki-blast pose (his group 5: the state that plays it) re-posed on
# Hercule's body the way the beam pose is (compose): frame 0 the wind-up is his own guard (the lunge, the back fist
# chambered), frames 1 / 2 the front arm thrust out with the palm open (Goku's frame 2 arm in Hercule's colours: the
# hand his skin, from the wrist band up his brown gi sleeve; Goku's skin outline 0x15 and bright band 0x4F are not in the
# beam pose's maps). From the front and the back a one-handed thrust is an arm foreshortened toward the viewer, which in
# Hercule's colours reads as a tan block (the beam pose's finding); there his beam pose's own two-handed push is used,
# the charge for the wind-up and the thrust (without the hold's bristling afro) for the shot.
KI_EXTRA = {0x15: (0x17, 0x71), 0x4F: (0x6F, 0x6F)}
KI_SIDE = {
    "base": (7, 0),
    "erase": [(15, 21, 13, 17, H_SKIN)],                              # the front fist at the waist
    "arms": {2: [((12, 19, 0, 15), (-1, 4), ("lt", 7))]},
    "extra": KI_EXTRA,
}


def ki_frames(sp):
    """Group 23 per facing: 3 frames (xo, yo, w, h, attr, display pixels)."""
    out = {}
    for d, spec in ((0, FRONT), (1, BACK)):
        box, charge = compose(sp, d, spec, 0)
        _, thrust = compose(sp, d, spec, 1)
        out[d] = (box, [charge, thrust, thrust])
    xo, yo, w, h, _, guard = display(sp, HERCULE, *KI_SIDE["base"][:1], 2, KI_SIDE["base"][1])
    _, push = compose(sp, 2, KI_SIDE, 2, group=5)
    out[2] = ((xo, yo, w, h), [guard, push, push])
    g = sp.groups(sp.record(GOKU))[BEAM_GROUP]
    dirs = []
    for d in range(4):
        (xo, yo, w, h), imgs = out[2 if d == 3 else d]
        attr = sp.descriptor(g[d], 0)[4]        # Goku's attributes (shape/size; h-flip on the right)
        dirs.append([(xo, yo, w, h, attr, A.hflip(px) if attr & HFLIP else px) for px in imgs])
    return dirs


def write_frames(lines, blobs, name, frames, counter):
    """Emit a descriptor array; frames are (xo, yo, w, h, attr, display pixels), blobs stored unflipped and shared."""
    ents, nbytes = [], 0
    for xo, yo, w, h, attr, px in frames:
        data = A.tiles(A.hflip(px) if attr & HFLIP else px, w, h)
        if data not in blobs:
            blob = compress(data)
            assert m.decompress(blob, 8, 1)[:len(data)] == data, "LZ round trip"
            blobs[data] = f"{counter}{len(blobs)}"
            nbytes += len(blob)
            lines.append(f"static const u8 {blobs[data]}[{len(blob)}] __attribute__((aligned(4))) = "
                         f"{{{', '.join(f'0x{b:02x}' for b in blob)}}};")
        ents.append(f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {blobs[data]}}}")
    lines.append(f"static const HcDesc {name}[{len(ents)}] = {{{', '.join(ents)}}};")
    return nbytes


# ---- output -----------------------------------------------------------------------------------------
def write_c(path, sp, group18, gun=None):
    """gun: gun_frames() (None without Buu's Fury: group 22 is then group 5's arrays, the peace sign as before)."""
    rom = sp.rom
    rec = sp.record(HERCULE)
    header = rom[rec - S.ROM_BASE:rec - S.ROM_BASE + 0x4C]
    stock = sp.groups(rec)
    assert len(stock) == header[2] == 21 and not any(stock[BEAM_GROUP]), "record 72 changed?"
    assert not any(stock[SSJ.POWERUP_GROUP]), "record 72 has a group 9 now?"
    lines = ["/* generated by tools/herculesprites.py from the ROM: Hercule's record 72 with the Kamehameha",
             " * pose (group 18) and the flight seen from the side (group 12). Build output only; never commit. */",
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
    # group 12 facing left / right: the flight from the side (the right mirrors the left blob, as Goku's)
    gattr = sp.descriptor(sp.groups(sp.record(GOKU))[FLY_GROUP][2], 0)[4] & ~HFLIP
    fly = []
    for px in fly_side(sp):
        data = A.tiles(px, 32, 32)
        if data not in blobs:
            blob = compress(data)
            assert m.decompress(blob, 8, 1)[:len(data)] == data, "LZ round trip"
            blobs[data] = f"herc_blob_{len(blobs)}"
            nbytes += len(blob)
            lines.append(f"static const u8 {blobs[data]}[{len(blob)}] __attribute__((aligned(4))) = "
                         f"{{{', '.join(f'0x{b:02x}' for b in blob)}}};")
        fly.append(blobs[data])
    for nm, at in (("herc_g12_l", gattr), ("herc_g12_r", gattr | HFLIP)):
        lines.append(f"static const HcDesc {nm}[{len(fly)}] = {{" +
                     ", ".join(f"{{-8, -24, 32, 32, 0x{at:08x}u, {b}}}" for b in fly) + "};")
    # group 9: the Super Saiyan transform's power-up (tools/ssjhercule.py), one array for all four facings
    ents = []
    for xo, yo, w, h, attr, px in SSJ.powerup_frames(sp):
        data = A.tiles(px, w, h)
        if data not in blobs:
            blob = compress(data)
            assert m.decompress(blob, 8, 1)[:len(data)] == data, "LZ round trip"
            blobs[data] = f"herc_blob_{len(blobs)}"
            nbytes += len(blob)
            lines.append(f"static const u8 {blobs[data]}[{len(blob)}] __attribute__((aligned(4))) = "
                         f"{{{', '.join(f'0x{b:02x}' for b in blob)}}};")
        ents.append(f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {blobs[data]}}}")
    lines.append(f"static const HcDesc herc_g9[{len(ents)}] = {{{', '.join(ents)}}};")
    # ability 18's HUD icons (ready, cooling): the Transformation icon in Hercule's colours (ssjhercule.py)
    for i, ic in enumerate(SSJ.icons(rom)):
        lines.append(f"const u8 herc_ssj_icon_{i}[0x200] __attribute__((aligned(4))) = "
                     f"{{{', '.join(f'0x{b:02x}' for b in ic)}}};")
    # ability 25's: the Gun (ssjhercule.py)
    for i, ic in enumerate(SSJ.gun_icons(rom)):
        lines.append(f"const u8 herc_gun_icon_{i}[0x200] __attribute__((aligned(4))) = "
                     f"{{{', '.join(f'0x{b:02x}' for b in ic)}}};")
    grp = []
    for gi, g in enumerate(stock):
        if gi == BEAM_GROUP:
            grp.append("{" + ", ".join(arrs) + "}")
        elif gi == SSJ.POWERUP_GROUP:
            grp.append("{herc_g9, herc_g9, herc_g9, herc_g9}")
        elif gi == FLY_GROUP:
            assert all(g) and all(sp.frame_counts(stock)[q] == 4 for q in g), "record 72's group 12 changed?"
            grp.append("{" + ", ".join(f"(const HcDesc *)0x{q:08x}u" for q in g[:2]) + ", herc_g12_l, herc_g12_r}")
        else:
            grp.append("{" + ", ".join(f"(const HcDesc *)0x{p:08x}u" for p in g) + "}")
    # groups 21 (empty: the turn back is the transformed record's), 22 the Gun, 23 the Ki Blast's push
    grp.append("{0, 0, 0, 0}")
    if gun:
        names = [f"herc_gun_d{d}" for d in range(4)]
        for nm, frames in zip(names, gun):
            nbytes += write_frames(lines, blobs, nm, frames, "herc_blob_g")
        grp.append("{" + ", ".join(names) + "}")
    else:
        grp.append("{" + ", ".join(f"(const HcDesc *)0x{p:08x}u" for p in stock[5]) + "}")
    names = [f"herc_ki_d{d}" for d in range(4)]
    for nm, frames in zip(names, ki_frames(sp)):
        nbytes += write_frames(lines, blobs, nm, frames, "herc_blob_k")
    grp.append("{" + ", ".join(names) + "}")
    assert len(grp) == GROUP_COUNT
    header = bytearray(header)
    header[2] = GROUP_COUNT
    hdr = ", ".join(f"0x{b:02x}" for b in header)
    lines.append(f"typedef struct {{ u8 header[0x4C]; const HcDesc *groups[{GROUP_COUNT}][4]; u32 end; }} HcRecord;")
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


def shoot_preview(path, sp, gun):
    """Groups 5 (the peace sign, what both moves showed before), 22 (the Gun) and 23 (the Ki Blast), four facings x
    three frames each, on a ground cross at the entity's position; also a _x4 copy."""
    rows = [("5", [[display(sp, HERCULE, 5, d, fi) for fi in range(3)] for d in range(4)])]
    if gun:
        rows.append(("22", gun))
    rows.append(("23", ki_frames(sp)))
    cell = 40
    cv = S.Canvas(24 + 4 * (3 * cell + 6), len(rows) * (cell + 4) + 4, (70, 90, 70))
    for ri, (name, dirs) in enumerate(rows):
        y0 = 2 + ri * (cell + 4)
        cv.text(1, y0 + 2, name, (255, 255, 0))
        for d, frames in enumerate(dirs):
            for fi, (xo, yo, w, h, attr, px) in enumerate(frames[:3]):
                ox, oy = 24 + d * (3 * cell + 6) + fi * cell + 12, y0 + 30
                cv.put(ox, oy, (140, 160, 140))
                for y, line in enumerate(px):
                    for x, v in enumerate(line):
                        if v:
                            cv.put(ox + xo + x, oy + yo + y, sp.pal[v])
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
    buu = sys.argv[sys.argv.index("--buu") + 1] if "--buu" in sys.argv else "none"
    sp = S.Sprites(rom)
    g18 = build(sp)
    gun = gun_frames(sp, open(buu, "rb").read()) if buu != "none" else None
    n, nbytes = write_c(out_c, sp, g18, gun)
    if pdir:
        os.makedirs(pdir, exist_ok=True)
        preview(os.path.join(pdir, "hercule_beam_preview.png"), sp, g18)
        SSJ.preview(os.path.join(pdir, "ssj_hercule_transform_preview.png"), rom)
        shoot_preview(os.path.join(pdir, "hercule_shoot_preview.png"), sp, gun)
    print(f"wrote {out_c}: herc_record (sprite 72 + groups 18 and 9, 12's sides, 22 the Gun{'' if gun else ' = 5'}, "
          f"23 the Ki Blast), {n} frames, {nbytes} bytes; "
          "herc_ssj_icon_0/1")


if __name__ == "__main__":
    main()
