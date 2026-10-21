# Project index (wiki home)

Start with `CLAUDE.md` (rules, build, test, a task-routing table). This page maps every document,
source file and tool, and says where to look for a given question. Keep it current. Numbers already
taken (flags, stats, sprites, rows, portraits, abilities, CoopCall, test commands, maps, RAM) are in
`docs/ids.md`; step-by-step recipes for common jobs are in `docs/howto.md`.

## Documents

| doc | what it holds | read when |
|---|---|---|
| `PLAYING.md` | the players' guide: build, connecting player 2, controls, difficulty, co-op rules, the story in short | before playing |
| `CLAUDE.md` | rules, current phase, build and test commands, key addresses, where to start per task | every session |
| `docs/ids.md` | registry: every flag, stat, sprite, row, portrait, ability, CoopCall, test command, zone-15 area and RAM region the mod uses, and what is free | before taking any new number |
| `docs/howto.md` | recipes: add a chapter, CoopCall, boss, sprite, portrait, playable character, map; test; run parallel sessions; find things fast | doing a common job |
| `docs/log.md` | dated progress log: what landed, what is open | resuming work |
| `docs/testing.md` | test loop: Windows mGBA, WSL fork, pad senders, GDB stub, savestates | before testing anything |
| `docs/engine.md` | game loop, input, IWRAM globals, player entity, behaviour classes, characters | touching engine code |
| `docs/rom-map.md` | ROM regions, IWRAM image, toolchain, seeded function list, open questions | looking for an address |
| `docs/coop-design.md` | co-op findings and status: movement, camera, EXP, switching, enemies, twins, netplay | co-op behaviour |
| `docs/widescreen.md` | dynamic wide view design, map streaming, the mGBA fork | wide view / fork |
| `docs/maps.md` | map format (MapEntry, variation, layers, tiles, palette, collision, triggers, exits, gates), the map tools, adding maps, the zone-15 maps, Buu's Fury maps | maps, warps, new areas |
| `docs/sprites.md` | sprite record format, atlas, Super Saiyan Hercule, Buu's Fury conversion, Android 16 (boss and player), Krillin, Android 13 / Namekian, Paragus / ship, Super Android 13, rules for new enemy models | sprites, new characters |
| `docs/hercule.md` | Hercule hero pass: data patch flow, growth, moveset, Keep Goku, dialogue tooling | stats / abilities / text |
| `docs/plan-legacy-of-hercule.md` | the game plan: pillars, milestones, order of work, rules | planning |
| `docs/story.md` | the current story plan: status, standing rules, chapter table, Ryan's decisions per chapter, what is open | story work |
| `docs/story-rom.md` | what the stock ROM does per chapter (maps, speakers, flags, quotes from the script dump) | grounding a story answer in ROM facts |
| `docs/story-archive.md` | superseded options, cost tables, investigation notes, removed content (dated) | why something was decided |
| `docs/scripts.md` | story scripts: bytecode, dialogue, triggers, records, condition words, flags, lscript, story specs, chapters implemented | editing a scene or a map's records |
| `docs/party-8-slots.md` | `PARTY_SLOTS`: the relocated N-entry party, every patched site, save layout, switch screen, verification, limits | before touching the party, saves or the switch screen |
| `docs/roms.md` | reference ROM hashes | checking a dump |
| `README.md` | user-facing: setup, playing two-player, netplay, wide view | onboarding a player |

## Source (`src/coop/`, Thumb C linked at 0x08800000, hooked in by `tools/patch_rom.py`)

| file | role |
|---|---|
| `rom.h` | addresses, layouts and `ROM_FN` prototypes of the original game (Entity, CharacterEntry, PartyState, Hud, AbilityDef, display rows) |
| `coop.h` | `CoopState` (pinned at 0x0203FF04), `CoopExtra` (0x0203FB00), player-2 vtable, flags |
| `input.c` | replacement `Input_Poll`: player 1 as before, player 2 from the selected source; calls the per-frame co-op checks; `SSJ_TEST` trigger |
| `player.c` | idle/walk/diagonal control states ported with per-entity input; player-2 spawn, bind, cycle, leash, `p2_refresh_sprite` |
| `entities.c` | `EntityList_Update` replacement: per-entity player pointer, `coop.acting` |
| `camera.c` | midpoint camera, wide clamp and cull rect |
| `wide.c/.h` | wide-view descriptor and dynamic size, `sprite_emit_hook` (OAM x/y high bits) |
| `hud.c` | player 2's HUD panel |
| `exp.c` | kill EXP to the player of the last hit (the damage calculation notes who hit each enemy; a projectile's owner counts), scaled to that slot's level |
| `enemies.c` | co-op difficulty: scaled enemies, twins, twin bosses, second boss bar; routes enemy stat 109 to `android16.c` and 145 to `super13.c` (and slows Super 13's chase modules) |
| `android16.c/.h` | the Android 16 boss (stat 109, sprite 160): rocket specials between ticks of the stock boss AI, poise, projectiles of the ROM's projectile class, `COOP_CMD_A16` test spawn; tuning knobs in the header |
| `super13.c/.h` | Super Android 13, the post-game boss (stat 145, sprite 180): ki volley / fan / deadly bomber / ki wave / ki nova / rage roar between ticks of the stock melee AI, partly homing orbs, combinations, poise, slow walk, the explosion on defeat; test commands 17 / 18, CoopCall 17; state `s13` in `.party.s13`; tuning knobs in the header (`docs/coop-design.md` "Super Android 13") |
| `bosses.c/.h` | extra enemy stats (110+ = stock stat + overrides: 110 / 112 Android 13 beside 19 / Gero, 111 Cooler as the forest spirit, 113 Cooler's Namekian, 114 unused, 115 / 116 Broly / Paragus, 120 / 121 the bunker's two Android 19s; full list `docs/ids.md`) and per-boss rules (end_pct, partner model/stat, no twin, pair, rage: `BOSS_RAGE`, Broly's once-only refill when Paragus falls first); the both-down and rage rules live in `enemies.c` |
| `actors.c` | duplicate-actor rule: town NPCs showing a character either player controls are removed |
| `duel.c` | player-vs-player duel: opponent hits, final blow to 1 HP, winner, pad swap, shadow clone in party slot 7 (`DUEL_SLOT`) |
| `vmcall.c` | `CoopCall` (game action 59), the script -> C bridge: duels, winner, Hercule's awakening, player 2, Broly's staging and PartyKeep (9 / 10), Android 16 joining and leaving (11 / 12), the training waves (13 / 14), the Cell Games' solo fights and held beams (15 / 16), Super 13 (17), the yes / no question (18), Super Saiyan 2 Gohan joining (20) (`docs/scripts.md` 2.2b) |
| `waves.c` | the Time Chamber's training waves: ten fixed rounds of stock enemies spawned around the players, round captions through the chamber script, "Training completed!"; state `waves` in `.party.wv` (0x0203F880 in the merged build; it moves with `.party.kr`, check `coop.map`); `COOP_CMD_WAVE_CLEAR` / `_SKIP` tests (`docs/story.md` 4g, `docs/testing.md`) |
| `companion.c` | the story's lead (noted by wrapping the SetActiveCharacter steps): speaker-1 portraits show him when someone else plays (portrait getter), and when nobody plays him our scene scripts stage him as an actor walking and flying in (CoopCall 22); old Hercule walks with Trunks in the prologue alone; companions are never players for the rules |
| `players.c` | the 1 / 2 players question at every game start (Select Game arms it; CoopCall 21; one player keeps player 2 out) |
| `revive.c` | reviving a downed partner: going down, the countdown, holding A beside the body, the bars, game over (`docs/coop-design.md` "Revive"); state `.party.rv`, test command 10 |
| `difficulty.c` | Easy / Normal / Hard: enemy HP, damage, speed, twins, boss HP, revive time, drop rolls (`difficulty.h`, flags 460 / 461) |
| `cellgames.c` | the Cell Games: solo fights (CoopCall 15, `COOP_CMD_SOLO`), held beams (CoopCall 16), the test win `COOP_CMD_WIN_STAT`; state `.party.cg` |
| `a16player.c/.h` | Android 16 as a player in "Gero's computer": CoopCall 11 / 12 (16 joins the last party slot and the party locks to Krillin + 16; he leaves for good), Rocket Punch (ability 22), `COOP_CMD_A16_PARTY`; ids and flags 420-437 in the header |
| `hercule.c` | Hercule's four abilities (table entries 14-17 via `data/hercule.json`) |
| `krillin.c/.h` | Krillin (slot 6, row 23, sprite 161) and old Hercule (row 24): status-screen title / pages / technique pages for PARTY_SLOTS, the pause menu's tab strip (`status_tabs_hook`), dialogue name opcodes, blue ki blast, Afterimage Rush / Shockwave Punch / Destructo Disc (abilities 19-21), old Hercule's weak combat bytes and `old_hercule_set`, `KR_TEST` triggers; ids in the header |
| `ssj2gohan.c/.h` | Super Saiyan 2 Gohan (party slot 8, row 28, sprite 58): `ssj2_join` (CoopCall 20, called by `data/story/cell_games.lscript`), Super Saiyan 2 Burst (ability 23), `SSJ2_TEST` triggers; ids and join numbers in the header |
| `party.c` | the relocated party block (`g_party_block`) and the ROM's party functions re-implemented for `PARTY_SLOTS` entries: `Party_Init`, `Party_AddExp` (level-ups), flag accessors, VM counter / item / bestiary opcodes |
| `party_save.c` | save path for N entries: `SaveGame`, `LoadSlot`, `WriteSlot`, the save menu's slot store and queries, start game; EEPROM slots of `SAVE_STRIDE` |
| `switch.c` | Switch Characters screen: view party of six, scrolling window, edge indicators |
| `ssj.c` | Super Saiyan Hercule ability pair (`ssj_ready` / `ssj_use`): row 22, sprite 157, EP cost 50 |
| `controller.c` | "controller mode": this GBA becomes player 2's pad over the link cable |
| `wrap.c` | run original ROM handlers for player 2 by swapping the input block (dash, the beam abilities' charge states) |
| `shipyard.c` | the parked Saiyan ship: `map_variation_hook` (`FUN_0800EB4C`) gives the Capsule Corp backyard (6/21) zone 15 area 10's variation (`data/maps/yard_ship.json`, the ship painted in) while the ship is parked (flag 400, not away: not 401 or 402) |
| `util.c` | freestanding memcpy/memset helpers |
| `coop.ld` | linker script: `.coop` sections, pinned EWRAM regions, `EXTERN` list of hook symbols |

Generated at build time (never committed): `build/maps.patch.json` (the new maps, from
`tools/mapbuild.py` and `data/maps/*.json`; previews in `$(BUILD)/maps/`), `build/gen/sprites_data.c` (SSJ Hercule record, relocated
`sprite_index` and `display_rows`, from `tools/spriteclone.py`), `build/gen/a16_sprites_data.c`
(Android 16's sprite 160 and the missile / fist / explosion frames, from `tools/a16sprites.py`), `build/gen/buu_sprites_data.c`
(sprite ids 158 and 159 converted from Buu's Fury by `tools/buu2log2.py`, when `roms/buu.gba` is present),
`build/gen/boss_models_data.c` (Android 13, the Namekian, from `tools/bossmodels.py`), `build/gen/broly_sprites_data.c`
(Paragus, his portrait, the ship, from `tools/brolysprites.py`), `build/gen/hercule_sprites_data.c` (`herc_record`:
Hercule's record 72 plus the Kamehameha pose, group 18, from `tools/herculesprites.py`), `build/gen/halo_sprites_data.c`
(haloed Goku and Super Saiyan Goku, sprites 176 / 177, from `tools/halosprites.py`), `build/gen/ssj2_sprites_data.c`
(`ssj2_record`, id 58 repointed, from `tools/ssj2sprites.py`), `build/gen/super13_sprites_data.c` (Super Android 13's
sprite 180, ki orbs, portrait 122, from `tools/super13sprites.py`), `build/gen/krillin_sprites_data.c` (Krillin's
sprite 161, status portrait, ki blast / disc frames and icons,
the relocated `ui_strings` and `portrait_module` tables, from `tools/krillinsprites.py`),
`build/hercule.patch.json` (from `tools/romdata.py build data/hercule.json`).

## Data (`data/`)

`maps/*.json` are the new maps' specs (zone 15: `arctic` and the arctic route `arctic_climb1`, `arctic_climb2`,
`arctic_ridge`, `arctic_summit`, `time_chamber`, `broly_arena`, `broly_crater` (needs
`roms/buu.gba`); format in
`docs/maps.md`), built by `tools/mapbuild.py` and placed by `patch_rom.py --maps` (`MAPS=1`).
`story/broly.*` is the Broly side chapter (it also feeds `maps/broly_arena.json`). `story/*.json` + `story/*.lscript` are the story chapters (map edits and scenes, `docs/scripts.md`), included by `hercule.json`. `hercule.json` is the Legacy of Hercule data spec (party defaults, growth, ability table entries,
Keep-Goku byte, included text records); `hercule-text.json` holds dialogue records written by
`tools/script_dump.py`. Applied by `tools/romdata.py build` then `patch_rom.py --data-patch`.

## Tools (`tools/`)

| tool | what it does |
|---|---|
| `patch_rom.py` | builds the ROM: pad to 16 MiB, place the blob, vtable slot hooks, entry jumps, word/byte patches, literal relocations, `--party-slots N` (count patches, shadow tables, N-entry default table), `--wide`, `--debug-menu`, `--data-patch`, `--maps` (new maps; relocates the MapEntry table last) |
| `romdata.py` | reads/writes the character tables (party, growth, abilities, EXP) and compiles `data/*.json`; `build --party-slots N` targets the relocated N-entry table |
| `script_dump.py` | dumps a map's dialogue sequences and bytecode scripts; writes text records |
| `script_asm.py` | lscript text form of scripts and dialogue: dump, assemble into free ROM, round trip over every stock script (`docs/scripts.md`) |
| `oldherculeportrait.py` | old Hercule's dialogue portrait (126) from Buu's Fury's Hercule portrait (module 0x086F393C id 76) -> `build/gen/oldherc_portrait_data.c` |
| `gen_journal.py` | generates `build/gen/journal_data.c`, the journal's quest table: the stock quests plus our chapters' entries (`docs/scripts.md` 2.6) |
| `gen_postgame.py` | generates `data/story/postgame.lscript` (the Lookout after the Cell Games: wishes, finale, the Buu's Fury question, the ending) from the stock dumps plus our edits |
| `gen_cell_games.py` | generates `data/story/cell_games.lscript` from the arena maps' stock dumps plus our edits (edit the chapter there) |
| `mapedit.py` | story specs (`data/story/*.json`): triggers, records, map scripts, line edits; run by `romdata.py` for specs in `data/hercule.json`'s include |
| `headless/` | `st_headless.c` (headless test runner on the fork's libmgba; `reset` keeps the save data; `stepz ADDR MAX` presses A on both pads until a byte clears; `entrec RECORD` counts the entities on a sprite record; `cart` / `cartout` / `ssave` load a .sav, write one, save a state without it; `regs` prints the CPU registers, `watch` / `breakpc` / `trace` find who writes a word, when a pc runs and what code runs, `docs/testing.md` "Finding a hang"), `st_test.py` (run a script, tile the screenshots), `dropins.py` (Ryan's F1-F6 drop-in savestates at the important scenes, `docs/testing.md` "Drop-in savestates"), `st_shots.py`, `mkmenu.txt` (debug-menu state per build), `bossfight.py` (a boss fight with the autopilot: Super 13, the twin Perfect Cells or any record; HP log, damage rates), `objfree.py` (OBJ VRAM free list from `arctic_route.txt`'s peeks). Scripts: `ch9_gero.txt` (a whole chapter end to end), `arctic_route.txt` (the arctic route: world-map landing, every exit, the pad), `arctic_summit.txt` (the summit scene, the Super 13 fight, the explosion, flag 382), `beam_hercule.txt` (the stock Cell beam struggle with Hercule firing), `halo_goku.txt` (haloed Goku: walk, punch, transform both ways), `ssj_tumble.txt` (Super Saiyan Hercule's tumble facing right: the frame-count fix), `super13.txt` (Super 13: every special, the rage, lag, the explosion), `p2_special.txt` (player 2's held A: the twin-Cell hang's regression test), `cell_games.txt` (the whole Cell Games), `cell_games_1p.txt` (the Games alone), `scene_actor.txt` (Gohan's actor in the Frieza stretch: four pairings, `entrec` counts it), `exp_credit.txt` (kill EXP to the player of the last hit), `a16_gone.txt` (16 leaves the party at the Cell Games), `broly_ride.txt` (the Broly chapter's ship ride both ways: Paragus out of the ship's door, boarding up its ramp, the others riding along and getting off first, out of its door on New Planet Vegeta and at home), `duel_end.txt` (a duel's end: the winner's line, both refilled), `prologue_companion.txt` (old Hercule with Trunks alone, on the play ROM), `postgame.txt` (the Lookout after it), `journal.txt` (the journal page), `revive.txt` (down, revive both ways, the countdown's game over), `dropout.txt` (player 2 leaves and comes back), `difficulty.txt` (the new game's question); Super Saiyan 2 Gohan (`docs/testing.md` "Super Saiyan 2 Gohan"): `ssj2_gohan.txt` (slot 8, row 28, his abilities), `ssj2_join.txt` (CoopCall 20 from a script, with the test-only `ssj2_join_test.json` / `ssj2_join_test.lscript`), `ssj2_duel.txt` (a clone duel goes to slot 7, never 8), `ssj2_save.txt` (the save pad and a reload), `ssj2_oldsave1.txt` / `ssj2_oldsave2.txt` (an eight-slot save loaded on the nine-slot build) |
| `fork/st_*.ps1/.py` | `st_run.ps1` (fork without GDB), `st_place.ps1` (move a fork window), `st_pad.py` (TCP pad as player 1, `STORY_TEST` builds), `st_preset.py` (story-flag preset in a ROM) |
| `sprites.py` | sprite atlas, per-sprite pose sheets, report (record format decoder); `Sprites.frame_counts` (frames per descriptor array, by descriptor runs) is what every generator copies frames through; `--frame-diff` lists the stock arrays the old gap guess got wrong |
| `spriteclone.py` | clones + recolours a record (SSJ Hercule), builds display rows 22-28 (`--row-sprite`, `--krillin-sprite`, `--old-hercule-sprite`, `--a16-sprite`, `--halo-sprites`, `--ssj2-sprite`) and emits the relocated `sprite_index` / `display_rows`; `--extra` appends more records to the index (an id below 157 replaces the stock record: `72=herc_record`) |
| `ssj2sprites.py` | Super Saiyan 2 Gohan: record 58 with hurt / lying / tumble (groups 11, 12, 20) from record 59, by ROM address -> `build/gen/ssj2_sprites_data.c` (`ssj2_record`, id 58 repointed; row 28), preview `ssj2_gohan_preview.png` |
| `halosprites.py` | haloed Goku: Goku's (60) and Super Saiyan Goku's (61) records with the dead pose's halo on every frame -> `build/gen/halo_sprites_data.c` (`halo_goku_record` 176, `halo_ssj_record` 177; rows 26 / 27), preview `halo_goku_preview.png` (stock beside haloed) |
| `herculesprites.py` | Hercule's Kamehameha pose: record 72 (stock groups by ROM address) with group 18 built from his punch stance / idle frames and Goku's beam arms in his colours -> `build/gen/hercule_sprites_data.c` (`herc_record`, id 72 repointed), preview `hercule_beam_preview.png` beside Goku's and Gohan's beam frames |
| `buu_sprites.py` | Buu's Fury ROM: sprite atlas, per-sprite sheets, report (its record format decoder) |
| `a16sprites.py` | Android 16 boss sprite (id 160: sprite 8's own groups + Android 17's fight frames recoloured and re-headed + a rocket-punch group), his player record (id 175, `a16p_record`, row 25) and the projectile frames -> `build/gen/a16_sprites_data.c` |
| `bossmodels.py` | Android 13 (id 165: 16's body + 17's fight frames, re-dressed per body part: cap, white hair, vest, jeans) and Cooler's Namekian servant (id 166: Piccolo's capeless set 104 recoloured) -> `build/gen/boss_models_data.c`, previews |
| `super13sprites.py` | Super Android 13 (id 180: Buu's Fury Broly recoloured per body part, his special poses, one wide hurt box; Perfect Cell's groups with `BUU=0`), his ki orbs and portrait 122 (16's made Super 13) -> `build/gen/super13_sprites_data.c`, preview `super13_preview.png` |
| `brolysprites.py` | the Broly chapter's art: Paragus (id 170: Vegeta's set 137 with grey hair and a goatee, per frame), his dialogue portrait (id 120, via `krillinsprites.py --extra-portrait`), -> `build/gen/broly_sprites_data.c`, preview (the Saiyan ship is map art since 2026-10-21: `shipart.py`) |
| `krillinsprites.py` | Krillin's playable sprite (id 161: sprite 76 + Goku's fight groups re-headed), status portrait (73 in the menu BG palette), blue ki blast, Destructo Disc frames, HUD and status icons, the UI string table with per-slot blocks, the portrait module table (status portraits 119 Krillin, 121 Android 16; `--extra-portrait ID=SYMBOL` for other generators' portraits, e.g. 120 Paragus; ids must be unique) -> `build/gen/krillin_sprites_data.c` |
| `buu2log2.py` | converts Buu's Fury records to LoG2 format (palette remap, LZ encoder, previews, colour report) -> `build/gen/buu_sprites_data.c` |
| `mapchunks.py` | Python port of the IWRAM decompressor, savestate reader, map chunk tools |
| `maplib.py` | map format library: MapEntry, variation, layers, tileset slots + banks, animations, collision bitmaps, triggers, exits, gates (also reads Buu's Fury variations) |
| `mapexport.py` | render any map from the ROM: per-layer PNGs, composite, walk / projectile overlays, summary; `--list`, `--all`, `--tileset`, `--table/--count` for a built ROM |
| `mapbuild.py` | build new maps from `data/maps/*.json`: stamps / fills from stock maps (or Buu's Fury maps, palette-converted; map-anchored fills), generated tiles, recolour into new banks (per-stamp opt-out), effect overlays, collision ops, triggers, exits, gates, records (enemies copied from stock records, with their item drop) -> `--maps` records + MapEntry rows |
| `shipart.py` | the Saiyan ship as map art (256 x 160, our own art drawn by code: hull, cockpit dome, crest, ring of lights, fins, legs, an open door with a ramp), left-right symmetric; `draw()` for a `png` op's `"draw": "shipart"`, `footprint()` for `art_footprint` (all of it blocks but the doorway and ramp); `shipart.py OUT.png --x4 PREVIEW.png` |
| `widerender.py` | offline reference render of the wide view from a savestate |
| `savestate.py` | dump memory regions/words from an mGBA savestate |
| `romcheck.py` | ROM header/hash check |
| `seed_functions.py`, `ghidra_import.ps1`, `ghidra/*.java` | headless Ghidra project, function seeding, `DecompileFunctions.java <outdir> <addr...|all>` (whole-ROM corpus in `build/decomp_all/` for grepping), `FindRefs.java <addr...>` |
| `mgba_run.ps1`, `mgba_input.ps1`, `mgba_shot.ps1`, `mgba_script.ps1`, `mgba_window.ps1` | drive the Windows mGBA (Qt) by window: restart on a ROM copy, keys, screenshots, Scripting console |
| `mgba_cmd.py` + `mgba/cmd_server.lua` | focus-free TCP command server inside Windows mGBA (`seq`, `state`, `shot`); `--port=N` / `cmd_server_port` for a second instance |
| `mgba/coop.lua`, `peek.lua`, `ents.lua`, `force.lua`, `enemies.lua`, `p2_input.lua`, `wide_test.lua` | Lua helpers for Windows mGBA (console inspectors, player-2 pad bridge) |
| `mgba/netplay.lua`, `netplay_relay.py` | frame-lockstep netplay (Windows mGBA) |
| `p2_keys.py` | player-2 keyboard sender (IJKL / O U / [ ] / N M) over TCP (7777 Lua bridge, 7778 fork) |
| `win_keys.ps1`, `win_shot.ps1`, `win_burst.ps1` | keys and screenshots for any window by title (used for the WSL fork window); `win_burst` takes N frames from one process |
| `mgba-wide/` | the mGBA fork: `apply.py` patches a 0.10 clone, `software-wide.c` wide renderer, `p2pad.c` TCP pad, `build-wsl.sh` |
| `gdb/*.gdb` | GDB batch scripts for mGBA's stub (boot check, crash catch, state dumps) |
| `fork/` | WSL fork test loop: `runfork.ps1` (launch + attach), `gdbsrv.sh` (persistent gdb client over a FIFO), `peek.sh`/`peek.gdb` (pause, dump party + EEPROM, continue), `keys.sh` (player-1 buttons through the pad word), `tele.sh`, `reset.sh`; Android 16 fight: `runa16.ps1`, `p2hold.py` (scripted player-2 pad), `a16fight.sh`, `a16peek.sh/.gdb`, `a16hits.gdb`, `a16install.sh` (`docs/testing.md`); Krillin tests: `padq.sh` (queue steps for `p2hold.py --cmd`), `kr_peek.sh`, `kr_ents.gdb`, `kr_tele2.sh`, `kr_setidx.sh`; `runfork.ps1 -Mute` (no audio sync, for several forks at once); boss pairs: `bootisland.sh` (boot -> island test map, level-30 pair), `bossfight.sh` (spawn any boss record + autopilot, HP log), `bosstest.sh`, `q.sh` (one tagged gdb query); slots >= 1 install into `/root/fork-sN` (`slot.sh` ports and names per slot, `install.sh`, `waitlog.sh`, `warp.sh`, `a16shots.sh`) |

## Where to look for...

| question | place |
|---|---|
| an engine address or struct offset | `src/coop/rom.h`, then `docs/engine.md`, `docs/rom-map.md` |
| the party block, saves, the switch screen, slot counts | `docs/party-8-slots.md`, `src/coop/party.c`, `party_save.c`, `switch.c` |
| how player 2 is spawned / bound / switched | `src/coop/player.c`, `docs/coop-design.md` "Character switching" |
| why a build crashed with "Jumped to invalid address" | `docs/testing.md` GDB recipe; savestate from an older build? (vtable copies) |
| sprite record layout, frame encoding, frames per array | `docs/sprites.md`, `tools/sprites.py` |
| Buu's Fury sprites, converting them, which ids exist | `docs/sprites.md` "Buu's Fury", `tools/buu_sprites.py`, `tools/buu2log2.py` |
| Hercule's Kamehameha pose, how NPCFireKamehameha plays a beam | `docs/sprites.md` "Hercule's beam pose", `tools/herculesprites.py`, `tools/headless/beam_hercule.txt` |
| haloed (dead) Goku, his rows 26 / 27, the transform / detransform states | `docs/sprites.md` "Haloed Goku", `tools/halosprites.py`, `tools/headless/halo_goku.txt` |
| display rows, transformations | `docs/sprites.md` SSJ section, `src/coop/ssj.c`, `tools/spriteclone.py` |
| Krillin, old Hercule, per-slot names / portraits / status pages | `docs/hercule.md` 5b / 5c, `src/coop/krillin.c`, `docs/party-8-slots.md` "Krillin", `docs/sprites.md` "Krillin" |
| ability table, stats, growth | `docs/hercule.md`, `tools/romdata.py`, `data/hercule.json` |
| dialogue text and scripts | `docs/scripts.md`, `tools/script_asm.py`, `tools/mapedit.py`, `data/story/` |
| testing a story chapter | `docs/testing.md` "Story chapters, headless", `tools/headless/` (`ch9_gero.txt`: a whole chapter end to end) |
| world-map markers (star / plain), Gero's lab re-entry | `docs/maps.md` 9b |
| the arctic route (post-game maps, world-map landing, summit spots for the scene) | `docs/maps.md` 9a, 9c; `data/maps/arctic*.json`, `data/story/arctic_route.*` |
| a new map's name (place names after the UI string blocks) | `docs/ids.md` "UI strings and place names", `tools/krillinsprites.py` `PLACE_NAMES` |
| enemies, twins, boss bars | `src/coop/enemies.c`, `docs/coop-design.md` "Co-op difficulty" |
| the Android 16 boss, hit messages, projectiles, enemy control states | `src/coop/android16.c`, `docs/coop-design.md` "Android 16 boss", `docs/sprites.md` "Android 16 boss sprite" |
| Super Android 13 (the post-game boss): placing him, his attacks, tuning, the explosion | `src/coop/super13.c/.h`, `docs/coop-design.md` "Super Android 13", `docs/sprites.md` "Super Android 13", `docs/testing.md` "Super Android 13" |
| wide view | `docs/widescreen.md`, `src/coop/wide.c`, `tools/mgba-wide/` |
| a map's layout, collision, exits, tiles; adding or re-dressing a map | `docs/maps.md`, `tools/mapexport.py`, `tools/mapbuild.py`, `data/maps/` |
| what Ryan decided / asked for | `docs/story.md` (current), `docs/log.md`, `docs/plan-legacy-of-hercule.md` section 4 "Rules we keep" |
| a free flag / stat / sprite / row / portrait / ability / CoopCall / test command / map area | `docs/ids.md` |
| how to add a chapter, boss, sprite, character, map | `docs/howto.md` |
| what the stock game does on a map | `build/scripts/map_NNN_zZ_aA.txt` (script dump), `docs/story-rom.md` |
| the Time Chamber rounds, Roshi's spar | `src/coop/waves.c`, `data/story/wait_chamber.*`, `wait_spar.*`, `docs/coop-design.md` |
| the Broly chapter | `data/story/broly.*`, `data/maps/broly_arena.json`, `tools/brolysprites.py`, `docs/story.md` 4g |
