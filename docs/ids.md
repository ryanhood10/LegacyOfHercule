# ID and address registry

Every number the mod has taken, who owns it, and what is free. **Take new numbers from the free
ranges here and update this page in the same commit.** Before parallel sessions start, give each one
its own range of every kind (`docs/howto.md` "Work in parallel sessions"). Last checked against
the code on 2026-10-19 (main e45c765).

## Allocations for the tournament build (2026-10-25, `src/coop/tourney.h`)

The tournament team (docs/tournament.md "The team") takes numbers only from its own row. Cross-piece calls go through
`tourney_hooks.c` (weak defaults; dispatch of CoopCall 23-30, test commands 21-30 and the per-frame tick), so no
piece edits `vmcall.c`, `enemies.c` or `input.c` for them (exception: moves made `is_coop_enemy` / `is_enemy` in
`enemies.c` also count a fighter with a kit, through `tny_moves_base()`). RAM: each piece's own `.tny.<name>` section in
`tourneyram` (0x0203EC00, 0x400 bytes; the heap cap now ends there). ROM: big generated art goes to `rom2`
(0x08E00000, 2 MiB) with `TNY_ART` (`.rodata.art`), written by `patch_rom.py --art`.

| piece | flags | stats | sprites | rows | portraits | abilities | CoopCall | test cmds | maps / strings | RAM |
|---|---|---|---|---|---|---|---|---|---|---|
| grounds | 441-445 (441 used) | - | NPC crowd reuses stock ids | - | - | - | 30 | 30 | zone 15 areas 11-17 (11-16 used); ui_strings 357-379 (357, 358 used) | `.tny.gr` 0x40 (0x24 used) |
| art-new | - | - | 185-189 (King Cold, Roshi, Chi-Chi, Videl, Chiaotzu) | - | 127-139 | - | - | - | - | - |
| art-buu | - | - | 195-229 (`tourney.h` TS_*), 230 the announcer | - | 140-189 | - | - | - | - | - |
| playables | - | - | 235-249 (player-layout LoG2 bodies) | 29-79 | - | 24-40 | - | - | - | - |
| engine | 446-459 | 160-219 | - | - | - | - | 23-27 | 21-25 | - | `.tny.eng` 0x240 |
| moves (built, branch `tny-moves`) | - | 220-229 (none used: stand-in stock stats for tests) | - | - | - | - | - | 26 spawn a fighter with his kit, 27 drive kits / Rumble | - | `.tny.mv` 0x54 used (`MvGlobal`, magic 0x4D4F5645 "MOVE"); kit state on the heap |
| ui | - | - | - | - | 190-199 | - | 28-29 | 28-29 | ui_strings 380-399 | `.tny.ui` 0x100 |

## Allocations for the Cell Games and post-game build (2026-10-19)

Each piece takes numbers only from its own row; say in its report if it needs more. RAM: nobody adds to
`coop2` (about 0x24 bytes left); a piece that needs state takes its own `.party.<name>` section in
`partyram` (about 0x1E4 free with 8 slots; 0x150 with 9, after Super Saiyan 2 Gohan), at most 0x40 bytes each.

| piece | flags | stats | sprites | rows | portraits | abilities | CoopCall | test cmds | zone 15 / other |
|---|---|---|---|---|---|---|---|---|---|
| Cell Games chapter (built) | 460-479 (none used) | 140 used | 162 (a record copy of Perfect Cell 30) | - | - | - | 15 Solo, 16 Beam | 15 SOLO, 16 WIN_STAT | maps 323-326 and 176 edits; `.party.cg` 0x18 |
| Super 13 boss (art, stats, AI) | - | 145-149 | 180-184 | - | 122-123 | - | 17 | 17-18 | - |
| Arctic route maps | 381-389 (381, 382 used) | - | - | - | - | - | - | - | areas 5-9 (5-8 used); area 1 reworked; world-map landing; place name `ui_strings` 356 |
| Super Saiyan 2 Gohan character | - | - | - (58 repointed) | 28 used, 29 unused | 124-125 unused (stock 61 / 12) | 23 used, 24 unused | 20 Ssj2Join used | - | party slot 8 (`PARTY_SLOTS=9`) |
| Post-game chapter (Lookout part built) | 480 the wishes made, 483 the finale seen (the new-game clear now covers 400-503) | 150-154 | - | - | - | - | 18 Choice used, 19 | 19-20 | Lookout (map 261 record 4), talk show, epilogue |

## Story flags (bank at 0x0203F108 + n / 8 with 9 party slots, 816 flags, saved)

The bank sits after the party entries, so it moves with `PARTY_SLOTS`: `0x0203F00C + N * 0x1C`
(0x0203F0EC with 8 slots). Headless scripts use the runner's `FLAGS` variable (`docs/testing.md`).

Stock scripts use flags up to 306. `Party_Init` clears 0-399 at a new game, and `party.c` also clears
400-503 (`PARTY_WAIT_FLAG_BYTES` 13). Each flag's meaning is in `docs/scripts.md` 2.5.

| range | owner | in use |
|---|---|---|
| 360-362 | story chapters 1-7 | 360 Krillin has visited, 361 inside Triceratops Jungle, 362 Hercule's deal |
| 363-375 | Roshi's gym (`ch8_gym`) | 363-375 |
| 376 | the removed arctic Android 13 encounter | free since 2026-10-18 (the scene is kept in `data/archive/ch8_arctic.lscript`, not built) |
| 377-379 | the Broly chapter's ship ride | 377 the players may board (Paragus' offer taken; his scene does not replay), 378 the ship landed on New Planet Vegeta, 379 out of the ship at home after the ride back (the room's door no longer leads anywhere) |
| 380-399 | new maps (`data/maps/*.json`, `data/story/arctic_route.*`) | 380 free (the arctic's Vegeta barrier, removed 2026-10-19); **381 the arctic is open** (the world-map landing and its star; the post-game chapter sets it at the explosion); **382 Super 13 beaten** (the star turns into the plain marker; the post-game chapter sets it); 383-389 the arctic route's, free |
| 400-419 | Broly chapter (`broly`) | 400 ship landed, 401 trip under way, 402 done, 403 confrontation seen, 404-419 party kept by CoopCall 10 |
| 420-439 | Gero's computer (`ch9_gero`, `src/coop/a16player.h`) | 420 running, 421 over, 422 rubble punched, 423 / 424 west / east generator down, 425 bunker done, 430-438 saved "available" bits of slots 0-8 (all but 6 and 7) |
| 440 | Lookout / dojo (`wait_spar`) | 440 Roshi's dojo booked for a spar |
| 441-445 | the tournament grounds (`data/story/papaya.lscript`, `tny_ring.h`) | 441 the plaza's first-visit welcome has played; 442-445 free |
| 446-459 | the tournament engine (`tourney.h` `TNY_FLAG_ENGINE0`) | |
| 460-461 | difficulty (`difficulty.h`): 460 Easy, 461 Hard, neither Normal; set by the new game's question |
| 462-463 | free | |
| 464-467 | the story's lead (slot + 1), noted by the SetActiveCharacter steps (`companion.c`) | |
| 468-479 | free (cleared at a new game) | |
| 480-503 | the post-game (480 wishes, 483 finale; cleared at a new game) | |
| 504-815 | free (not cleared at a new game: avoid, or extend `PARTY_WAIT_FLAG_BYTES` in `party.c`) | |

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
| 140 | the second Perfect Cell of the final fight (copy of stat 25) | 25 | Cell Games |
| 141-144 | reserved for the Cell Games, unused | | |
| 145+ | see the allocations above | | |
| 140-144 | allocated to the Cell Games chapter | | |
| 145 | Super Android 13 (own spawn path, `s13_stat`; `BOSS_NO_TWIN` by construction: no twin, no stat-table row) | (own row: 10000 HP, 48 / 64 / 56, AI set 21) | `super13.c` |
| 146-149 | Super 13's allocation, unused | | |
| 150+ | 150-154 allocated to the post-game chapter; 155+ free | | |

## Sprite ids (relocated `sprite_index`, stock 0-156, gaps are 0)

| id | what | source | Makefile variable |
|---|---|---|---|
| 58 | Super Saiyan 2 Gohan, **repointed**: `ssj2_record` = the stock record 58 (ROM groups) with hurt 11, lying 12 and tumble 20 from record 59 (party slot 8, row 28; the Cell Games' stock row 4 too) | `tools/ssj2sprites.py` | `EXTRA_SPRITES` `58=ssj2_record` |
| 72 | Hercule, **repointed**: `herc_record` = the stock record 72 (ROM groups) plus the Kamehameha pose in group 18 (Cell Games beam struggle) | `tools/herculesprites.py` | `EXTRA_SPRITES` `72=herc_record` |
| 157 | Super Saiyan Hercule | `tools/spriteclone.py` | `SSJ_SPRITE` |
| 158 | Hercule from Buu's Fury (old Hercule's body) | `tools/buu2log2.py` | `BUU_SPRITES` |
| 159 | Legendary Super Saiyan Broly (from Buu's Fury) | `tools/buu2log2.py` | `BUU_SPRITES` (hardcoded in `bosses.h`) |
| 160 | Android 16 boss | `tools/a16sprites.py` | `A16_SPRITE` |
| 161 | Krillin (playable) | `tools/krillinsprites.py` | `KRILLIN_SPRITE` |
| 162 | the second Perfect Cell: a copy of record 30 (header and group table; frames stay the ROM's) so scripts can address it apart | `tools/spriteclone.py` `@30` | `CELL2_SPRITE` |
| 163-164 | free | | |
| 165 | Android 13 | `tools/bossmodels.py` | `A13_SPRITE` |
| 166 | Cooler's Namekian servant | `tools/bossmodels.py` | `NAMEK_SPRITE` |
| 167-169 | free | | |
| 170 | Paragus | `tools/brolysprites.py` | `PARAGUS_SPRITE` (hardcoded in `broly.lscript`) |
| 171-174 | free (171 / 172 were the Saiyan ship's halves until 2026-10-21: it is map art now, `tools/shipart.py`) | | |
| 175 | Android 16 as a player | `tools/a16sprites.py` | `A16P_SPRITE` |
| 176 | haloed Goku (dead after the Cell Games), display row 26 | `tools/halosprites.py` | `HALO_GOKU_SPRITE` |
| 177 | haloed Super Saiyan Goku, display row 27 | `tools/halosprites.py` | `HALO_SSJ_SPRITE` |
| 178-179 | free (the hercule-beam session's allocation, unused) | | |
| 180 | Super Android 13 (Buu's Fury Broly recoloured; Perfect Cell's groups as a stand-in with `BUU=0`) | `tools/super13sprites.py` | `S13_SPRITE` |
| 181-184 | Super 13's allocation, unused | | |
| 235-244 | the tournament's player-layout LoG2 bodies (`tourney.h` `TS_P_*`): 235 Yamcha (156), 236 Tien (121), 237 Frieza (48), 238 Mecha Frieza (49 + 48's attacks), 239 Tao (50), 240 Android 13 (`a13_record`'s frames rebuilt), 241 Android 17 (9), 242 Android 18 (11), 243 Perfect Cell (30, 64 x 64), 244 Cooler (34, 64 x 64) | `tools/tnyplayables.py` (`TNY_ART`) | `TNY_PLAY_SPRITES` |
| 245-249 | the playables' allocation, unused | | |

Several ids are also written as literals in scripts and headers, so changing a Makefile default alone
leaves copies out of step: grep for the number before moving one.

## Display rows (`display_rows`, stock 0-21, 80 rows, row limit patched at file 0x42D6)

| row | character | sprite | portrait (dialogue / status) | built by |
|---|---|---|---|---|
| 22 | Super Saiyan Hercule | 157 | Hercule's | `spriteclone.py` |
| 23 | Krillin | 161 | 73 / 119 | `spriteclone.py`, `krillin.h` |
| 24 | old Hercule | 158 (72 when `BUU=0`) | Hercule's | `spriteclone.py`, `krillin.h` |
| 25 | Android 16 (player) | 175 | 26 / 121 | `spriteclone.py`, `a16player.h` |
| 26 | haloed Goku (links 27 / 27, as 19 / 20 do) | 176 | Goku's (63 / 14) | `spriteclone.py` `--halo-sprites` |
| 27 | haloed Super Saiyan Goku (flags 1, links 26 / 26) | 177 | SSJ Goku's (65 / 15) | `spriteclone.py` `--halo-sprites` |
| 28 | Super Saiyan 2 Gohan (party slot 8): Gohan's row 3 (standard class), a base form linking to itself | 58 | 61 / 12 (the stock Super Saiyan 2 Gohan's, from row 4) | `spriteclone.py` `SSJ2_ROW`, `ssj2gohan.h` |
| 29-66 | the tournament's playables, one per `TR_BEAT` / `TR_CHALLENGER` fighter of `tourney_data.c` in `TFighterId` order (29 Yamcha ... 47 Super 13, 48-52 the Buu-saga five, 53-66 the challengers; the map `tny_play_rows`, `tny_play_row()`): Gohan's standard row, links to itself | the table's `play_sprite` (72 when that record is not in the build; 180 -> 243 with `BUU=0`) | the table's portrait (Hercule's 67 when 0) / Hercule's status portrait | `tools/tnyplayables.py` `display_rows` (via `spriteclone.py`), `tny_play.c` |
| 67-79 | the playables' allocation, unused (Hercule's row as filler) | | | |
| 80+ | free (raise the limit in `patch_rom.py`, an imm8, and `EXTRA_ROWS` in `spriteclone.py`) | | | |

The stock row 4 (sprite 58, class `0x030021E4`: fixed combat bytes 100, HP regeneration, an aura sound
loop; flags 3, links to row 2) stays the Cell Games' transformation (`SetCharacterTransformation 0, 58`
finds row 4 first).

## Portraits (`portrait_module`, stock words 0-118 copied; word id + 1)

| id | what | defined |
|---|---|---|
| 119 | Krillin's status portrait | `krillinsprites.py` `KRILLIN_STATUS_PORTRAIT`, `krillin.h` |
| 120 | Paragus' dialogue portrait | Makefile `PARAGUS_PORTRAIT`, `--extra-portrait`; `text 120` in `broly.lscript` |
| 121 | Android 16's status portrait | `krillinsprites.py` / `spriteclone.py` `A16_STATUS_PORTRAIT` |
| 122+ | free (122-123 allocated to the Super 13 boss; 124-125 to Super Saiyan 2 Gohan, unused: his row uses the stock 61 / 12) | `krillinsprites.py` asserts ids are unique |
| 122 | Super Android 13's dialogue portrait (Android 16's made blue, white-haired) | Makefile `S13_PORTRAIT`, `--extra-portrait`; dialogue `text 122 "SUPER 13: ..."` |
| 123 | Super 13's allocation (base Android 13's portrait), unused | |
| 124+ | 124-125 allocated to Super Saiyan 2 Gohan | |
| 126 | old Hercule's dialogue portrait (Buu's Fury's Hercule, module 0x086F393C id 76, on LoG2's frame; stock 67 without `roms/buu.gba`) | Makefile `OLD_HERCULE_PORTRAIT`, `tools/oldherculeportrait.py`; `text 126` in `ch1_opening` and the epilogue |
| 127+ | free | `krillinsprites.py` asserts ids are unique |

Dialogue speaker ids worth knowing: 26 Android 16, 36 Bulma, 38 Cell, 67 Hercule, 73 Krillin
(more in `docs/scripts.md` 2.2).

## Ability table (`data/hercule.json` `abilities`, stock 0-13, numbered in order)

| id | ability | C |
|---|---|---|
| 14-17 | Rolling Hercule Punch, Dynamite Kick, Megaton Punch, Hercule Taunt | `hercule.c` |
| 18 | Super Hercule (transform) | `ssj.c` |
| 19-21 | Afterimage Rush, Shockwave Punch, Destructo Disc | `krillin.c` |
| 22 | Rocket Punch (Android 16) | `a16player.c` |
| 23 | Super Saiyan 2 Burst (Super Saiyan 2 Gohan) | `ssj2gohan.c` |
| 24-40 | allocated to the tournament's playables, unused (their kits use 0-23: `tny_play.c`) | |
| 41+ | free (24 was allocated to Super Saiyan 2 Gohan, unused) | |

## Party slots (`PARTY_SLOTS` = 9)

0 Gohan, 1 Piccolo, 2 Vegeta, 3 Trunks, 4 Goku, 5 Hercule (old Hercule shares this slot on row 24),
6 Krillin, 7 the duel clone (`DUEL_SLOT`; borrowed by Android 16 during Gero's computer), 8 Super Saiyan 2
Gohan (empty until `ssj2_join`). A tenth character needs `PARTY_SLOTS=10` (`docs/party-8-slots.md`: the
pause menu's tab strip shows a window of 8, the save layout changes again, every test address after the
entries moves); `partyram` holds 0x9B0 of 0xB00 bytes at 9 slots, and a 16-slot build overflows it.

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
| 12 | A16Leave (op: 0 the chapter's end, 16 stays selectable; 1 the Cell Games, 16 gone) | Gero's computer, the Cell Games |
| 13 | WaveStart (seq, round) | Time Chamber |
| 14 | Wave (op) | Time Chamber |
| 15 | Solo (mode): 0 both, 1 player 1 alone, 2 player 2 alone on player 1's entity | Cell Games |
| 16 | Beam (sprite, on): a held Kamehameha the script does not wait for; on 0 ends it | Cell Games |
| 17 | Super13 (op, a, b): op 0 tune the next spawn (HP %, blast damage %), op 1 -> state (3 = exploded), op 2 spawn beside player 1 | Super 13 (`super13.c`) |
| 18 | Choice (op): op 1 arms a yes / no question before its text entry (B answers no and closes the box), op 0 pushes 1 yes / 2 no | post-game |
| 19 | allocated: post-game chapter | |
| 20 | Ssj2Join (): Super Saiyan 2 Gohan joins (`ssj2gohan.c`; called at the end of the Cell Games) | Super Saiyan 2 Gohan |
| 21 | Players: the 1 / 2 players answer (`players.c`) | every game start |
| 22 | Scene actor guards (`companion.c`): op 1 slot unplayed, op 2 actor of a sprite present, op 3 the lead's name | `ch1_opening` (Gohan's actor, sprite 55) |
| 23-27 | the tournament engine (23: the director's whole flow; pushes 1 when it took over) | tournament (`tourney_hooks.c`) |
| 28-29 | the tournament ui (28 op 1: the unlock board) | tournament |
| 30 | Grounds (op, b): op 0 the waiting room's fighters (on-load), op 1 b -> 1 when sprite b has a record in this build, op 2 -> 1 when the grounds' maps are built (the landing's condition) | tournament grounds (`tny_grounds.c`) |
| 31+ | free | |

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
| 10 | DOWN | player `arg` (0 / 1) goes down as from a lethal hit (`revive.c`) |
| 11 | A16_PARTY | Krillin player 1, Android 16 player 2 |
| 12 | free | |
| 13 | WAVE_CLEAR | every standing enemy dies |
| 14 | WAVE_SKIP | the next announced round is `arg` |
| 15 | SOLO | solo mode `arg` (Cell Games) |
| 16 | WIN_STAT | the fight of stat `arg` wins itself once it stands and player 1 is free (tests write `cg_state + 5` directly: the command only runs between scenes) |
| 17+ | free | |
| 15-16 | allocated to the Cell Games chapter | |
| 17 | S13 | spawn Super Android 13 72 px right of player 1, with a boss bar |
| 18 | S13_DRIVE | `arg` low byte = forced special (1 volley, 2 fan, 3 bomber, 4 wave, 5 nova, 6 roar; 0 = his choice), \| 0x100 = his HP := `arg2`, \| 0x200 = toggle the autopilot (`A16_TEST`) |
| 19+ | 19-20 allocated to the post-game chapter | |
| 21-29 | the tournament team (`docs/ids.md` allocations above) | |
| 30 | TNY_GROUNDS | `arg` 0 the waiting room's real pool, 1 force the pool to the Rookie opponents (stand-in for the engine's `tny_in_pool`), 2 spawn today's fighters now (`tny_grounds.c`) |
| 19+ | 19-20 allocated to the post-game chapter; 21-30 the tournament team (above) | |
| 26 | TNY_SPAWN (moves) | `arg` = fighter (TF_*) \| spot << 8 (0 = 64 px right of player 1, 1-7 round him) \| flags << 16 (1 boss bar, 2 the last spawn's side, 4 no specials for 600 frames); `arg2` = stat (0 or >= 1000: the engine's row 160 + id, else a stand-in): spawn him with his kit (`tny_moves.c`) |
| 27 | TNY_DRIVE (moves) | `arg` = op << 24: 0 force move (`reg << 8 \| move`: 1-6 his kit's slot, 0x80 \| kind a library kind, 0xFB-0xFF Multi-Form / self-KO / stun beam / stun projectile / big ball), 1 Rumble = `arg & 1`, 2 HP of kit `reg` := `arg2` (0x800000NN = NN %), 3 no specials for `arg2` frames, 4 every kit fighter leaves, 5 a passive on top (11 drain, 12 regen) |

## Zone 15 areas (new maps, `data/maps/*.json`, built by `tools/mapbuild.py`)

| area | map | notes |
|---|---|---|
| 1 | `arctic.json` | the arctic landing at the foot of the mountain (post-game, world-map landing flag 381); barrier and the 13 encounter removed |
| 2 | `time_chamber.json` | Hyperbolic Time Chamber; door back to zone 9 area 3 |
| 3 | `broly_arena.json` | New Planet Vegeta (Broly chapter) |
| 4 | `broly_crater.json` | needs `roms/buu.gba` |
| 5 | `arctic_climb1.json` | arctic route, first climb (7 enemies) |
| 6 | `arctic_climb2.json` | arctic route, second climb (6 enemies) |
| 7 | `arctic_ridge.json` | arctic route, the ridge: save pad |
| 8 | `arctic_summit.json` | arctic route, the summit: Super 13's battle, flight spot |
| 9 | `saiyan_ship.json` | inside the Saiyan ship (the Broly chapter's ride both ways) |
| 10 | `yard_ship.json` | the Capsule Corp backyard (6/21) with the Saiyan ship parked: never warped to; its variation stands in for the backyard's while the ship is parked (`src/coop/shipyard.c`) |
| 11 | `papaya_plaza.json` | Papaya Island's plaza (Buu 7/1 v2): the world-map landing's beach and flight spot, save pad, the announcer, the first-visit welcome (flag 441); needs `roms/buu.gba` |
| 12 | `papaya_street.json` | the street with the stalls (Buu 7/2): vendor, reporter, fans |
| 13 | `papaya_courtyard.json` | the courtyard (Buu 7/3): the unlock board (CoopCall 28, 1) |
| 14 | `papaya_infirmary.json` | the infirmary corridor (Buu 7/4) between the courtyard and the waiting room |
| 15 | `papaya_waiting.json` | the fighters' waiting room (Buu 7/5), the hub: save pad, the director (CoopCall 23), today's fighters (CoopCall 30, 0) |
| 16 | `papaya_ring.json` | the ring with the stands (Buu 7/6, 720 x 720): the engine's stage (`src/coop/tny_ring.h`) |
| 17 | grounds, free | |
| 18-19 | free | |

## UI strings and place names (`ui_strings`, `tools/krillinsprites.py`, `src/coop/krillin.h`)

The relocated UI string table: stock entries 0-259 (menu labels, names, technique texts, and the map
names: a map name id is an index here), six per-slot blocks of 16 (`UI_ENERGY1` .. `UI_CAPS`, 260-355),
then `PLACE_NAMES` from 356 (`UI_PLACES`): names for new maps (a MapEntry name, a world-map banner
payload, the save menu's location; `party_save.c` reads this table). Adding a per-slot block moves the
places: `krillinsprites.py` asserts the index.

| id | text | used by |
|---|---|---|
| 356 | The Arctic | `data/maps/arctic*.json` (zone 15 areas 1, 5-8), the world-map banner (`data/story/arctic_route.json`) |
| 357 | Papaya Island | the plaza, street, courtyard (zone 15 areas 11-13), the world-map banner (`data/story/papaya_route.json`) |
| 358 | World Tournament | the infirmary, waiting room, ring (zone 15 areas 14-16) |
| 359-379 | grounds, free | |
| 380-399 | the tournament ui | |

## Memory (EWRAM; `src/coop/coop.ld`, exact layout in `$(BUILD)/coop.map`)

| address | size | what |
|---|---|---|
| 0x02000000-0x0203F000 | | the game's heap (capped by `patch_rom.py`) |
| 0x0203F000 | 0xB00 region | `partyram` (9 slots): `g_party_block` (party, flags at +0x108, 0x1B8), `party_save` store (0x564), `switch` (0x1B8) |
| 0x0203F8D4 | 0xC | `.party.kr` Android 16 player state (`a16player.c`) |
| 0x0203F8E0 | 0x34 | `.party.kr` Krillin state (`krillin.c`) |
| 0x0203F914 | 0x9C | `.party.wv` `waves` (Time Chamber; 0x0203F880 with 8 slots); moves when anything before it grows |
| 0x0203F9B0 | 0x1C | `.party.cg` `cg_state` (Cell Games: magic, solo mode, test bytes, held beams; the post-game's yes / no question) |
| 0x0203F9CC | 0x2C | `.party.s13` `s13` (Super Android 13) |
| 0x0203F9F8 | 0x34 | `.party.rv` `rv` (revive: bodies, countdowns, progress, bars' VRAM, glow table, counters; `revive.h`) |
| 0x0203FA2C | 0x18 | `.party.ui` the Cell Games' one-player title card (`cellgames.c` `card_buf`) |
| 0x0203FA54 | 0x8 | `.party.cp` companion state (`companion.c`) |
| 0x0203FA44 | 0x14 | `.party.ui` `players` (the 1 / 2 players choice: magic, mode, asking, the question's action) |
| 0x0203FA58-0x0203FB00 | 0xA8 | free in `partyram` (each `.party.<name>` section added before `.party.wv` moves `waves`) |
| 0x0203F000 | 0xB00 region | `partyram`: `g_party_block` (party, flags at +0xEC, 0x19C), `party_save` store (0x50C), `switch` (0x19C) |
| 0x0203F844 | 0xC | `.party.kr` Android 16 player state (`a16player.c`) |
| 0x0203F850 | 0x30 | `.party.kr` Krillin state (`krillin.c`) |
| 0x0203F880 | 0x9C | `.party.wv` `waves` (Time Chamber); moves when `.party.kr` grows |
| 0x0203F91C | 0x2C | `.party.s13` `s13` (Super Android 13: live boss, debug counters, force, state, tuning; `super13.h` `S13Global`); moves when the sections before it grow |
| 0x0203F948-0x0203FB00 | ~0x1B8 | free in `partyram` |
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
