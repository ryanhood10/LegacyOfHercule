r"""Drop-in savestates for play-testing: mGBA's F1-F7 slots next to the play ROM, each at an important scene.
usage: python tools/headless/dropins.py PLAY_ROM DEBUG_ROM MENU_SS SAV OUTDIR
  PLAY_ROM   the ROM Ryan plays (`make`); the states are written for it (named <rom>.ss1-6 in OUTDIR)
  DEBUG_ROM  the same source built with DEBUG_MENU=1 (differs only in the title patch); MENU_SS its debug-menu
             state (tools/headless/mkmenu.txt)
  SAV        a battery save standing on the Northern Wastelands save pad (zone 3 area 3) with Krillin
F1 that save pad (the save, answered 1 Player); F2 Goku's landing site (played on from F1 through Frieza's landing,
then the fly pad's warp); F3 Gero's computer and F4 the Cell Games path (Map Test with the chapter tests' flags;
Goku player 1 for the ring's barrier); F6 the post-game Lookout below Dende and F5 the Arctic summit after the
wishes, both after the Cell Games played through alone; F7 the Broly chapter's backyard (Vegeta, the ship there) (cell_games_1p.txt, then postgame.txt's wishes), so the
party is the real one (Goku's halo, Super Saiyan 2 Gohan). F3-F6 are made on DEBUG_ROM and re-saved on PLAY_ROM. States carry no cartridge save (`ssave`), so
loading one never touches the .sav. Savestates break with every rebuild: run this again after each.
"""
import os, shutil, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
play, dbg, menu, sav, out = (os.path.abspath(a) for a in sys.argv[1:6])
os.makedirs(out, exist_ok=True)
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
    bad = [l for l in log.splitlines() if "FAILED" in l or "cannot" in l]
    if bad:
        sys.exit(f"{name}: " + "; ".join(bad))
    return log


W = lambda n: wsl(os.path.join(work, n))
shutil.copy(sav, os.path.join(work, "base.sav"))
WARP = "poke32 0x0203FD0C {za}\npoke32 0x0203FD54 {xy}\npoke32 0x0203FD08 5\n"

# F1: the save loaded (Start, Select Game, the file, 1 Player)
run("f1", play, f"""p2 off
cart {W('base.sav')}
frames 900
key 0x08 6
frames 120
key 0x01 6
frames 300
key 0x01 6
frames 300
key 0x01 6
frames 300
key 0x01 6
frames 400
ssave {W('f1.ss')}
""")
# F2: Frieza's landing played through, then the landing site's arrival (the group flies in)
run("f2", play, f"""load {W('f1.ss')}
p2 off
frames 30
{WARP.format(za='0x0403', xy='0x007800C8')}frames 280
mash 130 110
frames 300
{WARP.format(za='0x0603', xy='0x01C801A0')}frames 45
ssave {W('f2.ss')}
""")
# F3 / F4 on the debug ROM by Map Test with the chapter tests' flags (both before the Cell Games, so the stock
# party there is right; F4 makes Goku player 1: the ring's barrier lets only him through)
# the story before the ten-day wait done (a Map Test sets only its chapter's flags): every earlier world-map landing
# shows its plain marker, not a stale star, and the Lookout's landing (flag 127) is open; worked out from the world
# map's on-load (each landing's star condition false with these set; 158 would bring back Capsule Corp's star)
EARLY = ("+26 +30 +66 +68 +69 +77 +86 +127 +129 +132 +136 +140 +141 +143 +144 +148 +149 +151 +157 "
         "+162 +163 +164 +185 +186 +187 +188 +400")   # 400: Bulma's Saiyan-ship news (the Broly chapter: the ship waits in the backyard)
chapters = [
    ("f3", "+151 +155 +156 +157", 149, None, 300, ""),                           # Gero's computer (Capsule Corp)
    ("f4", "+191 +421", 323, ("0x010E", "0x00700108"), 300, "poke8 0x0203F000 4\n"),   # the Cell Games path
    # F7 the Broly chapter (Ryan): the Capsule Corp backyard with the Saiyan ship (flag 400), Vegeta player 1, just
    # right of the Paragus trigger on the walkway (rect 64..112 x 232..312: walk left into it); the ship is map art south of it
    ("f7", "+151 +155 +156 +157 +400", 165, ("0x1506", "0x011000A8"), 300, "poke8 0x0203F000 2\n"),
]
text = ""
for i, (n, flags, entry, warp, settle, extra) in enumerate(chapters):
    text += ("load menu.ss\n" if i else "") + f"p2 off\npreset {flags} {EARLY}\nmaptest {entry}\n"
    if warp:
        text += "frames 200\n" + extra + WARP.format(za=warp[0], xy=warp[1])
    text += f"frames {settle}\nssave {W('d' + n + '.ss')}\n"
shutil.copy(menu, os.path.join(work, "menu.ss"))
run("dbg", dbg, text, os.path.join(work, "menu.ss"))


def one_player(path, first=None, last=None):
    """A headless test's lines run alone (p2 0 -> p2 off, its other p2 lines dropped), lines first..last (1-based)."""
    lines = open(os.path.join(here, path), encoding="utf-8").read().split("\n")[(first or 1) - 1:last]
    out = []
    for l in lines:
        if l.startswith("#") or l.startswith("echo") or l.startswith("peek") or l.startswith("shot") or l == "map":
            continue
        if l == "p2 0":
            l = "p2 off"
        elif l.startswith("p2 "):
            continue
        out.append(l)
    return "\n".join(out) + "\n"


# F6 / F5 after the Cell Games played through (tools/headless/cell_games_1p.txt: Goku's halo, Super Saiyan 2
# Gohan, the flags the Games set), F6 below Dende on the Lookout (the flyer lands at the Lookout's world-map site,
# trigger 8 of map 176: the test warp only moves the flyer there; A first closes the Games' last 'Journal Updated!',
# which holds the command); F5 then makes the wishes (postgame.txt's first
# part) and stands at the Arctic summit
games = one_player("cell_games_1p.txt").replace("preset +191 +421\n", f"preset +191 +421 {EARLY}\n", 1)
assert EARLY in games
run("games", dbg, games + f"""key 1 6
frames 200
{WARP.format(za='0x6306', xy='0x05C40380')}frames 150
key 1 10
frames 300
{WARP.format(za='0x0109', xy='0x01C002F8')}frames 200
ssave {W('df6.ss')}
""" + one_player("postgame.txt", 11, 500) + f"""frames 300
{WARP.format(za='0x080F', xy='0x01D00150')}frames 300
ssave {W('df5.ss')}
""", os.path.join(work, "menu.ss"))
run("conv", play, "".join(f"load {W('d' + n + '.ss')}\nframes 1\nssave {W(n + '.ss')}\n" for n in ("f3", "f4", "f5", "f6", "f7")))

base = os.path.splitext(os.path.basename(play))[0]
for i in range(1, 8):
    shutil.copy(os.path.join(work, f"f{i}.ss"), os.path.join(out, f"{base}.ss{i}"))
print(f"wrote {base}.ss1-7 in {out}")
