# Maps: format, tools, and adding new maps

Everything a Legacy of Goku II map needs, decoded from the ROM (Ghidra corpus `build/decomp_all/`,
checked against savestates and by rendering every map), and the tools that read and write it:
`tools/maplib.py` (format library), `tools/mapexport.py` (render any map from the ROM),
`tools/mapbuild.py` (build new maps from `data/maps/*.json`), `tools/patch_rom.py --maps` (place
them and relocate the map table). The first new maps (zone 15) are listed at the end.

Addresses are GBA addresses (file offset + 0x08000000). "Resource" means the loader format of
`Resource_LoadOrDecompress` 0x0801F394: `{u32 mode, u32 size}` then data; mode 0 = stored, 1/2 =
compressed with the IWRAM decompressor (`tools/mapchunks.py` decodes, `tools/buu2log2.py compress`
encodes; the stream sets its own literal width). The Arefu/Legacy notes (facts only) agreed with
everything below that they cover; where they differ, the ROM wins and the difference is noted.

## 1. Loading a map

`Map_LoadInternal` 0x0800C390 (state, zone, area, spawn*, variation, flag), in order:

1. `Map_FindEntry` 0x0800EB7C: linear scan of the MapEntry table (`Map_GetCount` 0x0800E0AC returns
   the constant 327) for `(zone, area)`; not found = fatal error screen.
2. Variation: the warp's variation, or (0) the entry's variation script (+0x28) result, or 1.
   `FUN_0800EB4C` returns `variations[v - 1]`.
3. `FUN_080040EE`: marks (zone, area) visited in the area-visit table (unknown maps are skipped).
4. `Map_InitRenderer` 0x08006152 with the variation (section 3).
5. Level gates (+0x20, count +7), items (+0x18, count +5), objects/chests (+0x1C, count +6), each
   gated by its condition word; triggers (+0x10, count +3) and the variation's exits (`FUN_0800C2F8`,
   `FUN_08006264`); music (+0x24); map scripts (+0x14, count +4: NPCs, actors, enemies).
6. The previous map's on-leave script (+0x34), then this map's on-load script (+0x30).

The camera starts at spawn - (112, 76) (`FUN_0800C584`). Warps: script opcodes 38/39
(`WarpToMapWithEntities` / `NoEntities`) pop `y, x, variation, area, zone`; exits call the loader
directly. `docs/coop-design.md` has the co-op test warp (`coop2.cmd = 5`).

**Condition word** (`FUN_08003F06`, used by triggers, items, objects, gates' neighbours, map
scripts): 0 = always; `0x1nnnn` = story flag n set; `0x2nnnn` = flag n clear; `0x3nnnn` = flag n
set, and clear it (once); anything else = pointer to a bytecode script whose result decides.

## 2. MapEntry (0x08409368, 327 x 0x38)

| off | field |
|---|---|
| +0x00 | u8 zone, +1 u8 area, +2 u8 variation count |
| +0x03 | u8 trigger count, +4 map-script count, +5 item count, +6 object (chest) count, +7 level-gate count |
| +0x08 | u32 flags (1 on field maps, 0 in towns) |
| +0x0C | map name id (< 0x10000: index into the name table 0x080E2840; 0xDD = no banner) or a script returning it |
| +0x10 | triggers: array of `{u32 condition, trigger data*}` |
| +0x14 | map scripts: array of record pointers (handler 0x0800B711 NPC, 0x0800D6DF actor, 0x0800E77F enemy; `tools/script_dump.py`) |
| +0x18 | items*, +0x1C objects (chests: condition, item id, x, y, ...)*, +0x20 level gates* (8 bytes each) |
| +0x24 | music id, or a script returning it |
| +0x28 | variation script (picks the variation when a warp passes 0) |
| +0x2C | variations: array of `Map_VariationEntry*`, index variation - 1 |
| +0x30 | on-load script, +0x34 on-leave script |

Maps with the same (zone, area) exist only as variations. The table address is held by four code
literals (0x08005ADC debug menu, 0x08005D5C debug Map Test, 0x0800EBD8 `Map_FindEntry`, and
0x0800195C = `&table[3]`, the new-game map); a whole-ROM scan found no other pointer into it. The
count is `movs r0,#0xFF; adds r0,#0x48` at 0x0800E0AC (max 510 with that encoding). Zones 1-14 are
used; **zone 13 is "Snowy Highlands"** (10 maps of snow, falling-snow overlay), which `docs/story.md`
had missed.

## 3. Map_VariationEntry (0x50 bytes): the map itself

| off | field |
|---|---|
| +0x00 | u32 blend: BLDCNT \| EVA << 16 \| EVB << 24 |
| +0x04 | u8[4] BG0..BG3 priority (added to the BGnCNT base values) |
| +0x08 | u16 width, +0x0A u16 height: the camera's scroll range; the map is (w + 239) x (h + 159) px |
| +0x0C | animated tile sequence count, +0x10 array of sequence pointers (section 5) |
| +0x14..+0x20 | BG0..BG3 layer descriptors (section 4) |
| +0x24 | decoration count, +0x28 decorations (0x18 each), +0x2C decoration sprite sheet (resource) |
| +0x30 | s16 x, y: default spawn |
| +0x34 | walk collision bitmap (resource, 8 KB), +0x38 projectile mask (resource, 8 KB) |
| +0x3C | affine flag: 1 only on the world map (zone 6 area 99, BG2 rotate/scale; its layers are other classes) |
| +0x40 | exit count, +0x44 array of exit trigger-data pointers (section 7) |
| +0x48 | tileset slot table (resource), +0x4C tile bank table (0x084DF574 for every stock map) |

`Map_InitRenderer` 0x08006152 loads the two bitmaps into 8 KB buffers (renderer +0x6C walk, +0x70
projectile, +0x74 a working copy of walk), sets the camera range, creates decorations (`FUN_08005FBA`),
then `FUN_08005F1A`: animation list, the four layer objects (each descriptor's first word is its
create function), the static tileset upload, BGnCNT = table[affine] + priority, BLDCNT/BLDALPHA.

## 4. Layers

**Tiled layer** (create fn 0x080056A1; Buu's Fury 0x08008087 with the identical format):

| off | field |
|---|---|
| +0x04 / +0x08 | s32 scale x / y in 1/1024 (1024 = scrolls with the camera; 512 = half-speed parallax, e.g. the Lookout's clouds) |
| +0x0C / +0x10 | s32 offset x / y in 1/1024 px (layer pixel = world pixel * scale + offset) |
| +0x14 / +0x15 | u8 chunk columns / rows |
| +0x18 | chunk pointer grid, rows x columns; 0 = empty chunk |

A chunk is a resource that unpacks to 32 x 32 GBA text-BG entries (2 KB, 256 x 256 px): bits 0-9 the
VRAM tile slot, bit 10 h-flip, bit 11 v-flip (all BGs are 256-colour, charBase 0, so there are no
palette bits). The renderer keeps a 2 x 2 window of chunks decompressed per layer and DMAs the
256 x 256 window to VRAM (`docs/widescreen.md` "Map streaming"). Slot 0 is blank in every map; empty
cells are entry 0.

**Effect layer** (create fn 0x08004FCD, 49 layers, mostly BG0): an overlay that wraps: +0x04/+0x08
scale, +0x0C/+0x10 offset, +0x14/+0x18 s32 velocity per frame (1/1024 px), +0x1C one 32 x 32 entry
map (resource) DMA'd once to the BG's screen block (`FUN_08004F32`; scroll each frame in
`FUN_08004F76`). Zone 13's BG0 is falling snow drawn with slots 880-895 (an animated sequence of 16
tiles); others are rain and fog. Three other layer classes exist only on the world map.

## 5. Tiles and palettes

- **Palette:** one 256-colour BG palette for every map, 0x081DA4C8 (checked: palette RAM in
  savestates of different maps equals it); OBJ palette 0x081DA6C8. A map cannot have its own BG
  colours, so a recolour means new tiles whose pixel indices point at other entries of the same
  palette (the palette is a broad 256-colour spread: whites, ice blues, greys, every hue).
- **Tile banks:** the bank table (variation +0x4C, 0x084DF574, 168 banks) points at resources of up
  to 256 8bpp tiles (64 bytes each).
- **Slot table** (+0x48, `MapRenderer_UploadStaticTileset` 0x080050C8): u16 deltas, atlas[0] = d[0],
  atlas[i] = atlas[i - 1] + d[i] + 1 (16-bit). Slot i (VRAM 0x06000000 + 64 i) gets atlas tile
  atlas[i]: bank = high byte, tile = low byte. At most 896 slots (VRAM below the screen blocks at
  0x0600E000; the loader's stack buffer is 896 entries). Verified: a decoded table equals VRAM tile
  for tile in a savestate (363 of 363).
- **Animated tiles** (+0x0C/+0x10): sequence = u8 frame count, u8 tiles per frame, u16 first slot,
  then a pointer per frame; a frame is a u32 (packed size) followed by a compressed stream, uploaded
  over its slots every few frames (`FUN_0800501E`). Animated slots may lie beyond the static table.
- **Decorations** (+0x24/+0x28/+0x2C): 0x18-byte records `{s32 x1, y1, x2, y2, u16 0x2000, u16
  0x8000, u16 frame offset, u8 0x28, u8 1}`, OBJ graphics from the variation's sprite sheet (+0x2C,
  a resource of 4bpp frames). The barriers ("only Vegeta may pass") are decorations.

## 6. Collision

Both bitmaps are 256 x 256 tiles (2048 x 2048 px), one bit per 8 x 8 tile, eight u32 per tile row:
`word[8 * ty + (tx >> 5)] & (1 << (tx & 31))`; **set = passable**. `Collision_CheckRect` 0x08005D6C
tests a box against a bitmap and fails at the first clear bit, or when the box leaves 0..2048.

- Players test the walk bitmap (+0x34, renderer +0x6C: `FUN_08006366` / `FUN_0800638C`, called from
  the movement and dash states with the entity's box at +0x154).
- Enemies and actors test the working copy (+0x74, `FUN_080063B6` / `FUN_0800E438`).
- Projectiles (ki blasts, rockets) test the projectile mask (+0x38 -> +0x70, e.g. `FUN_08013FDA`
  destroys a blast at the first clear bit). Many maps point +0x38 at one shared all-open bitmap
  (0x084F4444).
- Scripts edit them at run time: opcode 71 `SetCollisionRect` / 72 `ClearCollisionRect` set / clear
  a rect in walk and the enemy copy; 117 `RemoveRectFromMask2` / 118 `AddRectToMask2` edit the
  projectile mask. That is how barriers and opened doors change collision.
- There is no height map: elevation (cliffs, plateaus) is drawn art plus blocked tiles.

## 7. Triggers, exits, gates

**Trigger data** (pointed to by the MapEntry trigger list and by the variation's exit list): +0
create function, +4 activation object getter (returns a static object in IWRAM that decides how the
trigger fires), +8 s16 rect x1, y1, x2, y2 (px), +0x10 payload. Create functions seen:

| create | count | what | payload |
|---|---|---|---|
| 0x0800D7E1 | 153 | dialogue | sequence pointer |
| 0x0800C2C3 | 179 | bytecode script | script pointer |
| 0x0800C233 | 51 (+ exits) | door exit: fires while the player presses the direction | see below |
| 0x0800D9C3 | 18 (+ exits) | walk-off exit at a map edge | see below |
| 0x08010AC7 | 30 | save / switch-characters pad | (unused) |
| 0x0801A6B3 | 14 | world-map landing name banner (world map only) | name id |

Activation objects: 0x0800D8AF exits (the held direction), 0x0800D839 "press A inside the rect"
(flight spots, save pads; verified: standing on the arctic's flight spot does nothing until A),
0x0800D765 dialogue on entering, 0x0800D87F dialogue at a barrier, 0x0801087B/0x0801087F other
script triggers.

**Exit payload** (+0x10, 8 bytes; trigger data is 0x18 bytes): u8 direction (low nibble 0 down,
1 up, 2 left, 3 right: the key that must be held, from the table 0x083ED1B4; high nibble = facing on
arrival + 1, 0 keeps it), u8 zone, u8 area, u8 variation, s16 x, s16 y (spawn in the target map).
Most exits live in the variation's exit list (+0x40/+0x44, always active); some are in the trigger
list behind a condition (story-dependent exits).

**Flight spot to the world map:** a script trigger (create 0x0800C2C3, activation 0x0800D839) whose
script is `PushByte 43; PlayMusic; PushMultiVarint [6, 99, 1, x, y]; WarpToMapWithEntities` (x, y =
where the flyer appears on the world map, e.g. (1632, 1592) at Gohan's house). There is no sign
graphic in the map data; the spot is a plain rect. The world map (zone 6 area 99, affine) has a
pair of triggers per landing site: a script trigger warping to the town and a 0x0801A6B3 banner.

**Level gates** (+0x20, 8 bytes): u16 x, u16 y (where the number is drawn), u8 barrier (the
decoration it labels, found by message 6), u8 colour, u8 level, u8 character (party slot; 255 = any).
Colour and character pairs in the stock maps: 6 Gohan (0), 4 Piccolo (1), 3 Vegeta (2), 7 Trunks (3),
39 Goku (4), 5 Hercule (5), 2 anyone (255).

**A working barrier** (zone 8 area 28, the "Only Vegeta" cave) is five pieces: the decoration (the
force field, id 0, its rect blocked in the walk collision); the gate (the number); an **item**
(MapEntry +0x18, count +5) `{u32 condition, u8 decoration id, u8 1, u8 3, u8 0, handler 0x0800F9A3,
script}` whose condition (a script: active character == 2 and level of character 2 >= 50) is tested
**when the map loads**, and whose script runs when that player presses A at the decoration (play
sound 47, `SetCollisionRect` + `RemoveRectFromMask2` over the rect, `SetStoryFlag 280`, and the
decoration disappears); a dialogue trigger "Only Vegeta is allowed to pass through this barrier."
whose condition is "active character != 2 and flag 280 clear"; and an on-load script that, once
flag 280 is set, hides decoration 0 and opens the rect again. So the right character must be the
active one when entering the map. `mapbuild`'s `barrier` block generates the item, the conditions
and the on-load script for any character, level and flag. Items in general: `FUN_0800F034` attaches
the record to the decoration found by id (message 6); decorations answer message 4 (show / hide,
argument id << 1 | visible) and 6 (find by id).

## 8. Tools

```
python tools/mapexport.py --list                         every map: index, zone, area, variations, size, name
python tools/mapexport.py 8 28 [--var V] [--grid]        build/maps/z8a28v1*.png: per layer, composite, walk / projectile overlays, .txt summary
python tools/mapexport.py --all --out build/maps/all     all 335 variations (about 25 minutes)
python tools/mapexport.py --tileset 8 28                 the VRAM tile slots of a map, numbered
python tools/mapexport.py 15 1 --rom build/maps/log2-wide.gba --table 0x08D05C80 --count 330   a built ROM's new maps
python tools/mapbuild.py build data/maps/*.json OUT.json [--free 0xD00000] [--preview DIR]
python tools/mapbuild.py preview data/maps/arctic.json --out build/maps/preview
```

`mapbuild.py` spec reference (the module docstring has the full list; coordinates are 8-px tiles
for layout and collision, pixels for objects):

- `base` [zone, area]: header defaults (priorities, blend, decoration sheet) and, with `"layout":
  "base"`, the starting layers, collision and the objects listed in `keep` (decorations, gates,
  triggers, exits), which follow every insert/crop.
- `tileset`: `"base"` keeps the base map's slot table (copies stay valid, animations keep working);
  `"fresh"` starts empty and adds only the tiles the layers use, from any map (slots are translated
  through the stock atlas, so tiles of different maps mix freely up to 896).
- `ops`, in order: `insert_cols` / `insert_rows` (repeat a band of the whole map: layers, collision,
  objects), `crop`, `stamp` (copy a rect of another map's layers and collision; `layers` /
  `to_layers` route BGs, `flip` "h"/"v"), per-layer `fill` (tile a source block over a rect),
  `copy`, `clear`, `solid` / `texture` (generated tiles in palette colours: our art, no ROM data),
  `png` (an image cut into tiles, matched against the tileset with flips, new tiles otherwise), and
  collision `walk` / `block` (rects), `walk_ellipse` / `block_ellipse`, `border`, `from_layer`;
  `"mask": "proj"` or `"both"` picks the bitmap.
- `recolor`: rules by hue range, saturation and luminance mapping each palette colour onto a colour
  ramp, quantised to the nearest palette entry; every tile (and animated frame) becomes a new tile in
  new banks appended to a copy of the bank table.
- `effect` {from: [zone, area]}: that map's BG0 overlay, its animated tiles and blend; the base
  map's own animations move out of the way when their slots collide.
- `triggers` (flight / warp / script), `exits` (edge / door), `gates`, `onload` (the barrier
  pattern with a flag), `spawn`, `name_id`, `music`, `flags`, `priorities`.
- Foreign maps: a `src.map` of `{"rom": "buu", "variation": "0x0822229C"}` reads a Buu's Fury
  variation record (`--buu-rom`, default `roms/buu.gba`); its tiles are converted to LoG2's palette
  (nearest colour) as new tiles. A spec with `"requires": "buu"` is skipped when that ROM is absent
  (the Makefile passes it when `BUU=1`), so builds without `roms/buu.gba` still work.

Output: one JSON with `records` (file offset + bytes, all in free ROM from `--free`), `rows` (one
MapEntry per map), `table_offset` (room for 327 + n rows). `patch_rom.py --maps` writes the records
(refusing anything that is not 0xFF padding), then copies the 327 stock rows **as patched so far**
(after `--data-patch`, so map-record edits at the original offsets, e.g. the story session's
`tools/mapedit.py`, are kept) to the new table with the new rows appended, repoints the four
literals and sets the count. The old table stays in place. `make` runs it by default (`MAPS=1`,
`MAP_FREE=0xD00000`; free-ROM use: 0x800000 code, 0x900000 Hercule data, 0xA00000 party tables,
0xB00000 story scripts, 0xD00000 maps). Previews land in `$(BUILD)/maps/`.

## 9. The new maps (zone 15)

| zone / area | spec | what | size | spawn |
|---|---|---|---|---|
| 15 / 1 | `data/maps/arctic.json` | Arctic: zone 8 area 28 widened and deepened into a snowfield, recoloured to snow and blue-grey ice, Snowy Highlands' falling snow; ice cliff wall with the cave mouth behind the stock **Vegeta barrier** (level 50), a frozen pond, two outcrops, a flight spot | 432 x 368 | (216, 200) |
| 15 / 2 | `data/maps/time_chamber.json` | Hyperbolic Time Chamber: white void floor (generated tiles), the living quarters at the top edge (the Lookout palace facade, dome and entrance canopy), a door exit under the canopy back to the Lookout | 640 x 480 | (312, 280) |
| 15 / 3 | `data/maps/broly_arena.json` | Broly's arena: the walled basin of zone 3 area 6 (Northern Wastelands) as the arena floor, the pod crater on the rim above as scenery, two boulders; only the basin is walkable; a flight spot | 640 x 544 | (276, 354) |
| 15 / 4 | `data/maps/broly_crater.json` | Optional (built when `roms/buu.gba` is present): Buu's Fury's walled wasteland crater (variation 0x0822229C there) imported whole, tiles converted to LoG2's palette, its own collision | 480 x 400 | (240, 340) |

Reaching them from story scripts: `PushMultiVarint [15, AREA, 1, x, y]` then `Step 38
WarpToMapWithEntities` (or 39 without entities), from a trigger script, a dialogue entry or an
exit record `{dir, 15, AREA, 1, x, y}`; the debug Map Test lists zone 15 too. The arctic's cave door
(rect (336, 0)-(353, 57) after the stretch) has no exit yet: Android 13's interior or encounter
attaches there. The arctic's barrier is a working copy of the stock one with its own story flag
**380** (the highest stock flag is 306; flags 0-399 are cleared at a new game): Vegeta (party slot
2) at level 50 or more, active when entering the map, presses A at it to open it for good.

**Verified in the WSL fork** (slot 4, `make BUILD=build/maps3 WIDE=1 DEBUG_MENU=1`, 2026-10-18):
the debug Map Test lists zone 15 and loads area 1; the co-op test warp (the WarpToMap opcode handler)
reaches areas 1 to 4; in each, both players walk and stop at the collision (the arctic's ice cliff,
outcrops and the Vegeta barrier, which shows its blue 50; the Time Chamber's building, canopy and
border; the arena's basin walls, rim and boulders; the imported crater's walls); the wide view
shows each map; with a level-1 test copy of the arctic, Vegeta active and pressing A at the barrier
opens it (it vanishes, flag 380 set, he walks into the cave) and it stays open after re-entering;
the Time Chamber's
door under the canopy leads to Kami's Lookout; the arctic's flight spot (press A) flies to the world
map. Screenshots: `build/maps/shots/` (`zone15_sheet.png`), not committed (ROM graphics).

## 10. Buu's Fury maps (decoded; one imported)

Buu's Fury uses the same map engine: the tiled layer header (create fn 0x08008087, 1374 layers), the
0x50-byte variation record (identical field layout: blend, priorities, size, animations, four layers,
decorations, spawn, collision, projectile mask, affine, exits, slot table, bank table) and the same
resource/decompressor format; its maps render with `maplib` unchanged given its BG palette
(0x0805632C, 0x200 below the OBJ palette 0x0805652C) and its own bank table (per variation +0x4C).
All 401 variation records found by scanning (records whose layer slots point at tiled-layer headers)
render: contact sheets come from rendering each record (a scratch script over `maplib`; worth turning
into `mapexport --buu` if more imports are wanted). What is not found: its MapEntry table (no
0x38-stride table with LoG2's shape; its map names are not plain text), so a record cannot be tied to
a zone/area, a name or the script that stages a fight; Broly's own battle map is therefore not
identified. Candidates by look: 0x0822229C (a big crater inside walled wasteland, imported as zone
15 area 4) and 0x0824F55C (a crater in sand). Also of note for the story: 0x082546A0 / 0x08254DA0
(round rooms with a table and a bed, likely the Time Chamber's living quarters inside) and
0x08255C9C / 0x08256F34 (dark rooms with light cones). Importing any of them is one stamp op
(section 8); what it does not bring: decorations (their sprite sheets live in the Buu ROM),
animated tiles (frame 0 only), exits and scripts. Finding the map table (through Buu's
`Map_FindEntry`, the routine that scans for zone and area) would name the records.

## Open

- Story flags taken by the maps: 380 (the arctic barrier). Keep new map flags in 380-399 and tell the
  story session. The story's flags are 360-379 (376: Android 13 met in the arctic).
- **Story content in a built map (story-scripts, 2026-10-18):** `tools/mapedit.py` edits the stock map
  table before the new maps exist, so a zone-15 map carries its story itself: `"sources"` (lscript
  files), `"triggers"` of type `"dialogue"` (`payload` seq:NAME, `condition` 0 / "flag:N" / "noflag:N" /
  "script:NAME"), and `"records"` (actors, and enemies copied from a stock record's behaviours with
  `"like": [zone, area, record]`, `"action"` seq:NAME). mapbuild assembles the referenced blocks into
  the map's free space (`StoryRefs`). The arctic uses it for Android 13 (`data/story/ch8_arctic.lscript`).
- The activation objects (0x0300209x) are named by use, not decoded.
- Decoration records beyond the barrier pattern, item and chest record layouts: not needed yet.
