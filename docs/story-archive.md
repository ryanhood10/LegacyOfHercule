# Story archive: superseded plans, cost estimates and investigations

History behind `docs/story.md` (the current plan) and `docs/story-rom.md` (stock ROM facts). Nothing
here is current: each entry says when it was written and what replaced it. Read it to learn why
something was decided, not what to build. Moved here from `docs/story.md` on 2026-10-19; quoted
passages keep their original wording, with corrections marked.

## A. Status paragraphs from the old section 0 (2026-10-18)

Superseded by the rewritten section 0 of `docs/story.md` (2026-10-19).

**Old mode line:** "story first. Ryan and Claude are walking the game chapter by chapter from the
player's side, deciding co-op casting and new beats; every decision is recorded here the same turn.
Story production started with the tools in `docs/scripts.md`."

**"Implemented (story-scripts, 2026-10-18)":** items 1 and 2 (old Hercule in the prologue, Krillin's
visit, the forest spirit, the landing sites, the West City deal, Triceratops Jungle), the duels of
items 3 and 6 (space clone duel and Hercule at the landing; Time Chamber duel, winner Piccolo / loser
Hercule), zone 7 of item 3 (Android 13 in the cutaway and beside 19, both-down, 13 flees with the
"true plan" line; the skip's boosts for Hercule and Krillin), Piccolo's chapter of item 4 (13 beside
Gero in his three stages, Hercule's "negotiate" at the lab door, 13 in the lab, the road with Vegeta
as player 2), item 5 (Hercule stays at the Lookout, Gohan as player 2 in Trunks' chapter, Kami's boost
for Hercule, Gingertown with Cell's Multi-Form duplicate), item 6's lab basement (Trunks + Krillin,
the NPC left out in co-op) and Kame House (Piccolo a scene actor, Krillin stays home), item 7
(Semi-Perfect Cell's duplicate, Krillin on the pad that played Vegeta, his one-hit KO), item 8's gym
duos and animal finale, Cooler's Namekian servant and the arctic Android 13 (alone until his partner
is decided; flag 376) and the Android 16 island fight from item 6 (Kame House deal, 16 over the two
stock stages, half health, Cell). Tested headless (`docs/testing.md`). *(The arctic 13 was removed
the same day: entry H.)*

**Old item 8 of "Decided so far":** "The ten-day wait: Capsule Corp hub; Roshi's gym as 2v2 duos
(Yamcha & Tien, Hercule & Krillin, 'come back with a different team' line) and the animal-ninja
finale; Dragon Balls and side quests; Cooler's two-part bonus kept (Piccolo gate) with a Namekian
servant beside him; optional Android 13 arctic encounter behind a Vegeta barrier (partner undecided;
story flag for the post-Cell variant); a Broly side arc wanted (maps and scripts; sprite 159 ready).
Revised in 4g (2026-10-19): Android 13 moved to after Cell; Broly chapter, dojo sparring, Time
Chamber waves and the Krillin + 16 'Gero's computer' chapter added (ends as Krillin + Hercule); the
wait's story is complete."

**Old "Next to decide" (obsolete):** "Android 13's arctic partner; then the Cell Games (Hercule's
stock 'victory', Cell Juniors, Gohan's win, Keep Goku), the ending, and the Buu's Fury phase (Super
Saiyan Hercule beats Buu, Goku + Hercule fusion, Vegito for Janemba; no Hercule reveal before then)."
The arctic partner question died when 13 was held back (2026-10-18); the rest is section 0 "Open".

**"Built (2026-10-18, on main): every decided chapter, 1 through 8,"** as data/story/*.json +
*.lscript (tools: `docs/scripts.md`), on the engine pieces in `docs/coop-design.md` (duels with the
CoopCall bridge, both-down / half-health / pair rules, duplicate-NPC removal, Krillin and old Hercule
playable, Android 13 / 16 / Namekian models, zone-15 maps). Tested per chapter with the headless
runner (`tools/headless/`); the space duel was also played in the fork with a real final blow.
Numbers chosen by Claude that Ryan may want to change: Kami's boost (+3 levels and a refill), the
arctic 13 (stat 114, half health; obsolete since the removal), the gym duos (also fought in
single-player). Then (2026-10-19) the wait's additions were built and merged.

**"Mechanics the story needs" (2026-10-17/18), all done:** player-vs-player duel mode + pad swap
(`duel.c`); both-down and half-health boss rules (`bosses.c` table, `enemies.c`); duplicate-actor
suppression (`actors.c`); single-pad world map; a bytecode assembler for new scenes
(`tools/script_asm.py`); new enemy-stat rows (Android 13, Multi-Form Cell, Cooler's Namekian, 2v2
gym duos, animal wave); Android 16 fight set + rocket projectiles + AI (sprite 160, `android16.c`);
Android 13 art (sprite 165, `tools/bossmodels.py`); Krillin's real row, portrait and abilities (row
23, sprite 161, Destructo Disc ability 21); old Hercule's row on sprite 158 (row 24); Kami's boost
script; per-chapter selectable-flag edits; map tooling for the arctic, Broly's arena and a chamber
interior (zone 15, `docs/maps.md`). The "or a Buu's Fury map decode" alternative was not needed.

## B. Opening cost table (old section 4, 2026-10-17)

Superseded by chapters 1 and 2 as built (2026-10-18; plan in `docs/story.md` 4). Cost tags: **text** =
dialogue swap; **actor** = add/remove a map actor record; **byte** = one script opcode/flag; **code**
= new C; **art** = new frames; **engine** = a data structure change.

| beat | Ryan's version | ROM facts and cost |
|---|---|---|
| Future prologue | keep Trunks as player 1; player 2 = **old Hercule** (Buu's Fury look), Trunks' buddy, comic relief, powerless, no hint of what he becomes; he rides the time machine back | text; new row + sprite for old Hercule. Buu's Fury's sprite tables use a different layout (LoG2's record signature does not appear in that ROM), so lifting its model needs a reversing pass first; recolouring LoG2's Hercule grey with the SSJ cloner is a day. **art/code** *(Done 2026-10-17: Buu's Fury sprites decoded, old Hercule = sprite 158.)* |
| Present day: Krillin visits | Krillin walks in instead of the phone; player 2 = Krillin from here | replace the phone trigger's sequence with a Krillin actor + dialogue in map 17; drop the dream map 14 from the flow. text, actor, byte |
| Forest "spirit" fight | Gohan and Krillin go into the forest (the Battle Armor cave gives the reason), fight Frieza **and Cooler**, told afterwards it was the spirit of the forest; no "it was a dream", no teleport home | reuse the dream map's Frieza boss record and add Cooler as its twin (sprite 34, stat 29 exists from the bonus area; the twin mechanism in `enemies.c` already pairs bosses). Rewrite the wake-up sequence into the spirit explanation and route the exit to the armor cave instead of home. text, actor, byte *(Built with a new stat 111 for this Cooler.)* |
| Landing site | Krillin must not stand there as an NPC because player 2 is Krillin | delete the Krillin actor record in map 52 and give his line ("Who is this guy?") to Tien or to a player-2 bubble; same in map 54 ("two energies"). actor, text *(Built: his lines stay, spoken by the player.)* |
| Goku's arrival, Trunks leaves | as stock, plus old Hercule leaves with Trunks | one actor beside Trunks in map 54 and two lines. actor, text |
| West City | Gohan and Krillin fly there (world-map sign; both players fly, verified); Piccolo playable; **Hercule joins here** after the mansion deal (he wants the flying "magic trick", Piccolo needs a roof) | the stock scene already has Hercule Day, the Mayor and the key theft, so Hercule is on site; add the Piccolo/Hercule dialogue and a `SetCharacterInParty(Hercule)` next to the Piccolo unlock. text, byte. Goku is in West City too in the stock script (he waits outside City Hall); keep or trim his lines. |
| Triceratops Jungle | Piccolo's solo stretch (King Triceratops); player 2 = Hercule only, level 1 beside Piccolo (clear Krillin's selectable bit at the jungle entrance, restore it at home); Hercule claims credit for the dinosaur peace treaty | byte, text *(Built: Gohan's and Krillin's bits cleared inside, restored on leaving; flags 361 / 362.)* |
| Then | normal game with Gohan + Krillin/Hercule as player 2 | every later map where Krillin stands as an NPC (18 maps: Kame House, Amenbo Island, the Cell chapters) needs the NPC removed or his line reassigned when player 2 is Krillin. text/actor, mechanical *(Replaced by the generic duplicate-NPC rule, `actors.c`.)* |

## C. Vegeta's chapter: costs and open questions (old 4b, 2026-10-17)

Superseded by the duels as built (2026-10-18: `src/coop/duel.c`, `data/story/ch3_duels`, CoopCall
1-3) and the zone 7 chapter (`ch3_zone7`).

| piece | mechanism | cost |
|---|---|---|
| the two cutscenes | dialogue records plus a scripted actor (Vegeta's sprite with a palette tint through the entity colour remap at +0x24, so the clone reads as a shadow), the opcodes already in the ROM for positions, flight, facing, fades, music and waits | text, actor, a small script assembler we do not have yet (the dumper exists; the encoder is the same format) |
| the clone as a character | one party slot "Vegeta's shadow" with Vegeta's row and abilities (needs the N-slot party, in progress), removed from the party after the duel | data, byte *(Built: the clone lives in party slot 7.)* |
| player-versus-player hits | new in `src/coop`: player attacks and blasts test against the other player's entity, damage through the existing player-2 damage routing, hurt/knockback states, death ends the duel and fires the "I've won" script (`Map_SpawnScript`) | code, one to two days *(Built: the final blow leaves the loser at 1 HP instead of a death.)* |
| the winner keeps Vegeta | swap the two pad sources in `input.c` when player 2 wins (one flag) | code, small |
| loser sits out | no selectable slot for player 2 means no player-2 entity (already how `pick_p2_slot` works) until the Hercule script sets his selectable bit | byte |
| Hercule meets Vegeta | actor + dialogue appended to zone 7 area 1's on-load scene, `SetCharacterFlag_Bit1(5)` | text, actor, byte *(Superseded the same day: the meeting moved to the landing map, zone 6 area 22 / map 166, so the loser sits out one scene, not fourteen robot areas.)* |

**Old open question:** "whether both players keep the Super Saiyan drain rules during the duel (the
stock form is a timed boost), and a draw rule if both die on the same frame (give it to player 1)."
Decided (2026-10-17/18): no drain (duel flag 8, full energy), a draw goes to player 1.

**Old art plan for Android 13 beside 19 (2026-10-17):** 13 "needs an enemy-stat row and a sprite";
"13's art is in Buu's Fury as an unused sprite, so this beat depends on decoding that ROM's sprite
tables; until then a recoloured LoG2 android (sprites 12-14 have the same enemy-grade pose sets)
stands in. Fallback if the art never lands: Android 20 as the twin." **Wrong:** Android 13 is not in
Buu's Fury (found 2026-10-18 when its sprites were decoded). Superseded by sprite 165, built from
Android 16's body and 17's fight frames by `tools/bossmodels.py` (2026-10-18), stat 110. The note
that "the both-down rule is a small addition to the twin-boss code in `enemies.c`" became the
both-down rule in `enemies.c` with the per-stat table in `bosses.c`. In the film 13 absorbs 14 and 15
to become Super 13.

## D. Piccolo's chapter and the road: build notes (old 4c, 2026-10-17)

Superseded by `data/story/ch4_piccolo` (2026-10-18).

- The original rule wording: "when a map spawns an actor whose sprite id is player 2's current
  sprite, skip that actor (the dialogue still plays with its portrait)". Built as `actors.c`, covering
  any character a player controls.
- "The chapter's fights, as scripted: Dr. Gero at the lab door and Android 18 on the road; both
  become two-on-two through the twin mechanism." Co-op at the lab door was to be "harder the way the
  difficulty code already does (scaled stats, twins, extra escorts)".
- The road: "17 joins as 18's twin (needs an enemy record; his fighting sprite is 9, a full
  enemy-grade set with attacks and hurt poses; 15 is Android 16's NPC set)". **Corrected 2026-10-18:**
  Android 16 is sprite 8; 15 is Dr. Gero. 17 and 18 were paired by rules on stats 3 and 4 (no new
  record). "The kiss stays Krillin's unless Ryan wants it on Hercule" (it stayed Krillin's).

## E. Android 13 "loose" notes (2026-10-17 and 2026-10-18)

- Old 4d (2026-10-17): "Android 13 is loose after the road: bonus content."
- Old 4e (2026-10-18): "Android 13 stays loose (his art is not in either ROM: drawn, or his role
  moves to an existing body, Ryan to decide)."

Superseded first by the arctic encounter (entry H) and sprite 165, then by Ryan holding 13 back until
after Cell (`docs/story.md` 4g, 2026-10-18).

## F. Android 16 as the island boss: the idea and its cost (old 4e, 2026-10-18)

Superseded by the decision in `docs/story.md` 4e and the build: branch `android16-boss` merged
2026-10-18 (sprite 160 from `tools/a16sprites.py`, stat 109, `src/coop/android16.c`), island scripts
in `data/story/a16_island` (the merged boss branch's island scripts still named 17 until then).

"Change the dialogue so 16 is furious Goku is not there, and Piccolo tricks him: beat me on the next
island and I'll tell you where Goku is. 16 as a strong boss with rocket attacks: heat-seeking rockets
that lock on and blow up after three seconds, rocket barrages, big explosives all over the field, none
of which hurt him." Facts then: "LoG2's Android 16 (sprite 15) is a 16x32 NPC set with idle, walk and
one blast pose only" (**corrected:** he is sprite 8, with idle, walk, hit-stun, one punch and
knock-down; 15 is Dr. Gero), "and he is not in Buu's Fury (he dies in the Cell saga), so a fighting 16
needs new frames (attacks, rocket punch with the forearm detached, hurt) drawn or edited from a big
body; the rockets need a projectile sprite (no missile exists; the robots' shots and ki blasts are the
nearest) and their behaviour (lock-on, fuse, barrage, area explosions that spare the owner) is new
boss AI in `src/coop` (a few days of code once the art exists). 17's fight sprite (9) is complete, so
17 + 18 is the no-art alternative."

The plan text for the Lookout also said "both Hercule and Krillin are selectable for the 17 fight":
that fight became the 16 fight.

## G. Roshi's gym: superseded notes (old 4f, 2026-10-18)

- **Solo-sparring co-op note** (superseded the same day by the 2v2 gym, `data/story/ch8_gym`):
  "Krillin is one of Roshi's sparring opponents (enemy stat 64, sprite 76); when player 2 is Krillin
  that match swaps its opponent (Hercule is the obvious stand-in) or is skipped by the duplicate-actor
  rule."
- **Gym cost note:** "the twin-spawn code already pairs enemies; a wave spawn of mixed enemy records
  and the room dialogue are new script and a little code; every animal above is a stock enemy type
  (ids to be confirmed in the atlas), the 'ninjas' map to the Warlord's bandit sprites." Built
  2026-10-18 with `BOSS_PAIR` rules on stats 106 / 98 / 64, `CoopCall 8` for the refusal, and the
  finale in three rounds because eleven kinds at once ran out of OBJ tiles (flags 363-375).
- **Cooler's servant art options:** "Piccolo's complete playable set (103/104) recoloured through the
  entity colour remap, or a cloned record with a changed palette, so no new frames." Built as sprite
  166 (`tools/bossmodels.py`, Piccolo's capeless set 104 recoloured), stat 113.
- **Cooler's gate, "check in play when it opens":** resolved 2026-10-18 (flag 60 from chapter 2,
  Piccolo selectable after chapter 7; no level or Goku gate).

## H. Android 13 in the arctic (planned and built 2026-10-18, removed 2026-10-18)

Superseded by `docs/story.md` 4g: Android 13 is held back until after Cell.

**The plan (old 4f):** "a new or re-dressed arctic area reached through a new 'Only Vegeta' barrier;
the party goes there to find something and runs into 13, who says HE was supposed to absorb 17 and
18, not Cell, his chance at perfection is ruined and he would kill Cell if he could; Vegeta: he won't
get the chance, because he'll be dead. A fight to half health, 13 leaves. Two-player (Ryan): 13 needs
a partner; candidates with no new art are one of Gero's large robots (sprites 37-39) under 13's
command, or a spare Android 19 unit recoloured; Androids 14 and 15 (his film sidekicks, whom Super 13
absorbs) would be the best story but need drawing. Optional: a story flag records whether it
happened, and the post-Cell Super 13 encounter has two openings (met before / first meeting)." The
partner was never decided.

**Built (2026-10-18):** the map, zone 15 area 1 (`data/maps/arctic.json`, `docs/maps.md`): Northern
Mountains zone 8 area 28 re-dressed in snow and ice with falling snow, a cave behind a working "Only
Vegeta" barrier (level 50, story flag 380). The encounter, `data/story/ch8_arctic.lscript`: 13 meets
Vegeta at the cave mouth, a half-health fight alone (new stat 114 and a rule in `bosses.c`), he flies
off, flag 376 for the post-Cell variant. Tested headless to the end of the fight.

**Removed (2026-10-18):** the cave-mouth trigger and the boss record were taken out of the map
(`data/maps/arctic.json` note "_arctic_13"); `data/hercule.json` has no `ch8_arctic.json`. What is
left: the arctic map itself (an empty cave behind the barrier); `ch8_arctic.lscript`, still named in
the map's `"sources"` but called by nothing; stat 114 and its half-health rule in `src/coop/bosses.c`,
unused. The file's own header still says the partner is undecided, and the `bosses.c` comment on the
rule still cites "docs/story.md 4f".

## I. Broly maps wanted (old 4f, 2026-10-18)

Superseded by the Broly side chapter (`docs/story.md` 4g, built 2026-10-19). "Broly (maps and scripts
wanted): maps built (2026-10-18, `docs/maps.md`): zone 15 area 3, a wasteland basin with a pod crater
(LoG2 tiles), and area 4, a walled crater imported from Buu's Fury (same map format; built when
`roms/buu.gba` is present). Buu's Broly battle map itself is not identified (its map table is not
found)." The chapter used area 3 with a landing zone added; area 4 is unused.

Original Broly plan details changed in the build: the ship was to be "an existing model copied in"
(became Buu's Fury's purple ship, sprite 171); Paragus was to be "a generated recolour/edit of every
frame (like tools/a16sprites.py)" (became `tools/brolysprites.py`, sprite 170, portrait 120).

## J. Time Chamber waves: planning notes (old 4g, 2026-10-18)

Superseded by the build (branch `lookout-training`, merged 2026-10-19). Open items then: "the enemy
list per round to be mapped"; "Limit to respect: sprite memory, not a fixed count; in co-op every
regular enemy brings a twin, so about 4 per wave (8 on screen) is the working number until it is
measured"; "Open: round display between waves, and any reward beyond EXP." Resolved: the round table
in `docs/story.md` 4g, the measurements below, "Round N" captions; rewards are still Ryan's call (section 0).

**Measurements behind the round table (2026-10-19, still valid; wide build, co-op, all eight
standing):** the largest free OBJ VRAM block never fell below 6.9 KB (rounds 3 and 7, the
pterodactyls); two Destroyers per round (four with twins) had left only 2.2 KB, so round 8 lost them
and round 10 keeps one. Lag: at most 16 dropped frames in 300 (round 3), 6 in round 10, none in an
empty chamber.

## K. "Gero's computer": investigation and first boss plan (old 4g, 2026-10-19)

Superseded by the build (branch `gero-computer`, merged 2026-10-19).

**First boss plan:** "the computer is finishing a spare Android 19 body; the players fight it
(Android 19's existing fight, enemy stat 5, sprite 12)." Changed by Ryan the same day to two Android
19s at once (stats 120 and 121, a `BOSS_PAIR`).

**Lab re-entry, checked 2026-10-19 (static data plus a headless run with flags 0-191 set):** the zone
8 landing stays open; area 16 to 47 (flag 126) works; the clearing outside the lab (area 48,
"Northern Mountains") has a flight spot back to the world map once flag 141 is set (verified, it lands
at 2608,1060). Flag 141 is set by Krillin's "head back to Master Roshi's Island" line after the
self-destruct, and from then on **the cave mouth into the basement is sealed** (a rubble layer; the
player stops below the door). Inside, the basement doors work (61 to 64 with flag 138, 64 to the outer
area 65 with flag 140) and the computer room (area 63) still loads, but after flag 140 its doors lead
to area 65 instead. So the chapter must reopen the cave mouth (16 punches through the rubble, a
chapter flag) and send the basement door to the computer room (63) while the chapter runs. Area 47's
top exit into 48 did not fire in the headless run (player stopped at y 20): recheck in play.
**Outcome:** the rubble is cleared by flag 422 (`ch9_a48_onload`, `docs/maps.md`), the basement door
leads to the computer room during the chapter, and area 47's top exit, broken in the stock game too,
is fixed.

## L. Branch names of the wait's builds (2026-10-19)

The 4g build notes named their branches before the merge: dojo sparring and the training waves on
`lookout-training` (worktree `F:\MeltyCreations\log2-lookout`), "Gero's computer" on `gero-computer`
(`F:\MeltyCreations\log2-gero`), Broly on `broly-chapter` (`F:\MeltyCreations\log2-broly`). All three
merged 2026-10-19 (`docs/log.md`).

## M. Engine limits found while planning (old section 5, 2026-10-17), all resolved

1. **Six party slots.** "`PartyState` holds exactly 6 characters (Gohan, Vegeta, Goku, Piccolo,
   Trunks, Hercule): default party, growth rows, EXP and the save all assume 6. Krillin as a real
   party member is a 7th. Options: (a) grow the party to 8 slots: engine work touching the party
   struct in IWRAM, the save layout, the switch screen and the level-up tables, maybe two days; (b)
   Krillin borrows Trunks' slot while Trunks is away: row swap per chapter, stats shared with Trunks,
   cheap but a hack that breaks when both are needed. Old Hercule in the prologue can borrow
   Hercule's slot (same slot, different row) at no cost." **Resolved:** Ryan chose 8 slots
   (2026-10-17, `docs/party-8-slots.md`); `party-slots` merged 2026-10-18 (`PARTY_SLOTS`, default 8;
   Krillin slot 6, the duel clone and Android 16 use slot 7); old Hercule shares slot 5 on row 24.
2. **Krillin's poses.** "Sprite 76 has 13 groups: idle, walk, run, two attacks, charge, hurt, fly and
   the blast pose (12). Hercule's playable set has 21 with extra attack groups 7/8, 11, 13 and the
   knocked-out pose 20." **Resolved:** sprite 161 (sprite 76 plus Goku's fight groups re-headed,
   `tools/krillinsprites.py`), row 23.
3. **Destructo Disc.** "No disc projectile sprite is in LoG2; a new projectile needs frames and an
   ability pair. Stock ki blasts can be recoloured for Krillin at no art cost." **Resolved:** ability
   21 Destructo Disc; his Ki Blast is the stock one recoloured blue.
4. **Old Hercule's model:** "Buu's Fury needs its own sprite-format pass before any of its art can be
   lifted." **Resolved 2026-10-17:** the format is decoded (`docs/sprites.md` "Buu's Fury"), old
   Hercule is sprite 158.
5. **Dialogue capacity.** New lines go to free ROM space, so length is not a problem; speaker
   portraits are limited to the ids the game has (now in `docs/story-rom.md` section 2).

## N. Other resolved items

- **Party slots (old section 7, 2026-10-17):** "decided, grow to 8 (Ryan); plan and site inventory in
  `docs/party-8-slots.md`." Merged 2026-10-18.
- **Buu's Fury (old section 6, 2026-10-17):** the fused form was to be "built with the same cloner
  once that ROM has its decompile pass", and the needs list began with "Buu's Fury decompile pass
  (party, display rows, sprites, saves)". The sprite part is done (2026-10-17) and its maps use LoG2's
  format (2026-10-18); the rest stays in `docs/story.md` section 6.
- **Time Chamber interior (old 4e, 2026-10-18):** "The chamber interior needs a map." Built
  2026-10-18 as zone 15 area 2.
- **Hercule-hero captions (2026-10-18):** the prologue caption rewrites from the first text swap
  ("Earth's true hope is a man with no ki at all" and "Sixteen years before Hercule's rise") were
  removed under the no-reveal rule; `data/hercule-text.json` is empty again.
