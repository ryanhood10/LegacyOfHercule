r"""Write tools/headless/staging_ch9.txt, staging_broly.txt and staging_arctic.txt: the side chapters per lineup (ch9_gero, broly, arctic_summit).
usage: python tools/headless/staging_ch9_gen.py     (the scripts are committed; rerun this after changing them here)
Gero: the workshop's end (whoever nobody plays of Krillin and 16 leaves by the door), the bunker's end (the birds on foot, no flying pose on
their sprites; Hercule on the map at the end, alone he walks out, with two players player 2 is him). Broly: the rim (all walk in, Paragus to
his twin's spot), the retry (flag 403: straight to the fight), Vegeta's line at the end, the debrief, for any of the trip's five in either
hand. The Arctic: the watchers by lineup (player 2 as Super Saiyan 2 Gohan has no Gohan watcher beside him)."""
from staging_lib import *

GERO = ["poke32 FLAGS+0x00 0xFFFFFFFF", "poke32 FLAGS+0x04 0xFFFFFFFF", "poke32 FLAGS+0x08 0xFFFFFFFF", "poke32 FLAGS+0x0C 0xFFFFFFFF",
        "poke32 FLAGS+0x10 0xFFFFFFFF", "poke32 FLAGS+0x14 0xFFFFFFFF", "flags +420", f"poke32 {CMD} 11", "frames 30"]
s = Script("staging_ch9_gen.py")
for p1, p2, s76 in ((GOHAN, None, 0), (KRILLIN, None, 0), (GOHAN, HERCULE, 0), (HERCULE, KRILLIN, 0)):
    n = lineup_name(p1, p2)
    s.begin(f"workshop {n}", p1, p2, "+151 +155 +156 +157")
    s.warp(6, 5, 224, 300, 200)
    s.end_scene()
    s.out("frames 30")
    s.shot(f"c9_{n}_ws")
    s.actors(s76=s76)             # nobody left standing by the table
    s.on_map(6, 5)
for p1, p2, s72 in ((KRILLIN, A16, 1), (A16, KRILLIN, 1), (KRILLIN, None, 0), (A16, None, 0)):
    n = lineup_name(p1, p2)
    s.begin(f"bunker {n}", p1, p2, "+151 +155 +156 +157", GERO)
    s.warp(8, 63, 216, 300, 200)
    s.end_scene()
    s.win(240)
    s.press(12, 110)
    s.shot(f"c9_{n}_bird")
    s.out("ents")
    s.end_scene()
    s.out("frames 60")
    s.shot(f"c9_{n}_home")
    s.actors(s72=s72)
    s.on_map(6, 5)
s.write("staging_ch9.txt")

TRIP = "+400 +401 +378 +377"
s = Script("staging_ch9_gen.py")
for p1, p2 in ((VEGETA, TRUNKS), (TRUNKS, VEGETA), (VEGETA, None), (TRUNKS, None), (GOHAN, None), (KRILLIN, None), (PICCOLO, None), (GOHAN, KRILLIN), (PICCOLO, VEGETA)):
    n = lineup_name(p1, p2)
    s.begin(f"rim {n}", p1, p2, TRIP)
    s.warp(15, 3, 312, 460, 150)
    s.press(3, 100)
    s.shot(f"cb_{n}_rim")
    s.end_scene()
    s.shot(f"cb_{n}_fight")
    s.actors(s159=1, s170=1)      # the bosses
    s.on_map(15, 3)
    s.win(120)
    s.out("frames 200")
    s.shot(f"cb_{n}_veg")
    s.out("ents")
    s.end_scene(300)
    s.on_map(15, 9)
    s.begin(f"home {n}", p1, p2, TRIP + " +402")
    s.warp(15, 9, 120, 76, 60)
    s.press(10, 100)
    s.shot(f"cb_{n}_home")
    s.out("ents")
    s.end_scene()
    s.out("frames 60")
    s.on_map(6, 21)
s.begin("rim retry", VEGETA, TRUNKS, TRIP + " +403")
s.warp(15, 3, 312, 460, 150)
s.end_scene()
s.shot("cb_retry")
s.actors(s159=1, s170=1)
s.on_map(15, 3)
s.write("staging_broly.txt")

ARC = "+195 +191 +480 +381"
SS = ["poke16 0x0203F0EC 300", "poke16 0x0203F0EE 300", "poke16 0x0203F0F0 100", "poke16 0x0203F0F2 100", "poke8 0x0203F0F4 28", f"poke8 {flags_of(SS2)} 0x7F"]  # slot 8 as ssj2_join leaves it
s = Script("staging_ch9_gen.py")
for p1, p2, s56 in ((KRILLIN, SS2, 0), (SS2, KRILLIN, 0), (HERCULE, SS2, 0), (GOHAN, SS2, 0), (HERCULE, None, 1), (SS2, None, 0), (VEGETA, None, 1), (HERCULE, VEGETA, 1), (VEGETA, GOHAN, 0)):
    n = lineup_name(p1, p2)
    s.begin(f"summit {n}", p1, p2, ARC, SS, maptest=261)
    s.warp(15, 8, 336, 430, 200)
    s.actors(s56=s56)              # Gohan's watcher, never beside a played Gohan or Super Saiyan 2 Gohan
    s.press(4, 100)
    s.out("frames 60")
    s.shot(f"ca_{n}_watch")
    s.end_scene(120)
    s.shot(f"ca_{n}_fight")
    s.on_map(15, 8)
    s.win(240)
    s.out("frames 200")
    s.end_scene()
    s.out("frames 200")
    s.on_map(9, 1)
s.write("staging_arctic.txt")
