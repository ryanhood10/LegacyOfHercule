"""Map format library for Legacy of Goku II (USA): reads the MapEntry table, map variations, tiled
layers, the tileset (slot table + shared tile banks), animated tile sequences, the global BG palette,
collision bitmaps, triggers, exits and level gates. Shared by tools/mapexport.py (render to PNG) and
tools/mapbuild.py (build new maps). Format notes: docs/maps.md.

All addresses are GBA addresses (ROM 0x08000000 + file offset). Nothing here writes the ROM.
"""
import struct
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mapchunks import unpack, decompress  # noqa: E402,F401  (resource loader + IWRAM decompressor port)

ROM_BASE = 0x08000000
MAP_ENTRIES = 0x08409368
MAP_COUNT = 327
MAP_ENTRY_SIZE = 0x38
VARIATION_SIZE = 0x50
BG_PALETTE = 0x081DA4C8          # 256 x BGR555, the one BG palette every map uses (loaded once)
TILE_BANKS = 0x084DF574          # shared bank table: pointer per 256-tile 8bpp bank (resource)
MAP_NAMES = 0x080E2840           # map name id -> string pointer
LAYER_TILED = 0x080056A1         # TiledLayer create fn (chunked 32x32-entry text BG)
LAYER_TILED_BUU = 0x08008087     # the same layer class in Buu's Fury (identical header and chunks)
LAYER_EFFECT = 0x08004FCD        # effect overlay layer (rain, fog, ...): not a chunk grid
EXIT_CREATE = 0x0800C233         # trigger: exit (press the direction inside the rect)
EDGE_EXIT_CREATE = 0x0800D9C3    # trigger: walk-off exit (map edge)
SCRIPT_CREATE = 0x0800C2C3       # trigger: run a bytecode script
DIALOG_CREATE = 0x0800D7E1       # trigger: play a dialogue sequence
SAVEPAD_CREATE = 0x08010AC7      # trigger: save / switch-characters pad
WORLDLABEL_CREATE = 0x0801A6B3   # trigger (world map only): landing-spot name banner
SLOT_LIMIT = 896                 # VRAM 0x06000000..0x0600E000 holds 896 8bpp tiles (map BGs at 0x0600E000+)


class Rom:
    def __init__(self, data):
        self.data = data if isinstance(data, (bytes, bytearray)) else open(data, "rb").read()

    def off(self, a):
        return a - ROM_BASE

    def u8(self, a):
        return self.data[a - ROM_BASE]

    def s16(self, a):
        return struct.unpack_from("<h", self.data, a - ROM_BASE)[0]

    def u16(self, a):
        return struct.unpack_from("<H", self.data, a - ROM_BASE)[0]

    def u32(self, a):
        return struct.unpack_from("<I", self.data, a - ROM_BASE)[0]

    def s32(self, a):
        return struct.unpack_from("<i", self.data, a - ROM_BASE)[0]

    def read(self, a, n):
        return bytes(self.data[a - ROM_BASE:a - ROM_BASE + n])

    def is_rom(self, a):
        return ROM_BASE <= a < ROM_BASE + len(self.data)

    def resource(self, a):
        """Resource_LoadOrDecompress (0x0801F394): {u32 mode, u32 size} then data; 0 = stored."""
        return bytes(unpack(self.data, a))

    def cstr(self, a):
        o = a - ROM_BASE
        return self.data[o:self.data.index(b"\0", o)].decode("latin-1")


def bgr555_to_rgb(c):
    return ((c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31)


def rgb_to_bgr555(r, g, b):
    return (r * 31 + 127) // 255 | ((g * 31 + 127) // 255) << 5 | ((b * 31 + 127) // 255) << 10


def bg_palette(rom, addr=BG_PALETTE):
    return [rom.u16(addr + 2 * i) for i in range(256)]


# ---- MapEntry -----------------------------------------------------------------------------------
ENTRY_FIELDS = """+0x00 u8 zone, +0x01 u8 area, +0x02 u8 variation count, +0x03 u8 triggers, +0x04 u8 scripts,
+0x05 u8 items, +0x06 u8 objects (chests), +0x07 u8 level gates, +0x08 u32 flags (+8 byte: town/no-enemy flag),
+0x0C name id or script, +0x10 triggers*, +0x14 mapScripts*, +0x18 items*, +0x1C objects*, +0x20 gates*,
+0x24 music id or script, +0x28 variation script, +0x2C variations*, +0x30 on-load script, +0x34 on-leave script"""


class MapEntry:
    def __init__(self, rom, addr, index=None):
        self.rom, self.addr, self.index = rom, addr, index
        b = rom.read(addr, MAP_ENTRY_SIZE)
        (self.zone, self.area, self.nvar, self.ntrig, self.nscript, self.nitem, self.nobj, self.ngate) = b[:8]
        (self.flags, self.name, self.triggers, self.scripts, self.items, self.objects, self.gates,
         self.music, self.var_script, self.variations, self.on_load, self.on_leave) = struct.unpack_from("<12I", b, 8)

    def raw(self):
        return self.rom.read(self.addr, MAP_ENTRY_SIZE)

    def variation(self, v=1):
        return Variation(self.rom, self.rom.u32(self.variations + 4 * (v - 1)))

    def name_text(self):
        if self.name >> 16:
            return f"script {self.name:#x}"
        return self.rom.cstr(self.rom.u32(MAP_NAMES + 4 * self.name)) if self.name != 0xDD else "(none)"

    def trigger_list(self):
        out = []
        for t in range(self.ntrig):
            cond, d = struct.unpack("<II", self.rom.read(self.triggers + 8 * t, 8))
            out.append(Trigger(self.rom, cond, d))
        return out

    def gate_list(self):
        return [Gate(self.rom.read(self.gates + 8 * k, 8)) for k in range(self.ngate)]


def entries(rom, table=MAP_ENTRIES, count=MAP_COUNT):
    return [MapEntry(rom, table + i * MAP_ENTRY_SIZE, i) for i in range(count)]


def find_entry(rom, zone, area, nth=0, table=MAP_ENTRIES, count=MAP_COUNT):
    hits = [e for e in entries(rom, table, count) if e.zone == zone and e.area == area]
    return hits[nth] if len(hits) > nth else None


def condition_text(c):
    """Gate word used by triggers, items, objects, scripts (FUN_08003F06)."""
    if c == 0:
        return "always"
    k = c >> 16
    if k == 1:
        return f"flag {c & 0xFFFF} set"
    if k == 2:
        return f"flag {c & 0xFFFF} clear"
    if k == 3:
        return f"flag {c & 0xFFFF} set (consumed)"
    return f"script {c:#x}"


class Trigger:
    """Trigger record: +0 create fn, +4 activation object getter, +8 rect x1,y1,x2,y2 (s16), +0x10 payload."""

    def __init__(self, rom, cond, addr):
        self.rom, self.cond, self.addr = rom, cond, addr
        self.create, self.activation = rom.u32(addr), rom.u32(addr + 4)
        self.rect = tuple(rom.s16(addr + 8 + 2 * k) for k in range(4))
        self.payload = rom.u32(addr + 0x10)

    def kind(self):
        return {EXIT_CREATE: "exit", EDGE_EXIT_CREATE: "edge-exit", SCRIPT_CREATE: "script",
                DIALOG_CREATE: "dialogue", SAVEPAD_CREATE: "savepad",
                WORLDLABEL_CREATE: "world-label"}.get(self.create, f"create {self.create:#x}")

    def exit_target(self):
        """Exit payload at +0x10: u8 dir (low nibble: 0 none, 1 up, 2 left?, 3 right.. index into the key
        table 0x083ED1B4; high nibble: facing on arrival + 1), u8 zone, u8 area, u8 variation, s16 x, s16 y."""
        b = self.rom.read(self.addr + 0x10, 8)
        return dict(dir=b[0], zone=b[1], area=b[2], variation=b[3],
                    x=struct.unpack_from("<h", b, 4)[0], y=struct.unpack_from("<h", b, 6)[0])


class Gate:
    """Level gate (barrier with a level number), MapEntry +0x20: u16 x, u16 y, u8 kind, u8 b5, u16 level?"""

    def __init__(self, b):
        self.x, self.y = struct.unpack_from("<HH", b, 0)
        self.kind, self.b5 = b[4], b[5]
        self.level = struct.unpack_from("<H", b, 6)[0]
        self.raw = b


# ---- Map_VariationEntry ----------------------------------------------------------------------------
VARIATION_FIELDS = """+0x00 u32 blend (BLDCNT | EVA<<16 | EVB<<24), +0x04 u8[4] BG0..3 priority, +0x08 u16 scroll width,
+0x0A u16 scroll height (map px = +239 / +159), +0x0C anim count, +0x10 anim sequences*, +0x14..+0x20 layer 0..3*,
+0x24 decoration count, +0x28 decorations* (0x18 each), +0x2C sprite sheet (decorations), +0x30 s16 x,y default
spawn, +0x34 walk collision*, +0x38 projectile mask*, +0x3C affine (world map) flag, +0x40 variation trigger count,
+0x44 variation triggers*, +0x48 tileset slot table*, +0x4C tile bank table*"""


class Variation:
    def __init__(self, rom, addr):
        self.rom, self.addr = rom, addr
        b = rom.read(addr, VARIATION_SIZE)
        self.blend = struct.unpack_from("<I", b, 0)[0]
        self.priority = list(b[4:8])
        self.scroll_w, self.scroll_h = struct.unpack_from("<HH", b, 8)
        (self.anim_count, self.anims, l0, l1, l2, l3, self.deco_count, self.decos, self.sheet) = struct.unpack_from("<9I", b, 0xC)
        self.layers = [l0, l1, l2, l3]
        self.spawn = struct.unpack_from("<hh", b, 0x30)
        (self.collision, self.projmask, self.affine, self.vtrig_count, self.vtrigs, self.tileset,
         self.banks) = struct.unpack_from("<7I", b, 0x34)

    def raw(self):
        return self.rom.read(self.addr, VARIATION_SIZE)

    def exit_list(self):
        """Variation triggers (+0x40 count, +0x44 array of pointers to trigger data, always active):
        the map's exits (walk-off edges 0x0800D9C3 and doors 0x0800C233), loaded by FUN_0800BF42."""
        return [Trigger(self.rom, 0, self.rom.u32(self.vtrigs + 4 * k)) for k in range(self.vtrig_count)]

    @property
    def width(self):
        return self.scroll_w + 239

    @property
    def height(self):
        return self.scroll_h + 159


class Layer:
    """TiledLayer header: +0 create fn 0x080056A1, +4/+8 scale x/y (1024 = follows the camera 1:1),
    +0x0C/+0x10 offset x/y (1/1024 px), +0x14 u8 chunk columns, +0x15 u8 chunk rows, +0x16 u16 pad,
    +0x18 chunk pointer grid [rows][cols] (0 = empty chunk). A chunk is a resource that unpacks to
    32 x 32 text-BG entries (2 KB): bits 0-9 VRAM tile slot, 10 h-flip, 11 v-flip (8bpp, no palette bits)."""

    def __init__(self, rom, addr):
        self.rom, self.addr = rom, addr
        self.create = rom.u32(addr) if addr else 0
        self.tiled = self.create in (LAYER_TILED, LAYER_TILED_BUU)
        if self.tiled:
            self.scale = (rom.s32(addr + 4), rom.s32(addr + 8))
            self.offset = (rom.s32(addr + 0xC), rom.s32(addr + 0x10))
            self.cols, self.rows = rom.u8(addr + 0x14), rom.u8(addr + 0x15)
            self.pad = rom.u16(addr + 0x16)
            self.grid = [rom.u32(addr + 0x18 + 4 * i) for i in range(self.cols * self.rows)]
        else:
            self.scale, self.offset, self.cols, self.rows, self.grid = (1024, 1024), (0, 0), 0, 0, []

    def chunk(self, cx, cy, cache=None):
        p = self.grid[cy * self.cols + cx]
        if not p:
            return None
        if cache is not None:
            if p not in cache:
                cache[p] = self.rom.resource(p)
            return cache[p]
        return self.rom.resource(p)

    def entries(self, cache=None):
        """Full entry grid [rows*32][cols*32] (None where the chunk is empty)."""
        W, H = self.cols * 32, self.rows * 32
        out = [[None] * W for _ in range(H)]
        for cy in range(self.rows):
            for cx in range(self.cols):
                ch = self.chunk(cx, cy, cache)
                if ch is None:
                    continue
                for ty in range(32):
                    row = out[cy * 32 + ty]
                    for tx in range(32):
                        row[cx * 32 + tx] = struct.unpack_from("<H", ch, (ty * 32 + tx) * 2)[0]
        return out


# ---- tiles --------------------------------------------------------------------------------------------
class TileBanks:
    """Shared tile banks: bank table entry b -> resource of up to 256 8bpp tiles (64 bytes each)."""

    def __init__(self, rom, table=TILE_BANKS):
        self.rom, self.table, self.cache = rom, table, {}

    def bank(self, b):
        if b not in self.cache:
            self.cache[b] = self.rom.resource(self.rom.u32(self.table + 4 * b))
        return self.cache[b]

    def tile(self, atlas):
        bk = self.bank(atlas >> 8)
        o = (atlas & 0xFF) * 64
        return bk[o:o + 64]

    def count(self):
        n = 0
        while True:
            p = self.rom.u32(self.table + 4 * n)
            if not self.rom.is_rom(p):
                return n
            n += 1


def slot_table(rom, var):
    """Tileset slot table (MapRenderer_UploadStaticTileset 0x080050C8): u16 deltas, atlas[0] = d[0],
    atlas[i] = atlas[i-1] + d[i] + 1 (16-bit). Slot i of VRAM (0x06000000 + 64 i) gets atlas tile
    atlas[i] = bank (high byte, via +0x4C) and tile (low byte)."""
    raw = rom.resource(var.tileset)
    out, cur = [], -1
    for i in range(len(raw) // 2):
        d = struct.unpack_from("<H", raw, 2 * i)[0]
        cur = (cur + 1 + d) & 0xFFFF
        out.append(cur)
    return out


def encode_slot_table(atlas):
    out = bytearray()
    prev = -1
    for a in atlas:
        out += struct.pack("<H", (a - prev - 1) & 0xFFFF)
        prev = a
    return bytes(out)


def anim_sequences(rom, var):
    """Animated tiles (+0x0C count, +0x10 array of pointers). Sequence: u8 frames, u8 tiles per frame,
    u16 first VRAM slot, then per frame a pointer to a frame resource (decoded after a 4-byte skip).
    Returns [(slot, ntiles, frames, [frame pointers])]."""
    out = []
    for i in range(var.anim_count):
        s = rom.u32(var.anims + 4 * i)
        nf, nt, slot = rom.u8(s), rom.u8(s + 1), rom.u16(s + 2)
        out.append((slot, nt, nf, [rom.u32(s + 4 + 4 * k) for k in range(nf)]))
    return out


def anim_frame(rom, ptr):
    """FUN_0800501E calls the IWRAM decompressor directly on frame + 4 (word 0 = packed size; the
    stream sets its own literal width), then DMAs tiles-per-frame x 64 bytes to the sequence's slot."""
    return bytes(decompress(rom.data, ptr - ROM_BASE + 4, 8))


def vram_tiles(rom, var, banks=None, with_anims=True):
    """64-byte tile per VRAM slot (static tileset, then frame 0 of each animated sequence)."""
    banks = banks or TileBanks(rom, var.banks)
    tiles = [banks.tile(a) for a in slot_table(rom, var)]
    if with_anims:
        for slot, nt, nf, frames in anim_sequences(rom, var):
            try:
                data = anim_frame(rom, frames[0])
            except Exception:  # noqa: BLE001
                continue
            for k in range(nt):
                while len(tiles) <= slot + k:
                    tiles.append(bytes(64))
                tiles[slot + k] = data[64 * k:64 * k + 64].ljust(64, b"\0")
    return tiles


# ---- collision ------------------------------------------------------------------------------------------
def bitmap(rom, ptr):
    """256 x 256 tiles, 1 bit per 8x8 tile, 8 u32 words per tile row: word[8*ty + (tx>>5)] bit (tx&31).
    Set = passable (Collision_CheckRect 0x08005D6C: blocked at the first clear bit; outside 2048 px blocked)."""
    if not ptr:
        return bytes(8192)
    return rom.resource(ptr)[:8192].ljust(8192, b"\0")


def bit(bm, tx, ty):
    if not (0 <= tx < 256 and 0 <= ty < 256):
        return 0
    return (bm[(8 * ty + (tx >> 5)) * 4 + ((tx & 31) >> 3)] >> (tx & 7)) & 1


def set_bit(bm, tx, ty, v):
    i = (8 * ty + (tx >> 5)) * 4 + ((tx & 31) >> 3)
    if v:
        bm[i] |= 1 << (tx & 7)
    else:
        bm[i] &= ~(1 << (tx & 7)) & 0xFF


def resource_stored(data):
    """A mode-0 (stored) resource: {0, size} + data, padded to 4."""
    blob = struct.pack("<II", 0, len(data)) + bytes(data)
    return blob + bytes(-len(blob) % 4)
