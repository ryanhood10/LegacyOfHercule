# Sprite inventory (tools/sprites.py)

`python tools\sprites.py roms\log2.gba build\sprites` writes `build/sprites/atlas.png` (every sprite
id, first frame facing down/up/left/right), `build/sprites/<id>.png` (every pose group x 4
directions) and `build/sprites/report.md` (groups, frames, sizes). Sprites are 8-bit OBJ tiles,
frames compressed with the game's decompressor, one shared 256-colour OBJ palette at
`0x081DA6C8`; sprite ids 3..156 index `g_CharacterSpriteIndex` at `0x083B4E74`.

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

## Super Saiyan Hercule (plan)

Hercule's sprite 72 recoloured: hair indices -> the yellow indices that SSJ Goku (61) uses, skin and
gi unchanged, as a NEW sprite id (clone of record 72 with re-encoded frames) and a NEW display row
flagged as a transformed form that reverts to row 21, reached by a transformation ability. All
frames are literal-encodable with the decompressor's literal op, so re-encoding is simple.
