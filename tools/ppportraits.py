"""Dialogue portraits for the tournament fighters that had none (docs/tournament.md 24.2, docs/ids.md "Portraits"
169-172): Android 13, Cell Jr., the Namekian Warrior and Evil Buu; and 173 the mother dinosaur of the generator's eggs
(data/story/ch4_eggs.lscript). Each one is a stock 64x64 portrait of the body the sprite was made from, repainted the
way that sprite was (so face and body agree):

  a13     drawn, not a repaint (tools/drawnportraits.py, 2026-11-12): the movie's three-quarter face under the trucker cap
  celljr  Perfect Cell's portrait (40; Cell Jr. is a small Perfect Cell): the green crest blue with dark-blue
          spots, the whites and greys blue-tinted, the gold jaw and purple marks kept, an orange background
  namek   Piccolo's portrait (94, no turban; the Namekian Warrior's sprite is Piccolo's capeless set recoloured,
          tools/bossmodels.py namek_colours): the skin his darker teal-green, the gi maroon; the purple background
          every LoG2 Namekian portrait has (46, 69, 85, 90, 94)
  evilbuu Buu's Fury has no Evil Buu portrait (all 155 entries of its module 0x086F393C checked): its Super Buu
          (127, on LoG2's frame as tools/oldherculeportrait.py does) with grey skin, red eyes and a dark blue
          background (his cape's). LoG2's Hercule (67) without the Buu's Fury ROM, as every Buu import's portrait.
  dino    King Triceratops' own portrait (96, the bestiary's Triceratops King; her sprite 245 is his 127 recoloured,
          tools/dinosprites.py): the hide's browns onto that tool's ramps, the frill above the brow (the rows down
          to just under the horns) pink and the face orange, as the sprite's head is (its green is the feet, not in
          the picture); horns, eyes and the red background kept

The plan's first recipe (the sprite's face, 32x32 scaled x2, on the stock frame) was built and compared beside the
stock portraits (--compare): the idle frame's top 32x32 holds these small bodies whole (heads of 10-12 px), drawn in
2x2 blocks next to the stock portraits' hand-drawn 64 px faces, so the repaints are used; the sheet shows both.

Emits C (`pp_portrait_<id>`, compressed 64x64 blobs in the second ROM blob) for krillinsprites.py --extra-portrait.
Build output only; never commit.

    python -I tools/ppportraits.py roms/log2.gba OUT.c --buu roms/buu.gba|none [--check BUILT.gba]
        [--compare BUILT.gba coop.sym SHEET.png] 169=a13 170=celljr 171=namek 172=evilbuu 173=dino
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mapchunks as m  # noqa: E402
import sprites as S  # noqa: E402
from buu2log2 import compress  # noqa: E402
from oldherculeportrait import build as buu_portrait, LOG_TABLE, LOG_HERCULE, B  # noqa: E402
import drawnportraits as drawn  # noqa: E402

W = 64
IN = range(3, W - 3)                       # inside LoG2's 3 px grey frame
SOURCE = {"a13": 95, "celljr": 40, "namek": 94, "evilbuu": ("buu", 127), "dino": 96}
NAMES = {"a13": "Android 13", "celljr": "Cell Jr.", "namek": "Namekian Warrior", "evilbuu": "Evil Buu",
         "dino": "Mother Dinosaur"}
SPRITE = {"a13": 165, "celljr": 31, "namek": 166, "evilbuu": 203, "dino": 245}   # their bodies (tourney_data.c, Makefile)
PAL = None


def near(rgb, avoid=(0,)):
    """The LoG2 OBJ palette index nearest to rgb."""
    return min((i for i in range(256) if i not in avoid),
               key=lambda i: sum((a - b) ** 2 for a, b in zip(PAL[i], rgb)))


def stock(log, i):
    return bytearray(m.unpack(log, struct.unpack_from("<I", log, LOG_TABLE - B + 4 * i)[0])[:4096])


def flood(pic, colours, seeds):
    """The pixels reached from `seeds` through `colours`, inside the frame."""
    seen, stack = set(), list(seeds)
    while stack:
        y, x = stack.pop()
        if (y, x) in seen or y not in IN or x not in IN or pic[y * W + x] not in colours:
            continue
        seen.add((y, x))
        stack += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    return seen


def top_seeds():
    return [(3, x) for x in IN] + [(y, 3) for y in range(3, 40)] + [(y, W - 4) for y in range(3, 40)]


def remap(pic, table, where=None):
    for y in IN:
        for x in IN:
            if where is not None and (y, x) not in where:
                continue
            c = pic[y * W + x]
            if c in table:
                pic[y * W + x] = table[c]


# ---- Android 13 -----------------------------------------------------------------------------------------------
# Drawn since 2026-11-12 (tools/drawnportraits.py): the movie's face under his trucker cap. Until then it was Tien's portrait (95)
# repainted (a cap, white hair), before that Android 16's (26): Ryan had "complaints about andriod 13's portrait being teins recycled".
def a13(log):
    return bytes(drawn.a13(log))


# ---- Cell Jr. -------------------------------------------------------------------------------------------------
def celljr(log):
    pic = stock(log, 40)
    bg = flood(pic, {31, 30, 252, 25}, top_seeds() + [(y, x) for y in range(40, 50) for x in (3, W - 4)])
    remap(pic, {31: near((255, 180, 41)), 30: near((255, 213, 65)), 252: near((255, 148, 24)),
                25: near((213, 98, 0))}, bg)
    body = {(y, x) for y in IN for x in IN} - bg
    remap(pic, {
        129: near((150, 205, 250)), 132: near((60, 140, 230)), 42: near((32, 98, 172)), 167: near((32, 32, 123)),
        2: near((235, 245, 255)), 43: near((235, 245, 255)), 9: near((205, 225, 250)), 10: near((180, 200, 240)),
        11: near((150, 175, 225)), 12: near((123, 150, 205)), 13: near((90, 115, 180)), 14: near((60, 80, 150)),
        15: near((40, 50, 110)),
    }, body)
    return bytes(pic)


# ---- the Namekian Warrior -------------------------------------------------------------------------------------
def namek(log):
    pic = stock(log, 94)
    bg = flood(pic, {233, 225, 92, 91, 90, 95}, top_seeds())
    skin = {153: near((140, 220, 175)), 48: near((90, 189, 140)), 49: near((66, 164, 120)), 57: near((49, 148, 106)),
            155: near((40, 130, 95)), 50: near((16, 98, 74)), 58: near((16, 98, 74)), 167: near((16, 70, 57))}
    gi = {233: near((172, 41, 49)), 225: near((205, 74, 82)), 92: near((205, 74, 82)), 91: near((230, 98, 115)),
          90: near((230, 98, 115)), 95: near((150, 30, 40)), 97: near((90, 8, 24))}
    body = {(y, x) for y in IN for x in IN} - bg
    remap(pic, skin, body)
    remap(pic, gi, {p for p in body if p[0] >= 44})
    return bytes(pic)


# ---- Evil Buu -------------------------------------------------------------------------------------------------
def evilbuu(log, buu):
    if buu is None:
        return bytes(stock(log, LOG_HERCULE))
    pic = bytearray(buu_portrait(log, buu, 127))
    bg = flood(pic, {5, 24, 25, 27, 30}, top_seeds() + [(y, x) for y in range(40, 60) for x in (3, W - 4)])
    remap(pic, {5: near((57, 32, 160)), 24: near((82, 57, 205)), 25: near((41, 24, 123)), 27: near((24, 8, 82)),
                30: near((110, 90, 220))}, bg)
    body = {(y, x) for y in IN for x in IN} - bg
    remap(pic, {106: near((200, 198, 190)), 107: near((165, 162, 150)), 248: near((125, 123, 118)),
                30: near((150, 148, 140)), 105: near((80, 80, 80)), 249: near((50, 50, 60)), 27: near((40, 40, 50)),
                43: near((230, 230, 225)), 2: near((235, 235, 235)),
                33: near((255, 24, 57)), 34: near((213, 16, 41)), 35: near((123, 0, 0)), 113: near((123, 0, 0))}, body)
    return bytes(pic)


# ---- the mother dinosaur ----------------------------------------------------------------------------------------
DINO_BROW = 21          # the first row under the horns: the frill above it pink, the face from it orange


def dino(log):
    import dinosprites as D
    pic = stock(log, 96)
    maps = {name: {src: D.nearest(PAL, c) for src, c in zip(D.HIDE, ramp)} for name, ramp in D.RAMPS.items()}
    assert any(pic[y * W + x] in D.HIDE for y in IN for x in IN), "portrait 96 has no hide browns"
    for y in IN:
        m = maps["pink"] if y < DINO_BROW else maps["orange"]
        for x in IN:
            pic[y * W + x] = m.get(pic[y * W + x], pic[y * W + x])
    return bytes(pic)


def make(kind, log, buu):
    if kind == "evilbuu":
        return evilbuu(log, buu)
    return {"a13": a13, "celljr": celljr, "namek": namek, "dino": dino}[kind](log)


# ---- the comparison sheet ---------------------------------------------------------------------------------------
def sprite_face(rom_built, sym, sid, log):
    """The plan's first recipe: the idle facing-down frame's head, 32x32 from the top of the body, scaled x2, on the
    stock frame and a plain background (for the comparison only)."""
    import tnysheet as T
    b = T.Built(rom_built, sym)
    g = b.group(b.record(sid), 0)
    _, _, px = b.sp.frame_pixels(g[0], 0)
    rows = [y for y, r in enumerate(px) if any(r)]
    cols = [x for r in px for x, v in enumerate(r) if v]
    top = min(rows)
    head = [x for x, v in enumerate(px[top + 4]) if v] or cols
    cxh = (min(head) + max(head)) // 2
    out = stock(log, 26)
    for y in IN:
        for x in IN:
            out[y * W + x] = near((120, 170, 235))
    for y in range(29):
        for x in range(29):
            sy, sx = top - 2 + y, cxh - 14 + x
            if 0 <= sy < len(px) and 0 <= sx < len(px[0]) and px[sy][sx]:
                for a in range(2):
                    for c in range(2):
                        out[(3 + 2 * y + a) * W + 3 + 2 * x + c] = px[sy][sx]
    return bytes(out)


def compare(path, made, log, rom_built, symf):
    """Each new portrait beside the stock portrait it was made from, the plan's sprite-face recipe and two stock
    portraits of the cast it talks beside (Hercule 67, Goku 63), at 3x."""
    from PIL import Image, ImageDraw
    sym = {}
    for line in open(symf):
        p = line.split()
        if len(p) == 3:
            sym[p[2]] = int(p[0], 16)
    built = open(rom_built, "rb").read()
    Z, gap, lab = 3, 10, 14
    cols = ["source", "new", "plan: sprite x2", "stock 67", "stock 63"]
    im = Image.new("RGB", (len(cols) * (W * Z + gap) + 110, len(made) * (W * Z + gap + lab) + lab), (36, 36, 40))
    d = ImageDraw.Draw(im)
    for c, name in enumerate(cols):
        d.text((110 + c * (W * Z + gap), 2), name, fill=(255, 255, 0))
    for r, (pid, kind, pic, src) in enumerate(made):
        y0 = lab + r * (W * Z + gap + lab)
        d.text((4, y0 + 40), f"{pid}", fill=(255, 255, 0))
        d.text((4, y0 + 56), NAMES[kind], fill=(255, 255, 255))
        imgs = [src, pic, sprite_face(built, sym, SPRITE[kind], log) if kind != "evilbuu" else None,
                bytes(stock(log, 67)), bytes(stock(log, 63))]
        for c, img in enumerate(imgs):
            if img is None:
                continue
            x0 = 110 + c * (W * Z + gap)
            for y in range(W):
                for x in range(W):
                    v = img[y * W + x]
                    if v:
                        d.rectangle((x0 + x * Z, y0 + y * Z, x0 + x * Z + Z - 1, y0 + y * Z + Z - 1), fill=PAL[v])
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    im.save(path)
    print(f"wrote {path}")


def main():
    global PAL
    args = sys.argv[1:]
    rom, out = args[0], args[1]
    opts = {"--buu": "none", "--check": None}
    cmp_args = None
    pairs = []
    i = 2
    while i < len(args):
        if args[i] == "--compare":
            cmp_args = args[i + 1:i + 4]
            i += 4
        elif args[i] in opts:
            opts[args[i]] = args[i + 1]
            i += 2
        else:
            pid, kind = args[i].split("=")
            assert kind in SOURCE, f"unknown portrait {kind}"
            pairs.append((int(pid), kind))
            i += 1
    log = open(rom, "rb").read()
    buu = open(opts["--buu"], "rb").read() if opts["--buu"] != "none" and os.path.exists(opts["--buu"]) else None
    PAL = S.Sprites(log).pal
    made = []
    for pid, kind in pairs:
        src = SOURCE[kind]
        if isinstance(src, tuple):
            src_pic = buu_portrait(log, buu, src[1]) if buu else bytes(stock(log, LOG_HERCULE))
        else:
            src_pic = bytes(stock(log, src))
        made.append((pid, kind, make(kind, log, buu), src_pic))
    if opts["--check"]:
        built = open(opts["--check"], "rb").read()
        word = lambda a: struct.unpack_from("<I", built, a - B)[0]   # noqa: E731
        table = word(0x0800BE10)
        for pid, kind, pic, _ in made:
            got = m.unpack(built, word(table + 4 * (pid + 1)))[:4096]
            assert got == pic, f"portrait {pid} ({kind}) does not load from {opts['--check']}"
        print(f"check: {len(made)} portraits load from the table at {table:#x} in {opts['--check']}")
        return
    if cmp_args:
        compare(cmp_args[2], made, log, cmp_args[0], cmp_args[1])
    text = ["/* GENERATED by tools/ppportraits.py: dialogue portraits for Android 13, Cell Jr., the Namekian Warrior,"
            " Evil Buu and the mother dinosaur" + ("" if buu else " (no Buu's Fury ROM: Evil Buu gets LoG2's Hercule)") +
            ". Build output only; never commit. */", '#include "rom.h"']
    total = 0
    for pid, kind, pic, _ in made:
        blob = compress(pic)
        assert m.decompress(blob, 8, 1)[:4096] == pic
        total += len(blob)
        rows = [", ".join(f"0x{b:02x}" for b in blob[k:k + 16]) for k in range(0, len(blob), 16)]
        text.append(f"const u8 pp_portrait_{pid}[{len(blob)}] __attribute__((aligned(4), section(\".rodata.art\"))) = "
                    "{\n    " + ",\n    ".join(rows) + "\n};")
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    open(out, "w", encoding="utf-8", newline="\n").write("\n".join(text) + "\n")
    print(f"wrote {out}: {len(made)} portraits, {total} bytes")


if __name__ == "__main__":
    main()
