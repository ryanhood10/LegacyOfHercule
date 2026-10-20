# Co-op design (Phase 1 findings -> Phase 2 plan)

## How the player moves (confirmed by decompile)

Per frame: `GameLoop_Tick` -> `Input_Poll(g_InputSystem)` -> coroutine update -> entity updates.
The player entity (`g_PlayerObject`, 0x1A0 bytes, vtable `0x08025604`) runs a **state machine**:

- `entity+0x18` = current state object. States are singletons in IWRAM selected by animation id
  through the table at `0x083B5268+0x10` (`SetAnimation` = player vtable slot 10, `0x080088BB`):

  | anim | state object | vtable | update (slot 2) | meaning |
  |---|---|---|---|---|
  | 0 | `0x03001F64` | `0x08025A84` | `0x08007F35` | idle: D-pad press -> facing + anim 1; same direction twice within 20 frames -> anim 0xD; fidget anim 0xE after 0x400 frames |
  | 1 | `0x03001F44` | `0x080258DC` | `0x08008031` | walk (all directions): the diagonal / 8-way state; plain walking runs here (corrected 2026-10-20) |
  | 0xD | `0x03001F48` | `0x080256C4` | `0x0800811F` | **sprint** (a direction twice within 20 frames; corrected 2026-10-20: the anim table at `0x083B5278` maps 0xD to this 4-way walk state, not to `0x08025E20`): moves `+0x158/+0x15C` (8.8 fixed) by speed `+0x188` along facing `+0x10`; collision via `0x08006366(ctx[4], &pos)`; wall -> sound 8, anim 0x1F |
  | ? | - | `0x080258DC` | `0x08008031` | diagonal/8-way variant (`0x08007D24` step helper, facing lookup `ctx+0x9C`) |
  | ? | - | `0x08025E20` | `0x080083C3` | flight / charge (EP drain against display row byte +7) |
  | 0x24 | `0x03002180` | `0x080256AC` | `0x0801358C` (slot 1) | detransform (abilities 11 / 12 when the row's flag bit 0 is set): group 21 of the current record (record + 0x19C), frame = counter / 16 for 64 frames, then back through the row's link |
  | 0x25 | `0x03002188` | `0x08025CE0` | `0x0801560E` (slot 1) | transform (bit 0 clear): plays group 9, then the row's link |
  | 30 | `0x03002268` | `0x08025038` | `0x0801CB24` (slot 1) | NPC beam pose (`NPCFireKamehameha`): sprite group 18, frame 0 for 8 frames, then 1 / 2 every 8; never leaves by itself |
  | ? | - | `0x080251F4` | `0x08008277` | |
  | ? | - | `0x08025D70` | `0x080085AD` (slot 5) | end of an attack: A held + the active slot's flags bit 2 (special melee page) -> anim 0x27 (group 16), else 0; replaced by `wrap.c` `attack_end` (each player's own slot, only with group 16) |

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
(now in `src/coop/player.c`; there is no `coop.c`), which lazily creates a second player entity next
to the first the moment the
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
back to sharing the active slot. (At that point player 2 still lacked the dash state, a HUD and
enemy targeting; all three landed the same day, see the sections below. The one-member fallback was
later dropped: the players never share a character.)

**Dual HUD reached (2026-10-17).** The HUD object (vtable `0x0802408C`, created by `HUD_Create`
`0x08008B14`) has its draw / update / destroy slots redirected to `src/coop/hud.c`, which calls the
originals and then draws a second panel 144 px to the right from player 2's party slot: own
`ObjVram_Alloc` tile block, same panel art, HP = `hp*48/max`, EP = `ep*49/max`, XP bar, meter and
ready/cooling ability icon (a port of `0x08008BEF`), L on player 2's pad cycles its ability. Tiles
are freed when the HUD object is destroyed. Verified in mGBA.

**Input transport note.** A GBA cartridge sees exactly 10 buttons, so a second key set on one
keyboard cannot reach the ROM; same-system play uses mGBA's second multiplayer window (link cable)
with a second gamepad, or `P2_MODE_SWAP`. (Done since, without an emulator patch: a second key set
on one keyboard goes through the TCP bridge below, online play through lockstep netplay, section
"Lockstep netplay".)

## Player 2 input transports (2026-10-17)

Player 2's buttons reach the game through `g_p2_remote` (EWRAM word, bit 15 = valid, bits 0-9 =
GBA keys active-high; address in `build/coop.sym`) with the link cable as fallback:

1. **Same PC, one keyboard (works).** mGBA (Qt) > Tools > Scripting, run
   `dofile("F:/MeltyCreations/log2/tools/mgba/p2_input.lua")`; then `python tools/p2_keys.py`.
   The sender reads the keyboard globally (IJKL move, O = A, U = B, [ ] = L R, N/M = Select/Start)
   and streams changes over TCP 7777; the script writes them into `g_p2_remote` every frame.
   Verified: P2 walks while P1 stands still, in a single window.
2. **Remote (same path).** The brother runs `p2_keys.py --host <host ip>` and watches the game
   over a screen share; the host forwards port 7777. Lockstep netplay (each player runs the ROM, no
   screen share) was built later: section "Lockstep netplay".
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
  `0x080085AD` (the end of an attack) is reimplemented instead (`attack_end`): it reads the party flags
  of the active slot, which for player 2 is player 1's ("The twin-Cell stall" under Super Android 13).
- `make DEBUG_MENU=1` applies the documented title-screen patch: the title's idle timeout, which
  normally starts the intro video, opens the debug menu instead. The title waits 1800 frames (30 s)
  with no key held (`0x08000434` compares `g_FrameCounter` with 0x708); counted from power-on it is
  about 75 s, since the logos and intro run first;
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
- A zoomed-out view is not possible on GBA hardware (tiled backgrounds cannot be scaled). Done as an
  emulator-side renderer: `make WIDE=1` with the WSL mGBA fork widens the view as the players
  separate (`docs/widescreen.md`).

## Memory placement (important)

All co-op RAM lives in pinned regions at the top of EWRAM (`src/coop/coop.ld`), above the game's
heap. The heap (`Arena_Init`, `0x0801EFA4`) hands out `[0x02000808, 0x02000808 + 4 * cap)`; the stock
cap `0xFDFE` words runs to `0x02040000` and overwrote co-op state on entity-heavy maps (player 2
respawning every frame, garbage input). `tools/patch_rom.py` (`WORD_PATCHES`, file offset `0x1F20C`)
sets the cap to **`0xF9FE`**, so the heap ends at `0x0203F000`. The regions above it:

| address | region / section | contents |
|---|---|---|
| `0x0203F000` | `partyram` (0xB00): `.party`, `.party.kr`, `.party.wv` | relocated party block `g_party_block`, save-menu slot store, switch screen's `switch_view`; then `a16p` and `kr_state`; then the Time Chamber `waves` state (`0x0203F880` in the merged build, 0x91C of 0xB00 used with `PARTY_SLOTS` 8) |
| `0x0203FB00` | `ewram2`: `.coop2` | `coop2` (`CoopExtra`, enemies.c; static assert <= 0x2C0) |
| `0x0203FDC0` | (fixed address) | `StoryTest` preset copy (0x40, coop.h) |
| `0x0203FE00` | `vtram`: `.coop.vt` | `g_p2_vtable[48]` (0xC0) |
| `0x0203FEC0` | (fixed address) | wide-view descriptor (8 bytes), `WideState` at `0x0203FEC8` (wide.h) |
| `0x0203FF00` | `ewram`: `.coop`, `.bss` | `g_p2_remote`, `g_p1_remote` (`0x0203FF02`), `coop` (`CoopState`, `0x0203FF04`), then `.bss` |

The full map with every field is in `docs/ids.md` "Memory"; exact addresses of a build are in its
`coop.map` / `coop.sym`. (History: the cap was `0xFD7E` on 2026-10-17, then `0xFCC0` when `coop2`
arrived; both superseded by `0xF9FE` when the party block moved to `0x0203F000`.)

Entity state fields: `+0x18` is the control state (idle `0x08025A84`, walk `0x080256C4`, ...), `+0x1C`
a secondary state (default `0x03001F3C`).

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
  benign. Rows of another type (4, 5, 7, 10, 11, 14, 16, 17, 20) use their own cells. (The build has
  29 rows; the added rows 22-26 and 28 are copies of standard rows and use the base-form cell, 27 is a copy
  of Super Saiyan Goku's row 20 and uses its cell, see `docs/engine.md`.)

## Player 2's beam abilities (2026-10-19)

- The six beam abilities' charge states read B from the global input block in their update (slot 2):
  Masenko (anim 45, vtable `0x08025D5C`, `0x08014F88`), Big Bang (46, `0x080258B4`, `0x08015078`) and
  Burning Attack (47, `0x08025E48`, `0x08015176`) fire on **B released**; Kamehameha (48, `0x08025CB8`,
  `0x08015248`, fires after 24 frames), Special Beam Cannon (49, `0x08025F70`, `0x0801539A`) and Spirit
  Bomb (51, `0x08025CCC`, `0x080156B6`) keep charging **while B is held**. For player 2 that was player
  1's B: its held beams stopped at once and its Masenko charged until player 1 let go of B. `wrap.c`
  (`masenko_update_hook` .. `spiritbomb_update_hook`, `VTABLE_HOOKS`) runs them with player 2's input.
- Player 2's Select (`p2_cycle`) switched characters in any state; in the Masenko charge it freed the
  held ball (the state's leave) while the scene list still held it, and the next flush jumped to garbage
  (r14 `0x08006ACF`). `p2_cycle` now refuses (the "can't" sound) unless player 2 stands, walks or moves
  diagonally. Seen with Super Saiyan 2 Gohan as player 2 (Masenko, then Select).

## Character switching (2026-10-17)

- The game switches characters only at a save pad (A on the pad opens the Switch Characters
  screen, `0x08010BCE`; its input handler `0x08011172` writes the chosen slot to
  `g_Party->active_slot`, swaps player 1's sprite record (`+0x48`) in place, re-installs the
  behaviour (`0x080086A2`) and resets every non-active slot's display row to its base form
  (`0x08003ED4`). No map reload, so player 2's entity survives the switch. The pause menu's Status
  page only browses characters (Left/Right).
- Rule: the two players never share a party slot. `coop_active_check()` (every input poll) sees
  the active slot change; if player 1 took player 2's character, player 2 is rebound to player 1's
  previous slot (a trade), else to the first other party member. `p2_bind()` mirrors the game's
  switch: new sprite record, behaviour, idle animation reset; HUD, damage and EXP follow the slot.
- Player 2 switches on its own with **Select** (next party member that is in the party, flagged
  available (flags bit 1, the save-pad screen's greying rule) and not player 1's), anywhere while
  player 1 is controllable. The chosen slot survives map changes (`pick_p2_slot` keeps it).
- Verified from a savestate: Select moved player 2 Gohan -> Vegeta (own HUD updated); an emulated
  save-pad switch (active slot := player 2's, player 1 sprite := player 2's) rebound player 2 to
  Piccolo. A switch at a real save pad is still to be played through.
- Savestates survive rebuilds: `coop_init_once` rebuilds the RAM vtable copies when the stored
  `code_id` (address of `coop_init_once`) differs from the running build.

## Cutscene map changes and the prologue (2026-10-17)

- Bug found in Ryan's playthrough: after the prologue's first cutscene only one player and one HUD
  remained and the Flight Circle would not respond. Cause: the cutscene replaced the player entity
  *inside* `EntityList_Update`'s loop, and the co-op replacement restored the `g_PlayerObject` it had
  captured before the loop, leaving the game pointing at freed memory. The loop now re-reads both
  player pointers for every entity and only restores `g_PlayerObject` if the callee left it alone.
- The prologue party is Trunks alone, which made player 2 share his slot (and HP). Rule: the players
  never share a character, so `pick_p2_slot` returns -1 and player 2 only joins once a second party
  member exists (`ensure_p2` re-checks every idle frame, so it appears as soon as someone joins).
- Flight Circles: stand on the circle and press A; the scripted flight then loads the next map, which
  destroys player 2 and respawns it on player 1 at the landing spot. A takeoff animation for player 2
  (each player lifting off from where they stand) is not done yet.

## Player 2 = Hercule (2026-10-17)

- Ryan's call: player 2 defaults to Hercule (party slot 5, display row 21, sprite 72). The game
  fills his entry at new game (level 1, 50 HP, 20 EP, no flight, one ability) but only flags him
  into the party late in the story, so `slot_ok` accepted slot 5 whenever its `max_hp` was set.
  (Superseded 2026-10-18: `slot_ok` in `player.c` now needs party flag bit 0 (in the party) and, when
  picking player 2, bit 1 (available), for Hercule too, so the story's flag opcodes choose and lock
  player 2. `pick_p2_slot` still prefers Hercule when he qualifies.) Player 2 can still cycle to other
  available members with Select and comes back to Hercule by cycling.
- Verified from the prologue Flight Circle savestate: Hercule appears with his own panel, flies with
  Trunks and respawns on him at the landing spot. Player 2 spawns 12 px left of player 1 when that
  spot is walkable, else exactly on player 1 (overlap beats leaving the map).

## Plan: co-op difficulty, twin bosses (historical, 2026-10-17; today's rules are in "Difficulty" below)

History (2026-10-17, superseded by "Co-op difficulty: enemy object map, twins and scaling" and "Boss
rules and extra stats" below, which are authoritative): the spawn hook turned out to be the map
loader's enemy record handler (`0x0800E77E`), not a bytecode native; the enemy stat entry is mapped;
the pair ideas (Frieza + Cooler, Cooler + King Cold, robots duplicated) gave way to the rules table in
`src/coop/bosses.c`. The plan as written then:

Ryan's direction: with two players, enemies should be stronger, smarter, faster, more numerous,
and every boss fight gets a second boss of a different model at the original's strength (Frieza +
Cooler, Cell + Cell, Cooler + King Cold on Namek, Buu + Buu, robots duplicated), with two boss HP
bars and the two models placed side by side rather than overlapping.

Engine facts to build on:
- Enemies are class `0x080252A4` entities; their behaviour object (`+0x180`) starts with base EXP,
  the rest (HP, damage, speed, abilities) still has to be mapped. Movement speed is `+0x188/+0x18C`.
- Enemies already target the nearest player (entities.c). Twin spawns should alternate targets
  (one per player) instead of both chasing the same person.
- Enemy and boss placement comes from the map scripts (bytecode VM natives; the reference notes
  in the Arefu clone list the opcodes). The spawn native is the hook: in co-op, spawn a twin 16-24 px
  beside the original with a scale factor applied to HP/damage, and for bosses with the alternate
  sprite/behaviour of the partner character.
- The boss HP bar is drawn by the HUD object (vtable `0x0802408C`); the second bar follows the same
  pattern as the player-2 panel (own tile block, drawn under the first), bound to the twin entity.
- King Cold has fight sprites but no enemy class in the ROM; the twin for Namek Cooler needs a new
  behaviour object built from Cooler's with King Cold's sprite record.

Steps: (1) map the enemy behaviour object and the spawn native, (2) co-op scale factors for HP,
damage and speed plus twin spawns for regular enemies, (3) boss twins with partner models, (4) the
second boss bar, (5) tune so a level-1 Hercule beside Gohan is survivable.

## Co-op difficulty: enemy object map, twins and scaling (2026-10-17)

### Enemy entity (class vtable `0x080252A4`, 0x1F0 bytes)

Created by the map loader through the enemy record handler `0x0800E77E` = `Enemy_Create(0, record,
&stats[record->stat_index])` (`0x0800E281`); the loader adds the returned entity to the list itself.

| offset | field |
|---|---|
| `+0x160..+0x164` | combat bytes copied from the stat entry by the base init `0x0800F7CC`: melee stat, ranged stat, defence, melee resistance, ranged resistance |
| `+0x180` | `EnemyStat *` (word 0 = base EXP, used by the EXP award) |
| `+0x184` | record's action object `{func, payload}` (`0x0801069B` bytecode, `0x080106B3` dialogue, `0x08010699` none) |
| `+0x188` | current HP (max = stat `+4`) |
| `+0x190` | behaviour-list state `{current, index, record+0x18}` (waypoints / waits, ticked by `0x0800D156`) |
| `+0x19C` | AI state `{entity, count, (module, class) x5 ...}` ticked by `0x0801AB09` from update slot 2 |

Vtable slots (function = table + word): 0 destroy `0x0800E33B` (unregisters the collision box, frees
the AI and behaviour list), 2 update `0x0800E5A5`, 9 collision / message `0x0800E5B9` (type 0 =
hit by a player attack: box test, damage calc, AI notify; `0x14` = scouter registration by stat
index), 10 / 11 control-state set / test, 12 / 13 collision box register / unregister, 14 OnHit
`0x0800E485` (knockback + popup `0x0800F833`, `HP -= hit->dmg`; at 0 it broadcasts list message
`0xF` and calls the stat class' death handler), 15 heal `0x0800E561` (clamped to the stat max).
Like the player vtable it is 48 words long; the co-op copy takes all of them.

**Stat database** `0x086D56AC`, 24 bytes per entry (`EnemyStat` in rom.h): `+0` base EXP, `+4` max
HP, `+8` AI set, `+0xC` melee / ranged / defence / melee-resistance bytes, `+0x10` ranged
resistance, `+0x14` class object (word 2 = death handler). Wolves (zone 2 area 7) are stat 53:
13 EXP, 35 HP, AI set 8, melee 7, defence 3. Bosses have 0 EXP and >= 1000 HP; scripted targets
(rocks, tutorial dummies) have 0 EXP and tiny HP.

**Map entry** (`0x08409368`, 327 x 0x38): `+0` zone, `+1` area, `+4` script count (byte), `+0x14`
`mapScripts` pointer array (handlers `0x0800E77F` enemy, `0x0800B711` NPC, `0x0800D6DF` actor), `+5`
/ `+0x18` items, `+6` / `+0x1C` objects, `+7` / `+0x20` triggers (8 bytes), `+0x30` on-load script.
Enemy record: `+4` condition flags (evaluated by the loader), `+8` stat index, `+0xC` x,y, `+0x10`
sprite id, `+0x14` action object, `+0x18` behaviour count, `+0x1C` behaviour pointers. 666 records on
167 maps; every boss is one of them (with a condition), nothing spawns enemies from bytecode.

**Damage** `0x0800F910(defender, hit)` (`Hit` in rom.h: `+4` multiplier 10.10, `+8` attack
descriptor (byte 0: 0 melee / else ranged, word `+4` base power), `+0xC` attacker, out `+0x10` crit,
`+0x14` damage, `+0x18` secondary): melee uses the attacker's `+0x160` byte with a crit chance of
`stat*0xA4/65536`, ranged the `+0x161` byte; the curve `stat*(stat*26-701)+115618`, `(128 - defence)/128`
and a `(256 + rand(26))/256` jitter give the damage. Both players' hits on enemies and enemy hits on
players go through it (player collision slot 9 `0x08008945` cases 0 / 1 call it with the hit at
`desc+0x14`). Two RNG draws (`0x0802142F` chance, `0x0802141D` range) in a fixed order.

**AI** sets: table `0x086D5194`, 154 x `{class, param}`; module classes are `{create, update, finish,
param, ...}` trees: SELECT by distance² to `g_PlayerObject` (`0x0801AEF3`, `0x0801D749` with
hysteresis), GOTO set (`0x0801AB65`), SEQUENCE (`0x0801B887`), PARALLEL (`0x0801B63F`), CHANCE
(`0x0801BA47`), wait (`0x0801BBAD`), and the leaf movers whose objects are `{vtable, speed 8.8}`:
8-direction chase `0x0801B333` (vtable `0x08023BE0`; wolves 0xB3 = 0.7 px/frame) and axis-aligned
approach `0x0801B15F` (`0x08023D88`). Wolves: set 8 = patrol until a player is within 72 px, set 9 =
chase. Every module reads the target from `g_PlayerObject`, which is why the per-entity swap in
entities.c retargets them.

**Death**: the stat class' handler (regular enemies `0x0801A74D`) awards EXP (`0x0800E4D2`, our
hook), queues a 0x14-byte popup event (vtable `0x080238FC`, fire slot `0x080109AD` runs the record's
action object: defeat dialogues and scripts fire here), removes the entity, spawns dust and plays
the death SFX. The boss class `0x086FF3C4` (`0x0801A82D`) only awards EXP and queues the event; the
boss entity stays for the script.

**Boss bar**: `DrawBossHealthBar(charIdx)` (opcode 85, `0x0800AB1B`; 90 takes a percent range) finds
the entity by sprite id (`0x08009371` -> list find type 5) and adds `BossHealthBar_Create(0, e, lo,
hi)` (`0x080104C1`, `BossBar` in rom.h, vtable `0x08025748`): two 32x8 sprites at x = 160 sliding
from y = -8 to 8, fill = `lo + hp * range / stat->max_hp` px of 60, redrawn from a 60-entry offset
table at `0x086D60F4` into its own 0x200-byte tile block. Message `0xF` (entity died) clears its
entity pointer, `0x10` slides it out.

### What co-op changes (src/coop/enemies.c)

- `.coop2` block at `0x0203FB00` (the heap cap was `0xFCC0` then; it is `0xF9FE` now, see "Memory
  placement"): enemy and boss-bar vtable copies, a 24-pair twin registry and counters (`ents()` prints them).
- Spawn hook on `0x0800E77E`: with a player-2 source present every enemy with EXP (and every boss)
  is created from a private stat copy and record copy carried in its own arena block (`0x1F0` +
  stat + record, freed with the entity), plus a twin 24 px to the right (left if blocked, else on
  top) from a record copy with the shifted spawn point, so the collision box the constructor
  registers is right. Regular enemies and their twins get `max_hp x 1.5`; bosses keep their stats.
  Twins are added to the entity list by the hook and registered as a pair. Twins give full EXP.
- Chase-module constructors (`0x0801B332`, `0x0801B15E`) are replaced: a co-op enemy's module gets
  `speed x 1.2`. NPC actors and single-player modules are unchanged.
- Damage calc (`0x0800F910`) is ported; an enemy hit on either player is `x 1.25` when player 2 exists.
- Targeting: entities.c asks `coop_target()`: originals see the nearest player, a twin whose original
  is alive sees the *other* player, so a pair splits up instead of both chasing the nearest.
- The private enemy vtable only overrides destroy (clears the registry entry); everything else runs
  the original code.
- Boss bar hook on `0x080104C1`: when the entity has a registered twin, a second bar bound to the
  twin is added with a vtable copy whose update settles it at y = 20 (under the first).
- Nothing changes without a player-2 source: the hooks call the originals' logic with the ROM data.

### Verified (zone 2 area 7 from the debug menu, player 2 forced present)

- 8 enemies (5 wolves, 3 snakes) spawn as 8 pairs: `ents()` shows every `CE` with a `TW` 24 px
  away, wolves 52/52 HP (35 x 1.5) and snakes 97/97 (65 x 1.5); `twins=8 scaled=16`.
- Chase modules: `spdwatch()` logs the wolves' chase at speed 214 (0.84 px/frame) in co-op and 179
  (0.70) in the single-player pass of the same map (snakes 256 -> 307).
- Damage: with `hits()` logging every party-slot HP change, wolf bites do 7 with the multiplier and
  6 with `coopoff(1)` on the same characters (`dmg()` shows the hook's counters and last result;
  Rand draws are unchanged, lockstep-safe).
- Targeting: `twins()` shows chasing pairs split (original nearest player 1, twin going for player 2,
  or the reverse) while patrolling pairs stay together.
- Single-player pass (pad word cleared, 3 s source timeout, then the same Map Test): plain
  `0x080252A4` enemies, 35 / 65 HP, no twins, `scaled=0`; and the registry is empty after the map
  unloads (destroy hook).
- Hercule (level-40 debug party) and Piccolo both survive about a minute idle among 16 scaled
  enemies; tuning for a level-1 Hercule is still open (stage 5).

### Boss roster (identified in mGBA by standing each sprite id next to the player: `actor(n)`)

Boss records are spawned by scripts: their condition word is `1`, which the loader's evaluator
(`0x08003F07`: type = high half, 0 = never) rejects, and the fight's bytecode spawns them with the
TriggerMapEvent opcode (84, `0x0800AAAF` -> `Map_SpawnScript` `0x0800C73B`: `mapScripts[index]`
without the condition). The same opcode path goes through the co-op spawn hook, so scripted bosses
get twins too. First forms with condition 0 (Cell at the Cell Games) spawn at map load.

| map (debug-menu zone/area) | stat | sprite | HP | who |
|---|---|---|---|---|
| Z6 A61 "New Namek" (6/28) | 29 | 34 | 10200 | Cooler |
| Z11 A4-A6 Master Roshi's island (11/4..6) | 3 | 9 | 3550 | Android 17 |
| Z8 A49 (8/49) | 4 | 11 | 2910 | Android 18 |
| Z7 A15 / A19 (7/15, 7/19) | 5 / 6 | 12 / 14 | 2970 / 2810 | Garlic Jr / a Spice boy |
| Z8 A13, A27, A40 | 7 | 14 | 1900 | the Spice boys |
| Z2 A50 East District 439 (2/40) | 44 | 50 | 6400 | the red robot |
| Z10 A20 / Z12 A4 | 60 / 76 | 28 / 29 | 5210 / 4890 | Imperfect / Semi-Perfect Cell |
| Z12 A6, Z14 A2, Z14 A3 | 78-79, 21-25 | 30 | 5160-8230 | Perfect Cell forms; stat 26 sprite 31 x3 = Cell Jr |
| Z6 A70 (6/31) | 106, 64, 98 | 156, 76, 121 | 800, 1800, 2110 | tournament gauntlet (EXP, condition 1): Yamcha, Krillin, Tien |

Other sprites: 74 King Cold (cutscene only), 49 Frieza (cutscene only, no enemy record anywhere),
41 Dr. Gero, 72 Hercule, 75 Korin, 76 Krillin, 102 Oolong, 121 Tien, 7 Saibaman, 38-40 and 44-47
and 124 robots (regular enemies), 123 / 98 / 128 the big beasts. **There is no Frieza boss fight in
Legacy of Goku II**: Frieza and King Cold only appear in the opening cutscene, so "Frieza + Cooler"
cannot be built; the hidden Cooler fight on New Namek is the Frieza-family boss. Majin Buu is not in
this game either (Buu's Fury).

The debug menu's Area counter is the index within the zone's map-entry block, not the area byte:
`menu area = entry index - first entry of that zone + 1` (zone first entries: 1:0 2:7 3:49 4:63
5:141 6:145 7:177 8:203 9:261 10:268 11:293 12:299 13:313 14:323).

### Boss pairs (stage 3) and the second bar (stage 4)

- Boss = `max_hp >= 1000` and (no EXP or condition word 1). The twin keeps the original's stats and
  AI; `partner_sprite()` gives it another model: Android 17 <-> 18 (verified: 17 on Z11A6 with an
  18-model twin fights normally). Cooler's AI set 21 crashes ("Jumped to invalid address") with any
  other sprite record (King Cold 74, Android 17 9 tried; Cell's AI with Cooler's model is fine), so
  Cooler's twin is a second Cooler until that module (`0x0801C05B` sequences bound to the record) is
  understood. `partner(n)` from the console overrides the table for experiments.
  (Superseded 2026-10-18: the crash came from King Cold's 5-group record, Cooler with Android 17's
  model fights fine, and Cooler's partner is now the Namekian servant, sprite 166 / stat 113; see
  "Android 13, the Namekian servant and the boss-pair tests". The partner table moved to
  `src/coop/bosses.c`, "Boss rules and extra stats".)
- History (2026-10-17, superseded by the both-down rule in "Boss rules and extra stats"; this
  behaviour now runs only with `coop2.off |= COOP_OFF_BOTHDOWN`). Pair rule (private vtable slot 14): the first boss of a pair to reach 0 HP dies quietly (list
  broadcast `0xF` so its bar empties, regular-enemy death routine: dust, SFX, removal, no action);
  if it was the original, the twin inherits the record action and becomes the original. The last one
  dies the normal boss way, which runs the fight's script. A partner-model twin switches back to the
  original model first so the script's sprite lookups find it. Twins' record copies point their
  action at a no-op pair, so a twin's death never fires the defeat dialogue. Verified on the Cell
  Games arena (14/3): both Cells at 5 HP, player 1 killed one (removed, registry shows the survivor
  as the original), then the other (stays at 0 HP, the arena cutscene started).
- Boss bars: in co-op both bars use a vtable copy whose draw is a port of the ROM's (two 32x8
  sprites, 60 px fill from the entity's HP and stat max) with the bar's own x / target y (bytes
  +0x19 / +0x1A of the object): x = 88 (the gap between the two player panels), the original's at
  y = 8, the twin's at y = 20. Single-player keeps the ROM object at x = 160. When the script finds
  the twin (the list search runs from the end) the first bar is still bound to the original.
  Verified with `bar(0)` on the Cell pair and on a Cooler pair (6/28, `spawnrec(1)`).
- Test commands: `coop2.cmd` (`0x0203FD08`) with `cmd_arg` takes the `COOP_CMD_*` numbers 1-14
  defined in `src/coop/coop.h` (10 and 12 unused); they run from the entity-list hook, so not during
  dialogue boxes. The list with arguments is in `docs/ids.md` "Test commands". `coop2.off` takes the
  `COOP_OFF_*` bits in coop.h. The Lua wrappers for the Windows mGBA console (`spawnrec`, `bar`,
  `actor`, `lowhp`, `tele`, `partner`, `coopoff`, ...) are in `tools/mgba/enemies.lua`.

## Two-player dash crash and the view limit (2026-10-17)

- Ryan hit "jumped to invalid address D061087A" when both players double-tap dashed at once. The
  player vtable at `0x08025604` has 48 slots (up to the walk vtable at `0x080256C4`); player 2's
  private copy held 24, so a dash/flight-state call past slot 23 read `coop` fields as code
  addresses. The copy is now `g_p2_vtable[48]` in its own pinned block at `0x0203FE00` (section
  `.coop.vt`, below the state block, above the heap); `coop_init_once` repoints a live player 2
  from an older savestate at it. Verified: simultaneous dashes left and right (into a map change).
- Separation: the leash now allows the whole 240 x 160 view minus a sprite half-width
  (`x in [left+4, left+236]`, `y in [top+20, top+158]`). More is not possible in the ROM: the GBA
  draws exactly 240 x 160 and the emulator only scales that image, so a player beyond it is simply
  invisible. Ryan wants 2-3x the width for big screens; that needs an emulator that renders a wider
  window of the map (a modified mGBA renderer plus ROM-side camera/leash/HUD changes); built the same
  day as `WIDE=1` with the WSL fork (`docs/widescreen.md`). An in-ROM split screen (per-scanline BG scroll via HDMA) is the
  hardware-only alternative but doubles every sprite and clips badly at the seam.

## Test tooling additions

- `tools/mgba/ents.lua`: `ents()` lists both players and every enemy (vtable `0x080252A4`) with
  position, base EXP and control state, plus the camera view origin; `ents(true)` lists everything.
- `tools/mgba/force.lua`: `p2_force = 0x8000 | keys` drives player 2's pad from the console (bit 15
  valid, bits 0-9 GBA keys), `p2_force = nil` returns control to the TCP bridge; `p2_play{{word,frames},...}`
  scripts a frame-exact sequence and `p2_dash(keys)` performs a double-tap dash. Useful with mGBA
  savestates (Shift+F1 / F1) to retry a fight without the debug-menu wait (about 75 s from power-on).
- `tools/mgba/enemies.lua` (loaded by coop.lua): `spd()` scans EWRAM for live chase modules and
  prints their speeds, `track(e, n)` logs an entity's displacement over n frames, `hits()` logs every
  party-slot HP change (damage per hit). `ents()` tags co-op enemies `CE` and twins `TW` and prints
  HP / max.
- `tools/mgba/cmd_server.lua` + `tools/mgba_cmd.py`: a TCP line server (7779) inside mGBA that runs
  Lua from the command line without touching the keyboard or the window focus. `seq("DOWN:4 WAIT:20
  A:4")` drives player 1's pad in frames through `g_p1_remote`, `p2seq` player 2's, `state(n)` /
  `save(n)` load and save slots, `shot(path)` screenshots, `flush()` returns what frame callbacks
  logged since the last command, `--wait` blocks until the pad queues drain. Load it once from the
  Scripting console; from then on nothing needs SendKeys, so another session's helper typing into
  mGBA no longer collides (two sessions driving one window by keystrokes garbled both).
- Debug-menu navigation that works from a fresh menu: Down, Down (Map Test row, Zone editable),
  Right = zone+1, Down (Area), Right x n, A. Keep a savestate of the menu itself (slot 3) so a Map
  Test costs seconds instead of the wait (30 s of title idle, about 75 s from power-on);
  `tools/mgba_input.ps1` knows F1-F9 for the slots.
- Typing into the Scripting console while another helper focuses the game window sends keystrokes
  into the game (one such slip opened the pause menu and quit to the title); keep console typing and
  game-window key presses sequential.

## Lockstep netplay (2026-10-17)

`tools/mgba/netplay.lua` + `tools/netplay_relay.py`: both players run the ROM in their own mGBA
(0.10.5 Qt, Lua scripting, no emulator rebuild) and exchange one pad word per frame; a frame runs
only once both pads for it are known, so the emulations stay identical. Verified on one PC with two
instances through the loopback relay: 11 400 lockstep frames (3+ minutes) of two-player movement,
attacks and the map-intro dialogue with 190 matching hash checks and 0 desyncs; single-shot
screenshots of both windows show the same scene while player 2 walks.

Design:

- **Both pads come from EWRAM.** `input_poll_hook` now also reads `g_p1_remote` (`0x0203FF02`, same
  format as `g_p2_remote`: bit 15 valid, bits 0-9 keys) and, when valid, uses it instead of
  `KEYINPUT` for everything downstream (player 1, the link-cable mirror word, controller-mode
  detection). The script writes both words at the frame boundary for the coming frame, so the
  physical buttons of either machine never reach the game directly.
- **Why not `emu:setKeys` for player 1?** mGBA's Qt frontend re-applies the keyboard state every
  frame in `CoreController::updateKeys` as `activeKeys |= core->getKeys() & ~m_removedKeys`
  (0.10.5), i.e. keys set by a script stick until the user *releases* that key, and the thread's
  own `videoFrameEnded` callback (which does that) runs before the script's `frame` callback. So
  after a `setKeys` override, `getKeys()` can no longer tell a physically held key from the
  override: the guest could not read its own pad. Feeding the ROM from memory keeps `getKeys()`
  pure on both sides (host = player 1, guest = player 2), at the cost of trusting that the game reads
  `KEYINPUT` only through `Input_Poll` (it does; everything else uses `g_InputSystem`).
- **Frame timing.** Lockstep frame F: at the end of frame F-1 each side sends its pad sampled now,
  tagged for frame F-1+delay, then blocks in the `frame` callback until the peer's word for F has
  arrived (`socket:select` on the raw socket, 20 ms slices, 10 s timeout), writes
  `g_p1_remote`/`g_p2_remote` and lets the frame run. The frame callback fires at the start of
  VBlank, before the game's own VBlank handler polls input, so both sides see the words at the same
  emulated point. Frames 1..delay-1 use empty pads on both sides.
- **Handshake / start state.** HELLO carries protocol, role, ROM CRC32 (`emu:checksum`) and delay;
  the host then `saveStateBuffer`s (save data + RTC, no screenshot: 405 568 bytes) and sends it, the
  guest `loadStateBuffer`s inside its frame callback and answers READY; both treat that boundary as
  frame 0. Loading a state from inside the callback is fine because the state was also taken from
  inside the same callback on the host (`_startHblank` schedules the next video event before
  `GBAFrameEnded`, so the continuation is identical). Fresh boot = the host resets before connecting;
  a map start = the host loads a savestate first. Same mechanism re-runs on a desync (`RESYNC`).
- **Desync check.** Every 60 frames an FNV-1a hash of both players' entity pointer, position
  (`+0x158/+0x15C`), control state and facing, the whole `CoopState` and `g_FrameCounter`; every
  300 frames the whole IWRAM too (`emu:readRange`). Hashes are exchanged (`H frame hash`) and
  compared when both are known; "in sync" is logged every 600 frames, mismatches immediately.
- **Transport.** mGBA's Lua sockets are non-blocking (`receive` returns `temporary failure`
  / WSAEWOULDBLOCK on Windows, mapped to `AGAIN`); a 400 KB `send` therefore needs the retry loop.
  Line protocol (`I frame word`, `H frame hash`, `HELLO`, `STATE n` + bytes, `READY`, `RESYNC`,
  `BYE`). The relay sets `TCP_NODELAY` and uses blocking threads per direction (a non-blocking
  `sendall` dies with WinError 10035 when the receiver is slower than the sender).
- **Stalls.** With both instances on one PC the host waited on the guest in about half the frames
  (the faster side always waits; `netplay_status()` shows how much), 60 fps on both. Delay 2 gives
  one frame of network slack; use 3-4 over the internet.

Lessons:

- A savestate made with an older ROM build crashes (`Jumped to invalid address 09000A42`) once
  player 2 acts: the private vtable copies in EWRAM hold the old blob's function addresses. Both
  instances crashed on the same frame, which at least confirmed the lockstep. Re-make states after
  `make`.
- An off-by-one that treated the input for the frame currently awaited as "late" stalled both
  sides until the 10 s timeout; inputs for `frame` itself are the ones we wait for.
- Debug menu controls (for making states): the cursor rows are Music, Sample, Map Test; on Map
  Test, Down cycles the edited field (Zone > Area > Variation, green; a further Down wraps the
  cursor to Music), Right/Left change it (changing Zone resets Area to 1), A starts the map, B
  leaves the menu back to the title (and the title's 30 s idle timer starts over).
- Test helpers: `tools/mgba_script.ps1 '<lua>' -Window N` opens Tools > Scripting by mouse (the
  menu bar ignores Alt mnemonics) and types the line into instance N's console;
  `tools/mgba_shot.ps1 out.png -Title 'Scripting*' -Of N` shoots that console;
  `Get-MgbaWindow -Title ... -ProcessId` / `Get-WindowPid` in `mgba_window.ps1` pick windows per
  instance. Capture both game windows with one `CopyFromScreen` to compare the same instant.

Open: a `ROM mismatch` check uses the CRC of the loaded ROM (so both need the same `make` flags,
e.g. `DEBUG_MENU=1`); mGBA's BIOS choice is not checked (HLE on both is the default); the guest's
save file is replaced by the host's snapshot.

## Plan for the two-Goku proof (Phase 2 start)

History (2026-10-17, resolved): the plan that opened Phase 2. It was carried out differently: the
control states were ported to C taking input per entity (no `entity+0x1A0` field), player 2 is
created lazily from `idle_update_hook` rather than by hooking `PlayerEntity_Create`, `Input_Poll` is
replaced by `input_poll_hook`, and the camera follows the midpoint (sections above). Kept as written:

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

## World map rule (Ryan, 2026-10-17)

History (resolved 2026-10-18): no code was needed; the stock behaviour already matches the rule (see
"World map" under "Player 2's combat stats, duplicate NPCs, test warp"). The rule as written:

Only the player who stepped on the flight sign controls the world map (zone 6 area 99); the other
player is a passenger and has no entity there. Both players land together at the destination. To
implement when the story scripts land: suppress player 2's spawn while the world map is loaded and
give the pad that initiated the flight player 1's input for its duration.

## Android 16 boss (2026-10-18, merged 2026-10-18)

Ryan's decision (`docs/story.md` 4e): 16 is the island boss Piccolo and player 2 fight next to Kame
House. What exists: sprite **160** (`tools/a16sprites.py`, `docs/sprites.md` "Android 16 boss
sprite"), enemy stat **109**, the AI in `src/coop/android16.c` with knobs in `src/coop/android16.h`.
The island story uses him: `data/story/a16_island.lscript` / `.json` (story-scripts, merged
2026-10-18) put stat 109 / sprite 160 on the island records (maps 296-298) and rewrite the Kame House
and island scenes. (When this section was written the island scripts were still stock.)

### How to place him (for the story work)

- An enemy record (`EnemyRecord`, handler `0x0800E77F`) with **stat 109** and **sprite 160** spawns
  him through `enemy_spawn_hook` (map load, or TriggerMapEvent opcode 84 with condition word 1 for a
  scripted boss). Stat 109 is past the stock table (0..108); the hook resolves it to `a16_stat`.
  He never gets a co-op twin (one boss; 17 and 18 watch).
- (Done by `data/story/a16_island.json`; kept as the recipe.) The stock island fight refers to the
  boss by sprite: map 296's trigger runs
  `DespawnCharacterAndRecordPosition [9, 1]` and `DrawBossHealthBarRange [9, 75, 100]`, and its
  defeat script moves char 9. A 16 fight needs those arguments to say 160, the record changed (map 296
  script 1 at `0x08406494`: `+8` stat 3 -> 109, `+0x10` sprite 9 -> 160), and the actor standing in
  for him before the fight (`actor sprite 8`) swapped with 17's (`actor sprite 9`).
- Android 16 in the stock game is **sprite 8** (the island actor), not 15 (that is Dr. Gero).
- History (superseded 2026-10-18 by "Android 16 ends at half health" under "Story-scripts engine
  pieces"): defeated (HP 0) he stayed on the map in his knock-down frame and fired nothing. Now the
  fight ends at half health (`A16_END_PCT` 50): at the bottom of each boss-bar stage the record's
  defeat action fires (`seq:a16_island_break`, then `seq:a16_island_half` in `a16_island.json`) and he
  stays standing.

### Attacks (special-attack scheduler between ticks of the stock boss AI set 21)

| attack | what happens | knobs |
|---|---|---|
| stock melee | AI set 21 (17's): approach, punches, kicks | `A16_MELEE` stat |
| heat-seeking rocket | charge pose (group 5) 30 frames, one missile locked on the **nearer player**, steering after them, explodes on contact or when the **three-second fuse** ends; below a third of his HP one rocket per player | `A16_HOMING_*` |
| rocket barrage | charge 24, a fan of 5 (7 enraged) fast missiles, weak homing, 80-frame fuse | `A16_BARRAGE_*` |
| area explosives | arms up 30, 5 (7 enraged) of the ROM's bombs dropped from 96 px: the first four around the players, the rest anywhere in view; 72-frame fuse after landing; big fire explosion (3 sprites, debris, shake) | `A16_BOMB_*` |
| rocket punch | group 13: wind-up 18 frames, the forearm flies straight along his facing, explodes on contact or after 60 frames; the arm returns 36 frames later | `A16_PUNCH_*` |

Choice by HP: above 2/3 homing 40 / barrage 20 / punch 40; between 1/3 and 2/3 25 each; below 1/3
homing 25, barrage 25, bombs 35, punch 15, and cooldowns times `A16_RAGE_PCT`%. A special starts only
from idle or walk, with a player within `A16_RANGE` px, after a cooldown of 150-240 frames.
**Poise**: a hit that stuns him opens `A16_POISE` (90) frames in which hits only do damage, and a
special cannot be interrupted, so two players cannot stun-lock him.

### Engine facts found for this (decompiled, verified in the fork)

- **Hit messages** (list broadcast `0x08006DA7`, message 0): `{u8 type; s32 x1, y1, x2, y2; u8 facing
  (+0x14); s32 mult 10.10 (+0x18); attack descriptor (+0x1C); attacker (+0x20); out crit, dmg,
  applied (+0x24..+0x2F); source (+0x30)}`. Players accept types 0 and 2 (`0x08008945`), enemies 1
  and 2 (`0x0800E5B9`); the ROM's bomb uses 2 (it hurts everyone), 16's blasts use 0. The receiver
  runs the damage calc on `msg+0x14` and OnHit on `msg+0x24`, which reads the applied damage at
  `+8` (`msg+0x2C`). Attack descriptors are `{u8 kind (1 = ranged), u8 reaction, pad, s32 base}`;
  `0x086DB2DC` is 17's melee.
- **Projectile class** (base init `0x08013BC1(obj, vram_bytes, owner)`, 0x58 bytes): +0x18 frame
  descriptor (raw tiles, DMA'd by slot 5 when it differs from +0x10), +0x20/+0x24 position 8.8,
  `0x08013BF5` updates the box and sort key, +0x1C owner (cleared by message 0xF), slot 2 update.
  The NPC ki blast (`FUN_08011BC4`, vtable `0x080259C4`) is a script effect that only reacts to
  message 0x11 from its target; the arcing bomb (`FUN_0801CCC8`, landing `0x0801CDFD`, explosion
  `0x0801CD33`: 4 debris `0x0800EC09`, dust explosion `0x0800FA1D`, SFX `0x14 + rand 2`) is the only
  damaging enemy projectile found.
- **Enemy control states**: `0x0840DB00` lists IWRAM state objects by index (0 idle, 1 walk, 2/3 hurt,
  6/7/8/10 = play group 6/7/8/10 once); entity +0x4C = group pointer (record + 0x4C + 0x10 * group),
  +0x14 frame, +0x12 timer; `0x0800763D` switches state (exit, set +0x18, enter). The engine calls
  state vtable slots above 3 (a 4-slot private state crashed with "Jumped to invalid address" after
  the radar screen), so copy the idle state's vtable `0x08024BA0` and replace slots 0-3.
- Enemy update = `0x0800772B` (aux + control state), then the AI tick `0x0801AB09(e + 0x19C)`.
- **Camera_Shake(cam, frames, strength 0-100)** links the camera object into the controller chain
  headed at `0x03000FF4`; calling it again while it shakes links it to itself (an endless loop).
  16 starts a shake only when the head is not the camera and otherwise extends the timer. (The
  player bump's unguarded call is fixed: `camera_shake_safe()` in `player.c`, and since 2026-10-18 a
  hook on `Camera_Shake` itself, `0x0800DD80` in `camera.c`, guards every caller.)
- OBJ VRAM free space = `0x08006D71(g_EntityList)` (largest free block); 16 skips a projectile's art
  when it would leave less than 0x100 bytes (the damage still happens).
- The scouter registration (message 0x14) searches the bestiary by stat pointer, so a stat outside
  the table simply has no scouter entry.
- **Player 2's combat bytes** (entity +0x160..+0x162: melee, ranged, defence) are player 1's
  character's, not their own (Hercule as player 2 at level 40 had 61/65/51 = Piccolo's). The tests
  below wrote the right bytes by hand. (Fixed 2026-10-18: `install_behaviour()` / `p2_sync_combat()`,
  section "Player 2's combat stats, duplicate NPCs, test warp".)

### Tuning and test results (fork, island area 6, Piccolo + Hercule at level-30 stats)

Level-30 stats were scaled from the debug party's level-40 values along each character's growth:
Piccolo 554 HP, str/pow/end 46.7/49.8/39.6; Hercule 245 HP, 47.0/45.5/63.0 (party entries and
entity bytes; savestate `build/a16/a16_base.ss1`, recipe in `docs/testing.md`). Each player hit takes
about 71 off 16 (a crit about 150). The test autopilot (`A16_TEST`, `coop2.a16_bot`) walks both
players to his sides and punches, steps out of bomb and rocket blasts and his swings, and backs off
below 35% HP; it uses no abilities and no items, so it is a lower bound on two people.

| 16's numbers (HP / melee / ranged) | result |
|---|---|
| 5200 / 34 / 40, no poise | stun-locked to 1321 HP in 30 s, then the rage bombs killed both |
| 5200 / 34 / 40, poise | both dead in 19 s (melee about 57 / 41 per hit on Piccolo / Hercule) |
| 7000 / 22 / 30 | Piccolo dead in 30 s |
| 5000 / 12 / 28, bombs x1.25, 9 rage bombs | 80 s, stalled at 1801 HP, both died |
| 4500 / 12 / 28, bombs x1.0, 7 rage bombs | lost twice, at 862 and 693 HP |
| **4000 / 12 / 28 (current)** | **won 3 of 5**: KOs at 28 s (Piccolo 143, Hercule 182 left), 31 s (232 / 138) and 35 s (38 / 159); lost twice, at 953 and 1179 HP |

Per hit now (co-op x1.25 included), Piccolo / Hercule: melee 17-22 / 12-13, homing rocket 35 / 31,
barrage 25 / 22, punch and bombs about 40 / 33. People who dodge and use abilities and items should
win more often; level 25 is harder (not measured). Raise `A16_HP` for a longer fight. Single-player
gets `A16_SOLO_HP_PCT` (70%) of his HP (not tested).

### Open

- Screenshots of a missile in flight: the in-fight captures show 16 between both players, the boss
  bar, hit flashes and explosions, but no frame caught a missile mid-flight; the paused-frame
  capture (`tools/fork/a16shots.sh`) made the fork exit twice (cause not found).
- AI set 21 drives his melee, so he keeps 17's moves (dash, kicks) rather than moves of his own.
- Resolved since: the island scripts now name 16 (`data/story/a16_island.*`); the player-2
  combat-byte bug and the Camera_Shake guard are fixed (see the notes above).

## Super Android 13 (2026-10-19, branch `super13-boss`, `src/coop/super13.c/.h`)

The post-game boss (`docs/story.md` 4i): after absorbing 17 and 18 at the arctic summit, players 1 and 2
fight him alone. Ryan's spec: a big towering boss with lots of health, new art and fighting moves, a full AI
strategy, slow, combinations of ki blasts and large ki blasts that partly home in on the players, a little
above the twin Perfect Cells, no reward beyond EXP, and he explodes when beaten. Art: `docs/sprites.md`
"Super Android 13" (sprite 180, portrait 122). Built on the Android 16 boss (section above): the same private
vtable, control state, projectile class, hit messages and poise, in a module of its own.

### How to place him (for the post-game chapter)

- An enemy record (handler `0x0800E77F`) with **stat 145** and **sprite 180**: `enemy_spawn_hook` sends it to
  `s13_spawn` (own stat row `s13_stat`, no co-op twin). TriggerMapEvent (opcode 84) spawns a condition-1
  record for a scripted boss; `DrawBossHealthBarRange 180, 0, 100` draws his bar (one bar, plain range).
- His **defeat action fires the moment his HP hits 0** (the boss death class), while he explodes for
  `S13_DEATH_FRAMES` (96) frames: pose group 12 (light bursting out of him) under explosions, then a big blast
  with debris and a strong shake, then an empty pose. The entity stays (scripts addressing sprite 180 still
  find him; `DespawnEntity 180` is safe), invisible and inert. A defeat script waits with
  `CoopCall 17, 1, 0, 0` (pushes his state: 0 none, 1 fighting, 2 exploding, **3 gone**) or about 2 s of
  frames before it goes on (17 and 18 die with him: the scene's own lines).
- `CoopCall 17, 0, hp%, dmg%` before the spawn scales his HP and every blast's damage for that fight (0 =
  100); `CoopCall 17, 2, 0, 0` spawns him beside player 1 with a bar (a scene without a map record).
- He is 64 px tall, drawn from his position to 32 px right (64 px frames from 16 px left), 48 px up and 16 px
  down; leave that much room (his hurt box is x - 6 .. x + 38, y - 46 .. y + 16).
- EXP: the stat row's EXP is 0, like every story boss's; the chapter awards EXP (Ryan: no other reward).

### Moves (a special-attack scheduler between ticks of the stock melee boss AI set 21)

| move | what happens | knobs |
|---|---|---|
| walking, melee | AI set 21 (Broly's, Cooler's): approach, punches, dash, at `S13_SPEED_PCT` (55 %) of the stock speed (`enemies.c` scales his chase modules) | `S13_MELEE`, `S13_SPEED_PCT` |
| ki volley | wind-up, then a small orb from alternate fists every 10 frames, **alternately at each player**, each steering a fraction of the way to its player every frame (turn divisor 26: stepping aside works), 4 orbs (6 enraged) | `S13_VOLLEY_*` |
| ki fan | a spread of 5 (7) small orbs at one player, weak homing | `S13_FAN_*` |
| deadly bomber | hunched, then arms up while a giant orb grows over his head (32x32, then 64x64 when OBJ VRAM allows), thrown after the **farther** player: slow (0.75 px/frame), homing (divisor 18), 5.5 s fuse, the biggest blast (3 extra explosions, debris, shake); enraged, one per player | `S13_BOMB_*` |
| ki wave | the crouching power-up with fire at his feet (48 frames, a rumble), then a slow beam head (32x32, 1.1 px/frame, partly homing) leaving a trail of ki orbs (one per 7 frames, 44 frames each) that burst on whoever touches them | `S13_WAVE_*`, `S13_TRAIL_*` |
| ki nova | he spins into a ki shell; at the peak a blast all round him (46 px) and a ring of 8 orbs flying out; likely when both players crowd him (within 60 px) | `S13_NOVA_*`, `S13_CROWD` |
| the roar | once, when he drops below a third of his HP: 48 frames of roaring with flames and a big shake, then **rage**: more orbs, sharper homing, cooldowns x 70 %, two bombers at once; it chains into a bomber | `S13_RAGE_*` |

Choice: above 2/3 HP volley 35, fan 20, bomber 15, wave 20, nova 10 (+60 when crowded); middle 25 / 20 / 20 /
25 / 10; below a third 20 / 15 / 25 / 30 / 10. **Combinations**: a special can chain into the next at once
(volley -> fan 40 %, 60 % enraged; volley -> bomber 20 % enraged; fan -> wave 20 / 45 %; wave -> volley 15 /
35 %), at most two links, the next one often at the other player. A special starts from idle or walking with
a player within 230 px of his centre, after a cooldown of 130-200 frames; the target is the nearer player 65 %
of the time. Specials cannot be interrupted, and a stun opens 60 frames of poise (hits only do damage), so two
players cannot lock him. Every blast is a type-0 hit message (players only); orbs in flight when he dies
fizzle without damage. Test command 18 forces a special (1 volley, 2 fan, 3 bomber, 4 wave, 5 nova, 6 roar).

### Engine facts found for this

- **Hurt box**: `Entity_GetBox` (`0x080079B8`) = position + `record + 0xC + facing * 0x10` ({x1, y1, x2, y2});
  the enemy message handler (`0x0800E5B9`) tests the attack box against it and needs entity `+0x168` set.
  A sprite's boxes must cover its drawn body in every facing: with Broly's side boxes (29 px, from x + 3) only
  1 of 33 player attacks landed (counted with `s13.msgs` / `s13.landed`).
- A 64x64 boss needs the record header's size word 0x1000 (`docs/sprites.md` "Paragus"). OBJ VRAM held up: the
  least largest-free block seen was 8 KB in the wide fork and 3.5-16 KB headless with both players, two 64x64
  bombers, orbs, a trail and explosions up (`s13.vram_min`); no projectile was ever skipped for want of VRAM.
- No lag: 600 game frames (`g_FrameCounter`) in 600 emulated frames during the rage, with two bombers and
  orbs up.

### Tuning and test results (headless, `tools/headless/bossfight.py`, New Planet Vegeta, Piccolo + Hercule)

The autopilot (`A16_TEST`) walks each player to a flank, punches, steps out of swings and orbs; no abilities,
no items. `--strong` = a Cell Games-level pair (our estimate: Piccolo 1500 HP, 72 / 75 / 62; Hercule 900 HP,
72 / 70 / 84; the debug party's level 40 dies to either fight in 15-30 s). `--refill` puts both back to full
every second (the pair never falls; rates over the active seconds).

| fight | refilled pair | strong pair, no refill |
|---|---|---|
| twin Perfect Cells (stat 25 x 2, both-down; 16460 HP) | 3 of 6 runs killed them in 22, 24 and 31 s by stun-locking them (players deal 530-740 HP/s, take 8-37/s); 3 hung (the crash below, fixed since) | won once at full HP (24 s, locked); otherwise hung, or KO at 26-27 s with the Cells at 74-88 % |
| Super 13 first cut (19000 HP, 66 / 64 / 68, specials every 60-120 frames) | not dead in 150 s: players deal 100-131/s, take 73-96/s (est. kill 145-190 s) | - |
| 15000 HP, melee 60, defence 62, blasts x 0.7, gaps 100-170 | killed in 90 and 131 s, take 54-76/s | - |
| 12000 HP, poise 60, gaps 110-180 | killed in 89-123 s, take 59-68/s | KO in 22-31 s, him at 65-70 % |
| 12000 HP, melee 52, defence 56, blasts x 0.6 more, gaps 130-200 | killed in 86-99 s, take 38-48/s | KO in 29-74 s, him at 29-65 % |
| **current: 10000 HP, 48 / 64 / 56, blasts x 0.75 more** | **killed in 58-88 s** (4 runs; players deal 113-168/s, take 27-37/s) | **KO in 37-96 s with him at 4-35 %** (443-3525 HP left; 8 runs) |

So the autopilot pair narrowly loses to him where it beats the twin Cells whenever it manages to lock them,
which is how "a little above" was read. Its damage rate on him (about 140 HP/s) is a quarter of what it does
to locked Cells: it spends most of the fight dodging his orbs, the fight Ryan described. People who dodge
better and use abilities and items should beat him more often; `CoopCall 17, 0` adjusts a fight without a
rebuild, and every number above is a knob in `super13.h`. HP 10000 is still more than one Perfect Cell
(8230). Not measured: single-player (`S13_SOLO_HP_PCT`, 55 % of the HP) and real two-person play.

**The twin-Cell "stall" was a crash, fixed 2026-10-19 (`src/coop/wrap.c` `attack_end`; it happens in real play).**
Every entity stopping, frozen popups, a running `g_FrameCounter` and a dead Start were the game after a jump through a
zeroed vtable (the headless runner prints `CRASH ... Jumped to invalid address`, LR `0x08006C05`); IRQs still run, so
the frame counter does. Traced with `st_headless.c`'s `watch` / `breakpc` / `trace` (2026-10-19):

1. The end of an attack (control state `0x08025D70`, slot 5, ROM `0x080085AD`) sends message 0x27, the special melee
   (pose group 16, `record + 0x14C`), when the state's +8 is set, A is held and **the active party slot's** flags have
   bit 2 (the special melee page); otherwise message 0 (idle). Player 2's call went through `call_as_player`, which swaps
   in player 2's input but not the active slot, so player 2 read **player 1's** page.
2. Hercule (slot 5, flags 0x43) has no group 16 (all four facings 0). Beside a player 1 with the page (the debug party's
   Piccolo, 0x7F; Super Saiyan 2 Gohan joins with 0x7F) his held A gave him group 16, and his frame pointer
   (`entity + 0x3C`) became 0.
3. The draw (`0x08007772`) decodes a changed frame into the staging buffer and DMA3s `w * h` bytes (frame bytes 2 and 3)
   to the entity's tile buffer (`+0x30`, sized from the record header: 0x400 for Hercule, at `0x0202E35C`, just below his
   entity). Frame 0 reads its size from BIOS address 0: 0x5416 bytes. The copy zeroed player 2, the HUD and player 1.
4. The next frame's draw pass (`0x08006AD8`: slot 3 on each entity, then slot 4 on the kept ones) called player 1's slot 4
   through the zeroed vtable (`0xE55EC002`).

Reproduced without the test's stat pokes and without enemies: the debug party in Map Test 147, player 2 holding A,
hangs in 26 frames (`tools/headless/p2_special.txt`). The poked runs were only where it was noticed (the autopilot
holds A through its punches). The fix reimplements `0x080085AD` for both players: it reads the attacking player's own
slot (`coop.p2_slot` for player 2) and gives message 0x27 only to a sprite with group 16 in all four facings. After it
the twin Cells died in 37-49 s in 6 of 6 refilled autopilot runs (delays 0, 13, 37, 51, 77, 99; 4 of 4 hung before),
player 1's held A still reaches group 16, and `cell_games.txt` passes. The "downed twin's pointer clears" and "the
autopilot stops hurting the survivor" readings were the same hang (HP frozen, pointers read after the wipe). A player 2
with the page and group 16 (Gohan beside Hercule) now gets its own special melee, which it never did before.

### Open

- Real two-player play.

## Boss rules and extra stats (2026-10-18, src/coop/bosses.c)

Story bosses are configured in `src/coop/bosses.c` (declarations and field meanings in `bosses.h`):

- **Extra stats** (`extra_stats[]`, index `EXTRA_STAT_BASE` 110 + i; `gero_stats[]` from
  `GERO_STAT_BASE` 120): a stock stat to start from plus overrides (AI set, HP, EXP,
  melee/ranged/defence). The spawn resolves it into the enemy's private stat copy, so no ROM table is
  copied into the build. 109 stays Android 16 (own spawn path in `android16.c`). Defined today:

  | stat | who | base stat (AI set, class) | HP, melee/ranged/defence |
  |---|---|---|---|
  | 110 | Android 13 beside 19 | 5 (19) | 3200, 22/16/14 |
  | 111 | the forest spirit's Cooler | 29 (Cooler) | 130, 3/2/3, 20 EXP (Frieza's tutorial numbers) |
  | 112 | Android 13 beside Dr. Gero | 5 | 2400, 24/20/16 |
  | 113 | Cooler's Namekian servant | 29 | 6000, 74/70/68 |
  | 114 | Android 13 in the arctic | 5 | 6000, 30/24/22; unused: the arctic encounter was removed 2026-10-18 (13 returns after Cell), the stat and its rule stay |
  | 115 | Broly | 29 | 8000, 52/54/50 |
  | 116 | Paragus | 29 | 2000, 24/22/20 |
  | 120, 121 | the bunker's two Android 19 bodies ("Gero's computer") | 5 | 3600, 23/17/16 each |

  117-119 and 122-124 have no entry (the Gero chapter reserves 120-124). Take new numbers from
  `docs/ids.md`, the registry.
- **Boss rules** (`rules[]`, by stat index, single-player and co-op alike): `end_pct` (the entity
  gets only the HP above `end_pct` and its bar is drawn over `[end_pct, 100]`, so the fight ends
  "at half health" with the bar half full), `partner_sprite` / `partner_stat` (the co-op twin's model
  and stat; 0 = same model, a Multi-Form duplicate), flags `BOSS_NO_TWIN`, `BOSS_PAIR` (a pair in
  single-player too) and `BOSS_RAGE` (see "Broly and Paragus"). The table in `src/coop/bosses.c`:

  | stat | rule |
  |---|---|
  | 3 (17) / 4 (18) | twin with the other's model (sprite 11 / 9); `BOSS_RULE_TEST` builds: 17 alone, ends at 50 % |
  | 5 (19) + 110 | `BOSS_PAIR`, partner Android 13 (`A13_SPRITE` 165, stat 110) |
  | 7 (Dr. Gero) + 112 | `BOSS_PAIR`, partner Android 13 (165, stat 112) |
  | 41 (Frieza, forest spirit) + 111 | `BOSS_PAIR`, partner Cooler (sprite 34, stat 111) |
  | 29 (Cooler, New Namek) | co-op twin is the Namekian servant (`NAMEK_SPRITE` 166, stat 113) |
  | 106 (Yamcha) + 98 (Tien) | gym duo, `BOSS_PAIR`, partner Tien's model 121 and stat 98 |
  | 64 (Krillin) | gym duo, `BOSS_PAIR`, partner Hercule's playable sprite 72 at Krillin's numbers |
  | 114 (arctic 13) | `BOSS_NO_TWIN`, ends at 50 % (no map spawns it now) |
  | 78, 79 (Perfect Cell) | `BOSS_NO_TWIN`, ends at 50 % |
  | 115 (Broly) + 116 (Paragus) | `BOSS_PAIR`, partner Paragus (`PARAGUS_SPRITE` 170, stat 116); 116 has `BOSS_RAGE` |
  | 120 + 121 (bunker 19s) | `BOSS_PAIR`, partner model 12 (19), stat 121 |

  Any other boss (Imperfect Cell 60, Semi-Perfect Cell 76, ...) gets the default co-op twin: a
  same-model duplicate with the original's stat. A partner model needs every pose group the boss's AI
  set plays (`docs/sprites.md` "Making a new enemy model").
- **Both-down rule** (all boss pairs): the first boss of a pair to fall takes the blow as a
  non-lethal hit (knock-back, damage popup), then HP 0, no action, list message 0xF (its bar lets
  go), plays pose group 3 (the knock-down: standing, falling, lying on 17, Gero and the Cell forms;
  19 has none) and freezes on the lying frame; its AI stops (vtable slot 2 wrapper) and further hits
  are ignored. The ROM's boss death is never called for it because that removes the entity. The
  survivor carries the original's record action; its death runs the fight's script as before. The
  survivor keeps its own model: the downed original is still on the map, so the defeat script finds it
  by its sprite (19's explodes it, Gero's flies it off and despawns it, Cooler's explodes it) and the
  partner (13, the Namekian) stays standing at HP 0 for the story script to address by its own sprite.
  (Until 2026-10-18 the survivor switched to the original's model before dying; with two entities of
  that sprite on the map Gero's script flew off and despawned the downed Gero and Android 13 vanished.)
  `coop2.off |= COOP_OFF_BOTHDOWN` (32) restores the old quiet removal, and with it the model switch.

Verified in the fork (island zone 11 area 6, `a16_base.ss1`, Map_SpawnScript 0 = the Android 17 boss
record): 17 + 18-model twin spawned; 17 knocked out first lay down and stayed (HP 0, frozen, no
action) while the twin fought on with 17's action; knocking out the twin switched it back to 17's
model and ran the stock defeat dialogue ("This is ridiculous. Piccolo was supposed to be nothing to
us!"). With the test rule (`make CFLAGS_EXTRA=-DBOSS_RULE_TEST`: 17 alone, end 50 %) 17 spawned
alone with 1775 of 3550 HP and his bar read 58 % at 300 HP. Test drivers used: `build/bdtest.sh`,
`bdfind.sh`, `bdpeek.sh` (not committed; the pattern is in `docs/testing.md`).

### Android 13, the Namekian servant and the boss-pair tests (2026-10-18, merged 2026-10-18)

Models: `docs/sprites.md` "Android 13 (id 165) and Cooler's Namekian servant (id 166)". Tested in the
fork (slot 3) on the island path (debug Map Test 11/6) with `COOP_CMD_SPAWN_RECORD` (8: any map's
boss record beside player 1, plus its bars; `tools/fork/bossfight.sh`, `docs/testing.md` "Boss pairs"):
Piccolo (player 1) and Hercule (player 2) at the level-30 stats of the Android 16 tests, the
`A16_TEST` autopilot (now fights any registered pair: player 1 the original, player 2 the twin).

| pair (record) | result |
|---|---|
| 19 + 13 (`0x083FD3BC`, map 7/15 script 2) | spawns with two bars; 13 (cap, vest, jeans) fights player 2. Autopilot: won at 50 s with both players up (19 fell first at ~20 s, 13 at 50 s); earlier tunings: won at 40 s once, lost twice at 155 s with 19 at 904 HP (13 already down) |
| both-down, 13 first | 13 lies in his group-3 pose with an empty bar; 19's death runs the stock dialogue ("Silly robot. You really thought you had a chance against me?") |
| both-down, 19 first | 19's defeat script (white flash, damage effect, velocity on sprite 12) explodes the downed 19; 13 stands at HP 0; the dialogue follows |
| Gero + 13 (`0x083FF970`, map 8/13 script 1) | autopilot: both down at 40 s (Gero first at ~20 s) with Piccolo at 0 HP; Gero's script (FlyToPosition 14, DespawnEntity 14, collision, ReloadMap) removes the downed Gero, 13 stays |
| Cooler + Namekian (`0x083FB574`, map 6/28 script 1) | spawns, fights, no crash; both orders of both-down work (Namekian lies down; Cooler's script explodes the downed Cooler and pans the camera). Level 30 is far below this fight (Cooler took ~150 HP/s off Piccolo with god-mode HP; the stock bonus fight is endgame) |
| Cooler + King Cold's model (`partner(74)`) | crash "Jumped to invalid address C002E564" at the first charge/attack: record 74 has 5 groups and set 21 plays groups 5-8 |
| Cooler + Android 17's model (`partner(9)`) | fights 40 s, no crash (the old note that any other model crashes was wrong) |

Tuning (bosses.c): 13 beside 19 = 3200 HP, 22/16/14 (19: 2970, 21/14/11): more HP than 19, his
damage, because the twin goes for the other player and 13 usually ends up on player 2 (Hercule beside
Vegeta in the story). 13 beside Gero = 2400 HP, 24/20/16 (Gero 1900, 25/21/17). The Namekian = 6000
HP, 74/70/68 (Cooler 10200, 75/85/79); at 68/60/64 he barely scratched Hercule. The autopilot only
walks up and punches, so these fights are a lower bound; the real level at the 19 fight is lower than
30 for Hercule, which is the next thing to check when the story's levels are known.

**Single-player (story-scripts, 2026-10-18):** stat 5 (19) and 7 (Gero) are `BOSS_PAIR` rules now, with
rules for 13's stats 110 / 112, so 13 fights beside them without a player 2 too: the chapter scripts
(13's lines, 13 flying off by sprite 165) need him on the map. Tested headless against one player;
the defeat runs as in co-op.

**Leftover bar (fixed 2026-10-18, story-scripts):** under both-down the first boss's bar keeps showing
empty (message 0xF clears its entity), but the stock death only removes the last boss's own bar, so
the empty one stayed on screen after every pair fight (19 + 13, Gero + 13). `coop_bar_update` now
slides a bar without an entity out once no co-op boss with HP is left on the map.

**Gym duos (story-scripts, 2026-10-18):** Roshi's sparring stats are pair rules: Yamcha (106) with Tien's
model and stat (121 / 98), Krillin (64) with Hercule's playable sprite 72 at Krillin's numbers (AI set 98
plays on the 21-group player set without a crash). `BOSS_PAIR`, so single-player fights the duos too.

**Found and fixed on the way:** player 2 froze whenever player 1 was in the hurt state
(`0x08025B98`): `p1_controllable()` whitelists player 1's control states to tell cutscenes from play,
and the hurt state was missing, so a boss pair pummelling player 1 also froze player 2. It is in the
list now (player.c).


**Broly and Paragus (2026-10-19, Broly chapter):** stats 115 (Broly: Cooler's AI set 21 and class,
8000 HP, 52/54/50) and 116 (Paragus: 2000 HP, 24/22/20), `BOSS_PAIR` with Paragus' sprite 170 as the
partner model. New rule flag **`BOSS_RAGE`** (on the partner's stat): when that boss falls while the
other still stands, the survivor rages once (`enemies.c` `rage()`): HP back to its max, attack bytes
+`BOSS_RAGE_MELEE` (10), defence +`BOSS_RAGE_DEFENCE` (6), chase modules created from then on x 13/10
(`speed_module`), and the dialogue set by `CoopCall 9, 0, seq:NAME` queued like a defeat dialogue. The
mark is bit 31 of the record copy's spawn-condition word. Then the both-down rule as usual. Verified:
headless, Broly at 3000 HP when Paragus fell went back to 8000 with 62/64/56; a natural autopilot fight
(test build, HP topped up) went Paragus down, the refill, Broly down, the ending; the same in fork slot
6. Unrefilled, the autopilot with the Map Test party (about 1000 HP) lost to the first tuning
(58/60/55, rage +14) in about 20 s. `COOP_CMD_PARTNER_WIN` (9) drops only boss partners (tests).

## Player 2's combat stats, duplicate NPCs, test warp (2026-10-18)

- **Player 2's combat bytes**: the behaviour classes write entity +0x160/+0x161/+0x162 (melee,
  ranged, defence = str/pow/end >> 8) from the *active* party slot (`0x08016FD8`), so player 2 used
  player 1's character's stats. The class init also adds the form's bonus (the Super Saiyan classes,
  e.g. `0x0801718C`: +15/+10/+15) and sets the speeds, so `install_behaviour()` (player.c) now runs it
  with the active slot temporarily set to player 2's slot, and `p2_sync_combat()` re-runs it whenever
  player 2's str/pow/end, row or slot change (level-up, capsule, transformation). Verified in the
  fork: Hercule as player 2 got 46/45/63 (his own) on load, and raising his strength by 2 in the party
  entry gave 48 within a frame, while player 1 kept Piccolo's 46/49/39.
- **Camera shake**: `camera_shake_safe()` (player.c) extends a running shake instead of calling
  `Camera_Shake` again (which links the camera into its own controller chain: an endless loop); the
  player's wall bump uses it. android16.c has its own copy.
- **Duplicate-actor rule** (`src/coop/actors.c`, run at the start of every entity-list update): a
  town NPC (vtable `0x080255CC`, map records with handler `0x0800B711`) that shows the character
  player 1 or player 2 controls is removed with `EntityList_Remove`, whose vtable slot 1 is the
  deleting destructor (teardown + free). Same person = the NPC's sprite record is the sprite of a
  display row reachable from the player's current row through the transformation links, or an
  NPC-only alias (`aliases[]`: sprite 76 -> Krillin, slot 6). Cutscene actors (handler `0x0800D6DF`,
  bytecode spawns) are not touched: scripts address them by sprite. `COOP_OFF_DUPES` (64) disables
  it; `coop2.dupes_removed` counts. Verified: zone 3 area 6 with Piccolo as player 1 lost Piccolo's
  NPC and kept Krillin's, Vegeta's, Tien's and the others.
- **World map** (Ryan's rule: one player flies): already the case. Player 2 is despawned by the
  warp and only respawns from player 1's on-foot idle state, which never runs while flying, so the
  world map has no player-2 entity and only player 1's pad drives it; both land together and player 2
  reappears beside player 1. Verified by warping to zone 6 area 99 (player 2 pointer 0, one flyer).
- **Test warp**: `coop2.cmd = 5` (`COOP_CMD_WARP`) with `cmd_arg = zone | area << 8` and
  `cmd_arg2` (CoopExtra +0x254) `= x | y << 16` calls the WarpToMapNoEntities opcode handler
  (`0x08009A89`, stack `{5, zone, area, 1, x, y}`) from the entity-list hook. Offsets: `coop2` is
  at `0x0203FB00`, `cmd` +0x208, `cmd_arg` +0x20C.

## Gero's bunker and Android 16 as a player (2026-10-19, merged 2026-10-19)

**Two Android 19s.** Ryan wants two 19s on screen, in co-op and alone. The bunker's record (map 257,
record 1) has stat **120** (19's stat 5 with overrides, in `gero_stats[]` of `bosses.c`, looked up
before the main extra table so the 115-139 ranges of the other side chapters merge without
renumbering); its rule is `BOSS_PAIR` with partner model 12 and partner stat **121** (the same
numbers), so exactly one twin spawns whatever the player count, and both-down holds the first 19
down until the second falls, when the record's defeat dialogue runs once. Numbers: 3600 HP, melee 23,
ranged 17, defence 16 each (stock 19: 2970, 21/14/11).

Fork tests (slot 5, `A16_TEST=1` autopilot, Krillin and 16 at the debug party's level 40: Krillin 625 HP,
16 about 1000; `bossfight.sh 0x08B0DEA4`, the record address of that build): 4000 HP at 28/20/18: one
19 down at 24 s, the other at 799 HP when Krillin died at 30 s; 25/18/16: Krillin died at 30 s;
20/15/16: both down at 32 s, Krillin at 488; **23/17/16** (kept): both down at 32 s with Krillin at
438 and at 326 in two boots. The twin goes for the other player (16), the original for Krillin. Runs
started from a savestate taken right after the party switch sometimes stalled with every boss idle
(the stock 19 + 13 pair too), never from a fresh boot: a test-state artefact, not seen headless.

**Rocket Punch (ability 22).** A projectile of the ROM's class (like Krillin's disc) with the boss's fist
frames, 5 px per frame for 40 frames, one hit on the first enemy it touches, then the fire explosion
hits every other enemy within 20 px. Its attack must be the **melee kind** (`{0x0400, 0x500}`):
**Android 19 absorbs ranged-kind hits** (a ranged fist landed four hits without damage), so ki blasts,
the Destructo Disc and 16's Big Bang do nothing to the 19s, which suits the dialogue ("I will do the
punching"). Measured: three punches, five hits, about 310 HP each against the 19s at level 40.

## Player-vs-player duel (2026-10-18, src/coop/duel.c)

For the story's two duels (Vegeta vs his shadow clone in space, Vegeta vs Trunks in the Time Chamber).

- **Hits**: player attacks broadcast hit messages of type 1, which players ignore (the player
  message handler `0x08008945`, vtable `0x08025604` slot 9, takes types 0 and 2). `player_msg_hook`
  (hooked on slot 9 for both players; player 2's vtable copy inherits it) handles a type-1 hit whose
  attacker is the opponent (or a projectile owned by the opponent, +0x1C) as type 0 for that player
  only: box test, damage calculation with the attacker's own combat bytes, knock-back, OnHit (slot 14,
  `0x08008807`, which calls take-damage slot 16).
- **Final blow**: player 1's take-damage (slot 16, `0x08008785`: HP -= dmg from the *active* slot's
  entry, death animation 0x21 and the game-over event at 0) is hooked (`p1_take_damage_hook`): in a
  duel the blow is trimmed to leave 1 HP and the duel ends. Player 2's own take-damage asks
  `duel_p2_final_blow` first. No game over, no revive.
- **End**: `coop2.duel_winner` = 1 or 2, the pads swap when player 2 won and `DUEL_SWAP_ON_P2_WIN`
  is set (`coop2.pad_swap`, applied in `input_poll_hook` while a player-2 source exists), and the
  duel's end script (a mapScripts index, TriggerMapEvent style) runs.
- **Clone**: `duel_start(-1, flags, end_script)` copies player 1's party entry into the last party
  slot (`DUEL_SLOT`, fixed at 7 in `rom.h` since slot 8 is Super Saiyan 2 Gohan; slot 7 is reserved for duels; during the "Gero's computer"
  chapter it also holds Android 16 as a player, `A16_SLOT` in `a16player.h`, since no duel runs
  there) and binds player 2 to it;
  `DUEL_SHADOW` draws player 2 through a 256-entry colour remap toward darker violet palette colours
  (entity +0x24, decoded per frame by the sprite emitter `0x08007772`; the hit flash borrows the same
  field and clears it, so `duel_tick` puts the tint back). `duel_clear()` drops the clone slot and the
  tint once the story's end scene is over.
- **Flags**: `DUEL_CLONE` 1, `DUEL_SHADOW` 2, `DUEL_SWAP_ON_P2_WIN` 4, `DUEL_FULL_EP` 8 (EP held full:
  no Super Saiyan drain during the fight). Debug: `coop2.cmd = 6` (`COOP_CMD_DUEL`), `cmd_arg =
  (p2_slot + 1, 0 = clone) | flags << 8 | (end_script + 1) << 16`.

Verified in the fork (island save, Piccolo + Hercule): a clone duel turned player 2 into a shadow
Piccolo; each player's punches hurt the other (554 -> 485 and 554 -> 414); player 1's final blow left
the clone at 1 HP with `winner = 1`; in a rematch player 2's final blow left player 1 at 1 HP, no game
over, `winner = 2`, pads swapped (player 1's pad then walked player 2's entity).

## Story-scripts engine pieces (2026-10-18, merged 2026-10-18, `docs/scripts.md`)

- **Boss bar argument fix**: the entry jump into `bossbar_create_hook` was `ldr r3,[pc]; bx r3`, which
  replaced `BossHealthBar_Create`'s fourth argument (r3 = the upper percent of
  `DrawBossHealthBarRange`) with the hook's address. `patch_rom.py` now jumps through r12 for the
  entries in `R3_ARG_JUMPS` (push {r0}; ldr r0,[pc]; mov r12,r0; pop {r0}; bx r12).
- **Camera_Shake hook** (0x0800DD80, `camera.c`): a shake requested while the camera already heads the
  shake chain (0x03000FF4) extends the running one instead of linking the camera to itself; this was
  a crash when a player was hit during one of 16's explosion shakes (ROM callers included, beside
  `player.c`'s `camera_shake_safe`).
- **Android 16 ends at half health** (`A16_END_PCT` 50, `android16.c`): `A16_HP` is what the players take
  off over the whole fight; a bar range drawn for him slices his nominal health (stage [75, 100] and
  [50, 75] on the island, 2000 HP each with two players, 1400 alone); at the bottom of a stage the defeat
  action fires and he stays standing (idle state, AI off, hits ignored); the rage phases follow the whole
  fight.
- **`BOSS_PAIR`** (`bosses.h`): a pair even in single-player, under the boss rules whatever its HP and
  EXP: the forest spirit's Frieza (stat 41) + Cooler (sprite 34, stat 111 = Cooler's AI with Frieza's
  tutorial numbers).
- **Player 2 by party flags** (`player.c`): player 2 is a party member with bits 0 and 1 (Hercule is no
  longer allowed outside the party), so the story's flag opcodes pick and lock player 2.
- **Chapter tests**: `StoryTest` preset (0x0203FDC0, applied by Party_Init), `COOP_CMD_STORY_WIN` (7),
  `STORY_TEST=1` builds; `tools/headless/st_headless.c`.

## Time Chamber training waves and Roshi's dojo spar (merged 2026-10-19)

Story side: `docs/story.md` 4g. Both open in the ten-day wait.

**Training waves** (`src/coop/waves.c`, `data/story/wait_chamber.*`, map `data/maps/time_chamber.json`).
From story flag 157 (Cell's TV announcement) the Lookout's chamber door (zone 9 area 3) warps the
players into the chamber (zone 15 area 2) and calls `CoopCall 13` (WaveStart: the round sequence
`seq:lk_tc_round`, first round 1). Ten fixed rounds of stock enemies follow, four records per round,
harder as they go; the records are built in RAM around the players' midpoint and spawned through
`enemy_spawn_hook`, so in co-op each brings its twin (eight on screen). The controller is a small
state machine run every input poll (`waves_tick`): CLEAR (a 75-frame pause after the last kill, then
the round number goes up and the round sequence is queued like a defeat dialogue), ANNOUNCE (the
sequence shows the "Round N" caption and calls `CoopCall 14, 1` = WaveSpawn; `CoopCall 14, 0` returns
the round), FIGHT (until no enemy is left standing), DONE (after round 10 the sequence says "Training
completed!" and nothing more spawns). Leaving the map (the chamber's door, a game over) switches it
off; the Lookout door starts again from round 1. Rewards: EXP from the kills only. State: `waves`
(`WaveState`, 0x9C bytes) in section `.party.wv` (`0x0203F880` in the merged build; read `coop.map`).
Test commands: `COOP_CMD_WAVE_CLEAR` (13) and `COOP_CMD_WAVE_SKIP` (14, arg = round).

The rounds (`rounds[]` in waves.c; stock stat / sprite, HP before the co-op x1.5; copied from
`docs/story.md` 4g, the code is the source):

| round | enemies | HP |
|---|---|---|
| 1 | 2 wolves (57 / 148), 2 hawks (107 / 64) | 675, 700 |
| 2 | 2 sabre-toothed tigers (92 / 108), 2 Kuma mercenaries (70 / 91) | 750, 940 |
| 3 | 2 ninjas (75 / 96), 2 pterodactyls (80 / 105) | 900, 900 |
| 4 | 2 Warlord's henchmen (12 / 142), 2 warthogs (56 / 145) | 900, 900 |
| 5 | 2 hawks (84 / 65), 2 wolves (104 / 149) | 1000, 1000 |
| 6 | 2 Warlord's henchmen (51 / 143), 2 tiger bandits (10 / 19) | 1100, 1120 |
| 7 | 2 Eggbots (40 / 47), 2 pterodactyls (88 / 106) | 1130, 1200 |
| 8 | 2 ninjas (86 / 98), 2 Eggbots (39 / 46) | 1400, 1349 |
| 9 | sabre-toothed tiger (89 / 110), 2 scorpions (47 / 112), ninja (45 / 97) | 1750, 2000, 1600 |
| 10 | snake (48 / 117), 2 wolves (50 / 147), Destroyer (32 / 38) | 3530, 3980, 4200 |

Sprite memory, not a count, is the limit: with all eight standing the largest free OBJ VRAM block
stayed above 6.9 KB; two Destroyers per round had left only 2.2 KB, so round 10 keeps one
(measurements in `docs/story.md` 4g).

**Roshi's dojo spar** (`data/story/wait_spar.*`). Once Roshi's gym matches are cleared and the last
capsule is paid (story flag 214), talking to Master Roshi in the Capsule Corp atrium (zone 6 area 3)
ends with his offer: with player 2 present (`CoopCall 5`) he books the dojo (story flag 440); alone, he
says you need a partner. Walking onto the sparring mat (zone 6 area 70) starts a 1 v 1 player duel
(`CoopCall 1` DuelStart with player 2's own slot from `CoopCall 8`, flags 0: no clone, no pad swap,
no stakes; duel mode as in "Player-vs-player duel"). The final blow leaves the loser at 1 HP; a
caption names the winner (`CoopCall 3` gives the winner, `CoopCall 14, 2` player 2's name), the duel
is cleared (`CoopCall 2`), both players are healed and the fade returns them to the atrium. The match
clears flag 440 as it starts, so each spar needs a new talk with Roshi.

## The Cell Games: solo fights, held beams, the top-edge leash (2026-10-19, `src/coop/cellgames.c`)

- **Solo fights (CoopCall 15, `COOP_CMD_SOLO`).** Mode 1: player 1 fights alone; mode 2: player 2 fights alone
  on player 1's entity. While set, player 2's entity is removed (`EntityList_Remove`, its destroy clears
  `coop.p2`) and `ensure_p2` does not respawn it, the twin spawner treats the pad as absent (one enemy, one boss
  bar), and `input.c` routes the pads (mode 1 drops player 2's, mode 2 gives player 2's to player 1's entity).
  So a solo-2 loss is the normal game over, as Ryan wanted. The mode lives in `.party.cg` with a magic word
  (partyram is not cleared at boot) and is cleared whenever the map is not zone 14 areas 2-4.
- **Held beams (CoopCall 16).** Step 137 `NPCFireKamehameha` appends a beam command to the entity's command
  chain (entity +0x150, next pointer at +4), and the script then waits for it (or a batch's commit does, and a
  commit waits for every pending command). A beam held through dialogue therefore cannot be a command:
  `cg_beam` does the command's work directly (`Entity_GetBox` 0x080079B9, `Beam_Init` 0x0801D84D, the list add,
  animation 30, `Entity_BeamPoint` 0x08007A27 plus the per-facing offsets at 0x08701DF0, `Beam_Fire`
  0x08014479; `Beam_End` 0x0801451D to stop). A beam object fades on its own, so the struggle refires them.
  The stock clash beam between Gohan and Cell is a decoration of map 326, not a beam.
- **Entities by sprite.** The VM finds "the entity showing sprite N" by its record pointer (`0x08009370`:
  0 = player 1, 1-6 = party slots, else `sprite_index[N]`, then message 5). A second actor of the same model
  needs its own record: sprite 162 is a copy of Perfect Cell's record 30 (`tools/spriteclone.py` `162=@30`).
- **The leash at a map's top edge.** `leash_p2` kept both players 20 px below the view's top, so at a map's top
  edge (view top 0) a door whose rect ends above y 20 could never be reached in co-op (the Cell Games ring,
  (232, 0)-(281, 17); Gero's lab area 47, which got a workaround exit). The head room now applies only when the
  view is scrolled.
- **Test win (`COOP_CMD_WIN_STAT`, 16).** A queued stat wins its fight once an enemy of it stands and player 1
  is in a player-driven state for half a second; it matches coop enemies by their record copy's stat and stock
  enemies by their stat pointer.

## Revive (2026-10-20, `src/coop/revive.c/.h`)

Ryan: a downed partner is revived by holding A beside them, with a visible timer; too late is a game over.

- **Going down:** both players' take-damage hooks (`p1_take_damage_hook` in `duel.c`, `p2_take_damage` in
  `player.c`) ask `revive_hit` first. A lethal hit knocks back, sets HP 0 and plays the stock death state
  (message 0x21, what player 1's own stock death plays: `FUN_08008784` sets `+0x168` = 0, message 0x21, and
  queues the game-over event) without the event. The hook returns 1 ("died"), so OnHit (`FUN_08008806`) plays no
  hurt state. The body takes no more hits; the partner's getting-up frames (`REVIVE_SAFE`, 90) take none either.
- **While down:** the downed pad is zeroed (`revive_input`, in the input hook after the solo switch); enemies
  turn to the partner (`coop_target`; Super 13 and Android 16 through their `real_player`); player 1 lying down
  counts as controllable for player 2's input (`p1_controllable`). The countdown is the difficulty's
  (`DIFF_REVIVE_S`: 20 / 10 / 6 s).
- **Reviving:** the partner within `REVIVE_REACH` (28 px) holding A: the A is taken from the game (no punch)
  and the progress counts to `REVIVE_HOLD` (180 frames); letting go, or a hit on the partner, starts it over.
  Then HP = `REVIVE_HP_PCT` (30 %) of the maximum, message 0 (idle), and the bars go.
- **Game over:** the countdown runs out, or the partner goes down too (player 1 last: its own stock death;
  player 2 last: `game_over()` drops player 1 the stock way and queues the stock event, `{0x08023A50, 200}` on
  the entity list's events, which shows "You have died!").
- **Scenes:** a dialogue sequence sets `0x03001FDC` (`0x0800B4AE`, cleared by `0x0800B4FC`); while it is set the
  countdown stops, and a downed player gets up at once (a boss's death scene, a trigger). A map change that
  replaces the body (player 1's entity, player 2 respawned) puts the player back at 30 %.
- **Not here:** duels (1 HP, `duel.c`), the Cell Games' solo fights, single-player (no player 2): the stock
  death, and player 2's old half-HP respawn where the revive does not apply.
- **Bars:** `revive_draw` from the HUD draw hook: 8bpp 32 x 8 sprites (the game draws its sprites 8bpp in one
  OBJ palette: 1 black, 2 white, 5 red, 8 yellow, 14 dark grey) above the body, from the camera's view origin
  (`CAMERA_CELL` +0x1C / +0x20): the countdown (red, the seconds in a 3 x 5 white font, blinking white for the
  last 3 s) and, while the partner revives, a yellow bar under it. 0x200 bytes of OBJ VRAM while someone is down.
  The body flashes bright while the revive runs: a remap toward white (entity +0x24, as the duel's shadow),
  built 16 colours a frame into an arena block.
- **Sounds:** going down 0x22 (an impact), getting up 55 (the transformations' power-up); change in `revive.c`.
- **Test:** `tools/headless/revive.txt` (test command 10 `DOWN`): player 1 revives player 2 (30 %: 147 / 493),
  player 2 revives player 1, the countdown runs out into the game over. Also checked beside Super 13 (he turns
  to the standing player; the body's HP stays 0; "You have died!" at the end).
- **State:** `.party.rv` `rv` (0x34 bytes, after `.party.s13`): bodies, countdowns, progress, getting-up frames,
  the bars' VRAM, the glow table, counters (`downs`, `revives`, `timeouts`).

## Difficulty (2026-10-20, `src/coop/difficulty.c/.h`)

Picked once, at a new game's first scene (`data/story/ch1_opening` `ch1_training`: "Play on Normal?" then "Easy
or Hard?", CoopCall 18 questions); story flags 460 Easy / 461 Hard, neither = Normal (older saves play Normal).
Normal is exactly the co-op tuning from before.

| | Easy | Normal | Hard |
|---|---|---|---|
| regular enemies' HP (with player 2) | x 1.0 | x 1.5 | x 2.0 |
| damage enemies deal the players (with player 2) | x 0.75 | x 1.25 | x 1.5 |
| regular enemies' walk speed (with player 2) | x 1.0 | x 1.2 | x 1.3 |
| twin regular enemies | no | yes | yes |
| boss HP (every boss, also Android 16 and Super 13; never below 1000 for a boss that had it) | x 0.8 | x 1.0 | x 1.25 |
| revive countdown | 20 s | 10 s | 6 s |
| drops | a second roll when the first gave nothing | as stock | one roll in three skipped |

Drops: a regular enemy's record action is the bytecode action (`0x0801069B`) on one of five stock drop
scripts (`StackRand 100` against thresholds, then `SpawnItem`: `0x083EE120`, `0x083EEDE4`, `0x083FEB28`,
`0x083FFAF4`, `0x083FE538`; every map's defeat scripts classified, the rest are story scripts and stay as they
are). `create_coop_enemy` points such a record copy at `drop_action`, which runs the same script once, twice
(Easy, when the entity list did not grow) or not at all (Hard, 1 in 3). Single-player enemies keep the stock
records. The table's numbers are the macros at the top of `difficulty.h`. Test: `tools/headless/difficulty.txt`.

## One player, and player 2 dropping in and out (2026-10-20)

**Player count (Ryan, 2026-10-20): asked at every game start** (`src/coop/players.c`). The Select Game screen
(`save_menu_load_hook`, which every new game and loaded save passes) arms it; the first moment player 1 can
move, "How many players?  (A: 1 Player  B: 2 Players)" (a sequence built in C, CoopCall 18 + CoopCall 21).
1 Player keeps player 2 out even with a second controller (`p2_present()` reads 0, so a player 2 already
there drops out); 2 Players is the pad's rule below. Not saved. Before any Select Game (power-on, the debug
menu's Map Test, every headless chapter test) nothing is asked and the pad decides. Tested on the play ROM
with a second pad: A -> mode 1, no player 2; B -> mode 2, player 2 joins.

With 2 Players, player 2 exists while a second controller sends input (`p2_present()`, the 3 s source timeout
in the default `P2_MODE_SIO` build). Without one, the game is single-player. Audited 2026-10-20
(every story scene and every co-op module); fixed:

- **The Cell Games' player-2 fights** (fight 2, J2 Piccolo, J4 Trunks: solo mode 2 hands player 2's pad to player
  1's entity): alone, player 1's pad plays them (`input.c`). The title cards are CoopCall 14 op 3 (`cg_card`):
  "Player 1 fights!" / "Player 2 fights!" / "Players 1 & 2 fight!" with two players, "<character> fights!" alone
  (built in `.party.ui`).
- **Drop-out:** when player 2's pad goes away, its entity leaves at the next free moment (`p2_drop_out`: player 1
  controllable, no scene, no solo fight) instead of standing frozen; `ensure_p2` brings it back with the pad. A
  player 1 lying downed when player 2 leaves gets the game over (nobody can revive); a downed player 2 just goes.
  A queued game over keeps the body down (`revive_tick` checks `over` before the scene / no-partner get-up).
- **`p2_coming()`** (player 2's entity, or a live pad and a party member it can play) decides twins, enemy
  scaling, the boss bars and Android 16's / Super 13's solo HP. The pad alone counted before, so a pad with no
  playable member (one-member party) got twins.
- **Story pairs alone** (Ryan: keep them): the `BOSS_PAIR` fights spawn both bosses in single-player too, now at
  `SOLO_PAIR_HP_PCT` (70 %) each, and with two bars as in co-op (one bar before, for the first boss only).
  `BOSS_PARTNER_ONLY` marks the rows of a pair's second boss (13 beside 19 / Gero, Cooler, Tien 98, Paragus, the
  second bunker 19, the second Cell) so a record of that stat on its own is not paired with a copy of itself.
- **CoopCall 14 op 2** (player 2's name) reads player 1's when there is no player 2 or no valid slot (the slot is
  0xFF after a fresh boot: it read past the name table).
- **Duels** (CoopCall 1) that cannot start after all (player 2 left, no slot) take the single-player path: player
  1 wins and the end sequence runs. Alone, the space duel and the chamber duel play a short flash and shake in
  place of the fight (`ch3_duels`).
- **Scenes:** the summit's "%s: Let's take him down together!" plays only with player 2 (`arctic_summit` [14]);
  the dojo mat needs a second player (`lk_spar_cond`); the gym's "Two against two!", "Good teamwork" and "You make a
  fine team" now read for one player or two.
- Already fine alone (checked): every `CoopCall 5` branch in chapters 4-8, the sideline / watcher guards, PartyKeep,
  Stage, the held beams, Choice, revive (stock death without player 2), Android 16 and Super 13 targeting.
- **Balance (2026-10-20, a review of the numbers):** alone, a story pair fights at `SOLO_PAIR_HP_PCT` 50 % each
  (70 % made the final twin Cells about 2.2x the co-op load, with no healing) and Android 16 at
  `A16_SOLO_HP_PCT` 45 %; with two players a lone boss (`BOSS_NO_TWIN`: the Cell Games' fights 3 / 7, Cell vs
  Trunks + Krillin) has `COOP_LONE_BOSS_HP_PCT` 180 % (stock HP made the pair's fight half as long). Alone the
  difficulty has its own numbers (`DIFF_SOLO_ENEMY_HP` 100 / 100 / 130, `DIFF_SOLO_DAMAGE` 75 / 100 / 120; Normal
  alone is the stock game) and Easy / Hard enemies alone get the drop roll. Halving the players' damage in
  co-op (Ryan asked) was rejected: it doubles every co-op fight (twins at x1.5: 3x a one-player fight) and the
  bosses were tuned against two full-strength players (Android 16, Super 13, the twin Cells).
- **Revive fixes (2026-10-20 bug review):** player 2 leaving while down gets its HP back (30 %) and the glow table
  freed; player 1's own death with player 2 down marks the game over (no second event); a save-pad trade onto a
  downed player 2's slot gets it up first; a duel in progress when player 2 leaves ends as player 1's win
  (`duel_p2_left`); the revive's A is taken only while player 1 is in play (`p1_in_play`: menus keep it); the
  glow clears the cached frame (+0x20) so the remap shows; a downed player 2's form change keeps the pose.
  Untested: going down while charging a held beam (the forced exit that once crashed `p2_cycle`).
- Base Android 13 has no dialogue portrait yet (123 is allocated); he speaks as narration.
- **The sprint bump (fixed 2026-10-20, Ryan's report):** our port of the 4-way walk (`player.c` `walk_update`,
  which only the sprint uses) passed 0 as the collision check's box where the stock passes the entity's box
  (+0x154, `r2` at 0x0800816E): `0x08006366` read a box from address 0 and the sprint bumped on open ground.
- **The story's lead and companions (2026-10-20, `src/coop/companion.c`; Ryan: no swapping, no follower in free
  play):** the lead is the character the story scripts last made active (VM steps 70 SetActiveCharacter and 115
  ...NoReset are repointed to `vm_set_active_hook` / `vm_set_active_noreset_hook`, which note the slot in flags
  464-467, saved with the game; none noted, as in an older save, reads Gohan while he is in the party). Stock
  scenes are written for the lead: their speaker-1 lines (the active character's box) are his. So:
  * **portraits:** the portrait getter (`0x0800BDCC`, re-implemented as `portrait_get_hook`) gives speaker 1 the
    lead's portrait when someone else plays (alone and in co-op); speakers 2-7 stay party slot id - 2;
  * **the lead as an actor (Ryan: walk or fly in like the others, never pop in):** when nobody plays the lead,
    our scene scripts stage him as an ordinary actor who walks and flies in with the rest of the cast. The steps
    are guarded by CoopCall 22 (op 1 slot: in the party and played by neither player, player 2 counting while a
    second player is in even before his entity respawns after a warp; op 2 sprite: such an actor stands on the
    map; op 3: the lead's name in capitals for "%s: Hey dad!"). Done for the Frieza stretch (`ch1_opening`):
    kid Gohan in armour (sprite 55) walks up with the Z fighters at the landing (map 52), stands with them on the
    return, flies in with the group at the Goku-arrival site (map 54), walks up to Trunks for "how do you know my
    dad?", kneels beside the others when Goku lands and turns to him. Nobody is swapped; nobody follows in free
    play; the sequence start / end are stock again;
  * **old Hercule (alone):** while party slot 5 shows him (row 24, the future prologue) he walks along with player
    1 (player 2 with a follower pad: toward player 1 beyond 28 px, a jump beside him when walled off far behind).
  A companion is no player for the rules: `p2_coming()` is 0 (no twins or co-op scaling), enemies ignore him
  (`coop_target`), he takes no hits, the damage and speed multipliers and the revive see one player, CoopCall 5
  reads 0 and duels are skipped; his HUD panel is hidden; a real second controller takes over (2 Players). A
  downed player 1 whose player 2 leaves for a companion is a game over. Our scripts' per-character player lines
  (the summit) use their own speaker ids, not 1. Tests: `tools/headless/scene_actor.txt` (the landing in four pairings and the
  arrival alone: the actor's count by `entrec`), `prologue_companion.txt` (the play ROM). Old Hercule's lines use portrait 126 (Buu's Fury's Hercule,
  `tools/oldherculeportrait.py`).
- **Known, kept:** speakers who are player 2 in co-op talk off screen alone (old Hercule in the prologue, Krillin,
  Hercule, Android 16 in "Gero's computer", who is only in the party). The test runner's `stepz` no longer presses
  player 2's pad after `p2 off`, so one-player runs stay one-player.
- Tests: `tools/headless/cell_games_1p.txt` (the whole Games alone: every fight won, "Goku fights!", player 1's A
  punching as Goku in fight 2), `tools/headless/dropout.txt` (leave after 4 s, come back, a downed player 1 when
  player 2 leaves = game over); the summit, post-game and Gero chapter runs pass with `p2 off` too.
