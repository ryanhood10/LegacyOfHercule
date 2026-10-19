"""Convert Buu's Fury sprite records into Legacy of Goku II records (palette remapped to LoG2's OBJ
palette) and emit them as C data for the co-op ROM, in the shape tools/spriteclone.py uses.

usage: buu2log2.py roms/log2.gba roms/buu.gba build/gen/buu_sprites_data.c 158=128 [159=28 ...]
           [--template 72] [--preview-dir build] [--report build/gen/buu_sprites_report.md]
           [--map 158 5:3,6:4,...]

  158=128      LoG2 sprite id 158 <- Buu record 128 (emits `buu_record_158`)
  --template   LoG2 record whose group layout (count, frames per group, header) the output copies
               (default 72 = Hercule: the game's animation tables index frames per group, so a
               converted record keeps the template's frame counts, cycling the Buu frames)
  --map        per-id override of the LoG2 group -> Buu animation table (see GROUP_MAP)

What it does, per frame: composites the Buu frame's parts (most frames have one, a few two or
four), applies the record's own 256-byte colour remap table if it has one, maps every Buu palette
index to the nearest LoG2 OBJ palette colour (RGB distance), tiles the result as 8-bit 1D OBJ
tiles sized to the smallest OBJ shape that covers the parts, and compresses it with an LZ encoder
for the game's decompressor (tools/mapchunks.py port; round-tripped on every frame). Left/right
frames that share a Buu blob keep sharing one LoG2 blob with the h-flip attribute, as LoG2 does.
Facts: docs/sprites.md ("Record format", "Buu's Fury").
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mapchunks as m  # noqa: E402
import sprites as S  # noqa: E402
import buu_sprites as B  # noqa: E402

# LoG2 group index -> Buu animation index, for Buu records with the playable/boss layout
# (Buu: 0 idle, 1 walk, 2 run, 3 charge, 4-6 attacks, 7 flex, 9 hurt, 12 shout, 13 stand,
#  17 fall, 18 fist up; LoG2: 0 idle, 1 walk, 2 run, 3 fall, 4 shout, 5 charge/attack (3 frames),
#  6-8 attacks, 11 hurt, 12 dazed, 13 shout, 14 fidget, 17 halo, 18/19 beams, 20 tumble)
GROUP_MAP = {0: 0, 1: 1, 2: 2, 3: 17, 4: 12, 5: 3, 6: 4, 7: 5, 8: 6, 9: 18, 11: 9, 12: 9, 13: 13,
             14: 7, 16: 7, 17: 13, 18: 19, 19: 21, 20: 17}
OBJ_SHAPES = {  # (w, h) -> (shape, size)
    (8, 8): (0, 0), (16, 16): (0, 1), (32, 32): (0, 2), (64, 64): (0, 3),
    (16, 8): (1, 0), (32, 8): (1, 1), (32, 16): (1, 2), (64, 32): (1, 3),
    (8, 16): (2, 0), (8, 32): (2, 1), (16, 32): (2, 2), (32, 64): (2, 3),
}


# ---- LZ encoder for the game's bit stream ---------------------------------------------------------
class BitWriter:
    def __init__(self):
        self.bits = []

    def bit(self, b):
        self.bits.append(b & 1)

    def bits_(self, v, n):
        for i in range(n - 1, -1, -1):
            self.bit((v >> i) & 1)

    def gamma(self, v):
        """Elias-gamma as the decoder reads it: the bits below the leading 1, each followed by a
        continue flag (v >= 2)."""
        assert v >= 2
        n = v.bit_length()
        for i in range(n - 2, -1, -1):
            self.bit((v >> i) & 1)
            self.bit(1 if i else 0)

    def bytes(self):
        bits = self.bits + [0] * ((32 - len(self.bits) % 32) % 32)
        out = bytearray()
        for i in range(0, len(bits), 32):
            word = 0
            for b in bits[i:i + 32]:
                word = (word << 1) | b
            out += struct.pack("<I", word)
        return bytes(out)


def gamma_len(v):
    return 2 * (v.bit_length() - 1)


def match_adjust(dist):
    if dist >= 0x10000:
        return 3
    if dist >= 0x37FF:
        return 2
    if dist >= 0x27F:
        return 1
    if dist <= 127:
        return 4
    return 0


def compress(data):
    """Greedy LZ for the decoder in mapchunks.decompress: literals (9 bits), zero bytes and
    near single-byte copies (7 bits), short matches (dist < 128, len 2..5, 12 bits), gamma
    matches, and repeat-last-offset matches. Returns a kind-1 blob {1, size, stream}."""
    w = BitWriter()
    w.bits_(0b001, 3)      # short-code path
    w.bits_(0, 4)          # v = -1: width op
    w.bit(0)               # not a raw run
    w.bit(1)               # literal width 8, no base
    n = len(data)
    heads = {}
    pos = 0
    last_off = 1
    while pos < n:
        best_len, best_dist = 0, 0
        if pos + 2 <= n:
            key = data[pos:pos + 2]
            for cand in reversed(heads.get(key, ())[-48:]):
                dist = pos - cand
                if dist >= 0x10000:
                    break
                ln = 2
                lim = min(n - pos, 255)
                while ln < lim and data[cand + ln] == data[pos + ln]:
                    ln += 1
                if ln > best_len:
                    best_len, best_dist = ln, dist
                    if ln == lim:
                        break
        # candidate encodings: (cost_bits, kind, length)
        choices = [(9, "lit", 1)]
        b = data[pos]
        if b == 0:
            choices.append((7, "zero", 1))
        for d in range(1, 15):
            if pos - d >= 0 and data[pos - d] == b:
                choices.append((7, "near", d))
                break
        if best_len >= 2:
            if best_dist == last_off:
                choices.append((2 + 2 + gamma_len(best_len), "rep", best_len))
            if best_dist <= 127 and best_len <= 5:
                choices.append((12, "short", best_len))
            adj = match_adjust(best_dist)
            if best_len - adj >= 2:
                hi = best_dist >> 8
                choices.append((2 + gamma_len(hi + 3) + 8 + gamma_len(best_len - adj), "gamma", best_len))
            elif best_dist <= 127 and best_len > 5:
                choices.append((12, "short", 5))
        # pick the lowest cost per byte covered
        cost, kind, ln = min(choices, key=lambda c: (c[0] / c[2], c[0]))
        if kind == "lit":
            w.bit(1)
            w.bits_(b, 8)
        elif kind == "zero":
            w.bits_(0b001, 3)
            w.bits_(1, 4)
        elif kind == "near":
            w.bits_(0b001, 3)
            w.bits_(ln + 1, 4)
            ln = 1
        elif kind == "short":
            w.bits_(0b000, 3)
            w.bits_(best_dist, 7)
            w.bits_(ln - 2, 2)
            last_off = best_dist
        elif kind == "rep":
            w.bits_(0b01, 2)
            w.gamma(2)
            w.gamma(ln)
        else:  # gamma
            w.bits_(0b01, 2)
            w.gamma((best_dist >> 8) + 3)
            w.bits_(best_dist & 0xFF, 8)
            w.gamma(ln - match_adjust(best_dist))
            last_off = best_dist
        for p in range(pos, pos + ln):
            if p + 2 <= n:
                heads.setdefault(data[p:p + 2], []).append(p)
        pos += ln
    w.bits_(0, 3)          # not literal, not match, not short
    w.bits_(0, 7)          # o = 0
    w.bits_(0, 2)          # k = 0 -> end
    blob = struct.pack("<II", 1, n) + w.bytes()
    dec = m.decompress(blob, 8, 1)
    assert dec[:n] == bytes(data) and len(dec) >= n, "encoder round trip failed"
    return blob


# ---- conversion -----------------------------------------------------------------------------------
def nearest_table(src_pal, dst_pal):
    """Buu index -> (LoG2 index, distance). Index 0 stays 0 (transparent)."""
    table = {0: (0, 0)}
    for i in range(1, 256):
        r, g, b = src_pal[i]
        best = min(range(1, 256), key=lambda j: (dst_pal[j][0] - r) ** 2 + (dst_pal[j][1] - g) ** 2 + (dst_pal[j][2] - b) ** 2)
        d = (dst_pal[best][0] - r) ** 2 + (dst_pal[best][1] - g) ** 2 + (dst_pal[best][2] - b) ** 2
        table[i] = (best, d)
    return table


def obj_size(w, h):
    """Smallest OBJ shape covering w x h."""
    for ow, oh in sorted(OBJ_SHAPES, key=lambda s: s[0] * s[1]):
        if ow >= w and oh >= h:
            return ow, oh
    return None


def tile_bytes(px, w, h):
    data = bytearray(w * h)
    tw = w // 8
    for ty in range(h // 8):
        for tx in range(tw):
            base = (ty * tw + tx) * 64
            for py in range(8):
                row = px[ty * 8 + py]
                for q in range(8):
                    data[base + py * 8 + q] = row[tx * 8 + q]
    return bytes(data)


class Converter:
    def __init__(self, log2_rom, buu_rom, template):
        self.l2 = S.Sprites(log2_rom)
        self.bu = B.BuuSprites(buu_rom)
        self.table = nearest_table(self.bu.pal, self.l2.pal)
        self.template = template
        rec = self.l2.record(template)
        groups = self.l2.groups(rec)
        counts = self.l2.frame_counts(groups)
        self.t_counts = [counts.get(g[0], 0) if g[0] else 0 for g in groups]
        self.t_header = bytearray(log2_rom[rec - S.ROM_BASE:rec - S.ROM_BASE + 0x4C])
        self.used = {}      # Buu index -> pixel count (for the report)
        self.blob_cache = {}

    def convert_frame(self, d, remap, bu_rec):
        """Buu descriptor -> (xo, yo, w, h, attr, blob bytes, raw tile bytes) in LoG2 form."""
        box, parts = self.bu.descriptor(d)
        if len(parts) == 1:
            x, y, w, h, attr, b = parts[0]
            key = (b, bu_rec)                                            # left/right share one blob
            if key not in self.blob_cache:
                px = self.bu.part_pixels(w, h, attr & ~0x30000000, b)   # unflipped data, flip stays in attr
                self.blob_cache[key] = self.encode(px, w, h, w, h, remap)
            ow, oh, blob, raw = self.blob_cache[key]
            return (x, y, ow, oh, attr, blob, raw)
        key = (tuple(p[5] for p in parts), tuple(p[0:4] for p in parts), bu_rec)
        if key not in self.blob_cache:
            (xo, yo, w, h), px = self.bu.frame_pixels(d)                 # composite, flips applied
            if w > 64 or h > 64:
                print(f"  warning: frame {d:#x} is {w}x{h}, cropped to 64x64")
                px = [row[:64] for row in px[:64]]
                w, h = min(w, 64), min(h, 64)
            ow, oh = obj_size(w, h)
            shape, size = OBJ_SHAPES[(ow, oh)]
            attr = (parts[0][4] & 0x0FFF0FFF & ~0xC000) | (shape << 14) | (size << 30)
            self.blob_cache[key] = (xo, yo, attr) + self.encode(px, w, h, ow, oh, remap)
        xo, yo, attr, ow, oh, blob, raw = self.blob_cache[key]
        return (xo, yo, ow, oh, attr, blob, raw)

    def encode(self, px, w, h, ow, oh, remap):
        """Remap a [h][w] index map onto an ow x oh OBJ, compress: (ow, oh, blob, raw)."""
        out = [[0] * ow for _ in range(oh)]
        for yy in range(h):
            for xx in range(w):
                ci = px[yy][xx]
                if ci:
                    if remap:
                        ci = remap[ci]
                    self.used[ci] = self.used.get(ci, 0) + 1
                    out[yy][xx] = self.table[ci][0]
        raw = tile_bytes(out, ow, oh)
        return (ow, oh, compress(raw), raw)

    def convert(self, bu_id, group_map):
        """Returns (header bytes, groups [[frames x4]], where frames = list of converted frames)."""
        rec = self.bu.record(bu_id)
        anims = [self.bu.anim(a) if a else [] for a in rec["anims"]]
        header = bytearray(self.t_header)
        header[2] = len(self.t_counts)
        for k in range(4):                        # the Buu record's collision boxes
            header[0x0C + 16 * k:0x1C + 16 * k] = struct.pack("<iiii", *rec["boxes"][k])
        groups = []
        for gi, count in enumerate(self.t_counts):
            if count == 0:
                groups.append(None)
                continue
            src = group_map.get(gi, 0)
            frames = anims[src] if src < len(anims) and anims[src] else anims[0]
            dirs = []
            for d in range(4):
                dirs.append([self.convert_frame(frames[fi % len(frames)][d], rec["remap"], bu_id) for fi in range(count)])
            groups.append(dirs)
        # header +0 (u16) is the record's largest frame in bytes: the entity's OBJ VRAM slot and staging
        # buffer are sized from it (stock: 0x400 for 32x32 bodies, 0x1000 for 64x64 ones), so a 64x64
        # record on a 32x32 template must say so. Byte 3 follows the stock 64x64 bosses (Frieza 34,
        # Perfect Cell 30: 0x18).
        size = max(f[2] * f[3] for g in groups if g for dirs in g for f in dirs)
        if size > struct.unpack_from("<H", header, 0)[0]:
            struct.pack_into("<H", header, 0, size)
            if size >= 0x1000:
                header[3] = max(header[3], 0x18)
        return header, groups, rec


def emit_c(records):
    """records: [(log2 id, header, groups)] -> C source text."""
    lines = ["/* generated by tools/buu2log2.py from the Buu's Fury ROM: sprite records converted to",
             " * LoG2's record format and OBJ palette. Build output only; never commit. */",
             '#include "rom.h"', "",
             "typedef struct { signed char xo, yo; u8 w, h; u32 attr; const void *blob; } BuuDesc;", ""]
    blob_names = {}
    for sid, header, groups in records:
        n = 0
        for g in groups:
            if not g:
                continue
            for dirs in g:
                for fr in dirs:
                    blob = fr[5]
                    if blob not in blob_names:
                        name = f"buu{sid}_blob_{n}"
                        n += 1
                        blob_names[blob] = name
                        words = ", ".join(f"0x{b:02x}" for b in blob)
                        lines.append(f"static const u8 {name}[{len(blob)}] __attribute__((aligned(4))) = {{{words}}};")
        arr_names = {}
        for gi, g in enumerate(groups):
            if not g:
                continue
            for d, dirs in enumerate(g):
                name = f"buu{sid}_g{gi}_d{d}"
                arr_names[(gi, d)] = name
                entries = ", ".join(f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {blob_names[blob]}}}" for (xo, yo, w, h, attr, blob, _) in dirs)
                lines.append(f"static const BuuDesc {name}[{len(dirs)}] = {{{entries}}};")
        hdr = ", ".join(f"0x{b:02x}" for b in header)
        grp = ", ".join("{" + ", ".join(arr_names[(gi, d)] if g else "0" for d in range(4)) + "}" for gi, g in enumerate(groups))
        lines.append(f"typedef struct {{ u8 header[0x4C]; const BuuDesc *groups[{len(groups)}][4]; u32 end; }} BuuRecord{sid};")
        lines.append(f"const BuuRecord{sid} buu_record_{sid} = {{{{{hdr}}}, {{{grp}}}, 0}};")
        lines.append("")
    return "\n".join(lines) + "\n"


def preview(conv, sid, bu_id, header, groups, rec, path):
    """Left: the Buu frames in Buu colours; right: the converted frames in LoG2's palette."""
    cell = 72
    anims = [conv.bu.anim(a) if a else [] for a in rec["anims"]]
    cv = S.Canvas(40 + 4 * cell + 24 + 4 * cell + 8, len(groups) * cell + 16)
    cv.text(40, 2, str(bu_id), (255, 255, 0), 2)
    cv.text(40 + 4 * cell + 24, 2, str(sid), (255, 255, 0), 2)
    for gi, g in enumerate(groups):
        y = 14 + gi * cell
        cv.text(2, y + 2, str(gi), (255, 255, 0), 2)
        if not g:
            continue
        src = GROUP_MAP.get(gi, 0)
        frames = anims[src] if src < len(anims) and anims[src] else anims[0]
        for d in range(4):
            conv.bu.draw(cv, 40 + d * cell + cell // 2, y + cell - 4, frames[0][d], rec["remap"])
            xo, yo, w, h, attr, blob, raw = g[d][0]
            hflip, vflip = attr & 0x10000000, attr & 0x20000000
            ox = 40 + 4 * cell + 24 + d * cell + cell // 2
            for yy in range(h):
                for xx in range(w):
                    ci = raw[((yy // 8) * (w // 8) + xx // 8) * 64 + (yy % 8) * 8 + xx % 8]
                    if ci:
                        px = w - 1 - xx if hflip else xx
                        py = h - 1 - yy if vflip else yy
                        cv.put(ox + xo + px, y + cell - 4 + yo + py, conv.l2.pal[ci])
    cv.save(path)


def main():
    args = sys.argv[1:]
    opts = {"--template": "72", "--preview-dir": None, "--report": None}
    maps = {}
    pos = []
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--map":
            sid = int(args[i + 1])
            maps[sid] = {int(k): int(v) for k, v in (kv.split(":") for kv in args[i + 2].split(","))}
            i += 3
        elif a in opts:
            opts[a] = args[i + 1]
            i += 2
        else:
            pos.append(a)
            i += 1
    log2_rom = open(pos[0], "rb").read()
    buu_rom = open(pos[1], "rb").read()
    out_c = pos[2]
    pairs = [tuple(int(x) for x in p.split("=")) for p in pos[3:]]
    conv = Converter(log2_rom, buu_rom, int(opts["--template"]))
    records = []
    report = [f"# Buu's Fury -> LoG2 sprite conversion (template LoG2 {conv.template}, groups {conv.t_counts})", ""]
    for sid, bu_id in pairs:
        gmap = dict(GROUP_MAP)
        gmap.update(maps.get(sid, {}))
        conv.used = {}
        header, groups, rec = conv.convert(bu_id, gmap)
        records.append((sid, header, groups))
        nblobs = len({fr[5] for g in groups if g for dirs in g for fr in dirs})
        total = sum(len(b) for b in {fr[5] for g in groups if g for dirs in g for fr in dirs})
        rawtotal = sum(len(raw) for raw in {fr[5]: fr[6] for g in groups if g for dirs in g for fr in dirs}.values())
        report.append(f"## LoG2 sprite {sid} <- Buu record {bu_id} ({rec['addr']:#x}, {rec['count']} anims, remap {'yes' if rec['remap'] else 'no'})")
        report.append(f"- {nblobs} frame blobs, {total} bytes compressed (literal-only would be about {rawtotal * 9 // 8})")
        report.append(f"- boxes {rec['boxes']}")
        changed = [(ci, n) for ci, n in sorted(conv.used.items(), key=lambda kv: -kv[1]) if conv.table[ci][1] > 0]
        exact = sum(n for ci, n in conv.used.items() if conv.table[ci][1] == 0)
        report.append(f"- {exact} pixels kept an exact colour, {sum(n for _, n in changed)} pixels in {len(changed)} colours moved to the nearest LoG2 colour:")
        report.append("")
        report.append("| Buu idx | Buu colour | LoG2 idx | LoG2 colour | distance | pixels |")
        report.append("|---|---|---|---|---|---|")
        for ci, n in changed:
            j, d = conv.table[ci]
            r, g, b = conv.bu.pal[ci]
            r2, g2, b2 = conv.l2.pal[j]
            report.append(f"| {ci} | #{r:02x}{g:02x}{b:02x} | {j} | #{r2:02x}{g2:02x}{b2:02x} | {int(d ** 0.5)} | {n} |")
        report.append("")
        print(f"sprite {sid} <- Buu {bu_id}: {nblobs} blobs, {total} bytes, {len(changed)} colours changed")
        if opts["--preview-dir"]:
            os.makedirs(opts["--preview-dir"], exist_ok=True)
            p = os.path.join(opts["--preview-dir"], f"buu_preview_{sid}.png")
            preview(conv, sid, bu_id, header, groups, rec, p)
            report.append(f"- preview: `{p}`")
            report.append("")
    os.makedirs(os.path.dirname(out_c) or ".", exist_ok=True)
    open(out_c, "w", encoding="utf-8", newline="\n").write(emit_c(records))
    if opts["--report"]:
        open(opts["--report"], "w", encoding="utf-8").write("\n".join(report) + "\n")
    print(f"wrote {out_c}")


if __name__ == "__main__":
    main()
