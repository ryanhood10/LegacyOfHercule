# Progress log

Newest first. One entry per landed piece or decision; details live in the linked docs.

## 2026-10-31

- **Team combos built and merged** (`docs/tournament.md` 25): the meter (Select fires it), the frozen-fight runner,
  all ten combos tuned and filmed (`build/combos/`), the draw pairing known partners; full regression clean.
- **Team combos: the eight tuned** (branch `tny-combo-tune`, `docs/tournament.md` 25.3): Ryan's pairs now play as he described them: Krillin's disc pins the target for Gohan's Masenko; Piccolo's real Special Beam Cannon (the game's own object, decompiled) holds him while Hercule's Dynamite Kick lands with it and Hercule takes the credit (0 damage); Goku grabs him from behind and the drill goes through both; the father-son Kamehameha side by side (Hercule's pose, no beam); Death Beam volleys and Cooler's Supernova held overhead; Paragus's control device freezes the target for Broly's Eraser Cannon; Mecha Frieza & King Cold's crossed Death Beams; three Juniors swarm him (copies of Cell Jr.'s body drawn from his own tiles) before Cell's Kamehameha. Runner verbs `sbc`, `copy`, held / pinned shots; fighters step round frozen bystanders. Every combo within 4 % of the others' damage at equal stats (`docs/tournament-combos.md` section 2). Test `tools/headless/tny_combo_tune.txt`.
- **Team combos: the runner** (branch `tny-combo-run`, `docs/tournament.md` 25.2): a combo plays inside the game's own command modal (one director command; nothing's update runs), every body held still, every shot and beam in flight frozen by a vtable copy, then the fight goes on with everyone where the animation left him (kept on the ring's floor). Damage runs the game's formula with the attackers' own combat bytes (each partner six blows plus a finisher of six; 4111 in 264 frames for Goku & Vegeta against 947 for both players mashing; 1359 / 7286 at two strengths), only on the target, then the knock-down and the get-up; at HP 0 he falls as usual. Hercule only poses (his weight goes to his partner). The step tables are Ryan's doc `docs/tournament-combos.md` (`tools/tnycombo.py` at build time): the generic barrage and Goku & Vegeta tuned and filmed, eight first drafts. Test `tools/headless/tny_combo_run.txt` (test command 33), also end to end with the meter's Select.
- **Team combos: the meter** (branch `tny-combo-meter`, `docs/tournament.md` 25.1): a shared bar under both HP panels for a listed pair in a co-op fight, full at every fight's start, fired by **Select** on either pad (player 1's R opens the stock area map), a 90 s cooldown, then refilled by 11 shared hits on one opponent and drained when the players split or stop; listed opponent pairs fill an undrawn bar from 10 hits on either player and fire by themselves within 80 px of a player; `tny_caption` (a line at the bottom in a black box, 90 frames, three queued, never over a scene). Hooks: `kit_message` and both players' take-damage. Test `tools/headless/tny_combo_meter.txt` (test command 32).
- **Team combos started (lead):** the contract `src/coop/tny_combo.h`, the pair table (`tny_combo.c`, Ryan's picks),
  `comboram` (0x0203EA00, 0x200 bytes; the heap cap lowered to 0xF87E words), test commands 32 / 33 for the meter
  and the runner, and the draw: known pairs share a 2 v 2 team, a Rumble pack, and the headliner brings his partner
  (Broly with Paragus) (`tools/headless/tny_combo_draw.txt`).
- **Team combos decided** (`docs/tournament.md` 24.10): a meter under the HP panels (full at the start, 90 s
  cooldown, one button fires it), damage as a multiplier of the attackers' moves, listed pairs only (six hero pairs,
  six villain entries), the AI combos back with a looser trigger, Rumble on, known opponent pairs drawn together.
  No tell before an AI combo. Nothing open; it is the next build. Ryan is play-testing the 2026-10-25 build meanwhile.

## 2026-10-25

- **Team combos in scope** (Ryan): two players hitting one opponent fast trigger a frozen-frame combo move by that
  pair; the plan and Ryan's open questions are `docs/tournament.md` 24.10, to detail with him when he resumes.
- **Plan for the open points** (`docs/tournament.md` 24): the Rumble's non-pausing captions, four portraits, Yakon's
  lying pose, the announcer walking off and on, the co-op turn's black, Papaya's landmark, walking speeds, Ryan's three
  dialogue decisions. Ryan play-tests this build first; the next session starts with his notes.
- **Docs wrap-up:** CLAUDE.md's current phase, `docs/tournament.md` status and its open points (24), the full
  regression in the repo (`tools/headless/regfull.sh`, `docs/testing.md` "Full regression"), INDEX entries for
  `tny_hpbars.c`, `regfull.sh`, `ch1_room_forest.txt`.
- **The fighters' dialogue is in (Ryan's approved plan, `docs/tournament-dialogue.md`):** before every 1 v 1 / 2 v 2 /
  challenge / co-op final the other side opens and your fighters answer (rivals, meeting yourself, teams, defaults);
  the winner speaks after the result; Start skips. The Rumble's announcer has the plan's round, headliner, cleared and
  result calls (the calls during a round wait: a dialogue box stops the fight). `tny_talk_hook` in `tny_talk.c`.
- **Tournament dialogue planned (for Ryan's review, not built):** `docs/tournament-dialogue.md`: Budokai-style lines in 1 v 1
  and 2 v 2 (every fighter's own, rival exchanges, meeting yourself, team lines), the announcer only in the Rumble.
- **Polish pass 2: the tournament as scenes (branch `tny-polish2`):** every scene change in the ring happens behind
  a curtain (fade to black, change, fade in): the trip from the waiting room (its menus chain through black, no
  scrambled frame of the map load), the bracket back to a staged ring (players on their side, the opponent walking in
  from off screen; no snap, no pop), the end (bars slide away, the fallen keep lying through the line and the pop-ups,
  they leave in the dark), the co-op turn reloads, the trophy (the winner centred, facing the crowd), the challenger,
  the way out. The Rumble's pack walks in, the fallen blink out; the duel final faces the two players off before it
  starts; the announcer stands where the camera shows him; A no longer punches between fights; overhead HP bars only
  in a fight; the bracket screen's one blank frame is gone; Yamu's scrambled body (4bpp attributes on 8bpp frames,
  Kid Buu's victory pose too) is fixed in `buu2log2.py`. The dialogue seat `tny_talk_hook` (before the call, after
  the result line) is in place, doing nothing yet. Films: `tools/headless/tny_flow.py`. `docs/tournament.md` 23.
- **Sprite leftovers (polish2-art):** Yakon, Broly and Janemba lie on their side (Broly / Janemba turn another
  recoil frame, Yakon, drawn only from the front, a turned frame with the limbs drawn in: `lying.tuck_limbs`); King
  Cold's and Cooler's knock-down (groups 3 / 20) is their recoil then that lying frame, not Cooler's feet-first death;
  so is every import whose anim 17 is no fall (Broly, Janemba, Kid Buu) and Yakon's, Pui Pui's, Evil Buu's; Videl's
  hair column is back; Roshi's floating fists by the head are gone; the Buus' victory pose has no steam cloud; Yamu
  (293 is the only Yamu in Buu's Fury) gets his black hair tuft. `docs/sprites.md` "Sprite leftovers".
- **No mirror matches with a tournament pick (Ryan: picked Yamcha, met Yamcha in round one):** the engine's pool
  (`pool_of`) and the Rumble's headliner skip the players' picks; last tournament's picks are cleared first
  (`tools/headless/tny_nomirror.txt`: three seeds, Yamcha only in player 1's seat).
- **Second pass on Ryan's playtest:** a fighter knocked out mid-fight now lies in the same pose as at the line
  (`tny_lie_pose()` in tny_engine.c, used by tny_moves.c `fallen()`: the lying table first; before, bodies without a
  lying group 12 stood in their old group-12 / knock-down frame, "the flying animation", once the line ended). Krillin's
  dash: the pasted head never sits above that frame's own shoulders (`krillinsprites.py`; the back-view dash had no
  face to find and floated a row above a dipping body); `headcheck.py` on 161: 0 floating frames.
- **Overhead HP bars in the ring (Ryan: "should we display the hp of the other fighters during the royal
  rumble?"):** a small bar over each Rumble fighter's head (and a 2 v 2 partner's), green / yellow / red as it drains;
  1 v 1 keeps the big boss bar (`src/coop/tny_hpbars.c`, `docs/tournament.md` 19).
- **Art fixes from Ryan's playtest (branch `tny-artfix`):** Chi-Chi's head sits over her body, whole, in every facing
  (anchored by the body, frames widened where it stuck out; her tabard in side views and the victory pose); Roshi's head
  anchor and back-view kicks fixed the same way. Every tournament body lies down for real at a match's end (Buu lie-down
  frames, turned hit-stuns via `tools/lying.py`, the generated table `tny_lie`; the loser lies in a side facing).
  Spopovich's animation remapped (full walk, side-view crouch and grab, flex); every Buu import's hit-stun is its hurt
  anim, not its run. Sheets `tools/tnysheet.py`, films `tools/headless/tny_film.py`. `docs/tournament.md` 20.
- **Tournament fight fixes (Ryan's playtest, branch `tny-fightfix`):** tournament opponents get knocked back by
  player combos (the stock set-21 reaction slid them 7-12 px on the first punch and then held them in place: Yamcha
  stayed at 288 px through five more punches; now three punches move him ~90 px, the finisher 40); a fighter at HP 0
  falls, lies still and is out of the fight (he kept fighting at HP 0: Rumble zombies; a Rumble is now finished by real
  hits round by round); Spopovich and Yamu steal ki only from half HP down and turn it into HP (25 ki = 10 % of max HP);
  as players both have Energy Steal (new ability 24: a dash and a grab, 20 % ki back, Spopovich at half HP 8 % HP) and
  new numbers; a tournament playable is built on the party's best numbers (it was player 1's own slot; player 2's pick
  was built on player 1's pick); a thin beam outliving its fighter no longer jumps through his freed kit. Seen, not fixed: Yamu as player 1 in a co-op 1 v 1 draws scrambled (also on main).
  `docs/tournament.md` 21, `tny_fightfix.txt`.
- **Heads stay on (Ryan: Krillin's head popped off in his dash):** `krillinsprites.py` measures the chin per frame (the
  lowest face skin under Goku's hair) instead of reusing the idle frame's, so leaning and crouched frames no longer
  float the head. `tools/headcheck.py` flags detached heads; a dash film of every playable row (22-66) checked by eye.
  The Buu imports' dash group (13) now plays their run (was an arms-up frame). F8 / F9 drop-ins come powered up
  (`dropins.py`: max HP x3, ki and stats x2).
- **Easter eggs:** Yamcha knocked out in the infirmary's last bed; a crack behind the courtyard's flowerbed into a
  hidden arena (Buu's Other World Stadium) where Pikkon gives a hint and a Senzu bean. The waiting room holds seven
  (eight hung the Champion Rumble test). `tny_eggs.txt`, `docs/tournament.md` 17.
- **Crowds and the patient (Ryan):** the waiting room shows up to seven of today's competition (not unlocked, no Roshi),
  the courtyard the unlocked fighters; General Tao named for his cyborg body; an injured man in the infirmary's first
  bed with player 1's line. `tny_crowd.txt`.
- **The grounds' people (Ryan's first look):** Roshi with advice beside the food stall, vendors behind the stalls, the
  reporter in a suit with a cameraman and the ZZTV van (`tools/vanart.py`), a woman fan relabelled, a woman nurse; the
  waiting room shows unlocked challengers and the newest tier first. `docs/tournament.md` 16, `tny_npcs.txt`.
- **The tournament runs as scenes (Ryan):** no free movement or moves in the ring between fights (only A for the
  lines; Start / Select blocked as before); the players stay in the ring from the first match to the trophy.
- **Tournament polish (branch `tny-polish`):** the ring presentation (the announcer, sprite 230 with his portrait,
  at the ring's edge between fights; both sides walk in from the platform's edges through the stock script commands;
  his call; at the end the loser lies down and the winner poses while he calls it, poses held through the line; the
  Rumble's pack enters after he leaves), the hidden fight's alternatives (one rule: the challenger, else the
  tournament's alternative while locked: Champion Rumble -> Majin Vegeta, Legend 1 v 1 -> Goku (Buu saga), Legend
  2 v 2 -> both, Legend Rumble -> Super Buu's final form until beaten once), Android 16 and Super 13 as opponents on
  their own paths with the tier's numbers (Super 13 kneels in the ring, no explosion), the giants' faces (cropped
  heads on the select grid and the unlock board, half size on the bracket and the Rumble board). Test
  `tools/headless/tny_polish.txt`; every `tny_*.txt` and ch9_gero, cell_games_1p, duel_end, a16_moves pass on the
  debug ROM. `docs/tournament.md` 14.
- **Tournament engine (branch `tny-engine`):** `src/coop/tny_engine.c`: the pool (tiers on flags 85 / 136 / 191 / 382,
  confirmed in the scripts; no mirror matches), the director's flow from CoopCall 23 (tier, division, Quick / Full,
  select), random brackets with off-screen simulation, matches in the ring spawned from RAM records on stats
  160 + fighter id (tier-scaled, never twinned), full heal and the 1 HP rule, co-op 1 v 1 turns with a duel final,
  2 v 2 as four teams, the Royal Rumble's rounds and headliners, prizes (+3 capsules, Senzu), an EXP trickle, the
  global record on the save chip (EEPROM 0x1E00), beat-to-unlock, the hidden challengers (Uub / Vegito conditions),
  pop-ups and the story watch (after the first visit). Player 1 changes body only through a ring reload. Test
  `tools/headless/tny_engine.txt`; regression (ch9_gero, cell_games_1p, duel_end, a16_moves) unchanged.
  `docs/tournament.md` 13.
- **Tournament screens (branch `tny-ui`):** `src/coop/tny_ui*.c`: a full-screen modal framework on the save pad's
  switch-screen recipe (stock fades, our `Modal_Run` object, the map's entities and OBJ VRAM allocator set aside
  and restored, a bitmap BG for panels and text, stock Portrait faces, 64 x 64 dialogue portraits as OBJ), the list
  box, the character select (four tabs, pages, black "?" silhouettes, portrait and name; player 2 on the second
  pad), the bracket (Quick / Full with pages; results animated: loser greys, winner slides, quick slides for the
  rest, "GOHAN vs FRIEZA", the champion), the Rumble board, the unlock board (CoopCall 28, 1), the pop-up (a
  narration line in the dialogue box). Test `tools/headless/tny_ui.txt` (every screen, no OBJ VRAM leaked, display
  registers restored); regression identical to main. `docs/tournament.md` 12.
- **Tournament grounds built (branch `tny-grounds`):** Papaya Island as zone 15 areas 11-16, Buu's Fury's zone 7
  imported whole (plaza, street, courtyard, infirmary, waiting room, ring; `data/maps/papaya_*.json`), a world-map
  landing open from the start in the island sea at (1260, 1820) (the plan's spot is desert on our map), both
  save pads on Buu's Capsule Corp pads, the first-visit welcome (flag 441), a crowd of stock townspeople, the
  announcer (art-buu's sprite 230 or a stand-in), the director (CoopCall 23 with a fallback line), the unlock
  board (our art, CoopCall 28, 1), the waiting room's fighters (CoopCall 30, 0; test command 30 forces the Rookie
  pool), the ring sealed for the engine (`src/coop/tny_ring.h`). `mapbuild.py` gained npc records, script
  payloads, trigger activations and an on-load script; `spriteclone.py` emits `sprite_index_count`.
  `docs/tournament.md` 9b, test `tools/headless/tny_grounds.txt`.
- **Tournament playables (branch `tny-playables`):** every `TR_BEAT` / `TR_CHALLENGER` fighter (38) has a display row
  (29-66; limit 80) and a kit: `tny_play_row` / `tny_play_entry` (`src/coop/tny_play.c`: the lead's level, per-fighter
  HP / EP / STR / POW / END percentages, 0x7B, four abilities from 0-23). Player-layout bodies 235-244 for the LoG2
  enemy sets (Yamcha, Tien, Frieza, Mecha Frieza, Tao, Android 13 / 17 / 18, Perfect Cell, Cooler) by
  `tools/tnyplayables.py` (the a16p recipe, arrays padded to Goku's frame counts, `TNY_ART`); Yamcha, Cell and
  Cooler fire the Kamehameha (a group 18). Missing art degrades to Hercule's 72. `p2_bind` rebuilds player 2 when
  the new body has another OBJ VRAM size (a 64 x 64 one overran the old slot's buffer and crashed).
  `tools/headless/tny_play.txt`: all 38 as player 2 and as player 1, walk, punch, four abilities each, no crash;
  regression (ch9_gero, cell_games_1p, duel_end, a16_moves) output identical to main. `docs/tournament.md` 10.
- **Tournament moves (branch `tny-moves`)**: `src/coop/tny_moves.c` gives every opponent and challenger his kit
  (`tny_kits[]`, one row per fighter) from a move library written once (beam, ball, homing, volley, wave, nova, rush,
  teleport, stun, heal, drain, regen, transform, summon) and adds Rumble mode (fighters hurt fighters of other sides,
  70 / 30 target weights, no EXP for NPC kills). Transforms and spawns check the body against the AI set's pose
  groups (Mecha Frieza's sprite 49 crashes under set 21). Every AI set of the roster ran in Rumble packs without a
  crash; tests `tny_moves.txt`, `tny_rumble.txt`; `docs/tournament.md` 10.
- **Tournament art: Buu's Fury imports (branch `tny-art-buu`):** 28 fighters (sprites 195-222: Spopovich to the Buu-saga
  Piccolo, the three forms of Super Buu, Gotenks and Goku) and the announcer (230) converted by `buu2log2.py --spec
  data/tny_buu.txt --fit` into the second ROM blob (about 950 KB), with 29 dialogue portraits (140-168,
  `tools/tnyportraits.py`) in the roster's portrait column. Records checked by eye: Kibito 148, Pikkon 214, Evil Buu 32,
  Goten 114, Kid Trunks 260, Gotenks 116, Vegito 92, Super Buu (Gotenks) 41, the announcer 8 replace the plan's
  guesses. Partial sets mapped by hand; LoG2-sized hurt boxes (Buu's own let 20 of 28 dodge punches); Yakon recentred.
  Test command 8 accepts an EWRAM record. `tools/headless/tny_artbuu.txt` spawns, fights and hit-tests every one;
  regression unchanged (`docs/tournament.md` 4e, `docs/testing.md` "Tournament art").
- **Tournament art-new (branch `tny-art-new`):** King Cold 185, Master Roshi 186, Chi-Chi 187, Videl 188, Chiaotzu 189
  built from existing bodies by `tools/tnysprites.py` (Cooler's set with Cold's head and a cape; Krillin's body with
  Roshi's head, beard and shell; Android 18's set with Chi-Chi's / Videl's heads and outfits; kid Gohan's set with
  Chiaotzu's head), one player-layout record each that also runs on AI set 21, all in the second ROM blob (about
  185 KB); portraits stock (70, 80, 43, 42) plus Videl's 127 from Buu's Fury. Tested headless
  (`tools/headless/tny_artnew.txt`): each fights 10 s on set 21 and falls, the four normal bodies play as player 1.
  Older sprite notes corrected (Roshi is 88, Chiaotzu 32, Chi-Chi 33; Buu 55 Chiaotzu, 56 Chi-Chi, 276 Videl; a
  player record's group 2 is the hit-stun). `docs/sprites.md` "Tournament fighters (art-new)", `docs/tournament.md` 9b.
- **Tournament build started** (Ryan: "deploy a team"): the contract on main (`src/coop/tourney.h`, `tourney_data.c`,
  `tourney_hooks.c`), a 0x400-byte `tourneyram` region (the heap cap now ends at 0x0203EC00: `patch_rom.py` 0xF8FE),
  a second ROM blob for art (`rom2` at 0x08E00000, `patch_rom.py --art`), allocations in `docs/ids.md`; seven
  sessions build the pieces (`docs/tournament.md` 9a). Full regression unchanged on the foundation.
- **Tournament plan closed** (Ryan): no mirror matches; Rookie's pool of 13 (King Cold, Master Roshi, Chi-Chi,
  Videl, Chiaotzu to build; Tao, Spopovich, Yamu as regulars); the unlock board (silhouettes with conditions);
  a global unlock record; Quick (8) / Full (16) at the director; no ring-outs; full heal every round; capsules,
  Senzu and a small EXP trickle; the Rumble's rounds and headliners per tier; the hidden challengers re-dealt.
  `docs/tournament.md` is the plan; nothing built yet.

## 2026-10-24

- **Buu's Fury's map table read (branch `buu-maps`):** its MapEntry table (0x0808E2E0, 452 rows, LoG2's layout) and
  UTF-16 name table (0x0806BCE8) are decoded; `maplib` picks the game from the ROM header (`Game` profiles) and
  `python tools/mapexport.py --buu --list` names every Buu map with its variations' addresses and sizes
  (`build/maps/buu_maps.txt`); `--map Z/A ... --sheet` renders several maps into a labelled contact sheet. The World
  Tournament grounds (zone 7: plaza with or without Bulma's plane, street, courtyard, infirmary, waiting room with
  the save pad, the ring, the ring with the smashed wall) and the Other World Stadium (1/20) are rendered in
  `build/maps/buu/tournament/` with their exits (one chain, 7/5 the hub); Broly's crater is 8/71 "Giant Crater" v2
  and the ship's room 6/6 "Hyperbolic Time Chamber". `docs/maps.md` section 10.
- **Tournament mode planned** (Ryan): `docs/tournament.md` holds the decisions (Papaya Island from Buu's
  Fury, a landing south-west of West City, 1v1 / 2v2 (co-op only) / Royal Rumble as survival rounds, a
  progression track beside the story, tiers, three unlock kinds, the select and bracket screens) and the
  engine facts (hit types, targeting, the OBJ VRAM cap, the save chip's free space). Buu's Fury's map table
  (0x0808E2E0) and name table (0x0806BCE8) were found; a side session builds the lister and renders.

## 2026-10-21

- **The big Saiyan ship with a door (Ryan):** the ship is 256 x 160 map art drawn by code (`tools/shipart.py`), not
  sprites (twice the old 128 x 64 would take all of OBJ VRAM; sprites 171 / 172 freed): the backyard's variation while
  it is parked (zone 15 area 10, `data/maps/yard_ship.json`; `map_variation_hook` on `FUN_0800EB4C`,
  `src/coop/shipyard.c`) and painted into New Planet Vegeta's landing zone (Goku's pod crater scenery removed for the
  tile slots). Its open door and ramp are walkable: Paragus steps out of it, the players walk in to board and walk out
  on arrival and at home (flag 379). The door now works right after Paragus' scene (its trigger had a flag-377
  condition, tested only when the map loads, so it stayed off until the backyard was entered again); his scene no
  longer replays. Paragus and the others ride in the room both ways and get off first; on New Planet Vegeta they wait
  at the ramp's foot for the welcome, then walk ahead to the basin. Krillin, Yamcha and Puar (the wait's backyard
  NPCs) moved off the ship's spot. `mapbuild.py`: `png` `"draw"`, `art_footprint`. `broly_ride.txt` rewritten.
- **Android 16 as a player (Ryan):** he was stuck on Ki Blast. The L cycle (0x08004284) moves to the next
  ability whose "learned" bit (flags bits 3-6, one per ability_list index) is set, and his flags were 0x43
  (only Ki Blast's). Now 0x7B: Rocket Punch, Afterimage Rush, Big Bang and Ki Blast all cycle and fire for
  either player. A little weaker as a player (the boss is untouched): HP 160 -> 140 %, strength and endurance
  150 -> 130 %, power 110 -> 100 % of Krillin's, growth row 7 lowered (360-560 / 280-480 / 360-560). Saves
  from before get both once (`a16_frame`). Slot 7's status page now has a technique icon pair (a rocket
  fist, the ki ball): a slot with technique bits and no pair crashes the stats page. `a16_moves.txt`.
- **Drop-ins** are saved 300 frames after their warp (F3-F5 and F7 were saved mid-load; F7 stayed on the
  Map Test spot in mGBA).

## 2026-10-20

- **Duel endings (Ryan):** a duel's end refills both players' HP and ki and shows "Player N wins the duel!" before
  the duel's own ending (the dojo's sparring match keeps its "X wins the sparring match!", `DUEL_OWN_WINNER`). The
  final blow already never ended the game. `duel_end.txt`.
- **The Broly ship ride (Ryan):** the ship doubled (128 x 64 as two 64 x 64 halves, sprites 171 / 172; 8bpp attr bit
  set, the stock collision box kept so the walks into it still finish); the players board through its door, the
  flight shakes a new interior (zone 15 area 9, Buu's Fury's round room without its Capsule Corp pad) and lands,
  its door leads out to New Planet Vegeta; the ride home after Broly; the ship stays parked. `broly_ride.txt`.
- **Ryan's F4 batch:** tougher Cells and Juniors (`hp_pct` / `speed_pct` boss rules), six Juniors with two each for
  Vegeta and Trunks, fight 8 drawn to 0, the beam struggle restaged (helpers behind, beams capped at the clash,
  the second Cell explodes, Hercule stays), Super Saiyan 2 Gohan on the Super Saiyan 2 class, Roshi's dojo tougher
  by map with "a special treat", and the drop-ins with the earlier story's flags plus an F7 for the Broly chapter.
- **Android 16 selectable (Ryan's F3 test):** after the workshop scene 16 stays in the party and selectable on the
  save pads (the players trade Krillin and 16) through the rest of the wait; `CoopCall 12, 1` takes him out at the
  Cell Games' start. Alone, player 1 is 16 after the scene, Krillin an actor by the table. `ch9_gero.txt` checks
  slot 7 after the chapter.
- **Super 13 (Ryan):** 14000 HP (9800 alone, was 5500) and a five-beat transformation (surges, his line, the
  shaking mountain, the white-out reveal with his music, a new boast). **Kill EXP to the last hit:** the damage
  calculation notes the player behind each hit (a ki blast's owner too); kills by ki blasts no longer go to the
  player the enemy targeted. `exp_credit.txt`.
- **Keep Goku restored** (Ryan's F6 test: no haloed Goku to pick): the Cell Games' restated end script (entry 53)
  ran the stock `ClearCharacterInPartyFlag 4`, bypassing the old byte patch at 0x3EC738, so Goku left the party
  after the Games. The generator now keeps him (`SetCharacterInPartyFlag 4`); he shows with his halo on the save
  pad's Switch Characters screen. Both Cell Games tests check his flags (0x0203F090 = 7f).
- **The summit (Ryan's F5 test):** four breakable rocks (`mapbuild.py` `rocks`: stock decoration + item, the large
  rock's drop) and, after Super 13, the fade back to Dende's Lookout where the finale plays by itself. The drop-ins'
  F5 / F6 now come from the Cell Games played through (Goku's halo, Super Saiyan 2 Gohan).
- **Ryan's drop-in play-test fixes:** old Hercule travels with Trunks through Goku's landing (behind him, with
  the group, hanging back at the crater's rim for his "Psst!", back with everyone); the workshop shows 16 on
  Briefs' table until he wakes, then a fade to an empty table with 16 standing beside it (the stock on-load
  shows the body decorations only before flag 420; no standing double); the Cell Games ring lets only Goku
  through, at any level; the camera follows Hercule out and pans to Cell; Gohan flies into the ring and can go
  Super Saiyan 1 (teen row 2 from the Games' start).
- **Gohan walks and flies in for his scenes** (Ryan: he appeared out of thin air): the pop-in stand-in is gone.
  When nobody plays Gohan, `ch1_opening` stages him as an actor (sprite 55) guarded by the new CoopCall 22: he
  walks up with the Z fighters at Frieza's landing, stands with them on the return, flies in with the group to
  meet Trunks, walks up to Trunks for "how do you know my dad?" and kneels for Goku's landing; "Hey dad!" names
  the lead. With a player on Gohan (alone or as player 2) there is no actor. `scene_actor.txt`; the runner's
  `entrec`.
- **Cutscenes cleaned up** (Ryan: the swap made scenes jumpy; the lead should only walk in for cutscenes): no
  more swapping and no follower in free play. Speaker-1 lines show the story's lead's portrait when someone
  else plays (the portrait getter re-implemented); alone, the lead first popped in as player 2 for the scenes
  that give him lines (replaced the same day by the scene actor above). The summit's per-character lines got their own speaker ids.
- **Older saves:** no lead noted yet reads Gohan (Ryan's save from before the lead was noted showed no Gohan and
  Krillin's portrait on Gohan's line); a downed player 1 whose player 2 leaves for a companion is a game over.
- **Scenes alone as someone else** (Ryan: Gohan missing from Frieza's landing while playing Krillin, and Krillin
  saying "how does he know my father?"): a scene now runs with the story's lead as player 1 and your character
  beside him, then puts you back (`companion.c`, the sequence start / end re-implemented). With two players the same:
  a player 2 on the lead trades for the scene, another player 2 stays. `scene_swap.txt`, `scene_swap_2p.txt`.
- **Ryan's test, round 3:** alone, the story's lead follows as a companion when you play someone else (Gohan with a
  player who switched to Krillin; the lead is noted by wrapping the VM's SetActiveCharacter steps, flags
  464-467); the companion never counts as a player for the rules. Frieza's landing (`ch1_frieza_landing`) has
  old Hercule with Future Trunks: he arrives with him, King Cold and Krillin do not know him ("who's that old guy
  with the afro?"), he knocks out one of Frieza's soldiers himself, catches up at the landing site ("I can't
  fly, remember?") and leaves with Trunks.
- **Ryan's test, round 2:** the sprint bumped on open ground (our 4-way walk port passed no box to the collision
  check; the sprint is the only user of that state); old Hercule now walks with Trunks in the prologue alone
  (`companion.c`: player 2 with a follower pad, HUD hidden) and talks with Buu's Fury's Hercule portrait (126) in
  the prologue, at Future Trunks' visit and in the epilogue. Our notes' state table had the sprint and the walk
  states mixed up; corrected.
- **Scene-ending sweep** (Ryan: find more like the chair lock): every scene we edited compared with the stock steps
  it cut (a reviewer expanded every `stock` / `stockseq` line). Fixed: the forest spirit fight kept the camera
  panned to the clearing's top (now `SetActiveEntity 0` after the fight starts); the wishes left Gohan as player
  1 (stock entry 0's `SetActiveCharacter 0`): the party and players are kept with CoopCall 10 and restored after
  the scene. Everything else read clean; two play testers run the edited scenes headless.
- **1 Player / 2 Players at every game start** (Ryan): after Select Game, the first moment player 1 can move,
  "How many players?" (A one, B two); one player keeps player 2 out even with a second controller connected.
  `src/coop/players.c`, CoopCall 21; the runner gained `stepnz`. Tested on the play ROM both ways.
- **Ryan's first test (alone), chapter 1:** after Krillin's visit Gohan was stuck in his chair (the stock "sits down"
  step puts him inside the desk's collision, and the stock dream used to warp him out); now he gets up and walks out
  with Krillin. After the forest spirit the two stay in the clearing: behind the white fade the scene moves to the
  real clearing (zone 2 area 3, the same layout as the fight's area 8, which has no way out) instead of the cave
  mouth. Test: `tools/headless/ch1_room_forest.txt`.
- **Final pass before the first playthrough** (Ryan: "clean it up with the team"): four reviewers (balance,
  bugs in the new mechanics, softlocks / save traps, docs) and the fixes: the Broly arena's save pad removed
  (Ryan; the boarding question is gone again); balance alone and together (`docs/coop-design.md` "One player":
  pairs 50 % alone, Android 16 45 % alone, lone bosses 180 % in co-op, difficulty numbers alone, halving the
  players' damage rejected); seven revive / drop-out fixes; the Time Chamber door's rect; Broly's lines about the
  pad; docs cleaned (INDEX, ids CoopCall table, story status, ch8_arctic moved to `data/archive/`), the p2 input
  script's fallback address, `coop_state.gdb`, README; `PLAYING.md`, a guide for the players.
- **Polish pass** (Ryan: "a full pass and polish"): two read-only audits (every line of our dialogue; every
  story flag, gate, star and journal condition). Fixed: the Broly arena's save pad is gone (Ryan: the arena has
  no way out until Broly falls, so a save there could trap a weak party); the world map's arena star / marker and the journal's "Enter the Cell Games" wait for
  the Gero chapter (421) like the landing; the Lookout's star is off during the Arctic quest; a journal line
  points to Dr. Briefs' workshop at the wait's start (50 journal quests); dialogue: 18 was never dead (the
  summit), 13's "Remember me?" to Piccolo, Krillin had not seen 13, the second Cell's "Twice the perfection!"
  is his and not a double taunt, US spellings, punctuation and names; pages over 90 characters split (a box
  holds four lines, about 100 characters); the docs on the flags a new game clears (400-503). Ryan's own
  dictated lines are unchanged. Tests: the whole suite in both modes.
- **One player and drop-in / drop-out** (Ryan: everything must work alone and together; story pairs stay alone):
  the Cell Games' player-2 fights play on player 1's pad alone with "<character> fights!" cards; player 2's
  entity leaves when its controller goes away and returns with it; story pairs at 70 % HP and two bars alone;
  the summit's partner line, the dojo mat, the gym's team lines and the skipped duels read right for one
  player; CoopCall 14 op 2 no longer reads past the name table. `docs/coop-design.md` "One player".
- **Revive and difficulty** (Ryan): a player at 0 HP goes down instead of dying; the partner holds A beside them
  for 3 s (a yellow bar, the body glowing) to get them up with 30 % HP; a red countdown with the seconds runs
  10 s on Normal, and running out (or both down) is a game over (`src/coop/revive.c`, `docs/coop-design.md`
  "Revive"). Easy / Normal / Hard is asked at a new game's first scene (flags 460 / 461) and changes enemy HP,
  damage, speed, twins, boss HP, the revive time and drop rolls (`src/coop/difficulty.c`, "Difficulty").
  Tested headless (`revive.txt`, `difficulty.txt`, and the story tests). Test command 10 `DOWN`.
- **Pre-play check fixes** (Ryan): the journal gets our chapters (`tools/gen_journal.py`, `docs/scripts.md` 2.6:
  Gero's computer, Broly's ship and trip, the Arctic, back to Dende, Dende's question; "Talk to Dende" ends with the
  wishes); Super 13's fight plays a boss track (music 30, Broly's) and the summit's own music after; Broly's
  lines that assumed the Games were ahead (Vegeta's "little tournament", Trunks' "only days away", "Cell won't be
  any easier", Piccolo's "days we have left") now fit before or after them. No refills between the Cell Games
  fights (Ryan: harder is better): the arena's rubble already drops HP / EP pickups (`083EE6A8`, items on maps
  324-326). The coop ELF relinks when the Makefile changes.

## 2026-10-19

- **The twin-Cell "stall" fixed** (`twin-cell-stall`): it was a crash, and it happened in real play. Player 2 holding A at
  the end of a punch read player 1's special melee page, so Hercule (no pose group 16) played a null frame whose
  garbage size DMA'd over both players and the HUD, and the next draw jumped through a zeroed vtable. `wrap.c` now
  reimplements the attack end (`0x080085AD`) with each player's own slot and only for sprites with group 16
  (`docs/coop-design.md` "The twin-Cell stall"). The Makefile's lost `ssj2_sprites_data.o` recipe is back (a clean build
  failed to link). New: `tools/headless/p2_special.txt`, and the runner's `watch` / `breakpc` / `trace`. The twin Cells
  die in 37-49 s in 6 of 6 autopilot runs; `cell_games.txt` passes.
- **Super 13 merged and in the summit** (`super13-boss`): the summit scene swaps 13 for Super 13 (sprite 180) after the
  absorptions, his line uses portrait 122, and the fight is record 11 with stat 145. The win-stat test hook now also
  matches Android 16 and Super 13 (their own vtables), and the Makefile regenerates the portrait table when the
  extra-portrait list changes (`$(BUILD)/gen/portraits.stamp`; a stale table showed garbage for portrait 122).
  `arctic_summit.txt`, `cell_games.txt` and `postgame.txt` pass headless.
- **The post-game on the Lookout built** (`data/story/postgame`, generated by `tools/gen_postgame.py`; `docs/story.md` 4i):
  Yamcha's wish, Hercule's tease, the explosion and 18 leaving (the Arctic opens), Dende's reminder, the finale, the
  Buu's Fury question (CoopCall 18, a yes / no question in a text box), the talk show, credits and the new epilogue with
  old Hercule. Merged on the way: `arctic-route` (the arctic climb, ridge pad, summit, world-map landing) and
  `ssj2-gohan` (Super Saiyan 2 Gohan, party slot 8, `PARTY_SLOTS=9`), who joins at the end of the Cell Games.
  Tested headless (`tools/headless/postgame.txt`). The Arctic summit is built (`data/story/arctic_summit`,
  `tools/headless/arctic_summit.txt`).
- **The Cell Games built** (`data/story/cell_games`, generated by `tools/gen_cell_games.py`; `docs/story.md` 4h):
  solo fights with title cards (`src/coop/cellgames.c`, CoopCall 15), Goku as player 1 then on player 2's pad,
  Hercule subbed in, Gohan + Hercule, four Cell Juniors fought by Krillin / Piccolo / Vegeta / Trunks
  alternating pads, Hercule's retreat and return, 16's bird line, two Perfect Cells (stat 140, sprite 162, a
  record copy of 30), the two-on-two beam struggle with held beams (CoopCall 16), the ending without Hercule's
  join and with Goku's halo, the ring barrier for anyone at level 30 (Goku only, any level, since 2026-10-20), the arena gated on Gero's computer.
  Fixed on the way: the co-op leash kept players 20 px under a map's top edge, so top-edge doors (the ring,
  Gero's lab area 47) could not be reached in co-op. Tested end to end headless (`tools/headless/cell_games.txt`,
  new runner command `stepz`, test command `COOP_CMD_WIN_STAT`).
- **The arctic route** (branch `arctic-route`, worktree `F:\MeltyCreations\log2-arctic`; `docs/story.md` 4i,
  maps and access only): the arctic landing (zone 15 area 1) lost its Vegeta barrier and the old
  Android 13 encounter's sources; its cave is now a door up the mountain and its flight spot sits on a
  grass patch (Snowy Highlands' landing art). New maps from Snowy Highlands pieces: two climbs (15 / 5,
  15 / 6; wolves, lions, birds, bandits, tiger bandits copied from the stock records with their item drop),
  the ridge with the save pad (15 / 7), and the summit (15 / 8), a 720 x 592 clearing for Super 13's battle
  with sideline room (spots in `docs/maps.md` 9a). The world map has a new landing on the snowy island in
  the far north (`data/story/arctic_route.*`), there only while flag 381 is set, with the minimap star while
  382 is clear (both flags are the post-game chapter's to set: the explosion, Super 13 beaten). Every arctic
  map is named "The Arctic": `ui_strings` gained a place-name block (id 356, `tools/krillinsprites.py`
  `PLACE_NAMES`), which the save menu now reads too (`party_save.c`). Tools: `mapbuild.py` fills with
  `"anchor": "map"`, stamps with `"recolor": false`, enemies with `"action": "like"`; the headless runner's
  `reset`; `tools/headless/arctic_route.txt` and `objfree.py`. Verified headless (`docs/testing.md` "Arctic
  route"): no landing without 381; star and plain marker; landing, every exit both ways, twins on both
  climbs, OBJ VRAM >= 15.6 KB free, a save on the ridge pad and a Continue back to it, both flight spots.
  Not verified: the world-map name label (not seen for stock sites either), the wide view (the WIDE=1
  build builds, not run), real play and the balance of the climbs.

- **Super Saiyan 2 Gohan as his own party member** (branch `ssj2-gohan`, worktree
  `F:\MeltyCreations\log2-ssj2`; `docs/hercule.md` 5d, `docs/party-8-slots.md`): `PARTY_SLOTS=9` is the
  default, slot 8 empty until `ssj2_join()` (CoopCall 20, proposed for the Cell Games chapter's end, then
  the message line) writes him at Gohan's level with Gohan's HP / EP x1.5 and stats x1.7, growth row 8 about
  twice Gohan's, abilities Kamehameha / Masenko / **Super Saiyan 2 Burst (23)** / Ki Blast. Display row 28
  (Gohan's row 3 with sprite 58 and the stock portraits 61 / 12), sprite 58 repointed to `ssj2_record` (hurt,
  lying, tumble from 59), status title "SSJ2 GOHAN" with Gohan's technique pages. Slot 7 stays the duel
  clone's (`DUEL_SLOT` fixed). Eight-slot saves migrate to the nine-slot layout on the first Select Game.
  The story-flag bank moved to 0x0203F108 (headless `FLAGS` variable; `ch9_gero.txt` and `peek.gdb`
  updated). Fixed on the way, all also on main: the pause menu's tab strip read past its image tables
  with 7+ members (garbage beside Gohan's title; with 8, Android 16's chapter included, a crash stepping
  onto Gohan): `status_tabs_hook`; player 2's beam abilities followed player 1's B (Kamehameha never
  fired, Masenko charged forever) and a Select switch in that state crashed: `wrap.c` hooks and a guard in
  `p2_cycle`. Verified headless (join through a script, both players, switch and status screens with 8
  members, save / reload, old-save migration, a clone duel, Gero's computer, haloed Goku, old Hercule)
  and in fork slot 9 (both players' walk, combo and abilities).
- **Super Android 13, the post-game boss** (branch `super13-boss`, worktree `F:\MeltyCreations\log2-s13`;
  `docs/story.md` 4i; ready for the post-game chapter to place): sprite **180** (`tools/super13sprites.py`:
  Buu's Fury's 64 px Legendary Super Saiyan Broly recoloured per body part, per frame: blue skin, white hair,
  red-brown trousers, bare chest and forearms, black boots; his special poses as groups 21-25, the death in
  12; one wide hurt box), red-pink ki orbs (16 / 32 / 64 px), dialogue portrait **122** (Android 16's made
  Super 13); stat **145** and `src/coop/super13.c`: ki volley (alternating players), ki fan, deadly bomber
  (a giant orb over his head, slow and homing), ki wave (a slow beam head with a ki trail), ki nova (a shell
  blast when crowded), a once-only rage roar, combinations, partly homing orbs, poise, 55 % walking speed,
  and an explosion on defeat (96 frames, then gone; `CoopCall 17, 1` -> 3). Test commands **17** (spawn) and
  **18** (force a special, set his HP, toggle the autopilot); CoopCall **17** (tune, state, spawn). Measured
  headless (`tools/headless/bossfight.py`, `super13.txt`) against the twin Perfect Cells with a Cell Games-level
  autopilot pair: the pair stun-locks the stock Cells (22-31 s, at full HP) but narrowly loses to him (KO with
  him at 4-35 % in 8 runs; with HP refills it kills him in 58-88 s): 10000 HP, 48 / 64 / 56. No lag; OBJ VRAM
  never ran short. Seen in the wide fork (slot 7) against both players. Found: twin-Cell autopilot runs stall
  (entities stop, the frame counter runs) on main too (`docs/coop-design.md` "Super Android 13").
- **Sprite frame counts fixed for every generator** (branch `sprite-frame-counts`): `tools/sprites.py`
  `Sprites.frame_counts` now counts the run of valid descriptors (OBJ size, ROM blob of kind 0-2) up to
  the next array start of any stock record, replacing the gap guess that cut each record's
  highest-addressed array to 1 frame (100 stock arrays; `sprites.py --frame-diff` lists them);
  `halosprites.py` uses it instead of its own copy. Regenerated records: Super Saiyan Hercule 157
  (group 20 facing right 1 -> 4), Paragus 170 (group 20 facing right 1 -> 4), Android 13 165 (group 12
  facing right 1 -> 4), the Namekian 166 (group 22 facing right 1 -> 2); every other generator's output is
  byte-identical. Seen headless (`tools/headless/ssj_tumble.txt`, `docs/testing.md` "Frame counts"): on
  main, Paragus' tumble facing right overwrote the party block and jumped to an invalid address (SSJ
  Hercule's overflow happened to land on his facing-left array: wrong frames, no crash); with the fix all
  four records play every frame with the party block and entity list intact.
- **Haloed Goku** (branch `hercule-beam`), for after the Cell Games (art and rows only; the swap belongs
  to the chapter): `tools/halosprites.py` builds Goku (176) and Super Saiyan Goku (177) with the dead
  pose's halo on every frame (white on gold hair), display rows 26 / 27 linked like 19 / 20 (row limit
  now 28), `SetCharacterTransformation 4, 176` makes him dead. Buu's Fury has no haloed Goku (halos only
  on Bubbles and Gregory). Found and fixed on the way: `sprites.frame_counts` truncates some arrays
  (Super Saiyan Goku's group 21, the detransform, is 4 frames, not 1); the copy crashed the game on
  detransform until frames were counted by descriptor runs. The same truncation sits in the last
  array of SSJ Hercule, Paragus, the Namekian and Android 13 (follow-up). Seen headless: walk, attack,
  hit-stun, transform and back (`tools/headless/halo_goku.txt`).
- **Hercule's Kamehameha pose** (branch `hercule-beam`, worktree `F:\MeltyCreations\log2-beam`), for the
  Cell Games beam struggle (art only; the chapter is not built): `tools/herculesprites.py` builds
  `herc_record`, Hercule's record 72 with group 18 (the pose `NPCFireKamehameha` plays: charge, thrust,
  hold, 4 facings, Goku's 7-frame layout) from his own punch stance and idle frames with Goku's beam
  arms in his sleeve and skin colours; id 72 is repointed in the relocated sprite index, so every user
  of sprite 72 has the pose (no new id). Decompiled the opcode (state 30, the beam's per-facing
  offsets). Seen headless: the stock struggle with Hercule as player 1 (clash and Kamehameha, beam
  from his palms) beside Gohan's run, right / down / up facings, player 2 in the pose in all four
  facings, walking / punching / a sprite-72 actor unchanged. Found on the way: player 2 Hercule's
  second A press on Map Test 323 crashes with the stock record too (`docs/testing.md`). Details:
  `docs/sprites.md` "Hercule's beam pose".
- **Docs pass for faster agents:** new `docs/ids.md` (every allocated flag, stat, sprite, row, portrait,
  ability, CoopCall, test command, zone-15 area and RAM region, with the free ranges) and `docs/howto.md`
  (recipes: chapter, CoopCall, boss, sprite, portrait, playable character, map, test, parallel sessions);
  `CLAUDE.md` reorganised around a task-routing table; stale statements fixed across the docs;
  `docs/story.md` split into the current plan, `docs/story-rom.md` (stock ROM facts) and
  `docs/story-archive.md` (superseded options).
- **The ten-day wait merged into main** (8ef947c lookout-training, 8ae34a6 broly-chapter, 10f7e98
  gero-computer, e45c765): each branch's tests re-run on the merged build and passed. Merge fixes: the
  Broly calls to `coop_run_sequence` use the waves branch's two-argument form; Paragus' dialogue portrait
  and Android 16's status portrait both took id 120, so 16's moved to 121 and `krillinsprites.py` now
  asserts portrait ids are unique. The three entries below describe the branches as built.
- **Ten-day wait: Time Chamber training waves and Roshi's dojo sparring** (branch `lookout-training`,
  worktree `F:\MeltyCreations\log2-lookout`): `src/coop/waves.c` (ten fixed rounds of stock enemies,
  spawned on a ring around the players, the next round when none stands, state in `.party.wv` (0x0203F880 after the merge),
  no coop2 bytes) behind `CoopCall 13` / `14`; `data/story/wait_chamber` (the Lookout's chamber door opens
  from flag 157, Mr. Popo beside it with his line, "Round N" captions, "Training completed!"; the
  chamber's door now returns to the chamber room) and `data/story/wait_spar` (Roshi's offer after flag
  214, flag 440, the mat starts a players-only duel, winner by name, healed, back to the atrium).
  Measured: OBJ VRAM >= 6.9 KB free in every round (two Destroyer pairs had left 2.2 KB, so they went),
  at most 16 lag frames in 300. Tested headless end to end, round 3 in the fork. Per-round table and
  reward ideas: `docs/story.md` 4g; recipe: `docs/testing.md` "Time Chamber waves".
- **Broly side chapter of the ten-day wait** (branch `broly-chapter`, worktree `F:\MeltyCreations\log2-broly`,
  `data/story/broly`): Bulma walks in after Cell's broadcast with the news of a Saiyan ship (flag 400);
  in Capsule Corp's backyard, with Vegeta as a player, Paragus steps out of the ship and offers New
  Planet Vegeta, the trip's five board, the ship lifts off (flag 401; the party is kept in flags
  404-419); New Planet Vegeta = zone 15 area 3 with a new landing zone (the ship, a save pad, any of the
  five selectable); crossing the basin's rim brings everyone on screen, Paragus' betrayal, Broly
  (sprite 159) attacks; players 1 and 2 fight Broly (stat 115) + Paragus (stat 116, sprite 170) as a
  `BOSS_PAIR`; Paragus falling first makes Broly rage once (new `BOSS_RAGE`: full HP, +10 attack, +6
  defence, chase x1.3, a rage dialogue); both down: the planet explodes, a Capsule Corp debrief, fade,
  the party as before (flag 402). New art from `tools/brolysprites.py`: Paragus (Vegeta's 21-group set
  with grey hair and goatee), his dialogue portrait (id 120), the ship (Buu's Fury record 1, sprite
  171). Fixes on the way: `buu2log2` records now carry their real frame size (Broly's 64x64 frames had
  the template's 0x400 VRAM slot), a new game clears story flags 400-479. Tested headless (every beat,
  co-op and single-player, the rage numbers, a natural autopilot win) and the fight in fork slot 6.
  Details: `docs/story.md` 4g, `docs/scripts.md` sections 2.2b / 2.5 / 5, `docs/sprites.md` "Paragus and
  the Saiyan ship", `docs/maps.md` section 9, `docs/testing.md` "Broly chapter".
- **"Gero's computer" side chapter (branch `gero-computer`, worktree `F:\MeltyCreations\log2-gero`)**:
  Android 16 playable in the last party slot for the chapter (sprite 175 from sprite 8 and the boss
  record's groups, row 25, status portrait 121 (moved from 120 at the merge: Paragus took 120), Rocket Punch ability 22, `src/coop/a16player.c`, CoopCall
  11 / 12); `data/story/ch9_gero`: the workshop scene (16 repaired, the computer's signal, Krillin goes
  along, party Krillin + 16), the world-map star (actions 101 / 102 are the plain / star markers; opcode
  0x05 is OR), the generators under the waterfalls, the rubble 16 punches through, the basement door to
  the computer room, two Android 19s in the bunker (stats 120 / 121, `BOSS_PAIR`), the Android 13 file,
  the birds, home as Krillin + Hercule. Stock fix: area 47's top exit never fired. Headless end to end
  (`tools/headless/ch9_gero.txt`), bunker fight and Rocket Punch in the fork. IDs: flags 420-437,
  CoopCall 11-12, COOP_CMD 11, stats 120-121, sprite 175, row 25, ability 22, portrait 120 (121 after the merge); no CoopExtra
  bytes. Details: `docs/story.md` 4g, `docs/scripts.md`, `docs/maps.md` 9b, `docs/sprites.md`,
  `docs/coop-design.md` "Gero's bunker", `docs/testing.md` "Gero's computer".

## 2026-10-18

- **Boss models** (branch `boss-models`, worktree `F:\MeltyCreations\log2-bm`): Android 13 base form
  (sprite 165) and Cooler's Namekian servant (166) from `tools/bossmodels.py`; stats 110 (13 beside
  19), 112 (13 beside Gero), 113 (Namekian; 111 is main's forest-spirit Cooler); rules give them to the co-op twins of 19, Gero and Cooler.
  Tested in the fork with both players (spawn, autopilot fights, both-down in both orders, defeat
  scripts). Fixed: player 2 froze while player 1 was in the hurt state; under both-down the survivor
  keeps its model (Gero's script had despawned the wrong Gero). King Cold's sprite crashes as Cooler's
  twin because record 74 lacks the attack groups set 21 plays. Fork: per-slot helper dirs and process
  names, pause waits for the stop (no more gdb disconnects). Details: `docs/coop-design.md` "Android
  13, the Namekian servant and the boss-pair tests", `docs/sprites.md`, `docs/testing.md` "Boss pairs".
- **Chapter 8, Android 13 in the arctic (story-scripts)**: `data/story/ch8_arctic` through
  `data/maps/arctic.json`: `tools/mapbuild.py` now takes story content for built maps (`"sources"`,
  dialogue triggers, actor / enemy records with seq:NAME actions; `docs/maps.md`), since mapedit runs on
  the stock table before zone 15 exists. 13 meets Vegeta at the cave mouth behind the barrier, a
  half-health fight (new stat 114 and rule in `bosses.c`, no twin until Ryan picks his partner), he
  flies off, flag 376 for the post-Cell variant. Tested headless to the end of the fight.
- **Chapter 8, Cooler (story-scripts)**: `data/story/ch8_cooler`: in co-op Cooler introduces his Namekian
  servant before the fight (one co-op check after the three stock openings); both Piccolo gates stay
  and are reachable in the wait (flag 60 from chapter 2, Piccolo selectable after chapter 7). Tested
  headless: the Piccolo opening, the servant's lines, Cooler + Namekian with two bars.
- **Chapter 8, Roshi's gym (story-scripts)**: `data/story/ch8_gym`: the duos Yamcha + Tien and Krillin +
  Hercule (`BOSS_PAIR` rules on the sparring stats 106 / 98 / 64, only used in that room), the "come
  back with a different team" refusal when a player is Hercule or Krillin (new `CoopCall 8`, P2Slot),
  and the animal-ninja finale in three rounds (eleven kinds at once ran out of OBJ tiles; flags
  363-375). The bestiary table at 0x086F8E40 ({name, description, sprite, portrait, stat}, 128
  entries) gives every enemy's sprite and stat. Tested headless: both duos (refusal with player 2 as
  Hercule, the fight with Gohan; Hercule's model as Krillin's twin does not crash), the finale to the
  capsule and flag 214.
- **Chapter 7 (story-scripts)**: `data/story/ch7_cell`: Vegeta + Trunks out of the chamber, Semi-Perfect Cell's
  Multi-Form duplicate in co-op and its reabsorption, Krillin flies in after Perfect Cell's kick and
  takes the pad that played Vegeta (new `CoopCall 7`, PadSwap: the pads trade players, so the Trunks
  player is player 1 from here), Krillin's one-hit KO before Cell's tournament speech, then Trunks +
  Krillin with Vegeta, Piccolo and Hercule selectable. Tested headless in co-op from the Semi-Perfect
  scene to the journal (flag 151) and the single-player Semi-Perfect path. Test note: a Map Test
  Trunks is base form 128, which has no transformation pose 35, so the stock "transform unless
  transformed" step hangs; the real flow has him at 129 from the chamber exit.
- **Chapter 6 before the chamber (story-scripts)**: `data/story/ch6_basement`: Gingertown's end locks
  Trunks + Krillin (player 2 Krillin); the lab basement scenes skip Krillin's NPC when player 2 is
  there (single-player keeps the stock walk); the lab exit no longer makes Piccolo selectable (a scene
  actor at Kame House); Krillin stays home when Goku leaves; the chamber's end makes Piccolo, Hercule
  and Krillin selectable for the island. Tested headless in co-op: basement entry, larva, computer,
  lab exit, Kame House to the beach (flag 143, no player 2), Gingertown's end (player 2 Krillin).
- **Chapter 5 (story-scripts)**: `data/story/ch5_lookout`: Hercule stays at the Lookout when Kami defers,
  Gohan is player 2 in Trunks' chapter, Kami draws out Hercule's potential before the fusion (+3
  levels and a refill; the amount is ours, Ryan's note said "level + EXP"), Piccolo and Hercule go to
  Gingertown, Cell's Multi-Form lines and the reabsorption of the fallen body. Engine fixes found
  there: after a warp without entities player 2's entity is gone until player 1 is free, so
  `CoopCall 5` (P2Present) now also counts the pad, `CoopCall 6` (P2Bind) without an entity sets the
  slot the respawn takes, and the boss-bar hook picks the co-op bars by the pad too (Gingertown's bar
  had been the single-player one, under player 2's HUD, with no twin bar). Tested headless in co-op
  (both fall orders at Gingertown) through to each chapter end.
- **Chapter 4, Piccolo's chapter (story-scripts)**: `data/story/ch4_piccolo`: 13 backs Gero up in all three
  stages and flies off after each; Hercule's "negotiate" at the lab door; 13 watches 17 and 18 turn
  on Gero in the lab and walks out promising to fulfil his destiny; the road: Vegeta arrives and is
  player 2 for 18 + 17 in co-op, Hercule again for the stock ending. Engine fix (`enemies.c`): a
  co-op boss bar whose boss went down first now slides out once no boss stands (the twin's empty bar
  stayed on screen after every pair fight). Tested headless: Gero's stage 1, both generators in both
  orders, the lab, the road in co-op (through to the world map, flag 127, Piccolo / Hercule) and
  single-player.
- **CoopCall moved to game action 59 (story-scripts)**: action 130 (`sub_0800AFD4`) was not unused: nine
  stock on-load scripts (zones 2, 7, 8, 10, 12, 13) call it to add a scene object, and the repoint
  had them pop an empty stack. 59 is `Deprecated_CharacterSpecialAttack` (an assert, no stock caller;
  the unused list is in `docs/scripts.md` 2.2b). Space duel retested headless.
- **Chapter 3, zone 7 (story-scripts)**: `data/story/ch3_zone7`: Android 13 (sprite 165) in the zone 7
  cutaway with 19 and 20 and two lines, beside 19 in the fight (stat 5 and Gero's stat 7 are now
  `BOSS_PAIR`, so 13 is there in single-player too), the defeat (19 explodes, 13 swears revenge, "not
  even we know what the old man's true plan is", flies off), stock scene after; the three-year skip
  raises Hercule and Krillin one level with a refill. Tested headless in co-op (both fall orders) and
  single-player, through to area 16 with Piccolo / Hercule.
- **Script -> C bridge and the duels (story-scripts)**: game action 130 is `CoopCall` (`vmcall.c`: duel
  start / clear / winner, Hercule's awakening, player 2 present / bind); lscript arguments `seq:NAME`;
  dialogue sequences read across all their segments (script entries return next indexes; round trip
  3068 entries + 328 stops, 0 failures). `data/story/ch3_duels`: the space clone duel and Hercule at the
  landing, the Time Chamber duel with the winner's line and Piccolo / Hercule after.
- **Story scripts (branch `story-scripts`, worktree `F:\MeltyCreations\log2-story`)**:
  `tools/script_asm.py` (lscript text form of scripts and dialogue, assembler, round trip over every
  stock script: 563 scripts, 367 sequences, 2317 entries, 0 failures) and `tools/mapedit.py` (story
  specs `data/story/*.json`: triggers, records, map scripts, line edits, grown arrays), formats in
  `docs/scripts.md`. Chapters: the Android 16 island fight (Kame House deal, 16 over both stages, ends at
  half health standing, Cell arrives), chapter 1 (old Hercule as player 2 in the prologue, Krillin's
  visit, the forest spirit Frieza + Cooler, no Krillin NPC at the landing site, old Hercule leaves with
  Trunks) and chapter 2 (the West City mansion deal, Triceratops Jungle with Piccolo and Hercule).
  Fixes: boss-bar upper percent (r3 clobbered by the entry jump), Camera_Shake self-link crash.
  Testing moved to a headless libmgba runner (`tools/headless/`) after the fork's GDB link and window
  proved unreliable for long scenes.

- **Production resumed (Ryan, after the model switch): "get as much progress with the game as you
  can", story planning paused where it stopped** (`docs/story.md` section 0, "Next to decide").
- **Merged**: `android16-boss` (sprite 160, rockets, AI, stat 109; island scripts still name 17).
- **Engine (main)**: boss rules and extra stats (`bosses.c`: Android 13 = stat 110, end-at-percent,
  partner model/stat, no-twin, Perfect Cell alone to half health) and the both-down rule (first boss
  down lies in its knock-down pose until the pair is beaten); player 2's combat stats and speeds from
  its own slot with form bonuses; safe camera shake; duplicate town NPCs removed (`actors.c`);
  world-map rule confirmed (player 2 has no entity there); player-vs-player duel (`duel.c`: opponent
  hits, final blow to 1 HP, winner, pad swap, shadow clone in party slot 7); Hercule starts without
  Super Hercule (`hercule_awaken()` grants it later); debug warp/duel commands; several fork
  instances in parallel (slots 0-4). All verified in the fork (`docs/coop-design.md`).
- **Side sessions started** (own worktrees and fork slots): story-scripts (slot 1: script assembler,
  map patcher, island 16 fight, opening chapters), krillin-playable (slot 2: Krillin's row,
  portrait, names, Destructo Disc; old Hercule row), boss-models (slot 3: Android 13 sprite 165,
  Cooler's Namekian 166), map-tools (slot 4: map format, new arctic / Time Chamber / Broly maps).
- **New maps (branch `new-maps`, worktree `F:\MeltyCreations\log2-maps`)**: the map format is decoded
  end to end (`docs/maps.md`: MapEntry, variation, tiled and effect layers, the one global BG
  palette, tileset slot table + shared tile banks, animated tiles, walk / projectile collision
  bitmaps, triggers, exits, flight spots, level gates and barriers), with `tools/maplib.py`,
  `tools/mapexport.py` (render any map) and `tools/mapbuild.py` (build maps from
  `data/maps/*.json`). `patch_rom.py --maps` places them after the data patches and relocates the
  MapEntry table (331 entries). Zone 15: area 1 arctic (Northern Mountains re-dressed in snow and ice,
  falling snow, the stock Vegeta barrier at a cave), area 2 Hyperbolic Time Chamber (white void,
  living quarters), area 3 Broly's arena (wasteland basin and pod crater), area 4 a crater imported
  from Buu's Fury (built when `roms/buu.gba` is there). All four tested in the WSL fork with both
  players (collision, wide view, door exit, flight spot). Found on the way: zone 13 is a stock "Snowy
  Highlands" zone; Buu's Fury uses the same map format (401 maps render; its map table is not found,
  so Broly's own map is not identified). Fork tip: `runfork.ps1 -Mute` (audio sync stalls a fork).

- **Krillin is a real party member; old Hercule row** (branch `krillin-party`, worktree
  `F:\MeltyCreations\log2-krillin`, not merged): slot 6 on display row 23 with sprite 161 (sprite 76
  plus Goku's fight groups re-headed, `tools/krillinsprites.py`), dialogue portrait 73 and a status
  portrait (id 119, 73 in the menu BG palette), his name on the status / pause screens and in "%s"
  dialogue (relocated UI string table with per-slot blocks), status technique pages, stats / growth
  weaker than the Z fighters, abilities 21 Destructo Disc (piercing, cooldown, 12 EP), 19 Afterimage
  Rush, 20 Shockwave Punch and the stock Ki Blast recoloured blue. Old Hercule = slot 5 on row 24
  (sprite 158) via opcode 73 `(5, 158)`, back with `(5, 72)`, combat bytes / 3, no Super Hercule.
  All tested in fork slot 2 (`docs/hercule.md` 5b / 5c, screenshots `build/kr/shots/`).
- **Level-up crash fixed** (same branch): every level-up in a PARTY_SLOTS build crashed when its
  dialogue closed (`Party_AddExp` read module words below the moved literal 0x3EA0;
  `docs/party-8-slots.md` "Level-ups"). `party_add_exp_hook` re-implements it.
- **Story planning walked from the player's side through the Cell Games wait** (Ryan + Claude,
  all in `docs/story.md`, section 0 is the summary): the two duels, Android 13 and 16 as new
  bosses, Multi-Form Cell, 2v2 gym, Cooler's Namekian, the half-health and both-down rules, casting
  per chapter. Next: Android 13's partner, then the Cell Games. No story code written.
- **No-reveal rule**: Hercule is not revealed as the hero until much later; the prologue caption
  rewrites were removed (`data/hercule-text.json` is empty).
- **Merged to main**: `party-slots` (eight slots, scrolling switch screen, new save layout; plain
  `make` is now the 8-slot game, old saves read as empty, Krillin is a placeholder row) and the
  Buu's Fury sprite decode/converter (ids 158 Buu-saga Hercule, 159 LSSJ Broly; Android 13 is not
  in that ROM). Both built together after a doc-conflict merge (2f4ca5d).
- **Side sessions**: party-slots, buu-sprites and hercule-hero are done and idle; an Android 16
  boss session (art, rockets, AI, own worktree) was started from a card and then stopped; its
  branch, if any, is not merged. Ryan is closing sessions and switching models next.
- **Android 16 boss** (branch `android16-boss`, worktree `F:\MeltyCreations\log2-a16`, not merged):
  sprite 160 (`tools/a16sprites.py`: 16's stock sprite 8 groups + 17's fight frames re-headed + a
  rocket-punch group), missile / fist / fire-explosion frames, the AI `src/coop/android16.c`
  (heat-seeking rocket on the nearer player with a 3 s fuse, barrage, area bombs, rocket punch,
  poise), enemy stat 109 (4000 HP), `A16_TEST` spawn + autopilot. The autopilot pair at level 30
  wins 3 of 5 fights. Android 16 is sprite 8 in the stock game (15 is Dr. Gero). Placement for the
  island scripts, engine facts and tuning: `docs/coop-design.md` "Android 16 boss".

## 2026-10-17

- **Story decision (Ryan): "Canon plus Hercule"**: the DBZ story stays, Hercule is in every scene as
  player 2, the twist is saved for the end (surprise Super Saiyan Hercule beats Buu; Goku fuses with
  Hercule, not Vegeta, against Buu; Vegito stays for Janemba). Beat sheet in `docs/story.md`.
- **Opening redesigned by Ryan (late evening)**: future prologue keeps Trunks with old Hercule as
  comic-relief player 2; present day opens with Krillin visiting Gohan (player 2 = Krillin), a
  Frieza + Cooler "spirit of the forest" fight replaces the dream, Hercule joins in West City via a
  mansion deal with Piccolo. Checked against the dumped scripts (`build/scripts/`); the one engine
  blocker is the six-slot party (`docs/story.md` section 5 then; now `docs/story-archive.md` M). No code yet, by Ryan's request.
- **Eight party slots decided** (Ryan): `docs/party-8-slots.md` inventories every six-slot
  assumption in the ROM (27 pointer words, ~15 tail offsets, 8 count constants, the default table,
  the save path, two screens) from a whole-ROM decompile (`build/decomp_all/`) and plans the work
  in five testable steps, about three days. Not started.
- **Super Saiyan Hercule works in game** (d7093cd): sprite 157 / display row 22 generated from
  Hercule's record by `tools/spriteclone.py`, display rows and sprite index relocated into the blob
  (`patch_rom.py` LITERAL_RELOCATIONS, row-limit byte), `src/coop/ssj.c` ability pair wired into
  Hercule's third ability slot by the hercule-hero session (d8abb54). Two bugs found with the fork's
  GDB stub: co-op code read the stock display table (no row 22) and the clone stopped at 9 of 21 pose
  groups. See `docs/sprites.md` for the record facts and `docs/testing.md` for the GDB recipe.
- **Hercule hero pass** (0bf5214, d8abb54, other session): `data/hercule.json` data patch flow
  (`tools/romdata.py`), growth curve, four-ability moveset, Keep-Goku byte, dialogue dumper and the
  first text swap. `docs/hercule.md`.
- **Story and plan docs**: `docs/plan-legacy-of-hercule.md` (build plan), `docs/story.md` (ROM
  chapters, rewrite options, questions for Ryan), `docs/sprites.md` (atlas, roster: Krillin 76 and
  Tien 121 are playable-grade; 136 squirrel-like). Ryan's direction: shape the story first; only
  Super Saiyan Hercule proceeds among new-character work; Krillin/Tien deferred.
- **Dynamic wide view** (WIDE=1 + WSL fork): 240x160 when close, up to 720x480 as the players
  separate, verified both ways with player 2 on the TCP pad. `docs/widescreen.md`.
- **Co-op difficulty**: scaled enemies, twins, twin bosses with a second bar (other session).
  `docs/coop-design.md`.
- **EXP to the killing player, save-pad switching with no duplicate characters, Hercule as player
  2's default, post-cutscene stale-pointer fix, two-player dash crash fix.** `docs/coop-design.md`.
- **Netplay** (212d8e7): frame-lockstep over TCP with a relay; one-PC test only so far.

- **Buu's Fury sprite decode + converter**: index `0x086B6BDC` (297 records), palette `0x0805652C`,
  record/animation/descriptor layout, per-record colour remap tables, multi-part frames
  (`docs/sprites.md` "Buu's Fury"). `tools/buu_sprites.py` atlas + sheets + report;
  `tools/buu2log2.py` converts records to LoG2 format with the palette remapped and an LZ encoder for
  the game's decompressor; `make` links them as sprite ids 158 (Buu-saga Hercule, record 128) and
  159 (LSSJ Broly, record 28) when `roms/buu.gba` is present. Hercule verified in game as player 2.
  **Android 13 is not in Buu's Fury** (all 297 records checked); the story needs it drawn.
- **PARTY_SLOTS** (branch `party-slots`, 2026-10-17): the party block relocated to EWRAM and grown to
  N entries (default 8; Krillin placeholder at slot 6, slot 7 empty), every six-slot site patched or
  re-implemented (`src/coop/party.c`), a new EEPROM save layout and save-menu slot store
  (`party_save.c`), a scrolling Switch Characters screen with edge indicators (`switch.c`). Verified
  in the WSL fork: save, Select Game, load, switch to Krillin. `docs/party-8-slots.md`. Found on the
  way: the arena's usable end is cap words + 8 bytes (old cap overlapped `coop2` by 8 bytes).

## Open items

- Boss tuning: re-tune Android 13 (stats 110 / 112) once the story fixes the levels for the 19 and
  Gero fights (13 mostly fights player 2, Hercule); re-run the Namekian's final 74/70/68 numbers in
  the fork; King Cold needs a fight set (pose groups 2 and 5-8) before he can partner anyone.
- Old Hercule needs his own dialogue portrait (he uses Hercule's).
- Party: the save menu's six-wide portrait rows, the status screen's paging key; the journal with
  seen bits set was not exercised in the fork.

- Human test of the save-pad switch with player 2 present; a real two-PC netplay run.
- Unexplained crash on the stock ROM reported by Ryan ("jumped to invalid address 27994586"),
  parked until it recurs (`docs/testing.md` has the GDB recipe to catch it).
- Lua frame callbacks never fire in the SDL fork (helpers are Windows-mGBA only).
- Super Saiyan Hercule: icon for the ability, aura/flash on transform, EP drain and revert rules,
  Hercule-specific SSJ moveset; the game's own transformed-state handling only runs for the active
  player (opcode 73 refreshes player 1's entity only; `p2_refresh_sprite` covers player 2).
- Story: Ryan plans the Cell Games, the ending and the Buu phase next (`docs/story.md` section 0).
- Removed from the wait (2026-10-18): the arctic Android 13 encounter (`ch8_arctic`), held for after Cell;
  stat 114 and flag 376 are unused.
- Training waves: rewards beyond EXP are Ryan's call (ideas in `docs/story.md` 4g).
