"""Build new Legacy of Goku II maps from JSON specs (data/maps/*.json) and emit patch records for
tools/patch_rom.py --maps: compressed chunks, tile banks, tileset slot table, collision, triggers,
exits, gates, the variation and a MapEntry row per map. All graphics come from the ROM at build time
(stamps and fills copied from stock maps, recoloured tiles) or are generated (solid / speckled floor
tiles), so nothing from the ROM is committed. Format: docs/maps.md; library: tools/maplib.py.

usage: mapbuild.py build SPEC.json [SPEC.json ...] OUT.json [--rom roms/log2.gba] [--free 0xD00000] [--preview DIR]
                    [--buu-rom roms/buu.gba]
       mapbuild.py preview SPEC.json [--rom ...] [--out DIR]     render the built map (layers, collision) without a ROM

Spec (coordinates: "c, r" in 8-px tiles for layout and collision ops, pixels for objects):
{
  "name": "arctic", "zone": 15, "area": 1,
  "base": [8, 28],                   header defaults (priorities, blend, sprite sheet) + optional layout
  "tileset": "base" | "fresh",       start from the base map's slot table, or an empty one (only used tiles)
  "layout": "base" | [W, H],         copy the base map's layers/collision/objects, or a blank W x H px map
  "stack_ops": [...],                ops on every layer + collision + objects at once (insert_cols/rows, stamp, crop)
  "layers": {"1": [ops], "2": [...], "3": [...]},   per-layer ops (fill, copy, clear, texture, solid, png);
                                     a png op's picture is a "file" or "draw": NAME (drawn by tools/NAME.py's
                                     draw(), our own art, e.g. shipart: the Saiyan ship), placed "at" [col, row]
                                     a fill with "anchor": "map" phases its block to the map origin (seamless)
  a stamp with "recolor": false keeps its tiles out of the spec's recolour (e.g. a flight spot's grass)
  "collision": [ops],                walk / block rects and ellipses, "outside": "block" for the map border;
                                     {"op": "art_footprint", "draw": NAME, "at": [col, row]}: a drawn picture's collision
  "projectile": "walk" | [ops],      projectile mask (default: same as walk collision)
  "recolor": {"rules": [...]},       remap every tile's colours (new tile banks); see recolor()
  "effect": {"from": [13, 1]},       BG0 effect overlay (snowfall, rain) + its animated tiles, blend, BG0 priority
  "keep": ["decorations", "gates", "triggers", "exits"],  base objects kept (positions follow stack ops)
  "gates": [{"x":, "y":, "barrier": 0, "color": 3, "level": 50, "char": 2}],  replace/add level gates
  "triggers": [{"type": "flight", "rect": [x1,y1,x2,y2], "world": [x, y]}, {"type": "script", ...},
               {"type": "dialogue", "rect": [...], "payload": "seq:NAME", "condition": COND},
               {"type": "savepad", "rect": [...]}],   the save / switch-characters pad (press A; stamp its art)
  "sources": ["data/story/NAME.lscript"],  story scripts (tools/script_asm.py) named by payloads, record
                                     actions and conditions as seq:NAME / script:NAME; a COND is 0, "flag:N",
                                     "noflag:N", "script:NAME", "never" or a number
  "records": [{"kind": "actor", "sprite":, "x":, "y":, "facing": 0, "condition": COND},
              {"kind": "enemy", "like": [zone, area, record], "stat":, "sprite":, "x":, "y":, "condition": COND,
               "action": "seq:NAME" | "like" | "none"}],   map script records; an enemy copies a stock record's
                                     behaviours ("action": "like" also its defeat action, e.g. the random item drop)
  "exits": [{"type": "edge"|"door", "rect": [...], "dir": "down|up|left|right", "to": [z, a, v], "at": [x, y]}],
  "barrier": {"char": 2, "level": 50, "flag": 380, "rect": [x1,y1,x2,y2], "decoration": 0, "dialogue_trigger": 0},
                                     a working level barrier on a kept decoration + gate (item, on-load, dialogue condition)
  "onload": {"barrier_flag": 280, "rect": [x1,y1,x2,y2]} | null,
  "rocks": {"sheet": [Z, A], "list": [{"x":, "y":, "frame": 0, "hits": 3, "drop": "0x083EE6A8"}, ...]},
                                     breakable stock rocks: a 32x32 decoration each (frame = offset on that stock map's
                                     decoration sheet, 32-byte units) + an item (handler 0x0800F9A3) running a stock rock's
                                     drop script (random HP / EP pickups by player 1's bars); solid until broken, back on
                                     every load. Stock drops: 0x083ED894 small (1 hit), 0x083ED6CC medium (2), 0x083EE6A8 large (3)
  "spawn": [x, y], "name_id": 233, "music": 5, "flags": 1,
  "requires": "buu"                  only built when that ROM is given (--buu-rom)
}
A stamp / fill / copy source "map" is [zone, area(, variation)] of this ROM, or a foreign variation record:
{"rom": "buu", "variation": "0x0822229C"} (Buu's Fury: same map format; its tiles are converted to LoG2's
palette by nearest colour).
"""
import argparse
import colorsys
import json
import os
import random
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import maplib as L        # noqa: E402
from buu2log2 import compress   # noqa: E402  (LZ encoder for the IWRAM decompressor)
from mapchunks import decompress   # noqa: E402

DIRS = {"down": 0, "up": 1, "left": 2, "right": 3, "none": 0}
BUU_BG_PALETTE = 0x0805632C     # Buu's Fury BG palette (0x200 below its OBJ palette 0x0805652C)
FREE_DEFAULT = 0xD00000          # file offset: maps region (0x800000 code, 0x900000 Hercule data, 0xB00000 story)
TABLE_LITERALS = [0x5ADC, 0x5D5C, 0xEBD8]   # code literals holding the MapEntry table address
TABLE_ENTRY3_LITERAL = 0x195C               # &table[3] (the new-game map)
COUNT_AT = 0xE0AC                           # Map_GetCount: movs r0,#0xFF; adds r0,#(count-255)
ACT_EXIT = 0x0800D8AF            # activation object of exits
ACT_PRESS = 0x0800D839           # activation object of the flight / save-pad / script spots
ACT_DIALOG_FACING = 0x0800D87F   # activation object of barrier dialogue triggers
ITEM_SCRIPT_HANDLER = 0x0800F9A3 # map item handler: run the record's script (+0xC) when touched


class SpecError(Exception):
    pass


def lz(data):
    """Compressed resource {1, size, stream}, checked by decoding it again."""
    blob = compress(bytes(data))
    if decompress(blob, 8, 1)[:len(data)] != bytes(data):
        raise SpecError("encoder round-trip failed")
    return blob + bytes(-len(blob) % 4)


# ---- tiles ------------------------------------------------------------------------------------------
class Tileset:
    """The new map's VRAM slot table. A slot holds ('atlas', index) (a stock bank tile) or ('raw', 64 bytes).
    Slots inside animated ranges are reserved (the sequence overwrites them every few frames)."""

    def __init__(self, rom, banks):
        self.rom, self.banks = rom, banks
        self.slots, self.index, self.reserved = [], {}, set()

    def load_base(self, var):
        for a in L.slot_table(self.rom, var):
            self.slots.append(("atlas", a))
        for i, k in enumerate(self.slots):
            self.index.setdefault(k, i)

    def reserve(self, lo, n):
        self.reserved.update(range(lo, lo + n))

    def tile(self, slot):
        k = self.slots[slot]
        return self.banks.tile(k[1]) if k[0] == "atlas" else k[1]

    def add(self, key):
        if key in self.index:
            return self.index[key]
        slot = len(self.slots)
        while slot in self.reserved:
            self.slots.append(("atlas", 0))
            slot += 1
        if slot >= L.SLOT_LIMIT:
            raise SpecError(f"tileset full ({L.SLOT_LIMIT} slots)")
        self.slots.append(key)
        self.index[key] = slot
        return slot


class Source:
    """A map variation read into world-aligned grids: layers 0..3 (entries with the SOURCE's slots),
    walk and projectile bitmaps, and the slot -> key mapping used to translate into a Tileset.
    A stock map is named by zone / area; a foreign map (Buu's Fury) by its variation address, its tiles
    becoming raw tiles with the foreign palette mapped onto LoG2's (remap: foreign index -> LoG2 index)."""

    def __init__(self, rom, zone=None, area=None, var=1, nth=0, foreign=None):
        self.rom = rom
        self.remap = foreign["remap"] if foreign else None
        if foreign:
            self.entry, self.var = None, L.Variation(rom, foreign["variation"])
        else:
            e = L.find_entry(rom, zone, area, nth)
            if e is None:
                raise SpecError(f"no map zone {zone} area {area}")
            self.entry, self.var = e, e.variation(var)
        v = self.var
        self.w, self.h = (v.width + 7) // 8, (v.height + 7) // 8
        self.layers = {}
        cache = {}
        for bg in range(4):
            lay = L.Layer(rom, v.layers[bg])
            if not lay.tiled or lay.scale != (1024, 1024):
                continue
            ox, oy = lay.offset[0] >> 10, lay.offset[1] >> 10
            if ox % 8 or oy % 8:
                continue
            ents = lay.entries(cache)
            grid = [[None] * self.w for _ in range(self.h)]
            for ty in range(self.h):
                ly = ty + oy // 8
                if not 0 <= ly < len(ents):
                    continue
                for tx in range(self.w):
                    lx = tx + ox // 8
                    if 0 <= lx < len(ents[0]):
                        grid[ty][tx] = ents[ly][lx]
            self.layers[bg] = grid
        self.walk = bytearray(L.bitmap(rom, v.collision))
        self.proj = bytearray(L.bitmap(rom, v.projmask))
        self.slot_keys = [("atlas", a) for a in L.slot_table(rom, v)]
        self.anim = {}
        for slot, nt, nf, frames in L.anim_sequences(rom, v):
            data = L.anim_frame(rom, frames[0])
            for k in range(nt):
                self.anim[slot + k] = ("raw", data[64 * k:64 * k + 64].ljust(64, b"\0"))
        if self.remap:
            banks = L.TileBanks(rom, v.banks)
            self.slot_keys = [("raw", bytes(self.remap[p] for p in banks.tile(a))) for _, a in self.slot_keys]
            self.anim = {s: ("raw", bytes(self.remap[p] for p in t)) for s, (_, t) in self.anim.items()}

    def key(self, slot):
        if slot in self.anim:
            return self.anim[slot]
        if slot < len(self.slot_keys):
            return self.slot_keys[slot]
        return ("raw", bytes(64))


# ---- the map being built --------------------------------------------------------------------------------
class Build:
    def __init__(self, rom, spec, sources, foreign=None):
        self.rom, self.spec, self.sources = rom, spec, sources
        self.foreign = foreign or {}
        base = spec.get("base")
        self.base = self.source(base) if base else None
        bvar = self.base.var if self.base else None
        self.banks = L.TileBanks(rom, bvar.banks if bvar else L.TILE_BANKS)
        self.ts = Tileset(rom, self.banks)
        self.fresh = spec.get("tileset", "base") == "fresh" or not self.base
        if not self.fresh:
            self.ts.load_base(bvar)
            for slot, nt, nf, _ in L.anim_sequences(rom, bvar):
                self.ts.reserve(slot, nt)
        if self.fresh:
            self.ts.add(("raw", bytes(64)))      # slot 0: blank (empty map cells are entry 0)
        # animated sequences: {"ptr": stock sequence, "slot": VRAM slot, "table": recolour table or None}
        self.anims = [] if self.fresh else [{"ptr": rom.u32(bvar.anims + 4 * i), "slot": rom.u16(rom.u32(bvar.anims + 4 * i) + 2),
                                             "table": None} for i in range(bvar.anim_count)]
        lay = spec.get("layout", "base")
        if lay == "base":
            if not self.base:
                raise SpecError("layout 'base' needs a base map")
            self.W, self.H = self.base.w, self.base.h
            self.layers = {0: self.empty()}
            for bg in (1, 2, 3):
                g = self.base.layers.get(bg)
                self.layers[bg] = [row[:] for row in g] if g else self.empty()
                if self.fresh and g:
                    self.layers[bg] = [[self.tr(self.base, e) for e in row] for row in g]
            self.walk = bytearray(self.base.walk)
            self.proj = bytearray(self.base.proj)
        else:
            self.W, self.H = (lay[0] + 7) // 8, (lay[1] + 7) // 8
            self.layers = {bg: self.empty() for bg in (0, 1, 2, 3)}
            self.walk, self.proj = bytearray(8192), bytearray(8192)
        self.keep_colors = set()      # slots a stamp with "recolor": false keeps in their own colours
        self.objects_from_base(spec.get("keep", []) if lay == "base" else [])
        self.spawn = list(bvar.spawn) if (bvar and lay == "base") else [self.W * 4, self.H * 4]

    def source(self, ref):
        """[zone, area(, variation)] of this ROM, or {"rom": "buu", "variation": "0x0822229C"}."""
        key = json.dumps(ref)
        if key not in self.sources:
            if isinstance(ref, dict):
                if ref["rom"] not in self.foreign:
                    raise SpecError(f"a map from the '{ref['rom']}' ROM, which was not given")
                frm, remap = self.foreign[ref["rom"]]
                self.sources[key] = Source(frm, foreign={"variation": int(ref["variation"], 0), "remap": remap})
            else:
                self.sources[key] = Source(self.rom, *ref)
        return self.sources[key]

    def empty(self):
        return [[None] * self.W for _ in range(self.H)]

    def tr(self, src, e):
        """Translate a source entry into this map's slots."""
        if e is None:
            return None
        if src is self.base and not self.fresh:
            return e
        slot = self.ts.add(src.key(e & 0x3FF))
        return slot | (e & 0xC00)

    # -- objects kept from the base map (positions in px)
    def objects_from_base(self, keep):
        e, v = (self.base.entry, self.base.var) if self.base else (None, None)
        self.decos = []
        if "decorations" in keep and v.deco_count:
            self.decos = [bytearray(self.rom.read(v.decos + 0x18 * k, 0x18)) for k in range(v.deco_count)]
        self.gates = [bytearray(g.raw) for g in e.gate_list()] if "gates" in keep else []
        self.triggers = []     # (cond, bytearray data, extra blobs)
        if "triggers" in keep:
            for k in range(e.ntrig):
                cond, d = struct.unpack("<II", self.rom.read(e.triggers + 8 * k, 8))
                self.triggers.append([cond, bytearray(self.rom.read(d, 0x18)), None])
        self.exits = []
        if "exits" in keep:
            for t in v.exit_list():
                self.exits.append(bytearray(self.rom.read(t.addr, 0x18)))
        self.sheet = v.sheet if v else 0

    def shift(self, axis, at_px, d):
        """Move every object at or beyond at_px on the axis by d px (insert / delete of rows or columns)."""
        def mv(v):
            return v + d if v >= at_px else v
        i = 0 if axis == "x" else 1
        for r in self.decos:
            x1, = struct.unpack_from("<i", r, 4 * i)
            x2, = struct.unpack_from("<i", r, 8 + 4 * i)
            struct.pack_into("<i", r, 4 * i, mv(x1))
            struct.pack_into("<i", r, 8 + 4 * i, mv(x2) if x2 > at_px else x2)
        for g in self.gates:
            v, = struct.unpack_from("<H", g, 2 * i)
            struct.pack_into("<H", g, 2 * i, mv(v))
        for t in [t[1] for t in self.triggers] + self.exits:
            x1, = struct.unpack_from("<h", t, 8 + 2 * i)
            x2, = struct.unpack_from("<h", t, 12 + 2 * i)
            struct.pack_into("<h", t, 8 + 2 * i, mv(x1))
            struct.pack_into("<h", t, 12 + 2 * i, mv(x2) if x2 > at_px else x2)
        self.spawn[i] = mv(self.spawn[i])

    def op(self, op):
        """One step of the spec's ordered "ops" list: a layer op when it names "bg", a collision op
        (walk / block / ellipses / border / from_layer; "mask": "proj" for the projectile mask), else a stack op."""
        if "bg" in op and op["op"] not in ("from_layer",):
            self.layer_op(op["bg"], op)
        elif op["op"] in ("walk", "block", "walk_ellipse", "block_ellipse", "border", "from_layer"):
            self.coll_op(self.proj if op.get("mask") == "proj" else self.walk, op)
            if op.get("mask") == "both":
                self.coll_op(self.proj, op)
        else:
            self.stack_op(op)

    # -- stack ops (all layers, both bitmaps, objects)
    def stack_op(self, op):
        k = op["op"]
        if k in ("insert_cols", "insert_rows"):
            at, (s0, s1), n = op["at"], op["src"], op.get("times", 1)
            span = (s1 - s0) * n
            if k == "insert_cols":
                for bg, g in self.layers.items():
                    for row in g:
                        row[at:at] = row[s0:s1] * n
                for name in ("walk", "proj"):
                    old = getattr(self, name)
                    new = bytearray(8192)
                    for ty in range(256):
                        for tx in range(256):
                            sx = tx if tx < at else (s0 + (tx - at) % (s1 - s0) if tx < at + span else tx - span)
                            if L.bit(old, sx, ty):
                                L.set_bit(new, tx, ty, 1)
                    setattr(self, name, new)
                self.W += span
                self.shift("x", at * 8, span * 8)
            else:
                for bg, g in self.layers.items():
                    g[at:at] = [row[:] for _ in range(n) for row in g[s0:s1]]
                for name in ("walk", "proj"):
                    old = getattr(self, name)
                    new = bytearray(8192)
                    for ty in range(256):
                        sy = ty if ty < at else (s0 + (ty - at) % (s1 - s0) if ty < at + span else ty - span)
                        if 0 <= sy < 256:
                            new[ty * 32:ty * 32 + 32] = old[sy * 32:sy * 32 + 32]
                    setattr(self, name, new)
                self.H += span
                self.shift("y", at * 8, span * 8)
        elif k == "stamp":
            src = self.source(op["src"]["map"])
            sc, sr, sw, sh = op["src"]["rect"]
            ac, ar = op["at"]
            flip = op.get("flip", "")
            layers = op.get("layers", [1, 2, 3])
            dst_layers = op.get("to_layers", layers)
            for bg, dbg in zip(layers, dst_layers):
                g = src.layers.get(bg)
                if g is None:
                    continue
                for y in range(sh):
                    for x in range(sw):
                        sx = sc + (sw - 1 - x if "h" in flip else x)
                        sy = sr + (sh - 1 - y if "v" in flip else y)
                        if not (0 <= sy < src.h and 0 <= sx < src.w):
                            continue
                        e = g[sy][sx]
                        if e is None or (op.get("skip_empty", True) and self.is_blank_src(src, e)):
                            if op.get("clear", False):
                                self.put(dbg, ac + x, ar + y, None)
                            continue
                        e2 = self.tr(src, e)
                        if op.get("recolor", True) is False:
                            self.keep_colors.add(e2 & 0x3FF)
                        if "h" in flip:
                            e2 ^= 0x400
                        if "v" in flip:
                            e2 ^= 0x800
                        self.put(dbg, ac + x, ar + y, e2)
            if op.get("collision", True):
                for y in range(sh):
                    for x in range(sw):
                        sx = sc + (sw - 1 - x if "h" in flip else x)
                        sy = sr + (sh - 1 - y if "v" in flip else y)
                        L.set_bit(self.walk, ac + x, ar + y, L.bit(src.walk, sx, sy))
                        L.set_bit(self.proj, ac + x, ar + y, L.bit(src.proj, sx, sy))
        elif k == "crop":
            c, r, w, h = op["rect"]
            for bg in self.layers:
                self.layers[bg] = [row[c:c + w] for row in self.layers[bg][r:r + h]]
            for name in ("walk", "proj"):
                old = getattr(self, name)
                new = bytearray(8192)
                for ty in range(h):
                    for tx in range(w):
                        L.set_bit(new, tx, ty, L.bit(old, c + tx, r + ty))
                setattr(self, name, new)
            self.W, self.H = w, h
            self.shift("x", 0, -c * 8)
            self.shift("y", 0, -r * 8)
        else:
            raise SpecError(f"unknown stack op {k}")

    def is_blank_src(self, src, e):
        k = src.key(e & 0x3FF)
        t = self.banks.tile(k[1]) if k[0] == "atlas" else k[1]
        return not any(t)

    def put(self, bg, x, y, e):
        if 0 <= x < self.W and 0 <= y < self.H:
            self.layers[bg][y][x] = e

    # -- per-layer ops
    def layer_op(self, bg, op):
        k = op["op"]
        g = self.layers[bg]
        if k == "clear":
            c, r, w, h = op.get("rect", [0, 0, self.W, self.H])
            for y in range(r, r + h):
                for x in range(c, c + w):
                    self.put(bg, x, y, None)
        elif k in ("fill", "copy"):
            src = self.source(op["src"]["map"])
            sg = src.layers.get(op["src"].get("bg", bg))
            sc, sr, sw, sh = op["src"]["rect"]
            if k == "fill":
                c, r, w, h = op["rect"]
            else:
                (c, r), w, h = op["at"], sw, sh
            # "anchor": "map" phases the source block to the map's origin instead of the rect's, so fills
            # over separate rects (or next to a stamp shifted by a multiple of the block) join seamlessly
            ox, oy = (c, r) if op.get("anchor") == "map" else (0, 0)
            for y in range(h):
                for x in range(w):
                    e = sg[sr + (oy + y) % sh][sc + (ox + x) % sw]
                    if e is None and k == "copy":
                        continue
                    self.put(bg, c + x, r + y, self.tr(src, e))
        elif k in ("solid", "texture"):
            c, r, w, h = op["rect"]
            pal = L.bg_palette(self.rom)
            base_i = nearest(pal, op["color"])
            tiles = []
            rng = random.Random(op.get("seed", 1))
            nvar = op.get("variants", 1 if k == "solid" else 6)
            for n in range(nvar):
                t = bytearray([base_i] * 64)
                if k == "texture":
                    for spec_c in op.get("speckle", []):
                        ci = nearest(pal, spec_c["color"])
                        for p in range(64):
                            if rng.random() < spec_c.get("density", 0.05):
                                t[p] = ci
                    if op.get("seam"):
                        si = nearest(pal, op["seam"])
                        for p in range(8):
                            t[p] = si          # top edge line: a faint floor grid
                            t[p * 8] = si
                tiles.append(self.ts.add(("raw", bytes(t))))
            for y in range(r, r + h):
                for x in range(c, c + w):
                    self.put(bg, x, y, tiles[rng.randrange(len(tiles))] if k == "texture" else tiles[0])
        elif k == "png":
            self.png_op(bg, op)
        else:
            raise SpecError(f"unknown layer op {k}")
        del g

    def png_op(self, bg, op):
        im = art_module(op["draw"]).draw() if "draw" in op else None
        if im is None:
            from PIL import Image
            im = Image.open(op["file"])
        im = im.convert("RGBA")
        pal = L.bg_palette(self.rom)
        c0, r0 = op.get("at", [0, 0])
        for ty in range(im.height // 8):
            for tx in range(im.width // 8):
                t = bytearray(64)
                for p in range(64):
                    rgba = im.getpixel((tx * 8 + p % 8, ty * 8 + p // 8))
                    t[p] = 0 if rgba[3] < 128 else nearest(pal, rgba[:3])
                if not any(t):
                    continue
                self.put(bg, c0 + tx, r0 + ty, self.match_or_add(bytes(t)))

    def match_or_add(self, t):
        for flip in (0, 0x400, 0x800, 0xC00):
            f = flip_tile(t, flip)
            for i, k in enumerate(self.ts.slots):
                if i in self.ts.reserved:
                    continue
                if (self.banks.tile(k[1]) if k[0] == "atlas" else k[1]) == f:
                    return i | flip
        return self.ts.add(("raw", t))

    # -- collision ops (tiles)
    def coll_op(self, bm, op):
        k = op["op"]
        v = 1 if k in ("walk", "walk_ellipse") else 0
        if k in ("walk", "block"):
            c, r, w, h = op["rect"]
            for y in range(r, r + h):
                for x in range(c, c + w):
                    L.set_bit(bm, x, y, v)
        elif k in ("walk_ellipse", "block_ellipse"):
            cx, cy, rx, ry = op["ellipse"]
            for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
                for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
                    if ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2 <= 1:
                        L.set_bit(bm, x, y, v)
        elif k == "border":
            n = op.get("width", 1)
            for y in range(256):
                for x in range(256):
                    if x < n or y < n or x >= self.W - n or y >= self.H - n:
                        L.set_bit(bm, x, y, 0)
        elif k == "art_footprint":
            # the collision a drawn picture brings (tools/<draw>.py footprint(): {(tx, ty): 0 block / 1 walk})
            c0, r0 = op["at"]
            for (tx, ty), v in art_module(op["draw"]).footprint().items():
                L.set_bit(bm, c0 + tx, r0 + ty, v)
        elif k == "from_layer":
            # walkable where the layer has a tile from the given slot set (e.g. floor tiles)
            g = self.layers[op["bg"]]
            for y in range(self.H):
                for x in range(self.W):
                    e = g[y][x]
                    if e is not None and (op.get("any") or (e & 0x3FF) in op.get("slots", [])):
                        L.set_bit(bm, x, y, 1)
        else:
            raise SpecError(f"unknown collision op {k}")


def art_module(name):
    """A picture drawn by code: tools/NAME.py with draw() (an RGBA PIL image) and footprint() (its collision)."""
    import importlib
    if not name.isidentifier():
        raise SpecError(f"bad draw module {name!r}")
    return importlib.import_module(name)


def flip_tile(t, flip):
    rows = [t[i * 8:i * 8 + 8] for i in range(8)]
    if flip & 0x400:
        rows = [r[::-1] for r in rows]
    if flip & 0x800:
        rows = rows[::-1]
    return b"".join(rows)


_NEAR = {}


def nearest(pal, rgb):
    """Nearest non-zero palette index (index 0 is transparent / backdrop)."""
    key = tuple(rgb)
    if key not in _NEAR:
        best, bi = None, 1
        for i in range(1, 256):
            r, g, b = L.bgr555_to_rgb(pal[i])
            d = 2 * (r - rgb[0]) ** 2 + 4 * (g - rgb[1]) ** 2 + 3 * (b - rgb[2]) ** 2
            if best is None or d < best:
                best, bi = d, i
        _NEAR[key] = bi
    return _NEAR[key]


def recolor_table(pal, spec):
    """Palette index -> palette index. Rules are tried in order; a rule matches a colour by hue range
    (degrees), minimum saturation and luminance range, and maps it to a ramp by luminance:
    {"hue": [lo, hi], "sat": [lo, hi], "lum": [lo, hi], "ramp": [[r,g,b], ...], "gamma": 1.0}.
    "keep": [indices] are left alone. The result is the nearest palette colour."""
    out = list(range(256))
    for i in range(1, 256):
        if i in spec.get("keep", []):
            continue
        r, g, b = L.bgr555_to_rgb(pal[i])
        h, l_, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
        lum = (0.3 * r + 0.59 * g + 0.11 * b) / 255
        for rule in spec["rules"]:
            hl, hh = rule.get("hue", [0, 360])
            sl, sh = rule.get("sat", [0, 1])
            ll, lh = rule.get("lum", [0, 1])
            hd = h * 360
            inhue = hl <= hd <= hh if hl <= hh else (hd >= hl or hd <= hh)
            if inhue and sl <= s <= sh and ll <= lum <= lh:
                ramp = rule["ramp"]
                t = max(0.0, min(1.0, (lum - rule.get("lo", 0)) / max(1e-6, rule.get("hi", 1) - rule.get("lo", 0))))
                t = t ** rule.get("gamma", 1.0)
                pos = t * (len(ramp) - 1)
                k = min(int(pos), len(ramp) - 2)
                f = pos - k
                c = [ramp[k][j] * (1 - f) + ramp[k + 1][j] * f for j in range(3)]
                out[i] = nearest(pal, [int(x) for x in c])
                break
    return out


# ---- emission ----------------------------------------------------------------------------------------------
class Packer:
    def __init__(self, free):
        self.free, self.start, self.records = free, free, []

    def put(self, data, note="", align=4):
        self.free = (self.free + align - 1) & ~(align - 1)
        at = self.free
        self.records.append({"offset": hex(at), "bytes": bytes(data).hex(), "note": note})
        self.free += len(data)
        return L.ROM_BASE + at

    def reserve(self, n, align=4):
        self.free = (self.free + align - 1) & ~(align - 1)
        at = self.free
        self.free += n
        return at


def varint_bytes(v):
    """Script varint (BytecodeVM_PushVarint): sign fold (v >= 0 -> 2v, v < 0 -> -2v + 1), 7-bit groups MSB first."""
    u = 2 * v if v >= 0 else -2 * v + 1
    groups = []
    while True:
        groups.append(u & 0x7F)
        u >>= 7
        if not u:
            break
    groups.reverse()
    return bytes([g | 0x80 for g in groups[:-1]] + [groups[-1]])


def push_multi(vals):
    return bytes([0x1D, len(vals)]) + b"".join(varint_bytes(v) for v in vals)


def script_warp(zone, area, var, x, y, music=None, with_entities=True):
    s = b""
    if music is not None:
        s += bytes([0x00, music, 0x02, 0x31])          # PushByte m; Step 49 PlayMusic
    s += push_multi([zone, area, var, x, y]) + bytes([0x02, 38 if with_entities else 39])   # WarpToMap...
    return s + b"\x11"


def script_barrier_onload(flag, rect, deco=0):
    """if flag: Step 30 (hide decoration deco); SetCollisionRect rect; RemoveRectFromMask2 rect (stock barrier pattern)."""
    body = bytes([0x00, deco, 0x02, 0x1E]) + push_multi(rect) + b"\x02\x47" + push_multi(rect) + b"\x02\x75"
    head = bytes([0x01]) + varint_bytes(flag) + b"\x02\x13" + bytes([0x13, len(body)])
    return head + body + b"\x11"


class StoryRefs:
    """seq:NAME / script:NAME from a spec's lscript sources, assembled into the map packer on first use
    (script_asm's placer interface: put, stock_equal)."""

    def __init__(self, rom, spec, pk):
        import script_asm as sa
        self.sa, self.rom, self.pk, self.placed, self.blocks = sa, bytes(rom.data), pk, {}, {}
        for path in spec.get("sources", []):
            text = open(path, encoding="utf-8").read()
            for name, blk in sa.parse_file(text, path, self.rom).items():
                self.blocks[name] = (text, path, blk["type"])
        sa.SYMBOL_RESOLVER = self.ref

    def put(self, data, align=1, note=""):
        return self.pk.put(data, note, max(align, 1))

    def stock_equal(self, addr, data):
        return False

    def ref(self, v):
        kind, _, name = v.partition(":")
        if kind not in ("seq", "script") or name not in self.blocks:
            raise SpecError(f"unknown story reference {v!r}")
        if name not in self.placed:
            text, path, _ = self.blocks[name]
            blk = self.sa.parse_file(text, path, self.rom)[name]
            self.placed[name] = self.sa.place_block(name, blk, self.rom, self)
        return self.placed[name]

    def kind(self, v):
        return self.blocks[v.partition(":")[2]][2]

    def condition(self, v):
        if isinstance(v, int):
            return v
        k, _, arg = v.partition(":")
        if k == "flag":
            return 0x10000 | int(arg, 0)
        if k == "noflag":
            return 0x20000 | int(arg, 0)
        if k == "script":
            return self.ref(v)
        return {"always": 0, "never": 1}[v] if v in ("always", "never") else int(v, 0)


FACE_AND_WAIT = 0x0800CB0F      # NpcBehavior_FaceAndWait_Create {fn, facing << 16 | ticks} (tools/mapedit.py)
HANDLER_ACTOR, HANDLER_ENEMY = 0x0800D6DF, 0x0800E77F
ACTION_SCRIPT, ACTION_DIALOG, ACTION_NONE = 0x0801069B, 0x080106B3, 0x08010699
TRIGGER_DIALOG_COND, TRIGGER_DIALOG_FN = 0x0800D7E1, 0x0800D765   # a dialogue run on entering the rect


def emit_records(b, pk, refs):
    """The spec's map script records (actors, enemies); returns (count, array pointer)."""
    rom, ptrs = b.rom, []
    for r in b.spec.get("records", []):
        cond = refs.condition(r.get("condition", 0))
        x, y, sprite = r["x"], r["y"], r["sprite"]
        if r["kind"] == "actor":
            beh = pk.put(struct.pack("<II", FACE_AND_WAIT, (r.get("facing", 0) << 16) | 0x7D00), "actor behaviour")
            ptrs.append(pk.put(struct.pack("<IIhhIII", HANDLER_ACTOR, cond, x, y, sprite, 1, beh), f"{b.spec['name']} actor record"))
        elif r["kind"] == "enemy":
            z, a, n = r["like"]
            src = rom.u32(L.find_entry(rom, z, a).scripts + 4 * n)
            cnt, beh_arr = rom.u32(src + 0x18), rom.u32(src + 0x1C)
            beh = pk.put(rom.read(beh_arr, 4 * cnt), "enemy behaviours") if cnt else 0
            act = r.get("action", "none")
            if act == "like":       # the stock record's own action (e.g. its random item drop)
                action = rom.u32(src + 0x14)
            elif act == "none":
                action = pk.put(struct.pack("<II", ACTION_NONE, 0), "enemy action")
            else:
                fn = ACTION_SCRIPT if refs.kind(act) == "script" else ACTION_DIALOG
                action = pk.put(struct.pack("<II", fn, refs.ref(act)), "enemy action")
            ptrs.append(pk.put(struct.pack("<IIIhhIIII", HANDLER_ENEMY, cond, r["stat"], x, y, sprite, action, cnt, beh),
                               f"{b.spec['name']} enemy record"))
        else:
            raise SpecError(f"record kind {r['kind']!r} (actor, enemy)")
    if not ptrs:
        return 0, 0
    return len(ptrs), pk.put(b"".join(struct.pack("<I", p) for p in ptrs), f"{b.spec['name']} record array")


def emit(b, pk):
    """Write one built map into the packer; returns the MapEntry row (0x38 bytes)."""
    rom, spec = b.rom, b.spec
    bvar = b.base.var if b.base else None
    # recolour: every slot gets a recoloured raw tile
    if spec.get("recolor"):
        table = recolor_table(L.bg_palette(rom), spec["recolor"])
        for i in range(len(b.ts.slots)):
            if i in b.ts.reserved or i in b.keep_colors:
                continue
            t = b.ts.tile(i)
            b.ts.slots[i] = ("raw", bytes(table[v] for v in t))
        for a in b.anims:
            a["table"] = table
    # effect overlay (BG0) and its animated tiles
    eff = spec.get("effect")
    eff_layer = None
    if eff:
        es = b.source(eff["from"])
        eff_layer = es.var.layers[0]
        eff_ranges = []
        for i in range(es.var.anim_count):
            ptr = rom.u32(es.var.anims + 4 * i)
            slot, nt = rom.u16(ptr + 2), rom.u8(ptr + 1)
            eff_ranges.append(range(slot, slot + nt))
        taken = set(s for r in eff_ranges for s in r)
        # the base map's own animated tiles move out of the way when they overlap the effect's
        for a in b.anims:
            nt = rom.u8(a["ptr"] + 1)
            if taken & set(range(a["slot"], a["slot"] + nt)):
                new = len(b.ts.slots)
                for _ in range(nt):
                    b.ts.slots.append(("atlas", 0))
                remap = {a["slot"] + k: new + k for k in range(nt)}
                for g in b.layers.values():
                    for row in g:
                        for x, e in enumerate(row):
                            if e is not None and (e & 0x3FF) in remap:
                                row[x] = remap[e & 0x3FF] | (e & 0xC00)
                b.ts.reserve(new, nt)
                a["slot"] = new
        used = {e & 0x3FF for g in b.layers.values() for row in g for e in row if e is not None}
        if used & taken:
            raise SpecError(f"slots {sorted(used & taken)} used by the layers overlap the effect's animated tiles")
        for r in eff_ranges:
            b.ts.reserve(r.start, len(r))
        for i in range(es.var.anim_count):
            ptr = rom.u32(es.var.anims + 4 * i)
            b.anims.append({"ptr": ptr, "slot": rom.u16(ptr + 2), "table": None})
        if len(b.ts.slots) > min(taken):
            raise SpecError(f"tileset ({len(b.ts.slots)} slots) runs into the effect's animated tiles at {min(taken)}")
    # blank slot for empty cells: slot 0 must be transparent
    if any(b.ts.tile(0)):
        raise SpecError("slot 0 is not blank")
    # tile banks for raw tiles
    raw = [i for i, k in enumerate(b.ts.slots) if k[0] == "raw"]
    nstock = b.banks.count()
    new_banks, atlas = [], []
    for i, k in enumerate(b.ts.slots):
        if k[0] == "raw":
            n = raw.index(i)
            if n % 256 == 0:
                new_banks.append(bytearray())
            new_banks[-1] += k[1]
            atlas.append((nstock + n // 256) << 8 | (n % 256))
        else:
            atlas.append(k[1])
    bank_table = b.banks.table
    if new_banks:
        ptrs = [pk.put(lz(bk), f"{spec['name']} tile bank {nstock + j}") for j, bk in enumerate(new_banks)]
        old = [rom.u32(b.banks.table + 4 * j) for j in range(nstock)]
        bank_table = pk.put(b"".join(struct.pack("<I", p) for p in old + ptrs), f"{spec['name']} bank table")
    slot_tab = pk.put(lz(L.encode_slot_table(atlas)), f"{spec['name']} tileset slots")
    # animated sequences
    anim_ptrs = []
    for a in b.anims:
        ptr, slot, table = a["ptr"], a["slot"], a["table"]
        nf, nt = rom.u8(ptr), rom.u8(ptr + 1)
        if table is None and slot == rom.u16(ptr + 2):
            anim_ptrs.append(ptr)
            continue
        frames = []
        for k in range(nf):
            fp = rom.u32(ptr + 4 + 4 * k)
            if table is None:
                frames.append(fp)
                continue
            data = L.anim_frame(rom, fp)
            blob = compress(bytes(table[v] for v in data))
            frames.append(pk.put(struct.pack("<I", rom.u32(fp)) + blob[8:], f"{spec['name']} anim frame"))
        anim_ptrs.append(pk.put(struct.pack("<BBH", nf, nt, slot) + b"".join(struct.pack("<I", f) for f in frames)))
    anim_arr = pk.put(b"".join(struct.pack("<I", p) for p in anim_ptrs)) if anim_ptrs else 0
    # layers
    cols, rows = (b.W + 31) // 32, (b.H + 31) // 32
    chunk_cache = {}
    bg0_used = any(e is not None and (e & 0x3FF) for row in b.layers[0] for e in row)
    if eff_layer and bg0_used:
        raise SpecError("BG0 has tiles and an effect overlay")
    layer_ptrs = []
    if not bg0_used:
        layer_ptrs = [eff_layer or (bvar.layers[0] if (bvar and L.Layer(rom, bvar.layers[0]).create != L.LAYER_TILED) else 0)]
    for bg in ((0, 1, 2, 3) if bg0_used else (1, 2, 3)):
        g = b.layers[bg]
        grid = []
        for cy in range(rows):
            for cx in range(cols):
                ch = bytearray(2048)
                anyt = False
                for ty in range(32):
                    for tx in range(32):
                        y, x = cy * 32 + ty, cx * 32 + tx
                        e = g[y][x] if (y < b.H and x < b.W) else None
                        if e is not None and (e & 0x3FF):
                            struct.pack_into("<H", ch, (ty * 32 + tx) * 2, e)
                            anyt = True
                if not anyt:
                    grid.append(0)
                    continue
                key = bytes(ch)
                if key not in chunk_cache:
                    chunk_cache[key] = pk.put(lz(ch), f"{spec['name']} BG{bg} chunk")
                grid.append(chunk_cache[key])
        hdr = struct.pack("<IiiiiBBH", L.LAYER_TILED, 1024, 1024, 0, 0, cols, rows, 0) + b"".join(struct.pack("<I", p) for p in grid)
        layer_ptrs.append(pk.put(hdr, f"{spec['name']} BG{bg} layer"))
    if not layer_ptrs[0]:
        # BG0 needs a layer object: an all-empty 1 x 1 tiled layer
        layer_ptrs[0] = pk.put(struct.pack("<IiiiiBBHI", L.LAYER_TILED, 1024, 1024, 0, 0, 1, 1, 0, 0), "empty BG0")
    # collision
    walk = pk.put(lz(b.walk), f"{spec['name']} walk collision")
    proj = walk if spec.get("projectile", "walk") == "walk" else pk.put(lz(b.proj), f"{spec['name']} projectile mask")
    # breakable rocks (docs/maps.md 7): decorations on a stock map's sheet; their items are added with the barrier's
    rocks = spec.get("rocks")
    rock_items = []
    if rocks:
        sheet = b.source(rocks["sheet"]).var.sheet
        if b.decos and b.sheet and b.sheet != sheet:
            raise SpecError("rocks: the map already has decorations on another sheet")
        b.sheet = sheet
        for r in rocks["list"]:
            rock_items.append((len(b.decos), r))
            b.decos.append(struct.pack("<iiiiHHHBB", r["x"], r["y"], r["x"] + 32, r["y"] + 32, 0x2000, 0x8000,
                                       r.get("frame", 0), 0x28, 1))
    # decorations, gates
    deco_ptr = pk.put(b"".join(b.decos)) if b.decos else 0
    for g in spec.get("gates", []):
        b.gates.append(bytearray(struct.pack("<HHBBBB", g["x"], g["y"], g.get("barrier", 0), g.get("color", 2), g.get("level", 1), g.get("char", 255))))
    if "gate_edit" in spec:
        for k, ch in spec["gate_edit"].items():
            gg = b.gates[int(k)]
            for f, off in (("barrier", 4), ("color", 5), ("level", 6), ("char", 7)):
                if f in ch:
                    gg[off] = ch[f]
            if "x" in ch:
                struct.pack_into("<H", gg, 0, ch["x"])
            if "y" in ch:
                struct.pack_into("<H", gg, 2, ch["y"])
    gate_ptr = pk.put(b"".join(b.gates)) if b.gates else 0
    # triggers
    # barrier (the stock pattern of zone 8 area 28): an item on the decoration that runs the open script when
    # its condition holds at load (the right character at the level), the on-load script that keeps it open
    # once the flag is set, and the "only X may pass" dialogue trigger's condition
    refs = StoryRefs(rom, spec, pk)
    rec_count, rec_arr = emit_records(b, pk, refs)
    bar = spec.get("barrier")
    items = []
    if bar:
        c, lv, deco, flag, rect = bar["char"], bar["level"], bar.get("decoration", 0), bar["flag"], bar["rect"]
        cond = pk.put(bytes([0x02, 0x0B, 0x00, c, 0x0B, 0x00, c, 0x02, 0x16, 0x00, lv, 0x0E, 0x04, 0x11]), "barrier condition")
        opener = pk.put(bytes([0x00, bar.get("sound", 47), 0x02, 0x2E]) + push_multi(rect) + bytes([0x02, 0x47]) + push_multi(rect)
                        + bytes([0x02, 0x75, 0x01]) + varint_bytes(flag) + bytes([0x02, 0x1B, 0x11]), "barrier open script")
        items.append(pk.put(struct.pack("<IBBBBII", cond, deco, 1, 3, 0, ITEM_SCRIPT_HANDLER, opener), "barrier item"))
        talk = pk.put(bytes([0x02, 0x0B, 0x00, c, 0x0C, 0x01]) + varint_bytes(flag) + bytes([0x02, 0x14, 0x04, 0x11]), "barrier dialogue condition")
        k = bar.get("dialogue_trigger")
        if k is not None:
            b.triggers[k][0] = talk
    for k, r in rock_items:
        items.append(pk.put(struct.pack("<IBBBBII", 0, k, r.get("hits", 3), 3, 0, ITEM_SCRIPT_HANDLER,
                                        int(r.get("drop", "0x083EE6A8"), 0)), "rock item"))
    item_arr = pk.put(b"".join(struct.pack("<I", p) for p in items)) if items else 0
    trig = []
    for cond, data, _ in b.triggers:
        trig.append((cond, pk.put(data)))
    for t in spec.get("triggers", []):
        x1, y1, x2, y2 = t["rect"]
        if t["type"] == "flight":
            wx, wy = t["world"]
            s = pk.put(script_warp(6, 99, 1, wx, wy, music=43), f"{spec['name']} flight script")
            d = struct.pack("<IIhhhhI", L.SCRIPT_CREATE, ACT_PRESS, x1, y1, x2, y2, s)
        elif t["type"] == "warp":
            z, a, v = t["to"]
            x, y = t["at"]
            s = pk.put(script_warp(z, a, v, x, y, music=t.get("music")), f"{spec['name']} warp script")
            d = struct.pack("<IIhhhhI", L.SCRIPT_CREATE, ACT_PRESS, x1, y1, x2, y2, s)
        elif t["type"] == "script":
            s = pk.put(bytes.fromhex(t["bytes"]), f"{spec['name']} script")
            d = struct.pack("<IIhhhhI", L.SCRIPT_CREATE, t.get("activation", ACT_PRESS), x1, y1, x2, y2, s)
        elif t["type"] == "savepad":     # the stock save / switch-characters pad: 16 bytes, no payload
            d = struct.pack("<IIhhhh", L.SAVEPAD_CREATE, ACT_PRESS, x1, y1, x2, y2)
        elif t["type"] == "dialogue":
            d = struct.pack("<IIhhhhI", TRIGGER_DIALOG_COND, TRIGGER_DIALOG_FN, x1, y1, x2, y2, refs.ref(t["payload"]))
        else:
            raise SpecError(f"unknown trigger type {t['type']}")
        trig.append((refs.condition(t.get("condition", 0)), pk.put(d)))
    trig_arr = pk.put(b"".join(struct.pack("<II", c, p) for c, p in trig)) if trig else 0
    # exits (variation triggers)
    ex_ptrs = [pk.put(x) for x in b.exits]
    for x in spec.get("exits", []):
        x1, y1, x2, y2 = x["rect"]
        z, a, v = x["to"]
        tx, ty = x["at"]
        dir_ = DIRS[x.get("dir", "none")] | (x.get("facing", -1) + 1) << 4
        create = L.EDGE_EXIT_CREATE if x.get("type", "edge") == "edge" else L.EXIT_CREATE
        ex_ptrs.append(pk.put(struct.pack("<IIhhhhBBBBhh", create, ACT_EXIT, x1, y1, x2, y2, dir_, z, a, v, tx, ty)))
    ex_arr = pk.put(b"".join(struct.pack("<I", p) for p in ex_ptrs)) if ex_ptrs else 0
    # on-load script
    onload = 0
    ol = spec.get("onload")
    if ol:
        onload = pk.put(script_barrier_onload(ol["barrier_flag"], ol["rect"]), f"{spec['name']} on-load")
    elif bar:
        onload = pk.put(script_barrier_onload(bar["flag"], bar["rect"], bar.get("decoration", 0)), f"{spec['name']} on-load")
    # variation
    var = bytearray(bvar.raw() if bvar else bytes(L.VARIATION_SIZE))
    if not bvar:
        struct.pack_into("<I", var, 0, 0x10101E40)
        var[4:8] = bytes([1, 1, 2, 3])
    if eff:
        es = b.source(eff["from"])
        struct.pack_into("<I", var, 0, es.var.blend)
        var[4] = es.var.priority[0]
    for bg, p in enumerate(spec.get("priorities", [])):
        if p is not None:
            var[4 + bg] = p
    struct.pack_into("<HH", var, 8, b.W * 8 - 239, b.H * 8 - 159)
    struct.pack_into("<II", var, 0xC, len(anim_ptrs), anim_arr)
    struct.pack_into("<4I", var, 0x14, *layer_ptrs)
    struct.pack_into("<III", var, 0x24, len(b.decos), deco_ptr, b.sheet if b.decos else 0)
    sx, sy = spec.get("spawn", b.spawn)
    struct.pack_into("<hh", var, 0x30, sx, sy)
    struct.pack_into("<II", var, 0x34, walk, proj)
    struct.pack_into("<I", var, 0x3C, 0)
    struct.pack_into("<II", var, 0x40, len(ex_ptrs), ex_arr)
    struct.pack_into("<II", var, 0x48, slot_tab, bank_table)
    var_ptr = pk.put(var, f"{spec['name']} variation")
    var_arr = pk.put(struct.pack("<I", var_ptr))
    # MapEntry
    be = b.base.entry if b.base else None
    row = bytearray(L.MAP_ENTRY_SIZE)
    row[0:8] = bytes([spec["zone"], spec["area"], 1, len(trig), rec_count, len(items), 0, len(b.gates)])
    struct.pack_into("<I", row, 0x08, spec.get("flags", be.flags if be else 1))
    struct.pack_into("<I", row, 0x0C, spec.get("name_id", be.name if be else 0xDD))
    struct.pack_into("<IIIII", row, 0x10, trig_arr, rec_arr, item_arr, 0, gate_ptr)
    struct.pack_into("<I", row, 0x24, spec.get("music", be.music if be else 2))
    struct.pack_into("<IIII", row, 0x28, 0, var_arr, onload, 0)
    info = dict(name=spec["name"], zone=spec["zone"], area=spec["area"], size=[b.W * 8, b.H * 8], spawn=[sx, sy],
                slots=len(b.ts.slots), new_tiles=len(raw), chunks=len(chunk_cache), variation=hex(var_ptr))
    return bytes(row), info


def build_all(rom, specs, free, preview=None, buu_rom=None):
    pk = Packer(free)
    rows, infos, sources = [], [], {}
    foreign = {}
    if buu_rom and os.path.exists(buu_rom):
        frm = L.Rom(buu_rom)
        lpal, fpal = L.bg_palette(rom), L.bg_palette(frm, BUU_BG_PALETTE)
        foreign["buu"] = (frm, [0] + [nearest(lpal, L.bgr555_to_rgb(fpal[i])) for i in range(1, 256)])
    for path in specs:
        spec = json.load(open(path))
        need = spec.get("requires")
        if need and need not in foreign:
            print(f"map {spec['name']}: skipped (needs the {need} ROM)")
            continue
        b = Build(rom, spec, sources, foreign)
        for op in spec.get("ops", []):
            b.op(op)
        for op in spec.get("stack_ops", []):
            b.stack_op(op)
        for bg, ops in spec.get("layers", {}).items():
            for op in ops:
                b.layer_op(int(bg), op)
        for op in spec.get("collision", []):
            b.coll_op(b.walk, op)
        if isinstance(spec.get("projectile"), list):
            for op in spec["projectile"]:
                b.coll_op(b.proj, op)
        row, info = emit(b, pk)
        rows.append(row)
        infos.append(info)
        if preview:
            render_preview(b, preview)
    table = pk.reserve((L.MAP_COUNT + len(rows)) * L.MAP_ENTRY_SIZE)
    return {"records": pk.records, "maps": infos, "rows": [r.hex() for r in rows],
            "table_offset": hex(table), "free_start": hex(pk.start), "free_end": hex(pk.free)}


def render_preview(b, out):
    """Composite of the built layers (BG3 back .. BG1 front) and the walk collision overlay."""
    from PIL import Image
    import mapexport as X
    pal = L.bg_palette(b.rom)
    tiles = [b.ts.tile(i) for i in range(len(b.ts.slots))]
    for a in b.anims:                          # frame 0 of each animated sequence at its slot
        ptr = a["ptr"]
        data = L.anim_frame(b.rom, b.rom.u32(ptr + 4))
        if a["table"]:
            data = bytes(a["table"][v] for v in data)
        for k in range(b.rom.u8(ptr + 1)):
            while len(tiles) <= a["slot"] + k:
                tiles.append(bytes(64))
            tiles[a["slot"] + k] = data[64 * k:64 * k + 64]
    tc = X.TileCache(tiles, pal)
    im = Image.new("RGBA", (b.W * 8, b.H * 8), L.bgr555_to_rgb(pal[0]) + (255,))
    prio = b.spec.get("priorities") or (b.base.var.priority if b.base else [1, 1, 2, 3])
    for bg in sorted(b.layers, key=lambda k: (-prio[k], -k)):
        for y, row in enumerate(b.layers[bg]):
            for x, e in enumerate(row):
                if e is not None and (e & 0x3FF):
                    im.alpha_composite(tc.image(e & 0x3FF, e & 0x400, e & 0x800), (x * 8, y * 8))
    os.makedirs(out, exist_ok=True)
    name = b.spec["name"]
    im.save(os.path.join(out, f"{name}.png"))
    X.collision_overlay(im, b.walk).save(os.path.join(out, f"{name}_walk.png"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd", choices=["build", "preview"])
    ap.add_argument("files", nargs="+")
    ap.add_argument("--rom", default="roms/log2.gba")
    ap.add_argument("--free", type=lambda s: int(s, 0), default=FREE_DEFAULT)
    ap.add_argument("--preview")
    ap.add_argument("--out", default="build/maps/preview")
    ap.add_argument("--buu-rom", default="roms/buu.gba", help="Buu's Fury ROM, for specs that import its maps")
    a = ap.parse_args()
    rom = L.Rom(a.rom)
    if a.cmd == "build":
        *specs, out = a.files
        res = build_all(rom, specs, a.free, a.preview, a.buu_rom)
        json.dump(res, open(out, "w"), indent=1)
        for m in res["maps"]:
            print(f"map {m['name']}: zone {m['zone']} area {m['area']} {m['size'][0]}x{m['size'][1]} px, spawn {m['spawn']}, "
                  f"{m['slots']} slots ({m['new_tiles']} new tiles), {m['chunks']} chunks, variation {m['variation']}")
        print(f"{len(res['records'])} records, free {res['free_start']} -> {res['free_end']}, table at {res['table_offset']}")
    else:
        build_all(rom, a.files, a.free, a.out, a.buu_rom)


if __name__ == "__main__":
    main()
