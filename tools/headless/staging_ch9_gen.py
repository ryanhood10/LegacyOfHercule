r"""Write tools/headless/staging_ch9.txt, staging_broly.txt and staging_arctic.txt: the side chapters per lineup (ch9_gero, broly, arctic_summit).
usage: python tools/headless/staging_ch9_gen.py     (the scripts are committed; rerun this after changing them here)
Gero: the workshop's end (nobody left by the table), the bunker's end (the birds on foot, no flying pose on their sprites; alone no Krillin
beside 16 and no Hercule at home; with two players player 2 is Hercule). Only who is played is in a scene (Ryan, 2026-11-05). Broly: the rim (all walk in, Paragus to
his twin's spot), the retry (flag 403: straight to the fight), Vegeta's line at the end, the debrief, for any of the trip's five in either
hand: only who is played (the others' actors never: 132, 56, 103, 76; Vegeta's 137 only at the rim when nobody plays him). The Arctic:
no party watcher in any lineup (Tien and Yamcha stay)."""
from staging_lib import *

GERO = ["poke32 FLAGS+0x00 0xFFFFFFFF", "poke32 FLAGS+0x04 0xFFFFFFFF", "poke32 FLAGS+0x08 0xFFFFFFFF", "poke32 FLAGS+0x0C 0xFFFFFFFF",
        "poke32 FLAGS+0x10 0xFFFFFFFF", "poke32 FLAGS+0x14 0xFFFFFFFF", "flags +420", f"poke32 {CMD} 11", "frames 30"]
s = Script("staging_ch9_gen.py")
for p1, p2, s76 in ((GOHAN, None, 0), (KRILLIN, None, 0), (GOHAN, HERCULE, 0), (HERCULE, KRILLIN, 0)):
    n = lineup_name(p1, p2)
    s.begin(f"workshop {n}", p1, p2, "+151 +155 +156 +157")
    s.warp(6, 5, 224, 300, 200)
    s.actors(s76=int(p2 is not None and KRILLIN not in (p1, p2)))   # Krillin walks in only with two players (he is player 1 after it)
    s.press(9, 110)
    s.shot(f"c9_{n}_ws_line")
    s.end_scene()
    s.out("frames 30")
    s.shot(f"c9_{n}_ws")
    s.actors(s76=s76)             # nobody left standing by the table
    s.on_map(6, 5)
for p1, p2, s72 in ((KRILLIN, A16, 1), (A16, KRILLIN, 1), (KRILLIN, None, 0), (A16, None, 0)):
    n = lineup_name(p1, p2)
    s.begin(f"bunker {n}", p1, p2, "+151 +155 +156 +157", GERO + [f"poke8 0x0203F000 {p1}"] + ([] if p2 is None else [f"poke8 0x0203FF84 {p2}"]))   # test command 11 makes Krillin player 1: the lineup again
    s.warp(8, 63, 216, 300, 200)
    s.actors(s76=0, s8=0)          # nobody comes in behind as an actor
    # the bunker's androids may land a hit soon after the intro: a pick-up box can open within the check's 30 frames,
    # earlier or later with any change to the frame's timing (docs/testing.md); step through it before the check
    s.out("stepz 0x03001FDC 150", "frames 60")
    s.end_scene()
    s.win(240)
    s.out("mashmap 8 48 40", "frames 150")                         # through the lines to the warp outside; before the first line there (a text box on
    s.shot(f"c9_{n}_bird")                                          # this full map takes the actors and birds off the list while it is open: docs/scripts.md)
    s.actors(s76=int(KRILLIN in (p1, p2) and p1 != KRILLIN), s8=int(p1 != A16))   # Krillin beside 16 only as player 2's stand-in (player 2 has no entity
    s.out("frames 270")                                                             # there); 16 an actor unless player 1 is 16 (alone as Krillin: kept for the plot)
    s.shot(f"c9_{n}_bird2")
    s.press(8, 110)
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
    s.actors(s132="[01]" if TRUNKS in (p1, p2) else 0, s56="[01]" if GOHAN in (p1, p2) else 0, s103="[01]" if PICCOLO in (p1, p2) else 0, s76=0)
    s.shot(f"cb_{n}_rim")
    s.press(2, 100)
    s.out("frames 150")
    s.shot(f"cb_{n}_power")       # the camera has gone east to Broly: his power-up
    s.out("ents")
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
    s.press(3, 100)
    s.actors(s132="[01]" if TRUNKS in (p1, p2) else 0, s56="[01]" if GOHAN in (p1, p2) else 0, s103="[01]" if PICCOLO in (p1, p2) else 0, s76=0, s137="[01]" if VEGETA in (p1, p2) else 0)
    s.press(7, 100)
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
    s.actors(s56="[01]" if GOHAN in (p1, p2) else 0, s72="[01]" if HERCULE in (p1, p2) else 0, s137="[01]" if VEGETA in (p1, p2) else 0, s103=0, s76=0, s132=0, s121=1)   # no party watcher; Tien stays
    s.press(4, 100)
    s.out("frames 60")
    s.shot(f"ca_{n}_watch")
    # the players are in the picture (docs/cutscenes/batch3 6.2): player 1 walked up to the view's bottom edge, player 2 beside him
    s.out("echo EXPECT map zone 15 area 8 var [0-9]+ player 324,372", "map")
    if p2 is not None:
        s.out("deref P2E 0x0203FF08", f"echo EXPECT [0-9a-f]+: {350 << 8:08x} {372 << 8:08x}", "peek P2E+0x158 2")
    s.out("echo EXPECT stepz 03001fdc: zero", f"stepz {SCENE} 120")     # the scene ends (not a peek after it: the fight starts at once and a hit blips the flag)
    s.shot(f"ca_{n}_fight")
    s.on_map(15, 8)
    s.actors(s180=1, s165=0, s9=0, s11=0)       # the boss; 13, 17 and 18 are gone
    if (p1, p2) in ((KRILLIN, SS2), (HERCULE, None)):
        # a retry in the same session (a loss, a reload at the ridge pad) plays the short form (docs/cutscenes/batch3 6.2): he stands transformed at once
        s.warp(15, 8, 336, 430, 200)
        s.actors(s180=1, s165=0, s9=0, s11=0)
        s.shot(f"ca_{n}_retry")
        s.out("echo EXPECT stepz 03001fdc: zero", f"stepz {SCENE} 12")       # his last line and the players' lines only
        s.on_map(15, 8)
    s.win(240)
    s.out("frames 200")
    s.end_scene()
    s.out("frames 200")
    s.on_map(9, 1)
s.write("staging_arctic.txt")
