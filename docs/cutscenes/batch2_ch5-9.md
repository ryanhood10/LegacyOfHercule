# Cutscene research, batch 2: chapters 5 to 9 and the ten-day wait (2026-11-06)

Research only: no game data was changed. The guidelines are `docs/cutscenes.md` (this file is its section 6
"Research"); the mechanics are `docs/howto.md` "Stage a scene". Scripts: `data/story/ch5_lookout`, `ch6_basement`,
`ch3_duels` (the Time Chamber half), `a16_island`, `ch7_cell`, `ch8_cooler`, `ch8_gym`, `wait_spar`, `ch9_gero`,
`broly`; the stock scenes and the merged state of every map from `python -I tools/script_asm.py dump map Z A --rom
build/ref/log2-coop.gba` (it reads the built ROM, so a dump shows our edits and the stock entries in one listing).

## 0. How it was filmed, and what the films cannot tell

- Reference ROM `build/ref/log2-coop.gba` + `build/ref/menu.ss`, read only, through `tools/headless/st_test.py`. Each
  scene: Map Test entry 7, poke the lineup, warp into the trigger (recipes of `staging_ch568_gen.py`,
  `staging_ch9_gen.py`, `cell_staging_gen.py`, `lead_scenes.txt`; section 12 lists the flags and warps that worked),
  A once a second, a shot every 30 frames with the scene flag (`0x03001FDC`) and the map printed beside it. Frames
  whose picture outside the text box did not change are dropped, the rest tiled 6 across. Everything is in
  `build/research2/film/` (not committed): `fl.py`, `gen_all.py`, `gen_c.py`, `run.sh`, `sheet.py`, `zoom.py`, one
  folder a film (`<tag>/`, sheets `<tag>_s1.png` ...). 37 films, named below as "film `fus_TG`" (the letters are the
  lineup: P Piccolo, H Hercule, T Trunks, G Gohan, K Krillin, V Vegeta, A Android 16).
- **Not filmed** (read from the scripts only): Roshi's talk in the atrium and the dojo's sparring match
  (`wait_spar`); the talks after Perfect Cell leaves (16 asking for repairs, Vegeta's "Foolish boy", zone 12 area 6
  records 2 and 3); the Capsule Corp receptionist; the gym's animal rounds past their opening line. **Lost to a game
  over:** the one-player island film (`isl_P`) and the first zone 12 films' last stage: an idle player dies within
  seconds against Android 16 and Perfect Cell, so a test must win the fight at once (`stepz 0x03001FDC`, then test
  command 7; `z12b_VT` / `z12b_V` were made that way). The island and Cell's arrival were seen with two players
  (`isl_PH`); alone they are the same script without player 2.
- **Box positions, as filmed:** a line with no prefix or `#` opens the box at the bottom, `!` at the top, `@` in the
  middle of the screen; `@^` is the small centred caption. Shot lists below name the prefix when a box would sit on
  the action; changing a prefix changes no word.
- **Player 2 has no body in a scene that follows a warp.** After `WarpToMapNoEntities` (and in the areas a scene
  flies through) player 2's entity is gone until player 1 is free. Filmed: Hercule's "T-two bugs?!" in the crater
  with no Hercule (`gin_PH`); Hercule left by a palm off screen for the whole of Cell's arrival (`isl_PH`); Trunks'
  debrief line in the backyard with only Vegeta and Bulma on screen (`rim_VT`); nobody beside Vegeta in the ship.
  Where a shot list says **stand-in** it means: an actor of player 2's sprite on his mark for the scene (the
  `CoopCall 31` pattern when his entity stands, a plain spawn by slot when it does not), gone before control so the
  respawn 12 px left of player 1 does not show a second body. Where his entity does stand (`CoopCall 31, 0`), he is
  placed with `CoopCall 9, 1, x, y` and moved through the cine helpers' player-2 form.
- **After a fight the camera sits on the player wherever the fight ended.** The island's break plays over a tree
  with nobody in view; Cooler dies off screen. Every defeat scene needs marks behind a short fade first (the way
  `ch7_perfect_end` entry 1 already does).
- **The HP bars stay up** in several scenes (Kame House's first lines, the island, Perfect Cell's speech between his two stages,
  Cooler). Batch 1 raised the same point: settle once how to hide them, then use it in every showcase.
- **Sprite groups** (read from the reference ROM's table at `*0x0800959C`; 0 idle, 1 walk, 2 hit-stun, 3 knock-down,
  4 shout / start, 5-8 attacks, 12 flight or lying, 18 / 19 the beam): **Kami 73: 0 only** (he can stand, turn and
  be glided or flashed, nothing else). Mr. Popo 93, Dr. Briefs 41, Roshi 88, Dende 36: 0 1. Bulma 23 / 24, Chi-Chi
  33, long-haired Trunks 131: 0 1 4. Android 16's actor 8: 0-4 6 12; his boss set 160: 0-3 5-8 12 13. 17 (9): 0-3
  5-8 12. 18 (11): 0-8 12. 19 (12) and Cooler (34): 0 1 2 5-8 12. Imperfect and Semi-Perfect Cell 28 / 29: 0-3
  5-10 12 18 19. Perfect Cell 30: 0-3 5-8 12 18. **Tien 121: 0-6 8 12, no beam group.** Yamcha 156: 0-8 12. Krillin
  76: 0-6 12. Hawks 63 / 64: 0 1 2 6. Broly 159: 0-8 11-13 17 20 (no 18 / 19). Paragus 170: Vegeta's full set. The
  Namekian 166 and the player sets (53, 56, 60, 72, 103, 128, 132, 134, 137, 140, 161, 175) have the fight and
  flight groups. The stock scripts play Cell's absorption with animation numbers above the group count (27 tail up,
  25 swallow, 28, 9 on sprites 28 / 29; 29 with `LockEntityPose 11, 41` on 18; Piccolo's lost arm 22 and its
  regrowth 26 on 103; 35 the Super Saiyan change): reuse them as the stock scenes do, do not invent neighbours.
  **A missing group is a heap overwrite.**
- Helper names (as batch 1): **DASH** (afterimage dash), **STRIKE-L / STRIKE-H** (light / heavy blow with knock-back
  and landing dust), **FLURRY** (exchange between two bodies), **CHARGE** (charge-up: rising shake, flashes, the
  aura sound), **BLAST-1 / VOLLEY / BEAM**, **RISE / KNEEL / FALL**, **LOOK** (look-at). `FlashScreenColor`,
  `GlideToPosition`, `LockEntityPose` and `EntitySetVelocity` are stock steps the scenes below already use.
  Coordinates are map pixels from the scripts: starting points.
- Ratings: **leave**, **touch up**, **rework**, **showcase**. Sizes S / M / L as batch 1.
- **No new line is proposed anywhere in this batch.** Every quoted line is in the game today.

## 1. The batch at a glance

| # | scene | where | rating | size |
|---|---|---|---|---|
| 5.1 | the Lookout, first visit: Kami defers | `ch5_lookout_first` 0-19, map 266 (9/6) trigger 0 | touch up | S |
| 5.2 | Gohan's house: Bulma's photo, "I'm coming with you, Trunks!" | same sequence 20-39, plays on 2/20 (stock) | leave | - |
| 5.3 | the time capsule | stock, map 275 (10/8) trigger 0 | leave | - |
| 5.4 | the shed skin; Kami's vision and the news from Gingertown | `ch5_fusion` 0-35 (stock), map 283 (10/16) | leave | - |
| 5.5 | Kami draws out Hercule's potential | `ch5_fusion` 36-46 | touch up | S |
| 5.6 | **the fusion** | `ch5_fusion` 47-59, plays on 9/4 | **showcase** | M |
| 5.7 | **Gingertown: "call me The Namek", the town blown away** | `ch5_gingertown`, map 284 (10/17) | **showcase** (with 5.8) | M |
| 5.8 | **Cell's Kamehameha, the arm, Cell's story, the escape** | `ch5_gingertown_end`, map 287 (10/20) record 0 | **showcase** | L |
| 6.1 | the lab basement: larva, computer, self-destruct, the cave | `ch6_larva`, `ch6_computer`, `ch6_lab_exit` | touch up | S |
| 6.2 | Kame House: Goku wakes | `ch6_goku_wakes`, map 295 (11/3) | leave | - |
| 6.3 | the Lookout: "only two of us can train" | `ch6_lookout` 0-17 (stock), map 263 (9/3) trigger 1 | leave | - |
| 6.4 | **the Time Chamber duel, Vegeta and Trunks** | `ch6_lookout` 18-22, `ch6_chamber_end`, zone 15 area 2 | **rework** | M |
| 6.5 | Kame House: the androids land, Piccolo's deal with 16 | `a16_kame_house`, map 293 (11/1) trigger 3 | touch up | S |
| 6.6 | the island: before the fight | `a16_island_intro`, map 296 (11/4) | touch up | S |
| 6.7 | "This island is breaking apart!" | `a16_island_break`, map 296 record 1 | rework | S |
| 6.8 | **Cell arrives, 17 is absorbed, Tien's stand, Goku's rescue** | `a16_island_half` + stock `083e6078`, map 298 (11/6) | **showcase** | L |
| 6.9 | the Lookout after: "That was an honorable move, Tien" | stock tail of the same sequence, 9/1 | leave | - |
| 7.1 | out of the Time Chamber: the armour | stock, map 263 trigger 2 (one edit) | touch up | S |
| 7.2 | Vegeta and Semi-Perfect Cell: "a power that has not been seen for thousands of years" | `ch7_semi_perfect`, map 302 (12/4) | rework | M |
| 7.3 | "push-ups and sit-ups", "Here's my son!", the deal | `ch7_semi_perfect_end` 0-20 | touch up | S |
| 7.4 | **Cell absorbs 18; Perfect Cell; the kick; Vegeta falls** | `ch7_semi_perfect_end` 21-26, 12/5 and 12/6 | **showcase** | L |
| 7.5 | Trunks' vow, Krillin flies in, Trunks powers up | `ch7_semi_perfect_end` 27-37 | touch up | S |
| 7.6 | **Krillin's one hit, "pump up your muscles", the tournament** | `ch7_perfect_end`, map 304 (12/6) record 1 | **rework** | M |
| 7.7 | 16 asks for repairs; "Foolish boy" | stock talks, map 304 records 2, 3 | leave | - |
| W.1 | Capsule Corp: Roshi; 16's memory; Cell's broadcast; "Meanwhile..." | stock, maps 147, 149, 160; `broly_tv` | leave | - |
| W.2 | Cooler, the first meeting | stock, map 97 (4/50) trigger 2 | touch up | S |
| W.3 | Cooler's fight: the opening and his end | `ch8_cooler`, map 172 (6/61) | rework | S |
| W.4 | Roshi's gym: two duos and the animals | `ch8_gym`, map 175 (6/70) | touch up | S |
| W.5 | the dojo's sparring match | `wait_spar`, maps 147, 175 | touch up | S |
| 9.1 | Briefs' workshop: 16 wakes | `ch9_workshop`, map 149 (6/5) | touch up | S |
| 9.2 | the two generators | `ch9_gen_west`, `ch9_gen_east`, maps 229, 242 | rework | S |
| 9.3 | the rubble | `ch9_rubble`, map 250 (8/48) | leave | - |
| 9.4 | the bunker: two 19s; the file on 13; the computer smashed | `ch9_bunker`, `ch9_bunker_won`, map 257 (8/63) | touch up | S |
| 9.5 | 16 and the birds; home | `ch9_bunker_won` 11-27 | touch up | S |
| B.1 | Bulma's news; Paragus in the backyard | `broly_tv`, `broly_yard`, maps 160, 165 | touch up | S |
| B.2 | the ship: flight out, landing, flight home | `broly_board`, `broly_ship_out`, `broly_end` 7-11 | leave | - |
| B.3 | **the rim: the betrayal, Broly** | `broly_rim`, zone 15 area 3 | **showcase** | M |
| B.4 | **Broly's rage** | `broly_rage` | **showcase** (with B.3) | S |
| B.5 | **Broly's end, the planet** | `broly_end` 0-7 | **showcase** (with B.3) | M |
| B.6 | the debrief | `broly_home_out`, map 165 | touch up | S |

## 2. Chapter 5: the Lookout, the fusion, Gingertown

### 5.1 The Lookout, first visit: Kami defers (touch up, S)

- **Where:** `ch5_lookout_first` entries 0-19 (0-16 stock), map 266 (zone 9 area 6) trigger 0. Films `lk1_P`,
  `lk1_PH`.
- **Today:** a dark throne room. Kami stands on a lit dais at the top of the view, Piccolo at the bottom edge, the
  `@` box between them for sixteen lines; neither moves once. With two players Hercule stands beside Piccolo,
  half cut by the screen's edge; his line and Piccolo's "don't touch anything" open at the top. Fade to Gohan's house.
- **Lineups:** the fade makes Piccolo player 1 whoever walks in. Piccolo alone; Piccolo + Hercule (his two lines);
  Piccolo + Krillin or Gohan (no line; a body beside Piccolo).
- **Anime:** Kami at the Lookout's edge with his back turned, already knowing why Piccolo came; a tense talk, no
  blows; Piccolo ends it by going to the edge to wait, arms crossed, while Kami watches the Earth.
- **Shot list** (Kami has one pose group: he only turns):
  1. Open on Kami facing up, away (`SetEntityFacing 73, 1`). Piccolo walks in two steps from the bottom
     (`WalkToPosition 0`), camera panned so both have room. Kami turns to him on "@KAMI: Piccolo.  You've returned."
  2. Before "@Stop smiling.  I'm not doing this because I want to. ...": Piccolo turns his back (facing 0).
  3. Before "@Then we agree!": he turns back and takes a step up.
  4. Before "@You want to see how things "play out"?  Have you lost your mind?": a step closer, `ScreenShake 0, 4, 8`
     (his temper, once).
  5. Before "@Have it your way.  You're scared.  I know.  If you need more time, then take it.\x0bI'll be waiting.":
     he walks to the bottom mark and stops with his back to Kami; Kami turns away again (facing 1). Hold 30 frames.
  6. Player 2 (when there): placed beside the door mark, LOOK at each speaker; Hercule looks about the room
     (two facings) before "!HERCULE: So, uh...  we're waiting?  Up here?\x0bGreat!  Nice view.  Nice and far away from
     those Androids!" and Piccolo turns to him for "!PICCOLO: You stay here, Hercule.  And don't touch anything."
- **Risks:** Kami's sprite (never walk, never animate); the room is taller than the view, so the pan must keep both
  heads clear of the middle box. **Size S.**

### 5.2 Gohan's house, 5.3 the time capsule, 5.4 the shed skin and Kami's vision (leave)

- **Where:** 5.2 the same sequence's entries 20-39 on map 2/20 (stock, with our party lines); 5.3 stock map 275
  trigger 0 (film `cap_TG`); 5.4 `ch5_fusion` 0-35, stock (film `fus_TG`).
- **Today:** these already move. Bulma walks in with the photo and out again, Yamcha and Gohan leave by the door,
  Trunks walks to the egg shells and to the machine and back, Bulma backs away from the skin, the news plays as a
  cutaway to Kame House with Krillin walking to the television, Kami's and Piccolo's turns are scripted.
- **Lineups:** Trunks alone, Gohan alone, or both; the stock stands "the other one" itself. 5.4's Lookout half makes
  Piccolo player 1.
- **Anime:** the same beats (the moss-covered machine, the melted dome, the egg, the skin, the Gingertown report).
- **Rating:** leave. One fact for the developer: `staging_ch5`'s fusion scenes run without flag 129, so Kami's
  record (flag 129 set, 132 clear) does not stand in them; film and test the Lookout half with `+129 +131`.

### 5.5 Kami draws out Hercule's potential (touch up, S; build with 5.6)

- **Built 2026-11-06 (`nb-cs2`, with 5.6).** Piccolo turns to watch Hercule come; a hum, a rising tremble, a white pulse and dust at
  his feet before the stock flash; a punch each way after the caption. No hop (the actor has no jump pose worth it).

- **Where:** `ch5_fusion` entries 36-46 (ours). **Today:** Hercule's actor (72) walks in from the right rim to
  Kami, four lines, `WhiteFlash 20` and the level sound, the caption "@^Hercule has achieved Level %d!", he throws a
  punch (animation 6), his line, Piccolo's "Hmph.  Don't let it go to your head.", he walks aside.
- **Lineups:** one. Hercule is kept for the plot here and is always the actor (player 2 has no entity after the
  warp to the Lookout and is bound to him only when the scene ends).
- **Anime:** none (the Grand Elder's hand on Krillin's and Gohan's heads on Namek is the model: a hand, a glow, a
  gasp).
- **Shot list:** 1. Kami turns to Hercule (stock does). 2. In place of the bare flash: CHARGE on the actor 72
   (short: the aura sound, `ScreenShake 0, 6, 30`, one `WhiteFlash 10`), Piccolo LOOK. 3. The caption and the
   punch as today; add a second punch the other way and a small hop (he tries it out). 4. Lines kept: "HERCULE:
   Whoa...  I feel like I could lift a truck!\x0bHa ha ha!  Not that I couldn't before, of course!" /
   "PICCOLO: Hmph.  Don't let it go to your head."
- **Risks:** none beyond 72's groups (full set). **Size S.**

### 5.6 The fusion (showcase, M)

- **Built 2026-11-06 (`nb-cs2`), script steps only (the cine helpers were not merged yet: CHARGE, FALL / RISE and DASH are
  hand-rolled here).** The whole scene stays on the Lookout: the blank map 1/6 is not used (the stock `DespawnEntity 0` /
  `UploadSpritePalette` there only hid the player on the white map; the level and the form are plain party writes). Mr. Popo steps
  toward Kami for "No, Kami!", goes to the stair head and turns back; Hercule gives them room; Piccolo steps up; a held look; the
  rumble, a rising shake, two white pulses, pale green light while Kami glides down into him (`GlideToPosition 73, 192, 118, 50`:
  the last argument is a speed in 1/100 px a frame, measured: 300 crossed 22 px in under 10 frames), the flash that takes Kami, a white;
  out of it Piccolo stands on Kami's spot in the start pose, a ring of dust, the Lookout shakes, Hercule is thrown on his back, Popo
  steps away; quiet; the stock caption under him; the three lines in the bottom box (they were `@`); he turns to the rim, walks, flies
  off; Hercule runs to the rim, looks after him and back at Popo; the stock white and the world map. The camera is held with
  `PanCameraToPosition 72, 46` (the view follows player 1 otherwise) and handed back with `SetActiveEntity 0` under the last white.
- **Facts found:** the Lookout top's floor is the blue disc only: about x 92-247, y 82-176 (centre (170, 128)); the stock mark
  (240, 152) and a walk to (268, 116) both stand on air (no wall stops an actor there). `ScreenShake`, `FlashScreenColor`,
  `SpawnDustCloud`, `EntityWaitFrames` and `GlideToPosition` are entity commands: in a batch each body's queue runs beside the
  others', so a shake on one queue, waits and flashes on a second and a glide on a third make one timed beat; `WhiteFlash` and the
  fades are modal and cannot be batched. `SetEntityAnimation 0, 4` (the start pose) takes about 64 frames.

- **Where:** `ch5_fusion` entries 47-59 (stock 36-48 with our ending), reached from map 283's trigger; it plays on
  the Lookout's top (zone 9 area 4: Kami's record at (192, 96), Mr. Popo's at (144, 96)) and then on the blank
  white map 1/6. Films `fus_TG`, `fus_T`.
- **Today:** Kami's promise, "MR. POPO: No, Kami!  No!", Kami's long answer, "Goodbye, Kami.  I'll miss you.
  Please be careful." Popo walks down the stairs and is gone. Sixty frames. Piccolo walks up onto Kami's spot, the
  screen fades to white over two seconds, and the rest happens on a blank white screen: the level caption, then
  three lines with waits between them, then the world map. Kami never visibly joins him; there is no flash, no
  shake, no sound but the level jingle. The most famous moment of the chapter is a walk and a fade.
- **Lineups:** one staging for every lineup (the script makes Piccolo player 1; Hercule is the actor; player 2 is
  bound to Hercule at the end).
- **Anime:** Kami: "I'm ready." He lays his hand on Piccolo's chest; a blinding light; Kami is gone and Piccolo
  stands alone in a column of power that shakes the Lookout; Popo weeps; "I am neither Kami nor Piccolo.  I am a
  Namekian who has long since forgotten his name."; he leaves at once, and fighters across the world feel it.
- **Shot list:**
  1. Lines kept as written: "Our union will make you more powerful, Piccolo... and you will obtain my vast
     knowledge...\x0b...I hope you will use these new powers wisely." / "MR. POPO: No, Kami!  No!" / "There is
     nothing to worry about, Popo.  Piccolo has changed. ..." / "Goodbye, Kami.  I'll miss you.  Please be
     careful." LOOK: Piccolo and Hercule to each speaker.
  2. Popo walks toward the stairs as today but **stops at the stair head and turns back** (no despawn): he watches.
  3. Hercule backs two steps toward the rim (`WalkToPosition 72, 264, 168`), facing them.
  4. Piccolo walks to (192, 120), one body below Kami, facing up; Kami faces down. **Hold 40 frames, no sound.**
  5. The light builds: the rumble (`PlayAudio 43`, the sound the absorption scenes use), `ScreenShake 0, 4, 60`
     then `0, 10, 60`, two `WhiteFlash 6` twenty frames apart, CHARGE on Piccolo (char 0).
  6. Kami goes into him: `GlideToPosition 73` down onto Piccolo's mark under three pale `FlashScreenColor` pulses,
     then `WhiteFlash 10` with `DespawnEntity 73` on the flash. (A glide needs no pose; Kami cannot walk.)
  7. The burst: `FadeToWhite 20`; under the white the stock bookkeeping (the level, the Super Namek form), Piccolo
     set on Kami's spot facing down; `WhiteFadeOut 40` **on the Lookout, not the blank map**; `ScreenShake 0, 20,
     120`, a ring of `SpawnDustCloud` at his feet, CHARGE held about 80 frames. Hercule: FALL (on his back from
     the shock wave), then RISE. Popo: one step back, turned half away.
  8. Quiet: `StopAudio 43`, 50 frames, then the music (`PlayMusic 34` as stock).
  9. The caption kept: "@^Piccolo has achieved Level %d!\x0bPiccolo now has Super Namek form!\x0bPiccolo can now use
     the Scatter Shot!" over the Lookout.
  10. The three lines kept, at the bottom so he is in view (they are `@` today): "No longer am I Kami or
      Piccolo... " facing the camera; "I am the Namek who has long since forgotten his name..." as he turns to the
      rim and walks two steps; "I must go."
  11. Exit: the flight sound, DASH to the rim, `FlyToPosition 0` off the top right. Hercule runs to the rim, looks
      down after him, looks back at Popo; the stock two-second white and the world map follow unchanged.
- **Risks:** the stock leaves for map 1/6 with `DespawnEntity 0` and `UploadSpritePalette` around the level-up:
  the Super Namek change may need them. If it cannot be done on 9/4 under the white, keep the blank map for the
  caption only and come back with `WarpToMapNoEntities 9, 4`: that reload drops the actors (spawn Hercule and Popo
  again; set flag 132 first so Kami's record stays away). `GlideToPosition` has only been used on sprite 76.
  Kami's record is an NPC record (talkable): despawning it mid-scene is what the stock ending relies on anyway.
  **Size M.**

### 5.7 Gingertown: the meeting and the town blown away (showcase with 5.8, M)

- **Where:** `ch5_gingertown`, map 284 (zone 10 area 17) trigger 0; the fight is on map 287 (10/20). Films
  `gin_PH`, `gin_P`.
- **Today:** Piccolo walks up the empty street, the camera pans to Cell standing still. "@???: Hisssss!!!" and
  ten more lines in the middle box with nobody moving. On "you're history!" one ki blast, a dust cloud and a
  half-second fade to white; the same spot is now a crater. Two lines; with two players the split's three lines
  over two Cells drawn on one spot; the fight. Hercule walks in behind Piccolo, is left below the view by the pan,
  and has no body at all in the crater for "HERCULE: T-two bugs?!  Nobody said anything about two bugs!".
- **Lineups:** Piccolo alone (no split); Piccolo + Hercule. Hercule alone arrives as Piccolo (the fade).
- **Anime:** an empty town, clothes in the street; Cell turns from his last victim; Piccolo's power-up alone blows
  the town flat ("a Super Namek!").
- **Shot list:**
  1. The walk-in kept. Player 2 (Hercule, his entity stands here): placed 32 px behind Piccolo before the pan, so
     the pan keeps him in the bottom of the view; a step back at the hiss.
  2. Cell begins with his back turned (facing 1) and turns slowly for "@???: Hisssss!!!" (two facings 15 frames
     apart), the tail up once (animation 27 for 20 frames, then idle).
  3. During the talk, one small move a beat: Cell walks a short arc (three short walks) through "@One might say
     that we're brothers!\x0bHahahahahaha!!!!", Piccolo turning with him; Piccolo drops into his ki pose before
     "@I'll still destroy you, whoever you are."
  4. "@You keep using that name.  Piccolo is gone; you can call me, "The Namek".\x0bAnd as for you, well, you're
     history!", then **the power before the blast**: CHARGE on Piccolo (`ScreenShake` rising 8 to 20 over 90
     frames, two flashes, a dust ring); Cell braces (hit-stun pose, a step back); Hercule FALL, away from it. Then
     the stock blast and the white.
  5. The crater: hold 40 frames on the flattened town before "I'm impressed, "Namek".  Your power is astounding.
     You will make a most satisfying meal!" / "Don't count on it."
  6. Two players, the split: on `WhiteFlash 10` a second body walks out of the first to the twin's spot (as zone
     12 does). Lines kept: "@???: Two of you?  Then two of me!" / "@PICCOLO: He split himself in two...  That's
     Tien's Multi-Form technique!" / "HERCULE: T-two bugs?!  Nobody said anything about two bugs!" with Hercule's
     **stand-in** spawned under the white 12 px left of Piccolo, a step back at his line, gone before control.
  7. The first clash (optional, S): Piccolo and Cell DASH, one FLURRY of a second, spring apart, control.
- **Risks:** `WarpToMapNoEntities 10, 20` in the middle (the actor record of Cell on 10/17 is replaced by the enemy
  record of 10/20; the split's walking body must go before `DrawBossHealthBarRange`: a step finds the first body
  of a sprite); the twin stands by `TWIN_OFFSET`, so the first Cell must end on his record's spot. **Size M.**

### 5.8 Cell's Kamehameha, the arm, Cell's story, the escape (showcase, L)

- **Where:** `ch5_gingertown_end`, map 287 record 0's defeat sequence (entries 3-61 are stock `083e24a0`). Film
  `gin_PH` (sheets 1-2).
- **Today:** (two players: a flash takes the second Cell back) a fade, Cell and Piccolo on marks facing each
  other. "Kaaaaaaaaa... Meeeeeeee..." is typed in a box while Cell stands idle; the beam (`NPCFireKamehameha 28,
  140`) crosses the screen and Piccolo flashes red twelve times where he stands; he ends in the lost-arm pose (22).
  Then Cell's whole story: **about a minute of text even when mashing A, with no move at all**. Piccolo's "Yes,
  I'm ready" has two waits and the regrowth pose (26). Krillin and Trunks fly in; Cell flies off **through them**
  to the bottom of the screen; Vegeta flies in, then Tien; fade. Hercule (player 2) stands in mid-screen with his
  back to the camera from the first frame to the last and reacts to nothing.
- **Lineups:** Piccolo alone; Piccolo + Hercule (no Hercule line; his body). The scene ends by making Trunks
  player 1 and binding player 2 to Krillin.
- **Anime:** Cell fires a Kamehameha and Piccolo knows whose it is; Cell seizes him and drains his arm to a husk
  with the tail; Piccolo plays beaten to make him talk, then tears the arm off and grows another ("you've been
  had"); Krillin and Trunks arrive; Cell blinds everyone with the Solar Flare and is gone. The game's version (the
  beam costs him the arm) stays.
- **Shot list:**
  1. The fade's marks also place player 2: 40 px behind Piccolo, facing Cell.
  2. "#Kaaaaaaaaa...\x0aMeeeeeeee...\x0aHaaaaaaaa...\x0aMeeeeeeee...": Cell is in the wind-up from the first syllable
     (CHARGE with a blue `FlashScreenColor`, a small rising shake). Piccolo's start (stock animation 4) and "#It
     can't be!\x0bThe "Kamehameha" wave is Goku's attack!" as today.
  3. "#HAAAAAAAAAAAAAAAAAAAA!!!!!!!!!!!!!!!!!!!!!!": the stock beam; Piccolo braces and is **driven back about 40
     px along the beam's line** while the hits land, a dust trail, `ScreenShake 0, 16, 60`. Hercule dives out of
     the line (a step, FALL).
  4. The arm: Piccolo KNEEL in pose 22, dust at his side. Cell **walks** over (two slow walks) and stands above
     him for "#???: I got you now.\x0bResistance is useless. ... Without your arm you are no match for me!"
  5. The story (entries 11-24, every word kept) staged as an interrogation: Cell paces, one short walk at each new
     entry (left, back, right, never more than 24 px), Piccolo's eyes on him (LOOK); on "#Very well, my name is
     Cell." he stops and turns to the camera; on the entry that begins "#I suppose I can answer this last one." he
     turns his back to Piccolo and looks at the sky, the tail swaying once; Hercule edges one step back at "#You
     will be absorbed into...\x0bPerfection!" and again at the story's end, no more.
  6. The trick: "#Yes, I'm ready.  You've said more than enough!\x0bI just needed a little time." Piccolo RISE. The
     regrowth (26) on a `WhiteFlash 6`, the ki sound, a short shake; Cell recoils a step before "#You tricked
     me!!!".
  7. "#That's right.  I'm afraid you haven't even come close to seeing the depths of my abilities.\x0bDon't think I
     won't pay you back for taking my arm.  ", a short CHARGE, then DASH to within 16 px of Cell and stop, face to
     face, for "#Maybe I'll take yours." Hold 30 frames.
  8. Krillin and Trunks fly in (stock); everybody LOOK at each arrival; Cell's turns as stock.
  9. The escape, after "@You all must feel honored. ... I'll absorb you later!": Cell's hands up (animation 27 is
     the nearest stock pose; check it reads), `FadeToWhite 8`, hold 30 with everyone in the start pose (4), Cell
     despawned under the white, `WhiteFadeOut 40`, the flight sound trailing off. No word added: the anime's Solar
     Flare shown as light. (Today he flies out between Trunks and Krillin.)
  10. Vegeta and Tien fly in (stock), LOOK turns, the stock ending.
- **Risks:** the beam's twelve hits are a command batch on the player (`EntityWaitTouchPlayer 0`): the knock-back
  must keep him in the beam's row or the batch waits for a touch that never comes; player 2 is a live entity here
  and can only be placed (`CoopCall 9`): his step and fall need the P2Actor hand-over, and the stand-in must be gone
  before `SetActiveCharacter 3` / `CoopCall 6, 6`; the regrowth pose exists on 103 (group 22) only: the player's
  body is 103 here by the fade; the text box sits at the bottom (`#`) under bodies at mid-screen: keep the marks
  above y 224. **Size L** (two commits with 5.7: this one first).

## 3. Chapter 6: the basement, the Time Chamber, the island

### 6.1 The lab basement (touch up, S)

- **Where:** `ch6_larva` (map 256, 8/62), `ch6_computer` (map 257, 8/63), `ch6_lab_exit` (map 250, 8/48 trigger
  1). Films `bas_TK`, `bas_T`.
- **Today:** player 1 walks to the tank, the lines, one ki blast and a quick white: the tank is broken. The same at
  the computer, then the caption "@^WARNING!  WARNING!  SECURITY BREACH!  DESTRUCT SEQUENCE ACTIVATED!" over a
  still room. Outside, a rumble sound with a still picture, the boulders appear in the cave mouth, the line, a pan
  to a sign.
- **Lineups:** Trunks alone, Krillin alone, Trunks + Krillin, Krillin + Trunks (Krillin's lines only when played;
  the lead's `%s` lines).
- **Anime:** Trunks and Krillin find the plans and the embryo and destroy everything; the lab goes up behind them.
- **Shot list:** 1. Each blast: a wind-up (the ki pose, 20 frames), the stock shot, then **impact before the
  white**: `WhiteFlash 6`, `ScreenShake 0, 16, 20`, dust at the tank / the console. 2. The warning: three red
  `FlashScreenColor` pulses and a low continuous shake while the caption is up; player 1 looks left and right. 3.
  The cave: `ScreenShake 0, 12, 120` under the stock rumble, dust at the cave mouth, a `WhiteFlash 6` as the
  boulders appear, the players a step back from it.
- **Risks:** both blasts end in `WarpToMapNoEntities` (player 2 loses his body there: nothing of his after the
  white). **Size S.**

### 6.2 Goku wakes, 6.3 "only two of us can train" (leave)

- **Where:** `ch6_goku_wakes`, map 295 (film `wake_TK`); `ch6_lookout` entries 0-17, map 263 trigger 1 (film
  `tc_T2`).
- **Today:** both have their own life: Goku sits up, gets out of bed and walks, the room starts (animation 4),
  Chi-Chi goes to him, he flies out of the window and the camera follows him over the sea; at the Lookout Vegeta
  turns away, Mr. Popo opens the door, Vegeta and Trunks walk into the light.
- **Lineups:** 6.2 the basement's four (alone as Krillin, Trunks is kept and Krillin hands over); 6.3 Trunks as
  player 1 (his body walks in), or his actor.
- **Anime:** Goku leaves by Instant Transmission; the game's flight is its own and reads well.
- **Rating:** leave.

### 6.4 The Time Chamber duel, Vegeta and Trunks (rework, M)

- **Where:** `ch6_lookout` entries 18-22 and `ch6_chamber_end` (`data/story/ch3_duels.lscript`), zone 15 area 2.
  Films `tc_T2` (two players, the duel ended by a real blow), `tc_T` (alone).
- **Today:** fade in: both already Super Saiyans, standing 64 px apart on the white floor. Three lines. Two
  players: the duel, then the winner's line with both standing, a fade. **Alone:** Trunks' actor is despawned, two
  white flashes and a shake play over Vegeta standing by himself, and he says "VEGETA: Hmph.  That is how a prince
  fights.\x0bNow get up.  We have a year of training ahead of us." **to an empty room**.
- **Lineups:** two players (player 1 Vegeta, player 2 Trunks: the duel; the pads swap if Trunks wins); one player
  (Vegeta, Trunks an actor, no duel).
- **Anime:** their year is never shown (the game's own exit scene makes a point of it: "Our training is not to be
  discussed...\x0b...EVER!"). So this is our scene: keep it short and proud. The standard is the Time Chamber mode's
  opening.
- **Shot list:**
  1. Fade in with both in **base form** (137, 128) by the living quarters at the top. They walk out onto the white
     side by side; Trunks looks left and right (LOOK); Vegeta keeps walking 32 px further and stops with his back
     to his son.
  2. "TRUNKS: Well, father, let's get settled in.  We have a whole year in here."
  3. Vegeta, not turning: CHARGE, the change on a `WhiteFlash` (stock animation 35 / the Super Saiyan sprite 140),
     `ScreenShake 0, 12, 40`. Then he turns: "VEGETA: Settled in?  Don't be ridiculous.\x0bWe spar.  Right now, boy!"
  4. "TRUNKS: Right now?  ...Fine.  Don't expect me to go easy on you.", then Trunks' own CHARGE and change (134).
  5. Hold 30 frames, 96 px apart. **Two players:** both DASH to the centre, one clash (`WhiteFlash 6`,
     `ScreenShake 0, 16, 12`), spring back 40 px, and the duel starts with control (`CoopCall 1` as today).
     **One player:** the spar replaces the two flashes: FLURRY (two seconds), apart, Trunks DASH behind him, Vegeta
     turns and catches it (STRIKE-L), Trunks' answer (STRIKE-L, Vegeta slides 16 px), Vegeta's STRIKE-H: Trunks
     thrown 64 px, dust, FALL. About eight seconds; Trunks' actor stays on the floor.
  6. The end: marks behind a 20-frame fade (the duel leaves them anywhere). Vegeta won: Trunks is down (KNEEL);
     "VEGETA: Hmph.  That is how a prince fights.\x0bNow get up.  We have a year of training ahead of us."; Vegeta
     turns and walks toward the quarters; Trunks RISE. Trunks won: Vegeta on one knee, turned away; "TRUNKS: I
     won, father...\x0bNow let's train.  Cell won't wait for us."; Vegeta RISE without a word, walks past him.
  7. The stock fade and the Lookout.
- **Risks:** the duel's rules want both as Super Saiyans when `CoopCall 1` runs: today the script sets both
  transformations before the warp; changing in the scene must leave the party's transformation bytes as they are
  now at that call. Player 2's entity is born by the duel: until then Trunks is an actor of two sprites (128, then
  134: swap on the flash). The loser's body at the end is player 2's entity or player 1's by the pad swap: the
  marks must go through `CoopCall 3`. `duel_end.txt` steps into the duel by timing. **Size M.** The dojo's
  sparring (W.5) can reuse beat 5's clash.

### 6.5 Kame House: the androids land, Piccolo's deal (touch up, S)

- **Built 2026-11-06 (`nb-cs2`), but for beat 3 (player 2's own move waits for the cine helpers' player-2 bodies).** The HUD is hidden
  from the first line. 16 walks up to (592, 448), the ground shakes and dust rises at his feet before "Not here...?"; 17 turns to him
  and steps away for "Whoa, take it easy, 16."; Piccolo steps toward 16 with the offer; after the island's pan 16 looks at the
  island, then at Piccolo, "Agreed."; **16 flies off first**, then 17's line, Piccolo's aside, and 17 and 18 leave together.
- **Fixed (the research's finding):** the flight pad's stock script (trigger 4) stood Krillin's actor beside Tien whoever played
  him; it is `a16_kame_flight` now and leaves the actor out when Krillin is played (`CoopCall 22, 4, 6`). Tested in `staging_ch6`
  ("kame": Tien alone with a played Krillin, both otherwise).

- **Where:** `a16_kame_house`, map 293 (zone 11 area 1) trigger 3. Films `kh_PH`, `kh_K`.
- **Today:** 17's "Hi!  Can Goku come out and play?" from off screen; the three fly in below the house; 16 scans
  (three turns, stock); the lines; Piccolo walks down; the camera pans to the far beach and back for "You and
  me."; they fly off one after another, 16 last. **16's fury is only told**: he stands still through "Where is
  he?  Tell me where Goku is!".
- **Lineups:** the fade makes Piccolo player 1. Piccolo alone; Piccolo + Hercule; Piccolo + Krillin (picked at the
  yard's pad). Player 2 has no line.
- **Anime:** 17 calls up at the house; Piccolo offers to settle it on an empty island and throws down his cape;
  16 says nothing and watches birds. Ours has 16 as the fighter, by Ryan's story.
- **Shot list:** 1. Before "ANDROID 16: Not here...?\x0bI was built for one purpose: to destroy Goku.\x0bWhere is he?
  Tell me where Goku is!": 16 walks two steps toward Piccolo and stops; `ScreenShake 0, 10, 20`, dust at his feet
  (the ground answers before he speaks). 2. 17 LOOK at him and a step back for "Whoa, take it easy, 16.  I've
  never seen you like this."; 18 does not move (bored). 3. Player 2: Hercule hops behind Piccolo at the stomp;
  Krillin steps up level with Piccolo, on guard. 4. Piccolo's step forward for "!You want Goku?  Then let's make
  a deal. ..." (he already walks down before it). 5. "ANDROID 16: Agreed.\x0bWhen you lose, you will tell me where
  to find Goku.": 16 turns to the island first. 6. The exits: **16 first** (he is the eager one; today he is
  last), then 17 and 18 together after "Ha!  This I've got to see.  Come on, 18."
- **Risks:** the scene ends in `ReloadMap`; the stock trigger 4 then spawns Krillin's actor (76) and Tien to watch
  the flight: check it against a played Krillin (two Krillins). **Size S.**

### 6.6 The island, before the fight (touch up, S)

- **Where:** `a16_island_intro`, map 296 (11/4) trigger 0. Film `isl_PH` (sheet 1).
- **Today:** 18 and 17 on the cliff at the top left, 16 in the middle, the players at the bottom right; four lines;
  16's actor becomes the boss.
- **Lineups:** Piccolo alone; Piccolo + Hercule or Krillin.
- **Anime:** Piccolo sheds his weighted cape and turban, and the first exchange is dead even.
- **Shot list:** after "!Then I will not hold back.": 16 walks two heavy steps, Piccolo sets his stance; 16's punch
  (animation 6) and Piccolo's block: STRIKE-L, he slides back 24 px, dust; one held look; control. Player 2 a step
  back before it.
- **Risks:** `DespawnCharacterAndRecordPosition 8, 1` puts the boss where the actor stands: finish 16 on the spot
  the fight should start from. **Size S.**

### 6.7 "This island is breaking apart!" (rework, S)

- **Built 2026-11-06 (`nb-cs2`).** Behind a 20-frame fade 16 (the boss, sprite 160) stands at (264, 164), player 1 at (304, 168), player 2
  at (328, 172) (`CoopCall 9`), the camera at (152, 100) with 17 and 18 on their cliff in the same view; dust bursts across the grass
  during the two rumbles, 18 looks down at them, 16 looks up at her and back, 17 steps to the cliff's edge, Piccolo steps up for his
  line. The HUD is hidden as before. Tested in `staging_ch6` ("island break", three lineups).
- **Research corrected:** "nothing breaking" was half wrong. `PlayAudioBlocking a, b` is not a sound of b frames: decompiled
  (`0x0800A5BE`) it starts sound 43 (the rumble) and runs a modal camera shake of **b frames at strength a**; the stock scene
  already shook for 180, 120 and 90 frames, over a tree. The scene keeps those lengths (as `PlayAudio 43` + a batched
  `ScreenShake` so the dust and turns run beside it): no new wait. A `SetEntityPositionInstant` on the live boss at 75 % holds.
  A bottom box of three lines covers the view's lowest 72 px: marks' feet stay at camera y + 84 or higher.

- **Where:** `a16_island_break`, map 296 record 1's defeat (16 at 75 %). Film `isl_PH` (sheet 1).
- **Today:** the camera stays on player 1 wherever the fight left him (in the film: half behind a tree at the
  shore); 18's, 17's and Piccolo's lines play **with no speaker in view and nothing breaking**; two seconds of
  white; area 5 is the broken island (rock stacks in the sea), and the three fly on.
- **Lineups:** as 6.6.
- **Shot list:** 1. A 20-frame fade: Piccolo, player 2 and 16 on marks in the clearing, 17 and 18 on their cliff,
  the camera on all of them. 2. `ScreenShake 0, 8, 60` under the stock rumble (`PlayAudioBlocking 30, 180`), dust
  at three or four spots; "ANDROID 18: This island is breaking apart!\x0b16, is he really pushing you around?"
  with 18 on her feet at last, 16 LOOK at her and back. 3. A harder shake; 17's and Piccolo's stock lines with a
  step each. 4. The stock white and area 5.
- **Risks:** 16 is the live boss here (sprite 160): the stock script already turns and flies him by sprite; a
  position set on a boss mid-fight state wants a look at `android16.c` (he is stopped at 75 % by the bar range).
  **Size S.**

### 6.8 Cell arrives, 17 is absorbed, Tien's stand, Goku's rescue (showcase, L)

- **Where:** `a16_island_half` (map 298, 11/6, record 0's defeat): entries 0-7 ours, then the stock sequence
  `083e6078` from its entry 1 (90 stock entries; 97 in the merged sequence), with a cutaway to the Lookout (9/4) and the end on 9/1. Film
  `isl_PH` (sheets 2-4).
- **Today:** this is the stock game's most choreographed scene and most of it holds. Cell walks in from the bottom
  and circles the androids one by one ("Android 17!  Amazingly efficient! ", "And 18!  Poetry in motion!").
  Piccolo fires three small ki blasts, Cell walks over and punches him: Piccolo lies there for the rest of the
  scene. 16 walks up, three punches, a red flash, Cell lies. Tien flies in at the top right; Cell **walks behind
  17 in plain view** before "He's behind you!"; the tail, a white fade, purple flashes, a long shake:
  Semi-Perfect Cell. 18 and 16 run, then fly. Tien's stand is **four small ki blasts at a time, level across the
  grass**, Cell answering with the same; Tien flashes red and lies down. Goku appears in a white blink, walks to
  Tien (who vanishes), walks to Piccolo (the player vanishes), "!Be seeing you!", a blink. Player 2 is off screen
  from the fade to the end.
- **Lineups:** Piccolo alone; Piccolo + Hercule or Krillin (no player-2 line in the scene).
- **Anime:** Piccolo's all-out blast does nothing and Cell breaks him; 16 tears into Cell; Cell comes up behind 17
  and the tail swallows him; **Tien's Neo Tri-Beam from the sky, blast after blast, each one hammering Cell back
  into a square pit, until Tien has nothing left**; Goku appears, takes Tien and Piccolo, looks at Cell ("not
  today") and is gone.
- **Shot list** (three commits; (c) is the Short):
  - **(a) the arrival to Piccolo's fall (entries 7-39), touch up:**
    1. The 180-frame fade also marks player 2 beside Piccolo (**stand-in** or `CoopCall 9`), facing the androids.
    2. Before Cell is seen: `ScreenShake 0, 6, 40`; 16 turns south first (he detected it), then 17, 18, Piccolo.
    3. Cell's slow walk-in and the inspection kept as they are; 17 and 18 LOOK as he passes, 18 one step back at
       "And 18!  Poetry in motion!".
    4. Piccolo's attack (stock entry 38): a short CHARGE, VOLLEY of three (the stock hits on Cell), then Cell DASH
       and STRIKE-H: Piccolo thrown 48 px, dust, the stock red flash, FALL into the stock lying pose (33). Player
       2: Hercule scrambles back and cowers; Krillin starts toward Piccolo and stops dead. They stay where they
       are until Goku comes.
  - **(b) 16 and the absorption (entries 40-58), rework:**
    5. "I will kill Cell.\x0bThis world is a good place.  Cell wishes to destroy it.\x0bI cannot let that happen." 16
       walks, as today: he does not hurry. STRIKE-L, STRIKE-L, STRIKE-H: Cell thrown 64 px, a dust burst, a shake,
       the stock red flash, Cell lies.
    6. "Incredible!  16 did it!" / "16 squashed the bug!\x0bAmazing!"; 17 strolls over to look at the body.
    7. Tien's fly-in (stock pan to the top right). **While the camera is on Tien, Cell is moved behind 17**, so
       when the stock pan comes back for "!You should have listened to your little friend." he is simply there.
    8. The tail (stock 27, 25), a rising shake, the stock recoils of 18, 16 and Tien; the white and the purple as
       stock; then the reveal: a full white for the swap (28 to 29), **60 silent frames**, the new body turns to
       the camera, and "I love my new look... much better than the last.\x0bAnd merging with 18 should have a
       similar effect!"
  - **(c) Tien and Goku (entries 59-91), showcase:**
    9. "NO!!!!!!!!" Tien flies **up** to a mark above and right of Cell (flight group 12) and sets his hands (pose
       4 or 5: check which reads as the triangle).
    10. The Tri-Beam is light from above, not a projectile: `WhiteFlash 6`, the blast sounds (20, 21),
        `ScreenShake 0, 20, 16`, the hit effect on Cell, four dust clouds in a **square** round him, Cell FALL.
        Three times, 24, 18 and 12 frames apart.
    11. "!What do you think you're doing?" (Cell RISE) / "!I'm doing the only right thing!  I'm stopping you!"
    12. The stock cutaway to Goku at the Lookout, kept.
    13. "!You fool.\x0bYou've seen my power.  You must know that you're no match for me.": Cell takes one step
        toward Tien and two more blasts flatten him in mid-step.
    14. "#18!!!!  Take 16 and run!!!!\x0bI don't know how long I can hold Cell back!\x0bWhat are you waiting for?
        Go!!!\x0bNow!!!!"; 18 and 16 fly off (stock) while the blasts keep coming every 14 frames.
    15. He weakens: the gaps grow (30, then 45 frames), Tien sinks 8 px lower with each; "Can't... do... this...
        much... longer..."; one last flash; Tien FALL from the air, dust where he lands.
    16. Cell RISE, walks two steps toward the body, a hand raised: "You puny little man... I can't believe such a
        weakling held me back!\x0bIt's almost too embarrassing to take!"
    17. Goku: DASH's arrival flash between Cell and Tien (the stock blink at 10 frames is nearly this). His five
        lines kept. The pick-ups as Instant Transmission: a blink to Tien (both gone), a blink beside Piccolo, a
        third to player 2's body when there are two players; then "!I don't think so.  We'll fight right here.
        \x0bNow!", Cell BLAST-1 at him, "!Be seeing you!", and the blast passes through the place he stood.
    18. "#Instant transmission...\x0bThat's quite a move...\x0b...I must figure out how he does that!" alone on the
        island; the stock fade.
- **Risks:** the Lookout cutaway is `SetMapRestoreFlag` / `LoadMapWithEntities 9, 4` / `RestoreMap` in mid-scene:
  check that a body left in the air or lying comes back as it was. Tien has **no beam group** and his poses 4 / 5
  are unread: light and dust only. A text box on this full map can take bodies off the list (seven actors and
  effects): watch `ents`. The player leaves by `DespawnEntity 0` (stock): keep that step last; player 2's
  stand-in before it. The stock entry indexes are absolute (`stockseq @083e6078 1`): restated entries go after
  the end. **Size L** ((a) S, (b) M, (c) M).

### 6.9 The Lookout after (leave)

- **Where:** the stock tail (entries 92-96) on 9/1. Piccolo's "That was an honorable move, Tien... but it wasn't
  enough.", Mr. Popo's news, they walk off. Short, with bodies. Leave.

## 4. Chapter 7: zone 12

### 7.1 Out of the Time Chamber (touch up, S)

- **Where:** stock, map 263 trigger 2 (our edit is the party bits at its end). Film `ex_PH`.
- **Today:** the door opens, Vegeta and long-haired Trunks walk out into a crowd; the level captions; twelve lines
  in place (Piccolo, the player, takes one step for his); Bulma walks in with the armour and everyone turns to her
  (the stock uses `SetEntityFollow` for that); Goku and Gohan go in; fade.
- **Lineups:** written for Piccolo as player 1 (+ Hercule or Krillin). **From the script, not filmed:** the scene
  before it ends on the Lookout's main level, which has a save pad, so player 1 can be Hercule or Krillin here;
  entry 18 then walks *him* up for "PICCOLO: Listen, I don't know how powerful you think you are now...". By the
  presence rule that line belongs only to a played Piccolo (Tien's next line carries the warning).
- **Anime:** Vegeta steps out in torn armour, sure of himself; he cuts Trunks off; Bulma arrives with the new
  suits; he leaves first.
- **Shot list:** 1. Vegeta walks **through** the group to the open floor before anyone speaks; the others LOOK
  after him. 2. "VEGETA: QUIET!\x0bYou've said quite enough.  Our training is not to be discussed...\x0b...EVER!":
  before it a `WhiteFlash 4` and `ScreenShake 0, 6, 10`; after it Trunks' start (pose 4 on his sprite
  131, beside the stock's 4 on Vegeta). 3. Before "I will take care of everything now." Vegeta starts for the exit and the
  played Piccolo steps into his way (the stock step, a beat earlier). 4. Bulma's entrance as stock.
- **Risks:** the absolute indexes of a stock sequence; the `SetEntityFollow` turns are stock and hold player 1 too.
  **Size S.**

### 7.2 Vegeta and Semi-Perfect Cell (rework, M)

- **Where:** `ch7_semi_perfect`, map 302 (zone 12 area 4) trigger 0. Films `z12_VT`, `z12_V`.
- **Today:** Vegeta (and Trunks beside him) below Cell; nine lines; "Tell me...  Are you ready now to witness a
  power that has not been seen for thousands of years?" is answered by the stock change (animation 35) and Cell
  walking back two steps; "It's your end!"; with two players a second Cell walks out of the first; the fight.
- **Lineups:** the fade makes Vegeta player 1. Vegeta alone; Vegeta + Trunks (the split and its three lines).
- **Anime:** the power-up is the scene: the island groans, the sea stands up, Cell shields his face; then "Super
  Vegeta" walks up and buries a fist in him.
- **Shot list:** 1. The stock approach and lines 1-9 kept; one move a beat: Vegeta folds his arms and half turns
  away on "I find it hard to believe that my cells are in your body. ...". 2. After "What is this?": CHARGE on
  Vegeta, long (100 frames, shake rising 8 to 20, three flashes), dust and stones round him, the change (35) at
  its peak; Cell is **pushed** back to the stock mark (216, 272) in the hit-stun pose, not walking; Trunks (his
  entity stands in area 4) braces a step back. 3. "It's your end!" 4. The first blow: Vegeta DASH, STRIKE-H in the
  body: Cell slides 32 px and comes up (RISE). Alone: control. Two players: "CELL: Two Saiyans against me?  Then
  I'll even the odds." reads as his answer to the blow; the split as built; "TRUNKS: Father, he's split in two!
  ..." / "VEGETA: Hmph.  Then I'll destroy both halves."; control.
- **Risks:** `DespawnCharacterAndRecordPosition 29, 1` takes the actor's place for the boss and the twin stands 24
  px right of it: bring Cell back to the mark after the slide; entry 12's "already transformed" branch (a player
  who arrives as a Super Saiyan skips the change): the CHARGE must sit before that test or have its own branch;
  `cell_staging`'s press windows `a` / `b`. **Size M.**

### 7.3 "Push-ups and sit-ups", "Here's my son!", the deal (touch up, S)

- **Where:** `ch7_semi_perfect_end` entries 0-20, map 302 record 1's defeat. Film `z12_VT` (sheet 2).
- **Today:** a two-second fade; Cell stands at (176, 294) with Vegeta at (168, 312), **18 px apart, one drawn on
  the other**, for fifteen lines (about 35 seconds when mashing); Trunks flies in at "Here's my son!" and steps
  up once.
- **Lineups:** one staging: player 2's Trunks makes way for the actor (132) in the fade, so "Here's my son!" flies
  him in for everyone.
- **Anime:** Cell on his knees, beaten and raging; Vegeta strolling round him; Trunks putting himself between them.
- **Shot list:** 1. New marks: Cell down (the stock lying pose 3 it already starts in), Vegeta 56 px off with his
  arms folded; Cell RISE to one knee for "CELL: What are you?  You're not Vegeta.  You can't be Vegeta!" and to
  his feet for "You will pay for your insolence Vegeta!". 2. Vegeta walks a slow quarter circle round him through
  "I do a lot of push-ups and sit-ups.\x0bAnd I drink plenty of juice." 3. Trunks' fly-in (stock). 4. Before
  "Vegeta...  Allow me to achieve my final perfect form." (the last page of Cell's long entry cannot be split:
  the step goes before the entry) Cell takes two steps toward Vegeta, low. 5. Trunks steps **between them**, his
  back to Cell, for "TRUNKS: No father!  He's trying to deceive you!"; Vegeta turns his back on him for "So be it
  Cell...".
- **Risks:** entry 3's marks feed the stock entry 21's flights (Cell and Vegeta leave from where they stand).
  **Size S.**

### 7.4 Cell absorbs 18; Perfect Cell; the kick; Vegeta falls (showcase, L)

- **Built 2026-11-06 (`nb-cs2`, with the cine helpers, `CoopCall 37`).** *Area 5:* Cell waits below as Vegeta lands at (204, 272) and
  walks up into the clearing; 16 steps in front of 18, she steps back to the cliff; Cell is at 16's side in a blink (dash) and one
  heavy strike throws him against the right-hand tree (the stock red flash on the blow), where he stays down; 18 turns to run, two
  steps, and Cell is in front of her; she backs onto the stock mark and the tail takes her as before (the stock poses 27 / 29 / 25),
  under a slow rising shake, Vegeta one step forward. Then the change: the stock morph (animation 9, started with
  `SetEntityAnimationNoWait` so the timeline is ours) under purple and white pulses and three rings of dust, Vegeta pushed back
  16 px (`GlideToPosition 0`: a glide moves a player without turning him); **a full white, the music and the rumble cut**; out of it
  Perfect Cell with his back to us for a second and a half of silence, a look over the shoulder, he turns, a held look; the music
  again and the stock flights. *Area 6:* Cell stands with his back to Vegeta, who lands lower right (300, 284); the first line in
  the bottom box (it was `@`, on both bodies); Vegeta is beside him in a blink, the kick (the stock animation 8, the hit blink on
  Cell, a jolt) and **Cell does not move**; a hold; Cell turns his head; Vegeta slides back into the start pose for "That kick!";
  Cell walks two steps for "Now, do you understand..."; then the blow: a dash and a heavy strike that throws Vegeta to (296, 262),
  where the stock lays his body after the fade.
- **The Final Flash (its own commit, for Ryan's review; decided for him while away):** after "That kick!" Vegeta springs back
  to (330, 266), a 70-frame charge (rings, flickers), a yellow flash, the beam held 60 frames on Cell, who blocks with an arm
  out (`CoopCall 37, 8, 18, 60`; the beam is the game's blue Kamehameha: cine has no beam colour); a white; when it clears Cell
  stands where he stood among dust, and Vegeta drops to a knee, spent (ours, with the beat: say if he should stay on his feet).
  Then Cell's two steps, his line, the blow. No line added; the outcome is unchanged. About 5 s.
- **One lineup** (filmed as Vegeta alone, Vegeta + Trunks, Trunks + Vegeta: the same scene). The sequence grew by one entry
  (indexes 26-37 are 27-38). About 1.5 s longer than the stock in area 5 (the silence), none before control.
- **Facts found:** `SetEntityAnimation 29, 9` is the whole morph into Perfect Cell and holds his new body for about a second after
  it; `StopMusic` takes no argument and `PlayMusic n` starts the tune again; the clearing of area 5 is about x 130-210, y 170-250
  between the trees; with `EntityWaitTouchPlayer` gone nothing in the scene waits on a contact.

- **Where:** `ch7_semi_perfect_end` entries 21-26 (stock 20-22): entry 21 is one script that flies Cell and Vegeta
  out of area 4, warps with entities to area 5 (map 303: 16's actor at (176, 184), 18's at (152, 200)), plays
  the absorption, flies on and warps to area 6 (map 304). Films `z12_VT` (sheet 3), `z12_V`.
- **Today:** in a clearing Cell walks up and punches 16 (a red screen), walks to 18, the tail comes over her head
  (the stock poses are there and read), a short white, three purple flashes and a long shake while he stands in
  place, and then **Perfect Cell is simply standing there**; 160 frames; he flies off and Vegeta after him. On the
  beach: "@So this is your new body?  Pathetic, Cell.  You've wasted my time. "; Vegeta **walks into Cell's
  sprite** and kicks from inside it; "#That kick!  I put all my energy into it! "; "#Now, do you understand what
  I mean by "perfection"?\x0bOr shall I educate you further? "; Cell's arm moves and Vegeta is lying at his feet in
  his base form. No knock-back, no shake, no flash anywhere in the kick.
- **Lineups:** **one, for everybody**: Vegeta is player 1 by the fade and player 2 has no body in areas 5 and 6
  (his Trunks comes back as the stock fly-in afterwards).
- **Anime:** Krillin cannot press the remote and crushes it; 16 and Trunks try to stop Cell; the Solar Flare; the
  tail takes 18; a dome of light and lightning; Perfect Cell rises without a word and looks at his own hands.
  Vegeta's kick lands on his neck and he does not even turn: "I put everything into that kick." Then the Final
  Flash, and one blow that ends it. The game has no Krillin, no remote and no Final Flash here; its story (Vegeta
  lets it happen and watches) stays.
- **Shot list:**
  - **Area 5, the absorption:**
    1. The stock flight in; Vegeta lands lower (about (184, 264)) and folds his arms: he allowed this, he watches.
    2. 16 steps between Cell and 18 (he says nothing; the stock has no line); 18 one step back.
    3. Cell DASH to 16, STRIKE-H: 16 thrown against the cliff foot, dust, `ScreenShake 0, 16, 20`, the stock red
       flash; he stays down (the stock pose 3).
    4. 18 turns to run, two steps; Cell DASH **in front of her**; she stops and backs away.
    5. The tail as stock (27 on Cell, 29 and the locked pose on 18, the sound, 25), with the camera closing on
       them and a slow rising shake; Vegeta takes one step forward and stops himself.
    6. The stock short white takes 18. Then CHARGE on Cell in place of the bare wait: the stock 160-frame shake
       and purple flashes, white flashes between them, three widening rings of dust; Vegeta is pushed back 16 px
       and braces; 16's body slides.
    7. **A full white** for the swap (29 out, 30 in), held 20 frames, a slow `WhiteFadeOut 60`. Then **silence**:
       the rumble stopped, no music, 90 frames. Perfect Cell stands with his back to the camera, turns his head
       (two facings), turns round. A held look at Vegeta, 40 frames. Not a word (there is none in the script).
    8. The stock flights out.
  - **Area 6, the kick:**
    9. The stock fly-in and pan; Cell with his back to Vegeta. "@So this is your new body?  Pathetic, Cell.
       You've wasted my time. "
    10. Vegeta DASH to a spot **in front of** Cell (about (248, 284)), the stock kick (animation 8), STRIKE-L on
        Cell **with no knock-back**: the hit effect and the stock sound, `ScreenShake 0, 8, 10`, and Cell does
        not move. Vegeta held in the kick 40 frames. Cell turns his head.
    11. "#That kick!  I put all my energy into it! " with Vegeta dropping back two steps (the stock start pose 4).
    12. "#Now, do you understand what I mean by "perfection"?\x0bOr shall I educate you further? " after Cell has
        walked two slow steps toward him.
    13. The blow: Cell DASH, STRIKE-H: Vegeta thrown toward (296, 262) (where the stock lays his body after the
        fade), dust, `WhiteFlash 8`, `ScreenShake 0, 20, 30`, FALL. The stock fade, Trunks' fly-in and his vow
        follow unchanged.
  - **Variant with the Final Flash (only on Ryan's yes: it adds a beat the game does not have):** between 11 and
    12 Vegeta leaps back to the beach, CHARGE (60 frames, a yellow `FlashScreenColor`), BEAM up the screen at
    Cell, `WhiteFlash 20`, `ScreenShake 0, 24, 60`, dust; the smoke clears and Cell stands where he stood (the
    anime's lost arm has no sprite). No line. About six seconds; the outcome is the same.
- **Risks:** entry 21 carries bodies through two `WarpToMapWithEntities`: 16 and 18 are area 5's own records,
  Perfect Cell is spawned by the script and rides the second warp; anything else spawned drops. The stock batches
  wait on contact (`EntityWaitTouchPlayer`): replacing them with helpers moves the timing under
  `cell_staging`'s `c` and `d` windows. The player's body is thrown: he must come to rest on walkable ground and
  the stock `SetEntityPositionInstant 0, 128, 540` after the fade still resets him (he becomes Trunks). The `#`
  boxes are at the bottom where Vegeta stands: pan a little lower than the stock
  `PanCameraToPosition 136, 192` so both sit above the box, or use `!`. The beam variant needs group 18 on 140 (present) and a C-driven beam on the player
  (`CoopCall 16` is the model). **Size L** (area 5 M, area 6 M; the variant S more).

### 7.5 Trunks' vow, Krillin flies in, Trunks powers up (touch up, S)

- **Where:** `ch7_semi_perfect_end` entries 27-37. Films `z12_VT` (sheet 4), `z12b_V`.
- **Today:** Trunks flies in under Vegeta's body, his vow, Krillin flies in to his side, three lines (alone:
  Krillin's draft line and he goes to Vegeta), Trunks' change is the bare stock animation, the fight.
- **Lineups:** Trunks alone (Krillin's actor goes to Vegeta's side); Trunks + Krillin (the actor hands over to
  player 2 on the spot).
- **Anime:** Trunks swells into the bulked form, the ground breaking under him; Cell watches, amused.
- **Shot list:** 1. Before "@I've already seen my own world laid to waste by Dr. Gero's creations. ... I WILL
  FIGHT!!!!!" Trunks looks at his father's body (LOOK), then at Cell. 2. In place of the bare change (entry 36):
  CHARGE on Trunks, long, the ground dust in rings, `ScreenShake 0, 16, 80`; Krillin shields his face and slides
  a step; Cell does not move at all, facing him. 3. Control.
- **Risks:** entry 36's "already transformed" branch; Krillin's actor is despawned for player 2's respawn before
  it in co-op (his reaction is then player 2's entity or nothing). **Size S.**

### 7.6 Krillin's one hit, "pump up your muscles", the tournament (rework, M)

- **Where:** `ch7_perfect_end`, map 304 record 1's defeat. Films `z12b_VT`, `z12b_V`.
- **Today:** marks behind a fade; "KRILLIN: He's wide open!  Now's my chance!  Hyaaaah!", his rush, Cell's arm, a
  flash and a shake, Krillin thrown past Trunks and down (staged 2026-11-05: it reads). Then Cell's lesson on speed
  and the whole tournament speech: eleven more lines with Trunks standing 40 px under Cell. Cell flies off; Krillin
  gets up; Vegeta gets up, looks about, walks; 16 flies in; the journal.
- **Lineups:** Trunks alone or Trunks + Krillin: one staging (player 2's Krillin is handed to the actor in the
  fade).
- **Anime:** the bulked Trunks swings and swings and never touches him; Cell is behind him every time; then the
  lesson, and Trunks lets the power go.
- **Shot list:** 1. Krillin's rush as built (STRIKE-H in place of the hand-rolled throw, for the common look). 2.
  After "KRILLIN: Ugh...  One hit...  He took me out with one hit...": Trunks lunges (a plain rush and the punch,
  6: he is the slow one) and Cell DASH **behind him**; Trunks turns and swings; Cell is back on his first spot,
  arms folded, looking away. Two misses, three seconds, **then** "CELL: All you've done is pump up your muscles.
  \x0bBut that won't get you anywhere if your opponent has any sort of speed.\x0bYou fool!  Honestly it's a very
  simple concept to understand." The lesson explains what was just seen. 3. After Trunks' "...You can finish me if
  you want.\x0bI've failed.": he powers down and KNEEL; Cell walks two steps to stand over him for "How gracious
  of you!   *sigh*  What shall I do with you? ...". 4. Before "No contest?\x0bNo contest!\x0bAhhh!!!  A
  tournament!" Cell turns away from him to the sea. 5. After "Bring me Goku!": DASH upward, then the stock flight
  out. Trunks RISE. 6. Vegeta's stock rise and 16's fly-in kept.
- **Risks:** Trunks is char 0: the lunges must end on the stock mark (248, 320); Perfect Cell has the punch (6)
  and walks and flies; a stock sequence's indexes (`stockseq @083e77b0 1 13`); `cell_staging`'s `e` / `f`
  windows. **Size M.**

### 7.7 16 asks for repairs; "Foolish boy" (leave)

- **Where:** map 304 records 2 and 3, talks after the reload. Not filmed. Two short exchanges with a standing
  body each. Leave.

## 5. The ten-day wait

### W.1 Capsule Corp: Roshi, 16's memory, Cell's broadcast, "Meanwhile..." (leave)

- **Where:** stock: Roshi in the atrium (map 147 record 2, restated by `lk_roshi`; not filmed); Bulma and Dr.
  Briefs over 16's memory (map 149 trigger 1, film `mem_TK`); the broadcast (map 160 trigger 1, `broly_tv`, film
  `tv_P`) and its jump to "Meanwhile..." (`083dbdd4`: Goku, Dende and the others on the Lookout).
- **Today:** talk scenes with bodies in view: the television shows Cell's face and the room starts when it comes
  on; Bulma runs in for the ship; Goku stands with Dende while the others wait below.
- **Lineups:** anyone; the stock "Meanwhile" stands Piccolo, Trunks and Vegeta as actors by player 1's slot only:
  check it against a player 2 on one of them (two bodies).
- **Rating:** leave.

### W.2 Cooler, the first meeting (touch up, S)

- **Where:** stock, map 97 (zone 4 area 50) trigger 2 (flag 60 set, 61 clear). Film `co1_P`.
- **Today:** Cooler flies down beside the player, "%s: Frieza???  But how?", his four lines, he flies away.
- **Lineups:** behind an "Only Piccolo" barrier: Piccolo as player 1, anyone as player 2.
- **Anime:** the film's Cooler arrives without warning and is all contempt.
- **Shot list:** 1. He lands hard: `ScreenShake 0, 12, 16`, dust; the players a step back into a guard. 2. He turns
  his back on them for "I always thought they were short-sighted fools.  However, they were still my family." 3.
  He leaves by DASH (gone in a blink), not the slow flight.
- **Risks:** Cooler's sprite 34: 0 1 2 5-8 12. **Size S.**

### W.3 Cooler's fight: the opening and his end (rework, S)

- **Built 2026-11-06 (`nb-cs2`), but for beat 3 (Cooler's CHARGE waits for the cine helpers).** The HUD is hidden for the opening
  (`SetPlayerVisible 0`, back at the fight's entry). Player 1 walks up to (200, 298), 34 px under Cooler, player 2 is placed beside him
  (`CoopCall 9`), the camera at (100, 214): Cooler above a bottom box and the players just clear of it; a top box (the player's
  lines) covers Cooler and leaves the players. Cooler starts with his back turned and turns round as they arrive. The three
  openings and the servant's walk are unchanged. **His end:** record 1's defeat is now `ch8_cooler_death`, the stock script
  with a pan to wherever he fell first (`PushEntityPosition 34`), so the jolts and explosions are seen; the stock pan to the door and
  `SetActiveEntity 0` hand the camera back.
- **Not done (found, left):** with two players the body of whichever of the two fell first stays lying on the map after the
  explosion, as before this change (the both-down rule's downed body is never removed here; the same in the build before this
  branch). Beat 5's "the servant's body goes in the last flash" needs `enemies.c`, not the script: the fallen one may be Cooler
  himself with the servant dying in his place.

- **Where:** `ch8_cooler`, map 172 (zone 6 area 61) trigger 0; the defeat is the stock script of record 1. Films
  `co2_PH`, `co2_G`.
- **Today:** the camera pans up to Cooler on his stone circle and **the players are left below the view** for the
  whole opening; with two players the Namekian walks in from the right. After the fight the stock death (hit
  effects, jolts, five shakes) plays **off screen** when the players are not near him.
- **Lineups:** anyone (three stock openings: Goku, Trunks, anyone else; "%s" lines), with or without player 2 (the
  servant only with two).
- **Shot list:** 1. The players walk up into the view (player 1 to about (200, 336), player 2 beside him) before
  the first line. 2. Cooler stands with his back to them and turns for "COOLER: You're not Goku...  I thought I
  specifically asked for Goku!" (or the Goku / Trunks opening). 3. Before the fight: CHARGE on Cooler (he says he
  has "transcended far past" his brother: show it), the players braced. 4. The servant's walk as built. 5. The
  end: `CenterCameraOnChar 34` (or a pan to him) before the stock death script, so it is seen; the servant's
  body goes in the last flash.
- **Risks:** the three openings return absolute indexes (restated already); the death is a plain script on the
  enemy record, not a sequence. **Size S.**

### W.4 Roshi's gym (touch up, S)

- **Where:** `ch8_gym`, map 175 (zone 6 area 70) triggers 1-3 and the records' defeats. Film `gym_GP`.
- **Today:** the players on the mat at the bottom, the opponents at the top edge of the view; a line each, the
  fight, a line each, the capsule, a fade to the atrium. Master Roshi's "Ho ho!  Not bad at all! ..." opens the
  animal rounds with his portrait and no Roshi in the room.
- **Lineups:** any two; the second duo refuses when a player is Hercule or Krillin (built).
- **Shot list:** 1. A pan up to the opponents for their lines and back to the mat. 2. After each match the beaten
  duo is on the floor (pose 3) for "YAMCHA: Whoa... You sure showed me!" / "KRILLIN: Owwww... That hurt." and
  Hercule's "Ow ow ow...  I let you win, of course!  It's called good sportsmanship!", then RISE. 3. Roshi (88:
  idle and walk) walks in at the door to say his line and steps aside before the animals come.
- **Risks:** the room ran out of OBJ tiles once (three rounds because of it): an extra actor in the animal round
  must be gone before `TriggerMapEvent`. **Size S**, low priority.

### W.5 The dojo's sparring match (touch up, S)

- **Where:** `wait_spar`: `lk_spar` / `lk_spar_end` on map 175's mat. Not filmed (from the script): a caption, the
  duel, "@^%s wins the sparring match!", a fade.
- **Lineups:** two players only, as any two characters.
- **Shot list:** the first clash of 6.4 beat 5 before control (both DASH, one flash, apart), and the loser KNEEL
  under the winner's caption. Both bodies are players: player 2's moves go through his entity.
- **Size S**; build after 6.4.

## 6. "Gero's computer" (chapter 9)

### 9.1 Briefs' workshop: 16 wakes (touch up, S)

- **Where:** `ch9_workshop`, map 149 (zone 6 area 5) trigger 2. Films `ws_GH`, `ws_K`.
- **Today:** Dr. Briefs and Bulma by the table; with two players Krillin walks in round it; 16 lies on the table
  as map decorations (not a body) and speaks lying there; captions; a fade; the table is empty.
- **Lineups:** anyone walks in; after the scene Krillin + 16 (two players) or 16 alone.
- **Shot list:** 1. "ANDROID 16: ...Systems online.": a `WhiteFlash 4` and the stock sound, Bulma a step back. 2.
  A 20-frame fade swaps the decorations for his actor (8) standing by the table; he turns his head north (toward
  the mountains) for "ANDROID 16: I am receiving a signal. ..." 3. LOOK turns for Bulma's and Krillin's lines; 16
  walks two steps toward the door before the caption.
- **Risks:** the body on the table is drawn by the map's on-load (flag 420): hiding it in mid-scene needs the
  decoration steps the on-load uses; the scene ends in a warp to the same map. **Size S.**

### 9.2 The two generators (rework, S)

- **Where:** `ch9_gen_west` (map 229, 8/27), `ch9_gen_east` (map 242, 8/40). Film `gero_KA` (sheet 1).
- **Today:** the camera pans up a waterfall away from the players; a caption; 16's "This generator feeds the
  bunker's defenses.  Stand back."; a flight sound, a flash, a shake; his next line. **Nobody is in view and
  nothing is seen to be destroyed.**
- **Lineups:** Krillin + 16, 16 + Krillin, Krillin alone, 16 alone (16's lines are player 1's by name when nobody
  plays him).
- **Shot list:** 1. Whoever plays 16 flies up into the view to the foot of the fall (player 1: `FlyToPosition 0`;
  player 2: his entity). Alone as Krillin, Krillin does. 2. The line, then the blow: 16's punch (STRIKE-H into the
  rock face) or Krillin's BLAST-1; `WhiteFlash 10`, `ScreenShake 0, 20, 30` as today, dust at the wall, the hum
  (if a looping sound is playing) cut. 3. He drops back down; the next line with him in view.
- **Risks:** the pan leaves the players' ground: flying a player to a ledge he cannot stand on needs him flown
  back before control; player 2's body through the cine helpers. **Size S.**

### 9.3 The rubble (leave)

- **Where:** `ch9_rubble`, map 250 trigger. Film `gero_KA`. Krillin's line, 16's "Stand back.", the punch or the
  blast, a flash, a shake, dust, the cave open, "KRILLIN: ...Remind me never to arm-wrestle you." It reads. Leave.

### 9.4 The bunker (touch up, S)

- **Where:** `ch9_bunker` and `ch9_bunker_won` entries 0-11, map 257 (8/63). Films `gero_KA`, `gero_K`.
- **Today:** the camera goes to the computer and stays there; the players are below the view for the "ASSEMBLY
  COMPLETE" captions, the lines, and after the fight for the whole file on Android 13 (seven lines over a still
  picture of the computer); a blast and a white.
- **Lineups:** as 9.2.
- **Shot list:** 1. The players walk up to stand **in view** under the computer (two marks) before the first line.
  2. "ACTIVATING...": a `WhiteFlash 6`, and two 19 bodies walk out from behind the computer, left and right, to
  the record's spot and its twin's (then the actors go and `TriggerMapEvent 1` stands the bosses). 3. After the
  fight: marks behind a short fade; a green `FlashScreenColor` flicker of the screen before "ANDROID 16: Wait.
  There is one more file.\x0bANDROID 13:  UNFINISHED.  FINAL STAGE ON HOLD."; 16 a step closer to the console,
  Krillin LOOK from him to it. 4. "KRILLIN: ...I don't like the sound of that.  Smash it!", a wind-up, the stock
  blast, impact (flash, shake) before the white.
- **Risks:** 19's sprite: 0 1 2 5-8 12 (walks); two actors of one sprite (a step finds the first: move and
  despawn the first before the second, as `z8_dino_defeat`). **Size S.**

### 9.5 16 and the birds; home (touch up, S)

- **Where:** `ch9_bunker_won` entries 11-27 (8/48, then the workshop). Film `gero_KA` (sheet 2).
- **Today:** outside, two hawks walk in above; Krillin wanders over; "ANDROID 16: The birds.\x0bDr. Gero built me
  to destroy Goku.  He did not build me to watch birds.\x0b...I like them."; he turns to Krillin; the Cell Games;
  home: Bulma, Krillin, and with two players Hercule walks in.
- **Lineups:** as 9.2; alone as Krillin, 16 is kept for the plot.
- **Anime:** the birds come to 16; he stands very still for them.
- **Shot list:** one beat: before his line a hawk hops down to his feet (a walk, the hawks have 0 1 2 6) and stays;
  16 does not move for 40 frames. When he turns to Krillin the bird hops away.
- **Risks:** a text box on this full map takes bodies off the list (noted in `staging_ch9_gen.py`). **Size S.**

## 7. The Broly side chapter

### B.1 Bulma's news; Paragus in the backyard (touch up, S)

- **Where:** `broly_tv` entries 9-11 (map 160); `broly_yard`, map 165 (zone 6 area 21) trigger 1. Films `tv_P`,
  `ride_VT`.
- **Today:** Bulma runs in and out. In the yard the players walk to the lawn, Paragus comes down the ship's ramp,
  five lines standing; the others' lines when played; he goes back in.
- **Lineups:** Vegeta as player 1 or 2, with anyone (Trunks, Krillin, Piccolo, Gohan have a line each; with none of
  them Paragus' "Then come, my king.  Your people are waiting.").
- **Anime:** Paragus goes down on one knee before the prince.
- **Shot list:** 1. Paragus KNEEL at the ramp's foot for "PARAGUS: Prince Vegeta...  At last I have found you."
  and stays down through Vegeta's "That armor...  You're a Saiyan!  Who are you?"; RISE for "My name is
  Paragus. ...". 2. Vegeta turns to look at the ship, then back, before "A planet of my own...  Hmph. ...". 3.
  Each played companion steps up for his line (rule 8) and Vegeta does not look at him.
- **Risks:** Vegeta may be player 2: his turn is then his entity's. **Size S.**

### B.2 The ship (leave)

- **Where:** `broly_board`, `broly_ship_out`, `broly_end` entries 7-11 (zone 15 area 9, then the landing). Films
  `ride2_VT`, `rim_VT`. The room shakes for the flight under two captions, Paragus gets off first, the players
  walk out and down the ramp to his welcome. Functional, short, Ryan's own design. Leave (player 2 is not drawn in
  the room: see section 0).

### B.3 The rim: the betrayal, Broly (showcase, M)

- **Built 2026-11-06 (`nb-cs2`, cine helpers).** The basin is empty while Paragus speaks (Broly waits out of view at (520, 404)).
  On "Broly!" Paragus turns east, the ground shudders twice, dust at the players' feet, everyone looks east and the camera pans
  56 px to him: his back to them, a 90-frame charge (`CoopCall 37, 7, 90, 2`: the ki pose, white flickers, rings, the burst) with
  pale green light, the lead pushed back 8 px; silence; he turns his head, then round; "BROLY: ...Ka...  ka...  rot..." in a bottom
  box (speaker 0 without `@^`), nothing over him. The lines of who is played as before. Before "BROLY: KAKAROOOOT!!" (bottom box) a
  white flash and the ground heaves; then he is in front of them in a blink (dash to (400, 414), the spot the fight opens from)
  and his fist goes into the ground at their feet (a heavy strike at a point: ring, flash, boom) with dust and the lead thrown
  back a step; Paragus walks to his twin's spot through it. The sequence grew by one entry (the fight is entry 18).
- **Vegeta's knee (its own commit, for Ryan's review; decided for him while away):** after his step back he drops to one knee
  for "The Legendary Super Saiyan...  It can't be..." (`CoopCall 37, 10, 0, 1` on `-12`, the player who plays him, and on his actor
  137: the cast nobody fills plays nothing) and gets up on his own before the roar (after Trunks' shout when Trunks is played).
- **Marks:** player 1 (326, 414), player 2 (284, 400), Paragus (384, 380), Vegeta's actor when nobody plays him (352, 400).
  Filmed: Vegeta + Trunks, Gohan alone (Vegeta's actor), Piccolo + Vegeta. `staging_broly` (nine lineups) unchanged but for a
  shot of the power-up.

- **Where:** `broly_rim`, zone 15 area 3 (through `data/maps/broly_arena.json`), once (flag 403; the retry goes
  straight to the fight). Films `rim_VT`, `rim_G`.
- **Today:** the players walk to their marks; **Broly strolls in** from the top right to his spot while Paragus
  comes down the rim; the three lines of the betrayal; `WhiteFlash 5`, a short shake, and the caption "@^BROLY:
  ...Ka...  ka...  rot..." **in a centred box that half covers him**; Gohan's and Piccolo's lines when played;
  Vegeta backs one step for "The Legendary Super Saiyan...  It can't be..."; Trunks' line when played; "@^BROLY:
  KAKAROOOOT!!"; Paragus walks to his son's side; the fight.
- **Lineups:** in play Vegeta alone or Vegeta + Trunks (boarding makes them so); the tests cover any of the trip's
  five in either hand, and Vegeta's actor (137) runs up when nobody plays him (kept for the plot).
- **Anime:** the power comes first: the ground shudders, a green light climbs, and the Legendary Super Saiyan is
  just there, huge, breathing "Kakarot". Vegeta, of all people, goes to his knees and cannot fight; the others have to shout him back up. Then the scream.
- **Shot list:**
  1. The players to their marks and Paragus down the rim as today, **with the basin empty** (Broly's walk-in
     removed: he is placed out of view).
  2. The betrayal's lines kept: "VEGETA: Paragus!  What is this?  Where are my people?" / "PARAGUS: Your people?
     There are none, Prince.  There is no kingdom.\x0bThis planet is a graveyard...  and it is about to become
     yours." / "PARAGUS: Your father sentenced my son to death as an infant because his power dwarfed yours.
     \x0bWe survived.  Now meet him.  Broly!" Paragus turns to the basin on the name.
  3. Power before the body: the rumble, `ScreenShake 0, 6, 40` then `0, 12, 40`, small dust near the players'
     feet; everyone turns east; a slow pan east until Broly is in the view, standing in the basin with his back
     to them.
  4. CHARGE on Broly: green `FlashScreenColor` pulses with white between, `ScreenShake 0, 20, 90`, a ring of
     dust. The players are pushed back 16 px and brace; Paragus shields his face.
  5. Thirty silent frames. Broly turns his head, then his body. "BROLY: ...Ka...  ka...  rot..." **in a bottom
     box** (speaker 0, no `@^`), so nothing covers him.
  6. Gohan's "Kakarot?  That's my dad's Saiyan name!  He thinks I'm...?" and Piccolo's "His power is off the
     charts...  Everyone, stay sharp!" when played, each with a step.
  7. Vegeta backs away (the stock step) and **KNEEL** on "VEGETA: The Legendary Super Saiyan...  It can't be..."
     (his body: char 0 when he is played, the guarded actor otherwise). With Trunks played: "TRUNKS: Father, snap
     out of it!  We have to fight!" and Vegeta RISE. Without him: Vegeta RISE on the roar.
  8. "BROLY: KAKAROOOOT!!" (bottom box) with `WhiteFlash 8` and `ScreenShake 0, 24, 40` before it; then Broly
     DASH 80 px toward the players and STRIKE-H **into the ground** in front of them: a dust burst, a shake, the
     players thrown back a step. He does not touch them: the fight is the players'. Paragus walks to the twin's
     spot through the dust (as built); control.
- **Risks:** `DespawnCharacterAndRecordPosition 159, 2` starts the fight from where Broly's actor stands and
  Paragus' twin is 24 px right of him: his dash must end on the spot the fight should open from. Broly's groups:
  0-8 11-13 17 20 (which of them is a charge pose is unread; no beam). The kneel on Vegeta's actor needs the
  `CoopCall 22, 2, 137` guard the scene already uses. `staging_broly`'s rim presses (`press(3, 100)` then the
  record counts) move with every added beat. The caption-to-box change keeps the words. **Size M.**

### B.4 Broly's rage (showcase with B.3, S)

- **Where:** `broly_rage` (queued by `enemies.c` when Paragus falls first). Film `rim_VT` (test command 9).
- **Today:** a flash and a shake; two captions over him; a flash and a shake; "@^Broly's power explodes!  He is
  stronger and faster than ever!" tells what happened.
- **Lineups:** whoever is fighting.
- **Anime:** in the film Broly kills his father himself; here he rages at his fall (Ryan's story). The look is the
  same: he stops, he sees, and the power doubles.
- **Shot list:** 1. Broly LOOK at Paragus' body (the downed twin lies on the map by the both-down rule). 2.
  "BROLY: Pa...  ra...  gus...?" (bottom box). Hold 40 frames. 3. CHARGE, longer and harder than at the rim:
  green pulses, `ScreenShake 0, 24, 80`, two rings of dust, the players pushed back. 4. "BROLY:
  RRRAAAAAAAAGH!!" 5. The caption kept.
- **Risks:** it runs in the middle of a fight: players and enemies are live and anywhere; keep it under about four
  seconds, address only sprite 159 and the players, nothing on fight frames (`perf_budget`). **Size S.**

### B.5 Broly's end, the planet (showcase with B.3, M)

- **Where:** `broly_end` entries 0-7. Film `rim_VT`.
- **Today:** "@^BROLY: KAKAROOOOOOT!!", hit effects and four shakes on him, Vegeta's line (or player 1's), two more
  shakes, a white, the ship's room. **Broly is still on his feet when the screen goes white**, and the planet's end
  is a caption in the ship.
- **Lineups:** as B.3 (Vegeta's line is player 1's when nobody plays him).
- **Anime:** his own power tears him apart from inside in green light as the planet breaks up.
- **Shot list:** 1. Marks behind a 20-frame fade: Broly in the middle of the view, the players below. 2. "BROLY:
  KAKAROOOOOOT!!" 3. He overloads: CHARGE that does not stop, green pulses, the stock hit effects on him, dust in
  lines across the ground (the cracks), the shakes rising as stock (30, 40, 80). 4. "VEGETA: His power is tearing
  the whole planet apart!  Everyone, back to the ship!" and the players **turn and fly for the south edge** before
  the white (player 2 with him). 5. Broly KNEEL; the last flash; under the white the stock jolts of a boss's death
  (map 172's defeat script is the pattern). 6. The ship's room as today, with a `WhiteFlash 20` and a hard shake
  as "@^The ship escapes the exploding planet and heads home..." opens.
- **Risks:** the players fly as char 0 and player 2's entity, then the stock `WarpToMapWithEntities 15, 9`: they
  must not be despawned first; `staging_broly`'s `cb_*_veg` shot waits 200 frames after the win. **Size M.**

### B.6 The debrief (touch up, S)

- **Where:** `broly_home_out`, map 165. Film `rim_VT` (sheet 2).
- **Today:** player 1 walks down the ramp, Bulma comes to meet him, the lines of whoever is played. **Player 2's
  line plays with no body of his in view** (Trunks' "If that's what a Legendary Super Saiyan is like..." over
  Vegeta and Bulma).
- **Shot list:** player 2's **stand-in** comes down the ramp behind player 1 and stops beside him; each speaker a
  step (rule 8); Vegeta turns away from Bulma for "Hmph.  A Legendary Super Saiyan...  and I sent him to the
  grave.  Remember that."
- **Risks:** the scene ends by restoring the party (`CoopCall 10`) and a reload: the stand-in goes before it.
  **Size S.**

## 8. Priority for the developer (build order)

1. **5.6 The fusion** (M). One lineup, the Lookout's small disc, no fight around it: the place to prove CHARGE,
   FALL / RISE, the glide and DASH before the bigger scenes use them. Build 5.5 with it.
2. **7.4 Perfect Cell: the absorption, the kick, Vegeta's fall** (L, two commits: area 5, area 6). One lineup for
   every player. The batch's best clip. The Final Flash variant waits for Ryan.
3. **B.3 + B.4 + B.5 Broly** (M + S + M). His entrance is a stroll and his end leaves him standing today; the three
   share one CHARGE look.
4. **6.8 (c) Tien's stand and Goku's rescue**, then (b) 17's absorption, then (a) (M, M, S). The stock scene is
   good; the Tri-Beam is the weak beat and the famous one.
5. **5.8 the Kamehameha and the arm**, then **5.7 the town** (L, M). The minute of unmoving text is the worst
   stretch of the batch.
6. **6.4 the Time Chamber duel** (M). It also repairs the one-player version, where Vegeta tells an empty room to
   get up.
7. **7.6 "pump up your muscles"** with **7.2 Vegeta's power-up** (M, M): speed and power shown before they are
   spoken, in the two zone 12 scenes that only tell them.
8. **The cheap repairs:** 6.7 the island's break (nobody in view), W.3 Cooler (players below the view, death off
   screen), 9.2 the generators (nothing seen), 6.5 16's anger, B.6 and the other stand-ins (each S).

Everything else rated touch up is S and can follow in any order; the scenes rated leave should stay as they are.

## 9. The batch's best Short

**Perfect Cell (7.4), about 35 seconds, the same for every player.** It needs no setup: a monster's tail closes
over a girl, the screen goes white, the shaking stops, and in the silence a new figure stands with his back to
us. Vegeta's kick lands square and nothing happens; "That kick!  I put all my energy into it!"; "Now, do you
understand what I mean by "perfection"?"; one blow, and the Prince of all Saiyans is face down in the sand in his
black hair. It has the three things the guidelines ask for in one clip (power shown and then spoken, a hit with
weight, a held silence), the lines are the game's own, the anime's most quoted villain is the star, and because
the scene makes Vegeta player 1 for everybody there is a single cut to film. Second choices: the fusion (5.6,
about 25 seconds, the quiet one) and Tien's Tri-Beam into Goku's rescue (6.8 (c), about 40 seconds, the loud
one); Broly's entrance (B.3) is the pick if the Short should be a jump scare.

## 10. Findings outside the shot lists (for whoever builds)

- `staging_ch5`'s fusion scenes run without flag 129: Kami does not stand in them (5.4).
- 7.1: player 1 can be Hercule or Krillin when Vegeta and Trunks come out; the stock walks him up for Piccolo's
  line. From the script, not filmed.
- 6.5: the stock flight trigger spawns Krillin's actor to watch, whoever plays him. W.1: "Meanwhile..." spawns
  Piccolo, Trunks and Vegeta by player 1's slot only. Both from the scripts: check for a second body when player 2
  plays one of them.
- One player, 6.4: "Now get up." is said to an empty chamber (filmed, `tc_T`).
- Idle players die fast against Android 16 and Perfect Cell: tests of the scenes after those fights must win at
  once.

## 11. Lines

No DRAFT line is proposed. Variant beats that need Ryan's word because they add something the game does not have:
the Final Flash in 7.4; Cell's escape shown as a blinding light in 5.8 beat 9 (today a flight); Vegeta on his
knees in B.3 beat 7 (the film's beat, on the game's own lines).

## 12. Reaching each scene headless (what the films used)

All from `menu.ss`: `maptest 7`, 200 frames, `poke8 0x0203F000 P1` (and `poke8 0x0203FF84 P2` with `p2 0`), then
test command 5 (`cmd_arg` zone | area << 8, `cmd_arg2` x | y << 16). Presets are the `preset` line before the Map
Test.

| scene | preset | warp (zone, area, x, y) | notes |
|---|---|---|---|
| 5.1, 5.2 | none | 9, 6, 112, 224 | runs on into Gohan's house |
| 5.3 | none | 10, 8, 224, 262 | |
| 5.4-5.6 | `+129 +131` | 10, 16, 96, 150 | 129 for Kami's record |
| 5.7, 5.8 | `+129 +131 +132` | 10, 17, 280, 300 | test command 7 after the intro for 5.8 |
| 6.1 | `+136 +466` / `+136 +137 +138 +466` / `+136 +137 +138 +139 +140 +466` | 8, 62, 128, 240 / 8, 63, 200, 320 / 8, 48, 140, 290 | larva / computer / cave |
| 6.2 | `+136 +140 +141` | 11, 3, 136, 100 | |
| 6.3, 6.4 | `+136 +140 +141 +142 +143` | 9, 3, 128, 230 | two players: the duel ended by a blow (player 2's HP at `0x0203F060` to 1, his entity beside player 1, Right, A x4) |
| 6.5 | `+136 +140 +141 +142 +143 +144` | 11, 1, 640, 432 | |
| 6.6-6.9 | `+136 +140 +141 +142 +143 +144 +145` | 11, 4, 352, 224 | command 7; A in area 5 flies on; command 7 in area 6 at once |
| 7.1 | `+136 +140 +141 +142 +143 +144 +145 +146 +148` | 9, 3, 128, 230 | |
| 7.2-7.6 | `+144` | 12, 15, 344, 100, then A | `stepz` each scene, command 7 at once (three times) |
| W.1 | `+151` / `+151 +155 +156` and `maptest 160` | 6, 5, 224, 300 / walk Left 30, Down 40, Right 30 | memory / broadcast |
| W.2 | `+60` | 4, 50, 470, 230 | |
| W.3 | `+60 +61` | 6, 61, 200, 400 | |
| W.4 | none / `+212` / `+212 +213` | 6, 70, 132, 262 | |
| 9.1 | `+151 +155 +156 +157` | 6, 5, 224, 300 | |
| 9.2-9.5 | the same, flags 0-191 set, `+420`, test command 11, the lineup poked again | 8, 27, 476, 190 / 8, 40, 30, 300 / 8, 48, 140, 290 / 8, 63, 216, 300 | |
| B.1 | `+151 +155 +156 +157 +400`, `maptest 165` | 6, 21, 168, 272, then walk Left 80 (both pads) | |
| B.2 | `+151 +155 +156 +157 +400 +377` / `+151 +155 +156 +157 +400 +401 +377 +378` | 6, 21, 344, 440 then Up / 15, 9, 120, 100 then Up | flight out / landing |
| B.3-B.5 | `+151 +155 +156 +157 +400 +401 +378 +377` | 15, 3, 312, 460 | command 9 for the rage, 7 for the end |
| B.6 | the same `+402` | 15, 9, 120, 76 | |
