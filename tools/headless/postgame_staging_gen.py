r"""Write tools/headless/staging_postgame.txt: the post-game's scenes on the Lookout staged for every lineup (tools/gen_postgame.py;
docs/cutscenes/batch3_games-postgame.md 5.7; docs/testing.md "Post-game staging"). Run by regfull.sh through the staging_*.txt glob, or
    python tools/headless/postgame_staging_gen.py               write the script (committed; rerun this after changing it here)
    python tools/headless/staging_run.py tools/headless/staging_postgame.txt ROM build/<dir>/hl/menu.ss --out build/<dir>/hl --cols 6

The talk show (Dende's question answered yes twice): Hercule is its guest whoever player 1 was (player 1's body walked and posed under
"HERCULE: OH YEAH!!!!!")."""
from staging_lib import *

S = Script("postgame_staging_gen.py")


def talkshow(p1, p2):
    tag = "ts_" + lineup_name(p1, p2)
    S.begin(tag, p1, p2, preset="+195 +191 +480 +381 +382 +483", maptest=261)
    S.warp(9, 1, 760, 448, frames=200)
    S.out("key 0x40 12", "frames 30", "key 1 6", "frames 120", "key 1 6", "frames 120", "key 1 6", "frames 150", "key 1 6", "frames 60")
    S.out("waitmap 4 20 900", "frames 420")
    S.on_map(4, 20)
    S.out("echo EXPECT 0203f000: 05", "peek8 0x0203F000 1")                     # Hercule is the guest
    S.actors(s72=1, s103=0, s137=0)                                             # one Hercule (the player), nobody else's body
    S.out("mash 3 110")
    for i in range(4):          # "HERCULE: OH YEAH!!!!!" beside Hercule's body and portrait, his walk and poses
        S.out("key 1 6", "frames 110")
        S.shot(f"pg_{tag}_{i}")
    S.out("ents")
    S.out("echo EXPECT 0203f000: 05", "peek8 0x0203F000 1")


for p1, p2 in ((PICCOLO, HERCULE), (KRILLIN, None), (HERCULE, None), (VEGETA, GOHAN)):
    talkshow(p1, p2)
S.write("staging_postgame.txt")
