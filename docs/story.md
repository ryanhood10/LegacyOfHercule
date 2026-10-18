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
| Then | normal game with Gohan + Krillin/Hercule as player 2 | every later map where Krillin stands as an NPC (18 maps: Kame House, Amenbo Island, the Cell chapters) needs the NPC removed or his line reassigned when player 2 is Krillin. text/actor, mechanical |

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

- Party slots: grow to 8 (clean) or let Krillin borrow Trunks' slot (hack)?
- Super Saiyan look: gold afro (what we have) or a different "Super Human" look?
- Name of the Goku + Hercule fusion in our text ("Gokule" is the fan name).
