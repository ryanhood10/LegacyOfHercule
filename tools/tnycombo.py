"""The tournament's team combos, from Ryan's doc (docs/tournament-combos.md) to C data (build/gen/tny_combo_data.c).

The doc is the source: section 3's tables are read as they stand, so an edit there is an edit in the game (the format is
the doc's section 1). Each combo is a heading `### <title> (`ID`)` with ID one of tny_combo.h's COMBO_* names without the
prefix, a `Name:` line (the caption) and a table | at | who | does | how |:
  at    the frame the step starts (60 a second) from the combo's start, or +N: N frames after the row above (rows
        may interleave several fighters' sequences: they are sorted by frame, rows of one frame in table order)
  who   A (fighter 0, the pair's lead), B (fighter 1), T (the target), several (A B), or - (a screen step)
  does  the verb, how its numbers / words (section 1 of the doc lists them)
Out (C): one TComboStep array per combo, sorted by frame, and tny_combo_defs[COMBO_COUNT] (src/coop/tny_combo_run.h).
Every COMBO_* must have a table that ends with an `end` row.
Colours (the doc's section 4, a table | colour | core | hi | mid | lo | edge |): five OBJ palette indices each, brightest
first. Out: tny_combo_tints[], a remap per colour of the energy arts' own ramps (FAMILIES) onto those five, and with
--rom, tny_combo_beams[]: the Kamehameha beam's frame sets with their tiles redrawn in each colour a `beam` row (or a
kit's move) uses.
The kits' table (the same section, | fighter | move | colour |: a TF_* name without the prefix, his tny_kits slot 1-6
first in the move cell, a colour of the table above): tny_kit_colours[], the colour of a fighter's own move outside the
combos (tny_moves.c; src/coop/tny_colours.c paints it). The fighter names come from tourney.h beside the header.

usage: python -I tools/tnycombo.py docs/tournament-combos.md src/coop/tny_combo.h OUT.c [--rom roms/log2.gba]
"""
import os
import re
import sys

OPS = ["END", "GO", "TELE", "POSE", "FACE", "PUNCH", "BLAST", "BALL", "BEAM", "DISC", "SHAKE", "SOUND", "FLASH",
       "SCREEN", "KNOCK", "CAPTION", "SBC", "COPY"]
ATTACKS = ("PUNCH", "BLAST", "BALL", "BEAM", "DISC", "SBC")
POSES = ["idle", "walk", "hurt", "fall", "shout", "charge", "punch", "kick", "dash", "special", "beam", "beam2", "ki",
         "lie"]
CP_RAW, CP_KEEP = 0x40, 0xFF
SCREEN_OPS = {"SHAKE", "SOUND", "SCREEN", "CAPTION", "END"}
TCOL_MAX = 12                   # src/coop/tny_combo_run.h
# The energy arts' own ramps, brightest first (OBJ palette indices; docs/sprites.md: one shared palette, 0x081DA6C8):
# a colour maps each to its own five. Later families win an index two share (76: the beam's edge, the ki's darker ring).
FAMILIES = [
    ("orb", [2, 106, 247, 252, 26]),    # Super 13's orbs (s13_orb_*): white core, pink, rose, red, dark red
    ("ki", [2, 184, 191, 76, 196]),     # the blue ki blast (kr_kiblast, tools/krillinsprites.py)
    ("beam", [43, 67, 68, 74, 76]),     # the Kamehameha beam (the ROM's tiles 0x086E29D8..0x086E3458)
]
BEAM_SETS = 0x086E3458          # the beam's four facings' frame sets (Beam_Fire 0x08014478: facing * 0x60)
STEP_WORDS = ("ki", "orb", "large", "medium", "bigbang", "hold", "front", "through", "pin", "charge", "kick",
              "special", "loop", "off")


def fail(where, msg):
    sys.exit(f"tnycombo: {where}: {msg}")


def combo_ids(header):
    src = open(header, encoding="utf-8").read()
    body = re.search(r"enum\s*\{(.*?COMBO_COUNT)", src, re.S).group(1)
    return [n for n in re.findall(r"\bCOMBO_(\w+)", body) if n != "COUNT"]


def weight(tok, where):
    try:
        w = float(tok)
    except ValueError:
        fail(where, f"weight {tok!r} is not a number")
    if w < 0 or w > 100:
        fail(where, f"weight {w} out of range (0-100 hits)")
    return int(round(w * 10))


def num(tok, where, lo=-500, hi=2000):
    try:
        v = int(tok, 0)
    except ValueError:
        fail(where, f"{tok!r} is not a number")
    if not lo <= v <= hi:
        fail(where, f"{v} out of range ({lo}..{hi})")
    return v


def pose_kind(tok, where):
    t = tok.lower()
    if t in POSES:
        return POSES.index(t)
    m = re.fullmatch(r"g(\d+)", t)
    if m and int(m.group(1)) < 32:
        return CP_RAW + int(m.group(1))
    fail(where, f"pose {tok!r}: one of {', '.join(POSES)} or gN (group N)")


def frames_of(tok, where):
    m = re.fullmatch(r"(\d+)(?:-(\d+))?", tok)
    if not m:
        fail(where, f"frames {tok!r}: F or F-G")
    a = int(m.group(1))
    return a, int(m.group(2)) if m.group(2) else a


def take_colour(args, colours, where):
    """-> (args without a colour word, colour id: 0 none, else 1 + its row in the colour table)"""
    col, rest = 0, []
    for t in args:
        if t.lower() in colours:
            if col:
                fail(where, "two colours")
            col = colours.index(t.lower()) + 1
        else:
            rest.append(t)
    return rest, col


def step(verb, args, where, colours):
    """-> (op, a, b, c, d, col)"""
    v = verb.lower()
    col = 0
    if v in ("blast", "ball", "beam"):
        args, col = take_colour(args, colours, where)
        if v == "blast" and col and len(args) == 1:     # a coloured blast is the ki shot unless `orb`
            args = args + ["ki"]
    return step_ops(v, args, where) + (col,)


def step_ops(v, args, where):
    """-> (op, a, b, c, d)"""
    n = len(args)
    if v in ("dash", "walk", "slide"):
        if n != 3:
            fail(where, f"{v} X Y N (px from him on my side, px down, frames)")
        kind = {"dash": POSES.index("dash"), "walk": POSES.index("walk"), "slide": CP_KEEP}[v]
        return "GO", num(args[0], where), num(args[1], where), num(args[2], where, 0, 600), kind
    if v == "teleport":
        if n != 2:
            fail(where, "teleport X Y")
        return "TELE", num(args[0], where), num(args[1], where), 0, 0
    if v == "pose":
        if not 1 <= n <= 4:
            fail(where, "pose NAME [F | F-G] [N | loop N]")
        kind = pose_kind(args[0], where)
        f0, f1 = frames_of(args[1], where) if n > 1 else (0, 0)
        d = 0
        if n > 2:
            if args[2].lower() == "loop":
                if n != 4:
                    fail(where, "pose NAME F-G loop N")
                d = -num(args[3], where, 1, 60)
            else:
                d = num(args[2], where, 0, 600)
        return "POSE", kind, f0, f1, d
    if v == "face":
        to = (args[0].lower() if args else "a")
        if to not in ("a", "b", "him", "away"):
            fail(where, "face [A | B | him | away]")
        return "FACE", {"a": 0, "him": 0, "b": 1, "away": 2}[to], 0, 0, 0
    if v == "punch":
        if not 1 <= n <= 2:
            fail(where, "punch W [kick | special]")
        kind = 0
        if n == 2:
            kind = {"kick": POSES.index("kick"), "special": POSES.index("special"), "punch": POSES.index("punch")}.get(args[1].lower())
            if kind is None:
                fail(where, "punch W [kick | special]")
        return "PUNCH", weight(args[0], where), kind, 0, 0
    if v == "blast":
        if not 1 <= n <= 2 or (n == 2 and args[1].lower() not in ("ki", "orb")):
            fail(where, "blast W [ki | orb] [COLOUR]")
        return "BLAST", weight(args[0], where), 1 if n == 2 and args[1].lower() == "ki" else 0, 0, 0
    if v == "ball":
        usage = "ball W [large | medium | bigbang] [hold N] [front] [COLOUR]"
        if n < 1:
            fail(where, usage)
        size, hold, front, rest = 1, 0, 0, [a.lower() for a in args[1:]]
        while rest:
            t = rest.pop(0)
            if t in ("large", "medium", "bigbang"):
                size = {"medium": 1, "large": 2, "bigbang": 3}[t]
            elif t == "hold" and rest:
                hold = num(rest.pop(0), where, 1, 240)
            elif t == "front":
                front = 1
            else:
                fail(where, usage)
        if front and not hold:
            fail(where, "ball: front is where a held orb waits (hold N front)")
        return "BALL", weight(args[0], where), size, hold, front
    if v == "beam":
        if not 2 <= n <= 3 or (n == 3 and args[2].lower() != "through"):
            fail(where, "beam W N [through] [COLOUR]")
        return "BEAM", weight(args[0], where), num(args[1], where, 1, 240), 1 if n == 3 else 0, 0
    if v == "disc":
        usage = "disc W [hold N] [pin]"
        if n < 1:
            fail(where, usage)
        hold, pin, rest = 0, 0, [a.lower() for a in args[1:]]
        while rest:
            t = rest.pop(0)
            if t == "hold" and rest:
                hold = num(rest.pop(0), where, 1, 240)
            elif t == "pin":
                pin = 1
            else:
                fail(where, usage)
        return "DISC", weight(args[0], where), hold, pin, 0
    if v == "sbc":
        usage = "sbc W N [through] [charge C]"
        if n < 2:
            fail(where, usage)
        through, charge, rest = 0, 1, [a.lower() for a in args[2:]]
        while rest:
            t = rest.pop(0)
            if t == "through":
                through = 1
            elif t == "charge" and rest:
                charge = num(rest.pop(0), where, 1, 240)
            else:
                fail(where, usage)
        return "SBC", weight(args[0], where), num(args[1], where, 1, 240), through, charge
    if v == "copy":
        usage = "copy K X Y (copy K of his body there: K 1-3) | copy K off (K 0: all)"
        if n == 2 and args[1].lower() == "off":
            return "COPY", num(args[0], where, 0, 3), 0, 0, 1
        if n != 3:
            fail(where, usage)
        return "COPY", num(args[0], where, 1, 3), num(args[1], where), num(args[2], where), 0
    if v == "shake":
        if n != 2:
            fail(where, "shake N S (frames, strength)")
        return "SHAKE", num(args[0], where, 1, 240), num(args[1], where, 1, 200), 0, 0
    if v == "sound":
        if n != 1:
            fail(where, "sound ID")
        return "SOUND", num(args[0], where, 0, 255), 0, 0, 0
    if v == "flash":
        return "FLASH", num(args[0], where, 1, 120) if n else 4, 0, 0, 0
    if v == "screen":
        if n != 1:
            fail(where, "screen N (frames)")
        return "SCREEN", num(args[0], where, 1, 120), 0, 0, 0
    if v == "knockdown":
        if n != 2:
            fail(where, "knockdown LIE UP (frames lying, frames getting up)")
        return "KNOCK", num(args[0], where, 0, 600), num(args[1], where, 1, 120), 0, 0
    if v == "caption":
        return "CAPTION", 0, 0, 0, 0
    if v == "end":
        return "END", 0, 0, 0, 0
    fail(where, f"unknown step {verb!r}")


def who_bits(cell, op, where):
    if cell.strip() in ("", "-"):
        if op not in SCREEN_OPS:
            fail(where, "this step needs a who (A, B, T)")
        return 0
    bits = 0
    for tok in re.split(r"[\s,&+]+", cell.strip()):
        if not tok:
            continue
        b = {"A": 1, "B": 2, "T": 4}.get(tok.upper())
        if b is None:
            fail(where, f"who {tok!r}: A, B or T")
        bits |= b
    if op in ("KNOCK",) and bits != 4:
        fail(where, "knockdown is the target's (T)")
    if (op in ATTACKS or op == "COPY") and bits & 4:
        fail(where, "the target does not attack (A or B)")
    return bits


def colour_tables(lines, doc_path):
    """the tables of the section whose heading names Colours, by their header row's first cell:
    -> {"colour": [(where, cells)], "fighter": [...]}"""
    out, inside, table = {"colour": [], "fighter": []}, False, None
    for no, line in enumerate(lines, 1):
        where = f"{doc_path}:{no}"
        if line.startswith("## "):
            inside, table = "colour" in line.lower(), None
            continue
        if not inside:
            continue
        if not line.startswith("|"):
            table = None
            continue
        if re.match(r"\|\s*[-:]", line):
            continue
        cells = [c.strip().strip("`") for c in line.strip().strip("|").split("|")]
        head = cells[0].lower()
        if head in ("colour", "color", "fighter"):
            table = "fighter" if head == "fighter" else "colour"
            continue
        if table is None:
            fail(where, "a table in the colours section starts with a | colour | or a | fighter | header row")
        out[table].append((where, cells))
    return out


def parse_colours(lines, doc_path):
    """the colour table (in a section whose heading names Colours): -> [(name, [5 palette indices])]"""
    out = []
    for where, cells in colour_tables(lines, doc_path)["colour"]:
        if len(cells) < 6:
            fail(where, "a colour row is | colour | core | hi | mid | lo | edge |")
        name = cells[0].lower()
        if not re.fullmatch(r"[a-z]+", name) or name in [c[0] for c in out] or name in STEP_WORDS:
            fail(where, f"colour {name!r}: one plain word, once, not a step's word")
        out.append((name, [num(c, where, 1, 255) for c in cells[1:6]]))
    if len(out) >= TCOL_MAX:
        fail(doc_path, f"at most {TCOL_MAX - 1} colours")
    return out


def fighter_ids(header):
    """tourney.h's TF_* roster (beside tny_combo.h)"""
    src = open(os.path.join(os.path.dirname(header), "tourney.h"), encoding="utf-8").read()
    body = re.search(r"typedef enum\s*\{(.*?)TF_COUNT", src, re.S).group(1)
    body = re.sub(r"//[^\n]*", "", body)
    return re.findall(r"\bTF_(\w+)", body)


def parse_kits(lines, doc_path, fighters, colours):
    """the kits' table: -> [(TF name, slot 0-5, colour id)]"""
    out = []
    for where, cells in colour_tables(lines, doc_path)["fighter"]:
        if len(cells) < 3:
            fail(where, "a kits' row is | fighter | move | colour |")
        name, move, col = cells[0].upper(), cells[1], cells[2].lower()
        if name.startswith("TF_"):
            name = name[3:]
        if name not in fighters:
            fail(where, f"fighter {cells[0]!r}: a TF_* name of tourney.h without the prefix (FRIEZA, KING_COLD...)")
        m = re.match(r"(\d+)\b", move)
        if not m or not 1 <= int(m.group(1)) <= 6:
            fail(where, f"move {move!r}: his tny_kits slot first, 1-6 (then its name)")
        if col not in colours:
            fail(where, f"colour {col!r}: one of the table above ({', '.join(colours)})")
        slot = int(m.group(1)) - 1
        if any(k[0] == name and k[1] == slot for k in out):
            fail(where, f"{name} move {slot + 1} twice")
        out.append((name, slot, colours.index(col) + 1))
    return out


def parse(doc_path, ids, colours):
    lines = open(doc_path, encoding="utf-8").read().split("\n")
    combos, cur = {}, None
    for no, line in enumerate(lines, 1):
        where = f"{doc_path}:{no}"
        m = re.match(r"###\s+(.*)\(`(\w+)`\)\s*$", line)
        if m:
            cid = m.group(2)
            if cid not in ids:
                fail(where, f"`{cid}` is not a combo of tny_combo.h ({', '.join(ids)})")
            if cid in combos:
                fail(where, f"`{cid}` twice")
            cur = combos[cid] = {"title": m.group(1).strip(), "name": None, "steps": [], "at": 0}
            continue
        if line.startswith("## "):
            cur = None
            continue
        if cur is None:
            continue
        m = re.match(r"\s*Name:\s*(.+)$", line)
        if m:
            name = m.group(1).strip().strip("*").strip().strip('"')
            if not all(32 <= ord(c) < 127 for c in name) or "%" in name or len(name) > 28:
                fail(where, f"name {name!r}: plain ASCII, at most 28 characters")
            cur["name"] = name
            continue
        if not line.startswith("|") or line.startswith("|--") or line.startswith("|:") or re.match(r"\|\s*-", line):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if cells[0].lower() == "at":
            continue
        if len(cells) < 3:
            fail(where, "a row is | at | who | does | how |")
        at_s, who_s, does = cells[0], cells[1], cells[2]
        how = cells[3] if len(cells) > 3 else ""
        if at_s.startswith("+"):
            at = cur["at"] + num(at_s[1:], where, 0, 600)
        else:
            at = num(at_s, where, 0, 1800)
        cur["at"] = at
        args = how.replace("`", "").split()
        op, a, b, c, d, col = step(does.replace("`", "").strip(), args, where, colours)
        if op == "BALL" and b == 3 and col:
            fail(where, "the Big Bang has its own colours")
        cur["steps"].append((at, who_bits(who_s, op, where), op, a, b, c, d, col))
    for cid in ids:
        if cid not in combos:
            fail(doc_path, f"no table for `{cid}`")
        c = combos[cid]
        if not c["name"]:
            fail(doc_path, f"`{cid}` has no Name: line")
        c["steps"].sort(key=lambda s: s[0])          # stable: rows of one frame keep their order
        ends = [s for s in c["steps"] if s[2] == "END"]
        if len(ends) != 1 or c["steps"][-1][2] != "END":
            fail(doc_path, f"`{cid}`: exactly one `end` row, the last")
        if not any(s[2] == "CAPTION" for s in c["steps"]):
            c["steps"].insert(0, (0, 0, "CAPTION", 0, 0, 0, 0, 0))
    return combos


def totals(steps):
    """the damage weights per fighter (hits), for the generated file's comment and the build log"""
    out = {1: 0, 2: 0}
    for at, who, op, a, b, c, d, col in steps:
        if op in ATTACKS:
            for bit in (1, 2):
                if who & bit:
                    out[bit] += a
    return out[1] / 10, out[2] / 10


def tint_of(ramp):
    t = list(range(256))
    for _, fam in FAMILIES:
        for src, dst in zip(fam, ramp):
            t[src] = dst
    return t


def byte_rows(data, indent="    "):
    return [indent + ", ".join(str(b) for b in data[i:i + 32]) + "," for i in range(0, len(data), 32)]


def beam_sets(rom, tint, tag):
    """the beam's four facings' frame sets (8 descriptors {geom, attr, src} each) with their tiles redrawn: -> C lines"""
    base = BEAM_SETS - 0x08000000
    words = [int.from_bytes(rom[base + 4 * i: base + 4 * i + 4], "little") for i in range(4 * 24)]
    sizes = {}
    for i in range(32):                 # src = word 2 of each descriptor: 0x100 bytes, the end's (k 7) 0x40
        src = words[i * 3 + 2]
        if not 0x08000000 <= src < 0x0A000000:
            sys.exit(f"tnycombo: the beam's frame set {i} has no tiles at {BEAM_SETS + 12 * i:#x} (not the USA ROM?)")
        sizes[src] = max(sizes.get(src, 0), 0x40 if i % 8 == 7 else 0x100)
    tiles, at = bytearray(), {}
    for src in sorted(sizes):
        at[src] = len(tiles)
        o = src - 0x08000000
        tiles += bytes(tint[b] for b in rom[o:o + sizes[src]])
    out = [f"static const u8 beam_tiles_{tag}[{len(tiles)}] __attribute__((aligned(4))) = {{"] + byte_rows(tiles) + ["};"]
    out.append(f"static const TBeamDesc beam_{tag}[32] = {{")
    for i in range(32):
        g, a, src = words[i * 3], words[i * 3 + 1], words[i * 3 + 2]
        out.append(f"    {{ {g:#010x}, {a:#010x}, beam_tiles_{tag} + {at[src]} }},")
    out.append("};")
    return out


def main():
    argv = sys.argv[1:]
    rom = None
    if "--rom" in argv:
        k = argv.index("--rom")
        rom = open(argv[k + 1], "rb").read()
        del argv[k:k + 2]
    doc, header, out = argv[:3]
    ids = combo_ids(header)
    doc_lines = open(doc, encoding="utf-8").read().split("\n")
    colours = parse_colours(doc_lines, doc)
    names = [c[0] for c in colours]
    kits = parse_kits(doc_lines, doc, fighter_ids(header), names)
    combos = parse(doc, ids, names)
    c = ["// Generated by tools/tnycombo.py from docs/tournament-combos.md (edit the doc, not this file).",
         '#include "tny_combo_run.h"', ""]
    defs = []
    for cid in ids:
        k = combos[cid]
        ta, tb = totals(k["steps"])
        length = k["steps"][-1][0]
        c.append(f"// {k['title']}: \"{k['name']}\", {length} frames; damage weights A {ta:g}, B {tb:g} hits")
        c.append(f"static const TComboStep steps_{cid.lower()}[] = {{")
        for at, who, op, a, b, cc, d, col in k["steps"]:
            c.append(f"    {{ {at}, {who}, CO_{op}, {a}, {b}, {cc}, {d}, {col} }},")
        c.append("};")
        name = k["name"].replace("\\", "\\\\").replace('"', '\\"')
        defs.append(f"    [COMBO_{cid}] = {{ \"{name}\", steps_{cid.lower()}, {len(k['steps'])}, {length} }},")
        print(f"tnycombo: {cid}: {len(k['steps'])} steps, {length} frames, weights A {ta:g} B {tb:g}")
    c.append("")
    c.append("const TComboDef tny_combo_defs[COMBO_COUNT] = {")
    c += defs
    c.append("};")
    # the colours: a remap each, and (with the ROM) the beam's frame sets in each colour a beam row uses
    beam_cols = sorted({st[7] for k in combos.values() for st in k["steps"] if st[2] == "BEAM" and st[7]}
                       | {col for _, _, col in kits})     # (a kit's move may be a beam: its colour's sets too)
    tints, beams = [], []
    for i, (name, ramp) in enumerate(colours, 1):
        t = tint_of(ramp)
        c.append("")
        c.append(f"// {name}: {', '.join(str(r) for r in ramp)}")
        c.append(f"static const u8 tint_{name}[256] = {{")
        c += byte_rows(t)
        c.append("};")
        tints.append(f"    [{i}] = tint_{name},")
        if rom is not None and i in beam_cols:
            c += beam_sets(rom, t, name)
            beams.append(f"    [{i}] = beam_{name},")
    c.append("")
    c.append("const u8 *const tny_combo_tints[TCOL_MAX] = {")
    c += tints
    c.append("};")
    c.append("const TBeamDesc *const tny_combo_beams[TCOL_MAX] = {")
    c += beams
    c.append("};")
    if "white" not in names:
        fail(doc, "the colour table needs `white` (the spark where a body vanishes)")
    c.append("const u8 *const tny_combo_spark = tint_white;")
    c.append("")
    c.append("// the kits' own moves outside the combos (the doc's kits' table): fighter, slot, colour")
    c.append("const TKitColour tny_kit_colours[] = {")
    for name, slot, col in kits:
        c.append(f"    {{ TF_{name}, {slot}, {col} }},   // {names[col - 1]}")
    c.append("    { 0xFF, 0, 0 },")
    c.append("};")
    print(f"tnycombo: {len(colours)} colours ({', '.join(names)}); beams in "
          + (", ".join(names[i - 1] for i in beam_cols) or "none") + ("" if rom is not None else " (no ROM: stock beams)")
          + f"; {len(kits)} kit moves coloured")
    open(out, "w", newline="\n").write("\n".join(c) + "\n")


if __name__ == "__main__":
    main()
