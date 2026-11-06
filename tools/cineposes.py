r"""Which cine pose each sprite of a built ROM has (docs/cutscenes.md section 5 "Cine", the table of what bodies lack).
usage: python -I tools/cineposes.py build/<dir>/log2-coop.gba [build/<dir>/coop.sym] [--all]

Reads the relocated sprite index (`sprite_index` / `sprite_index_count` in coop.sym) and applies src/coop/cine.c's own
rule (group_of: the group is inside the record's count and each facing's first frame is drawable) to the groups the
poses use: punch 7 / 6 / 8, kick 8 / 6 / 7, hurt 2 / 11, fall and kneel 3 / 2, ki pose 5 / 6 / 7, beam pose 18 (without
it CINE_BLAST's beam is a big blast), charge 9 (records of 20 groups or more) / 5, dash 13 / 1, lying 12 / 3 / 2 (the
tournament's lying table may give a drawn one instead). Prints one line a sprite: the poses it falls back on, or lacks
(shown as the idle stance). --all lists the complete ones too."""
import struct
import sys

args = [a for a in sys.argv[1:] if not a.startswith("--")]
rom = open(args[0], "rb").read()
sym = args[1] if len(args) > 1 else args[0].rsplit("/", 1)[0] + "/coop.sym"
B = 0x08000000
syms = {}
for ln in open(sym):
    p = ln.split()
    if len(p) >= 3:
        syms[p[-1]] = int(p[0], 16)


def u32(a):
    return struct.unpack_from("<I", rom, a - B)[0]


def is_ptr(v):
    return B <= v < B + len(rom) and not v & 3


def desc_ok(d):
    if not is_ptr(d):
        return False
    w, h = rom[d - B + 2], rom[d - B + 3]
    blob = u32(d + 8)
    return w in (8, 16, 32, 64) and h in (8, 16, 32, 64) and is_ptr(blob) and u32(blob) <= 2


def group(rec, g):
    if g >= rom[rec - B + 2]:
        return False
    return all(desc_ok(u32(rec + 0x4C + 0x10 * g + 4 * f)) for f in range(4))


index, count = syms["sprite_index"], u32(syms["sprite_index_count"])
POSES = (("punch", (7, 6, 8)), ("kick", (8, 6, 7)), ("hurt", (2, 11)), ("fall", (3, 2)), ("ki", (5, 6, 7)), ("beam", (18,)),
         ("charge", (9, 5)), ("dash", (13, 1)), ("lie", (12, 3, 2)))
for sid in range(7, count):
    rec = u32(index + 4 * sid)
    if not is_ptr(rec):
        continue
    n = rom[rec - B + 2]
    notes = []
    for name, tries in POSES:
        if name == "charge" and n < 20:
            tries = (5,)
        got = next((g for g in tries if group(rec, g)), None)
        if got is None:
            notes.append(f"no {name}")
        elif got != tries[0]:
            notes.append(f"{name}={got}")
    if notes or "--all" in sys.argv:
        print(f"{sid:3d} groups {n:2d} " + (", ".join(notes) if notes else "complete"))
