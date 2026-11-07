r"""The frame budget, self-checking (docs/testing.md "Frame time and OAM probe"; docs/perfect-pass.md 11.4, 11.6).
usage: python tools/headless/perf_budget.py ROM STATE [--out DIR]     (a DEBUG_MENU=1 ROM and its menu.ss)

Runs the spots of perf_spots.txt alone and with player 2, then West City's arrival, a forced eviction and the plaza's
flight sign. Prints "ok ..." or "FAIL ..." per check, a summary, exit 1 on a failure. regfull.sh greps FAIL.
Builds its own runner (/root/st_perf, or ST_HEADLESS=...): the `perf` commands are newer than other sessions' builds."""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import perf_probe as P   # noqa: E402

fails = checks = 0


def check(ok, text):
    global fails, checks
    checks += 1
    if not ok:
        fails += 1
    print(("ok " if ok else "FAIL ") + text)


def symbol(rom, name):
    for ln in open(os.path.join(os.path.dirname(os.path.abspath(rom)), "coop.sym")):
        p = ln.split()
        if len(p) >= 3 and p[-1] == name:
            return int(p[0], 16)
    raise SystemExit(f"no symbol {name}")


def ensure_runner():
    if os.environ.get("ST_HEADLESS"):
        return
    src = os.path.join(HERE, "st_headless.c").replace("\\", "/")
    src = "/mnt/" + src[0].lower() + src[2:]
    build = (f"test /root/st_perf -nt {src} || gcc -O2 -o /root/st_perf {src} -I/root/mgba-build/include "
             "-I/mnt/f/MeltyCreations/tools/downloads/mgba/include -L/root/mgba-build -lmgba -Wl,-rpath,/root/mgba-build")
    r = subprocess.run(["wsl", "-d", "Ubuntu", "-u", "root", "--", "bash", "-c", build], capture_output=True, text=True,
                       env=dict(os.environ, MSYS_NO_PATHCONV="1"))
    if r.returncode:
        raise SystemExit("FAIL building the runner: " + r.stderr[-500:])
    os.environ["ST_HEADLESS"] = "/root/st_perf"


def perf_line(out, label):
    for ln in out.splitlines():
        m = P.PERF_RE.match(ln)
        if m and m.group(1) == label:
            return ln, dict(zip(("lab", "frames", "ticks", "lag", "avg", "max", "oam", "shadow", "line", "cut", "ents", "drop"), m.groups()))
    return None, {}


def spots_budget(rom, state, outdir):
    rows, bad = P.measure(rom, state, outdir, ["light", "wc_after", "plaza", "street", "courtyard", "waiting", "rumble", "wolves", "crowded"])
    check(not bad, f"every spot reached ({bad[:3]})")
    check(len(rows) == 34, f"{len(rows)} perf lines of 34")
    light = rows
    for lab in sorted(rows):
        fps, lag, av, mx, oam, line, cut, ents, drop = rows[lab]
        check(fps >= 58 and av <= 75, f"{lab}: {fps:.1f} fps, {lag} lagged, frame {av} % avg, {mx} % max (budget: 58 fps, 75 % avg)")
        check(oam < 128 and cut == 0, f"{lab}: {oam} OBJs, worst line {line} of 1210 cycles, {cut} cut")
    rows, bad = P.measure(rom, state, outdir, ["wc_talk", "wait_talk"])
    check(not bad and len(rows) == 4, f"talks measured ({len(rows)}, {bad[:3]})")
    for lab in sorted(rows):
        check(rows[lab][8] == 0, f"{lab}: a talk's box takes nobody (drop {rows[lab][8]})")
    # the probe against a known-light map: a map with nothing on it sits near the floor, and co-op adds little there
    check(light["light_stand.alone"][2] <= 40 and light["light_stand.coop"][2] <= 45,
          f"the floor: {light['light_stand.alone'][2]} % alone, {light['light_stand.coop'][2]} % with player 2")


ARRIVAL = """preset +4 +360 +21 +22 +23 +24 +26 +27 +29
maptest 176
waitmap 6 99 2000
frames 60
poke8 0x0203F000 0
poke32 0x0203FD0C 0x6306
poke32 0x0203FD54 0x063805D8
poke32 0x0203FD08 5
frames 150
key 1 10
waitmap 4 3 600
frames 60
"""


def arrival(rom, state, outdir, room):
    # A at West City's landing from the world map; the first line's box opens about 150 frames on
    for mode in ("alone", "coop"):
        body = ["{P2}\n", ARRIVAL, "perfstart\n", "frames 180\n", "perfstop wc_first{ROW}\n",
                f"peek {room:#x} 2\n", "echo ents\n", "peek 0x03001124\n", "shot wc_first{ROW}\n"]
        out = P.run_spot("wc_first", body, mode, rom, state, outdir)
        ln, r = perf_line(out, f"wc_first.{mode}")
        print(ln)
        check(bool(r) and int(r["drop"]) <= 1 and int(r["cut"]) == 0 and int(r["oam"]) < 128,
              f"West City's arrival {mode}: the first line's box takes nobody (drop {r.get('drop')}: the map's name leaving; 20 in co-op before the fix; {r.get('oam')} OBJs)")
        m = re.search(r"^0*%x: (\w{8}) (\w{8})" % room, out, re.M)
        taken = (int(m.group(1), 16) >> 24) if m else -1
        check(taken == 0, f"West City's arrival {mode}: on-screen bodies taken {taken}")


def forced(rom, state, outdir, room, hold=4096, name="forced"):
    # Test command 45 holds `hold` bytes at the free run's low end: the box does not fit as it is. With 4 KB the eviction's own
    # rounds (what is off screen) and the pack of the bodies' blocks (objroom.c, 2026-11-06) make its room: nobody on screen goes.
    # With 8 KB packing cannot do it: some of the crowd goes, one at a time, the players stay
    body = ["p2 0\n", ARRIVAL, f"poke32 0x0203FD0C {hold}\n", "poke32 0x0203FD08 45\n", "frames 2\n",
            "findent PL 0x08025604\n", "findent PTWO 0x0203FE00\n", "peek 0x03001124\n",
            "perfstart\n", "frames 180\n", f"perfstop {name}\n",
            f"peek {room:#x} 3\n", "ents\n", f"shot wc_{name}\n"]
    for _ in range(24):
        body += ["key 1 8\n", "frames 60\n"]
    body += ["frames 120\n", "echo after\n", "peek 0x03001124\n", "poke32 0x0203FD0C 0\n", "poke32 0x0203FD08 45\n", "frames 2\n", "map\n"]
    out = P.run_spot(name, body, "coop", rom, state, outdir)
    ln, r = perf_line(out, name)
    print(ln)
    m = re.search(r"^0*%x: (\w{8}) (\w{8}) (\w{8})" % room, out, re.M)
    word = int(m.group(1), 16) if m else 0
    taken, rnd, calls = word >> 24, (word >> 16) & 0xFF, int(m.group(2), 16) if m else 0
    moved = (int(m.group(3), 16) >> 8) & 0xFF if m else 0
    check(calls >= 1 and rnd >= 15, f"{name} eviction ran to its last round (round {rnd}, {calls} calls)")
    if hold <= 4096:
        check(taken == 0, f"{name} eviction: {taken} on-screen bodies taken (the pack makes the room: {moved} bodies moved)")
    else:
        check(1 <= taken <= 12, f"{name} eviction: {taken} on-screen bodies taken, not the whole crowd ({moved} bodies moved)")
    listed = re.findall(r"vt (\w{8})", out[out.index("perf " + name):out.index("\nafter")] if "\nafter" in out else "")
    check("08025604" in listed and "0203fe00" in listed, "forced eviction: both players stay listed through the box")
    check("08025974" in listed or "08024ee4" not in listed, "forced eviction: no decoration left without its sheet (it would show the box's tiles)")
    after = re.search(r"^after\s*\n03001124: (\w{8})", out, re.M)
    check(bool(after) and int(after.group(1), 16) >= 20, f"forced eviction: they are all back after the box ({after.group(1) if after else '?'} listed)")
    check("TIMEOUT" not in out and "CRASH" not in out, "forced eviction: no crash")


def sign(rom, state, outdir):
    # the plaza's flight sign: the decoration's entity, its post stops a walker, A at its foot flies
    body = ["{P2}\n", "maptest 176\n", "waitmap 6 99 2000\n", "frames 60\n", "flags +441\n",
            "poke32 0x0203FD0C 0x0B0F\n", "poke32 0x0203FD54 0x018800E8\n", "poke32 0x0203FD08 5\n", "frames 300\n",
            "findent SIGN 0x08025974\n", "oam\n", "keys 0x40\n", "frames 40\n", "keys 0\n", "frames 10\n", "map\n", "shot plaza_sign{ROW}\n",
            "key 1 10\n", "waitmap 6 99 300\n"]
    for mode in ("alone", "coop"):
        out = P.run_spot("sign", body, mode, rom, state, outdir)
        m = re.search(r"findent SIGN = (\w{8})", out)
        check(bool(m) and int(m.group(1), 16) != 0, f"plaza {mode}: the sign's decoration entity {m.group(1) if m else '?'}")
        check(re.search(r"^  obj +\d+ x +\d+ y +\d+ 32x32 tile", out, re.M) is not None, f"plaza {mode}: a 32 x 32 OBJ on screen")
        m = re.search(r"^map zone 15 area 11 var 1 player (\d+),(\d+)", out, re.M)
        check(bool(m) and 364 <= int(m.group(2)) <= 376, f"plaza {mode}: a walker stops at the sign's foot ({m.group(0) if m else 'no map line'})")
        check("waitmap 6 99: reached" in out.split("key")[0] or out.count("waitmap 6 99: reached") >= 2, f"plaza {mode}: A at its foot flies to the world map")


if __name__ == "__main__":
    args = sys.argv[1:]
    out_dir = args[args.index("--out") + 1] if "--out" in args else "build/hl"
    rom_path, state_path = args[0], args[1]
    os.makedirs(out_dir, exist_ok=True)
    ensure_runner()
    room_addr = symbol(rom_path, "obj_room")
    spots_budget(rom_path, state_path, out_dir)
    arrival(rom_path, state_path, out_dir, room_addr)
    forced(rom_path, state_path, out_dir, room_addr)
    forced(rom_path, state_path, out_dir, room_addr, 8192, "forced_full")
    sign(rom_path, state_path, out_dir)
    print(f"== expect: {checks} checks, {fails} failed")
    sys.exit(1 if fails else 0)
