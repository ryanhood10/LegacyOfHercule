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

Live inspection without pausing: `dofile("â€¦/tools/mgba/peek.lua")` in the Scripting console, then
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
  into the party late in the story, so `slot_ok` accepts slot 5 whenever its `max_hp` is set. Player
  2 can still cycle to other available members with Select and comes back to Hercule by cycling.
- Verified from the prologue Flight Circle savestate: Hercule appears with his own panel, flies with
  Trunks and respawns on him at the landing spot. Player 2 spawns 12 px left of player 1 when that
  spot is walkable, else exactly on player 1 (overlap beats leaving the map).

## Plan: co-op difficulty, twin bosses (stages 1-4 done, see the next section; stage 5 tuning open)

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

- `.coop2` block at `0x0203FB00` (heap size word now `0xFCC0`, reserving the top 1.25 KiB): enemy and
  boss-bar vtable copies, a 24-pair twin registry and counters (`ents()` prints them).
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
| Z6 A70 (6/31) | 106, 64, 98 | 156, 76, 121 | 800, 1800, 2110 | tournament gauntlet (EXP, condition 1): Yamcha?, Krillin, Tien |

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
- Pair rule (private vtable slot 14): the first boss of a pair to reach 0 HP dies quietly (list
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
- Console test commands (`coop2.cmd`, run by the entity-list hook, so not during dialogue boxes):
  `spawnrec(i)` = TriggerMapEvent i, `bar(e)`, `actor(sprite)` (grid of 12 around player 1),
  `lowhp(n)`, `tele(dx)`, `partner(n)`, `coopoff(bits)` (1 damage, 2 twins, 4 speed, 8 HP, 16 partner
  models).

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
  window of the map (a modified mGBA renderer plus ROM-side camera/leash/HUD changes), tracked as
  the Phase 3 "widescreen" item. An in-ROM split screen (per-scanline BG scroll via HDMA) is the
  hardware-only alternative but doubles every sprite and clips badly at the seam.

## Test tooling additions

- `tools/mgba/ents.lua`: `ents()` lists both players and every enemy (vtable `0x080252A4`) with
  position, base EXP and control state, plus the camera view origin; `ents(true)` lists everything.
- `tools/mgba/force.lua`: `p2_force = 0x8000 | keys` drives player 2's pad from the console (bit 15
  valid, bits 0-9 GBA keys), `p2_force = nil` returns control to the TCP bridge; `p2_play{{word,frames},...}`
  scripts a frame-exact sequence and `p2_dash(keys)` performs a double-tap dash. Useful with mGBA
  savestates (Shift+F1 / F1) to retry a fight without the 75 s debug-menu idle.
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
  Test costs seconds instead of the 75 s idle; `tools/mgba_input.ps1` knows F1-F9 for the slots.
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
  leaves the menu back to the title (and the 75 s idle starts over).
- Test helpers: `tools/mgba_script.ps1 '<lua>' -Window N` opens Tools > Scripting by mouse (the
  menu bar ignores Alt mnemonics) and types the line into instance N's console;
  `tools/mgba_shot.ps1 out.png -Title 'Scripting*' -Of N` shoots that console;
  `Get-MgbaWindow -Title ... -ProcessId` / `Get-WindowPid` in `mgba_window.ps1` pick windows per
  instance. Capture both game windows with one `CopyFromScreen` to compare the same instant.

Open: a `ROM mismatch` check uses the CRC of the loaded ROM (so both need the same `make` flags,
e.g. `DEBUG_MENU=1`); mGBA's BIOS choice is not checked (HLE on both is the default); the guest's
save file is replaced by the host's snapshot.

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
