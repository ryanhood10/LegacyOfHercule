# The story as the stock ROM tells it

What the unmodified *Legacy of Goku II* does, chapter by chapter, as checked while planning "The
Legacy of Hercule". The plan itself is `docs/story.md` (section numbers there are cited below as
"plan 4b" and so on); superseded options are in `docs/story-archive.md`.

Sources: the game's own caption cards and dialogue, dumped per map by `tools/script_dump.py` into
`build/scripts/` (`python tools/script_dump.py map ZONE AREA`; file names `map_NNN_zZ_aA.txt`, so map
numbers and zone / area pairs can be read off the file list), plus the quest table and Ghidra. Map
order follows play order closely. Chapter names are ours. "Verified" means checked in the dumped
scripts or from the pad in play, on the date given.

## 1. Overview: the Android and Cell sagas (2026-10-17)

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

Zones 1-14 are used; zone 13 is a stock "Snowy Highlands" zone (10 snow maps with falling snow),
which the dialogue never names (2026-10-18, `docs/maps.md`). Zone 15 is ours (new maps).

## 2. Opening and West City (chapters 1-2; plan 4) (2026-10-17)

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
   "I'm heading off to meet Goku. Would you guys like to come?" Krillin: "Who is this guy?"
5. Map 54 (zone 3 area 6): waiting for Goku (Krillin: "two energies headed this way"), Goku lands,
   Trunks' warning, caption "Goku tells his friends about the warning from the future", Trunks leaves.
6. Zone 4 West City (maps 65, 90, ...): Goku, Gohan and Piccolo arrive during the Hercule Day
   parade; the Mayor's "I called Hercule" speech, the club sandwich, the stolen key to the city
   (Hercule: "Who me? ... I can't!"); "Piccolo is now available!" at the Save Point. Goku waits
   outside City Hall. Back home: "Did you have a good time in West City?", "three years to train",
   caption "Three years later", Piccolo: "time to head to Amenbo Island".

More facts from the opening's planning (2026-10-17):

- Both players fly to West City from the world-map sign (verified).
- Cooler exists as sprite 34, stat 29 (the bonus area), and the twin code in `enemies.c` pairs
  bosses of different models, which the forest spirit fight uses.
- Krillin stands as an NPC on 18 maps (Kame House, Amenbo Island, the Cell chapters), which is why
  the duplicate-NPC rule exists.
- Speaker portraits are limited to the ids the game has: Krillin 73, Hercule 67, Piccolo 3 / 90,
  Trunks 98 (plus the ones we added, `docs/scripts.md`). New lines go to free ROM space, so length
  is not a limit.

## 3. Vegeta's chapter (chapter 3; plan 4b) (verified 2026-10-17)

Stock flow: Amenbo Island (zone 5), Yamcha found hurt, "Meanwhile..." the script adds Vegeta and
Piccolo to the party, makes Vegeta the only selectable character and loads the space map (zone 6
area 50, map 168): the "I'm finally a Super Saiyan" scene plus the Super Saiyan tutorial, no enemies.
Warp to zone 6 area 22 (map 166, the landing, one line, no enemies), then the world map into zone 7:
areas 1-15 of Gero's robot grunts, opening with the Goku / Android 20 scene in zone 7 area 1 (Goku
coughing, 19 volunteering). Area 15: you (Vegeta) arrive on Goku dominating 19, Goku collapses from
the heart virus, Yamcha carries him home, 19 asks to "finish Vegeta", and Vegeta vs Android 19 is a
real boss fight (enemy stat 5, sprite 12); then Android 20 flees after a talk, Vegeta chases him
("You're not needed anymore") and control passes to Piccolo (`SetActiveCharacterNoReset 1`). Nobody
else fights 19. Only-Vegeta barriers elsewhere are bonus areas.

**Three-year skip (map 7, verified):** Gohan and Piccolo each get one level (`StackGetCharLevel + 1 ->
SetCharacterLevel`) and a full HP/EP refill (500 caps to max), nothing more. Other scripted level
sets: Piccolo's unlock puts him at level 10, Vegeta's space scene at 18, Trunks at 27 when unlocked,
the Time Chamber and the ending (Hercule to 40).

## 4. Piccolo's chapter (chapter 4; plan 4c) (2026-10-17)

Stock flow (zone 8): after the Android 19 fight, Piccolo is player 1. Robot grunts throughout,
Dr. Gero at his lab door ("I won't let you get into my lab"; **Gero is a boss there**, enemy stat 7,
sprite 14, with robot escorts: three in zone 8 area 27, six in area 40, the two map variants), Gero
activates 17 and 18, the Androids scene (`SetActiveCharacter 1` keeps Piccolo), Cell's lab under the
computer (Krillin's lines: "that thing is made up of cells from the greatest fighters", the
self-destruct), Yajirobe's Senzu drop, then the Lookout (zone 9 area 1) hands player 1 to Gohan.
Krillin stands as an NPC in zone 8 areas 47, 48 and 65 and talks through the lab scenes.

Inside the lab (area 60): Gero wakes 17 and 18, 17 takes and destroys the controller ("You're not
going to put us to sleep again, old man") and Gero dies at their hands.

**The road, from the pad:** entering the road area hides Piccolo, spawns 17, 18 and 16 and plays the
car talk; a winnable boss fight against 18 (enemy stat 4, sprite 11, 8000 EXP; the early note "a boss
with stat 4, sprite 11, likely 17" is this fight); then the script takes the win away (she gets up,
blasts Piccolo flat, "Nice try... who's next?"); Krillin pleads, 18 kisses him, the three fly off, a
full heal, Vegeta storms off, Piccolo leaves alone for the Lookout.

Sprites: Android 17's fighting sprite is 9, a full enemy-grade set with attacks and hurt poses
(stat 3). Android 16 is sprite 8 (idle, walk, hit-stun, one punch, knock-down; the island scenes'
actor); sprite 15 is Dr. Gero's NPC set. (Corrected 2026-10-18: earlier notes called 15 Android 16.)

## 5. Lookout, Trunks' chapter, fusion, Gingertown (chapter 5; plan 4d) (verified 2026-10-17)

1. **World map as Piccolo** to the Lookout (zone 9 area 6): Kami refuses to fuse yet ("I need more
   time... I'll be waiting"). Cut to Gohan's house: the script adds Trunks, Piccolo and Vegeta to
   the party, sets Trunks to level 27, makes him player 1 and leaves only Gohan and Trunks selectable
   (Piccolo cleared). Krillin: get Goku to Roshi's; Bulma's satellite photo of a second time capsule
   near Gingertown; Gohan: "I'm coming with you, Trunks!"; caption "Trunks is now an available
   character! You are now playing as Trunks."
2. **Trunks' chapter** (zone 10): the capsule and Cell's shed skin with Bulma, Trunks and Gohan
   ("giant cicada?"), "head back to Roshi's". Ordinary enemies (animals and robots, no boss),
   Yajirobe's Senzu. The script then makes Piccolo player 1 again.
3. **The fusion** (Lookout): Kami has felt "a terrible disturbance for four years"; the TV news from
   Gingertown (15,000 residents gone, piles of clothes), Krillin blames the Androids, Trunks
   doubts it; Kami agrees ("when you and I become one you will know"), Mr. Popo's goodbye.
4. **Gingertown** (zone 10 area 20): Piccolo meets Imperfect Cell (boss, enemy stat 60, sprite 28):
   Cell's Kamehameha, Piccolo's "I can't fight without my arm" trick to make Cell talk (time
   machine, the computer in the bunker, absorbing 17 and 18 for perfection), the fight, Krillin and
   Trunks arrive, Cell escapes, and Trunks is player 1 again.

## 6. Lab basement to the island (chapter 6; plan 4e) (verified 2026-10-18)

**After Gingertown:** Piccolo sends Trunks and Krillin back to Dr. Gero's lab ("I'll explain
everything to you, and then we have to go back to Dr. Gero's lab." / Krillin: "I'm on it!") and
Trunks is player 1: ordinary areas (zone 10, "Only Trunks" barriers) to the **lab basement** (zone 8
areas 61-65) with Krillin beside him as a talking NPC: the Android design plans, Cell's larval form
("This has to be Cell in his larval form... I better destroy it before we have another Cell on our
hands"), Dr. Gero's master computer, blasting it, the self-destruct escape ("WARNING! SECURITY
BREACH! DESTRUCT SEQUENCE ACTIVATED!").

**From the pad, after the computer blows:** the self-destruct script warps Trunks and Krillin to the
lab's outer area (zone 8 area 65, no dialogue), you walk out to the world map with the journal reading
"Check on Goku's progress at Master Roshi's island". At Kame House (zone 11 area 1, then the upstairs
room area 3) Goku wakes, "Kamiccolo", "I'm going to go find Vegeta. Meet me at Kami's Lookout", and
you are put back outside the house. World map to the Lookout (zone 9 area 3): Goku's "only two of us
can train", Vegeta and Trunks enter the chamber, Mr. Popo's rules, then Goku: "Piccolo, you should get
back to Roshi's Island... when the Androids don't find me..." and the script makes **Piccolo player
1** (it clears Trunks' and Gohan's selectable bits; Vegeta's was already cleared on the road when he
stormed off), so in the stock game Piccolo is the only playable character here. The stock 17 scene
spawns Krillin at Kame House as an actor, with Maron around; his next stock job is the detonator at
Capsule Corp. World map back to Roshi's island: 17, 18 and 16 are waiting, the island fight (areas 4,
5, 6), Cell's arrival, and the warp back to the Lookout where Vegeta comes out ("Vegeta has achieved
Level %d", Tien: "Cell has swallowed up Android 17", Bulma's armour) and becomes player 1.

**The same, in quest order** (the ROM's quest table, each quest opened by the previous one's
completion flag): "Check on Goku's progress at Master Roshi's island" (Trunks and Krillin fly there:
Goku wakes, "Kamiccolo", Goku leaves to train; Piccolo is a scene actor here) -> "Meet Goku at Kami's
Lookout" (the Time Chamber intro: "only two of us can train", Vegeta and Trunks go in first; the
script makes Piccolo player 1) -> "Intercept the Androids on Master Roshi's Island" -> "Meet Trunks
and Vegeta outside the Hyperbolic Time Chamber" (Vegeta becomes player 1) -> "Prevent Cell from
absorbing Android 18" (Vegeta vs Cell, then Trunks vs Perfect Cell) -> "Meet Master Roshi at Capsule
Corporation" -> Cell's announcement at the Briefs' -> the Dragon Balls for Dende -> "Enter the Cell
Games".

**Kame House** (zone 11): Piccolo asks after Goku, Goku wakes ("Kamiccolo!") and leaves to train; 17,
18 and 16 land ("Can Goku come out and play?"), Piccolo takes 17 to the next island, "you and me":
**Piccolo vs Android 17** as a multi-map boss (enemy stat 3, sprite 9, the island breaking apart, 18
sitting it out), then 16 detects two powers and **Cell arrives** ("Thanks for bringing 17 and 18
here"). The script warps to the Lookout.

**Kame House from the pad (verified 2026-10-18):** landing on the island does not start the scene;
the 17 arrival scene fires when you walk into the yard in front of the house, and the island has a
save pad in the yard (the same trigger signature as Gohan's house: condition `0x08010AC7`, function
`0x0800D839`), so a player can switch to Krillin at the pad before walking into the scene. At the end
of the scene the script flies you straight to the fight island (zone 11 area 4): no stop, no small
enemies (zone 11 has none), and the fight island has no pad. The boss continues across areas 4, 5 and
6 by script warps.

**Lookout / Time Chamber** (zone 9): Dende and the Dragon Balls hub with Gohan as player 1; the
Hyperbolic Time Chamber scene makes Piccolo then Vegeta player 1. LoG2 has only the chamber door on
the Lookout map ("You cannot enter the Hyperbolic Time Chamber"); no interior exists in this ROM (ours
is zone 15 area 2). Buu's Fury's map format turned out to be LoG2's; its round living-quarters rooms
(records 0x082546A0 / 0x08254DA0) could be imported the same way.

**Zone 12:** Vegeta vs (Semi-Perfect) Cell (boss, sprite 29; afterwards Trunks is player 1), Cell vs
Trunks ("all you've done is pump up your muscles", Perfect Cell sprite 30, two boss records, stats 78
and 79), robot and animal areas between; then zones 13-14, the Cell Games.

## 7. Perfect Cell and after (chapter 7; plan 4f) (verified 2026-10-18)

In the stock Perfect Cell scene Vegeta is the one knocked out ("That kick! I put all my energy into
it!" and Cell's "now do you understand perfection?"); Krillin is not in the scene at all. Trunks'
transformation beats are scripted, and the script resets both Saiyans to their base sprites
afterwards.

**After that, from the pad:** Cell announces the tournament ("a tournament will begin in ten days...
I'm giving you a second chance"); Trunks: "I should return to Capsule Corporation and tell the
others"; 16 asks Trunks for repairs ("I need help. Please take me to someone who knows how to repair
me... Once my circuits are repaired, I can fight alongside you"); Vegeta's "Foolish boy" and Cell's
"I have fought you before... in the future" close the scene; warp to the world map. Then Capsule
Corporation: the receptionist sends you to Master Roshi in the atrium; Bulma and Dr. Briefs over 16's
memory ("loaded with Goku's data... designed for the sole purpose of destroying Goku"); Cell's TV
announcement at the Briefs' residence ("The Cell Games will be held nine days from today near the
Northern Wastelands"); the Scouter Note; the Dragon Ball hunt for Dende; "Enter the Cell Games".

## 8. The ten-day wait (chapter 8; plan 4f, 4g)

**The wait (verified 2026-10-18):** Capsule Corporation is the hub. Master Roshi in the atrium offers
sparring matches (zone 6 area 70: Yamcha, Krillin and Tien as one-on-one "enemies" with +3 stat
capsules as prizes; quest "Train with Master Roshi"; Krillin is enemy stat 64, sprite 76). Bulma's
Dragon Radar and the seven Dragon Balls scattered across the world map (quest "Collect the seven
Dragon Balls", then "Return the Dragon Balls to Dende on the Lookout", who counts them: "You've found
%d Dragon Balls! Keep looking!"). Side quests that open in this window: Dr. Briefs' 25 Golden
Capsules, the seven Missing Nameks, the Warlord's kidnapping, the Capsule Corp espionage, the Scouter
parts. Then "Enter the Cell Games".

**Roshi's sparring room (2026-10-18):** the room (zone 6 area 70) has one spot where a single opponent
stands; you talk to the opponent to start, each win sets a story flag and the next one appears
(Yamcha, then Krillin, then Tien); Roshi is not in the room (his line elsewhere explains everyone moved
to Capsule Corp during the Cell crisis). The bestiary table at 0x086F8E40 ({name, description, sprite,
portrait, stat}, 128 entries) gives every enemy's sprite and stat.

**Cooler (2026-10-18):** a two-part bonus. First meeting: zone 4 area 50, reached from Triceratops
Jungle (map 103) behind an "Only Piccolo" barrier, gated by West City flag 60: "Frieza??? But how?" /
"I am not Frieza. I am his brother Cooler... the last surviving member of my family. You and your
friends killed my father and my brother." The fight: zone 6 area 61 (map 172; boss, enemy stat 29,
sprite 34) behind another Piccolo barrier (area 62), with "You're not Goku... I thought I specifically
asked for Goku!" There is no level gate and no Goku gate, so with Piccolo selectable in the Cell Games
wait both parts are reachable. Piccolo's complete playable set is sprites 103 / 104.

**The Broly film's timing:** the film is set in exactly this gap (Paragus lures Vegeta to "New
Planet Vegeta" while the Z fighters wait for the Cell Games). Legendary Super Saiyan Broly's full
fighting sprite is converted from Buu's Fury (LoG2 id 159). Buu's Fury uses LoG2's map format (401 of
its maps render) but its map table is not found, so Buu's own Broly battle map is not identified
(2026-10-18). In the Briefs' residence (zone 6 area 16, map 160) Bulma's records end at flags 68 /
151, so she is not there after Perfect Cell (2026-10-19). The backyard is zone 6 area 21 (map 165).

**Facts behind "Gero's computer" (2026-10-19):** the game never shows 16 repaired. On the island he
asks for repairs; Bulma and Dr. Briefs read his memory in Dr. Briefs' workshop (zone 6 area 5, map
149) just before Cell's TV announcement; he is next seen standing at the Cell Games arena, where
Krillin says they removed his bomb. The workshop map places no 16 actor (no sprite 8). Cell tells
Piccolo that Gero's computer "was hidden in a bunker deep below Gero's lab". Krillin has been in the
lab basement (zone 8 areas 48 and 61-63), and 16 was built in that lab.

**World map (2026-10-19):** the world map's on-load script (map 176, zone 6 area 99) places icons at
tile coordinates through steps 101 and 102 (the plain and star markers), each gated on story flags,
and the minimap already shows star markers. The zone 8 landing (world-map rect 2560,1000) is gated
only on flag 86.

**The lab after the self-destruct (2026-10-19):** flag 141 (Krillin's "head back to Master Roshi's
Island" line) seals the cave mouth into the basement with rubble, and after flag 140 the computer
room's doors lead to area 65. Area 47's top exit into 48 never fires in the stock game. Full notes in
`docs/story-archive.md` "Lab re-entry".

**Dragon Balls and the Cell Games (unconfirmed):** the hunt appears to gate the Cell Games: the quest
table puts "Return the Dragon Balls to Dende" right before "Enter the Cell Games", and the world map
checks the all-seven flag 158. To confirm in play.

## 9. The Cell Games and after (walked from the script dump, 2026-10-19)

Maps: zone 14 area 1 (map 323, the arena grounds), area 2 (324, the ring), area 3 (325, the desert
after the ring is destroyed), area 4 (326, the beam struggle). Disassembly:
`python tools/script_asm.py dump map 14 N`. Perfect Cell is sprite 30 with five stage stats 21-25;
the Cell Juniors are stat 26, sprite 31. None of 21-26 has a boss rule in `bosses.c`, so in co-op
each stage would spawn a twin today.

**How it opens.** Hand all seven Dragon Balls to Dende at the Lookout (zone 9 area 1): "You've found
all the Dragon Balls!", then Goku: "Now all that remains is to finish up any training we still need
to do. Once we're ready, let's head for the Cell Games Arena." That sets flag 191, which opens the
arena's landing on the world map (zone 14 area 1). So the Dragon Ball hunt does gate the Games.

**Arena grounds (323).** Everyone stands around as NPCs with one line each: Android 16 ("I was
created solely to destroy Goku, though I've chosen not to"), Yamcha, Tien, Piccolo, Hercule ("everything
he does is a trick!"), Gohan, Vegeta, Trunks, Krillin (the new Dragon Balls cannot revive anyone
wished back before), Goku. The way into the ring is a barrier: "Only Goku is allowed to pass through
this barrier." A flight spot leads back to the world map until the Games are over (flag 195).

**The ring (324).** Goku: "I'll fight Cell first." Hercule cuts in ("I've had enough of you clowns"),
the announcer introduces him, he attacks, Cell: "Your ignorance is painful," Hercule is knocked out
of the ring (actor sprite 72 despawns). Krillin: "To be honest... I was rooting for Cell that time."
Cell asks for Goku. **Fight 1: Goku vs Cell (stat 21) down to 75%.** Then Cell: "I'm tired of being
confined to this little ring," blows the ring apart, and the fight moves to the desert.

**The desert (325).**
1. **Fight 2: Goku vs Cell (stat 22)** to about half. Goku gives up ("You win. I give up"), Vegeta:
   "You coward!!!", Goku names Gohan.
2. **Fight 3: Gohan vs Cell (stat 23) down to 60%** (the script makes Gohan player 1). Cell taunts
   him, Gohan explains his hidden power, Piccolo wants to help, Goku says no.
3. Android 16 grabs Cell to self-destruct; Krillin: "Your bomb was removed!" Cell destroys 16.
4. Cell spawns three Cell Juniors ("My children! Give them pain!"). **Fights 4-6: one Cell Junior
   each, in turn, as Piccolo, then Vegeta, then Trunks** (the script switches player 1 between them).
5. After the third Junior falls (still in its defeat scene), Gohan is player 1 again: Hercule carries
   16's head to Gohan (16: "Take me over to the boy!"); 16's last words; Cell crushes him; Gohan: "I
   won't watch this ANYMORE!!!", becomes Super Saiyan 2 ("Gohan has entered Super Saiyan Rage!"),
   Cell: "NOW TASTE MY TRUE POWER." **Fight 7: Super Saiyan 2 Gohan vs Cell (stat 24).**
6. Beaten (stat 24's defeat scene), Cell spits out Android 18 (back to Semi-Perfect, sprite 29), then Cell
   swells to self-destruct (sprite 27). Goku: "Goodbye friends," teleports Cell to King Kai's planet.
   Gohan: "DADDY!!!" Krillin consoles him, remembers 18 is alive.
7. Cell returns as Perfect Cell (he regenerated from his core), kills Trunks, Vegeta attacks in rage.
   Goku speaks to Gohan from the Other World. **Fight 8: Gohan vs Cell (stat 25) down to 20%.**

**The beam struggle (326).** Scripted: Goku: "Dig deeper Gohan!", Vegeta's blast distracts Cell,
Gohan's Kamehameha wins. Piccolo: "Cell's energy completely vanished!" They leave for Dende with
Trunks and 18; Vegeta's thoughts about Goku; then the announcer and Hercule: "I DEFEATED CELL!!!!
SINGLE HANDEDLY!!!!!!" Flag 195 is set; Hercule joins the party at level 40 ("Hercule is now an
available character!"), Goku leaves the party, and the game warps to the world map.

**After the beam struggle (still map 326).** Piccolo: "Cell's energy completely vanished!"; Yamcha:
saving the world took a lot out of Gohan; Krillin: they will take him to Dende; Tien takes Trunks,
Krillin takes 18 ("she is a victim of Cell"); Gohan thanks Vegeta, who refuses help; Vegeta's thoughts
about Goku. Then the announcer finds Hercule: "I DEFEATED CELL!!!! SINGLE HANDEDLY!!!!!!" ("I just got
so irritated with the light shows and the tricks..."; the boy and his friends "thanked me kindly and
went home"). Flag 195. **Hercule joins only if story flags 302, 303, 305 and 306 are all set**
(entry 50: in party, level 40, "Hercule is now an available character!"); otherwise only "Journal
Updated!" (entry 53). Goku leaves the party (our "Keep Goku" patch keeps him). Warp to the world map.

**The five statues.** The Capsule Corp atrium (zone 6 area 3, map 147) has five stone pedestals around
the fountain; flags 195, 302, 305, 303 and 306 each put a fighter's statue on one (seen in a headless
run, 2026-10-19). No map script sets 302-306; where the game sets them (likely optional achievements)
is not traced yet.

**Talk to Dende (the main ending).** The journal's quest after flag 195 is "Talk to Dende on the
Lookout". At the Lookout (zone 9 area 1): Dende: "Gohan! You're okay! I'm really sorry about your
father." Gohan: Goku was with him the whole time; Piccolo: "You made us all proud"; 18: "Gohan beat
Cell?"; Piccolo tells 18 to thank Krillin; Gohan teases Krillin about his crush; 18: "Not that easy."
Shenron's first wish (Yamcha): revive everyone Cell killed. Goku cannot be revived (already wished
back once); Goku speaks telepathically and chooses to stay dead (the villains came for him). The second
wish: Yamcha jokes about getting rich; Krillin asks to make the Androids human, which is beyond
Shenron's power, so he wishes the bombs out of them instead ("THERE - IT HAS BEEN DONE."). 18: "If you
think you won my heart because of that lousy wish, you're mistaken! But it was kind of nice of
you..."; she declines to join them and leaves; Yamcha: "That chick digs you." Tien and Yamcha leave
(Tien: "You're the man of the house now!"), Piccolo stays on the Lookout, Gohan says goodbye. Flag 196,
a warp to zone 2 area 1 (Gohan's home), the credits.

**The bonus ending.** With flag 195 and all four of 302, 303, 305, 306, a building in zone 4 area 5
(map 67) plays "Late Night with Rob Howard": Hercule, "hero of the Cell Games", is interviewed; asked
about critics who say he did not defeat Cell and about "those strange fighters who could turn their
hair gold" and the little boy, he calls it "parlor tricks... mirrors and laser lights"; the crowd
chants "HERCULE!"; then the credits and flag 196.

**After the credits.** Thirteen maps test flag 196 (names, music, actors change, e.g. Gero's lab
area 47's name); not walked yet.
