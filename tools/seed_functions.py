"""Derive Thumb function start addresses from a GBA ROM for seeding Ghidra.

Sources: every `bl` target inside the code region, plus odd (Thumb) pointers into the code region
that sit in literal pools (code/rodata, the IWRAM image) or inside pointer tables elsewhere in ROM.

usage: seed_functions.py ROM OUT.txt [--code-end 0x28000] [--extra 0x21d74,0x158]
"""
import argparse
import collections
import struct

ROM_BASE = 0x08000000
DATA_IMAGE = (0x7FD6BC, 0x7FE53B)  # .data copied to IWRAM at boot (LoG2 USA)


def is_ptr(w):
    return (0x08000000 <= w < 0x08800000) or (0x02000000 <= w < 0x02040000) or (0x03000000 <= w < 0x03008000)


def bl_targets(data, code_end):
    out = set()
    for i in range(0, code_end, 2):
        h1 = data[i] | data[i + 1] << 8
        if (h1 & 0xF800) != 0xF000:
            continue
        h2 = data[i + 2] | data[i + 3] << 8
        if (h2 & 0xF800) != 0xF800:
            continue
        off = ((h1 & 0x7FF) << 12) | ((h2 & 0x7FF) << 1)
        if off & 0x400000:
            off -= 0x800000
        t = i + 4 + off
        if 0 <= t < code_end and t % 2 == 0:
            out.add(t)
    return out


def pointer_targets(data, code_end, rodata_end):
    words = struct.unpack("<%dI" % (len(data) // 4), data[: len(data) // 4 * 4])
    out = set()
    sources = collections.Counter()
    for j, w in enumerate(words):
        if not (ROM_BASE + 1 <= w < ROM_BASE + code_end and w & 1):
            continue
        off = j * 4
        in_pool = off < rodata_end or DATA_IMAGE[0] <= off < DATA_IMAGE[1]
        in_table = (j > 0 and is_ptr(words[j - 1])) or (j + 1 < len(words) and is_ptr(words[j + 1]))
        if in_pool or in_table:
            out.add(w - ROM_BASE - 1)
            sources[off // 0x40000] += 1
    return out, sources


def relative_vtable_targets(data, code_end, rodata_end):
    """Slot functions of the game's C++ vtables, whose words are offsets relative to the table address.

    The vtables are packed back to back, so instead of scanning for runs we take every absolute
    literal (in code pools, rodata or the IWRAM image) that points into the rodata area as a vtable
    start, and walk each table's slots until a word stops resolving to Thumb code or the next known
    table begins.
    """
    words = struct.unpack("<%dI" % (len(data) // 4), data[: len(data) // 4 * 4])
    lo, hi = ROM_BASE, ROM_BASE + rodata_end  # rodata (and the vtables) are interleaved with code
    starts = set()
    for j in range(0, rodata_end // 4):
        w = words[j]
        if lo <= w < hi and w % 4 == 0:
            starts.add(w)
    img = data[DATA_IMAGE[0]:DATA_IMAGE[1]]
    for w in struct.unpack("<%dI" % (len(img) // 4), img[: len(img) // 4 * 4]):
        if lo <= w < hi and w % 4 == 0:
            starts.add(w)
    # Adjacent vtables would keep resolving against the wrong base, so each slot's target must also
    # look like a function start: a bl target, or preceded by a return / branch / a referenced pool.
    bl = bl_targets(data, code_end)
    pools = set()
    for i in range(0, code_end, 2):
        h = data[i] | data[i + 1] << 8
        if (h & 0xF800) == 0x4800:
            pools.add(((i + 4) & ~3) + (h & 0xFF) * 4)

    def plausible_start(off):
        if off in bl:
            return True
        prev = data[off - 2] | data[off - 1] << 8
        if prev in (0x4770, 0x0000, 0x46C0) or (prev & 0xFF00) == 0xBD00 or 0xE000 <= prev < 0xE800 or prev >= 0xF800:
            return True
        return any((off - k) in pools for k in (2, 4, 8, 12, 16))

    out = set()
    tables = 0
    for table in sorted(starts):
        hits = []
        addr = table
        while addr < hi and addr < table + 24 * 4:
            t = (table + words[(addr - ROM_BASE) // 4]) & 0xFFFFFFFF
            off = t - ROM_BASE - 1
            if not (ROM_BASE + 1 <= t < ROM_BASE + code_end and t & 1):
                break
            if plausible_start(off):
                hits.append(off)
            addr += 4  # a resolving-but-implausible word is skipped (data slot or mid-function), not fatal
        if len(hits) >= 3:
            out.update(hits)
            tables += 1
    return out, tables


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom")
    ap.add_argument("out")
    ap.add_argument("--code-end", type=lambda s: int(s, 0), default=0x28000)
    ap.add_argument("--rodata-end", type=lambda s: int(s, 0), default=0x40000)
    ap.add_argument("--extra", default="0x21d74,0x158", help="comma-separated extra starts (ROM offsets)")
    a = ap.parse_args()

    data = open(a.rom, "rb").read()
    bl = bl_targets(data, a.code_end)
    ptrs, sources = pointer_targets(data, a.code_end, a.rodata_end)
    vt, tables = relative_vtable_targets(data, a.code_end, a.rodata_end)
    seeds = bl | ptrs | vt | {int(x, 0) for x in a.extra.split(",") if x}
    with open(a.out, "w") as f:
        for s in sorted(seeds):
            f.write(f"{ROM_BASE + s:08x}\n")
    print(f"bl targets {len(bl)}, pointer targets {len(ptrs)}, relative-vtable targets {len(vt)} ({tables} tables), total seeds {len(seeds)} -> {a.out}")
    print("pointer sources by 256KB:", " ".join(f"{k * 0x40000:06x}:{v}" for k, v in sorted(sources.items())))


if __name__ == "__main__":
    main()
