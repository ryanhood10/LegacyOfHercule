r"""The frame-time and OAM probe's helper (docs/testing.md "Frame time and OAM probe").

python tools/headless/perf_probe.py hot TRACE ROM [--top N]
    Roll a runner `ptrace N FILE` up by function: emulated cycles and share per function. Stock functions by the
    decompile corpus' start addresses (build/decomp_all, or LOG2_DECOMP=<dir>), ours by coop.sym next to the ROM.
python tools/headless/perf_probe.py table ROM STATE [--out DIR] [--spots a,b,c]
    Run every spot of tools/headless/perf_spots.txt alone and with player 2 and print the perf lines as a table."""
import bisect
import glob
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
PERF_RE = re.compile(r"perf (\S+)\s+frames (\d+) ticks (\d+) lag (\d+) busy avg \d+ \((\d+)%\) max \d+ \((\d+)%\) "
                     r"oam max (\d+) avg \d+ shadow (\d+) line max (\d+) cut (\d+) in \d+ frames ents max (\d+) drop (\d+)")


def sym_table(rom):
    table = []
    sym = os.path.join(os.path.dirname(os.path.abspath(rom)), "coop.sym")
    if os.path.exists(sym):
        for ln in open(sym):
            p = ln.split()
            if len(p) >= 3 and re.fullmatch(r"[0-9a-f]{8}", p[0]) and p[1] in ("T", "t", "F", "f"):
                table.append((int(p[0], 16), p[-1]))
    corpus = os.environ.get("LOG2_DECOMP", "F:/MeltyCreations/log2/build/decomp_all")
    for f in glob.glob(os.path.join(corpus, "*.c")):
        m = re.match(r"([0-9a-f]{8})_(\w+)\.c", os.path.basename(f))
        if m:
            table.append((int(m.group(1), 16), m.group(2)))
    table.sort()
    return table


def hot(trace, rom, top):
    table = sym_table(rom)
    starts = [a for a, _ in table]
    per = {}
    total = 0
    for ln in open(trace):
        pc, n, c = ln.split()
        pc, n, c = int(pc, 16), int(n), int(c)
        i = bisect.bisect_right(starts, pc) - 1
        name = table[i][1] if i >= 0 else "?"
        if 0x03000000 <= pc < 0x08000000:
            name = "ram_%07x" % (pc & ~0xFF)
        if pc < 0x4000:
            name = "bios (the Halt)"
        e = per.setdefault(name, [0, 0])
        e[0] += c
        e[1] += n
        total += c
    print(f"total {total} cycles")
    for name, (c, n) in sorted(per.items(), key=lambda kv: -kv[1][0])[:top]:
        print(f"{c:10d} {100 * c / total:5.1f}% {n:9d} instrs  {name}")


def spots(only=None):
    out, name = {}, None
    for ln in open(os.path.join(HERE, "perf_spots.txt")):
        if ln.startswith("== "):
            name = ln[3:].strip()
            out[name] = []
        elif name and not ln.startswith("#"):
            out[name].append(ln)
    return {k: v for k, v in out.items() if not only or k in only}


def run_spot(name, body, mode, rom, state, outdir, extra=""):
    """One spot in one mode ("alone" or "coop"); returns the runner's stdout."""
    log = os.path.abspath(os.path.join(outdir, f"perf_{name}_{mode}.log")).replace("\\", "/")
    log = "/mnt/" + log[0].lower() + log[2:]
    text = "".join(body).replace("{ROW}", "." + mode).replace("{LOG}", log)
    text = text.replace("{P2A}", "p2 1" if mode == "coop" else "").replace("{P2W}", "p2 0x20" if mode == "coop" else "").replace("{P2}", "p2 0" if mode == "coop" else "p2 off")
    path = os.path.join(outdir, f"perf_{name}_{mode}.txt")
    open(path, "w", newline="\n").write("load menu.ss\n" + text + extra)
    r = subprocess.run([sys.executable, os.path.join(HERE, "st_test.py"), path, rom, state, "--out", outdir],
                       capture_output=True, text=True)
    if r.stderr.strip():
        sys.stderr.write(r.stderr[-800:])
    return r.stdout


def measure(rom, state, outdir, only=None):
    """{label: (fps, lag, avg%, max%, oam, line, cut, ents, drop)} over the spots in both modes, plus any TIMEOUT / CRASH lines."""
    os.makedirs(outdir, exist_ok=True)
    jobs = [(n, b, m) for n, b in spots(only).items() for m in ("alone", "coop")]
    with ThreadPoolExecutor(4) as ex:
        outs = list(ex.map(lambda j: run_spot(j[0], j[1], j[2], rom, state, outdir), jobs))
    rows, bad = {}, []
    for o in outs:
        for ln in o.splitlines():
            m = PERF_RE.match(ln)
            if m:
                lab, fr, ti, lag, av, mx, oam, sh, line, cut, ents, drop = m.groups()
                rows[lab] = (60 * int(ti) / max(1, int(fr)), int(lag), int(av), int(mx), int(oam), int(line), int(cut), int(ents), int(drop))
            elif "TIMEOUT" in ln or "CRASH" in ln:
                bad.append(ln)
    return rows, bad


def table(rom, state, outdir, only):
    rows, bad = measure(rom, state, outdir, only)
    for ln in bad:
        print(ln)
    print(f"{'spot':24s} {'fps':>5s} {'lag':>5s} {'avg%':>5s} {'max%':>5s} {'oam':>4s} {'line':>5s} {'cut':>4s} {'ents':>4s} {'drop':>4s}")
    for lab in sorted(rows):
        fps, lag, av, mx, oam, line, cut, ents, drop = rows[lab]
        print(f"{lab:24s} {fps:5.1f} {lag:5d} {av:5d} {mx:5d} {oam:4d} {line:5d} {cut:4d} {ents:4d} {drop:4d}")


if __name__ == "__main__":
    args = sys.argv[1:]
    if args and args[0] == "hot":
        hot(args[1], args[2], int(args[args.index("--top") + 1]) if "--top" in args else 30)
    elif args and args[0] == "table":
        outdir = args[args.index("--out") + 1] if "--out" in args else "build/hl"
        only = args[args.index("--spots") + 1].split(",") if "--spots" in args else None
        table(args[1], args[2], outdir, only)
    else:
        print(__doc__)
