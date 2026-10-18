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
| 15, 16-19 | 1-3 | Androids 17 / 16 | enemy-grade (attack, hurt) |
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
