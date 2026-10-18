r"""Run a headless chapter test (tools/headless/st_headless.c in WSL) and tile its screenshots.
usage: python tools/headless/st_test.py SCRIPT ROM [STATE] [--sheet OUT.png] [--cols 3]
SCRIPT, ROM and STATE are Windows paths under F:\MeltyCreations; "shot NAME" lines in the script may
give a bare name, which lands next to the script as NAME.ppm. Prints the runner's output; the sheet
defaults to SCRIPT with .png. Build the runner first (st_headless.c header)."""
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
args = sys.argv[1:]
sheet, cols = None, "3"
if "--sheet" in args:
    i = args.index("--sheet"); sheet = args[i + 1]; del args[i:i + 2]
if "--cols" in args:
    i = args.index("--cols"); cols = args[i + 1]; del args[i:i + 2]
script, rom = args[0], args[1]
state = args[2] if len(args) > 2 else None


def wsl(p):
    p = os.path.abspath(p).replace("\\", "/")
    return "/mnt/" + p[0].lower() + p[2:]


here = os.path.dirname(os.path.abspath(script))
lines = []
for ln in open(script):
    m = re.match(r"^(\s*shot\s+)(\S+)(.*)$", ln)
    if m and "/" not in m.group(2):
        ln = f"{m.group(1)}{wsl(os.path.join(here, m.group(2)))}.ppm{m.group(3)}\n"
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
    subprocess.run([sys.executable, os.path.join(os.path.dirname(__file__), "st_shots.py"),
                    sheet or os.path.splitext(script)[0] + ".png", *files, "--cols", cols])
