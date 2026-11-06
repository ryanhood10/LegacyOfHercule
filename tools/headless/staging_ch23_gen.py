r"""Write tools/headless/staging_ch2.txt and staging_ch3.txt: chapters 2 and 3's added scenes per lineup (ch2_west_city, ch3_zone7, ch3_duels).
usage: python tools/headless/staging_ch23_gen.py     (the scripts are committed; rerun this after changing them here)
Chapter 2: the mansion deal alone as Gohan / Krillin and with both; the jungle's end alone as Piccolo, alone as Hercule and with both.
Chapter 3: zone 7's cutaway; 19's battlefield alone as Vegeta, alone as Hercule (made Vegeta), both ways round with two players; the landing
alone (no Hercule: he only becomes selectable) and with player 2's entity handed over from Hercule's actor; space, alone and to the duel. Only who is played is in a scene
(Ryan, 2026-11-05): Piccolo's actor 103 never, Hercule's sprite 72 only on a player (or his hand-over actor with two players at the landing)."""
from staging_lib import *

s = Script("staging_ch23_gen.py")
for p1, p2 in ((GOHAN, None), (KRILLIN, None), (GOHAN, KRILLIN), (KRILLIN, GOHAN)):
    n = lineup_name(p1, p2)
    s.begin(f"deal {n}", p1, p2, "+34 +43")
    s.warp(4, 3, 280, 984, 200)
    s.out("key 0x40 4", "frames 10")
    s.press(9, 100)
    s.shot(f"c2_{n}_deal")
    # (batch 1, 2.2) the view is up at (168, 860) through the talk, so the top boxes leave Piccolo and Hercule in the clear
    s.out("echo EXPECT 030010ac: 000000a8 0000035c", "peek 0x030010AC 2")
    s.end_scene()
    s.on_map(4, 3)
for p1, p2 in ((PICCOLO, None), (HERCULE, None), (PICCOLO, HERCULE), (HERCULE, PICCOLO)):
    n = lineup_name(p1, p2)
    s.begin(f"jungle {n}", p1, p2)
    s.warp(4, 58, 112, 420, 100)
    s.out("frames 230")             # (batch 1, 2.3) the King stamps as the camera reaches him, before his first word; the HUD hidden
    s.shot(f"c2_{n}_stamp")
    s.end_scene()
    s.win(200)
    s.press(9, 90)
    s.out("frames 100")
    s.shot(f"c2_{n}_end")
    s.actors(s103=int(PICCOLO in (p1, p2)), s72=int(HERCULE in (p1, p2)))
    s.out("ents")
    s.end_scene()
    s.out("frames 30")
    s.on_map(4, 56)
s.write("staging_ch2.txt")

s = Script("staging_ch23_gen.py")
# Amenbo Island (the cutscene pass, 2026-11-06): 20's power and his volley at the city in place of one blast, the white cut to the burning street
# (zone 5 area 4), Goku's flare before "You monster!!!"; alone as Gohan and with Piccolo and Hercule
for p1, p2 in ((GOHAN, None), (PICCOLO, HERCULE)):
    n = lineup_name(p1, p2)
    s.begin(f"amenbo {n}", p1, p2)
    s.warp(5, 3, 144, 300, 60)
    s.press(19, 112)
    for i in range(4):
        s.out("key 1 6", "frames 56")
        s.shot(f"c3_{n}_amenbo{i}a")
        s.out("frames 56")
        s.shot(f"c3_{n}_amenbo{i}b")
    s.out("ents")
    s.on_map(5, 4)
s.begin("cutaway", VEGETA, None)
s.warp(7, 1, 200, 310, 200)
s.press(16, 100)
for i in range(3):                  # Goku's power before the gold: his stock power-up (animation 9 on sprite 60) with Cine's charge held alongside (nb-ssjfix), then 61 stands
    s.out("key 1 6", "frames 50")
    s.shot(f"c3_cut_power{i}a")
    s.out("frames 50")
    s.shot(f"c3_cut_power{i}b")
s.out("frames 160")                 # (the stock power-up runs its own length, about 140 frames)
s.actors(s61=1, s60=0)
s.out("ents")
s.shot("c3_cut")
s.end_scene()
s.on_map(7, 1)
for p1, p2, s72 in ((VEGETA, None, 0), (HERCULE, None, 0), (VEGETA, HERCULE, 1), (HERCULE, VEGETA, 1)):
    n = lineup_name(p1, p2)
    # the cutscene pass (2026-11-06): the players stand at the area's own entry point, below the view (the scene's trigger is the whole
    # map), so Vegeta comes into the fight out of nowhere: shots of 19's hand over Goku, the kick, 19 thrown; the story's lead is poked to
    # Vegeta (flags 464-467), so speaker 1 shows his portrait as it does in play
    s.begin(f"battle {n}", p1, p2, "+76", ["poke8 FLAGS+0x3A 3"])
    s.warp(7, 15, 468, 556, 300)
    s.press(4, 110)                # (3.5 beats 1-2) the opening as an exchange and Goku's heavy blow; later his knee before the collapse
    s.shot(f"c3_{n}_open")
    s.press(21, 110)
    s.shot(f"c3_{n}_drain")
    s.press(4, 110)
    s.out("key 1 6", "frames 44")
    s.shot(f"c3_{n}_kick")
    s.out("frames 24")
    s.shot(f"c3_{n}_thrown")
    s.out("frames 100")
    s.shot(f"c3_{n}_mark")
    s.press(1, 110)
    s.actors(s72=s72)              # no Hercule actor for "clown" alone (Ryan, 2026-11-05)
    s.press(14, 110)
    for i in range(5):             # the debut: he transforms for real (animation 35, the stock's power-up with Cine's charge alongside), the line
        s.press(1, 110)
        s.shot(f"c3_{n}_debut{i}")
    s.end_scene(200)
    s.shot(f"c3_{n}_fight")
    s.actors(s72=s72, s165=1, s140=1)     # Hercule's actor gone before the fight (player 2 only); 13's actor replaced by his twin; 140 is Vegeta himself
    s.out("echo EXPECT 0203f000: 00000602", "peek 0x0203F000 1")      # Vegeta plays
    # he starts the fight a Super Saiyan (Ryan, 2026-11-06): his row is 11 (sprite 140, the form's flag; the entry's +8), the meter was filled by the transformation
    s.out("echo EXPECT 0203f04c: 0b", "peek8 0x0203F04C 1", "echo EXPECT 0203f008: 00000[1-5][0-9a-f]{2}", "peek 0x0203F008 1")
    s.out("echo EXPECT map zone 7 area 15 var 1 player 4[0-9][0-9],4[0-9][0-9] ", "map")
    s.out("frames 300", "echo EXPECT 0203f04c: 0b", "peek8 0x0203F04C 1")   # still a Super Saiyan 5 s into the fight (the drain: 1 EP every 20 frames)
    s.on_map(7, 15)
    s.out(f"poke32 {CMD} 7")       # 19's end: he reels and runs, Vegeta's palm, the big blast, the stock explosions
    for i in range(6):
        s.out("frames 50")
        s.shot(f"c3_{n}_end{i}")
    s.out("frames 200")
    s.actors(s12=0)
    s.press(5, 100)
    s.actors(s72=s72)
    s.shot(f"c3_{n}_13")
    s.out("ents")
    s.end_scene()
    s.out("frames 60")
    s.on_map(7, 16)
for p1, p2, s72 in ((VEGETA, None, 0), (VEGETA, TRUNKS, 1)):
    n = lineup_name(p1, p2)
    s.begin(f"landing {n}", p1, p2)
    s.warp(6, 22, 240, 312, 100)
    if p2 is None:
        s.press(1, 110)
        s.actors(s72=0)
    s.press(5, 110)
    s.actors(s72=s72)              # alone Hercule is not in the scene; with two players his actor walks up to hand over to player 2
    s.shot(f"c3_{n}_land")
    s.end_scene()
    s.out("frames 60")
    s.shot(f"c3_{n}_landed")
    s.actors(s72=s72)              # alone he walked off; with two players player 2 is him now
    s.on_map(6, 22)
# space, Vegeta and his clone (the cutscene pass, 2026-11-06: the blows played by the cinematic helpers): alone to the landing (the fight's end shown,
# no clone left, Vegeta back in base form on sprite 137); with two players to the duel's start (duel_active, coop2 + 0x25C)
for two in (False, True):
    n = "2p" if two else "1p"
    # as in play, nobody else is selectable here (Amenbo's "Meanwhile..." locks them): player 2 has no body until the duel makes the clone his
    s.begin(f"space {n}", VEGETA, None, "", ["poke8 FLAGS+0x3A 3"] + [f"poke8 {flags_of(i)} 0" for i in (0, 1, 3, 4, 5, 6, 8)] + (["p2 0"] if two else []) + ["frames 30"])
    s.warp(6, 50, 248, 560, 120)
    for i, k in enumerate((1, 2, 2, 2, 3, 3, 3)):
        s.out(f"mash {k} 160", "frames 60")
        s.shot(f"c3_space{n}_{i}")
    s.out("ents")
    if two:
        s.out("stepnz 0x0203FD5C 60", "frames 60", "echo EXPECT 0203fd5c: 01", "peek8 0x0203FD5C 1")
        s.shot(f"c3_space{n}_duel")
        s.out("ents")
        s.on_map(6, 50)
    else:
        for i in range(4):
            s.out("key 1 6", "frames 60")
            s.shot(f"c3_space{n}_end{i}")
        s.end_scene()
        s.out("frames 60")
        s.actors(s140=0, s137=1)
        s.on_map(6, 22)
s.write("staging_ch3.txt")
