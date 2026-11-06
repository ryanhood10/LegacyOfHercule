# ID and address registry

Every number the mod has taken, who owns it, and what is free. **Take new numbers from the free
ranges here and update this page in the same commit.** Before parallel sessions start, give each one
its own range of every kind (`docs/howto.md` "Work in parallel sessions"). Last checked against
the code on 2026-10-19 (main e45c765).

## The Time Chamber (2026-11-05, `docs/time-chamber.md` 12; branch `nb-chamber`)

| kind | taken | kept for the rest of the plan |
|---|---|---|
| flags | 504-509 a room's lever thrown (light n out); 512 the hidden room's first scene seen, 513 Popo beaten (his first reward paid), 514-519 room n's first win paid (its +3 capsule), 520-521 times round (bits 0 / 1, 0-3), 530 Popo beaten this time round (he stands and talks; cleared by the loop and by leaving) (`popo.h`, branch `nb-chamber-popo`); 510 a run under way: its opening has played (cleared by leaving), 531-535 the side rooms' finds taken, by area 27-31 (the Saiyan ship's bedroom, its store room, Frieza's crystal room, his control room, the Saiyan ship's lounge) (`chamber.h`, branch `nb-chamber2`, `docs/time-chamber.md` 13, 13.6) | 510-535 (cleared at a new game by `party.c` `party_new_game`): 511 the gate's fall seen, 522-524 the panel, 525-529 the trophy hall; none free |
| stats | 155 `CH_SPOT_STAT`: an open-area spot record (the tier picks the real stat); 156 `POPO_STAT` Mr. Popo (`popo.c` `popo_stat`: the rating curve at 500 + 50 a time round; `bosses.c` rule `BOSS_NO_TWIN`, hp_pct 100: the lone boss's 180 % with two players) | 157 the crew if recoloured (section 6.2; not needed so far: the airship's crew is the stock Henchmen 12 / 51) |
| CoopCall | 35 Chamber (op, a, b): `chamber.c` `chamber_call`, ops in its enum (1 room load .. 20 open the door, 21 / 22 the lead's step after his crew; op 4 takes `b` = `seq:ch_crew_parts`; 23-53 since `nb-chamber2`, `docs/time-chamber.md` 13: 23 the open area's on-load (`a` = `seq:ch_open_scenes`), 24 its scene's route, 25-33 the opening (stage, look, go / skip, a room's view: actors, shots, area, going, the end), 34-37 the door home (stage, route, step back, the run reset), 38 the training's end, 39 the portal lit, 40-45 the lever's way to the gate (start, legs), 46 a room's side doorways, 47-49 the prize and its box, 50 Gohan's gift, 51 / 52 a side room's find, 53 the portal's warp); 36 Popo (op, a, b): `popo.c` `popo_call`, ops in its enum (1 the hidden room's load .. 18 a big jar, 19 / 20 a room's prize, 21 / 22 the view on player 1, 23 beaten this time round, 24 the talk's route) | |
| test commands | 48 CHAMBER (`chamber_test`): `arg` 0 the levers := `arg2` bits 0-5; 1 `cmd_arg` := bodies at the last check \| the smallest largest-free OBJ VRAM block (32-byte units) << 16; 2 records with two players := `arg2`; 3 the refill frozen := `arg2`; 4 reset that smallest block; 5 the old rounds on 15/2 from round `arg2` (no captions); 6 `cmd_arg` := lights out \| the room's state << 8 \| raged << 16 \| the crew stood up << 24 (states: 0 idle, 1 armed, 2 fight, 3 fall, 4 line, 5 blink, 6 won, 7 the crew fights, 8 the crew's scene); op 0 also sets flag 510 (no opening at the next load); 7 flag 510 := `arg2`; 8 `cmd_arg` := the open area's scene \| bits << 8 \| the last prize << 16 (a scene running holds test commands: read `.combo.ch` +0x30 instead); 49 POPO (`popo_test`): `arg` 0 `cmd_arg` := popo.c's state \| times round << 8 \| flag 512 << 16 \| 513 << 17 \| this win is the first << 18 \| flag 530 << 19; 1 his HP := `arg2` (bit 31: `arg2 & 0xFF` percent); 2 the autopilot := `arg2`; 3 `cmd_arg` := his far hops \| dodges << 16; 4 times round := `arg2` (0-3); 5 `cmd_arg` := the fight's frames \| his HP << 16; 6 his max HP; 7 his distance from the nearer player (px); 8 the intro's entries for this lineup (player 1's \| player 2's << 8). Test commands run between scenes only | |
| zone 15 areas | 18 the open area (`chamber_open.json`), 19 Roshi's dojo (`chamber_dojo.json`), 20 the pirate airship, 21 Frieza's ship, 22 the Namek ship, 23 the lab, 24 the Saiyan ship (`chamber_<room>.json`; 20, 21, 24 have a `chamber_<room>_plain.json` built only without `roms/buu.gba`), 25 the hidden room (`chamber_popo.json`, Mr. Popo), 27 the Saiyan ship's bedroom (`chamber_saiyan_bed.json`), 28 its store room (`chamber_saiyan_store.json`), 29 Frieza's crystal room (`chamber_frieza_crystal.json`), 30 his control room (`chamber_frieza_control.json`), 31 the Saiyan ship's lounge (`chamber_saiyan_lounge.json`; 27-31 need `roms/buu.gba`); 21 is his throne room since 13.6 | 26 the quarters (the panel) |
| RAM | `.combo.ch` 0x34 at most (`ChState`, magic 0x4348414D "CHAM"; after `.combo.ag`; 0x30 until `nb-chamber2`): +4 room state, timer, target, frozen; +8 / +0xC the room's fighters; +0x10 the drop; +0x14 the smallest largest-free OBJ VRAM block; +0x16 spawned; +0x18 removed; +0x1E bodies at the last check; +0x20 the rating being spawned (id, percent, rating); +0x24 the lever's room (rooms[] index), +0x25 the crew stood up, +0x26 raged, +0x27 the last prize; +0x28 `ch_crew_parts`; +0x2C `ch_open_scenes`; +0x30 the open area's scene (kind 1 opening, 2 the door, 3 the training's end, 4 the portal; 0x40 running, 0x80 skip), +0x31 bits (1 at the door, 2 the training's end seen, 4 the portal lit, 8 its phase, 0x10 the door's second question asked), +0x32 the portal's frames, +0x33 a queued scene's wait; `.combo.po` 0x28 of at most 0x30 (`popo.c` `PopoState`, magic 0x4F504F50 "POPO"; after `.combo.ch`, 0x0203EBBC since `nb-chamber2`): +4 state, timer, autopilot, first win; +8 the fighter; +0xC the three scenes; +0x18 the scene's action; +0x20 the fight's frames; +0x22 the autopilot's pads | |
| sprites, portraits | 163 `POPO_SPRITE` Mr. Popo (`tools/popoart.py`); no new portrait (Popo's stock 82) | 164, 167-169, 171-174 free for a crew |

## Allocations for the perfecting pass (2026-11-01, `docs/perfect-pass.md` 7)

Each piece takes numbers only from its own row and says in its report if it needs more. RAM: a story piece takes a
`.party.<name>` section in `partyram` (at most 0x40), a tournament piece a `.tny.<name>` section in `tourneyram`
(at most 0x10; about 0x50 left). Nobody adds to `coop2`.

| piece (branch) | flags | CoopCall | test cmds | portraits | RAM |
|---|---|---|---|---|---|
| story (`pp-story`) | 468-475 (cleared at a new game; 468 used) | 31-32 (31 used) | 34-35 (none used) | - | `.party.pp` 0x40 allotted, **0x10 used** (0x0203FAF0, the test win's record `cg_note_win`, after `.party.ssj`): `partyram` is then full |
| ssj (`pp-ssj`) | 476-477 (none used) | - | 36 (none used) | - | `.party.ssj` 0x10 (used: `ssj_state`, after `.party.bv`) |
| tny (`pp-tny`) | - | - | 37-38 (both used) | - | `.tny.cap` 0x10 (used, 0x0203EFAC); `.tny.eng`'s old padding (the announcer's walks, the reload's frames: `TnyEng` +0x155..+0x167) |
| art (`pp-art`) | - | - | - | 169-172 (used: the four portraits) | - |
| combo (`pp-combo`) | - | - | 39 (not used) | - | in `comboram`'s free 0xB8: `.combo.run` +0x88..+0x91, the copies' state (`RunRam` 0x92 of 0x140) |
| map (`pp-map`) | 478-479 (not needed: unused, free) | - | - | - | - |
| regen + pickup (main, 2026-11-01, Ryan's two-player session) | - | - | - | - | `.combo.rg` 4 bytes in `comboram` after `.combo.run` (the resting timers, `regen.c`; `ewram` is full) |

## Allocations for the second two-player round (2026-11-02, `docs/perfect-pass.md` 11.8)

Each piece takes numbers only from its own row. `ewram`, `partyram` and `coop2` are full: RAM is a section in `comboram`
(0x0203EA00, 0x200 bytes, 0x148 used before this round) or, for the tournament, `tourneyram`'s last 0x44.

| piece (branch, worktree `F:/MeltyCreations/log2-<piece>`) | flags | CoopCall | test cmds | RAM |
|---|---|---|---|---|
| tny (`pp2-tny`, `log2-pp2-tny`) | - | - | 40 used, 41 free | `.tny.cp` 0x10 of at most 0x20 (`tny_engine.c`); `UiState.vs` in `.tny.ui`'s padding (+141) |
| agency (`pp2-agency`, `log2-pp2-agency`) | 478-479 (not used) | 33-34 (not used) | 42-43 (not used) | `.combo.ag` 0x30 of 0x40 (`agency.c` `AgState`, `player.c`'s regroup) |
| regen (`pp2-regen`, `log2-pp2-regen`) | - | - | 44 (used) | `.combo.rg` grows to at most 0x10 (**0xC used**: the resting and ki timers) |
| perf (`pp2-perf`, `log2-pp2-perf`) | - | - | 45-46 (45 used, 46 free) | `.combo.pf` at most 0x10 (8 used: `obj_room`, `src/coop/objroom.h`; after `.combo.rg`, so read its address from `coop.sym`) |

## Allocations for the tournament build (2026-10-25, `src/coop/tourney.h`)

The tournament team (docs/tournament.md "The team") takes numbers only from its own row. Cross-piece calls go through
`tourney_hooks.c` (weak defaults; dispatch of CoopCall 23-30, test commands 21-30 and the per-frame tick), so no
piece edits `vmcall.c`, `enemies.c` or `input.c` for them (exception: moves made `is_coop_enemy` / `is_enemy` in
`enemies.c` also count a fighter with a kit, through `tny_moves_base()`). RAM: each piece's own `.tny.<name>` section in
`tourneyram` (0x0203EC00, 0x400 bytes); the team combos' sections in `comboram` (0x0203EA00, 0x200 bytes; the heap cap
now ends there, `patch_rom.py` 0xF87E words). ROM: big generated art goes to `rom2`
(0x08E00000, 2 MiB) with `TNY_ART` (`.rodata.art`), written by `patch_rom.py --art`.

| piece | flags | stats | sprites | rows | portraits | abilities | CoopCall | test cmds | maps / strings | RAM |
|---|---|---|---|---|---|---|---|---|---|---|
| grounds | 441-445 (441, 442 used) | - | NPC crowd reuses stock ids | - | - | - | 30 | 30 | zone 15 areas 11-17 (all used); ui_strings 357-379 (357-359 used) | `.tny.gr` 0x40 (0x24 used) |
| hpbars (main) | - | - | - | - | - | - | - | - | - | `.tny.hp` 8 |
| talk (main) | - | - | - | - | - | - | - | 31 | - | `.tny.talk` 0x58 |
| combo lead (main, 2026-10-31) | - | - | - | - | - | - | - | - | - | the pair table (`tny_combo.c`), `comboram` itself |
| combo meter (built, `tny-combo-meter`) | - | - | - | - | - | - | - | 32 | - | `.combo.meter` 0x80 (0x0203EA00), `.combo.cap` 0x30 (0x0203EA80), both padded to exactly their size |
| combo runner (built, `tny-combo-run`) | - | - | - | - | - | - | - | 33 | - | `.combo.run` 0x92 of 0x140 (0x0203EAB0; 0x88 until the perfecting pass) |
| art-new | - | - | 185-189 (King Cold, Roshi, Chi-Chi, Videl, Chiaotzu) | - | 127-139 | - | - | - | - | - |
| art-buu | - | - | 195-229 (`tourney.h` TS_*), 230 the announcer | - | 140-189 | - | - | - | - | - |
| playables | - | - | 235-249 (player-layout LoG2 bodies) | 29-79 | - | 24-40 | - | - | - | - |
| engine | 446-459 (none used yet) | 160-219 (160-209 used: 160 + fighter id) | - | - | - | - | 23 (24-27 free) | 21-25 | EEPROM 0x1E00-0x1E3F (the global record) | `.tny.eng` 0x240 (0x228 used), `.tny.eq` 0x10 |
| moves (built, branch `tny-moves`) | - | 220-229 (none used: stand-in stock stats for tests) | - | - | - | - | - | 26 spawn a fighter with his kit, 27 drive kits / Rumble | - | `.tny.mv` 0x7C used (`MvGlobal`, magic 0x4D4F5645 "MOVE"; +0x54 the fight-fix counters, docs/testing.md); kit state on the heap |
| ui (built, branch `tny-ui`) | - | - | - | - | 190-199 (none used) | - | 28 (a=1 the unlock board, a=2 the bracket phase b; pushes 1), 29 (the select for player a at tier b; pushes the id or -1) | 28 open a screen (arg 1 / 2 list boxes, 3 select (arg2 player \| tier << 8), 4 bracket (arg2 phase), 5 Rumble board (arg2 fallen mask), 6 unlock board, 7 pop-up from the next frame (arg2 0 fighter, 1 tier, 2 division), 8 the tier list (arg2 the locked mask), 9 the champion's screen (arg2 fighter \| fighter 2 << 8 (0xFF none) \| tier << 16 \| division << 24)), 29 test stand-ins (arg 0 clear, 1 / 2 unlock bits 0-31 / 32-63 = arg2, 3 fake bracket (arg2 size \| coop << 8 \| 2 v 2 << 9), 4 play its round (arg2 1 = the players win), 5 tier mask = arg2) | ui_strings 380-399 (none used: labels are C strings) | `.tny.ui` 0x8C used (`UiState`, magic 0x7A1C0002; +0x7C / +0x7E OBJ VRAM before / after the last screen, +0x88 a screen waits for a button); screen state on the heap |

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
400-503 (`PARTY_WAIT_FLAG_BYTES` 13); a new game (not a Map Test) also clears the Time Chamber's 504-535 (`party_new_game`, 2026-11-05;
old saves read them as clear, `docs/time-chamber.md` 12.2). Each flag's meaning is in `docs/scripts.md` 2.5.

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
| 441-445 | the tournament grounds (`data/story/papaya.lscript`, `tny_ring.h`) | 441 the plaza's first-visit welcome has played; 442 Pikkon's Senzu bean taken (the hidden arena); 443-445 free |
| 446-459 | the tournament engine (`tourney.h` `TNY_FLAG_ENGINE0`) | |
| 460-461 | difficulty (`difficulty.h`): 460 Easy, 461 Hard, neither Normal; set by the new game's question |
| 462 | Krillin's numbers lifted (`krillin.h` `KRILLIN_LIFT_FLAG`): set at every new game; a save without it gets `krillin_lift` once on load (2026-11-01) | |
| 463 | free | |
| 464-467 | the story's lead (slot + 1), noted by the SetActiveCharacter steps (`companion.c`) | |
| 468-475 | the perfecting pass's story piece (`pp-story`) | 468 Gohan's Super Saiyan given at the Time Chamber's first lever thrown (`chamber.lscript` `ch_lever`; until 2026-11-05 the first "Training completed!"); 469-475 free |
| 476-478 | the generator's eggs (`data/story/ch4_eggs.*`): 476 the mother's fight begun, 478 the eggs hatched, 477 she is down | |
| 479 | free (the map piece's: the allocation table above) | |
| 480-503 | the post-game (480 wishes, 483 finale; cleared at a new game) | |
| 504-535 | the Time Chamber (`chamber.h`, `popo.h`): 504-509 room 0-5's lever thrown, 510 a run under way, 512-521 and 530 Popo, the first wins, the rounds, 531-533 the side rooms' finds, the rest kept (the table at the top) | cleared at a new game |
| 536-815 | free (not cleared at a new game: avoid, or extend `PARTY_WAIT_FLAG_BYTES` in `party.c`) | |

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
| 117 / 118 | the mother dinosaur at the generator's eggs (`bosses.c`, `BOSS_NO_TWIN`; `data/story/ch4_eggs.*`), to half her health / the half left | 101 | zone 8 area 50 |
| 119 | the mother dinosaur's babies (`bosses.c`: a stat 94's numbers on stat 54's class and AI set) | 54 | zone 8 area 50 |
| 120 / 121 | the bunker's two Android 19s (`gero_stats[]`) | 5 | Gero's computer |
| 122-124 | reserved for Gero's computer, unused | | |
| 125-139 | allocated to the Lookout session, unused | | |
| 140 | the second Perfect Cell of the final fight (copy of stat 25) | 25 | Cell Games |
| 141-144 | reserved for the Cell Games, unused | | |
| 145+ | see the allocations above | | |
| 140-144 | allocated to the Cell Games chapter | | |
| 145 | Super Android 13 (own spawn path, `s13_stat`; `BOSS_NO_TWIN` by construction: no twin, no stat-table row) | (own row: 10000 HP, 48 / 64 / 56, AI set 21) | `super13.c` |
| 146-149 | Super 13's allocation, unused | | |
| 150+ | 150-154 allocated to the post-game chapter | | |
| 155 | the Time Chamber's spot record (`CH_SPOT_STAT`): never a real stat, `chamber_spot` replaces it by the tier's | | `chamber.c` |
| 156 | the Time Chamber's Mr. Popo (`POPO_STAT`): `popo_stat` reads the tournament's rating curve at 500 + 50 a time round (5200 HP, 40 / 42 / 42 at round 0) | 29 (the boss class) | `popo.c`, `bosses.c` |
| 157-159 | 157 kept for the Time Chamber (a crew); 158-159 free | | |
| 160-209 | the tournament's opponents: stat 160 + fighter id (`tourney.h` TF_*), resolved per tournament tier (`tny_engine.c` `tny_resolve_stat`; the rule `BOSS_NO_TWIN \| BOSS_FLAT`); 16 and Super 13 keep 109 / 145 | (boss class of 29; Cell Jr 26) | engine |
| 210-219 | the engine's, free | | |

## Sprite ids (relocated `sprite_index`, stock 0-156, gaps are 0)

| id | what | source | Makefile variable |
|---|---|---|---|
| 58 | Super Saiyan 2 Gohan, **repointed**: `ssj2_record` = the stock record 58 (ROM groups) with hurt 11, lying 12 and tumble 20 from record 59 (party slot 8, row 28; the Cell Games' stock row 4 too) | `tools/ssj2sprites.py` | `EXTRA_SPRITES` `58=ssj2_record` |
| 72 | Hercule, **repointed**: `herc_record` = the stock record 72 (ROM groups) plus the Kamehameha pose in group 18 (Cell Games beam struggle) | `tools/herculesprites.py` | `EXTRA_SPRITES` `72=herc_record` |
| 157 | Super Saiyan Hercule (22 groups: group 21 the turn back, `tools/ssjhercule.py`) | `tools/spriteclone.py` | `SSJ_SPRITE` |
| 158 | Hercule from Buu's Fury (old Hercule's body) | `tools/buu2log2.py` | `BUU_SPRITES` |
| 159 | Legendary Super Saiyan Broly (from Buu's Fury) | `tools/buu2log2.py` | `BUU_SPRITES` (hardcoded in `bosses.h`) |
| 160 | Android 16 boss | `tools/a16sprites.py` | `A16_SPRITE` |
| 161 | Krillin (playable) | `tools/krillinsprites.py` | `KRILLIN_SPRITE` |
| 162 | the second Perfect Cell: a copy of record 30 (header and group table; frames stay the ROM's) so scripts can address it apart | `tools/spriteclone.py` `@30` | `CELL2_SPRITE` |
| 163 | Mr. Popo, the Time Chamber's hidden room (Buu's Fury r.183 in Hercule's 21-group layout, hit-stun, knock-down and lying drawn by code; LoG2's 93 with `BUU=0`) | `tools/popoart.py` (`.rodata.art` blob) | `POPO_SPRITE` (a literal in `data/maps/chamber_popo.json`) |
| 164 | free | | |
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
| 245, 246 | the mother dinosaur, her babies: King Triceratops (127) and a triceratops (125) recoloured (`tools/dinosprites.py`, `data/story/ch4_eggs.*`); her dialogue portrait 173 ("Portraits") | | |
| 247-249 | the playables' allocation, unused | | |
| 195-222 | the tournament's Buu's Fury fighters (art-buu, `tourney.h` TS_*): 195 Spopovich (Buu r.242), 196 Yamu (293), 197 Pui Pui (224), 198 Yakon (290), 199 Kibito (148), 200 Pikkon (214), 201 Dabura (63), 202 Fat Buu (33), 203 Evil Buu (32), 204 Kid Buu (37), 205 / 206 / 207 Super Buu, Gotenks absorbed, Gohan absorbed (40 / 41 / 43), 208 Super Janemba (144), 209 Goten SSJ (114), 210 Kid Trunks SSJ (260), 211 / 212 / 213 Gotenks, SSJ, SSJ3 (116 / 119 / 120), 214 Vegito SSJ (92), 215 Uub (263), 216 / 217 / 218 Goku, SSJ, SSJ3 (105 / 108 / 110), 219 Majin Vegeta (274), 220 Mystic Gohan (100), 221 Great Saiyaman (101), 222 Piccolo with the cape (212); Hercule's 21-group layout, second ROM blob | `tools/buu2log2.py --spec data/tny_buu.txt --fit --section .rodata.art` | `TNY_BUU_IDS` (with `BUU=1`) |
| 223-229 | art-buu's allocation, unused | | |
| 230 | the World Tournament announcer (Buu r.8, NPC: idle, walk, mic raised in group 4; town NPC 80's 5-group layout) | `data/tny_buu.txt` `230=8 @80` | `TNY_BUU_IDS` |
| 185 | King Cold (tournament; Cooler's 64x64 set 34 recoloured, Cold's head from 74, a cape; player layout + AI set 21) | `tools/tnysprites.py` | `KING_COLD_SPRITE` (`tourney.h` `TS_KING_COLD`) |
| 186 | Master Roshi (tournament; Krillin's 76 + Goku's 60 groups re-headed with Roshi's 88 head and beard, a shell) | `tools/tnysprites.py` | `ROSHI_SPRITE` (`TS_ROSHI`) |
| 187 | Chi-Chi (tournament; Android 18's set 11 with Chi-Chi's 33 head, her LoG2 outfit) | `tools/tnysprites.py` | `CHICHI_SPRITE` (`TS_CHICHI`) |
| 188 | Videl (tournament; Android 18's set 11 with Buu's Fury's Videl head, record 276, white shirt) | `tools/tnysprites.py` | `VIDEL_SPRITE` (`TS_VIDEL`) |
| 189 | Chiaotzu (tournament; kid Gohan's set 53 with Chiaotzu's 32 head and outfit) | `tools/tnysprites.py` | `CHIAOTZU_SPRITE` (`TS_CHIAOTZU`) |

Several ids are also written as literals in scripts and headers, so changing a Makefile default alone
leaves copies out of step: grep for the number before moving one.

## Display rows (`display_rows`, stock 0-21, 80 rows, row limit patched at file 0x42D6)

| row | character | sprite | portrait (dialogue / status) | built by |
|---|---|---|---|---|
| 22 | Super Saiyan Hercule: transformed (flag bit 0, back to 21), the Super Saiyans' class `0x030021EC` | 157 | Hercule's | `spriteclone.py` |
| 23 | Krillin | 161 | 73 / 119 | `spriteclone.py`, `krillin.h` |
| 24 | old Hercule | 158 (72 when `BUU=0`) | Hercule's | `spriteclone.py`, `krillin.h` |
| 25 | Android 16 (player) | 175 | 26 / 121 | `spriteclone.py`, `a16player.h` |
| 26 | haloed Goku (links 27 / 27, as 19 / 20 do) | 176 | Goku's (63 / 14) | `spriteclone.py` `--halo-sprites` |
| 27 | haloed Super Saiyan Goku (flags 1, links 26 / 26) | 177 | SSJ Goku's (65 / 15) | `spriteclone.py` `--halo-sprites` |
| 28 | Super Saiyan 2 Gohan (party slot 8): Gohan's row 3 with row 4's class `0x030021E4` (slot 1 replaced by `regen.c`'s rules), a base form linking to itself | 58 | 61 / 12 (the stock Super Saiyan 2 Gohan's, from row 4) | `spriteclone.py` `SSJ2_ROW`, `ssj2gohan.h` |
| 29-66 | the tournament's playables, one per `TR_BEAT` / `TR_CHALLENGER` fighter of `tourney_data.c` in `TFighterId` order (29 Yamcha ... 47 Super 13, 48-52 the Buu-saga five, 53-66 the challengers; the map `tny_play_rows`, `tny_play_row()`): Gohan's standard row, links to itself | the table's `play_sprite` (72 when that record is not in the build; 180 -> 243 with `BUU=0`) | the table's portrait (Hercule's 67 when 0) / Hercule's status portrait | `tools/tnyplayables.py` `display_rows` (via `spriteclone.py`), `tny_play.c` |
| 67-79 | the playables' allocation, unused (Hercule's row as filler) | | | |
| 80+ | free (raise the limit in `patch_rom.py`, an imm8, and `EXTRA_ROWS` in `spriteclone.py`) | | | |

The stock row 4 (sprite 58, class `0x030021E4`: fixed combat bytes 100, an aura sound loop; its stock HP
regeneration replaced, `docs/perfect-pass.md` 11.1; flags 3, links to row 2) stays the Cell Games' transformation (`SetCharacterTransformation 0, 58`
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
| 250 | pseudo id, no portrait of its own: player 1's own character's portrait, never the lead's (`companion.c` `PLAYER1_PORTRAIT`; the story piece, 2026-11-01) | `data/story/player_lines.json` (`fmt 250 "%s: ..."`) |
| 126 | old Hercule's dialogue portrait (Buu's Fury's Hercule, module 0x086F393C id 76, on LoG2's frame; stock 67 without `roms/buu.gba`) | Makefile `OLD_HERCULE_PORTRAIT`, `tools/oldherculeportrait.py`; `text 126` in `ch1_opening` and the epilogue |
| 128-139 | art-new's allocation (tournament), unused | |
| 140-168 | the tournament's dialogue portraits from Buu's Fury (its portrait module 0x086F393C, on LoG2's grey frame; LoG2's Hercule 67 for every id with `BUU=0`): 140 the announcer (Buu portrait 10), 141 Broly (16), 142 Spopovich (126), 143 Yamu (160), 144 Pui Pui (119), 145 Yakon (116), 146 Kibito (83), 147 Pikkon (113), 148 Dabura (24), 149 Fat Buu (92), 150 Kid Buu (85), 151 Super Buu (127), 152 Super Buu, Gotenks absorbed (130), 153 Super Buu, Gohan absorbed (129), 154 Janemba (81), 155 Goten SSJ (65), 156 Kid Trunks SSJ (139), 157 / 158 / 159 Gotenks, SSJ, SSJ3 (68 / 71 / 73), 160 Vegito SSJ (151), 161 Uub (142), 162 / 163 / 164 Goku, SSJ, SSJ3 (57 / 59 / 61), 165 Majin Vegeta (148), 166 Mystic Gohan (42), 167 Great Saiyaman (47), 168 Piccolo, Buu saga (112). Evil Buu has none in Buu's Fury. | Makefile `TNY_PORTRAITS`, `tools/tnyportraits.py` (`tny_portrait_<id>`), `--extra-portrait`; `tourney_data.c`'s portrait column |
| 169-172 | dialogue portraits for the fighters that had none (the perfecting pass, `docs/tournament.md` 24.2): 169 Android 13 (since 2026-11-06 Tien's 95 under a yellow trucker cap that says 13, long white hair, a yellow vest, the androids' red background; it was Android 16's 26 with a cap), 170 Cell Jr. (Perfect Cell's 40 in blue), 171 the Namekian Warrior (Piccolo's 94 teal, a maroon gi), 172 Evil Buu (Buu's Fury's Super Buu 127 grey; LoG2's Hercule 67 without `roms/buu.gba`) | Makefile `PP_PORTRAITS`, `tools/ppportraits.py` (`pp_portrait_<id>`, second ROM blob), `--extra-portrait`; `tourney_data.c`'s portrait column |
| 173 | the mother dinosaur's dialogue portrait (King Triceratops' stock 96, the bestiary's Triceratops King, recoloured as her sprite 245: the frill pink, the face orange; taken from art-buu's unused 173-189, 2026-11-05) | Makefile `PP_PORTRAITS` `173=dino`, `tools/ppportraits.py` (`pp_portrait_173`), `--extra-portrait`; `text 173 "@MOTHER DINOSAUR: ..."` in `ch4_eggs.lscript` |
| 174-189 | art-buu's allocation, unused | |
| 190-199 | ui's allocation (tournament) | |
| 127 | Videl's dialogue portrait (Buu's Fury's short-haired Videl, module 0x086F393C id 153, on LoG2's frame; stock 43 Chi-Chi without `roms/buu.gba`); the tournament's other new fighters use stock portraits: King Cold 70, Roshi 80, Chi-Chi 43, Chiaotzu 42 (`tourney_data.c`) | Makefile `VIDEL_PORTRAIT`, `tools/tnysprites.py` `tny_videl_portrait`, `--extra-portrait` |
| 128-139 | art-new's allocation, unused | |
| 140+ | tournament allocations (art-buu 140-189, ui 190-199) | `krillinsprites.py` asserts ids are unique |

Dialogue speaker ids worth knowing: 26 Android 16, 36 Bulma, 38 Cell, 67 Hercule, 73 Krillin
(more in `docs/scripts.md` 2.2).

## Ability table (`data/hercule.json` `abilities`, stock 0-13, numbered in order)

| id | ability | C |
|---|---|---|
| 14-17 | Flash (Rolling Hercule Punch in the tournament's kits), Ki Blast (Dynamite Kick there), Megaton Punch, Hercule Taunt | `hercule.c` |
| 18 | Super Hercule (the stock Transformation's rules; icons `herc_ssj_icon_0/1`) | `ssj.c` |
| 19-21 | Afterimage Rush, Shockwave Punch, Destructo Disc | `krillin.c` |
| 22 | Rocket Punch (Android 16) | `a16player.c` |
| 23 | Super Saiyan 2 Burst (Super Saiyan 2 Gohan) | `ssj2gohan.c` |
| 24 | Energy Steal (the tournament's Spopovich and Yamu as players) | `tny_play.c` |
| 25 | Hercule's Gun | `hercule.c`, `krillin.c` `kiblast_spawn_hook` |
| 26-40 | allocated to the tournament's playables, unused | |
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
| 22 | Scene presence (`companion.c`): op 1 slot unplayed, op 2 actor of a sprite present, op 3 speaker 1's name (the lead in the scene, else player 1), op 4 slot played (2026-11-05: who is in a scene) | every staged chapter (`ch1_opening` .. `cell_games`) |
| 31 | P2Actor (`companion.c` `p2_actor_call`, the story piece): player 2's entity handed to a scene actor (op 0 stands?, 1 / 2 x / y, 3 remove) | `ch1_opening.json` (old Hercule: the knock-out, Gohan's death) |
| 23 | Director (op): op 0 the director's whole flow (pushes 1 when the engine took over, 0 when it cannot start now); op 1 pushes 1 while a tournament runs; op 2 the announcer's line opens: staged, nothing pushed (`say_wait_ann`, `docs/tournament.md` 27) (`tny_engine.c`) | the waiting room's director (`papaya.lscript`); the ring's lines |
| 24 | TalkLine (): a fight's talk reaches its next line: staged (`tny_stage_line`), pushes that line's slot (the end past the last) (`tny_talk.c` `tny_talk_call`, `docs/tournament.md` 27) | the talk's ROM `line_hook` entry |
| 25-27 | the tournament engine, free | |
| 28-29 | the tournament ui (28 op 1: the unlock board) | tournament |
| 30 | Grounds (op, b, c): op 0 the waiting room's fighters (on-load), op 1 b -> 1 when sprite b has a record in this build, op 2 -> 1 when the grounds' maps are built (the landing's condition), 3 player 1's name, 4 the courtyard's crowd, 5 an item, 6 sprite b's entities play group c; op 7 player 2 faces b, op 8 sprite b's actors hold group c & 0xFF's last frame (0 idle), facing (c >> 8) - 1 (2026-11-05, `docs/tournament.md` 27) | tournament grounds (`tny_grounds.c`) |
| 32+ | free (32 the story piece's; 33-34 agency's, unused) | |
| 35 | Chamber (op, a, b) [-> value]: the Time Chamber's rooms, the gate's six, the drops (`chamber.c`, `data/story/chamber.lscript`) | Time Chamber |
| 36 | Popo (op, a, b) [-> value]: the Time Chamber's hidden room, Mr. Popo's scenes and fight, the loop, a room's prize (`popo.c`, `data/story/chamber_popo.lscript`, `ch_lever`) | Time Chamber |

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
| 8 | SPAWN_RECORD | spawn the enemy record at address `arg` with a boss bar; `arg` may be in EWRAM (since 2026-10-25: a test pokes a record into `coop2.boss_rec` 0x0203FD6C itself and passes that address, `tny_artbuu.txt`) |
| 9 | PARTNER_WIN | only the boss twins fall (Broly's rage test) |
| 10 | DOWN | player `arg` (0 / 1) goes down as from a lethal hit (`revive.c`) |
| 11 | A16_PARTY | Krillin player 1, Android 16 player 2 |
| 12 | free | |
| 13 | WAVE_CLEAR | every standing enemy dies |
| 14 | WAVE_SKIP | the next announced round is `arg` |
| 15 | SOLO | solo mode `arg` (Cell Games) |
| 16 | WIN_STAT | the fight of stat `arg` wins itself once it stands and player 1 is free (tests write `cg_state + 5` directly: the command only runs between scenes) |
| 17+ | free | |
| 48 | CHAMBER (`chamber.c` `chamber_test`, the Time Chamber) | the ops: the table "The Time Chamber" at the top |
| 49 | POPO (`popo.c` `popo_test`, the Time Chamber's Mr. Popo) | the ops: the table "The Time Chamber" at the top |
| 15-16 | allocated to the Cell Games chapter | |
| 17 | S13 | spawn Super Android 13 72 px right of player 1, with a boss bar |
| 18 | S13_DRIVE | `arg` low byte = forced special (1 volley, 2 fan, 3 bomber, 4 wave, 5 nova, 6 roar; 0 = his choice), \| 0x100 = his HP := `arg2`, \| 0x200 = toggle the autopilot (`A16_TEST`) |
| 19+ | 19-20 allocated to the post-game chapter | |
| 45 | OBJ_ROOM_HOLD (perf, `objroom.c`) | `arg` bytes of OBJ VRAM held at the largest free run's low end (a map fuller than it is), 0 gives them back; answers `cmd_arg` := the largest free block. `perf_budget.py` forces an eviction with it |
| 46 | HUM_COUNT (`ssj.c`, `tools/headless/p2_cases.txt`) | answers `cmd_arg` := how many Super Saiyan aura hums are playing (one while either player is transformed, else none) |
| 47 | HERCULE_KIT (`enemies.c`, `tools/headless/hercule_kit.txt`) | `arg` 0: `cmd_arg` := the enemies the flash holds, 1: the last hit's damage, 2: 60 EXP for slot `arg2` |
| 21 | TNY_SEED (engine) | `arg` = the next tournament's draw seed (`tny_engine.c`); `arg2` = 0x7E000000 \| fighter: he is the players' first opponent (the seat across from player 1, outside the pool's rules) or the Rumble's headliner; any other `arg2`: none |
| 22 | TNY_FORCE (engine) | `arg` 0: every opponent of the running fight takes a lethal blow from player 1 (a won fight); 1: the fight is lost (player at 1 HP); 2: the running duel ends, `arg2` = the winner (1 / 2) |
| 23 | TNY_RECORD (engine) | `arg` 0: wipe the global record; 1 / 2: set / clear fighter `arg2`'s unlock bit; 3: reread it into `eng.rec` (0x0203ED68) |
| 24 | TNY_FORCE_POOL (engine) | `arg` = tiers forced open (bits 0-4); `arg2` bit 0 ignore the fighters' story flags, bit 1 a fighter without art fights with Tien's body (121); no story notices while anything is forced |
| 25 | TNY_START (engine) | a tournament without the director's screens, from where player 1 stands (the way back): `arg` = division \| full << 4 (0 / 1, 2: 32 fighters, a 2 v 2's alone) \| tier << 8, `arg2` = player 1's pick \| player 2's << 8 (0xFF = his own character) |
| 28-29 | the tournament ui (below / `docs/tournament.md` 12) | |
| 32 | TNY_COMBO_METER (team combos: the meter, `tny_combo_meter.c`, `docs/tournament.md` 25.1) | `arg` = op, `arg2` = value: 0 as at a fight's start (players' bar full, AI bars cleared), 1 the players' bar full, 2 empty with the cooldown over, 3 end a running cooldown, 4 cooldown of `arg2` frames, 5 AI slot `arg2` full (0xFF all), 6 the button for player `arg2` (0 / 1) next frame, 7 a caption (`arg2` 0-2 test lines, 3 `tny_combo_name` of the players' combo), 8 the players' fill := `arg2` (filling; the drain's delay starts now), 9 `arg2` bit 0: AI pairs never fire by themselves, 10 player `arg2 & 1` throws his A punch at living kit fighter `arg2 >> 8 & 0xFF` (list order), standing `arg2 >> 16 & 0xFF` px (0 = 20) on his left (`arg2` bit 24: right), 11 kit fighter `arg2 & 0xFF` stands `arg2 >> 8 & 0xFFFF` px right of player `arg2 >> 24`. State `.combo.meter` 0x0203EA00 (magic 0x524D4354 "TCMR"): +4 bytes on, pstate (0 ready, 1 cooldown, 2 filling), pcombo (0xFF none), hold; +8 fill (s16, 0x3C0 full); +0xA cooldown frames; +0xC tries / starts; +0x10 players' hits on kits / shared; +0x14 splits / kit hits on players; +0x18 each player's last hit frame; +0x1C their last targets; +0x24 the shared target; +0x28 last shared frame / fights seen; +0x2C press, drawn (1 = the bars are up), AI slots, living kits; +0x30 the bars' tile / presses with no opponent; +0x34 the last `tny_combo_start` call: lead, partner, target; +0x40 its combo, side, result; +0x44 three AI slots of 0x10 (ida, idb, combo, state 0 free 1 filling 2 full 3 cooldown; fill; cool / retry; tries; starts; hits); +0x74 frames drained; +0x76 Select held. Caption `.combo.cap` 0x0203EA80 ("TCAP"): +4 queue (3 pointers), +0x10 queued, sprites, frames left (u16 +0x12); +0x14 tile / shown; +0x18 fails / drops; +0x1C frames drawn / width; +0x20 frames waited behind a scene |
| 33 | TNY_COMBO_RUN (team combos: the runner, `tny_combo_run.c`, `docs/tournament.md` 25.2) | `arg` = op << 24 \| ...: op 0 `combo \| a << 8 \| b << 16`, `arg2` = `target \| side << 8`: start combo `combo` (0xFF: the pair's own, `tny_combo_of`) by a and b on the target; who: 0 player 1, 1 player 2, 2 + k the k-th kit fighter in the entity list (ids: a player's pick on the floor, else his slot's story fighter; a kit's fighter); it plays from the next frame tick (whole, inside the command modal). op 1: clear the counters. State `.combo.run` 0x0203EAB0 (magic 0xC0B0A001, `RunRam`): +4 busy (1 starting, 2 playing), combo, side, reason (1 played out, 2 target down, 3 cut short); +8 the frame of the last played frame; +0xC runs, frames of the last; +0x10 the target's HP before / after (a player: his entry's); +0x18 damage by fighter 0 / 1; +0x20 fighter 0, 1, the target: x, y after (s16 px); +0x2C hits, bodies held, objects frozen, bystanders moved; +0x30 the pair's ids, the last refusal (1 busy, 2 no table, 3 a body missing / down, 4 a scene or command batch, 5 no heap, 6 gone before the modal, 7 test: no combo for the pair), starts; +0x34 the three entities; +0x40 the working block (heap); +0x44 the hold state and its vtable; +0x78 start positions; +0x84 shots, knock-down frames; +0x88 the copies as last drawn (u8 x3: 0 none, 1 his tiles, 2 its own VRAM, 3 his frame mirrored), +0x8B their facings (2 left, 3 right), +0x8E 1 = left of the target, +0x91 copies made with their own VRAM (`docs/tournament.md` 25.5) |
| 40 | TNY_AUTO (tny, `tny_engine.c`, `docs/testing.md` "Tournament in co-op") | every fight of a tournament decides itself after `arg` frames of it (a player's hit-stop pauses the count, since 2026-11-05; it used to reset it, and a fight whose opponents kept landing hits was never decided), through test command 22's ways; `arg` 0 off; `arg2` bits 0-23 the fights to lose (bit n: the n-th from now), bits 24-25 the co-op final's winner (0 / 1 player 1, 2 player 2) |
| 37 | TNY_CAPTION (tny, `tny_caption.c`, `docs/tournament.md` 24.1) | the Rumble announcer's calls: `arg` 0 answers the lines shown so far into the `cmd_arg` word when at least `arg2` (else 0), 1 queues line `arg2 & 0xFF` (0-3 the fall calls, 4 the flurry, 5 a player down, 6 / 7 Super Buu's first / next form) for fighter `arg2 >> 8` during a Rumble round, 2 clears the queue and the count. State `.tny.cap` 0x0203EFAC (magic 0x504E5954 "TYNP"): +4 the queue's lines, +7 their fighters, +0xA queued, +0xB shown, +0xC the head's frames left (no tiles: frames to wait after a combo), +0xD width / 2 \| (rows - 1) << 7, +0xE OBJ tiles |
| 38 | TNY_CHECKS (tny, `tny_engine.c`, `docs/tournament.md` 24.4 / 24.5 / 24.7) | `arg` 0: a check, `arg2` = kind << 16 \| n, the answer replaces the `cmd_arg` word (0 = no; post `arg` 0 so a command not taken also reads 0): kind 0 the last ring reload took 1..n frames (answer: its frames, `eng` +0x157), 1 the announcer walked off and on at least once each (answer: offs \| ons << 8, `eng` +0x160 / +0x161), 2 player 1 stands n px (+-1) right of the mark (answer: the px). `arg` 2: mark player 1's x (`eng` +0x162); 4: clear the count of the announcer's walks started (`eng` +0x164); 5: wipe the scenes' staging counts (`.combo.st`, magic 0x47415453 "STAG": +4 lines staged, +5 the announcer's lines, +6 the champions' highest hop in px, +7 boxes moved to the bottom, +8 winners' celebrations started, +9 their highest hop in px, +0xA the bodies posed for them; `docs/tournament.md` 27, 28, `tny_scenes.txt`); 6: the win pose of sprite `arg2` into the `cmd_arg` word (0 = not in the build, else 1 << 24 \| frames in its group << 16 \| the frame << 8 (0: a held pose shows its group's first frame) | the group, 0x3F a built one; bit 31 = a frame not drawable)\| the group, 0x3F the drawn one; bit 31 = a frame not drawable); 7: every actor (test command 3) of sprite `arg2` strikes its win pose and the celebration starts over; 8: those poses end and the posed actors leave (2026-11-05, `docs/tournament.md` 28) |
| 31 | TNY_TALK (dialogue, main) | the fighters' dialogue (`tny_talk.c`): `arg` = the opening side (fighter \| fighter << 8, 0xFF nobody), `arg2` = the answering side; `arg` bit 31: fighter `arg & 0xFF`'s win line instead (`tools/headless/tny_talk.txt`) |
| 30 | TNY_GROUNDS | `arg` 0 the waiting room's real pool, 1 force the pool to the Rookie opponents (stand-in for the engine's `tny_in_pool`), 2 spawn today's fighters now (`tny_grounds.c`) |
| 19+ | 19-20 allocated to the post-game chapter; 21-30 the tournament team (above) | |
| 44 | REGEN (`regen.c`) | `arg` = frames: both players' ki timers at it (0: as if each had just used a ki move; 1800: rising now) |
| 26 | TNY_SPAWN (moves) | `arg` = fighter (TF_*) \| spot << 8 (0 = 64 px right of player 1, 1-7 round him) \| flags << 16 (1 boss bar, 2 the last spawn's side, 4 no specials for 600 frames); `arg2` = stat (0 or >= 1000: the engine's row 160 + id, else a stand-in): spawn him with his kit (`tny_moves.c`) |
| 27 | TNY_DRIVE (moves) | `arg` = op << 24: 0 force move (`reg << 8 \| move`: 1-6 his kit's slot, 0x80 \| kind a library kind, 0xFB-0xFF Multi-Form / self-KO / stun beam / stun projectile / big ball), 1 Rumble = `arg & 1`, 2 HP of kit `reg` := `arg2` (0x800000NN = NN %), 3 no specials for `arg2` frames, 4 every kit fighter leaves, 5 a passive on top (11 drain, 12 regen), 6 player 1 throws one punch at kit `reg` (`reg << 8 \| gap << 16 \| step`: step 1 = first stand `gap` px (0 = 20) on his left; the A button's own routine 0x08007DE8) |

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
| 12 | `papaya_street.json` | the street with the stalls (Buu 7/2): Roshi, a vendor behind each stall, the reporter, his cameraman and the ZZTV van (`tools/vanart.py`), fans |
| 13 | `papaya_courtyard.json` | the courtyard (Buu 7/3): the unlock board (CoopCall 28, 1), the unlocked fighters (CoopCall 30, 4), the crack into the hidden arena |
| 14 | `papaya_infirmary.json` | the infirmary corridor (Buu 7/4) between the courtyard and the waiting room: the nurse, the injured man (`tools/patientart.py`), Yamcha in the last bed |
| 15 | `papaya_waiting.json` | the fighters' waiting room (Buu 7/5), the hub: save pad, the director (CoopCall 23), today's fighters (CoopCall 30, 0) |
| 16 | `papaya_ring.json` | the ring with the stands (Buu 7/6, 720 x 720): the engine's stage (`src/coop/tny_ring.h`) |
| 17 | `papaya_secret.json` | the hidden arena (Buu's Other World Stadium, 1/20 there): Pikkon, the Senzu (flag 442); an easter egg |
| 18 | `chamber_open.json` | the Time Chamber's open area (2048 x 2048): the gate in its wall with the portal, the jars, the red carpet, the door home (no save pad since `nb-chamber2`), the spot records |
| 19 | `chamber_dojo.json` | the Time Chamber: Roshi's dojo (zone 6 area 70's gym with a plaque) |
| 20 | `chamber_pirate.json` (`chamber_pirate_plain.json` without `roms/buu.gba`) | the Time Chamber: the pirate airship (Buu's Fury's deck 12/1), Tao and his crew |
| 21 | `chamber_frieza.json` (`chamber_frieza_plain.json`) | the Time Chamber: Frieza's ship, his throne room (Buu's Fury's 8/27 since `docs/time-chamber.md` 13.6; the crystal room until then), Frieza and King Cold |
| 22 | `chamber_namek.json` | the Time Chamber: the Namek ship (Capsule Ship 3, 6/58, in green), Buu-saga Piccolo and the Namekian |
| 23 | `chamber_lab.json` | the Time Chamber: the lab (Dr. Gero's computer room 8/63), 18 and 17 |
| 24 | `chamber_saiyan.json` (`chamber_saiyan_plain.json`) | the Time Chamber: the Saiyan ship (the Broly chapter's round room), Broly and Paragus |
| 25 | `chamber_popo.json` | the Time Chamber's hidden room behind the gate: Mr. Popo (`docs/time-chamber.md` 12.7); since 13 the portal between its pillars, a violet tint |
| 26 | kept | the Time Chamber: 26 the quarters |
| 27 | `chamber_saiyan_bed.json` | the Time Chamber: the Saiyan ship's bedroom (Buu's 6/8; needs `roms/buu.gba`; `docs/time-chamber.md` 13) |
| 28 | `chamber_saiyan_store.json` | the Time Chamber: the Saiyan ship's store room (Buu's 6/7, the cold box; needs `roms/buu.gba`) |
| 29 | `chamber_frieza_crystal.json` | the Time Chamber: Frieza's crystal room (Buu's 8/19), the second room of his ship (13.6; needs `roms/buu.gba`) |
| 30 | `chamber_frieza_control.json` | the Time Chamber: Frieza's control room (Buu's 8/34 with 8/20's console; needs `roms/buu.gba`) |
| 31 | `chamber_saiyan_lounge.json` | the Time Chamber: the Saiyan ship's lounge, north of its round room (needs `roms/buu.gba`) |

## World-map landmarks (decorations of zone 6 area 99, `docs/maps.md` 9e)

The id a world-map on-load passes to `FindCharacterEntity N` (show) / `FindEntityByType N` (hide) is the
decoration's index in the world map's list. Stock 0-12 (one per stock site); ours are appended by
`tools/mapedit.py` `"add_decorations"` in `data/hercule.json`'s spec order. Frames: stock 0-8 on the sheet, ours
appended after them (`tools/wmlandmark.py` `ART`).

| decoration | site | frame | spec |
|---|---|---|---|
| 13 | the arctic (post-game, flag 381) | 4 (stock snowy mountain) | `data/story/arctic_route.json` |
| 14 | Papaya Island (the grounds built) | 9 (`wmlandmark.py papaya`) | `data/story/papaya_route.json` |
| 15+ | free | | |

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
| 359 | Hidden Arena | the hidden arena (zone 15 area 17, an easter egg) |
| 360-379 | grounds, free | |
| 380-399 | the tournament ui | |

## Memory (EWRAM; `src/coop/coop.ld`, exact layout in `$(BUILD)/coop.map`)

| address | size | what |
|---|---|---|
| 0x02000000-0x0203F000 | | the game's heap (capped by `patch_rom.py`) |
| 0x0203F000 | 0xB00 region | `partyram` (9 slots): `g_party_block` (party, flags at +0x108, 0x1B8), `party_save` store (0x564), `switch` (0x1B8) |
| 0x0203F8D4 | 0xC | `.party.kr` Android 16 player state (`a16player.c`) |
| 0x0203F8E0 | 0x34 | `.party.kr` Krillin state (`krillin.c`) |
| 0x0203F914 | 0x9C | `.party.wv` `waves` (the old Time Chamber rounds, tests only since 2026-11-05; 0x0203F880 with 8 slots); moves when anything before it grows |
| 0x0203EB88 | 0x30 | `.combo.ch` the Time Chamber (`chamber.c` `ChState`, magic "CHAM"; after `.combo.ag`: find it by the magic) |
| 0x0203EBB4 | 0x28 of 0x30 | `.combo.po` the Time Chamber's Mr. Popo (`popo.c` `PopoState`, magic "POPO"; after `.combo.ch`: find it by the magic) |
| 0x0203EBDC | 0xC | `.combo.st` the ring's staging counts for the tests (`tny_engine.c`, `docs/tournament.md` 27, 28 (12 bytes since the celebrations); after `.combo.po`) |
| 0x0203F9B0 | 0x1C | `.party.cg` `cg_state` (Cell Games: magic, solo mode, test bytes, held beams; the post-game's yes / no question) |
| 0x0203F9CC | 0x2C | `.party.s13` `s13` (Super Android 13) |
| 0x0203F9F8 | 0x34 | `.party.rv` `rv` (revive: bodies, countdowns, progress, bars' VRAM, glow table, counters; `revive.h`) |
| 0x0203FA2C | 0x18 | `.party.ui` the Cell Games' one-player title card (`cellgames.c` `card_buf`) |
| 0x0203FA54 | 0x8 | `.party.cp` companion state (`companion.c`) |
| 0x0203FA44 | 0x14 | `.party.ui` `players` (the 1 / 2 players choice: magic, mode, asking, the question's action) |
| 0x0203FAE0 | 0x10 | `.party.ssj` `ssj_state` (`ssj.c`, magic 0x53534A48 "HJSS": +4 player 2's last slot, the last transformed slot; +6 transforms; +8 reverts; +0xA hand-offs; +0xC EP drained, +0xE of it player 2's); after `.party.xp` 0x0203FA5C (0x44) and `.party.bv` 0x0203FAA0 (0x40) |
| 0x0203FAF0 | 0x10 | `.party.pp` the test win's record (`cellgames.c` `cg_note_win`: magic "JRWN", enemies standing, stat, two positions; tests `find` it) |
| 0x0203FAF0-0x0203FB00 | 0x10 | free in `partyram` (each `.party.<name>` section added before `.party.wv` moves `waves`) |
| 0x0203F000 | 0xB00 region | `partyram`: `g_party_block` (party, flags at +0xEC, 0x19C), `party_save` store (0x50C), `switch` (0x19C) |
| 0x0203F844 | 0xC | `.party.kr` Android 16 player state (`a16player.c`) |
| 0x0203F850 | 0x30 | `.party.kr` Krillin state (`krillin.c`) |
| 0x0203F880 | 0x9C | `.party.wv` `waves` (Time Chamber); moves when `.party.kr` grows |
| 0x0203F91C | 0x2C | `.party.s13` `s13` (Super Android 13: live boss, debug counters, force, state, tuning; `super13.h` `S13Global`); moves when the sections before it grow |
| 0x0203F948-0x0203FB00 | ~0x1B8 | free in `partyram` |
| 0x0203EA00 | 0x200 region | `comboram` (team combos, `tny_combo.h`, NOLOAD): `.combo.meter` 0x0203EA00 (0x80, the meter), `.combo.cap` 0x0203EA80 (0x30, the caption), `.combo.run` 0x0203EAB0 (0x88 of 0x140, the runner: test command 33); then `.combo.rg`, `.combo.pf`, `.combo.ag`, `.combo.ch`, `.combo.st` (0xC, the tournament scenes' staging counts for tests, `tny_engine.c`, 2026-11-05: 0x1C0 of 0x200 used); the heap cap ends here since 2026-10-31; offsets at test command 32 |
| 0x0203EC00 | 0x228 of 0x240 | `.tny.eng` the tournament engine (`tny_engine.c` `TnyEng`, NOLOAD, magic 0x7E6E0003): phase +4, kind +6, result +7, picks +12, the fight's frames +0x16, opponents +0x2C, challengers +0x57 (count +0x59), bracket +0x6C (72 bytes: seats +0x70, 32 of them; winners +0x90; p1_seat / p2_seat +0xB0; `docs/tournament.md` 26), pop-ups queued / shown +0xEC (the queue: `.tny.eq`), the tests' wait bytes +0xFC / +0xFD, the winners' celebration +0x115 (its frame, 0xFF none) / +0x116 (started for this fight's end; `docs/tournament.md` 28, in the old padding), the held poses +0x11C, the announcer +0x14C, presentation stage +0x150, final form +0x153, the curtain +0x154, the announcer's walk +0x155 (1 on, 2 off; frames +0x156, target +0x158 / +0x15A), the last ring reload's frames +0x157, the last warp's spot +0x15C / +0x15E, walks off / on done +0x160 / +0x161, test mark x +0x162, walks started +0x164 (2026-11-01, `docs/tournament.md` 24.4 / 24.5), the record as last read +0x168 (`extra` +0x38: bit 0 Super Buu's final form beaten), the line +0x1B4 (speaker +0x1B7, text +0x1B8); then `.tny.ui`, `.tny.mv`, `.tny.gr`, `.tny.hp`, `.tny.talk`, `.tny.cap`, `.tny.cp`, `.tny.eq` (0x10, the pop-up queue), `.tny.us` (0x10, the bracket's first seats as last drawn) (`coop.map`): 0x3F8 of `tourneyram`'s 0x400 used, **8 bytes left** (`comboram` has 0x80) |
| 0x0203FB00 | 0x29C used, 0x2C0 max | `coop2` (`CoopExtra`, `coop.h`): about 0x24 bytes left, avoid adding fields |
| 0x0203FDC0 | 0x40 | `StoryTest` preset (headless `preset`, `STORY_TEST`) |
| 0x0203FE00 | 0xC0 | player 2's vtable copy |
| 0x0203FEC0 | | wide-view descriptor (`wide.h`) |
| 0x0203FF00 | 2 + 2 | `g_p2_remote`, `g_p1_remote` pad words |
| 0x0203FF04 | 0xE4 | `coop` (`CoopState`): `p2` +4, `p2_slot` +0x80, `exp_awards` +0xD8 |

All of it, 0x0203EA00-0x02040000 (`coop.ld` `__coop_ram_start` / `__coop_ram_end`), is zeroed before a soft reset
(`input.c` `soft_reset`, `docs/coop-design.md` "Session logic"), as a power cycle leaves it.

Save chip (8 KiB EEPROM, `party_save.c`): the header 0x0-0xF, three save slots of `SAVE_STRIDE` (0x10-0x54F with 9
party slots), **the tournament's global record 0x1E00-0x1E3F** (`tny_engine.c` `TRecord`), the rest free.

Useful `coop2` words: `cmd` 0x0203FD08, `cmd_arg` 0x0203FD0C, `a16` 0x0203FD34, `a16_bot` 0x0203FD44,
`cmd_arg2` 0x0203FD54, `duel_active` / `duel_winner` 0x0203FD5C / 0x0203FD5D.

IWRAM and ROM: entity list 0x03001120, world/camera object 0x03001090, input 0x0300229C, player
object pointer 0x03001FA8 (position at +0x158 / +0x15C, 24.8), the trigger / touch-box list 0x03001FE0 and the player object 0x030020BC (`docs/engine.md` "Dropped items and touch boxes"), stock party 0x03000E90 (unused once
relocated); blob at 0x08800000, sprite index (relocated, stock at 0x083B4E74), ability table literal
0x08008024, CoopCall repointed at file 0x3B5E14, story-flag preset in ROM at 0xC00000. More in
`docs/engine.md`, `docs/rom-map.md` and `src/coop/rom.h`.
