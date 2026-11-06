# The cutscene pass, for Ryan to review (2026-11-06)

Your brief, while you were away: captivating cutscenes for the game and for your Shorts, people moving and turning to
whoever talks, blasts and fights that are not basic, true to Dragon Ball Z, nothing cheesy, dynamic by who plays,
improving the stock scenes rather than replacing them; and better sprites, especially the ones we made.

Everything here is merged, pushed and tested headless: the full regression passes and every changed scene
was filmed in its lineups and looked at. **Nothing has been played in real time and no sound has been heard.** Lines:
no dialogue was added or reworded in this pass; stock lines and yours are word for word.

## How it was done

- **The rules:** `docs/cutscenes.md` (improve, do not replace; what is DBZ and what is cheesy; dynamic by who plays; the
  toolkit; research then development). Every later session works from it.
- **Research:** three researchers filmed and rated 95 scenes, stock ones included, against how the anime staged the
  same moment: `docs/cutscenes/batch1_ch1-4.md`, `batch2_ch5-9.md`, `batch3_games-postgame.md` (each scene's shot list,
  its lineups, what was built).
- **The fight toolkit (`CoopCall 37`, `src/coop/cine.c`):** afterimage dashes, light and heavy strikes (wind-up, impact
  star, flash, hit-stop, knock-back in an arc, landing in smoke), exchange flurries with clashes and vanishing,
  power-ups with rings and rumble, single / volley / beam blasts, a beam struggle, kneel / fall / rise, everyone
  turning to the speaker in one call, a camera that follows the fight. Every scene's hits look alike because they use
  it. Player 1's Start skips a move in progress.
- **Development:** one developer a batch. Two of the three stopped early (the model's credits ran out); their work was
  committed scene by scene, and I merged and tested it.

## What changed, by chapter

**Chapters 1-4**
- **Trunks against Frieza and King Cold:** his Super Saiyan is a charge with rings and a burst; the soldiers are cut
  down with afterimage dashes and heavy strikes; the fight has weight.
- **Gohan's death in the future:** the androids are seen (they walk in, a held look); Gohan powers up and holds 17 in an
  exchange before 18 drives him back; the blasts fall as one storm with the ground shaking.
- **Space, Vegeta and his clone:** the blows are played (a flurry from the rock's two edges, the clone behind him).
- **Goku falls, Vegeta arrives (your call: the debut):** 19's hand over Goku, then a boot from nowhere; Vegeta powers
  up and stands gold before "does a machine like yourself ever experience fear?".
- **19's end:** 19 reels and runs, Vegeta's palm, the big blast, the explosions as its impact; "Silly robot..." now
  right after 19 blows up.
- **The road's end, "Who's next?":** the anime's fight the stock hid in a black fade, about 13 seconds, silent:
  Vegeta's charge and exchange with 18, and the others.
- **The lab door:** weight on your approved flow (17's blast throws Gero; 13's blasts land in dust).
- **Finished afterwards:** Goku's fall to the virus (an exchange he wins first, a jolt at each flinch, a knee before the
  collapse), the training talk (Gohan's blow puts Trunks on a knee; old Hercule backs off and winds up at the boulder),
  the knock-out (the chop has weight; your two lines word for word), Krillin's visit, the forest spirit (silence and a
  rumble before the white), Goku's arrival (everyone looks up, a flash, dust on the pod), the mansion deal (Hercule
  walks round Piccolo looking for wires), King Triceratops (he stamps and steps in), Gero's stages, the road's
  aftermath (Vegeta's power bursts as he takes off, Piccolo shakes the ground).
- **Small ones:** Amenbo Island (20's power, a volley at the city), the zone 7 cutaway (Goku's power shown before the
  gold), Gero's generators (13 lands with dust and a shake), the lab cutaway (a shake as 17 crushes the controller),
  the road (Vegeta lands hard).

**Chapters 5-9 and Broly**
- **Kami's boost and the fusion:** Kami glides into Piccolo, the burst, Hercule thrown back, Popo watching.
- **Gingertown's end:** Cell charges through the Kamehameha, the beam holds on Piccolo. *For your review:* Cell
  escapes with a blinding light (the anime's Solar Flare, no line); drop that one commit to remove it.
- **The island:** the break plays on marks with the camera on the speakers; Tien's Tri-Beam from above hammers Cell
  flat again and again until Tien drops, and Goku's rescue.
- **Perfect Cell:** 16 struck down, 18 cut off, the change under light and dust, a white, silence, his back to us; the
  kick that does nothing. *For your review:* Vegeta's Final Flash between the kick and Cell's lesson (Cell shrugs it
  off; the story is unchanged); drop that one commit to remove it.
- **Vegeta's power-up in zone 12, Trunks on a knee under Cell, the Time Chamber duel** (Vegeta answers with his power
  before his line).
- **16's anger at Kame House** (he stamps up, the ground answers, 17 gives him room).
- **Cooler:** the players walk up into view, he turns; his end is seen.
- **Broly:** the basin empty, the ground shudders, the camera finds him powering up with his back turned; his rage at
  his father's body in green light; his end (his power tears him apart, a knee, the white). *For your review:* Vegeta
  drops to one knee at Broly's power and gets up on his own; drop that one commit to remove it.
- **Finished afterwards** (a fourth developer took up what the stopped ones left): Gingertown (Cell turns slowly with his
  back to them, his tail up on the hiss, Piccolo powers up, the town blown flat, the crater held; a real crash with
  Hercule as player 1 and Piccolo as player 2 found and fixed there), Cooler powers up before his fight, Vegeta's first
  blow on Semi-Perfect Cell (a blink and a heavy body blow, Cell on a knee), Trunks' two misses on Perfect Cell, the
  island's arrival (the ground trembles first, Piccolo's volley, Cell's blink and one blow lays him out, 16 throws
  Cell, Goku's Instant Transmission takes player 2 too), Hercule cowering at 16's stomp at Kame House.

**The Cell Games, the post-game, the summit**
- **Fixed (they were wrong pictures):** the talk show starred whoever player 1 was (Piccolo's body under "HERCULE: OH
  YEAH!!!!!"); the finale showed two Piccolos or two Krillins and Gohan's lines with no Gohan; Gohan's own text box
  covered him before Super Saiyan 2; fight 2's end placed nobody; 18 walked off the screen by herself; Cell flashed red
  at each of Hercule's 25 punches; the players stood below the view through the whole summit scene.
- **16's head and Super Saiyan 2:** Cell is on the head in an afterimage dash and crushes it; the music stops; two
  tremors with dust, Cell backing off, a silence, the white, Gohan under flickering light. *For your review:* the
  scream is read before the white-out (no words change).
- **The beam struggle:** the clash moves (toward Gohan, back on Hercule's line, into the Cells), Vegeta's Big Bang is
  seen landing, one wave into white. *For your review:* Goku's spirit flickers at Gohan's shoulder while his lines are
  open. Both review items are switches in `tools/gen_cell_games.py`, one commit each. A real crash was found and fixed
  here (a text box destroyed the held beams; one player crashed at the struggle's end).
- **"I give up":** Goku lowers his guard, the crowd turns to a Hercule who was leaving, Cell steps up, Hercule hides
  behind Goku, Gohan lands between them.
- **Cell's return, Vegeta's charge** (dash, two volleys, Cell lays him out, Gohan dashes into the shot), **the bomb and
  Goku's goodbye** (three pulses, a tremor under every line, they flicker out, silence, "DADDY!!!" shakes the ground).
- **The ring** (the blow has weight, the ring is seen breaking), **the aftermath** (Gohan lies spent, Vegeta apart,
  Hercule walks into the middle and nobody turns), **the announcer's chop**.
- **The summit:** 17 and 13 trade a flurry, 13 throws 17 down and absorbs him, takes 18's volley and absorbs her, three
  surges blow the snow out, Super 13 powers up as the white lifts; Tien and Yamcha's blasts do nothing and his one
  blast lays them at the rim. A retry plays a short form of about 8 seconds.
- **The post-game:** the wishes, Hercule's stomach mid-brag, the explosion (Piccolo senses it first), the epilogue (old
  Hercule throws the Megaton Punch at the air).

## Art

- **Android 13's portrait:** Buu's Fury has no 13 or Super 13 face (all 155 portraits were checked). His portrait is
  rebuilt from Tien's three-quarter head under a yellow trucker cap that says **13**, long white hair, a smirk, his
  vest. His body's cap is redrawn to match, on every frame. Look at it next to Tien's, who is also a tournament fighter.
- **Hercule** flies sideways properly (he slid sideways facing the camera); **Super Saiyan Hercule** has a beam pose;
  **Mr. Popo's** attacks show from the side; **Android 16's** borrowed frames are cleaned; **Super 13's** sash and cuffs
  are the right colours; the Chamber's **EXIT sign** is tidier. The full audit is `docs/art-audit.md`.
- **Not done:** Super 13's portrait is still Android 16's face in blue (three tries did not look good enough to keep).

## For you

1. **Watch the showcases** (F1-F9, `build/showcase/SHOWCASE.txt` once they are installed) and say what reads well and
   what does not; each scene is small commits, easy to tune or drop.
2. **The five review items:** the Solar Flare, the Final Flash, Vegeta's knee before Broly, the scream before the
   white, Goku's spirit in the struggle.
3. **Best Shorts, by the developers:** Trunks against Frieza (about 35 s), Perfect Cell (about 35 s, same whoever plays),
   16's head into Super Saiyan 2 (about 30 s), the road's end (13 s).
4. **Sound:** nobody has heard the new impacts and power-ups in place.
5. **Still not built** (each scene's note in `docs/cutscenes/` says why): the five fighters getting up after the road's
   fight (they are map decorations), the rest of Goku's arrival talk, two small beats in the forest and the jungle,
   Hercule hopping behind Piccolo, and the batch 3 touch-ups listed in its research file.
