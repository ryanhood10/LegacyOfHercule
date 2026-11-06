r"""Writes tools/headless/cine.txt and tools/headless/cine_demo.txt: the cinematic helpers' tests (docs/testing.md
"Cinematic helpers"; src/coop/cine.c, CoopCall 37, test command 50).
usage: python tools/headless/cine_gen.py

cine.txt (self-checking, through staging_run.py: EXPECT lines, the `ents` lists' vtables; in regfull.sh): every blocking
op for several pairs of sprites (a player-layout body, an enemy set, a 64 x 64 body, one with five groups, one with a
single group, a bird), with a missing body, an id past the sprite index, the point as the target, player 1 and player 2
as either party, a party-slot code, a transformed player 1 (and a blow that knocks him and an actor out of the form), a skip with Start, the looks, the camera, the held charge,
and the demo scene in both modes. After each op: it ended the way expected, every op started has ended, and at the end
of a block nobody is left held, no hum plays, OBJ VRAM's largest free block is what it was.
cine_demo.txt: the demo scene filmed (a shot every 6 frames):
  python tools/headless/st_test.py tools/headless/cine_demo.txt ROM build/<dir>/hl/menu.ss --out build/<dir>/hl/cine --cols 8
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
CMD, ARG, ARG2 = 0x0203FD08, 0x0203FD0C, 0x0203FD54
SCENE = 0x03001FDC
ARENA = (15, 3)                     # New Planet Vegeta (data/maps/broly_arena.json): the basin
SPOT = (330, 340)
# cine.h's ops
END, CAST, POINT, DASH_TO, DASH_BY, STRIKE, EXCHANGE, CHARGE, BLAST, STRUGGLE, POSE, LOOK, LOOKAT, CAM, PAN, JOLT, HAS, HELD = range(18)
P1, P2, THE_POINT, NOBODY = 0, -1, -2, -3
PLAYED, SKIPPED, QUIET = 1, 2, 5    # cine.c END_*


class Script:
    def __init__(self):
        self.out = []

    def w(self, s):
        self.out.append(s)

    def cmd(self, n, arg=0, arg2=0, wait=3):
        self.w(f"poke32 {ARG:#x} {arg & 0xFFFFFFFF:#x}")
        self.w(f"poke32 {ARG2:#x} {arg2 & 0xFFFFFFFF:#x}")
        self.w(f"poke32 {CMD:#x} {n}")
        if wait:
            self.w(f"frames {wait}")

    def cine(self, op, b=0, c=0, wait=3):
        self.cmd(50, op, (b & 0xFFFF) | ((c & 0xFFFF) << 16), wait)

    def actor(self, sprite, dx, dy):
        self.cmd(50, 102, sprite | ((dx & 0xFF) << 16) | ((dy & 0xFF) << 24), 3)

    def mark(self):
        """the actors stand: OBJ VRAM's largest free block now is what clean() wants back"""
        self.w("frames 6")
        self.cmd(50, 103)

    def skip(self):
        """player 2's Start (player 1's would open the pause menu once the op is over: no scene runs in these tests)"""
        self.w("p2 8")
        self.w("frames 3")
        self.w("p2 0")
        self.w("frames 6")

    def expect(self, regex, line):
        self.w(f"echo EXPECT {regex}")
        self.w(line)

    def ended(self, op, how):
        """the last op and how it ended (CineRam +0x0C: busy, cam, armed, last_op; +0x10: last_end, aura_on, ...), and
        every op started has ended (+0x18: ops, done)"""
        self.expect(rf"[0-9a-f]{{8}}: {op:02x}[0-9a-f]{{4}}00 [0-9a-f]{{6}}{how:02x}", "peek CN+0x0C 2")
        self.expect(r"[0-9a-f]{8}: ([0-9a-f]{4})\1", "peek CN+0x18 1")

    def op(self, op, b=0, c=0, frames=120, how=PLAYED):
        self.cine(op, b, c, frames)
        self.ended(op, how)

    def answer(self, op, b, value):
        self.cine(op, b, 0, 4)
        self.expect(rf"[0-9a-f]{{8}}: {value:08x}", f"peek {ARG:#x} 1")

    def clean(self, held=0, hums=0):
        """nobody held, no hum of ours, OBJ VRAM as it was marked"""
        self.answer(HELD, 0, held)
        self.cmd(46, 0, 0, 3)
        self.expect(rf"[0-9a-f]{{8}}: {hums:08x}", f"peek {ARG:#x} 1")
        if not held:
            self.w("frames 70")                     # (the last effects play out)
            self.cmd(50, 104, 0, 3)
            self.expect(r"[0-9a-f]{8}: 00000001", f"peek {ARG:#x} 1")

    def setup(self, name, two, pokes=()):
        self.w(f"echo == setup {name}")
        self.w("load menu.ss")
        self.w("p2 0" if two else "p2 off")
        self.w("maptest 7")
        self.w("frames 200")
        for p in pokes:
            self.w(p)
        self.cmd(5, ARENA[0] | ARENA[1] << 8, SPOT[0] | SPOT[1] << 16, 200)
        self.expect(rf"map zone {ARENA[0]} area {ARENA[1]}\b", "map")
        self.cine(CAM, 0, 0, 4)                     # (the state's magic)
        self.w("find CN 0xC14E0001")
        self.cmd(50, 101)
        self.cmd(50, 103)
        self.w(f"save {name}.ss")

    def save(self, path):
        open(os.path.join(HERE, path), "w", newline="\n").write("\n".join(self.out) + "\n")


def every_op(s, a, b):
    """every blocking op once for the cast (a, b), each ending played"""
    s.cine(CAST, a, b)
    s.op(DASH_BY, 0, 0, 30)
    s.op(STRIKE, 0, 0, 60)
    s.op(STRIKE, 2, 0, 60)
    s.op(STRIKE, 1, 0, 110)                         # heavy: b lies
    s.answer(HELD, 0, 1)
    s.op(POSE, 1, 0, 40)                            # b rises
    s.op(STRIKE, 5, 40, 110)                        # heavy: b ends on a knee
    s.op(POSE, 1, 0, 40)
    s.op(DASH_BY, 5, 0, 30)
    s.op(DASH_TO, SPOT[0] - 60, SPOT[1] + 30, 30)
    s.op(EXCHANGE, 70, 0, 150)
    s.op(EXCHANGE, 110, 2, 250)                     # b lands the last blow: a lies
    s.op(POSE, 0, 0, 40)
    s.op(CHARGE, 40, 2, 70)
    s.op(DASH_BY, 1, 90, 30)
    s.op(BLAST, 0, 0, 80)
    s.op(BLAST, 1 + 16, 4, 110)                     # a volley, blocked
    s.op(BLAST, 2 + 32, 40, 190)                    # a beam (a big blast without group 18): b thrown
    s.op(POSE, 1, 0, 40)
    s.op(BLAST, 3, 0, 120)
    s.op(STRUGGLE, 50, 2, 210)                      # b's beam wins: a lies
    s.op(POSE, 0, 0, 40)
    s.op(STRUGGLE, 30, 0, 140)
    s.op(POSE, 0, 1, 20)                            # a on a knee
    s.op(POSE, 1, 2, 40)                            # b falls and lies
    s.answer(HELD, 0, 2)
    s.op(POSE, 0, 0, 30)
    s.op(POSE, 1, 0, 40)
    s.clean()
    s.w("ents")


def main():
    s = Script()
    s.w("# GENERATED by tools/headless/cine_gen.py (docs/testing.md \"Cinematic helpers\"): the cinematic helpers, self-checking")
    s.w("#   python tools/headless/staging_run.py tools/headless/cine.txt ROM build/<dir>/hl/menu.ss --out build/<dir>/hl --cols 6")
    s.setup("cine2", True)
    s.setup("cine1", False)

    # sprite pairs: Vegeta 137 (a player layout), Android 17 (9, an enemy set), Cooler's last form 34 (64 x 64), King Cold
    # 74 (64 x 64, five groups, no hit-stun), Piccolo's NPC 73 (one group), a bird 63 (seven groups, no attack)
    for a, b in ((137, 9), (9, 137), (34, 137), (137, 74), (74, 9), (73, 63), (63, 73)):
        s.w(f"echo == pair {a} on {b}")
        s.w("load cine2.ss")
        s.w("p2 0")
        s.w("frames 2")
        s.actor(a, -56, 36)
        s.actor(b, 44, 36)
        s.mark()
        s.answer(HAS, a, 1)
        s.answer(HAS, b, 1)
        every_op(s, a, b)
        s.w(f"shot cine_{a}_{b}")

    s.w("echo == a missing body, an id past the index, the point")
    s.w("load cine2.ss")
    s.w("p2 0")
    s.w("frames 2")
    s.actor(137, -56, 36)
    s.mark()
    s.answer(HAS, 9, 0)
    s.answer(HAS, 5000, 0)
    s.answer(HAS, P1, 1)
    s.answer(HAS, P2, 1)
    s.answer(HAS, NOBODY, 0)
    s.cine(CAST, 137, 9)                            # nobody shows sprite 9
    s.op(DASH_BY, 0, 0, 10, QUIET)
    s.op(STRIKE, 1, 0, 40)                          # he swings at the air
    s.op(EXCHANGE, 60, 0, 10, QUIET)
    s.op(CHARGE, 30, 0, 60)
    s.op(BLAST, 1, 3, 100)                          # straight ahead
    s.op(BLAST, 2, 30, 150)
    s.op(STRUGGLE, 60, 1, 10, QUIET)
    s.op(POSE, 1, 2, 10, QUIET)
    s.cine(CAST, 9, 137)                            # A missing: nothing plays
    for o in (DASH_BY, STRIKE, EXCHANGE, CHARGE, BLAST, STRUGGLE, POSE):
        s.op(o, 30 if o == CHARGE else 0, 0, 10, QUIET)
    s.cine(CAST, 5000, -7)
    for o in (DASH_TO, STRIKE, BLAST):
        s.op(o, 0, 0, 10, QUIET)
    s.cine(CHARGE, 0, 1, 20)                        # a held charge on nobody
    s.answer(HELD, 0, 0)
    s.cine(POINT, SPOT[0] + 70, SPOT[1] + 40)
    s.cine(CAST, 137, THE_POINT)
    s.op(DASH_BY, 1, 0, 30)
    s.op(STRIKE, 0, 0, 40)
    s.op(DASH_BY, 1, 80, 30)
    s.op(BLAST, 1, 3, 100)
    s.op(BLAST, 3, 0, 120)
    s.op(BLAST, 2, 30, 150)
    s.op(EXCHANGE, 60, 0, 10, QUIET)                # the point trades no blows
    s.clean()
    s.w("ents")

    for two in (True, False):
        s.w(f"echo == the players as parties, {'two players' if two else 'one player'}")
        s.w(f"load {'cine2' if two else 'cine1'}.ss")
        s.w("p2 0" if two else "p2 off")            # (the runner's pad setting is not in a savestate)
        s.w("frames 2")
        s.actor(137, -56, 36)
        s.actor(9, 44, 36)
        s.mark()
        s.w("deref pa 0x03001FA8")
        s.cine(CAST, P1, 9)                         # player 1 on 17
        s.op(DASH_BY, 0, 0, 30)
        s.op(STRIKE, 1, 0, 110)
        s.op(POSE, 1, 0, 40)
        s.op(EXCHANGE, 70, 0, 150)
        s.op(CHARGE, 30, 1, 60)
        s.op(DASH_BY, 1, 80, 30)
        s.op(BLAST, 2, 30, 150)
        s.op(STRUGGLE, 40, 1, 200)
        s.op(POSE, 1, 0, 40)
        s.cine(CAST, 9, P1)                         # 17 on player 1: he is thrown, lies, and is up again by himself
        s.op(STRIKE, 1, 0, 110)
        s.w("frames 40")
        s.answer(HELD, 0, 0)
        s.w("deref st pa+0x18")
        s.expect(r"[0-9a-f]{8}: 08025a84", "peek st 1")     # his idle state: control is his
        s.cine(CAST, 9, -11)                        # the player on party slot 1 (Piccolo: player 1 on a Map Test)
        s.op(BLAST, 0, 0, 80)
        s.cine(CAST, 9, -14)                        # the one on slot 4 (Goku: nobody plays him): nobody to dash to
        s.op(DASH_BY, 0, 0, 10, QUIET)
        s.answer(HAS, P2, 1 if two else 0)
        s.cine(CAST, P2, 137)                       # player 2 on Vegeta (alone: nothing plays)
        s.op(DASH_BY, 0, 0, 30, PLAYED if two else QUIET)
        s.op(STRIKE, 3, 0, 110, PLAYED if two else QUIET)
        if two:
            s.op(POSE, 1, 0, 40)
        s.op(EXCHANGE, 50, 0, 130, PLAYED if two else QUIET)
        s.op(BLAST, 1, 3, 100, PLAYED if two else QUIET)
        s.cine(CAST, 137, P2)                       # Vegeta on player 2
        s.op(STRIKE, 1, 0, 110)                     # (alone: at the air)
        s.w("frames 40")
        s.answer(HELD, 0, 0)
        s.cine(CAST, P1, P2)                        # the players on each other
        s.op(EXCHANGE, 60, 0, 140, PLAYED if two else QUIET)
        s.op(STRUGGLE, 40, 0, 160, PLAYED if two else QUIET)
        s.cine(CAST, P1, 137)                       # a charge held on player 1 past the "scene": he is freed
        s.cine(CHARGE, 0, 2, 30)
        s.answer(HELD, 0, 0)
        s.clean()
        s.w("ents")
        s.w(f"shot cine_players_{2 if two else 1}")

    s.w("echo == a transformed player 1 (Goku, slot 4, on his Super Saiyan row 20)")
    s.w("load menu.ss")
    s.w("p2 0")
    s.w("maptest 7")
    s.w("frames 200")
    s.w("poke8 PARTY 4")
    s.w("poke8 PARTY+0x84 20")                      # chars[4].display_row
    s.cmd(5, ARENA[0] | ARENA[1] << 8, SPOT[0] | SPOT[1] << 16, 200)
    s.cine(CAM, 0, 0, 4)
    s.w("find CN 0xC14E0001")
    s.cmd(50, 101)
    s.cmd(50, 103)
    s.actor(9, 44, 36)
    s.mark()
    s.expect(r"[0-9a-f]{8}: 14", "peek8 PARTY+0x84 1")
    s.cine(CAST, P1, 9)
    s.op(DASH_BY, 0, 0, 30)
    s.op(STRIKE, 0, 0, 60)
    s.op(EXCHANGE, 60, 1, 200)
    s.op(POSE, 1, 0, 40)
    s.op(CHARGE, 40, 2, 70)
    s.op(DASH_BY, 1, 90, 30)
    s.op(BLAST, 2 + 32, 40, 190)
    s.op(POSE, 1, 0, 40)
    s.op(STRUGGLE, 40, 2, 200)                      # he loses it: thrown, lies, up again by himself
    s.w("frames 40")
    s.clean(hums=1)                                 # (his form's own hum plays on: one, as before the charge)
    s.w("ents")
    s.w("shot cine_ssj")
    s.w("echo == a blow that knocks him out of his form (strike flag 8): player 1 turns back, an actor takes the base sprite")
    s.cine(CAST, 9, P1)
    s.op(STRIKE, 1 + 8, 40, 110)
    s.expect(r"[0-9a-f]{8}: 13", "peek8 PARTY+0x84 1")       # Goku's base row 19
    s.w("frames 40")
    s.clean()                                       # his form's hum went with it
    s.actor(140, -50, 30)                           # Vegeta's Super Saiyan actor: on the base sprite 137 after the blow
    s.cine(CAST, 9, 140)
    s.op(STRIKE, 1 + 8, 40, 110)
    s.w("deref TBL 0x0800959C")
    s.w(f"deref R140 TBL+{(140 - 7) * 4:#x}")
    s.w(f"deref R137 TBL+{(137 - 7) * 4:#x}")
    s.expect(r"entrec [0-9a-f]+: 0$", "entrec R140")
    s.expect(r"entrec [0-9a-f]+: 1$", "entrec R137")
    s.cine(END, 0, 0, 4)
    s.answer(HELD, 0, 0)
    s.w("ents")
    s.w("shot cine_ssj_down")

    s.w("echo == Start skips an op: the timeline at once, the bodies where they would have ended")
    s.w("load cine2.ss")
    s.w("p2 0")
    s.w("frames 2")
    s.actor(137, -56, 36)
    s.actor(9, 44, 36)
    s.mark()
    s.cine(CAST, 137, 9)
    s.cine(EXCHANGE, 300, 1, 40)
    s.skip()
    s.ended(EXCHANGE, SKIPPED)
    s.answer(HELD, 0, 1)                            # 17 lies, as at the played end
    s.op(POSE, 1, 0, 40)
    s.cine(STRUGGLE, 300, 1, 60)
    s.skip()
    s.ended(STRUGGLE, SKIPPED)
    s.op(POSE, 1, 0, 40)
    s.cine(BLAST, 2 + 32, 200, 50)
    s.skip()
    s.ended(BLAST, SKIPPED)
    s.cine(END, 0, 0, 4)
    s.clean()
    s.w("ents")

    s.w("echo == looks, the camera, the held charge, the end")
    s.w("load cine2.ss")
    s.w("p2 0")
    s.w("frames 2")
    s.actor(137, -56, 36)
    s.actor(9, 44, 36)
    s.mark()
    s.w("deref pa 0x03001FA8")
    s.cine(LOOKAT, 137, NOBODY, 4)                  # everyone to Vegeta (down and left of player 1: a side facing)
    s.expect(r"[0-9a-f]{8}: 02", "peek8 pa+0x10 1")
    s.cine(LOOK, 9, P1, 4)                          # everyone to 17; 17 to player 1
    s.expect(r"[0-9a-f]{8}: 03", "peek8 pa+0x10 1")
    s.cine(POINT, SPOT[0], SPOT[1] - 60)
    s.cine(LOOKAT, THE_POINT, 9, 4)                 # everyone but 17 to a point above
    s.expect(r"[0-9a-f]{8}: 01", "peek8 pa+0x10 1")
    s.cine(CAST, 137, 9)
    s.cine(CAM, 1, 0, 4)                            # the camera follows the cast
    s.op(EXCHANGE, 90, 0, 170)
    s.op(STRIKE, 1, 90, 110)
    s.cine(CAM, 0, 0, 4)
    s.cine(PAN, 137, 6, 60)
    s.cine(PAN, P1, 0, 80)
    s.cine(JOLT, 12, 3, 30)
    s.cine(CHARGE, 0, 2, 60)                        # held on Vegeta
    s.expect(r"[0-9a-f]{8}: [0-9a-f]{4}01[0-9a-f]{2}", "peek CN+0x10 1")
    s.answer(HELD, 0, 2)                            # he and 17 (lying)
    s.cine(CHARGE, -1, 0, 20)
    s.expect(r"[0-9a-f]{8}: [0-9a-f]{4}00[0-9a-f]{2}", "peek CN+0x10 1")
    s.answer(HELD, 0, 1)
    s.cine(CHARGE, 0, 6, 90)                        # beside his own animation (level 2 + 4, the transformations' power-up):
    s.expect(r"[0-9a-f]{8}: [0-9a-f]{4}01[0-9a-f]{2}", "peek CN+0x10 1")
    s.answer(HELD, 0, 1)                            # he is not held (only 17, lying)
    s.cmd(46, 0, 0, 3)
    s.expect(r"[0-9a-f]{8}: 00000000", f"peek {ARG:#x} 1")   # and no hum of its own
    s.cine(CHARGE, -1, 0, 20)                       # the silent burst
    s.expect(r"[0-9a-f]{8}: [0-9a-f]{4}00[0-9a-f]{2}", "peek CN+0x10 1")
    s.cine(CHARGE, 0, 1, 20)
    s.cine(END, 0, 0, 20)                           # everything ends
    s.clean()
    s.w("ents")

    for two in (True, False):
        s.w(f"echo == the demo scene, {'two players' if two else 'one player'}")
        s.w(f"load {'cine2' if two else 'cine1'}.ss")
        s.w("p2 0" if two else "p2 off")            # (the runner's pad setting is not in a savestate)
        s.w("frames 2")
        s.cmd(50, 100)
        s.cmd(5, ARENA[0] | ARENA[1] << 8, SPOT[0] | SPOT[1] << 16, 0)
        s.w(f"waitnz {SCENE:#x} 600")
        s.w("frames 100")
        s.w(f"shot cine_demo{2 if two else 1}_a")
        s.w("key 1 4")
        s.w("frames 60")
        s.w(f"shot cine_demo{2 if two else 1}_b")
        s.expect(r"[0-9a-f]{8}: [0-9a-f]{4}01[0-9a-f]{2}", "peek CN+0x10 1")   # the charge is held through the line
        s.w("key 1 4")
        for i in range(8):
            s.w("frames 200")
            s.w(f"shot cine_demo{2 if two else 1}_{i}")
        s.w(f"stepz {SCENE:#x} 400")
        s.w("frames 40")
        s.expect(r"[0-9a-f]{8}: 00000000", f"peek {SCENE:#x} 1")
        s.expect(r"[0-9a-f]{8}: ([0-9a-f]{4})\1", "peek CN+0x18 1")
        s.w("peek CN+0x18 4")
        s.clean()
        s.expect(rf"map zone {ARENA[0]} area {ARENA[1]}\b", "map")
        s.w("ents")
    s.save("cine.txt")

    f = Script()
    f.w("# GENERATED by tools/headless/cine_gen.py: the cinematic helpers' demo scene (data/story/cine_demo.lscript) filmed, a shot")
    f.w("# every 6 frames (docs/testing.md \"Cinematic helpers\"):")
    f.w("#   python tools/headless/st_test.py tools/headless/cine_demo.txt ROM build/<dir>/hl/menu.ss --out build/<dir>/hl/cine --cols 8")
    f.w("p2 0")
    f.w("maptest 7")
    f.w("frames 200")
    f.cmd(50, 100)
    f.cmd(5, ARENA[0] | ARENA[1] << 8, SPOT[0] | SPOT[1] << 16, 0)
    f.w(f"waitnz {SCENE:#x} 600")
    n = 0
    for frames in (120, 90, 1560):
        for _ in range(0, frames, 6):
            f.w("frames 6")
            f.w(f"shot dm_{n:04d}")
            n += 1
        if frames != 1560:
            f.w("key 1 4")
    f.w(f"stepz {SCENE:#x} 400")
    f.w("find CN 0xC14E0001")
    f.w("peek CN 16")
    f.w("ents")
    f.save("cine_demo.txt")


if __name__ == "__main__":
    main()
