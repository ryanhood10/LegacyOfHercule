"""Contact sheets of the tournament's bodies, read from a BUILT ROM (docs/testing.md "Tournament bodies: sheets").

Every body the tournament shows (the roster's opponent sprite and play_sprite from src/coop/tourney_data.c, the story
playables' sprites, the display rows 29-66's sprites) one row each, in all four facings, drawn as the engine draws
them (h-flip applied, the descriptor's x / y offset on a common ground line, a grey cross at the entity's position).

modes:
  --lying            the frame the tournament engine poses a loser in, facing left and right (tny_engine.c
                     pose_hold: group 12's last frame, else the generated lying table tny_lie (read from the ROM),
                     else 3's last, else 2's first; Super 13 kneels in 23)
  --cheer            the winner's pose (group 4's last frame, else 14's first, else 5's first; facing down)
  --groups G[,G..]   every frame of these groups (e.g. 1,13 for the walk and the dash)
  --last G[,G..]     the last frame of these groups (e.g. 12,3: the lying candidates)

usage: python -I tools/tnysheet.py BUILT.gba coop.sym OUT.png (--lying | --cheer | --groups 1,13) [--ids 187,188]
       [--scale 3]
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "src", "coop")
HF = 0x10000000
STORY = [57, 161, 103, 137, 135, 60, 72, 175, 58]       # Gohan, Krillin, Piccolo, Vegeta, Trunks, Goku, Hercule, 16, SSJ2


def roster_ids():
    ts = {m.group(1): int(m.group(2)) for m in
          re.finditer(r"#define\s+(TS_\w+)\s+(\d+)", open(os.path.join(SRC, "tourney.h"), encoding="utf-8").read())}
    ids = []
    for m in re.finditer(r'\[TF_\w+\]\s*=\s*\{\s*"[^"]*",\s*(\w+),\s*(\w+),',
                         open(os.path.join(SRC, "tourney_data.c"), encoding="utf-8").read()):
        for v in m.groups():
            n = ts.get(v, None) if not v.isdigit() else int(v)
            if n and n not in ids:
                ids.append(n)
    for n in STORY:
        if n not in ids:
            ids.append(n)
    return ids


def lie_order():
    """tny_engine.c pose_hold's fallback list `lie[][2]` ({group, frame}, frame -1 = the last)."""
    txt = open(os.path.join(SRC, "tny_engine.c"), encoding="utf-8").read()
    m = re.search(r"lie\[\]\[2\]\s*=\s*\{(.*?)\};", txt, re.S)
    return [(int(a), int(b)) for a, b in re.findall(r"\{\s*(-?\d+)\s*,\s*(-?\d+)\s*\}", m.group(1))]


class Built:
    def __init__(self, rom, sym):
        self.rom = rom
        self.sp = S.Sprites(rom)
        self.idx = sym["sprite_index"]

    def record(self, sid):
        r = int.from_bytes(self.rom[self.idx - S.ROM_BASE + 4 * sid: self.idx - S.ROM_BASE + 4 * sid + 4], "little")
        return r if self.sp.is_ptr(r) else 0

    def desc_ok(self, a):
        try:
            return self.sp.valid_descriptor(a, 0)
        except Exception:  # noqa: BLE001
            return False

    def group(self, rec, g):
        """The engine's grp_of: all four facings set and the facing's first descriptor valid."""
        if not rec or g >= self.rom[rec - S.ROM_BASE + 2]:
            return None
        ptrs = [self.sp.u32(rec + 0x4C + 0x10 * g + 4 * d) for d in range(4)]
        return ptrs if all(ptrs) and all(self.desc_ok(p) for p in ptrs) else None

    def frames(self, rec, arr):
        """The engine's frames_of (at most 8): up to the next array start of this record or an invalid descriptor."""
        starts = {self.sp.u32(rec + 0x4C + 0x10 * g + 4 * d) for g in range(self.rom[rec - S.ROM_BASE + 2])
                  for d in range(4)}
        n = 1
        while n < 8:
            p = arr + 12 * n
            if p in starts or not self.desc_ok(p):
                break
            n += 1
        return n

    def lie_table(self, sym):
        """The generated tny_lie table in the built ROM: sprite -> (group, frame, group pointer or 0)."""
        out = {}
        a = sym.get("tny_lie")
        while a:
            sid, g, f, grp = struct.unpack_from("<HbbI", self.rom, a - S.ROM_BASE)
            if not sid:
                break
            out[sid] = (g, f, grp)
            a += 8
        return out

    def pick(self, sid, mode, lie):
        """[(descriptor array, frame) per shown facing] as tny_engine.c pose_hold picks it: the loser lies in a side
        facing (left, right), the winner faces down."""
        rec = self.record(sid)
        facings = (2, 3) if mode == "lying" else (0,)
        out = []
        for d in facings:
            got = None
            if mode == "lying" and sid == 180:
                grp = self.group(rec, 23)
                got = (grp[d], 0, "g23") if grp else None
            elif mode == "lying" and sid in lie:
                g, f, gp = lie[sid]
                if gp:
                    arr = self.sp.u32(gp + 4 * d)
                    got = (arr, f, "table")
                else:
                    grp = self.group(rec, g)
                    if grp:
                        got = (grp[d], self.frames(rec, grp[d]) - 1 if f < 0 else f, f"g{g}*")
            tab = lie_order() if mode == "lying" else [(4, -1), (14, 0), (5, 0)]
            for g, f in tab:
                if got:
                    break
                grp = self.group(rec, g)
                if grp:
                    got = (grp[d], self.frames(rec, grp[d]) - 1 if f < 0 else f, f"g{g}")
            out.append(got)
        return out


def draw(cv, sp, x, y, arr, fi, cell):
    xo, yo, w, h, attr, blob = sp.descriptor(arr, fi)
    fp = sp.frame_pixels(arr, fi)
    if not fp:
        return
    _, _, px = fp
    ox, oy = x + cell // 2, y + cell - 10
    for yy in range(h):
        for xx in range(w):
            v = px[yy][w - 1 - xx] if attr & HF else px[yy][xx]
            if v:
                cv.put(ox + xo + xx, oy + yo + yy, sp.pal[v])
    for k in range(-2, 3):
        cv.put(ox + k, oy, (150, 150, 150))
        cv.put(ox, oy + k, (150, 150, 150))


def main():
    a = sys.argv
    rom = open(a[1], "rb").read()
    sym = {}
    for line in open(a[2]):
        p = line.split()
        if len(p) == 3:
            sym[p[2]] = int(p[0], 16)
    out = a[3]
    ids = [int(v) for v in a[a.index("--ids") + 1].split(",")] if "--ids" in a else roster_ids()
    scale = int(a[a.index("--scale") + 1]) if "--scale" in a else 3
    b = Built(rom, sym)
    sp = b.sp
    lie = b.lie_table(sym)
    rows = []
    if "--groups" in a or "--last" in a:
        last = "--last" in a
        gs = [int(v) for v in a[a.index("--last" if last else "--groups") + 1].split(",")]
        for sid in ids:
            rec = b.record(sid)
            cells = []
            for g in gs:
                grp = b.group(rec, g)
                if not grp:
                    cells.append(None)
                    continue
                for d in range(4):
                    n = b.frames(rec, grp[d])
                    cells += [(grp[d], fi) for fi in (range(n - 1, n) if last else range(n))]
                    cells.append(None)
            rows.append((f"{sid}", cells))
    else:
        mode = "lying" if "--lying" in a else "cheer"
        for sid in ids:
            rec = b.record(sid)
            cells = []
            tag = ""
            for got in b.pick(sid, mode, lie):
                if not got:
                    cells.append(None)
                    continue
                arr, fi, tag = got
                cells.append((arr, fi))
            rows.append((f"{sid} {tag}", cells))
    big = any(c and sp.descriptor(c[0], c[1])[2] > 32 or c and sp.descriptor(c[0], c[1])[3] > 32
              for _, cells in rows for c in cells)
    cell = 76 if big else 44
    ncol = max(len(c) for _, c in rows)
    per = 10 if "--groups" not in a and "--last" not in a else (2 if "--last" in a else 1)
    nr = (len(rows) + per - 1) // per
    lab = 40
    cv = S.Canvas(per * (lab + ncol * cell), nr * cell, (70, 90, 70))
    for i, (name, cells) in enumerate(rows):
        x0, y0 = (i % per) * (lab + ncol * cell), (i // per) * cell
        cv.text(x0 + 2, y0 + 2, name, (255, 255, 0))
        for c, item in enumerate(cells):
            if item:
                draw(cv, sp, x0 + lab + c * cell, y0, item[0], item[1], cell)
    cv.save(out)
    try:
        from PIL import Image
        im = Image.open(out)
        im.resize((im.width * scale, im.height * scale), Image.NEAREST).save(out)
    except ImportError:
        pass
    print(f"{out}: {len(rows)} bodies")


if __name__ == "__main__":
    main()
