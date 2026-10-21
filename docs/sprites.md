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

Each group's four descriptor arrays usually sit back to back with the frame blobs after them, but the
record does not store a frame count: `Sprites.frame_counts` counts the run of valid descriptors (a GBA
OBJ size and a ROM blob whose header kind is 0-2) up to the next array start of any stock record (see
"Frame counts" below; the gap to the next array or blob is wrong for 100 stock arrays). Frames are
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
| 57-59 | 18-22 | teen Gohan (57, row 3, his party row), Super Saiyan 2 Gohan (58, row 4, purple gi), Super Saiyan teen Gohan (59, row 5); checked against the display rows 2026-10-19 (Trunks is 135, row 18) | yes (stock; 58 lacks hurt / lying / tumble, see "Super Saiyan 2 Gohan") |
| 60-62 | 9-10 | Goku (base, SSJ) | yes (stock) |
| 66-71 | 3 | Vegeta forms as NPC/enemy-grade sets | partly; 137-140 look like the playable Vegeta/Goku sets (9-10) |
| 72 | 9 | Hercule | yes (stock) |
| 73, 103, 104 | 1 / 10 / 9 | Piccolo (NPC, playable with cape, variant) | yes (103) |
| 76 | 13 (7 used) | **Krillin** NPC set: idle, walk, run, knock-down, shout, ki pose, punch combo, lying (12) | playable as **sprite 161** (below) |
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
`ssj_record` (all 21 groups, 144 frames, ~130 KB), `sprite_index[158]` at the time (stock 3..156 plus 157 ->
`ssj_record`; it now runs to id 177, `docs/ids.md`) and `display_rows[23 x 20]` (now 28 rows) (stock rows; row 21 Hercule now links transformed -> 22;
row 22 = Hercule's row with flag bit0, sprite 157, detransformed -> 21). `patch_rom.py` repoints the
18 display-row literals and the one sprite-index literal (`table + 0x1C`, file 0x959C) at the new
tables and raises `CharacterData_FindBySprite`'s row limit (file 0x42D6), first to 23, now 28. `src/coop/ssj.c`
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
  New ids need display rows of their own before a character can use them (rows 22-27 exist:
  `docs/ids.md` "Display rows").

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

### Android 16 as a player (sprite 175, `a16p_record`, same generator, 2026-10-19)

For the "Gero's computer" chapter (`docs/story.md` 4g) 16 plays as a party member (last party slot,
display row 25, `src/coop/a16player.c`). `tools/a16sprites.py` also emits `a16p_record` (Makefile
`A16P_SPRITE` = 175; preview `build/<dir>/a16_preview_player.png`): Goku's playable header (player
collision boxes, sprite 60) with 21 groups filled the way Hercule's stock set is (9, 10, 14, 15, 16,
18, 19 empty), every group taken from sprite 8 (by ROM address) or from the boss record above (by
name), nothing new drawn:

| player group | source |
|---|---|
| 0 idle, 1 walk, 2 run (= walk), 13 dash (= walk), 17 halo (= idle) | sprite 8 groups 0 / 1 |
| 3 knock-down, 20 tumble | boss group 3 (17's knock-back re-headed) |
| 4 shout | sprite 8 group 4 (looking up) |
| 5 ki pose | boss group 13, **the rocket punch**: every ability animation that plays group 5 shows the forearm launched |
| 6 punch combo, 7, 8 attacks | boss groups 6, 7, 8 |
| 11 hurt | sprite 8 group 2 (hit-stun) |
| 12 lying | sprite 8 group 12 |

Display row 25 (`tools/spriteclone.py`, `--a16-sprite`): Gohan's standard row with sprite 175,
dialogue portrait 26 and **status portrait 121** (120 at first; Paragus' dialogue portrait took 120 in the merge), his dialogue portrait remapped to the menu BG
palette and appended to `portrait_module` by `tools/krillinsprites.py` (which also writes the
slot-7 names "Android 16" / "ANDROID 16"). `CharacterData_FindBySprite`'s row limit is 28 (rows 26 / 27: haloed Goku).
Seen in the fork as player 2 beside Krillin: walking, punching, the rocket punch pose and fist.

## Krillin (sprite 161, `tools/krillinsprites.py`, 2026-10-18)

Krillin's stock record (76) has groups 0-6 and 12 only (byte 2 says 13 groups; 7-11 are empty). A
player row needs the attack combo groups 7 and 8, hurt 11, the flying lean 13, the fidget 14, the
special melee 16 (the debug party unlocks the charged A attack for every slot but Hercule's), the
halo 17, the Kamehameha pose 18 and the tumble 20. **Goku's playable record (60) wears the same turtle
gi in the same palette indices (orange 32-35/39, blue 79/80/195/215) and the same frame boxes**
(16x32 idle, shoulders on row 15 in both), so `tools/krillinsprites.py` builds **sprite id 161**
(`krillin_record`, Makefile `KRILLIN_SPRITE`) at build time into `build/gen/krillin_sprites_data.c`
(preview `build/<dir>/krillin_preview.png`):

| group | source |
|---|---|
| 0-6, 12 | sprite 76's own descriptor arrays, by ROM address (76 stays untouched for the cutscenes) |
| 7, 8, 11, 13, 14, 16, 17, 18 | sprite 60 frames re-headed: Goku's hair (largest black component in the top 45 %) and face are cleared and Krillin's head, cut from his idle frame of the same facing (top row to the row before three gi pixels), is pasted with its chin on Goku's chin row (Goku's idle neck row per facing) and its face centre on Goku's (71 frames, all swapped) |
| 20 (tumble) | sprite 76's group 3 (knock-down): Goku's tumble frames are rotated bodies with no head on top |
| 9, 10, 15, 19 | empty, as in Hercule's set (no transformation, no Spirit Bomb) |

The header is Goku's (collision boxes) with the group count 21. Frames are LZ-compressed with
`buu2log2.compress`. Seen in the fork as player 1 and player 2: walking, the A combo, hurt, ki
pose, the level-up message (`build/kr/shots/`).

### Portraits (all from the ROM; `Portrait_Table` 0x083EC9D4, `Character_GetPortraitResource` 0x0800BDCC)

`Portrait_Table` holds ids 8..114, each a compressed 64x64 *linear* 8bpp image. Ids 8-25 are the
status-screen portraits drawn into a BG canvas with the **menu BG palette (raw at 0x081D4F50)**; the
others (dialogue, switch screen) use the OBJ palette. A display row's +2 picks the dialogue portrait,
+3 the status portrait. Party ids 2-7 = slots 0-5 (`(id - 2) * 0x1C + party`), so slots 6+ only ever
use explicit ids. **Krillin has a dialogue portrait: id 73** (95 is Tien, 67 Hercule); no status
portrait exists, so the generator remaps 73 to the BG palette by nearest colour and appends it as
**id 119** in `portrait_module`, a copy of the portrait module table 0x083EC9D0 (word 0 = the party,
word id + 1 = `Portrait_Table[id]`, word 116 = the entity list; 120 words copied verbatim) that
`patch_rom.py` points the one literal (file 0xBE10) at.

### Projectiles and icons (same generator)

| symbol | what |
|---|---|
| `kr_kiblast[16]` | the player ki blast's 4 facings x 4 frames (`0x0840E47C`, raw 16x16, same attrs) recoloured by brightness onto a white / cyan / blue ramp; `kiblast_spawn_hook` swaps them in for Krillin |
| `kr_disc[4]` | the Destructo Disc: a 32x32 raw frame (attr `0x80002000`), an ellipse rx 13 / ry 5 with a saw-toothed orange rim, gold ring, yellow body, a white-hot centre line and spokes that rotate per frame |
| `kr_disc_icon_0/1` | HUD ability icons (32x16, 8bpp tiles, 0x200 bytes): the Ki Blast pair with the picture area (rows 2-8, columns 13-28) redrawn as a small disc, the cooling one in the stock greys |
| `kr_status_icon_disc / _ki` | the status screen's 18x8 technique icons (linear, menu BG palette, compressed like `0x086FF6D0`) |

The player ki blast (`FUN_0800E8B0`, 0x54-byte object, vtable `0x08024CBC`): frame base +0x50 =
`0x0840E47C + facing * 0x30`, +0x18 the current frame, velocity from `0x0840E554` (0x300 8.8 per
frame), hit message type = owner +0x16C, attack `0x086DB2E4` `{1, 3, 0x400}`, impact frame
`0x0840E53C`; it is spawned by `FUN_0800EAD8(xy, owner)` from the ki blast control state (animation
34, `0x03002194`, vtable `0x08025D48`; holding B repeats it at 2 EP each).

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

## Paragus and the Saiyan ship (id 170, portrait 120; `tools/brolysprites.py`, 2026-10-19)

**Record header +0 (u16) is the record's largest frame in bytes** (checked on every stock record:
0x200 for 16x32 NPCs, 0x400 for 32x32 bodies, 0x1000 for the 64x64 bosses 27, 30, 34, 37, 74, 105, 122);
the entity's OBJ VRAM slot and staging buffer are sized from it. `tools/buu2log2.py` copied its
template's header (Hercule, 0x400), so Broly's 64x64 frames (id 159) would have overrun their slot; it
now writes the real size (and byte 3 = 0x18 for 64x64, as Frieza 34 and Perfect Cell 30 have). Broly
then draws cleanly as a boss in the fork.

**Paragus (sprite 170, `paragus_record`)**: Vegeta's playable set in Saiyan armour (sprite 137, all 21
groups, 213 unique frames) aged per frame. Vegeta's skin shows only on his face (white gloves, blue
suit), so the hair is the black left after removing the eyes (black with face skin on both sides
within 4 px, across or up and down), taken as the 4-connected black components that touch the face
(8-way), or in back views the component holding the topmost black pixel (the ground shadow under the
boots touches no skin). Hair edges stay black, the inside becomes mid grey, a darker band sits on the
forehead line, Vegeta's grey highlights (13) inside the hair go light grey. A grey goatee: the chin's
lowest face rows at the centre in front views, at the chin's front edge in side views (the side the
stored frame faces, h-flip aware), only when the head is upright (hair centroid above the face's).
Group 9 (the Super Saiyan transform's flames) stays as it is; Paragus never plays it. The header is
137's, so the melee boss AI set 21 finds groups 0-2 and 5-8, and group 3 serves the both-down rule.

**Paragus' dialogue portrait (id 120, `paragus_portrait`)**: Vegeta's (Portrait_Table 106, OBJ
palette) with the hair (black and navy outside the face box, read off the portrait) grey with a black
outline and lighter inner streaks, plus a grey moustache over the upper lip and a pointed chin beard.
Speakers >= 8 read `portrait_module[id + 1]` (`0x0800BDCC`), so `tools/krillinsprites.py` takes
`--extra-portrait 120=paragus_portrait` and appends the word; dialogue `text 120 "PARAGUS: ..."`.

**The Saiyan ship** was sprite 171 (Buu's Fury record 1, a purple ship 64x32, later doubled into two
64 x 64 halves, 171 and 172) until 2026-10-21; Ryan wanted it twice that size again, which would take all
of OBJ VRAM, so it is map art now (`tools/shipart.py`, `docs/maps.md` section 9) and 171 / 172 are free.
Preview: `build/<dir>/paragus_preview.png` (every group, both portraits).

## Hercule's beam pose (group 18 of sprite 72, `tools/herculesprites.py`, 2026-10-19)

For the Cell Games beam struggle (`docs/story.md` 4h: Gohan and Hercule side by side firing at the
twin Perfect Cells) Hercule needs a Kamehameha pose; his stock record 72 (21 groups) leaves group 18
empty.

**How the engine plays a beam (decompiled).** Script step 137 `NPCFireKamehameha (char, frames)`
(`0x0800A3A2`; the struggle runs `NPCFireKamehameha 0, 140` on player 1, map 287 runs it on
Imperfect Cell 28) finds the entity showing sprite `char` (0 = player 1) and appends a 0x14-byte
command object (vtable `0x0802521C`: {vt, 0, beam, frames, frames - 8}) to it:

- start (slot 1, `0x0801D788`): creates the beam object (`0x0801D84C`, 0x50 bytes, the player
  Kamehameha's beam class `0x080143A8`) at the centre of the entity's box, adds it to the entity
  list, and calls the entity's `SetAnimation(30)` (vtable slot 10);
- update (slot 2, `0x0801D7DC`): counts `frames` down; at `frames - 8` (8 frames in) fires the beam
  (`0x08014478`) from the entity's position plus a per-facing offset (table `0x08701DEC + 4`: down
  0, 0; up 0, 0; **left -9, +2; right +10, 0**); at 0 ends it (`0x0801451C`); slot 3 = done when 0.

Animation 30 is the control state `0x03002268` (vtable `0x08025038`): enter (`0x0801CB10`) points
the entity's frame group at record + 0x16C = **group 18** and frame 0; its frame logic (`0x0801CB24`)
shows **frame 0 for 8 frames (the charge), then frames 1 and 2 alternating every 8 frames** for as
long as the state lasts. The beam clash earlier in the same scene plays group 18 too. Goku (60),
Gohan (53, 56, 59) and Goku SSJ (61) all hold 7 frames per facing in group 18 but only three
images: 0 the charge (hands cupped at the back hip), 1 the thrust, 2-6 one repeated hold image (the
hair blown up); 16x32 front / back at (0, -24), 32x32 sides at (-8, -24), right = the left blob
h-flipped (attr `0x90002000`).

**The record (`herc_record`).** Record 72's header and all 21 group pointers (ROM addresses, nothing
copied), with group 18 filled by the generator. The Makefile repoints **id 72** at it in the
relocated sprite index (`EXTRA_SPRITES` `72=herc_record`; `spriteclone.py --extra` now lets an id
below 157 replace a stock record), so everything that shows sprite 72 (Hercule as player 1 or 2 on
display row 21, his actors in cutscenes) gains the pose and is otherwise unchanged (checked: only
group 18 differs from the stock record in the built ROM). No new sprite id was taken (176-179 stay
free). Super Saiyan Hercule (157) and old Hercule (158) do not have the pose.

**The frames** (Goku's frame boxes, offsets and attributes; 9 unique frames, 2.4 KB LZ-compressed with
`buu2log2.compress`), all Hercule's own body and colours:

| facing | charge (0) | thrust (1) / hold (2-6) |
|---|---|---|
| left / right | his punch stance (group 7 frame 0: the same lunge and ground line as Goku's beam stance) with the chambered back arm and the front fist removed; Goku's charge arms pasted at the back hip | the same stance with Goku's beam arms (both straight out, open palms) pasted 3 px nearer the body |
| front | idle (group 0) with the hands at the sides removed; drawn: the far arm across the belly and both hands cupped at the hip on the viewer's left | drawn: elbows out in brown sleeves, both open palms over the belt |
| back | idle with the arms trimmed; Goku's back-view hands at the hip | arms out of sight (as Goku's) |

Pasted arms are recoloured: Goku's skin left of the wrist band becomes Hercule's skin ramp (11-14,
outline 17), the band and everything up to the shoulder his gi sleeve (6F / 70 / 71); edges opened
by the erase get the gi or skin outline. The hold image is the thrust with the afro bristling (a
ragged black fringe on every other column of its top and back edge, as Goku's hair rises). The front
arms are drawn because Goku's front beam arms are foreshortened skin over his chest that reads as a tan
block in Hercule's colours.

Preview: `build/<dir>/hercule_beam_preview.png` (and `_x4`): rows Goku 60, Gohan 59, Hercule's new
group 18, Hercule's base frames; four facings x (charge, thrust, hold). Engine proof
(`docs/testing.md` "Hercule's beam pose"): the stock struggle with Hercule as player 1 (the clash and
the Kamehameha, beam from his palms), the same run as Gohan side by side, right / down / up facings
by holding the facing byte while the beam fires, and player 2 (row 21) in the pose in all four facings.

## Haloed Goku (ids 176, 177; display rows 26, 27; `tools/halosprites.py`, 2026-10-19)

After the Cell Games Goku stays playable but dead, so he wears a halo (`docs/story.md` 4h "After the
Games"). Every LoG2 player record has the dead pose, group 17 (2 frames, front view in all four
facings): the character standing with a pale yellow ring (palette 0x24 / 0x25 / 0x2C) round the top of
the hair, its top arc one row above the hair, its front and side arcs over it; Goku's (60) ring is
13 x 5 px and identical in both frames (only the hair moves). Super Saiyan Goku (61) has no group 17.

**Buu's Fury has no haloed Goku**: its Goku records (ids 97-120; 105 / 107 base, 109 Super Saiyan,
110 / 111 Super Saiyan 3) wear no halo in any animation, and LoG2's dead pose has no Buu animation
(pose table above). Its only halos (found by scanning every frame of all 297 records for a top edge of
halo-coloured pixels) float clear of the head on NPCs: **30 Bubbles** and **125 Gregory** (King Kai's
monkey and cricket), an 11 x 3 ellipse with a gap above the head. Goku's frames have no room above the
hair (it reaches the frame's top rows), so the generator keeps LoG2's own hugging ring.

**The records.** `halo_goku_record` (id 176) = sprite 60 and `halo_ssj_record` (id 177) = sprite 61,
every group and frame decoded, haloed and LZ-compressed (`buu2log2.compress`), headers, frame boxes,
offsets and attributes unchanged (106 KB for both). Per frame the head is the hair: black, or Super
Saiyan yellow (0x2E / 0x78; Goku's own group 9, the transformation, turns gold half way), taken as the
8-connected hair component holding the topmost hair pixel; the ring goes on at its top row - 1 (row 0
at least) centred on the hair's top four rows, as in the stock dead pose. On yellow hair the ring is
white / 0x2C with a black line under the front arc. Group 17 keeps its stock halo. Preview:
`build/<dir>/halo_goku_preview.png` (`_x3`): every group x 4 facings, stock beside haloed.

**Frame counts** (fixed for every generator 2026-10-19, branch `sprite-frame-counts`). The engine
plays a fixed number of frames per pose (the detransform frames 0-3 of group 21, the tumble 0-3 of
group 20) and reads descriptors past a short array without a check, so a generated array must hold
every frame of its source. `sprites.frame_counts` used to guess an array's length from the gap to the
next array or blob, which has no bound for the record's highest-addressed array and cut it to 1 frame
(Goku's group 20 facing right, Super Saiyan Goku's group 21 facing down: 4 frames each, guessed 1);
the haloed Goku crashed on detransform until frames were counted by descriptor runs.
`Sprites.frame_counts` now counts the run of valid
descriptors (OBJ size 8/16/32/64 each way, blob a ROM pointer to a header of kind 0-2) up to the next
array start of these groups or of any stock record (ids 3..156, cached), at most 32. Checked on the
stock ROM: 100 of 2254 arrays change, every one upward (1 -> 2, 3, 4 or 7) and every one the
highest-addressed array of its record (mostly the last group facing right; group 21 / 22 facing down
where those are stored last), and with the run counter all four facings of each of the 898 non-empty
stock groups have the same count (with the gap guess they differed in 131);
`python tools\sprites.py roms\log2.gba --frame-diff` lists them. Generated records that changed:

| record | array | frames (was) | old build |
|---|---|---|---|
| Super Saiyan Hercule 157 (`spriteclone.py`) | 72 group 20 facing right | 4 (1) | overflow read the facing-left array placed after it by the linker: wrong frames, no crash |
| Paragus 170 (`brolysprites.py`) | 137 group 20 facing right | 4 (1) | overflow read blob bytes: party block overwritten, "Jumped to invalid address 0B026920" (r14 `0x08006C05`) |
| Android 13 165 (`bossmodels.py`) | 8 / 9 group 12 facing right | 4 (1) | read neighbouring descriptors: wrong frames |
| Namekian 166 (`bossmodels.py`) | 104 group 22 facing right | 2 (1) | read a neighbouring descriptor: wrong frame |

Unchanged output: `a16sprites.py`, `krillinsprites.py` and `herculesprites.py` (their copied groups
were not truncated; stock groups are referenced by ROM address), `halosprites.py` (it had its own run
counter, now the shared one) and `buu2log2.py` (template 72's facing-down counts were right; the
template layout takes facing-down counts for all four facings, which now always agree). The a16 and
Krillin previews now show every frame of their stock groups. Test: `docs/testing.md` "Frame counts".

**Rows and the swap.** Row 26 = Goku's row 19 with sprite 176, row 27 = Super Saiyan Goku's row 20
with sprite 177; they link to each other as 19 / 20 do, so Goku's Transformation abilities (11 / 12:
animation 0x25 transform, 0x24 detransform, both following the row's link) keep the halo.
`CharacterData_FindBySprite`'s limit is 28. A script makes Goku (slot 4) dead with
`SetCharacterTransformation 4, 176` (opcode 73 finds the row showing sprite 176) and alive again with
`4, 60`; refresh player 2's entity when he plays slot 4 (`old_hercule_set` in `krillin.c` shows how).
The duplicate-NPC rule (`actors.c`) treats sprites 60 / 61 as Goku's while he is on rows 26 / 27.
Seen headless (`docs/testing.md` "Haloed Goku"): walking in four facings, the punch / kick combo and
hit-stun with the halo, the transformation (halo through the gold-hair frames) to row 27, Super Saiyan
walking and punching, and back to row 26.

## Super Saiyan 2 Gohan (record 58 repointed, display row 28; `tools/ssj2sprites.py`, 2026-10-19)

The stock game shows sprite 58 only as player 1 in the last two Cell Games fights (`SetCharacterTransformation
0, 58` -> row 4). Its record has 19 groups with 9, 10, 11, 12, 14, 15 and 17 empty: no hurt pose (11), no
lying pose (12, the knock-out) and no tumble (20, past the group count). Super Saiyan teen Gohan (59) wears
the same purple gi on the same body and frame boxes, so `ssj2_record` (`build/gen/ssj2_sprites_data.c`) is
record 58 with its own header (collision boxes) and group count 21, groups 0-8, 13, 16, 18 by ROM address,
11, 12 and 20 from record 59 by ROM address (Super Saiyan 1 hair on those frames; they show for a few
frames), and 9, 10, 14, 15, 17, 19 empty as in Super Saiyan Goku's 61 (no transformation, Spirit Bomb,
fidget or halo poses). Nothing is copied or drawn. The Makefile repoints id 58 (`EXTRA_SPRITES`
`58=ssj2_record`), so the stock Cell Games scenes get the filled groups too. Preview
`build/<dir>/ssj2_gohan_preview.png` (first frame of every group per facing, ssj2_record above 59).

Portraits: the stock ones fit. Dialogue / switch-screen portrait **61** and status portrait **12** (both
on the stock row 4) are Super Saiyan 2 Gohan; row 28 uses them (portraits 124-125 were not needed).
Seen in play (headless and fork): walk in four facings, the A combo, the Kamehameha (group 18), Masenko,
the ki pose (Burst, Ki Blast), hurt frames from 59.
## Super Android 13 (id 180, portrait 122; `tools/super13sprites.py`, 2026-10-19)

The post-game boss (`docs/story.md` 4i): blue skin, white hair, huge muscles, red-brown trousers, much
larger than a normal fighter. Neither ROM has him (`docs/sprites.md` "Buu's Fury": no Super 13), so the
body is **Legendary Super Saiyan Broly from Buu's Fury (Buu record 28)**: a 64 px tall muscle giant (a LoG2
fighter is about 32 px; Perfect Cell 30 about 40), frames 32x64 / 64x64, with a full fight set and the
cutscene poses his specials need. `tools/super13sprites.py` converts it with `buu2log2`'s converter
(`S13Converter` overrides only the colour step) into **sprite id 180** (`s13_record`, Makefile
`S13_SPRITE`), preview `build/<dir>/super13_preview.png` (and `_x2`: every group x 4 facings x every frame,
the orbs and both portraits).

**Recolour per frame, by body part from geometry** (Buu indices after Broly's own remap table):

| part | how it is found | Super 13 |
|---|---|---|
| skin | Broly's tan ramp (200-205, 16, 18, 66, 67) | blue ramp (LoG2 184 / 186 / 189 / 187 / 76 / 193 / 192) |
| hair | yellow components (28, 110, 5, 29, 30 with their white / orange shading) whose centroid is above the skin's | white and greys (2, 9-12); the skin-ramp strands inside it (pixels with 9+ hair pixels in their 5x5 window, grown three times) grey, its dark outline 14 |
| trousers | yellow components below the skin's centroid of 30+ pixels: the pixels with no transparent 4-neighbour and their 8-neighbours | red-brown (117 / 110 / 111 / 112); the rest of such a component is a swing arc touching the leg: white |
| necklace, arm cuffs | small yellow components below the skin's centroid; silver and gem pixels above the trousers' centroid | blue skin (bare chest and forearms) |
| sash | the largest red component and any red within 6 rows of it | charcoal belt (14 / 15 / 1) |
| boots | silver and gem pixels below the trousers' centroid; red below them (soles) | black boots |
| light, sparks, fire, mouth | anything else (and in the death frames, yellow patches above the belt: light bursting out) | nearest LoG2 colour; Broly's brown swing arcs (210 / 211) light grey |

**Groups.** Hercule's 21-group template layout (as `buu2log2`'s Broly 159), so the melee boss AI set 21
finds idle 0, walk 1, hit-stun 2, charge 5 and attacks 6-8, plus the poses `src/coop/super13.c` plays itself
(`S13_G_*` in `super13.h`); 27 groups:

| group | Buu animation (frames) | used for |
|---|---|---|
| 0 idle, 1 walk | 0 (2), 1 (4) | AI set 21 |
| 2 hit-stun, 3 knock-back, 11 hurt | 9 (shout with shock lines, recoil, crouch) | the stock hurt states |
| 5 charge, 6-8 attacks, 4 / 13 / 14 / 16 / 17 / 18 / 19 / 20 | `buu2log2.GROUP_MAP`, cycled to the template's frame counts | AI set 21, unused fillers |
| 12 | 17 (8): light bursting out of him | the death (S13_G_DEATH) |
| 21 | 3 (3): wind-up, thrust, thrust | ki volley / fan, the beam's release (S13_G_KI) |
| 22 | 18 frames 0-1 (hunch, arms up), 3 frame 1 (throw) | deadly bomber (S13_G_BOMB) |
| 23 | 20 (6): crouching power-up, fire at his feet | ki wave charge (S13_G_POWER) |
| 24 | 21 (6): the roar with flames | the rage roar (S13_G_ROAR) |
| 25 | 19 (8): spinning into a ki shell and out | ki nova (S13_G_NOVA) |
| 26 | an empty 8x8 frame | after the explosion (S13_G_BLANK) |

Buu animations 17-21 draw one front view for every facing. Header: Hercule's template with the group count
27, the largest frame 0x1000 (64x64) and byte 3 = 0x18 (`docs/sprites.md` "Paragus": the entity's OBJ VRAM
slot is sized from it) and **one hurt box for every facing, (-6, -46, 38, 16)** around his position
(`Entity_GetBox` 0x080079B8 adds the position to `record + 0xC + facing * 0x10`): Broly's side-facing boxes
are only 29 px wide, starting 3 px right of the position, so fists landing on his drawn body missed (1 hit
in 33 landed, counted headless, until the box was widened). 80 frame blobs, about 100 KB LZ-compressed.

**Without `roms/buu.gba` (`BUU=0`)** the record aliases Perfect Cell's stock groups (sprite 30, which has
0-3, 5-8, 12, 18) with Cell's groups standing in for the special poses: the build links and the AI runs,
but he looks like Cell.

**Ki orbs** (raw 8-bit tiles, the ROM projectile class's format as `a16sprites.py`): red-pink orbs drawn
procedurally (white core, pink, red, dark red, a flickering spiky rim): `s13_orb_s` 16x16 x 3 frames,
`s13_orb_m` 32x32 x 2, `s13_orb_l` 64x64 x 2 (the deadly bomber over his head), `s13_trail` 16x16 x 2 (the
ki wave's trail). His explosions reuse `a16_boom` (Android 16's fire, `a16_sprites_data.c`).

**Portrait 122** (`s13_portrait`, through `krillinsprites.py --extra-portrait 122=s13_portrait`, Makefile
`S13_PORTRAIT`): Android 16's dialogue portrait (Portrait_Table 26, the square-jawed android face) with the
skin blue, the orange hair white and grey (its outline dark grey), the green armour bare blue shoulders, the
earring gone and the eyes red. Dialogue: `text 122 "SUPER 13: ..."` (speakers >= 8 read
`portrait_module[id + 1]`). Base Android 13 still has no portrait (123 is allocated for it, unused).

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
