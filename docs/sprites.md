# Sprite inventory (tools/sprites.py; Buu's Fury: tools/buu_sprites.py, tools/buu2log2.py, last section)

`python tools\sprites.py roms\log2.gba build\sprites` writes `build/sprites/atlas.png` (every sprite
id, first frame facing down/up/left/right), `build/sprites/<id>.png` (every pose group x 4
directions) and `build/sprites/report.md` (groups, frames, sizes). Sprites are 8-bit OBJ tiles,
frames compressed with the game's decompressor, one shared 256-colour OBJ palette at
`0x081DA6C8`; sprite ids 3..156 index `g_CharacterSpriteIndex` at `0x083B4E74`.

## Record format (verified in game, 2026-10-17)

```
+0x00  u8  ?            (0 for every sprite)
+0x01  u8  ?            (4)
+0x02  u8  group count  number of 16-byte pose groups that follow the header (Hercule 21, Gohan 21,
                        Piccolo 103 23, enemies 3-8, NPCs 1-2); `sprite[2] < 0xF` means "no fidget"
+0x03  u8  ?            (0x10)
+0x04  u32 (8)          then 0, then four {s32 x, y, w, h} collision boxes (-2, -26, 18, 10 for Hercule)
+0x4C  groups[count][4] descriptor-array pointer per facing (down, up, left, right); a group may be
                        all zero and must stay in place: the game addresses groups by fixed index
                        (idle 0, walk 1, ..., fidget 14, beam groups 18/19), checking for 0.
```

Each group's four descriptor arrays are back to back and the group's frame blobs follow them, so an
array's frame count = (next array or next blob address - array) / 12. Frames are
`{s8 xoff, s8 yoff, u8 w, u8 h, u32 attr, u32 blob}`; blob = `{u32 kind, u32 size, stream}` decoded
by the IWRAM decompressor with r3 = kind (1 or 2; 0 = plain copy). A literal-only stream (kind 1,
starts with the literal-width op, 9 bits per byte) is accepted by the game, so re-encoding recoloured
frames needs no compressor. Loaded frames go through `FUN_0801F394` into the staging buffer
`0x03006B8C` and then to the entity's OBJ VRAM slot. A cloned record must copy *all* groups
(the first clone stopped at the first empty group and the transformed player could not walk).

## What the pose-group count means

A record holds one 16-byte group (4 direction pointers) per pose set. Town NPCs have 1-2 groups
(idle, walk), enemies 3 (walk, attack, hurt), playables 9-10 (walk, run, attacks, charge, fly,
hurt, ...). A character can be made playable cheaply only if its sprite already has the attack and
hurt groups a player row expects; otherwise its missing poses must be drawn (or borrowed from a
similar body and recoloured).

## Candidates (identities from the atlas, to be confirmed in play; ids are sprite ids)

| id | groups | looks like | playable-grade? |
|---|---|---|---|
| 53-56 | 9-10 | Gohan (base, SSJ, variants) | yes (stock) |
| 57-59 | 9-10 | Trunks (base, SSJ) | yes (stock) |
| 60-62 | 9-10 | Goku (base, SSJ) | yes (stock) |
| 66-71 | 3 | Vegeta forms as NPC/enemy-grade sets | partly; 137-140 look like the playable Vegeta/Goku sets (9-10) |
| 72 | 9 | Hercule | yes (stock) |
| 73, 103, 104 | 1 / 10 / 9 | Piccolo (NPC, playable with cape, variant) | yes (103) |
| 76 | 7 | **Krillin** (bald, orange gi, attack and hurt poses) | yes, needs abilities + a row |
| 121 | 7 | **Tien** (bald, green pants, attack and hurt poses) | yes, needs abilities + a row |
| 75 | 1 | Master Roshi | no (idle only) |
| 94 | 2 | Android 18 (blonde) | no (NPC poses) |
| 8 | 7 | **Android 16** (orange mohawk, green armour): idle, walk, hit-stun (2-4), one punch (6), knock-down (12) | the island scenes' actor; the boss uses id 160 (below) |
| 9, 10, 11 | 13 / 2 / 13 | Android 17 fight set (the island boss), 17 NPC, Android 18 fight set | enemy-grade, full |
| 15 | 3 | Dr. Gero (white hair under a glass dome): idle, walk, knock-down | NPC (not Android 16: an earlier note said so) |
| 16-19 | 1-9 | a man in a suit, the tiger bandits | NPC / enemy-grade |
| 29, 30, 28 | 4 | Imperfect Cell, Perfect Cell, Cell Jr. | enemy-grade |
| 34, 48, 49, 50, 74 | 2-3 | Frieza final form, Mecha Frieza (small), King Cold (large cape) | enemy-grade; a playable Mecha Frieza needs run/charge poses drawn |
| 37-39, 44-47 | 3 | large and small robots | enemy-grade |
| 23, 24, 95, 102 | 2 | Bulma, Dende, Chiaotzu | NPC |
| 63-65, 105-106, 108-110, 112-119, 122-127, 136, 144-149 | 2-3 | birds, dragons, dinos, scorpions, snakes, wolves, a squirrel-like animal (136), boars | enemy-grade; 136 is the animal-quest candidate |
| 128-135, 141-143, 150-156 | 2-10 | more human sets (kid Goku/Gohan, purple-haired, red-haired) | check sheets |

## Super Saiyan Hercule (done, 2026-10-17)

`tools/spriteclone.py` clones record 72 with the afro recoloured (connected black components in the
upper half of each frame: interior -> gold 120, brown highlights -> yellow 46; eyes, moustache and
outlines stay black), re-encodes every frame literal-only, and emits `build/gen/sprites_data.c`:
`ssj_record` (all 21 groups, 144 frames, ~130 KB), `sprite_index[158]` (stock 3..156 plus 157 ->
`ssj_record`) and `display_rows[23 x 20]` (stock rows; row 21 Hercule now links transformed -> 22;
row 22 = Hercule's row with flag bit0, sprite 157, detransformed -> 21). `patch_rom.py` repoints the
18 display-row literals and the one sprite-index literal (`table + 0x1C`, file 0x959C) at the new
tables and raises `CharacterData_FindBySprite`'s row limit (file 0x42D6) to 23. `src/coop/ssj.c`
transforms through the game's own opcode-73 handler `{2, slot, 157}` (EP cost 50) and re-binds
player 2's entity when the slot is player 2's; `src/coop/rom.h` `DISPLAY_DATA` is the relocated
table. The hercule-hero session wired it as ability 18 in Hercule's third slot (`docs/hercule.md`).
Verified in the fork: transform, gold afro, walk, attack, HUD intact.

## Buu's Fury (roms/buu.gba, USA BG3E): format, tools, ids (2026-10-17)

Same engine family, different tables. Found by scanning for pointer tables into regions of
compressed blobs (`{u32 kind, u32 size, stream}` headers), then following the pointers back to
frames, animations, records and the index; verified in game through the converter below.

| what | where | notes |
|---|---|---|
| decompressor | ROM `0x087B79E4` -> IWRAM | byte-identical to LoG2's (`0x087FD6BC`); `tools/mapchunks.py` decodes both |
| OBJ palette | `0x0805652C` | 256 x BGR555, index 0 transparent, 1 black, 2 white, 3 red...; code literals at file `0x4F14`, `0x5458`, `0x5DB8`, `0x65B4`, `0x14C4C`, `0x15940`, `0x17C0C`, `0x21BCC` |
| sprite index | `0x086B6BDC` | 297 record pointers, ids 0..296 (no 3-offset); read by code at `0x080003C0` and `0x0801715C` |
| records | `0x086A89D0`..`0x086B6BDC` | packed in id order, so a record ends where the next begins |

Record (`tools/buu_sprites.py` `BuuSprites.record`):

```
+0x00  u8  animation count     +0x01/+0x02 0     +0x03 u8 flags (4 = 32x32 class, 2 = 16x32 NPC,
                               0x10 = 64x64, also 0, 1, 8, 0x18, 0x24)
+0x04  u32 0
+0x08  u32 0x040A0810          219 of 297 records; a few carry 0x0810, 0x1040, 0x2040808, 0xA181020
+0x0C  u32 0
+0x10  four {s32 x, y, w, h}   collision boxes, as in LoG2 (+0x0C there)
+0x50  u32 0                   (4 records hold a ROM pointer here; meaning unknown)
+0x54  u32 anim[count]         0 = no such pose; indices are fixed per pose, as LoG2's groups are
+...   u8 remap[256]           optional, 65 records: per-record colour remap table (palette index ->
                               palette index) applied to every frame; this is how Buu does recolours
```

Animation: `u32 frames`, then `frames x 4` descriptor pointers in the order down, up, left, right
(one animation holds all four facings; LoG2 keeps four separate descriptor arrays per group).

Descriptor: `u32 parts; s8 x, s8 y, u8 w, u8 h` (frame box), then `parts x {s8 xoff, s8 yoff, u8 w,
u8 h, u32 attr0 | attr1 << 16, u32 blob}`: the 12-byte part **is** LoG2's descriptor. 13588 frames
have one part, 64 two, 8 four (a 32x48 frame = 32x32 + 32x16 parts, for example). Blobs are all
kind 1, `size = w * h`, 8-bit 1D-mapped OBJ tiles. Right-facing frames share the left blob with
attr1 bit 12 (h-flip); LoG2 does the same in 2665 of its frames, so attributes pass through.

Pose indices differ between the games (read off the sheets of the playables):

| pose | LoG2 group (frames) | Buu anim (frames) |
|---|---|---|
| idle, walk, run | 0 (2), 1 (4), 2 (4) | 0 (2), 1 (4), 2 (4) |
| fall / knocked-down sequence | 3 (4) | 17 (4) |
| shout (open mouth) | 4 (4), 13 (4) | 12 (2) |
| charge / raised fist | 5 (3) | 3 (3) |
| attacks | 6, 7, 8 (4 each) | 4, 5, 6 (4 each) |
| hurt | 11 (2) | 9 (3) |
| dazed sitting | 12 (4) | - |
| fidget | 14 (5) | 7 (1, flex) |
| halo (dead) | 17 (2) | - (13 = plain stand, 2) |
| beams | 18, 19 (7) | 19, 21 (Broly: 8, 6) |
| tumble | 20 (4) | 17 |
| extra in Buu | - | 8 (11, carrying an item), 18 (1, fist up), 14-16 (3-4, ki poses) |

### Tools

- `python tools\buu_sprites.py roms\buu.gba build\buu_sprites` -> `atlas.png` (every id, first
  frame x 4 facings), `<id>.png` (every animation x 4 facings), `report.md` (anims, frames per
  anim, max frame box, multi-part frames, remap flag). About 20 s.
- `python tools\buu2log2.py roms\log2.gba roms\buu.gba build\gen\buu_sprites_data.c 158=128 159=28
  --preview-dir build --report build\gen\buu_sprites_report.md` -> LoG2-format records as C data
  (`buu_record_158`, `buu_record_159`), `build/buu_preview_<id>.png` (Buu colours left, converted
  frames in LoG2's palette right, every group) and a colour report. Per frame it composites the
  parts, applies the record's remap table, maps each Buu palette index to the nearest LoG2 OBJ
  colour (RGB distance), tiles to the smallest OBJ shape and compresses with its own LZ encoder for
  the game's decompressor (literals, zero/near codes, short and gamma matches, repeat-offset; every
  frame is round-tripped through `mapchunks.decompress`). Hercule: 20 KB for 77 blobs where
  literal-only would be 66 KB; Broly 60 KB against 221 KB. The output copies a LoG2 template
  record's group layout (default 72 = Hercule: 21 groups with Hercule's frame counts, `--template`),
  filling each group from the mapped Buu animation and cycling its frames to the template's count,
  because the game's animation tables index frames per group; `--map <id> g:a,...` overrides the
  table above for one id.
- Build: `BUU=1` (default when `roms/buu.gba` exists) adds `build/gen/buu_sprites_data.c` to the
  link and appends the records to the relocated sprite index (`spriteclone.py --extra`);
  `BUU_SPRITES="158=128 159=28"` picks them; `SSJ_SPRITE=158` makes the Super Saiyan Hercule row
  (22) use a converted sprite instead of 157 (test builds). `BUU=0` builds without the Buu ROM.
  New ids need display rows of their own before a character can use them (row 22 is the only
  relocated row so far).

Colour report (`build/gen/buu_sprites_report.md`): Hercule keeps 24574 pixels exact and moves 2429
pixels in 10 colours, all greys and dark browns within distance 13-51 (worst: a near-black brown
`#311000` -> black); Broly keeps 58401 and moves 15861 in 15 colours (hair yellows 17-42, red sash
33, orange 16); nothing visible at 1x in the previews.

**In game (2026-10-17):** `make BUILD=build/buu DEBUG_MENU=1 SSJ_SPRITE=158`, savestate 1, party
rows poked to 22, player 2 cycled with Select: player 2's entity took `buu_record_158`
(`sprite_record` at entity +0x48 = `0x088328A8`) and walked, turned and attacked as the Buu-saga
Hercule (`build/buu/proof_ingame.png`, `build/buu/t13`-`t19`). Broly (159) converts and compiles
but was not spawned in game.

### Ids in the Buu ROM (from `build/buu_sprites/atlas.png`; identities read off the art)

Full fight set (attack groups 2-6 present; boss or playable grade): 28 **Legendary Super Saiyan
Broly** (64x64, 22 anims, beams), 33 Fat Buu, 37/39 Kid Buu, 40-43 Super Buu forms, 63 Dabura
(cape), 92-93 Super Saiyan Goten/Trunks, 97-120 the party (Goku, Gohan, Goten, Trunks, Vegeta,
Gotenks and their Super Saiyan forms; 110-111, 120 SSJ3), 128 **Hercule** (brown gi, afro,
moustache: idle, walk, run, charge, three attacks, hurt, shout, fall), 144 horned demon in red
(64x64), 212-215 Goten/Trunks variants, 259-260 Gohan/Goten, 263 **Uub** (dark skin, mohawk, red
and white), 269-277 Vegeta and Vegito forms.

NPC / cutscene grade (2-20 anims, no attack set): 0-1 ships, 2 mushroom-cap man, 4 Future Trunks
with sword, 5 armoured Saiyan, 6 Android 18, 7/67/69/70 pirate robot, 9-12 and 229 Dr. Briefs
variants, 18 Baba, 21/272 Yamu with the energy container (244 = Yamu recoloured purple),
29/112/262 Van Zant (sunglasses, "HUGE" tank top, rifle), 113 Smitty, 31 Bulma, 50-54 a crouching
brown creature in five palettes, 55 Chi-Chi, 56 Videl, 64 Dabura cutscene, 81 Roshi,
86-87/210/220 ghost Kamikaze (Gotenks), 88-89 Dabura large, 137-143 Fat Buu large and parts,
149 Supreme Kai, 154 Babidi's ship, 183 Mr. Popo, 184 blonde girl (Erasa), 196-198 Bulma / Launch
types, 199 Oolong, 207/216 tournament announcer and Spopovich, 242 pale giant, 280 Dragon Ball,
290-291 green dragon beast, 294 pale muscle man in purple.

**Android 13 is not in this ROM.** Every first frame of all 297 records and every full-set record
was inspected: no cap, no silver-haired muscle man, no blue-skinned Super 13; the nearest shapes
are Broly 28 (big, full moveset) and Van Zant 29/262 (big, NPC set). The story's base-form Android
13 has to be drawn (a recolour of a big human set such as 28, or of LoG2's Android 16/17 bodies).

## Android 16 boss sprite (id 160, `tools/a16sprites.py`, 2026-10-18)

Android 16 is **sprite 8** in the stock game (the island scenes place him as `actor sprite 8` on
maps 296-298); sprite 15 is Dr. Gero. Sprite 8 has 7 groups: idle 0, walk 1, hit-stun 2-4, one
punch 6 and the knock-down 12, so a fighting 16 needs only the knock-back, the charge pose and two
more attacks. `tools/a16sprites.py` builds **sprite id 160** (`a16_record`, `A16_SPRITE` in the
Makefile) from the ROM at build time into `build/gen/a16_sprites_data.c` (preview
`build/<dir>/a16_preview.png`):

| group | source | frames per facing |
|---|---|---|
| 0 idle, 1 walk, 2 hit-stun, 6 punch, 12 knock-down | sprite 8's own descriptor arrays, referenced by ROM address (nothing copied; sprite 8 is unchanged for the cutscenes) | 2, 4, 4, 4, 4/4/4/1 |
| 3 knock-back, 5 charge, 7 and 8 attacks | sprite 9 (Android 17) frames, recoloured and re-headed (below) | 4, 3, 4, 4 |
| 13 rocket punch (new) | sprite 8's punch: wind-up, then the punch with the fist and half the forearm cut away and a grey socket at the elbow (the swing arc removed) | 3 |
| 4, 9-11 | empty, as in sprite 9 | - |

The record copies sprite 9's header (collision boxes) with the group count raised to 14, so the
boss AI set 21 (17, 18 and Cooler use it; it addresses groups by index) finds every group where
17's record has it. The 17 frames: 17's grey vest and orange scarf become 16's green armour
(indices 42/41/46/54), the blue trousers olive, the green shoes grey boots and the skin 16's skin
ramp; 17's long black hair (the largest black component in the top of the frame) and face are
cleared and 16's head (his idle frame of the same facing, cut above the first row with armour
pixels) is pasted with its chin on 17's chin row and its face centre on 17's. 16's build: his
own stock frames are already broader than 17's at the game's 16x32 / 32x32 sizes, and the four
groups taken from 17 keep 17's silhouette below the head (a wider torso would need hand-drawn
frames). Frames are LZ-compressed with `buu2log2.compress`.

### Rocket, fist, bomb and explosion frames (same generator)

The ROM's projectile class (`0x08013BC0` base, 0x58-byte objects, vtable slots 3 cull / 4 OAM /
5 DMA) draws **raw** 8-bit tiles: a descriptor `{s8 xo, s8 yo, u8 w, u8 h, u32 attr0|attr1<<16,
tiles}` whose `xo, yo` is the anchor (8, 8 for 16x16), uploaded by DMA when the object's frame
pointer (+0x18) changes. There is no missile in the ROM; what exists and is reused:

| ROM frames | what |
|---|---|
| `0x086FFBFC` x3 (16x16) | a black bomb with a burning fuse, thrown by `FUN_0801CCC8` (an arcing bomb whose landing broadcasts a type-2 hit and explodes through `FUN_0801CD32`) - **reused as 16's area explosive** |
| `0x086AF614` x4 (16x16, 32x32 x3) | the dust-cloud explosion (`FUN_0800FA1C`) - **recoloured to fire** for the rocket explosions |
| `0x0840E47C` x16, `0x0840E53C` | the NPC ki blast (4 facings x 4) and its impact |
| `0x086E1590` x4 | white impact ring |
| `0x086FF8DC` 4 velocity pairs | the debris particles `FUN_0800EC08` spawns around an explosion |

`tools/a16sprites.py` adds, as raw tiles in `build/gen/a16_sprites_data.c`: `a16_missile[16]`
(a small grey missile with a red nose, fins and exhaust, drawn procedurally at 8 angles from 0 =
right clockwise, 2 flame frames each), `a16_fist[8]` (the rocket fist drawn by hand pointing left,
rotated to right / down / left / up, 2 flame frames each) and `a16_boom[4]` (the dust explosion
with its colours mapped by brightness onto white / yellow / orange / dark orange / red).

## Android 13 (id 165) and Cooler's Namekian servant (id 166) (`tools/bossmodels.py`, 2026-10-18)

Both are built from the ROM at build time into `build/gen/boss_models_data.c` (records `a13_record`,
`namek_record`; previews `build/<dir>/a13_preview.png`, `namek_preview.png` and `_x3` copies). The
Makefile passes `A13_SPRITE=165` and `NAMEK_SPRITE=166` to the C code and appends both records to the
relocated sprite index; `src/coop/bosses.c` gives them to the co-op twins (`docs/coop-design.md`
"Boss rules and extra stats").

**Android 13, base form** (white hair under a yellow-and-black trucker cap with a red Red Ribbon dot,
pale skin, a dark-yellow sleeveless vest, blue jeans, brown gloves, dark boots). Neither ROM has him
(`docs/sprites.md` "Buu's Fury"), so he is a re-dress of the biggest android body: the same
composition as Android 16's sprite 160 (16's own sprite 8 for idle 0, walk 1, hit-stun 2, punch 6,
knock-down 12; Android 17's fight frames re-headed with 16's head for the fall 3, charge 5 and attacks
7, 8), then every frame is recoloured by body part. Parts are found per frame from geometry, so lying
and attacking frames work too: the "up" axis runs from the body's centroid (armour and trousers) to
the hair's centroid; the face is the skin touching the hair; the torso is a band 3.6 px either side
of the face's line along that axis; the waist sits at 70 % of the body's length from the head.

| part (16-space colours) | Android 13 |
|---|---|
| hair (16's orange mohawk) at or above the forehead line, at most 3.6 px deep | trucker cap: yellow crown, black outline, black band over the forehead, red dot in front views, a 2 px black peak over the face in side views; mohawk above the cap cut off |
| hair below the cap | white hair (sides and back); in front views hair inside the face's span (16's fringe) becomes forehead skin, two columns of white hair stay at each side for five rows |
| face skin | pale skin (one step lighter than 16's) |
| other skin (hands) | brown gloves |
| armour in the torso band, above the waist | dark-yellow vest (16's red chest light stays: the Red Ribbon badge) |
| armour outside the band, above the waist | bare pale arms |
| armour below the waist; 17's blue trousers | blue jeans |
| back views (facing up) | the face pixels become hair: white hair under the cap |

The head gets a black outline (transparent pixels next to cap, face or hair), which 16's head lacks.
The record keeps 17's header and group layout (13 groups: 0-3, 5-8 and 12 filled), so Android 19's AI
set 29 and the melee set 21 find every group they play, and group 3 (stand, stagger, fall, lie) is
there for the both-down rule. 82 frames, all LZ-compressed (`buu2log2.compress`).

**Cooler's Namekian servant**: Piccolo's capeless complete set (sprite 104, all 23 groups, 168 unique
frames; 103 is the caped set) recoloured, nothing redrawn: darker teal-green skin (48/49/50 ->
teal ramp), a maroon warrior gi (88/89, outline 211), a gold sash (77/195/72), black boots (34/35),
the arm patches pink instead of red (26 -> 247-like pink, 247 lighter) so they stand out from the gi.
Distinct from Piccolo (player 1 in the Cooler fight) at a glance. Its group 3 is a fall that ends
lying, used by the both-down rule.

## Making a new enemy model (generic)

What an enemy record needs depends on the AI set its stat names, because AI modules and enemy control
states address pose groups by fixed index and read the four direction pointers of a group without
checking the record's group count: a group past the count reads whatever follows the record (descriptor
arrays, compressed frame bytes) as pointers. A group inside the count may be all zero only where the
code checks for 0. Rules of thumb:

- Copy a header (`+0x00..+0x4B`: group count, collision boxes) from a stock enemy of the same size
  class (32x32 bodies: 9, 12, 14; 64x64: 34, 74). Byte 1 is 4 for 32x32 bodies and 0x10 for 64x64.
- Fill at least every group the stock sprites of that AI set have (table below; from the 666 enemy
  records in the ROM: groups present on every sprite the game pairs with the set), keep empty groups
  in place, and set the group count to cover them.
- Add group 3 (stand, stagger, fall, lie) for a boss that can be the first of a co-op pair to fall
  (both-down rule, `enemies.c` `DOWN_GROUP`); without it the downed boss freezes in whatever pose
  the knock-back leaves (Android 19, Cooler).
- Group 2 is the hit-stun; group 12 the knock-down / death pose of the androids and Cell forms.
- Build frames with a generator under `tools/` (never commit ROM-derived data): reference unchanged
  stock descriptor arrays by ROM address (as `a16sprites.py` does) or recolour and re-encode them
  (`bossmodels.py`, `buu2log2.compress`); add the record to `EXTRA_SPRITES` in the Makefile (the
  relocated index takes any id; gaps are 0) and preview every group x 4 facings before testing.

| AI set | stats (stock) | sprites the game uses with it | groups on all of them |
|---|---|---|---|
| 8 | wolves, snakes and other animals (8, 14, 37-40, 53...) | 17, 44-47, 90-92, 112-119 | 0, 1, 2, 6 |
| 21 (boss melee) | 3 (17), 4 (18), 26 (Cell Jr), 29 (Cooler), 41, 106 | 9, 11, 31, 34, 48, 156 | 0, 1, 2, 5, 6, 7, 8 |
| 29 (boss) | 5 (19), 6, 7 (Gero) | 12, 14 | 0, 1, 2, 5, 6, 7, 8, 12 |
| 45 / 52 | 9, 10 / 75, 86 (tiger bandits, ...) | 18, 19 / 96, 98 | 0, 1, 2, 6, 7, 8 |
| 59 / 91 | 11, 16, 99, 101 / 31-33 (robots) | 37, 122-124, 127 / 37-39 | 0, 1, 2, 6 / 0, 1, 2, 6, 7 |
| 80 / 116 | 60 Imperfect Cell / 76 Semi-Perfect Cell | 28 / 29 | 0-3, 5-10, 12, 18, 19 |
| 98 | 64, 98 (tournament Krillin, Tien) | 76, 121 | 0-6, 12 |
| 129 | 44 (the red robot) | 50 | 0, 1, 2, 5, 6 |
| 138 / 148 | 21-25 / 78, 79 (Perfect Cell) | 30 | 0-3, 5-8, 12, 18 |
| 11, 15, 32, 38, 66, 74, 106, 149 | regular enemies | (various) | 0, 1, 2, 6 (106 also 18) |

So a regular enemy needs idle, walk, hit-stun and one attack; a boss on set 21 or 29 needs the charge
pose and three attacks as well. King Cold's sprite 74 (5 groups: 0, 1, 3, 4; group 2 empty) under set
21 crashes the first time the AI charges or attacks: groups 5-8 lie past its count and read compressed
frame bytes as descriptor pointers ("Jumped to invalid address C002E564", reproduced in the fork). It
would need a fight set drawn before it can partner Cooler; the story uses the Namekian servant instead.
