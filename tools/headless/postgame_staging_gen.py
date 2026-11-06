r"""Write tools/headless/staging_postgame.txt: the post-game's scenes on the Lookout staged for every lineup (tools/gen_postgame.py;
docs/cutscenes/batch3_games-postgame.md 5.5, 5.7; docs/testing.md "Post-game staging"). Run by regfull.sh through the staging_*.txt glob, or
    python tools/headless/postgame_staging_gen.py               write the script (committed; rerun this after changing it here)
    python tools/headless/staging_run.py tools/headless/staging_postgame.txt ROM build/<dir>/hl/menu.ss --out build/<dir>/hl --cols 6

The finale (the arrival below Dende with Super 13 beaten): who stands by who plays. The map's four party actors (Piccolo 103, Vegeta 137,
Trunks 132, Krillin 76) stand unless player 1 plays that character, who then stands on the actor's spot (two Piccolos stood there; a
played Piccolo and Vegeta show the actor's own record, so their count stays 1 and a double reads 2); Gohan, whose scene this is, is an
actor (56; 58 when player 2 is Super Saiyan 2 Gohan) unless player 1 plays a Gohan; player 2 as Goku (176), Hercule (72) or the other
Gohan stands as an actor beside the group. Nothing changes who plays whom; the only flag set is 483.
The talk show (Dende's question answered yes twice): Hercule is its guest whoever player 1 was (player 1's body walked and posed under
"HERCULE: OH YEAH!!!!!")."""
from staging_lib import *

S = Script("postgame_staging_gen.py")
SS2_POKES = ("poke16 0x0203F0EC 500", "poke16 0x0203F0EE 500", "poke16 0x0203F0F0 500", "poke16 0x0203F0F2 500", "poke8 0x0203F0F4 28", "poke8 0x0203F100 0x7F")
GOHANS = (GOHAN, SS2)


def finale(p1, p2):
    tag = "fin_" + lineup_name(p1, p2)
    S.begin(tag, p1, p2, preset="+195 +191 +480 +381 +382", pokes=SS2_POKES if SS2 in (p1, p2) else ())
    S.warp(9, 1, 760, 448, frames=330)
    S.on_map(9, 7)
    if p1 in GOHANS:
        g56 = 1 if (p1 == SS2 and p2 == GOHAN) else 0
    else:
        g56 = 0 if p2 == SS2 else 1
    S.actors(s103=1, s137=1, s132=0 if p1 == TRUNKS else 1, s76=0 if p1 == KRILLIN else 1, s56=g56,
             s58=(p1 == SS2) + (p2 == SS2), s72=1 if HERCULE in (p1, p2) else 0, s176=1 if p2 == GOKU else 0,
             s121=1, s156=1, s36=1, s93=1, s11=0)
    S.shot(f"pg_{tag}_a")
    S.out("key 1 6", "frames 100")
    S.shot(f"pg_{tag}_b")       # the opening line: player 1's own portrait, or Piccolo's own line when player 1 is Piccolo
    S.end_scene(60)
    S.on_map(9, 1)
    S.out("echo EXPECT [0-9a-f]+: 09", "peek8 FLAGS+60 1")                      # 480 and 483, nothing else in that byte
    S.out(f"echo EXPECT 0203f000: {p1:02x}", "peek8 0x0203F000 1")              # player 1 as he was
    if p2 is not None:
        S.out(f"echo EXPECT 0203ff84: {p2:02x}", "peek8 0x0203FF84 1")


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


for p1, p2 in ((GOHAN, None), (PICCOLO, None), (VEGETA, None), (TRUNKS, None), (GOKU, None), (HERCULE, None), (KRILLIN, None), (SS2, None),
               (PICCOLO, HERCULE), (VEGETA, HERCULE), (GOHAN, SS2), (SS2, GOHAN), (GOKU, HERCULE), (HERCULE, GOKU), (TRUNKS, VEGETA),
               (KRILLIN, PICCOLO), (PICCOLO, GOHAN), (GOHAN, GOKU)):
    finale(p1, p2)
for p1, p2 in ((PICCOLO, HERCULE), (KRILLIN, None), (HERCULE, None), (VEGETA, GOHAN)):
    talkshow(p1, p2)
S.write("staging_postgame.txt")
