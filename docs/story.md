# The story as the ROM tells it, and how "The Legacy of Hercule" bends it

Source: the game's own caption cards and dialogue (dumped per map by `tools/script_dump.py` into
`build/scripts/`, map order follows play order closely), plus the quest table. Chapter names are ours.

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
   becomes player 1. Nobody else fights 19. To keep it two-player: Android 20 (sprite 13, an actor
   with no enemy record in the stock game) joins as 19's twin through the co-op twin mechanism
   (`enemies.c` already pairs bosses of different models; it needs one new enemy-stat row for
   sprite 13, say Gero's lab-door stats, stat 7). After both fall, 20 "escapes damaged" so the stock
   dialogue and the chase still read. Super Android 13 from Buu's Fury is saved for a side story:
   that ROM's sprite format is not decoded yet and 13 deserves his own arc.

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
zone 8 areas 47, 48 and 65 and talks through the lab scenes, so while Hercule is player 2 Krillin's
selectable bit stays clear (set it again when the party next meets at the Lookout). Hercule gets a
line per scene: scared of the lab door, hiding behind Piccolo when 17 and 18 wake, "I knew it" at
Cell's tank. Cost: text and a few bytes.

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
