"""Film tournament bodies in the ring and tile the crops into one sheet per run (docs/testing.md "Playable bodies:
floating heads and the dash"). Every shot is cropped around the entity's own screen position (x / y at entity +0x158
/ +0x15C, 8.8, minus the view origin at camera +0x1C / +0x20), so a lagging camera never cuts a body.

player mode (default): each display row is poked into party slot 1 (PARTY+0x30), slot 1 made player 1 (PARTY = 1),
warped into the ring (zone 15 area 16 at (248, 384), test command 5), then: idle, walk left / right, a double-tap
dash right and left (6 shots each), up, down, the A combo. Rows run six to a headless run (the runner's sheet of a
run stays under Windows' command-line limit). Out: OUTDIR/<name>_sheet.png (one line per row, x3), <name>_N.out.
The ROM-level sheets of every body (lying, victory, any group) are tools/tnysheet.py's.

--opp 195,196: films each sprite as an opponent instead (AI set 21, 6 s, a shot every 6 frames labelled with his
pose group and frame), one line per fighter.

usage: python tools/headless/tny_film.py ROM MENU_SS OUTDIR [--rows 29-66 | --rows 38,39 | --opp 195] [--name film]
       [--big]
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CROP = 48          # px around the body (80 for giants: --big, and every --opp film)


def script(rows, big):
    L = ["maptest 13", "frames 300", "deref cam 0x081DA974"]
    shots = []

    def shot(name):
        L.extend(["deref pl 0x03001FA8", f"echo POS {name}", "peek pl+0x158 2", "peek cam+0x1C 2", f"shot {name}"])
        shots.append(name)

    for row in rows:
        n = f"r{row}"
        L += [f"poke8 PARTY+0x30 {row}", "poke8 PARTY+0 1", "poke32 0x0203FD0C 0x100F", "poke32 0x0203FD54 0x018000F8",
              "poke32 0x0203FD08 5", "frames 200"]
        shot(f"{n}_idle")
        L += ["keys 0x20", "frames 8"]
        shot(f"{n}_wl1")
        L += ["frames 6"]
        shot(f"{n}_wl2")
        L += ["keys 0x10", "frames 8"]
        shot(f"{n}_wr1")
        L += ["frames 6"]
        shot(f"{n}_wr2")
        L += ["keys 0", "frames 10"]
        for k, d in ((0x10, "r"), (0x20, "l")):
            L += [f"keys {k:#x}", "frames 4", "keys 0", "frames 6", f"keys {k:#x}"]
            for i in range(6):
                L += ["frames 3"]
                shot(f"{n}_d{d}{i}")
            L += ["keys 0", "frames 20"]
        L += ["keys 0x40", "frames 6", "keys 0", "frames 4"]
        shot(f"{n}_up")
        L += ["keys 0x80", "frames 6", "keys 0", "frames 4"]
        shot(f"{n}_down")
        L += ["key 1 4", "frames 4"]
        shot(f"{n}_p1")
        L += ["frames 8"]
        shot(f"{n}_p2")
    return "\n".join(L) + "\n", shots


def opp_script(sprites, frames=360, every=6):
    """Each sprite spawned as an opponent on AI set 21 (stat 29, test command 8 from a RAM record), alone, 40 px right
    of player 1 on New Planet Vegeta; player 1 punches every 40 frames; a shot of the opponent every `every` frames
    with his pose (record +0x48, group pointer +0x4C, frame +0x14)."""
    L = ["p2 off", "maptest 147", "frames 300", "deref cam 0x081DA974"]
    shots = []
    for sid in sprites:
        L += ["poke32 0x0203FD0C 0x030F", "poke32 0x0203FD54 0x01620114", "poke32 0x0203FD08 5", "frames 300",
              "deref pl 0x03001FA8", "poke16 PARTY+0x28 9999", "poke16 PARTY+0x2A 9999",
              "poke32 0x0203FD6C 0x0800E77F", "poke32 0x0203FD70 1", "poke32 0x0203FD74 29", "poke32 0x0203FD78 0",
              f"poke32 0x0203FD7C {sid}", "poke32 0x0203FD80 0x086D66BC", "poke32 0x0203FD84 0",
              "poke32 0x0203FD0C 0x0203FD6C", "poke32 0x0203FD08 8", "frames 4", "findent E 0x080252a4", "key 0x10 2"]
        for k in range(frames // every):
            if k % (40 // every) == 0:
                L += ["key 1 2", f"frames {every - 2}"]
            else:
                L += [f"frames {every}"]
            name = f"o{sid}_{k:03d}"
            L += [f"echo POS {name}", "peek E+0x158 2", "peek cam+0x1C 2", f"echo POSE {name}", "peek E+0x48 2",
                  "peek16 E+0x14 1", f"shot {name}"]
            shots.append(name)
        L += ["poke32 0x0203FD08 13", "frames 30"]
    return "\n".join(L) + "\n", shots


def main():
    rom, state, out = sys.argv[1:4]
    a = sys.argv
    spec = a[a.index("--rows") + 1] if "--rows" in a else "29-66"
    rows = []
    for part in spec.split(","):
        if "-" in part:
            lo, hi = part.split("-")
            rows += range(int(lo), int(hi) + 1)
        else:
            rows.append(int(part))
    name = a[a.index("--name") + 1] if "--name" in a else "film"
    big = "--big" in a
    os.makedirs(out, exist_ok=True)
    shots, log = [], ""
    opp = [int(v) for v in a[a.index("--opp") + 1].split(",")] if "--opp" in a else None
    units = opp if opp else rows
    for k in range(0, len(units), 6 if not opp else 1):
        txt, sh = opp_script(units[k:k + 1]) if opp else script(rows[k:k + 6], big)
        sp = os.path.join(out, f"{name}_{k // 6}.txt")
        open(sp, "w").write(txt)
        res = subprocess.run([sys.executable, os.path.join(HERE, "st_test.py"), sp, rom, state, "--out", out,
                              "--cols", "11"], capture_output=True, text=True)
        open(os.path.join(out, f"{name}_{k // 6}.out"), "w").write(res.stdout + res.stderr)
        shots += sh
        log += res.stdout
    pos, pose, cur, kind = {}, {}, None, None
    for line in log.splitlines():
        if line.startswith("POS ") or line.startswith("POSE "):
            kind, cur = line.split()[0], line.split()[1]
        elif cur and re.match(r"^[0-9a-f]{8}: ", line):
            w = [int(v, 16) for v in line.split()[1:3]]
            if kind == "POS":
                pos.setdefault(cur, []).append(w)
                if len(pos[cur]) == 2:
                    cur = None
            else:
                pose.setdefault(cur, []).append(w)
                if len(pose[cur]) == 2:
                    rec, gp = pose[cur][0]
                    pose[cur] = ((gp - rec - 0x4C) // 16, pose[cur][1][0] & 0xFFFF)
                    cur = None
    if opp:
        rows = opp
        big = True
    from PIL import Image
    c = 80 if big else CROP
    per_row = len(shots) // max(1, len(rows))
    sheet = Image.new("RGB", (per_row * (c + 2) + 30, len(rows) * (c + 2)), (255, 0, 255))
    for i, s in enumerate(shots):
        r, col = divmod(i, per_row)
        try:
            im = Image.open(os.path.join(out, s + ".ppm")).convert("RGB")
        except FileNotFoundError:
            continue
        (ex, ey), (vx, vy) = pos.get(s, [(120 << 8, 90 << 8), (0, 0)])
        sx, sy = (ex >> 8) - vx, (ey >> 8) - vy
        mid = sx + (16 if big else 8)               # bodies centre near x + 8, giants near x + 16
        crop = im.crop((mid - c // 2, sy - c + 12, mid + c // 2, sy + 12))
        sheet.paste(crop, (30 + col * (c + 2), r * (c + 2)))
    from PIL import ImageDraw
    dr = ImageDraw.Draw(sheet)
    for r, row in enumerate(rows):
        dr.text((2, r * (c + 2) + 2), str(row), fill=(255, 255, 0))
    for i, sname in enumerate(shots):
        if sname in pose and isinstance(pose[sname], tuple):
            r, col = divmod(i, per_row)
            dr.text((30 + col * (c + 2) + 1, r * (c + 2) + 1), "g%d f%d" % pose[sname], fill=(255, 0, 0))
    path = os.path.join(out, name + "_sheet.png")
    sheet.resize((sheet.width * 3, sheet.height * 3), Image.NEAREST).save(path)
    print(f"{path}: {len(rows)} rows x {per_row} shots; CRASH" if "CRASH" in log else f"{path}: {len(rows)} rows x {per_row} shots")


if __name__ == "__main__":
    main()
