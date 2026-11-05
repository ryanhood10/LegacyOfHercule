r"""Write tools/headless/staging_ch5.txt, staging_ch6.txt and staging_ch8.txt: chapters 5, 6 and Cooler per lineup (ch5_lookout, ch6_basement, ch8_cooler).
usage: python tools/headless/staging_ch568_gen.py     (the scripts are committed; rerun this after changing them here)
Chapter 5: Kami defers, whoever is selectable (the fade makes Piccolo player 1; Hercule's actor when nobody plays him), then Kami's boost and
the fusion. Chapter 6: Goku wakes, Krillin's actor comes up only when nobody plays him. Cooler: the three openings alone (no servant) and with
two players (the servant's actor, then his twin on the same sprite 166)."""
from staging_lib import *

s = Script("staging_ch568_gen.py")
for p1, p2 in ((PICCOLO, None), (HERCULE, None), (KRILLIN, None), (GOHAN, None), (PICCOLO, HERCULE), (HERCULE, PICCOLO), (PICCOLO, KRILLIN), (KRILLIN, PICCOLO)):
    n = lineup_name(p1, p2)
    s.begin(f"lookout {n}", p1, p2)
    s.warp(9, 6, 112, 224, 300)
    s.press(26, 110)
    s.shot(f"c5_{n}_stay")
    s.out("ents")
    s.end_scene(200)
    s.on_map(2, 20)
for p1, p2 in ((TRUNKS, None), (GOHAN, None), (TRUNKS, GOHAN), (GOHAN, TRUNKS)):
    n = lineup_name(p1, p2)
    s.begin(f"fusion {n}", p1, p2)
    s.warp(10, 16, 96, 150, 100)
    s.press(56, 80)
    s.shot(f"c5_{n}_kami")
    s.out("ents")
    s.end_scene(300)
    s.on_map(6, 99)
s.write("staging_ch5.txt")

s = Script("staging_ch568_gen.py")
for p1, p2, s76 in ((TRUNKS, None, 1), (KRILLIN, None, 0), (TRUNKS, KRILLIN, 0), (KRILLIN, TRUNKS, 0)):
    n = lineup_name(p1, p2)
    s.begin(f"wakes {n}", p1, p2, "+141")
    s.warp(11, 3, 136, 100, 100)
    s.press(45, 80)
    s.out("frames 60")
    s.shot(f"c6_{n}_stay")
    s.actors(s76=s76)
    s.end_scene()
    s.on_map(11, 1)
s.write("staging_ch6.txt")

s = Script("staging_ch568_gen.py")
for p1, p2, s166 in ((GOHAN, None, 0), (GOKU, None, 0), (TRUNKS, None, 0), (GOHAN, HERCULE, 1), (GOKU, VEGETA, 1), (TRUNKS, PICCOLO, 1), (PICCOLO, HERCULE, 1)):
    n = lineup_name(p1, p2)
    s.begin(f"cooler {n}", p1, p2, "+60")
    s.warp(6, 61, 200, 400, 260)
    s.end_scene()
    s.shot(f"c8_{n}_fight")
    s.actors(s166=s166)
    s.on_map(6, 61)
    s.win(240)
    s.end_scene()
    s.out("frames 30", "ents")
s.begin("cooler servant", GOHAN, HERCULE, "+60")
s.warp(6, 61, 200, 400, 260)
s.press(8, 100)
s.shot("c8_servant")
s.actors(s166=1)
s.end_scene()
s.write("staging_ch8.txt")
