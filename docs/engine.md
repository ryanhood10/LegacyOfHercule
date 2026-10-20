# Engine notes (LoG2 USA)

Addresses are ROM (`0x08...`) or IWRAM (`0x03...`). Function names follow the Arefu/Legacy IDA database
where we have confirmed them against our own Ghidra decompile; everything here was re-derived from the
ROM, not copied (that repo has no license).

## Game loop

The game is a stack of coroutine objects. Each has a vtable of **relative** offsets (function =
vtable address + word), invoked through `Ptr_InvokeHandler` (`0x08021D2C`, just `bx r2`).

| slot | offset | role |
|---|---|---|
| 1 | +0x04 | enter |
| 2 | +0x08 | leave |
| 3 | +0x0C | update (before VBlank) |
| 4 | +0x10 | vblank |
| 5 | +0x14 | late update; nonzero return ends the coroutine |

- `GameLoop_Init` `0x0801F248`: `Input_Init`, arena, interrupts, VBlank IRQ on, audio init, first poll.
- `GameLoop_Tick` `0x0801F294`: loop { update; `Audio_MixerUpdate`; `IntrWait(VBlank)`; **`Input_Poll`**; late update } until nonzero.
- `GameLoop_VBlankHandler` `0x0801F230`: `g_FrameCounter++`, slot 4.
- `GameLoop_ExecuteAndWait` `0x0801F2F4` / `Coroutine_SwitchTo` `0x0801F2CE`: push/pop on `g_CoroutineStack`.
- `0x08021388`: fade/sleep routine (30 frames, `swi 3`), then waits until A/B/Start-ish keys (`0x304`) are released.

## Input

`Input_Poll` `0x0801EEFC` (called once per frame by `GameLoop_Tick`) reads `KEYINPUT` via
`ldr =0x04000120; ldrh [#0x10]`, inverts it, and fills `g_InputSystem` at **`0x0300229C`**:

```
+0x00 u8  history_head
+0x01 u8  history_armed     (nonzero once the frontend saw a key; gates the soft-reset chord)
+0x02 u16 history[8]        ring of newly-pressed masks
+0x12 u16 current           keys down, active-high          (= 0x030022AE)
+0x14 u16 pressed           went down this frame            (= 0x030022B0)
+0x16 u16 released          went up this frame              (= 0x030022B2)
```

On any new press it also stores `g_FrameCounter` (`0x03000E20`) into `g_LastInputFrame` (`0x03002298`).
A+B+Select+Start all held with one newly pressed -> `SoftReset` (`0x0802135C`).

**Co-op hook:** this is the single point where hardware keys enter the game, so the build replaces
it (`input_poll_hook`, `src/coop/input.c`, entry jump at `0x0801EEFC`). Player 1's keys come from
`KEYINPUT`, or from `g_p1_remote` (EWRAM `0x0203FF02`) when its bit 15 is set (lockstep netplay).
Player 2's keys fill a second `InputSystem` (`coop.input2`) from the `P2_MODE` source: by default
`g_p2_remote` (`0x0203FF00`, bit 15 = valid, bits 0-9 = keys, written by the TCP bridge, netplay or the
fork's pad port), else the link cable (`SIOMULTI1`); test modes mirror player 1 or swap on Select.
The ported control states read the `InputSystem` of the entity they run for. Details:
`docs/coop-design.md` "Player 2 input transports" and "Lockstep netplay".

## Globals (IWRAM)

| address | name |
|---|---|
| `0x03000000` | JCALG1 decompressor (ARM, copied at boot) |
| `0x03000E1C` | `g_CurrentCoroutine` |
| `0x03000E20` | `g_FrameCounter` |
| `0x03000E24` | `g_CoroutineDepth` |
| `0x03000E3C` | `g_IntrVectorTable` |
| `0x03000E90` | `g_PartyState` (stock: 0x164 bytes; 6 x 28-byte `CharacterEntry` at +0x0C). The build relocates it to `g_party_block` 0x0203F000 with `PARTY_SLOTS` entries: `docs/party-8-slots.md` |
| `0x03001120` | `g_EntityList` |
| `0x03001FA8` | `g_PlayerObject` (player entity pointer, 0 if none) |
| `0x03001FAC` | `g_VMContextPtr` |
| `0x0300229C` | `g_InputSystem` |
| `0x030022B8` | `g_RootCoroutine` |
| `0x030026BC` | `g_AudioMixer` |
| `0x03007FFC` | BIOS IRQ handler pointer |

## Player entity

`PlayerEntity_Create(mem, a, b)` `0x080086D4`: allocates 0x1A0 bytes, base init `0x0800F80C(this, a,
0x03001F64, b)`, vtable = **`0x08025604`**, `g_PlayerObject = this`, clears +0x190..+0x19C, then
reads the active party slot's display row and installs its **behaviour class**:
`this+0x180 = row.ppVTable->vtable`, calling that class's slot 0 (init). `PlayerEntity_RefreshSprite`
`0x080086A2` re-reads the row and re-inits the behaviour (used when a slot transforms).

Player vtable `0x08025604` (relative words; slot n = table + word[n]):

| slot | function | | slot | function |
|---|---|---|---|---|
| 0 | `0x08008743` (destroy, clears `g_PlayerObject`) | | 8 | `0x080078CD` |
| 1 | `0x080263A1` | | 9 | `0x08008945` |
| 2 | `0x08008767` | | 10 | `0x080088BB` SetAnimation |
| 3 | `0x08007A85` | | 11 | `0x080088CB` |
| 4 | `0x08007773` | | 12 | `0x080260A1` |
| 5 | `0x0800784D` | | 13 | `0x080263BD` |
| 6 | `0x080078A3` | | 14 | `0x08008807` OnHit |
| 7 | `0x0800678D` | | 15 | `0x0800888D` ChangeHP |
| | | | 16 | `0x08008785` TakeDamage (HP -= damage from the *active* slot; death animation and game over at 0) |

The table has 48 slots in all (it runs up to the walk state's vtable at `0x080256C4`); player 2's
private copy `g_p2_vtable[48]` overrides 15 and 16 with its own slot (`player.c`), and the duel code
hooks player 1's 16 (`duel.c`).

### Behaviour classes (installed into BSS by the static-init function `0x08022EC8` at `0x080232E4`)

Each class has **4 method slots** (words 4+ are data; they happen to resolve into the middle of
functions, which fooled a first pass):

| `ppVTable` (IWRAM) | vtable (ROM) | display rows | 0 init | 1 update | 2 | 3 |
|---|---|---|---|---|---|---|
| `0x030021E0` | `0x08024004` | 0,1,2,3,6,8,9,12,13,15,18,19,21 (standard) | `0x08016FD9` | `0x08017029` | `0x0802609D` (stub) | `0x080263B9` (stub) |
| `0x030021E4` | `0x08024E4C` | 4 | `0x080170D9` | `0x0801710F` | `0x08017159` | `0x08017175` |
| `0x030021E8` | `0x0802526C` | 10, 14 | `0x0801718D` | `0x080171E5` | `0x0801727D` | `0x08017299` |
| `0x030021EC` | `0x08024C58` | 5, 11, 16, 17, 20 | `0x080172B1` | `0x08017307` | `0x08017377` | `0x08017393` |
| `0x030021F0` | `0x08024ABC` | 7 | `0x080173AB` | `0x08017401` | `0x0802609D` (stub) | `0x080263B9` (stub) |

The same init also stores `0x08025684` / `0x08025EAC` at `0x030021FC` and `0x08025698` at `0x03002204`.

Rows 22-27 exist only in the build (`tools/spriteclone.py`): 22 Super Saiyan Hercule and 24 old
Hercule are copies of Hercule's row 21, 23 Krillin and 25 Android 16 copies of Gohan's row 0, 26 / 27
haloed Goku / Super Saiyan Goku copies of Goku's rows 19 / 20. Only bytes 0-5 (flags, sprite,
portraits, transformation links) change, so each keeps its source row's class (27 that of row 20).

**Why static input searches failed:** the controller lives behind these RAM-resident vtable pointers,
so nothing in ROM references it by address.

## Characters

- Party = 6 `CharacterEntry` slots in the stock game (hp/ep/stats/exp/flags/abilityList[4]); the
  build has `PARTY_SLOTS` (default 8) in the relocated block (`docs/party-8-slots.md`). The active
  slot's `displayDataIndex` selects a `CharacterDisplayData` row (stock `0x081D967C`, 22 rows, 20
  bytes) which carries sprite id, portrait, transform links and a behaviour vtable pointer. The build
  moves the rows to `display_rows` in the code blob with **28 rows** (`patch_rom.py`
  `LITERAL_RELOCATIONS`, 18 literals; `CharacterData_FindBySprite`'s limit byte at file `0x42D6` 22 -> 28).
- `Character_GetSpriteId` `0x08009324`: 0 = player, 1..6 = party slots, >= 7 = `g_CharacterSpriteIndex`.
  The build relocates the sprite index to `sprite_index` in the blob (stock ids up to 156, new ones up
  to 175; ids and owners in `docs/ids.md`).
- Abilities: `g_AbilityTable` stock `0x081D9900`, 14 x {readyIcon, coolingIcon, isReady fn, onUse fn}.
  `HERCULE=1` builds relocate it to `0x08900000` (start of the data-patch region; `tools/romdata.py`
  from `data/hercule.json`, literals at file `0x8024` and `0x8E8C`) and extend it to ids 0-22: 14-17
  Hercule's four moves, 18 Super Hercule, 19-21 Krillin's Afterimage Rush, Shockwave Punch and
  Destructo Disc, 22 Android 16's Rocket Punch. `python tools/romdata.py abilities <built rom>` prints it.
- Map entities are spawned from `MapEntry.mapScripts[]` by handlers `0x0800B711` (town NPC),
  `0x0800D6DF` (script actor), `0x0800E77F` (enemy); behaviour lists ticked by `0x0800D156`.
