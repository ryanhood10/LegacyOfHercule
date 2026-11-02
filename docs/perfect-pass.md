# The perfecting pass: finish the LoG2 half before the Buu campaign (planned 2026-11-01, nothing built)

Ryan, 2026-11-01: "perfect what we have FIRST". This page is the plan: what the two-player playtest has to cover
(section 1), the decisions made (2), the story polish to address (3), Super Saiyan Hercule (4), the tournament's and
combos' open fixes (5), and the order (6). Nothing here is built; each item names its test. The Buu campaign waits
(`docs/story-buu-rom.md`).

## 1. The co-op playtest list (Ryan and his brother, on the play build; the combo notes wait for this)

**Setup.** Two pads on one mGBA (`P2_MODE_SIO`: player 2 exists while a second controller sends input; 3 s silence
drops him). Every game start asks "How many players?" (A: 1, B: 2). The drop-ins F1-F9 (`build/play/DROP-INS.txt`)
are one-player saves: after loading one, player 2 joins by pressing a button on pad 2 (the join is `dropout.txt`'s
recipe). Saving inside F2-F7 replaces the real save (backups in `build/play/backup/`).

**First session (2026-11-01, Ryan and his brother over Parsec, his brother's pad through `p2_keys.py`):** three
things came back, two built the same night (section 9): player 2 could not take the food dropped from boulders and
enemies (fixed: `src/coop/pickup.c`); Ryan wants resting to heal (built: `src/coop/regen.c`, to 2/3 of the maximum, the
rest the stock way); and "at a certain point it quit allowing us to ki blast, I guess after we saved" (open: most
likely both EP bars were empty, 2 EP a blast and 3 % back every 10 s, and player 2 could not refill from drops; to
confirm with Ryan: was the lower bar empty, who were they playing, did L change the ability).

Take notes as you go: one line per thing (where, who played whom, what happened, what you expected). The things no
headless test can judge are **feel**: is it fun with two, is anyone bored, is anyone lost.

| where | what to try | what to watch |
|---|---|---|
| any field map | walk apart, fight a pack, one player idles | the shared camera: nobody pushed off screen or stuck at the view's edge; the idle player not dying (the twins target both) |
| any save pad | Switch Characters for each player; player 2 picks a character player 1 has | never the same character twice; player 2's HUD, portrait and moves follow the pick |
| any fight | let one player fall | the revive prompt, the countdown, the game over when nobody revives; EXP going to the player with the last hit |
| the world map | both players at a flight sign; the passenger | one player flies, the other rides with no entity; both land |
| F1, F2 | the Frieza landing and Trunks' arrival with player 2 as Hercule, then as Krillin | the lead walks or flies in like everyone else (never pops in); nobody doubled on the sideline |
| F3 | Gero's computer: 16 on the table, the bunker's two 19s | 16 selectable after; the pair rule (the first 19 down stays down) |
| the Time Chamber | the ten rounds with two players | eight on screen, the captions, no soft-lock when both fall |
| the dojo and the duels | a duel between the two players | the winner's line, both refilled, the loser not stuck |
| F4 | the Cell Games with two players | the barrier (only Goku passes), the player-2 fights (J2 Piccolo, J4 Trunks), the beam struggle with both, the six Juniors' spawn |
| F5 | Super 13 with two players | the rocks, the transformation, both players' damage counting |
| F6 | Dende's wishes, the ending, the "Peace Times" question | the question's Yes / No, nothing after Yes breaks a save |
| F8 | Papaya Island early: a Rookie 1 v 1 with player 2 waiting | the co-op final (the two players duel), the passenger's view |
| F9 | 2 v 2 with each listed pair, Select for the combo | the combo bar, the freeze, who it hurts, the colours; then a Royal Rumble with both |
| difficulty | the same fight on Easy and Hard | whether Hard is a wall for two and Easy a bore |
| player 2 leaves | unplug or stop pressing for 3 s, then come back | the drop-out and the return, in the field and in a fight |

## 2. Decisions (Ryan, 2026-11-01)

- **A loser's line after a tournament fight: no.** Only the winner speaks (the default stays).
- **Start skips the fighters' talk:** not decided; the default (it skips) stays until he says.
- **Yamu walks at normal speed (Ryan, 2026-11-01)**, though a 64 x 64 body (the giants walk at 85 %): "the quicker
  thief" (`tny_walk_pct`).
- **Super Saiyan Hercule: the same rules as the Super Saiyans** (section 4).
- **The story polish:** all of it is to be addressed (section 3), after the playtest's notes are in.
- **The combos' playtest notes** wait for the two-player session; the combos are not touched until then.

## 3. Story polish (offered 2026-10-20, now all wanted; each with its test)

**Built 2026-11-01 on `pp-story`** (items 1-4, the scene list in `docs/story.md` 0; tests `lead_scenes.txt`,
`ch9_actors.txt`, `gohan_ssj1.txt`, `cell_juniors.txt`, `ch1_notes.txt`, `player_lines.txt`).

1. **The story's lead in every scene when someone else plays him.** Done for the Frieza stretch only
   (`src/coop/companion.c`: when nobody plays the lead, an actor with his sprite walks or flies in and speaks his
   speaker-1 lines; `docs/coop-design.md` "The story's lead and companions"). To do: audit every chapter's scenes for
   a lead the players may not be playing (the Lookout and Trunks' chapter with Gohan as the lead; Vegeta's chapter;
   Piccolo's; the island; the Cell Games' arrival; the post-game), and give each the same actor. Test: a
   `scene_actor`-style script per chapter in both pairings (the lead played, the lead not played).
2. **Krillin and 16 in the Gero rubble and bunker scenes** when they are not the players' characters: actors in
   `ch9_gero` for the rubble (16 punching through) and the bunker (the computer, the two 19s), the duplicate-NPC rule
   hiding them when a player is one of them. Test: `ch9_gero.txt` in the four pairings.
3. **Gohan's Super Saiyan 1 from the Time Chamber.** Now the teen Super Saiyan Gohan exists from the Cell Games'
   start (`docs/story.md` 4h). To do: the chamber's "Training completed!" (or the wait's end) sets the flag that
   gives Gohan his Super Saiyan row and ability, so he has it for the ten-day wait's fights too. Check: the Cell
   Games' cutscene Gohan and Super Saiyan 2 Gohan (slot 8) are untouched. Test: `wait_chamber` then a fight as Gohan
   transformed.
4. **The six Cell Juniors' spawn** (`cell_games.txt`): confirm all six come out in co-op, where, and that the pair
   rule and the twins don't double them. A read-and-check first; a fix only if it is wrong.

## 4. Super Saiyan Hercule (Ryan: the same rules as the Super Saiyans)

**Built 2026-11-01** (branch `pp-ssj`): the rules as found are in `docs/engine.md` "Super Saiyan transformation" and
what he now does in `docs/hercule.md` 3b; test `tools/headless/ssj_hercule.txt`. Two corrections to the plan below:
everyone's Transformation is ability 12 (13 is Trunks' 5-EP move), and the drain is the Super Saiyan class's update
(`0x08017306`), not the dash state `0x080083C3`. The stock transformation costs no EP (it needs 10).

What he has (`src/coop/ssj.c`): ability 18 switches slot 5 to row 22 (sprite 157) for 50 EP through the game's
transformation opcode; `hercule_awaken` puts it in his list late in the story; no aura, no drain, no revert.

The stock Super Saiyan (Goku's Transformation): costs EP to transform, **drains EP while transformed** (the
flight / charge state `0x080083C3` drains against display-row byte +7; the transformed row's flag bit 0 marks it),
**reverts when EP runs out** and when the ability is used again, and `Party_DetransformOthers` turns back any
transformed slot that is not player 1 (the reason the game's handling runs only for the active player; `p2_refresh_sprite`
covers player 2). To do, in order:
1. Decompile Goku's transformation ability (the stock onUse for ability 13 and the row-20 handling) to list the exact
   rules: EP cost, drain per frame, the revert, the aura / flash and its sound.
2. Row 22 gets the same row bytes (flag bit 0, drain byte +7, the links 21 <-> 22) so the stock drain and revert run
   for him; where the stock code only serves player 1, extend it the way `p2_refresh_sprite` does.
3. The transform's aura / flash and sound: the stock transformation's (found in step 1), played for either player.
4. An icon pair for ability 18 (now borrowed): a recoloured Transformation icon with Hercule's face, through the
   status-module icon tables (`docs/party-8-slots.md`).
5. His moves while transformed stay his (Ryan has not asked for a Super Saiyan moveset).
Test: `SSJ_TEST=1` (`docs/testing.md` "Super Saiyan Hercule"): transform as player 1 and 2, the drain, the revert at
0 EP and by the ability, a save and reload while transformed.

## 5. The open fixes (no decision needed)

The tournament's (**24.1-24.7 all built and merged 2026-11-01**; what was built is in `docs/tournament.md` 24), each planned there with its test and size: **24.1** the announcer's
calls during a Rumble as a non-pausing caption (half a day); **24.2** portraits for Android 13, Cell Jr., the Namekian
Warrior and Evil Buu, ids 169-172 (hours); **24.3** Yakon's lying pose (an hour or two); **24.4** the announcer walks
off and on (hours); **24.5** the co-op turn change's black screen, a readiness check instead of 90 frames (an hour);
**24.6** Papaya Island's world-map landmark (built 2026-11-01, branch `pp-map`: `docs/tournament.md` 24.6); **24.7** walking speed per playable (half a day
to a day); **24.9** the merged worktrees removed once the films are not wanted.

The combos' leftovers (`docs/tournament.md` 25.4 "Still not done"; **all three built and merged 2026-11-01, `docs/tournament.md`
25.5**; the kits' colours also give Cooler's own Supernova gold and Broly's orbs green, as he has no beam move):
- **Cell Jr.'s copies mirror his frame**: a copy draws the source body's current frame; a copy facing the other way
  should draw that facing's frame from the record's direction array (`copy K X Y` gets a facing from X's sign). Test:
  `tny_combo_tune.txt`'s Cell film.
- **Android 16 and Super 13 keep animating as bystanders**: their own code owns their states; give `a16.c` and
  `super13.c` a check of the combo's freeze (`tny_combo_active()`) in their per-frame update so they hold. Test: a
  Rumble with 16 in it and a combo fired.
- **The kits' beams in their colours outside combos**: the kits table (`tny_moves.c`) gets a colour column (Frieza
  pink, Cooler purple, King Cold red, Gohan's Masenko yellow); the beam move calls `beam_paint` with it after
  `Beam_Fire`. The colour sets exist (`tny_combo_beams`). Test: `tny_moves.txt` filmed.

## 6. Order

1. The two-player playtest (Ryan and his brother; section 1), notes in.
2. The notes' fixes plus the combos' notes.
3. Section 3 (story polish) and section 4 (Super Saiyan Hercule): a build round each, with Ryan's lines verbatim.
4. Section 5 in branches by a small team (art: 24.2, 24.3, the copies; engine: 24.1, 24.4, 24.5, 24.7, the
   bystanders, the beams; decoding: 24.6), merged and regressed (`regfull.sh`).
5. Then the Buu campaign's plan (`docs/story-buu-rom.md`).

## 7. The team (2026-11-01, Ryan: "split off into a TEAM ... get these developments done")

Six agents, each in its own worktree `F:/MeltyCreations/log2-<piece>` (the story piece: `log2-pp-story`) on branch `pp-<piece>`, building into
`build/pp-<piece>`; numbers from `docs/ids.md` "Allocations for the perfecting pass". The lead (the main session)
merges each branch, rebuilds and runs `regfull.sh`. Nobody touches the combos' step tables or tuning.

| piece | owns | done when |
|---|---|---|
| story | section 3: `companion.c`, `data/story/*` scene edits, the chamber's end (`waves.c` / `wait_chamber`), the Juniors' check | the lead walks into every scene in both pairings; Krillin and 16 in the Gero scenes; Gohan transforms after the chamber; the Juniors checked |
| ssj | section 4: `ssj.c`, row 22's bytes, the aura and sound, ability 18's icons | transform, drain, revert at 0 EP and by the ability, as player 1 and 2, a save and reload |
| tny | 24.1 caption, 24.4 the announcer's walk, 24.5 the turn change, 24.7 walking speed | each has its test in `regfull.sh` |
| art | 24.2 portraits 169-172, 24.3 Yakon's lying pose | the faces in `tny_talk.txt`; the lying sheet |
| combo | the copies' facing, 16 / Super 13 holding during a combo, the kits' beams in their colours | the Cell film, a Rumble with 16, `tny_moves.txt` filmed |
| map | 24.6 Papaya Island's world-map landmark | the landmark in a world-map flight |
| reset (added 2026-11-01, `log2-reset`) | section 9: the soft-reset load crash (reproduced: two players, save, A+B+Select+Start, load) | a soft-reset load test in `regfull.sh` |

## 8. Ryan's one-player notes (2026-11-01, playing `build/play` alone)

"The beginning of the game is strong." Notes in his order; Ryan's lines are kept word for word (spelling and
punctuation only); owner in brackets.

1. **The prologue: Future Gohan knocks Trunks out** [story]. After the knock-out and before Gohan's "sorry" line,
   Hercule reacts: HERCULE: "What are you doing?!" GOHAN: "It's not safe out there Hercule! You stay here and watch
   after Trunks." Then Gohan's "sorry Trunks" line as it is. (The line is not in the text dump of zone 1 areas 1-2 by
   a plain search: find where the knock-out plays.)
2. **The prologue: Gohan's death** [story]. Walking up to the scene with old Hercule following, he stops at the
   scene's start and pops back next to Trunks. He should walk into the scene with the player (`SetEntityFollow` /
   the companion's position at the trigger), never pop.
3. **After Trunks kills Frieza, playing as Gohan** [story]: Krillin is missing from the group that flies over to talk
   to Trunks (the companion / lead-actor work: Krillin's actor when nobody plays him).
4. **Talking to old Hercule beside Trunks** [story]: he has no line. Player 1's character asks "Who are you?" and he
   laughs it off: "Oh, don't worry kid, you'll find out!" (Ryan: "or something like that"; nothing may reveal him).
5. **Goku's arrival talk** [story]: old Hercule turns towards whoever is speaking (now often his back to the scene).
6. **Goku and Trunks' talk** [story]: old Hercule's interruption becomes Ryan's line: HERCULE: "Psst!  Trunks!  Is it
   safe to come out now?" then "Trunks! You didn't say we would be runnin' into guys like that on this trip! That Frieza
   fella isn't coming back, is he?" (replaces "That Frieza fellow isn't coming back, is he?" in `ch1_trunks_talk`).
7. **Krillin at level 1 is too weak** [lead; **done 2026-11-01**: 70 HP, str 2.5, pow 3.5, end 2.0, old saves lifted once; the Disc missed every enemy of a one-player game, fixed, `krillin_disc.txt`]: he cannot kill an animal. Still clearly weaker than Gohan, but slightly
   stronger and with slightly more health. His **Destructo Disc does not really hit** enemies.
8. **Goku's "Alright, Gohan, it's about time to fly over to West City and meet up with Piccolo."** [story] (stock,
   maps 11 / 12, zone 2 areas 5 / 6): drop "Gohan" so it fits Krillin as player 1: "Alright, it's about time to fly
   over to West City and meet up with Piccolo."

9. **West City's parade, playing as Krillin** [story]: player 1's lines show "GOHAN:" (stock map 65, zone 4 area 3:
   `#GOHAN: Sure!` is hard-coded on speaker 1; Piccolo's "Well Gohan" and "How's your training going, Gohan?" too).
10. **Yajirobe's Senzu Bean, playing as Krillin** [story]: he says "What's up, Gohan?" and the player speaks as Gohan
   (stock map 135, zone 4 area 94, and map 231: `PushActiveCharName`, step 12, gives "Gohan" for Krillin: find why,
   likely the name comes from his display row, a copy of Gohan's).
11. **A sweep for the same** [story]: every stock line that names Gohan, or speaks as him on speaker 1, in a scene
   Krillin (or another character) can play as player 1; fix each to the player's own name or a neutral line, as in
   item 8.

## 9. Found during the pass (not asked for; to fix or to ask Ryan)

- **Fixed (2026-11-01, Ryan's two-player session): player 2 could not pick up dropped food.** Decompiled: a dropped
  item with no popup and no count (food, senzu) registers a touch box in the list at 0x03001FE0 and the stock scan
  fires a box only for player 1's position (`docs/engine.md` "Dropped items and touch boxes"). `src/coop/pickup.c`
  tests player 2's position every input poll and fires the box the stock way with the active slot swapped to his.
  Test: `tools/headless/pickup.txt` (self-checking, in `regfull.sh`).
- **Built (2026-11-01, Ryan: "if you get away from enemies and you aren't hit, your health will regen after a certain
  amount of time to 2/3rds; the last 1/3rd the player will have to get the traditional way"):** `src/coop/regen.c`,
  from the base class's update (`ssj.c`): 5 s with no hit and no enemy within 128 px, then 1 % of the maximum every
  20 frames up to 2/3 (0 to 2/3 in about 22 s); a transformed player never rests (the Super Saiyan class runs
  instead, as stock). Numbers are the macros at the top of `regen.c`. Test: `tools/headless/regen.txt`.
- **Open (the same session): "at a certain point it quit allowing us to ki blast, I guess after we saved the game".**
  A save at a pad does not touch the ability gate or EP (headless `build/go/sv.txt`: both players blast after the
  save). Ryan: the EP bar was not empty. Found next: every ability's isReady checks the world switch (the object
  `0x03002080` +0xC = **0x0300208C**: `DisableWorld` / `EnableWorld` Steps 122 / 123 write it, and a map load sets it
  from the map header's +0x08 flags, 1 on field maps, 0 in towns: `docs/maps.md`), and the switch is **0 on Papaya
  Island's grounds** (zone 15 areas 11-15: beach, street, courtyard, waiting room, infirmary; headless from Ryan's save,
  which sits in the waiting room) and 1 in the ring (area 16) and on every field map: the stock town rule, no
  fighting in town. Ryan: they were elsewhere, "fighting enemies on the way to meet Future Trunks and Frieza's
  arrival" (zone 3, from the F1 pad), "if you don't find the bug it's okay". Not reproduced headless from the new F1
  with two players in (Krillin player 1, Gohan player 2): both players' blasts fire on the route's maps and after a
  save at the pad with player 2 in (`build/go/ch1.txt`, `abx2.txt`); the switch stays 1. One plausible cause for next
  time: **L (LB / LT on the pads) changes the selected move**, and a move costing more EP than the bar holds does
  nothing on B; the icon by the EP bar shows which move is selected. Left open.
- **Found on the way (2026-11-01 night):** the drop-ins F1, F2 and F8 were saved answered "1 Player" (from his save),
  so a second pad could not join them (the other six had never been asked and let the pad decide): `dropins.py` now
  saves every state with the question armed and unanswered, so each F-key asks on load (`docs/testing.md` "Drop-in
  savestates"). Also: player 2 as Gohan (slot 0, row 1) using the Transformation (ability 12, his list's index 2,
  reachable only by poke: L never selects it) jumps to garbage; and player 2's L did not cycle his ability in the
  headless run (`abx2.txt`: Gohan's index stayed 3) while player 1's did: to check in real mGBA before chasing.
- **Fixed (2026-11-01, branch `pp-reset`): a soft reset, then loading a save, crashed** a few frames after the load (a
  jump to garbage from `0x080109AC`); found by the ssj piece. The cause: the soft reset keeps EWRAM, so `coop.p2`
  still pointed at the old player 2's entity (any game where player 2 was in at the reset), no new player 2 was built
  and our code wrote into whatever the new heap put there. The soft reset now clears all our RAM first, as a power
  cycle does (`docs/coop-design.md` "Session logic"); test `tools/headless/softreset.txt` in `regfull.sh`, and
  `ssj_hercule.txt` reloads through the soft reset again.
- **The Super Saiyan cool-down is about half as long in co-op**: both players' base classes drain the shared party
  meter (+8), so the meter empties twice as fast (stock alone: about 21 s).
- **Broly's Eraser Cannon bursts at his own feet** right after he throws it in the tournament, and a damage number shows
  on him (90); both are older than the colour work (the combo piece's film, `build/pp-combo/films/kits/`).
- **After loading a save, the "How many players?" question showed on a black screen with no text box.** Fixed
  2026-11-01 (Ryan hit it in the two-player session: the first A after a load answered "1 player"): a loaded save gives
  player 1 control before its map fades in, and the open question held the fade. `players.c` now asks once the
  background palette has colour plus 40 frames (`screen_lit`, `shown`); `softreset.txt` expects the new byte.
- **Gero's lab door (`ch4_piccolo`)**: Hercule's "Let's negotiate!" and Piccolo's "Hercule.  Get behind me." play even
  when neither is on screen (one player as someone else); the scene is staged for Piccolo and Hercule together.
- **Ryan to decide (story piece, 3.1):** in four fights that are the lead's own (the Lookout's first visit, Kame House
  and the island for Piccolo, Gingertown, Semi-Perfect Cell for Vegeta) the lead is made player 1 behind a short fade,
  as Vegeta's Android 19 fight already did; the other way would be the lead as an actor watching his own fight.
- **Unused allocations**: ssj's flags 476-477 and test command 36; map's flags 478-479.

- **Ryan, 2026-11-02 (the second two-player evening): the revive countdown is 60 s on every difficulty** (`difficulty.h`
  `DIFF_REVIVE_S`, was 20 / 10 / 6; `revive.txt` waits for it). And "you loaded us in with 2/3rds health ... characters
  should have full health when you first get them": the resting cap never lowers anyone; the F-key states carried the
  health of the save they were made from (Krillin 40 / 70, Gohan 64 / 96). Every drop-in now starts with the whole
  party at full HP and EP (`dropins.py` `fill`).

- **Fixed (2026-11-02, Ryan's two-player session): West City's arrival put both players beside Piccolo, behind the
  parade float.** "A bunch of character models momentarily disappeared, then it returned to normal, but it loaded us
  in on a weird spot ... next to Piccolo (he's gated off) ... almost like the game was trying to force one of the
  players to be Piccolo." The stock scene (map 65 trigger 1, Goku's arrival) ends with a camera pan up to Piccolo
  at (280, 960) and 100 frames of wait, and when the sequence ends the view glides back to the players over the next
  frames; the co-op leash (`leash_p2`: both players kept inside the view) then snapped both into the still-far view,
  the landing is y 1248 and they ended at 1077, the panned view's bottom, past the parade float (not solid until
  flag 34). Alone, player 1 stays. Now the leash never moves a player more than 8 px (a walker is only ever a few
  px past the edge; more is the view's own doing) and stands down during a sequence, and player 2's pad is held to
  A while one runs (`input.c`; the stock already holds player 1). Test: `tools/headless/wc_arrival.txt` (self-checking, in `regfull.sh`). Full regression on that build
  (`build/wc`) clean: 59 blocks, exit 0; in `build/play` since 2026-11-02 01:26.

- **Ryan, 2026-11-02 (in the corn maze with his brother): no twins for the corn maze's triceratops.** "Remove the
  twin rule for the corn field maze, it is causing them to run into and block each other (hence blocking the path)
  and we cannot proceed; just for these small rhino enemies in the corn maze sections", then, played with no twins,
  "make those rhinos 50 % slower", then "if it's easier, just remove those triceratops". So: with two players the
  maze's triceratops (sprite 125, the bestiary's 108-111) on zone 4 areas 56, 57, 59 (maps 103, 104, 106) are not
  there. The loader puts the spawn hook's entity on the list (a null crashed it at frame 68), so each record's enemy
  is built and parked at (-256, -256) with no collision box, out of sight and chase range (`enemies.c`
  `corn_maze_single`). The King (sprite 127), the other triceratops maps (zone 6 area 3, zone 8 areas 15 / 19 / 21)
  and one-player play are unchanged. Test: `tools/headless/corn_maze.txt` (self-checking, in `regfull.sh`). Full regression on that build clean (60 blocks).

- **Ryan, 2026-11-02 (two players): Super Saiyan Hercule locked off.** "Hercule's Super Saiyan mode should NOT be an
  option yet in the game (don't even bother looking to when to unlock it, it's MUCH later in development). Fix this
  so player 2 can actually use Hercule's ki moves again." `ssj.c` `SSJ_HERCULE_UNLOCK` 0: CoopCall 4 (`hercule_awaken`)
  does nothing, and a save that already holds the ability (18 in his list's slot 2, over Megaton Punch) or his
  transformed row gets Megaton Punch (16) and row 0x15 back at the load (`party_save.c` -> `hercule_lock`). The
  transformation itself stays built and tested (`ssj_hercule.txt` pokes the ability in). Test:
  `tools/headless/ssj_lock.txt` (self-checking, in `regfull.sh`). The unlock's place in the story is Ryan's, later. Full regression on that build (61 blocks): only `ssj_hercule.txt`'s C3 differed, as the lock means it to (a
  transformed Hercule is turned back by a load and has Megaton Punch; its expectations updated, rerun 57 / 57); in
  `build/play` since 2026-11-02 02:48.

## 10. State at the end of the team round (2026-11-01)

All seven pieces merged into main and pushed; the full regression of the merged main (`build/main-final`, play ROM
`build/main-final/play`) exits 0: 56 blocks, `ssj_hercule` 56/56, `softreset` 12/12, the prologue on the play ROM
fine. Player 2's stock Transformation now changes his own slot (the story piece saw it turn player 1's before the ssj
merge; `gohan_ssj1.txt` checks it). In `build/play` since 2026-11-01 09:08, the night's build (pickups, resting heals, the fade fix, the drop-ins asking) since 23:47 (ROM identical to the regressed one; drop-ins F1-F9 remade and checked; his save and the old states backed up as `backup/legacy-of-hercule.11010906.sav` and `backup/ss_11010906/`). Open: section 9's items, the
lead-made-player-1 choice, the two-player playtest (section 1).

## 11. The second two-player session's notes (Ryan, 2026-11-02) and the plan for them

**Planning only.** Ryan: "I don't want to actually build any of them, but I want to make a detailed build/update plan
... we could even use a TEAM to stay aligned and break the tasks up ... ask and clarify any questions you have with me
before we ever proceed with writing code." Nothing below is built. Each item: his note, what is known, the plan, its
test, and the open questions (collected in 11.9). The pieces for a team are in 11.8.

### 11.1 Health regeneration

**Ryan:** "the health regeneration is perfect right now, great timing, battle awareness, etc, but you have player 1
regenerating all the way to full health when it should just be 2/3rds (player 2 worked fine). Also, you had player 1's
Ki regenerating too. We do not have ki regeneration."

**Known.** Our resting heal (`regen.c`) caps at 2/3 and never lowers. The rest comes from the **stock** base-class
update (`ssj.c` `ssj_base_update`, the stock rule rewritten): every ~10 s the active slot gets EP + 3 % of its maximum
and every other stock slot (0-5) HP and EP + 5 % (the stock game's way of healing the party members you are not
playing). In co-op that update runs once per player entity, so it runs **twice**: the active slot's EP comes back at
6 % every 10 s, and the other played slot (player 2's character, in slots 0-5) gets 10 % HP and EP every 10 s to full.
Which player that is depends on who is the active slot: player 1's slot is the active one, so by the code player 2's
character should be the one healing to full and player 1 the one whose ki fills; Ryan saw player 1 heal to full
(to confirm who was whom: 11.9 Q1). Either way the cause is the stock trickle, doubled, not the resting heal.

**Plan.** (a) Run the stock trickle once a frame (from player 1's entity only). (b) Keep the stock trickle for the
party members nobody is playing (benched: slots 0-5 not played by either player), and keep its benched HP to full as
stock, or cap benched HP at 2/3 too (Q2). (c) The two played slots get no stock HP trickle and no stock EP trickle:
their HP comes from the resting heal (2/3) and their ki from 11.2's rule. Numbers stay macros.

**Test.** `regen.txt` extended: both players at 10 HP in the town, 3 minutes: both end at exactly 2/3, EP untouched by
the old trickle; a benched member's HP and EP keep rising as stock (or to 2/3, per Q2).

### 11.2 Ki regeneration

**Ryan:** "the game does already have a mechanic to generate like a tiny tiny microscopic amount of ki, not sure how it
works. But I think we should make a ki regeneration mechanic as follows: it regenerates only up to 1/3rd (the rest of
gaining ki must be from the traditional way). This is independent from the health mechanic: we can take damage, hit
enemies, be near enemies, and have our ki regenerate to 1/3rd after 60 seconds of us not using any ki moves."

**Known.** The "microscopic amount" is the stock active-slot trickle above (3 % of the maximum every ~10 s, doubled in
co-op). Using a ki move: every ability's on_use spends EP (the stock ones, Hercule's, Krillin's, the Super Saiyans'
drain), so "used a ki move" = the slot's EP went down by an ability (a hook in `use_ability`, `player.c`, for both
players, plus the transformation's drain).

**Plan.** `regen.c` gains a ki timer per player: reset by any ability use on that player's slot; after **30 s** with
none (Ryan's answer to Q3), EP rises to 1/3 of the maximum over **40 s** (a 120th of the maximum every 20 frames; the
numbers are macros), regardless of damage, hits or enemies near. The stock trickle for played slots goes (11.1c), or stays
underneath as the stock game's own crumb (Q3). Transformed players: the Super Saiyan class drains EP and runs instead
of the base class, so no ki regeneration while transformed (stock).

**Test.** `regen.txt`: EP at 0 in the town, 29 s: 0; at 31 s rising; 1/3 at about 70 s and no more; a ki move resets
it; a hit does not.

### 11.3 Player 2 is a co-pilot

**Ryan:** "player 2 feels too much like a co-pilot. Right now, player 2 can't press A or B to advance cutscenes, press A
to activate the save / switch character pad, use the fly pads, or open doors to enter character-locked areas (by
punching with the correct character), walk into new areas (taking both players with them; right now only P1 can guide
the gameplay experience). Fly pads need to also make player 1 and 2 fly (for Hercule you can just make him run)."

**Known.**
- Dialogue advances on the stock input system's A / B (player 1's pad); player 2's pad is read into `coop.input2`
  and, since 2026-11-02, held to A during a sequence (`input.c`) but that A goes nowhere.
- Save pads, fly pads and map exits are trigger boxes in the list at 0x03001FE0 (`docs/engine.md` "Dropped items and
  touch boxes"): the stock scan tests player 1's position only (the save pad: kind 0, condition 0x08010AC7, A to
  open; exits: `WarpToMapWithEntities` payloads). Player 2's pickups already scan that list for him (`pickup.c`).
- The save pad's menu (save / switch characters) works on the active slot (player 1's); a save-pad switch in co-op is
  `switch.c` (player 1's character; player 2's own switch is Select, `p2_cycle`).
- The fly pad: "only the player who stepped on the flight sign controls the world map; the world map has no player-2
  entity; both land together" (`docs/coop-design.md`). The take-off is a scene on player 1's entity.
- "Only Piccolo" / "Only Vegeta" barriers (`docs/story-rom.md`): the barrier opens to a punch from the right
  character; the check reads the active slot (player 1's character), so player 2's punch as the right character does
  nothing. The barrier's class and its hit handler are not decompiled yet (first step of the piece).
- Map exits: player 2 is leashed to the view, so he reaches an exit but cannot fire it.

**Plan (the "player 2 agency" piece).**
1. Cutscenes: while a sequence runs, player 2's A (and B where the stock takes B) is merged into the stock input's
   pressed bits for that frame (one press, no double-advance when both press the same frame).
2. Pads and exits: every input poll, player 2's position is tested against the trigger list like his pickups; a
   save pad fires on his A (the menu opens; "switch characters" changes **his** character, Q4; saving saves), a fly pad
   fires on his A (the take-off scene, then the world map as today), an exit fires on contact and takes both (Q5).
   The stock's own scan for player 1 is untouched. The condition scripts of a trigger (kind / condition fields) run
   as the stock runs them (same functions, player 2's position in place of player 1's).
3. Fly-off for both: the take-off scene animates player 1's entity; player 2's entity gets the same flight animation
   beside him (an actor-style move as the story scenes do, `CoopCall 31`), Hercule runs off instead (no flight
   sprite; `docs/story.md`: he "runs" where others fly). Landing: both already land together.
4. Barriers: decompile the barrier object (find its class from a map's object records, e.g. map 103's "Only
   Piccolo"), then make its character check use the **attacker's** slot: `coop.acting` / the hit message's attacker
   word (`docs/engine.md` "Hits") gives the entity; player 2's slot is `coop.p2_slot`.

**Tests.** Headless: a two-player sequence advanced by `p2 1` presses only; the Northern Wastelands pad opened by
player 2 (the menu's shot, a switch that changes slot 0x0203FF84), a fly pad by player 2 (the world map loads), an
exit walked into by player 2 alone (both on the next map); a barrier punched by player 2 as the right character
(the barrier gone), by the wrong one (still there). The real-time feel (both flying) is Ryan's.

### 11.4 Papaya Island in co-op: lag, and the missing world-map board

**Ryan:** "I flew to Papaya Island to check out our tournament mode and the tournament grounds and waiting room to
fight were both very laggy (maybe because we placed randomly generated characters there) but it wasn't like that in
1 player. Also on Papaya Island, the world map indicator is missing from the ground; it has the ability to use it but
there's no board / map to walk up to."

**Known.** The grounds' crowds are actors drawn from the roster (`tny_grounds.c`, `tny_crowd.txt`); in co-op every
entity update also picks the nearer player (`entities.c`, cheap) and the waiting room keeps "today's competition" per
frame (`tny_grounds.c`). Parsec's own stream ran at ~53 fps on the host (its log), which looks like lag on the guest's
screen regardless of the game; mGBA's title bar shows the emulation's fps on the host (Q6 tells the two apart). The
fly pad's marker: our island maps (`data/maps/papaya_*.json`) have the fly trigger but no pad decoration / object on
the ground (the stock landing sites draw a sign; `docs/maps.md` 179).

**Plan.** (a) Measure before guessing: a frame-time probe (the frame's scanline at the end of the main loop, read
headless with the runner; `docs/testing.md` will get the recipe) on the plaza, the waiting room and West City, alone
and with player 2, on this build; then fix what the numbers point at (likely candidates: per-frame roster work in
`tny_grounds.c` / `tny_ui.c` with player 2 present, the second HUD panel's redraw, OBJ VRAM churn from the crowd).
(b) The board: add the stock flight-sign object to `papaya_plaza.json` at the fly trigger (the same object other
landing towns use), one-player and co-op alike.

**Tests.** The probe's numbers alone vs co-op (a budget: under one frame); `tny_grounds.txt` gains the sign's entity
and a shot of it.

### 11.5 Tournament mode in co-op: 2 v 2 and the Rumble do not start

**Ryan:** "most importantly, tournament mode did not work when I was in 2-player co-op mode. I tried loading in to 2v2
tournament, we selected our characters, and then nothing happened. It's like the trigger didn't work or I'm not sure.
I also tried the same when selecting royal rumble and nothing happened. So we really gotta get tournament mode working
for co-op! All the way through!"

**Known.** The engine's co-op paths are tested headless for 1 v 1 (`tny_turn.txt`, the turn change) and the combos
(`tny_combo_run.txt` "an opponent pair on a player"), and `tny_engine.txt` runs every division; whether its runs had
player 2 *in* through the director's selection and the ring entry for 2 v 2 and the Rumble is the first thing to
check (the test list in `docs/testing.md` "Tournament"). The selection screen's end hands over to the ring reload
(`tny_ui.c` -> `tny_engine.c`); a wait for "both players ready" or a pad check that only sees player 1 would hang
exactly as described (Q7 narrows it: did the screen stay on the selection, go black, or return to the room?).

**Plan.** Reproduce headless first: from F9 (every tier open) with `p2 0`, the director, 2 v 2, both picks, A: where
does it stop (a `tny_flow.py` film of it); then the Rumble. Fix the hand-off; then run every division **with player 2
in** end to end (the whole bracket, the record, the reload), the Rumble, and the 2 v 2 with the team combos, as new
blocks of `tny_engine.txt` or a `tny_coop.txt`. "All the way through" = the test plays a full tournament of each
division with two players and checks the record and the prize.

### 11.6 West City in co-op: models vanish for a moment, and it lags

**Ryan:** "the cutscene still had people disappear for a second when we entered West City, and also it was really
really laggy in that area of West City. I'm not sure if that's because now with co-op there are too many sprite
characters or what? We need to figure it out."

**Known.** The arrival's drag was fixed (section 9); the vanishing was not that. West City's first screen holds the
parade crowd (nine NPCs and the float) plus Goku's and Piccolo's actors in the scene, two players, two HUD panels and
the HUD meters: the GBA draws at most 128 sprites and OBJ VRAM is one pool (`docs/engine.md`; the allocator's 64-byte
blocks, "Tournament bodies"). Sprites past the limit are simply not drawn for that frame (they "disappear for a
second"), and a frame that runs past VBlank is a lagged frame: both symptoms point at the same place, too much per
frame, in co-op only (the second player, his panel and the twins' code paths are the difference). Not reproduced
yet: the headless runner draws every frame it is asked for and never shows an overrun.

**Plan.** The same probe as 11.4a (frame time and the OAM count per frame, in West City's first screen with the
parade, alone and in co-op); then the fix the numbers call for: fewer OBJ for the second HUD panel, the crowd culled
outside the view (the stock already culls by the view rect: check it holds for our entity loop), the per-frame
co-op work trimmed. The disappearing: if it is the OAM limit, the HUD and the players get priority (drawn first).

### 11.7 Order

1. 11.5 the tournament in co-op (the thing Ryan called most important; a reproduction first).
2. 11.3 player 2's agency (the biggest piece; cutscenes and pads first, exits, fly-off, barriers).
3. 11.1 + 11.2 the regeneration rules (small, one piece).
4. 11.4a + 11.6 performance (one piece: the probe, then the fixes), 11.4b the board (tiny, with it).
5. A two-player session to check all of it, with the combo notes still waiting (section 1).

### 11.8 The team (when Ryan says go)

| piece (branch) | scope | done when |
|---|---|---|
| tny-coop (`pp2-tny`) | 11.5 | 2 v 2, Rumble and every division start and finish with player 2 in, headless, end to end |
| agency (`pp2-agency`) | 11.3 | the five abilities above, each with a headless check; `docs/coop-design.md` "One player" updated |
| regen (`pp2-regen`) | 11.1, 11.2 | the trickle once, played slots capped, the ki rule; `regen.txt` covers all of it |
| perf (`pp2-perf`) | 11.4a, 11.6, 11.4b | the probe in the runner and `docs/testing.md`; West City and the island under a frame in co-op; the board |

Rules as in section 7: own worktree and `BUILD=`, numbers from `docs/ids.md`, the lead merges and runs `regfull.sh`,
nothing swapped into `build/play` until Ryan asks. The pieces touch different files (tny_*, input/player/switch,
regen/ssj, hud/entities/tny_grounds + a map spec); the perf piece's entity-loop change is the one overlap with
agency (both in `entities.c` / `player.c`): perf goes last or merges first.

### 11.9 Questions for Ryan (before any code) and his answers (2026-11-02)

1. **Who healed to full:** which character was player 1 and which player 2 when player 1 filled up? (The code says the
   benched trickle feeds the non-active slot; the answer tells whether something else is going on.)
   **Ryan: no idea, it's like every time.** So the regen piece starts by reproducing it headless with both pairings.
2. **Benched party members:** the stock game heals the characters nobody is playing to full over time (5 % every
   ~10 s). Keep that for them, or cap them at 2/3 too? **Ryan: keep stock, full.**
3. **Ki rule's shape:** once the 60 s pass, how fast to 1/3 (the heal takes ~22 s from empty to 2/3; the same pace?),
   and does the stock crumb (3 % every ~10 s) stay underneath or go entirely? **Ryan: start it after 30 s (not 60)
   and have it take 40 s to reach 1/3.** (The crumb: not answered; the plan drops it for played slots, so the only
   ki back is his rule and the traditional ways. To confirm when the piece starts.)
4. **Player 2 at a save pad:** "switch characters" there changes player 2's character (and saving saves), yes?
   **Ryan: yes, the same experience player 1 gets.**
5. **Exits:** when player 2 walks into a map exit, both go at once, exactly as when player 1 does? **Ryan: yes.**
6. **The island's lag:** on the host, did mGBA's title bar fps drop under 60, or was it the Parsec picture on his
   brother's side that stuttered? **Ryan: lag on his mGBA, he believes; not measured.** So the probe (11.4a) comes
   first in the perf piece.
7. **The tournament:** after both picks, did the screen stay on the selection, go black, or drop back to the waiting
   room? And which characters were picked? **Ryan: it just dropped back to the waiting room; Gohan and Krillin.** A
   clean lead: the selection's end refused the pair or player 2's pick and returned to the room (Krillin is one of
   our playables, Gohan the stock lead; a pair-table / roster lookup that does not know the pairing, or a check that
   reads player 1's pick twice). The reproduction: F9, `p2 0`, the director, 2 v 2, Gohan and Krillin.
8. **West City's vanishing:** did it happen playing alone through West City too, or only in co-op? **Ryan: only in
   co-op.** Consistent with the sprite / frame budget (11.6).

**State (2026-11-02):** every question answered. The stock ki crumb for played characters: dropped (Ryan did not
answer; reversible). Ryan then handed the build-out to a team under another model: the prompt is
`build/play/HANDOFF-PROMPT-2.txt` (the order, the team, the rules, when `build/play` may be swapped, the report).
