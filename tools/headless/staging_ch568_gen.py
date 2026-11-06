r"""Write tools/headless/staging_ch5.txt, staging_ch6.txt and staging_ch8.txt: chapters 5, 6 and Cooler per lineup (ch5_lookout, ch6_basement, ch8_cooler).
usage: python tools/headless/staging_ch568_gen.py     (the scripts are committed; rerun this after changing them here)
Chapter 5: Kami defers, whoever is selectable (the fade makes Piccolo player 1; Hercule only when played), then Kami's boost and
the fusion (Hercule kept there for the plot: Kami draws out his potential; flags 129 and 131 so Kami stands; he is gone after it and Mr. Popo is not). Chapter 6: the basement and Goku's waking, only who is played: no
Krillin NPC or actor alone as Trunks, no Trunks actor alone as Krillin (Ryan, 2026-11-05). Cooler: the three openings alone (no servant) and with
two players (the servant's actor, then his twin on the same sprite 166)."""
from staging_lib import *

s = Script("staging_ch568_gen.py")
for p1, p2 in ((PICCOLO, None), (HERCULE, None), (KRILLIN, None), (GOHAN, None), (PICCOLO, HERCULE), (HERCULE, PICCOLO), (PICCOLO, KRILLIN), (KRILLIN, PICCOLO)):
    n = lineup_name(p1, p2)
    s.begin(f"lookout {n}", p1, p2)
    s.warp(9, 6, 112, 224, 300)
    s.press(26, 110)
    s.shot(f"c5_{n}_stay")
    s.actors(s72=int(HERCULE in (p1, p2) and p2 is not None))   # alone the fade makes Piccolo player 1: Hercule is not there
    s.out("ents")
    s.end_scene(200)
    s.on_map(2, 20)
for p1, p2 in ((TRUNKS, None), (GOHAN, None), (TRUNKS, GOHAN), (GOHAN, TRUNKS)):
    n = lineup_name(p1, p2)
    s.begin(f"fusion {n}", p1, p2, "+129 +131")     # 129: Kami's record stands on the Lookout (it did not without it: batch 2's finding)
    s.warp(10, 16, 96, 150, 100)
    s.press(56, 80)
    s.shot(f"c5_{n}_kami")
    s.actors(s73=1, s72=1, s93=1)                   # Kami, Hercule's actor (kept for the plot), Mr. Popo
    s.out("ents")
    s.press(16, 80)                                 # the boost, "Our union...", Popo's goodbye: into the fusion
    for i in range(5):
        s.out("frames 80")
        s.shot(f"c5_{n}_fuse{i}")
    s.actors(s73=0, s72=1, s93=1)                   # Kami has gone into Piccolo; Popo watched from the stair head
    s.out("ents")
    s.end_scene(300)
    s.on_map(6, 99)
s.write("staging_ch5.txt")

s = Script("staging_ch568_gen.py")
for p1, p2 in ((TRUNKS, None), (KRILLIN, None), (TRUNKS, KRILLIN), (KRILLIN, TRUNKS)):
    n = lineup_name(p1, p2)
    t128 = 1 if p1 == TRUNKS else "[01]" if p2 == TRUNKS else 0   # a played Trunks shows 128 himself (player 2's entity may be away after a warp)
    for tag, area, x, y, shots in (("larva", 62, 128, 240, 4), ("computer", 63, 200, 320, 6)):
        s.begin(f"{tag} {n}", p1, p2, "+136 +466")
        s.warp(8, area, x, y, 150)
        s.actors(s76=0, s128=t128)
        for i in range(shots):
            s.press(1, 120)
            s.shot(f"c6_{n}_{tag}{i}")
        s.actors(s76=0, s128=t128)
        s.end_scene(150)
        s.actors(s76=0, s128=t128)
for p1, p2, s76 in ((TRUNKS, None, 0), (KRILLIN, None, 0), (TRUNKS, KRILLIN, 0), (KRILLIN, TRUNKS, 0)):
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
