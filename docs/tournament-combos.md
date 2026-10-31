# Team combos: the step tables (Ryan's doc, the game's data)

The team combos of the World Tournament (`docs/tournament.md` 24.10: the design decided with Ryan; 25.2: the runner; 25.3: the combos as built).
This page is both the plan and the data: `tools/tnycombo.py` reads section 3's tables at build time into
`build/gen/tny_combo_data.c`, so **an edit here is an edit in the game** (rebuild to see it). A table that does not parse
stops the build with the row's line number and what is wrong.

## 1. The format

A combo is a heading `### <who> (`ID`)`, a `Name:` line (the caption shown at the bottom while it plays) and one table:

| column | what |
|---|---|
| at | the frame the step starts, counted from the combo's start (60 frames = 1 second), or `+N`: N frames after the row above. Rows may follow one fighter and then the other: they are played by frame (rows of one frame in table order) |
| who | `A` the pair's first fighter (the lead: the first of the pair in section 4), `B` the second, `T` the target, several (`A B`), or `-` for a screen step |
| does | the step (below) |
| how | its numbers and words |

Sides: when a combo starts, A keeps the side of the target he stands on and B takes the other one. Distances in
`dash` / `walk` / `slide` / `teleport` are measured from the target **towards the mover's own side** (a negative distance:
the other side; for the target himself: away from A), then pixels down (negative: up). Every spot is kept on the ring's
floor, and a fighter's spot that would cover a bystander held in the freeze (the other players, other opponents) is
moved just clear of him: nearer the target, else below or above him. When the combo ends everyone stays where the
last row left him.

| does | how | what happens |
|---|---|---|
| `dash` | X Y N | runs (the dash pose) to X px from the target, Y px down, in N frames |
| `walk` | X Y N | the same at a walk |
| `slide` | X Y N | the same, keeping whatever pose he has (a knock-back) |
| `teleport` | X Y | vanishes and appears there at once, a flash at both spots |
| `pose` | NAME [F or F-G] [N, or loop N] | shows that pose: frame F; F-G runs through the frames in N frames, `loop N` repeats them, a frame every N frames |
| `face` | [A, B, him, away] | turns: A and B to the target; the target to A (or B); `away` the other way |
| `punch` | W [kick, special] | a melee blow worth W of his own punches (the punch pose, or the kick, or the special melee) |
| `blast` | W [blue] | a ki blast flying at the target, worth W of his own ki blasts when it reaches him (a yellow orb, or blue ki) |
| `ball` | W [large] [hold N] [front] | a big orb thrown slowly; W ki blasts, a big blast when it lands. `hold N`: it first waits N frames just over his head (a Supernova), or with `front` in front of his hands (an Eraser Cannon), then flies (pose him meanwhile with a `pose` row) |
| `beam` | W N [through] | his beam (the Kamehameha's object), held N frames; W ki blasts when its tip reaches the target; `through`: it goes on past him (through a partner on the far side, who is never hurt) |
| `disc` | W [hold N] [pin] | a Destructo Disc. `hold N`: it spins over his head N frames first; `pin`: when it lands it stays spinning on the target (he is pinned) until the partner's next blow lands, then bursts |
| `sbc` | W N [through] [charge C] | Piccolo's Special Beam Cannon, the game's own object: the spark at his fingers for C frames (pose him `beam 0-1 loop 8`, as the game's own SBC), then the drill; W ki blasts when its tip reaches the target, held N frames, then it slides off; `through` as `beam` |
| `copy` | K X Y, or K `off` | copy K (1-3) of the fighter's body appears there (a flash; X / Y as a dash's), facing the target; it shows his pose frame for frame (it strikes as he strikes) and never hurts anyone: Cell Jr.'s swarm. Again: it moves there; `off` (K 0: all of them) it vanishes; any left vanish at the end |
| `knockdown` | LIE UP | the target falls (his knock-down pose), lies LIE frames, gets up in UP frames |
| `flash` | [N] | a white flash on whoever, N frames (4) |
| `screen` | N | the whole screen flashes white and fades back over N frames |
| `shake` | N S | the camera shakes N frames at strength S (a punch is 4 30, a big blast 24 100) |
| `sound` | ID | a sound of the game's effect table: 13 a launch, 20 a blast, 34 an impact, 40 Hercule's shout, 55 a power-up |
| `caption` | | the move's name at the bottom (added at frame 0 when a table has none) |
| `end` | | the combo ends here: the fight goes on. Exactly one, the last row |

Pose names (each falls back to the next group a body has when it lacks one): `idle` 0, `walk` 1, `hurt` 2, `fall` 3,
`shout` 4, `charge` 5, `punch` 7 (6, 8), `kick` 8, `dash` 13, `special` 16 (the special melee), `beam` 18 (the beam
pose), `beam2` 19, `ki` 5 (the ki pose), `lie` (the body's lying pose); `gN` is group N as it is.

**Hercule is the showman** (the rule of the whole mod): his steps are poses and shouts only, nothing leaves his hands and
nothing he does hurts anyone; the weight of each of his steps goes to his partner's next blow, so the pair's combo is
as strong as any other and the partner's attack does the work.

## 2. Damage: what a weight is

Ryan's rule (24.10): the damage is a multiplier of the attackers' own moves, never a share of the target's HP, so story
characters who grew stronger feel it when they come back, and villains hit from their tier's stats. A step's weight W is
**W times one of his own blows**: the game's damage formula runs with the attacking fighter's combat bytes (a player's
from his party entry, a villain's from his tier stat), the target's defence and the attack's base power (a punch:
melee 0x400, the players' A button's own blow; everything else: ranged 0x400, the players' ki blast), times W. Melee
blows can be critical hits as any punch. On a player the co-op multiplier of the difficulty applies as on every hit,
and the revive and the tournament's KO rules hold.

Per 24.10 each partner's part is worth **about six of his normal hits plus a finisher at about three times a special**:
the tables give each partner six blows of weight 1 and a finisher of weight 6 (a special counted as two blows: the
players' strongest single ability hit, the Destructo Disc, is twice the ki blast's base power). Measured headless
(2026-10-31, `tools/headless/tny_combo_run.txt`, co-op Goku & Vegeta on a Rookie opponent, 264 frames):

| what | damage in 264 frames |
|---|---|
| the Goku & Vegeta combo | 4111 (Goku 2180, Vegeta 1931) |
| both players mashing A and chasing him (the third punch pushes him 40 px) | 947 (6 hits landed) |
| player 1 punching as fast as the A button's routine can, stepped back beside him before every punch (no human can) | 2747 (14 hits) |
| the same combo with both players' combat bytes at 0x20 / at 0x50 (STR / POW 0x2000 / 0x5000) | 1359 / 7286 |

So a combo is about four times what the pair could punch in the same time, plus a knock-down, and it grows with the
attackers' strength. Hercule's pairs deal the same total (his weight goes into his partner's blows).

**Every combo against every other** (2026-10-31, `tools/headless/tny_combo_tune.txt`): every attacker's combat bytes at
0x30 (players: their entry's STR / POW 0x3000 too); the players' pairs on the same Rookie opponent (defence 9), the
villains' on player 1 (Piccolo, defence 0x40; the co-op multiplier of Normal applies). A finisher of weight 6 is a
ranged blow everywhere (a melee critical hit on a weight of 6 alone moved one combo 23 % above the rest), so melee
weights stay in blows of 1.

| combo (side) | frames | damage (A, B) |
|---|---:|---|
| Gohan & Krillin (players) | 274 | 2517 (1212, 1305) |
| Piccolo & Hercule | 206 | 2447 (Piccolo 2447, Hercule 0) |
| Goku & Vegeta | 264 | 2503 (1179, 1324) |
| Goku & Piccolo | 224 | 2513 (1307, 1206) |
| Goku & Gohan (father-son) | 250 | 2413 (1203, 1210) |
| Hercule & Super Saiyan 2 Gohan (father-son) | 250 | 2477 (Hercule 0, Gohan 2477) |
| Frieza & Cooler (villains, on player 1) | 228 | 1644 (819, 825) |
| Broly & Paragus | 232 | 1618 (813, 805) |
| Mecha Frieza & King Cold | 224 | 1601 (804, 797) |
| Perfect Cell & Cell Jr. | 276 | 1624 (819, 805) |
| the generic barrage (Spopovich & Yamu) | 232 | 1593 (786, 807) |

The players' pairs lie within 4 % of each other, the villains' within 3 %: no pair is stronger than another except
through the attackers' own numbers.

## 3. The combos

### The generic barrage (`GENERIC`)

Both fighters vanish and reappear on either side of the target, throw ki blasts in turn, charge together and throw a
big orb each at once (the androids' pairs, Spopovich & Yamu: villains on a player).

Name: Team Barrage

| at | who | does | how |
|---:|---|---|---|
| 0 | A B | flash | 6 |
| 0 | A B | teleport | 56 0 |
| 0 | T | pose | hurt 0 |
| 6 | A B | pose | ki 0 |
| 14 | A | blast | 1 |
| +8 | B | blast | 1 |
| +8 | A | blast | 1 |
| +8 | B | blast | 1 |
| +8 | A | blast | 1 |
| +8 | B | blast | 1 |
| +8 | A | blast | 1 |
| +8 | B | blast | 1 |
| +8 | A | blast | 1 |
| +8 | B | blast | 1 |
| +8 | A | blast | 1 |
| +8 | B | blast | 1 |
| 112 | A B | pose | charge 0-1 loop 4 |
| 112 | A B | flash | 6 |
| 112 | - | shake | 30 20 |
| 112 | - | sound | 55 |
| 142 | A B | ball | 6 large |
| 160 | T | knockdown | 40 12 |
| 160 | A B | pose | idle |
| 232 | - | end | |

### Goku & Vegeta (`GOKU_VEGETA`)

Both dash in from either side and trade punches with him between them, spring back out, and fire together: Goku's
Kamehameha from one side, Vegeta's Big Bang from the other.

Name: Kamehameha and Big Bang

| at | who | does | how |
|---:|---|---|---|
| 0 | A B | dash | 18 0 10 |
| 0 | T | pose | hurt 0 |
| 12 | A | punch | 1 |
| +8 | B | punch | 1 kick |
| +8 | A | punch | 1 kick |
| +8 | B | punch | 1 |
| +8 | A | punch | 1 |
| +8 | B | punch | 1 kick |
| +8 | A | punch | 1 kick |
| +8 | B | punch | 1 |
| +8 | A | punch | 1 |
| +8 | B | punch | 1 kick |
| +8 | A | punch | 1 special |
| +8 | B | punch | 1 special |
| 108 | A B | dash | 64 0 10 |
| 120 | A | pose | beam 0 |
| 120 | B | pose | charge 0-1 loop 4 |
| 120 | A B | flash | 4 |
| 120 | - | shake | 36 25 |
| 120 | - | sound | 55 |
| 150 | A | beam | 6 40 |
| 150 | B | ball | 6 large |
| 192 | T | knockdown | 40 12 |
| 194 | A B | pose | idle |
| 264 | - | end | |

### Gohan & Krillin (`GOHAN_KRILLIN`)

Both dash in from either side and trade punches and kicks with him between them, spring back out and throw ki blasts
in turn. Krillin raises his hand: the Destructo Disc spins over it while Gohan lifts his arm for the Masenko; the disc
flies, pins him (it stays spinning on him, he is stuck in his hurt pose) and Gohan's Masenko finishes him (the disc
bursts as the beam lands).

Name: Disc and Masenko

| at | who | does | how |
|---:|---|---|---|
| 0 | A B | dash | 20 0 10 |
| 0 | T | pose | hurt 0 |
| 12 | A | punch | 1 |
| +8 | B | punch | 1 kick |
| +8 | A | punch | 1 kick |
| +8 | B | punch | 1 |
| +8 | A | punch | 1 |
| +8 | B | punch | 1 kick |
| +8 | A | punch | 1 special |
| +8 | B | punch | 1 special |
| 76 | A | dash | 64 0 10 |
| 76 | B | dash | 76 0 10 |
| 90 | A | blast | 1 |
| +6 | B | blast | 1 blue |
| +6 | A | blast | 1 |
| +6 | B | blast | 1 blue |
| 116 | B | pose | shout 3 |
| 116 | B | disc | 6 hold 28 pin |
| 116 | A | pose | shout 1 |
| 116 | A | flash | 6 |
| 116 | - | sound | 55 |
| 144 | B | pose | ki 2 |
| 160 | T | pose | hurt 1 |
| 166 | A | pose | beam 0 |
| 166 | A | flash | 4 |
| 166 | - | shake | 20 25 |
| 172 | A | beam | 6 30 |
| 202 | T | knockdown | 40 12 |
| 204 | A B | pose | idle |
| 274 | - | end | |

### Piccolo & Hercule (`PICCOLO_HERCULE`)

Piccolo takes his spot and pelts him with ki blasts while Hercule, out on the other side, shadow-boxes and shouts.
Piccolo puts two fingers up for the Special Beam Cannon (the game's own: the spark at his fingers, then the drill) and
holds him there: he freezes. Hercule winds up, then runs in and lands his Dynamite Kick at the very moment the beam
hits; the opponent goes down and Hercule waves to the crowd, taking the credit, while Piccolo turns away. Hercule's
steps are poses and his shout: their weight goes into Piccolo's blasts and the beam.

Name: Special Beam Dynamite

| at | who | does | how |
|---:|---|---|---|
| 0 | A | dash | 64 0 12 |
| 0 | B | walk | 100 0 20 |
| 0 | T | pose | hurt 0 |
| 14 | A | blast | 1 blue |
| +8 | A | blast | 1 blue |
| +8 | A | blast | 1 blue |
| +8 | A | blast | 1 blue |
| +8 | A | blast | 1 blue |
| +8 | A | blast | 1 blue |
| 24 | B | punch | 1 |
| +8 | B | punch | 1 |
| +8 | B | punch | 1 |
| +8 | B | punch | 1 |
| +8 | B | punch | 1 |
| +8 | B | punch | 1 |
| 66 | A | pose | beam 0-1 loop 8 |
| 66 | A | sbc | 6 24 charge 56 |
| 66 | - | sound | 55 |
| 66 | T | flash | 8 |
| 70 | B | pose | shout 0-3 loop 6 |
| 84 | B | punch | 6 |
| 92 | B | dash | 30 0 26 |
| 122 | B | punch | 0 kick |
| 130 | T | knockdown | 40 12 |
| 136 | B | pose | g5 0-2 loop 8 |
| 136 | A | pose | idle |
| 170 | B | pose | shout 0-3 loop 6 |
| 206 | - | end | |

### Goku & Piccolo (`GOKU_PICCOLO`)

Goku appears behind him in a flash and pounds him from behind while Piccolo, on the other side, throws ki blasts; a
spinning blow, then Goku grabs him from behind, arms around him, and holds him while he struggles. Piccolo charges the
Special Beam Cannon (the game's own) and fires it through both of them: the drill goes through the opponent and on
through Goku, who flashes white and is not hurt (Ryan); only the opponent is. Goku lets go and hops back as he falls.

Name: Special Beam Cannon

| at | who | does | how |
|---:|---|---|---|
| 0 | A | teleport | 18 0 |
| 0 | B | dash | 84 0 12 |
| 0 | T | pose | hurt 0 |
| 6 | A | punch | 1 |
| +8 | A | punch | 1 kick |
| +8 | A | punch | 1 |
| +8 | A | punch | 1 kick |
| +8 | A | punch | 1 |
| +8 | A | punch | 1 kick |
| 54 | A | pose | ki 1 |
| 54 | A | ball | 6 |
| 14 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| 66 | A | slide | 8 -1 4 |
| 66 | A | pose | beam 1 |
| 66 | T | face | B |
| 66 | T | pose | hurt 0-1 loop 6 |
| 70 | B | pose | beam 0-1 loop 8 |
| 70 | B | sbc | 6 30 through charge 50 |
| 70 | - | sound | 55 |
| 132 | A | flash | 16 |
| 132 | T | flash | 8 |
| 152 | A | dash | 44 0 8 |
| 152 | T | knockdown | 40 12 |
| 154 | B | pose | idle |
| 224 | - | end | |

### The father-son Kamehameha (`FATHER_SON`)

Father and son run to the same side of him, side by side (the son a step ahead), and throw ki blasts in turn; then
both crouch into the Kamehameha together, the charge flashes and shakes the ring, and the two beams go out side by side
as one. Goku & Gohan: both beams. Hercule & Super Saiyan 2 Gohan: the same animation; Hercule waves to the crowd
where Goku throws (his ki pose is his wave), strikes the Kamehameha pose beside Gohan and nothing comes out of him: the
beam is Gohan's, and Hercule's weight goes into Gohan's blows.

Name: Father-Son Kamehameha

| at | who | does | how |
|---:|---|---|---|
| 0 | A | dash | 72 -10 12 |
| 0 | B | dash | -58 10 12 |
| 0 | T | pose | hurt 0 |
| 16 | A | blast | 1 |
| +6 | B | blast | 1 |
| +6 | A | blast | 1 |
| +6 | B | blast | 1 |
| +6 | A | blast | 1 |
| +6 | B | blast | 1 |
| +6 | A | blast | 1 |
| +6 | B | blast | 1 |
| +6 | A | blast | 1 |
| +6 | B | blast | 1 |
| +6 | A | blast | 1 |
| +6 | B | blast | 1 |
| 92 | A B | pose | beam 0 |
| 92 | A B | flash | 4 |
| 92 | - | shake | 40 25 |
| 92 | - | sound | 55 |
| 116 | A B | flash | 4 |
| 132 | A B | beam | 6 44 |
| 178 | T | knockdown | 40 12 |
| 180 | A B | pose | idle |
| 250 | - | end | |

### Frieza & Cooler (`FRIEZA_COOLER`)

(Villains on a player.) The brothers appear on either side of him and fire Death Beams in turn, finger pointed: a
volley of quick blue shots from both sides. Cooler lifts a big orb over his head (the Supernova), the ring shakes,
Frieza points for one last Death Beam; Cooler throws the Supernova and both land together.

Name: Death Beam Supernova

| at | who | does | how |
|---:|---|---|---|
| 0 | A B | flash | 6 |
| 0 | A B | teleport | 64 0 |
| 0 | T | pose | hurt 0 |
| 10 | A | blast | 1 blue |
| +0 | A | pose | ki 2 |
| +7 | B | blast | 1 blue |
| +0 | B | pose | ki 2 |
| +7 | A | blast | 1 blue |
| +0 | A | pose | ki 2 |
| +7 | B | blast | 1 blue |
| +0 | B | pose | ki 2 |
| +7 | A | blast | 1 blue |
| +0 | A | pose | ki 2 |
| +7 | B | blast | 1 blue |
| +0 | B | pose | ki 2 |
| +7 | A | blast | 1 blue |
| +0 | A | pose | ki 2 |
| +7 | B | blast | 1 blue |
| +0 | B | pose | ki 2 |
| +7 | A | blast | 1 blue |
| +0 | A | pose | ki 2 |
| +7 | B | blast | 1 blue |
| +0 | B | pose | ki 2 |
| +7 | A | blast | 1 blue |
| +0 | A | pose | ki 2 |
| +7 | B | blast | 1 blue |
| +0 | B | pose | ki 2 |
| 98 | B | pose | ki 1 |
| 98 | B | ball | 6 large hold 36 |
| 98 | - | shake | 36 30 |
| 98 | - | sound | 55 |
| 104 | A | pose | ki 2 |
| 104 | A | flash | 4 |
| 126 | A | beam | 6 24 |
| 134 | B | pose | ki 2 |
| 156 | T | knockdown | 40 12 |
| 158 | A B | pose | idle |
| 228 | - | end | |

### Broly & Paragus (`BROLY_PARAGUS`)

(Villains on a player.) Paragus steps out to the other side and raises the control device: it glows, the screen
flashes and the target freezes in place (he flashes white and stays in his hurt pose). Broly walks into him with six
blows while Paragus throws ki from his spot; the device flashes again with a bigger blast of Paragus's. Broly steps
back, an Eraser Cannon (his kit's: the medium orb) grows in his palm and he throws it.

Name: Eraser Cannon

| at | who | does | how |
|---:|---|---|---|
| 0 | B | dash | 76 0 10 |
| 0 | T | pose | hurt 0 |
| 12 | B | pose | g14 0-4 6 |
| 12 | B | flash | 10 |
| 12 | - | screen | 24 |
| 12 | - | sound | 55 |
| 12 | T | flash | 24 |
| 16 | A | dash | 30 0 12 |
| 30 | A | punch | 1 |
| +8 | A | punch | 1 |
| +8 | A | punch | 1 kick |
| +8 | A | punch | 1 |
| +8 | A | punch | 1 kick |
| +8 | A | punch | 1 special |
| 32 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| 50 | T | flash | 6 |
| 82 | B | pose | g14 0-4 6 |
| 82 | B | flash | 8 |
| 82 | - | screen | 12 |
| 82 | T | flash | 12 |
| 88 | B | blast | 6 blue |
| 88 | A | dash | 56 0 10 |
| 102 | A | pose | ki 2 |
| 102 | A | ball | 6 hold 36 front |
| 102 | - | shake | 36 40 |
| 102 | - | sound | 55 |
| 138 | A | flash | 4 |
| 160 | T | knockdown | 40 12 |
| 162 | A B | pose | idle |
| 232 | - | end | |

### Mecha Frieza & King Cold (`MECHA_COLD`)

(Villains on a player.) Father and son arrive together in a flash, one on each side, and fire Death Beams at once,
fingers pointed, six times; then both point and charge, the ring shakes, and their two Death Beams meet on him.

Name: Father and Son

| at | who | does | how |
|---:|---|---|---|
| 0 | A B | flash | 6 |
| 0 | A | teleport | 64 0 |
| 0 | B | teleport | 84 0 |
| 0 | T | pose | hurt 0 |
| 14 | A B | blast | 1 blue |
| +0 | A B | pose | ki 2 |
| +10 | A B | blast | 1 blue |
| +0 | A B | pose | ki 2 |
| +10 | A B | blast | 1 blue |
| +0 | A B | pose | ki 2 |
| +10 | A B | blast | 1 blue |
| +0 | A B | pose | ki 2 |
| +10 | A B | blast | 1 blue |
| +0 | A B | pose | ki 2 |
| +10 | A B | blast | 1 blue |
| +0 | A B | pose | ki 2 |
| 80 | A B | pose | beam 0 |
| 80 | A B | flash | 6 |
| 80 | - | shake | 40 30 |
| 80 | - | sound | 55 |
| 120 | A B | beam | 6 30 |
| 152 | T | knockdown | 40 12 |
| 154 | A B | pose | idle |
| 224 | - | end | |

### Perfect Cell & Cell Jr. (`CELL_JR`)

(Villains on a player.) Cell stays back; the Juniors swarm him: Cell Jr. and two more (copies of his body, striking as
he strikes) appear round him in flashes, punch and kick from three sides, vanish and reappear on other sides. The
Juniors leap back to stand round Cell, Cell throws a volley of ki blasts, crouches into the Kamehameha and fires; when
the target is down the extra Juniors vanish.

Name: Perfect Kamehameha

| at | who | does | how |
|---:|---|---|---|
| 0 | A | walk | 64 0 16 |
| 0 | B | teleport | 16 0 |
| 0 | B | copy | 1 -16 0 |
| 0 | B | copy | 2 4 -16 |
| 0 | T | pose | hurt 0 |
| 6 | B | punch | 1 |
| +7 | B | punch | 1 |
| +7 | B | punch | 1 kick |
| 28 | B | teleport | -16 4 |
| 28 | B | copy | 1 16 -4 |
| 28 | B | copy | 2 -2 16 |
| 32 | B | punch | 1 |
| +7 | B | punch | 1 |
| +7 | B | punch | 1 kick |
| 54 | B | teleport | 16 -6 |
| 54 | B | copy | 1 -16 6 |
| 54 | B | copy | 2 2 -16 |
| 58 | B | punch | 1 |
| +7 | B | punch | 1 kick |
| +7 | B | punch | 1 |
| +7 | B | punch | 1 kick |
| +7 | B | punch | 1 |
| +7 | B | punch | 1 special |
| 102 | B | teleport | -40 -28 |
| 102 | B | copy | 1 -40 28 |
| 102 | B | copy | 2 -84 26 |
| 102 | B | pose | idle |
| 108 | A | blast | 1 |
| +6 | A | blast | 1 |
| +6 | A | blast | 1 |
| +6 | A | blast | 1 |
| +6 | A | blast | 1 |
| +6 | A | blast | 1 |
| 146 | A | pose | beam 0 |
| 146 | A | flash | 4 |
| 146 | - | shake | 30 25 |
| 146 | - | sound | 55 |
| 172 | A | beam | 6 36 |
| 204 | T | knockdown | 40 12 |
| 206 | A | pose | idle |
| 236 | B | copy | 0 off |
| 276 | - | end | |

## 4. The pairs (reference)

Which pairs have a combo is Ryan's list (`docs/tournament.md` 24.10); the table the game uses is `src/coop/tny_combo.c`
(`tny_combo_of`, `tny_combo_lead`: the lead's piece), kept there because the draw uses it too. A pair's first fighter is
the tables' A.

| pair (A first) | combo |
|---|---|
| Gohan & Krillin | `GOHAN_KRILLIN` |
| Piccolo & Hercule | `PICCOLO_HERCULE` |
| Goku & Vegeta | `GOKU_VEGETA` |
| Goku & Piccolo | `GOKU_PICCOLO` |
| Goku & Gohan; Hercule & Super Saiyan 2 Gohan | `FATHER_SON` |
| Frieza & Cooler | `FRIEZA_COOLER` |
| Broly & Paragus | `BROLY_PARAGUS` |
| Mecha Frieza & King Cold | `MECHA_COLD` |
| Perfect Cell & Cell Jr. | `CELL_JR` |
| any two of Dr. Gero, Android 19, 13, 17, 18; Spopovich & Yamu | `GENERIC` |
