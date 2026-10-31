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

usage: python tools/tnycombo.py docs/tournament-combos.md src/coop/tny_combo.h OUT.c
"""
import re
import sys

OPS = ["END", "GO", "TELE", "POSE", "FACE", "PUNCH", "BLAST", "BALL", "BEAM", "DISC", "SHAKE", "SOUND", "FLASH",
       "SCREEN", "KNOCK", "CAPTION", "SBC", "COPY"]
ATTACKS = ("PUNCH", "BLAST", "BALL", "BEAM", "DISC", "SBC")
POSES = ["idle", "walk", "hurt", "fall", "shout", "charge", "punch", "kick", "dash", "special", "beam", "beam2", "ki",
         "lie"]
CP_RAW, CP_KEEP = 0x40, 0xFF
SCREEN_OPS = {"SHAKE", "SOUND", "SCREEN", "CAPTION", "END"}


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


def step(verb, args, where):
    """-> (op, a, b, c, d)"""
    v = verb.lower()
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
        if not 1 <= n <= 2 or (n == 2 and args[1].lower() not in ("blue", "orb")):
            fail(where, "blast W [blue]")
        return "BLAST", weight(args[0], where), 1 if n == 2 and args[1].lower() == "blue" else 0, 0, 0
    if v == "ball":
        usage = "ball W [large | medium] [hold N] [front]"
        if n < 1:
            fail(where, usage)
        size, hold, front, rest = 1, 0, 0, [a.lower() for a in args[1:]]
        while rest:
            t = rest.pop(0)
            if t in ("large", "medium"):
                size = 2 if t == "large" else 1
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
            fail(where, "beam W N [through]")
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


def parse(doc_path, ids):
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
        op, a, b, c, d = step(does.replace("`", "").strip(), args, where)
        cur["steps"].append((at, who_bits(who_s, op, where), op, a, b, c, d))
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
            c["steps"].insert(0, (0, 0, "CAPTION", 0, 0, 0, 0))
    return combos


def totals(steps):
    """the damage weights per fighter (hits), for the generated file's comment and the build log"""
    out = {1: 0, 2: 0}
    for at, who, op, a, b, c, d in steps:
        if op in ATTACKS:
            for bit in (1, 2):
                if who & bit:
                    out[bit] += a
    return out[1] / 10, out[2] / 10


def main():
    doc, header, out = sys.argv[1:4]
    ids = combo_ids(header)
    combos = parse(doc, ids)
    c = ["// Generated by tools/tnycombo.py from docs/tournament-combos.md (edit the doc, not this file).",
         '#include "tny_combo_run.h"', ""]
    defs = []
    for cid in ids:
        k = combos[cid]
        ta, tb = totals(k["steps"])
        length = k["steps"][-1][0]
        c.append(f"// {k['title']}: \"{k['name']}\", {length} frames; damage weights A {ta:g}, B {tb:g} hits")
        c.append(f"static const TComboStep steps_{cid.lower()}[] = {{")
        for at, who, op, a, b, cc, d in k["steps"]:
            c.append(f"    {{ {at}, {who}, CO_{op}, {a}, {b}, {cc}, {d} }},")
        c.append("};")
        name = k["name"].replace("\\", "\\\\").replace('"', '\\"')
        defs.append(f"    [COMBO_{cid}] = {{ \"{name}\", steps_{cid.lower()}, {len(k['steps'])}, {length} }},")
        print(f"tnycombo: {cid}: {len(k['steps'])} steps, {length} frames, weights A {ta:g} B {tb:g}")
    c.append("")
    c.append("const TComboDef tny_combo_defs[COMBO_COUNT] = {")
    c += defs
    c.append("};")
    open(out, "w", newline="\n").write("\n".join(c) + "\n")


if __name__ == "__main__":
    main()
