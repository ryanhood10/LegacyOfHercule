# Dynamic wide view ("zoom out" as the players separate)

Goal (Ryan, 2026-10-17): the view stays the normal 240 x 160 while the two players are close and
widens up to 2-3x as they walk apart, shrinking back when they rejoin, so people on big monitors
can roam. The GBA draws exactly 240 x 160 and no emulator option enlarges that, so this is a
co-design: a modified mGBA renderer plus ROM-side changes. Findings from mGBA 0.10 source
(`tools/downloads/mgba`, MPL-2.0) and the running game.

## What the game gives the hardware

| Fact | Value | Consequence |
|---|---|---|
| Video mode | 0, BG0-3 text layers, OBJ 1D mapping (`DISPCNT=0x1F40`) | tile backgrounds, no affine scaling |
| Tilemap size, all four BGs | 256 x 256 px (`BGnCNT` size 0) | only ~16 px of map beyond the 240 visible exists in VRAM |
| OBJ coordinates | 9-bit x (0-511, >=256 means negative), 8-bit y | a sprite cannot be placed past 255 px down or past ~240 right without wrapping |
| Culling | per-entity screen box test against the camera cell's view rect (`0x08007A84`) | entities outside 240 x 160 emit no OBJ |

So the emulator cannot "render more map": the map outside the 256-px tilemap ring is not in VRAM.
The game streams tile columns/rows into that ring as the camera moves (streaming routine still to
be located; the map loader `0x0800C584` and the camera `Camera_Apply 0x080062E8` are the entry
points).

## Design that works within those limits

1. **Extended tilemaps in EWRAM.** The ROM, when the wide view is active, writes the tilemap
   entries of the whole wide area (up to 720 x 480 px = 90 x 60 tiles per BG) into an EWRAM buffer
   per BG using the game's own map-to-tile routine (reimplemented in C once found). VRAM keeps the
   normal 256 x 256 maps for the stock view. The tile *graphics* stay in VRAM (the game keeps every
   tile of the current map resident: the tilemap entries index them).
2. **Custom renderer.** mGBA's software renderer (`src/gba/renderers/video-software.c`,
   `software-mode0.c`) draws one scanline at a time into `row[240]` with `start/end` windows; the
   mode-0 fetch computes `(start + BGx) & 0x1FF` into the VRAM tilemap. The patch: a runtime frame
   size (W, H) read each frame from a fixed EWRAM word the ROM owns (`coop.view_w/view_h`, default
   240 x 160); row buffers sized for 720; mode-0 fetch from the EWRAM extended tilemap when W > 240;
   rows 160..H-1 drawn in `finishFrame` with the last scanline's register state (per-line raster
   effects are not used by this game's maps); `desiredVideoDimensions` reports the maximum so the
   SDL texture is allocated once, and the frontend scales the (W, H) sub-rectangle to the window,
   which is what makes the picture "zoom out" while the window stays the same size.
3. **OBJ beyond 8/9 bits.** For non-affine sprites OAM attr1 bits 9-11 are unused (they are the
   affine matrix index only when attr0 bit 8 is set). In wide mode the renderer reads attr1 bit 9 as
   x bit 9 (0-1023) and bit 10 as y bit 8 (0-511), and treats x as unsigned up to W + 64 (sprites
   partly off the left edge use the top of the range as negative). The ROM's OAM writer gets a hook
   that stores those bits from the entities' full screen coordinates (`+0x40/+0x44`), after the
   cull rect in the camera cell has been widened to (W, H).
4. **ROM camera.** Midpoint camera as today, but the view size is dynamic: W = clamp(240 .. 720)
   from the players' separation plus a margin, H = W * 2/3, smoothed so it grows and shrinks over a
   second or two; the leash uses the wide rect; HUD panels stay native-sized at the corners of the
   wide view (OBJ x up to W-88 via the extended bits); `Camera_Apply`'s map clamp is replaced for the
   wide size.
5. **Netplay/lockstep** is unaffected (the view size is derived from deterministic game state).

Risks: the tilemap generation (1) needs the map format and streaming routine reverse-engineered,
which is the largest unknown; frame cost at 720 x 480 is 9x the pixels (the software renderer is
fast; fine on this laptop); HBlank/raster effects, if any map uses them, would be lost in the extra
rows; mGBA savestates and the Lua bridge keep working since the core is unchanged.

## Map streaming, decoded (2026-10-17)

- Each background layer object (`+4` BG index, `+8` map header pointer, `+0xC..+0x18` four chunk
  pointers, `+0x1C..+0x1F` four cached chunk ids, `+0x20/+0x24` current chunk x/y, `+0x28` four
  2 KB chunk caches) is updated per frame by `0x080055A4` (BG0/1-style layers) or `0x0800561A`:
  scroll = layer_scale * world_pos + world_px + layer_offset, in 1/1024 px; the top-left margin is
  (-8, -48) px; chunk coordinates are scroll >> 8.
- `0x0800532E(layer, chunkXY)` keeps a 2 x 2 window of **256 x 256 px chunks** decompressed: the map
  header has width/height in chunks at `+0x14/+0x15` and a chunk pointer table at `+0x18`
  (`0xFF` = empty chunk, `DAT_08005588` = blank map); chunk data is unpacked with `0x0801F394`
  (header word 0: 0 = plain copy of `word1` bytes from `+8`, 1/2 = the JCALG1 decompressor the boot
  code copies to IWRAM `0x03000000`). A chunk is 32 x 32 standard GBA text-BG entries (2 KB).
- `0x080054A4(layer, px)` DMAs the 32 x 32-tile VRAM map (`0x0600E000 + bg * 0x800`) from the four
  chunk caches every frame, offset by `(px & 0xFF) >> 3`, so VRAM only ever holds a 256 x 256 window.
- Consequence for the wide view: a renderer that knows this format can draw any width straight from
  the ROM's chunk table (decompressing chunks itself with a C port of the JCALG1 decoder) using the
  tile graphics and palettes already in VRAM. The ROM only has to publish, at a fixed EWRAM
  address, the view size, and per BG the map header pointer and the full (unwrapped) scroll value;
  the camera/leash/cull/OAM-bit work stays as planned. No large EWRAM buffers are needed.

## Blocker: Windows Smart App Control (2026-10-17)

MSYS2 installed fine (GCC 16, CMake 4.4, Ninja, SDL2, Lua), but `ld.exe` is blocked by Smart App
Control ("An Application Control policy has blocked this file"; `VerifiedAndReputablePolicyState=1`,
code-integrity enforcement 2). It allowed `gcc.exe`/`cmake.exe` only because those exact unsigned
binaries have cloud reputation; a freshly built `mgba-sdl.exe` would have none and be blocked too.
Options: turn Smart App Control off (Windows Security > App & browser control; it cannot be turned
back on without reinstalling Windows), or build and run the custom mGBA inside WSL (Linux binaries
are not subject to it; WSLg shows the window; the Lua bridge works over localhost), or shelve the
wide view. Decision is Ryan's.

## Build requirements

MSYS2 is now installed at C:\msys64 with the packages below. The lean route is
MSYS2 (`mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,SDL2,lua,zlib,libpng,libzip,sqlite3}`), building
the SDL frontend with scripting enabled (`-DBUILD_QT=OFF -DBUILD_SDL=ON -DENABLE_SCRIPTING=ON
-DUSE_FFMPEG=OFF`), 2-3 GB on disk. The SDL frontend has no Scripting window; a `--script` option
will be added so `coop.lua` loads from the command line. The Qt frontend needs Qt 5/6 development
packages on top and is optional.

## The fork (tools/mgba-wide, 2026-10-17)

- `apply.py <mgba src>` patches an mGBA 0.10 tree: the renderer base gets a `gba` pointer (ROM/RAM
  access); the software renderer gets a runtime frame size (`wideW/wideH`, max 720 x 480) read at
  scanline 0 from the EWRAM descriptor at `0x0203FEC0` (`u32 "WIDE", u16 w, u16 h`; anything else
  means the stock 240 x 160), row buffers and scanline caches sized for the maximum, rows 160..H-1
  drawn in `finishFrame` with the last line's register state, `_drawMode0()` routing each text
  background with a chunked map to `software-wide.c` (chunk cache of 64 x 2 KB, C port of the
  decompressor, per-pixel tile fetch and the same compositing helpers as mode 0) and the rest (BG0's
  dialogue text, blank layers) to the stock VRAM drawer; non-affine sprites read x bit 9 / y bit 8
  from OAM attr1 bits 9/10 and are culled against the wide size; the SDL presenter keeps a
  720 x 480 texture and copies the current `mWideFrameW x mWideFrameH` to the window (that scaling is
  the "zoom"); `MGBA_SCRIPT=file.lua` loads a Lua script in the SDL frontend (it has no console).
- Build and run (WSL, see the Smart App Control note): `wsl -d Ubuntu -u root -- bash
  /mnt/f/MeltyCreations/log2/tools/mgba-wide/build-wsl.sh`, then
  `MGBA_SCRIPT=/mnt/f/MeltyCreations/log2/tools/mgba/coop.lua /root/mgba-build/sdl/mgba -3
  /mnt/f/MeltyCreations/log2/build/log2-coop.gba` (WSLg shows the window on the Windows desktop;
  `tools/win_shot.ps1 out.png -Title "mGBA (Ubuntu)"` screenshots it). `tools/mgba/wide_test.lua`
  forces a descriptor (`WIDE_W`/`WIDE_H` env) so the renderer can be exercised before the ROM side.

## Status: working (2026-10-17 evening)

Verified in the WSL fork from the wolf-map savestate: with `make WIDE=1` the view starts at 240 x 160,
widens as the players separate (seen at 344 x 228 and 452 x 300 with both HUD panels at the wide
corners and sprites placed correctly), and shrinks back when they rejoin. Player 2's keys reach the
fork through `MGBA_P2=1` (TCP pad reader in `tools/mgba-wide/p2pad.c`, port `MGBA_P2_PORT`,
default 7777) with the unchanged `tools/p2_keys.py --port <n>` sender. Fixes that mattered: the C
decompressor's copy macro re-evaluated its length argument; mGBA's window columns were 8-bit
(`endX`, `GBAVideoWindowRegion`) and wrapped at 256; every scanline must be marked dirty when the
view size changes; the view only shrinks after a second without player control, so a knockback
does not zoom in.

ROM side (`WIDE=1` build only, stock build untouched): `src/coop/wide.c/.h` (descriptor at
`0x0203FEC0`, state at `0x0203FEC8`, target width = separation + 144 px, height follows 2:3, eased
1/8 per frame, 240..720), `camera.c` (midpoint minus half the wide size, map clamp for the wide
size, cull rect widened after `World_SetView`), `player.c` leash, `hud.c` (player 2's panel at
`w - 96`, OAM x bit 9), `input.c` calls `wide_update()`, `patch_rom.py --wide` redirects the draw
slot of the 13 sprite classes to `sprite_emit_hook` (OAM attr1 bits 9/10 from the entity's full
screen coordinates). `make WIDE=1 WIDE_TEST=480` forces a size for renderer tests;
`make WIDE=1 P2_MODE=1` (mirror mode) separates the players with one key.

Known limits: Lua scripts never fire frame callbacks in the SDL fork (loading works, callbacks do
not), so `netplay.lua` and the console helpers are Windows-mGBA only for now; dialogue boxes, the
pause menu and BG0 text stay at native size in the top-left of a wide frame; OBJ count is still 128;
the stock Windows mGBA ignores the descriptor and shows the normal 240 x 160 of a `WIDE=1` ROM.

## Plan

1. Toolchain install (needs Ryan's OK), stock SDL build, run `build/log2-coop.gba`.
2. Locate and document the map tile streaming routine and tilemap entry format; C reimplementation
   that fills an EWRAM extended tilemap for a given world rect.
3. Renderer patch: runtime (W, H), extended tilemap source, extended OBJ bits, frontend scaling,
   `--script`.
4. ROM side under `WIDE_VIEW`: dynamic view size, camera clamp, leash, cull rect, OAM bits, HUD.
5. Test with both players separating and rejoining; stock mGBA keeps running the normal build.
