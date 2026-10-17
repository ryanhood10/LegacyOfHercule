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
