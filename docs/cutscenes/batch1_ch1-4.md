# Cutscene research, batch 1: chapters 1 to 4 (2026-11-06)

Research only: no game data was changed. The guidelines are `docs/cutscenes.md` (this file is its section 6
"Research"); the mechanics are `docs/howto.md` "Stage a scene". Scripts: `data/story/ch1_opening`, `ch2_west_city`,
`ch3_duels`, `ch3_zone7`, `ch4_piccolo`, `ch4_eggs`; the stock scenes from `tools/script_asm.py dump map Z A` and
`dump seq 0x083dae70` (the lab door's item scene is in no map dump).

## 0. How it was filmed, and what the films cannot tell

- Reference ROM `build/ref/log2-coop.gba` + `build/ref/menu.ss`, read only, through `tools/headless/st_test.py`. Each
  scene: Map Test, poke the lineup, warp into the trigger (the recipes of `staging_ch1_gen.py`, `staging_ch23_gen.py`,
  `ch4_staging_gen.py`, `ch1_notes.txt`, `ch4_eggs_gen.py`), then A every 96 to 160 frames with a shot every 10 to 40
  frames; near-identical frames dropped, the rest tiled 8 across. Everything is in `build/research1/` (not committed):
  generators `gen1.py` `gen2.py` `gen3.py` on `film_lib.py`, runner scripts `f_*.txt`, `sheets.py`, the contact
  sheets `*.png` (named below as "sheet `landG_1`").
- **Test artifacts, not bugs:** a Map Test party has Piccolo as player 1, so in the prologue films Piccolo's body
  stands where Trunks does (`train_1`, `knock_1`); in the zone 7 film Vegeta's lines on speaker 1 showed Gohan's
  portrait (`btV_2`: the lead flags of a poked lineup). The developer's staging test should check that portrait once.
- **The HP bars stay on screen** in several scenes (space, the generators, the jungle, the first frames of Gohan's
  death, the road). A scene meant for a Short wants them hidden from its first frame to control's return. Whether
  `SetPlayerVisible 0` alone hides them was not traced: a fact for the developer to settle once (Ghidra), then use
  everywhere.
- **Sprite groups** (stock records, read from the ROM's sprite table; 0 idle, 1 walk, 2 hit-stun, 3 knock-down,
  4 shout, 5-8 attacks, 12 flight or lying on androids): Mecha Frieza's actor 49: 0 1 3 5. King Cold 74: 0 1 3 4.
  Soldiers 66-71: 0 1 2. Future Gohan 54: 0 1 2 3 5 10 12. Yardrat Goku 62: 0 1 4 12. Gero's NPC 15: 0 1 12; his
  boss set 14: 0-3 5-8 12. 19 (12) and 20 (13): 0 1 2 5-8 12. 17 (9): 0-3 5-8 12. 18 (11): 0-8 12. 16 (8):
  0-4 6 12. Tien 121: 0-6 8 12. Krillin 76: 0-6 12. King Triceratops 127: 0 1 2 4 6. Player sets (53, 60, 61, 72,
  103, 128, 134, 137, 140) have the full fight and flight groups. Android 13 (165): 0-3 5-8 12. Old Hercule (158, a
  Buu's Fury import): not read here; check before giving him any animation beyond his walk and the punch (6) the
  landing already plays. **A missing group is a heap overwrite**: every shot list below stays inside these.
- Helper names used in the shot lists (the cinematic helpers of `docs/cutscenes.md` 5): **DASH** (afterimage dash),
  **STRIKE-L / STRIKE-H** (light / heavy blow with knock-back and landing dust), **FLURRY** (exchange between two
  bodies), **CHARGE** (charge-up: rising shake, flashes, the aura sound), **BLAST-1 / VOLLEY / BEAM**, **RISE /
  KNEEL / FALL**, **LOOK** (look-at). Coordinates are map pixels from the scripts; treat them as starting points.
- Ratings: **leave**, **touch up** (a few steps in the existing script), **rework** (the scene's action is
  re-staged), **showcase** (a rework worth a 20-40 s Short). Size S: under an hour of script, one lineup pass.
  M: a restated stock entry or two, all lineups. L: a choreography of 8+ beats, many actors, every lineup filmed.

## 1. The batch at a glance

| # | scene | where | rating | size |
|---|---|---|---|---|
| 1.1 | Future Gohan's training talk | `ch1_training`, map 0 trigger 0 | touch up | S |
| 1.2 | the knock-out | `ch1_knockout`, map 0 record 0 | touch up | S |
| 1.3 | **Gohan's death** | `ch1_future_end`, map 1 trigger 0 | **showcase** | L |
| 1.4 | "Sixteen years from the present day" cards | stock, map 3 | leave | - |
| 1.5 | Krillin's visit | `ch1_krillin_visit`, map 19 | touch up | S |
| 1.6 | the forest spirit and its end | `ch1_spirit`, `ch1_spirit_end`, map 14 | touch up | M |
| 1.7 | **Trunks against Frieza and King Cold** | `ch1_frieza_landing`, map 52 trigger 0 | **showcase** | L |
| 1.8 | Goku's arrival and Trunks' warning | `ch1_trunks_talk`, map 54 | touch up | M |
| 1.9 | old Hercule's talk; Goku's call; Chi-Chi at home | `ch1_old_hercule_talk`, maps 11 / 12, stock map 17 | leave | - |
| 2.1 | West City: Goku's arrival talk, Piccolo's exit | stock, map 65 triggers 1, 3 | leave | - |
| 2.2 | the mansion deal | `ch2_piccolo_deal`, map 65 record 2 | touch up | S |
| 2.3 | King Triceratops, before and after | stock map 105 trigger 0, `ch2_triceratops_end` | touch up | S |
| 2.4 | the three-year skip | `ch3_three_years`, map 7 trigger 11 | touch up | M |
| 3.1 | Amenbo Island: Yamcha, Android 20 burns the city | stock, map 143 | touch up | M |
| 3.2 | **space: Vegeta and his clone, the transformation** | `ch3_space`, `ch3_space_end`, map 168 | **showcase** | M |
| 3.3 | the landing, Hercule meets Vegeta | `ch3_landing`, map 166 | leave | - |
| 3.4 | zone 7 cutaway: Goku turns Super Saiyan before 19 and 20 | `ch3_z7_cutaway`, map 177 | touch up | S |
| 3.5 | **Goku falls, Vegeta arrives** | `ch3_z7_battle`, map 191 | **showcase** | L |
| 3.6 | 19's end, 13 leaves, 20 flees | `ch3_z7_defeat`, map 191 record 2 | rework | M |
| 3.7 | the wrecked robot's autograph | `ch3_z7_autograph` | leave | - |
| 4.1 | Gero's first stage | `ch4_gero_first`, map 215 | touch up | S |
| 4.2 | the two generators at the lab door | `ch4_gero_door_a27` / `_a40`, maps 229, 242 | touch up | S |
| 4.3 | the lab cutaway: 17 and 18 wake | `ch4_lab`, map 248 trigger 0 | leave (one beat) | S |
| 4.4 | **the lab door: 17 kills Gero, 13's revenge, Trunks' blast** | `ch4_lab_door`, map 248 item 0 | rework | M |
| 4.5 | the road: the car talk, Vegeta drops in | `ch4_road`, map 251 | touch up | S |
| 4.6 | **the road's end: "Who's next?"** | `ch4_road_end`, map 251 record 0 | **showcase** | L |
| 4.7 | the road's aftermath: they get up, Vegeta storms off | same sequence, stock 23-48 | touch up | M |
| 4.8 | the generator's eggs | `ch4_eggs` | leave | - |

## 2. Chapter 1 (scenes 1.x)

### 1.1 Future Gohan's training talk: touch up (S)

- **Where:** `ch1_training` (map 0, zone 1 area 1, trigger 0): the difficulty question, then stock `@083b6ec8`.
- **Today** (sheet `train_1`): Trunks, Future Gohan and old Hercule stand shoulder to shoulder in one clump for the
  whole talk; only facings change. Hercule's two lines come from a body that never moves. Gohan walks off at the end.
- **Lineups:** Trunks is player 1 (the only party member); old Hercule is player 2 (two players) or the companion
  that follows (one player). One picture for both.
- **Anime:** the History of Trunks special: Gohan, one-armed, in his father's colours, training Trunks on a
  wasteland bluff; Trunks cannot reach Super Saiyan and breaks down; Gohan is hard on him and kind after.
- **Shot list:**
  1. Open with a short FLURRY Trunks-Gohan (30 frames) that Gohan ends with one STRIKE-L: Trunks skids back two steps
     and KNEELs. This is the lesson the talk is about (the stock opens on a 60-frame wait). Player 1 is moved as
     char 0; Gohan is sprite 54 (attack group 5, the chop 10).
  2. Stock lines 1-5 with Trunks on his knee for "This isn't fair...", RISE at "#I won't let you down."
  3. Old Hercule starts 40 px behind them, not in the clump: at his line "#HERCULE: And don't you boys worry about
     me!\x0bI'll be guarding the rear.  From way back here." he takes two steps further back (two players: his body
     is player 2's, so stand an actor on his spot the way the knock-out does, CoopCall 31).
  4. "#A boulder?  Ha!  I've chopped bigger rocks than that in my sleep!\x0b...You go first, kid.": he steps
     forward, winds up (his punch pose 6), stops, steps back.
- **Lines kept:** all, verbatim. No new line.
- **Risks:** the scene runs right after a new game's first fade: nothing may delay control (the flurry replaces the
  stock wait, it does not add to it). Sprite 54 has no groups 6-8: the FLURRY's Gohan side uses 5 and 10 only.
- **Built 2026-11-06 (`nb-cs4`).** Stock entry 0's 60-frame wait is now the lesson: old Hercule (player 2's entity, or the companion
  alone: both are player 2's entity) hands over to an actor on his spot (`CoopCall 31`) and backs off 36 px, a 30-frame exchange
  Trunks-Gohan, and Gohan's blow puts Trunks on a knee 16 px back (`CoopCall 37, 5, 5, 16`, held); Trunks says "This isn't fair..." on
  his knee and gets up in stock entry 4's beat. Old Hercule takes two steps further back for "I'll be guarding the rear", steps up
  and winds up a punch (his group 6) for "A boulder?  Ha!", and at the end walks to where player 2 comes back (12 px left of player
  1) and makes way, so the respawn does not show. About a second longer before control than the stock's wait. `staging_ch1`
  "training" (alone and two players) shoots the knee and checks one body of old Hercule at the end.

### 1.2 The knock-out: touch up (S)

- **Where:** `ch1_knockout` (map 0 record 0 talk, after both lessons).
- **Today** (sheets `knock_1`, `knock_2`): the two argue, a long walk up the path, Gohan walks up behind Trunks,
  the chop with a white flash, Trunks lies; "HERCULE: What are you doing?!", Gohan's answer, "Sorry about that,
  Trunks", Gohan flies off, fade, Trunks wakes alone and looks left, down, up. It already reads well.
- **Lineups:** as 1.1 (Trunks; old Hercule handed to an actor).
- **Anime:** exactly this: the chop to the neck from behind, "Trunks, you are our last hope".
- **Shot list** (small): 1. the chop gets weight: ScreenShake 0, 4, 8 with the flash, Trunks FALLs (the helper's
  fall with dust instead of the instant lying pose). 2. Hercule's actor takes one step toward Gohan on his line and
  Gohan LOOKs at him for the answer, then back down at Trunks for "Sorry about that". 3. Gohan's take-off: a dust
  cloud under him. Nothing else.
- **Lines kept:** "#HERCULE: What are you doing?!" and "#GOHAN: It's not safe out there, Hercule!  You stay here and
  watch after Trunks." are Ryan's: word for word.
- **Risks:** none beyond the existing actor hand-over.

### 1.3 Gohan's death: SHOWCASE (L)

- **Where:** `ch1_future_end` = stock `@083b7adc` entry 0 (map 1, zone 1 area 2, trigger 0), old Hercule's line
  after entry 3, then the stock jump to the present.
- **Today** (sheets `death_1`, `death_2`): rain on a ruined city. Future Gohan stands on one spot in a puddle and is
  hit by ki blasts for about nine seconds (six batches, each a red flinch). **The androids are never seen**: 17 and
  18 are spawned at y 272 while the camera's view starts at y 288, so the blasts come from off screen. Dust, a white
  flash, Gohan lies in the puddle. Trunks walks up with old Hercule; four lines; "#GOHAN!!!!!!!!!!!!!"; then old
  Hercule's line; only then the stock transformation flash and the cut to "Present day Earth...".
- **Lineups:** Trunks (player 1), old Hercule (player 2 or companion, already an actor here). One picture.
- **Anime:** the special's best-known minutes: Gohan, Super Saiyan, one arm, takes on both androids alone in the
  rain; he holds 17 for a moment, 18 joins, he is driven into the ground and buried under a storm of blasts from
  both. Trunks wakes, finds him face down in the water, and his grief becomes his first Super Saiyan scream.
- **Shot list** (no line is spoken until Trunks arrives, as today):
  1. Rain, Gohan alone facing up-screen, 40 frames. One distant flash (WhiteFlash 3).
  2. 17 and 18 walk into view from the top (camera 104, 256, or spawn them at y 300 and walk to 320): 17 a step
     ahead, 18 looking away. They stop. A held look, 30 frames. Nobody pops.
  3. Gohan CHARGE (short: shake, the aura sound) and DASH at 17: FLURRY Gohan-17, 45 frames; 17 gives one step.
  4. 18 DASHes behind Gohan: STRIKE-H in the back. He is thrown down-screen, skids through the puddle (dust),
     KNEELs, RISEs.
  5. Gohan BLAST-1 at 18; she DASHes out of it. The two androids raise a hand each (17's and 18's attack group 8):
     VOLLEY from both sides, 60 frames, six to eight blasts: the stock's six batches compressed. Each hit keeps the
     stock's sound, `PlayDamageEffect` and hit-stun; the shake grows with each.
  6. The stock ending, unchanged: the velocity kicks, the explosion sound, WhiteFlash 10, music 22, he falls and the
     lying body shows in the water.
  7. The androids: 17 turns away first, both fly off up-screen (group 12) instead of being despawned in place.
  8. Trunks' walk (stock), old Hercule behind. Stock lines: "#TRUNKS: Gohan!\x0bWhat did they do to you,
     Gohan?!\x0bYou were my best friend!\x0bYou were everything to me." Old Hercule's line moves here, before the
     scream, said from a few steps back with a look over his shoulder: "#T-Trunks...  They're gone.\x0bLet's get out
     of here before those two come back!" Then "#GOHAN!!!!!!!!!!!!!" runs straight into the transformation (stock
     entry 4: the aura sound, animation 24), with a CHARGE under it (shake, two flashes) and Hercule knocked back a
     step by it. Today his line sits between the scream and the flash and lets the air out of the moment.
- **Lines kept:** all three boxes verbatim; only the order of old Hercule's line changes (ours, not Ryan's: ask him).
- **Risks:** sprite 54 has no groups 4, 6-8 (the FLURRY and BLAST poses on his side are 5 and 10); the rain layer
  plus a volley is the heaviest OBJ load of the batch (probe it with `perf`); the lying Gohan is a stock map body
  (`FindCharacterEntity 0`), keep that exact step; the first frames show the HP bars; the reference film had Piccolo
  as player 1 (Map Test), so Trunks' own animation 24 was not seen: film it from a real prologue start.
- **Size L** (M without beats 3-4: the androids in view, the volley, the reordered line).
- **Built 2026-11-06 (`nb-cs1`), the M version with script steps; beats 3-4 (Gohan's charge, the flurry, 18's blow) are marked CINE.**
  The view sits at (104, 232), 56 px above the stock's: the androids' ground is in it and the players, who wait at the trigger, are not
  (the stock had Trunks stand in view watching). 17 and 18 walk in from the top, 18 looking away, a held look; eight blasts from both
  fall as one batch (each a hit: the stock sound, the red flinch) with the shake growing 2, 4, 7; the stock ending; 17 turns away, then
  18, and they fly off the top; the view comes down for Trunks' walk. **Old Hercule's line stays where Ryan put it** (the research
  wanted it before the scream: not done). The stock transformation (entry 4) gets a rising shake and old Hercule slides back a step.
  The hand-over of player 2's old Hercule moved from a json edit into the restated entry 0. The HP bars in the research film's first
  frames come from the test warp's fade, not the scene. Flags and party after it: identical (one and two players).
  **With Cine (second commit), beats 3-4:** Gohan's charge (level 1), an exchange of 45 frames with 17 that he holds, they break; 18 is
  over him and one heavy blow drives him 36 px down into the puddle (17's mark moved to x 288 so that he lands where the stock's lying
  body appears); he gets up; then the storm. About four seconds more than the first commit, the scene as long as the stock's nine
  seconds of blasts.

### 1.5 Krillin's visit: touch up (S)

- **Where:** `ch1_krillin_visit` (map 19, Gohan's room).
- **Today** (sheet `combo_misc`, third block): Gohan at his desk, Krillin walks in, ten boxes face to face, "Krillin
  is now available!", both walk out. Clean; nobody pops.
- **Lineups:** Gohan alone (Krillin joins here; with two players he ends where player 2 appears).
- **Anime:** the Z fighters each feel Frieza's power from wherever they are and take off; Krillin's fear shows.
- **Shot list:** 1. before Krillin's first line (stock "are you feeling what I'm feeling?"), power shown then spoken:
  ScreenShake 0, 3, 40 with a low rumble, Gohan stands from the desk and LOOKs up, Krillin comes in on that. 2. On
  "KRILLIN: I'll come with you.  With Frieza around, two of us are safer than one." Krillin takes a step to the
  door and looks back. Nothing else.
- **Risks:** the stock "sit" puts Gohan inside the desk's collision (already handled at the end): stand him only by
  the same `SetEntityPositionInstant`.

### 1.6 The forest spirit and its end: touch up (M)

- **Where:** `ch1_spirit` (map 14, zone 2 area 8, trigger 0), `ch1_spirit_end` (record 1 defeat, then zone 2 area 3).
- **Today** (sheets `spGK_1`, `spGKe_1`): player 1 walks to the middle of the clearing; the lines; a long white fade
  brings Frieza and Cooler in at the top; "Your dad isn't coming back!", Cooler's line; player 1 backs one step;
  the fight. **With two players, player 2 never moves**: Krillin stood at the clearing's lower left edge, half under
  the bushes and behind the text box, for the whole scene and into the fight. After the fight: a white and black
  fade, the caption, the voice, two lines, with player 1 alone in view.
- **Lineups:** Gohan alone, Krillin alone, Gohan + Krillin, Krillin + Gohan (who is player 1 walks; the lines branch
  as built).
- **Anime:** no such scene (ours). The model is the villains' entrances: the air goes still, then the pressure.
- **Shot list:**
  1. Entry 0: with two players, place player 2 beside player 1 (CoopCall 9, 12 px left, facing up) after player 1's
     walk, so both stand in the clearing for "Huh?  The whole forest just went quiet."
  2. Before the fade: the music stops (`StopMusic`), a 30-frame hold, then ScreenShake 0, 3, 50. The stock white
     fade follows; at its end a second, harder shake (0, 8, 20) as the two stand there.
  3. Frieza's stock line. Cooler takes one step forward for his: "COOLER: And when we are finished with you, Earth
     will learn what it means to cross our family."
  4. Entry 9: both players recoil (player 2 by his entity, C side), not only player 1.
  5. The end: the two figures are not faded with the whole screen: each goes pale and bursts into dust
     (`SpawnDustCloud` on each, then the despawn) one after the other, and only then the stock white fade to the
     real clearing. The players LOOK up at the voice (player 1 does today; player 2 too).
- **Lines kept:** all as built (the reworded variants are drafts already listed in `docs/story.md` 0).
- **Risks:** Frieza's dream sprite 48 has no knock-down group, Cooler 34 none either: no pose for the "melting",
  dust only. The end warps to area 3 with entities: player 2 is away until the scene ends (as today).

### 1.7 Trunks against Frieza and King Cold: SHOWCASE (L)

- **Where:** `ch1_frieza_landing` (map 52, zone 3 area 4, trigger 0): the Z fighters gather on the cliff, the scene
  loads the ship's area (zone 3 area 5) with Frieza (49), King Cold (74), six soldiers (66-71), Trunks (128 / 134)
  and old Hercule (158), then warps back to the cliff for Trunks' invitation.
- **Today** (the whole scene filmed alone as Gohan, sheets `landG_1`, `landG_2` hold its last third; about three
  minutes at the film's pace of one A every 100 frames): the fighters walk
  up one by one; the ship's roof with King Cold and Frieza, their talk; a long camera ride down the ship as the
  soldiers march out; "TRUNKS: Not so fast!"; the face-off (Trunks and old Hercule sit at the bottom edge, **under
  the text box** for most of it); the Super Saiyan is a sound and a sprite swap; Trunks walks from soldier to
  soldier, three punches and two blasts, each soldier a puff of dust; old Hercule's one punch and the two lines;
  a two-second wait; **Trunks walks up to Frieza, one slash, a red flash, "AHHHHHHHHH!!!!", Frieza is bounced
  around by four velocity kicks and vanishes**; King Cold's offer; "Not a chance."; **one blast, King Cold bounces
  and vanishes in a red flash**; Trunks powers down; the cliff, Trunks flies up, old Hercule runs up puffing, the
  invitation, both leave. The fighting itself is about fifteen seconds of a three-minute scene.
- **Lineups:** Gohan alone, Krillin alone, Gohan + Krillin, Krillin + Gohan. **No player's body is in the fight**:
  the players are on the cliff map, hidden. The four lineups differ only in lines, as built (Krillin's "Who is this
  guy?" and "old guy with the afro" only when he is played; "who's your friend?" by Krillin or Gohan). So the
  choreography is written once.
- **Anime** (episodes "Frieza's Counterattack" to "Another Super Saiyan?"): Trunks cuts the soldiers down before
  they can aim; turns Super Saiyan with the ground breaking under him and Frieza recognising the gold; Frieza's
  blast does nothing; Frieza's great death ball, caught and lifted with one hand; Trunks' Burning Attack makes Frieza
  jump, and Trunks is already above him: **the sword comes down and Frieza is in two halves before he understands**,
  then cut to pieces, then the pieces are blasted to nothing. King Cold flatters him, asks to hold the sword, swings
  it; Trunks stops the blade with one hand and puts a blast through him, then a second to finish.
- **Shot list** (ship area; soldiers in two columns x 96 / 144, y 376-424; Frieza 120, 368; King Cold 152, 368;
  Trunks 120, 448; old Hercule 92, 440):
  1. **The entrance** (stock 15): keep the 10-frame white cut but land Trunks: he comes down from above the view
     (`FlyToPosition` to 120, 448), dust at his feet; old Hercule runs in from the bottom edge two seconds later and
     stops behind him. Lower the camera 24 px for the face-off (`PanCameraToPosition 8, 328, 2`) so the two are
     clear of the text box.
  2. Lines 16 to 32 as they are (ours included: "OLD HERCULE: Yeah!  What he said!", King Cold's "!We have
     guests!\x0b...And what is that thing with the hair?").
  3. **The Super Saiyan** (stock 33): CHARGE on Trunks: 60 frames, the shake rising from 2 to 8, the aura sound
     (55), the stock power-up animation 9, a WhiteFlash on the swap to sprite 134. The six soldiers each give a step;
     Frieza's "!It's impossible!!!" lands on the last flash. Old Hercule backs off four steps with his arm up.
  4. **The soldiers** (stock 37, first half): Trunks does not walk between them. DASH to soldier 68, STRIKE-H (he is
     thrown off the column, dust where he lands, despawn); DASH to 71, STRIKE-H; DASH to 70, STRIKE-H; the stock's
     two blasts at 67 and 69 as a two-shot VOLLEY without turning his head. Under three seconds, then a full second
     of Trunks standing still with his back to Frieza.
  5. Old Hercule's soldier and his two lines, exactly as built ("OLD HERCULE: Ha!  Did you see that, Trunks?\x0bOne
     punch!  That's the power of the World Champion!" / "TRUNKS: ...Stay behind me, Hercule.").
  6. **Frieza** (stock 37, second half, replacing the wait and the walk-up):
     a. Frieza raises his hand (his attack group 5) and fires: BLAST-1 at Trunks.
     b. Trunks DASHes out before it lands; the blast tears up the ground where he stood: dust, ScreenShake 0, 6, 20.
        Old Hercule throws himself clear of it (a run of 16 px, his own doing).
     c. Twenty frames of nothing. Frieza looks left, then right.
     d. Trunks is above him (DASH in at 120, 352): the sword (animation 16), a 4-frame WhiteFlash, the stock sound
        59, `PlayDamageEffect`, the stock red screen flash. A second cut from the other side (DASH to 120, 384,
        animation 16 again).
     e. Stock 38: "!AHHHHHHHHH!!!!"
     f. Stock 39's tumble, and before the despawn Trunks' open hand: BLAST-1 into the falling body: WhiteFlash 8,
        ScreenShake 0, 10, 30, two dust clouds, Frieza gone inside the flash.
  7. King Cold's "!Frieza!!!", the pan, the offer, "@Not a chance.", "@Very well.  If you won't join me, I'll
     destroy you!": stock, verbatim.
  8. **King Cold** (stock 47): he strides at Trunks (his walk) and rears up (his shout pose 4 is the only big pose he
     has); Trunks does not step back. A short CHARGE in Trunks' palm (12 frames) and BLAST-1 at arm's length: the
     stock hit sound, ScreenShake 0, 12, 30, King Cold thrown back by the stock velocities and left lying (group 3),
     dust. A 50-frame hold; Cold lifts his head (pose 4). Trunks walks two steps to him and fires again: the stock
     red flash, a white one over it, the despawn.
  9. The stock end: Trunks alone, turns to the camera, two seconds, powers down (animation 21). Old Hercule comes
     back out from where he hid.
- **Lines kept:** every line of the sequence, verbatim; none added.
- **Risks:** Frieza's actor 49 has no hit-stun (2) and no flight (12): he is only ever knocked down (3), moved by
  velocity, or posed on 5. King Cold 74 has no attack, hit-stun or flight group: the anime's sword swing cannot be
  drawn, which is why beat 8 is the stride and the point-blank blast. The scene loads the ship area and later warps
  back: everything above happens between the two, no actor crosses. Ten bodies stand before the blasts start (stock
  already does). The sword's animation 16 exists only on the Super Saiyan record here (134). Old Hercule's groups:
  check before his "arm up" (a plain walk backwards if in doubt).
- **Size L.** The Short: beat 3 to beat 8, about 35 seconds.
- **Built 2026-11-06 (`nb-cs1`), script steps only; the helper beats wait for `cine.c`.** What the research had wrong: the stock
  "velocity kicks" are not movement: `EntitySetVelocity` is the dust-explosion effect at an offset from the body (`docs/scripts.md` 2.1),
  so Frieza and King Cold never bounced, they stood in a ring of puffs; knock-backs are `GlideToPosition`. The view is 240 x 160 and a
  box hides 64 px of it: nobody can be clear of both the top and the bottom box across the 80 px between Trunks and Frieza, so the view
  sits at (12, 328) (whoever is spoken to is in the clear), drops to y 352 for old Hercule's punch and line (both bodies above the bottom
  box) and goes to (36, 322) for King Cold (he above the middle box, Trunks below it). Trunks comes in low from the lower left, not from
  above (a flight down x 120 would pass through Frieza). Beats as built: 1 the swoop, dust, old Hercule runs in; 3 a rising shake under
  the stock power-up, a flash on the gold, the soldiers give a step, old Hercule slides back; 4 three soldiers struck from beside them
  (a cut to the spot stands in for the DASH; each is thrown 50 px, dust, gone), back on his mark with his back turned, a blast each for
  the last two; 5 the last soldier goes for old Hercule, who has crept back up; 6 Frieza fires, Trunks is gone, dust and a shake on empty
  ground, old Hercule runs clear, Frieza looks left and right, the sword from above, again from below, Trunks slides through with his
  back to him, "AHHH", the stock explosions, an open hand, a white cut, gone; 8 King Cold strides up and roars, a blast at arm's length
  slides him back and drops him, he gets up roaring, two steps and a second blast, the stock red flash under a white one. Flags and the
  party block after the scene are identical to the build before (`staging_ch1` checks the first flag word).
  **With Cine (second commit):** the Super Saiyan is Cine's charge (level 2: the stance, rings, the growing shake, the burst) under the
  stock's sprite swap; the three soldiers are an afterimage dash and a heavy strike each (thrown 44 px, smoke where they land), and he
  dashes back to his mark; the blast into what is left of Frieza and the one under King Cold's chin are the big blast (the ball grows in
  his palm; King Cold is thrown, lies, gets up roaring); the second is a single blast. Left as cuts on purpose: Trunks leaving the view
  before Frieza's blast lands and coming down above him (a Cine dash keeps its body inside the view and 30 px off its top); the sword is
  the stock animation 16 (Cine's strike has no sword pose).

### 1.8 Goku's arrival and Trunks' warning: touch up (M)

- **Where:** `ch1_trunks_talk` = stock `@083bfddc` with our branches (map 54, zone 3 area 6, talking to Trunks).
- **Today** (sheet `arr_1`; the long private talk read from the script, its frames not looked at one by one): the
  group flies in; the player talks to Trunks; **Vegeta's and Yamcha's lines come from off screen** (only Trunks,
  the player and old Hercule are in view; the box sits over the top half); "Goku should be arriving any second
  now."; two sounds, and the camera rides up to a crater where **the pod already lies open**: the landing itself is
  never shown; Goku climbs out in his Yardrat clothes, flies down; greetings; Trunks and Goku fly to the rim for the
  warning (about forty boxes, two bodies, a handful of facing changes); old Hercule's "Psst!" (Ryan's lines); the
  caption card; home.
- **Lineups:** Gohan alone, Krillin alone, Gohan + Krillin, Krillin + Gohan; lines by who is played, as built.
- **Anime:** the pod comes down like a meteor and digs the crater; Goku steps out; Trunks takes him aside, both turn
  Super Saiyan and Goku stops every sword stroke with one finger. **The sword test cannot be staged**: Yardrat Goku
  (62) has idle, walk, shout and flight only, and the Super Saiyan Goku record (61) wears the orange gi. The game's
  story wins: no test.
- **Shot list:**
  1. Vegeta's challenge: for "VEGETA: Why not?  I'll tell you..." Vegeta's actor (137) walks into view to face
     Trunks (he "follows" him today from outside the frame); he squares up closer for "That's a lie!" (stock plays
     his shout pose there).
  2. **The landing** (stock 18): keep the two sounds, but between them: everyone LOOKs up, a WhiteFlash 6, ScreenShake
     0, 12, 40, and when the camera reaches the crater two dust clouds are still settling on the pod. Then the stock
     climb-out.
  3. Goku's flight down ends with the group stepping toward him (the stock turns them only).
  4. The private talk: three moves for forty boxes: Goku staggers a step at "#WHAT?!?  What a bummer..." (he has the
     shout pose, stock plays it), Trunks turns his back to the group at "#Quiet down or they'll hear you.", and for
     the medicine the stock step toward Goku stays. No more: the talk is the content.
  5. Old Hercule's "Psst!" as built.
- **Lines kept:** all; Ryan's "#HERCULE: Psst!  Trunks!  Is it safe to come out now?\x0bTrunks!  You didn't say we
  would be runnin' into guys like that on this trip!\x0bThat Frieza fella isn't coming back, is he?" word for word.
- **Risks:** the sequence's indexes are absolute (every stock script returns them): add only inside existing script
  entries or after the end, as `ch1_trunks_talk` already does. Sprite 62: never an attack or hurt animation.

### 1.4, 1.9 Leave

The caption cards of map 3 ("Sixteen years from the present day..."), old Hercule's "Who are you?" talk, Goku's call
to West City (maps 11 / 12, one line; not filmed) and the stock Chi-Chi scene at home (map 17 triggers 4 / 5: a
domestic argument with facing changes that already carry it; not filmed, read from the dump) are left alone.

## 3. Chapter 2 (scenes 2.x)

### 2.1 West City: Goku's arrival talk and Piccolo's exit: leave

Stock (map 65 triggers 1 and 3; filmed with Gohan + Krillin and alone as Gohan: Goku and the player among the
parade crowd, four boxes, Goku walks off; later Piccolo walks up, one line, flies off). Short and functional.

### 2.2 The mansion deal: touch up (S)

- **Where:** `ch2_piccolo_deal` (map 65 record 2: talking to Piccolo).
- **Today** (filmed with Gohan + Krillin): Piccolo's two stock boxes, Hercule runs in from the right, six boxes, the
  two walk off together. **The text box covers Piccolo and Hercule for most of the exchange** (the bodies stand in
  the top third of the view, where the box is).
- **Lineups:** Gohan alone, Krillin alone, Gohan + Krillin, Krillin + Gohan (Piccolo and Hercule join here; both are
  story actors in this scene).
- **Anime:** none (ours). Piccolo is dry and watchful; Hercule is all bluster.
- **Shot list:** 1. after the stock boxes pan the camera up 40 px (or put the lines on the bottom box) so the two
  bodies are in the clear. 2. Hercule circles Piccolo once while he asks about "Wires?  Mirrors?" (four short
  walks), looking up over Piccolo's head for the wires. 3. Piccolo turns his back on "PICCOLO: It's no trick.  Get
  lost." and only half turns back for "(A roof over my head for three years...)". 4. Hercule's victory pose (his
  group 11) on "HERCULE: Ha ha ha!  You've got yourself a deal, Green Bean!". 5. The players LOOK from one to the
  other.
- **Lines kept:** all five, verbatim.
- **Risks:** Hercule's actor is sprite 72 and a player may be showing 72 later, never here (he joins in this
  scene): safe to address by sprite. Piccolo's actor is the map's 103.

### 2.3 King Triceratops: touch up (S)

- **Where:** stock map 105 trigger 0 (the intro), `ch2_triceratops_end` (record 0 defeat).
- **Today:** a pan up to the King, eight boxes, the fight; after it he says he gives up, a slow fade to the farm.
  The King never moves; in the end scene Piccolo and Hercule stood on top of each other for Hercule's credit line.
- **Lineups:** Piccolo alone, Hercule alone, Piccolo + Hercule, Hercule + Piccolo.
- **Shot list:** 1. power before speech: as the camera reaches him the King stamps (his attack pose 6, ScreenShake
  0, 6, 16, sound 60) and only then "KING TRICERATOPS: What do you want?". 2. On "I don't have to listen to you!
  I'm King Triceratops!" he takes two steps at the players and they brace (one step back). 3. The end: he sinks
  (pose 4, as stock) with a last small shake; with both played, player 2 is placed two steps from player 1
  (CoopCall 9) instead of on him, and the one who plays Hercule steps out from the map's edge for "HERCULE: Ha ha
  ha!\x0bAnother peace treaty, brought to you by the World Martial Arts Champion!", so that "!PICCOLO: You hid
  behind a rock the whole fight." has something to point at.
- **Risks:** sprite 127 has groups 0, 1, 2, 4, 6 only.

### 2.4 The three-year skip: touch up (M)

- **Where:** `ch3_three_years` = stock `@083b88a0` with Hercule's level line (map 7, trigger 11).
- **Today** (sheet `combo_misc`, second block): three boxes in the yard, a four-second fade out and in, "Three years
  later...", four level-up captions, three boxes, Goku and the other fly off. The three years are a black screen.
- **Lineups:** any of Gohan, Piccolo, Hercule, Krillin on either pad (all four are selectable since West City); the
  stock stages Goku and "the other one" of Gohan / Piccolo as actors.
- **Anime:** the training years are a montage: Goku, Gohan and Piccolo sparring three ways in the hills.
- **Shot list:** 1. inside the stock fade (entry 7 already warps to the yard's later variation): fade in for three
  seconds on Goku (60) and the staged "other one" in a FLURRY, breaking apart, both lowering their fists; fade out;
  then the stock "Three years later..." fade-in. No line. 2. On each level-up caption the character named, when a
  player shows him, strikes one pose (Hercule's victory pose for "Hercule has learned the Ki Blast!").
- **Risks:** entry 7 is the stock script that sets the levels, refills and flag 68: restate it whole and change only
  the waits; the staged actor differs by who is player 1 (103 or 53): guard every sprite-addressed step (a step on a
  sprite nobody shows is a null write). Optional scene: last in the batch.

## 4. Chapter 3 (scenes 3.x)

### 3.1 Amenbo Island: touch up (M)

- **Where:** stock `@083ccaa4` (map 143, zone 5 area 3, trigger 0), ending in the "Meanwhile..." cut to space.
- **Today** (filmed alone as Gohan): "%s: Oh no!  Yamcha!", the group gathers round the body, Krillin flies him
  off, the talk with Android 20, **20 fires once, a white fade, and the street is burning ruins** (a second map with
  flames: the best stock image of the batch), "@NO!!!!!!!!!!!", "@You monster!!!", more talk, everyone flies off.
- **Lineups:** any of the four selectable on either pad; the stock stages "the other one" of Gohan / Piccolo.
- **Anime:** Gero's hand through Yamcha's chest; then 20's eye beams sweep the city and it goes up; Goku, shaking
  with anger, demands they fight elsewhere.
- **Shot list:** 1. 20's shot (stock 15): he turns away from the heroes first (stock), a short CHARGE (two flashes at
  his eyes' height), then VOLLEY left and right instead of the one blast, the shake building; the heroes shield
  their faces (players too) in the white. 2. In the ruins: a held four-second shot before "@NO!!!!!!!!!!!", the
  shake dying away. 3. "@You monster!!!": Goku's stock attack pose with a CHARGE flare (shake, the aura sound),
  and 19 takes half a step back behind 20. 4. The fly-off: Goku first, a beat, the androids, then the rest (stock
  order, 10 more frames between).
- **Lines kept:** all stock.
- **Risks:** entry 15 warps without entities to the burning map (5/4): actors placed again by the stock; nothing
  added may straddle it. 19 and 20: no shout or knock-down groups.
- **Built 2026-11-06 (`nb-cs1`), beats 1 and 3 in part** (two `edits` on the stock entries in `ch3_duels.json`, map 143): entry 15: 20 turns
  away, a short charge, a volley of six at the building (bursts, the last an explosion) in place of the single blast, then the stock white
  cut to the burning street; entry 17: Goku's flare (a charge of 24 frames, rings at his feet) before his stock attack pose and "@You
  monster!!!". Not built: the players shielding their faces, the held shot in the ruins, 19's half step, the fly-off's spacing.

### 3.2 Space: Vegeta and his clone: SHOWCASE (M)

- **Where:** `ch3_space` (map 168, zone 6 area 50), `ch3_space_end`.
- **Today** (film `space`, looked at in full): the caption; the clone walks up and punches twice, Vegeta flinches red;
  "#VEGETA: Tch...  A lucky shot."; one ki blast, the clone slides to the rock's edge with a shake and a flash;
  "#Pathetic..."; the clone walks back, a white flash, it is a Super Saiyan; its line; it glides over, one punch,
  Vegeta is moved to the left with a flash; the monologue (two boxes); the transformation flash; the stock lines;
  "#Only one of us leaves alive, shadow?  So be it!"; the duel (two players) or two flashes (one player); "I've
  won.  I'm a Super Saiyan."; the tutorial. The beats are right; every blow is a stand-still pose and a slide.
  The HP bars are on screen throughout.
- **Lineups:** Vegeta only (player 1). Player 2 has no body until the duel makes the clone his. One picture; the
  ending differs (duel / no duel).
- **Anime:** Vegeta alone on a dead world in a meteor storm, beaten by his own limits, giving up on everything,
  and the rage at himself is what does it. Ours trades the storm for the clone (Ryan, `docs/story.md` 4b) and keeps
  the core: he is second, he will not be second.
- **Shot list** (the asteroid: keep every body inside x 200-360, y 530-590):
  1. After the caption: both DASH in from the rock's two edges, FLURRY 45 frames at the centre, break apart skidding.
  2. The clone DASHes behind him: STRIKE-L, STRIKE-L (the two stock punches, now from his blind side, each a step
     of knock-back). "#VEGETA: Tch...  A lucky shot."
  3. Vegeta's stock blast as BLAST-1; the clone thrown to the edge as a struck body (flight, landing, dust, the stock
     shake and flash). "#Pathetic.  You're nothing but a cheap copy, made to keep the real prince sharp.\x0bKnow
     your place, shadow!"
  4. The clone RISEs (no walk back). CHARGE: the shake rising, then the stock flash and the swap to sprite 140.
     Vegeta gives one step. "CLONE: A copy?  Look again, Prince.\x0bOnly one of us leaves here alive.  I'm done
     living in your shadow!"
  5. The clone DASHes: STRIKE-H. Vegeta is thrown across the rock (the stock target 208, 576), tumbles, dust, the
     stock shake. The clone does not follow; it stands and waits. Hold 40 frames.
  6. Vegeta KNEELs through the first box of the monologue ("#VEGETA: Frieza enslaved my race...") and RISEs on
     "#No!  It is in my blood.  This is MY legend...\x0b...and I will take what is mine!"
  7. CHARGE, the long one: 90 frames, shake 2 to 10, three flashes, the aura sound; the clone is pushed back a step
     at each flash; then the stock transformation and its lines.
  8. "#Only one of us leaves alive, shadow?  So be it!": both DASH to the centre, one clash frame (flash, shake),
     and control (the duel). One player: in place of the two blind flashes, a FLURRY of 60 frames and a STRIKE-H
     that sends the clone off the rock, then "#VEGETA: I've won.  I'm a Super Saiyan."
- **Lines kept:** all, verbatim (the monologue follows Ryan's outline).
- **Risks:** player 1's body is moved as char 0 (the script already animates it: 2, 6, 34, 35); the duel start must
  come at the same entry (`CoopCall 1, -1, 15`) with the clone actor despawned first, as today; beat 8's one-player
  branch must fit in the time the two blind flashes take today (about 120 frames), not add to it. Hide the HP bars until the duel.
- **Size M.** The Short: beats 4 to 7, about 30 seconds, ending on the gold.
- **Built 2026-11-06 (`nb-cs1`, with Cine): all eight beats, the lines untouched.** They start at the rock's two edges (220 and 340);
  an exchange of 45 frames, they break; the clone is behind him: a punch, a kick; his blast throws it (Cine blast 0 + 32); it gets up,
  a charge, the gold (140 spawned on its own spot, `PushEntityPosition 137`); it is on him, one heavy blow throws him across the rock;
  40 frames; he is on a knee through the monologue's first box and on his feet for the second (a script entry between the two boxes);
  a charge of 80 frames at level 2 into the stock transformation. Two players: a clash of 24 frames, he dashes back to (208, 576),
  the stock's spot for the duel's start (`duel_end.txt` hits from there), the clone goes and the duel starts as before. One player: the
  clone stays, and in place of the two blind flashes the end sequence plays an exchange of 60 frames that his heavy blow ends (about
  125 frames against 120), a flash, the clone gone. The HUD is hidden from the first frame to the duel (the stock's second
  `SetPlayerVisible 1` in entry 0 is gone) and through the one-player fight. Not built: Vegeta's step back at the clone's gold and the
  clone pushed back at each flash of his charge. Flags and party after it: identical, one and two players.

### 3.3 The landing: leave

`ch3_landing` (map 166). Alone: one stock line as Vegeta walks down the ship's ramp. Two players: Hercule's actor
walks up, the gun exchange (seven boxes), he hands over to player 2 (sheet `combo_misc`, fourth block). It works;
the only thing worth a step is Vegeta turning his back on "#VEGETA: Hmph.  Stay out of my way." (one facing).

### 3.4 Zone 7 cutaway: touch up (S)

- **Where:** `ch3_z7_cutaway` (map 177 trigger 1; loads the battlefield, map 191, with its actors).
- **Today** (sheet `cut_1`): Goku and 19 face each other below the watchers' plateau, 20 beside, 13 to the right;
  fourteen boxes; Goku's Super Saiyan is a sound and a swap; 13 strolls up for his line and back. No player body.
- **Lineups:** none in the picture (a cutaway): Vegeta alone, Vegeta + Hercule.
- **Anime:** Goku powers up and the androids' sensors cannot read it.
- **Shot list:** 1. stock entry 9: CHARGE on Goku (shake, flashes, the stock animation 9), 19 and 20 step back, the
  watchers' shout poses. 2. "ANDROID 13: Outta date already, huh?" stays with its stroll. Nothing else.
- **Risks:** the stock swaps sprite 60 for 61 by despawn and spawn: the CHARGE goes on 60 before the swap.
- **Built 2026-11-06 (`nb-cs1`):** stock entry 9 restated: Cine's charge (level 2) on Goku's 60, the swap to 61 in a flash, the watchers' shout
  poses (Tien, Gohan, Krillin; guarded). Not built: 19 and 20 stepping back.

### 3.5 Goku falls, Vegeta arrives: SHOWCASE (L)

- **Where:** `ch3_z7_battle` (map 191, zone 7 area 15, trigger 0): stock `@083d40c8` 0-40 with 13's line and walk,
  the "clown" line, then the fight against 19 and 13.
- **Today** (sheets `btV_1`, `btV_2`): Goku lands two punches, dodges, a three-hit flurry (the stock's own little
  choreography, the liveliest in the batch); Tien and Piccolo comment; Goku staggers, 19 laughs, the virus lines;
  Goku drops out of Super Saiyan and lies; "#Ah yes...\x0bPerfect!"; Piccolo's "let's move in!" and 20's "You will
  stay where you are." with nobody moving; **Vegeta's entrance is `FlyToPosition 0, 424, 416`: in the film he had
  stood 30 px from Goku through the whole beating and shuffled a few pixels**; his long box; Yamcha flies down and
  carries Goku off; 19 and 20; 13's line and walk to his twin's spot; 20 flies up; "@So, does a machine like
  yourself ever experience fear?"; the fight.
- **Lineups:** the scene makes Vegeta player 1. Vegeta alone; Vegeta + Hercule (player 2); Hercule + Vegeta is
  turned into the second by the scene's first entry. "Watch those hands, clown" only when Hercule is played.
- **Anime** ("Goku's Assassin" to "No Match for the Androids"): Goku wins every exchange and then cannot breathe;
  19 pins him and drinks his energy through the palm; **a boot from nowhere kicks 19 off him: Vegeta**, who then
  says the line about Kakarot being his to kill, has Goku sent home, and shows them what he became: the Super Saiyan
  nobody knew he was. "Does a machine like yourself ever experience fear?" is said in gold.
- **Shot list** (Goku 61 at 464, 408; 19 at 480, 408; 20 at 464, 384; 13 at 528, 384; watchers on the plateau):
  1. The opening: the stock batches as one FLURRY Goku-19 (60 frames), then Goku's STRIKE-H: 19 thrown back 40 px,
     dust. Tien's and Piccolo's lines.
  2. The virus (stock 5-20): his aura stutters (two small shakes under the stock hit-stun poses); at entry 20 he
     KNEELs first, then the stock collapse (animation 25).
  3. After "#Ah yes...\x0bPerfect!": 19 walks onto him and holds his grab pose (attack group 8) over the body, a
     slow pulse of flashes: the drain. On "!Goku needs help!  Let's move in!" Piccolo and Tien start down from the
     plateau (two steps); 20 DASHes into their path for "#ANDROID 20: You will stay where you are..."; they stop.
  4. **The entrance** (stock 28): player 1's body comes in fast from the view's edge (a flight to 456, 408) into a
     STRIKE-H on 19: a white flash, ScreenShake 0, 12, 24, 19 thrown 60 px right, dust. Vegeta lands between 19 and
     Goku with his back to Goku. Thirty frames of silence. Then the stock box: "!VEGETA: Nobody kills Kakarot while
     I'm around.  Destiny has reserved that pleasure for me!..." while 19 RISEs and walks back to his mark
     (504, 408).
  5. Yamcha's pick-up, the 19 / 20 lines, 13's "!ANDROID 13: Not by yourself, you don't.\x0bI've been itchin' for a
     real fight all day." and his walk: as built.
  6. **The debut**, before the last line: CHARGE on Vegeta (shake 2 to 10, flashes), the transformation the way
     `ch3_space` does it (animation 35, `SetCharacterTransformation 2, 140`), 19 gives a step, the watchers' shout
     poses, 13 unfolds his arms (a facing change). Then "@So, does a machine like yourself ever experience fear?"
     and the stock fight start, Vegeta gold with a full energy bar.
- **Lineup variants:** alone: as written. With Hercule: at the first entry player 2 is placed behind and left of
  Vegeta's entry line (CoopCall 9); the kick's shake staggers him a step; the transformation pushes him back two;
  "!VEGETA: Watch those hands, clown.  They drink ki.\x0bFists...  or whatever that toy of yours is." stays where it
  is, Vegeta not turning his head. Hercule is never hit and never funny at Goku's expense.
- **Lines kept:** all stock and ours, verbatim. No new line.
- **Risks:** twelve bodies on the map and long boxes: the text-box eviction trap (`docs/cutscenes.md` 5), watch
  `ents` at every box. 19 has no knock-down or shout group (0, 1, 2, 5-8, 12): his fall and RISE are group 12 and
  back. Super Saiyan Goku (61) has no group 9 / 14 / 17: only the stock's 2, 16, 25 on him. 19 must be back on
  504, 408 and 13 on his twin's spot before `DespawnCharacterAndRecordPosition 12, 2`, or the swap shows. Starting
  the fight transformed: decide with Ryan (the form drains energy by the stock rule; the fill-up makes it fair) or
  play the CHARGE without the form. In real play the player walks into this area from its edge; the test warp puts
  him beside Goku: film it once from the area's entrance.
- **Size L.** The Short: beat 3 to beat 6, about 35 seconds.
- **Beats 1 and 2 built 2026-11-06 (`nb-cs4`):** the opening is one Cine exchange Goku wins (`CoopCall 37, 6, 60, 0`: 60 frames,
  they spring apart), then his heavy blow throws 19 28 px (he reels, lies, gets up) and both walk back to their marks before Tien's
  line (stock entry 0 restated; the stock's own punch at entry 2 stays). The virus: a small jolt under each of Goku's flinches
  (entries 5 and 7, `CoopCall 37, 15`), and at entry 20 he drops to a knee first (`CoopCall 37, 10, 0, 1`), then the stock collapse
  out of Super Saiyan (25). `staging_ch3` "battle" shoots the opening (`c3_<n>_open`) and its press count to the drain moved by one;
  flags and party after the scene equal the build before (alone and with Hercule).
- **Built 2026-11-06 (`nb-cs1`, with Cine): beats 3 (in part), 4 and 6; 1 and 2 left as the stock plays them** (its opening is the
  liveliest stock choreography of the batch: kept). The scene's trigger is the whole map, so in play the players stand at the area's
  entry (468, 556), below the view, through Goku's fight (the research film's Vegeta "30 px from Goku" was the test warp). 19 raises
  his hand over the fallen Goku (his attack 8, Goku flinches red, a flash): the drain. The entrance: Vegeta is beside 19 out of nowhere
  (Cine dash, the view staying on the fight) and kicks him 60 px (heavy); he steps over Kakarot to the stock mark with his back to the
  machine; the stock box; 19 gets up and walks back to his mark before "One of you needs to take Kakarot home". Not built: Piccolo and
  Tien starting down with 20 dashing into their path; player 2 (Hercule) stays at the entry until the fight, as before (he cannot be
  walked by a script, and a Cine dash is not his way to move). **The debut is its own commit** (Ryan's decision): before the last line
  a charge (level 2) and he stands gold: **an actor (140) on his spot while player 1's body waits out of view**, so nothing of his
  state changes (a real transformation in the scene left the form's meter running after it was undone: tried and dropped); a flash at
  the fight's start puts him back, in base form as the stock starts it. Skipped when he walks in transformed already.

### 3.6 19's end, 13 leaves, 20 flees: rework (M)

- **Where:** `ch3_z7_defeat` (map 191 record 2 defeat): stock `@083d4174`, 13's exchange, stock 1-14.
- **Today** (sheets `btVe_1`, `btVe_2`): 19 is kicked around the screen by five batches of velocity steps with red
  flashes, shakes and dust, and is gone. 13 gets up from where he lay, flies to Vegeta, three boxes, flies off.
  **Then** Vegeta's "Silly robot.  You really thought you had a chance against me?..." (about 19, said after 13 has
  come and gone). Vegeta flies up to 20, five boxes, 20 blasts the cliff, a white cut, Vegeta chases, the others
  talk and fly off; control passes to Piccolo.
- **Lineups:** Vegeta alone; Vegeta + Hercule.
- **Anime:** 19 grabs Vegeta's arms to drain him; Vegeta plants his boots on 19's face and pulls the hands off;
  19 runs in plain terror; the Big Bang Attack, one raised palm, takes everything but the head. 20 watches from a
  rock and runs.
- **Shot list** (the finishing blow around the real fight, `docs/cutscenes.md` 4):
  1. On the defeat: 19 RISEs if he lies, staggers (hit-stun), turns and runs up-screen (his walk, 48 px).
  2. Vegeta (char 0) turns after him, raises one palm (animation 34), CHARGE 30 frames, BLAST-1 heavy: the stock
     explosion chain plays as its impact (keep the five batches: they are the explosion), a white flash over the
     last, the despawn. EXP as stock.
  3. Stock entry 1 moves up to here: "VEGETA: Silly robot. You really thought you had a chance against me?..."
  4. Behind him 13 RISEs (as built), flies over; his lines and Vegeta's "#VEGETA: Another tin can?  Stay down and
     I'll make it quick." verbatim; his take-off kicks up dust.
  5. Stock 2-14 unchanged, with one addition: 20's blast at the cliff (entry 7) gets a ScreenShake and the players
     shielding their eyes.
- **Lineup variants:** with Hercule: player 2's body is where the fight left it; he LOOKs at 13 while 13 talks and
  backs off one step at "I'm comin' back for you". No line.
- **Risks:** both-down: 19 may have fallen first and be the lying body, or be the last standing: beat 1 must work
  from either (guard by `CoopCall 22, 2, 12`). The stock explosion addresses sprite 12: with the co-op twins make
  sure one 19 is left on the list. Entry 7 warps to map 192 and hands control to Piccolo
  (`SetActiveCharacterNoReset 1`): nothing of ours after it.
- **Size M.** Could be one commit with 3.5 or its own.
- **Built 2026-11-06 (`nb-cs1`, with Cine), two commits.** (a) The finishing blow: the HUD goes, 19 reels (his hit-stun), turns and
  runs 48 px up-screen from wherever the fight left him (a walk to his own position, `PushEntityPosition 12`), Vegeta turns after him,
  the ball grows in his palm with rings on the ground and flies (Cine blast 3), and the stock explosion chain plays as its impact, EXP
  and music as stock; everyone on screen looks at 13 as he gets up; dust under his take-off. The research's "five batches of velocity
  steps" are explosions round a body that stands (`EntitySetVelocity`, `docs/scripts.md` 2.1). 19 has no knock-down pose: when he fell
  first in the pair he stands frozen until this scene, so the reel works from either case. (b) Vegeta's stock "Silly robot..." moved
  up to right after the explosion (its own commit: Ryan may drop it). Not built: the shake on 20's blast at the cliff (stock entry 7
  left alone: it warps and hands control to Piccolo).

### 3.7 The wrecked robot: leave

Three captions (`ch3_z7_autograph`). A text gag that needs no staging.

## 5. Chapter 4 (scenes 4.x)

### 4.1 Gero's first stage and 4.2 the generators at the lab door: touch up (S each)

- **Where:** `ch4_gero_first` (map 215), `ch4_gero_door_a27` (map 229), `ch4_gero_door_a40` (map 242); each has a
  first-visit and a second-visit branch (flags 88 / 89).
- **Today** (film `g27PH` and the others, Piccolo + Hercule; alone as Piccolo filmed, not looked at frame by frame):
  Gero by the fence, his line; Hercule's offer; "!PICCOLO: Hercule.  Get behind me." and player 1 steps up; 13 flies
  down beside Gero; his offer; Hercule's "Run?"; the fight. Smooth already (the staging pass of 2026-11-05). The HP
  bars show throughout.
- **Lineups:** Piccolo, Hercule, Krillin on either pad: P, H, K, PH, HP, PK, KP, HK, KH. Lines by who is played, as
  built (three variants of 13's answer).
- **Anime:** Gero is hunted through these rocks by Piccolo; he absorbs energy through his palms and runs when he
  loses. 13 is ours.
- **Shot list:** 1. 13's drop-in lands: ScreenShake 0, 6, 12 and a dust cloud under him (today he floats down to a
  stop). 2. Gero raises his palm at the players on "DR. GERO: I won't let you get into my lab!" (his attack pose,
  group 5). 3. On 13's "turn around and run" he cracks his knuckles: one punch pose (6) at the air. 4. Second
  visits: 13's "ANDROID 13: Back for seconds, huh?\x0bSuit yourself." with the same landing. 5. First stage's
  defeat: Gero's flight off gets a dust cloud; 13 follows after a beat (as built).
- **Risks:** the actor Gero here is the record's sprite 14 (the boss set): group 5 is there; never group 4. Player 2
  may show any sprite: only player 1 (char 0), Gero and 13 are addressed, as today.

### 4.3 The lab cutaway: leave (one beat, S)

- **Where:** `ch4_lab` (map 248 trigger 0, loads zone 8 area 60).
- **Today** (film `lab`): the camera leaves the players for the door, the lab; Gero and 13; the pods open with their
  light, 17 steps out, then 18; the talk; 17 takes the controller and crushes it (a sound and a white flash); back
  outside. It is the stock game's best-staged scene and 13 fits in it.
- **Lineups:** no player in the picture.
- **Anime:** 17 takes the remote from Gero's hand and crushes it in his fist without looking at it.
- **One beat:** at entry 24 (the crushing) add ScreenShake 0, 4, 10 and let Gero take two steps back from him
  before "!What have you done?  You destroyed it!". 13's step back is there already.

### 4.4 The lab door: 17 kills Gero, 13's revenge: rework (M)

- **Where:** `ch4_lab_door` (map 248 item 0, the door broken by walking into it): stock `@083dae70` with entries
  67-79 appended (13 at Gero's side, his revenge).
- **Today** (sheets `door_1`, `door_2`, Piccolo + Hercule): the players at the cave mouth, Trunks, Krillin, Tien and
  Vegeta fly in; Vegeta's sneer; cut into the lab: Gero's order, 17's refusal, 18 at 16's pod; cut outside (one
  line); cut inside; "That's it.  I think it's about time I sent you to the scrap heap."; **17 walks over, one ki
  blast at arm's length, Gero is bounced by velocity steps and gone**; "ANDROID 13: DOC!", his two boxes; **two
  small blasts at 17 and 18, a white flash**, "ANDROID 17: ...Was that it?", 13's promise, he flies out the top;
  18 and 17 shrug; "Now that that's taken care of... open the chamber!"; outside Trunks fires three blasts into the
  door, a white fade, the lab is rubble with the three of them standing in it; 16 wakes; they leave; the fighters
  talk and fly off.
- **Lineups:** the nine of 4.1. The lab half has no player in it. Outside, the stock walks player 1 to the door and
  spawns Vegeta's actor (and Piccolo's only when player 1 is Gohan, which cannot happen here).
- **Anime** ("Nightmare Comes True"): Gero screams at them, 17 puts his arm through him from behind, kicks the head
  from the body and steps on it. Trunks, who knows what is in that room, levels the laboratory with everything he
  has; the three walk out of the smoke carrying 16's pod, untouched. Vegeta: you only showed them how weak you are.
- **Shot list:**
  1. Stock 22-25 as they are; after "scrap heap" Gero is still shouting at 18 (facing left), his back to 17. A
     20-frame hold.
  2. 17 DASHes to his back: STRIKE-H (17's attack 6): a white flash, the hit, Gero thrown left across the floor to
     16's pod, lying (his group 3), dust, a shake.
  3. 17 walks over without hurry (two walks). The stock blast, point-blank, as BLAST-1: the stock flash and the
     velocity steps, the despawn.
  4. 18 has not turned round: she turns now, looks at the spot, looks at the pod.
  5. 13: at beat 2 he starts forward one step and stops when 17 looks at him. "ANDROID 13: DOC!" and his two boxes
     verbatim. Then CHARGE (shake, two flashes: his power shown before it fails) and a VOLLEY at both instead of
     the two single shots; it bursts on them in dust; the dust clears and neither has moved. "ANDROID 17: ...Was
     that it?" His last line, his take-off, a burst of dust at the ceiling where he goes through.
  6. "ANDROID 18: He's heading north.  Should we bother?" / "ANDROID 17: Forget him." and stock 27.
  7. Outside, stock 29-30: a CHARGE on Trunks through "If they set free that Android it'll be the end of us all!
     No!!!!!" and the three stock blasts as a BEAM; the fighters and both players thrown a step back and shielding;
     the stock white fade. In the rubble the stock pan holds two seconds on the three silhouettes before Vegeta's
     "Bad move. What did you go and do that for?".
- **Lineup variants:** outside, with two players, player 2 is placed beside player 1 at the door (CoopCall 9) as the
  scene starts and both LOOK at each arrival; at Trunks' blast player 1 steps back by char 0, player 2 by his
  entity. Nothing else differs.
- **Lines kept:** all stock; 13's five boxes are drafts as built (2026-11-06) and stay as they are.
- **Risks:** the scene loads the lab twice and restores the outside map twice (entries 6, 14, 16, 28): 13 is
  spawned again after each load (67, 79), and anything of ours must begin and end inside one visit. Stock indexes
  are absolute: new entries after the end with `->` back. Gero in the lab is record sprite 14 (groups 0-3, 5-8, 12:
  the knock-down exists). The blasts at 17 and 18 are guarded by `CoopCall 22, 2, sprite` today: keep the guards.
  Stock entry 59 is Piccolo's line ("They can't be headed for West City...") and plays in every lineup; when nobody
  plays him it has no body (an existing gap, the presence rule's business, not this scene's).
- **Size M.**
- **Built 2026-11-06 (`nb-cs1`), by Ryan's approved flow: every beat and line kept, weight only.** Not built from the shot list: its
  beat 2 (17's STRIKE-H before the blast) would be a new beat, and the outside half (Trunks' blasts as a beam) is untouched. Entry 26
  restated: 17 walks over, a held look, the blast at arm's length, a flash and a shake, Gero slides 28 px and falls (his group 3), the
  stock explosions over the lying body, gone; 13 starts forward and stops when 17 looks at him; 18 only now looks round and turns back
  to the pod. Entry 73: the room shakes as 13 powers up, each blast now lands (the hit sound and a dust puff on 17, then on 18; before,
  the white flash came while they were still flying), the flash, a shake, 18 already looking at the pod again. Entry 76: dust under his
  take-off, a shake as he goes through the ceiling. `ch4_staging` films it (P, PH) and checks Gero gone and 13, 17, 18 standing.

### 4.5 The road: the car talk, Vegeta drops in: touch up (S)

- **Where:** `ch4_road` (map 251, zone 8 area 49, trigger 0).
- **Today** (sheet `rdPH_1`): 17, 18 and 16 by the roadside, twenty-two stock boxes with facing changes (it plays as
  it should: three bored machines); a white cut brings the players up the road; Hercule's bow-out; Vegeta floats
  down; "!VEGETA: Out of my way, clown.\x0bThe machines are mine.  Namek, try to keep up."; a fade swaps player 2
  to Vegeta; 17 picks him; the fight.
- **Lineups:** as 4.1 on entry; with two players the fight is player 1 + Vegeta (player 2), alone it is player 1.
- **Anime:** Vegeta, gold, lands on the highway in front of them and challenges 18; she walks out to meet him.
- **Shot list:** 1. Vegeta's landing: fast, ScreenShake 0, 8, 14, dust (today a slow descent). 2. After "ANDROID
  18: Fine by me." she walks four steps out onto the road toward the players and 17 leans back against the
  guard-rail side (a facing away, then back): the fight starts from her walk. 3. Alone: after "Never mind him.
  I'll handle this." the same walk-out. 4. Hercule's bow-out: when player 1 is Hercule he backs off two steps on
  "Hey, look at the time!" (char 0); player 2's Hercule cannot be walked by sprite: leave him.
- **Risks:** the stock fight start despawns the three actors and spawns the boss by `TriggerMapEvent 0`: 18's actor
  must end her walk on the record's spot (280, 264) or the swap shows.

### 4.6 The road's end, "Who's next?": SHOWCASE (L)

- **Where:** `ch4_road_end` (map 251 record 0 defeat): stock `@083d98ac` entries 0-2 with player 2's rebinding.
- **Today** (sheet `rePH_1`, Piccolo + Vegeta after the fight): 18 falls, gets up, one blast, player 1 is hit and
  lies; "ANDROID 18: Nice try!\x0bI was almost scared for a second there.\x0bWho's next?"; **a two-second fade to
  black; when it lifts, Vegeta, Piccolo, Trunks, Tien and Gohan all lie on the road** and 17 strolls among them.
  The whole fight the anime is remembered for happens inside the fade. In the film Vegeta (player 2) was still on
  his feet during "Who's next?": the line is an open door.
- **Lineups:** player 1 Piccolo, Hercule or Krillin; player 2 Vegeta (two players) or nobody. After the scene's
  rebinding player 2 is Hercule again (Piccolo when player 1 is Hercule).
- **Anime** ("Double Trouble for Goku" to "Last Ditch Effort"): Vegeta, sure of himself, fights 18 down the highway
  and she never tires; **she breaks his arm with one kick**; Trunks cannot watch and comes in with the sword, which
  chips on her forearm; 17 steps in and drops him from behind; Piccolo and Tien go for 17: one blow for Piccolo, a
  headlock for Tien. Krillin, the only one standing, cannot move his legs.
- **Shot list** (replacing the `FadeOut 120` of stock entry 2; no line is spoken in it; about 25 seconds):
  1. Vegeta answers. Two players: player 2's body is handed to an actor of his sprite on his spot (the CoopCall 31
     pattern of chapter 1) so the script can move him. One player: Vegeta's actor (137) lands from the sky as he
     does in the co-op opening. CHARGE, Super Saiyan (140).
  2. DASH at 18: FLURRY Vegeta-18, 50 frames; she gives two steps and none of it lands. They break.
  3. Vegeta's BLAST-1; 18 DASHes through it. **STRIKE-H: her kick on his arm**: a short red screen flash, the hit
     sound, ScreenShake 0, 12, 20. He KNEELs, holding (hit-stun held). A second, casual STRIKE-L: FALL; he turns
     back to black hair (animation 21) lying.
  4. Trunks flies in from the lower left (128): DASH and the sword (16) at 18: she stops it on her forearm, a
     3-frame flash, twelve frames of the two locked. 17 DASHes behind Trunks: STRIKE-H, FALL. Gohan, who came with
     him, is swatted aside by 18 without a look (STRIKE-L, FALL): the stock has him lying there afterwards.
  5. Piccolo. Player 1 is Piccolo: he RISEs from where 18's blast left him (char 0), takes one step at 17; 17 does
     not turn his head: STRIKE-H, FALL. Player 1 is Hercule or Krillin: he stays down, and Piccolo's actor (103)
     flies in with Tien and takes this beat (the stock ending has Piccolo lying here in every lineup: kept for the
     plot, `docs/story.md` 0).
  6. Tien (121) last: DASH at 17's back; 17 catches him and drops him (STRIKE-L, FALL).
  7. Silence, 40 frames, the bodies where the stock's lying bodies are. A 6-frame white cut covers the swap to the
     stock's own bodies (`FindCharacterEntity 0-4`) and player 2's rebinding (today hidden by the fade).
  8. The stock from "17 strolls": his walk through them, the pan to Krillin shaking at the roadside, "ANDROID 17:
     You look scared.  Just relax.  You're not the one we're looking for, okay?\x0bWe want Goku."
- **Lineup table:**

  | lineup at the fight | Vegeta | Piccolo's beat | player 1 in the montage |
  |---|---|---|---|
  | Piccolo alone | actor, lands from the sky | player 1's body (char 0) | rises, is struck by 17, falls |
  | Piccolo + Vegeta | player 2's body, handed to an actor | player 1's body | the same |
  | Hercule or Krillin alone | actor | Piccolo's actor, with Tien | stays down from 18's blast |
  | Hercule or Krillin + Vegeta | player 2's body, handed to an actor | Piccolo's actor, with Tien | stays down |

- **Lines kept:** "Nice try!..." and everything after, verbatim. No new line: the montage is silent.
- **Risks:** the most actors of the batch (17, 18, 16, Krillin, Vegeta, Trunks, Gohan, Piccolo, Tien, both
  players): bring them in one pair at a time and turn each fallen actor into nothing more than a lying pose; no text
  box is open during it, so no eviction. Which of the stock bodies 0-4 is whose, and where each lies, must be read
  from the map before placing the falls. With the twins, the first of 17 / 18 to fall stays lying on the map until
  the pair's defeat: check that the stock `SpawnCharacterEntity 9` does not leave two 17s (film `ents` here).
  Stock entry 2 also clears and sets the selectable bits and stock 23-27 stand the bodies by who player 1 is:
  restate entry 2 whole, leave 3-48 at their indexes. Tien has no group 7; Gohan's actor is 53. Player 1 Krillin
  meets his own NPC (76) in the stock ending: the duplicate rule's case, look at it in the lineup film.
- **Size L.** The Short: from "Who's next?" to Krillin's shaking, 30 seconds, the batch's best candidate after 1.7.
- **Built 2026-11-06 (`nb-cs1`, with Cine), about 13 seconds, silent.** As built: a 6-frame white cut puts the stage where the stock
  ending has it (the view 152, 296; 16 by the roadside; 17 beside 18: **the fight's own twin when there is one**: the stock ending
  spawned a second 17 beside the twin the fight left standing, now there is one). Vegeta: with two players player 2's own body (Cine's
  body -1, in whatever form the player left him), alone his gold actor (140) down from the sky; nobody is handed to an actor, because
  `CoopCall 31, 3` leaves player 2 without a body until control returns and the ending needs him. His charge, an exchange of 50 frames,
  18 over him, the kick (heavy, he ends on a knee, a red flash), a second kick, he lies. Trunks and Gohan fly in; 18 is beside Trunks
  before he moves, his swing stops on her (a flash), 17 is over him; Gohan is kicked down. Piccolo: player 1's body gets up (knee, feet)
  or his actor arrives with Tien; 17 puts him down; Tien dashes for 17's back and 17 is beside him first. 40 frames; a second white cut
  swaps to the stock's lying bodies (everyone falls within about 20 px of his stock spot), does the stock's flag changes and player 2's
  rebinding, and stands player 2 by the roadside (Hercule: in view for his line; Piccolo, when player 1 is Hercule: off the stage). Not
  built: Vegeta's blast that 18 dashes through (two Cine moves cannot overlap), the transformation (alone he arrives gold; a played
  Vegeta keeps his form). Player 1 as Hercule or Krillin lies by the roadside where the blast left him. The stock bodies: 0 Tien
  (208, 432), 1 Piccolo (252, 416), 2 Gohan (288, 432), 3 Vegeta (224, 400), 4 Trunks (236, 440) (`FindCharacterEntity n` shows body n,
  `FindEntityByType n` removes it). Flags, party and player 2's slot after the scene: identical in five lineups.

### 4.7 The road's aftermath: touch up (M)

- **Where:** the same sequence, stock entries 3-48 with Hercule's backing line.
- **Today** (sheets `rePH_1`, `rePH_2`): 17's talk, 16 and the birds, Krillin's plea, "HERCULE: Y-yeah!  What the
  little guy said!...", "Good luck!  Bye!", the kiss, they fly off one by one, "!What am I doing?"; then **each
  lying body is swapped for a standing one in a single frame** (Vegeta, Piccolo, Trunks, Gohan, Tien, one per box);
  "!VEGETA: ARRRRRGGGHHHHHH!!!!!" and he flies off; twenty boxes of talk in a standing group; Piccolo's outburst and
  exit; the Kami talk; fade.
- **Lineups:** as 4.6 after the rebinding; Hercule's line only when he is played (as built).
- **Anime:** Krillin's Senzu beans; Vegeta, arm healed and pride not, screams at the sky and leaves; Piccolo walks
  away from all of them to become one with Kami.
- **Shot list:** 1. each stand-up (stock 23, 25, 27) is a RISE instead of a pop. 2. Vegeta: he RISEs with his back
  to everyone, then the scream with a CHARGE (shake, a flash, dust blown out from him) and the stock take-off;
  Trunks takes two steps after him for "!TRUNKS: Father!  Where are you going?". 3. The kiss: 18's walk is there;
  Krillin's shout pose holds four seconds after she has gone, then "!What am I doing?". 4. Piccolo's "This is
  something that I have to do ALONE!!!!": a short shake, the others give a step, his take-off with dust.
- **Risks:** entries 23, 25, 27 place player 1's own body or an actor by who plays: the RISE has both cases.
  Absolute indexes throughout.

### 4.8 The generator's eggs: leave

`ch4_eggs` (map 252), staged on 2026-11-05 (sheet `combo_misc`, last block; alone as Piccolo): the egg cracks with a
flash, the mother charges round the generator shaking the ground, her portrait lines, the hatching, the walk-off
with the second baby looking back. It is already what this document asks of the others.

## 6. Priority list for the developer (build order, showcases first)

**State (2026-11-06, branch `nb-cs1`): built 1.7, 4.6, 3.5, 3.6, 3.2, 1.3, 4.4 (items 1-6: each scene's "Built" note says what and what not), 3.1 in part (item 7), and of item 8 the charge of 3.4, the landings of 4.2, the shake of 4.3 and Vegeta's landing of 4.5. Not built: 1.6 (item 7), 1.1, 1.2, 1.5, 1.8, 2.2, 2.3, 2.4, 4.1, 4.7 and the rest of 3.1 / 4.2 / 4.5 (the HUD is still shown at the generators).**

Measured on the build (A pressed every 100 frames, so each figure is good to about 2 s; `perf` on the two heaviest): 1.7 +14 s, 1.3 +5 s,
3.5 +7 s (the entrance and the debut), 3.6 +3.5 s, 4.6 +9 s (13 s of fight in place of a 2 s fade), 4.4 +2 s, 3.2 3.5 s shorter. Gohan's death in
the rain: 12 OBJs at most, no scanline cut, no lagged frame; the road's montage: 20 entities, 11 OBJs, none cut. Every added second is action:
no scene waits longer before control than its new beats take, and Start skips a helper move in progress.

1. **1.7 Trunks against Frieza and King Cold** (L). No player body in the fight, so one choreography for every
   lineup: the cheapest showcase to test and the scene every fan knows. It is also the helpers' first full use
   (DASH, STRIKE-H, BLAST-1, CHARGE).
2. **4.6 The road's end, "Who's next?"** (L). The biggest gain: the fight is a black fade today.
3. **3.5 Goku falls, Vegeta arrives** (L), with **3.6 19's end** (M) as its second commit.
4. **3.2 Space: Vegeta and his clone** (M). One lineup, a small map, every helper.
5. **1.3 Gohan's death** (L). One lineup; needs the OBJ load probed in the rain.
6. **4.4 The lab door: 17 kills Gero, 13's revenge** (M).
7. **3.1 Amenbo Island** (M) and **1.6 the forest spirit** (M): the two touch-ups with the most to gain.
8. The small touch-ups in play order: 1.1, 1.2, 1.5, 1.8, 2.2, 2.3, 3.4, 4.1 / 4.2, 4.3, 4.5, 4.7; 2.4 last
   (optional).

**The batch's best Short.** Trunks against Frieza (1.7, beats 3 to 8): it needs no knowledge of the mod, it is the
most replayed thirty seconds of the saga, and today's version undersells it more than any other scene: a walk-up, a
slash and a bounce. Cut it from "!So this is a Super Saiyan..." with the ground shaking: three soldiers gone in
afterimages, Frieza's blast hitting empty ground, the silence while he looks for him, the sword from above, the
pieces blasted, then King Cold's stride into an open palm. Old Hercule in the corner of the frame, diving for cover
and then claiming a soldier, tells the viewer whose game this is without a word of explanation. The road's end
(4.6) is the second Short and the better surprise for people who played the stock game, because they remember the
fade.

## 7. Could not film, and open points

- **Not filmed** (read from the scripts and dumps only): Goku's call to West City (maps 11 / 12), old Hercule's
  "Who are you?" talk, the Chi-Chi scene at home (map 17), the jungle's two approach scenes (maps 97, 98), the
  wrecked robot, the two-player duel itself in space (only the one-player flash ending was filmed), Gero's stage
  defeats beyond their first frames, the first-stage defeat's fly-off.
- **Filmed but only partly looked at:** the Goku-arrival talk after Goku lands (the forty private boxes: read from
  the script), the generators and the road alone as Piccolo (sheets `reP_1`, `reP_2` exist), the forest spirit alone
  as Krillin, zone 7 with Hercule as player 2 (sheet `btVHe_1`).
- **One lineup per scene** in most films (the richest one). Every lineup must be filmed by the developer
  (`docs/cutscenes.md` 7); the lineup variants above come from the scripts' branches, not from frames.
- **My first road-end film failed**: test command 7 (win the fights) poked while the opening scene still ran was
  not applied and the idle players were beaten for 400 frames. Wait for the scene's end first (`stepz 0x03001FDC`),
  as `ch4_staging_gen.py` does.
- **Facts to settle before building:** how the HP bars are hidden in a scene; old Hercule's (158) pose groups;
  which stock body 0-4 on the road is whose; whether Vegeta enters the 19 fight transformed (Ryan's call); the
  order of old Hercule's line at Gohan's death (ours, but ask).
- **DRAFT lines after the build: none new** (no line was added by the development pass).
- **DRAFT lines in this batch: none new.** Every shot list keeps the lines as they are today (the drafts already
  listed in `docs/story.md` 0 stay drafts). Two lines move: old Hercule's at Gohan's death (1.3) and Vegeta's stock
  "Silly robot..." (3.6), both to sit next to what they are about.
