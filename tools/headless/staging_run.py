r"""Run a staging_*.txt script through st_expect.py and check its `ents` lists too (docs/testing.md "Scene staging").
usage: python tools/headless/staging_run.py SCRIPT ROM STATE [st_test.py options: --out DIR --cols N]
A FAIL line for: a failed expect, a CRASH line, an entity whose vtable is not a ROM or EWRAM address, a player 1 far off any map.
The heap corruption of an actor on a pose group its sprite lacks shows as garbage vtables a little later (coordinator's note,
2026-11-05), and the runner keeps going after it. Exit status 1 on any failure."""
import os
import re
import subprocess
import sys

run = subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "st_expect.py"), *sys.argv[1:]],
                     capture_output=True, text=True)
sys.stderr.write(run.stderr)
bad = 0
for ln in run.stdout.splitlines():
    print(ln)
    m = re.match(r"\s+ent\s+\d+ ([0-9a-f]{8}) vt ([0-9a-f]{8}) ", ln)
    if m and not (m.group(1)[:3] in ("020", "030") and m.group(2)[:2] in ("08", "02", "03")):
        bad += 1
        print(f"FAIL an entity looks wrong | {ln}")
    m = re.match(r"map zone \d+ area \d+ var \d+ player (-?\d+),(-?\d+)", ln)
    if m and not (-64 <= int(m.group(1)) < 8192 and -64 <= int(m.group(2)) < 8192):
        bad += 1
        print(f"FAIL player 1 off the map | {ln}")
print(f"== staging: {bad} sanity failures")
sys.exit(1 if bad or run.returncode else 0)
