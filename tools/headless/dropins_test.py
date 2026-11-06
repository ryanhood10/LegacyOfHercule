r"""F1-F9 for Ryan's one-player test of the 2026-11-05 builds: each state stands just before one new thing.
usage: python tools/headless/dropins_test.py PLAY_ROM DEBUG_ROM MENU_SS OUTDIR
  PLAY_ROM   the ROM in build/play (`make`); DEBUG_ROM the same source with DEBUG_MENU=1, MENU_SS its debug-menu state
Made on the debug ROM by Map Test and test warps, then re-saved on the play ROM (dropins.py's way: the two differ only in
the title patch). Every state: one player, the party at full HP and ki, "How many players?" asked again on load, no
cartridge save inside (loading one never touches the .sav). Rerun after every rebuild: states break with it.
F1 Gero's first stage   F2 the lab (13's revenge)   F3 the generator's eggs   F4 the road (17 and 18)
F5 Papaya Island's welcome   F6 the waiting room, every tier open (Legend 2 v 2: 32 fighters)
F7 the Time Chamber, first steps in   F8 the Chamber, five lights out, at the lab's door   F9 six lights out, at the gate
"""
import os, re, shutil, struct, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
play, dbg, menu, out = (os.path.abspath(a) for a in sys.argv[1:5])
work = os.path.join(out, "dropin_work")
os.makedirs(work, exist_ok=True)


def wsl(p):
    p = os.path.abspath(p).replace("\\", "/")
    return "/mnt/" + p[0].lower() + p[2:]


def run(name, rom, text, state=None):
    script = os.path.join(work, name + ".txt")
    open(script, "w", newline="\n").write(text)
    cmd = [sys.executable, os.path.join(here, "st_test.py"), script, rom] + ([state] if state else []) + ["--out", work]
    res = subprocess.run(cmd, capture_output=True, text=True)
    log = res.stdout + res.stderr
    bad = [l for l in log.splitlines() if "FAILED" in l or "cannot" in l or "CRASH" in l]
    if bad:
        sys.exit(f"{name}: " + "; ".join(bad))
    return log


W = lambda n: wsl(os.path.join(work, n))


def warp(za, x, y, settle=300):
    return f"poke32 0x0203FD0C {za:#x}\npoke32 0x0203FD54 {x | y << 16:#x}\npoke32 0x0203FD08 5\nframes {settle}\n"


WAIT = "+143 +144 +148 +149 +151 +154 +157"          # the ten-day wait (chamber_gen.py's LOCKED)
LEGEND = "+85 +127 +136 +146 +191 +301 +402 +421 +382"  # every tier open, Cooler beaten, the Broly chapter done (tny_n32_gen.py)
CHAMBER_DOOR = ("warp", 0x0309, 96, 160)


def chamber(lights, spot=None):
    t = f"p2 off\npreset {WAIT}\nmaptest 263\nframes 200\npoke8 0x0203F000 1\n"
    if lights:
        t += f"poke8 FLAGS+0x3F {lights:#x}\n"
    # through the Lookout's door: a real load of the open area
    t += warp(0x0309, 96, 160, 150) + "keys 0x40\nframes 20\nkeys 0\nkey 1 6\nwaitmap 15 18 600\nframes 260\n"
    if spot:
        t += warp(0x120F, *spot)
    return t


STATES = {
    "f1": "p2 off\nmaptest 13\nframes 300\npoke8 0x0203F000 1\n" + warp(0x0D08, 192, 336),          # below Gero's trigger (y <= 280)
    "f2": "p2 off\nmaptest 13\nframes 300\npoke8 0x0203F000 1\n" + warp(0x2E08, 190, 740),          # above the lab cutaway's rect (y >= 769): walk down
    "f3": "p2 off\nmaptest 13\nframes 300\npoke8 0x0203F000 1\n" + warp(0x2C08, 224, 90),           # area 44 by the north door: walk up
    "f4": "p2 off\nmaptest 13\nframes 300\npoke8 0x0203F000 1\n" + warp(0x3108, 200, 700, 12),      # inside the road's rect, saved while the map loads: the scene plays as it fades in
    "f5": "p2 off\nmaptest 263\nframes 300\nflags -441\n" + warp(0x0B0F, 0x188, 0xE8),             # the beach, the welcome ahead
    "f6": f"p2 off\nmaptest 263\nframes 300\nflags {LEGEND} +441\n" + warp(0x0F0F, 300, 200)      # beside the director
          + "key 1 6\nframes 150\n" * 8,                                                         # the new tiers' pop-ups read
    "f7": chamber(0),
    "f8": chamber(0x2F, (432, 1800)),                                                             # lights 0-3 and 5 out: the lab (4) left
    "f9": chamber(0x3F, (1028, 400)),                                                             # all six out: walk up between the pillars
}
shutil.copy(menu, os.path.join(work, "menu.ss"))
text = ""
for i, (n, t) in enumerate(STATES.items()):
    text += ("load menu.ss\n" if i else "") + t + f"shot dbg_{n}\nmap\nssave {W('d' + n + '.ss')}\n"
log = run("dbg", dbg, text, os.path.join(work, "menu.ss"))
print("\n".join(l for l in log.splitlines() if l.startswith("map ")))
run("conv", play, "".join(f"load {W('d' + n + '.ss')}\nframes 1\nssave {W(n + '.ss')}\n" for n in STATES))

PARTY_ADDR = 0x0203F000


def fill(name):
    log = run(name + "_read", play, f"load {W(name + '.ss')}\nframes 2\npeek PARTY 0x1F0\n")
    words = next(l for l in log.splitlines() if l.startswith("0203f000:")).split(":", 1)[1].split()
    data = b"".join(struct.pack("<I", int(w, 16)) for w in words)
    pokes = []
    for slot in range(9):
        o = 0xC + slot * 0x1C
        if not struct.unpack_from("<I", data, o + 20)[0] & 1:
            continue
        mhp, mep = struct.unpack_from("<H", data, o + 2)[0], struct.unpack_from("<H", data, o + 6)[0]
        pokes += [f"poke16 0x{PARTY_ADDR + o:08X} {mhp}", f"poke16 0x{PARTY_ADDR + o + 4:08X} {mep}"]
    run(name + "_fill", play, f"load {W(name + '.ss')}\n" + "\n".join(pokes) + f"\nframes 1\nssave {W(name + '.ss')}\n")


def players_addr():
    sym = os.path.join(os.path.dirname(play), "coop.sym")
    if os.path.exists(sym):
        for line in open(sym):
            m = re.match(r"^([0-9a-f]{8}) \w players$", line)
            if m:
                return int(m.group(1), 16)
    sys.exit("no `players` in the play ROM's coop.sym")


PLAYERS = players_addr()
for n in STATES:
    fill(n)
    run(n + "_ask", play, f"load {W(n + '.ss')}\npoke32 0x{PLAYERS:08X} 0x91A7E250\npoke32 0x{PLAYERS + 4:08X} 0\nframes 1\nssave {W(n + '.ss')}\n")
base = os.path.splitext(os.path.basename(play))[0]
for i, n in enumerate(STATES, 1):
    shutil.copy(os.path.join(work, n + ".ss"), os.path.join(out, f"{base}.ss{i}"))
    # Map Test parties are past the three-year skip, which they did not play: Hercule's Ki Blast (dropins.py)
    path = os.path.join(out, f"{base}.ss{i}")
    data = bytearray(open(path, "rb").read())
    data[0x21000 + 0x3F0AC] |= 0x10
    open(path, "wb").write(data)
print(f"wrote {base}.ss1-9 in {out}")
