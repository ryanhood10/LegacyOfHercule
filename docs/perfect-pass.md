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
- **After loading a save, the "How many players?" question shows on a black screen with no text box** (the headless runner;
  a power cycle too, so older than the reset fix; the tournament test notes mention the black screen). To check in
  real mGBA: Ryan has not reported it.
- **Gero's lab door (`ch4_piccolo`)**: Hercule's "Let's negotiate!" and Piccolo's "Hercule.  Get behind me." play even
  when neither is on screen (one player as someone else); the scene is staged for Piccolo and Hercule together.
- **Ryan to decide (story piece, 3.1):** in four fights that are the lead's own (the Lookout's first visit, Kame House
  and the island for Piccolo, Gingertown, Semi-Perfect Cell for Vegeta) the lead is made player 1 behind a short fade,
  as Vegeta's Android 19 fight already did; the other way would be the lead as an actor watching his own fight.
- **Unused allocations**: ssj's flags 476-477 and test command 36; map's flags 478-479.

## 10. State at the end of the team round (2026-11-01)

All seven pieces merged into main and pushed; the full regression of the merged main (`build/main-final`, play ROM
`build/main-final/play`) exits 0: 56 blocks, `ssj_hercule` 56/56, `softreset` 12/12, the prologue on the play ROM
fine. Player 2's stock Transformation now changes his own slot (the story piece saw it turn player 1's before the ssj
merge; `gohan_ssj1.txt` checks it). In `build/play` since 2026-11-01 09:08 (ROM identical to the regressed one; drop-ins F1-F9 remade and checked; his save and the old states backed up as `backup/legacy-of-hercule.11010906.sav` and `backup/ss_11010906/`). Open: section 9's items, the
lead-made-player-1 choice, the two-player playtest (section 1).
