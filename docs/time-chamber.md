# The Hyperbolic Time Chamber as a game mode (plan, 2026-11-05, nothing built)

**Status: a plan to discuss.** No game data changed, no flag, stat, sprite, map or CoopCall taken. Ryan reads part 1
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

## 10. Decisions for Ryan

1. **The gate:** copy the Lab's gate as it is, white pillars with six red lights and three beams, one light per room?
2. **The six rooms:** Roshi, the pirates and Tao, the Frieza family, Piccolo, plus which two? (Mine: the Lab with 17 and 18, the
   Saiyan ship with Broly. Others: Cooler's ship; Gero and 19.)
3. **Order:** rooms in any order with bosses growing by lights out (mine), or a fixed order with stock level-gate numbers?
4. **After Popo:** the lights stay out for good, or he offers to start again harder (mine)?
5. **The ten rounds:** gone (mine), or kept as a choice on the panel?
6. **Gohan's Super Saiyan:** at the first lever (mine), the gate, or Popo?
7. **The enemies:** do you mean the Warlord's Henchmen by "grenadiers"? Are the tiers in section 4 what you pictured, with
   first-tier bears and snakes strong enough for that point of the story, or do you want the truly weak ones?
8. **Pirates:** Warlord's Henchmen as they are (mine, to start), recoloured, or Buu's pirate robot as a mini boss too?
9. **Popo:** how hard (top of the game, or above it?), and what is the big reward?
10. **Big area:** one map at about 16 enemies at once with a refill as you walk (mine), or several linked maps?
11. **First build:** the panel and the trophy hall now, or after the rest has been played (mine)?

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
