"""Clone and recolour a sprite record (Super Saiyan Hercule) and emit it, plus the relocated sprite
index and display-row tables, as C source compiled into the co-op ROM. Nothing from the ROM is
committed: the output lives in build/gen.

usage: spriteclone.py roms/log2.gba build/gen/sprites_data.c [--preview build/ssj_preview.png]
                      [--extra 158=buu_record_158,159=buu_record_159] [--row-sprite 157]
                      [--krillin-sprite 161] [--old-hercule-sprite 158] [--a16-sprite 175]
  --extra       more records (symbols defined in another generated file, e.g. tools/buu2log2.py's
                build/gen/buu_sprites_data.c) appended to the relocated sprite index at those ids
  --row-sprite  the sprite id the Super Saiyan Hercule display row (22) uses (default 157; a test
                build can point it at a converted Buu record to see it in game)
  --krillin-sprite      sprite of Krillin's row 23 (default 161, tools/krillinsprites.py; 76 = his NPC set)
  --old-hercule-sprite  sprite of old Hercule's row 24 (default 158, the Buu's Fury Hercule; 72 without it)
  --a16-sprite          sprite of Android 16's player row 25 (default 175, a16p_record from tools/a16sprites.py)
Facts: see docs/sprites.md. The game's decoder needs each frame stream to start with the
literal-width op (0 0 1 0000 0 1), then literal bytes (1 + 8 bits), then the end code
(0 0 0 0000000 00); streams are 32-bit little-endian words, bits MSB first.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402
import mapchunks as m  # noqa: E402

SRC_SPRITE = 72          # Hercule
NEW_SPRITE = 157         # first free id after the stock 3..156
SRC_ROW = 21             # Hercule's display row
NEW_ROW = 22
ROW_COUNT = 22
KRILLIN_SPRITE = 161     # Krillin's playable record (tools/krillinsprites.py; 76 is his NPC set)
KRILLIN_ROW = 23
KRILLIN_PORTRAIT = 73    # Portrait_Table: his dialogue portrait (switch screen / dialogue, OBJ palette)
KRILLIN_STATUS_PORTRAIT = 119   # status-screen portrait, appended by tools/krillinsprites.py (portrait_module)
A16_ROW = 25             # Android 16 as a player (party slot 7 in the "Gero's computer" chapter)
A16P_SPRITE = 175        # his player record (tools/a16sprites.py a16p_record)
A16_PORTRAIT = 26        # his dialogue portrait (speaker 26)
A16_STATUS_PORTRAIT = 120   # appended by tools/krillinsprites.py (portrait_module)
OLD_HERCULE_SPRITE = 158 # Hercule in his Buu-saga gi (tools/buu2log2.py)
OLD_HERCULE_ROW = 24
GOHAN_ROW = 0            # a standard ground/flight row (behaviour class 0x030021E0) to copy for Krillin
DISPLAY_DATA = 0x081D967C
HAIR_GOLD, HAIR_SHADE, HAIR_LIGHT, HAIR_BRIGHT = 120, 20, 46, 8   # SSJ Goku's hair indices
BLACK = 1
HIGHLIGHTS = {23, 20, 18, 17, 19, 113, 111, 112}                   # Hercule's brown hair highlights


class BitWriter:
    def __init__(self):
        self.bits = []

    def bit(self, b):
        self.bits.append(b & 1)

    def bits_(self, v, n):
        for i in range(n - 1, -1, -1):
            self.bit((v >> i) & 1)

    def bytes(self):
        bits = self.bits + [0] * ((32 - len(self.bits) % 32) % 32)
        out = bytearray()
        for i in range(0, len(bits), 32):
            word = 0
            for b in bits[i:i + 32]:
                word = (word << 1) | b
            out += struct.pack("<I", word)
        return bytes(out)


def encode_literal(data):
    """Literal-only stream for the game's decompressor (kind 1 header)."""
    w = BitWriter()
    w.bits_(0b001, 3)      # short-code path
    w.bits_(0, 4)          # v = -1
    w.bit(0)               # not a raw run
    w.bit(1)               # literal width 8, no base
    for b in data:
        w.bit(1)
        w.bits_(b, 8)
    w.bits_(0, 3)          # no literal, no match, not short
    w.bits_(0, 7)          # o = 0
    w.bits_(0, 2)          # k = 0 -> end
    stream = w.bytes()
    return struct.pack("<II", 1, len(data)) + stream


def recolour(px, w, h):
    """The afro is the big connected blob of black (index 1) in the upper part of the frame: its
    interior pixels (not touching transparency) turn gold and its brown highlights yellow; eyes,
    moustache and outlines stay black because they are small components or edge pixels."""
    out = [row[:] for row in px]
    limit = max(10, int(h * 0.5))
    seen = [[False] * w for _ in range(h)]
    for sy in range(min(limit, h)):
        for sx in range(w):
            if px[sy][sx] != BLACK or seen[sy][sx]:
                continue
            comp, stack = [], [(sy, sx)]
            seen[sy][sx] = True
            while stack:
                y, x = stack.pop()
                comp.append((y, x))
                for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < limit and 0 <= nx < w and not seen[ny][nx] and px[ny][nx] == BLACK:
                        seen[ny][nx] = True
                        stack.append((ny, nx))
            if len(comp) < 30:
                continue
            top = min(y for y, _ in comp)
            if top > limit // 2:
                continue   # a big black patch low in the frame is clothing, not hair
            for y, x in comp:
                edge = any(not (0 <= y + dy < h and 0 <= x + dx < w) or px[y + dy][x + dx] == 0
                           for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)))
                if not edge:
                    out[y][x] = HAIR_GOLD
            # brown highlights adjacent to the blob become yellow
            blob = set(comp)
            for y, x in list(blob):
                for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < h and 0 <= nx < w and px[ny][nx] in HIGHLIGHTS and ny < limit:
                        out[ny][nx] = HAIR_LIGHT
    return out


def frame_bytes(px, w, h):
    data = bytearray(w * h)
    tw = w // 8
    for ty in range(h // 8):
        for tx in range(tw):
            base = (ty * tw + tx) * 64
            for py in range(8):
                for q in range(8):
                    data[base + py * 8 + q] = px[ty * 8 + py][tx * 8 + q]
    return bytes(data)


def main():
    rom = open(sys.argv[1], "rb").read()
    out_c = sys.argv[2]
    preview = sys.argv[sys.argv.index("--preview") + 1] if "--preview" in sys.argv else None
    extra = {}
    if "--extra" in sys.argv:
        for kv in sys.argv[sys.argv.index("--extra") + 1].split(","):
            k, v = kv.split("=")
            extra[int(k)] = v
    row_sprite = int(sys.argv[sys.argv.index("--row-sprite") + 1]) if "--row-sprite" in sys.argv else NEW_SPRITE
    krillin_sprite = int(sys.argv[sys.argv.index("--krillin-sprite") + 1]) if "--krillin-sprite" in sys.argv else KRILLIN_SPRITE
    old_sprite = int(sys.argv[sys.argv.index("--old-hercule-sprite") + 1]) if "--old-hercule-sprite" in sys.argv else OLD_HERCULE_SPRITE
    a16_sprite = int(sys.argv[sys.argv.index("--a16-sprite") + 1]) if "--a16-sprite" in sys.argv else A16P_SPRITE
    sp = S.Sprites(rom)
    rec = sp.record(SRC_SPRITE)
    groups = sp.groups(rec)
    counts = sp.frame_counts(groups)
    header = rom[rec - S.ROM_BASE:rec - S.ROM_BASE + 0x4C]

    arrays = {}   # descriptor array addr -> list of (xo, yo, w, h, attr, blob bytes)
    assert len(groups) == rom[rec - S.ROM_BASE + 2], "record group count"
    for g in groups:
        for p in g:
            if not p or p in arrays:
                continue
            frames = []
            for fi in range(counts.get(p, 1)):
                xo, yo, w, h, attr, blob = sp.descriptor(p, fi)
                f = sp.frame_pixels(p, fi)
                if not f:
                    frames.append((xo, yo, w, h, attr, None))
                    continue
                _, _, px = f
                npx = recolour(px, w, h)
                frames.append((xo, yo, w, h, attr, encode_literal(frame_bytes(npx, w, h))))
            arrays[p] = frames
    # verify the encoder against the decoder on every frame
    for p, frames in arrays.items():
        for xo, yo, w, h, attr, blob in frames:
            if blob is None:
                continue
            kind, size = struct.unpack_from("<II", blob)
            dec = m.decompress(blob, 8, kind)
            assert len(dec) >= size and dec[:size] == blob_data(blob), "encoder round trip failed"

    if preview:
        cell = 66
        cv = S.Canvas(4 * cell + 40, len(groups) * cell + 4)
        for gi, g in enumerate(groups):
            cv.text(2, gi * cell + 2, str(gi), (255, 255, 0), 2)
            for d in range(4):
                if not g[d]:
                    continue
                xo, yo, w, h, attr, blob = arrays[g[d]][0]
                if blob is None:
                    continue
                data = m.decompress(blob, 8, 1)
                for yy in range(h):
                    for xx in range(w):
                        ci = data[((yy // 8) * (w // 8) + xx // 8) * 64 + (yy % 8) * 8 + xx % 8]
                        if ci:
                            cv.put(40 + d * cell + xx, gi * cell + 2 + yy, sp.pal[ci])
        cv.save(preview)
        print("preview", preview)

    # ---- C source --------------------------------------------------------------------------------
    names = {p: f"ssj_arr_{i}" for i, p in enumerate(arrays)}
    lines = ["/* generated by tools/spriteclone.py from the ROM: Super Saiyan Hercule sprite, relocated",
             " * sprite index and display rows. Build output only; never commit. */",
             '#include "rom.h"', "",
             "typedef struct { signed char xo, yo; u8 w, h; u32 attr; const void *blob; } SpriteDesc;",
             "typedef struct { u8 header[0x4C]; const SpriteDesc *groups[%d][4]; u32 end; } SpriteRecord;" % len(groups), ""]
    blob_id = 0
    blob_names = {}
    for p, frames in arrays.items():
        for fi, (xo, yo, w, h, attr, blob) in enumerate(frames):
            if blob is None:
                continue
            name = f"ssj_blob_{blob_id}"
            blob_id += 1
            blob_names[(p, fi)] = name
            words = ", ".join(f"0x{b:02x}" for b in blob)
            lines.append(f"static const u8 {name}[{len(blob)}] __attribute__((aligned(4))) = {{{words}}};")
    for p, frames in arrays.items():
        entries = []
        for fi, (xo, yo, w, h, attr, blob) in enumerate(frames):
            bn = blob_names.get((p, fi), "0")
            entries.append(f"{{{xo}, {yo}, {w}, {h}, 0x{attr:08x}u, {bn}}}")
        lines.append(f"static const SpriteDesc {names[p]}[{len(frames)}] = {{{', '.join(entries)}}};")
    hdr = ", ".join(f"0x{b:02x}" for b in header)
    grp = ", ".join("{" + ", ".join(names[p] if p else "0" for p in g) + "}" for g in groups)
    lines.append(f"const SpriteRecord ssj_record = {{{{{hdr}}}, {{{grp}}}, 0}};")
    # sprite index: stock words 0..156, the new record at 157, then any extra records (158+)
    idx_words = [struct.unpack_from("<I", rom, S.INDEX - S.ROM_BASE + i * 4)[0] for i in range(NEW_SPRITE)]
    entries = [f"(const void *)0x{v:08x}u" for v in idx_words] + ["&ssj_record"]
    for sid in range(NEW_SPRITE + 1, max(extra, default=NEW_SPRITE) + 1):
        entries.append(f"&{extra[sid]}" if sid in extra else "0")
    for sym in extra.values():
        lines.append(f"extern const char {sym}[];")
    lines.append("const void *const sprite_index[%d] __attribute__((aligned(4))) = {%s};" % (len(entries), ", ".join(entries)))
    # display rows: stock 22 rows then row 22 = Hercule's row with sprite 157, transformed flag, links
    rows = bytearray(rom[DISPLAY_DATA - S.ROM_BASE:DISPLAY_DATA - S.ROM_BASE + ROW_COUNT * 20])
    new = bytearray(rows[SRC_ROW * 20:SRC_ROW * 20 + 20])
    new[0] |= 1            # transformed form
    new[1] = row_sprite
    new[4] = NEW_ROW       # transformedDataIndex (itself)
    new[5] = SRC_ROW       # detransformedDataIndex -> Hercule
    rows[SRC_ROW * 20 + 4] = NEW_ROW   # Hercule's own row now links to the new form
    rows += new
    # row 23 = Krillin (party slot 6, docs/party-8-slots.md): Gohan's row (standard behaviour class, flight
    # timing) with Krillin's playable sprite, his dialogue portrait (+2) and his status portrait (+3), no
    # transformation links
    krillin = bytearray(rows[GOHAN_ROW * 20:GOHAN_ROW * 20 + 20])
    krillin[0] = 0         # a base form, like Hercule's row 21
    krillin[1] = krillin_sprite
    krillin[2] = KRILLIN_PORTRAIT
    krillin[3] = KRILLIN_STATUS_PORTRAIT
    krillin[4] = KRILLIN_ROW
    krillin[5] = KRILLIN_ROW
    rows += krillin
    # row 24 = old Hercule (future prologue, docs/hercule.md): Hercule's row with the Buu-saga sprite; the
    # story switches slot 5 to it with SetCharacterTransformation(5, sprite) and back with (5, 72)
    old = bytearray(rows[SRC_ROW * 20:SRC_ROW * 20 + 20])
    old[0] = 0             # a base form (the story switches it by script)
    old[1] = old_sprite
    old[4] = OLD_HERCULE_ROW
    old[5] = OLD_HERCULE_ROW
    rows += old
    # row 25 = Android 16 as a player (docs/story.md 4g "Gero's computer"): a standard row like Krillin's with
    # his player sprite and portraits, no transformation links
    a16 = bytearray(rows[GOHAN_ROW * 20:GOHAN_ROW * 20 + 20])
    a16[0] = 0
    a16[1] = a16_sprite
    a16[2] = A16_PORTRAIT
    a16[3] = A16_STATUS_PORTRAIT
    a16[4] = A16_ROW
    a16[5] = A16_ROW
    rows += a16
    lines.append("const u8 display_rows[%d] __attribute__((aligned(4))) = {%s};" % (len(rows), ", ".join(f"0x{b:02x}" for b in rows)))
    lines.append(f"const u32 display_row_count = {ROW_COUNT + 4};")
    os.makedirs(os.path.dirname(out_c), exist_ok=True)
    open(out_c, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    total = sum(len(b) for (_, _), b in [((p, fi), fr[5]) for p, frs in arrays.items() for fi, fr in enumerate(frs) if fr[5]])
    print(f"wrote {out_c}: {len(arrays)} descriptor arrays, {blob_id} frames, {total} bytes of frame data")


def blob_data(blob):
    """The raw bytes an encoded blob should decode to (kept beside the blob for the round-trip check)."""
    return blob_data.cache[blob]


blob_data.cache = {}
_orig_encode = encode_literal


def encode_literal(data):  # noqa: F811
    blob = _orig_encode(data)
    blob_data.cache[blob] = bytes(data)
    return blob


if __name__ == "__main__":
    main()
