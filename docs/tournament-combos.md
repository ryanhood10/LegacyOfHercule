# Team combos: the step tables (Ryan's doc, the game's data)

The team combos of the World Tournament (`docs/tournament.md` 24.10: the design decided with Ryan; 25.2: the runner).
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
floor. When the combo ends everyone stays where the last row left him.

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
| `ball` | W [large] | a big orb thrown slowly; W ki blasts, a big blast when it lands |
| `beam` | W N [through] | his beam (the Kamehameha's object), held N frames; W ki blasts when its tip reaches the target; `through`: it goes on past him (through a partner on the far side, who is never hurt) |
| `disc` | W | a Destructo Disc |
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

(A first draft for the follow-up piece to tune.) Both dash in trading punches; Krillin springs back and throws the
Destructo Disc that pins him; Gohan's Masenko finishes.

Name: Disc and Masenko

| at | who | does | how |
|---:|---|---|---|
| 0 | A B | dash | 18 0 10 |
| 0 | T | pose | hurt 0 |
| 12 | A | punch | 1 |
| +8 | B | punch | 1 |
| +8 | A | punch | 1 kick |
| +8 | B | punch | 1 kick |
| +8 | A | punch | 1 |
| +8 | B | punch | 1 |
| +8 | A | punch | 1 kick |
| +8 | B | punch | 1 kick |
| +8 | A | punch | 1 |
| +8 | B | punch | 1 |
| +8 | A | punch | 1 special |
| +8 | B | punch | 1 special |
| 108 | B | teleport | 64 0 |
| 108 | A | dash | 72 0 10 |
| 116 | B | pose | ki 0-1 loop 4 |
| 124 | B | disc | 6 |
| 136 | T | pose | hurt 1 |
| 136 | A | pose | beam2 0 |
| 136 | A | flash | 4 |
| 136 | - | sound | 55 |
| 156 | A | beam | 6 36 |
| 196 | T | knockdown | 40 12 |
| 198 | A B | pose | idle |
| 268 | - | end | |

### Piccolo & Hercule (`PICCOLO_HERCULE`)

(First draft.) Piccolo dashes in throwing ki blasts and takes the Special Beam Cannon pose while Hercule cheers, then
runs in with the Dynamite Kick at the moment the beam lands, and takes the credit. Hercule's blows are poses (the
showman): their weight goes to Piccolo's next blast and the beam.

Name: Special Beam Dynamite

| at | who | does | how |
|---:|---|---|---|
| 0 | A | dash | 64 0 10 |
| 0 | B | walk | 100 0 20 |
| 0 | T | pose | hurt 0 |
| 12 | A | blast | 1 blue |
| 12 | B | punch | 1 |
| +8 | A | blast | 1 blue |
| +0 | B | punch | 1 |
| +8 | A | blast | 1 blue |
| +0 | B | punch | 1 |
| +8 | A | blast | 1 blue |
| +0 | B | punch | 1 |
| +8 | A | blast | 1 blue |
| +0 | B | punch | 1 |
| +8 | A | blast | 1 blue |
| +0 | B | punch | 1 |
| 60 | A | pose | beam 0-1 loop 6 |
| 60 | - | sound | 55 |
| 62 | B | dash | 20 0 24 |
| 88 | B | punch | 6 kick |
| 92 | A | beam | 6 36 |
| 130 | T | knockdown | 40 12 |
| 130 | B | pose | shout 0-3 loop 6 |
| 130 | A | pose | idle |
| 202 | - | end | |

### Goku & Piccolo (`GOKU_PICCOLO`)

(First draft.) Goku appears behind him and holds him, punching; Piccolo charges the Special Beam Cannon on the other
side and fires it through both of them: only the opponent is hurt (Ryan).

Name: Special Beam Cannon

| at | who | does | how |
|---:|---|---|---|
| 0 | A | teleport | 14 0 |
| 0 | B | dash | 84 0 12 |
| 0 | T | pose | hurt 0 |
| 6 | A | punch | 1 |
| +8 | A | punch | 1 |
| +8 | A | punch | 1 |
| +8 | A | punch | 1 |
| +8 | A | punch | 1 |
| +8 | A | punch | 1 special |
| 14 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| +8 | B | blast | 1 blue |
| 60 | B | pose | beam 0-1 loop 6 |
| 60 | A | pose | special 0 |
| 60 | - | sound | 55 |
| 84 | A | punch | 6 special |
| 96 | B | beam | 6 40 through |
| 98 | A | flash | 30 |
| 138 | A | teleport | 48 0 |
| 140 | T | knockdown | 40 12 |
| 140 | B | pose | idle |
| 212 | - | end | |

### The father-son Kamehameha (`FATHER_SON`)

(First draft.) Father and son side by side on one side, throwing ki blasts together, then the Kamehameha together.
Goku & Gohan: both beams. Hercule & Super Saiyan 2 Gohan: the same animation; Hercule strikes the pose beside Gohan
and nothing comes out of him (the beam is Gohan's, and Hercule's weight goes into it).

Name: Father-Son Kamehameha

| at | who | does | how |
|---:|---|---|---|
| 0 | A | dash | 72 -10 12 |
| 0 | B | dash | -72 10 12 |
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
| 90 | A B | pose | beam 0 |
| 90 | A B | flash | 4 |
| 90 | - | shake | 40 25 |
| 90 | - | sound | 55 |
| 130 | A B | beam | 6 44 |
| 176 | T | knockdown | 40 12 |
| 178 | A B | pose | idle |
| 248 | - | end | |

### Frieza & Cooler (`FRIEZA_COOLER`)

(First draft; villains on a player.) The brothers appear on either side and fire Death Beams in turn; Frieza's last
Death Beam and Cooler's Supernova together.

Name: Death Beam Supernova

| at | who | does | how |
|---:|---|---|---|
| 0 | A B | flash | 6 |
| 0 | A B | teleport | 64 0 |
| 0 | T | pose | hurt 0 |
| 10 | A | blast | 1 |
| +7 | B | blast | 1 |
| +7 | A | blast | 1 |
| +7 | B | blast | 1 |
| +7 | A | blast | 1 |
| +7 | B | blast | 1 |
| +7 | A | blast | 1 |
| +7 | B | blast | 1 |
| +7 | A | blast | 1 |
| +7 | B | blast | 1 |
| +7 | A | blast | 1 |
| +7 | B | blast | 1 |
| 96 | B | pose | charge 0-1 loop 4 |
| 96 | - | shake | 30 30 |
| 96 | - | sound | 55 |
| 126 | B | ball | 6 large |
| 130 | A | beam | 6 24 |
| 160 | T | knockdown | 40 12 |
| 160 | A B | pose | idle |
| 232 | - | end | |

### Broly & Paragus (`BROLY_PARAGUS`)

(First draft; villains on a player.) Paragus's control device flashes and freezes him; Broly pounds him and Paragus
fires; Broly's Eraser Cannon finishes.

Name: Eraser Cannon

| at | who | does | how |
|---:|---|---|---|
| 0 | B | pose | charge 0 |
| 0 | B | flash | 10 |
| 0 | - | screen | 24 |
| 0 | T | pose | hurt 0 |
| 0 | A | dash | 20 0 12 |
| 4 | B | walk | 80 0 20 |
| 16 | A | punch | 1 |
| +8 | B | blast | 1 |
| +0 | A | punch | 1 |
| +8 | B | blast | 1 |
| +0 | A | punch | 1 kick |
| +8 | B | blast | 1 |
| +0 | A | punch | 1 |
| +8 | B | blast | 1 |
| +0 | A | punch | 1 |
| +8 | B | blast | 1 |
| +0 | A | punch | 1 special |
| +8 | B | blast | 1 |
| 72 | B | blast | 3 |
| +8 | B | blast | 3 |
| 80 | A | slide | 56 0 10 |
| 92 | A | pose | ki 0-1 loop 4 |
| 92 | - | shake | 30 40 |
| 92 | - | sound | 55 |
| 120 | A | ball | 6 large |
| 156 | T | knockdown | 40 12 |
| 156 | A B | pose | idle |
| 228 | - | end | |

### Mecha Frieza & King Cold (`MECHA_COLD`)

(First draft; villains on a player.) Father and son arrive together, one on each side, and fire Death Beams.

Name: Father and Son

| at | who | does | how |
|---:|---|---|---|
| 0 | A B | flash | 6 |
| 0 | A B | teleport | 64 0 |
| 0 | T | pose | hurt 0 |
| 10 | A B | blast | 1 |
| +10 | A B | blast | 1 |
| +10 | A B | blast | 1 |
| +10 | A B | blast | 1 |
| +10 | A B | blast | 1 |
| +10 | A B | blast | 1 |
| 70 | A B | pose | beam 0 |
| 70 | - | sound | 55 |
| 96 | A B | beam | 6 30 |
| 130 | T | knockdown | 40 12 |
| 130 | A B | pose | idle |
| 202 | - | end | |

### Perfect Cell & Cell Jr. (`CELL_JR`)

(First draft; villains on a player.) Cell Jr. swarms him, striking from one side and then the other, while Cell throws
ki blasts and charges; Cell's Kamehameha finishes.

Name: Perfect Kamehameha

| at | who | does | how |
|---:|---|---|---|
| 0 | A | walk | 80 0 16 |
| 0 | B | teleport | 16 0 |
| 0 | T | pose | hurt 0 |
| 4 | B | punch | 1 |
| +6 | B | punch | 1 |
| +8 | B | teleport | -16 0 |
| +2 | B | punch | 1 kick |
| +8 | B | teleport | 16 -8 |
| +2 | B | punch | 1 |
| +8 | B | teleport | -16 8 |
| +2 | B | punch | 1 kick |
| +8 | B | teleport | 16 0 |
| +2 | B | punch | 1 |
| +8 | B | punch | 6 special |
| 20 | A | blast | 1 |
| +8 | A | blast | 1 |
| +8 | A | blast | 1 |
| +8 | A | blast | 1 |
| +8 | A | blast | 1 |
| +8 | A | blast | 1 |
| 80 | B | teleport | -56 0 |
| 80 | A | pose | beam 0 |
| 80 | - | shake | 30 25 |
| 80 | - | sound | 55 |
| 110 | A | beam | 6 40 |
| 152 | T | knockdown | 40 12 |
| 154 | A B | pose | idle |
| 224 | - | end | |

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
