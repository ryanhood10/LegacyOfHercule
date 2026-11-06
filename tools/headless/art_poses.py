r"""Every frame of a sprite record seen in the running game (docs/testing.md "Art checks: every frame in game").

For each sprite id: a warp to the open yard of zone 15 area 3 (as tny_artnew.txt), an actor of that sprite beside
player 1 (test command 3), then the runner's `pose` holds it in every group x facing x frame the built record has (read from the ROM as
tools/artsheet.py reads it) and a shot is taken of each. The shots are cropped round the actor and tiled, one row per
group, as OUT/art_<id>.png (and _x3). The game's own loader decompresses and uploads every frame, so a bad blob, a
frame larger than the record's slot or a missing descriptor shows here (garbage, a CRASH line, a broken entity list).

Pass: no CRASH line, every pose found its body ("1 bodies"), the entity list the same before and after.
Needs a runner built after 2026-11-06 (the `pose` command; ST_HEADLESS=PATH for a private build).

usage: python tools/headless/art_poses.py BUILT.gba coop.sym MENU.ss --ids 165,160 [--out DIR] [--groups 0,5,12]
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, ".."))
import artsheet as AS  # noqa: E402
import sprites as S  # noqa: E402

# the actor is moved 48 px left of player 1, who stands at the screen's middle (120, 80): the crop round a 32 px body
# and round a 64 px one (the HUD ends at y = 24)
CROPS = ((44, 40, 100, 92), (28, 24, 116, 100))


def wsl(p):
    p = os.path.abspath(p).replace("\\", "/")
    return "/mnt/" + p[0].lower() + p[2:]


def ppm(path):
    d = open(path, "rb").read()
    m = re.match(rb"P6\s+(\d+)\s+(\d+)\s+(\d+)\s", d)
    w, h = int(m.group(1)), int(m.group(2))
    return w, h, d[m.end():]


def main():
    a = sys.argv
    rom_path, sym_path, state = a[1], a[2], a[3]
    ids = [int(v) for v in a[a.index("--ids") + 1].split(",")]
    out = a[a.index("--out") + 1] if "--out" in a else os.path.join(os.path.dirname(rom_path), "hl")
    only = [int(v) for v in a[a.index("--groups") + 1].split(",")] if "--groups" in a else None
    os.makedirs(out, exist_ok=True)
    rom = open(rom_path, "rb").read()
    sym = {}
    for line in open(sym_path):
        p = line.split()
        if len(p) == 3:
            sym[p[2]] = int(p[0], 16)
    b = AS.Built(rom, sym)
    fail = 0
    for sid in ids:
        rec = b.record(sid)
        if not rec:
            print(f"{sid}: not in the build")
            fail += 1
            continue
        groups = b.groups(rec)
        starts = {p for g in groups for p in g if p} | b.sp.array_starts()
        plan = []
        for gi, g in enumerate(groups):
            if not all(g) or (only and gi not in only):
                continue
            for d in range(4):
                for fi in range(b.frames(starts, g[d])):
                    plan.append((gi, d, fi))
        lines = ["load " + wsl(state), "p2 off", "maptest 147", "frames 300", "poke32 0x0203FD0C 0x030F",
                 "poke32 0x0203FD54 0x01C00114", "poke32 0x0203FD08 5", "frames 300", f"poke32 0x0203FD0C {sid}",
                 "poke32 0x0203FD08 3", "frames 30", f"entmove {sid} -48 0", "frames 4", "echo == before", "ents"]
        for k, (gi, d, fi) in enumerate(plan):
            lines += [f"pose {sid} {gi} {d} {fi}", "shot " + wsl(os.path.join(out, f"art_{sid}_{k}.ppm"))]
        lines += ["echo == after", "ents", "frames 120", "ents"]
        run = os.path.join(out, f"art_{sid}.run")
        open(run, "w", newline="\n").write("\n".join(lines) + "\n")
        cmd = ["wsl", "-d", "Ubuntu", "-u", "root", "--", os.environ.get("ST_HEADLESS", "/root/st_headless"),
               wsl(rom_path), wsl(run)]
        res = subprocess.run(cmd, capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))
        text = res.stdout
        open(os.path.join(out, f"art_{sid}.out"), "w", encoding="utf-8").write(text + res.stderr[-2000:])
        poses = re.findall(r"^pose \d+ g\d+ d\d+ f\d+: (\d+) bodies", text, re.M)
        ents = re.findall(r"^entities (\d+)", text, re.M)
        vts = [re.findall(r"vt ([0-9a-f]{8})", blk) for blk in re.split(r"^entities \d+", text, flags=re.M)[1:]]
        bad = []
        if "CRASH" in text or "Jumped to invalid" in text + res.stderr:
            bad.append("CRASH")
        if len(poses) != len(plan) or any(p != "1" for p in poses):
            bad.append(f"poses {len(poses)}/{len(plan)}, bodies {sorted(set(poses))}")
        if len(set(ents)) != 1 or len({tuple(v) for v in vts}) != 1:
            bad.append(f"entity list changed: {ents}")
        if any(not v.startswith("08") for blk in vts for v in blk):
            bad.append("a vtable outside the ROM")
        # the sheet
        from PIL import Image, ImageDraw
        CROP = CROPS[int.from_bytes(rom[rec - S.ROM_BASE:rec - S.ROM_BASE + 2], 'little') > 0x400]
        cw, ch = CROP[2] - CROP[0], CROP[3] - CROP[1]
        rows = sorted({gi for gi, _, _ in plan})
        per = max(sum(1 for g2, _, _ in plan if g2 == gi) for gi in rows)
        im = Image.new("RGB", (24 + per * cw, len(rows) * ch), (20, 20, 20))
        dr = ImageDraw.Draw(im)
        col = {gi: 0 for gi in rows}
        for k, (gi, d, fi) in enumerate(plan):
            f = os.path.join(out, f"art_{sid}_{k}.ppm")
            if not os.path.exists(f):
                continue
            w, h, data = ppm(f)
            shot = Image.frombytes("RGB", (w, h), data).crop(CROP)
            im.paste(shot, (24 + col[gi] * cw, rows.index(gi) * ch))
            dr.text((24 + col[gi] * cw + 1, rows.index(gi) * ch - 1), "dulr"[d] + str(fi), fill=(255, 255, 0))
            col[gi] += 1
            os.remove(f)
        for r, gi in enumerate(rows):
            dr.text((2, r * ch + 2), str(gi), fill=(255, 255, 0))
        sheet = os.path.join(out, f"art_{sid}.png")
        im.save(sheet)
        im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(sheet[:-4] + "_x3.png")
        print(f"{sid}: {len(plan)} frames in {len(rows)} groups, entities {ents[0] if ents else '?'}: "
              f"{'FAIL ' + '; '.join(bad) if bad else 'ok'} -> {sheet}")
        fail += bool(bad)
    sys.exit(1 if fail else 0)


if __name__ == "__main__":
    main()
