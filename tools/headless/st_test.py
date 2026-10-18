r"""Run a headless chapter test (tools/headless/st_headless.c in WSL) and tile its screenshots.
usage: python tools/headless/st_test.py SCRIPT ROM [STATE] [--sheet OUT.png] [--cols 3] [--out DIR]
SCRIPT, ROM and STATE are Windows paths under F:\MeltyCreations. Bare names in "shot NAME", "save NAME"
and "load NAME" lines land in DIR (default build/hl): NAME.ppm for shots. Prints the runner's output;
the sheet defaults to DIR/<script name>.png. Build the runner first (st_headless.c header).
tools/headless/mkmenu.txt makes the debug-menu savestate (DIR/menu.ss) for a DEBUG_MENU=1 ROM."""
import os
import re
import subprocess
import sys

args = sys.argv[1:]
sheet, cols, outdir = None, "3", "build/hl"
if "--sheet" in args:
    i = args.index("--sheet"); sheet = args[i + 1]; del args[i:i + 2]
if "--cols" in args:
    i = args.index("--cols"); cols = args[i + 1]; del args[i:i + 2]
if "--out" in args:
    i = args.index("--out"); outdir = args[i + 1]; del args[i:i + 2]
script, rom = args[0], args[1]
state = args[2] if len(args) > 2 else None


def wsl(p):
    p = os.path.abspath(p).replace("\\", "/")
    return "/mnt/" + p[0].lower() + p[2:]


here = os.path.abspath(outdir)
os.makedirs(here, exist_ok=True)
lines = []
for ln in open(script):
    m = re.match(r"^(\s*(shot|save|load)\s+)(\S+)(.*)$", ln)
    if m and "/" not in m.group(3):
        ext = ".ppm" if m.group(2) == "shot" else ""
        ln = f"{m.group(1)}{wsl(os.path.join(here, m.group(3)))}{ext}{m.group(4)}\n"
    lines.append(ln)
tmp = os.path.join(here, os.path.basename(script) + ".run")
open(tmp, "w", newline="\n").write("".join(lines))
cmd = ["wsl", "-d", "Ubuntu", "-u", "root", "--", "/root/st_headless", wsl(rom), wsl(tmp)] + ([wsl(state)] if state else [])
out = subprocess.run(cmd, capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))
sys.stdout.write(out.stdout)
sys.stderr.write(out.stderr[-2000:])
shots = [re.match(r"shot (\S+)", l).group(1) for l in out.stdout.splitlines() if l.startswith("shot ")]
if shots:
    files = ["F:" + s[len("/mnt/f"):] for s in shots]
    subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "st_shots.py"),
                    sheet or os.path.join(here, os.path.splitext(os.path.basename(script))[0] + ".png"), *files,
                    "--cols", cols])
