"""The tournament's playable LoG2 bodies (docs/tournament.md 9a "playables"): player-layout sprite records
for the fighters that exist only as enemy sets, and the display rows 29-79 of every tournament playable.
Nothing from the ROM is committed: the output lives in build/gen.

usage: tnyplayables.py roms/log2.gba build/gen/tny_play_sprites_data.c [--preview-dir build/x]
       (tools/spriteclone.py imports display_rows() for rows 29-79)

Player records (sprite ids 235-244, src/coop/tourney.h TS_P_*): the Android 16 player recipe of
tools/a16sprites.py a16p_record. Goku's playable header (sprite 60: the player collision boxes, 21 groups),
the groups a player state plays filled the way Hercule's stock set is (0-8, 11, 12, 13, 17, 20; 9, 10, 14-16,
18, 19 empty) from the body's enemy groups (PLAYER_FROM: the first source group the body has), every
descriptor array padded to the frame count Goku's own array has (the engine plays a fixed number of frames
per state and reads past a short array). Frames stay the ROM's: a new array only holds descriptors that
point at the stock frame blobs, so a record costs a few hundred bytes. Exceptions:
  * Android 13 (240) is the generated boss model a13_record (tools/bossmodels.py build_a13): its frames are
    rebuilt here and emitted again (TNY_ART), since that file's arrays are static.
  * Mecha Frieza (49) has no hit-stun and no attacks: those come from Frieza's set (48), the same body; his own
    group 3 is his death (Trunks' sword cuts him in two), so the knock-down, lying and tumble are Frieza's too.
  * 64 x 64 bodies (Perfect Cell 30, Cooler 34) keep their own OBJ VRAM size (header bytes 0-1: 0x1000) and
    their own hurt boxes; the map box (byte 3) stays Goku's 16 px so they fit through doors.
  * Perfect Cell keeps his beam group 18 (7 frames) as player group 18; Yamcha and Cooler get one made of their
    charge pose (OVERRIDE), so the Kamehameha (ability 3) plays on the three of them.

Display rows: one row per fighter of the roster table (src/coop/tourney_data.c) with role TR_BEAT or
TR_CHALLENGER, in TFighterId order from row 29: Gohan's standard row (behaviour class, flight timing) with
the fighter's play_sprite, his dialogue portrait (Hercule's 67 when the table has 0) and Hercule's status
portrait (the table has no status portraits yet), links to itself. A play sprite missing from the build
(art not merged, BUU=0) degrades to Hercule's 72; Super Android 13's 180 is Perfect Cell's enemy groups with
BUU=0 and degrades to the Perfect Cell player body 243. Rows past the last fighter are Hercule's row.
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402
import a16sprites as A  # noqa: E402
from buu2log2 import compress  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "src", "coop")

PLAYER_HEADER_SPRITE = 60       # Goku: the player collision boxes, 21 groups
GROUP_COUNT = 21
# frames the engine plays per player group (Goku's own arrays): a shorter source array is padded with its
# last frame
FRAMES = {0: 2, 1: 4, 2: 4, 3: 4, 4: 4, 5: 3, 6: 4, 7: 4, 8: 4, 11: 2, 12: 4, 13: 4, 17: 2, 18: 7, 20: 4}
# player group -> enemy groups to take it from, first present wins ("3L" = group 3's last frame: lying)
PLAYER_FROM = {
    0: [0],               # idle
    1: [1],               # walk
    2: [1],               # run (cutscenes): the walk
    3: [3, 12, 2],        # knock-down: the knock-back (stand, stagger, fall, lie), the death fall, the hit-stun
    4: [4, 5, 0],         # shout / looking up; else the charge pose
    5: [5, 6],            # ki pose (every ability animation plays it): the charge pose
    6: [6, 7, 8],         # punch combo
    7: [7, 8, 6],         # attacks
    8: [8, 7, 6],
    11: [2, 3],           # hurt: the hit-stun
    12: [12, "3L", 2],    # lying
    13: [1],              # dash / flight: the walk
    17: [0],              # halo (never shown for them): idle
    18: [18],             # beam pose (only Perfect Cell has one)
    20: [3, 12, 2],       # tumble
}

ROW0 = 29
ROW_LAST = 79
HERCULE_ROW = 21
GOHAN_ROW = 0
HERCULE_SPRITE = 72
DISPLAY_DATA = 0x081D967C


def ts_ids():
    """TS_* sprite ids from src/coop/tourney.h."""
    txt = open(os.path.join(SRC, "tourney.h"), encoding="utf-8").read()
    return {m.group(1): int(m.group(2)) for m in re.finditer(r"#define\s+(TS_\w+)\s+(\d+)", txt)}


TS = ts_ids()
# the player bodies built here: sprite id -> (symbol stem, source, borrow-from sprite for missing groups)
BODIES = {
    TS["TS_P_YAMCHA"]: ("yamcha", 156, None),
    TS["TS_P_TIEN"]: ("tien", 121, None),
    TS["TS_P_FRIEZA"]: ("frieza", 48, None),
    TS["TS_P_MECHA"]: ("mecha", 49, 48),         # his group 3 is his death (Trunks' sword cuts him in two): skipped
    TS["TS_P_TAO"]: ("tao", 50, None),
    TS["TS_P_A13"]: ("a13", "a13", None),
    TS["TS_P_A17"]: ("a17", 9, None),
    TS["TS_P_A18"]: ("a18", 11, None),
    TS["TS_P_PCELL"]: ("pcell", 30, None),
    TS["TS_P_COOLER"]: ("cooler", 34, None),
}


def fighters():
    """[(TF name, play_sprite expression, portrait, role expression)] in TFighterId order (tourney.h enum,
    rows of tourney_data.c)."""
    h = open(os.path.join(SRC, "tourney.h"), encoding="utf-8").read()
    enum = re.search(r"typedef enum \{(.*?)\} TFighterId;", h, re.S).group(1)
    enum = re.sub(r"//[^\n]*", "", enum)
    order = [n.strip() for n in enum.split(",") if n.strip() and n.strip() != "TF_COUNT"]
    d = open(os.path.join(SRC, "tourney_data.c"), encoding="utf-8").read()
    rows = {}
    for m in re.finditer(r"\[(TF_\w+)\]\s*=\s*\{\s*\"[^\"]*\"\s*,\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*\w+,\s*\w+,\s*([^,]+),", d):
        rows[m.group(1)] = (m.group(3).strip(), m.group(4).strip(), m.group(5).strip())
    out = []
    for name in order:
        if name in rows:
            ps, por, role = rows[name]
            out.append((name, ps, por, role))
    return order, out


def value(expr):
    expr = expr.strip()
    if expr in TS:
        return TS[expr]
    try:
        return int(expr, 0)
    except ValueError:
        return 0


def playable_rows():
    """{TF name: (row, play sprite, dialogue portrait)} for every TR_BEAT / TR_CHALLENGER fighter."""
    _, rows = fighters()
    out = {}
    row = ROW0
    for name, ps, por, role in rows:
        if not re.search(r"\b(BEAT|CHAL|TR_BEAT|TR_CHALLENGER)\b", role):
            continue
        assert row <= ROW_LAST, "more tournament playables than display rows 29-79"
        out[name] = (row, value(ps), value(por))
        row += 1
    return out


def display_rows(rom, have, buu):
    """The 20-byte display rows 29-79. `have(sprite)`: the build's sprite index has a record there; `buu`:
    the Buu's Fury imports are in the build (Super Android 13's 180 is a player-layout body only then)."""
    base = DISPLAY_DATA - S.ROM_BASE
    gohan = bytes(rom[base + GOHAN_ROW * 20:base + GOHAN_ROW * 20 + 20])
    herc = bytes(rom[base + HERCULE_ROW * 20:base + HERCULE_ROW * 20 + 20])
    by_row = {r: (s, p) for r, s, p in playable_rows().values()}
    out = []
    for row in range(ROW0, ROW_LAST + 1):
        if row not in by_row:
            r = bytearray(herc)
            r[0] = 0
            r[4] = r[5] = HERCULE_ROW
            out.append(bytes(r))
            continue
        sprite, portrait = by_row[row]
        if sprite == 180 and not buu:
            sprite = TS["TS_P_PCELL"]
        if not sprite or not have(sprite):
            sprite = HERCULE_SPRITE
        r = bytearray(gohan)
        r[0] = 0                 # a base form
        r[1] = sprite
        r[2] = portrait or herc[2]
        r[3] = herc[3]
        r[4] = r[5] = row
        out.append(bytes(r))
    return out


# ---- the records ----------------------------------------------------------------------------------
def source_groups(sp, src):
    """Groups of a source body: list (by group index) of None or 4 lists of frames
    (xo, yo, w, h, attr, blob) with blob a ROM address (int) or a pixel array (list of rows)."""
    if src == "a13":
        import bossmodels as B
        B.PAL = sp.pal
        _, groups, _ = B.build_a13(sp)
        return [g and [list(f) for f in g] for g in groups]
    rec = sp.record(src)
    g = sp.groups(rec)
    c = sp.frame_counts(g)
    out = []
    for grp in g:
        if not any(grp):
            out.append(None)
            continue
        out.append([[sp.descriptor(arr, fi) for fi in range(c[arr])] if arr else [] for arr in grp])
    return out


SKIP = {49: {3}}     # source groups never used: Mecha Frieza's 3 is the sword cutting him in two
# per-body player groups taken from other source groups: a beam pose (group 18, what the Kamehameha's charge and
# firing states play) made of the charge pose, for the fighters whose kit has the Kamehameha (src/coop/tny_play.c)
OVERRIDE = {156: {18: [5]}, 34: {18: [5]}}


def build_record(sp, src, borrow):
    groups = source_groups(sp, src)
    for gi in SKIP.get(src, ()):
        groups[gi] = None
    extra = source_groups(sp, borrow) if borrow is not None else []

    def have(gi):
        return gi < len(groups) and groups[gi]

    def get(gi):
        if have(gi):
            return groups[gi], src
        if gi < len(extra) and extra[gi]:
            return extra[gi], borrow
        return None, None

    out, used = [], {}
    for pg in range(GROUP_COUNT):
        choice = None
        for cand in OVERRIDE.get(src, {}).get(pg, PLAYER_FROM.get(pg, [])):
            if cand == "3L":
                g, who = get(3)
                if g:
                    choice = ([[f[-1]] for f in g], f"{who}:3L")
                    break
                continue
            g, who = get(cand)
            if g:
                choice = (g, f"{who}:{cand}")
                break
        if not choice:
            out.append(None)
            continue
        g, tag = choice
        need = FRAMES.get(pg, 1)
        dirs = []
        for d in range(4):
            frames = list(g[d] or g[0])
            assert frames, f"{src} {tag} facing {d} empty"
            while len(frames) < need:
                frames.append(frames[-1])
            dirs.append(frames)
        out.append(dirs)
        used[pg] = tag
    return out, used


def header_for(sp, src):
    rom = sp.rom
    goku = sp.record(PLAYER_HEADER_SPRITE) - S.ROM_BASE
    h = bytearray(rom[goku:goku + 0x4C])
    h[2] = GROUP_COUNT
    if src != "a13":
        s = sp.record(src) - S.ROM_BASE
        if rom[s + 1] > h[1]:                    # a 64 x 64 body: its OBJ VRAM size and its hurt boxes
            h[0:2] = rom[s:s + 2]
            h[0xC:0x4C] = rom[s + 0xC:s + 0x4C]
    return bytes(h)


class Writer:
    def __init__(self):
        self.lines = ["/* generated by tools/tnyplayables.py from the ROM: the tournament's player-layout bodies",
                      " * (sprites 235-244) and the fighter -> display row map. Build output only; never commit. */",
                      '#include "tourney.h"', "",
                      "typedef struct { signed char xo, yo; u8 w, h; u32 attr; const void *blob; } TpDesc;", ""]
        self.blobs = {}
        self.bytes = 0

    def blob(self, px, w, h):
        key = (w, h, tuple(tuple(r) for r in px))
        if key not in self.blobs:
            name = f"tp_blob_{len(self.blobs)}"
            data = compress(A.tiles(px, w, h))
            self.bytes += len(data)
            self.blobs[key] = name
            self.lines.append(f"static const u8 {name}[{len(data)}] TNY_ART __attribute__((aligned(4))) = "
                              f"{{{', '.join(f'0x{b:02x}' for b in data)}}};")
        return self.blobs[key]

    def record(self, stem, header, groups):
        names = []
        for gi, g in enumerate(groups):
            if not g:
                names.append("{0, 0, 0, 0}")
                continue
            arrs = []
            for d, frames in enumerate(g):
                ent = []
                for xo, yo, w, h, attr, blob in frames:
                    b = f"(const void *)0x{blob:08x}u" if isinstance(blob, int) else self.blob(blob, w, h)
                    ent.append(f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {b}}}")
                name = f"tp_{stem}_g{gi}_d{d}"
                self.lines.append(f"static const TpDesc {name}[{len(ent)}] TNY_ART = {{{', '.join(ent)}}};")
                self.bytes += 12 * len(ent)
                arrs.append(name)
            names.append("{" + ", ".join(arrs) + "}")
        hdr = ", ".join(f"0x{b:02x}" for b in header)
        t = f"tp_{stem}_t"
        self.lines.append(f"typedef struct {{ u8 header[0x4C]; const TpDesc *groups[{len(groups)}][4]; u32 end; }} {t};")
        self.lines.append(f"const {t} tnyp_{stem}_record TNY_ART = {{{{{hdr}}}, {{{', '.join(names)}}}, 0}};")

    def rows(self):
        m = playable_rows()
        ent = ", ".join(f"[{n}] = {r}" for n, (r, _, _) in m.items())
        self.lines.append(f"const u8 tny_play_rows[TF_COUNT] = {{{ent}}};")

    def save(self, path):
        os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
        open(path, "w", encoding="utf-8", newline="\n").write("\n".join(self.lines) + "\n")


def preview(path, sp, records):
    """Every body: each player group x 4 facings (all frames), 2x."""
    cell = 66
    rows = []
    for sid, (stem, groups, used) in records.items():
        for gi, g in enumerate(groups):
            if not g:
                continue
            row = []
            for frames in g:
                for xo, yo, w, h, attr, blob in frames:
                    if isinstance(blob, int):
                        px = sp.frame_pixels_blob(blob, w, h)
                    else:
                        px = blob
                    row.append(A.hflip(px) if attr & A.HFLIP else px)
                row.append(None)
            rows.append((f"{sid}", f"{gi}", row))
    width = max(len(r) for _, _, r in rows) * 36 + 60
    cv = S.Canvas(width, len(rows) * cell, (60, 60, 90))
    for ri, (sid, gi, row) in enumerate(rows):
        cv.text(2, ri * cell + 4, sid, (255, 255, 0), 2)
        cv.text(2, ri * cell + 20, gi, (0, 255, 255), 2)
        x = 50
        for px in row:
            if px is None:
                x += 8
                continue
            for y, line in enumerate(px):
                for xx, v in enumerate(line):
                    if v:
                        cv.put(x + xx, ri * cell + 1 + y, sp.pal[v])
            x += len(px[0]) + 2
    cv.save(path)
    try:
        from PIL import Image
        im = Image.open(path)
        im.resize((im.width * 2, im.height * 2), Image.NEAREST).save(path.replace(".png", "_x2.png"))
    except ImportError:
        pass


def frame_pixels_blob(self, blob, w, h):
    import mapchunks as m
    if blob not in self.blobs:
        self.blobs[blob] = m.unpack(self.rom, blob)
    data = self.blobs[blob]
    tw = w // 8
    return [[data[((y // 8) * tw + x // 8) * 64 + (y % 8) * 8 + x % 8] if ((y // 8) * tw + x // 8) * 64 + 64 <= len(data) else 0
             for x in range(w)] for y in range(h)]


S.Sprites.frame_pixels_blob = frame_pixels_blob


def main():
    rom = open(sys.argv[1], "rb").read()
    out_c = sys.argv[2]
    pdir = sys.argv[sys.argv.index("--preview-dir") + 1] if "--preview-dir" in sys.argv else None
    sp = S.Sprites(rom)
    w = Writer()
    records = {}
    for sid, (stem, src, borrow) in BODIES.items():
        groups, used = build_record(sp, src, borrow)
        w.record(stem, header_for(sp, src), groups)
        records[sid] = (stem, groups, used)
        missing = [pg for pg in FRAMES if pg not in used]
        print(f"  {sid} {stem:7s} " + " ".join(f"{pg}<{t}" for pg, t in used.items())
              + (f"  (empty: {missing})" if missing else ""))
    w.rows()
    w.save(out_c)
    if pdir:
        os.makedirs(pdir, exist_ok=True)
        preview(os.path.join(pdir, "tny_play_preview.png"), sp, records)
    print(f"wrote {out_c}: {len(records)} player bodies, {len(w.blobs)} rebuilt frames, {w.bytes} bytes; "
          f"{len(playable_rows())} playable rows from {ROW0}")


if __name__ == "__main__":
    main()
