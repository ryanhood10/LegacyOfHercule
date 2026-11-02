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
| `0x030021E4` | `0x08024E4C` | 4 (and our 28) | `0x080170D9` | `0x0801710F` | `0x08017159` | `0x08017175` |
| `0x030021E8` | `0x0802526C` | 10, 14 | `0x0801718D` | `0x080171E5` | `0x0801727D` | `0x08017299` |
| `0x030021EC` | `0x08024C58` | 5, 11, 16, 17, 20 | `0x080172B1` | `0x08017307` | `0x08017377` | `0x08017393` |
| `0x030021F0` | `0x08024ABC` | 7 | `0x080173AB` | `0x08017401` | `0x0802609D` (stub) | `0x080263B9` (stub) |

The same init also stores `0x08025684` / `0x08025EAC` at `0x030021FC` and `0x08025698` at `0x03002204`.

The updates, stock (2026-11-02, `docs/perfect-pass.md` 11.1): **each works on the active slot, whichever entity it runs for.**
`0x030021E0` is "The base class's update" below. `0x030021E4`: the active slot's HP + 1 % (`max * 0x290 >> 16`) every 20
frames (+0x184 + 0xCCD) to the maximum, nothing else. `0x030021E8`: not while in animation 0x24; 1 EP from the active slot
every 40 frames (+0x184 + 0x667), animation 0x24 at 0 EP with HP above 0. `0x030021EC`: the Super Saiyans' (below).
`0x030021F0`: as `0x030021E8` every 20 frames, with the active slot's HP + 1 % each time. In the build slot 1 is
replaced for all of them: `0x08024004` and `0x08024E4C` are `regen.c`'s rules for the entity's own slot, `0x08024C58`
`ssj_class_update`, and `0x0802526C` / `0x08024ABC` run the stock code with player 2's slot presented as the active one
(`ssj.c` `form_update`). The stock `Party_LevelUp` (`0x08003CA0`) sets HP and EP to the new maximum.

Rows 22-28 exist only in the build (`tools/spriteclone.py`): 22 Super Saiyan Hercule and 24 old
Hercule are copies of Hercule's row 21, 23 Krillin and 25 Android 16 copies of Gohan's row 0, 26 / 27
haloed Goku / Super Saiyan Goku copies of Goku's rows 19 / 20, 28 Super Saiyan 2 Gohan a copy of Gohan's
row 3 with row 4's sprite and portraits (row 4's class `0x030021E4`: combat bytes fixed at 100, an aura sound in
slots 2 / 3; its stock update, the active slot's HP regenerated, is replaced). Only bytes 0-5 (flags, sprite,
portraits, transformation links) change, so each keeps its source row's class (27 that of row 20).

**Why static input searches failed:** the controller lives behind these RAM-resident vtable pointers,
so nothing in ROM references it by address.

## Characters

- Party = 6 `CharacterEntry` slots in the stock game (hp/ep/stats/exp/flags/abilityList[4]); the
  build has `PARTY_SLOTS` (default 9) in the relocated block (`docs/party-8-slots.md`). The active
  slot's `displayDataIndex` selects a `CharacterDisplayData` row (stock `0x081D967C`, 22 rows, 20
  bytes) which carries sprite id, portrait, transform links and a behaviour vtable pointer. The build
  moves the rows to `display_rows` in the code blob with **29 rows** (`patch_rom.py`
  `LITERAL_RELOCATIONS`, 18 literals; `CharacterData_FindBySprite`'s limit byte at file `0x42D6` 22 -> 29).
- `Character_GetSpriteId` `0x08009324`: 0 = player, 1..6 = party slots, >= 7 = `g_CharacterSpriteIndex`.
  The build relocates the sprite index to `sprite_index` in the blob (stock ids up to 156, new ones up
  to 177, and id 58 repointed to `ssj2_record`; ids and owners in `docs/ids.md`).
- Abilities: `g_AbilityTable` stock `0x081D9900`, 14 x {readyIcon, coolingIcon, isReady fn, onUse fn}.
  `HERCULE=1` builds relocate it to `0x08900000` (start of the data-patch region; `tools/romdata.py`
  from `data/hercule.json`, literals at file `0x8024` and `0x8E8C`) and extend it to ids 0-23: 14-17
  Hercule's four moves, 18 Super Hercule, 19-21 Krillin's Afterimage Rush, Shockwave Punch and
  Destructo Disc, 22 Android 16's Rocket Punch, 23 Super Saiyan 2 Gohan's Burst. `python tools/romdata.py abilities <built rom>` prints it.
- Map entities are spawned from `MapEntry.mapScripts[]` by handlers `0x0800B711` (town NPC),
  `0x0800D6DF` (script actor), `0x0800E77F` (enemy); behaviour lists ticked by `0x0800D156`.

## Super Saiyan transformation (decompiled 2026-11-01 for Super Saiyan Hercule)

The stock rules, read from the ROM (thumb disassembly); what the build changes is at the end.

- **Display rows.** Byte 0 is the form's flags: bit 0 = transformed, bit 1 = cannot transform (row 4, the Cell
  Games' Super Saiyan 2 Gohan, flags 3). Byte 4 = the row the form transforms to, byte 5 = the row it turns back to
  (Goku 19 <-> 20). Bytes 16-19 = the behaviour class: every transformed row (5, 11, 16, 17, 20) has the Super
  Saiyan class `0x030021EC`. (An older note said the drain is in the flight / charge state `0x080083C3` against row
  byte +7: wrong. That state is the dash; it reads row byte +8 as a bit mask of frames that leave a trail and the
  word at +0xC as a speed. The drain is the class update below.)
- **The abilities.** Everyone's Transformation is **ability 12** (Gohan 4 3 12 2, Goku 3 10 12 2, Vegeta 0 7 12 2,
  Trunks 1 13 12 2 on the Map Test party); ability 11 is the same with another animation and is in nobody's list.
  isReady `0x080134F2` (11 and 12): row bit 1 -> never; row bit 0 -> always (to turn back); else the ability gate
  (`[[0x086DB284] + 0xC] != 0`, 0 on a Map Test), the party meter (`PartyState` +8) at 0 and **EP >= 10**. onUse
  (12: `0x0801353C`, 11: `0x080134C0`) **costs nothing**: row bit 0 -> animation 0x24 (turn back), else 0x23 (11:
  0x25). (Ability 13, `0x08013474`, is Trunks' 5-EP move, animation 0x32.)
- **Transform, animation 0x23** (state `0x03002184`, vtable `0x08025D9C`): enter `0x080155A8` starts the power-up
  hum (SFX_TABLE + 0x294, `0x0801FC9C(mixer + 0x5C4, sfx, 0x40, 0)`) and shows **group 9** of the entity's record
  (`0x08011D38`: +0x4C = record + 0xDC, frame 0, counter 0). Update `0x080155E4`: the party meter +8 a frame, then
  `0x08011D4A`: frames 0, 1 (16 frames each) then 2 / 3 alternating every 16 frames; at frame 160 the transform's
  sound (SFX_TABLE + 0x114) and slot 5. Leave `0x080155F6` stops the hum. Slot 5 `0x0801563C`: SFX_TABLE + 0x114
  again, then **`0x08014D58(entity, 1)`**. (Animation 0x25, ability 11: state `0x03002188`, vtable `0x08025CE0`, the
  same group 9 for 64 frames, frame = counter / 16, the meter +20 a frame, the same slot 5; no hum.) The meter
  ends full (1280, five HUD pips), so the charged attack is ready as the form starts.
- **`0x08014D58(entity, up)`, the form change.** Uses the **active** party slot: its row's class slot 3 (the
  Super Saiyan class stops the hum), row := byte 4 (up) or 5, entity +0x48 := that row's sprite record, animation
  0, `PlayerEntity_RefreshSprite` (`0x080086A2`: the active slot's class into +0x180 and its init), the new class's
  slot 2 (the Super Saiyan class starts the hum, SFX_TABLE + 0x294, looping while transformed).
- **The Super Saiyan class** (`0x030021EC`, vtable `0x08024C58`): init `0x080172B0` writes the combat bytes from
  the active slot's str / pow / end high bytes **+10 each** (base class `0x08016FD8`: +0), +0x163 / +0x164 = 0x80,
  walk speed **1.75** px (base 1.25) and dash **3.0** (base 2.5), the accumulator +0x184 = 0. Update `0x08017306`:
  nothing while in animation 0x24 (vtable slot 11 asks); else +0x184 += 0xCCD and at each overflow (**every 20
  frames: 3 EP a second**) the active slot's EP -1 (not below 0), and **at EP 0 with HP above 0, animation 0x24**.
  No regeneration and no meter decay while transformed.
- **The base class's update** (`0x08017028`): the party meter -1 a frame (the cool-down: a transform's 1280 frames
  before the next, less what the charged attack spends); +0x184 += 0x6E and at each overflow (about 10 s) the
  active slot's EP +3 % of its maximum and every other stock slot (0-5) EP and HP +5 %. The build's rules are `regen.c`
  (once a frame, not per entity; only for characters nobody plays; no 3 % for the played ones; `docs/perfect-pass.md` 11.1).
- **Turn back, animation 0x24** (state `0x03002180`, vtable `0x080256AC`): enter `0x0801579A` (+0x163 / +0x164 := 0,
  restored to 0x80 by leave `0x080157BA`) and `0x08013578` (**group 21** of the record, +0x19C); update `0x0801358C`:
  frame = counter / 16 for 64 frames; slot 5 `0x080157AE` = `0x08014D58(entity, 0)`. No sound. Goku's group 21
  (record 61) fades the hair gold -> light -> brown -> black.
- **`Party_DetransformOthers`** (`0x08003ED4`): every stock slot (0-5) but the active one whose row has bit 0 goes to
  its byte-5 row. Called at a load (`save_menu_start_hook`) and at a save pad's switch (`switch.c`), so a transformed
  player 1 stays transformed through a save and a reload (the class's slot 2 restarts the hum) and nobody else does.

**The build** (`src/coop/ssj.c`, 2026-11-01): `0x08014D58` and the class update served only the active slot, so a
player 2 who transformed changed player 1's row and drained player 1's EP. The transform's slot 5 (vtables
`0x08025D9C`, `0x08025CE0`), the turn-back's slot 5 (`0x080256AC`) and the Super Saiyan class's update
(`0x08024C58` slot 1) are replaced by C that does the same for the entity's own slot (player 2: `p2_refresh_sprite`
for the sprite and class); the base class's update (`0x08024004` slot 1) is `regen.c`'s (no trickle for a slot on a
transformed row, none for a played one);
and `ssj_frame` applies `Party_DetransformOthers`' rule to a slot player 2 leaves. For player 1 alone nothing changes.

## Messages, projectiles and beams (decompiled 2026-10-31 for the team combos)

How the combos use these: `docs/tournament.md` 25.2-25.4. Facings everywhere below: 0 down, 1 up, 2 left, 3 right.

- **Hits.** A hit is an entity message: +0x24 the hit, +0x20 the attacker; `0x08008944` passes it to OnHit
  `0x08008807` (vtable slot 14), which calls take-damage `0x08008785`. Attack descriptors (punch, ki blast) at
  `0x086DB2CC`. Players' combat bytes are their party entry's STR / POW high bytes (class init `0x08016FD8`).
- **The command modal (a frozen fight).** `CommitCommandBatch` (`0x0800E038`) runs the modal `0x0800DE20` (vtable
  `0x08025150`): its update `0x08004EA8` draws the entity list without updating it, its late update `0x0800DE6E`
  broadcasts message 10 and ends when no entity answers 1. The base entity message `0x08019B70` answers 10 by
  stepping the entity's command (+0x150 `{vtable, next, done}`: slots 1 start, 2 step, 3 done?, 4 stay; popped by
  `0x0800E078`) and animating it. No entity's update (slot 2) runs inside it, but projectiles' messages answer 10 by
  updating: the projectile class `0x08013D68`, the beam `0x0801D91C`, the players' ki blast `0x0800EA3A`.
- **Projectiles (the ProjBase class).** +0x0C the VRAM tile, +0x10 loaded, +0x18 the frame descriptor; vtable slot 5
  (`0x08013D40`) DMAs the descriptor's blob (raw w x h bytes, 8bpp) to `0x06010000 + tile * 32` with DMA3. The OBJ
  VRAM allocator counts 64-byte blocks: a size under 0x40 is a block of 0 and breaks its free list.
- **Recolouring.** Every OBJ shares one 256-colour palette (`0x081DA6C8`) and tiles are 8bpp, so a colour change is a
  remap of palette indices in the tiles. The energy arts' ramps (brightest first): Super 13's orbs 2 106 247 252 26,
  the blue ki 2 184 191 76 196, the Kamehameha beam 43 67 68 74 76; the stock Masenko (a lobbed ball, object
  `0x080139BC`) uses 37 43 44 45 46.
- **The beam (the Kamehameha object, every kit's beam).** `Beam_Fire` (`0x08014478`) takes the facing's frame set from
  `0x086E3458 + facing * 0x60` (eight 12-byte descriptors `{geometry, attr, tiles}`; the alternate yellow / pink
  art's sets at `0x08704604`) into +0x44 / +0x48 (+0x40 the current frame) and DMAs descriptors 0, 2, 1 to the
  beam's VRAM + 0, + 0x100, + 0x200; the update (`0x08014556`) and the end (`0x0801451C`) upload later frames from
  the same set, so pointing +0x40 / +0x44 / +0x48 at another set redraws the whole beam.
- **The Big Bang.** The player's ability state is vtable `0x080258B4`; its enter `0x08015018` makes the object
  `FUN_08013E48` (0x5C bytes, OBJ VRAM 0x400), whose update `0x08013FDA` animates the charge with the three
  descriptors at `0x086DEB6C` (frames 0 1 2 1, every 8 frames) and the flight with four at `0x086E0B90 + facing *
  0x30` (32 x 32).
- **The Special Beam Cannon.** Object 0x4C bytes, OBJ VRAM 0x300: `0x080157E0` makes it charging at the fingers,
  `0x0801585A` fires it, `0x0801592E` ends it; its message `0x08015EE0` answers 10 with the update `0x0801594C(o,
  10)` (grows the drill 3 px a frame and hits every enemy on it; with 0 it only animates). Charge and shot spots per
  facing: `0x086E382C`, `0x086E385C`.
- **Drawing a body.** `0x08007A84`: a frame's descriptor and its screen spot from the camera cell (descriptor = the
  group's array for the facing, `*(+0x4C)[facing +0x10] + frame +0x14 * 12`, kept at +0x3C); `0x08007772`: the OAM words,
  and when the frame changed (+0x20 the last one) it unpacks it (`0x0801F394`, or `0x0801F3BA` through the remap +0x24)
  into a 4 KB buffer on the stack (sp - 0x1004) and DMAs it to the entity's own buffer (+0x30); a plain (kind 0) frame is
  DMA'd later straight from the ROM. The draw pass (`0x08006AD8`) calls every entity's slot 3 (on screen?), sorts, then
  slot 4 (OAM); a nonzero slot 4 queues it, and the upload pass (`0x08006AA6`) calls the queued ones' slot 5. Records
  share one blob between a side facing and its mirror (the right frame's attribute flips it; the offsets are the same).
- **Android 16 and Super 13 in the tournament.** Their kits wrap the co-op enemy vtable (`coop2.enemy_vtable`), whose
  message is the stock enemy's (`0x0800E5B8`, message 10 to the base entity message `0x08019B70`: with no command it
  runs the control state's and the aux state's animate, slot 1); `android16.c`'s `a16_message` / `super13.c`'s
  `s13_message` are the story bosses' only. Idle's animate `0x08007570` counts +0x12 and shows frame 1 for 15 of every
  256 frames.
- **The camera.** `CAMERA_CELL` (`src/coop/rom.h`) words 3-6 are the view rectangle in world px (left, top, right,
  bottom), 7 / 8 the origin; nothing scrolls it while the command modal runs.

## OBJ VRAM and the text box (decompiled 2026-11-02 for West City's vanishing)

- **The frame.** `GameLoop_Tick`'s wait is `FUN_08020D40`: a `swi 2` (Halt) loop at 0x08020D6E until the VBlank flag. It
  also keeps a CPU meter at the mixer's +0x3C: 100 if VBlank already fired, else the scanline scaled. The probe
  (`docs/testing.md` "Frame time and OAM probe") measures from the emulated clock instead.
- **The entity list is also the allocator.** `g_EntityList` (0x03001120) = `{u8 free head, u8 free records; +4 count; +8
  items (128: the draw pass's array is 129 words, and a full list reports no room); +0x208 the upload queue; +0x40C the aside
  pile {count, items}; +0x694 block records, 10 bytes each; +0xC00 + tile / 2 the record id at a tile}`. A record = `{u8
  used, u16 +2 offset, u16 +4 size (both 64-byte units), u8 +6 / +7 next and previous by address, u8 +8 / +9 the free
  list}`. `ObjVram_Alloc` (0x080067AC): an exact fit if any, else the smallest bigger block, **cut from that block's top**.
  `0x08006D70`: the largest free block in bytes. OBJ VRAM is 32 KB.
- **OAM.** A body is one OBJ. The shadow (pointer 0x030022BC, 128 entries, filled with 0x200 first) is written without a
  bound by each entity's slot 4 and copied by `0x08006AA6`. Far from full on every map measured: 22 at most.
- **The text box.** `FUN_0800B1E2` asks `0x0800700E(list, need)` for one contiguous run: 0xA0 << 6 = 0x2800 bytes without a
  portrait, 0xE0 << 6 = **0x3800** with one (the box is 64 x 64 x 3 + 32 x 64, 8bpp). Others ask too: 0x1680 (`0x080109EC`),
  0x400 (`0x080119F4`, `0x0801EC16`). Short of it, `0x0800700E` runs rounds 0, 1, 2...: each asks every entity's **slot 6**
  (`round`): 1 = destroy, 2 = aside (slot 7 frees its block), and redraws. The answers: a body `0x080078A2` off screen
  from round 13, **on screen at exactly round 15**; the HUD `0x08008EFA` from round 4; `0x08024230` from 8 or 9; decorations
  and the system entities `0x08006528` at round 15. So the last round takes **everything on screen at once**, the players
  and the panels too. When the box closes `0x0800712E` (`0x0800B2DA`) brings the pile back through slot 8 (a new block; +0x20
  := 0 uploads the frame again). A run that can never fit spins for ever.
- **The build** (`src/coop/objroom.c`): the allocator cuts from the top, so player 2's body and panel, made last, sat in the
  middle of the free run. They now go to its low end (`obj_alloc_low`: a block on the run's top for the rest, the wanted one,
  the first freed). `obj_make_room_hook` replaces `0x0800700E`: player 2's two blocks settle first (into a hole they fit, else
  the low end), then the stock's rounds to 14, then round 15 one body at a time (the one whose block makes the longest
  free run, then the farthest from player 1), stopping when the box fits, player 1's body moved low first; then the
  decorations together (the first holds the sheet's block, the rest draw from it: one left behind shows the box's tiles); the
  players last. It also gives up at round 64 instead of hanging. `obj_room` (`.combo.pf`) keeps the last eviction's round, the bodies
  taken and the count. **`coop.hud2.size` is now the panel's "allocated" flag: tile 0 is a real place for it.**

## Dropped items and touch boxes (decompiled 2026-11-01 for player 2's pickups)

- **A drop.** The drop scripts' `SpawnItem` (Step 87 / 92, handler `0x0800ABA4`) builds an item entity (`0x08011802`,
  0x44 bytes, vtable `0x08024C90`: slot 3 update `0x08011896` bounces it to the ground, slot 10 `0x0801193E` "taken"
  removes it from the list and runs the item record's +0x34 handler, slot 9 `0x08011960` answers message 3 (a punch's
  hit box, `Player_Attack`) only for a counted item). Fields: +0x14 the item record, +0x18..+0x24 the bounds (x1, y1,
  x2, y2), +0x30 / +0x34 the bounce, +0x38 landed, +0x3C 1 for a direct-effect item, +0x40 the item id. The item
  table `0x086ADE24` (0x38 a record): +0 counted, +2 popup, +3 max count, +0x30 the use handler (`0x0800F592`:
  ids 0xB / 0xC / 0xD heal the **active slot** by fractions of its maximum, 0x15 gives EP), +0x34 the pick-up handler
  (`0x0800F3F0`, our `item_use_hook`, which runs +0x30 for a direct item).
- **The touch box.** A direct-effect item (no popup, not counted: the food and senzu that drop) gets a box from
  `0x0801177C`: `{vtable 0x08023F18, s16 x1 - 15, s16 y1 - 7, s16 x2, s16 y2, the player object 0x030020BC, the item}`
  (0x14 bytes), appended to the list at **0x03001FE0** (`{count, pointers...}`; the map's own trigger boxes of classes
  `0x080240E4` / `0x08023E38` / `0x08023E44` share it). Class slots: 0 the destructor, 1 fire (`0x080117C2`: the item's
  slot 10, then `0x0800BF6E` takes the box off the list), 4 returns +0x14. The stock scan fires a box whose rectangle
  holds **player 1's** position, moving or standing, and never tests player 2 (headless: the box moved onto player 2
  alone did nothing for 120 frames; onto player 1 alone it was taken within 60). `0x030020BC` is the "always" activation
  object of the next section (`{vtable 0x080240F0, the camera-follow vtable 0x08025A6C, player 1's entity, 0x0802480C, ...}`).
- **The build** (`src/coop/pickup.c`): every input poll player 2's position is tested against the item boxes and one
  is fired the stock way with the party's active slot and `g_PlayerObject` swapped to his for the call, so the handler
  heals his character. `tools/headless/pickup.txt`.

## Trigger boxes: the scan, the activations, the fires (decompiled 2026-11-02 for player 2's agency)

- **The list** at 0x03001FE0: `[0]` count, `[1..]` the boxes, `[0x21]` scans since the last reset, `[0x22]` resets (a map
  load, `0x0800BF08`; a variation change, `0x0800C0F8`), `[0x23]` 1 during a scan. **The scan** `0x0800BFC0`, once a world
  update (`0x08004CAC`, after the entity list's update): nothing without a player or with the active slot's HP at 0; from
  the top of the list down, a box whose rect holds player 1's position (x1 <= x < x2, y1 <= y < y2, `0x0800791A`: +0x158 /
  +0x15C >> 8) has its activation's slot 1 asked (activation, box, player); the first nonzero answer ends the scan.
  **`0x0800C06E`** (Player_Attack, when the punch hit nothing and message 3 found nobody): the same walk with the
  activation's slot 2 (activation, player): a box that would take this A press: no swing. For player 1's position only.
- **A box:** `{vtable, s16 x1, y1, x2, y2, activation}`, slot 1 its fire (box, player), modal, 1 when it did something.

| box class (create) | fire | notes |
|---|---|---|
| door `0x080240E4` (`0x0800C233`) | `0x0800C1AC` | fires only while the key at +0x11 is held on the stock input; +0x10 the facing, +0x14 the payload: fade (`0x0800C84C`, freezes the entity it is given), `0x0800C584` loads the map at the payload's spot and builds player 1 there |
| walk-off `0x08023E38` (`0x0800D9C3`) | `0x0800D93C` | the same, the arrival = the payload's spot + (the given entity's position - the rect's corner) |
| script `0x08023BC0` (`0x0800C2C3`) | `0x0800C2A6` | runs the script at +0x10; the entity plays no part |
| dialogue `0x08023E44` (`0x0800D7E1`) | `0x0800D7A0` | runs the sequence at +0x14 with the entity's +0x190 as its context |
| save pad `0x08023BCC` (`0x08010AC7`) | `0x080109EC` | below |

| activation (getter) | object, vtable | slot 1 |
|---|---|---|
| enter once (`0x0800D765`) | `0x03002094`, `0x080238E4` | fires; the box leaves the list unless the fire reset it |
| A (`0x0800D839`) | `0x03002098`, `0x08023E50` | A pressed on the stock input |
| A facing (`0x0800D87F`, `0x0800D889`) | `0x0300209C` .. `0xB4`, `0x08025188` | A pressed and the entity's facing +0x10 == object +4 |
| key held, once (`0x0801087B`, `F`, `85`, `8B`) | `0x03002154` .. , `0x0802498C` | the key at +4 held (0x40, 0x80, 0x20, 0x10), then as enter once (every door's opening animation and sound: only `FindEntityByType` / `PlayAudio`) |
| on entering (`0x0800D8EB`, an object per box) | `0x08024980` | fires unless +4 == the scan's number - 1 (it was inside at the last scan); writes +4 |
| always (`0x0800D8AF`) | `0x030020BC`, `0x080240F0` | the box's own test (exits, dropped items) |

- **What each kind does in the stock maps** (the dumps): script + A: the flight spots and flight circles, map viewers
  (`LoadMapNoEntities`, `WaitForConfirmButton`, `RestoreMap`); script + on-entering: seven bridge layer swaps on zone 4
  areas 53 / 55 only; dialogue / script + enter once: the story's walk-in scenes; dialogue + A facing: doors' lines and barriers.
- **The save pad** `0x080109EC`: with more than one member (`0x08003E00`) the menu `0x080112C0` (modal: 1 Save, 2 Switch
  Characters, else nothing); Save: `0x08004030(party, the given entity's position)` writes the map, that spot and the save,
  then the one-line sequence 0x083B6428 "Game Saved!" (a scene, so `0x03001FDC` is set); Switch: a fade, `0x08010B08` (ours,
  `switch.c`), `0x08005F1A`, the fade back. `0x08006F4A` (the screen's EntityList_Begin) copies the whole entity list out
  and empties it, `0x08006FA4` puts it back: removing an entity under that screen corrupts the list (`0x080068CA` finds no index).
- **Character barriers** ("Only Piccolo"). A barrier is an item (MapEntry +0x18, count +5): `{cond, deco id, hits, flags,
  0, handler 0x0800F9A3, script}`, attached when the map loads if its condition holds (`0x0800C390`: `0x08003F06`, then
  `0x0800F034`: the item object, vtable `0x08025720`, +0x24 the record, +0x28 the decoration, takes the decoration's place in
  the list), and a refusal: a dialogue trigger with the opposite condition, also tested once, at the load. Both conditions read
  the active character with Step 11 (bytes 02 0B); those bytes are in no other trigger's or item's condition in the ROM
  (19 refusals, 20 items; zone 4 area 5's opener is a script trigger on the refusal's own rect, no item). The item's message
  (`0x0800F2B6`, slot 9) takes the hit in message 0: `{+0 kind, +4 x1, y1, x2, y2, +0x18 power, +0x1C the attack
  descriptor, +0x20 the attacker (a punch: the player; a ki blast: its owner, 0x0800E946), +0x30 the object}`; flags bit 0 melee,
  bit 1 energy; `0x0800F1AE` counts the hits, the last runs the handler. Nothing there asks who hit. **Stock quirk:** zone 4 area
  50's refusal says "Only Piccolo" but its condition is `active == 0` (Gohan) and flag 275 clear: only Gohan ever sees it.
- **Commands** (see `docs/scripts.md` for the batch): FlyToPosition's (`0x08009E30`) is `{vt 0x08024864, next, done, mover, x,
  y}`; its start `0x0800E838` calls the entity's slot 12 and the mover `0x0800D5B8` with `{0, s16 x, y, speed 0x200}`: 2 px a
  frame, animation 1 then 12 (state `0x03002130`, **group 12**, `0x0800E7A0`). The walk's start `0x0800E004` / `0x08010898` the mover
  `0x0800D584`, speed 0x100, animation 1. Hercule's record 72 has a group 12 but it is no flying pose (a stub of a body): stock
  flights of a player 1 Hercule show that. A world-map pad (`PushByte 43; PlayMusic; ...; WarpToMapWithEntities`) has no
  take-off at all in the stock: the music, the fade.

**The build** (`src/coop/agency.c`, `docs/coop-design.md` "Player 2's agency"): entry jumps on `0x0800BFC0` and `0x0800C06E`,
slot 9 of `0x08025720`, VM steps 54 and 38.
