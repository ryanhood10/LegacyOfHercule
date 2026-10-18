# Project index (wiki home)

Start with `CLAUDE.md` (rules, build, test in one page). This page maps every document, source
file and tool, and says where to look for a given question. Keep it current.

## Documents

| doc | what it holds | read when |
|---|---|---|
| `CLAUDE.md` | rules, build and test commands, key addresses | every session |
| `docs/log.md` | dated progress log: what landed, what is open | resuming work |
| `docs/testing.md` | test loop: Windows mGBA, WSL fork, pad senders, GDB stub, savestates | before testing anything |
| `docs/engine.md` | game loop, input, IWRAM globals, player entity, behaviour classes, characters | touching engine code |
| `docs/rom-map.md` | ROM regions, IWRAM image, toolchain, seeded function list, open questions | looking for an address |
| `docs/coop-design.md` | co-op findings and status: movement, camera, EXP, switching, enemies, twins, netplay | co-op behaviour |
| `docs/widescreen.md` | dynamic wide view design, map streaming, the mGBA fork | wide view / fork |
| `docs/maps.md` | map format (MapEntry, variation, layers, tiles, palette, collision, triggers, exits, gates), the map tools, adding maps, the zone-15 maps, Buu's Fury maps | maps, warps, new areas |
| `docs/sprites.md` | sprite record format, atlas, playable candidates, SSJ Hercule clone | sprites, new characters |
| `docs/hercule.md` | Hercule hero pass: data patch flow, growth, moveset, Keep Goku, dialogue tooling | stats / abilities / text |
| `docs/plan-legacy-of-hercule.md` | the game plan: pillars, milestones, order of work, rules | planning |
| `docs/story.md` | the ROM's story as scripted, Ryan's opening, beat costs, what is still open | story work |
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
| `exp.c` | EXP credited to the killing player |
| `enemies.c` | co-op difficulty: scaled enemies, twins, twin bosses, second boss bar; routes enemy stat 109 to `android16.c` |
| `android16.c/.h` | the Android 16 boss (stat 109, sprite 160): rocket specials between ticks of the stock boss AI, poise, projectiles of the ROM's projectile class, `COOP_CMD_A16` test spawn; tuning knobs in the header |
| `bosses.c/.h` | extra enemy stats (110+ = stock stat + overrides) and per-boss rules (end_pct, partner model/stat, no twin); the both-down rule lives in `enemies.c` |
| `actors.c` | duplicate-actor rule: town NPCs showing a character either player controls are removed |
| `duel.c` | player-vs-player duel: opponent hits, final blow to 1 HP, winner, pad swap, shadow clone in the last party slot |
| `hercule.c` | Hercule's four abilities (table entries 14-17 via `data/hercule.json`) |
| `party.c` | the relocated party block (`g_party_block`) and the ROM's party functions re-implemented for `PARTY_SLOTS` entries: `Party_Init`, flag accessors, VM counter / item / bestiary opcodes |
| `party_save.c` | save path for N entries: `SaveGame`, `LoadSlot`, `WriteSlot`, the save menu's slot store and queries, start game; EEPROM slots of `SAVE_STRIDE` |
| `switch.c` | Switch Characters screen: view party of six, scrolling window, edge indicators |
| `ssj.c` | Super Saiyan Hercule ability pair (`ssj_ready` / `ssj_use`): row 22, sprite 157, EP cost 50 |
| `controller.c` | "controller mode": this GBA becomes player 2's pad over the link cable |
| `wrap.c` | run original ROM handlers for player 2 by swapping the input block |
| `util.c` | freestanding memcpy/memset helpers |
| `coop.ld` | linker script: `.coop` sections, pinned EWRAM regions, `EXTERN` list of hook symbols |

Generated at build time (never committed): `build/maps.patch.json` (the new maps, from
`tools/mapbuild.py` and `data/maps/*.json`; previews in `$(BUILD)/maps/`), `build/gen/sprites_data.c` (SSJ Hercule record, relocated
`sprite_index` and `display_rows`, from `tools/spriteclone.py`), `build/gen/a16_sprites_data.c`
(Android 16's sprite 160 and the missile / fist / explosion frames, from `tools/a16sprites.py`), `build/gen/buu_sprites_data.c`
(sprite ids 158+ converted from Buu's Fury by `tools/buu2log2.py`, when `roms/buu.gba` is present),
`build/hercule.patch.json` (from `tools/romdata.py build data/hercule.json`).

## Data (`data/`)

`maps/*.json` are the new maps' specs (zone 15: `arctic`, `time_chamber`, `broly_arena`; format in
`docs/maps.md`), built by `tools/mapbuild.py` and placed by `patch_rom.py --maps` (`MAPS=1`).
`hercule.json` is the Legacy of Hercule data spec (party defaults, growth, ability table entries,
Keep-Goku byte, included text records); `hercule-text.json` holds dialogue records written by
`tools/script_dump.py`. Applied by `tools/romdata.py build` then `patch_rom.py --data-patch`.

## Tools (`tools/`)

| tool | what it does |
|---|---|
| `patch_rom.py` | builds the ROM: pad to 16 MiB, place the blob, vtable slot hooks, entry jumps, word/byte patches, literal relocations, `--party-slots N` (count patches, shadow tables, N-entry default table), `--wide`, `--debug-menu`, `--data-patch`, `--maps` (new maps; relocates the MapEntry table last) |
| `romdata.py` | reads/writes the character tables (party, growth, abilities, EXP) and compiles `data/*.json`; `build --party-slots N` targets the relocated N-entry table |
| `script_dump.py` | dumps a map's dialogue sequences and bytecode scripts; writes text records |
| `sprites.py` | sprite atlas, per-sprite pose sheets, report (record format decoder) |
| `spriteclone.py` | clones + recolours a record (SSJ Hercule) and emits the C data file; `--extra` appends more records to the index |
| `buu_sprites.py` | Buu's Fury ROM: sprite atlas, per-sprite sheets, report (its record format decoder) |
| `a16sprites.py` | Android 16 boss sprite (id 160: sprite 8's own groups + Android 17's fight frames recoloured and re-headed + a rocket-punch group) and the projectile frames -> `build/gen/a16_sprites_data.c` |
| `buu2log2.py` | converts Buu's Fury records to LoG2 format (palette remap, LZ encoder, previews, colour report) -> `build/gen/buu_sprites_data.c` |
| `mapchunks.py` | Python port of the IWRAM decompressor, savestate reader, map chunk tools |
| `maplib.py` | map format library: MapEntry, variation, layers, tileset slots + banks, animations, collision bitmaps, triggers, exits, gates (also reads Buu's Fury variations) |
| `mapexport.py` | render any map from the ROM: per-layer PNGs, composite, walk / projectile overlays, summary; `--list`, `--all`, `--tileset`, `--table/--count` for a built ROM |
| `mapbuild.py` | build new maps from `data/maps/*.json`: stamps / fills from stock maps, generated tiles, recolour into new banks, effect overlays, collision ops, triggers, exits, gates -> `--maps` records + MapEntry rows |
| `widerender.py` | offline reference render of the wide view from a savestate |
| `savestate.py` | dump memory regions/words from an mGBA savestate |
| `romcheck.py` | ROM header/hash check |
| `seed_functions.py`, `ghidra_import.ps1`, `ghidra/*.java` | headless Ghidra project, function seeding, `DecompileFunctions.java <outdir> <addr...|all>` (whole-ROM corpus in `build/decomp_all/` for grepping), `FindRefs.java <addr...>` |
| `mgba_run.ps1`, `mgba_input.ps1`, `mgba_shot.ps1`, `mgba_script.ps1`, `mgba_window.ps1` | drive the Windows mGBA (Qt) by window: restart on a ROM copy, keys, screenshots, Scripting console |
| `mgba_cmd.py` + `mgba/cmd_server.lua` | focus-free TCP command server inside Windows mGBA (`seq`, `state`, `shot`); `--port=N` / `cmd_server_port` for a second instance |
| `mgba/coop.lua`, `peek.lua`, `ents.lua`, `force.lua`, `enemies.lua`, `p2_input.lua`, `wide_test.lua` | Lua helpers for Windows mGBA (console inspectors, player-2 pad bridge) |
| `mgba/netplay.lua`, `netplay_relay.py` | frame-lockstep netplay (Windows mGBA) |
| `p2_keys.py` | player-2 keyboard sender (IJKL / O U / [ ] / N M) over TCP (7777 Lua bridge, 7778 fork) |
| `win_keys.ps1`, `win_shot.ps1` | keys and screenshots for any window by title (used for the WSL fork window) |
| `mgba-wide/` | the mGBA fork: `apply.py` patches a 0.10 clone, `software-wide.c` wide renderer, `p2pad.c` TCP pad, `build-wsl.sh` |
| `gdb/*.gdb` | GDB batch scripts for mGBA's stub (boot check, crash catch, state dumps) |
| `fork/` | WSL fork test loop: `runfork.ps1` (launch + attach), `gdbsrv.sh` (persistent gdb client over a FIFO), `peek.sh`/`peek.gdb` (pause, dump party + EEPROM, continue), `keys.sh` (player-1 buttons through the pad word), `tele.sh`, `reset.sh`; Android 16 fight: `runa16.ps1`, `p2hold.py` (scripted player-2 pad), `a16fight.sh`, `a16peek.sh/.gdb`, `a16hits.gdb`, `a16install.sh` (`docs/testing.md`) |

## Where to look for...

| question | place |
|---|---|
| an engine address or struct offset | `src/coop/rom.h`, then `docs/engine.md`, `docs/rom-map.md` |
| the party block, saves, the switch screen, slot counts | `docs/party-8-slots.md`, `src/coop/party.c`, `party_save.c`, `switch.c` |
| how player 2 is spawned / bound / switched | `src/coop/player.c`, `docs/coop-design.md` "Character switching" |
| why a build crashed with "Jumped to invalid address" | `docs/testing.md` GDB recipe; savestate from an older build? (vtable copies) |
| sprite record layout, frame encoding | `docs/sprites.md`, `tools/sprites.py` |
| Buu's Fury sprites, converting them, which ids exist | `docs/sprites.md` "Buu's Fury", `tools/buu_sprites.py`, `tools/buu2log2.py` |
| display rows, transformations | `docs/sprites.md` SSJ section, `src/coop/ssj.c`, `tools/spriteclone.py` |
| ability table, stats, growth | `docs/hercule.md`, `tools/romdata.py`, `data/hercule.json` |
| dialogue text and scripts | `tools/script_dump.py`, `docs/hercule.md` section 5 |
| enemies, twins, boss bars | `src/coop/enemies.c`, `docs/coop-design.md` "Co-op difficulty" |
| the Android 16 boss, hit messages, projectiles, enemy control states | `src/coop/android16.c`, `docs/coop-design.md` "Android 16 boss", `docs/sprites.md` "Android 16 boss sprite" |
| wide view | `docs/widescreen.md`, `src/coop/wide.c`, `tools/mgba-wide/` |
| a map's layout, collision, exits, tiles; adding or re-dressing a map | `docs/maps.md`, `tools/mapexport.py`, `tools/mapbuild.py`, `data/maps/` |
| what Ryan decided / asked for | `docs/log.md`, `docs/plan-legacy-of-hercule.md` section 4, `docs/story.md` |
