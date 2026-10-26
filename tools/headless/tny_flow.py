"""Film a whole tournament as the players see it (docs/testing.md "Tournament flow films"): a shot every few frames from
the waiting room to the way back, labelled with the engine's phase, then contact sheets and a list of the frames that
jump (a big change from both neighbours: a pop, a frame of the wrong map or body).

The run: Map Test 13, stand-in bodies, the record wiped, seed 42, the players warped to the waiting room's director
spot, then test command 25 (the director's choices preset; the screens are tny_ui's, filmed by tny_ui.txt) or, with
--director, the real talk (A through the menus: the first choice everywhere). While filming, A is pressed on both
pads every --a frames (a player closing the lines) and test command 22 (every opponent takes a lethal blow) is sent
every --win frames (a no-op outside a fight), so fights last up to that long.

usage: python tools/headless/tny_flow.py ROM MENU_SS OUTDIR [--div 0|1|2] [--tier 0-4] [--full] [--p2] [--pick1 N]
       [--pick2 N] [--seed 42] [--len 6000] [--every 4] [--a 60] [--win 240] [--name flow] [--director] [--lose]
       [--unlock 9,8]
Out: OUTDIR/<name>_NN.png (sheets of 48 shots, each labelled "index frame phase pres diff"), OUTDIR/<name>.txt (the timeline:
every shot's phase and its difference to the shot before; JUMP marks a one-shot spike, CUT a big change).
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ENG = 0x0203EC00


def arg(name, default):
    a = sys.argv
    return a[a.index(name) + 1] if name in a else default


def main():
    rom, state, out = sys.argv[1:4]
    div, tier = int(arg("--div", "0"), 0), int(arg("--tier", "0"), 0)
    full = 1 if "--full" in sys.argv else 0
    p2 = "--p2" in sys.argv
    pick1, pick2 = int(arg("--pick1", "0xFF"), 0), int(arg("--pick2", "0xFF"), 0)
    seed = int(arg("--seed", "42"), 0)
    total, every = int(arg("--len", "6000")), int(arg("--every", "4"))
    a_every, win_every = int(arg("--a", "60")), int(arg("--win", "240"))
    name = arg("--name", "flow")
    lose = "--lose" in sys.argv
    os.makedirs(out, exist_ok=True)
    L = ["p2 off", "maptest 13", "frames 300"]
    if p2:
        L += ["p2 0", "frames 200"]
    L += ["poke32 0x0203FD0C 0", "poke32 0x0203FD54 2", "poke32 0x0203FD08 24", "frames 10",
          "poke32 0x0203FD0C 0", "poke32 0x0203FD08 23", "frames 10",
          f"poke32 0x0203FD0C {seed}", "poke32 0x0203FD54 0", "poke32 0x0203FD08 21", "frames 10",
          # the waiting room, in front of the director
          "poke32 0x0203FD0C 0x0F0F", "poke32 0x0203FD54 0x013200A8", "poke32 0x0203FD08 5", "frames 300"]
    for fid in [int(v, 0) for v in arg("--unlock", "").split(",") if v]:   # record bits (test command 23 op 1)
        L += ["poke32 0x0203FD0C 1", f"poke32 0x0203FD54 {fid}", "poke32 0x0203FD08 23", "frames 10"]
    if "--director" in sys.argv:
        L += ["key 0x80 6", "key 1 10"]
    else:
        L += [f"poke32 0x0203FD0C {div | full << 4 | tier << 8:#x}", f"poke32 0x0203FD54 {pick1 | pick2 << 8:#x}",
              "poke32 0x0203FD08 25"]
    shots, frame, last_a, last_win = [], 0, 0, 0
    while frame < total:
        step, used = every, 0
        if frame - last_a >= a_every:
            last_a = frame
            if p2:
                L += ["p2 1"]
            L += ["key 1 2"]                       # 2 held + 4 released
            if p2:
                L += ["p2 0"]
            step, used = max(0, every - 6), 6
        if frame - last_win >= win_every:
            last_win = frame
            L += ["poke32 0x0203FD0C 1" if lose else "poke32 0x0203FD0C 0", "poke32 0x0203FD08 22"]
        if step:
            L += [f"frames {step}"]
        frame += step + used
        n = f"{name}_{len(shots):04d}"
        L += [f"echo SHOT {len(shots)} {frame}", f"peek8 {ENG + 4:#x} 4", f"peek8 {ENG + 0x150:#x} 4",
              f"peek8 {ENG + 0xFC:#x} 2", f"shot {n}"]
        shots.append(n)
    sp = os.path.join(out, name + ".script")
    open(sp, "w").write("\n".join(L) + "\n")
    # st_test.py would tile every shot into one sheet: run the runner directly
    def wsl(p):
        p = os.path.abspath(p).replace("\\", "/")
        return "/mnt/" + p[0].lower() + p[2:]
    lines = []
    for ln in L:
        m = re.match(r"^shot (\S+)$", ln)
        lines.append(f"shot {wsl(os.path.join(out, m.group(1)))}.ppm" if m else ln)
    run = os.path.join(out, name + ".run")
    open(run, "w", newline="\n").write("\n".join(lines) + "\n")
    res = subprocess.run(["wsl", "-d", "Ubuntu", "-u", "root", "--", os.environ.get("ST_HEADLESS", "/root/st_headless"),
                          wsl(rom), wsl(run), wsl(state)], capture_output=True, text=True,
                         env=dict(os.environ, MSYS_NO_PATHCONV="1"))
    log = res.stdout + res.stderr
    open(os.path.join(out, name + ".out"), "w").write(log)
    info, cur, got = {}, None, []
    for line in log.splitlines():
        if line.startswith("SHOT "):
            cur = int(line.split()[1]); got = []
            info[cur] = {"frame": int(line.split()[2])}
        elif cur is not None and re.match(r"^[0-9a-f]{8}: ", line):
            got.append([int(v, 16) for v in line.split()[1:]])
            if len(got) == 3:
                ph, pend, kind, res_ = got[0][:4]
                info[cur].update(phase=ph, kind=kind, pres=got[1][0], nofight=got[2][0], busy=got[2][1])
    from PIL import Image, ImageDraw
    import numpy as np
    prev, diffs = None, []
    for i, s in enumerate(shots):
        try:
            a = np.asarray(Image.open(os.path.join(out, s + ".ppm")).convert("RGB"), dtype=np.int16)
        except FileNotFoundError:
            diffs.append(-1); prev = None; continue
        diffs.append(float(np.abs(a - prev).mean()) if prev is not None else 0.0)
        prev = a
    tl = []
    for i, s in enumerate(shots):
        d = info.get(i, {})
        mark = ""
        if 0 < i < len(shots) - 1 and diffs[i] > 12 and diffs[i + 1] > 12:
            mark = "JUMP"
        elif diffs[i] > 25:
            mark = "CUT"
        tl.append(f"{i:4d} f{d.get('frame', 0):5d} ph{d.get('phase', -1):2d} pres{d.get('pres', -1)} "
                  f"nf{d.get('nofight', -1)} busy{d.get('busy', -1)} diff{diffs[i]:6.1f} {mark}")
    open(os.path.join(out, name + ".txt"), "w").write("\n".join(tl) + "\n")
    per = 48
    for k in range(0, len(shots), per):
        sheet = Image.new("RGB", (8 * 242, 6 * 172), (40, 0, 40))
        dr = ImageDraw.Draw(sheet)
        for j, s in enumerate(shots[k:k + per]):
            i = k + j
            try:
                im = Image.open(os.path.join(out, s + ".ppm")).convert("RGB")
            except FileNotFoundError:
                continue
            x, y = (j % 8) * 242, (j // 8) * 172
            sheet.paste(im, (x, y + 12))
            d = info.get(i, {})
            dr.text((x + 2, y), f"{i} f{d.get('frame', 0)} ph{d.get('phase', -1)} p{d.get('pres', -1)} d{diffs[i]:.0f}",
                    fill=(255, 255, 0))
        sheet.save(os.path.join(out, f"{name}_{k // per:02d}.png"))
    print(f"{name}: {len(shots)} shots, {(len(shots) + per - 1) // per} sheets" + ("; CRASH" if "CRASH" in log else ""))
    for t in tl:
        if t.endswith("JUMP") or t.endswith("CUT"):
            print(t)


if __name__ == "__main__":
    main()
