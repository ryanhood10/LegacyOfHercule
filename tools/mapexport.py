"""Render a Legacy of Goku II (or Buu's Fury) map from the ROM: one PNG per background layer, a composite
in world coordinates, the walk collision and projectile masks as overlays, and a text summary (header
fields, layers, tileset, triggers, exits with their target maps' names, gates). Format: docs/maps.md,
code: tools/maplib.py (the ROM's game is read from its header: LoG2 ALFE or Buu's Fury BG3E).

usage: mapexport.py ZONE AREA [--var V] [--nth N] [--rom roms/log2.gba] [--out build/maps] [--table ADDR --count N]
       mapexport.py --map Z/A [--map Z/A ...] [--sheet FILE]   several maps; --sheet adds a labelled contact sheet of the
                                                               composites (50 %) plus FILE's stem + _half (25 %)
       mapexport.py --list [--rom ...]                list every map: index, zone, area, variation count, name id and
                                                      name, each variation's address and size, non-tiled layer classes
       mapexport.py --all [--out DIR]                 composite + collision PNG of every map variation
       mapexport.py --tileset ZONE AREA [--var V]     sheet of the map's VRAM tile slots (16 per row, slot numbers)
       mapexport.py --buu ...                         Buu's Fury: --rom roms/buu.gba (its table, names, palette and
                                                      trigger kinds come from maplib's BUU profile)
Options: --scale N (default 1), --no-collision, --grid (chunk grid lines on layer PNGs).
Examples: mapexport.py --buu --list > build/maps/buu_maps.txt
          mapexport.py --buu --map 7/1 --map 7/2 --map 1/20 --out build/maps/buu/tournament --sheet build/maps/buu/tournament/sheet.png
"""
import argparse
import os
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import maplib as L  # noqa: E402


def tile_rgba(tile, pal):
    """64-byte 8bpp tile -> list of 64 RGBA (index 0 transparent)."""
    out = []
    for v in tile:
        out.append((0, 0, 0, 0) if v == 0 else L.bgr555_to_rgb(pal[v]) + (255,))
    return out


class TileCache:
    def __init__(self, tiles, pal):
        self.tiles, self.pal, self.cache = tiles, pal, {}

    def image(self, slot, hflip, vflip):
        key = (slot, hflip, vflip)
        if key not in self.cache:
            t = self.tiles[slot] if slot < len(self.tiles) else bytes(64)
            im = Image.new("RGBA", (8, 8))
            im.putdata(tile_rgba(t, self.pal))
            if hflip:
                im = im.transpose(Image.FLIP_LEFT_RIGHT)
            if vflip:
                im = im.transpose(Image.FLIP_TOP_BOTTOM)
            self.cache[key] = im
        return self.cache[key]


def render_layer(layer, tc, chunk_cache=None, grid=False):
    """Layer in its own coordinates (cols*256 x rows*256)."""
    W, H = max(layer.cols, 1) * 256, max(layer.rows, 1) * 256
    im = Image.new("RGBA", (W, H))
    if not layer.tiled:
        return im
    ents = layer.entries(chunk_cache)
    for ty, row in enumerate(ents):
        for tx, e in enumerate(row):
            if e is None:
                continue
            slot = e & 0x3FF
            if slot == 0 and not tc.tiles[0].strip(b"\0"):
                continue
            im.alpha_composite(tc.image(slot, e & 0x400, e & 0x800), (tx * 8, ty * 8))
    if grid:
        d = ImageDraw.Draw(im)
        for cx in range(layer.cols + 1):
            d.line([(cx * 256, 0), (cx * 256, H)], fill=(255, 0, 255, 160))
        for cy in range(layer.rows + 1):
            d.line([(0, cy * 256), (W, cy * 256)], fill=(255, 0, 255, 160))
    return im


def layer_world_origin(layer, cam=(0, 0)):
    """Where the layer's (0,0) lands on screen for a camera at world px cam: scroll = (scale*cam + off)>>10,
    the layer pixel at scroll is drawn at the screen origin."""
    sx = (layer.scale[0] * cam[0] + layer.offset[0]) >> 10
    sy = (layer.scale[1] * cam[1] + layer.offset[1]) >> 10
    return -sx, -sy


def composite(rom, var, layer_imgs, layers, pal):
    """World-coordinate composite of the map rectangle (width x height px): layers drawn back to front
    by BG priority (3 = back), lower BG index in front on ties; layers that scroll at another rate than
    1:1 (parallax) are drawn as they sit with the camera at the origin."""
    W, H = var.width, var.height
    im = Image.new("RGBA", (W, H), L.bgr555_to_rgb(pal[0]) + (255,))
    order = sorted(range(4), key=lambda bg: (-var.priority[bg], -bg))
    for bg in order:
        lay = layers[bg]
        if not lay.tiled:
            continue
        ox, oy = layer_world_origin(lay)
        paste(im, layer_imgs[bg], ox, oy)
    return im


def paste(dst, src, x, y):
    """alpha_composite src at (x, y), clipping negative offsets."""
    sx, sy = max(0, -x), max(0, -y)
    if sx >= src.width or sy >= src.height:
        return
    dst.alpha_composite(src.crop((sx, sy, src.width, src.height)), (max(0, x), max(0, y)))


def collision_overlay(im, bm, color=(255, 0, 0, 110), passable=False):
    W, H = im.size
    ov = Image.new("RGBA", (W, H))
    d = ImageDraw.Draw(ov)
    for ty in range((H + 7) // 8):
        for tx in range((W + 7) // 8):
            b = L.bit(bm, tx, ty)
            if (b == 1) == passable:
                d.rectangle([tx * 8, ty * 8, tx * 8 + 7, ty * 8 + 7], fill=color)
    out = im.copy()
    out.alpha_composite(ov)
    return out


def target_text(rom, t):
    """'-> zone 7 area 2 var 1 (224, 772) dir 1 "Papaya Island"' for an exit trigger."""
    x = t.exit_target()
    name = rom.map_name(x["zone"], x["area"])
    s = f"-> zone {x['zone']} area {x['area']} var {x['variation']} at ({x['x']}, {x['y']}) dir {x['dir']:#x}"
    return s + (f" {name!r}" if name is not None else " (no such map)")


def summary(rom, e, v, var, layers):
    lines = [f"map entry {e.index} @ {e.addr:#010x}: zone {e.zone} area {e.area} variation {v}/{e.nvar}, name {e.name} {e.name_text()!r}",
             f"  music {e.music:#x}, flags {e.flags:#x}, triggers {e.ntrig}, scripts {e.nscript}, items {e.nitem}, objects {e.nobj}, gates {e.ngate}",
             f"  on-load {e.on_load:#x} on-leave {e.on_leave:#x} variation script {e.var_script:#x}",
             f"variation @ {var.addr:#010x}: size {var.width} x {var.height} px (scroll {var.scroll_w} x {var.scroll_h}), spawn {var.spawn}",
             f"  priorities {var.priority}, blend {var.blend:#x}, affine {var.affine}, decorations {var.deco_count} @ {var.decos:#x}",
             f"  tileset {var.tileset:#x} ({len(L.slot_table(rom, var))} slots), banks {var.banks:#x}, anims {var.anim_count} @ {var.anims:#x}",
             f"  collision {var.collision:#x}, projectile mask {var.projmask:#x}, variation triggers {var.vtrig_count} @ {var.vtrigs:#x}"]
    for bg, lay in enumerate(layers):
        if lay.tiled:
            used = sum(1 for p in lay.grid if p)
            lines.append(f"  BG{bg} @ {lay.addr:#x}: {lay.cols} x {lay.rows} chunks ({used} used), scale {lay.scale}, offset "
                         f"({lay.offset[0] / 1024:g}, {lay.offset[1] / 1024:g}) px, pad {lay.pad:#x}")
        else:
            lines.append(f"  BG{bg} @ {lay.addr:#x}: {lay.kind()} (create {lay.create:#x}, not a chunk grid)")
    for k, t in enumerate(e.trigger_list()):
        s = f"  trigger {k} [{L.condition_text(t.cond)}] {t.kind()} rect {t.rect}"
        if t.is_exit():
            s += " " + target_text(rom, t)
        else:
            s += f" payload {t.payload:#x} activation {t.activation:#x}"
        lines.append(s)
    for k, t in enumerate(var.exit_list()):
        if t.is_exit():
            lines.append(f"  exit {k}: {t.kind()} rect {t.rect} {target_text(rom, t)}")
        else:
            lines.append(f"  exit {k}: {t.kind()} rect {t.rect} payload {t.payload:#x} activation {t.activation:#x}")
    for k, g in enumerate(e.gate_list()):
        lines.append(f"  gate {k}: ({g.x},{g.y}) kind {g.kind} b5 {g.b5} level {g.level} raw {g.raw.hex()}")
    return "\n".join(lines)


def export(rom, e, v, out, scale=1, coll=True, grid=False, sheet=None):
    """Write the map's PNGs and summary under out; with sheet (a list) also collect (label, composite) for a contact sheet."""
    var = e.variation(v)
    pal = L.bg_palette(rom)
    layers = [L.Layer(rom, a) for a in var.layers]
    tc = TileCache(L.vram_tiles(rom, var), pal)
    cache = {}
    imgs = [render_layer(lay, tc, cache, grid) for lay in layers]
    base = os.path.join(out, f"z{e.zone}a{e.area}v{v}")
    os.makedirs(out, exist_ok=True)
    for bg, im in enumerate(imgs):
        if layers[bg].tiled:
            sv(im, f"{base}_bg{bg}.png", scale)
    comp = composite(rom, var, imgs, layers, pal)
    sv(comp, f"{base}.png", scale)
    if coll:
        bm = L.bitmap(rom, var.collision)
        sv(collision_overlay(comp, bm), f"{base}_walk.png", scale)
        pm = L.bitmap(rom, var.projmask)
        sv(collision_overlay(comp, pm, (0, 80, 255, 110)), f"{base}_proj.png", scale)
    txt = summary(rom, e, v, var, layers)
    open(f"{base}.txt", "w", encoding="utf-8").write(txt + "\n")
    if sheet is not None:
        label = f"{e.zone}/{e.area}" + (f" v{v}" if e.nvar > 1 else "") + f"  {e.name_text()}  {var.width}x{var.height}"
        sheet.append((label, comp))
    return txt


def sv(im, path, scale):
    if scale != 1:
        im = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
    im.save(path)


def contact_sheet(items, path, scale=0.5, small=0.25):
    """Grid of the composites at scale with a label over each; also path's stem + '_half' at the small scale."""
    try:
        font = ImageFont.load_default(size=16)
    except TypeError:  # older Pillow
        font = ImageFont.load_default()
    n = len(items)
    cols = max(1, int(n ** 0.5 + 0.999))
    rows = (n + cols - 1) // cols
    cw = max(int(im.width * scale) for _, im in items) + 16
    ch = max(int(im.height * scale) for _, im in items) + 16 + 22
    sheet = Image.new("RGBA", (cols * cw, rows * ch), (24, 24, 24, 255))
    d = ImageDraw.Draw(sheet)
    for i, (label, im) in enumerate(items):
        x, y = (i % cols) * cw + 8, (i // cols) * ch + 8
        d.text((x, y), label, fill=(255, 255, 255, 255), font=font)
        th = Image.Image.resize(im, (max(1, int(im.width * scale)), max(1, int(im.height * scale))), Image.LANCZOS)
        sheet.alpha_composite(th, (x, y + 22))
    sheet.save(path)
    stem, ext = os.path.splitext(path)
    k = small / scale
    sheet.resize((max(1, int(sheet.width * k)), max(1, int(sheet.height * k))), Image.LANCZOS).save(f"{stem}_half{ext}")
    return path


def tileset_sheet(rom, var, path):
    pal = L.bg_palette(rom)
    tiles = L.vram_tiles(rom, var)
    tc = TileCache(tiles, pal)
    n = len(tiles)
    rows = (n + 15) // 16
    im = Image.new("RGBA", (16 * 12 + 30, rows * 12), (40, 40, 40, 255))
    d = ImageDraw.Draw(im)
    for i in range(n):
        x, y = 30 + (i % 16) * 12, (i // 16) * 12
        im.alpha_composite(tc.image(i, 0, 0), (x, y))
        if i % 16 == 0:
            d.text((0, y), str(i), fill=(255, 255, 255, 255))
    im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(path)


def list_maps(rom, table, count):
    """One line per MapEntry: index, zone, area, variation count, name id and name, then each variation's
    address and size; a variation with a non-tiled layer class (effect overlay, the affine world map) says so."""
    for e in L.entries(rom, table, count):
        parts = []
        for v in range(1, e.nvar + 1):
            var = e.variation(v)
            s = f"v{v} {var.addr:#010x} {var.width}x{var.height}"
            notes = []
            for bg, addr in enumerate(var.layers):
                lay = L.Layer(rom, addr)
                if addr and not lay.tiled:
                    notes.append(f"BG{bg}:{lay.kind()}")
            if var.affine:
                notes.insert(0, "affine")
            if notes:
                s += " [" + " ".join(notes) + "]"
            parts.append(s)
        print(f"{e.index:3d} z{e.zone:2d} a{e.area:3d} vars {e.nvar} name {e.name:3d} {e.name_text()!r:34} | " + " | ".join(parts))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("zone", nargs="?", type=int)
    ap.add_argument("area", nargs="?", type=int)
    ap.add_argument("--map", action="append", default=[], metavar="Z/A", help="a map to export (repeatable; with --sheet)")
    ap.add_argument("--var", type=int, default=0, help="variation (default: all)")
    ap.add_argument("--nth", type=int, default=0)
    ap.add_argument("--rom", default=None, help="default roms/log2.gba, or roms/buu.gba with --buu")
    ap.add_argument("--buu", action="store_true", help="Buu's Fury (roms/buu.gba): its tables and trigger kinds")
    ap.add_argument("--out", default="build/maps")
    ap.add_argument("--table", type=lambda s: int(s, 0), default=None, help="MapEntry table (default: the game's)")
    ap.add_argument("--count", type=int, default=None)
    ap.add_argument("--scale", type=int, default=1)
    ap.add_argument("--no-collision", action="store_true")
    ap.add_argument("--grid", action="store_true")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--tileset", action="store_true")
    ap.add_argument("--sheet", default=None, help="contact sheet PNG of the exported composites (50 %%, labelled) + _half (25 %%)")
    a = ap.parse_args()
    rom = L.Rom(a.rom or ("roms/buu.gba" if a.buu else "roms/log2.gba"))
    if a.buu and rom.game is not L.BUU:
        sys.exit("--buu given but the ROM is not Buu's Fury (header code BG3E)")
    if a.list:
        list_maps(rom, a.table, a.count)
        return
    if a.all:
        for e in L.entries(rom, a.table, a.count):
            for v in range(1, e.nvar + 1):
                if e.variation(v).affine:
                    continue
                try:
                    export(rom, e, v, a.out, 1, not a.no_collision)
                except Exception as ex:  # noqa: BLE001
                    print(f"map {e.index} z{e.zone}a{e.area}v{v}: {ex}")
        return
    wanted = [tuple(int(x) for x in m.split("/")) for m in a.map]
    if a.zone is not None:
        wanted.append((a.zone, a.area))
    if not wanted:
        ap.error("give ZONE AREA, --map Z/A, --list or --all")
    sheet = [] if a.sheet else None
    for zone, area in wanted:
        e = L.find_entry(rom, zone, area, a.nth, a.table, a.count)
        if e is None:
            sys.exit(f"no map zone {zone} area {area}")
        vs = [a.var] if a.var else range(1, e.nvar + 1)
        for v in vs:
            if a.tileset:
                p = os.path.join(a.out, f"z{e.zone}a{e.area}v{v}_tiles.png")
                os.makedirs(a.out, exist_ok=True)
                tileset_sheet(rom, e.variation(v), p)
                print("wrote", p)
                continue
            print(export(rom, e, v, a.out, a.scale, not a.no_collision, a.grid, sheet))
    if sheet:
        os.makedirs(os.path.dirname(os.path.abspath(a.sheet)), exist_ok=True)
        print("wrote", contact_sheet(sheet, a.sheet))


if __name__ == "__main__":
    main()
