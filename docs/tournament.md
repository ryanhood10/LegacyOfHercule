# Tournament mode: design in progress (Ryan, 2026-10-24)

Status: **being planned in chat, nothing built.** Ryan wants the plan settled one decision at a time before
any code. This page records what is decided, what is proposed, and the engine facts the plan rests on.
Research behind it: three read-only passes on 2026-10-24 (the Buu's Fury maps, the fighter roster, the
duel / boss / menu infrastructure), plus the sprite check and world-map photos in `build/wm/`.

## 1. Decided

- **A World Martial Arts Tournament on Papaya Island**, imported from Buu's Fury's own tournament maps
  (zone 7 there: arrival plaza with a save pad, the city street with stalls and banners, the courtyard, the
  infirmary, the fighters' waiting room with a save pad, the 720 x 720 ring with full stands). Tiles only;
  the crowd, the waiting fighters, the director and the pads are ours.
- **A world-map landing of its own**, reachable from the start of the game. Buu's Fury draws its own
  world map (Papaya is south-east of West City there); on ours that direction is inland, so the site goes
  in the island sea **south-west of West City, around (1100-1200, 2000)**, about Gingertown's distance away.
  The marker is the stock star / plain marker like every other site (Buu's icons are not decoded; a unique
  marker would be a one-off graphic).
- **Three divisions at the director's door:** 1v1, 2v2, Royal Rumble.
  - 1v1 in co-op seeds the two players on opposite sides of the bracket; they meet in the final as a duel
    (`duel.c`, different characters allowed). Earlier rounds alternate: one player fights, the other sits
    out (solo mode, as the Cell Games).
  - **2v2 is co-op only** (no AI teammate exists; 1P does not see the option).
  - **Royal Rumble = survival rounds** (Ryan, 2026-10-24): each round is a free-for-all pack in the ring,
    NPCs hit each other too, the packs get stronger each round, a headliner at the end. In co-op both
    players are in every round. NPCs target the players most but still fight each other (a target weight,
    section 4).
- **A progression track beside the story.** The tournament opens in chapter 1 with a small roster and
  grows with the story: three kinds of unlock, and story characters never unlock by being beaten (the
  tournament cannot leak the story).
  1. *Story-playables:* a party character is selectable in the tournament from the moment the story gives
     him (follows the party flags).
  2. *Beat-to-unlock:* a tournament-only fighter becomes playable the first time a player beats him in a
     match. A chosen few (each needs a player-layout record, a row, moves and derived stats).
  3. *Challengers:* win a tier under a condition, a cutscene, a hidden fighter walks in, beat him to add him
     (Mercenary Tao, King Cold once he has fight poses, a Buu-era surprise; the Mario 64 twist: win Champion
     1v1 as Hercule to meet "the real champion").
- **Fixed tiers you grow into** (Budokai's Novice / Adept / Advanced), opened by story flags, so the LoG2
  level matters; the three difficulty settings still apply.
- **Every fighter gets a kit, budgeted per fighter**: a transform-at-N%-HP boss rule (Goku goes Super
  Saiyan at half health, Frieza goes Mecha, Cell goes up a form), one or two signature moves from a shared
  projectile library (the player's Kamehameha, Masenko, Big Bang, Special Beam Cannon, Destructo Disc with
  an enemy owner), hand-written specials (16's rockets, 13's nova) only for tier finals and challengers.
- **Two new screens:** a character select with tabs (Z Fighters / Androids / Villains / Buu Saga), a face
  grid under each tab (icons made from the sprites the way the save pad's switch screen makes its
  portraits), locked fighters dark with "?", the big dialogue portrait and stats at the side; and the
  **bracket**, animated: next match highlighted, faces slide to the middle, announcer line, fade to the ring;
  in the ring both walk in, face off, the announcer calls it; the loser lies, the winner poses, the announcer
  calls the winner; back on the bracket the loser greys, the winner slides to the next column, the other
  matches of the round resolve off-screen with quick slides (NPC-vs-NPC is never played), the next match
  highlights. Buu's Fury's announcer sprite (its record 207) is cheap to bring over.
- **The waiting room shows the pool:** the fighters standing around are today's pool, spawned as actors
  with a line each.
- **Fights end at 1 HP** like duels; nobody dies. Prizes are items and a record line (LoG2 has no money);
  little or no EXP so the tournament cannot over-level the story.

## 2. Proposed, not yet decided

- **Bracket size:** 8 (one clean screen, three rounds) or 16 (two pages or small icons).
- **Unlock progress global across the three save files** (Budokai-style, in the unused 6.8 KB of the save
  chip) or per save file.
- **Ring-outs:** skip for now (no edge mechanic in LoG2; doable later as "knocked past the ring's edge").
- **Heal between rounds:** partial (a Senzu-sized chunk), full before the final.
- **Rumble rounds, a first draft:** round 1 three normal-size fighters, round 2 four, round 3 four stronger
  with one giant, then the headliner with two helpers; partial heal between; you must be standing at the
  end of a round to advance.
- **Opening tiers** (chapter -> tier): 1 Rookie from the start; 2 Adept after the Android 19 fight; 3
  Z-Fighter after Gingertown; 4 Champion in the ten-day wait; 5 Legend after the post-game; 6 the Buu phase.

### Roster by chapter (proposed)

| when | new opponents in the pool | new selectable | tier |
|---|---|---|---|
| Start (chapter 1, from Gohan's house) | Yamcha, Tien, Frieza, Mecha Frieza, Hercule "the champ" | Gohan, Krillin | 1 Rookie |
| After the forest spirit fight | (Cooler held back) | | |
| West City (2) | Piccolo | Piccolo, Hercule | |
| The Android 19 fight (3) | Android 19, Android 13 (base), Dr. Gero | Vegeta | 2 Adept |
| The road (4) | Android 17, Android 18 | | |
| Lookout / Trunks' chapter (5) | | Trunks | |
| Gingertown (5) | Imperfect Cell | | 3 Z-Fighter |
| The island (6) | Android 16 | Goku (when playable) | |
| Perfect Cell (7) | Semi-Perfect, Perfect Cell | | |
| The ten-day wait (8) | Cooler + the Namekian, Broly, Paragus; Krillin and Hercule as dojo bodies | Android 16; Super Saiyan forms | 4 Champion |
| After the Cell Games (9) | Cell Jr, the final Cell | SSJ2 Gohan | |
| Post-game (10) | Super 13 | | 5 Legend |
| Buu phase (later) | Dabura, Fat Buu, Super Buu, Kid Buu, SSJ3 Goku, Gotenks, Vegito, Uub... | Buu-era imports | 6 |

Beat-to-unlock candidates (first picks): Yamcha, Tien, Frieza, Android 17, Android 18, Perfect Cell, Broly,
plus the challengers. The rest stay opponents.

## 3. Who can fight today (facts, 2026-10-24)

- **Playable:** Gohan (kid, teen, SSJ, SSJ2), Piccolo, Vegeta (+SSJ), Trunks (+SSJ), Goku (+SSJ, halo),
  Hercule (+Super Hercule, old Hercule), Krillin, Android 16, SSJ2 Gohan. All have dialogue and status
  portraits (`docs/ids.md`).
- **Opponents with real AI now:** Cell (Imperfect 60, Semi 76, Perfect 78/79, Games 21-25, second Cell 140),
  Cell Jr 26 (pairs 141/142), Android 16 boss 109 (`android16.c`), Super 13 145 (`super13.c`), Androids 17 (3),
  18 (4), 19 (5, bunker 120/121), 20/Gero (6/7), 13 base (110/112/114), Cooler 29, the Namekian 113, Frieza 41,
  Broly 115, Paragus 116, Mercenary Tao 44 (sprite 50; a Goku-only bonus boss), Yamcha 106, Tien 98, Krillin
  64 (dojo). AI sets: 21 melee boss, 29 the 19/Gero set, 80/116/148/138 the Cell forms, 98 Krillin/Tien
  (`docs/sprites.md` 614-631).
- **Fight art checked** (`build/wm/fighters_sheet.png`): Frieza 48 and Mecha Frieza 49 have full fight sets;
  King Cold 74 has cutscene poses only (crashes under set 21); Tien 121, Yamcha 156, Krillin 76, 17 (9), 18
  (11), 19 (12), Gero (14), the Cells (28-31) all full.
- **Importable from Buu's Fury with attack sets:** Fat Buu 33, Kid Buu 37/39, Super Buu 40-43, Dabura 63,
  SSJ Goten / Trunks 92/93, the Buu-era party 97-120 (SSJ3 Goku 110/111/120), Hercule 128, a red horned
  demon 144, Goten / Trunks 212-215, Gohan / Goten 259/260, Uub 263, Vegeta and Vegito 269-277. The
  Broly pipeline (`buu2log2.Converter`, `BUU_SPRITES`) imports a humanoid boss on AI set 21 cheaply; Super
  13's hand-written specials are the expensive kind.
- **Playable with no new art:** Tien (needs abilities and a row), Paragus (Vegeta's layout).

## 4. Engine facts the plan rests on

- **Hits are list broadcasts with a type byte**: players accept types 0 and 2, enemies accept 1 (player
  attacks); enemies never hurt each other only because their handler ignores type 0. We already wrap every
  enemy's vtable for co-op, so accepting a type-0 hit from a *different* fighter is a small hook. **Who an
  enemy targets** is our `coop_target()` (`enemies.c`), per frame; pointing it at another NPC some of the time
  is the same mechanism (planned: 70 % nearest player, 30 % nearest other fighter, re-rolled every few
  seconds, always the player when close). Risk: stock AI modules assume a player target and may read
  player-only fields; prototype with two fighters in a room first.
- **Sprite memory is a GBA hardware cap**: 32 KB of OBJ tiles, and OAM's tile index cannot address more
  (a patched emulator would not help). A normal body holds about 1 KB resident, a 64 x 64 giant (Broly,
  Super 13, Cooler) 4 KB, plus HUD, bars (0x200 each, two bar positions) and projectiles. Comfortable: 2
  players plus 4 normal fighters; a giant counts for four. Entrants wait off-screen and step in as others
  fall. ROM space is plentiful.
- **Working RAM**: the GBA's 256 KB + 32 KB is not the limit; our reserved co-op region is nearly full
  (`partyram` 0x20 bytes free, `coop2` 0x24), grown by lowering the heap cap (`docs/party-8-slots.md`);
  tournament state lives on the heap while at the grounds.
- **Save chip**: 6.8 KB of the 8 KB EEPROM is unused (`party_save.c`): room for a global tournament record
  (unlocks, wins) through `Eeprom_Write`.
- **Duels** need two real pads (`vmcall.c` skips the fight otherwise); the winner is the final blow, and an
  enemy's lethal hit would also end a duel (no NPCs in the player-vs-player final).
- **Rounds**: `waves.c` (RAM-built records, clear detection, a script between rounds) is the template;
  Roshi's gym shows flag-gated rounds; `wait_spar.lscript` the duel loop.
- **UI pieces**: `switch.c` (the save pad's screen: `Portrait_Create` OBJ portraits from sprite records,
  `Canvas_*`, BG tilemap writes), `krillin.c` `status_tabs_hook` (a tab strip), `Modal_Run` screen objects.
  No list-choice widget exists (only yes / no questions).
- **A new world-map landing** has been added once (the Arctic: `data/story/arctic_route.json`, `docs/maps.md`
  351-371): a landing trigger pair on a rect with a condition, a banner name, and a minimap icon at about
  (153 + wx/45, wy/45 - 9).
- **Buu's Fury's map table** was found on 2026-10-24 (MapEntry table 0x0808E2E0, 452 rows; names UTF-16LE
  through 0x0806BCE8). A lister and renders of the tournament maps are being built in a side session
  (branch from "Buu's Fury map lister and tournament-ground renders").

## 5. Build order (once the plan is done)

1. Buu map lister; the grounds imported; the world-map landing; the save pads; the director with the
   division list.
2. The fight engine: the pool table, tiers, the 1v1 bracket in 1P (solo rounds), heal between rounds.
3. The select and bracket screens, with the animations.
4. Co-op 1v1 (duel final), then the Rumble (NPC-vs-NPC hook and target weights), then 2v2.
5. Roster imports, kits, transforms.
6. Unlocks, challengers, the global record.
