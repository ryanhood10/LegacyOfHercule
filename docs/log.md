# Progress log

Newest first. One entry per landed piece or decision; details live in the linked docs.

## 2026-10-17

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

## Open items

- Human test of the save-pad switch with player 2 present; a real two-PC netplay run.
- Unexplained crash on the stock ROM reported by Ryan ("jumped to invalid address 27994586"),
  parked until it recurs (`docs/testing.md` has the GDB recipe to catch it).
- Lua frame callbacks never fire in the SDL fork (helpers are Windows-mGBA only).
- Super Saiyan Hercule: icon for the ability, aura/flash on transform, EP drain and revert rules,
  Hercule-specific SSJ moveset; the game's own transformed-state handling only runs for the active
  player (opcode 73 refreshes player 1's entity only; `p2_refresh_sprite` covers player 2).
- Story decisions (see `docs/story.md` section 4).
