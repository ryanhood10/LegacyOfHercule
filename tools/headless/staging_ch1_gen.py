r"""Write tools/headless/staging_ch1.txt: chapter 1's added scenes, staged for each lineup (data/story/ch1_opening.lscript).
usage: python tools/headless/staging_ch1_gen.py     (the script is committed; rerun this after changing it here)
The forest spirit, Frieza's landing and the Goku-arrival talk: only who is played is there (Ryan, 2026-11-05): no actor of Gohan or Krillin in any lineup.
Krillin's visit, with the party as it really is then (only Gohan) so player 2's entity is born when Krillin joins: alone Krillin leaves by
the door, with two players he stands where player 2 appears; he does not blink out at "Krillin is now available!"."""
from staging_lib import *

s = Script("staging_ch1_gen.py")
# entrec counts a player on that record too: a played Gohan shows 53; a played Krillin shows 161. Only who is played is in the scene (Ryan,
# 2026-11-05): alone as Gohan no Krillin (76), alone as Krillin no Gohan (53)
for p1, p2, want in ((GOHAN, None, dict(s53=1, s76=0)), (KRILLIN, None, dict(s53=0, s76=0)),
                     (GOHAN, KRILLIN, dict(s53=1, s76=0)), (KRILLIN, GOHAN, dict(s53=1, s76=0))):
    n = lineup_name(p1, p2)
    s.begin(f"spirit {n}", p1, p2, "+4 +360")
    s.warp(2, 8, 96, 220, 100)
    s.actors(**want)
    s.press(2, 110)
    s.shot(f"c1_{n}_talk")
    s.press(6, 110)
    s.shot(f"c1_{n}_frieza")
    s.end_scene()
    s.shot(f"c1_{n}_fight")
    s.actors(**want)
    s.on_map(2, 8)
    s.win(300)
    s.press(3, 90)
    s.shot(f"c1_{n}_after")
    s.on_map(2, 3)
    s.actors(**(want if p2 is None else dict(s76=0)))  # player 2's entity is away until the scene's end
    s.end_scene()
    s.actors(**want)
    s.on_map(2, 3)

# Frieza's landing and the Goku-arrival site (zone 3 areas 4 and 6): no actor of Gohan (55) or Krillin (76) in any lineup
LEAD_GOHAN = ["poke8 FLAGS+0x3A 1"]          # flags 464-467: the story's lead is Gohan (slot + 1)
# The fight is one choreography for every lineup (no player's body is on the ship's map; the cutscene pass of 2026-11-06): shots of the
# Super Saiyan, the soldiers, old Hercule's punch, Frieza's blast into empty ground, the sword, King Cold's fall. Afterwards: flags 4 and 22
# and nothing else in the first flag word (as before the pass), old Hercule (158) and Trunks (128 / 134) gone with the scene.
for p1, p2 in ((GOHAN, None), (KRILLIN, None), (GOHAN, KRILLIN), (KRILLIN, GOHAN)):
    n = lineup_name(p1, p2)
    s.begin(f"landing {n}", p1, p2, "+4 +360", LEAD_GOHAN, maptest=52)
    s.warp(3, 4, 200, 120, 280)
    s.actors(s55=0, s76=0)
    s.shot(f"c1_{n}_land")
    s.press(45, 110)
    s.actors(s158=1, s128=1)        # Trunks has landed, old Hercule behind him
    s.shot(f"c1_{n}_faceoff")
    for i, k in enumerate((8, 2, 1, 2, 2, 1, 2, 2, 5)):
        s.press(k, 110)
        s.shot(f"c1_{n}_fight{i}")
    s.actors(s55=0, s76=0)
    s.press(60, 110)
    s.end_scene(200)
    s.shot(f"c1_{n}_landed")
    s.actors(s158=0, s128=0, s134=0, s49=0, s74=0)
    s.out("echo EXPECT [0-9a-f]+: 00400010$", "peek FLAGS 1")
    s.on_map(3, 4)
for p1 in (GOHAN, KRILLIN):
    n = lineup_name(p1, None)
    s.begin(f"arrival {n}", p1, None, "+4 +360 +21 +22", LEAD_GOHAN, maptest=54)
    s.warp(3, 6, 416, 456, 500)
    s.actors(s55=0, s76=0)
    s.out("keys 0x20", "frames 200", "keys 0", "key 0x01 4", "frames 300")
    s.shot(f"c1_{n}_arr0")
    for i in range(1, 6):
        s.out("key 1 6", "frames 140")
        s.shot(f"c1_{n}_arr{i}")
    s.actors(s55=0, s76=0)
    s.end_scene(200)
    s.on_map(2, 1)

only_gohan = [f"poke8 {flags_of(i)} 0" for i in range(1, 9)] + ["frames 30"]
s.begin("visit alone", GOHAN, None, "+4", only_gohan)
s.warp(2, 22, 136, 170, 150)
s.press(10, 90)
s.shot("c1_G_visit")
s.end_scene()
s.actors(s76=0)
s.on_map(2, 22)

s.begin("visit two players", GOHAN, None, "+4", only_gohan + ["p2 0"])
s.warp(2, 22, 136, 170, 150)
s.out("stepnz FLAGS+45 60", "frames 40")      # flag 360: he has joined
s.shot("c1_GK_avail")
s.actors(s76=1)                 # still there through "Krillin is now available!" (the actor), player 2 not yet
s.end_scene()
s.shot("c1_GK_end")
s.out("frames 60")
s.on_map(2, 22)
s.out("echo EXPECT 0203ff84: 06", "peek8 0x0203FF84 1")
s.write("staging_ch1.txt")
