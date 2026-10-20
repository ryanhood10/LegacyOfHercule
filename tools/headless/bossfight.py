r"""Headless boss fight with the test autopilot (docs/testing.md "Super Android 13"): Map Test, warp to an open
map, spawn a boss, turn on the A16_TEST autopilot and log both players' and the boss's HP every second until
the boss is down, a player's HP is 0, or SECS pass. Runs from st_headless's debug-menu state; deterministic,
so DELAY (frames idled before the spawn) varies the RNG between samples.

usage: python tools/headless/bossfight.py ROM MENU_STATE KIND [--delay N ...] [--secs 240] [--map Z,A,X,Y]
           [--level30] [--out DIR] [--shots]
  ROM          an A16_TEST=1 DEBUG_MENU=1 build (Windows path)
  KIND         s13 (Super Android 13, test command 17) or cell (the Cell Games' final Perfect Cell, stat 25
               record 0x084090F8, through COOP_CMD_SPAWN_RECORD: a co-op twin pair, both-down) or a record
               address (0x...)
  --delay      one fight per value (default 0)
  --hp N       with --strong: both players' HP (a deep pool, so the autopilot lives long enough for a
               time-to-kill; damage taken per second is the boss's damage rate)
  --refill     with --strong: both players back to full HP after every one-second reading (the pair never
               falls, so the fight runs to a time-to-kill; the drops add up to the damage the boss dealt)
  --strong     a Cell Games-level pair (STRONG below) for the Cell-tier comparison
  --level30    the level-30 stats of the Android 16 tests (Piccolo 554 HP, Hercule 245) instead of the debug
               party's level 40
Players: Map Test's player 1 is Piccolo (slot 1), player 2 Hercule (slot 5).
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
P1E, P2E = 0x0203F028, 0x0203F098          # Piccolo, Hercule party entries (hp u16 at +0)
CELL_RECORD = 0x084090F8
S13 = 0x0203F91C                            # super13.c `s13` (.party.s13); read from the build's coop.sym when there
TWINS = 0x0203FC24                          # coop2.twins[0].orig / .twin


def stats(p1, p2):
    """Party entries and entity combat bytes for both players: (hp, str, pow, end) each; bytes = stat >> 8."""
    L = []
    for e, (hp, st, pw, en) in ((P1E, p1), (P2E, p2)):
        L += [f"poke16 {e:#x} {hp}", f"poke16 {e + 2:#x} {hp}", f"poke16 {e + 0xA:#x} {st}",
              f"poke16 {e + 0xC:#x} {pw}", f"poke16 {e + 0xE:#x} {en}"]
    L += ["deref pl 0x03001FA8", f"poke8 pl+0x160 {p1[1] >> 8}", f"poke8 pl+0x161 {p1[2] >> 8}", f"poke8 pl+0x162 {p1[3] >> 8}",
          "deref q 0x0203FF08", f"poke8 q+0x160 {p2[1] >> 8}", f"poke8 q+0x161 {p2[2] >> 8}", f"poke8 q+0x162 {p2[3] >> 8}"]
    return L


# --strong: a Cell Games-level pair (our estimate: the twin Perfect Cells' own numbers are 60 / 59 / 65)
STRONG = ((1500, 72 << 8, 75 << 8, 62 << 8), (900, 72 << 8, 70 << 8, 84 << 8))


def script(kind, delay, secs, mp, level30, shots, strong=False, hp=0, refill=False):
    z, a, x, y = mp
    L = ["p2 0", "maptest 147", "frames 300",
         f"poke32 0x0203FD0C {z | a << 8:#x}", f"poke32 0x0203FD54 {x | y << 16:#x}", "poke32 0x0203FD08 5",
         "frames 300"]
    if level30:   # docs/testing.md "Android 16 boss fight": party entries and entity combat bytes
        L += [f"poke16 {P1E:#x} 554", f"poke16 {P1E + 2:#x} 554", f"poke16 {P1E + 0xA:#x} 11955",
              f"poke16 {P1E + 0xC:#x} 12750", f"poke16 {P1E + 0xE:#x} 10140",
              f"poke16 {P2E:#x} 245", f"poke16 {P2E + 2:#x} 245", f"poke16 {P2E + 0xA:#x} 12030",
              f"poke16 {P2E + 0xC:#x} 11650", f"poke16 {P2E + 0xE:#x} 16130",
              "deref pl 0x03001FA8", "poke8 pl+0x160 46", "poke8 pl+0x161 49", "poke8 pl+0x162 39",
              "deref q 0x0203FF08", "poke8 q+0x160 47", "poke8 q+0x161 45", "poke8 q+0x162 63"]
    if strong:
        p1, p2 = STRONG
        if hp:
            p1, p2 = (hp,) + p1[1:], (hp,) + p2[1:]
        L += stats(p1, p2)
    L.append(f"frames {delay + 1}")
    if kind == "s13":
        L += ["poke32 0x0203FD08 17", "frames 30", f"deref b {S13 + 4:#x}"]
    else:
        rec = CELL_RECORD if kind == "cell" else int(kind, 0)
        L += [f"poke32 0x0203FD0C {rec:#x}", "poke32 0x0203FD08 8", "frames 30",
              f"deref b {TWINS:#x}", f"deref w {TWINS + 4:#x}"]
    # player 2 on the far side of the boss (the bot walks straight at its spot and a boss's body blocks it):
    # the spawn stands at player 1's x + 72 (Super 13) or + 48 and the twin TWIN_OFFSET further (pairs)
    L += ["deref q 0x0203FF08", f"poke32 q+0x158 {(x + (132 if kind == 's13' else 150)) << 8:#x}", f"poke32 q+0x15C {y << 8:#x}"]
    L.append("poke16 0x0203FD44 1")
    if shots:
        L.append("shot f_000")
    for t in range(1, secs + 1):
        L.append("frames 60")
        L.append(f"echo T {t}")
        if kind == "s13":
            L.append("peek b+0x188")
        else:
            L += [f"deref b {TWINS:#x}", f"deref w {TWINS + 4:#x}", "peek b+0x188", "peek w+0x188"]
        L.append(f"peek16 {P1E:#x}")
        L.append(f"peek16 {P2E:#x}")
        if refill:   # back to full after each reading: the drops add up to the damage taken
            L += [f"poke16 {P1E:#x} {STRONG[0][0]}", f"poke16 {P2E:#x} {STRONG[1][0]}"]
        if shots and t % 5 == 0:
            L.append(f"shot f_{t:03d}")
    if kind == "s13":
        L.append(f"peek {S13:#x} 11")
    return "\n".join(L) + "\n"


def run(rom, state, kind, delay, secs, mp, level30, out, shots, strong=False, hp=0, refill=False):
    os.makedirs(out, exist_ok=True)
    path = os.path.join(out, f"fight_{kind}_{delay}.txt")
    open(path, "w", newline="\n").write(script(kind, delay, secs, mp, level30, shots, strong, hp, refill))
    r = subprocess.run([sys.executable, os.path.join(HERE, "st_test.py"), path, rom, state, "--out", out, "--cols", "6"],
                       capture_output=True, text=True)
    t, rows = 0, []
    vals = []
    ptr = {}
    for ln in r.stdout.splitlines():
        m = re.match(r"deref (\w+) = ([0-9a-f]+)", ln)
        if m:
            ptr[m.group(1)] = int(m.group(2), 16)
            continue
        m = re.match(r"T (\d+)", ln)
        if m:
            if vals:
                rows.append((t, vals))
            t, vals = int(m.group(1)), []
            continue
        m = re.match(r"^([0-9a-f]{8}): ([0-9a-f]+)$", ln.strip())
        if m and t:
            v = int(m.group(2), 16)
            v = v - (1 << 32) if len(m.group(2)) == 8 and v >= 1 << 31 else v
            if int(m.group(1), 16) < 0x02000000:      # a boss pointer of 0 (a fallen twin): down
                v = 0
            vals.append(v)
    if vals:
        rows.append((t, vals))
    result = None
    for t, v in rows:
        boss = v[:-2]
        p1, p2 = v[-2], v[-1]
        if all(b <= 0 for b in boss):
            result = ("win", t, boss, p1, p2)
            break
        if p1 == 0 or p2 == 0:
            result = ("ko", t, boss, p1, p2)
            break
    if not result and rows:
        t, v = rows[-1]
        result = ("time", t, v[:-2], v[-2], v[-1])
    tail = [ln for ln in r.stdout.splitlines() if ln.startswith(f"{S13:08x}")]
    return result, rows, tail


def main():
    a = sys.argv[1:]
    def opt(k, d=None, many=False):
        vals = []
        while k in a:
            i = a.index(k)
            vals.append(a[i + 1] if i + 1 < len(a) and not a[i + 1].startswith("--") else True)
            del a[i:i + (2 if vals[-1] is not True else 1)]
        return vals if many else (vals[-1] if vals else d)
    delays = [int(v) for v in opt("--delay", many=True)] or [0]
    secs = int(opt("--secs", 240))
    mp = tuple(int(v) for v in opt("--map", "15,3,276,354").split(","))
    level30 = bool(opt("--level30", False))
    shots = bool(opt("--shots", False))
    verbose = bool(opt("--verbose", False))
    strong = bool(opt("--strong", False))
    hp = int(opt("--hp", 0))
    refill = bool(opt("--refill", False))
    out = opt("--out", "build/hl/fight")
    rom, state, kind = a[0], a[1], a[2]
    global S13
    sym = os.path.join(os.path.dirname(os.path.abspath(rom)), "coop.sym")
    if os.path.exists(sym):
        for ln in open(sym):
            f = ln.split()
            if len(f) == 3 and f[2] == "s13":
                S13 = int(f[0], 16)
    for d in delays:
        result, rows, tail = run(rom, state, kind, d, secs, mp, level30, out, shots, strong, hp, refill)
        if refill and result:
            # rates over the active seconds only: a second counts while some HP changed within the last
            # 10 s (the Cell pair sometimes stalls under the autopilot: everyone stands, nothing changes)
            n = [r for r in rows if r[0] <= result[1]]
            active, dealt, taken, last, prev = 0, 0, 0, 0, None
            for t, v in n:
                hit = (STRONG[0][0] - v[-2]) + (STRONG[1][0] - v[-1])
                boss = sum(max(0, b) for b in v[:-2])
                done = (prev - boss) if prev is not None else 0
                prev = boss
                if hit or done:
                    last = t
                if t - last <= 10:
                    active += 1
                    taken += hit
                    dealt += done
            start = sum(max(0, b) for b in n[0][1][:-2]) if n else 0
            pd, bd = dealt / max(1, active), taken / max(1, active)
            print(f"{kind} delay={d}: {result[0]} t={result[1]} boss={result[2]} active {active} s: players deal {pd:.0f}/s "
                  f"(est. kill {start / max(1, pd):.0f} s of {start} HP), take {bd:.0f}/s  {' '.join(tail)}")
        else:
            print(f"{kind} delay={d}: {result}  {' '.join(tail)}")
        if verbose:
            for t, v in rows[:result[1] if result else None:5]:
                print(f"   t={t:3d} boss={v[:-2]} p1={v[-2]} p2={v[-1]}")
        sys.stdout.flush()


if __name__ == "__main__":
    main()
