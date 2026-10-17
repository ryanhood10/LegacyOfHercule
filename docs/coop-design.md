# Co-op design (Phase 1 findings -> Phase 2 plan)

## How the player moves (confirmed by decompile)

Per frame: `GameLoop_Tick` -> `Input_Poll(g_InputSystem)` -> coroutine update -> entity updates.
The player entity (`g_PlayerObject`, 0x1A0 bytes, vtable `0x08025604`) runs a **state machine**:

- `entity+0x18` = current state object. States are singletons in IWRAM selected by animation id
  through the table at `0x083B5268+0x10` (`SetAnimation` = player vtable slot 10, `0x080088BB`):

  | anim | state object | vtable | update (slot 2) | meaning |
  |---|---|---|---|---|
  | 0 | `0x03001F64` | `0x08025A84` | `0x08007F35` | idle: D-pad press -> facing + anim 1; same direction twice within 20 frames -> anim 0xD; fidget anim 0xE after 0x400 frames |
  | 1 | `0x03001F44` | `0x080256C4` | `0x0800811F` | walk: moves `+0x158/+0x15C` (8.8 fixed) by speed `+0x188` along facing `+0x10`; collision via `0x08006366(ctx[4], &pos)`; wall -> sound 8, anim 0x1F |
  | ? | - | `0x080258DC` | `0x08008031` | diagonal/8-way variant (`0x08007D24` step helper, facing lookup `ctx+0x9C`) |
  | 0xD | `0x03001F48` | `0x08025E20` | `0x080083C3` | dash / flight (EP drain against display row byte +7) |
  | ? | - | `0x080251F4` | `0x08008277` | |
  | ? | - | `0x08025D70` | `0x080085AD` (slot 5) | |

  Slot 0 = enter(state, entity), slot 3 = leave; `0x0800763C` performs the transition.
  All handlers end with: new press A -> `0x08007DE9` (attack), B -> `Player_UseAbility 0x08007EDB`.

- **Input source:** every handler loads the ROM context table `0x083B50E8` and reads word 0
  (`&g_InputSystem`), then `current` at +0x12 / `pressed` at +0x14. The same table supplies the
  collision map (+0x10), facing vectors (+0x24..), facing-combination bytes (+0x9C). Pool words that
  carry the table address: `0x8014`, `0x82B4`, `0x8548` (+0xE0), `0x85F8`.

- **Creation:** `PlayerEntity_Create(mem, a, b)` `0x080086D4` sets the vtable, `g_PlayerObject`,
  and installs the display row's behaviour class (`+0x180`).

## Status

**Shadow milestone reached (2026-10-17).** `make` produces `build/log2-coop.gba` (16 MiB, code at
`0x08800000`). The only ROM edit is the idle state's update slot, redirected to `idle_update_hook`
(`src/coop/coop.c`), which lazily creates a second player entity next to the first the moment the
player is idle on a map. The second entity shares the input block, so it mirrors player 1: verified
in mGBA on the prologue map (two Trunks, both walking with the D-pad, each with its own collision).
The second entity gets a private copy of the player vtable whose destroy clears `coop.p2` instead
of `g_PlayerObject`, so map changes rebuild it cleanly.

**Independent input reached (2026-10-17).** `Input_Poll`'s entry is patched with a long jump to
`input_poll_hook` (`src/coop/input.c`), which polls player 1 exactly as before and fills a second
`InputSystem` for player 2 from a selectable source (`P2_MODE`: link-cable serial, mirror, or
Select-swaps-control). The idle, walk and diagonal control states are ported to C
(`src/coop/player.c`) and take input and the party slot per entity; both players run the ports.
Verified in mGBA in mirror mode: holding Right sends P1 right and P2 left. Player 2 binds to the
first other in-party slot (own HP/EP/abilities/sprite/behaviour); with a one-member party it falls
back to sharing the active slot. Not yet: dash state for P2, HUD for P2, enemies targeting P2.

**Dual HUD reached (2026-10-17).** The HUD object (vtable `0x0802408C`, created by `HUD_Create`
`0x08008B14`) has its draw / update / destroy slots redirected to `src/coop/hud.c`, which calls the
originals and then draws a second panel 144 px to the right from player 2's party slot: own
`ObjVram_Alloc` tile block, same panel art, HP = `hp*48/max`, EP = `ep*49/max`, XP bar, meter and
ready/cooling ability icon (a port of `0x08008BEF`), L on player 2's pad cycles its ability. Tiles
are freed when the HUD object is destroyed. Verified in mGBA.

**Input transport note.** A GBA cartridge sees exactly 10 buttons, so a second key set on one
keyboard cannot reach the ROM; same-system play uses mGBA's second multiplayer window (link cable)
with a second gamepad, or `P2_MODE_SWAP`. Extra keyboard keys and online play both need the
emulator-side input patch planned for Phase 3.

## Player 2 input transports (2026-10-17)

Player 2's buttons reach the game through `g_p2_remote` (EWRAM word, bit 15 = valid, bits 0-9 =
GBA keys active-high; address in `build/coop.sym`) with the link cable as fallback:

1. **Same PC, one keyboard (works).** mGBA (Qt) > Tools > Scripting, run
   `dofile("F:/MeltyCreations/log2/tools/mgba/p2_input.lua")`; then `python tools/p2_keys.py`.
   The sender reads the keyboard globally (IJKL move, O = A, U = B, [ ] = L R, N/M = Select/Start)
   and streams changes over TCP 7777; the script writes them into `g_p2_remote` every frame.
   Verified: P2 walks while P1 stands still, in a single window.
2. **Remote (same path).** The brother runs `p2_keys.py --host <host ip>` and watches the game
   over a screen share; the host forwards port 7777. Proper lockstep netplay stays Phase 3.
3. **Link cable (real hardware; stalls under mGBA).** Every boot puts the serial port in multiplayer
   mode; the host starts a 16-bit transfer per frame and reads `SIOMULTI1`; holding L+R in the first
   10 s turns a second cartridge into a pad (`src/coop/controller.c`, mode-3 button display, marker
   `0x4000`). Under mGBA 0.10.5's lockstep the master's `SIOCNT` sticks at `0x208B` (busy) and the
   slave never goes busy, whether the slave is the controller loop or a plain game instance; the
   stuck phase matches `_masterUpdate` never being re-run after STARTING. Untested on hardware.

## Enemies, damage and the remaining control states (2026-10-17)

- `EntityList_Update` `0x080069A4` (expires the event queue, then calls vtable slot 2 on every
  entity) is replaced by `src/coop/entities.c`; while a non-player entity updates, `g_PlayerObject`
  points at the player nearest to it, so chase, attack and hit tests target player 2 when they
  should. Verified in zone 2 area 7 (wolves): a wolf left player 1 alone and went for player 2.
- Damage: player vtable slot 14 `OnHit` -> slot 16 `TakeDamage` (active slot!) and slot 15
  `ChangeHP`. Player 2's private vtable copy overrides 15/16 to use its own slot; at 0 HP player 2
  revives at half HP beside player 1 instead of dying (a wolf hit took slot 0 from 975 to 917 while
  the active slot 1 was untouched).
- Dash/charge (`0x08025E20`), `0x080251F4` and `0x08025D70` control states run the *original* code
  for player 2 through `src/coop/wrap.c`, which swaps player 2's `InputSystem` contents into the
  global block (and the last-direction globals) around the call.
- `make DEBUG_MENU=1` applies the documented title-screen patch: ~75 s idle opens the debug menu;
  Map Test loads any map with a full level-40 party (so player 2 gets a real second character).
  Rows: Music, Sample, Map Test; Zone/Area/Variation adjust with the d-pad under Map Test.
- The TCP bridge now repeats the pad word 20x/s and the Lua side drains the socket every frame
  (`hasdata`); the callback-only version dropped presses.

## Shared-screen camera (2026-10-17)

- The camera-follow object (vtable `0x08025A6C`, update slot 0, original `0x0800D9F8`) aims the
  view at its target entity's bounding-box centre minus (120, 84) with per-frame smoothing of 3/64
  (x) and 9/128 (y), then `Camera_Apply` `0x080062E8` clamps to the map and sets the world object's
  view origin (`0x03001090` +0x1C/+0x20; position kept as px << 10 at +4/+8). `src/coop/camera.c`
  replaces that slot: when player 2 exists the target point is the midpoint of both players'
  box centres, same smoothing, same setter.
- Both players are leashed to the visible area (`leash_p2`, applied in the ported control states
  and after the wrapped originals), so the screen edge is the separation limit: a player walking
  away stops at the edge until the other follows. Scripted movement is unaffected since the leash
  only runs in player-controlled states. Verified in zone 2 area 7.
- A zoomed-out view is not possible on GBA hardware (tiled backgrounds cannot be scaled); it would
  be an emulator-side renderer feature for Phase 3, alongside widescreen.

## Memory placement (important)

The co-op state lives at the top of EWRAM in a pinned section (`g_p2_remote` `0x0203FF00`, `coop`
`0x0203FF04`, see `coop.ld`). The game's heap (`Arena_Init`, `0x0801EFA4`) is `0xFDFE` words from
`0x02000800`, i.e. it reaches `0x0203FFF8` and overwrote that block on entity-heavy maps (symptoms:
player 2 respawning every frame, garbage input). `tools/patch_rom.py` shrinks the size word at ROM
`0x1F20C` to `0xFD7E`, reserving the top 512 bytes. Entity state fields: `+0x18` is the control state
(idle `0x08025A84`, walk `0x080256C4`, ...), `+0x1C` a secondary state (default `0x03001F3C`).

Live inspection without pausing: `dofile("…/tools/mgba/peek.lua")` in the Scripting console, then
`peek()` prints both players' positions/states, both slots' EXP/HP and the co-op counters (addresses
from `coop.sym`). See also `ents.lua` and `force.lua` below.

## Session logic

- Player 2 is only spawned while a player-2 input source is present: a valid bridge word
  (`g_p2_remote` bit 15) or a valid link-cable word within the last 3 s (`coop.source_age`). With no
  second pad the game is unchanged single-player. In the mirror/swap test modes a source is implied.
- Player 2 spawns on player 1's position (not offset), so map-edge spawns stay walkable.
- While player 1 is not in a player-driven control state (cutscenes, scripted walks, menus), player
  2 sees no input (`p1_controllable()` checks the control-state vtable), so it cannot wander during
  dialogue.

## Soak results (2026-10-17)

- Pause menu (Start) opens and closes with player 2 present; the status screen shows the active slot.
- Map transition (zone 2 area 7 -> east neighbour, both players walking off the edge): player 2 is
  destroyed with the old map (`destroys=1`) and recreated on player 1 at the new spawn
  (`spawns=2`); the new map's intro dialogue ran normally.
- Dialogue boxes suspend entity updates, so neither player moves during them; the control-state
  gate (`p1_controllable`) covers scripted movement states where updates keep running.
- The screen-edge leash means a parked player 2 stops player 1 at the view edge (by design);
  with one person testing, drive both pads (`UP+I`) to walk together.

## EXP crediting (2026-10-17)

- Enemy death calls `0x0800E4D2(enemy)`: base EXP = word 0 of the enemy's behaviour object
  (wolves 13, snakes 45), scaled to the *active* slot's level (clamped to `[span>>7, span>>1]` of the
  current level's EXP span, level 50 gets nothing), then queued as a 12-byte event
  `{vtable 0x080245B0, ticks 1, amount}` on the entity list (`0x03001120`, count +0x610). The event's
  slot 3 fires `Party_AddExp(g_Party, active_slot, amount)` (`0x08003D58`, handles level-ups: stat
  gains, popup, sound), slot 2 destroys it.
- `src/coop/exp.c` replaces the entry (10-byte long jump, the function starts at a 2-byte-aligned
  address) and credits the player who was acting when the enemy died: `coop.acting` is maintained by
  the entity-list replacement (the player being updated, or the nearest player while a non-player
  entity updates). The event grows to 16 bytes with the credited slot, using a private vtable copy
  whose fire slot calls `Party_AddExp` with that slot; the level clamp uses the credited slot too.
- Verified in zone 2 area 7 with a level-40 party: player 2 (Gohan, slot 0) killing a wolf added
  4747 EXP to slot 0 and nothing to the active slot; player 1 killing one added 4747 to the active
  slot only (`peek()` shows both slots' EXP and the `awards` counters).
- Note: one award to the active slot is counted on every debug-menu Map Test entry, before the
  menu resets the party's EXP to level 40; it is the original game's behaviour, not a co-op path.
- Both players' `+0x180` behaviour pointer is the same IWRAM cell (`0x030021E0`): the 22 display
  rows map onto five cells by behaviour *type* (rows 0-3 = the base forms share one), so
  `install_behaviour` for player 2 re-initialises a type singleton of the same type, which is
  benign. Rows of another type (4, 5, 7, 10, 11, 14, 16, 17, 20) use their own cells.

## Test tooling additions

- `tools/mgba/ents.lua`: `ents()` lists both players and every enemy (vtable `0x080252A4`) with
  position, base EXP and control state, plus the camera view origin; `ents(true)` lists everything.
- `tools/mgba/force.lua`: `p2_force = 0x8000 | keys` drives player 2's pad from the console (bit 15
  valid, bits 0-9 GBA keys), `p2_force = nil` returns control to the TCP bridge. Useful with mGBA
  savestates (Shift+F1 / F1) to retry a fight without the 75 s debug-menu idle.
- Typing into the Scripting console while another helper focuses the game window sends keystrokes
  into the game (one such slip opened the pause menu and quit to the title); keep console typing and
  game-window key presses sequential.

## Plan for the two-Goku proof (Phase 2 start)

1. **Extend the ROM to 16 MiB** and place new code at `0x08800000` (only ~6.8 KB is free below).
2. **Reimplement in C** (compiled with arm-none-eabi-gcc, calling original ROM routines by address):
   - the idle / walk / diagonal / dash handlers, identical except the input pointer comes from a new
     entity field (`entity+0x1A0`, grow the allocation to 0x1A8) instead of the ROM table;
   - `PlayerEntity_Create2`: second entity with the same vtable, its own copies of the state objects,
     `input = &g_Input2`, registered in `g_EntityList`;
   - `Input_Poll2(&g_Input2)`: first milestone mirrors P1's keys (a "shadow" Gohan), then reads the
     serial register (`0x04000120`, already the handlers' literal base) for real link-cable input.
3. **Repoint** the state vtables' slot 2 at the new handlers, hook `PlayerEntity_Create` to also create
   P2, and hook `GameLoop_Tick` after `Input_Poll` to poll P2.
4. Camera: leave on P1 for the proof; leash P2 to the screen later.

Known hook points: `EntityList_Add(list, entity)` `0x080068B8`; `PlayerEntity_Create` is called from
`0x0800C60E` (inside the map loader, fn `0x0800C584`) and `0x080129DA` (fn `0x080129B8`).

Open items: which player-vtable slot calls `state->update` each frame (find dynamically with
`tools/gdb/watch_input.gdb` once a savestate exists) and the camera object.
