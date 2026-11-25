# How The Legacy of Hercule was made: outline and audit (2026-11-24)

For Ryan's video. Part 1 is the story of the build in the order it happened, as video chapters, each with what was
made, the interesting problem, and what to put on screen. Part 2 is the audit: the numbers checked against the repo
today, what is finished, what is half done, and where the docs disagree with the code.

Every number in this page was counted from the repo on 2026-11-24 at commit `46d6e89`, not copied from older notes.

---

## Part 1. The outline (video chapters)

### 0. The pitch (30 seconds)

- A Game Boy Advance game from 2003, *Dragon Ball Z: The Legacy of Goku II*, one player only.
- Now: two players at once, Hercule as a playable hero, new bosses, a World Tournament, a Time Chamber mode, a post-game.
- Built in 39 days (2026-10-16 to 2026-11-24), 684 commits, by Ryan directing and Claude writing the code.
- Nothing from the original game is in the repo: only our code, tools and notes. The build needs your own ROM.

**Show:** the stock title screen, then two characters fighting on one screen.

### 1. Day one: reading a game with no source code (Oct 16-17)

- There is no source. The ROM was opened in Ghidra (a decompiler) and the functions were named one by one.
- First finds: where the game reads the pad, and the player's state machine (idle, walk, dash, attack).
- The proof it could work: our own C code compiled and placed in empty space at the end of the ROM (the ROM is padded
  to 16 MiB, our code sits at `0x08800000`), with the game's own jump tables pointed at it.
- The rule that came out of it: decompile before guessing.

**Show:** Ghidra output next to the C in `src/coop/rom.h`; `tools/patch_rom.py`. **Docs:** `docs/rom-map.md`, `docs/engine.md`.

### 2. Player 2 (Oct 17)

- A second player entity with its own input, its own party slot, its own HUD panel.
- The game assumed one player everywhere. Each assumption was a bug: enemies only chased player 1, damage went to the
  wrong body, the camera followed one person, EXP went to one person.
- Fixes: enemies target the nearest player, a midpoint camera with a leash, kill EXP to whoever landed the last hit.
- The first real crash: player 2 dashing. The player's function table had 48 slots and we had copied too few.
- How player 2 connects: a keyboard or controller sent over TCP, a link cable mode, netplay, and later Parsec.

**Show:** `src/coop/player.c`, `input.c`, `camera.c`, `exp.c`; the first two-player footage. **Docs:** `docs/coop-design.md`.

### 3. Making co-op fair (Oct 17-20)

- Two players make the game too easy, so enemies scale, come in twins, and bosses bring a partner with a second bar.
- A downed partner can be revived: a countdown, hold A beside the body.
- Easy / Normal / Hard asked at a new game. One player or two asked at every start; player 2 can drop in and out.

**Show:** twin bosses with two HP bars; a revive. **Files:** `enemies.c`, `bosses.c`, `revive.c`, `difficulty.c`, `players.c`.

### 4. The wide screen detour (Oct 17)

- Two players need more room than 240 pixels. The ROM was taught to describe a wider view, and a fork of the mGBA
  emulator was patched to draw it.
- Windows blocked the unsigned build (Smart App Control), so the fork lives in WSL. That fork later became the test rig.

**Show:** wide view beside the normal view. **Docs:** `docs/widescreen.md`. **Files:** `wide.c`, `tools/mgba-wide/`.

### 5. Hercule becomes a hero (Oct 17 onward)

- The decision: "Canon plus Hercule". The real story stays; Hercule is in it. Nothing reveals him as the true hero yet.
- His stats, growth curve and four moves are a data patch (`data/hercule.json`), not hand edits.
- Super Saiyan Hercule: the game's sprite tables were relocated so new bodies could be added, then his body was
  cloned and recoloured, with a transformation that follows the stock rules (drain, turning back at zero energy).
- His Kamehameha pose was built from his own punch stance and Goku's beam arms.

**Show:** Super Saiyan Hercule transforming; the pose sheet. **Docs:** `docs/hercule.md`, `docs/sprites.md`. **Files:** `ssj.c`, `hercule.c`, `tools/spriteclone.py`, `tools/herculesprites.py`.

### 6. A bigger party (Oct 17-19)

- The game was hard-wired for six characters. The party block was moved to new memory and every place that assumed
  six was patched: saves, the switch screen, the status pages, the pause menu tabs.
- Now nine slots: Krillin, Android 16 (in one chapter), Super Saiyan 2 Gohan, plus a slot for the duel clone.
- Old saves still load on the bigger layout.

**Show:** the switch screen scrolling past six. **Docs:** `docs/party-8-slots.md`. **Files:** `party.c`, `party_save.c`, `switch.c`, `krillin.c`, `a16player.c`, `ssj2gohan.c`.

### 7. Writing new story into an old game (Oct 17-21)

- The game's scenes are bytecode. We wrote a dumper and an assembler (`lscript`) that round-trips every stock script,
  so scenes can be edited as text.
- A bridge from scripts into our C (`CoopCall`, 35 entries in the registry) for anything the script language cannot do.
- 24 chapter scripts, 11,466 lines: the opening, West City, Vegeta's clone duel (player against player), Piccolo's
  chapter, the Lookout, Gingertown, the island, Gero's computer with Android 16 as a player, the ten-day wait, the
  Cell Games, the wishes, the post-game.
- Rules that shaped it: lines Ryan dictated stay word for word; only who is being played appears in a scene; scenes
  change by who plays and whether it is one player or two.

**Show:** an `.lscript` file beside the scene it makes. **Docs:** `docs/scripts.md`, `docs/story.md`, `docs/story-rom.md`.

### 8. New bosses and new bodies (Oct 18-21)

- Android 16 as a boss with rocket punches and missiles; Android 13 beside 19 and Gero; Cooler as the forest spirit and
  his Namekian; Broly and Paragus on New Planet Vegeta; Super Android 13 as the post-game boss with homing orbs, a
  rage and an explosion.
- The art problem: no new art could be drawn by hand at scale, so bodies are built by code from existing ones (16's
  body plus 17's fight frames, recoloured part by part), and others are converted from the sequel, *Buu's Fury*,
  whose sprite format was decoded and translated (palette remap, recompression).
- 33 sprite ids in the registry beyond the stock 157.

**Show:** the sprite sheets, before and after a recolour. **Docs:** `docs/sprites.md`, `docs/art-audit.md`. **Files:** `android16.c`, `super13.c`, `tools/a16sprites.py`, `bossmodels.py`, `super13sprites.py`, `brolysprites.py`, `buu2log2.py`.

### 9. New places (Oct 18-25, Nov 5)

- The map format was decoded (layers, tiles, collision, exits, triggers) and a builder makes new maps from JSON specs.
- 33 map specs: the arctic route and its summit, the Broly arena, Papaya Island (six areas, from Buu's Fury), the Time
  Chamber's open area and its rooms.
- Some scenery is drawn by code: the Saiyan ship, the news van, the notice board, the Chamber's six entrances.
- New landmarks on the world map.

**Show:** `mapexport.py` renders; the ship. **Docs:** `docs/maps.md`. **Files:** `tools/maplib.py`, `mapbuild.py`, `shipart.py`, `chamberart.py`.

### 10. Testing a game without playing it (Oct 18 onward)

- The mGBA fork was turned into a headless runner: a text script presses buttons, checks memory, takes screenshots.
- 117 test scripts; the full regression is 103 of them run six at a time. It passed 103 of 103 on the current build.
- Drop-in savestates on F1-F9 so Ryan can jump straight to a scene.
- Debugging tools when something hangs: break on an address, watch a word, read the stack.
- Good bug stories for the video: savestates break after every rebuild (they hold code addresses); a leftover test
  launcher kept an old ROM mapped and crashed a new one; one animation call on a fresh actor never returns; a kit's
  beam freed while still in use (a jump to `0x1CF51CF0`).

**Show:** a test script, its tiled screenshot sheet, the 103 passes. **Docs:** `docs/testing.md`. **Files:** `tools/headless/`.

### 11. The World Tournament (Oct 24-31)

- Papaya Island from Buu's Fury as a place to land, walk and talk.
- An engine for brackets: 1 v 1, 2 v 2, a Royal Rumble, a 32-fighter Legend bracket, hidden challengers, unlocks, and a
  record kept on the save chip.
- Every opponent has a kit from a shared move library (beam, ball, homing, volley, wave, rush, teleport, heal, transform).
- Full-screen menus built from nothing: character select, the bracket animating, the unlock board.
- Team combos: two fighters fire one joint move, driven by step tables that are generated from a document Ryan edits.
- Fighters talk: rival lines, meeting yourself, team lines, an announcer whose calls never pause the fight.

**Show:** the bracket screen, a team combo, a Rumble. **Docs:** `docs/tournament.md`, `tournament-combos.md`, `tournament-dialogue.md`. **Files:** `src/coop/tny_*.c`.

### 12. The perfecting pass and playing with his brother (Nov 1-5)

- "Perfect what we have FIRST." A list of fixes worked as a team of sessions on separate branches, merged one by one.
- Real two-player sessions over Parsec turned up what tests cannot: player 2 could not pick up items, could not
  trigger exits or save pads, did not heal while resting. Each became a piece of code and a test.
- Performance work: frame time, the sprite memory a text box evicts.

**Show:** footage of the two of them playing. **Docs:** `docs/perfect-pass.md`. **Files:** `pickup.c`, `regen.c`, `agency.c`, `objroom.c`.

### 13. The Time Chamber as a game mode (Nov 5)

- A 2048 x 2048 open area with enemy tiers, six themed rooms with drawn entrances, levers and a gate with six lights,
  Mr. Popo as the hidden boss, and a loop that gets harder each time round.

**Show:** the open area, a room, Popo's fight. **Docs:** `docs/time-chamber.md`. **Files:** `chamber.c`, `popo.c`.

### 14. Cutscenes that move (Nov 6-13)

- Ryan's rule: never stagnant, every lineup has bodies, fits the story, smooth not overdone. Improve, never replace.
- Research first: 95 scenes filmed and rated against how the anime staged the same moment.
- A fight toolkit (`CoopCall 37`, `cine.c`): afterimage dashes, strikes with hit-stop, exchange flurries, power-ups,
  blasts, a beam struggle, kneel and fall, everyone turning to the speaker, a camera that follows.
- About 50 scenes re-staged with it, then rounds of Ryan's notes: the clone's fall and Vegeta's Big Bang, Android 16
  and Piccolo fighting Cell on the island, 17's absorption, Gohan and Hercule at the Cell Games.

**Show:** a scene before and after. **Docs:** `docs/cutscenes.md`, `docs/cutscenes/`, `docs/review-2026-11-06-cutscenes.md`.

### 15. Faces (Nov 12-24)

- Complaint: Android 13 and Super 13 wore recycled portraits. Three tries: repainted stock faces, then portraits drawn
  by code as shapes, then Ryan's own picked stills shrunk to 64 x 64 and mapped onto the game's 256 colours.
- The bodies were recoloured to match (Super 13's blue skin and orange hair, Paragus' cape and moustache).
- A good "it is harder than it looks" segment: a smoothing filter that erased the eyes, colours snapping to grey.

**Show:** the three versions side by side. **Files:** `tools/ppportraits.py`, `drawnportraits.py`, `portraitstills.py`.

### 16. How the two of us worked (the part people will ask about)

- Ryan decides, one decision at a time, from the player's point of view; Claude grounds each answer in facts from the ROM.
- Standing rules written down and kept: commit and keep going, never commit ROM data, never touch his emulator's keys,
  his dictated lines stay word for word.
- The repo is its own memory: an index, a registry of every number taken, a how-to, a dated log (176 entries).
- Many sessions at once on separate branches, each building into its own folder so they do not trip each other.

**Show:** `CLAUDE.md`, `docs/ids.md`, `docs/log.md`.

### 17. What is left

- The Buu saga as chapters inside this ROM (researched, not planned), where Hercule is finally revealed as the hero.
- The open items in Part 2.

---

## Part 2. The audit

### 2.1 Numbers (counted 2026-11-24)

| what | count |
|---|---|
| days, commits | 39 days (2026-10-16 to 2026-11-24), 684 commits (330 in October, 354 in November) |
| our C code | 56 `.c` files, 25,424 lines with headers (`src/coop/`) |
| Python tools | 93 files, 24,890 lines (`tools/`, `tools/headless/`) |
| story scripts | 24 `.lscript` files, 11,466 lines (`data/story/`) |
| new map specs | 33 (`data/maps/`) |
| test scripts | 117 in `tools/headless/`; 103 in the full regression |
| docs | 30 pages plus 3 research batches, 19,090 lines |
| registry (`docs/ids.md`) | 33 new sprite ids, 35 CoopCall entries, 47 test commands, 33 zone-15 areas, about 20 portrait rows |
| tracked files | 478; none is a ROM, save, savestate or image |

### 2.2 State of the build

| item | state |
|---|---|
| latest commit | `46d6e89` (digitized portraits), pushed to `origin/main`, `../log2` fast-forwarded |
| full regression | 103 of 103 pass on `46d6e89` |
| game in the mGBA folder | one build behind (`304e6b1`: hand-drawn portraits). The new ROM is built and waits for mGBA to be closed |
| played by a person | through chapter scenes and the tournament by Ryan alone on drop-ins; two-player sessions on 2026-11-01 and -02. The cutscene pass and everything since has been checked on film and by tests, and seen by Ryan only through the showcase keys |
| sound | never checked by Claude (headless tests have no audio) |

### 2.3 Finished and tested

Co-op core (player 2, camera, EXP, switching, pickups, agency, resting heal), difficulty and revive, one player and
drop-in, the nine-slot party and saves, Hercule and Super Saiyan Hercule, Krillin, Android 16 (boss and player), Super
Saiyan 2 Gohan, every story chapter through the Cell Games, the post-game (wishes, arctic, Super 13, ending), the Broly
side chapter, the tournament (all divisions, combos, dialogue, screens), the Time Chamber mode with Mr. Popo, the
cutscene toolkit and the re-staged scenes, the three portraits.

### 2.4 Built but waiting on Ryan

- His look at F5-F9 of the second showcase set.
- Five review items from the cutscene pass: Cell's Solar Flare at Gingertown, Vegeta's Final Flash on Perfect Cell,
  Vegeta's knee before Broly, the Super Saiyan 2 scream order, Goku's spirit.
- "BIG BANG!" or "FINAL FLASH!" as Vegeta's yell at the clone duel's end (the attack is the Big Bang, the line says Final Flash).
- Whether Gohan starts the "I give up" fight already Super Saiyan.
- The dialogue trim on the remaining cutscenes ("Not yet").
- Android 13's white hair blends into the icy background of his new portrait.

### 2.5 Not built

- The Buu campaign (decided as chapters inside this ROM; facts in `docs/story-buu-rom.md`; no plan yet).
- The Time Chamber's trophy hall and panel.
- A real draped cape on Paragus' body (his white chest stands in for it).
- The combos' longer step tables.
- A two-PC netplay run has never been done; the wide view is test-rig only.
- Boss re-tuning for Android 13 once story levels are fixed; King Cold has no story fight set.

### 2.6 Where the docs disagreed with the repo (fixed in this commit unless noted)

- `docs/INDEX.md` listed `buu2log2.py` twice: the shorter row is removed.
- `docs/INDEX.md` called `docs/perfect-pass.md` "planned, nothing built". It is built and merged: reworded.
- `CLAUDE.md` still heads its status "Current phase (2026-11-01)" and stops at 2026-11-06. **Not changed**: it is the
  file every session reads first, so a rewrite should be Ryan's call. The true latest state is `docs/log.md`.
- `docs/log.md` "Open items" still lists Super Saiyan Hercule's icon, drain and revert rules as open. They were built
  in the perfecting pass (`tools/headless/ssj_hercule.txt` tests them). **Not changed**, noted here.

### 2.7 For the video itself

- The repo holds no game data, so anything shown from it is ours. Footage of the game and the three portrait stills
  are the original owners' work: the stills live in `roms/stills/`, outside git.
- Ready-made visuals: pose sheets (`tools/artsheet.py`), map renders (`tools/mapexport.py`), test screenshot sheets
  (`tools/headless/st_test.py`), portrait sheets (`tools/drawnportraits.py`, `tools/portraitstills.py`), the showcase
  keys (`tools/headless/dropins_showcase.py`).
- The dated log (`docs/log.md`) has the exact day and wording for any moment in the outline.
