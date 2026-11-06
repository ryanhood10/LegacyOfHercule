# The Hyperbolic Time Chamber as a game mode (plan approved by Ryan, 2026-11-05; state of the build at section 12)

**Status: phases 1-6 and the loop built (section 12: what stands, the numbers measured, how to add a room); Ryan's first play
answered on branch `nb-chamber2` (section 13: the door home asks, no save pad, plaques ON then OFF, the lever's way to the gate,
every reward in a box, the red carpet, the gate's wall and portal, the opening scene, side rooms, the training's end; his
second round in 13.6: two questions mid-run, Broly's north room, Frieza's three rooms). Where 13
and an older section differ, 13 holds.** Ryan reads part 1
first (what the player sees), part 2 for what the stock game really has, parts 3-6 for the content, part 7 for the
technical notes, then the phases, the risks and the decisions (section 10). Facts carry their source; anything not
checked is marked **(not verified)** and listed again in section 11. All dialogue here is Claude's draft or a
placeholder, not final lines.

Ryan's brief: "this needs to be a super improved game mode. it needs to be almost as cool as our tournament feature."
A big open area of weak enemies with unique rooms in it. Beat the boss in a room, hit a lever, a light on a gate goes
out and the enemies outside get harder. Six lights, six rooms, then a hidden room with Mr. Popo. "keep going back and
training this way." His standing rule for scenes: characters are never stagnant, every lineup of 1 or 2 players and
characters is handled, it fits the story's progression, smooth and not overdone.

**The short answer on feasibility.** Everything Ryan remembered is in the game. The gate with 6 lights and 3 triggers
is real (the gate to Dr. Gero's lab), its 6 lights are already 6 separate objects, so 6 lights with 6 triggers needs no
new engine work. The numbered switches are real, the jars are real, the trophies are real (four of them). Four of the
named bosses already fight in the tournament with bodies, moves and portraits. New work: the big map, the room
framework, a little art, and Mr. Popo. The real limit is sprite memory, which decides how many enemies can stand on
the big map at once (section 7.4).

---

## 1. What the player sees

**Getting in.** During the ten-day wait, as today, Mr. Popo stands beside the chamber door on the Lookout. He says the
chamber is free and there is something inside for you. Through the door the players come out of the living quarters
(the white palace front you already know) into a huge white void, much bigger than today's room: about 1280 x 960,
roughly five screens across. There is a save pad at the quarters.

**The open area.** Bears and snakes wander the void. Far off in every direction there are places: a dojo, a pirate
airship, Frieza's ship, a Namekian ship, a lab, a Saiyan ship. Breakable jars sit around them. Straight north of the
quarters stand the two white pillars from Gero's lab, each with three red lights, with force beams humming between
them. Walking into the beams does nothing: the way is shut.

```
                   [ hidden room behind the gate ]
                                 |
                       ||  beams + 6 lights  ||
                                 |
     Namek ship                                       Frieza's ship
                 jar                       jar

                        [ living quarters ]
   Saiyan ship          save pad, door home               Pirate airship
                                 you
                 jar                       jar
         Lab                                            Roshi's dojo

     (bears, snakes, then harder things wander all of it)
```

**A room.** Walk up to Frieza's ship and in through the door. A small room, with Frieza at the far end. He walks
toward you, a short exchange (it knows who you are: Trunks gets a different line than Hercule does), the door seals,
the boss bar comes up. Alone you fight Frieza, who turns into Mecha Frieza at 40 %. With a partner you fight Frieza
and King Cold together.

**The lever.** He falls. On the wall is a plaque with ON and OFF, which did nothing but click when you tried it before
the fight. Either player walks up and presses A. It flips to ON, the screen cuts away to the gate: one red light goes
out with the stock power-down sound. Back in the room, the door opens.

**Outside it has changed.** The bears and snakes are gone. Now it is the next tier. Each lever brings the next one, so
"each time they go outside they get more nervous". The music can tighten as well. Boss rooms can be done in any order
(decision 3), and a boss gets stronger the more lights are already out, so the last room is the hardest whichever it
is. Jars and the regular enemies come back whenever you re-enter the open area.

**The gate falls.** The sixth lever: the cutaway shows the last light go out, then the three beams die one by one,
with the hum cut off, exactly as the stock game does it at Gero's lab. Top-tier enemies roam outside now. The players
cross the whole void to the gate.

**The hidden room.** Through the gate the screen shakes and the sound drops. A plain white room, a few big jars. Far
off, a small round figure with his back turned. Mr. Popo. He has been running all of this. He turns, walks down
slowly, and then he is gone in a flash and behind you. A boss fight unlike the others: he never comes close, he
teleports away the moment you reach him, throws blasts, and vanishes from some of your punches. Section 5.

**Afterwards.** Beating Popo pays a big reward. Then the chamber is yours to come back to: decision 4 settles whether
the lights stay out or Popo offers to start it again, harder. Optional later pieces: a panel of the stock numbered
switches inside the quarters that picks which animals are outside ("organized buttons to press for a specific
animal"), and a hall of the stock trophy statues where you press a statue and that character fights you (section 6.3).

**One player or two.** One player: one boss per room, no twins. Two players: every regular enemy has its twin, as everywhere; bosses come as a pair or a stronger
lone boss; either player can press a lever or a door (player 2's agency, `src/coop/agency.c`); a downed partner can be
revived as usual. Any of the party can be played, and the save pad at the quarters changes who.

---

## 2. What Ryan remembered, checked against the ROM

| remembered | in the game? | where |
|---|---|---|
| A gate with 6 lights, 3 triggers, 2 lights per trigger | **Yes, exactly** | zone 8 area 16 (Northern Mountains): the gate to Dr. Gero's lab |
| A generator / lever that turns off a light | **Yes**: three generators, each three breakable tubes | zone 8 areas 17, 39, 50 |
| Organized buttons to press | **Yes**: six numbered, coloured ON / OFF switches on a cave wall, a combination puzzle | zone 8 area 37 |
| Breakable jars with health and energy | **Yes**: a black clay pot and a grey stone pot, 3 hits each | zone 2 area 50 (3 of them), zone 14 area 2 (4) |
| Trophies for most player characters | **Four trophies** (Gohan, Piccolo, Trunks, Vegeta), found in chests, plus their stone statues at Capsule Corp | section 2.3 |
| Bears | **Yes, as soldiers**: "Kuma Mercenary", a bear with a sword. No wild bear | sprites 90-92 |
| Grenadiers | **No such word in the ROM** (nothing matches "grenad"). My guess: the Warlord's Henchmen, red bandanas and goggles, they attack at range. Decision 7 | sprites 141-143 |
| Pirates | **No pirate enemy in LoG2.** Buu's Fury has a "pirate robot" model (64 x 64, not a fight set) and airship maps | section 3 |

### 2.1 The gate (zone 8 area 16; `build/scripts/map_218_z8_a16.txt`)

Two white pillars painted into the map (38 tile slots), six light decorations and three beam decorations on top:

| decoration | what | where (x1, y1, x2, y2) | hidden by |
|---|---|---|---|
| 2, 3 | top lights, left and right, 8 x 16 8bpp, frame 0 (a red diamond) | (164, 82)-(172, 98), (236, 82)-(244, 98) | flag 112 |
| 0, 1 | middle lights | (164, 97)-(172, 113), (236, 97)-(244, 113) | flag 97 |
| 4, 5 | bottom lights | (164, 112)-(172, 128), (236, 112)-(244, 128) | flag 123 |
| 6, 7, 8 | three force beams, 64 x 32, frame 4 | (164, 61), (164, 76), (164, 91) | all three flags |

The whole mechanism is two small scripts:

- **At load** (the map's on-load script): for each flag set, `FindEntityByType N` hides that flag's two lights
  (`docs/maps.md` 9e: step 30 hides decoration N, step 29 shows it). With all three flags set: `StopAudio 17` (the
  hum), hide 6, 7, 8 and `SetCollisionRect 184, 64, 224, 144` (opens the way through).
- **At the generator** (the script of the last tube broken, e.g. `0x08400388` in area 17): `StopAudio 17`, `PlayAudio
  18`, wait 60, `SetMapRestoreFlag`, `LoadMapWithEntities 8, 16, 1, 80, 24` (cut to the gate), wait 60, `PlayAudio 47`,
  hide the pair, wait 90, `SetStoryFlag 97`. If the other two flags are set: hide beam 6, wait 25, hide 7, wait 25, hide
  8, `PlayAudio 47`, wait 80, the hum off, wait 90. Then `RestoreMap`.

The generators: area 17 (tube flags 94-96, gate flag 97), area 39 (109-111, 112, reached through the switch cave),
area 50 (120-122, 123). A tube is the "breakable" kind described in section 2.4 with 1 hit. (`docs/story.md` speaks of
generators under the waterfalls of 27 and 40: those are `ch9_gero`'s added scenes, and Dr. Gero's stages stand there in
the stock game; area 17 is reached from area 40.)

**Can it be 6 and 6? Yes, with no engine work.** The six lights are six separate decorations. One flag hides one
light instead of a pair, and the all-six test replaces the all-three test. The cut-away, sounds and beams carry over
as they are. A light per room also says which rooms are done, if each flag keeps to its room. Only `SetStoryFlag`
and a fade-to-the-gate per lever are written anew, as lscript. The map copies the pillars with a `stamp` op; the sheet
is 2176 bytes (0x880, one light and one beam, 8bpp) and sits in OBJ VRAM for as long as the map is loaded.

### 2.2 The switches (zone 8 area 37; `map_239_z8_a37.txt`)

Six plaques on a cave wall, each with a coloured square and "ON" over "OFF" (red, black, purple, yellow, green, blue).
The OFF state is map art; decorations 0-5 (16 x 32, 8bpp, frames 0x00-0x50, sheet 3072 bytes, hidden at load) paint
the ON state. Trigger 0-5 use the activation "A pressed facing the object" (0x0800D87F) on a small rect below each
plaque. Each toggles its flag 101-106: set, show the decoration, `PlayAudio 57`. The combination is 102, 105, 106 on
and 101, 103, 104 off: then `LoadMapWithEntities 8, 38, 1, 0, 32` cuts to Vinnie's House, `PlayAudio 47`, the rock
door (decoration 0 there) hides, flag 107 is set and `RestoreMap` comes back. Breaking the combination runs the same
cut-away backwards.

So **the lever Ryan wants is one of these plaques**, doing what a generator's last tube does. Art cost: 512 bytes.

### 2.3 The trophies

Four chests (the `+0x1C` objects of a map, handler 0x0800FFE7), each with a line "Congratulations! You found the ...
Trophy!":

| trophy | item | chest at | flag (chest empty when set) | statue on the Capsule Corp pedestals (6/3) |
|---|---|---|---|---|
| Gohan | 46 | zone 10 area 33 (280, 256) | 302 | decoration 4 |
| Piccolo | 47 | zone 6 area 63 (156, 96) | 303 | decoration 2 |
| Trunks | 48 | zone 4 area 63 (144, 80) | 305 | decoration 3 |
| Vegeta | 49 | zone 8 area 30 (248, 80) | 306 | decoration 1 |

This answers a loose end in `docs/story-rom.md` ("No map script sets 302-306"): they are the chests' flags (the chest
conditions say so; I did not read the chest handler's write). **Art:** the statues are grey stone figures of each
character in a pose, 16 x 32 or 32 x 32, 8bpp, on 6/3's sheet (6144 bytes with the atrium's door). A fifth statue,
decoration 5, comes from flag 195 (the Cell Games): a spiky-haired Super Saiyan **(not identified for sure)**. Each
trophy item also has a 32 x 64 picture for its pop-up, which I could not decode into an image **(not verified)**. No
trophy exists for Goku, Hercule, Krillin or 16.

### 2.4 The breakables and what they drop

A breakable is a decoration (solid while it stands) plus an item record `{condition, decoration id, hits, mask 3, 0,
handler 0x0800F9A3, drop script}`; each hit shakes it, the last hides it, reopens the collision and runs the script.
They come back on every map load (`docs/maps.md`). `mapbuild`'s `rocks` block writes them. The jars:

| art | where | hits | sheet |
|---|---|---|---|
| black clay pot, 32 x 32 | zone 2 area 50, at (32, 160), (128, 272), (328, 272), and on three more stock spots; frame 0x20 | 3 | 2048 bytes (holds another frame too) |
| grey stone pot, 32 x 32 | zone 14 area 2, four spots round the arena; frame 0 | 3 | 1024 bytes |

Both use the stock large drop `0x083EE6A8`: a roll of 100 that spawns EP item 14 / HP 11 / EP 15 / HP 12 / EP 16 / HP 13,
nothing if the bar it picks is full. Food and energy heal a share of the maximum (`0x0800F592`, `0x0800F638`): items 11
and 14 are 10 %, 12 and 15 are 20 %, 13 and 16 are 30 % (16 by the pattern; 13, 11, 12, 14, 15 read from the code). Item
0 is the Senzu bean. **Two things to change for the chamber's jars:** the stock rolls read player 1's bars only, so a
hurt player 2 next to a healthy player 1 gets nothing; and "big reward" needs its own drop script (`SpawnItem 13` and
`SpawnItem 16` with no roll is two lines). Player 2 picks up drops since `src/coop/pickup.c`.

Other breakables that could serve: stone blocks (2/26), boulders, the flamingo, gnome and bird bath of 8/33. Buu's
Fury has a small drum, a blue barrel, and a purple star shape; I have looked at them only as sheets.

---

## 3. The six rooms

Ryan named Piccolo, the Frieza family, the pirates with General Tao, and Roshi. Six rooms need two more; the Lab and the
Saiyan ship below are **my proposal** (decision 2). "Ready" means the tournament fights this body today, with moves and
a portrait; numbers come from `src/coop/tourney_data.c`, `src/coop/tny_moves.c` `tny_kits[]`, `docs/ids.md`.

| room | alone | with a partner | status of the fighters | interior | entrance on the open map |
|---|---|---|---|---|---|
| **Roshi's dojo** | Master Roshi (186, portrait 80): Kamehameha, Thunder Shock, Drunken Fist, MAX Power at 50 % | adds Chiaotzu (189, portrait 42) or Yamcha (156, 114) | **ready** | LoG2 6/70 "Sparring Arena", 280 x 296: Roshi's own gym | small dojo front, drawn |
| **Pirate airship** | a crew of pirates, then General Tao (50 / body 239, portrait 53): Dodon Ray, Crane rush, hidden blade | twins on the crew, Tao stronger | Tao **ready**. Pirates: **work**, below | Buu 12/1 "Airship" deck, 400 x 320: crates and chains (clear its C-logo pad) | a hull drawn smaller; Buu's whole airship (0/55) costs 449 slots, too many |
| **Frieza's ship** | Frieza (48 / 237, portrait 49): Death Beam, Death Ball, turns into Mecha Frieza (49 / 238) at 40 % | adds King Cold (185, 64 x 64, portrait 70): throw, Death Beam | **ready** (all three) | Buu 8/19 "Inside Babidi's Ship", 400 x 400: purple walls, blue floor, central crystal. Fallback without Buu: LoG2 6/58 recoloured | the round saucer: `tools/shipart.py`'s method, smaller |
| **Namek ship** | Piccolo: the tournament's "Piccolo (Buu Saga)" (222, Buu r.212, portrait 168): Special Beam Cannon, Hellzone Grenade | adds the Namekian Warrior (166, portrait 171) | **ready**. It is the Buu-saga look, not stock sprite 103; a stock-look opponent needs one row and a kit | LoG2 6/58 "Capsule Ship 3", 320 x 320, recoloured green (round, seats, a blue energy column) | small Namekian dome, drawn; LoG2's Capsule 3 exterior (6/50) costs 267 slots |
| **The lab** (proposed) | Android 18 or 17 | Android 17 + 18 (9, 11; 28, 29) | **ready** | LoG2 8/63 computer room, 368 x 368; or Buu 12/28 "Control Room" | a bunker door, drawn |
| **Saiyan ship** (proposed) | Broly (159, portrait 141), rage at 50 % | adds Paragus (170, portrait 120), the story's rage rule if he falls first | **ready** | `data/maps/saiyan_ship.json` (15/9) again | Paragus' round ship from `shipart.py`, smaller |

Alternatives for the two free slots: Cooler's ship (Cooler 34 + the Namekian), or the Gero stages' Gero + Android 19. Both
ready. The Saiyan ship spoils Broly for a player who has not done that side chapter, and Cooler spoils his, though both
open in the same wait.

**Pirates.** No fighter in LoG2 is a pirate. Three ways: (a) the Warlord's Henchmen as they are (sprites 141-143):
they already wear a red bandana, they fight at range (AI set 11), and they are the cheapest body in the game at 0.5 KB;
(b) the same, recoloured striped by `spriteclone`-style code, one sprite id; (c) Buu's Fury's "pirate robot" as a
mini boss before Tao (r.7 / r.69): 64 x 64, groups 0, 1, 4, 9, 18 only, so it needs a fight set built like the
tournament's Buu imports. I would start with (a) and look. Decision 8.

**Bodies in a small room are safe.** Two 64 x 64 giants with a player leave 12-21 KB of OBJ VRAM (`docs/tournament.md`).
Each interior is its own map with its own 896 tile slots and nothing else in it, so these limits never bite here.

**Global limits, for the art in all of this** (`docs/sprites.md`, `docs/maps.md`): one 256-colour OBJ palette
(0x081DA6C8) for every sprite and one 256-colour BG palette for every map; new art maps to the nearest existing colour.
Buu's Fury imports go through `buu2log2.py` / `mapbuild` and need `roms/buu.gba` (`"requires": "buu"`; decorations,
animated tiles and non-chunk layers are not imported). OBJ VRAM is 32 KB in one pool; a map's decoration sheet sits in
it whole.

---

## 4. The open area's enemies

Stock numbers from the stat table (0x086D56AC) and the bestiary (0x086F8E40); HP before the co-op x 1.5. A tier is
two or three kinds. "Body" is the sprite record's OBJ size: most are 1 KB, the henchmen 0.5 KB, and 4 KB bodies (Destroyer,
T-Rex, Pterodactyl, Triceratops King) can only be used one at a time.

**The very weak ones are too weak for this moment.** The chamber opens at the ten-day wait, when the last regular
enemies the players met (zone 12) have 600-900 HP and today's round 1 has 675-700. Snake 53 has 35 HP and Kuma 69 has
125. So the first tier is the strongest bears and snakes of the game's lower half. They are still the weakest thing
the player meets in the wait. Levels at the wait are not measured here **(not verified)**: if Ryan wants truly weak
animals first, it costs nothing to change the table.

| lights out | enemies | stat / sprite | HP | melee / ranged / def | EXP | AI |
|---|---|---|---|---|---|---|
| 0 | Kuma Mercenary (bear) | 71 / 92 | 596 | 25 / 1 / 20 | 3610 | 8 |
|   | Snake | 14 / 116 | 530 | 32 / 1 / 28 | 3290 | 8 |
| 1 | Wolf | 57 / 148 | 675 | 29 / 1 / 24 | 1680 | 74 |
|   | Hawk | 107 / 64 | 700 | 25 / 1 / 30 | 3000 | 32 |
|   | Sabre-toothed Tiger | 92 / 108 | 750 | 32 / 1 / 14 | 6160 | 74 |
| 2 | Kuma Mercenary, the big ones | 70 / 91 | 940 | 35 / 1 / 32 | 9800 | 8 |
|   | Warlord's Henchman ("grenadier"?) | 12 / 142 | 900 | 10 / 30 / 26 | 3200 | 11 |
| 3 | Ninja | 75 / 96 | 900 | 34 / 25 / 27 | 5200 | 52 |
|   | Warthog | 56 / 145 | 900 | 55 / 1 / 30 | 25000 | 66 |
|   | Henchman, stronger | 51 / 143 | 1100 | 15 / 42 / 36 | 16200 | 11 |
| 4 | Tiger Bandit | 10 / 19 | 1120 | 34 / 42 / 25 | 25250 | 45 |
|   | Hawk, stronger | 84 / 65 | 1000 | 35 / 1 / 30 | 21000 | 32 |
|   | Eggbot (explodes; no twin in co-op) | 40 / 47 | 1130 | 38 / 44 / 34 | 33170 | 8 |
| 5 | Ninja, stronger | 86 / 98 | 1400 | 37 / 31 / 31 | 36800 | 52 |
|   | Sabre-toothed Tiger, stronger | 89 / 110 | 1750 | 30 / 1 / 24 | 30000 | 74 |
|   | Scorpion | 47 / 112 | 2000 | 70 / 1 / 56 | 50000 | 8 |
| 6 (gate down) | Snake, the top one | 48 / 117 | 3530 | 49 / 1 / 41 | 50000 | 8 |
|   | Wolf, the top one | 50 / 147 | 3980 | 54 / 1 / 30 | 60000 | 74 |
|   | Ninja, the top one | 45 / 97 | 1600 | 47 / 64 / 52 | 53000 | 52 |
|   | one Destroyer at most (4 KB) | 32 / 38 | 4200 | 49 / 44 / 43 | 58900 | 91 |
| after Popo, if wanted | Cell Junior | 26 / 31 | 4990 | 46 / 50 / 46 | 0 | 21 |

This is largely today's ten rounds (`src/coop/waves.c` `rounds[]`), spread over seven tiers, so these sets are all known
to run together. Melee, ranged and defence do not rise evenly: stock stats were placed per zone, not as a ladder, so
play will reorder this table. Stock EXP is also uneven (Wolf 57 pays 1680, Warthog 56 pays 25000); a stat override
(`ExtraStat.exp`) can fix it. The top tiers' 50000-60000 EXP against about 3.3 million for level 41 (`docs/hercule.md`)
and a cap of 50 means a fast grind; section 9.

Technical notes on swapping tiers and the limits are in 7.3 and 7.4.

---

## 5. Mr. Popo

**What he is like to fight (Ryan's words).** "he teleports to keep distance from you and shoots blasts, and he has
decent dodge."

**Art.** Two sources, nearly the same drawing:

| source | groups | size |
|---|---|---|
| LoG2 sprite 93 | idle (2 frames), walk (4) | 16 x 32, 0.5 KB |
| Buu's Fury r.183 (`docs/sprites.md`) | idle (2), walk (4), group 12 (2 frames, arms held out) | 32 x 32, 1 KB |

Neither has a hit-stun, an attack or a lying pose. Buu's r.183 is the better start because of the arms-out pose (it is
the blast and the teleport charge). The rest: hit-stun, knock-down, lying are drawn by code from his frames, as
`tools/tnysprites.py` does for the tournament's fighters. A small piece of art, one sprite id, nearest-colour palette;
his red, white and black exist. **Portrait:** stock 82 (`docs/ids.md`, speaker 82): nothing to do. **Bestiary:** he has a
row (76, stat 72) that says scanning reveals very little about him and that the data is hard to interpret, which suits
the scene.

**The AI.** Nothing in the game behaves like this today, but the tournament's move library (`src/coop/tny_moves.c`) gets
most of the way:

| wanted | base | work |
|---|---|---|
| Shoots blasts | `BALL`, `VOLLEY` with `MF_KI`, `HOMING` art 1: Android 18's Power Blitz and Infinity Bullet, Yamcha's Spirit Ball | none, one kit row. Energy colour through `tny_colours.c` |
| Teleports to keep distance | `MK_TELEPORT` with `MF_DODGE`: vanish with a flash, reappear 56 px away from the target, map checked (Videl's flight dodge) | **small**: the spot must be far from both players (96 px or more), inside the room; a new flag on the move |
| Decent dodge | no such thing; `kit_message` (the kit's wrap of the enemy's message handler) sees every player hit before the stock code | **small**: on a hit, a roll (start 1 in 3, with a 90-frame gap so a combo is not all misses) cancels it and fires the hop |
| Does not walk up and punch | Stock AI set 21 is melee; Super 13 and 16 run it between specials (`src/coop/super13.c`) | **small**: a kit flag that skips `Ai_Tick`, so he stands, turns and shoots. I found no stock set that retreats, but I did not decode sets 11, 45, 52 **(not verified)** |
| Poise | `KF_POISE`, Android 16's rule | none |
| At 50 %: faster, angrier | `XFORM` on the same sprite (Roshi's MAX Power, Broly's rage) | none |

Cell Juniors are no base for this: they are stock AI set 21, melee, with their own class. Super 13's own scheduler
(`super13.c`) is a separate code path for one boss; the kit library already has his volley, wave and nova moves, so the
kit is the cheaper base. A new `TF_POPO` row is not needed if
he has his own spawn with `tny_moves_attach`, but the kit table is indexed by `TF_*`, so a row is the simple way.

**Numbers to start from.** The tournament's rating curve (`opp_curve` in `tny_engine.c`) has Champion, the wait's tier,
from 350, Broly at 470, Super 13 at 490. Popo at about 500: 5200 HP, 40 / 42 / 42. With two players one lone boss gets
180 % (`COOP_LONE_BOSS_HP_PCT`). He hits with blasts only, so `ranged` is the number that matters. Decision 9.

**The hidden room.** A map of about 480 x 400, the white floor again, nothing else. Four jars at the corners, as the
stock arena does it, with the big drop: 30 % food and 30 % energy together, 3 hits. They do not refill during the fight
(a jar comes back only at a load) and the door is sealed by collision, as Dr. Gero's stages do.

**Scene outline** (no final lines; every step keeps someone moving):

1. Crossing the gate: fade, the players arrive at the bottom. A slow `ScreenShake`, low sound, a `FlashScreenColor`. Both
   players walk in to marks (with two players, side by side, `CoopCall 31`), stop, look left and right, turn to each
   other.
2. Far up the room a small figure with his back turned. Pan up, hold, pan back. The shaking stops when he moves.
3. One line from player 1's character by who he is. With two players, one reply from player 2's. Table per character,
   fallback a plain "Who's there?": Piccolo (he has Kami's memories, so he knows exactly who it is, "...Popo?"), Goku
   (Popo trained him), Gohan, Vegeta (dismissive), Trunks, Krillin, Hercule (terrified), 16 (his scanner returns nothing,
   the bestiary's line), Super Saiyan 2 Gohan.
4. Mr. Popo turns and walks down, in his third-person way of talking. He filled the chamber; he watched every fight. He
   says the name of whoever threw the last lever.
5. He stops halfway. A flash, and he is behind the players mid-sentence; they spin round. The fight: boss bar, door
   sealed. No pose held for long.
6. At 0 HP he does not die: the arms-out pose, a flash, he stands at the far end unmarked, a line. Reward, door opens.
   A repeat visit is one line and the fight.

---

## 6. Progression

### 6.1 Today's ten rounds and the story link

- **The ten rounds.** The door today runs them (`wait_chamber`, `waves.c`, CoopCall 13 / 14). My proposal: they go, and
  their enemies become section 4's tiers; `waves.c` is the natural home of the new spawner. Parts stay however this
  ends: `CoopCall 14, 2` is used by Roshi's spar for player 2's name, and test commands 13 / 14 (`COOP_CMD_WAVE_*`) by at
  least seven headless scripts (`pickup`, `regen`, `tny_artnew`, `tny_play` and others). Alternative: keep them as a
  choice on the panel. Decision 5.
- **The old room stays.** Zone 15 area 2 is also the stage of the Vegeta vs Trunks duel (`ch3_duels`). It keeps that job
  unchanged; the mode is a new map and the door points there. Its look (white floor, palace front) is what gets copied.
- **Gohan's Super Saiyan.** Today the first "Training completed!" before the Cell Games sets flag 468, puts Gohan on row 2
  with the caption, and the Games give it anyway otherwise (`docs/story.md` 4h). With no "completed" any more it needs a
  new moment. **My pick: the first lever thrown**, about the same effort as ten rounds, early enough that he uses it in the
  chamber. Alternatives: all six (the gate), or Popo: too late for most players. Decision 6. `gohan_ssj1.txt` changes with it.
- **Mr. Popo's line** "ten rounds, each one stronger than the last" has to change (it is Claude's, not dictated).

### 6.2 When it opens, and what it saves

**Opens** where it does today: flag 157, Cell's announcement. Goku and Gohan have just come out, which is why it is free.
The door only tests 157, so it stays open through the post-game. Why Frieza stands in a ship in the chamber: the chamber
shows what the people inside remember, or Popo made it so; a line from him is enough. Nothing here touches Hercule's
secret. Bosses are drawn at the wait's level and rise from there, so a return in the post-game is easier but not trivial.

**Flags.** None taken. Free and cleared at a new game: 443-445, 463, 469-475, 479 (12, scattered, and others want them).
504-815 are free but not cleared at a new game; raising `PARTY_WAIT_FLAG_BYTES` in `src/coop/party.c` from 13 to 17 clears
up to 535. **I would reserve 504-535 that way** (32 flags, room for all of the table below). Old saves should read them as
clear **(not verified)**.

| need | flags |
|---|---|
| Six levers (one light each) | 6 |
| First entry to the open area; the gate's fall seen; hidden room first scene; Popo beaten | 4 |
| **Minimum build** (the door of a room stays sealed until the lever is thrown, so "boss beaten" is not separate) | **10** |
| Each room's first-ever win, for rewards and short intros once levers reset | 6 |
| Times round (decision 4) | 2 |
| The panel's choice (or in RAM, reset on load) | 3 |
| Trophy hall: each trophy's fighter beaten | 5 |
| **Everything** | **26** |

Also needed later, not taken now: one extra stat for Popo's spawn (155-159 are free) and one for the crew if recoloured;
sprites for Popo and perhaps a crew recolour (163-164, 167-169, 171-174 are free); a CoopCall from 32 on (registry says 32
is the story piece's, 33+ free, check at build time); nine maps (the registry lists zone 15 areas 18-19 free; the area is
a byte, the registry just needs extending); RAM: `partyram` has about 0x150 bytes free at 9 slots, and `WaveState` is
0x9C today.

### 6.3 Replay and rewards

| thing | what | cost |
|---|---|---|
| EXP | From kills, stock amounts per tier (today's only reward) | none |
| First win in each room | A +3 capsule, as Roshi's gym pays (`ITEM_STR3`, `PWR3`, `END3`) | small |
| Popo, first win | Something large: a Senzu bean (item 0) and a capsule of each, say. Decision 9 | small |
| **The loop** | After Popo he offers to reset: lights back on, tier back to 0, every boss a step stronger, the first-win rewards replaced by a Senzu | small to medium |
| **The panel** | Inside the quarters (a small room behind the facade: the save pad, the stock six switches): pick which tier is outside, among those you reached. Ryan's "organized buttons to press for a specific animal", and it lets a player level on chosen enemies | small, a load swap |
| **The trophy hall** | A room with the stock statues on pedestals. A statue stands there only if you found that trophy in the story (flags 302, 303, 305, 306, tested at load), so the old chests matter. Walk up, press A: that character fights you | medium: four rows and kits, Vegeta, Piccolo, Trunks, Gohan; their player-layout bodies run on set 21 (Mystic Gohan, Majin Vegeta, Buu-saga Piccolo already do). Goku, Hercule and Krillin have no statue |

---

## 7. Technical notes

### 7.1 The maps

| map | source | notes |
|---|---|---|
| Open area | new, `mapbuild` (as `time_chamber.json`: `texture` floor, `stamp` from 9/1 for the facade, 8/16 for the pillars) | about 1280 x 960. Stock precedent: 9/1 is 1536 x 1024, 4/55 1448 x 560 (`docs/widescreen.md` for the streaming). Collision bitmaps go to 2048 x 2048 |
| Six interiors | `stamp` whole from the table in section 3 | each has its own slots; a lever plaque per room; header flag 1 (see below) |
| Hidden room | new, small | |
| Quarters (if the panel is built) | new, small | |

**Set the map header's flag +0x08 to 1 on all of them**: 0 is the "town" rule that switches every ability off
(`docs/perfect-pass.md` section 9, Papaya Island's bug).

**The tile budget of the open map is the first thing to design around.** At most 896 8bpp tile slots per map. Measured with
`maplib` over rects on the stock layers: the Lookout facade 239, the gate's pillars 38, Buu's Babidi dome 103, LoG2's
Capsule Ship 3 exterior 267, Buu's stolen airship 449; `yard_ship.json` reaches 643 with one 256 x 160 ship. So the void's
floor (8 variants), facade, pillars and six entrances fit only if **each entrance is about 96 x 80 px and left-right
mirrored, at most about 70-100 slots**, and the facade may need cutting down. A new tool in the family of
`tools/shipart.py` (art drawn by code, no ROM data) can make all six; `{"op": "png", "draw": ...}` paints them in. The
decoration sheet of a map is one sheet, so the gate, the jars and any other objects join into one (`arctic_summit.json`
did this): 0x880 gate + 0x400 jar = about 3.2 KB of OBJ VRAM held on the open map.

### 7.2 A lever

A plaque in a room: tile art OFF, a decoration ON, a script trigger with activation 0x0800D87F on a rect below it. Its
condition is tested at load only, so it is always there and the script tests "boss down". Player 2 can fire it
(`agency.c`).

```
; sketch, not final
sequence room_lever
  script
        ; not beaten yet, or thrown already: PlayAudio 19, END (the stock refusal)
        SetStoryFlag FLAG_LEVER_n
        FindCharacterEntity 0              ; ON
        PlayAudio 57
        WaitFrames 40
        SetMapRestoreFlag
        LoadMapNoEntities ZONE, OPEN, 1, GATE_X, GATE_Y
        WaitFrames 60
        PlayAudio 47
        FindEntityByType n                 ; the light
        WaitFrames 90
        ; all six: beams one by one, the hum off (stock timings, section 2.1)
        RestoreMap
        ; open the door by collision; the next load of the open area does the rest
end
```

`LoadMapNoEntities` avoids building the void's enemies just to look at the gate; the stock uses it in that pattern
(`docs/engine.md`). A warp without entities removes player 2's entity until player 1 is free (`docs/scripts.md`,
CoopCall 5). The stock generator scenes use `LoadMapWithEntities` and work with two players, so that is the fallback if the
no-entities load misbehaves; `RestoreMap` from a map this big **(not verified)**.

A boss fight's plumbing: spawn from a record and attach the kit (`tny_moves_attach` is not tied to the ring: test
command 26 spawns a fighter on any map); it stays down at HP 0 (`fallen()`) "until the engine clears the body", so the
chamber needs the small watcher that the ring has in `tny_engine.c` to see the fall, clear the body, set the flag and open
the door. Stats: `tny_resolve_stat` works through `TNY_STAT0 + id` and `eng.tier`; a chamber override of the rating is
small. `tny_talk_pre` already chooses lines by who meets whom (yourself, rivals, team vs team, defaults), so the room
intros can use it **(not verified outside the ring)**.

### 7.3 Swapping the enemies at load

Enemy records and triggers are built at a real load, conditions tested then (`docs/scripts.md` 2.3, `docs/maps.md`). The
player always arrives by a door, so this suits the mode: the lever sets the flag, the door out loads the open map, the
new tier stands there. Two ways:

- **A. Conditions on records.** `{0x0800E77F, condition, stat, x, y, sprite, action, count, behaviours}`, one record per
  spot per tier; `mapbuild` `records` with `"like"`. About 8 spots x 7 tiers = 56 records (the count is a byte; stock's most is
  15, but untaken records cost only the test). A condition word tests one flag, so "exactly three levers" is a condition
  script with C behind it, or six more monotonic tier flags. This is the plain stock way and needs no C **except for that count**.
- **B. One list and a table (my pick).** 8-12 "spot" records with no condition. `enemy_spawn_hook` (`enemies.c`, every enemy
  already passes through it, and the twin path already makes a private record copy) sees the chamber's map and gives the
  spot the current tier's stat and sprite from a table in C. No extra flags, the panel is one variable, and the table is
  easy to reorder in play. The body's OBJ size comes from the sprite at construction, so it is right for each tier.

**A refill (optional, worth having).** The stock rule: kills stay dead until the next load, so a walk across the void
empties it. `waves.c`'s spawn code (records in RAM around the players, the loader's `EntityList_Add`) can top up out of
view when few are left, within the limit below. It is how a small number of bodies can feel like a populated big area.

### 7.4 The limits on enemies

**Sprite memory, not a count, is the limit** (the old finding in `docs/coop-design.md` on the waves), and it is worse on a
big map: `FUN_080075A8` takes the body's OBJ VRAM at construction, so **an enemy across the map holds its block** even off
screen. Off-screen bodies give way only under pressure, when a text box asks (`docs/engine.md`, `objroom.c`).

| | |
|---|---|
| OBJ VRAM | 32 KB. A body: 1 KB (most), 0.5 KB (henchmen, Popo's 93), 4 KB (64 x 64) |
| Taken | players (1 KB, or 4 KB as Cell, Cooler, Broly), HUD panels, the map's decoration sheet (3.2 KB here), projectiles |
| A text box with a portrait | 14 KB in one piece (0x3800); evicts off-screen bodies first |
| Measured | today's 8 bodies standing: largest free block above 6.9 KB, but a pterodactyl is 4 KB in those rounds; with two Destroyers only 2.2 KB |
| Entity list | 128 items, 22 OAM entries at most in anything measured |
| Twins | `TWIN_MAX` 24 pairs |
| Stock's most enemies on a map | 15 records (4/59), 13 (12/12), 12 (12/2, 4/57) |

**Working number: at most 16 regular bodies on the open map at once**, so 8 records with twins in co-op or up to 12-14
alone, no more than one 4 KB body. To be measured with `waves.obj_low`'s method on the real map.

**Frame time** (`docs/perfect-pass.md` 11.6; emulated clock, headless): 4/59 with 15 enemies 46 % of a frame alone, 52 % in
co-op (and those triceratops no longer spawn in co-op); Map Test 13's doubled wolves 40 % alone, 56 % co-op; a Rookie Rumble's
busiest frame 95 %. **The warning:** `tny_blasts.txt`, those doubled wolves with both players firing: 8-12 of 120 late
frames, busiest 120-190 %, the one place near the budget. So a big fight of 16 on screen will dip. The rooms are nothing
like as loaded. Whether off-screen enemies still run their AI: I read culling as draw-only (`docs/widescreen.md`) **(not
verified)**.

### 7.5 Saving, dying, leaving

Lights are flags, so a save keeps them and a game over loses what was not saved: hence the pad in the quarters. The door
home is always open, and leaving ends nothing (lights stay). During a boss fight the room's door is sealed, so a fight is
not left half done. If Ryan wants no lost progress on a death, one more pad near the gate is cheap, or the
lever could ask to save.

---

## 8. Phases

Sizes are rough, for one focused agent session each with tests: **S** under about 300 lines, **M** 300-800, **L** more or
several sessions. Each ends tested headless; real-time feel is Ryan's.

| # | phase | what | size | test |
|---|---|---|---|---|
| 0 | Decisions | Section 10; draw the map on paper | - | - |
| 1 | The open area and gate | The map, door swap, pillars, lights, beams, jars with new drops, save pad; a debug way to set flags. Prove 6 lights by flags at load, and the cut-away from a stand-in room | M | new `chamber_gate.txt`; `perf_probe` on the map |
| 2 | Tiers and spawning | Way B's hook and table, seven tiers, twins, measure OBJ VRAM and frame time per tier. The refill if the measure allows | S-M | `chamber_tiers.txt`, each tier's `obj_low` |
| 3 | One room, complete | Roshi's dojo end to end: door, intro with `tny_talk`, kit fight, watcher, lever, cut-away, tier change. This is the framework; do it in 1 and 2 players and with a transformed player | M-L | `chamber_room.txt` both modes |
| 4 | The other five rooms | Interiors, boss rows, pairs, stat scale by lights out, the pirates | M | one self-checking block per room |
| 5 | Entrance art | The `shipart`-family tool for six entrances within the slot budget | M | previews for Ryan; `mapbuild` slot count |
| 6 | Mr. Popo | Body, kit extras (far hop, dodge roll, no-walk flag), hidden room, big jars, scene with lineup lines | L | `chamber_popo.txt`; a bot fight for balance |
| 7 | Story and text | Gohan's moment, Popo's door lines, room lines for all lineups, journal if wanted, retire or keep rounds, fix the tests that lean on them, `docs/ids.md` | S-M | `gohan_ssj1.txt` changed, `regfull.sh` clean |
| 8 | Replay | Rewards, the loop, then the panel; the trophy hall last | S + S + M | one block each |
| 9 | Balance and play | `balance.txt`-style numbers per tier at wait levels, then a two-player session | S, then Ryan | drop-in states F-keys for the chamber |

Phases 1-3 are the proof, about a third of the work, and show whether it is fun before the art is drawn. Phases 4 and 5
could run in parallel on separate branches, as the tournament's team did, with numbers allocated in `docs/ids.md` first.
The whole is much smaller than the tournament was, because the fighters, moves, boss bars and dialogue system exist.

---

## 9. Risks

1. **Sprite memory on the open map.** Too few enemies to feel like a big area. Answered by the refill. If 16 feels thin, a
   smaller map (960 x 720) is denser for the same bodies.
2. **Tile budget.** Six good buildings in 896 slots is tight. If they look poor at 96 x 80 the map could become two or three
   linked maps instead of one, which loses "one big open area" but gains slots and memory (decision 10).
3. **Frame dips** with 16 bodies and both players firing (120-190 % measured on doubled wolves). Mitigated by fewer twins on
   the map (`no_twin` by tier) or fewer records in co-op.
4. **Grinding too fast.** Stock top-tier EXP (50000-60000) with a reload refill and a cap of 50 could trivialise the Cell Games.
   The refill makes this sharper. `ExtraStat.exp` or an EXP scale in the chamber.
5. **Kit outside the ring.** `tny_moves.c` has only been played in the ring and on test spawns. Watching a fall, clearing the
   body, `tny_on_fall`'s announcer, `eng`-based stats and the combo meter all assume the tournament. Phase 3 finds out early.
6. **Cut-aways with two players.** `LoadMapNoEntities` and `RestoreMap` from a big map, player 2's entity, the twins' registry
   at restore. The stock scenes work; this load is larger.
7. **Buu's Fury maps** need `roms/buu.gba`: the pirate deck and Frieza's room. Other chapters with `"requires": "buu"` already
   have this rule, but this map chain must not break with `BUU=0` (the door could keep the old rounds there), and non-chunk
   layers do not import, so the look needs checking.
8. **Regression.** `gohan_ssj1`, `pickup`, `regen`, `tny_play`, `tny_artnew` and others use the chamber or the wave commands.
   Keep the test commands.
9. **Story order.** A boss of an unplayed side chapter (Broly, Cooler) spoils it; and Piccolo, or anyone, fighting himself
   needs a line. Tournament's "meeting yourself" table covers this if it runs here.
10. **Save size.** Flags 504-529 are inside the existing bank (cap 815), so no layout change is expected **(not verified on an
    old save)**.

---

## 10. Decisions, settled (Ryan, 2026-11-05: "yes lets build it like that. itll take a very large time chamber area")

1. **The gate:** a copy of the Lab's gate, white pillars, six red lights, three beams, one light per room.
2. **The six rooms:** Roshi, the pirates and Tao, the Frieza family, Piccolo, the Lab with 17 and 18, the Saiyan ship with Broly.
3. **Order:** rooms in any order, bosses growing by lights out.
4. **After Popo:** he offers to start again, harder.
5. **The ten rounds:** gone, their enemies become the tiers (test commands 13 / 14 and `CoopCall 14, 2` stay).
6. **Gohan's Super Saiyan:** at the first lever thrown.
7. **The enemies:** "grenadiers" are taken as the Warlord's Henchmen, and section 4's table stands until play says otherwise (Claude's call, Ryan had no objection; easy to change).
8. **Pirates:** Warlord's Henchmen as they are, to start.
9. **Popo:** about rating 500, the top of the game as section 5 starts him; the reward as section 6.3 (Claude's call, tune in play).
10. **Big area:** one very large map with about 16 enemies at once and a refill as you walk. Ryan expects it very large; go as big as the tile and memory budgets allow.
11. **First build:** the panel and the trophy hall after the rest has been played.

---

## 11. Not verified

Read from code and dumps but not run; each is a thing to test in its phase, none is a blocker as far as I can see.

- Players' levels at the wait; how many punches a tier takes. No fight was run for this plan.
- `RestoreMap` from the larger map with two players; `LoadMapNoEntities` in the lever's sequence.
- `tny_talk_pre`, the kit's fall handling and the boss bar outside the ring (test command 26 proves the spawn only).
- Which AI sets retreat: 11, 45, 52 not decoded. What exactly the Henchmen's ranged attack is.
- The chest handler writing the trophy flags: inferred from the chest conditions (`0x2012E` etc.) and the statues' on-load.
- Which character statue decoration 5 shows. The trophy items' pop-up art did not decode with my quick script.
- Item 16's 30 % (the two below it read from `0x0800F638`, the same constants).
- Off-screen enemies: running their AI or not. Old saves reading flags past 503 as clear.
- A condition script using a CoopCall at load (way A only; way B avoids it).
- How the Buu interiors look in LoG2's palette without their non-chunk layers; all interior choices are from renders of the
  stock maps, decorations excluded, not from a build.
- Slot counts are single rects on one map's slot table, a good estimate of what a stamp adds, not a `mapbuild` result.

(Phases 1-3 settled several of these: the cut-away from a room, the kits, the fall and the boss bars outside the ring,
CoopCall-based record conditions at load, old saves reading 504-535 as clear. Section 12.5 lists what is still open.)

---

## 12. State of the build (phases 1-3, branch `nb-chamber`, 2026-11-05)

Built: the open area with its gate, jars, save pad and enemy tiers; the room framework; Roshi's dojo end to end; the ten
rounds retired for players. Code: `src/coop/chamber.c` / `chamber.h`; scripts `data/story/chamber.lscript`; maps
`data/maps/chamber_open.json` (15/18), `chamber_dojo.json` (15/19); placeholder entrances `tools/chamberart.py`; tests
`tools/headless/chamber_gen.py` (writes `chamber_gate.txt`, `chamber_tiers.txt`, `chamber_room.txt`, `gohan_ssj1.txt`,
`chamber_perf.txt`) and `tools/headless/chamber_save.py`. Every line of dialogue is Claude's draft (marked DRAFT).

### 12.1 What a player sees

Mr. Popo beside the Lookout's chamber door (from flag 157) says the chamber has six rooms and a gate with six lights
(`wait_chamber.lscript` `lk_popo`; his old "ten rounds" line replaced). Through the door: a fade, the white void, the living
quarters behind the players, the gate's two pillars far to the north with six red lights and three humming pink beams, six
coloured placeholder buildings round the void, grey stone jars beside each and two at the gate, a save pad right of the
quarters. Bears and snakes come at them; walking away, the far ones leave off screen and new ones walk in out of view ahead
(the refill), so the void always has company. The beams stop anyone walking between the pillars. Five buildings say "The
door will not open." (placeholder, phase 4). The red one's door is Roshi's dojo.

In the dojo (Roshi's own gym, the kanji on the mat) Roshi stands at the far end, Chiaotzu beside him with two players. A plaque
on the back wall clicks when pressed. A few steps in: both players turn to him, Roshi (and Chiaotzu) walk up, Roshi has a line
for whoever player 1 is (one per character, a default), Chiaotzu his, each player's character replies, the door's beam
flashes across it with a shake, boss bars, the fight (the tournament's kits: Kamehameha, Thunder Shock, Drunken Fist, MAX
Power; Chiaotzu's Dodon Ray and psychic hold). When both are down the bars slide away, Roshi's last line from the floor
("throw that switch on the wall"), the bodies blink out. The door stays sealed: the plaque is the way out. A press: it flips
to ON, the screen cuts to the gate, one light goes dark with the stock power-down sound, back in the dojo the door's beam
goes, and the first time before the Cell Games: "Gohan can now transform into a Super Saiyan!" (decision 6). Outside, the
next tier. The sixth lever also drops the three beams one by one and the hum stops, and from then on the way between the
pillars is open (nothing is behind it yet: phase 6). A room once won is empty, its plaque on.

### 12.2 The open area (phase 1)

| thing | built |
|---|---|
| map | 15/18, **2048 x 2048 px**: the most the collision bitmaps take (256 x 256 tiles, `docs/maps.md` 6), about 8.5 x 12.8 screens |
| tile slots | **372 of 896**: floor and quarters 214, the gate's pillars and the save pad 94, the six placeholders 64. Without the placeholders 308, so **588 slots are left for six entrances: 98 each** |
| layout (px) | quarters' door (976, 1224)-(1048, 1256) back to 9/3; arrival (1016, 1272); gate pillars x 984-1072, y 320-400 (between them x 1008-1048 blocked until all six); entrances 96 x 80 at Namek (384, 480), Frieza (1568, 480), Saiyan (192, 1056), pirate (1760, 1056), lab (384, 1664), dojo (1568, 1664), each with its door 32 px wide at +32, +56; save pad (1200, 1208)-(1249, 1249) |
| header | flags +0x08 = 1: the world switch every ability reads (0x0300208C) is 1 there (tested) |
| decorations | one sheet joined from parts of stock sheets (`mapbuild` `objects`): 0-5 the lights (8 x 16, 8/16's sheet), 6-8 the beams (64 x 32), 9-24 sixteen jars (14/2's stone pot, 3 hits); 3200 bytes of OBJ VRAM |
| on-load | `ch_open_onload`: the hum (`PlayAudioVolume 17, 40`); flag 504 + n hides light n; all six (`CoopCall 35, 15`): the hum off, 6-8 hidden, the gate's rect open. On-leave `ch_open_onleave` stops the hum (also when a cut-away's `RestoreMap` leaves) |
| jars | drop `ch_jar_drop` (`CoopCall 35, 17`, `chamber.c` `drop_item`): both players' bars; nothing when both are full (the stock rule), else food or energy by which bars are short, 10 % half the time, 20 %, 30 % |
| enemies' drops | `ch_enemy_drop` (`CoopCall 35, 18`): a quarter of kills roll the same table |
| save pad | the stock pad (13/7's art): a save there loads back in the open area with the lights as saved (tested) |
| load time | 78 frames of black (stock: the Lookout 9/1 54, the old chamber 48, West City 24): the tile banks and the bitmaps |

**Flags.** 504-509: room 0-5's lever thrown (= light n out). 510-535 reserved for the rest of the plan (section 6.2's
table: first entry, the gate's fall seen, the hidden room, Popo beaten, first wins 514-519, times round, panel, trophy
hall). A new game clears them (`party.c` `party_new_game`, called by the save menu's start game; tested in `chamber_gate`).
Not in `party_init_hook` (`PARTY_WAIT_FLAG_BYTES` stays 13): a Map Test runs that one, and the Map-Test-seeded tests
`pickup`, `regen` and `hercule_kit` drew differently when it grew by four bytes' worth of cycles (found in the full
regression, 2026-11-05).

**An existing save is safe (proved 2026-11-05).** The party block and the EEPROM layout do not change (flags 504-535 were
always inside the 816-flag bank). Ryan's `build/play/legacy-of-hercule.sav`, copied (the original untouched, its hash
unchanged): all three slots valid, flags 504-535 clear in each, nothing above 503 set; the same in all 41 backups in
`build/play/backup/`; no code writes a flag above 503. A real load of each slot in the new ROM gives the same party block,
byte for byte, as the same load in the current play ROM (`chamber_save.py load ROM SAVE --base PLAYROM`), flags 504-535
clear. The bytes a load changes in slots 0 and 2 are the older load fixes `hercule_lock` / `krillin_lift` (both ROMs
alike); `chamber_save.py` allows exactly those (`LOAD_FIXES`). It is in `regfull.sh` (`SAVE=` for another file).

### 12.3 Tiers and the refill (phase 2)

**Way B.** The map's seven "spot" records have stat 155 (`CH_SPOT_STAT`); `enemy_spawn_hook` hands such a record to
`chamber_spot`, which gives it the next kind of `tiers[lights out]` (stat, sprite, EXP), the wild-wolf wander and the
record's own drop, on a private stat copy (so one-player spawns copy it too). Twins as everywhere (Eggbots none). The tier
is the lights out now, so a lever's change shows at the next load of the open area (the door back from the room).

| lights out | kinds (stat / sprite) | EXP a kill |
|---|---|---|
| 0 | Kuma Mercenary 71 / 92, snake 14 / 116 | 3000 |
| 1 | wolf 57 / 148, hawk 107 / 64, sabre-toothed tiger 92 / 108 | 4000 |
| 2 | big Kuma 70 / 91, Warlord's Henchman 12 / 142 | 6000 |
| 3 | ninja 75 / 96, warthog 56 / 145, stronger Henchman 51 / 143 | 9000 |
| 4 | Tiger Bandit 10 / 19, stronger hawk 84 / 65, Eggbot 40 / 47 | 13000 |
| 5 | stronger ninja 86 / 98, stronger tiger 89 / 110, scorpion 47 / 112 | 18000 |
| 6 | top snake 48 / 117, top wolf 50 / 147, one Destroyer 32 / 38 (4 KB: at most one, and only with 5 KB free) | 25000 |

**EXP (risk 4).** Stock EXP is uneven (a warthog paid 25000, a wolf 1680). The stock award (0x0800E4D2, `exp.c`) clamps
a kill to between 1/128 and 1/2 of the killer's level span. The ladder above is about 35-45 kills a level where each tier
is met: tier 0 at level 33 (span about 190000) is 66 kills, tier 3 at level 36 (330000) 37, tier 6 at level 42 (1.1
million) 44. Every body pays, a twin pair is two kills. Tested: a body's stat carries the table's EXP, a kill at level 36
pays.

**The refill.** Every 16 frames (`open_tick`; not during a scene, nor while player 1 is not free): a body more than
300 x 220 px from the players' midpoint leaves (one a check, its OBJ VRAM freed; the list removal the tournament uses);
while fewer bodies stand than the target, one record comes in 170 px left or right or 125 px above or below (twelve
spots, a walkable one), never inside the 240 x 160 view; 90 frames' breath after a kill; never while OBJ VRAM's largest
free block is under 6 KB.

**The enemy count, from the measures** (`chamber_perf.txt`, headless; both players idle, then both holding B with their ki
topped up; tiers 1 / 4 / 6; "records" is the target with two players):

| two players | bodies | largest free OBJ VRAM | busy avg standing | firing: lagged frames of 120, busy avg |
|---|---|---|---|---|
| 5 records | 10 | 13.3 KB (tier 6: 8.2) | 55-59 % | 3-23, 62-78 % |
| 6 | 12 | 12.3 KB (tier 6: 6.1) | 60-64 % | 9-33, 67-100 % |
| **7 (chosen)** | **14** | **10.2 / 9.2 KB (tier 6: 3.1, the spawn floor holds it)** | **62-65 %** | **9-18, 67-80 %** |
| 8 | 16 | 7.2 KB (tier 6: 2.0) | 61-66 % | 9-25, 70-88 % |
| 9 | 18 | 6.1 KB (tier 6: 0.4) | 63-68 % | 9-29, 70-94 % |
| 10 | 20 | 4.1 KB (tier 6: 0.4) | 63-71 % | 11-26, 72-91 % |

| one player | bodies | largest free | busy avg | firing |
|---|---|---|---|---|
| 7-10 | as many | 19 KB down to 14 KB | 39-44 % | no lagged frame at any count |

**Chosen: 7 records with two players (14 bodies), 10 bodies alone** (`CH_TARGET`, `CH_TARGET_ONE`). With two players
the firing lag at 7 is what the stock's doubled wolves showed (`docs/perfect-pass.md` 11.6: 8-12 of 120) and grows past
8; OBJ VRAM at 8 and more leaves too little for the text box's 14 KB to come from off-screen bodies. Alone the frame never
lags, so 10 (the plan's 12-14 would fit too; 10 keeps one player's fight readable). A text box evicts off-screen bodies first
(`objroom.c`): the save pad's menu works on a full void (tested).

**The old rounds.** The Lookout's door no longer starts them; `waves.c` stays: test commands 13 (every standing enemy
dies) and 14 (skip to round N) and `CoopCall 14` ops 2 / 3 (player 2's name, the Cell Games' cards) work as before; test
command 48 op 5 starts the rounds on 15/2 without their captions (`chamber_tiers.txt` checks 13 and 14 there).

### 12.4 A room (phase 3): the framework, and how to add one

One row of `rooms[]` in `chamber.c` plus a map. Shared by every room: `ch_room_onload`, `ch_boss_cond`, `ch_partner_cond`,
`ch_room_intro` / `ch_room_parts`, `ch_lever` (`data/story/chamber.lscript`) and `chamber.c`'s state machine (`room_tick`):
R_IDLE -> (the intro's last entry, `CoopCall 35, 13`) R_ARMED -> (the scene ends: the actors become fighters on their
spots, `tny_moves_room_spawn`, bars; who stands decides the pair, not the pad at that moment) R_FIGHT -> (every fighter at 0
HP) R_FALL (bars out, 48 frames) -> R_TALK (the lead's `won` line) -> R_CLEAR (24 frames' blink, bodies removed) -> R_WON
(the lever answers). Leaving the map resets it; a death in a room is the stock death and game over.

| row field | meaning (Roshi's value) |
|---|---|
| `area` | the map's zone 15 area (19) |
| `light` | 0-5: the lever's flag 504 + light, and the gate's decoration `light` (0) |
| `alone` | one player: `{fighter, rating at 0 lights, HP %, won line}` (Roshi, 350, 100) |
| `pair[2]` | two players: the lead and the partner (Roshi 350, 70 %; Chiaotzu 330, 70 %); `pair[1].fighter = NONE`: a lone boss against two (give him about 180 %) |
| rating | `rating + 25 x lights out` on the tournament's curve (`opp_curve` through `tny_resolve_stat`, `chamber_rating`): 350 = 2800 HP, 25 / 26 / 26; 475 = 4750 HP. The difficulty's boss HP goes on top, as for every boss |
| `mark[2]` | where the lead and the partner walk to before the fight ((124, 136), (164, 136)) |
| `seal` | the door's strip, blocked during the fight ((120, 272)-(160, 280)) |
| `line` | a player past it (below; with `look` left, right of it) when the scene starts is put beside player 1 (256; until phase 4 `line_y`) |
| `open`, `open_by` | the boss's opening line, and a table of nine by party slot: player 1's character's line, else player 2's (a fighter meeting himself, a player's own story people), else `open` (since phase 4 a pointer to a shared table, 0 = none) |
| `partner_line` | the partner's line (two players) |
| `flags` | (phase 4) `RF_CREW`: the actors showing `crew_sprite` become regular enemies when the intro ends (twins with two players), the lead watches; when the last is down, `crew_line` as he steps 24 px towards the players (`ch_crew_parts`, ops 21 / 22), then his fight. `RF_RAGE`: the partner falling first enrages the lead once (`rage_line`; `tny_moves_rage`: full HP, his kit's next transform or the library's power-up, the pose and the shake) |
| `look` | (phase 4) the way both players face at the intro: `LOOK_UP` (the bosses above) or `LOOK_LEFT` (the airship: the door on the right) |
| `plain` | (phase 4) the row a build without `roms/buu.gba` uses (`have_buu()`: Buu-saga Piccolo's art is in the build): its fallback map's places and fighters |

The lines' order: the boss, the partner (two players), player 1's reply, player 2's reply (`replies[]`, one per party
slot, shared by every room; at most four lines, `tny_talk_scene`). The box sits at the bottom (no `!`): the bosses stand above.

**The map** (copy `chamber_dojo.json`): a base or a stamp; `"priorities"` with the floor's layer at 2 or more (at the stock
dojo's 1 the floor covered the door's beam and every Kamehameha); `objects`: decoration 0 the plaque's ON frame (16 x 32,
8/37's sheet: frame 0 red, 16 black, 32 purple, 48 yellow, 64 green, 80 blue; a colour per room) and 1 the gate's beam (64 x
32) across the door, both `"visible": 0`; the plaque's OFF art stamped from 8/37 (`[12, 15, 3, 6]` on BG2 is the red one;
the others stand at 8/37's decorations' places) and its legs' row blocked; records: the lead's actor (his
`tny_fighters[].sprite`, condition `script:ch_boss_cond`) and the partner's (`script:ch_partner_cond`); triggers:
`ch_room_intro` on a rect across the room some steps in from the door (`"activation": "enter"`, condition `noflag:` the
lever's flag), `ch_lever` on the plaque (`"activation": "facing"`, its rect from about 16 px above the plaque's foot to 24
below: the box test does not see a player standing right at the wall otherwise); the door exit below the seal (reachable
only while the seal is open); `"onload": "script:ch_room_onload"`; `"flags": 1`. On the open map: the entrance's door exit
to the room; the room's exit back to just below that door.

**Adding a room, the whole list:**
1. A map `data/maps/chamber_<name>.json` as above (areas 20-24 are kept for the rooms, `docs/ids.md`).
2. A row in `rooms[]` (`chamber.c`), its lines marked DRAFT.
3. In `chamber_open.json`: the entrance's door exit to the new area, in place of its `ch_door_shut` trigger.
4. The bosses' sprites must have walk group 1 (`docs/sprites.md`); the tournament's fighters all do.
5. Tests: `chamber_gen.py` `room()`'s cases go to Roshi's door and check his actors: make the door spot, the sprites and
   the light parameters of `case()` with the second room, add its cases, regenerate, run with `staging_run.py`.
6. `docs/ids.md` (the area) and this section.

**What the tests cover** (self-checking, in `regfull.sh`): `chamber_gate` (93 checks: the door, the switch, each light and
pairs of lights by flags at a load, the beams shut and, with six, open the way; a jar with both bars full drops nothing,
one for a hurt player 2 drops; the four corners and the border; a save at the pad, a soft reset, a load with two lights
out); `chamber_tiers` (each tier with two players and alone: the bodies, the stat's EXP and HP, a kill's EXP, the refill
back to the target; the old rounds' commands); `chamber_room` (161 checks: one player Piccolo winning by real punches; Gohan
+ Piccolo with player 2 throwing the lever; Super Saiyan Goku (a real B before the room) + Hercule; Hercule + Super Saiyan
Vegeta on player 2's B; Krillin + Vegeta with five lights out: HP by lights, the sixth lever's beams falling one by one;
each: the actors by lineup, the plaque's click before, the intro's scene ends, the seal, the fall, the bodies gone, the
cut-away with the light on then off, the flag, the door open, Gohan's gift, the next tier outside, a return to an empty
room); `gohan_ssj1` (Gohan as player 1 and as player 2); `chamber_save`. A death in the room (one player at 1 HP): the stock
death and game over, the room's state cleared (checked by hand).

**Full regression** (`regfull.sh build/t`, 2026-11-05, final build of the branch): 90 blocks, no FAIL, TIMEOUT or CRASH.

### 12.5 Not verified, open

- Real time: the feel of the refill, of 14 bodies, and the room fight's balance (Roshi 2800 HP alone; 1960 + 1792 for two)
  are headless numbers; Ryan's play decides. The levels at the wait are still not measured.
- The cut-away loads the open area without entities from a room (the engine stashes the room's entity list, 0x08006F4A,
  and brings it back at `RestoreMap`): fine in every headless lineup, with player 2 and with transformed players.
- `WIDE=1`, `BUU=0` and `PARTY_SLOTS=8` builds compile; `chamber_room` also passes on the `BUU=0` build (161 checks); the
  wide view was not run.
- The gate's stamped pillars carry a few stock pebbles beside their inner edge (zone 8 area 16's art); harmless.
- (Phases 4 and 5 are built: 12.6; phase 6 and phase 8's loop: 12.7.) Phase 4: five rooms (rows and maps); the pirates' crew before Tao needs one more state before R_ARMED (a wave of regulars,
  then the boss). Phase 5: the entrances' art in the 588 slots. Phase 6: the hidden room (a trigger between the pillars,
  open once all six are out). Phase 8: rewards (flags 514-519 kept for first wins).

### 12.6 The five rooms and the entrances (phases 4 and 5, branch `nb-chamber-rooms`, 2026-11-05)

Every room is one row of `rooms[]` (`chamber.c`) and a map, on 12.4's framework. Each has its own plaque colour (8/37's), the
gate's beam across its door during the fight, its lever's light (flag 504 + n) and the same lever, cut-away, gift and door as the
dojo. Ratings rise by 25 a light out; HP below is at no light out, Normal. Every line is Claude's DRAFT (`chamber.c`, marked).

| room (area, light) | the map | alone | two players | special |
|---|---|---|---|---|
| Roshi's dojo (19, 0) | 6/70 | Roshi 350: 2800 | Roshi + Chiaotzu: 1960 + 1792 | as 12.4 |
| Pirate airship (20, 1) | Buu's Fury's deck 12/1, its pad planked over, a generated sky (its own sky layers do not import), entered by the gangway on the right | a crew of three Warlord's Henchmen (stat 12, 900 HP), then General Tao 360: 2920 | the crew with twins (six), then a lone Tao at 180 %: 5256 | from three lights out the crew are the stronger Henchmen (51 / sprite 143) |
| Frieza's ship (21, 2) | Buu's 8/19, the crystal room; Frieza and Cold come down the sides of the crystal | Frieza 360: 2920, Mecha Frieza at 40 % (his kit: the play body 238) | Frieza + King Cold: 2044 + 1876 | |
| Namek ship (22, 3) | LoG2's Capsule Ship 3 (6/58) with a green floor | Buu-saga Piccolo 380: 3160 | Piccolo + the Namekian Warrior: 2128 + 1960 | |
| The lab (23, 4) | Dr. Gero's computer room 8/63 | Android 18 360: 2920 | 18 + 17: 2044 + 2128 | |
| Saiyan ship (24, 5) | the Broly chapter's round room (Buu's 6/6), entered from below | Broly 390: 3280, his rage at 50 % (kit) | Broly + Paragus: 2212 + 1792; Paragus down first: Broly back to full and powered up, "Pa... ra... gus...? RRRAAAAAAGH!!" | |

**What the player sees.** At the far end of each room the boss stands (with two players his partner beside him; on the airship
Tao among three Henchmen). A few steps in, the players turn to them, the bosses walk to their marks, the boss's line for whoever
player 1 is (else player 2: Piccolo meeting himself, Vegeta and Trunks meeting Frieza, Krillin meeting 18, Goku meeting Broly
...), the partner's line, each player's reply, the door's beam and a shake, boss bars. On the airship the Henchmen come at the
players first while Tao watches; when the last falls Tao says his line, steps forward, and his bar comes up. Frieza alone turns
into Mecha Frieza at 40 %. With two players, felling Paragus before Broly brings Broly's rage. Then the dojo's ending: the fall,
the last line, the bodies blink out, the plaque, the cut-away with this room's light going out, the door open, the next tier.

**Lines (DRAFT).** Openings by player 1's character (else player 2's), the partner, the crew line, the last line:

| room | default | by character | partner / crew / rage | last line |
|---|---|---|---|---|
| airship | TAO: General Tao. Someone has paid well for your heads. / Men! Deal with them. | Goku: the brat who broke my Dodon Ray, rebuilt since; Hercule: killed for less; Krillin: the little monk, not grown an inch; 16: took the Red Ribbon's money too, scrap it | crew down: Useless, the lot of you! / Very well. I'll kill them myself. | Beaten... again... / Take your switch, and be gone. |
| Frieza | FRIEZA: A monkey's training room. How quaint. / Let me show you what real power looks like. | Gohan (and SSJ2): the little Saiyan from Namek; Piccolo: another Namekian; Vegeta: the proud little prince, killed once; Trunks: the boy with the sword, this time I do the cutting; Goku: even here you haunt me; Hercule: a clown?; Krillin: such a lovely explosion | KING COLD: Careful, my son. / Do leave a little for your father. | Impossible! Beaten by the likes of you? / Fine. Pull your little switch. |
| Namek | PICCOLO: So this is what Popo dreamed up. / Show me you've been training, or get out. | Piccolo (himself): ...Me? Heh. Popo has a sense of humour. / Let's see which of us is the real one.; Gohan: don't hold back because it's me; Vegeta: still second best; Goku: I always said I'd beat you; Hercule: do you even know where you are? | NAMEKIAN: For Namek! / I fight beside you, Piccolo. | Hmph. Not bad. / Go on. Throw the switch. |
| lab | ANDROID 18: So I'm a training dummy now. Fine. / Don't expect me to go easy. | Vegeta: I broke your arm last time; Trunks: the boy who hates us so much; Goku: Gero built us to kill you; Hercule: cute belts; Krillin: you again, the one with the remote; 16: whose side are you on now? | ANDROID 17: Relax, sis. / I'll take the other one. | Tch. Fine, you win. / The switch is over there. |
| Saiyan ship | BROLY: ...More of you. / I will crush you all. | Gohan: Kakarot's... son; Vegeta: Prince... Vegeta. You will kneel.; Goku: Ka... ka... rot... / KAKAROT!! | PARAGUS: Steady, Broly. / Show them what a true Saiyan is. Rage: Pa... ra... gus...? / RRRAAAAAAGH!! | Impossible... / I am... the strongest... |

**Without `roms/buu.gba` (risk 7).** A `BUU=0` build has neither Buu-saga Piccolo nor Broly nor the Buu interiors. Mapbuild's
`"unless": "buu"` builds `chamber_pirate_plain`, `chamber_frieza_plain`, `chamber_saiyan_plain` (Capsule Ship 3 in brown,
purple, grey) on the same areas, the open map's doors carry both arrivals (`requires` / `unless`), and the rows' `plain` twins
stand Tao with his crew, Frieza with King Cold, the Namekian alone (180 % with two players) and Paragus alone there. Tested:
`chamber_rooms_plain.txt` on a `BUU=0` build, all eleven cases clean (343 checks).

**Two fixes in the framework, found here.** (1) A dialogue trigger starts its next run at the index its last stop word names: the
lever's plain `stop` (index 3) made the second press of a plaque start at the throw without asking `CH_LEVER`, so pressing a
dojo's plaque twice before the fight threw it, and in any other room the cut-away put out light 0 (the room index was never
set). Both stop words of `ch_lever` are 0 now and the tests press every plaque twice before the fight. (2) In C, `"\x0bDon't"` is
the escape `\x0bD` (hex escapes take every hex digit): the second page lost its first letter whenever it began with A-F or a
digit (Roshi's "Come on" and "Go on" lines too). Every page break in `chamber.c` now ends its literal (`"\x0b" "Don't"`).

**The entrances (phase 5).** `tools/chamberart.py` draws all six by code (no ROM data), 96 x 80, left-right mirrored so the
right half reuses the left's tiles flipped, exact BG palette colours: Roshi's dojo front (tiled roof, red pillars, sign board,
curtained door), the pirate airship (red gas bag with a skull, planked hull with portholes, a gangway hatch), Frieza's saucer
(white and lilac, yellow lights, purple windows, a lowered ramp), the Namekian dome (white, green bands, round windows), Gero's
bunker (a rock mound, a steel door slid open under a chevron lintel), Paragus' round ship (`shipart.py`'s design, small). Slots
(unique tiles with flips): Namek 42, Frieza 43, Saiyan 52, pirate 56, lab 53, dojo 55; the open map is at **602 of 896** (372
with the placeholders), about 290 left. The doorways stay where the collision and the exits expect them (x 32-63, y 56-79 of
each picture). Previews: `chamberart.py OUT.png --x4 OUT_x4.png`.

**Tests** (`chamber_gen.py` `rooms()`, self-checking, in `regfull.sh`): `chamber_rooms.txt`, eleven cases, 356 checks: the
airship alone as Krillin (punches) and with Super Saiyan Goku + Hercule at three lights out (the stronger crew, six bodies);
Frieza alone as Super Saiyan Vegeta (Mecha at 35 %) and Trunks + Gohan with player 2 throwing the lever; Namek alone as Piccolo
(himself) and Hercule + Super Saiyan Vegeta on player 2's B; the lab alone as Hercule and Super Saiyan Goku + Krillin; the Saiyan
ship alone as Super Saiyan Goku (punches through the rage) and Gohan + Piccolo (Paragus first: the rage, full HP); the sixth
lever through the lab (Krillin + Vegeta, five lights out: the beams fall). Each: the door from the open area, the actors by
lineup, the plaque clicking twice, the intro, the seal holding, the HP by lights, the fall, the bodies gone, the cut-away with
this room's light, the flag, the door, the gift, the way back below the entrance, a return to an empty room.

**Full regression** (`regfull.sh build/t`, `PLAY=` a private play ROM, 2026-11-05, the branch's final build): 92 blocks, no FAIL,
TIMEOUT or CRASH (`chamber_gate` 94, `chamber_tiers` 36, `chamber_room` 166, `chamber_rooms` 356, `gohan_ssj1` 21 checks). On a
`BUU=0` build: `chamber_rooms_plain` 343 and `chamber_room` 166 checks, clean.

**Not verified.** Real-time feel and balance: headless numbers only, no fight played by hand (Broly and King Cold, 64 x 64,
in the two smaller rooms; Frieza's ship is a ring round the crystal, its corridors 56 px wide; the Henchmen's ranged fire on the
deck). In the headless runs the crew stood a while before closing in (the stock wander, then the chase near a player).
`WIDE=1` and `PARTY_SLOTS=8` builds compile; neither was run.

### 12.7 Mr. Popo's hidden room and the loop (phase 6, phase 8's loop; branch `nb-chamber-popo`, 2026-11-05)

Built: the hidden room behind the gate, its scene by lineup, Popo's fight, his reward, the loop (decision 4) and the rooms'
prizes. Code: `src/coop/popo.c` / `popo.h` (CoopCall 36, test command 49, RAM `.combo.po`), Popo's kit in `src/coop/tny_moves.c`
(`popo_kit`, `tny_moves_popo_spawn`), his stat through `tny_rating_stat` (`tny_engine.c`); script `data/story/chamber_popo.lscript`;
map `data/maps/chamber_popo.json` (15/25); body `tools/popoart.py` (sprite 163); tests `tools/headless/popo_gen.py` (writes
`chamber_popo.txt`, in `regfull.sh`, and `chamber_popo_bal.txt`). In `chamber.c` only a few lines: the tick hands 15/25 to
`popo_tick`, the rooms' rating adds the loop's step, `chamber_cut_light` for the prize; in `chamber.lscript` `ch_lever` pays the
room's prize. Every line is Claude's draft (DRAFT).

**What a player sees.** With all six lights out the way between the pillars is open, and walking up it is a door (an exit
inside the rect that stays blocked until then, `chamber_open.json`). A plain white room of 480 x 400, the gate's two pillars
again either side of the door at the bottom, a stone jar in each corner, far up a small figure with his back turned. A white
flash, the gate's hum low, a slow shake; the players walk in side by side to their marks, look left, right, at each other
(alone: up). The camera pans up to the figure, holds, comes back. Player 1's character speaks (a line per character, a
fallback), player 2's answers (the same). The shaking stops, the hum cuts: Popo turns and walks down halfway, the camera between
them; two lines in his third person ("Popo filled this chamber... Popo watched every fight", "Six lights are out. Popo is
pleased. But Popo wants to see one more thing..."); a flash and he stands behind them, they spin round, the door's beam
flashes on with a shake: "...how strong you are. With Popo's own eyes!" Boss bar, the fight. Later visits: the players walk in,
he turns round far up, one line, the fight.

**The fight.** He never walks up or swings (no stock AI: `KF_STAND`); he turns and throws: a fan of five blue ki blasts, a fast
orb, two orbs that follow you, and between them a hop away. A player who stays within 48 px of him for 40 frames sees him
vanish in a flash and reappear 96-176 px from both players, inside the room (`KF_FAR`, `MF_FAR`; at most every 150 frames).
One hit in three that would land misses and he hops instead (`KF_DODGE`, at most every 120 frames, also out of a blast he was
throwing; never out of a hop or the rage). A hit that lands shows his hit-stun for 10 frames and pushes him a little. At half
HP he rages: +8 to his attacks, +4 defence, specials a third more often (the kit's `pace2`), hops 100 frames apart. At 0 HP he
does not fall (`KF_YIELD`): his arms out, untouchable; 40 frames later a flash, his body is gone and he stands at the far end
facing them (an actor again), the camera on him: first win ever "Enough. Popo has seen enough. You are strong now. Popo has
something for you." and a Senzu bean with a Strength, a Power and an Endurance +3 capsule (flag 513); later wins "Again you
win. Popo is happy. Take this." and a Senzu bean; "Popo will stay here. Talk to Popo when you want more." The door opens and
he stays at the far end, facing them (flag 530: beaten this time round). Nothing is asked at the end of the fight: mashing A
through his lines must not put six lights of progress back (the coordinator's note; the first build asked there).

**Talking to him.** Either player walks up and presses A facing him (a "facing" dialogue trigger in front of his spot, as the
rooms' plaques; player 2 through his agency). A beat of 30 frames (an A held from the walk is no press: CoopCall 18 ignores
a held button), then "Fight Popo again? (A: Yes B: No)". Yes: "Good. Popo is ready." and the fight, as before (the door seals;
a player in the door's strip is moved above it). No: "Popo can light the six lights again. Everyone inside will be stronger.",
a beat, "Shall Popo start again? (A: Yes B: No)" on a page of its own; yes "Good. The lights are on again. Go. Come back
stronger.", no "Then Popo will wait here." After a yes, asking again gets "Go. Popo will be here when the six lights are out
again." Leaving and coming back while 530 is set: no scene, he stands facing them and waits to be asked. During a scene or the
fight the trigger does nothing. Once six lights are out again (530 clear) the next visit is the later visit's scene and the
fight.

**The loop (decision 4).** Yes clears flags 504-509 and 530 (the six lights back on: the next load of the open area has tier 0 and the
gate shut, the rooms' bosses back) and counts a time round (flags 520-521, 0-3; a fourth yes keeps 3). Every boss is
`CH_LOOP_STEP` (50) rating a round stronger: the rooms (`rating + 25 x lights out + 50 x rounds`, Roshi alone 3400 HP in round
1) and Popo (500 + 50 a round: 5200, 6100, 7000 HP; the curve ends at 600). A room's win pays at its lever, after the light
goes out (`ch_lever`, CoopCall 36 ops 19-20): its first win ever a +3 capsule (Strength, Power, Endurance by its light % 3; flags
514-519), any win once round a Senzu bean. Gohan's Super Saiyan stays at the first lever only (flag 468).

| thing | built |
|---|---|
| map | 15/25, 480 x 400, 33 tile slots; spawn (244, 340) facing up; the door (224, 376)-(264, 392) down to the open area's (1028, 420) below the gate; the seal (224, 360)-(264, 376) blocked with the gate's beam (decoration 0, 64 x 32 at (212, 352)) shown during the fight; jars decorations 1-4 at (24, 24), (424, 24), (24, 296), (424, 296), 14/2's stone pot, 3 hits, `ch_popo_jar`; his actor (sprite 163) at (244, 72) facing up; on-load `ch_popo_onload` (hands popo.c the three scenes), on-leave stops the hum |
| jars | `POPO_JAR`: item 13 (30 % food) while either player's HP is short, item 16 (30 % energy) while either's ki is; both full, nothing (the stock rule) |
| his numbers | stat 156 (`POPO_STAT`): `tny_rating_stat(500 + 50 x rounds)`: the boss class of stat 29, AI set 21 (never ticked), no kill EXP, 5200 HP, 40 / 42 / 42. Rule `BOSS_NO_TWIN` with `hp_pct`: two players get the lone boss's 180 % (9360 HP); the difficulty's boss HP on top |
| his kit | `KF_STAND \| KF_DODGE \| KF_FAR \| KF_YIELD`, range 230, pace 40 (80 frames), pace2 26: `VOLLEY(0x280, 5, 8, MF_KI, 240)`, `BALL(0x480, 1, 30, 0, 300)`, `HOMING(0x380, 2, 1, 0, 420)`, `TELE(0, MF_DODGE \| MF_FAR, 420)`, `XFORM(50, 0, 8, 4)`; fighter id `TK_POPO` = `TF_COUNT` (not a roster row: every roster lookup checks the id) |
| his body | sprite 163 (`tools/popoart.py`, `docs/sprites.md` "Mr. Popo"): Buu's Fury r.183 (idle, walk, arms out) in Hercule's 21-group layout, hit-stun, knock-down and lying drawn by code; with `BUU=0` the same from LoG2's sprite 93 (his idle for the arms-out pose): he fights in every build |
| states | popo.c's header: P_WAIT (20 frames after the load's fade) -> P_SCENE -> P_ARMED (`POPO_FIGHT` sealed the door) -> P_FIGHT -> P_YIELD (40 frames) -> P_OUTRO -> P_DONE (`POPO_OPEN`; he stands; `ch_popo_talk` on A: `POPO_FIGHT` -> P_ARMED, or the loop). A load with flag 530 set goes straight to P_DONE. A scene that never seals gives up after 90 frames with the door open; a fighter that vanishes opens the door: the room never traps anyone |
| camera | the scenes pan with `PanCameraToPosition` (view top-left) and end with `SetActiveEntity 0`: without it the camera stayed where the last pan left it for the whole fight (found in the first films) |

**Balance (headless, `chamber_popo_bal.txt`, the Map Test party at level about 40, Normal).** The test autopilot (test command 49
op 2; popo.c `bot_pad`) walks up beside him and punches: no dodging, no abilities, a lower bound on two people. Over 7372 frames
(about 2 minutes), on the build merged with the five rooms (the numbers move a little with every build: the dice):

| lineup | his HP | the end |
|---|---|---|
| Piccolo + Hercule, round 0 | 9360 -> 3297 (65 % dealt) | both standing (277 / 1055, 370 / 726); 54 hops, 16 dodges |
| Piccolo alone, round 0 | 5200 -> 901 (83 %) | Piccolo down at about 73 s |
| Goku alone, round 0 | 5200 -> 2123 (59 %) | Goku down at about 113 s |
| Piccolo + Hercule, round 2 | 12600 -> 6068 (52 %) | both standing (368, 309); 53 hops, 15 dodges |

The first kit (blasts at about a third of these, a hop as soon as a player came within 56 px, anywhere in the room) was not a
fight: the autopilot landed almost nothing (the hops) and Popo took a few per cent of its HP in two minutes. Tuned to the
numbers above: a window of 40 frames before he hops, hops of 96-176 px, the blasts' multipliers raised. Alone the autopilot
nearly wins, so a player who dodges and uses his abilities should; with two the lone-boss HP makes it about twice as long.
Ryan's play decides.

**Tests** (`chamber_popo.txt`, 159 checks, on both a `BUU=1` and a `BUU=0` build): five lights shut the way, six open it into
the room; the actor, the door's beam hidden; one player (Piccolo): the scene's routes, the fight, 5200 HP, flag 512, the seal
(walking down stops above the door), a hop to 96 px or more, twelve punches and the autopilot (HP down, dodges), the win, the
arms out, the end scene with A on both pads to its end (no question, the lights untouched, flags 513 and 530, the door open,
he stands), the first reward (a Senzu, three capsules); talking to him: nothing armed 20 frames after the press, armed by 70;
no fight, no start again (lights stay out); out and back: no scene, he stands (state 7); asked: a fight again (5200 HP), won,
the later reward (a Senzu only); asked: no fight, start again yes (lights back on, round 1, 530 clear); asked once more: his
line only; out, the gate shut; six lights out again: the later visit's scene (one line) and the fight at 6100 HP; two players
(Goku + Hercule): 9360 HP, the autopilot's win from 5 %, A through the end (lights untouched), player 2 walks up and asks
(armed, no and no); five more
lineups (Hercule + Vegeta, Krillin alone, Gohan + Piccolo, Trunks + Krillin, Vegeta + Goku): the scene's routes and the fight's
start; round 2: 7000 HP; a room's prize in round 1: Roshi at 3400 HP, his first win's Strength +3 and flag 514; again with 514
set: a Senzu; the big jars: both players full nothing; player 1 full and player 2 hurt and out of ki, both items drop and he
walks over them (his HP 100 -> 3071 of 9999, his ki 10 -> 130 of 400). `chamber_gate`'s check of
the open way walks 50 frames instead of 90 (90 now reaches the door) and clears the save pad's surroundings first (the new
timing put a hawk on the player and the menu stayed shut).

**Full regression** (`regfull.sh build/t`, merged with the five rooms, the question moved to the talk, 2026-11-05): 93 blocks, no FAIL, TIMEOUT or CRASH;
`chamber_popo` and `chamber_rooms_plain` clean on a `BUU=0` build; a `WIDE=1` build compiles.

**Not verified, open.** Real-time feel (the hops, the dodge, the blasts' danger; the balance above is an autopilot's).
Popo's line naming whoever threw the last lever (section 5, step 4) is not built (nobody records the thrower). A player who
talks to Popo and then mashes A answers both questions yes after their beats (the fight again first: the loop needs a B). Music stays the chamber's (39). The wide view was not run. The trophy hall and the panel (phase 8's other parts)
are not built.

---

## 13. Ryan's first play (2026-11-05, branch `nb-chamber2`)

Ryan played the chamber alone as Goku: "i love the open area", Broly's ship and its fight enjoyed; then the notes this section
answers. His words, shortened: the exit is not obvious and he could leave mid-run by accident; the plaque should start ON and be
switched OFF; after a switch it "animated like i found a capsule" with no pop-up, and once "now gohan can turn super sayain" read
as his own reward though he played Goku; Broly's ship has doorways that lead nowhere, Frieza's ship a back room that could be a
throne or control room; he could not find the other places, so a red carpet from the centre to each; no save point inside; a
question before leaving ("are you sure you want to leave? youll lose all progress"); an opening scene that shows the ships, the
rooms and the switches. Then, through the coordinator: he "couldnt find the gate or where to go after i beat 18" (the lever's
cut-away now travels the way), the gate "just looks like a gate sitting in the middle of the room" (a wall and a portal), and
after Popo "it didnt tell em what they were" (every reward named) and "all the enemies ... should be gone. Training is
completed. and we should have a clear path to leave".

### 13.1 What the player sees now

**Coming in.** The Lookout's door, the white fade, the players at the quarters. A new run (flag 510 clear) opens with a scene:
the players look round; Mr. Popo's voice (portrait 82, never seen here): "The chamber is ready. Six rooms stand in this white.
Popo will show you." The camera pans to each entrance in turn, Popo naming it (Master Roshi's dojo; the pirates' airship,
General Tao and his men; Frieza's ship; the Namekian ship; the Saiyan ship; Dr. Gero's lab), then "In every room a fighter
waits, and on the wall beside him, a lever. Lit." Six short views follow, a map each with the load's own fade: inside every
room its boss standing at the far end (with his partner when two play: Chiaotzu, King Cold, the Namekian, 17, Paragus; Tao
alone), the camera tilting from the lit plaque to him. Back on the open area (on the lab's entrance, where the last view was) the
camera travels to the quarters and climbs the red carpet behind them to the gate in its wall, its six red lights and beams over the dark portal: "Each lever puts out one light on Popo's
gate. Six lights out, and the gate opens." / "Defeat the six. Switch off their six levers. Then walk into the gate. Follow the
red carpet." A white cut back to the players. Start or B (either pad) skips it: the boxes close by themselves and it ends
(through the way back from a room's view if it was in one). About 40 s read at a normal pace; a run under way never shows it
again (out to a room and back, a lever's cut-away); leaving starts a new run.

**Finding the way.** A red carpet with gold trim, 32 px wide, joins the quarters' door to every entrance's door and runs up the
middle to the gate (the sketch below). The gate's pillars stand in a white stone wall across the whole map: nothing walks round
the gate, and the north of the map is shut. Between the pillars a portal: a dark violet swirl in a stone ring while the beams
are up; with all six lights out the swirl is lit and turns (two frames in turn). Walking into it: a flash and the portal's sound
(audio 26, Popo's own flash), a white fade, Mr. Popo's room, which now has the same portal between its pillars (the way back)
and a violet tint in its white.

```
              [ wall ======== | gate + portal | ======== wall ]
                                     |
      Namek ===================== H0 ======================= Frieza
        |                            |                          |
        |                    .---- behind ----.                 |
        |                    |   [quarters]   |                 |
  Saiyan|                    |   door, mat    |                 |pirate
    |   |                    |                |                 |  |
    ======================== H1 (the arrival) ======================
                                     |
       lab ======================== H3 ======================== dojo
```

**The door home** is a dark doorway in a gold frame on the canopy's front with a green EXIT sign beside it. Walking up into it
(either player) asks, in a box of its own after a beat: "Are you sure you want to leave? / You'll lose all progress. / (A: Yes
B: No)" (with no light out yet: "Leave the Time Chamber? (A: Yes B: No)"). B: a step back, still inside; walking up again asks
again. An A held when the question comes does not answer it (CoopCall 18). Yes: the run's progress resets (flags 504-509 the
lights, 510 the run, 530 Popo beaten this time round: the six lights on, tier 0, every room's boss back), then the Lookout.
First-win rewards (514-519), the finds (531-533) and Popo's rounds (520-521) stay earned. **No save pad** inside: the quarters'
pad is gone (and with it the character switch; the lineup is chosen before going in). A game over loses the run too: the last
save was made outside.

**A lever.** The plaque is ON (lit, its decoration shown) from the start; pressing it before the fight clicks; after the win the
press switches it OFF (the decoration hides, the stock switch sound). The cut-away then opens on this room's entrance on the open
map and pans along the carpet, leg by leg, to the gate (chamber.c `way`, 8 px a frame; 2-6 s by room), where the room's light
goes out. The sixth also drops the three beams one by one, cuts the hum, lights the portal (a flash, its sound) and holds on it
with Popo's line: "Six lights are out. The gate is open. / Follow the red carpet north, past the quarters, into the light." Back
in the room the door's beam goes, then, 30 frames later and only when owed, the prize: the stock pick-up pose and a box of its
own in the stock wording ("You received a +3 Strength Capsule!", Power, Endurance, or "You received a Senzu Bean!" once round).
Then, once, before the Cell Games and only while Gohan is in the party: "Gohan's training has paid off! / Gohan can now become a
Super Saiyan!" in a box of its own (decision 6's gift now behind that test: a lever thrown without Gohan in the party leaves it
for a later one).

**The side rooms** (Buu's Fury's art; explore only, open once the room's lever is thrown, so a fight cannot be fled through them):
the Saiyan ship's left doorway leads to its bedroom (Buu's 6/8: the bed, two lamps; the round room's own neighbours in Buu's
Fury), the right to its store room (Buu's 6/7: the cold box); Frieza's top doorway to his throne room (Buu's 8/27 inside Babidi's
ship: four pillars, the round pad, the dome at the back as Frieza's hover pod; a doorway cut into its bottom wall from 8/19's
corridor). Each has two stone jars (the chamber's drop) and one find, taken once ever: beside the bed (A anywhere there) "A bed,
neatly made. Something is under the pillow..." and a +3 Endurance Capsule; facing the cold box "The cold box is stocked with
rations... / and something green at the back." and a Senzu Bean; facing the pod "Frieza's hover pod. Something is wedged in the
seat..." and a +3 Power Capsule; later only the look ("Nothing else here.", "Rations, and nothing else.", "It hums quietly.").
A `BUU=0` build has neither the art nor the doorways (the fallback Capsule Ship rooms have none).

**After Popo.** Each reward he gives has its own box: the first win "You received a Senzu Bean!" and each +3 capsule's, a later
win the Senzu's. Two lines more from him ("Popo will stay here. Talk to Popo when you want more." / "Outside, the chamber is quiet
now. The training is complete. The way home is open."). Back out through the portal (flag 530 set): no enemy stands or comes (the
spot records' condition `noflag:530`, the refill off), a box "Training complete!", the camera runs down the carpet from the gate
to the quarters' door, Popo: "Popo's training is complete. The chamber is quiet. / The door home is open. Follow the red
carpet." Leaving then asks nothing: the run is over (the lights reset as above, the round kept). Talking to Popo for the loop
(start again, harder) works as before: the lights on, 530 clear, the enemies back at the next load.

### 13.2 The numbers

| thing | value |
|---|---|
| maps | 15/18 602 of 896 slots (the carpet, the wall, the portal's ring, the door and the sign in; the save pad out); 15/25 70; new 15/27 the bedroom 240 x 160, 119 slots; 15/28 the store room 240 x 160, 52; 15/29 the throne room 400 x 400, 376 (the three `"requires": "buu"`) |
| open map decorations | 0-8 the gate, 9-24 the jars, **25 / 26 the portal** (32 x 64, one 2 KB frame drawn by `tools/chamberpath.py`, 26 the same frame turned half round by OAM's flips): the sheet holds 5248 bytes of OBJ VRAM (3200 before) |
| collision | the wall and the whole north: rows 0-49 blocked (the gate's way between the pillars opens with all six as before); the sign's posts |
| flags | 510 a run under way (its opening has played; cleared by leaving); 531-533 the side rooms' finds (once ever). Both cleared by a new game (504-535) |
| RAM | `.combo.ch` 0x34 (was 0x2C of 0x30): +0x27 the last prize, +0x2C the open area's scenes, +0x30 the scene (kind 1 opening, 2 the door, 3 the training's end, 4 the portal; 0x40 running, 0x80 skip), +0x31 bits (1 a player at the door, 2 the training's end seen, 4 the portal lit, 8 its phase), +0x32 the portal's frames, +0x33 the scene's wait. `.combo.po` moved to 0x0203EBBC (tests find both by magic) |
| timings | the opening's pans 12 px a frame, each room's view 72 frames after its load; the lever's way 8 px a frame and 30 frames at each end; the door's question 32 frames after the step in, the portal's warp 2 |

**Draft lines** (all Claude's, marked DRAFT in `data/story/chamber.lscript` / `chamber_popo.lscript`): the opening's nine boxes,
the two questions, "Training complete!" and Popo's line after it, the sixth lever's line, the four prize boxes (stock wording),
Gohan's box, the three finds and their looks, Popo's two new lines after his fight.

### 13.3 How it is built

- **chamber.c**: the opening, the door's question, the training's end and the portal's warp are one sequence (`ch_open_scenes`)
  started like Popo's scenes (`coop_run_sequence` through `coop2.call_action`) once player 1 is free; its entry 0 asks C where
  (CoopCall 35 op 24). The open area's on-load hands it over (op 23) and queues the opening (flag 510 clear) or the training's
  end (530 set, not yet seen this visit). The skip: `chamber_input` (input.c, after the yes / no hook) sees Start or B on either
  pad and from then on presses A every other frame so boxes close; every entry's end and every 24 frames of a pan ask op 32 / 27
  and jump to the end (or to the way back from a room's view). The rooms' views: `LoadMapNoEntities` to each room in turn under
  one `SetMapRestoreFlag` (no player is built: `map` shows -1, -1), its actors made by `MapScript_CreateCharacter` on the map's
  heap (op 28), its on-load lighting the plaque, then `RestoreMap`. The door's watch runs in `open_tick` (either player in the
  door's strip holding Up); the step back is a command-batch walk (op 36). The portal's trigger only queues its scene (op 53): a
  warp inside an "enter" trigger's own fire crashed (the box is taken off a list the load had reset).
- **The lever** (`ch_lever`, 22 entries now): the throw (3), the legs of the way (4-10, one entry each: a script's forward jumps
  reach 255 bytes), the gate (11), the sixth's line (12), back in the room (13), the prize boxes (14-17), Gohan (18-20).
- **Rows** (`ChRoom`): `stand` (the actors' spots for the opening), `shot` (its two views; alone, the boss's view centres on him),
  `ways` (the side doorways' rects, blocked by the map, opened by the on-load once thrown and at the lever).
- **mapbuild**: an `objects` part may be `{"draw": NAME, "arg": ARG}` (tools/NAME.py's `obj(ARG)`, converted to 8bpp in the OBJ
  palette), a `"32x64"` shape, a list entry's `"flip"` (`"h"`, `"v"`, `"hv"`: OAM attribute 1's flips).
- **Art**: `tools/chamberpath.py` (the carpet's runs `SEGMENTS`, its trim by depth inside their union; the wall; the portal's ring
  sealed and lit; the door; the EXIT sign; the portal's swirl as a decoration frame); previews `python tools/chamberpath.py OUT.png`.

### 13.4 Tests

`chamber_gen.py` writes `chamber_run.txt` (new, in `regfull.sh`): the opening in one player (Goku) and two (Gohan + Piccolo): it
starts, flag 510, the void's bodies gone, each room's view with its boss (and partner by lineup) and the plaque lit, the end with
the camera back on player 1 (player 2 beside him), the void filling again; skipped by Start in the first pan and by player 2's B
inside Frieza's view, no opening on a reload during the run; the training's end out of Popo's room (two players): its scene, no
enemies after 400 frames, out by the door at once with the lights and 530 reset and the round kept. `chamber_gate`: the save pad
gone; the question (lights out, the lose wording), B steps back with the lights kept, an A held as it comes does not answer, A
leaves with the run reset and the first win and the round kept; the plain question without lights; the wall stops a player at x
600; the portal closed with five lights, its two frames in turn with six. `chamber_room` / `chamber_rooms`: the plaque lit before,
clicked twice, OFF after; the cut-away opening on the room's entrance (the camera's view), arriving at the gate, the light out;
the sixth: the beams, the portal lit and the line; the prize's item (ChState +0x27) with its box up; the side rooms shut before
the lever, then each entered, its find (the item, its flag), the look, out again. `gohan_ssj1` and `chamber_popo` (the rewards'
boxes stepped through; out of his room after a win, the training's end) adjusted; `chamber_tiers` unchanged (the lights set flag
510).

**Full regression** (`regfull.sh build/t`, `PLAY=` a private play ROM, the branch's final build, 2026-11-06): 95 blocks, no FAIL,
TIMEOUT or CRASH (`chamber_run` 77, `chamber_gate` 106, `chamber_tiers` 36, `chamber_room` 181, `chamber_rooms` 417, `chamber_popo`
159, `gohan_ssj1` 21 checks). On a `BUU=0` build: `chamber_run_plain` 73, `chamber_rooms_plain` 376, `chamber_room` 181,
`chamber_popo` 159, clean.

**Test recipes changed**: `door()` sets flag 510 first (test command 48 op 7) unless the opening is wanted; op 0 (the lights) sets
it too. Read the scene state from RAM (`find CH`, `peek CH+0x30`): test commands wait while a scene runs. The lever's cut-away
takes 300-600 frames: wait for the light (`stepz D+0x10 10`), and `mashmap` back to the room through the sixth's line. The
portal: player 1 at (1028, 346), Up; the hidden room's scene starts after the portal's own fade (60 frames).

### 13.5 Not verified, open

- Real time, by Ryan: the opening's length (about 40 s read; skippable), the pans' speed (12 px a frame on the tour, 8 on the
  lever's way), whether the six rooms' views are wanted every run or only the first ever (flag 510 makes it once a run; a
  first-ever flag would be one line).
- The carpet's look over the whole map, the portal's two-frame turn and the wall at the top are seen in headless shots only.
- The rooms' views stand only the lead and the partner (Tao without his crew).
- A player 2 alone walking into the portal does nothing (player 1's position fires it, as the stock doors).

### 13.6 Ryan's second round (2026-11-06)

He played 13 (merged into `main` at `5cb4ec0`): "f5- everything is perfect. i would just add another message on the leaving the time
chamber question saying 'are you sure? you will lose all progress' -- i want progress to reset if they leave mid-run. they have to
conquer all 6 bosses AND Popo in one run." "f7- ... theres also supposed to be a room up north, theres a door for it. the side rooms
are good though." "f9- friezas is cool too. it would honestly make more sense if you used the new room as the initial fighting room
and the room we already have as the 2nd room. BUT, we still need another room north of that that has no doors and matches the ships
vibe. a control room would be cool".

- **The door home, mid-run** (a light out, Popo not beaten): two questions, each a box of its own after its beat: "Leave the Time
  Chamber? (A: Yes B: No)", and on yes his words, "Are you sure? / You will lose all progress. / (A: Yes B: No)". B (or no) on
  either: a step back, still inside, nothing lost. An A held from the first does not answer the second (CoopCall 18 arms it anew).
  Both yes: the run resets as in 13.1 (the six lights, 510, 530; first wins, finds and Popo's rounds kept), the Lookout. With
  nothing done (no light out): the first question only. After Popo (the training complete): none. The whole six and Popo are one
  run. (chamber.c `CH_EXIT_ROUTE`: a = 0 the first route, a = 1 after a yes; `B_ASKED` 0x10 in ChState +0x31.)
- **Broly's ship, north**: the round room's top doorway (blocked by the map, opened by the lever like the side ones: `ways` now
  holds three) leads up into a lounge (15/31, 240 x 176, 55 slots): composed from the round room's own Buu's Fury neighbours, 6/7's
  walls and floor without its cold box, its left opening shut by its right rim mirrored, the round room's bottom doorway stamped
  into its bottom rim (the way down), the round room's table in the middle. Two stone jars; facing the table "Bowls and cups on the
  table, and a capsule among them..." and a +3 Strength Capsule, once (flag 535); later "The table is bare now."
- **Frieza's ship, three rooms**: the door from the open area now enters **his throne room** (15/21, Buu's 8/27: the pillars, the
  round pad, the dome at the back as his hover pod; its bottom wall cut for 8/19's corridor; 431 slots), where the fight is: the
  plaque on the back wall left of the pod (purple, lit), Frieza (and King Cold with two players) standing either side in front of the
  pod and walking down onto the pad, the door's beam across the corridor, Mecha Frieza at 40 % alone, the lever and its cut-away as
  before. The doorway right of the pod (8/19's top doorway stamped into the back wall) opens with the lever: **the crystal room**
  (15/29, Buu's 8/19 whole, the old fight room; its right opening shut; 420 slots): two jars; facing the crystal's dais "The crystal
  hums. Something glints at its foot..." and a +3 Power Capsule (flag 533); later "The crystal hums quietly.". Its top doorway leads
  to **the control room** (15/30, Buu's 8/34 inside Babidi's ship with 8/20's right wall over its side opening and 8/20's console in
  the middle: no other door, the ship's purple walls and blue floor; 146 slots): two jars; facing the console "The ship's controls. A
  pouch is wedged under the panel..." and a Senzu Bean (flag 534); later "The ship's controls blink. Nothing else here."
- **Without `roms/buu.gba`** (`BUU=0`): Frieza's room is still `chamber_frieza_plain.json` on area 21 (Capsule Ship 3 in purple,
  one room, no doorway) and the Saiyan ship's `_plain` has none either; areas 27-31 are not built. The rooms' `plain` rows are
  unchanged.
- **Numbers**: areas 29 (the crystal room, `chamber_frieza_crystal.json`, in place of the throne room's file), 30 (the control
  room, `chamber_frieza_control.json`), 31 (the lounge, `chamber_saiyan_lounge.json`); finds 531-535 by area 27-31 (the bed, the
  cold box, the crystal, the console, the table); `CH_FINDS` 5; `ch_find` 22 entries. Frieza's row: marks (176, 200) / (232, 200),
  seal (184, 312)-(216, 320), line 300, stands (168, 132) / (248, 132), the opening's views (24, 8) and (88, 72) (alone centred on him),
  the way on (248, 56)-(280, 104). The opening's view of his ship is now the throne room.
- **Tests**: `chamber_gate` asks twice mid-run (B on the first, a held A, the second with its bit, B on it, then both yes and out)
  and once with nothing done (B, then A out); `chamber_rooms` enters the new Frieza room, checks the lounge's doorway and the way on
  shut before the lever, then each side room's find and look, the control room through the crystal room (`side_room`'s `next`).
- **Full regression** (`regfull.sh build/t`, the branch's final build, 2026-11-06): 95 blocks, no FAIL, TIMEOUT or CRASH
  (`chamber_gate` 118 checks, `chamber_rooms` 443, `chamber_run` 77). On a `BUU=0` build: `chamber_rooms_plain` 376, `chamber_run_plain` 73.
