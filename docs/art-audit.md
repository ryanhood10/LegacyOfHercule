# Art audit (2026-11-06, branch `nb-art2`)

Ryan: "Another part of this plan could be improving our sprites and animations especially on the characters that we
created. we can also borrow sprites and stuff from legacy of goku buus fury too or use either one as references when we
create unique ones. even for simple things like items and maps ... its best to recycle first but we will have to
generate some of our own stuff too. production is important."

Every sprite record and portrait we made or converted, looked at group by group next to the stock game's own bodies at
the same scale, with the cutscene work in mind (`docs/cutscenes.md`: scenes become clips, so a body that cannot strike,
fall, fly or power up limits what a scene can show). Section 1 is the table scene writers need; 2 the audit; 3 what
this pass fixed; 4 what it left and why; 5 portraits; 6 items and map art; 7 how to redo it.

Art is generated at build time from the two ROMs; nothing from a ROM is committed. Sheets are build output
(`build/<dir>/audit/`), never in the repository.

## 1. Which body has which cutscene pose (groups; a missing group is a crash)

**The rule first** (`docs/cutscenes.md` 5 "Traps"): an animation whose group the record lacks reads a null descriptor,
overwrites the heap and crashes later. A group number at or past the record's count reads whatever follows the record.
The low animation numbers play the group of the same number: known from the code for 0 idle, 1 walk, 5 ki pose, 13
dash, 14 fidget (`rom.h` `ANIM_*`, `docs/hercule.md`) and from the stock scripts' use for 2 hit-stun, 3 knock-down, 4
shout, 6-8 attacks; **not decompiled for 9 and 11** (the table is `0x083B5268 + 0x10`, one control state a number:
check a number there before a scene relies on it). `FlyToPosition` plays **group 12** (`docs/engine.md`); animation 30
(`NPCFireKamehameha`, `CoopCall 16`) plays **group 18**. A held pose shows its group's frame 0.

Columns: the group to play, or `-` (do not play it: the group is missing). "lying" is a body on the ground; where a
body has no lying group the knock-down (3) ends lying and can be played through, not held.

| body (sprite) | attack | hurt | knock-down / lying | kneel | fly (12) | charge / power-up | beam (18) | shout / victory | do not play |
|---|---|---|---|---|---|---|---|---|---|
| Hercule 72 | 6, 7, 8 | 2, 11 | 3 (ends lying) | 3's frame 1 (not holdable) | **12, all four facings** (sides new) | 9 (arms up, then the gold aura: his Super Saiyan power-up), 5 the wave | 18 | 4 shout, 5 wave | 10, 14-16, 19 |
| Super Saiyan Hercule 157 | 6, 7, 8 | 2, 11 | 3 | - | **12** (sides new) | 5; 21 the turn back | **18 (new)** | 4 | 9, 10, 14-16, 19 |
| old Hercule 158 | 6, 7, 8 | 2, 11 | 3; 12 is a lying pose in every facing | - | - (12 lies) | 5 | - | 4 | 9, 10, 14-16, 18, 19 |
| Android 13 165 (count 13) | 6, 7, 8 | 2 | 3 (ends lying); 12's side views | - | 12 | 5 | - | - | **4, 9, 10, 11, anything from 13 up** |
| Android 13, player body 240 | 6, 7, 8 | 2, 11 | 3, 12 | - | 12 | 5 | - | 4 (= his charge) | 9, 10, 14-16, 18, 19 |
| Super Android 13 180 | 6, 7, 8; 21 the ki throw, 22 the bomber | 2, 11 | 3 (recoil only: he never lies) | **23** (crouching power-up) | **- (12 is his death light)** | 23, 24 the roar, 25 the nova | - (21) | 4, 24 | 9, 10, 14-16, 18, 19; 12 outside his death |
| Krillin 161 (player) | 6, 7, 8, 16 | 2, 11 | 3, 12's side views | - | 12 | 5 | 18 | 4, 14 fidget | 9, 10, 15, 19 |
| Krillin 76 (actor, count 13) | 6 | 2 | 3, 12 | - | 12 | 5 | - | 4 | **7-11, anything from 13 up** |
| Android 16 175 (player) | 6, 7, 8 | 2, 11 | 3, 12 | - | 12 | 5 (the rocket-punch pose) | - | 4 (the look round) | 9, 10, 14-16, 18, 19 |
| Android 16 160 (boss, count 14) | 6, 7, 8, 13 the rocket punch | 2 | 3, 12 | - | 12 | 5 | - | - | 4, 9-11 |
| Android 16 8 (actor, count 13) | 6 | 2, 3 | 12 | - | 12 | **-** | - | 4 (the look round) | **5, 7-11, anything from 13 up** |
| Paragus 170 | 6, 7, 8, 16 | 2, 11 | 3, 12 | - | 12 | 5 | 18 | 4 | 9 (Vegeta's transformation: gold hair), 10, 15, 19 |
| the Namekian 166 | 6, 7, 8, 16 | 2, 11 | 3, 12 | - | 12 | 5 | 18, 19 | 4 | 9, 10, 14, 15; 17 and 21 (Piccolo's turban and cape) |
| Mr. Popo 163 | 6, 7, 8 (palm strikes from the side) | 2, 11 | 3, 12 (lying) | - | **- (12 lies; walk him, or a flash)** | 5 (arms out) | - | 4 (arms out) | 9, 10, 14-16, 18, 19 |
| Piccolo 103 | 6, 7, 8, 16 | 2, 11 | 3, 12 | - | 12 | 5, 9 | 18, 19 | 4 | 10, 15, 21 |
| Piccolo 73 (NPC) | - | - | - | - | - | - | - | - | **everything but 0** (use 103) |
| Vegeta 137, Trunks 128 | 6, 7, 8, 16 | 2, 11 | 3, 12 | - | 12 | 5, 9 | 18 | 4 | 10, 15, 19 |
| Android 17 9, Dr. Gero 14 (count 13) | 6, 7, 8 | 2 | 3, 12 | - | 12 | 5 | - | - | 4, 9-11, anything from 13 up |
| Android 18 11 (count 13) | 6, 7, 8 | 2 | 3, 12 | - | 12 | 5 | - | 4 | 9-11, anything from 13 up |
| Dr. Gero 15 (NPC, count 13) | - | - | 12 | - | 12 | - | - | - | everything but 0, 1, 12 (use 14) |
| Goku 60 / haloed 176 | 6, 7, 8, 16 | 2, 11 | 3, 12 | - | 12 | 5, 9 | 18, 19 | 4 | 10, 15 |

Nobody but Super 13 has a kneeling group. Where a scene needs a kneel, a C helper can hold frame 1 or 2 of the
knock-down (the half-fallen crouch) the way the tournament holds its poses (`tny_engine.c` `poses_apply`: the group
pointer, frame and timer written every frame); a script alone cannot.

## 2. The audit

Priority: A a cutscene gap or a fault a player sees every time; B a fault in some frames; C cosmetic. "fixed" = this
pass (section 3).

### Main actors

| sprite | used for | what is weak | fix | size | pri |
|---|---|---|---|---|---|
| Hercule 72 (`herculesprites.py`) | player, actor | the flight's left / right arrays were the stock front view: flying sideways he slid along as a face looking out of the screen | his lying frame raised, his side head, the stock glints | 4 frames | A, **fixed** |
| Super Saiyan Hercule 157 (`spriteclone.py`) | player | no group 18 (a transformed Hercule in a beam scene: the missing-group crash); the same flight; the gold afro is one flat colour with two highlights (the stock black afro is flat too) | take 18 and the flight from the base form's builders, gold | 32 frames | A, **fixed**; the flat afro C, left |
| old Hercule 158 (Buu's Fury r.128) | player, actor (prologue, epilogue) | a clean import. No flight (his 12 lies), no beam | none needed for his scenes | - | C |
| Android 13 165 / 240 (`bossmodels.py`) | boss, actor, tournament body | the cap was 16's mohawk cut 3.6 px deep in the dull end of its ramp: a ragged brown tray narrower than the head, gone on tilted and lying heads; brown specks round the head in the kicks; in back-view kicks the head was a white box beside the body. No shout (4), no hurt (11): count 13 | draw the cap; clean the head swap (below, 16) | 82 frames | A, **fixed**. Groups 4 / 11: B, left (section 4) |
| Super Android 13 180 (`super13sprites.py`, Buu's Fury Broly) | boss, tournament body | the best of our bodies. Broly's red stayed on the sash's knot in every side view and a gold cuff was painted as trousers on the walking arm (43 frames). No flight and no lying pose: 12 is his death light | two rules in the recolour | 43 frames | B, **fixed**. Flight / lying: A for scenes, left (section 4) |
| Android 16 160 / 175 (`a16sprites.py`) | boss, player | the four groups from 17 (fall, charge, two kicks) were a step duller than his own frames, so the punch was lime and the kicks olive; olive specks of 17's hair strands round the head; the head floated beside the body in the back-view spin kick | brighter trouser map, strands and crumbs removed, the head on 17's scarf | 51 frames | A, **fixed**. His body in those groups is still 17's slimmer build: C, would need drawing |
| Android 16 8 (stock actor) | actor (the island, the computer chapter) | no charge 5, no second attack: `SetEntityAnimation 8, 5` is the crash | repoint 8 to a record that adds 160's groups 5, 7, 8 (as 72 and 58 are repointed) | 20 lines | A for scene writers, **left**: section 4 |
| Krillin 161 (`krillinsprites.py`) | player | sound. Goku's frames re-headed; checked in the 2026-10-25 pass | - | - | - |
| Krillin 76 (stock actor) | actor, tournament opponent | groups 7, 8, 11 empty inside the count | as 8, from 161's; it is also an enemy body (AI set 98), so test the tournament Krillin | 20 lines | B, left |
| Paragus 170 (`brolysprites.py`) | boss, actor | sound: Vegeta's full set, grey hair and goatee on every frame. His 9 is Vegeta's Super Saiyan transformation (gold hair): never play it | point 9 at his charge, or empty it | small | C |
| the Namekian 166 (`bossmodels.py`) | boss, actor | sound. Groups 17 (halo) and 21 are Piccolo's frames in the turban and cape: a white-caped Piccolo if played | empty them or reuse the idle | small | C |
| Mr. Popo 163 (`popoart.py`) | boss, actor | Buu's Fury has three poses of him. From the side his three attacks showed him standing; the hit-stun from the front is a two-row flinch; no flight (12 lies); the charge and the shout are the arms-out pose again | side palm strikes, the hit marks | 12 frames | A, **fixed**. A flight (the carpet) and a charge of his own: B, left |

### The tournament's bodies and the rest

| sprite | used for | what is weak | fix | pri |
|---|---|---|---|---|
| King Cold 185 | tournament | sound: Cooler's set, Cold's head, a cape per frame; lying is his hit-stun turned | - | - |
| Master Roshi 186 | tournament | sound after the 2026-10-25 fixes | - | - |
| Chi-Chi 187, Videl 188 | tournament | the knock-down (3), lying (12) and tumble (20) are all Android 18's flight frame: no fall, and 18's long streaming hair in black covers the body in the side and back views (Chi-Chi wears a bun, Videl short hair): a black mass with a face | a knock-down from 18's own stagger (her 3) then the lying frame; cut the hair back to the head and redraw the back it hid | B, left (section 4) |
| Chiaotzu 189 | tournament | sound | - | - |
| player bodies 235-244 (`tnyplayables.py`) | tournament | stock frames re-ordered: as good as the stock sets; 240 gains the cap | - | - |
| Buu's Fury imports 158, 159, 195-222, 230 (`buu2log2.py`) | tournament, announcer | clean conversions, looked at as idle / hit-stun / attack / lying per facing. Yakon 198 is one front view in every facing (Buu's Fury drew him so); Broly 159's side sash is his own red (right for him) | Yakon needs drawing: not worth it | C |
| the mother dinosaur 245, her babies 246 (`dinosprites.py`) | actor (the eggs) | the recolour is by rows: pink above a line, orange under it, green feet. Seen from the side the pink / orange edge is a straight line across the body | follow the frill's outline (the pink only on the frill and horns' base) | B, left |
| the second Perfect Cell 162, haloed Goku 176 / 177, Super Saiyan 2 Gohan 58 | actor, players | stock frames (a copy, a halo ring, groups borrowed from 59): nothing to fault | - | - |

## 3. Fixed in this pass (one commit each, so any can be dropped)

| commit | what | checks |
|---|---|---|
| `49e63a2` | tools: `tools/artsheet.py`, `tools/headless/art_poses.py`, the runner's `pose` / `entmove` | - |
| `5b1ddf2` | Android 16 (160, 175): the 17-derived groups as bright as his own, no hair specks, back-view heads on the shoulders | `art_poses.py --ids 160,175`: 144 / 204 frames |
| `9150435` | Android 13 (165, 240): the drawn trucker cap on every frame; back-view kicks without the white boxes | `--ids 165,240`: 132 / 204 frames |
| `4c8397a` | Android 13's dialogue portrait 169: his own face, the cap says 13 | `ppportraits.py --check`; a line in game (`tny_talk` t8) |
| `2ba37d0` | Hercule (72): the flight seen from the side | `hercule_fly.txt` (a real flight), `--ids 72 --groups 12` |
| `7366a03` | Super Saiyan Hercule (157): the Kamehameha pose and the side flight, gold | `--ids 157`: 240 frames |
| `e105ff0` | Mr. Popo (163): side palm strikes, hit marks | `--ids 163`: 196 frames; `chamber_popo.txt` |
| `f5ebdce` | Super Android 13 (180): the side sash, the forearm cuff (43 frames) | `--ids 180`: 320 frames |
| `1fd72ec` | the Time Chamber's EXIT sign (`chamberpath.py`) | seen in game beside the door home; the chamber tests |

Every one was looked at before and after at 1x and 4x (group x facing x frame sheets of the built ROM) and then frame by
frame in the running game. Details per sprite: `docs/sprites.md`.

## 4. Left, and why

- **Stock actors 8 (Android 16) and 76 (Krillin): the empty groups.** The cheapest cutscene win left: a repointed record
  with the stock groups by ROM address and the missing ones from 160 / 161 (the way 72 and 58 are repointed). Not done
  here because the record's address changes, and the staging tests find these actors by the stock address (`entrec
  0x081E07C4` / `0x082E94B4` in `ch9_actors.txt`, `ch1_notes.txt`): a test written in parallel against the old address
  would count 0 actors and call it expected. Do it in the main session with those tests moved to the index (`deref`).
  Until then scenes play only the groups in section 1 on 8 and 76.
- **Super 13: flight and lying.** His 12 is the death (`S13_G_DEATH`, read by `super13.c` and the tournament). A flight
  needs a new group and the death moved (27), or scenes fly him with a C mover holding 13 (his walk) or 23. Broly has no
  flying frames in Buu's Fury (anims 0-21 checked): the pose would be his dash lean, built.
- **Android 13: shout (4) and hurt (11).** His record keeps 17's 13-group layout for the boss AI; a taunt (the smirk,
  thumb at his chest) would be drawn on his idle. The player body 240 has both (borrowed).
- **Super 13's portrait 122** is still Android 16's face in blue, the fault Ryan named on 169. Tried on the new base
  (Tien's 95, blue, red eyes) three ways: the bald skull reaches the frame's top, so drawn spikes read as a crown, and
  16's mohawk cut out and mirrored does not sit on the other head. Not committed. It needs hair drawn by hand over a
  face moved down, or a look at the result by Ryan first (he may want the two faces to match, or not).
- **Chi-Chi and Videl's fall and lying frames**, **the dinosaurs' banding**, **Popo's flight and charge**, **the flat gold
  afro**, **16's build in the borrowed groups**: each is drawing work of an hour or more per sprite for a body seen
  briefly; listed in section 2 with the fix.
- **The Buu's Fury imports** were not gone through frame by frame again (their last pass was 2026-11-01); the overview
  showed no new fault.

## 5. Portraits

Both ROMs' portrait tables were drawn whole and looked at (LoG2 `Portrait_Table` ids 26-114; Buu's Fury's module
0x086F393C ids 8-162, 155 pictures). **Neither has Android 13 or Super Android 13.** Buu's Fury has Broly (16),
Bojack's crew (104-109) and Janemba (81), no android but 18.

| id | what | verdict |
|---|---|---|
| 119, 121 | Krillin's and Android 16's status portraits (dialogue portraits remapped to the menu palette) | fine |
| 120 | Paragus (Vegeta's 106 aged) | fine |
| 122 | Super Android 13 (Android 16's 26 in blue) | **16's face**: section 4 |
| 126, 127, 140-168 | Buu's Fury portraits on LoG2's frame | fine: Buu's Fury's own drawings; a little heavier in line than LoG2's |
| 169 | Android 13 | **redone** (section 3): Tien's three-quarter face, a yellow trucker cap with "13" and the Red Ribbon dot, long white hair, a yellow vest, the androids' red background |
| 170, 171 | Cell Jr., the Namekian Warrior (recolours of the body's own stock portrait) | fine |
| 172 | Evil Buu (Buu's Fury's Super Buu in grey) | fine |
| 173 | the mother dinosaur (King Triceratops' 96 recoloured) | fine; the same row banding as her sprite, less visible on a face |

## 6. Items and small things

| thing | made by | verdict |
|---|---|---|
| the Time Chamber's six entrances | `tools/chamberart.py` (drawn by code) | the best of our map art: dithered, outlined, each reads at once. Nothing to do |
| the Saiyan ship, the van, the board, the world-map landmark | `shipart.py`, `vanart.py`, `boardart.py`, `wmlandmark.py` | not re-audited in this pass |
| the chamber's jars, capsules, Senzu, the lever plaques | stock decorations and stock tiles (`docs/time-chamber.md` 3) | stock art: nothing to fix |
| the chamber's door home, portals (ring, lit ring, the swirl's two frames) | `tools/chamberpath.py` (drawn by code; `python -I tools/chamberpath.py OUT.png`) | good: shaded, they read at 1x |
| the chamber's EXIT sign | `chamberpath.py` `sign` | was the one placeholder on that sheet: a flat green rectangle with a rim. **Fixed**: cut corners, the white line a real exit sign has inside its rim, light from the top and shade along the bottom, lit posts with the board's shadow |
| the red carpet | `chamberpath.py` `carpet` | seen in the chamber tests' films only; not changed |
| Krillin's Disc, the ki blast recolours, 16's missile, fist and explosion, Super 13's orbs | `krillinsprites.py`, `a16sprites.py`, `super13sprites.py` | seen in the fight tests; small and readable. Not changed |

One small thing was changed (the sign); the time went to the bodies, which is where the scenes need it.

## 7. Redoing the audit

```
python -I tools/artsheet.py build/<dir>/log2-coop.gba build/<dir>/coop.sym OUT.png --ids 165,9 [--side] [--first] [--groups 0,5,12] [--scale 4] [--box]
python tools/headless/art_poses.py build/<dir>/log2-coop.gba build/<dir>/coop.sym build/<dir>/hl/menu.ss --ids 165,160 --out build/<dir>/hl
```

`artsheet.py` reads the relocated sprite index of a built ROM, so generated and stock records come out the same way:
one row per group, the four facings, every frame, on a ground line with the entity's position marked; several ids one
under another or side by side. It prints each record's filled groups and frame counts (section 1 was written from
that). `art_poses.py` shows the same frames through the game: an actor of the sprite beside player 1, held in every
group, facing and frame by the runner's `pose` (a scratch group in `coop2.boss_rec` whose frame 0 is the wanted frame,
the timer held at 0), a shot of each, tiled; it fails on a CRASH line, a pose that found no body, or an entity list that
changed. It needs a runner built after 2026-11-06 (`docs/testing.md` "Art checks").
