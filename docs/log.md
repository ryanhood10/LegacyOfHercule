# Progress log

Newest first. One entry per landed piece or decision; details live in the linked docs.

## 2026-10-18

- **Krillin is a real party member; old Hercule row** (branch `krillin-party`, worktree
  `F:\MeltyCreations\log2-krillin`, not merged): slot 6 on display row 23 with sprite 161 (sprite 76
  plus Goku's fight groups re-headed, `tools/krillinsprites.py`), dialogue portrait 73 and a status
  portrait (id 119, 73 in the menu BG palette), his name on the status / pause screens and in "%s"
  dialogue (relocated UI string table with per-slot blocks), status technique pages, stats / growth
  weaker than the Z fighters, abilities 21 Destructo Disc (piercing, cooldown, 12 EP), 19 Afterimage
  Rush, 20 Shockwave Punch and the stock Ki Blast recoloured blue. Old Hercule = slot 5 on row 24
  (sprite 158) via opcode 73 `(5, 158)`, back with `(5, 72)`, combat bytes / 3, no Super Hercule.
  All tested in fork slot 2 (`docs/hercule.md` 5b / 5c, screenshots `build/kr/shots/`).
- **Level-up crash fixed** (same branch): every level-up in a PARTY_SLOTS build crashed when its
  dialogue closed (`Party_AddExp` read module words below the moved literal 0x3EA0;
  `docs/party-8-slots.md` "Level-ups"). `party_add_exp_hook` re-implements it.
- **Story planning walked from the player's side through the Cell Games wait** (Ryan + Claude,
  all in `docs/story.md`, section 0 is the summary): the two duels, Android 13 and 16 as new
  bosses, Multi-Form Cell, 2v2 gym, Cooler's Namekian, the half-health and both-down rules, casting
  per chapter. Next: Android 13's partner, then the Cell Games. No story code written.
- **No-reveal rule**: Hercule is not revealed as the hero until much later; the prologue caption
  rewrites were removed (`data/hercule-text.json` is empty).
- **Merged to main**: `party-slots` (eight slots, scrolling switch screen, new save layout; plain
  `make` is now the 8-slot game, old saves read as empty, Krillin is a placeholder row) and the
  Buu's Fury sprite decode/converter (ids 158 Buu-saga Hercule, 159 LSSJ Broly; Android 13 is not
  in that ROM). Both built together after a doc-conflict merge (2f4ca5d).
- **Side sessions**: party-slots, buu-sprites and hercule-hero are done and idle; an Android 16
  boss session (art, rockets, AI, own worktree) was started from a card and then stopped; its
  branch, if any, is not merged. Ryan is closing sessions and switching models next.
- **Android 16 boss** (branch `android16-boss`, worktree `F:\MeltyCreations\log2-a16`, not merged):
  sprite 160 (`tools/a16sprites.py`: 16's stock sprite 8 groups + 17's fight frames re-headed + a
  rocket-punch group), missile / fist / fire-explosion frames, the AI `src/coop/android16.c`
  (heat-seeking rocket on the nearer player with a 3 s fuse, barrage, area bombs, rocket punch,
  poise), enemy stat 109 (4000 HP), `A16_TEST` spawn + autopilot. The autopilot pair at level 30
  wins 3 of 5 fights. Android 16 is sprite 8 in the stock game (15 is Dr. Gero). Placement for the
  island scripts, engine facts and tuning: `docs/coop-design.md` "Android 16 boss".

## 2026-10-17

- **Story decision (Ryan): "Canon plus Hercule"**: the DBZ story stays, Hercule is in every scene as
  player 2, the twist is saved for the end (surprise Super Saiyan Hercule beats Buu; Goku fuses with
  Hercule, not Vegeta, against Buu; Vegito stays for Janemba). Beat sheet in `docs/story.md`.
- **Opening redesigned by Ryan (late evening)**: future prologue keeps Trunks with old Hercule as
  comic-relief player 2; present day opens with Krillin visiting Gohan (player 2 = Krillin), a
  Frieza + Cooler "spirit of the forest" fight replaces the dream, Hercule joins in West City via a
  mansion deal with Piccolo. Checked against the dumped scripts (`build/scripts/`); the one engine
  blocker is the six-slot party (`docs/story.md` section 5). No code yet, by Ryan's request.
- **Eight party slots decided** (Ryan): `docs/party-8-slots.md` inventories every six-slot
  assumption in the ROM (27 pointer words, ~15 tail offsets, 8 count constants, the default table,
  the save path, two screens) from a whole-ROM decompile (`build/decomp_all/`) and plans the work
  in five testable steps, about three days. Not started.
- **Super Saiyan Hercule works in game** (d7093cd): sprite 157 / display row 22 generated from
  Hercule's record by `tools/spriteclone.py`, display rows and sprite index relocated into the blob
  (`patch_rom.py` LITERAL_RELOCATIONS, row-limit byte), `src/coop/ssj.c` ability pair wired into
  Hercule's third ability slot by the hercule-hero session (d8abb54). Two bugs found with the fork's
  GDB stub: co-op code read the stock display table (no row 22) and the clone stopped at 9 of 21 pose
  groups. See `docs/sprites.md` for the record facts and `docs/testing.md` for the GDB recipe.
- **Hercule hero pass** (0bf5214, d8abb54, other session): `data/hercule.json` data patch flow
  (`tools/romdata.py`), growth curve, four-ability moveset, Keep-Goku byte, dialogue dumper and the
  first text swap. `docs/hercule.md`.
- **Story and plan docs**: `docs/plan-legacy-of-hercule.md` (build plan), `docs/story.md` (ROM
  chapters, rewrite options, questions for Ryan), `docs/sprites.md` (atlas, roster: Krillin 76 and
  Tien 121 are playable-grade; 136 squirrel-like). Ryan's direction: shape the story first; only
  Super Saiyan Hercule proceeds among new-character work; Krillin/Tien deferred.
- **Dynamic wide view** (WIDE=1 + WSL fork): 240x160 when close, up to 720x480 as the players
  separate, verified both ways with player 2 on the TCP pad. `docs/widescreen.md`.
- **Co-op difficulty**: scaled enemies, twins, twin bosses with a second bar (other session).
  `docs/coop-design.md`.
- **EXP to the killing player, save-pad switching with no duplicate characters, Hercule as player
  2's default, post-cutscene stale-pointer fix, two-player dash crash fix.** `docs/coop-design.md`.
- **Netplay** (212d8e7): frame-lockstep over TCP with a relay; one-PC test only so far.

- **Buu's Fury sprite decode + converter**: index `0x086B6BDC` (297 records), palette `0x0805652C`,
  record/animation/descriptor layout, per-record colour remap tables, multi-part frames
  (`docs/sprites.md` "Buu's Fury"). `tools/buu_sprites.py` atlas + sheets + report;
  `tools/buu2log2.py` converts records to LoG2 format with the palette remapped and an LZ encoder for
  the game's decompressor; `make` links them as sprite ids 158 (Buu-saga Hercule, record 128) and
  159 (LSSJ Broly, record 28) when `roms/buu.gba` is present. Hercule verified in game as player 2.
  **Android 13 is not in Buu's Fury** (all 297 records checked); the story needs it drawn.
- **PARTY_SLOTS** (branch `party-slots`, 2026-10-17): the party block relocated to EWRAM and grown to
  N entries (default 8; Krillin placeholder at slot 6, slot 7 empty), every six-slot site patched or
  re-implemented (`src/coop/party.c`), a new EEPROM save layout and save-menu slot store
  (`party_save.c`), a scrolling Switch Characters screen with edge indicators (`switch.c`). Verified
  in the WSL fork: save, Select Game, load, switch to Krillin. `docs/party-8-slots.md`. Found on the
  way: the arena's usable end is cap words + 8 bytes (old cap overlapped `coop2` by 8 bytes).

## Open items

- Party: Krillin's own names / portrait / abilities (the status screen's per-slot tables have six
  entries), the save menu's six-wide portrait rows, the status screen's paging key; the journal with
  seen bits set was not exercised in the fork.

- Human test of the save-pad switch with player 2 present; a real two-PC netplay run.
- Unexplained crash on the stock ROM reported by Ryan ("jumped to invalid address 27994586"),
  parked until it recurs (`docs/testing.md` has the GDB recipe to catch it).
- Lua frame callbacks never fire in the SDL fork (helpers are Windows-mGBA only).
- Super Saiyan Hercule: icon for the ability, aura/flash on transform, EP drain and revert rules,
  Hercule-specific SSJ moveset; the game's own transformed-state handling only runs for the active
  player (opcode 73 refreshes player 1's entity only; `p2_refresh_sprite` covers player 2).
- Story: continue from `docs/story.md` section 0 ("Next to decide").
