r"""Run a headless script through st_test.py and check its EXPECT lines (docs/testing.md "Super Saiyan Hercule").
usage: python tools/headless/st_expect.py SCRIPT ROM [STATE] [st_test.py options: --out DIR --cols N]

In the script, `echo EXPECT <regex>` before a peek: the next output line that is not a shot line must match
the regex (re.match). `{symbol}` in the regex is that symbol's address from coop.sym next to the ROM, as 8 hex
digits. Prints the runner's output with "ok <regex>" or "FAIL <regex> | <line>" after each check, and a summary;
regfull.sh greps FAIL. Exit status 1 on any failure."""
import os
import re
import subprocess
import sys

args = sys.argv[1:]
rom = args[1]
syms = {}
sym_path = os.path.join(os.path.dirname(os.path.abspath(rom)), "coop.sym")
if os.path.exists(sym_path):
    for ln in open(sym_path):
        p = ln.split()
        if len(p) >= 3 and re.fullmatch(r"[0-9a-f]{8}", p[0]):
            syms[p[-1]] = p[0]

run = subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "st_test.py"), *args],
                     capture_output=True, text=True)
sys.stderr.write(run.stderr)
lines = run.stdout.splitlines()
fails = checks = 0
pending = None
for ln in lines:
    print(ln)
    if ln.startswith("EXPECT "):
        if pending:
            fails += 1
            print(f"FAIL {pending} | (no line)")
        pending = re.sub(r"\{([A-Za-z_]\w*)\}", lambda m: syms.get(m.group(1), "MISSING_" + m.group(1)), ln[7:].strip())
        continue
    if pending and not ln.startswith("shot "):
        checks += 1
        if re.match(pending, ln):
            print(f"ok {pending}")
        else:
            fails += 1
            print(f"FAIL {pending} | {ln}")
        pending = None
if pending:
    fails += 1
    print(f"FAIL {pending} | (no line)")
if "CRASH" in run.stdout or "CRASH" in run.stderr:
    fails += 1
    print("FAIL a CRASH line")
print(f"== expect: {checks} checks, {fails} failed")
sys.exit(1 if fails else 0)
