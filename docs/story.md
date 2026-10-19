# The story as the ROM tells it, and how "The Legacy of Hercule" bends it

Source: the game's own caption cards and dialogue (dumped per map by `tools/script_dump.py` into
`build/scripts/`, map order follows play order closely), plus the quest table. Chapter names are ours.

## 0. Status and how to resume (2026-10-18)

**Mode:** story first. Ryan and Claude are walking the game chapter by chapter from the player's
side, deciding co-op casting and new beats; every decision is recorded here the same turn. Story
production started with the tools in `docs/scripts.md` (chapters listed below as implemented). Sections 3-4f below are the decisions in play
order; this section is the summary. Facts come from the dumped scripts (`python tools/script_dump.py
map ZONE AREA`, all maps in `build/scripts/` after a full dump) and the quest table.

**Implemented (story-scripts, 2026-10-18; `docs/scripts.md` section 5, `data/story/`):** items 1 and
2 below as written (old Hercule in the prologue, Krillin's visit, the forest spirit, the landing
sites, the West City deal, Triceratops Jungle), the duels of items 3 and 6 (space clone duel and
Hercule at the landing; Time Chamber duel, winner Piccolo / loser Hercule), zone 7 of item 3
(Android 13 in the cutaway and beside 19, both-down, 13 flees with the "true plan" line; the skip's
boosts for Hercule and Krillin), Piccolo's chapter of item 4 (13 beside Gero in his three stages,
Hercule's "negotiate" at the lab door, 13 in the lab, the road with Vegeta as player 2), item 5 (Hercule
stays at the Lookout, Gohan as player 2 in Trunks' chapter, Kami's boost for Hercule, Gingertown with
Cell's Multi-Form duplicate), item 6's lab basement (Trunks + Krillin, the NPC left out in co-op) and
Kame House (Piccolo a scene actor, Krillin stays home), item 7 (Semi-Perfect Cell's duplicate, Krillin
on the pad that played Vegeta, his one-hit KO), item 8's gym duos and animal finale, Cooler's Namekian
servant and the arctic Android 13 (alone until his partner is decided; flag 376) and the Android
16 island
fight from item 6 (Kame House deal, 16 over the two stock stages, half health, Cell). Tested headless
(`docs/testing.md`).

**Decided so far, in play order** (player 1 / player 2):

1. Future prologue: Trunks / old Hercule (sprite 158, comic relief, rides the time machine; no hint
   of what he becomes). Present day: Krillin visits Gohan instead of phoning; Gohan / Krillin; the
   forest "spirit" fight against Frieza + Cooler replaces the dream; Krillin's NPC removed from the
   landing site; Goku's arrival as stock.
2. West City: Hercule joins after the mansion deal with Piccolo (he is in the party from here on).
   Triceratops Jungle: Piccolo / Hercule only, Hercule level 1.
3. Amenbo Island, then the space scene rewritten as Vegeta vs his clone, then the **first duel**
   (P1 Vegeta vs P2 clone, both Super Saiyan, no drain, to the death, draw to P1): the winner's
   pad plays Vegeta, the loser's pad plays Hercule, who meets Vegeta at the landing map. Zone 7
   robots, then Android 19 + base-form Android 13 (both-down rule: 19 explodes, 13 flees swearing
   revenge and hinting at Gero's "true plan"). Three-year skip gives Krillin and Hercule the same
   +1 level and refill as the others.
4. Piccolo's chapter: Hercule default P2, pad free. Gero's lab door: Gero + 13 (13 leaves after 17
   and 18 kill Gero inside). The road: Piccolo + Vegeta vs 17 + 18 (Vegeta forced as P2 for the
   fight), stock ending (blast flattens both, Krillin's plea, the kiss, Vegeta storms off).
5. Lookout 1: Kami defers; Hercule stays at the Lookout. Gohan's house: Trunks / Gohan locked for
   Trunks' chapter (capsule, Cell's skin). Fusion scene: Kami boosts Hercule's "potential" (level +
   EXP) first, then fuses with Piccolo. Gingertown: Piccolo / Hercule vs Imperfect Cell + his
   Multi-Form duplicate (Tien's technique), Cell escapes.
6. Lab basement: Trunks / Krillin locked (plans, Cell larva, master computer, self-destruct).
   Kame House: Goku wakes (Piccolo is a scene actor), "meet me at the Lookout". Lookout: Vegeta
   and Trunks enter the chamber: **second duel** (Vegeta vs Trunks, same rules); winner's pad
   plays Piccolo, loser's pad plays Hercule. Island: **Android 16 boss** (rewritten dialogue: 16
   furious Goku is absent, Piccolo's trick), new frames + rockets + AI, ends at half health; Cell
   arrives. Krillin stays at Kame House (selectable there via the yard pad).
7. Vegeta out of the chamber: Vegeta / Trunks vs Semi-Perfect Cell + duplicate; Cell absorbs 18;
   Perfect Cell vs Ascended Trunks / Krillin (Vegeta's pad switches to Krillin after Vegeta's KO;
   Krillin's one-hit KO opens the ending) to half health, Cell laughs, announces the Games; 16 asks
   for repairs. After: Trunks / Krillin default, Vegeta stays selectable (his barriers), Piccolo on.
8. The ten-day wait: Capsule Corp hub; Roshi's gym as 2v2 duos (Yamcha & Tien, Hercule & Krillin,
   "come back with a different team" line) and the animal-ninja finale; Dragon Balls and side
   quests; Cooler's two-part bonus kept (Piccolo gate) with a Namekian servant beside him; optional
   Android 13 arctic encounter behind a Vegeta barrier (partner undecided; story flag for the
   post-Cell variant); a Broly side arc wanted (maps and scripts; sprite 159 ready).

**Next to decide:** Android 13's arctic partner; then the Cell Games (Hercule's stock "victory", Cell
Juniors, Gohan's win, Keep Goku), the ending, and the Buu's Fury phase (Super Saiyan Hercule beats
Buu, Goku + Hercule fusion, Vegito for Janemba; no Hercule reveal before then).

**Standing rules:** no Hercule-hero reveal until much later; one player controls the world map; a
boss that dies first only falls until its twin is down ("both-down"); some bosses end at half health
and are reused; a map actor whose sprite is player 2's current sprite is not spawned; duel losers
swap pads with the winner's role as above, no level handicap; Hercule and Krillin stay weaker.

**Built (2026-10-18, on main): every decided chapter, 1 through 8,** as data/story/*.json +
*.lscript (tools: `docs/scripts.md`), on the engine pieces in `docs/coop-design.md` (duels with the
CoopCall bridge, both-down / half-health / pair rules, duplicate-NPC removal, Krillin and old Hercule
playable, Android 13 / 16 / Namekian models, zone-15 maps). Tested per chapter with the headless
runner (`tools/headless/`); the space duel was also played in the fork with a real final blow.
Numbers chosen by Claude that Ryan may want to change: Kami's boost (+3 levels and a refill),
the arctic 13 (stat 114, half health), the gym duos (also fought in single-player).
Still open for Ryan: the rest of the ten-day wait (section 4g: Broly?), then the Cell Games, the
ending and the Buu phase. Android 13 returns after Cell (4g).

**Mechanics the story needs (status above):** player-vs-player duel mode + pad swap; both-down
and half-health boss rules in `enemies.c`; duplicate-actor suppression; single-pad world map; a
bytecode assembler for new scenes (the dumper exists, `tools/script_dump.py`); new enemy-stat rows
(Android 13, Multi-Form Cell, Cooler's Namekian = recoloured Piccolo, 2v2 gym duos, animal wave);
Android 16 fight set + rocket projectiles + AI; Android 13 art (not in either ROM); Krillin's real
row, portrait and abilities (Destructo Disc); old Hercule's row on sprite 158; Kami's boost script;
per-chapter selectable-flag edits; map tooling (chunk encoder, collision, map-table entries) for the
arctic, Broly's arena and a chamber interior, or a Buu's Fury map decode.

## 1. What is actually in the game (Android and Cell sagas)

1. **Prologue, Future Trunks.** The future first: Future Gohan trains Trunks (melee tutorial), the
   Androids kill Gohan, captions "Sixteen years from the present day". Flight Circle tutorial.
   "Present day Earth": Frieza and King Cold land; Trunks cuts them down and warns Goku about the
   Androids.
2. **Three years later, Gohan.** Gohan at home (phone, Chi-Chi, the Mathbook), West City and Capsule
   Corp (Bulma, Dr. Briefs), the woods, Korin and Yajirobe (Senzu beans), Krillin's lines ("You let me
   win, didn't you?"). Errands: missing children, fishing bait, a newspaper, Hercule's autograph (!),
   Scouter parts, the Dragon Radar. Goku arrives ("Sorry I'm late... left turn at Mars").
3. **Androids.** Dr. Gero's lab with Blue/Red access cards and computer-room access, Androids 19/20,
   then 16/17/18 released; "You are now playing as Vegeta"; Vegeta levels; Piccolo joins (Namek
   lines, "Only Vegeta is allowed through this barrier" gates per character).
4. **Cell.** Ginger Town, Imperfect Cell, the absorptions, Perfect Cell; Cell Games with Hercule's
   public "victory"; Gohan's win; Cell's defeat dialogue, after which the script removes Goku and makes
   Hercule available (both one-byte-ish facts in the script).
5. **Side content.** Missing Nameks, Golden Capsules, "Eyes of the Lion" book, the Scouter minigame,
   Cooler's bonus area, technique unlocks per character (Super Kick, Spin Punch, Two Handed Smash).

Buu's Fury (separate ROM) continues seven years later through Majin Buu.

## 2. Decision (Ryan, 2026-10-17): "Canon plus Hercule"

The DBZ story stays beat for beat; Hercule is written into the scenes as the man who was also
there, and player 2 is whoever makes sense in the chapter (old Hercule in the future prologue,
Krillin in the Gohan chapters, Hercule from West City on). The twist is kept for the end: a surprise
Super Saiyan Hercule beats Majin Buu, and against Buu the fusion is Goku + Hercule instead of
Goku + Vegeta. Vegito stays for Janemba (Ryan's call; the film has Gogeta against Janemba, and
whether Buu's Fury has a Janemba fight at all is to be checked in that ROM). Hercule gains; nobody
else loses their moment.

## 3. Opening as the ROM actually plays it (from the dumped scripts)

1. Zone 1 (maps 0-6): the future. Future Gohan trains Trunks (melee tutorial), the Androids kill
   Gohan, captions "Sixteen years from the present day" and "Trunks and Gohan are the last...".
   Flight Circle tutorial. Then "Present day Earth... Gohan becomes impatient waiting for his father".
2. Map 14 (zone 2 area 8): **Gohan's dream**. Goku walks in, then Frieza ("Your dad isn't coming
   back!"), the first boss fight (enemy stat 41, sprite 48, Frieza's final form), Gohan wakes
   ("What a crazy dream... I can sense him"), the phone rings.
3. Map 17 (zone 2 area 20, home): Krillin on the phone ("Are you feeling what I'm feeling?"), the
   Saiyan Battle Armor hidden in a cave, Chi-Chi's study scenes; later Goku's "Sorry I'm late".
4. Map 52 (zone 3 area 4): Frieza (Mecha, sprite 50, stat 44) and King Cold (sprite 74) land.
   Krillin, Tien, Vegeta, Piccolo, Bulma and Yamcha stand around as NPCs; Trunks fights; Trunks:
   "I'm heading off to meet Goku. Would you guys like to come?"
5. Map 54 (zone 3 area 6): waiting for Goku (Krillin: "two energies headed this way"), Goku lands,
   Trunks' warning, caption "Goku tells his friends about the warning from the future", Trunks leaves.
6. Zone 4 West City (maps 65, 90, ...): Goku, Gohan and Piccolo arrive during the Hercule Day
   parade; the Mayor's "I called Hercule" speech, the club sandwich, the stolen key to the city
   (Hercule: "Who me? ... I can't!"); "Piccolo is now available!" at the Save Point. Back home:
   "Did you have a good time in West City?", "three years to train", caption "Three years later",
   Piccolo: "time to head to Amenbo Island".

## 4. Ryan's opening (2026-10-17) mapped onto that, with cost

Cost tags: **text** = dialogue swap (`tools/script_dump.py` records); **actor** = add/remove a map
actor record; **byte** = one script opcode/flag; **code** = new C; **art** = new frames; **engine**
= a data structure change.

| beat | Ryan's version | ROM facts and cost |
|---|---|---|
| Future prologue | keep Trunks as player 1; player 2 = **old Hercule** (Buu's Fury look), Trunks' buddy, comic relief, powerless, no hint of what he becomes; he rides the time machine back | text; new row + sprite for old Hercule. Buu's Fury's sprite tables use a different layout (LoG2's record signature does not appear in that ROM), so lifting its model needs a reversing pass first; recolouring LoG2's Hercule grey with the SSJ cloner is a day. **art/code** |
| Present day: Krillin visits | Krillin walks in instead of the phone; player 2 = Krillin from here | replace the phone trigger's sequence with a Krillin actor + dialogue in map 17; drop the dream map 14 from the flow. text, actor, byte |
| Forest "spirit" fight | Gohan and Krillin go into the forest (the Battle Armor cave gives the reason), fight Frieza **and Cooler**, told afterwards it was the spirit of the forest; no "it was a dream", no teleport home | reuse the dream map's Frieza boss record and add Cooler as its twin (sprite 34, stat 29 exists from the bonus area; the twin mechanism in `enemies.c` already pairs bosses). Rewrite the wake-up sequence into the spirit explanation and route the exit to the armor cave instead of home. text, actor, byte |
| Landing site | Krillin must not stand there as an NPC because player 2 is Krillin | delete the Krillin actor record in map 52 and give his line ("Who is this guy?") to Tien or to a player-2 bubble; same in map 54 ("two energies"). actor, text |
| Goku's arrival, Trunks leaves | as stock, plus old Hercule leaves with Trunks | one actor beside Trunks in map 54 and two lines. actor, text |
| West City | Gohan and Krillin fly there (world-map sign; both players fly, verified); Piccolo playable; **Hercule joins here** after the mansion deal (he wants the flying "magic trick", Piccolo needs a roof) | the stock scene already has Hercule Day, the Mayor and the key theft, so Hercule is on site; add the Piccolo/Hercule dialogue and a `SetCharacterInParty(Hercule)` next to the Piccolo unlock. text, byte. Goku is in West City too in the stock script (he waits outside City Hall); keep or trim his lines. |
| Triceratops Jungle | Piccolo's solo stretch (King Triceratops); player 2 = Hercule only, level 1 beside Piccolo (clear Krillin's selectable bit at the jungle entrance, restore it at home); Hercule claims credit for the dinosaur peace treaty | byte, text |
| Then | normal game with Gohan + Krillin/Hercule as player 2 | every later map where Krillin stands as an NPC (18 maps: Kame House, Amenbo Island, the Cell chapters) needs the NPC removed or his line reassigned when player 2 is Krillin. text/actor, mechanical |

## 4b. Vegeta's chapter: the clone duel (Ryan, 2026-10-17)

Stock flow (verified in the scripts): Amenbo Island, Yamcha found hurt, "Meanwhile..." the script
adds Vegeta and Piccolo to the party, makes Vegeta the only selectable character and loads the space
map (zone 6 area 50): the "I'm finally a Super Saiyan" scene plus the Super Saiyan tutorial, no
enemies. Warp to zone 6 area 22 (landing, one line, no enemies), then the world map into zone 7:
areas 1-15 of Gero's robot grunts, opening with the Goku / Android 20 scene (Goku coughing, 19
volunteering). Area 15: Vegeta vs Android 19 is a real boss fight (enemy stat 5, sprite 12), then
Android 20 flees, Vegeta leaves ("You're not needed anymore") and control passes to Piccolo
(`SetActiveCharacterNoReset 1`). Only-Vegeta barriers elsewhere are bonus areas.

Ryan's version:

1. **Space cutscene, rewritten.** Vegeta trains against his "shadow", a clone of himself. The clone
   lands a couple of punches; Vegeta's blast sends it into the rocks (explosion). Vegeta taunts it as
   an inferior copy. The clone goes Super Saiyan first: only one of them leaves alive, it is done
   living in his shadow; it knocks Vegeta into an explosion. Vegeta's monologue (his race enslaved,
   the true prince surpassed by a low-class Saiyan from Earth, and now a clone meant for poultry
   training reaches the legend; it is in his DNA, it is his legend, he takes what is his) and he
   transforms.
2. **The duel.** Player 1 controls Vegeta, player 2 the clone, both Super Saiyan, to the death.
3. **Result.** The winner plays Vegeta. One line, "VEGETA: I've won. I'm a Super Saiyan", fade to the
   landing map. Whoever won is the only player in control through the landing and into zone 7.
4. **Hercule arrives at the landing map** (zone 6 area 22, moved up from zone 7 so the loser
   sits out one scene, not fourteen robot areas): Hercule walks up: "Thank God he's arrived, we
   need you in the fight." Vegeta: who the hell are you? He is Hercule, Piccolo told him to wait
   here and watch for Vegeta. From here the loser of the duel is Hercule (the only selectable
   player-2 character), useless beside Vegeta through the robot zone and the Android 19 fight.
   No level handicap for the loser (Ryan): pride is the stake.
5. **The Android 19 fight stays Vegeta's, made two-on-two.** Stock zone 7 area 15 from the player's
   side: you (Vegeta) arrive on Goku dominating 19, Goku collapses from the heart virus, Yamcha
   carries him home, 19 asks to "finish Vegeta", you fight 19 as a boss (enemy stat 5, sprite 12),
   then 20 flees after a talk and Vegeta chases him ("You're not needed anymore"), and Piccolo
   becomes player 1. Nobody else fights 19. To keep it two-player (Ryan): **Android 13, base form,
   not yet "Super"**, fights beside 19 as its twin through the co-op twin mechanism (`enemies.c`
   pairs bosses of different models; it needs an enemy-stat row and a sprite for 13). 19 dies as in
   the story; 13 flees damaged and is not heard from for a while, which sets up a later side arc
   (in the film he absorbs 14 and 15 to become Super 13). 13's art is in Buu's Fury as an unused
   sprite, so this beat depends on decoding that ROM's sprite tables (also needed for old Hercule
   and the Buu phase); until then a recoloured LoG2 android (sprites 12-14 have the same
   enemy-grade pose sets) stands in. Fallback if the art never lands: Android 20 as the twin.
   Ryan's rules for the fight: 13 stands with 19 and 20 in the zone 7 area 1 scene (Goku coughing,
   19 volunteering) and is introduced there; both players fight 13 and 19 together; a boss that
   dies first only falls to the ground, and only when both are down does 19 explode and 13 fly off,
   swearing revenge and saying they do not know Dr. Gero's true plan. That "both down" rule is a
   small addition to the twin-boss code in `enemies.c` (downed state, no death action until the
   pair is dead).
6. **Three-year skip boosts.** Verified in the skip script (map 7): Gohan and Piccolo each get
   one level (`StackGetCharLevel + 1 -> SetCharacterLevel`) and a full HP/EP refill (500 caps to
   max), nothing more. Other scripted sets: Piccolo's unlock puts him at level 10, Vegeta's space
   scene at 18, Trunks at 27 when unlocked, the Time Chamber and the ending (Hercule to 40).
   Krillin and Hercule get the same +1 and refill at the skip (Ryan: boosted too, but kept weaker;
   their curves are in `docs/hercule.md`).

What it costs (no code yet):

| piece | mechanism | cost |
|---|---|---|
| the two cutscenes | dialogue records plus a scripted actor (Vegeta's sprite with a palette tint through the entity colour remap at +0x24, so the clone reads as a shadow), the opcodes already in the ROM for positions, flight, facing, fades, music and waits | text, actor, a small script assembler we do not have yet (the dumper exists; the encoder is the same format) |
| the clone as a character | one party slot "Vegeta's shadow" with Vegeta's row and abilities (needs the N-slot party, in progress), removed from the party after the duel | data, byte |
| player-versus-player hits | new in `src/coop`: player attacks and blasts test against the other player's entity, damage through the existing player-2 damage routing, hurt/knockback states, death ends the duel and fires the "I've won" script (`Map_SpawnScript`) | code, one to two days |
| the winner keeps Vegeta | swap the two pad sources in `input.c` when player 2 wins (one flag) | code, small |
| loser sits out | no selectable slot for player 2 means no player-2 entity (already how `pick_p2_slot` works) until the Hercule script sets his selectable bit | byte |
| Hercule meets Vegeta | actor + dialogue appended to zone 7 area 1's on-load scene, `SetCharacterFlag_Bit1(5)` | text, actor, byte |

Open: whether both players keep the Super Saiyan drain rules during the duel (the stock form is a
timed boost), and a draw rule if both die on the same frame (give it to player 1).

## 4c. Piccolo's chapter (Ryan, 2026-10-17): Hercule stays player 2

Stock flow (zone 8): after the Android 19 fight, Piccolo is player 1. Robot grunts throughout,
Dr. Gero at his lab door ("I won't let you get into my lab", two maps; **Gero is a boss there**,
enemy stat 7, sprite 14, with robot escorts), Gero activates 17 and 18, the Androids scene
(`SetActiveCharacter 1` keeps Piccolo; **a boss with enemy stat 4, sprite 11 is fought here**, to
identify in play, likely Android 17 vs Piccolo), Cell's lab under the computer (Krillin's
lines: "that thing is made up of cells from the greatest fighters", the self-destruct), Yajirobe's
Senzu drop, then the Lookout (zone 9 area 1) hands player 1 to Gohan. Krillin stands as an NPC in
zone 8 areas 47, 48 and 65 and talks through the lab scenes. Ryan: player 2 defaults to Hercule
here but the save pad keeps the other unlocked characters selectable, so instead of clearing
Krillin's bit we add one generic rule in `src/coop`: when a map spawns an actor whose sprite id is
player 2's current sprite, skip that actor (the dialogue still plays with its portrait). Then Krillin
as player 2 never meets his own NPC anywhere in the game. Hercule gets a line per scene: scared of
the lab door, hiding behind Piccolo when 17 and 18 wake. The chapter's fights, as scripted: Dr.
Gero at the lab door and Android 18 on the road (a fight the script ends with her "Nice try... who's
next?"); both become two-on-two through the twin mechanism.

**Gero's lab door (Ryan):** yes, the player fights Gero himself (Android 20, enemy stat 7, sprite
14) at the door, "I won't let you get into my lab", with robot escorts (three in zone 8 area 27, six
in area 40, the two map variants). Co-op makes it harder the way the difficulty code already does
(scaled stats, twins, extra escorts), and **Android 13 backs Gero up** as his twin: both-down rule
again, 13 flies off when the pair is beaten. Inside the lab (area 60) the stock scene plays: Gero
wakes 17 and 18, 17 takes and destroys the controller ("You're not going to put us to sleep again,
old man") and Gero dies at their hands; 13 watches, then leaves promising his master he will fulfil
his destiny. Hercule's line at the door: offers to "negotiate".

**The road (Ryan): Piccolo and Vegeta fight 17 and 18.** Stock, from the pad: entering the road
area hides Piccolo, spawns 17, 18 and 16 and plays the car talk; a winnable boss fight against 18
(enemy stat 4, sprite 11, 8000 EXP); then the script takes the win away (she gets up, blasts
Piccolo flat, "Nice try... who's next?"); Krillin pleads, 18 kisses him, the three fly off, a full
heal, Vegeta storms off, Piccolo leaves alone for the Lookout. In the series it is Vegeta who fights
18 on this road and loses, with Piccolo beaten too, so making it Piccolo + Vegeta against 17 + 18
is closer to the source than the stock scene: for the fight the script makes Vegeta the only
player-2 choice (whoever lost the duel gets Vegeta back here), 17 joins as 18's twin (needs an
enemy record; his fighting sprite is 9, a full enemy-grade set with attacks and hurt poses; 15 is
Android 16's NPC set), both-down rule,
and the blast knocks both players flat before the stock dialogue resumes. Hercule stands with
Krillin for the plea; the kiss stays Krillin's unless Ryan wants it on Hercule.

## 4d. After the road: Lookout, Trunks' chapter, the fusion, Imperfect Cell (stock flow, verified)

1. **World map as Piccolo** to the Lookout (zone 9 area 6): Kami refuses to fuse yet ("I need more
   time... I'll be waiting"). Cut to Gohan's house: the script adds Trunks, Piccolo and Vegeta to
   the party, sets Trunks to level 27, makes him player 1 and leaves only Gohan and Trunks selectable
   (Piccolo cleared). Krillin: get Goku to Roshi's; Bulma's satellite photo of a second time capsule
   near Gingertown; Gohan: "I'm coming with you, Trunks!"; caption "Trunks is now an available
   character! You are now playing as Trunks."
2. **Trunks' chapter** (zone 10): the capsule and Cell's shed skin with Bulma, Trunks and Gohan
   ("giant cicada?"), "head back to Roshi's". The script then makes Piccolo player 1 again.
3. **The fusion** (Lookout): Kami has felt "a terrible disturbance for four years"; the TV news from
   Gingertown (15,000 residents gone, piles of clothes), Krillin blames the Androids, Trunks
   doubts it; Kami agrees ("when you and I become one you will know"), Mr. Popo's goodbye.
4. **Gingertown** (zone 10 area 20): Piccolo meets Imperfect Cell (boss, enemy stat 60, sprite 28):
   Cell's Kamehameha, Piccolo's "I can't fight without my arm" trick to make Cell talk (time
   machine, the computer in the bunker, absorbing 17 and 18 for perfection), the fight, Krillin and
   Trunks arrive, Cell escapes, and Trunks is player 1 again.

Ryan's decisions for this stretch:

- **World map rule:** only the player who stepped on the flight sign controls the world map; the
  other player is a passenger (no second entity there). Noted in `docs/coop-design.md`.
- **Lookout, first visit:** Piccolo flies there with Hercule for the playable moments. When the
  script locks Piccolo out (he waits for Kami's answer), Hercule is locked out too and stays at the
  Lookout.
- **Trunks' chapter:** player 1 Trunks, player 2 Gohan, everyone else locked out (the stock flags
  already leave only Gohan and Trunks selectable; Krillin is busy moving Goku to Roshi's). Verified
  content: ordinary enemies (animals and robots, no boss), Yajirobe's Senzu, the time capsule,
  Cell's shed skin. Nothing in the story needs changing here.
- **The fusion scene, combined:** before Kami fuses with Piccolo he unlocks Hercule's "potential"
  for befriending and treating Piccolo well: a scripted level and EXP boost (`SetCharacterLevel`
  and `AddEXP`, the same opcodes the stock scripts use for Trunks and Vegeta), then the fusion plays
  as written. Piccolo and the boosted Hercule then go to Gingertown together for Imperfect Cell.
- Android 13 is loose after the road: bonus content.

## 4e. Gingertown and after (decisions and verified flow, 2026-10-18)

- **Gingertown (Ryan): Imperfect Cell fights with a Multi-Form duplicate.** Cell has Tien's cells,
  so the split is Tien's technique and Piccolo says so; Cell plus Cell through the twin mechanism,
  both-down rule: a body that falls is reabsorbed, and when both are beaten Cell pulls himself
  together and escapes as the stock scene already has him do. Piccolo and the Kami-boosted Hercule
  are the players; Krillin and Trunks arrive at the end as scripted. Cell Juniors stay for the
  Cell Games; Android 13 stays loose (his art is not in either ROM: drawn, or his role moves to an
  existing body, Ryan to decide).
- **After the fight** Piccolo sends Trunks and Krillin back to Dr. Gero's lab ("I'll explain
  everything to you, and then we have to go back to Dr. Gero's lab." / Krillin: "I'm on it!") and
  Trunks is player 1: ordinary areas (zone 10, "Only Trunks" barriers) to the **lab basement**
  (zone 8 areas 61-65) with Krillin beside him as a talking NPC: the Android design plans, Cell's
  larval form ("This has to be Cell in his larval form... I better destroy it before we have
  another Cell on our hands"), Dr. Gero's master computer, blasting it, the self-destruct escape
  ("WARNING! SECURITY BREACH! DESTRUCT SEQUENCE ACTIVATED!"). Story-wise player 2 here is
  **Krillin** (Ryan: the locked pair for this chapter is Trunks + Krillin; his NPC copy is removed
  by the duplicate-actor rule), while Hercule flies on with Piccolo.
- **From the pad, after the computer blows:** the self-destruct script warps Trunks and Krillin to the
  lab's outer area (zone 8 area 65, no dialogue), you walk out to the world map (one player flies,
  Ryan's rule) with the journal reading "Check on Goku's progress at Master Roshi's island". At
  Kame House (zone 11 area 1, then the upstairs room area 3) Goku wakes, "Kamiccolo", "I'm going to
  go find Vegeta. Meet me at Kami's Lookout", and you are put back outside the house. World map to
  the Lookout (zone 9 area 3): Goku's "only two of us can train", Vegeta and Trunks enter the
  chamber, Mr. Popo's rules, then Goku: "Piccolo, you should get back to Roshi's Island... when the
  Androids don't find me..." and the script makes **Piccolo player 1** (the script clears Trunks' and Gohan's
  selectable bits; Vegeta's was already cleared on the road when he stormed off), so in the stock
  game Piccolo is the only playable character here. For us: Hercule is in the party from West City
  on (Ryan: he belongs in the stock party), so player 2 at the Lookout is Hercule; Krillin stays
  home at Kame House when Trunks flies to the Lookout (the stock 17 scene spawns him there as an
  actor, with Maron around, and his next stock job is the detonator at Capsule Corp), so when
  Piccolo lands back on the island both Hercule and Krillin are selectable for the 17 fight. World map back to
  Roshi's island: 17, 18 and 16 are waiting, the island fight (areas 4, 5, 6), Cell's arrival, and
  the warp back to the Lookout where Vegeta comes out ("Vegeta has achieved Level %d", Tien: "Cell
  has swallowed up Android 17", Bulma's armour) and becomes player 1.
- **The same, in quest order** (the ROM's quest table, each quest opened by the previous one's
  completion flag): "Check on Goku's progress at Master Roshi's island" (Trunks and Krillin fly
  there: Goku wakes, "Kamiccolo", Goku leaves to train; Piccolo is a scene actor here) -> "Meet
  Goku at Kami's Lookout" (the Time Chamber intro: "only two of us can train", Vegeta and Trunks go
  in first; the script makes **Piccolo** player 1) -> "Intercept the Androids on Master Roshi's
  Island" (Piccolo's fight below) -> "Meet Trunks and Vegeta outside the Hyperbolic Time Chamber"
  (Vegeta becomes player 1) -> "Prevent Cell from absorbing Android 18" (Vegeta vs Cell, then
  Trunks vs Perfect Cell) -> "Meet Master Roshi at Capsule Corporation" -> Cell's announcement at
  the Briefs' -> the Dragon Balls for Dende -> "Enter the Cell Games". Then **Kame House** (zone 11): Piccolo asks
  after Goku, Goku wakes ("Kamiccolo!") and leaves to train; 17, 18 and 16 land ("Can Goku come out
  and play?"), Piccolo takes 17 to the next island, "you and me": **Piccolo vs Android 17** as a
  multi-map boss (enemy stat 3, sprite 9, the island breaking apart, 18 sitting it out), then 16
  detects two powers and **Cell arrives** ("Thanks for bringing 17 and 18 here"). The script warps
  to the Lookout.
- **Lookout / Time Chamber** (zone 9): Dende and the Dragon Balls hub with Gohan as player 1; the
  Hyperbolic Time Chamber scene makes Piccolo then Vegeta player 1.
- **Zone 12**: Vegeta vs (Semi-Perfect) Cell (boss, sprite 29; afterwards Trunks is player 1), Cell
  vs Trunks ("all you've done is pump up your muscles", Perfect Cell sprite 30, two boss records),
  robot and animal areas between; then zones 13-14, the Cell Games.

**Time Chamber duel (Ryan, 2026-10-18).** At the Lookout, when Vegeta and Trunks go in, fade to
black and cut to the two of them walking into the chamber. Trunks: let's get settled in. Vegeta
demands they spar right now. Cut to the two players facing each other: a second duel with the same
rules as the space one (both start as Super Saiyans, no drain, to the death, draw to player 1, no
level handicap). The winner's pad plays Piccolo in the next scene, the loser's pad plays Hercule.
Then fade back to the Lookout where Goku sends Piccolo to Roshi's island. The chamber interior
needs a map (see the next paragraph); the duel code is the same PvP work as the space scene.

Time Chamber interior: LoG2 has only the door on the
Lookout map ("You cannot enter the Hyperbolic Time Chamber"); no interior exists in this ROM.
**Built (2026-10-18, `docs/maps.md`):** zone 15 area 2, a white void with the living quarters (the
Lookout's palace facade) at the top edge and a door back to the Lookout. Buu's Fury's map format
turned out to be LoG2's; its round living-quarters rooms (records 0x082546A0 / 0x08254DA0) can be
imported the same way if wanted. While Vegeta and
Trunks are inside, the story-consistent playables are Piccolo (player 1 by script) with Hercule,
Krillin or Gohan (Gohan's selectable bit is cleared by the stock script; we can leave it set).

Kame House from the pad (verified): landing on the island does not start the scene; the 17 arrival
scene fires when you walk into the yard in front of the house, and the island has a save pad in
the yard (the same trigger signature as Gohan's house: condition `0x08010AC7`, function
`0x0800D839`), so a player can switch to Krillin at the pad before walking into the scene. At the
end of the scene the script flies you straight to the fight island (zone 11 area 4): no stop, no
small enemies (zone 11 has none), and the fight island has no pad. The boss continues across areas
4, 5 and 6 by script warps.

**Android 16 as the island boss (Ryan's idea, 2026-10-18):** change the dialogue so 16 is furious
that Goku is not there, and Piccolo tricks him: beat me on the next island and I'll tell you where
Goku is. 16 as a strong boss with rocket attacks: heat-seeking rockets that lock on and blow up
after three seconds, rocket barrages, big explosives all over the field, none of which hurt him.
Facts: LoG2's Android 16 (sprite 15) is a 16x32 NPC set with idle, walk and one blast pose only, and
he is not in Buu's Fury (he dies in the Cell saga), so a fighting 16 needs new frames (attacks,
rocket punch with the forearm detached, hurt) drawn or edited from a big body; the rockets need a
projectile sprite (no missile exists; the robots' shots and ki blasts are the nearest) and their
behaviour (lock-on, fuse, barrage, area explosions that spare the owner) is new boss AI in
`src/coop` (a few days of code once the art exists). 17's fight sprite (9) is complete, so 17 + 18
is the no-art alternative.

**Rule (Ryan):** nothing in the story reveals Hercule as the hero until much later. The prologue
caption rewrites from the first text swap ("Earth's true hope is a man with no ki at all" and
"Sixteen years before Hercule's rise") are removed; `data/hercule-text.json` is empty again.

**Decided (Ryan, 2026-10-18): Android 16 is the island boss.** The Kame House dialogue changes:
16 is furious that Goku is not there; Piccolo tricks him ("beat me on the next island and I'll tell
you where Goku is"); the fight on the island is Piccolo and player 2 (Hercule or Krillin) against
16 with new fighting frames (attacks, rocket punch with the forearm detached, hurt), rocket
projectiles (heat-seeking rockets that lock on and detonate after three seconds, rocket barrages,
big explosives across the field, none of which hurt 16) and a new boss AI for them. 17 and 18
watch; after the fight Cell arrives as scripted and the warp to the Lookout follows.

## 4f. Boss endings and the road to the Cell Games (Ryan, 2026-10-18)

- **Android 16 is not killed.** The island fight is hard but ends when 16 is down to about half
  health; Cell arrives and the stock warp follows. 16 is kept for later and fought again. The stock
  game already brings him back: after the Perfect Cell fight he asks Trunks "I need help. Please
  take me to someone who knows how to repair me... Once my circuits are repaired, I can fight
  alongside you", Trunks takes him to Capsule Corporation, and Bulma reads his memory there.
- **Semi-Perfect Cell vs Vegeta and Trunks:** Cell splits into two (Multi-Form duplicate), the same
  mechanics as Gingertown. The stock twist stays: Cell talks Vegeta into letting him absorb 18.
- **Perfect Cell vs Trunks and Krillin (Ryan):** in the stock scene Vegeta is the one knocked out
  ("That kick! I put all my energy into it!" and Cell's "now do you understand perfection?");
  Krillin is not in the scene at all. So: after Vegeta goes down, Krillin flies in (as he does in
  the series, detonator in hand) and the pad that played Vegeta switches to Krillin, selectable only
  with Trunks for this fight; Ascended Trunks and Krillin fight Perfect Cell with no twin, the fight
  ends at about half of Cell's health when he laughs it off ("All you've done is pump up your
  muscles"), and Krillin's rush-and-one-hit-KO from the series is the opening beat of the scripted
  ending. Trunks' transformation beats stay as scripted (the script resets both Saiyans to their
  base sprites afterwards).
- **After that, from the pad (verified):** Cell announces the tournament ("a tournament will begin in
  ten days... I'm giving you a second chance"); Trunks: "I should return to Capsule Corporation and
  tell the others"; 16 asks for repairs; Vegeta's "Foolish boy" and Cell's "I have fought you
  before... in the future" close the scene; warp to the world map. Then Capsule Corporation: the
  receptionist sends you to Master Roshi in the atrium; Bulma and Dr. Briefs over 16's memory
  ("loaded with Goku's data... designed for the sole purpose of destroying Goku"); Cell's TV
  announcement at the Briefs' residence ("The Cell Games will be held nine days from today near the
  Northern Wastelands"); the Scouter Note; the Dragon Ball hunt for Dende; "Enter the Cell Games".

- **After Perfect Cell (Ryan):** Vegeta stays selectable (his barriers are needed to progress),
  the default pair is Trunks + Krillin since they are already in hand, and the pad lets players
  change later.
- **The wait for the Cell Games (stock, verified):** Capsule Corporation is the hub: Master Roshi in
  the atrium offers sparring matches (zone 6 area 70: Yamcha, Krillin and Tien as one-on-one
  "enemies" with +3 stat capsules as prizes; quest "Train with Master Roshi"); Bulma's Dragon Radar
  and the seven Dragon Balls scattered across the world map (quest "Collect the seven Dragon
  Balls", then "Return the Dragon Balls to Dende on the Lookout", who counts them: "You've found %d
  Dragon Balls! Keep looking!"); side quests that open in this window: Dr. Briefs' 25 Golden
  Capsules, the seven Missing Nameks, the Warlord's kidnapping, the Capsule Corp espionage, the
  Scouter parts. Then "Enter the Cell Games". Co-op note: Krillin is one of Roshi's sparring
  opponents (enemy stat 64, sprite 76); when player 2 is Krillin that match swaps its opponent
  (Hercule is the obvious stand-in) or is skipped by the duplicate-actor rule.

**Roshi's gym, redesigned (Ryan, 2026-10-18).** Stock facts: the sparring room (zone 6 area 70)
has one spot where a single opponent stands; you talk to the opponent to start, each win sets a
story flag and the next one appears (Yamcha, then Krillin, then Tien); Roshi is not in the room
(his line elsewhere explains everyone moved to Capsule Corp during the Cell crisis). Ryan's
version: the matches are **2 v 2**: the two players against Yamcha & Tien, then against Hercule &
Krillin (each win still pays a +3 capsule). If one player is currently Hercule or Krillin, only
the other of the pair stands in the room and talking to him gives "Sorry, Master Roshi says I can
only fight duos with Krillin on my team right now" (or "...with Hercule..."), so the players come
back with different characters. When both duos are beaten, the **final fight**: Roshi says he
hired some special animal ninjas for a different kind of training and releases one of every wild
animal into the room (alligator, hawk, ladybug, pterodactyl, sabertooth tiger, scorpion, snake,
T-rex, triceratops) plus two ninjas; clear the room for the last capsule. Cost: the twin-spawn
code already pairs enemies; a wave spawn of mixed enemy records and the room dialogue are new
script and a little code; every animal above is a stock enemy type (ids to be confirmed in the
atlas), the "ninjas" map to the Warlord's bandit sprites.

**Side-quest window (what the series did in those ten days):** the Broly film is set in exactly
this gap (Paragus lures Vegeta to "New Planet Vegeta" while the Z fighters wait for the Cell
Games), and Legendary Super Saiyan Broly's full fighting sprite is already converted from Buu's
Fury (LoG2 id 159), so a Broly side arc here needs maps and script but no new art. Cooler's stock
bonus area (zone 4 area 50 / zone 6 area 61) is gated by West City-era story flags, not the
ending; check in play when it opens.

**Side quests in the wait (Ryan, 2026-10-18):**

- **Cooler (Ryan: keep the Piccolo gate as is, make sure it is reachable now, and make the fight
  two-player).** Facts: Cooler is a two-part bonus. First meeting: zone 4 area 50, reached from
  Triceratops Jungle (map 103) behind an "Only Piccolo" barrier, gated by West City flag 60:
  "Frieza??? But how?" / "I am not Frieza. I am his brother Cooler... the last surviving member of
  my family. You and your friends killed my father and my brother." The fight: zone 6 area 61
  (boss, enemy stat 29, sprite 34) behind another Piccolo barrier (area 62), with "You're not
  Goku... I thought I specifically asked for Goku!" There is no level gate and no Goku gate; with
  Piccolo selectable in the Cell Games wait both parts are reachable now. Co-op: Cooler fights
  beside **a strong Namekian servant**, explained in the dialogue (a Namekian warrior taken from
  Namek by Frieza's family and kept by Cooler); art: Piccolo's complete playable set (103/104)
  recoloured through the entity colour remap, or a cloned record with a changed palette, so no new
  frames. Both-down rule.
- **Android 13 in the arctic (progress, not finish):** a new or re-dressed arctic area reached
  through a new "Only Vegeta" barrier; the party goes there to find something and runs into 13,
  who says HE was supposed to absorb 17 and 18, not Cell, his chance at perfection is ruined and he
  would kill Cell if he could; Vegeta: he won't get the chance, because he'll be dead. A fight to
  half health, 13 leaves. Two-player (Ryan): 13 needs a partner; candidates with no new art are
  one of Gero's large robots (sprites 37-39) under 13's command, or a spare Android 19 unit
  recoloured; Androids 14 and 15 (his film sidekicks, whom Super 13 absorbs) would be the best
  story but need drawing. Optional: a story flag records whether it happened, and the post-Cell
  Super 13 encounter has two openings (met before / first meeting). Facts: zone 13 is a stock
  "Snowy Highlands" zone (10 snow maps with falling snow), which the dialogue never names. **Built
  (2026-10-18, `docs/maps.md`):** zone 15 area 1, an arctic snowfield (Northern Mountains zone 8
  area 28 re-dressed in snow and ice, falling snow) with a cave behind a working "Only Vegeta"
  barrier (level 50, story flag 380); the cave door has no exit yet, for 13's encounter.
- **Broly (maps and scripts wanted):** maps built (2026-10-18, `docs/maps.md`): zone 15 area 3, a
  wasteland basin with a pod crater (LoG2 tiles), and area 4, a walled crater imported from Buu's
  Fury (same map format; built when `roms/buu.gba` is present). Buu's Broly battle map itself is not
  identified (its map table is not found). Broly's
  fight sprite is ready (id 159).

Half-health boss endings are one rule in `enemies.c` (a boss with an "ends at N%" flag fires its
defeat action at that threshold instead of at zero HP and becomes untargetable).

## 4g. The ten-day wait, revised (Ryan, 2026-10-18)

- **Android 13 is held back until after Cell.** He absorbs 17 and 18 to become Super 13, so his
  return comes after the Dragon Ball wish brings everyone (17 and 18 included) back to life. The
  arctic encounter built in chapter 8 comes out of the wait; his earlier appearances (beside 19,
  beside Gero, watching in the lab) stay.
- **Kept as planned:** Roshi's 2v2 gym and its finale; Cooler with his Namekian servant, reachable
  in the wait (the gate is the stock "Only Piccolo" barrier; there is no level gate).
- **Roshi's dojo sparring (Ryan):** once the players have cleared all three dojo matches and Roshi
  has handed over the last +3 capsule, they can ask him to use the dojo to spar 1 v 1, players only
  (the duel mode, no story stakes, as many times as they like).
- **Dragon Ball race:** dropped (Ryan).
- **Time Chamber training waves (Ryan: approved, to be scoped thoroughly):** at Kami's Lookout the
  players can enter the Hyperbolic Time Chamber (zone 15 area 2) and fight waves of enemies to level
  up instead of grinding in the wild. Decided: it opens at the start of the ten-day wait (after Cell's
  announcement; the game never shows Goku and Gohan coming out); Mr. Popo on the Lookout gets a line
  pointing the players to it; 10 rounds in fixed tiers, harder enemies as the rounds go up (Ryan;
  the enemy list per round to be mapped); the players can walk out the door at any time; after round 10 the spawning
  stops and a caption says "Training completed!". Repeatable as many times as the players want. Limit to respect: sprite memory, not a fixed
  count; in co-op every regular enemy brings a twin, so about 4 per wave (8 on screen) is the working
  number until it is measured. Open: round display between waves, and any reward beyond EXP.
- **Android 16 side chapter (Ryan, idea, 2026-10-18):** a chapter in the wait with the players locked
  to Android 16 and Krillin. Facts: the game never shows 16 repaired. Right before Cell's TV
  announcement, Bulma and Dr. Briefs read his memory in Dr. Briefs' workshop (zone 6 area 5, quest
  "Talk to Bulma in Dr. Briefs' workshop"); he is next seen standing at the Cell Games arena, and
  Krillin reveals there that they removed his bomb while repairing him. So he is fixed some time in
  the ten days, off screen. Where the players meet him and where the chapter goes: Ryan to decide.
  Making 16 playable is a new party character like Krillin (player poses, row, abilities); his
  portrait exists (speaker 26) and his fighting sprite (160) is enemy-grade.
- **Broly side chapter (Ryan, 2026-10-18), in the wait:**
  - *Trigger:* right after Cell's TV announcement in the Briefs' residence (zone 6 area 16, where
    Bulma already hands over the Scouter Note), Bulma adds: tell Vegeta a ship is arriving for him in
    the backyard, and it looks like a Saiyan ship.
  - *Backyard:* the Capsule Corp lawn (zone 6 area 21, the training yard east of the grounds). With
    Vegeta as a player, a spaceship stands there (an existing model copied in); Paragus steps out and
    says he has found a New Planet Vegeta for him to rule as king. Vegeta goes aboard; Trunks follows
    with Krillin, Gohan and Piccolo. The ship leaves.
  - *Arrival:* cut to the Broly arena (zone 15 area 3), expanded with a landing zone where the ship
    model stands, and a save pad there where the players can switch to anyone.
  - *The fight:* walking into the next area, everyone appears on screen for the stock-style dialogue
    (not dynamic), then only player 1 and player 2 fight Broly and Paragus. Broly is the real boss and
    much stronger; Paragus is weak and easy. If Paragus dies first, Broly's health refills once and he
    becomes stronger and faster (rage at his father's death).
  - *Models (Ryan):* Broly appears only as the Legendary Super Saiyan (sprite 159, from Buu's Fury),
    already waiting in the arena; no base-form Broly. Paragus = Vegeta's model in Saiyan armor
    (his playable set, sprites 137-140) made old: grey hair and a grey goatee, as a generated
    recolour/edit of every frame (like tools/a16sprites.py), plus a matching edit of Vegeta's
    dialogue portrait.
  - *End:* after both fall, a huge explosion, a short scene back at Capsule Corp with everyone talking
    it over, fade to black, and player 1 and player 2 are at Capsule Corp with the game back to normal.

## 5. What the plan is missing (engine limits found while checking)

1. **Six party slots.** `PartyState` holds exactly 6 characters (Gohan, Vegeta, Goku, Piccolo,
   Trunks, Hercule): default party, growth rows, EXP and the save all assume 6. Krillin as a real
   party member is a 7th. Options: (a) grow the party to 8 slots: `engine` work touching the party
   struct in IWRAM, the save layout, the switch screen and the level-up tables, maybe two days;
   (b) Krillin borrows Trunks' slot while Trunks is away (Trunks is only player 1 in the prologue
   and playable again in the Cell chapter): row swap per chapter, stats shared with Trunks, cheap
   but a hack that breaks when both are needed. Old Hercule in the prologue can borrow Hercule's
   slot (same slot, different row) at no cost.
2. **Krillin's poses.** Sprite 76 has 13 groups: idle, walk, run, two attacks, charge, hurt, fly
   and the blast pose (12). Hercule's playable set has 21 with extra attack groups 7/8, 11, 13 and
   the knocked-out pose 20. Either draw those (art) or test what the engine does with an empty
   attack group (it checks group pointers for 0 in at least one place).
3. **Destructo Disc.** No disc projectile sprite is in LoG2; a new projectile needs frames (art) and
   an ability pair (code). Stock ki blasts can be recoloured for Krillin at no art cost.
4. **Old Hercule's model** (see the table): Buu's Fury needs its own sprite-format pass before any
   of its art can be lifted.
5. **Dialogue capacity.** New lines are written into free ROM space (`data/hercule-text.json`
   records), so length is not a problem; speaker portraits are limited to the ids the game has
   (Krillin 73, Hercule 67, Piccolo 3/90, Trunks 98).

## 6. Buu's Fury (phase 2, separate ROM)

- The aura from the Cell Games is real; when Goku needs a fusion partner against Buu, Hercule is
  the one there: a new row and sprite for the fused form in the Buu ROM, built with the same
  cloner once that ROM has its decompile pass. Vegito stays for Janemba (check that fight exists).
- The final blow on Buu is Super Saiyan Hercule's, with the Spirit Bomb beat kept as his crowd call.
- Needs: Buu's Fury decompile pass (party, display rows, sprites, saves), save conversion from
  LoG2, the fusion scene as a script edit, two new sprites.

## 7. Still open

- Save-pad switching with only two characters already works as a trade (player 1 taking
  player 2's character hands player 2 the old one), noted for the UI.

- Party slots: decided, grow to 8 (Ryan, 2026-10-17); plan and site inventory in `docs/party-8-slots.md`.
- Super Saiyan look: gold afro (what we have) or a different "Super Human" look?
- Name of the Goku + Hercule fusion in our text ("Gokule" is the fan name).
