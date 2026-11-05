r"""Write tools/headless/staging_ch1.txt: chapter 1's added scenes, staged for each lineup (data/story/ch1_opening.lscript).
usage: python tools/headless/staging_ch1_gen.py     (the script is committed; rerun this after changing it here)
The forest spirit: alone as Gohan (Krillin's actor, 76), alone as Krillin (Gohan's, 53), both players either way round (no actor).
Krillin's visit, with the party as it really is then (only Gohan) so player 2's entity is born when Krillin joins: alone Krillin leaves by
the door, with two players he stands where player 2 appears; he does not blink out at "Krillin is now available!"."""
from staging_lib import *

s = Script("staging_ch1_gen.py")
# entrec counts a player on that record too: a played Gohan shows 53; a played Krillin shows 161
for p1, p2, want, after in ((GOHAN, None, dict(s53=1, s76=1), dict(s53=1, s76=0)), (KRILLIN, None, dict(s53=1, s76=0), dict(s53=0, s76=0)),
                            (GOHAN, KRILLIN, dict(s53=1, s76=0), dict(s53=1, s76=0)), (KRILLIN, GOHAN, dict(s53=1, s76=0), dict(s53=1, s76=0))):
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
    s.actors(**(want if p1 == GOHAN or p2 is None else dict(s76=0)))  # player 2's entity is away until the scene's end
    s.end_scene()
    s.actors(**after)           # the actor ran on ahead
    s.on_map(2, 3)

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
