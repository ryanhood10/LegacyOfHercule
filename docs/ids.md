# ID and address registry

Every number the mod has taken, who owns it, and what is free. **Take new numbers from the free
ranges here and update this page in the same commit.** Before parallel sessions start, give each one
its own range of every kind (`docs/howto.md` "Work in parallel sessions"). Last checked against
the code on 2026-10-19 (main e45c765).

## Story flags (bank at 0x0203F0EC + n / 8, 816 flags, saved)

Stock scripts use flags up to 306. `Party_Init` clears 0-399 at a new game, and `party.c` also clears
400-479. Each flag's meaning is in `docs/scripts.md` 2.5.

| range | owner | in use |
|---|---|---|
| 360-362 | story chapters 1-7 | 360 Krillin has visited, 361 inside Triceratops Jungle, 362 Hercule's deal |
| 363-375 | Roshi's gym (`ch8_gym`) | 363-375 |
| 376 | the removed arctic Android 13 encounter | free since 2026-10-18 (`ch8_arctic.lscript` is not included) |
| 377-379 | free | |
| 380-399 | new maps (`data/maps/*.json`) | 380 the arctic's Vegeta barrier |
| 400-419 | Broly chapter (`broly`) | 400 ship landed, 401 trip under way, 402 done, 403 confrontation seen, 404-419 party kept by CoopCall 10 |
| 420-439 | Gero's computer (`ch9_gero`, `src/coop/a16player.h`) | 420 running, 421 over, 422 rubble punched, 423 / 424 west / east generator down, 425 bunker done, 430-437 saved "available" bits of slots 0-7 |
| 440-459 | Lookout / dojo (`wait_chamber`, `wait_spar`) | 440 Roshi's dojo booked for a spar |
| 460-479 | free (still cleared at a new game) | |
| 480-815 | free (not cleared at a new game: avoid, or extend `PARTY_WAIT_FLAG_BYTES` in `party.c`) | |

Stock flags the mod's scripts test (meanings from the script dump):

| flag | meaning |
|---|---|
| 86 | zone 8 (Northern Mountains) landing open on the world map |
| 126 / 127 | Gero's lab door / lab scene done |
| 136-141 | the lab basement: 138 and 140 redirect the basement doors, 141 = after the self-destruct (cave mouth sealed) |
| 155-157 | the workshop memory scene, the Scouter Note and Cell's TV announcement (157: the ten-day wait has started; exact split in `docs/scripts.md` 5, `ch9_gero` row) |
| 158 | all seven Dragon Balls (the world map checks it) |
| 191 | the Cell Games landing open |
| 196 | set by the end credits |
| 212-217 | Roshi's gym rounds and technique lessons (214 = the last +3 capsule) |

## Enemy stats (stock 0-108; extra stats in `src/coop/bosses.c/.h`)

Stock stats with boss rules in `bosses.c`: 3 / 4 (17 / 18), 5 (19), 7 (Gero), 41 (Frieza, the forest
spirit), 106 (Yamcha, paired with Tien's model and stat 98) and 64 (Krillin, paired with Hercule's
sprite 72) for Roshi's gym.

| id | what | base stat | owner |
|---|---|---|---|
| 109 | Android 16 boss (own spawn path) | | `android16.c` |
| 110 | Android 13 beside Android 19 | 5 | bosses |
| 111 | Cooler as the forest spirit | 29 | bosses |
| 112 | Android 13 beside Gero | 5 | bosses |
| 113 | Cooler's Namekian servant | 29 | bosses |
| 114 | Android 13 in the arctic (unused since 2026-10-18) | 5 | bosses |
| 115 | Broly | 29 | Broly chapter |
| 116 | Paragus (`BOSS_RAGE`) | 29 | Broly chapter |
| 117-119 | free | | |
| 120 / 121 | the bunker's two Android 19s (`gero_stats[]`) | 5 | Gero's computer |
| 122-124 | reserved for Gero's computer, unused | | |
| 125-139 | allocated to the Lookout session, unused | | |
| 140+ | free | | |

## Sprite ids (relocated `sprite_index`, stock 0-156, gaps are 0)

| id | what | source | Makefile variable |
|---|---|---|---|
| 72 | Hercule, **repointed**: `herc_record` = the stock record 72 (ROM groups) plus the Kamehameha pose in group 18 (Cell Games beam struggle) | `tools/herculesprites.py` | `EXTRA_SPRITES` `72=herc_record` |
| 157 | Super Saiyan Hercule | `tools/spriteclone.py` | `SSJ_SPRITE` |
| 158 | Hercule from Buu's Fury (old Hercule's body) | `tools/buu2log2.py` | `BUU_SPRITES` |
| 159 | Legendary Super Saiyan Broly (from Buu's Fury) | `tools/buu2log2.py` | `BUU_SPRITES` (hardcoded in `bosses.h`) |
| 160 | Android 16 boss | `tools/a16sprites.py` | `A16_SPRITE` |
| 161 | Krillin (playable) | `tools/krillinsprites.py` | `KRILLIN_SPRITE` |
| 162-164 | free | | |
| 165 | Android 13 | `tools/bossmodels.py` | `A13_SPRITE` |
| 166 | Cooler's Namekian servant | `tools/bossmodels.py` | `NAMEK_SPRITE` |
| 167-169 | free | | |
| 170 | Paragus | `tools/brolysprites.py` | `PARAGUS_SPRITE` (hardcoded in `broly.lscript`) |
| 171 | the Saiyan ship (from Buu's Fury) | `tools/brolysprites.py` | `SHIP_SPRITE` (hardcoded in `broly.lscript`) |
| 172-174 | free | | |
| 175 | Android 16 as a player | `tools/a16sprites.py` | `A16P_SPRITE` |
| 176-184 | free (176-179 were allocated to the Hercule beam-pose session, unused: it repointed 72 instead; 180-184 were allocated to the Lookout session, unused) | | |

Several ids are also written as literals in scripts and headers, so changing a Makefile default alone
leaves copies out of step: grep for the number before moving one.

## Display rows (`display_rows`, stock 0-21, 26 rows, row limit patched at file 0x42D6)

| row | character | sprite | portrait (dialogue / status) | built by |
|---|---|---|---|---|
| 22 | Super Saiyan Hercule | 157 | Hercule's | `spriteclone.py` |
| 23 | Krillin | 161 | 73 / 119 | `spriteclone.py`, `krillin.h` |
| 24 | old Hercule | 158 (72 when `BUU=0`) | Hercule's | `spriteclone.py`, `krillin.h` |
| 25 | Android 16 (player) | 175 | 26 / 121 | `spriteclone.py`, `a16player.h` |
| 26+ | free (raise the limit in `patch_rom.py` and `ROW_COUNT` in `spriteclone.py`) | | | |

## Portraits (`portrait_module`, stock words 0-118 copied; word id + 1)

| id | what | defined |
|---|---|---|
| 119 | Krillin's status portrait | `krillinsprites.py` `KRILLIN_STATUS_PORTRAIT`, `krillin.h` |
| 120 | Paragus' dialogue portrait | Makefile `PARAGUS_PORTRAIT`, `--extra-portrait`; `text 120` in `broly.lscript` |
| 121 | Android 16's status portrait | `krillinsprites.py` / `spriteclone.py` `A16_STATUS_PORTRAIT` |
| 122+ | free | `krillinsprites.py` asserts ids are unique |

Dialogue speaker ids worth knowing: 26 Android 16, 36 Bulma, 38 Cell, 67 Hercule, 73 Krillin
(more in `docs/scripts.md` 2.2).

## Ability table (`data/hercule.json` `abilities`, stock 0-13, numbered in order)

| id | ability | C |
|---|---|---|
| 14-17 | Rolling Hercule Punch, Dynamite Kick, Megaton Punch, Hercule Taunt | `hercule.c` |
| 18 | Super Hercule (transform) | `ssj.c` |
| 19-21 | Afterimage Rush, Shockwave Punch, Destructo Disc | `krillin.c` |
| 22 | Rocket Punch (Android 16) | `a16player.c` |
| 23+ | free | |

## Party slots (`PARTY_SLOTS` = 8)

0 Gohan, 1 Piccolo, 2 Vegeta, 3 Trunks, 4 Goku, 5 Hercule (old Hercule shares this slot on row 24),
6 Krillin, 7 the duel clone (borrowed by Android 16 during Gero's computer). A ninth character needs
`PARTY_SLOTS=9` or a borrowed slot (`docs/party-8-slots.md`); `partyram` holds 0x91C of 0xB00 bytes at 8 slots,
and a 16-slot build overflows it.

## CoopCall functions (game action 59, `src/coop/vmcall.c`; lscript `CoopCall N, a, b, c`)

| N | name | owner |
|---|---|---|
| 1 | DuelStart (slot, flags, end_seq) | duels |
| 2 | DuelClear () | duels |
| 3 | DuelWinner () -> 1 / 2 | duels |
| 4 | HerculeAwaken () | Buu phase |
| 5 | P2Present () -> 0 / 1 | story |
| 6 | P2Bind (slot) | story |
| 7 | PadSwap () | duels |
| 8 | P2Slot () -> slot / -1 | story |
| 9 | Stage (op, b, c): rage dialogue, place player 2 | Broly chapter |
| 10 | PartyKeep (op) | Broly chapter |
| 11 | A16Join () | Gero's computer |
| 12 | A16Leave () | Gero's computer |
| 13 | WaveStart (seq, round) | Time Chamber |
| 14 | Wave (op) | Time Chamber |
| 15+ | free | |

Full argument notes: `docs/scripts.md` 2.2b and the header of `vmcall.c`.

## Test commands (`coop2.cmd` 0x0203FD08, `cmd_arg` 0x0203FD0C, `cmd_arg2` 0x0203FD54; `coop.h`)

| N | name | does |
|---|---|---|
| 1 | SPAWN_SCRIPT | spawn mapScripts[arg] like TriggerMapEvent |
| 2 | BOSS_BAR | draw a boss bar for an entity |
| 3 | ACTOR | stand an actor of sprite `arg` next to player 1 |
| 4 | A16 | spawn the Android 16 boss |
| 5 | WARP | `arg` = zone \| area << 8, `arg2` = x \| y << 16 (pixels) |
| 6 | DUEL | start a duel (`arg` packs slot, flags, end script) |
| 7 | STORY_WIN | win every boss fight on the map |
| 8 | SPAWN_RECORD | spawn the enemy record at address `arg` with a boss bar |
| 9 | PARTNER_WIN | only the boss twins fall (Broly's rage test) |
| 10 | free | |
| 11 | A16_PARTY | Krillin player 1, Android 16 player 2 |
| 12 | free | |
| 13 | WAVE_CLEAR | every standing enemy dies |
| 14 | WAVE_SKIP | the next announced round is `arg` |
| 15+ | free | |

## Zone 15 areas (new maps, `data/maps/*.json`, built by `tools/mapbuild.py`)

| area | map | notes |
|---|---|---|
| 1 | `arctic.json` | Vegeta barrier (flag 380); the 13 encounter was removed |
| 2 | `time_chamber.json` | Hyperbolic Time Chamber; door back to zone 9 area 3 |
| 3 | `broly_arena.json` | New Planet Vegeta (Broly chapter) |
| 4 | `broly_crater.json` | needs `roms/buu.gba` |
| 5-19 | free (5-9, 10-14, 15-19 were allocated to the three wait sessions, unused) | |

## Memory (EWRAM; `src/coop/coop.ld`, exact layout in `$(BUILD)/coop.map`)

| address | size | what |
|---|---|---|
| 0x02000000-0x0203F000 | | the game's heap (capped by `patch_rom.py`) |
| 0x0203F000 | 0xB00 region | `partyram`: `g_party_block` (party, flags at +0xEC, 0x19C), `party_save` store (0x50C), `switch` (0x19C) |
| 0x0203F844 | 0xC | `.party.kr` Android 16 player state (`a16player.c`) |
| 0x0203F850 | 0x30 | `.party.kr` Krillin state (`krillin.c`) |
| 0x0203F880 | 0x9C | `.party.wv` `waves` (Time Chamber); moves when `.party.kr` grows |
| 0x0203F91C-0x0203FB00 | ~0x1E4 | free in `partyram` |
| 0x0203FB00 | 0x29C used, 0x2C0 max | `coop2` (`CoopExtra`, `coop.h`): about 0x24 bytes left, avoid adding fields |
| 0x0203FDC0 | 0x40 | `StoryTest` preset (headless `preset`, `STORY_TEST`) |
| 0x0203FE00 | 0xC0 | player 2's vtable copy |
| 0x0203FEC0 | | wide-view descriptor (`wide.h`) |
| 0x0203FF00 | 2 + 2 | `g_p2_remote`, `g_p1_remote` pad words |
| 0x0203FF04 | 0xE4 | `coop` (`CoopState`): `p2` +4, `p2_slot` +0x80, `exp_awards` +0xD8 |

Useful `coop2` words: `cmd` 0x0203FD08, `cmd_arg` 0x0203FD0C, `a16` 0x0203FD34, `a16_bot` 0x0203FD44,
`cmd_arg2` 0x0203FD54, `duel_active` / `duel_winner` 0x0203FD5C / 0x0203FD5D.

IWRAM and ROM: entity list 0x03001120, world/camera object 0x03001090, input 0x0300229C, player
object pointer 0x03001FA8 (position at +0x158 / +0x15C, 24.8), stock party 0x03000E90 (unused once
relocated); blob at 0x08800000, sprite index (relocated, stock at 0x083B4E74), ability table literal
0x08008024, CoopCall repointed at file 0x3B5E14, story-flag preset in ROM at 0xC00000. More in
`docs/engine.md`, `docs/rom-map.md` and `src/coop/rom.h`.
