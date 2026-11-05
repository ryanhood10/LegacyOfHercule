r"""Write tools/headless/staging_ch2.txt and staging_ch3.txt: chapters 2 and 3's added scenes per lineup (ch2_west_city, ch3_zone7, ch3_duels).
usage: python tools/headless/staging_ch23_gen.py     (the scripts are committed; rerun this after changing them here)
Chapter 2: the mansion deal alone as Gohan / Krillin and with both; the jungle's end alone as Piccolo (Hercule's actor comes out to take the
credit), alone as Hercule (Piccolo's actor answers) and with both. Chapter 3: zone 7's cutaway; 19's battlefield alone as Vegeta (Hercule's
actor for "clown", gone before the fight), alone as Hercule (made Vegeta), both ways round with two players; the landing alone and with
player 2's entity handed over from Hercule's actor."""
from staging_lib import *

s = Script("staging_ch23_gen.py")
for p1, p2 in ((GOHAN, None), (KRILLIN, None), (GOHAN, KRILLIN), (KRILLIN, GOHAN)):
    n = lineup_name(p1, p2)
    s.begin(f"deal {n}", p1, p2, "+34 +43")
    s.warp(4, 3, 280, 984, 200)
    s.out("key 0x40 4", "frames 10")
    s.press(9, 100)
    s.shot(f"c2_{n}_deal")
    s.end_scene()
    s.on_map(4, 3)
for p1, p2 in ((PICCOLO, None), (HERCULE, None), (PICCOLO, HERCULE), (HERCULE, PICCOLO)):
    n = lineup_name(p1, p2)
    s.begin(f"jungle {n}", p1, p2)
    s.warp(4, 58, 112, 420, 100)
    s.end_scene()
    s.win(200)
    s.press(9, 90)
    s.out("frames 100")
    s.shot(f"c2_{n}_end")
    s.out("ents")
    s.end_scene()
    s.out("frames 30")
    s.on_map(4, 56)
s.write("staging_ch2.txt")

s = Script("staging_ch23_gen.py")
s.begin("cutaway", VEGETA, None)
s.warp(7, 1, 200, 310, 200)
s.press(16, 100)
s.shot("c3_cut")
s.end_scene()
s.on_map(7, 1)
for p1, p2, s72 in ((VEGETA, None, 0), (HERCULE, None, 0), (VEGETA, HERCULE, 1), (HERCULE, VEGETA, 1)):
    n = lineup_name(p1, p2)
    s.begin(f"battle {n}", p1, p2, "+76")
    s.warp(7, 15, 424, 416, 300)
    s.end_scene(200)
    s.shot(f"c3_{n}_fight")
    s.actors(s72=s72, s165=1)     # Hercule's actor gone before the fight (player 2 only); 13's actor replaced by his twin
    s.on_map(7, 15)
    s.win(200)
    s.press(5, 100)
    s.shot(f"c3_{n}_13")
    s.out("ents")
    s.end_scene()
    s.out("frames 60")
    s.on_map(7, 16)
for p1, p2, s72 in ((VEGETA, None, 0), (VEGETA, TRUNKS, 1)):
    n = lineup_name(p1, p2)
    s.begin(f"landing {n}", p1, p2)
    s.warp(6, 22, 240, 312, 100)
    s.press(5, 110)
    s.shot(f"c3_{n}_land")
    s.end_scene()
    s.out("frames 60")
    s.shot(f"c3_{n}_landed")
    s.actors(s72=s72)              # alone he walked off; with two players player 2 is him now
    s.on_map(6, 22)
s.write("staging_ch3.txt")
