# Cutscene research, batch 3: the Cell Games and after (2026-11-06)

Research only: no game data was changed. Format: `docs/cutscenes.md` section 6. Every scene below was filmed headless on
the reference ROM (`build/ref/log2-coop.gba`, the same bytes as `build/bal`) and the frames were looked at; what could not
be filmed is in section 9. Scratch (films, sheets, scripts): `build/research3/` (`filmlib.py`, `games.py`, `refilm.py`,
`post.py`, `extract.py`; sheets are `sheet_*.png` in each sub-folder).

Ratings: **leave**, **touch up** (a few steps in entries that exist), **rework** (new choreography, lineup branches),
**showcase** (worth a 20-40 s Short). Sizes: S (a few steps, one entry), M (one or two entries of new choreography, a
lineup test), L (needs the cinematic helpers or C, several lineups, OBJ VRAM to measure).

Shot lists name the script steps of `docs/cutscenes.md` section 5 and the cinematic helpers by the names they were
announced under (`cine:` afterimage dash, strike light / heavy with knock-back, exchange flurry, charge-up, blast single /
volley / beam, beam struggle, rise / kneel / fall, look-at). Positions are map pixels taken from the scripts.

## 0. Read first: three defects the films showed

These are not staging taste; they are wrong pictures. All three are in the ending, which every player sees last.

1. **The talk show stars whoever player 1 is.** `pg_dende` runs the stock talk show (`stockseq @083c4e98 1 28`) on
   char 0. The stock game only let Hercule through that door; ours comes from Dende's question with anyone. Filmed with
   player 1 Piccolo: Piccolo's body walks through the audience and poses, and the box reads "HERCULE: OH YEAH!!!!!"
   beside Piccolo's portrait (`build/research3/pg/zoom_ts.png`). Fix (S): make Hercule player 1 behind the fade that is
   already there (`SetActiveCharacter 5` before `WarpToMapNoEntities`, the way the wishes make Gohan player 1). Nothing
   needs restoring: the scene ends in the credits and the title screen.
   **Fixed (`nb-cs3`, 2026-11-06):** built as proposed (`gen_postgame.py`, the talk show's entry 0); nothing saves between the question
   and the title screen (`PlayCredits`, flag 196, `ReturnToTitleScreen`). Test: `staging_postgame.txt` `ts_*` (four lineups: one Hercule, player 1 = slot 5).
2. **The finale doubles player 1 and leaves Gohan's lines without a body.** The finale warps to the Lookout's edge
   (zone 9 area 7) with the stock actors standing (Piccolo 103, Krillin 76, Vegeta 137, Trunks 132, Tien, Yamcha, Dende,
   Popo) and player 1 as he is. Filmed as Piccolo: two Piccolos, and "Finally...  some time to relax." (player 1,
   Piccolo's portrait) is answered by "PICCOLO: Peace...  for now." (`pg/zoom_fin.png`). Filmed alone as Krillin: two
   Krillins (`pga/zoom_pga.png`). In both, Gohan's stock lines ("Will you be staying here Piccolo?", "Goodbye!") and the
   lines to him ("You take good care of your mother Gohan!", "Goodbye Gohan!  Be sure to visit!") play with no Gohan on
   the map. As Vegeta the opening line is out of voice. Scene 5.5 has the rework.
   **Fixed (`nb-cs3`, 2026-11-06):** scene 5.5, "Built".
3. **Player 1's own text box hides him in the rage before Super Saiyan 2.** Gohan stands in the top third of the view
   and his lines open at the top: for "You loved life!...", "I just let you die!" and "I won't watch this ANYMORE!!!!!!"
   the box sits on his body (`head/sheet_head_00.png`, first row). Fixed by the reframing in scene 3.6.
   **Fixed (`nb-cs3`, 2026-11-06):** scene 3.6, "Built": from "You loved life!..." to the transformation the view is 56 px higher.

A fourth, smaller: in fight 2's end nobody is placed, so the pans that follow are aimed at where Goku and Cell were
assumed to stand. Filmed after a fight that ended elsewhere: "CELL: This is your replacement, Goku?" shows Hercule alone
and no Cell (`gc/zoom_gc1.png`, first tile). Scene 3.3, beat 1.
**Fixed (`nb-cs3`, 2026-11-06):** behind a flash Goku and Cell stand at (368, 440) and (484, 440), dust between them, and the view that comes back
from the plateau frames both (centred on Goku it cut Cell at its edge). Test: `cell_staging.txt` ends fight 2 with player 1 poked to (250, 330) and
expects him at (368, 440) once the scene has begun, in its four Games lineups.

## 1. Lineups of this batch

| part | player 1 | player 2 | what a scene can count on |
|---|---|---|---|
| the Games (zone 14 areas 2-4) | scripted per fight: Goku, Goku, Gohan, Krillin, Piccolo, Vegeta, Trunks, Gohan (Super Saiyan 2), Gohan | no entity in any scene (`CoopCall 15` solo modes, warps without entities); bound to Hercule for fights 3, 7, 8 (`CoopCall 6, 5`); his pad drives player 1's body in fight 2, J2, J4 | **two lineups only: alone, and two players.** Player 2's choice at the ring changes nothing (`cell_staging_gen.py` runs Hercule, Krillin, Vegeta). Hercule is actor 72: with two players before fights 3, 7, 8, after fight 3, at the bomb and Cell's return; in both modes where the plot needs him (called out after fight 2, 16's head, the struggle, the aftermath) |
| the Lookout: wishes, explosion | Gohan (the stock entry makes him player 1; `CoopCall 10` puts the party back after) | no entity (warp without entities) | one staging; the stock actors are the story's cast |
| the Lookout: finale | whoever (Gohan, Piccolo, Vegeta, Trunks, haloed Goku, Hercule, Krillin, Super Saiyan 2 Gohan) | no entity | broken today for everyone but Gohan (section 0) |
| the summit (zone 15 area 8) | any of the eight | any other | no party watcher ever stands (`watch_*` push 0); Tien 121 and Yamcha 156 always; 13, 17, 18 |
| the talk show, the epilogue | must be Hercule (it is not, section 0); the epilogue has no player | none | one staging |
| Papaya Island, the Time Chamber | any | any | staged per lineup already |

Kept for the plot (from `docs/story.md` 0): Hercule at the Games' four plot beats alone too; everyone the stock wish scene
stands; Tien and Yamcha at the summit.

## 2. Before the Games (stock, never touched)

### 2.1 Dende takes the Dragon Balls (map 261, record 3 `m261_r3_talk`, entries 3-7)

- **Today (read from the dump, not filmed):** with seven balls, if player 1 is not Goku a Goku actor (60) flies in from
  below, "DENDE: You've found all the Dragon Balls!", "GOKU: Now all that remains is to finish up any training we still
  need to do.  Once we're ready, let's head for the Cell Games Arena.", he flies off, "Journal Updated!", flag 191.
- **Lineups:** any pair. The stock test is only "player 1 is Goku": with player 2 on Goku a second Goku flies in, and
  alone as anyone else an unplayed party member flies in for one line and leaves (the pattern Ryan cut on 2026-11-05).
- **Anime:** no such scene.
- **Rating: touch up (S).** Guard the fly-in with `CoopCall 22, 4, 4, 0` (Goku played by either player: no actor, the line
  is his where he stands). Nobody plays Goku: Ryan's call between keeping the stock arrival (a story arrival, like the
  road's ending) and Dende saying it; no draft offered, the line names its speaker.
- **Risk:** `m261_r3_talk` returns absolute indexes (entry 0 pushes 9 / 3 / 1): restate with `stockseq`, new entries after
  the end.

### 2.2 The arena grounds (map 323)

- **Today (one frame filmed):** ten NPC talks with one line each around the path, the barrier ("Only Goku is allowed to
  pass through this barrier."), the flight spot. Anime: the fighters wait by the ring. **Leave**: talkable NPCs, nothing
  to stage; the scene proper starts in the ring.

## 3. The Cell Games (`data/story/cell_games.lscript`, generated by `tools/gen_cell_games.py`)

All edits go in the generator. It restates stock entries by `body(A3, name, index)` and inserts by `b.index("exact step")`:
a step that is moved or removed breaks an insertion further down, so change one block, regenerate, diff. Entries inside a
`stockseq` range (`cg_ring` 1-26, most talk) must first be split out with `body()` to be edited.

### 3.1 The ring: the walk-in and Hercule's turn (`cg_ring`; map 324 trigger 0; generator block "map 324: the ring")

- **Today (filmed, `gc` and `ringh`):** Goku walks up, the others walk in from the bottom in pairs (Gohan and Piccolo,
  Vegeta and Trunks, Krillin). "#GOKU: I'll fight Cell first." The camera goes to Hercule walking in: "HERCULE: Whoa!
  Wait a second here!  I've had enough of you clowns and your stupid jokes.  Clear out of here before I get angry!",
  "KRILLIN: Don't blow a vein, grease-ball!" (Krillin hops, Hercule struts over), the "banjo" speech, the announcer,
  "It's payback time!...", the "mirrors and magnets" speech, a taunt pose, "LET'S GO!!!!!!". He runs up the ring with the
  camera and punches: 10 random attack poses, each with a hit sound and Cell's damage flash, "CELL: Your ignorance is
  painful.", 10 more, the announcer ("This man never rests!"), 5 more, "And now for my finishing move!", Cell's one
  blow, Hercule glides 400 px down off the ring with the camera on him, everyone turns to watch, a 2 s hold,
  "To be honest... I was rooting for Cell that time.", Cell calls Goku, Goku walks up, "#Let's do this!", the card.
- **What the film shows:** Hercule stands on Cell's own spot ((448, 440) against Cell's (440, 432)): his body sits inside
  Cell's for the whole flurry. Cell flashes red 25 times, which reads as Cell being hurt by punches that "do nothing".
  The blow itself has no flash, no shake, and nothing marks where he lands.
- **Lineups:** one staging (Goku is made player 1, player 2 hidden; Hercule is the stock actor even when player 2 chose
  him).
- **Anime:** Hercule's flurry and kicks land on a Cell who does not move or look at him; one backhand sends him into a
  rock face. The game has the right beats already.
- **Rating: touch up (M, because entries 15-21 leave the `stockseq`), a comedy Short candidate.**
- **Shot list** (all stock lines verbatim, none added):
  1. The run-up ends 14 px below Cell (`WalkToPosition 72, 448, 454`), facing up: two bodies, not one.
  2. The three flurries: drop `PlayDamageEffect 30` (Cell never flashes); keep the poses and sounds; `SpawnDustCloud` at
     Hercule's fist every third punch. Cell does one thing per flurry: nothing; then turns his head away
     (`SetEntityFacing 30, 2`) before "Your ignorance is painful."; then looks back down at him during the announcer.
  3. "And now for my finishing move!": Hercule backs off three steps (a walk down, facing up), holds his taunt pose 20
     frames, charges.
  4. `cine: strike heavy` Cell on Hercule: `WhiteFlash 3`, `ScreenShake 0, 10, 12`, the stock hit sound, then the stock
     glide with the camera.
  5. Landing: `SpawnDustCloud` twice and `ScreenShake 0, 6, 10` where he stops; the watchers' heads follow him down
     (stock batch), then turn back up to the ring one by one during the hold. Krillin's line as stock.
- **Built (`nb-cs3`, 2026-11-06), beats 1-5:** Hercule stops under Cell at (446, 466); the 25 punches keep their poses and sounds, flash nothing on
  Cell, and puff dust at his fists every third; Cell looks away before "Your ignorance is painful." and back down during the announcer; before the
  finishing move he backs off, poses (8) and charges; the blow has a flash and a shake, the landing a boom, a shake and two dust clouds; the watchers
  look back up one by one inside the stock 120-frame hold (no time added there). The punches are paced by their poses (`EntityWaitTouchPlayer` needs
  the overlap that is gone). Entries 15-21 are restated; indexes unchanged. Filmed; `cell_staging.txt` and `cell_games.txt` pass through it.
- **Risks:** the glide's `GlideToPosition 72, 448, 850, 1000` and `SetActiveEntity 72` are in one batch with Cell's pose;
  keep the batch. `EntityWaitTouchPlayer 30` paces the punches: with Hercule no longer overlapping Cell it may never
  fire; use fixed waits instead. The ring scene is the Games' first: `cell_games.txt` and `cell_staging.txt` reach it by
  walking up, then `stepz`; a longer scene only moves their frame counts.

### 3.2 Fight 1's end: Cell destroys the ring (`cg_fight1_end`; record 1's defeat on map 324)

- **Today (filmed):** a flash, "CELL: I want to make this fight a little more interesting.  I'm tired of being confined to
  this little ring.", one ki blast at the floor with a dust cloud and two sounds, and 10 frames later the screen is white
  and the desert has loaded. The blast is on screen for about a third of a second; the ring is never seen breaking. Then
  "How do you like that Goku?  Now the whole desert is our ring.  Let's say the last one standing wins the game!", the card
  ("Player 2 fights!" / "Goku fights!"), fight 2.
- **Lineups:** one staging (player 1 Goku, wherever the fight ended).
- **Anime:** Cell rises above the ring, tells everyone to get clear, and levels it with one blast.
- **Rating: touch up (S).**
- **Shot list:** 1. After his line Cell flies up 48 px (`FlyToPosition 30`; no stock scene flies sprite 30, so
  test the group: see risks). 2. `cine: charge-up` on Cell, 30 frames, `ScreenShake 0, 4, 30`; Goku looks up
  (`SetEntityFacing 0, 1`). 3. `cine: blast single` straight down at the ring's centre (the stock `NPCFireKiBlast 30,
  456, 464, 140`). 4. Impact: `WhiteFlash 8`, `ScreenShake 0, 20, 40`, four `SpawnDustCloud` across the tiles, the stock
  sounds 20 / 21. 5. Only then `FadeToWhite 30` (was 10) into the stock warp. About +2 s.
- **Built (`nb-cs3`, 2026-11-06):** behind the stock flash Cell stands at (456, 440) and Goku at (456, 504) with the view on the ring's middle (the
  stock placed nobody here either); after his line Cell flies up to (456, 404) (**sprite 30 has the flight group: seen**), holds his charge pose
  with the aura's sound and a tremor for 34 frames, fires the stock blast at the ring's middle; a flash, the biggest shake, five dust clouds across
  the tiles, the stock sounds, and a 30-frame white (was 10). About +1.5 s.
- **Risks:** whether Perfect Cell's sprite 30 has a flight group is not recorded (`docs/sprites.md` lists groups 0-3, 5-8,
  12, 18 from the Super 13 fallback note); if not, he stays on the floor and fires at his feet. Goku's position is
  wherever the fight ended: Cell may be anywhere on the ring, so aim the dust clouds by the ring's own coordinates.

### 3.3 Fight 2's end: "I give up" and Hercule called out (`cg_fight2_end`; record 0's defeat on map 325) **SHOWCASE**

- **Today (filmed, `gc`, `giveup2`, alone in `ga2`):** wherever the fight ended, with no pose and no placing: "#GOKU: It's
  over Cell. You win.  I give up.  I know how strong you are and there is no point in continuing this fight.  I can't beat
  you." A pan to the plateau: Vegeta hops, "VEGETA: You coward!!!", Trunks hops, "TRUNKS: Don't do it!  Don't give up!",
  back. "CELL: Surely you're joking Goku.  You can't just give up and expect to walk away!", "#Sure I can.  You beat me fair
  and square.  I give up!  But that doesn't mean the tournament is over.  There's still one more person for you to
  fight.", "You're not serious!  One of them?  And exactly which one of these pitiful specimens did you have in mind?",
  **"#I need someone to take over...  ...Hercule!  Get out here!"** The camera climbs to the crowd, Hercule walks in from
  the right edge of the plateau and all the way down to the floor with the camera on him: about 6 s of walking and
  nothing else. "HERCULE: Heh...  I only slipped last time, you know!", a hurt pose, "Ooh...  ohhh, my stomach!  Must have
  been something I ate!  The champ can't fight like this!", a pan, "CELL: This is your replacement, Goku?  The clown I
  threw out of the ring?", up to the plateau, "GOHAN: I can't let Hercule face Cell alone!", Gohan flies down with the
  camera, "HERCULE: Well...  since the kid insists...  stand back, boy, and watch a professional!", "Wonderful.  Let's get
  started!", a fade, the warp, the card, fight 3.
- **What the film shows:** the pans are absolute and nobody is placed (section 0); Goku surrenders in his fighting
  stance; the crowd does not react to Hercule's name; the walk is the longest dead stretch of the Games; Cell never
  moves; alone, Hercule says "watch a professional!" and is simply gone after the fade.
- **Lineups:** identical until the fade (player 1 is Goku, driven by player 2's pad with two players, so player 2's A
  advances the text). After it: two players, Hercule's actor at (356, 440) beside Gohan and player 2 takes him; alone, no
  Hercule.
- **Anime:** Goku at full power suddenly relaxes and smiles, "I give up"; every fighter on the cliff is stunned, Vegeta
  furious; he names his successor to general disbelief; Cell is offended. Then he asks Krillin for a Senzu Bean and
  throws it to Cell so the fight is fair. Ours names Hercule first (Ryan, 4h), which is the mod's own joke; the staging
  to borrow is the calm of Goku, the cliff's shock and Cell's contempt.
- **Shot list** (lines verbatim; Hercule's excuse is Ryan's own addition, the rest are the 4h plan's lines as built):
  1. **Place the two** behind the stock opening sound: `WhiteFlash 6`, `SetEntityPositionInstant 0, 368, 440`, Cell to
     (484, 440), facings 3 / 2, `PanCameraToPosition 306, 356, 3` (the generator's own comment assumes these spots). Two
     `SpawnDustCloud` between them: the last exchange just ended.
  2. Goku drops his guard: idle pose, a half step back, then the stock line. Cell holds a charge pose
     (`SetEntityAnimationNoWait 30, 5`) and lowers it on "Surely you're joking".
  3. The plateau (stock pan): on "You coward!!!" Vegeta takes two steps to the edge after his hop; Piccolo and Krillin
     turn to each other (`SetEntityFacing`), a small move each.
  4. "...Hercule!  Get out here!": `cine: look-at`, the whole plateau turns right, to where he is. The camera finds him
     already there, at the far right end, facing away, mid-step toward the rocks (one `WalkToPosition` away from the
     crowd as the camera arrives): he was leaving. He stops, hops (pose 4), turns his head left, right, then points
     at himself by facing down. No line.
  5. **Cut the walk.** `FadeToWhite 8`, Hercule to (424, 380), camera to the floor, `FadeFromWhite 8`; he walks the last
     60 px in three short starts and stops, to (424, 440) between them. Saves about 4 s.
  6. "Heh...  I only slipped last time, you know!" with his taunt pose (the ring scene's pose 8), then the stock hurt
     pose and the stomach line.
  7. Cell's line: Cell takes two steps toward him first (`WalkToPosition 30, 460, 440`); Hercule scuttles round behind
     Goku (a walk to (348, 440)) during the box; Goku stays where he is, facing Cell.
  8. Gohan (stock pan and flight): he lands between Hercule and Cell at (400, 432), `SpawnDustCloud`, facing Cell; on the
     plateau Piccolo starts forward one step and stops (the stock protest lines are gone from our version; the step
     keeps the worry).
  9. "Well...  since the kid insists...": Hercule comes out from behind Goku to Gohan's left shoulder, taunt pose on
     "watch a professional!".
  10. **Alone only:** during "Wonderful.  Let's get started!" Hercule backs away three steps and runs off to the right
      (the way he hides later, `WalkToPosition 72, 640, 240` cut by the fade), so the empty field after the fade is
      explained. Two players: he stays, as today.
- **Built (`nb-cs3`, 2026-11-06), beats 1-10 with script steps (look-at by facings):** as listed, with these differences: Piccolo and Krillin are not
  in the plateau's first view (Vegeta and Trunks only), so beat 3 is Vegeta coming to the edge; the crowd that turns at Hercule's name is 16,
  Krillin, Gohan and Piccolo (16's actor guarded: record 7 has a condition); Hercule is found at the far right walking away, starts (pose 4), looks
  left, right, down, then the white cut and three steps to (424, 440) and his taunt (8); Cell steps to (458, 440) and Hercule goes round behind Goku
  **before** Cell's box (a script cannot move under an open box); Gohan lands at (412, 432) with dust; alone, Hercule turns and runs off the left
  edge before the fade (running right would have taken him past Cell). The pan "over to Cell" is gone: the floor view holds all of them.
  The Senzu Bean was not built (decided). Filmed with two players and alone; `cell_staging.txt` passes through it.
- **Optional, Ryan decides (a beat the stock game lacks):** the Senzu. Cell starts fight 3 on a new stat at a full bar
  (`DrawBossHealthBarRange 30, 60, 100`), which the bean would explain. After beat 8: Goku tosses it (`NPCFireKiBlast`
  has no bean art; a pick-up sound and Cell's catch pose would carry it), Cell's heal sound and a flash, Vegeta's hop.
  DRAFT line if wanted: `GOKU: "Cell.  Catch.  A Senzu Bean...  so it's a fair fight."` Not in the default build.
- **Risks:** the map reloads at the fade (`WarpToMapNoEntities 14, 3`): record 0's Cell and the plateau's records come
  back, actors go (the generator spawns Hercule again there). Hercule's sprite 72 is only ever walked here ("his fly pose
  is a stub", generator header): no flight. In a two-player game this scene is advanced by player 2's A only. The text
  box is at the bottom; the floor action is at screen row 80 with the camera at y 356-360, clear of it.
- **Size: M.** As a Short: beats 1-9 run about 35 s read fast; it is the clip that says what this mod is, and it shows
  Hercule as the coward everyone takes him for.

### 3.4 Fight 3's end: Cell and Gohan, 16's last stand (`cg_fight3_end` entries 0-25; record 5's defeat)

- **Today (filmed, `gc`, `s16`):** a fade and reload; Gohan and Cell face each other (two players: Hercule's actor flat on
  his back beyond Cell). "!CELL: I know I'm hurting you Gohan...", Gohan's hidden-power speech (five pages), "Interesting...
  I'm determined to see it for myself!". A pan to the plateau: "PICCOLO: No!  I can't take it!", "You can just stand there
  if you want, but I'm helping Gohan!", "GOKU: No you're not!...", "And when that happens you watch!...". "ANDROID 16: Let
  me handle this.": 16 flies down in two hops with the camera, lands on Cell (their sprites overlap; nothing shows a
  hold), his speech (three boxes), "KRILLIN: Wait!  You can't 16!  Your bomb was removed!...", "That's too bad 16...  But
  if you have a death wish, I won't let you down!", three point-blank blasts and the stock's best effect of the Games:
  four batches of flashes, sounds and 16 thrown about by velocity jitter, about 2.5 s, then he is gone. "Does anyone else
  want to be a martyr?", the camera settles, "Hrmm... Still not angry enough Gohan?  Well maybe this will help."
- **Lineups:** alone: no Hercule. Two players: his actor lies at (540, 400) through all of it and does not move.
- **Anime:** 16 seizes Cell from behind in a bear hug; nothing happens; Krillin tells him; Cell turns in the hold and
  blows him apart with one blast; the head lands near Hercule. Gohan, on his knees, watches.
- **Rating: touch up (M).** The explosion already has weight; the hold and the silence are missing.
- **Shot list** (stock lines verbatim):
  1. Gohan is down on one knee when the fade lifts (`cine: kneel` on char 0) and rises during his own speech: the lines
     are about being pushed too far.
  2. 16's second hop lands **behind** Cell (8 px to the side Cell faces away from), both facing the same way; Cell
     jitters (`EntitySetVelocity` 2 px left and right in a batch) with `ScreenShake 0, 3, ...` under 16's speech: a hold.
  3. Krillin's line; then 40 frames with no box: the jitter and shake stop, 16 turns his head up to the plateau
     (`SetEntityFacing 8, 1`), Cell turns his head back at him (`SetEntityFacing 30, 1`).
  4. Cell's line; `cine: charge-up` 20 frames; one `cine: blast single` (`WhiteFlash 10`); then the stock explosion
     batches unchanged.
  5. One piece flies: a dust trail of three `SpawnDustCloud` from the blast toward the rocks at (640, 240), where Hercule
     will "supervise from over there" and where the head scene opens. A thud sound. No line.
  6. Two players: Hercule's lying actor flinches at the blast (hurt pose 2, back to lying 3). Gohan: the stock hop, then
     `cine: kneel` again for "Does anyone else want to be a martyr?".
- **Risks:** 16's actor sprite 8 has seven groups (idle, walk, hit-stun 2-4, punch 6, knock-down 12); the stock explosion
  plays 2 on it, nothing else is safe to add. Entry 21 is one long script of batches with pushes on the VM stack
  (`PushMulti`, `Negate`): insert before or after it, never inside.

### 3.5 The Cell Juniors and the four solo intros (`cg_fight3_end` entries 26 on, `cg_j1_end` .. `cg_j3_end`)

- **Today (filmed):** six Juniors appear one by one, each in a 5-frame white blink 40 frames apart, in two rows above
  Gohan; "What the heck are those things?", "My children!  Give them pain!". Two players: Hercule gets up, "HERCULE:
  Uh...  this fight is a little too much for the champ right now.  I'll, uh...  supervise from over there!", walks off to
  the rocks. Then, four times: a fade, the next fighter (Krillin, Piccolo, Vegeta, Trunks) at the plateau's foot, he
  flies down 64 px, the card, the fight.
- **Lineups:** alone, no Hercule beat (the `duo_only` jump). The fighters are char 0.
- **Anime:** Cell strains and seven small blue Cells crawl out of his tail, giggling; they tear into the fighters while
  Gohan is made to watch.
- **Rating: touch up (S).**
- **Shot list:** 1. Cell holds a strain pose through the births (`SetEntityAnimationNoWait 30, 5`), a `ScreenShake 0, 4,
  8` and a pop sound with each blink; Gohan steps back one step at the third. 2. After "Give them pain!" Cell points up
  (`SetEntityFacing 30, 1`, stock) and the fade begins on that: no new wait. 3. Each intro: the Junior hops once (a
  2 px `EntitySetVelocity` up and down) as the fighter lands, a `SpawnDustCloud` at the landing. 4. Hercule's walk-off:
  a look back at Gohan before he goes (one facing change).
- **Risk (a real one):** all six Juniors show sprite 31, and a sprite-addressed step finds only the first (`docs/howto.md`
  "Stage a scene"): they cannot be moved or posed one by one, which is why the list above only poses Cell. The Junior in
  an intro is the map's enemy record (`TriggerMapEvent 8 / 9 / 10 / 13`), a single body: the hop is safe there.

### 3.6 16's head and Gohan's Super Saiyan 2 (`cg_head_scene`; record 10's defeat) **SHOWCASE, the batch's best**

- **Today (filmed, `gc`, `head`):** a fade and reload; the camera on the plateau's rocks: Hercule alone, a small orange
  object beside him. "ANDROID 16: You!" (16's broken-head portrait), Hercule looks left, right, down, "HERCULE: Who said
  that?", "Please help me.  Take me over to the boy!", "You must be nuts!  Cell is out there!", "Please!  It is for the
  good of the Earth!", "Grrrrr!!!  Enough!  I will help you!  It would be shameful for a world champion to run away."
  He walks up a little, then all the way down to Gohan with the camera, about 7 s, and walks off again; nothing shows a
  head being carried or set down, it is beside Gohan afterwards. "Gohan.", 16's speech (four pages), "The birds I watched
  with Krillin...  protect them, Gohan.", "Ewww...  This sentimental downpour is killing me!  It's so nice of you to
  help, 16, but I think I'll do this my way!", "Gohan, you have the strength.  My scanners could sense it.  Just let it
  go." Cell fires one blast from where he stands, a dust cloud. "Yet another fighter you could have saved, Gohan.",
  "@ANDROID 16!!!!!!!", "You loved life!  You gave everything for it.  And you were just an Android!  I just let you
  die!", "I won't watch this ANYMORE!!!!!!". **The transformation: one second of his power-up pose, a 10-frame white, and
  the sprite is Super Saiyan 2.** No shake, no dust, no hold; Cell does not react. "AHHHHHHHHHHH!!!!!!!!!!!!!!!!!!!" (his
  new portrait), "@^Gohan has entered Super Saiyan Rage!", "You wretched child!!!  NOW TASTE MY TRUE POWER!!!!!!". Two
  players: Hercule walks back in to Gohan's left, "HERCULE: The champ is back!  Let's finish this together, kid!". The
  card, fight 7.
- **What the film shows:** the saga's biggest moment is its shortest scripted beat, played in the top third of the view
  behind Gohan's own text box (section 0); Cell kills 16 from a distance without moving; the walk is another long quiet
  stretch.
- **Lineups:** identical until the last beat (Gohan is made player 1, player 2 hidden; Hercule carries the head in both
  modes, kept for the plot). Two players: Hercule returns with his line. Alone: he stays away; the card follows
  Cell's line.
- **Anime:** Hercule, shaking, throws the head to Gohan's feet. 16 speaks; Cell walks up and crushes the head under his
  foot in mid-sentence. Silence; 16's words again; a tear; Gohan's scream splits the ground, the aura climbs in a
  pillar, rocks lift, lightning; then he stands quiet, hair up, one lock down, crying, and says nothing. Cell is
  pleased, then uneasy.
- **Shot list** (every line verbatim; "The birds..." and Hercule's last are the 4h additions as built):
  1. Opening as today. Hercule's looks are good; add `cine: look-at` so he ends facing the object for "Take me over to
     the boy!".
  2. **Shorten the carry.** He walks to the plateau's edge (the stock's first leg, to (600, 200)), a white cut
     (`FadeToWhite 8`, Hercule to (616, 320), camera to Gohan, `FadeFromWhite 8`), he comes down the last 56 px in
     nervous short steps, stops a body's length from Gohan, sets it down (`SpawnDustCloud`, a soft thud), backs away
     three steps facing Cell, turns and runs. Saves about 4 s, which the transformation spends.
  3. 16's speech: Gohan faces the head (stock); Cell comes closer by one step per page of 16's (three `WalkToPosition
     30` legs of 16 px): the threat arrives while the kindest lines play. Camera: `PanCameraToPosition` so Gohan stands
     at screen row 96, not 45, from here to the fight; his boxes then open above him, not on him.
  4. **The kill, the anime's way, in the game's story (Cell destroys the head, as stock):** on "Just let it go." Cell is
     one step away. `cine: afterimage dash` Cell onto the head, `cine: strike heavy` down (a stomp): the stock sound,
     `WhiteFlash 4`, `ScreenShake 0, 8, 12`, `SpawnDustCloud`, then the stock removal of the head and `StopMusic`.
     Fallback if the dash reads badly on Cell: the stock blast, fired from one step away with a `cine: charge-up` first.
  5. Thirty frames of nothing. Then "Yet another fighter you could have saved, Gohan." as Cell walks back past him
     without looking at him (a walk to his stock spot, facing away).
  6. "@ANDROID 16!!!!!!!" with the stock hop and `ScreenShake 0, 4, 20`. "You loved life!..." with the stock pose 6 and two
     `SpawnDustCloud` at his feet. "I won't watch this ANYMORE!!!!!!".
  7. **The transformation, about 7 s in place of 1:**
     a. `cine: charge-up` on char 0, stage one: the stock aura sound 55, his power-up pose held, `ScreenShake 0, 4, 40`,
        a ring of four `SpawnDustCloud` 16 px out.
     b. Stage two: `WhiteFlash 6`, `ScreenShake 0, 10, 40`, the ring again 32 px out. Cell, who was walking away, stops
        and turns round (`SetEntityFacing 30`), and takes one step **back**: the first time in the Games he gives ground.
     c. A hold: `StopAudio 55`, no shake, 20 frames. Gohan's head down (his facing-down frame).
     d. The stock step, bigger: `FadeToWhite 20`, the stock sounds 23 / 20, `SetCharacterTransformation 0, 58`, the stock
        HP / EP refill, `ScreenShake 0, 20, 60`, `WhiteFadeOut 30`.
     e. He stands still, facing Cell. Lightning as light: three `WhiteFlash 3` at uneven gaps (12, 7, 19 frames), a spark
        sound each, one `SpawnDustCloud` drifting. Nothing else moves for 30 frames.
  8. "AHHHHHHHHHHH!!!!!!!!!!!!!!!!!!!" and "@^Gohan has entered Super Saiyan Rage!" as stock. (Optional, Ryan decides: play
     the scream during 7a-b and leave only the caption after the reveal, so he is silent once transformed, as in the
     anime. It reorders two stock entries; no word changes.)
     **Built as the optional order (decided 2026-11-06; its own commit):** the scream is read during the first tremor; once transformed he is silent.
  9. Cell: `cine: charge-up` (a flash, `ScreenShake 0, 8, 20`), then "You wretched child!!!  NOW TASTE MY TRUE
     POWER!!!!!!": power shown, then spoken.
  10. **Two players:** Hercule's walk back as today, but he stumbles at the last tremor (hurt pose 2, 10 frames) before
      "The champ is back!  Let's finish this together, kid!". **Alone:** nothing; the card.
- **Built (`nb-cs3`, 2026-11-06), beats 2-10, with script steps only (no helper):** the carry is a white cut and three reluctant steps down
  from the view's top; he sets the head down (dust, a thud), looks at Cell and runs off the top. Cell comes closer in three legs between 16's
  boxes, steps onto the head and crushes it with his blow pose (6), a flash, the stock sound and dust; the music stops; he turns away, says his
  line, and walks off past Gohan. The transformation: a first tremor with a dust ring, a flash and a harder one with a wider ring while Cell
  turns round and backs off a step, a held silence with Gohan's head down, the white (20 frames, the biggest shake), then he stands still while
  three flashes flicker. Cell powers up (pose 5) before his last line. Two players: Hercule comes in from the top, stumbles at an aftershock
  (pose 2), "The champ is back!". Alone: the card. Beat 1's extra look and the afterimage dash were not built.
  **Research corrections (the boxes decide the view):** a box at the bottom covers screen rows 74-136, one at the top 0-62, and a shout
  (`@`, "ANDROID 16!!!!!!!") or a caption (`@^`) sits in the middle, rows 45-105. No single view clears them all, so the scene uses two: the
  stock height (Gohan at row 40) while 16 and Cell speak and for the shout and the caption, and 56 px higher (row 96) from "You loved
  life!..." through the transformation and "AHHH". `FindCharacterEntity 0 / 1` and `FindEntityByType 0 / 1` are how the map shows and removes
  the head (by the rocks / beside Gohan): its spot (616, 384) is the map's and was kept. A dust ring needs about 22 px of radius to read as one.
  Filmed alone and with two players (`C:/Users/ryan/AppData/Local/Temp/claude/.../scratchpad/cs3/`); `cell_staging.txt` passes through it.
  **With the helpers (after the merge of `nb-cine`):** Cell is on the head in an afterimage dash (`CoopCall 37` op 3 on sprite 30, the map's enemy
  record standing as a scene body: it works) before his blow, and his power-up before "NOW TASTE MY TRUE POWER!!!!!!" is the helpers' charge (op 7,
  level 1). Gohan's own change stays hand-built: his stock power-up pose and the white are the scene's, and a charge's burst would be a second flash.
- **Draft lines:** none.
- **Risks:** (a) Gohan may be Super Saiyan 1 (row 5, sprite 59) when this scene makes him player 1 again, if the player
  transformed in fight 3: the hair would go gold to gold. Put him on his base row before the fade lifts (the ring scene's
  `SetCharacterTransformation 0, 56`) and check that pose 9 exists on that sprite; the stock's blocking
  `SetEntityAnimation 0, 9` "never ended in co-op tests" (the generator's comment), so keep `NoWait` plus waits. (b)
  After the swap char 0 is our repointed sprite 58: hurt and lying come from 59 (Super Saiyan 1 hair for a few frames),
  transformation and fidget groups are empty (`docs/sprites.md` "Super Saiyan 2 Gohan"): pose nothing on him after 7d
  but idle, walk and facings. (c) How the head is drawn (a map decoration switched by flag 194, or record 7) was not
  traced; trace it before moving where it is set down. (d) The scene reloads the map at its start; nothing reloads
  inside it. (e) Cell is the map's enemy record brought by `TriggerMapEvent 11`, standing as a scene body: walking him
  is what the stock scenes do elsewhere (`WalkToPosition 30`), the dash is the helper's to prove. (f) `cell_staging.txt`
  presses through with `stepz`; `ssj2`-related tests start from fight 7 and are not touched.
- **Size: L** (helpers, the reframing, one lineup branch that exists already).

### 3.7 Fight 7's end, part one: 18, the bomb, Goku's goodbye (`cg_fight7_end` entries 0-33)

- **Today (filmed, `gc`):** a fade; Cell down, rises; Gohan's punch, a flash: Semi-Perfect Cell and Android 18 beside
  him. "CELL: You're not a boy!  You're a monster!", "PICCOLO: He spit out Android 18!", **18 walks off the screen by
  herself**, "I'll get you for this!!!!", "You thought you could beat me!  But you can't!  I'm going to blow myself up
  now!  Say goodbye to your precious planet!", one white fade and he is the swollen Cell (a good sprite, 27). Everyone
  flies in around him and turns to him. "TIEN: What do we do?  Someone come up with a plan!", "There's nothing we can
  do!  I'm afraid this is it.  Cell has beaten us.", "GOKU: Listen... I can only think of one way to save the Earth
  now.", the Instant Transmission line, Goku walks to Cell, "Goodbye friends!  You put up a good fight Gohan.  I'm proud
  of you.", "What?  Dad?", "Take care of your mother for me.  Tell her that I had to do this Gohan.  Goodbye my son.",
  "Goku...  No!!!!", a 5-frame white and both are gone. "He's gone!!!!!", "@DADDY!!!!!!!!!!!", Krillin and Gohan (five
  boxes), "Android 18!  I almost forgot about her.  She needs our help!  She's still alive!" (she is not on screen),
  Vegeta: "Krillin!  Leave that Android alone right now!..."
- **What the film shows:** the bomb never ticks (no shake, no pulse; nine fighters stand round a still picture); Goku
  leaves in a blink with no gesture; the one who "needs our help" walked away.
- **Lineups:** two players: Hercule's actor on the field, backs away to the bottom right at the bomb (kept). Alone: not
  there.
- **Anime:** Cell swells in stages while the ground quakes and counts down; Gohan drops to his knees; Goku, calm, puts a
  hand on Cell and two fingers to his forehead, "Goodbye, my son", and they vanish; a silence; far away the explosion.
- **Rating: rework (M).**
- **Shot list** (stock lines verbatim):
  1. 18 is thrown clear (`EntitySetVelocity` in an arc, a `SpawnDustCloud`), lands lying and **stays** until entry 51's
     fade clears the field. Cell staggers back two steps on "You're a monster!".
  2. The swelling: three pulses before the swap (`WhiteFlash`, `ScreenShake 0, 4 / 8 / 14`, sound 55), then the stock
     white swap.
  3. **The clock:** from the swap, a low `ScreenShake 0, 3, n` re-issued in every script entry until Goku goes, and a
     short `WhiteFlash 3` on Cell every second box. It costs no time and it is the scene.
  4. Everyone flies in (stock). Gohan: `cine: kneel` at "Cell has beaten us."; he looks up at "Listen...".
  5. Goku's walk (stock). At "Goodbye my son." he faces Gohan (stock), then turns to Cell, his ki pose toward him
     (`SetEntityAnimationNoWait 60, 5`, check the group).
  6. The leaving: no white. The blip sound, Goku and Cell flicker out together (`cine: afterimage dash` to nowhere, or
     two despawns after three 2-frame hides), **and the shake stops on that frame**, `StopMusic`. Forty frames of an
     empty field. Then, optional, a faint `WhiteFlash 3` and `ScreenShake 0, 2, 20`: King Kai's planet.
  7. "He's gone!!!!!"; "@DADDY!!!!!!!!!!!" with `ScreenShake 0, 6, 30` and a `SpawnDustCloud` ring: his cry has weight.
  8. Krillin walks to 18's body on "Android 18!" (two short legs, kneels by her: his facing down); Vegeta takes a step
     toward them on his line.
  9. Two players: Hercule's actor ends his retreat behind a rock (his stock walk to (536, 484)), peeks out after the
     silence.
- **Built so far (`nb-cs3`, 2026-11-06), beat 1 (the defect):** 18 is thrown clear in her hurt pose (group 3) to (516, 392), falls (her fight
  set's knock-down, group 12, which holds its last frame) and lies there until the two Cells' fade despawns her; the stock walk off the screen
  is gone, so "She needs our help!  She's still alive!" is said of a body. Test: `cell_staging.txt` (sprite 11 stands after the bomb's lines,
  gone at fight 8).
- **Built (`nb-cs3`, 2026-11-06), beats 2, 3, 5-8:** three pulses before the swap; from the swap a low shake is re-issued at every script entry
  (`TICK`, strength 2, 300 frames) with a flash at entries 9, 13, 17; Goku looks back at the group once, turns to Cell, and the two flicker out
  three times (off the view and back by `SetEntityPositionInstant`) and are gone: no white, the shake is cut (`ScreenShake 0, 1, 1`) and the
  music stops; 70 frames of field, a faint flash and tremor, then the stock walks; "DADDY!!!" has a shake and a dust ring, and the farewell's
  music starts again after it; Krillin walks to 18's side on "Android 18!" (he looked about for her), Vegeta starts toward them. Not built:
  Cell's stagger, Gohan's kneel (no helper), Goku's ki pose (his actor's groups were not checked), no sound at the vanish (no id could be
  auditioned headless), Hercule peeking out. `cell_games.txt` gives this scene 190 presses (it needs about 145 at 100 frames apart; the limit was 140).
- **Risks:** 18's sprite 11 is a fight set with 13 groups: **group 3 is her hurt pose, 12 the knock-down** (checked on film; `GlideToPosition`
  puts a body back on its idle pose when it arrives, so the knock-down is played after the glide). The swollen Cell 27 is a large body with nine actors and a player: OBJ
  VRAM is already at its busiest here; add no bodies, and measure with the runner's `vram` after adding dust. The stock
  batches use `SetEntityFollow` on seven sprites: the generator finds its insertion points by those exact strings.

### 3.8 Fight 7's end, part two: Cell returns, Trunks falls, Vegeta's charge (entries 34-50)

- **Today (filmed, `vegeta`):** a flash and a 3 s sound; everyone looks up. "Hey!  What's that?", "No!  It can't be!",
  "He's back!!!!". Cell is simply there at the top of the view; a small pink blast, Trunks flashes and lies down; the
  second Cell walks in beside the first; they walk down. "My aim is as good as ever.  It's so good to be back.
  Hahahahaha!!!  What's wrong?  Surprised to see me?", "When I exploded, two pieces of my core survived...  ...and each
  one grew back perfect!", "SECOND CELL: Twice the perfection!", "What do you want from us Cell?", the long "part of my
  design" speech, "You don't scare me Cell!!!!", Yamcha carries Trunks off, Vegeta's "Trunks!  My son!..." speech,
  Vegeta walks four steps, fires **one** blast (Cell flashes), Cell fires three (Vegeta flashes three times, standing),
  Gohan strolls up beside him. "Why do you protect Vegeta?  I'll never understand you Saiyans.  You are stubborn."
- **What the film shows:** Cell's return has no arrival; Vegeta's rage, one of his defining moments, is a single
  blast; nobody is knocked down; Gohan protects nobody, he arrives after the shots, and Cell's line then makes no sense.
- **Lineups:** two players: Hercule's actor jumps with the rest (kept). Alone: not there. Player 1 is Super Saiyan 2
  Gohan.
- **Anime:** a dust cloud and lightning; a beam out of it through Trunks' chest; Cell walks out with a new aura. Vegeta
  screams, turns Super Saiyan and empties himself in a storm of blasts; the smoke clears on Cell untouched, who swats
  him to the ground and fires to finish him; Gohan throws himself in front and takes it on the arm.
- **Rating: rework (M); the best pure action beat between story actors in the batch.**
- **Shot list** (lines verbatim; the two Cell lines are the 4h plan's, as built):
  1. The stock flash and looks. A dust column where Cell will be: three `SpawnDustCloud` stacked at (428, 328), and
     `ScreenShake 0, 6, 30`. Cell is spawned inside it.
  2. The three stock lines. On "He's back!!!!" `cine: blast single` out of the dust at Trunks (the stock
     `NPCFireKiBlastAtEntity 30, 132, 120`), then `cine: strike heavy` on Trunks without an attacker: the damage effect,
     a 24 px knock-back, the stock lying pose, a `SpawnDustCloud`. The stock follow-turns.
  3. Cell walks out of the dust (the stock walk), `cine: charge-up` as he clears it (lightning: two `WhiteFlash 3`);
     the second Cell steps out behind and takes his side.
  4. The talk as stock; one move per speaker: Cell opens a hand toward Trunks on "My aim", Vegeta's hop (stock).
  5. Vegeta's speech ends: `cine: charge-up` on 137 (`WhiteFlash 6`, `ScreenShake 0, 8, 20`), "He will pay!!!!".
  6. `cine: afterimage dash` Vegeta to (376, 416); `cine: blast volley`, two bursts of four at the Cells, dust over both,
     `ScreenShake 0, 6, 40`. A hold on the smoke, 30 frames; Vegeta stands, arms down.
  7. The smoke goes: both Cells as they were. `cine: strike heavy` first Cell on Vegeta: knocked back 40 px, falls, lies.
  8. Cell raises his hand (`cine: charge-up`, short). **Char 0 moves before the shot:** `cine: afterimage dash` Gohan
     between them; `cine: blast single` Cell at him; the damage effect on char 0, 8 px back, `cine: kneel`.
  9. "Why do you protect Vegeta?..." now answers what was shown. The stock fade.
  10. Two players: Hercule's actor runs for the far edge at beat 6 and is off the view by beat 7.
- **Built (`nb-cs3`, 2026-11-06), beats 1, 3, 5-10 with script steps (no helper):** dust and a tremor where Cell appears, two flashes as he
  has walked out; Vegeta powers up (a flash, a shake, the aura's sound, his hop), walks in as stock, fires two bursts of four at both Cells with
  dust on them and a shake after each; a 44-frame hold on the smoke, both as they were; Cell's one blast throws him back (137's hurt pose 2 in a
  glide to (340, 422), then lying, 3: the poses zone 12 uses on him); Cell raises his hand (pose 5); Gohan walks between them and the next
  blast bursts at his guard (fired at the ground 22 px before him, with his damage flash: a blast that reached the player's body would be a
  real hit); "Why do you protect Vegeta?" now answers what was shown. Two players: Hercule's actor runs for the far edge at the first burst.
  Not built: the afterimage dashes, Gohan's kneel, the knock-back on Trunks (the stock lays him down where he stands).
  **With the helpers (after the merge of `nb-cine`), replacing the hand-rolled hits above:** Vegeta's charge (op 7, level 1), his afterimage dash
  to (388, 414) (op 3), a volley of four at each Cell (op 8 kind 1: bursts on the body, the last an explosion; the first queued in a batch so
  Hercule's actor runs while it plays), Cell's heavy strike (op 5 flags 1, 40 px: Vegeta flies in an arc, lands in smoke, lies, held), Gohan's
  afterimage dash to Vegeta's side (op 4 on player 1, side 2, 18 px), Cell's blast on his guard (op 8 kind 0 + 16), and Gohan down on a knee (op 10)
  until "I will!", where he rises (op 10 rise in entry 64). Seen with the real Super Saiyan 2 Gohan (sprite 58): the kneel draws. Not built: the
  knock-back on Trunks.
- **Risks:** Vegeta's actor 137 needs a lying group for beat 7 (zone 12 lays "Vegeta's body" on the field in `ch7_cell`:
  use the same sprite and group as there). Char 0 is sprite 58: hurt frames come from 59 (a few frames of the wrong
  hair), acceptable for a flinch and a kneel; check the kneel's frame is drawable. Eight blasts with dust on a field of
  two Cells, six fighters and Gohan: bursts of four (the gym's finale ran out of OBJ tiles "all at once"). Entry 49 is
  one stock script with two batches; it is replaced, and entry 51 after it finds `DespawnEntity 30` by string.

### 3.9 Fight 7's end, part three: "ultimate perfection", Goku's voice (entries 51-64)

- **Today (filmed):** a fade; Gohan (two players: Hercule's actor beside him) faces the two Cells standing side by side.
  Cell's speech (about eight pages), "What's wrong Gohan?  Aren't you going to try and stop me now?", "What's the point?
  We've lost.  Now all of us will die together and we'll be with my dad...", "!GOKU: You've got to be joking Gohan!...",
  "Dad!?  Where are you?", the Other World speech, "Speak for yourself, home wrecker!" (King Kai), "But all this is my
  fault.  It's my fault you died!", "!Don't blame yourself...  Make him pay for what he's done!", "I will!", the card.
  Nothing moves for about a minute of reading.
- **Lineups:** as 3.8.
- **Anime:** Gohan, arm broken, has given up; his father's voice lifts his head; he stands.
- **Rating: touch up (S).** 1. Gohan is on one knee from the fade (`cine: kneel`). 2. The Cells: one `cine: charge-up`
  flash at "I am the universe's end!"; the second Cell crosses to the other side of Gohan during the speech (a slow
  walk): two against one. 3. At Goku's first line Gohan's head comes up (facing 0, stock) and, two players, Hercule
  looks about for the voice (left, right), which sets up his "Wh-who said that?!" in the struggle. 4. `cine: rise` on
  "I will!". No added time.
- **Risks:** the kneel and rise on sprite 58 (as 3.8). The second Cell is sprite 162 here and a boss record after the
  card (`TriggerMapEvent 12` brings the pair): do not move the pair after the event without checking which body the
  sprite step finds.

### 3.10 The beam struggle and Cell's end (`cg_beam_struggle` entries 0-19; record 12's defeat, map 326) **SHOWCASE**

- **Today (filmed, `gc` and `final`; alone in `ga3`):** a white screen that takes 10 s to clear (`WhiteFadeOut 600`) onto
  the picture: the two Cells crouched at the left, Gohan and Hercule at the right, a beam between with a large star in
  the middle. "GOKU: Dig deeper, Gohan!  You too, Hercule!" (the haloed portrait), "HERCULE: Wh-who said that?!", "CELL:
  These are your last minutes!  Make them count!". A pan left: Piccolo, Krillin, Tien and Yamcha fire a round each at the
  Cells, three times; "Would you stop interfering?!", a flash, all four lie down. A pan to Vegeta on his ledge and back.
  "Gohan, raise it higher!  Hercule, hold on!  He needs you!", "HERCULE: The champ...  never...  backs down!!!". The four
  stand, fire again, "They just keep lining up to die!", down again. "VEGETA: ARGHHH!!!", "Release it, both of you!
  Remember all the pain he's caused!  NOW MAKE THAT YOUR POWER!", "@SAY GOODBYE!!!", the camera goes to Vegeta, he flies
  down and fires the Big Bang; **about 20 frames later the screen is white**, and "@VEGETA!" and "Now's your chance!" are
  read on white. Then a bare field: Gohan fires an ordinary thin Kamehameha across it, the two Cells flash red and puff
  with small explosions for about 6 s, and are gone. A fade.
- **What the film shows:** the clash never moves, from the first frame to the last; Goku is a portrait; Vegeta's blast,
  the turning point, is hardly seen and Cell's reaction to it is text on a blank screen; the finishing beam is the same
  thin beam every fight has, and the villain of the game dies by small puffs.
- **Lineups:** one staging (Gohan player 1, `CoopCall 15, 1`; Hercule's actor in both modes, kept for the plot).
- **Anime:** Cell's wave drives Gohan's back until it is almost on him; Goku's spirit stands at his shoulder in the same
  stance; the others' blasts do nothing; Vegeta's one blast turns Cell's head; Gohan screams and his wave swallows Cell
  whole, cell by cell, in white.
- **Shot list** (lines verbatim: Goku's and Hercule's are the 4h plan's, as built):
  1. `WhiteFadeOut 120` (was 600): the picture in 2 s.
  2. **The clash is the helper's** (`cine: beam struggle` with a movable meeting point, the map's own star hidden as the
     stock hides it at the end with `DisableLayer 0 / 1`). It opens at the centre.
  3. Goku's first line: **his spirit** appears at Gohan's far shoulder, the haloed Goku (sprite 176) in the beam pose,
     drawn every other frame so the field shows through, and is there only while a line of his is open. Hercule's
     "Wh-who said that?!": his beam falters (`CoopCall 16, 72, 0` and on again) as he looks round.
  4. Cell's line: the clash moves a third of the way toward Gohan; `ScreenShake 0, 6, 20`; Gohan slides back 4 px.
  5. The first volleys (stock, with the camera): after "Would you stop interfering?!" and the stock flash, back on the
     clash: it is now two thirds of the way to Gohan.
  6. "Gohan, raise it higher!  Hercule, hold on!  He needs you!" (the spirit). "The champ...  never...  backs down!!!":
     Hercule's beam flares (a refire with `WhiteFlash 2`) and the clash is pushed back **to the centre and no
     further**. He holds; he does not win it.
  7. The second volleys and "They just keep lining up to die!" (stock): the clash creeps toward Gohan again.
  8. **Vegeta, shown:** the stock pan and "ARGHHH!!!", `cine: charge-up` on him; "Release it, both of you!..." and "SAY
     GOODBYE!!!" as stock; he flies down (stock) but the camera frames him **and** the Cells (they are 120 px apart);
     the Big Bang (stock step) travels and lands: the damage effect on Cell, `WhiteFlash 6`, `ScreenShake 0, 12, 20`,
     dust. Cell's head turns to him (`SetEntityFacing 30, 2`). "@VEGETA!" over that picture.
  9. "Now's your chance!" (the spirit, full, not flickering, for this one line). The clash runs to the Cells in 40
     frames, the shake rising.
  10. **The end:** the wide beam (`cine: blast beam`, or the stock Kamehameha with the helper's wider body) takes both
      Cells: one second of the stock hurt jitter inside it, `FadeToWhite 20`, a 40-frame white hold with the last stock
      shake (`ScreenShake 0, 20, 80`), both despawned under it. No puffs.
  11. `FadeFromWhite`: the field, Gohan back in his base form (the stock `SetCharacterTransformation 0, 56`), and he
      falls (`cine: fall`). Hercule is still in his beam pose a second too long, looks at his hands, lowers them
      (`CoopCall 16, 72, 0`, as today). The stock fade.
- **Built (`nb-cs3`, 2026-11-06), every beat, with C in the Games' own file (`cellgames.c`, CoopCall 16 on 3 / 4) and no helper:**
  the picture in 2 s; the held beams are out as the white lifts and no longer fade (their tick count wraps: the refires at every step are gone);
  **the clash moves**: the script sends the point where the two held beams meet along the line (440 the middle, 456 and 468 toward Gohan, 462
  the creep, 384 at the Cells) and it walks there 1-2 px a frame; the map's starburst (BG1) is on only while the clash sits in the middle and the
  beam's strip (BG0) stays, so what moves is the two beam heads; **Goku's spirit** (sprite 176 in the beam pose at (503, 376), behind Gohan's
  shoulder) flickers two frames in four while each of his three lines is open and stands whole for "Now's your chance!" (**for Ryan to review**);
  Hercule's beam drops while he looks round for the voice and flares at "The champ...  never...  backs down!!!", which pushes the clash back to
  the middle and no further; Vegeta flies down, the view takes him and the Cells, the Big Bang lands (flash, shake, dust) and the first Cell
  turns to him; "VEGETA!" is read over that picture (the view dropped so the shout's box clears the Cells); the clash runs into the Cells, the
  second Cell's beam breaks, Gohan's stock wave and Hercule's uncapped beam take both for the stock twelve beats of hurt, then white with the
  last shake, the two despawned under it; the field: Gohan in his base form goes down (pose 3), Hercule holds his beam pose a second, looks at
  his hands and at Gohan. No explosions.
  **Research corrections:** a moving cap alone needed no helper (the cap was a constant); the starburst cannot move (a background layer), so it
  is shown only at the middle; a body spawned for each line was evicted by the text box (OBJ VRAM: `docs/scripts.md` 5b), so the spirit is made
  once into a block Vegeta's off-view actor gives up and then only hidden and shown; the spirit's flicker runs from the input poll, since a box
  may take the beams (and their update) away. Tests: `cell_staging.txt` (the spirit's record, the clash's x at the start, after Cell's push and
  at the end, no spirit left behind), `cell_games.txt`.
- **Draft lines:** none. For Ryan: beat 3 puts an image (Goku's spirit) where the stock has a voice.
- **No-reveal check:** Hercule's beam was Ryan's own design (4h: "Hercule's beam pushes through with Gohan's"); beats 3
  and 6 make him waver, then hold. Nobody on the field looks at him afterwards, and scene 3.12 is his lie. Nothing here
  says hero.
- **Risks:** (a) the clash and its star are part of map 326, and the two held beams are capped "where the clash meets"
  (`cellgames.c`, CoopCall 16): a moving meeting point means the cap follows it, C work in the same piece as the
  helper. (b) OBJ VRAM: two 40 px Cells, Gohan, Hercule, two held beams, four fighters and Vegeta are on the map
  already; the spirit is one more body with a beam frame. Measure with `vram`; if it does not fit, show the spirit only
  in beats 3 and 9 and spawn it after the four are despawned for beat 9. (c) Haloed Goku's records carry the halo on
  every frame (`tools/halosprites.py`); check group 18 / pose 30 on 176 before posing it. (d) Beams fade by themselves
  and are refired at every entry (`REFRESH` in the generator): each new entry needs the refresh too. (e)
  `cell_games.txt` films this with `frames 1500` and fixed mash counts and saves `cg_pre8.ss`; `cell_staging.txt` uses
  `mash 42 100`: both need new counts. (f) Entry 16's `UploadSpritePalette` and the moves to (0, 0) are how the stock
  hides bodies under the white: beat 8 holds before them, not instead of them.
- **Size: L.**

### 3.11 The aftermath (`cg_beam_struggle` entries 19-42)

- **Today (filmed, alone too):** a fade; Piccolo is player 1; Vegeta, Tien, Yamcha and Krillin walk in to their spots;
  Gohan's actor **stands** among them; Hercule's actor at the right. "PICCOLO: He did it!  Cell's energy completely
  vanished!", "KRILLIN: You mean we're really safe again???", "TIEN: Yeah bud!  It looks like it.", "YAMCHA: Looks like
  saving the world took a lot out of Gohan.", Hercule steps up: "HERCULE: Is...  is it over?  Ha!  Hahaha!  Of course
  it's over!  Did everybody see that?!" (draft, 2026-11-05), nobody answers, "Don't worry Gohan, we can take you to Dende
  and he'll fix you right up.", "I'll take Trunks.", "Great!  And I'll take 18!", "Huh?", "Well, she is a victim of Cell
  and we might as well bring her along!", "Let's go Piccolo!", "You guys go ahead!"; they fly off, Hercule walks off
  right; Piccolo and Vegeta: "#That was brave what you did.", "#VEGETA: You can save your applause.  Now go on!  I don't
  need anyone's help!", "#Suit yourself.", Piccolo flies off; Vegeta's four thoughts alone; he flies off.
- **Lineups:** one staging.
- **Anime:** Gohan lies spent and smiling; Yamcha lifts him; Vegeta stands apart with his back to them.
- **Rating: touch up (S).** 1. Gohan's actor lies (the lying pose Trunks' actor uses, on 56) from the fade until Krillin
  walks to him; Krillin kneels by him (facing down) for "Don't worry Gohan". 2. Vegeta stands with his back to the group
  (facing away) from the start and only turns for his line to Piccolo. 3. Hercule's line: he walks into the middle of
  the group for it and each of them goes on looking at Gohan: being ignored is already the joke, make it visible. 4.
  Vegeta's thoughts: the camera closes on him alone, one slow step toward where the clash was.
- **Risks:** a lying pose on actor 56 (check the group; the stock only stands him). Player 1 is switched to Piccolo by
  the stock entry inside the fade: do not pose char 0 before it.

### 3.12 Hercule and the announcer (entries 42-48) and the Games' end (49-54)

- **Today (filmed):** the camera drops to the rocks: the announcer and Hercule, both facing the camera. "ANNOUNCER:
  Hercule!  What on this crazy Earth happened?", "!HERCULE: Well... What if I said...  I DEFEATED CELL!!!!  SINGLE
  HANDEDLY!!!!!!", "Just how did you do it?", the "light shows and tricks" speech, "CHOP!!!!  Hahahahaha!!!", "But then
  what happened to the little boy and all his friends?", "!Well they all thanked me kindly and went home of course!". A
  fade, the Capsule Corp atrium for the statue, the world map, "Super Saiyan 2 Gohan is now an available character!",
  "Journal Updated!".
- **Lineups:** none (no player on the map).
- **Anime:** the cameras come back on; he hesitates, then claims it, arms up, to a cheering world.
- **Rating: touch up (S).** 1. He starts turned away, looking at where the fighters flew off (facing up), and turns to
  the announcer on his name. 2. A beat of him looking left and right (nobody is left to contradict him) before "What if I
  said...". 3. His win pose on "SINGLE HANDEDLY!!!!!!" (the pose the talk show uses on "YEAH!!!!!", 7), a punch pose on
  "CHOP!!!!". 4. The announcer turns to the camera (facing down) for each question. The end entries: **leave**.
- **Risks:** the poses 6, 7, 8 on actor 72 are the ones the stock ring scene and talk show already play on him; no other.

## 4. Stock talk inside the Games that stays as it is

Krillin consoling Gohan, Vegeta's "Trunks!  My son!" speech, King Kai's interjection, Cell's "part of my design": long
text, each with the one or two small moves listed above; none needs more. The title cards (`cg_card`): leave.

## 5. The post-game on the Lookout (`data/story/postgame.lscript`, generated by `tools/gen_postgame.py`; Dende's record 4, map 261)

### 5.1 The wishes (`pg_dende` entries 1-50; the stock scene on zone 9 area 7)

- **Today (filmed, two players and alone as Krillin: the same):** a fade to the Lookout's edge, everyone in a ring round
  the Dragon Balls. The stock talk (Dende, Gohan, Piccolo, 18, Krillin, "Krillin, you have a crush on Android 18!",
  "Not that easy."), "Eternal Dragon!  By your name I summon you forth!  SHENRON!": the sky goes white and Shenron's
  body rises behind them (the stock's best picture in this batch). The first wish, "YOUR WISH IS GRANTED.", Piccolo
  cannot sense Goku, the second-wish talk, Goku's voice and his long goodbye, "YOU HAVE ONE WISH REMAINING." Then ours:
  "KRILLIN: I have a wish...", "YAMCHA: I wish we always had someone strong like Goku around!", a flash, "@YOUR WISH HAS
  BEEN GRANTED.  I MUST GO NOW.", Shenron leaves in a long white.
- **What the film shows:** our beat is three boxes with nobody moving: Krillin does not step forward, nobody turns to
  Yamcha, Yamcha does not notice what he did, Krillin does not react to losing his wish.
- **Lineups:** one (section 1). Hercule is rightly absent (he is at the studio), Goku rightly absent (dead).
- **Anime:** Krillin's wish for 18 is the scene's heart; ours removes it on purpose (Ryan, 4i). The staging to borrow is
  how everyone turns on whoever speaks to the dragon.
- **Rating: touch up (S).** 1. "I have a wish...": Krillin walks two steps toward the balls, facing Shenron; 18 turns her
  head to him. 2. Yamcha's line comes with him already looking up, arms behind his head (his hop, pose 4): he is musing,
  not wishing. 3. The stock's own turn-on-Yamcha batch (entry 46: everyone `SetEntityFollow ..., 156`) plays as the wish
  is granted; Krillin spins round to him (stock entry 13's hop). 4. After "GRANTED": the stock entry 48 (Yamcha looks
  left, right, left) and, if Ryan likes it, the stock line 47 **"Guys?"** reused in its own scene and speaker: the word
  is the stock's. 5. Shenron's exit as stock.
- **Risks:** the stock sequence returns no absolute indexes here but `pg_dende` branches by index (`PushByte 83 / 65 /
  63`, the question's 88 / 86 / 91): the generator counts them; added entries shift every one. `SetEntityFollow` on 93
  (Popo) is in the stock batch: keep the list as it is.

### 5.2 Hercule's stomach (entry "cut to Hercule at the TV station", zone 4 area 20)

- **Today (filmed):** a fade to the studio set; Hercule stands by the sofas, facing the audience; his hurt pose;
  "HERCULE: *grrrumble*  Huh?  Must've been something I ate!"; a fade back. About 5 s, one picture.
- **Lineups:** none. **Anime:** not in it. Ryan's beat (4i), and the line calls back to his stomach at the Games.
- **Rating: touch up (S).** 1. He is mid-brag when we arrive: walking along the front row, his win pose to the crowd. 2.
  The growl is shown: `ScreenShake 0, 2, 20` and a low sound, the pose breaks into the stock hurt pose. 3. The line,
  verbatim. 4. He looks left and right, shrugs it off with the win pose again as the fade takes him.
- **Not recommended:** any light or glow on him. The cut is the hint; a glow would say more than the rule allows.
- **Risks:** the set is an empty map lit by the scene; the audience is the map's art, not bodies. Only poses 2, 6, 7, 8.

### 5.3 The explosion, Bulma, 18 leaves (entries after the studio)

- **Today (filmed):** back on the edge; 30 frames, a shake and a boom; Bulma runs up from below; "BULMA: What was that
  explosion!?  Don't tell me you guys didn't finish off Cell!", "PICCOLO: No, I told you I no longer sense Cell!  There
  was just a great pulse of power in the Arctic.", "ANDROID 18: It must be my brother.  I'm going to go check on him.", she
  flies off to the right, "PICCOLO: We're all going.", a fade, the players on the Lookout, "Journal Updated!".
- **Lineups:** one (Gohan is still player 1; the party is put back after the fade).
- **Rating: touch up (S).** 1. Piccolo turns north before anything is heard (he senses it), then the shake with a
  `WhiteFlash 4` from that side, and everyone turns the same way. 2. Bulma as today. 3. 18 walks to the rim, looks back
  once at Krillin (one facing change, no line: the stock crush talk earlier earns it), flies. 4. On "We're all going."
  Vegeta is already walking to the rim (two steps), Krillin first of all (he follows 18's direction).
- **Risks:** Bulma's sprite 24 is a two-group NPC: walk only. 18 is sprite 11 here and the stock flies her.

### 5.4 Dende's reminder (entry "That power in the Arctic...")

One line when talked to before Super 13 is beaten. **Leave.**

### 5.5 The finale (entries from "back on the Lookout's edge, 18 gone"; plays by itself on arrival through `pg_finale_cond`)

- **Today (filmed as Piccolo with Hercule, as Vegeta with Hercule, alone as Krillin):** the arrival from the summit
  fades straight to the edge. Player 1's body stands at the stock's Gohan spot among the stock actors. "Finally...  some
  time to relax.  No more enemies to fight!" (player 1), "PICCOLO: Peace...  for now.", the stock goodbyes ("TIEN: Well
  it's about time I get going.", "Yeah, same here.", "You take good care of your mother Gohan!  You're the man of the
  house now!"), the four fly off, "Will you be staying here Piccolo?", "Yes.", "Goodbye Gohan!  Be sure to visit!",
  "Goodbye!", "DENDE: Talk to me if you want to discuss our times of peace!", a fade, the players on the Lookout.
- **The defects:** section 0, item 2. Also: player 2's character has no body; "No more enemies to fight!" from Vegeta.
- **Anime:** the fighters part quietly; Vegeta leaves without a word; Piccolo stays.
- **Rating: rework (M): who stands, not new action.**
- **Shot list:**
  1. Behind the fade, by who plays (`PushActiveCharIndex`, `CoopCall 8`, `CoopCall 22, 4, slot, 0`): **despawn the stock
     actor of each played character** (Piccolo 103, Krillin 76, Vegeta 137, Trunks 132), put player 1 on that actor's
     spot when he is one of them, else on the stock Gohan spot.
  2. **Gohan is in this scene whoever plays** (three stock lines are his, two are said to him): when neither player
     plays him, his actor stands on the stock spot and player 1 beside it. Kept for the plot, to be named in
     `docs/story.md` 0.
  3. Player 2: when his character is one of the four he is that spot's body; otherwise an actor of his sprite beside
     player 1 (the `CoopCall 31` pattern of `ch1_opening`), except Goku and Hercule, who have no place on this stage
     (dead; at the studio): Ryan's call whether they stand.
  4. The opening line by voice: as written for Gohan, Krillin, Trunks, Goku, Hercule. Player 1 Piccolo: skip it; his own
     stock-styled "Peace...  for now." opens the scene. Player 1 Vegeta: DRAFT `VEGETA: "Hmph.  Nothing left worth
     fighting.  For now."`, and Piccolo's line answers it.
  5. Small moves on the stock lines: Tien steps to Gohan for "man of the house"; the two pairs fly off in different
     directions (stock); Vegeta, when an actor, has his back turned through the goodbyes and leaves last without
     turning; Piccolo walks to the rim after "Yes." and looks out.
- **Built (`nb-cs3`, 2026-11-06; `gen_postgame.py` "finale", `postgame.json` zone 9 area 7):** the four stock party actors' records carry a
  condition (`pg_stand_*`: not in the finale, or player 1 is someone else), so player 1 on Piccolo, Vegeta, Trunks or Krillin stands on that
  actor's spot and there is one of him; Gohan is an actor on his stock spot unless player 1 plays a Gohan (kept for the plot); player 1 as
  Goku or Hercule stands beside him; player 2 has no entity here, so his stock actor is his body, and as Goku (176), Hercule (72) or the
  other Gohan (56 / 58) an actor stands beside the group (Ryan's decision: players present stand). The opening line is `text 250`
  (player 1's own portrait, never the lead's); player 1 Piccolo skips it and opens with "PICCOLO: Peace...  for now."; Vegeta says the line
  as written (no variant, decided). Tien steps toward Gohan for "man of the house"; Vegeta and Trunks fly off as actors only (a player who is
  one of them stays); the facings of entries 81 / 85 go to the body that is there. No `SetActiveCharacter`, no flag but 483.
  **Research corrections:** the VM's `Not` is a binary OR (`docs/scripts.md` 2.1), so a record condition is written `A; B; Not`; `DespawnEntity`
  on a sprite player 1 shows was not used (it could find the player): conditions on the records instead. Speaker 1 would have shown the
  lead (Gohan) once his actor stands, hence 250. Test: `staging_postgame.txt` `fin_*`, 18 lineups (sprite records, the map, flags, who plays).
- **Risks:** the scene starts from an arrival trigger with player 2's entity alive on area 1, then warps without
  entities; the party is not switched (no `CoopCall 10` here): do not add `SetActiveCharacter`. A sprite step on a
  despawned actor is a null write: guard every later step on 103 / 76 / 137 / 132 with `CoopCall 22, 2, sprite, 0` (the
  stock goodbyes fly 137 and 132 off: those entries must be restated with guards, they are inside `stockseq 75 86`
  today). Test: `postgame.txt` only checks flags; a lineup generator is needed (eight player-1 choices, a few pairs).

### 5.6 Dende's question

"DENDE: Enter Peace Times and start "Dragon Ball Z: Buu's Fury" co-op?  (A: Yes  B: No)", "DENDE: Okay.  Come talk to me
when you're ready!", "Are you sure you want to start the Buu's Fury campaign?  (A: Yes  B: No)". Filmed; a menu.
**Leave.**

### 5.7 The talk show (stock entries 1-28 of map 67 trigger 9, on zone 4 area 20)

- **Today (filmed as Piccolo, and alone as Krillin):** a slow pan up the audience to the stage; "ROB: Good evening, and
  welcome to Late Night with Rob Howard!..."; the guest walks down through the audience, "OH YEAH!!!!!  WHO LOVES YOU AND
  WHO DO YOU LOVE?", "CROWD: HERCULE!!!!...", poses left and right, walks up to the desk, the interview (about 25 pages),
  "YEAH!!!!!", the crowd, a fade to the credits. The guest is player 1's body and portrait (section 0, item 1).
- **Lineups:** must become one: Hercule.
- **Rating: fix (S), then touch up (S).** After the fix: 1. Camera flashes from the audience on each pose (`WhiteFlash
  2`, two or three). 2. `ScreenShake 0, 2, 10` under each "CROWD:" box. 3. On "What about those strange fighters who
  could turn their hair gold?  What about that little boy?" he freezes for the stock 40-frame wait, turned away from
  Rob (one facing change), then turns back for "That was all parlor tricks!". No line changes; the scene is the stock's
  showpiece for him and already walks and poses.
- **Risks:** `SetActiveCharacter 5` with player 2 on Hercule: the companion rule gives player 2 player 1's old character;
  harmless here, the game ends. Player 2 has no entity on this map.

### 5.8 The credits

Stock, about 50 s (filmed). **Leave.**

### 5.9 The epilogue (the last entries of `pg_dende`; zone 6 area 23, zone 1 area 2)

- **Today (filmed):** Future Trunks walks through the Capsule Corp yard to the time machine and looks about; a fade; the
  ruined city: he walks a few steps, the camera rises; old Hercule (sprite 158) walks in from the right and stops.
  "TRUNKS: I'm back.  Now I'm ready to take the Androids down!", "OLD HERCULE: Hey, Trunks!  You're too late!  I already
  beat them with my Megaton Punch while you were gone!", "TRUNKS: You...  what?!", "OLD HERCULE: I don't know what it
  is...  but lately I feel stronger than I've ever been!", "@^The End.  Thanks for playing!", the title screen. Four boxes
  between two standing bodies.
- **Lineups:** none. **Anime:** Trunks destroys his timeline's androids and Cell; ours replaces it (Ryan, 4i).
- **Rating: touch up (S to M).** This is the one place Ryan allows a hint ("A hint that our timeline's Hercule will one
  day be the hero").
- **Shot list** (his four lines verbatim): 1. Trunks arrives braced: he walks in fast, stops, turns left and right
  looking for them (two facing changes), his line with a fighting pose. 2. A rumble first (`ScreenShake 0, 4, 20`,
  Trunks spins toward it), then old Hercule strolls in, hands-on-hips idle, from where the rumble came. 3. "Megaton
  Punch": he throws it (his punch pose) at the air; a `SpawnDustCloud` and a tiny shake 12 px in front of the fist,
  which is more than a punch should do. 4. "You...  what?!": Trunks' hop. 5. "stronger than I've ever been": he looks at
  his fist (facing down), flexes; hold 30 frames before "The End."
- **Risks:** sprite 158 is a Buu's Fury conversion on Hercule's 21-group template (`buu2log2`): poses 6-8 exist by the
  template, check them in `build/<dir>/buu_preview_158.png`; a `BUU=0` build shows the stock Hercule. `LoadMapWithEntities`
  twice: actors go at each.

## 6. The Arctic

### 6.1 The route (`arctic_route.lscript`; the world-map landing, zone 15 areas 1, 5-7)

- **Today:** a star on the world map, a landing, the climbs with enemies, the ridge pad. No scene anywhere (the climbs
  were not filmed again; `docs/testing.md` "Arctic route" has them).
- **Rating: touch up (S), two beats without a word.** "PICCOLO: We're all going." promised a power at the top; the
  mountain should show it before the summit does. 1. The first landing: a far boom, `ScreenShake 0, 6, 40`, a `WhiteFlash
  3`, player 1 turns up the slope. 2. At the ridge pad, the first time: the same, stronger (`ScreenShake 0, 12, 60`).
- **Risks:** each needs a once-flag from `docs/ids.md`; the route's maps are `data/maps/arctic*.json` (triggers through
  `"sources"`). Never cost the player: both are under a second and a half.

### 6.2 The summit: 13, 17 and 18, the absorption, Super 13 (`summit_scene`; zone 15 area 8) **SHOWCASE**

- **Today (filmed as Vegeta with Hercule, and alone as Krillin; about 35 s to control):** the players cross the line; the
  camera goes up to the centre and **the players are below the view for the whole scene**. 13 stands between 17 and 18;
  Tien and Yamcha run in to the right edge. 18 fires once, 13 and 17 flash. "ANDROID 13: Ha!  The Dragon Balls brought
  your brother back...  and you came along too.  Just for me!", "ANDROID 17: Sis, get out of here!", "ANDROID 18: Not a
  chance.  We finish him together!". 13 walks to 17: a white flash, 17 is gone, a shake. "ANDROID 13: This is for the
  Doc.  Your power is mine!". He walks across to 18, who has stood and watched: a flash, gone. He walks to the middle;
  three flashes with growing shakes; "ANDROID 13: Yes...  YES!  I can feel their power!"; a long shake, Tien and Yamcha
  step back; "@^The whole mountain is shaking..." in a box across the middle of the screen, over him; a white-out; Super
  13, twice anyone's height, his music; "SUPER 13: Now...  I am complete!", "SUPER 13: Look at me.  Even Cell would
  tremble!"; the camera drops to the players; player 1's line ("VEGETA: Hmph.  Another tin can to scrap." / "HERCULE:
  Uh...  is it too late to go home?" / "KRILLIN: 18...  You'll pay for this!" / "PICCOLO: Careful.  He's stronger than
  Cell was." / "We have to stop this!"), with two players "%s: Let's take him down together!"; the fight.
- **What the film shows:** the reveal works (the size of him, the music). Everything before it is walking and blinking:
  the "struggle" is one blast, 17 and 18 do not fight, dodge or move, 18 waits her turn, and an absorption is a white
  frame. The line about the mountain covers the body it describes.
- **Lineups:** section 1. Only the last two lines vary, and they do already.
- **The movie (Super Android 13!):** 13 takes the parts of the two androids destroyed before him; his body convulses,
  swells, the skin goes blue, the hair red, in a column of red light while the ice breaks; then he stands silent, and
  what follows is the point: Vegeta's and Trunks' best blows land and he does not move; he walks through blasts. Ours
  absorbs 17 and 18 (Ryan, 4i), which borrows from Cell; the staging to borrow from the movie is the convulsing change
  and the proof that nothing hurts him.
- **Shot list** (every line verbatim; the five transformation lines were approved 4i, the player lines are ours):
  1. **The players come into the picture:** at the trigger char 0 walks up to (300, 372), player 2 is placed beside him
     (`CoopCall 9`), both facing up, on the view's bottom edge under the camera's stock position. They stay in frame to
     the end, so the pan down at the end goes.
  2. **A fight already under way:** `cine: exchange flurry` 17 and 13, 2 s; 13 gives no ground. 18's blast (the stock
     one) lands: dust; 13 walks out of it brushing his vest (idle, two steps). Tien and Yamcha run in (stock).
  3. The three lines. One move each: 13 cracks his knuckles (his charge pose 5, short); 17 steps between him and 18; 18
     steps up beside her brother, not behind.
  4. They go together: `cine: afterimage dash` both at him. `cine: strike heavy` 13 on 17: knocked back, down, lying.
     `cine: strike light` on 18: she skids back and stays up.
  5. **Absorbing 17:** 13 stands over him; a red-white pulse (`WhiteFlash 6`, sound 61); 17 is dragged the last 16 px
     into him (`EntitySetVelocity` toward 13 for 8 frames) and despawned on contact; `ScreenShake 0, 10, 30`; 13 hunches
     (hurt pose 2, 10 frames) and straightens, taller in bearing. "This is for the Doc.  Your power is mine!"
  6. **18 does not wait:** `cine: blast volley`, five, as he walks at her through them (no flinch, dust on him, the walk
     never stops); she backs two steps, still firing; he reaches her; the pulse and the drag as 5.
  7. **The change** (the approved beats, with a body in them): he walks to the middle. Surge one: `cine: charge-up`,
     snow blown out in a ring 16 px (four `SpawnDustCloud`), `ScreenShake 0, 6, 20`. Surge two: the ring at 32 px, shake
     12, Tien and Yamcha pushed back a step; the players pushed back 4 px (`EntitySetVelocity 0`, and player 2 by C).
     Surge three: the ring at 48 px, shake 18, he doubles over (pose 2 held). "Yes...  YES!  I can feel their power!"
     The long shake; "@^The whole mountain is shaking..." **at the bottom of the screen** (drop the `^`), so he is seen
     shaking.
  8. **The reveal** (stock of ours: the white-out, the swap to 180, the music, the biggest shake), and as the white
     lifts he is in his crouching power-up pose (`S13_G_POWER`, group 23) and rises into the roar (`S13_G_ROAR`, group
     24) for 40 frames: both are on the sprite already for his specials. Then idle. "SUPER 13: Now...  I am complete!"
  9. **Proof, between story actors only:** Tien and Yamcha fire (`cine: blast volley`, three each). Smoke. He has not
     moved; his head turns to them. One `cine: blast single` (his throw pose, group 21) into the snow between them:
     both go down and slide to the rim, where they lie through the fight (they stand there today). "SUPER 13: Look at
     me.  Even Cell would tremble!" now follows a demonstration.
  10. He turns to the players (facing down). Player 1's line with one step forward (Hercule's: one step back). Player
      2's line. The stock hand-over to the boss record. Control where it is today.
- **Built so far (`nb-cs3`, 2026-11-06), beat 1 only (the defect):** player 1 walks up to (324, 372) and player 2 is placed at (350, 372)
  (`CoopCall 9`), both facing the three, and the pan down before the players' lines is gone. Seen: with the stock camera a text box at
  the bottom covers screen rows 74-136, so the players show whole between boxes and by their feet under one; nothing on this view clears
  both the three and the players during a box. Test: `staging_arctic.txt` (player 1 at 324, 372 and player 2 at 350, 372 mid-scene, nine lineups).
- **Built (`nb-cs3`, 2026-11-06), beats 2-7, 9, 10 and the short form, with script steps (no helper):** 17 goes in first (two blows with their
  sounds and no flash on 13), 13's one answer throws him back (his hurt pose in a glide, dust), 18's blast lands and 13 walks out of its dust and
  cracks his knuckles (pose 5); 17 comes forward again, 18 steps up on the other side; they go in together, 13 throws 17 down (a flash, a shake,
  the knock-down 12) and knocks 18 back; he stands over 17, a pulse, 17 is dragged into him and gone, 13 doubles over (pose 2) and straightens;
  18 fires three as he walks at her, backs off, two more, he reaches her, the same pulse and drag; the three surges each blow a wider ring of
  snow, the second pushes Tien, Yamcha and both players back (glides; player 2 by `CoopCall 9`); after "I am complete!" Tien and Yamcha fire
  three each, dust on him, he turns his head and one blast between them throws both to the rim, where they lie through the fight and stand again
  when he is gone (`summit_13_end`); player 1 steps toward him for his line (Hercule: a step back). Not built: beat 8's crouch and roar (the
  boss draws those groups by its own frame code, not by an animation index a script can name), the caption at the bottom (`@` without `^` is a
  shout box in the middle too). **The short form:** `CoopCall 16, 0, 6` is 1 once the full scene has played since power-on: the players walk in,
  a white cut to Super 13 standing with Tien and Yamcha down at the rim, his music, "Look at me.  Even Cell would tremble!", the players' lines,
  the fight (about 8 s to control instead of 55).
- **Draft lines:** none.
- **Time:** about +12 s on 35. **This scene replays in full after every loss and every reload at the ridge pad** (Ryan
  kept that, 4i). Build with it a short form for a retry in the same session (a RAM bit, not a flag: he stands
  transformed, his last line, the players' lines): otherwise a better scene becomes a longer penalty.
- **Risks:** (a) OBJ VRAM: 13 (165), 17 (9) and 18 (11) are fight sets, plus Tien, Yamcha, two players, blasts and dust;
  then a 64 x 64 body (slot 0x1000) is spawned after three despawns into whatever is free: run `vram` at beat 6 and
  right after the swap. (b) Sprite 165 has groups 0-3, 5-8, 12 only; 9 and 11 have the same layout it was built on; 180
  as an actor has all 27, but its groups 21-25 draw one front view for every facing. (c) Tien 121 and Yamcha 156: the
  stock struggle lays both down with pose 3 and hops them with 4; nothing else. (d) Every step on a watcher's sprite in
  this script is guarded by `CoopCall 22, 2`: keep the guards, the watchers never stand now. (e) Player 2 is moved by C
  (`CoopCall 9`), never by a sprite step. (f) `arctic_summit.txt` presses a fixed 14 times, `staging_arctic.txt` uses
  `stepz`: the first needs recounting. (g) A `BUU=0` build shows Perfect Cell's body for 180 and has no groups 21-25:
  guard the two poses of beat 8 (`CoopCall 30, 1`-style test, or the helper's own group check).
- **Size: L.**

### 6.3 Super 13's end and the return (`summit_13_end`)

- **Today (filmed):** his own explosion (the boss's death frames, light bursting out), a white flash, a boom and shake,
  a fade to white and back; with Vegeta played, "VEGETA: *spits*  At last.  The end of those clankers."; "Journal
  Updated!"; a fade; the Lookout, and the finale starts.
- **Lineups:** Vegeta's line only when someone plays him (built).
- **The movie:** Goku's punch goes through him and he comes apart in the light.
- **Rating: touch up (S).** 1. Before the white: three `WhiteFlash` at shortening gaps with the shake climbing (6, 12,
  20): the light is breaking out, not switched on. 2. After the white: no music for 60 frames, drifting `SpawnDustCloud`
  as snow, Tien and Yamcha `cine: rise` at the rim and walk in two steps. 3. Vegeta's line (Ryan's, verbatim) said
  facing away from the others; with nobody on Vegeta the silence is the beat. 4. The summit's music, the journal, the
  fade.
- **Risks:** the boss's own explosion is 96 frames of `super13.c`; the script's `WaitFrames 100` is timed to it. If
  6.2's beat 9 is not built, Tien and Yamcha are standing and beat 2's rise is a turn instead.

## 7. Papaya Island (`papaya.lscript`): gaps only

Filmed: the welcome with one player and two (`tnyJ`). The ring's staging (`docs/tournament.md` 27-28) was not filmed
again. Nothing below is urgent.

1. **The welcome (`tny_arrival`):** the announcer's four lines are narration boxes (speaker 0, "ANNOUNCER:" in the text)
   in the middle of the screen, while every other speaker in the game has a portrait box; he stands at the top right
   among two other NPCs and is easy to miss. If the Buu's Fury announcer has a portrait in `roms/buu.gba`, give him a
   speaker; else leave.
2. **"...our reigning champion...  the great HERCULE!"** plays the same when a player **is** Hercule. A pose, not a
   line: when Hercule is played (`CoopCall 22, 4, 5, 0`), his win pose on that box (player 1 by char 0, player 2 by C),
   and the announcer's microphone swings to him (`CoopCall 30, 8`). Touch up (S).
3. **Directions told, not shown:** "Up the street, through the gate, past the infirmary: the fighters' waiting room!" is
   the kind of line the Time Chamber's opening replaced with a camera. The street, gate and stadium are other maps
   (15/12, 15/13, 15/16), so a tour would be views with their own fades, as the chamber's are. Optional (M); the red
   carpet's equivalent here is the street itself, which is hard to miss.
4. Pikkon's hidden arena, the infirmary's Yamcha, the vendors: one-line talks, fine as they are.

## 8. The Time Chamber (`chamber.lscript`, `chamber_popo.lscript`): only what is below its opening

Filmed: the opening with two players (`chopen`), Roshi's dojo end to end with its lever (`chroom`), Popo's room for two
lineups and the training's end (`chx`). The opening is the standard and stays.

1. **A beaten boss "blinks out"** (rule 4 of "Stage a scene": nobody pops). They are Popo's doing ("Popo filled this
   chamber"): let it look made: a short `WhiteFlash` and the gate's hum for half a second as the bodies go. S.
2. **The six room views of the opening** show each boss standing still. One idle move each as the camera reaches him (a
   turn of the head toward the camera, Broly's fidget) would cost nothing. S, optional.
3. **Mr. Popo at the Lookout door (`lk_popo`)** tells in one static line what the opening then shows. Leave the line;
   have him step aside from the door as he says it. S.
4. Popo's room, the lever's cut-away along the carpet, the door's two questions, "Training complete!": at the standard.

## 9. Not filmed, and how sure the rest is

- **The cinematic helpers** do not exist in the reference ROM: every `cine:` beat is a proposal, not a seen result.
- **The stock ROM itself** was not run: stock scenes were filmed inside our ROM, where they play from the stock data.
- **2.1 (Dende takes the Dragon Balls)** and **6.1 (the route's maps)**: read from the dump and the docs only.
- **The Games alone:** filmed through the aftermath (`ga2`, `ga3`) at one frame a page, enough to confirm who stands;
  the first alone run died in fight 2 because the idle fighter is killed while a film presses A (queue the win before
  filming). **Player 2 as Krillin or Vegeta at the ring:** not filmed; the generator and `cell_staging.txt` say the scenes
  are the same.
- **The summit:** Vegeta with Hercule, and Krillin alone. Not Gohan, Piccolo, Goku or Super Saiyan 2 Gohan as player 1.
- **The finale and the talk show:** player 1 Piccolo (with Hercule), Vegeta (with Hercule), Krillin alone. Player 1
  Gohan, the one lineup that should be right, was not filmed.
- **The statue in the atrium** after the interview fell between two samples.
- Sprite group numbers in the shot lists come from `docs/sprites.md` and from what the stock scripts already play on
  each sprite; where a pose is new on a sprite the risk line says to check it.

## 10. Priority for the developer

Build order, showcases first. Before them, the two ending fixes of section 0 (5.7's `SetActiveCharacter 5`, then 5.5):
small, and they are wrong today.

1. **3.6 Gohan's Super Saiyan 2** (showcase, L): the reframing alone fixes a defect; the transformation is the batch's
   best ten seconds.
2. **3.10 The beam struggle** (showcase, L): the moving clash, Goku's spirit, Vegeta's blast shown, the end in white.
   Shares the helpers' beam work; do it second while that is fresh.
3. **6.2 The summit** (showcase, L): the absorption with weight, the change with a body in it, the proof on Tien and
   Yamcha; with the short form for retries.
4. **3.3 "I give up" and Hercule called out** (showcase, M): place the two, cut the walk, the crowd's turn, the alone
   exit.
5. **3.8 Cell's return and Vegeta's charge** (rework, M): the volley, the knock-down, Gohan in the way.
6. **3.7 The bomb and Goku's goodbye** (rework, M): the ticking shake, the quiet leaving, 18 left lying.
7. **3.1 and 3.2 The ring** (touch up, M and S): Hercule's flurry on a Cell who does not flinch, the blow with weight,
   the ring seen breaking.
8. **5.1-5.3 The wishes, the stomach, the explosion** (touch up, S each), then 3.4, 3.9, 3.11, 3.12, 6.3, 5.9 as time
   allows.

**The batch's best Short.** Gohan's Super Saiyan 2, cut from Cell's last step toward the head: Cell crushes it and the
music stops; "Yet another fighter you could have saved, Gohan." as he walks away; "ANDROID 16!!!!!!!"; the tremor, the
dust ring, Cell turning round and stepping back; the held silence; the white; Gohan standing still with the lightning
flicker; "NOW TASTE MY TRUE POWER!!!!!!". About 30 seconds, four boxes of text, every fan knows the moment and will judge
the mod by how it treats it, and it needs no explaining. Film it alone (no Hercule at the end) for the pure version;
the two-player take, where "The champ is back!  Let's finish this together, kid!" breaks the tension, is the second
Short and the one that tells people this is The Legacy of Hercule. If one clip must carry the mod's own idea instead,
it is 3.3: Goku gives up and calls for Hercule.
