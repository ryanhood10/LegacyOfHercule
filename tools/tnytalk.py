"""The tournament's fighter dialogue, from the plan Ryan approved (docs/tournament-dialogue.md) to C data.

The doc is the source: its tables are read as they stand, so an edit there is an edit in the game. Read:
  section 4  every fighter's own lines: | fighter | opener 1 | opener 2 | reply | win |
  section 5  rival exchanges and section 6 meeting yourself: | pair | X: line / Y: line / ... |
  section 7  2 v 2 team lines (| team | ... |) and team vs team (| A & B vs C & D | ... |)
A line's speaker tag (V:, Gohan:, 17:, S13:) is matched to one of the row's fighters: the full name, any word of it,
or a word's initial; a tag several fighters match goes to the one no other tag of the row claims, else the first.

Out (C): tny_talk_voice[TF_COUNT] (four lines, 0 = none), tny_talk_pairs[] (meeting yourself first, then the rival
exchanges), tny_talk_teams[], tny_talk_tvt[]. Each line is the game's text with the speaker's name in capitals:
"!GOKU: Hey!  You look strong!" (the box at the top, two spaces after a sentence as the stock lines have them).

usage: python tools/tnytalk.py docs/tournament-dialogue.md src/coop/tourney_data.c OUT.c
"""
import re
import sys

ALIAS = {"S13": "Super Android 13", "TAO": "General Tao"}
SHORT = {"SSJ2 Gohan": "GOHAN", "General Tao": "TAO"}     # the speaker's name in the box (default: the name)


def roster(path):
    src = open(path, encoding="utf-8").read()
    names = {}
    for enum, name in re.findall(r'\[(TF_\w+)\]\s*=\s*\{\s*"([^"]+)"', src):
        names.setdefault(name, enum)          # "Hercule" / "Android 16": the opponent's entry (the voice is shared)
    return names


def box_name(name):
    if name in SHORT:
        return SHORT[name]
    return name.replace(" (Buu Saga)", "").upper()


def game_text(s):
    s = s.strip()
    s = re.sub(r"(?<!\bMr)(?<!\bDr)([.!?]) (?=\S)", r"\1  ", s)   # two spaces after a sentence (not after Mr. / Dr.)
    assert "%" not in s and all(32 <= ord(c) < 127 for c in s), s
    return s


def c_str(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def sections(doc):
    out, cur = {}, None
    for line in doc.split("\n"):
        m = re.match(r"## (\d+)\.", line)
        if m:
            cur = int(m.group(1))
            out[cur] = []
        elif cur is not None and line.startswith("| ") and not line.startswith("|---"):
            out[cur].append([c.strip() for c in line.strip().strip("|").split("|")])
    return out


def resolve(tags, members, names):
    """members: fighter names in row order; returns the member index of each tag."""
    def matches(tag, name):
        t = ALIAS.get(tag.upper(), tag)
        if t.lower() == name.lower():
            return True
        words = re.findall(r"[A-Za-z0-9.\-]+", name.replace("(", " ").replace(")", " "))
        if any(t.lower() == w.lower().rstrip(".") for w in words):
            return True
        return len(t) == 1 and any(w[0].lower() == t.lower() for w in words)
    cands = [[i for i, n in enumerate(members) if matches(t, n)] for t in tags]
    for t, c in zip(tags, cands):
        assert c, f"tag {t!r} matches none of {members}"
    unique = {c[0] for c in cands if len(c) == 1}
    out = []
    for c in cands:
        if len(c) > 1:
            rest = [i for i in c if i not in unique]
            c = rest or c
        out.append(c[0])
    return out


def member_names(cell, names):
    """'Goku, Vegeta' / 'Goku & Vegeta vs Frieza & King Cold' / 'Android 17 & 18' -> roster names."""
    out = []
    for side in re.split(r"\s+vs\s+", cell):
        parts = re.split(r"\s*[,&]\s*", side)
        prev = None
        for p in parts:
            if re.fullmatch(r"\d+", p):
                p = f"Android {p}"                          # "Android 17 & 18", "Krillin & 18"
            assert p in names, f"unknown fighter {p!r} in {cell!r}"
            out.append(p)
            prev = p
    return out


def lines_of(cell, members, names):
    parts = [p.strip() for p in cell.split(" / ")]
    tags, texts = [], []
    for p in parts:
        tag, text = p.split(":", 1)
        tags.append(tag.strip())
        texts.append(text.strip())
    who = resolve(tags, members, names)
    return [(w, f"!{box_name(members[w])}: {game_text(t)}") for w, t in zip(who, texts)]


def main():
    doc_path, data_path, out_path = sys.argv[1:4]
    names = roster(data_path)
    sec = sections(open(doc_path, encoding="utf-8").read())

    voice = {}
    for row in sec[4]:
        if row[0] == "fighter" or len(row) < 5 or row[1].startswith("("):
            continue
        assert row[0] in names, row[0]
        voice[names[row[0]]] = [f"!{box_name(row[0])}: {game_text(t)}" for t in row[1:5]]

    pairs = []
    for s in (6, 5):                                         # meeting yourself first
        for row in sec[s]:
            if row[0] == "pair":
                continue
            mem = member_names(row[0], names)
            assert len(mem) == 2, row
            pairs.append(([names[m] for m in mem], lines_of(row[1], mem, names)))
    teams, tvt = [], []
    for row in sec[7]:
        if row[0] in ("team", "teams"):
            continue
        mem = member_names(row[0], names)
        entry = ([names[m] for m in mem], lines_of(row[1], mem, names))
        (tvt if len(mem) == 4 else teams).append(entry)
        assert len(mem) in (2, 4), row

    longest = max(len(t) for _, ls in pairs + teams + tvt for _, t in ls)
    for ids, ls in pairs + teams + tvt:
        assert 1 <= len(ls) <= 4, ids
    L = ["// Generated by tools/tnytalk.py from docs/tournament-dialogue.md (Ryan's approved plan). Do not edit.",
         '#include "tourney.h"', '#include "tny_talk.h"', ""]
    L.append("const TalkVoice tny_talk_voice[TF_COUNT] = {")
    for enum, ls in voice.items():
        L.append(f"    [{enum}] = {{ {{ {', '.join(c_str(t) for t in ls)} }} }},")
    L.append("};")

    def table(ctype, name, rows, k):
        L.append(f"const {ctype} {name}[] = {{")
        for ids, ls in rows:
            spk = ", ".join(str(w) for w, _ in ls)
            txt = ", ".join(c_str(t) for _, t in ls)
            L.append(f"    {{ {{ {', '.join(ids)} }}, {len(ls)}, {{ {spk} }}, {{ {txt} }} }},")
        L.append(f"    {{ {{ {', '.join(['0xFF'] * k)} }}, 0, {{ 0 }}, {{ 0 }} }},")
        L.append("};")
    table("TalkPair", "tny_talk_pairs", pairs, 2)
    table("TalkPair", "tny_talk_teams", teams, 2)
    table("TalkTvt", "tny_talk_tvt", tvt, 4)
    open(out_path, "w", encoding="utf-8", newline="\n").write("\n".join(L) + "\n")
    print(f"{out_path}: {len(voice)} voices, {len(pairs)} pairs, {len(teams)} teams, {len(tvt)} team-vs-team; "
          f"longest line {longest}")


if __name__ == "__main__":
    main()
