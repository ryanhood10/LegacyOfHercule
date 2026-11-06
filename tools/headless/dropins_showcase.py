r"""F1-F9 for Ryan's YouTube Shorts: each state stands just before one of the new cinematic scenes, to play alone and record.
usage: python -I tools/headless/dropins_showcase.py PLAY_ROM DEBUG_ROM MENU_SS OUTDIR [--film] [--only f1,f4]
  PLAY_ROM   the play ROM (`make`); DEBUG_ROM the same source with DEBUG_MENU=1, MENU_SS its debug-menu state
  OUTDIR     gets legacy-of-hercule.gba (a copy of PLAY_ROM), coop.sym and legacy-of-hercule.ss1-.ss9
  --film     afterwards load each state on the copied ROM, answer the question, do Ryan's action, press A once a second
             and shoot every 60 frames: OUTDIR/film/<key>.png (one row of eight shots per 480 frames), the scene word, the map and
             the `ents` lists at the end (a vtable outside the ROM and EWRAM is a corrupted heap: reported)
  --only     make (and film) only these keys; the others' files are left as they are
Made as dropins_test.py makes its states (docs/testing.md "Test drop-ins"): Map Test and test warps on the debug ROM, re-saved on the
play ROM, every party member full, "How many players?" asked again on load (it waits until no scene runs), Hercule's Ki Blast bit, no
cartridge save inside. One player in each, the lineup the scene was built for alone. Rerun after every rebuild: states break with it.
F1 Trunks against Frieza and King Cold (1.7): walk up        F2 Gohan's death in the future (1.3): plays as it fades in
F3 Goku falls, Vegeta arrives, Super Saiyan against 19 (3.5): plays as it fades in
F4 the road's end, "Who's next?" (4.6): the road's fight is won as it loads   F5 Kami and Piccolo's fusion (5.5, 5.6): walk up
F6 Perfect Cell: 18 absorbed, the change, the kick, the Final Flash (7.4): A from Vegeta's "So be it, Cell"
F7 Broly's entrance (B.3): walk up    F8 16's head and Gohan's Super Saiyan 2 (3.6): plays as it fades in
F9 the Super 13 summit (6.2): walk up
"""
import os, re, shutil, struct, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
args = sys.argv[1:]
film = "--film" in args
if film:
    args.remove("--film")
only = None
if "--only" in args:
    i = args.index("--only")
    only = set(args[i + 1].split(","))
    del args[i:i + 2]
play_src, dbg, menu, out = (os.path.abspath(a) for a in args[:4])
BASE = "legacy-of-hercule"
os.makedirs(out, exist_ok=True)
play = os.path.join(out, BASE + ".gba")
# the states are made and checked on this copy: a rebuild of the source ROM during a run would otherwise be read half-way
if os.path.abspath(play_src) != play:
    shutil.copy(play_src, play)
    shutil.copy(os.path.join(os.path.dirname(play_src), "coop.sym"), os.path.join(out, "coop.sym"))
work = os.path.join(out, "work")
os.makedirs(work, exist_ok=True)


def wsl(p):
    p = os.path.abspath(p).replace("\\", "/")
    return "/mnt/" + p[0].lower() + p[2:]


def run(name, rom, text, state=None, cols="3"):
    script = os.path.join(work, name + ".txt")
    open(script, "w", newline="\n").write(text)
    cmd = [sys.executable, os.path.join(here, "st_test.py"), script, rom] + ([state] if state else []) + ["--out", work, "--cols", cols]
    res = subprocess.run(cmd, capture_output=True, text=True)
    log = res.stdout + res.stderr
    bad = [l for l in log.splitlines() if "FAILED" in l or "cannot" in l or "CRASH" in l]
    if bad:
        sys.exit(f"{name}: " + "; ".join(bad))
    return log


W = lambda n: wsl(os.path.join(work, n))
SCENE = "0x03001FDC"            # a dialogue sequence runs
CMD, ARG, ARG2 = "0x0203FD08", "0x0203FD0C", "0x0203FD54"


def warp(zone, area, x, y, settle=300):
    return f"poke32 {ARG} {zone | area << 8:#x}\npoke32 {ARG2} {x | y << 16:#x}\npoke32 {CMD} 5\nframes {settle}\n"


def begin(p1, maptest=7, preset="", settle=200):
    return "p2 off\n" + (f"preset {preset}\n" if preset else "") + f"maptest {maptest}\nframes {settle}\npoke8 0x0203F000 {p1}\n"


def fade_in(zone, area, x, y):
    """Warp into a whole-map trigger and save at the scene's first frame: it plays from its start as the map fades in."""
    return warp(zone, area, x, y, 0) + f"waitnz {SCENE} 900\n"


GOHAN, VEGETA, TRUNKS, PICCOLO, HERCULE = 0, 2, 3, 1, 5
CELL_Q = "poke32 0x0203F9B0 0xCE11CA3E\npoke8 0x0203F9B5 {q}\npoke8 0x0203F9B6 0\n"   # cg_state: the queued win (docs/testing.md "Cell saga staging")
SS2 = ("poke16 0x0203F0EC 300\npoke16 0x0203F0EE 300\npoke16 0x0203F0F0 100\npoke16 0x0203F0F2 100\npoke8 0x0203F0F4 28\n"
       "poke8 0x0203F100 0x7F\npoke32 0x0203F104 0x02170403\n")   # slot 8, Super Saiyan 2 Gohan, as ssj2_join leaves him (the
# post-game party), his abilities 3, 4, 23, 2 too: until 2026-11-06 the list was left to the map-visit bits, which made it 0, 0, 0, 64
# and crashed the game on its fourth ability (docs/party-8-slots.md "The map-visit bank")
UP = "keys 0x40\nframes 90\nkeys 0\n"

# key: (made on the debug ROM, Ryan's one action for the film after the question, what to watch)
STATES = {
    # 1.7 (staging_ch1_gen.py "landing"): the trigger is the clearing above y 144; Gohan alone (one choreography for every lineup)
    "f1": (begin(GOHAN, 52, "+4 +360") + "poke8 FLAGS+0x3A 1\n" + warp(3, 4, 200, 172), UP),
    # 1.3 (staging_ch1_gen.py "death"): the trigger is the whole map; Trunks alone with old Hercule (Hercule's row 24)
    "f2": ("p2 off\nmaptest 1\nframes 30\npoke8 0x0203F000 3\npoke8 0x0203F0A0 24\nflags +466\n" + fade_in(1, 2, 272, 432), ""),
    # 3.5 (staging_ch23_gen.py "battle"): the whole map; Vegeta alone at the area's entry point, the lead poked to him
    "f3": (begin(VEGETA, 7, "+76") + "poke8 FLAGS+0x3A 3\n" + fade_in(7, 15, 468, 556), ""),
    # 4.6 (ch4_staging_gen.py "_end"): the road's intro played through, then the fight won (test command 7 runs at the first frame
    # after the save, so the end plays as the state loads)
    "f4": ("p2 off\nmaptest 13\nframes 300\npoke8 0x0203F000 1\npoke8 FLAGS+11 0\n" + warp(8, 49, 200, 700)
           + f"stepz {SCENE} 80\nframes 60\npoke32 {CMD} 7\n", ""),
    # 5.5 / 5.6 (staging_ch568_gen.py "fusion"): the trigger is the Lookout above y 184 (from area 15's edge one arrives at y 232); Trunks alone
    "f5": (begin(TRUNKS, 7, "+129 +131") + warp(10, 16, 96, 214), UP),
    # 7.4 (cell_staging_gen.py zone12): in by area 15's pad, Semi-Perfect Cell's scene, the fight won, the deal's lines pressed to
    # Vegeta's "So be it, Cell" (25 presses); three more A and Cell flies to 18
    "f6": (begin(VEGETA, 7, "+144", 100) + warp(12, 15, 344, 100) + "key 1 6\nframes 600\n" + f"stepz {SCENE} 150\nframes 120\npoke32 {CMD} 7\nframes 120\n"
           + "key 1 6\nframes 100\n" * 25, ""),
    # B.3 (staging_ch9_gen.py "rim"): the rim's trigger is y 400-472; Vegeta alone on the plain between the ship and the basin
    "f7": (begin(VEGETA, 7, "+400 +401 +378 +377") + warp(15, 3, 320, 492), UP),
    # 3.6 (docs/testing.md "Cell saga staging", the table's row): Trunks player 1 (the scene makes Gohan player 1), alone, record 13,
    # fight 2's Cell kept out of the win, the win queued; saved at the scene's first frame once the win has fired
    "f8": (begin(TRUNKS, 325, "+191 +421 +193") + warp(14, 3, 368, 440) + f"poke32 {ARG} 1\npoke32 {CMD} 15\nframes 30\n"
           + "findent CELL 0x0203fb04\npoke32 CELL+0x188 0\n" + f"poke32 {ARG} 13\npoke32 {CMD} 1\nframes 30\n" + CELL_Q.format(q=142)
           + f"waitnz {SCENE} 900\n", ""),
    # 6.2 (staging_ch9_gen.py "summit", docs/testing.md "Cell saga staging"): Hercule alone at the summit's foot (the map's spawn), the
    # trigger above y 440; the post-game party with Super Saiyan 2 Gohan
    "f9": (begin(HERCULE, 261, "+195 +191 +480 +381") + SS2 + warp(15, 8, 336, 464), UP),
}
keys = [k for k in STATES if only is None or k in only]

shutil.copy(menu, os.path.join(work, "menu.ss"))
text = ""
for i, n in enumerate(keys):
    text += ("load menu.ss\n" if i else "") + STATES[n][0] + f"shot dbg_{n}\nmap\npeek {SCENE} 1\nssave {W('d' + n + '.ss')}\n"
log = run("dbg", dbg, text, os.path.join(work, "menu.ss"))
print("\n".join(l for l in log.splitlines() if l.startswith("map ") or l.startswith("03001fdc")))
run("conv", play, "".join(f"load {W('d' + n + '.ss')}\nframes 1\nssave {W(n + '.ss')}\n" for n in keys))

PARTY_ADDR = 0x0203F000


def fill(name):
    log = run(name + "_read", play, f"load {W(name + '.ss')}\nframes 1\npeek PARTY 0x1F0\n")
    words = next(l for l in log.splitlines() if l.startswith("0203f000:")).split(":", 1)[1].split()
    data = b"".join(struct.pack("<I", int(w, 16)) for w in words)
    pokes = []
    for slot in range(9):
        o = 0xC + slot * 0x1C
        if not struct.unpack_from("<I", data, o + 20)[0] & 1:
            continue
        mhp, mep = struct.unpack_from("<H", data, o + 2)[0], struct.unpack_from("<H", data, o + 6)[0]
        pokes += [f"poke16 0x{PARTY_ADDR + o:08X} {mhp}", f"poke16 0x{PARTY_ADDR + o + 4:08X} {mep}"]
    # no frame run here: a state saved at a scene's first frame keeps it
    run(name + "_fill", play, f"load {W(name + '.ss')}\n" + "\n".join(pokes) + f"\nssave {W(name + '.ss')}\n")


def players_addr():
    for line in open(os.path.join(out, "coop.sym")):
        m = re.match(r"^([0-9a-f]{8}) \w players$", line)
        if m:
            return int(m.group(1), 16)
    sys.exit("no `players` in the play ROM's coop.sym")


PLAYERS = players_addr()
for n in keys:
    fill(n)
    run(n + "_ask", play, f"load {W(n + '.ss')}\npoke32 0x{PLAYERS:08X} 0x91A7E250\npoke32 0x{PLAYERS + 4:08X} 0\nssave {W(n + '.ss')}\n")
for n in keys:
    path = os.path.join(out, f"{BASE}.ss{n[1:]}")
    shutil.copy(os.path.join(work, n + ".ss"), path)
    # Map Test parties are past the three-year skip, which they did not play: Hercule's Ki Blast (dropins.py)
    data = bytearray(open(path, "rb").read())
    data[0x21000 + 0x3F0AC] |= 0x10
    open(path, "wb").write(data)
print(f"wrote {BASE}.ss{'/'.join(k[1:] for k in keys)} in {out}")

if film:
    # as Ryan would: the question answered (A, then A on "One player."), his action, then A once a second; a shot every 60 frames
    fdir = os.path.join(out, "film")
    os.makedirs(fdir, exist_ok=True)
    SHOTS = dict(f1=220, f6=150)        # the longest scenes
    for n in keys:
        t = f"load {wsl(os.path.join(out, f'{BASE}.ss{n[1:]}'))}\np2 off\nframes 2\nmap\n"
        act = STATES[n][1]
        if act:
            t += "frames 100\nshot " + n + "_q\nkey 1 6\nframes 80\nkey 1 6\nframes 60\n" + act
        for i in range(SHOTS.get(n, 120)):
            t += f"shot {n}_{i:03d}\n" + ("map\n" if i % 8 == 0 else "") + f"peek {SCENE} 1\nkey 1 6\nframes 50\n"
        t += f"map\npeek {SCENE} 1\nents\n"
        log = run("film_" + n, play, t, cols="8")
        shutil.copy(os.path.join(work, f"film_{n}.png"), os.path.join(fdir, n + ".png"))
        vts = re.findall(r" vt ([0-9a-f]{8}) ", log)
        bad = [v for v in vts if not re.fullmatch(r"080[0-9a-f]{5}|0203[0-9a-f]{4}", v)]
        maps = [l for l in log.splitlines() if l.startswith("map ")]
        scene = [l.split()[-1] for l in log.splitlines() if l.startswith("03001fdc:")]
        open(os.path.join(fdir, n + ".log"), "w").write(log)
        print(f"{n}: {maps[0]} .. {maps[-1]}; scene word {''.join('1' if s != '00000000' else '0' for s in scene)}; "
              f"{len(vts)} entities, {len(bad)} with a bad vtable")
