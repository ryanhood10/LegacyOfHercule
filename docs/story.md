# The Legacy of Hercule: the story plan

This file is the current plan: Ryan's decisions, chapter by chapter. What the stock ROM does (scenes,
maps, flags, speaker ids, quotes) is in `docs/story-rom.md`; superseded options, cost estimates and
investigation notes are in `docs/story-archive.md`. Chapter specs and scripts are `data/story/*.json`
and `*.lscript` (format in `docs/scripts.md`, applied in the order listed in `data/hercule.json`
"story"). Game facts come from the script dump (`python tools/script_dump.py map ZONE AREA`, all
maps in `build/scripts/` after a full dump), the quest table and Ghidra.

## 0. Status and how to resume (2026-10-19)

**Mode: story planning.** Ryan plans the story chapter by chapter in chat, at a slow pace, one
decision at a time, answered from the player's point of view and grounded in ROM facts, with no
suggestions he did not ask for. Record each decision in this file the same turn. Build a part of the
story only after Ryan says its plan is done. Everything decided through the ten-day wait is built
and merged; what comes next is under "Open" below.

**Standing rules (Ryan):**

- Nothing reveals Hercule as the hero before the Buu phase (the prologue caption rewrites were
  removed; `data/hercule-text.json` is empty).
- One player controls the world map: whoever stepped on the flight sign; the other is a passenger
  with no entity there (`docs/coop-design.md`).
- Both-down: when bosses fight as a pair, the first to fall stays down on the map until the other
  is beaten, and only then does the fight's defeat script run. Half health: some bosses end at about
  half health and are reused later. Both are per-stat rules in the table in `src/coop/bosses.c`; the
  logic is in `src/coop/enemies.c`.
- Duplicate NPC: a map actor that shows a character one of the players is controlling is not
  spawned; its dialogue still plays with the portrait (`src/coop/actors.c`).
- Duels: both fighters start as Super Saiyans with no drain, the final blow leaves the loser at 1 HP,
  a draw goes to player 1, no level handicap (pride is the stake). The winner's pad keeps the lead
  role and the loser's pad plays the role the story gives the loser (the pads swap when player 2
  wins). Code: `src/coop/duel.c`, the clone in party slot 7.
- Hercule and Krillin stay weaker than the Z fighters (growth in `data/hercule.json`,
  `docs/hercule.md`).
- Android 13 is held back until after Cell: he returns after the Dragon Ball wish revives everyone,
  17 and 18 included (4g). His earlier appearances (beside 19, beside Gero, in the lab) stay.

**Chapters.** All built and tested headless (`docs/testing.md`); specs in `data/story/`.

| # | what happens | player 1 / player 2 | spec | status |
|---|---|---|---|---|
| 1 | Future prologue with old Hercule; Krillin visits Gohan; the forest "spirit" fight (Frieza + Cooler); no Krillin NPC at the landing site; Goku arrives as stock (4) | Trunks / old Hercule, then Gohan / Krillin | `ch1_opening` | merged 2026-10-18 |
| 2 | West City: Hercule joins after the mansion deal with Piccolo; Triceratops Jungle (4) | Gohan / Krillin; jungle Piccolo / Hercule | `ch2_west_city` | merged 2026-10-18 |
| 3 | Space clone duel; Hercule meets Vegeta at the landing; zone 7; Android 19 + base-form 13 (both-down, 13 flees); the skip's boosts (4b) | Vegeta / clone, then duel winner Vegeta / loser Hercule | `ch3_duels`, `ch3_zone7` | merged 2026-10-18 |
| 4 | Piccolo's chapter: Gero + 13 at the lab door, 13 in the lab, the road with Piccolo + Vegeta vs 17 + 18, stock ending (4c) | Piccolo / Hercule; road Piccolo / Vegeta | `ch4_piccolo` | merged 2026-10-18 |
| 5 | Lookout (Hercule stays), Trunks' chapter, Kami's boost for Hercule then the fusion, Gingertown vs Imperfect Cell + duplicate (4d, 4e) | Trunks / Gohan; Piccolo / Hercule | `ch5_lookout` | merged 2026-10-18 |
| 6 | Lab basement, Kame House, Time Chamber duel (Vegeta vs Trunks), Android 16 island fight to half health, Cell arrives (4e) | Trunks / Krillin; duel Vegeta / Trunks; island Piccolo / Hercule or Krillin | `ch6_basement`, `ch3_duels`, `a16_island` | merged 2026-10-18 |
| 7 | Semi-Perfect Cell + duplicate, Cell absorbs 18, Perfect Cell vs Ascended Trunks + Krillin to half health, Krillin's one-hit KO, the Games announced (4f) | Vegeta / Trunks; Trunks / Krillin | `ch7_cell` | merged 2026-10-18 |
| 8 | Ten-day wait: Roshi's 2v2 gym and the animal-ninja finale (4f) | any (save pad) | `ch8_gym` | merged 2026-10-18 |
| 8 | Cooler with his Namekian servant, Piccolo gate (4f) | any | `ch8_cooler` | merged 2026-10-18 |
| 8 | Roshi's dojo sparring, players 1 v 1 (4g) | the two players | `wait_spar` | merged 2026-10-19 |
| 8 | Time Chamber training waves, ten rounds (4g) | any | `wait_chamber` | merged 2026-10-19 |
| 8 | Broly side chapter (4g) | Vegeta / Trunks on arrival, any of the trip's five | `broly` | merged 2026-10-19 |
| 8 | "Gero's computer": Krillin + 16 back to the lab, two 19s, ends as Krillin + Hercule (4g) | Krillin / Android 16 | `ch9_gero` | merged 2026-10-19 |

`data/story/ch8_arctic.lscript` is the removed arctic Android 13 encounter, kept for his return and
not used by any trigger (`docs/story-archive.md`).

**Open (next to plan, Ryan in chat):**

- The Cell Games (in planning, section 4h): Hercule's stock public "victory", the Cell Juniors, Gohan's win, "Keep Goku" (an
  old note; the stock script removes Goku after Cell's defeat and makes Hercule available). Stock
  flow in `docs/story-rom.md` section 9.
- The ending of LoG2, and Android 13's return after the wish.
- The Buu's Fury phase (section 6). Also section 7.

**Numbers Ryan may want to change (our tuning, not his):** Kami's boost (+3 levels and a full
refill); the gym duos (stats 106 + 98 and 64 + 64, also fought in single-player); Broly 8000 HP
(52/54/50), Paragus 2000 HP (24/22/20), the rage (full HP, +10 attack, +6 defence, chase x1.3), the
win's 20000 EXP; the bunker's two 19s at 3600 HP (23/17/16 each); Android 16 as a player at Krillin's
level with 1.5x his strength and endurance; the generator scenes and the home scene's lines in the
16 chapter.

**Reward ideas for the training waves (not built, for Ryan):** a full HP / EP refill between rounds;
a one-time +3 capsule or Senzu bean the first time "Training completed!" is reached; a bonus round 11
of Cell Juniors (stat 26, already a regular-sized sprite). Today the waves pay EXP from kills only.

## 2. Decision (Ryan, 2026-10-17): "Canon plus Hercule"

The DBZ story stays beat for beat; Hercule is written into the scenes as the man who was also
there, and player 2 is whoever makes sense in the chapter (old Hercule in the future prologue,
Krillin in the Gohan chapters, Hercule from West City on). The twist is kept for the end: a surprise
Super Saiyan Hercule beats Majin Buu, and against Buu the fusion is Goku + Hercule instead of
Goku + Vegeta. Vegito stays for Janemba (Ryan's call; the film has Gogeta against Janemba, and
whether Buu's Fury has a Janemba fight at all is still to be checked in that ROM). Hercule gains;
nobody else loses their moment.

## 4. The opening and West City (Ryan, 2026-10-17; chapters 1 and 2)

- **Future prologue:** Trunks is player 1; player 2 is old Hercule (his Buu-saga look, sprite 158 on
  row 24), Trunks' buddy, comic relief, powerless, with no hint of what he becomes. He rides the time
  machine back and leaves with Trunks after Goku's arrival.
- **Present day:** Krillin walks in instead of phoning; player 2 is Krillin from here.
- **The forest "spirit" fight:** Gohan and Krillin go into the forest (the Battle Armor cave gives
  the reason) and fight Frieza and Cooler (stat 41 + stat 111 on Cooler's sprite 34, always a pair);
  afterwards they are told it was the spirit of the forest. No "it was a dream", no teleport home.
- **Landing site:** no Krillin NPC, because player 2 is Krillin; his lines stay, spoken by the player.
  Goku's arrival plays as stock.
- **West City:** Gohan and Krillin fly there; Piccolo becomes playable, and **Hercule joins** after
  the mansion deal (he wants Piccolo's flying "magic trick", Piccolo needs a roof for the three
  years). Hercule is in the party from here on.
- **Triceratops Jungle:** Piccolo's solo stretch with Hercule only as player 2 (level 1 beside
  Piccolo; Gohan and Krillin are locked inside and given back on the way out); Hercule claims the
  credit for the dinosaur peace treaty.

## 4b. Vegeta's chapter: the clone duel (Ryan, 2026-10-17)

1. **Space cutscene, rewritten.** Vegeta trains against his "shadow", a clone of himself. The clone
   lands a couple of punches; Vegeta's blast sends it into the rocks (explosion). Vegeta taunts it as
   an inferior copy. The clone goes Super Saiyan first: only one of them leaves alive, it is done
   living in his shadow; it knocks Vegeta into an explosion. Vegeta's monologue (his race enslaved,
   the true prince surpassed by a low-class Saiyan from Earth, and now a clone meant for poultry
   training reaches the legend; it is in his DNA, it is his legend, he takes what is his) and he
   transforms.
2. **The duel.** Player 1 controls Vegeta, player 2 the clone, both Super Saiyan, to the final blow
   (duel rules in section 0).
3. **Result.** The winner plays Vegeta. One line, "VEGETA: I've won. I'm a Super Saiyan", fade to the
   landing map. Whoever won is the only player in control until Hercule turns up.
4. **Hercule arrives at the landing map** (zone 6 area 22, map 166, so the loser sits out one scene,
   not fourteen robot areas): "Thank God he's arrived, we need you in the fight." Vegeta: who the
   hell are you? He is Hercule; Piccolo told him to wait here and watch for Vegeta. From here the
   loser of the duel is Hercule (the only selectable player-2 character), useless beside Vegeta
   through the robot zone and the Android 19 fight.
5. **The Android 19 fight stays Vegeta's, made two-on-two.** **Android 13, base form** (sprite 165,
   stat 110) stands with 19 and 20 in the zone 7 area 1 scene (Goku coughing, 19 volunteering) and is
   introduced there, then fights beside 19 as his partner (always a pair, so in single-player too).
   Both-down: when both are beaten, 19 explodes and 13 flies off, swearing revenge and saying not even
   they know Dr. Gero's "true plan". Then the stock scene: 20 flees, Vegeta chases him, Piccolo
   becomes player 1.
6. **Three-year skip boosts.** Krillin and Hercule get the same +1 level and full refill as Gohan and
   Piccolo (Ryan: boosted too, but kept weaker; curves in `docs/hercule.md`).

## 4c. Piccolo's chapter (Ryan, 2026-10-17): Hercule stays player 2

Player 2 defaults to Hercule, but the save pad keeps the other unlocked characters selectable (this
is where the duplicate-NPC rule came from: Krillin as player 2 never meets his own NPC). Hercule
gets a line per scene: scared of the lab door, hiding behind Piccolo when 17 and 18 wake.

**Gero's lab door:** the players fight Dr. Gero (Android 20) at the door in his three stages, with
robot escorts, and **Android 13 backs Gero up** as his partner (stat 112, both-down); 13 flies off
when the pair is beaten. Hercule offers to "negotiate". Inside the lab (zone 8 area 60) the stock
scene plays: Gero wakes 17 and 18 and dies at their hands; 13 watches, then leaves promising his
master he will fulfil his destiny.

**The road: Piccolo and Vegeta fight 17 and 18.** In the series it is Vegeta who fights 18 on this
road and loses, with Piccolo beaten too, so this is closer to the source than the stock scene. For
the fight Vegeta is the only player-2 choice (whoever lost the duel gets Vegeta back here); 17 joins
as 18's partner (his fight sprite 9), both-down, and the blast knocks both players flat before the
stock dialogue resumes. Hercule is player 2 again for the stock ending and stands with Krillin for
the plea; the kiss stays Krillin's.

## 4d. After the road: Lookout, Trunks' chapter, the fusion (Ryan, 2026-10-17)

- **World map rule:** only the player who stepped on the flight sign controls the world map
  (section 0).
- **Lookout, first visit:** Piccolo flies there with Hercule. When Kami defers and the script locks
  Piccolo out, Hercule is locked out too and stays at the Lookout.
- **Trunks' chapter:** player 1 Trunks, player 2 Gohan, everyone else locked out (Krillin is busy
  moving Goku to Roshi's). Nothing else in the story changes here.
- **The fusion scene, combined:** before Kami fuses with Piccolo he draws out Hercule's "potential"
  for befriending and treating Piccolo well: +3 levels and a full HP / EP refill (Ryan's note said
  "level + EXP"; the amount is ours), then the fusion plays as written. Piccolo and the boosted
  Hercule go to Gingertown together.

## 4e. Gingertown to the island (Ryan, 2026-10-18)

- **Gingertown: Imperfect Cell fights with a Multi-Form duplicate.** Cell has Tien's cells, so the
  split is Tien's technique and Piccolo says so; Cell plus Cell as a pair, both-down: a body that
  falls is reabsorbed, and when both are beaten Cell pulls himself together and escapes as in the
  stock scene. Players: Piccolo and the Kami-boosted Hercule; Krillin and Trunks arrive at the end as
  scripted. Cell Juniors stay for the Cell Games.
- **After the fight:** Trunks and Krillin go back to Dr. Gero's lab; the locked pair for the **lab
  basement** is Trunks + Krillin (his NPC copy is left out by the duplicate-NPC rule), while Hercule
  flies on with Piccolo.
- **Kame House:** Goku wakes as stock; Piccolo is a scene actor there, not playable. Krillin stays
  home at Kame House when Trunks flies to the Lookout.
- **The Lookout:** player 2 is Hercule (he belongs in the stock party from West City on).
- **Time Chamber duel.** When Vegeta and Trunks go in, fade to black and cut to the two of them
  walking into the chamber. Trunks: let's get settled in. Vegeta demands they spar right now. Cut to
  the two players facing each other: a second duel with the same rules as the space one. The
  winner's pad plays Piccolo in the next scene, the loser's pad plays Hercule. Then fade back to the
  Lookout where Goku sends Piccolo to Roshi's island. The chamber interior is a new map, zone 15 area
  2 (built 2026-10-18, `docs/maps.md`): a white void with the living quarters at the top edge and a
  door back to the Lookout. While Vegeta and Trunks are inside, the playables are Piccolo (player 1)
  with Hercule, Krillin or Gohan.
- **Android 16 is the island boss (decided 2026-10-18).** The Kame House dialogue changes: 16 is
  furious that Goku is not there, and Piccolo tricks him ("beat me on the next island and I'll tell
  you where Goku is"). The fight is Piccolo and player 2 (Hercule or Krillin; Krillin can be picked at
  the yard's save pad before the scene) against 16 over the two stock stages, with new fighting
  frames (sprite 160, stat 109), heat-seeking rockets that lock on and detonate after three seconds,
  rocket barrages, big explosives across the field, none of which hurt 16, and a rocket punch
  (`src/coop/android16.c`). 17 and 18 watch; the fight ends at half health (4f); Cell arrives as
  scripted and the warp to the Lookout follows.

## 4f. Boss endings and the road to the Cell Games (Ryan, 2026-10-18)

- **Android 16 is not killed.** The island fight is hard but ends when 16 is down to about half
  health; Cell arrives and the stock warp follows. 16 is kept for later: the stock game already
  brings him back asking for repairs, and the "Gero's computer" chapter (4g) uses him.
- **Semi-Perfect Cell vs Vegeta and Trunks:** Cell splits into two (Multi-Form duplicate), the same
  mechanics as Gingertown. The stock twist stays: Cell talks Vegeta into letting him absorb 18.
- **Perfect Cell vs Trunks and Krillin:** after Vegeta goes down from the kick, Krillin flies in (as
  in the series, detonator in hand) and the pad that played Vegeta switches to Krillin (the pads trade
  players, so the Trunks player is player 1 from here). Ascended Trunks and Krillin fight Perfect Cell
  with no partner; the fight ends at about half of Cell's health when he laughs it off ("All you've
  done is pump up your muscles"), and Krillin's rush and one-hit KO is the opening beat of the
  scripted ending. Trunks' transformation beats stay as scripted.
- **After Perfect Cell:** the default pair is Trunks + Krillin; Vegeta stays selectable (his
  barriers are needed to progress), Piccolo and Hercule too; the pad lets players change.

**Roshi's gym, redesigned.** The matches are **2 v 2**: the two players against Yamcha & Tien, then
against Hercule & Krillin (each win still pays a +3 capsule). If one player is currently Hercule or
Krillin, only the other of that duo stands in the room, and talking to him gives "Sorry, Master Roshi
says I can only fight duos with Krillin on my team right now" (or "...with Hercule..."), so the
players come back with different characters. When both duos are beaten, the **final fight**: Roshi
has hired special animal ninjas for a different kind of training: wild animals (alligator, hawk,
ladybug, pterodactyl, sabertooth tiger, scorpion, snake, T-rex, triceratops) plus two ninjas; clear
the room for the last capsule (built as three rounds, flags 363-375; flag 214 at the end).

**Side quests in the wait.**

- **Cooler:** keep the Piccolo gates as they are (reachable in the wait; no level gate) and make the
  fight two-player: Cooler fights beside **a strong Namekian servant** (sprite 166, Piccolo's set
  recoloured; stat 113), explained in the dialogue as a Namekian warrior taken from Namek by Frieza's
  family and kept by Cooler. Both-down.
- **Android 13 in the arctic:** planned and built 2026-10-18, removed the same day when Ryan held 13
  back until after Cell (4g). The arctic map (zone 15 area 1, a snowfield with a cave behind an "Only
  Vegeta" barrier, flag 380) stays built with an empty cave. Details in `docs/story-archive.md`.
- **Broly:** the side chapter is in 4g. Its arena is zone 15 area 3 (a wasteland basin with a pod
  crater); zone 15 area 4, a walled crater imported from Buu's Fury, is built but no chapter uses it.

Half-health endings are one rule in the boss table (`src/coop/bosses.c`): a boss with an "ends at N%"
value fires its defeat action at that threshold instead of at zero HP.

## 4g. The ten-day wait, revised (Ryan, 2026-10-18 and 2026-10-19)

- **Android 13 is held back until after Cell.** He absorbs 17 and 18 to become Super 13, so his
  return comes after the Dragon Ball wish brings everyone (17 and 18 included) back to life. The
  arctic encounter built in chapter 8 came out of the wait; his earlier appearances (beside 19,
  beside Gero, watching in the lab) stay.
- **Kept as planned:** Roshi's 2v2 gym and its finale; Cooler with his Namekian servant, reachable
  in the wait (the gate is the stock "Only Piccolo" barrier; there is no level gate).
- **Roshi's dojo sparring (built and merged 2026-10-19, `data/story/wait_spar`):** once the players
  have cleared all three dojo matches and Roshi has handed over the last +3 capsule (flag 214),
  talking to him in the Capsule Corp atrium ends with his offer ("the dojo is yours. One on one... no
  prizes, just pride!"). In co-op he books the dojo (flag 440) and walking onto the sparring mat starts
  a duel between the two players as they are (`CoopCall 1` with player 2's own slot, no clone, no pad
  swap, no story stakes, as many times as they like); the loser is left at 1 HP, a caption names the
  winner ("Piccolo wins the sparring match!"), both are healed and the fade returns them to Capsule
  Corp. Alone, Roshi says you need a partner. Tested headless (both outcomes, the repeat, single-player).
- **Dragon Ball race:** dropped (Ryan).
- **Time Chamber training waves (approved by Ryan; built and merged 2026-10-19,
  `data/story/wait_chamber`, `src/coop/waves.c`):** instead of grinding in the wild, the players
  enter the Hyperbolic Time Chamber (zone 15 area 2) from Kami's Lookout and fight ten rounds of stock
  enemies in fixed tiers, harder as the rounds go up. It opens at the start of the ten-day wait (flag
  157, after Cell's announcement; the game never shows Goku and Gohan coming out). Mr. Popo stands
  beside the chamber door (zone 9 area 3, A facing the door, the stock "You cannot enter" spot) and
  points the players to it ("Goku and Gohan have finished their training... ten rounds, each one
  stronger than the last... You may walk out at any time"). Before each round a caption "Round N" (A
  to start it); the round's four enemies (eight in co-op with their twins) spawn on a ring around the
  players; when none is left standing, a short pause and the next caption. After round 10 a caption
  says "Training completed!" and nothing more spawns. The chamber's door returns to the chamber room
  and stops the training at any time; the Lookout door starts it again from round 1. Repeatable.
  Rewards: EXP from the kills only (ideas in section 0). The rounds (stock stat / sprite, HP before
  the co-op x1.5; four records per round):

  | round | enemies | HP |
  |---|---|---|
  | 1 | 2 wolves (57 / 148), 2 hawks (107 / 64) | 675, 700 |
  | 2 | 2 sabre-toothed tigers (92 / 108), 2 Kuma mercenaries (70 / 91) | 750, 940 |
  | 3 | 2 ninjas (75 / 96), 2 pterodactyls (80 / 105) | 900, 900 |
  | 4 | 2 Warlord's henchmen (12 / 142), 2 warthogs (56 / 145) | 900, 900 |
  | 5 | 2 hawks (84 / 65), 2 wolves (104 / 149) | 1000, 1000 |
  | 6 | 2 Warlord's henchmen (51 / 143), 2 tiger bandits (10 / 19) | 1100, 1120 |
  | 7 | 2 Eggbots (40 / 47), 2 pterodactyls (88 / 106) | 1130, 1200 |
  | 8 | 2 ninjas (86 / 98), 2 Eggbots (39 / 46) | 1400, 1349 |
  | 9 | sabre-toothed tiger (89 / 110), 2 scorpions (47 / 112), ninja (45 / 97) | 1750, 2000, 1600 |
  | 10 | snake (48 / 117), 2 wolves (50 / 147), Destroyer (32 / 38) | 3530, 3980, 4200 |

  Round sizes are set by free OBJ VRAM, not a fixed count (measurements in `docs/story-archive.md` J).
  Tested headless (every round, leaving and re-entering, single-player) and round 3 in the fork.
- **Android 16 side chapter: "Gero's computer" (decided 2026-10-19; built and merged 2026-10-19,
  `data/story/ch9_gero`, `docs/scripts.md` section 5).** Player 1 is Krillin and player 2 is Android
  16, locked for the whole chapter. It fills a gap: the game never shows 16 repaired, and he next
  stands at the Cell Games arena with his bomb removed (`docs/story-rom.md` section 8).
  - *Trigger:* after Cell's TV announcement and the Scouter Note, entering Dr. Briefs' workshop
    (zone 6 area 5) plays a new scene: Dr. Briefs says 16 is repaired and the bomb is out; 16 wakes and
    picks up a signal from Gero's lab: the computer is running again. Krillin goes with him because he
    has been down there. While the chapter runs, the lab's landing site on the world map shows a star.
  - *The lab:* the zone 8 lab maps in the Northern Mountains. The robot grunts are back online and the
    generators (under the waterfalls of areas 27 and 40) have to be shut down again. The stock game
    seals the cave mouth into the basement after the self-destruct, so 16 punches through the rubble,
    and the basement door leads to the computer room again (investigation in `docs/story-archive.md`).
  - *The bunker boss:* the computer is finishing spare Android 19 bodies; the players fight **two
    Android 19s at once** (Ryan; stats 120 and 121 on 19's sprite 12, a pair). 16 is a puncher, so his
    first fight as a player suits a brawl.
  - *Easter egg:* before they smash the computer, one file on it names Android 13 as unfinished: a
    hint at the post-Cell story, without bringing 13 in.
  - *The way home:* on the way out 16 stops to watch the birds, and tells Krillin he will go to the
    Cell Games (this is why he stands at the arena later).
  - *End:* warp back to Capsule Corp; the players become Krillin (player 1) and Hercule (player 2),
    and 16 is never playable again (he stays a scene actor). From here the players proceed to the
    Cell Games.
  - *Build notes:* 16 plays in the last party slot during the chapter (sprite 175, row 25, Rocket
    Punch ability 22; `src/coop/a16player.c`). Area 47's top exit was broken in the stock game too and
    is fixed. Flags 420-439. Tested headless end to end (`tools/headless/ch9_gero.txt`), and the bunker
    fight and Rocket Punch in the fork. Tunable choices are in section 0.
- **Broly side chapter (decided 2026-10-18; built and merged 2026-10-19, `data/story/broly`,
  `docs/scripts.md` section 5):**
  - *Trigger:* right after Cell's TV announcement in the Briefs' residence (zone 6 area 16, where
    Bulma already hands over the Scouter Note), Bulma walks in (she is not in the room after Perfect
    Cell) and adds: tell Vegeta a ship is arriving for him in the backyard, and it looks like a Saiyan
    ship (flag 400).
  - *Backyard:* the Capsule Corp lawn (zone 6 area 21). A Saiyan ship stands on the patio (Buu's
    Fury's purple ship, sprite 171); without Vegeta as a player a narration line says Vegeta should see
    it. With Vegeta, Paragus steps out and says he has found a New Planet Vegeta for him to rule as
    king. Vegeta goes aboard; Trunks follows with Krillin, Gohan and Piccolo; the ship leaves. Those
    five are the only selectable characters until the way home (Hercule stays home).
  - *Arrival:* New Planet Vegeta is the Broly arena (zone 15 area 3) with a landing zone added south
    of the basin, the ship and a save pad where the players can switch to anyone of the five; Vegeta
    is player 1 and Trunks player 2 on arrival.
  - *The fight:* crossing into the basin, everyone appears on screen for the stock-style dialogue
    (each actor only when no player plays him; Broly mistakes Gohan for Kakarot's son), then only
    player 1 and player 2 fight Broly (stat 115) and Paragus (stat 116), on Cooler's melee boss AI.
    Broly is the real boss and much stronger; Paragus is weak and easy. If Paragus dies first,
    Broly's health refills once and he becomes stronger and faster (rage at his father's death).
    Numbers in section 0.
  - *Models:* Broly appears only as the Legendary Super Saiyan (sprite 159, from Buu's Fury), already
    waiting in the basin; no base-form Broly. Paragus is Vegeta's Saiyan-armor set (sprites 137-140)
    made old with grey hair and a grey goatee (sprite 170, `tools/brolysprites.py`), with a matching
    edit of Vegeta's dialogue portrait (portrait 120).
  - *End:* after both fall, a huge explosion, a debrief in the backyard with Bulma, fade to black,
    and the party is exactly as before the trip (selectable bits, player 1 and player 2; flags
    400-419).
- **The wait's story is complete (Ryan, 2026-10-19):** after the 16 chapter the players proceed to
  the Cell Games; no more story is planned for the wait.
- **Castle defense (Ryan, idea for later, not MVP):** each Dragon Ball found cuts to a separate
  castle-defense game where waves of Cell Juniors try to steal it. The hunt appears to gate the Cell
  Games (`docs/story-rom.md` section 8); confirm in play before building on it.

## 4h. The Cell Games (Ryan, 2026-10-19, in planning)

Stock flow, fight by fight with the scripted player 1: `docs/story-rom.md` section 9. Ryan's plan
(2026-10-19). Draft lines below are Claude's, for Ryan to change.

**Before fight 1 (unchanged):** the arena grounds, Hercule's ring scene (his punches do nothing,
"Your ignorance is painful," one blow sends him gliding off the ring).

**One Cell until the end (Ryan):** every Cell fight before the last has a single Perfect Cell, no co-op
twin (stats 21-24 get `BOSS_NO_TWIN`); the twin Cells are saved for the final fight.

**Solo fights with a title card.** Fights 1 and 2, and the Cell Junior fights, are fought by one
player alone; the other player watches (no entity on the field, no damage). A title card before each
says who fights: "Player 1 fights" / "Player 2 fights". Ryan: this is one of the only times a player
fights alone.

| # | where | fights | against | notes |
|---|---|---|---|---|
| 1 | the ring | player 1 as Goku, alone | Perfect Cell, no twin | stock end: Cell destroys the ring |
| 2 | the desert | player 2 as Goku, alone | Perfect Cell, no twin | new end: Goku subs in Hercule (below) |
| 3 | the desert | player 1 Gohan + player 2 Hercule | Perfect Cell, no twin | stock end: 16's failed self-destruct, Cell kills him, Cell spawns the Juniors (four now) |
| J1 | the desert | player 1 as Krillin, alone | Cell Junior | new fight |
| J2 | the desert | player 2 as Piccolo, alone | Cell Junior | |
| J3 | the desert | player 1 as Vegeta, alone | Cell Junior | |
| J4 | the desert | player 2 as Trunks, alone | Cell Junior | stock end: 16's head scene, Gohan turns Super Saiyan 2 |
| 7 | the desert | player 1 Gohan (Super Saiyan 2) + player 2 Hercule | Perfect Cell, no twin | new end: two Cells come back (below) |
| 8 | the desert | player 1 Gohan + player 2 Hercule | **two Perfect Cells** (the final fight, both-down) | then the beam struggle |

Fights 1-8 keep the stock numbering (J1-J4 replace the stock 4-6). Each player fights alone three
times (player 1: Goku, Krillin, Vegeta; player 2: Goku, Piccolo, Trunks), two of them against
Juniors, and the pair fights Cell together three times (3, 7, 8).

**End of fight 2 (new).** Goku still gives up, but names Hercule instead of Gohan. Draft:
- GOKU: "I give up, Cell. But the tournament isn't over. There's someone else who's going to fight
  you... Hercule! Get out here!"
- Hercule walks out to the ring, stops, clutches his stomach. HERCULE: "Ooh... ohhh, my stomach! Must
  have been something I ate! The champ can't fight like this!"
- CELL: "This is your replacement, Goku? The clown I threw out of the ring?"
- Cut to GOHAN: "I can't let Hercule face Cell alone!" Gohan walks out beside him.
- HERCULE (straightening up): "Well... since the kid insists... stand back, boy, and watch a
  professional!"

**The Cell Juniors.** Cell spawns four (stock three), one per fight; each falls before the next player
steps in, with the title card between.

**16's head scene (stock, kept).** Hercule is already on the field, so he is the one who carries 16's
head over, as in the stock scene, played by player 2's Hercule rather than a separate actor.

**End of fight 7 (new).** Stock until the explosion: Cell spits out 18, swells, Goku teleports him to
King Kai's planet. Then, when he comes back: two separate pieces of Cell survived the explosion, so
two Perfect Cells return, stronger. Draft:
- CELL: "Did you miss me? When I exploded, two pieces of my core survived... and each one grew back
  perfect!" CELL (the second): "Twice the perfection!"
- Trunks is killed and Vegeta attacks as in the stock scene; Goku speaks to Gohan from the Other
  World as in the stock scene.

**The beam struggle (new staging).** The two Cells stand side by side firing; Gohan and Hercule stand
side by side firing back. Goku encourages both. Draft, adapting the stock lines:
- GOKU: "Dig deeper, Gohan! You too, Hercule!"
- HERCULE: "Wh-who said that?!" (a callback to his line when 16's head speaks to him)
- CELL: "These are your last minutes! Make them count!"
- GOKU: "Gohan, raise it higher! Hercule, hold on! He needs you!"
- HERCULE: "The champ... never... backs down!!!"
- VEGETA: "ARGHHH!!!" (stock: his blast distracts the Cells)
- GOKU: "Release it, both of you! Remember all the pain he's caused! NOW MAKE THAT YOUR POWER!"
- GOKU: "Now's your chance!"
The stock ending follows, including Hercule's "I DEFEATED CELL! SINGLE HANDEDLY!", which is now half true.

**To settle before building (Ryan's calls):**
- Hercule's beam pose. His sprite (72) has idle, walk, hit-stun, victory, two punches and a kick, no
  beam pose. Cheapest stand-in: hold his straight-punch pose (group 7) with the beam coming off his
  fists. Otherwise, new art.
- Hercule's strength against Perfect Cell in fights 3, 7 and 8 (he stays weaker by rule).
- The standing rule that nothing reveals Hercule as the hero before the Buu phase: this plan puts
  him in the Cell fights by Ryan's choice; the public credit stays the stock fake.
- A lost solo fight (1, 2, J1-J4): what happens when the fighting player goes down.
- After the Games: Goku already stays in the party ("Keep Goku", `docs/hercule.md` 4); the stock
  ending also sets Hercule to level 40 and announces him as a new character, which no longer fits
  (he has been in the party since West City).

**Build list (mechanics, no decision needed):** hide the watching player during solo fights; the title
cards; player 2 playing Goku (party slot 4) while player 1 watches; Krillin and the fourth Junior
record; `BOSS_NO_TWIN` on stats 21-24 and a `BOSS_PAIR` rule for the twin Cells; player 2's Hercule
carrying 16's head instead of the Hercule actor; two Cells and two beams in the struggle scene.

## 6. Buu's Fury (phase 2, separate ROM)

- The aura from the Cell Games is real; when Goku needs a fusion partner against Buu, Hercule is
  the one there: a new row and sprite for the fused form in the Buu ROM. Vegito stays for Janemba
  (whether that fight exists in Buu's Fury is still unchecked).
- The final blow on Buu is Super Saiyan Hercule's, with the Spirit Bomb beat kept as his crowd call.
- Done so far: Buu's Fury's sprite format is decoded and its art converts into LoG2
  (`docs/sprites.md` "Buu's Fury": old Hercule 158, Broly 159, the ship 171), and its maps use
  LoG2's format (`docs/maps.md`). Still needed: a pass over its party, display rows, scripts and
  saves, save conversion from LoG2, the fusion scene as a script edit, the fused form's sprite.

## 7. Still open

- Super Saiyan Hercule's look: gold afro (what we have) or a different "Super Human" look?
- Name of the Goku + Hercule fusion in our text ("Gokule" is the fan name).
- UI note: save-pad switching with only two characters already works as a trade (player 1 taking
  player 2's character hands player 2 the old one).
