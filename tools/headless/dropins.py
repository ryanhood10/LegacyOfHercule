r"""Drop-in savestates for play-testing: mGBA's F1-F6 slots next to the play ROM, each at an important scene.
usage: python tools/headless/dropins.py PLAY_ROM DEBUG_ROM MENU_SS SAV OUTDIR
  PLAY_ROM   the ROM Ryan plays (`make`); the states are written for it (named <rom>.ss1-6 in OUTDIR)
  DEBUG_ROM  the same source built with DEBUG_MENU=1 (differs only in the title patch); MENU_SS its debug-menu
             state (tools/headless/mkmenu.txt)
  SAV        a battery save standing on the Northern Wastelands save pad (zone 3 area 3) with Krillin
F1 that save pad (the save, answered 1 Player); F2 Goku's landing site (played on from F1 through Frieza's landing,
then the fly pad's warp); F3-F6 Gero's computer, the Cell Games path, the Arctic, the post-game Lookout (Map Test with
each chapter test's flags, made on DEBUG_ROM, re-saved on PLAY_ROM). States carry no cartridge save (`ssave`), so
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
# F3-F6 on the debug ROM (Map Test + the chapter tests' flags), then re-saved on the play ROM
chapters = [
    ("f3", "+151 +155 +156 +157", 149, None, 40),                              # Gero's computer (Capsule Corp)
    ("f4", "+191 +421", 323, ("0x010E", "0x00700108"), 30),                     # the Cell Games path
    ("f5", "+195 +191 +480 +381", 261, ("0x080F", "0x01D00150"), 30),           # the Arctic
    ("f6", "+195 +191", 261, ("0x0109", "0x01C002F8"), 120),                    # the post-game Lookout
]
text = ""
for i, (n, flags, entry, warp, settle) in enumerate(chapters):
    text += ("load menu.ss\n" if i else "") + f"p2 off\npreset {flags}\nmaptest {entry}\n"
    if warp:
        text += "frames 200\n" + WARP.format(za=warp[0], xy=warp[1])
    text += f"frames {settle}\nssave {W('d' + n + '.ss')}\n"
shutil.copy(menu, os.path.join(work, "menu.ss"))
run("dbg", dbg, text, os.path.join(work, "menu.ss"))
run("conv", play, "".join(f"load {W('d' + n + '.ss')}\nframes 1\nssave {W(n + '.ss')}\n" for n, *_ in chapters))

base = os.path.splitext(os.path.basename(play))[0]
for i in range(1, 7):
    shutil.copy(os.path.join(work, f"f{i}.ss"), os.path.join(out, f"{base}.ss{i}"))
print(f"wrote {base}.ss1-6 in {out}")
