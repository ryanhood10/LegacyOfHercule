# Sprite inventory (tools/sprites.py)

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
