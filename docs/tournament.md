# Tournament mode (planned 2026-10-24, built 2026-10-25, in Ryan's playtest rounds)

Status: **plan done (Ryan, 2026-10-25); built by the team (9a-13), polished (14), then Ryan's playtest rounds: 15
the scenes, 16 the grounds' people, 17 the easter eggs, 18 the plaza's flight spot, 19 the overhead HP bars, 20 the
art fixes, 21 the fight fixes, 22 the sprite leftovers, 23 the second polish pass (the curtain, the talk seat); the
fighters' dialogue in `docs/tournament-dialogue.md` (approved and built); 24 what is still open and the plan to fix
it (the next session starts there).** Every decision below is Ryan's unless marked "our
call" (the few he left to us when he closed the plan). Build in the order of section 9; numbers (HP, damage,
EP costs) are set when built and tuned in play. Research behind it: three read-only passes on 2026-10-24 (the Buu's Fury maps, the
fighter roster, the duel / boss / menu infrastructure), the sprite checks in `build/wm/` (`fighters_sheet.png`
for LoG2, `buu_fighters.png` and `buu_part*.png` for Buu's Fury: idle and attack frame of every record with an
attack set), and the world-map photos (`build/wm/mosaic_half.png`).

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
    out (solo mode, as the Cell Games). NPC-vs-NPC matches are never played; they resolve off-screen.
  - **2v2 is co-op only** (no AI teammate exists; 1P does not see the option).
  - **Royal Rumble = survival rounds** (Ryan, 2026-10-24): each round is a free-for-all pack in the ring,
    NPCs hit each other too, the packs get stronger each round, a headliner at the end. In co-op both
    players are in every round. NPCs target the players most but still fight each other (section 8).
- **Rumble rounds and headliners** (Ryan, 2026-10-25): the count is fighters in the ring at once, players
  included. Rookie 2 rounds (3, 3), headliner King Cold; Adept 3 rounds (3, 4, 4), headliner Android 17 & 18
  together; Z-Fighter 3 rounds (4, 4, 4; a giant in the last), headliner Android 16; Champion 4 rounds (4 x 4),
  headliner Broly; Legend 4 rounds (4 x 4; giants from round 2), headliner Super Buu changing form twice. The
  headliner enters last with two helpers. Packs are drawn at random from the tier's pool minus the headliner,
  no fighter twice in one Rumble; a fallen fighter is out; the players advance when the pack is down and both
  stand (a KO'd player is revived between rounds; every round starts at full health). A giant counts double
  against the sprite-memory budget, so a round with one has one body fewer.
- **All of Buu's Fury's cast is in** (Ryan, 2026-10-24: not held for the Buu phase). The **Legend tier
  opens after Cell and Super 13** (the post-game's flag 382) and that is where the Buu-era fighters appear.
- **Villains are playable too** (Ryan): a chosen set of villains unlock as tournament fighters (section 4).
- **A progression track beside the story.** The tournament opens in chapter 1 with a small roster and
  grows with the story: three kinds of unlock, and story characters never unlock by being beaten (the
  tournament cannot leak the story).
  1. *Story-playables:* a party character is selectable in the tournament from the moment the story gives
     him (follows the party flags).
  2. *Beat-to-unlock:* a tournament-only fighter becomes playable the first time a player beats him in a
     match.
  3. *Challengers* (Ryan, 2026-10-24, Smash-style): **winning a tournament** (a division at a tier) plays a
     cutscene in which a hidden fighter walks in and challenges you; beat him and he is unlocked. **Lose and
     you must win that tournament again for another shot.** Each division x tier has its own challenger
     (section 6), so every tournament is worth winning; some have a Mario 64 condition on top (won *as*
     someone).
- **Pop-ups** (Ryan, 2026-10-24): when the story unlocks a tournament fighter, and especially a tier or a
  division, a pop-up says so ("New tournament tier: Champion!"), shown **at a safe moment**: not inside the
  scene that set the flag but on the first frame afterwards where no scene runs, no dialogue is pending and
  the player is controllable, through the same pop-up path as the game's own "Journal Updated!" (section 8).
- **No mirror matches** (Ryan, 2026-10-25): your own party never appears as opponents. The pool is other people.
- **Rookie is the first tournament and must be exciting** (Ryan, 2026-10-25): the early pool is widened with
  fighters we build from existing bodies: **King Cold** (Ryan: "we will have to create him"), **Master Roshi** with a
  strong Kamehameha, **Chi-Chi, Videl and Chiaotzu** (Ryan, 2026-10-25: build all three); plus Mercenary Tao, Spopovich and Yamu as
  canon tournament regulars from the start (section 4a).
- **The unlock board** (Ryan, 2026-10-25): a notice board outside the arena lists every unlockable tournament
  fighter as a greyed silhouette, no name; pick one and it shows exactly what to do ("Win the Rookie
  tournament as Krillin", "Beat this fighter in a match"). Once unlocked the face and name appear in colour.
  Only tournament unlockables are on it, never story characters (section 7).
- **Unlock progress is global** (Ryan, 2026-10-25): one tournament record on the save chip, shared by the three
  save files (beat-to-unlock, challengers, forms, wins and attempts), in the unused 6.8 KB after the save
  slots; it survives a deleted file. The *pool* still follows each file's story flags, so a fresh file cannot
  fight Cell at Rookie, but a fighter earned anywhere is selectable everywhere once his tier is open.
- **Bracket size is a choice at the director** (Ryan, 2026-10-25): **Quick** = 8 fighters, three rounds, one
  bracket screen, in every tier; **Full** = 16 fighters, four rounds, two bracket pages, offered once the tier's
  pool has 15 opponents (Adept after the road fight, and every tier above; never Rookie). Either win brings the
  tier's hidden challenger; Full pays the better item prize and has its own line in the record. Co-op seeding
  (opposite halves) holds for both.
- **Fixed tiers you grow into** (Budokai's Novice / Adept / Advanced), opened by story flags, so the LoG2
  level matters; the three difficulty settings still apply. Tournament-only playables derive their numbers
  from the lead's level (the Android 16 recipe), each with its own multipliers.
- **Every fighter gets a kit, budgeted per fighter**: a transform-at-N%-HP boss rule, one or two signature
  moves from a shared library (section 5), hand-written specials only for headliners and challengers.
- **Two new screens:** a character select with tabs and a face grid, and the **animated bracket** with the
  before / after cutscenes (section 7). The waiting room shows today's pool as actors with a line each.
- **No ring-outs** (Ryan, 2026-10-25): the platform's edge stays closed ground, matches are won by damage only.
- **Full heal before every round, in every division** (Ryan, 2026-10-25): HP and ki back to full between
  matches and between the Rumble's rounds; each fight starts clean, Budokai-style.
- **Fights end at 1 HP** like duels; nobody dies.
- **Rewards** (Ryan, 2026-10-25): the *first* win of each tournament pays a **+3 capsule** (the stat set per
  tier: fifteen capsules across the board, five per stat), Full pays a second one, every later win a **Senzu
  bean** (the Rumble a second for finishing with no player KO); plus the hidden challenger and the record's
  line. Fights pay **a small EXP trickle** (about a tenth of a story boss of that tier; tuned so a full run
  nudges a level, never jumps one). LoG2 has no money.

## 2. Our calls when the plan closed (2026-10-25)

- **Kits:** sections 4a / 4b as written are the move plan for every fighter; tuned in play like the dojo.
- **Venue:** everything plays at Papaya Island. The Other World Stadium (Buu 1/20) stays rendered as a possible
  later venue (a Legend variant), not part of this build.
- **Select screen tabs:** Z Fighters (the party), Tournament (Roshi, Chi-Chi, Videl, Chiaotzu, Yamcha, Tien,
  Tao, Spopovich, Yamu, Hercule), Villains (Frieza to Super 13), Buu Saga (the Legend imports).
- **Co-op 1v1 flow:** the two players' matches alternate round by round, the other watches from the ring's
  edge in solo mode; the final between them is a duel.
- **Stats:** opponents have fixed numbers per tier (the difficulty setting still multiplies them); a
  tournament-only playable derives his numbers from the lead's level with per-fighter multipliers (the
  Android 16 recipe), so your LoG2 strength carries over and a villain feels like himself.
- **The director:** one list box (1v1 / 2v2 / Royal Rumble; 2v2 hidden in 1P), then Quick / Full for 1v1, then
  the select screen. No house rules beyond that.
- **The announcer** (Buu record 8; 207 is the Ox-King, 4e) is imported and calls every match; the crowd is LoG2's townspeople.
- **Pop-up wording:** "New tournament fighter!" / "New tournament tier: Champion!" / "New division: Full
  bracket!", one line each, the journal pop-up's style.

## 3. Tiers and their story triggers

| tier | opens when | flag |
|---|---|---|
| 1 Rookie | from the start (Gohan's house flight spot) | none |
| 2 Adept | the Android 19 + 13 fight is won (chapter 3) | `ch3_zone7`'s end flag (to confirm when built) |
| 3 Z-Fighter | Gingertown is done (chapter 5) | `ch5_lookout`'s Gingertown end flag |
| 4 Champion | the ten-day wait begins (Perfect Cell, the Games announced) | 191 (the Cell Games landing opens) |
| 5 Legend | Cell and Super 13 beaten (the post-game) | 382 (Super 13 beaten) |

Each fighter below has his own "in the pool from" trigger; a tier's roster is everyone whose trigger is met
and whose tier is at or below it. The waiting room, the select screen and the bracket all read the same table.

## 4. The complete roster

Roles: **P** playable from the story (follows the party), **O** opponent, **B** beat-to-unlock (playable once a
player beats him), **C** challenger (section 6). Art: LoG2 sprite id, or Buu's Fury record id ("Buu r.N",
imported through `buu2log2`; identities read off `build/wm/buu_part*.png`, "(?)" = to confirm in play).
Cost: *cheap* = exists or one build-script change; *moderate* = a player-layout record built from boss frames
(the Android 16 player recipe) or a converted Buu record plus a row and a portrait; *big* = hand-written
specials in C (the Super 13 kind).

### 4a. The LoG2 era (tiers 1-4)

| fighter | art | role | in the pool from | kit (transform / signatures / specials) | cost |
|---|---|---|---|---|---|
| Gohan (kid, teen) | 53 / 56, SSJ 59 | P | start | SSJ at 50 % (once the story has it); Masenko, Kamehameha | cheap |
| Krillin | 161 | P | start | Destructo Disc, Afterimage Rush, Scatter Shot (Shockwave) | cheap |
| Yamcha | 156, stat 106 | O, **B** | start | Wolf Fang Fist = Afterimage Rush + combo; Spirit Ball = a homing ball (rocket class) | moderate (row, player layout from 8 groups) |
| Tien | 121, stat 98 | O, **B** | start | Tri-Beam = a short heavy beam that costs HP; Solar Flare = stun; Multi-Form = a twin at 50 % (the pair rule) | moderate (7 groups; stock body) |
| Hercule "the champ" | 72 | O from the start (he *is* the champion), P from West City | start / West City (flag 362) | Dynamite Kick, Megaton Punch, Taunt (his own four); as an opponent on AI set 98 (works, the dojo) | cheap |
| Frieza (final form) | 48, stat 41 base | O, **B** (villain) | start | Death Beam (thin beam), Death Ball; **transforms to Mecha Frieza at 40 %** (faster, +attack) | moderate |
| Mecha Frieza | 49 | O, **B** (villain) | start | Death Beam, a rush; no transform | moderate |
| **King Cold** | **built: sprite 185** (Cooler's 64 x 64 fight set 34 recoloured, Cold's head from 74, a red cape; section 9b); 74 keeps the cutscenes | O, **B** (villain) | start (Ryan: in the Rookie bracket) | melee giant with reach, Death Beam, a throw | moderate-big (a recolour + head; new frames only if the body reads wrong) |
| Mercenary Tao | 50, stat 44 (AI 129) | O, **B** | start (a 23rd Budokai finalist) | Dodon Ray (thin beam), Crane rush, the hidden blade (a one-off heavy hit); the ROM's own AI | moderate (4 groups) |
| **Master Roshi** | **built: sprite 186** (Krillin's 76 + Goku's 60 groups re-headed with Roshi's head and beard from his NPC sprite 88, a shell; section 9b) | O, **B** | start | **MAX Power** (bulks up at 50 %: +attack, slower), a slow heavy Kamehameha, Thunder Shock Surprise (a short stun), Drunken Fist (a rush) | moderate-big |
| Chi-Chi | **built: sprite 187** (Android 18's set 11 with her NPC sprite 33's head, her LoG2 outfit; section 9b) | O, **B** | start | a frying-pan heavy hit (stun), a fast combo, no ki | moderate-big |
| Videl | **built: sprite 188** (Android 18's set 11 with Buu's Fury's Videl head, record 276; portrait 127; section 9b) | O, **B** | start (a 25th Budokai fighter) | fast combo, Hawk Arrow (a rush), flight dodge | moderate-big |
| Chiaotzu | **built: sprite 189** (kid Gohan's set 53 with his NPC sprite 32's head and outfit; section 9b) | O, **B** | start | Dodon Ray, **psychic hold** (a stun), self-destruct at 10 % (a nova that costs him the match) | big (tiny body, poses from scratch) |
| Spopovich | Buu r.242 (bald giant in black trunks, partial set) | O, **B** | start (he fought Hercule in the 24th; pre-Majin here) | brawler, toughness (poise), a bear hug (grab) | cheap |
| Yamu | Buu r.293 (lean, purple belt, partial set) | O, **B** | start (his 25th Budokai partner) | brawler, **energy steal** (drains EP) | cheap |
| Piccolo | 103 | P | West City (flag 26 / 362) | Special Beam Cannon, Scatter Shot, cape off = Transformation at 50 % | cheap |
| Vegeta | 137, SSJ 139 | P | chapter 3 (the space duel) | Big Bang, Galick Gun (beam), SSJ at 50 % | cheap |
| Android 19 | 12, stat 5 | O | the 19 fight won | absorbs ranged hits (stock), eye beams, energy drain on grab | cheap |
| Dr. Gero / Android 20 | 14, stat 6 | O | the 19 fight won | energy absorb (heals on hit), eye beam | cheap |
| Android 13 (base) | 165, stat 110 | O, **B** (villain) | the 19 fight won | S.S. Deadly Bomber (ball), rush; no transform until Legend (Super 13 is his own entry) | moderate |
| Android 17 | 9, stat 3 | O, **B** (villain) | the road fight (chapter 4's end flag) | Power Blitz (ball), Energy Barrier (poise burst), afterimage | moderate |
| Android 18 | 11, stat 4 | O, **B** (villain) | the road fight | Power Blitz, Finger Beam (thin beam), Infinity Bullet (volley) | moderate |
| Trunks | 135, SSJ 134 | P | chapter 5 (the Lookout) | Burning Attack, Sword Blast, SSJ at 50 % | cheap |
| Imperfect Cell | 28, stat 60 | O | Gingertown done | Kamehameha, tail drain (a melee hit that heals him), Multi-Form twin at 50 % | cheap |
| Android 16 | 160 boss / 175 player, stat 109 | O until he joins; P from Gero's computer (flag 421, the tournament copy stays after the Games) | the island fight done | the existing rockets, barrage, bombs, rocket punch (`android16.c`) | cheap |
| Goku | 60, SSJ 61 | P | when he is playable (the wait) | Kamehameha, Spirit Bomb (Legend final only), Instant Transmission (teleport behind the target), SSJ at 50 % | cheap + IT moderate |
| Semi-Perfect Cell | 29, stat 76 | O | flag 191 | Kamehameha, Multi-Form twin | cheap |
| Perfect Cell | 30, stat 78 | O (Champion headliner), **B** (villain) | flag 191 | Kamehameha, Solar Flare (stun), Cell Jr summon (two 31s as helpers: Rumble / 2v2), **Super Perfect at 30 %** (speed, aura) | moderate; summon big-ish |
| Cell Jr | 31, stat 26 | O (Rumble pack) | flag 191 | Kamehameha (small), rush | cheap |
| Cooler | 34, stat 29 (giant) | O, **B** (villain) | Cooler beaten (`ch8_cooler`'s flag) | Death Flash (beam), Supernova (big ball); **Ryan: not in the pool before his story fight** | moderate |
| The Namekian servant | 166, stat 113 | O (filler) | Cooler beaten | Special Beam Cannon, melee | cheap |
| Broly (LSSJ) | 159, stat 115 (giant) | O (Champion headliner), **B** (villain) | flag 402 (the Broly chapter done) | Eraser Cannon (ball), Gigantic Meteor (big ball), Blaster Shell (volley); the existing rage at 50 % | moderate |
| Paragus | 170, stat 116 | O (filler) | flag 402 | melee, Ki Blast; "my son!" taunt | cheap |
| SSJ2 Gohan | 58 | P (slot 8) | the Cell Games done | Kamehameha, SSJ2 Burst (his own) | cheap |
| Super Android 13 | 180, stat 145 (giant) | O (Legend headliner), **B** (villain) | flag 382 | the existing nova, bomber, wave, roar (`super13.c`) | cheap as O; moderate as B |

### 4b. The Buu era (tier 5 Legend, flag 382; all imported from Buu's Fury)

| fighter | Buu record | role | kit | cost |
|---|---|---|---|---|
| Goku (Buu saga) | 105 base, 108 SSJ, 110 SSJ3 | O, **B** (a second Goku skin: "Goku (Other World)", 113 with the halo) | **SSJ at 60 %, SSJ3 at 25 %** (the three records as one fighter), Kamehameha, Instant Transmission, Dragon Fist (rush) | moderate |
| Majin Vegeta | 274 / 275 (SSJ, M on the brow) | O, **B** | Big Bang, Final Flash (beam), **Final Explosion** (a nova at 10 % that costs his life: he loses, you take heavy damage) | moderate |
| Mystic Gohan | 100 | O, **B** | Masenko, Kamehameha; no transform (already at full power), high stats | moderate |
| Great Saiyaman | 101 | O, **B** | justice poses (the Taunt), Masenko; a joke fighter with Gohan's numbers | moderate |
| Gohan with the Z Sword (?) | 106 (partial set) | O only | sword reach (Sword Blast) | cheap |
| Goten (SSJ) | 114 (113 base; 92 was a guess, 4e) | O, **B** | Kamekameha (short beam), rush; small, fast | moderate |
| Kid Trunks (SSJ) | 260 (259 base; 93 was a guess, 4e) | O, **B** | Big Tree Cannon (ball), Galick-style beam; small, fast | moderate |
| Gotenks | 116 base, 119 SSJ, 120 SSJ3 (4e: 277 is a red-haired kid) | **C** (Champion 2v2: the fusion alone against your team) | **Super Ghost Kamikaze Attack** (three homing ghosts: the rocket class), Galactic Donut (a ring that stuns), SSJ3 at 40 % | moderate; ghosts big-ish |
| Vegito | 92 (SSJ; 93 has the same frames; 273 is a Vegeta, 4e) | **C** (Legend 1v1) | Final Kamehameha (beam), Spirit Sword (long melee), Big Bang Kamehameha | moderate |
| Uub | 263 | **C** (Champion Rumble, won as Hercule: "the champion's student") | ki volley, rush; Kid Buu's numbers in a boy | moderate |
| Hercule (Buu saga) | 128 (= old Hercule's body, row 24 today) | O (Legend "the champion") | his four moves; a comic headliner | cheap |
| Piccolo (Buu saga) | 212 (cape), 214 / 215 (?) | O | Special Beam Cannon, Hellzone Grenade (volley that converges) | cheap |
| Dabura | 63 (giant, winged) | **C** (Adept 2v2: alone against your team) | Evil Flame (a fire cone: Super 13's wave), **Stone Spit** (a projectile that petrifies for 3 s), Darkness Sword (reach) | moderate; petrify big-ish |
| Fat Buu | 33 (giant) | O, **C** (Z-Fighter 2v2, with Hercule) | **Candy Beam** (a thin beam that stuns), Angry Explosion (nova), **regeneration** (HP regen rule), Mouth Blast | moderate; regen cheap |
| Super Buu | 40 base, 41 (Gotenks absorbed, vest), 43 (Gohan absorbed, gi) (4e) | O (Legend headliner), **B** (villain) | **Human Extinction Attack** (a volley), Vice Shout, absorb (heals on hit); **the Buu gauntlet: 40 -> 42 at 60 % -> 43 at 30 %**, each form stronger | moderate; the chain cheap once transforms exist |
| Kid Buu | 37 (32 x 64) | **C** (Champion 1v1) | **Planet Burst** (a huge ball), Vanishing Ball, mad rush, regeneration | moderate |
| Super Janemba | 144 (giant, horned, tail) | **C** (Z-Fighter Rumble) | Dimension Sword (reach), Rakshasa's Claw (teleport behind: Instant Transmission), Hell's Gate (a portal projectile) | moderate; portal big-ish |
| Evil Buu | 32 (grey, thin, caped; 62 is a capped soldier, 4e) | O, **C** (Adept Rumble) | Mouth blast (beam), Vanishing Ball, regeneration; thinner and faster than Fat Buu | moderate |
| Pikkon | 214 / 215 (two palettes: tall hat, white scarf; 132 / 179 are capped soldiers, 4e) | O, **C** (Adept 1v1) | Thunder Flash (beam), Hyper Tornado (a spin that hits around him), Burning Shoot | moderate |
| Kibito | 148 (NPC set: idle, walk, casting, lying; 32 is Evil Buu, 4e) | **C** (Z-Fighter 1v1) | melee, **Heal** (restores his teammate in 2v2 / a Rumble ally) | moderate |
| Pui Pui | 224 (32 x 64, partial set) | **C** (Rookie 1v1) | ki volley, melee | cheap |
| Yakon | 290 (giant beast, partial set) | **C** (Rookie Rumble: a monster, story-neutral) | **Darkness Eater** (his hits drain your EP), claws | moderate |
| Buu-era Vegeta base / Goku variants | 116 (Vegeta base), 269-272, 107, 109, 114; 99 (Goku's 64 x 64 Spirit Bomb / big-beam frames); 97 / 102 Gohan outfits; 113 Goku with the halo | skins and forms of the above | | |

Not fighters (NPC grade in Buu's Fury): Supreme Kai 149, Babidi, Videl 276 (56 is Chi-Chi, 55 Chiaotzu), Android 18 (Buu) 6, Future Trunks 4,
the announcer 8 (207 is the Ox-King, 216 Pintar), Yamu 21. Not in either ROM: Olibu, Bojack, Hirudegarn, Pan. Records 259 / 260 are a lavender-haired kid in Goku's gi
(base / SSJ): kid Trunks in an alternate outfit, or Goten mis-palettised; a skin either way.

### 4c. Playable villains (Ryan: "some villains playable to make it cooler")

Beat-to-unlock set, first pass: King Cold, Tao, Spopovich, Yamu, Frieza, Mecha Frieza, Android 17, Android 18,
Android 13, Perfect Cell, Cooler, Broly, Super 13, Super Buu, plus the challengers Pui Pui, Yakon, Dabura, Evil
Buu, Fat Buu, Janemba, Kid Buu as C-unlocks. Buu imports come out of the converter already in a player layout (old Hercule plays on row 24
today), so they are the cheaper playables; LoG2 boss bodies need the Android 16 regrouping each. Every
playable villain gets: the combo, Ki Blast, two signatures from the library, and his transform where he has
one (Frieza -> Mecha, Cell -> Super Perfect, Buu's chain), with stats derived from the lead's level.

## 4d. The pool and the unlocks, tier by tier (Ryan asked for exact counts, 2026-10-25)

No mirrors: the counts are opponents other than your party. "Beat" = he becomes playable the first time a
player beats him in any match (the last hit in a Rumble). An 8-man bracket needs 7 opponents, Full (16) 15,
the Rookie Rumble (2 rounds of 3 and a headliner) 7 distinct fighters, later Rumbles up to 14.

| tier | opens | opponents in the pool (count) | playable now | unlock by beating | hidden challengers |
|---|---|---|---|---|---|
| Rookie | the first flight | Yamcha, Tien, Frieza, Mecha Frieza, King Cold, Hercule, Master Roshi, Mercenary Tao, Spopovich, Yamu, Chi-Chi, Videl, Chiaotzu = **13** | Gohan, Krillin | all but Hercule (twelve) | Pui Pui (1v1), Goten & Kid Trunks (2v2), Yakon (Rumble) |
| Adept | the 19 + 13 fight won | + Android 19, Dr. Gero, Android 13; after the road + 17, 18 = **15-18** (Full 16 opens after the road) | + Piccolo, Hercule (West City), Vegeta (chapter 3) | 13, 17, 18 | Pikkon (1v1), Dabura (2v2), Evil Buu (Rumble) |
| Z-Fighter | Gingertown done | + Imperfect Cell; after the island + Android 16 = **17-20** | + Trunks (chapter 5) | none new | Kibito (1v1), Fat Buu & Hercule (2v2), Super Janemba (Rumble) |
| Champion | flag 191 (the wait) | + Semi-Perfect, Perfect Cell, Cell Jr; Cooler, the Namekian (Cooler beaten); Broly, Paragus (flag 402) = **24-27** | + Goku, Android 16 (flag 421) | Perfect Cell, Cooler, Broly | Kid Buu (1v1), Gotenks (2v2), Uub (Rumble, as Hercule) |
| Legend | flag 382 | + the final Cell, Super 13, Buu-saga Goku / Vegeta / Gohan / Saiyaman / Goten / Trunks / Hercule / Piccolo, Super Buu = **about 40** | + SSJ2 Gohan | Super 13, Super Buu, the Buu-era heroes | Vegito (1v1), the forms (2v2), Super Buu's final form headlines the Rumble |

## 4e. Buu's Fury imports: built (art-buu, 2026-10-25)

Every Buu's Fury fighter of 4b and the announcer are converted (`data/tny_buu.txt`, `tools/buu2log2.py --spec --fit
--section .rodata.art`, Makefile `TNY_BUU_IDS`) into Hercule's 21-group player layout at the sprite ids of `tourney.h`
(195-222, 230: `docs/ids.md`), in the second ROM blob (about 950 KB of the 2 MiB; the code blob is unchanged), with
their dialogue portraits (140-168, `tools/tnyportraits.py`) in `tourney_data.c`'s portrait column (Broly's 141 too).
Test: `tools/headless/tny_artbuu.txt` (`docs/testing.md` "Tournament art").

Our calls, made while building:
- **Records checked by eye; several first guesses were wrong.** Each candidate was drawn at 3-6x (idle and attack
  frames) and matched against Buu's own portraits: Kibito is 148 (32 is a grey caped Buu: Evil Buu), Pikkon 214 (132 /
  179 / 62 are a small capped soldier), Goten SSJ 114 (92 / 93 are Super Saiyan Vegito: orange undershirt, blue gi;
  102 / 104 are Gohan in the Saiyaman tunic), Kid Trunks SSJ 260 (259 base, the only lavender-haired kid), Gotenks
  116 (277 is a red-haired kid; 117 / 118 are Gotenks NPC sets), Super Buu with Gotenks absorbed 41 (the vest; 42 / 43
  both wear Gohan's gi top), the announcer 8 (blond, sunglasses, suit; 207 is the Ox-King and 216 Pintar). The ids in
  `tourney.h` stay; their comments name the records.
- **Partial sets are mapped by hand** (`data/tny_buu.txt`): Spopovich and Yamu lie down (anim 15) for the fall and
  daze groups and power up (17) for shout and charge; Pui Pui has idle, walk, three attacks and hurt only (run =
  walk, charge = his first attack); Yakon's darkness (18) is his beam group; Kibito has no attack frames at all,
  so every attack, charge and beam group is his casting pose (anim 12) and his hurt / fall / daze is the lying one;
  Evil Buu has no hurt anim (his lean, 7, stands in). The three forms of Super Buu, Gotenks and Goku use one map each,
  so the moves piece's transform rule swaps sprites with no group surprises.
- **Hurt boxes like LoG2's** (`--fit`): Buu's boxes are 2-5 px smaller than LoG2's, and at arm's length 20 of the
  28 imports never took a punch that hits Android 17 or Cooler (the hit test). Normal bodies now carry LoG2's
  standard (-2, -26, 18, 10) on every facing (raised to the head for tall Pui Pui); giants one box over the idle
  frame's drawn body, at least 20 px either side of its centre (Cooler's is 48 wide). After it every import, and both
  stock controls, lose HP to six punches.
- **Yakon is recentred**: his 64 px body fills the frame from x + 0 (centre x + 32; the other giants centre near
  x + 16), so his frames move 16 px left; without it he stood beside his own position and punches missed.
- **Giants** (64 x 64 frames: the OBJ slot is 4 KB) are Spopovich, Yamu (lean, but his kicks need the 64 px frame),
  Yakon, Dabura, Fat Buu, Evil Buu (the cape), the three Super Buus and Janemba: `tourney_data.c` marks Yamu and
  Evil Buu `TR_GIANT` and no longer Kibito (148 is a 32 x 32 body). Kid Buu and Pui Pui are 32 x 64 (2 KB).
  Two giants on screen with player 1 leave 12-21 KB of OBJ VRAM free, largest block 12 KB or more.
- **The announcer** is a town NPC's 5-group layout (record 80): idle, walk, and the raised microphone in group 4.
- **Portraits**: Buu's Fury's own, on LoG2's grey frame (old Hercule's 126 recipe); the forms have their own (Super
  Buu 151-153, Gotenks 157-159, Goku 162-164) for the moves piece's transforms, though the roster row names the
  first. Evil Buu has none in Buu's Fury: his row keeps 0 (the ui draws his face from the sprite). Without
  `roms/buu.gba` every id gets LoG2's Hercule portrait so no id is empty.
- **Colours**: the nearest LoG2 OBJ colour per Buu colour (`build/<dir>/gen/tny_buu_report.md`). Most move 8-24
  colours by small distances; Yakon's greens come out darker and browner than Buu's (LoG2's palette has no matching
  mid greens): readable, a re-tint later if Ryan dislikes it.

## 5. The move library (shared, each move written once)

| kind | base in the game today | gives |
|---|---|---|
| beam | the players' Kamehameha / held beam, with an enemy owner | Kamehameha, Galick Gun, Final Flash, Death Beam (thin), Dodon Ray, Thunder Flash, Special Beam Cannon, Final Kamehameha, Candy Beam (stun on hit), Tri-Beam (costs HP) |
| ball | Big Bang (0x080258B4), Masenko | Death Ball, Power Blitz, Deadly Bomber, Eraser Cannon, Planet Burst (big), Supernova, Big Tree Cannon |
| homing | Android 16's rocket class | Spirit Ball, Kamikaze Ghosts (x3), Gotenks' ghosts |
| volley | Super 13's ki volley | Human Extinction Attack, Blaster Shell, Infinity Bullet, Hellzone Grenade |
| cone / wave | Super 13's wave | Evil Flame, Mouth Blast |
| nova | Super 13's nova | Angry Explosion, Final Explosion (self-cost), Hyper Tornado (short) |
| rush | Afterimage Rush, Dynamite Kick | Wolf Fang Fist, Dragon Fist, Crane rush, mad rush |
| teleport | new: move behind the target with a flash | Instant Transmission, Rakshasa's Claw |
| stun | new: a status that freezes the target N frames | Solar Flare, Stone Spit (petrify), Galactic Donut, Candy Beam |
| drain | new: a hit that moves HP or EP | tail drain, energy absorb (19 / Gero), Darkness Eater, energy steal |
| regen / heal | new: HP ticks | Buu regeneration, Kibito's Heal |
| transform | new boss rule "at N % swap sprite + stats" | every chain in section 4; the rage rule exists |
| summon | new: spawn helpers mid-fight (the Rumble entrant path) | Cell Jr summon, Multi-Form twin (the pair rule exists) |

New kinds (teleport, stun, drain, regen, transform, summon) are each one piece of C shared by everyone who
uses them; the move table then just names which fighter has which, with numbers.

## 6. The hidden challengers (one per tournament; Smash-style re-win on a loss)

Not the bracket: the bracket is drawn at random from the pool (section 4). This table says which *hidden*
fighter walks in after you win a given tournament; he is never in the pool, so that tournament is the only
way to get him. A pair challenges you in 2v2; a lone giant can challenge the team too.

| tier | 1v1 | 2v2 | Royal Rumble |
|---|---|---|---|
| Rookie | Pui Pui | Goten & Kid Trunks (the junior champions) | Yakon |
| Adept | Pikkon | Dabura, alone against your team | Evil Buu |
| Z-Fighter | Kibito | Fat Buu & Hercule (Buu saga): the odd couple | Super Janemba |
| Champion | Kid Buu | Gotenks, the fusion alone against your team | Uub (won as Hercule: "the champion's student"); else Majin Vegeta |
| Legend | Vegito (won as Goku or Vegeta); else Goku (Buu saga: SSJ at 60 %, SSJ3 at 25 %) | Goku (Buu saga) and Majin Vegeta together (the forms) | Super Buu in his final form (Gohan absorbed), after the Rumble's headliner; beating it is the record's last line |

Each challenger fight is its own cutscene in the ring after the trophy; a loss ends with "come back when you've
won again" and the record notes the attempt.

**The alternatives** (*decided* 2026-10-25, the polish, section 14): one rule for every "else" above. After a win the
tournament's own challenger comes when he is still locked, in the build and his condition is met; otherwise the
tournament's *alternative* comes, when still locked. The alternatives are beat-to-unlock fighters of the Legend pool
(Majin Vegeta, Goku (Buu saga)), so the challenge is a second way to unlock them and a loss records no attempt (that
line of the record is the challengers'). The final form is the one exception to "still locked": the Legend Rumble's
headliner is Super Buu himself, unlocked when beaten there, so his final form comes after every Legend Rumble win until
it is beaten once (the record's `extra` bit 0). The unlock board's lines say both ways ("Beat him, or win the Champion
Rumble"). The pairings are Ryan's to shuffle; the structure (one hidden
fighter per tournament, shown on the board as a silhouette with its condition) is the point.

## 7. The screens and the cutscenes

- **Division choice** at the director: a three-line list box (new widget), then Quick / Full for 1v1.
- **Character select:** tabs (Z Fighters / Androids / Villains / Buu Saga), a face grid under each (icons
  made from the sprites the way the save pad's switch screen makes its portraits), locked fighters dark with
  "?", the big dialogue portrait, name and stats at the side; in co-op player 1 picks, then player 2 (the
  same fighter twice gets the duel's violet shadow tint).
- **Bracket, animated:** next match highlighted; both faces slide to the middle ("GOHAN vs FRIEZA"); the
  announcer's line; fade to the ring. In the ring: both walk in from opposite doors, face off, the announcer
  calls it. End: loser lying, winner's pose, the announcer calls the winner, fade. Bracket: the loser greys,
  the winner slides to the next column with a sound, the other matches of the round resolve with quick
  slides, the next match highlights. Full (16) shows two pages. The Rumble uses a board of faces instead.
  Buu's Fury's announcer (record 8, NPC grade) is imported as sprite 230 (4e).
- **The unlock board** (outside the arena, on the courtyard): a grid of silhouettes, one per unlockable
  tournament fighter (B and C of section 4), drawn in one dark colour (the duel's shadow remap technique, in
  black); the cursor on one shows its condition and nothing else: "Beat this fighter in a match", "Win the
  Rookie tournament (1v1)", "Win the Champion Rumble as Hercule". Unlocked ones show in colour with the name.
  The same table drives the board, the select screen's locks and the pop-ups.
- **Pop-ups:** "New fighter: Android 17!" / "New tournament tier: Champion!" through the game's pop-up
  path, queued by the flag and shown at the next safe frame (section 8).

## 8. Engine facts the plan rests on

- **Hits are list broadcasts with a type byte**: players accept types 0 and 2, enemies accept 1 (player
  attacks); enemies never hurt each other only because their handler ignores type 0. We already wrap every
  enemy's vtable for co-op, so accepting a type-0 hit from a *different* fighter is a small hook. **Who an
  enemy targets** is our `coop_target()` (`enemies.c`), per frame; pointing it at another NPC some of the time
  is the same mechanism (planned: 70 % nearest player, 30 % nearest other fighter, re-rolled every few
  seconds, always the player when close). Risk: stock AI modules assume a player target and may read
  player-only fields; prototype with two fighters in a room first.
- **Sprite memory is a GBA hardware cap**: 32 KB of OBJ tiles, and OAM's tile index cannot address more
  (a patched emulator would not help). A normal body holds about 1 KB resident, a 64 x 64 giant (Broly,
  Super 13, Cooler, Dabura, the Buus, Janemba) 4 KB, plus HUD, bars (0x200 each, two bar positions) and
  projectiles. Comfortable: 2 players plus 4 normal fighters; a giant counts for four. Entrants wait
  off-screen and step in as others fall. ROM space is plentiful.
- **Working RAM**: the GBA's 256 KB + 32 KB is not the limit; our reserved co-op region is nearly full
  (`partyram` 0x20 bytes free, `coop2` 0x24), grown by lowering the heap cap (`docs/party-8-slots.md`);
  tournament state lives on the heap while at the grounds.
- **Save chip**: 6.8 KB of the 8 KB EEPROM is unused (`party_save.c`): room for a global tournament record
  (unlocks, wins, challenger attempts) through `Eeprom_Write`.
- **Pop-ups at a safe moment**: a per-frame tick (like `a16_frame` in `input.c`) sees a queued notice and
  shows it only when `scene_running()` is false, no dialogue is pending and player 1 is controllable; the
  pop-up itself is the stock item / journal pop-up object (`ItemPopup_Create` + `Modal_Run`, `party.c`),
  which the game already shows mid-play without harm.
- **Duels** need two real pads (`vmcall.c` skips the fight otherwise); the winner is the final blow, and an
  enemy's lethal hit would also end a duel (no NPCs in the player-vs-player final).
- **Rounds**: `waves.c` (RAM-built records, clear detection, a script between rounds) is the template;
  Roshi's gym shows flag-gated rounds; `wait_spar.lscript` the duel loop. **Fighters as opponents**: an enemy
  record with the fighter's sprite on a boss AI set (Hercule's body already fights in the dojo on set 98);
  AI sets and their group needs: `docs/sprites.md` 614-631.
- **Playable bodies**: a Buu import through `buu2log2` lands in Hercule's 21-group player layout (old
  Hercule plays on row 24); a LoG2 boss body needs the `a16sprites.py` regrouping; each playable needs a
  display row (rows 29+ free), portraits (ids 127+ free; Buu's portrait module 0x086F393C is importable, as
  old Hercule's 126 was) and a temporary party slot for the match (the way 16 and the duel clone borrow 7).
- **UI pieces**: `switch.c` (the save pad's screen: `Portrait_Create` OBJ portraits from sprite records,
  `Canvas_*`, BG tilemap writes), `krillin.c` `status_tabs_hook` (a tab strip), `Modal_Run` screen objects.
  No list-choice widget exists (only yes / no questions).
- **A new world-map landing** has been added once (the Arctic: `data/story/arctic_route.json`, `docs/maps.md`
  351-371): a landing trigger pair on a rect with a condition, a banner name, and a minimap icon at about
  (153 + wx/45, wy/45 - 9).
- **Buu's Fury's map table** was found on 2026-10-24 (MapEntry table 0x0808E2E0, 452 rows; names UTF-16LE
  through 0x0806BCE8). A lister and renders of the tournament maps are being built in a side session.
- **Buu's Fury records with attack sets** (from `build/buu_sprites/report.md`, animations 2-6 present): 43
  full sets, 26 partial; the full list and frames are in `build/wm/buu_fighters.png`.

## 9a. The team (2026-10-25, Ryan: "deploy a team ... you make the decisions")

Seven sessions, each in its own git worktree on its own branch, building into its own `BUILD=build/<piece>`;
the main session merges, rebuilds and runs the whole regression. The contract is `src/coop/tourney.h` (ids, the
roster table's shape, every cross-piece function) with `tourney_data.c` (the roster) and `tourney_hooks.c`
(weak defaults and the dispatchers); numbers in `docs/ids.md` "Allocations for the tournament build".

| piece | owns | done when |
|---|---|---|
| grounds | Papaya Island maps (zone 15 areas 11-17), the world-map landing, pads, crowd, director, board object, waiting-room actors | you can fly in, walk the grounds, save, talk to the director (CoopCall 23) and the board (CoopCall 28) |
| art-new | King Cold, Master Roshi, Chi-Chi, Videl, Chiaotzu bodies (sprites 185-189, portraits 127-139) | each runs as an opponent on AI set 21 and as a player body |
| art-buu | every Buu's Fury import (195-230) and portraits (140-189) | each converted record spawns and fights; portraits load |
| playables | player-layout LoG2 bodies (235-249), display rows 29-79, `tny_play_row` / `tny_play_entry` | every TR_BEAT / TR_CHALLENGER fighter has a row and plays |
| engine | the pool, brackets, simulation, the match loop in all divisions, heal, rewards, the global record, unlocks, challengers, pop-up queue | a 1P Rookie 1v1 can be entered, fought, won, and unlocks recorded; co-op flows tested |
| moves | the kits (section 5), transforms, NPC-vs-NPC hits and Rumble targeting | every opponent fights with his kit; a Rumble pack fights itself |
| ui | list box, select screen, bracket screen and animation, Rumble board, unlock board, pop-up look | each screen opens from its CoopCall and returns its answer |

## 9b. The grounds (built 2026-10-25, branch `tny-grounds`)

Papaya Island as six zone-15 maps, Buu's Fury's zone 7 imported whole (`data/maps/papaya_*.json`, `"requires":
"buu"`: tiles converted to LoG2's palette, Buu's own collision), the world-map landing, the people, the director,
the board and the waiting room's fighters. Code: `src/coop/tny_grounds.c` (CoopCall 30, test command 30),
`src/coop/tny_ring.h` (the ring and waiting-room numbers the engine includes); scenes and lines:
`data/story/papaya.lscript`; the landing: `data/story/papaya_route.json`; the board's art: `tools/boardart.py`.

| area | map (Buu source) | size | what is there | exits (rect, hold) -> arrival |
|---|---|---|---|---|
| 11 | plaza (7/1 v2 0x0824AE74, no plane) | 480 x 560 | save pad on Buu's Capsule Corp pad (112, 304)-(169, 345); flight spot on the beach (200, 368)-(264, 416) -> world (1260, 1820); the welcome (enter (176, 344)-(288, 432), flag 441 clear); the announcer (232, 300) (sprite 230, stand-in 99 without it); three fans | top (176, 0)-(304, 48) Up -> street (240, 744) |
| 12 | street (7/2) | 480 x 800 | vendor at the food stall, ZZTV reporter, three fans | gate (216, 0)-(264, 48) Up -> courtyard (240, 196); bottom (0, 764)-(480, 800) Down -> plaza (240, 64) |
| 13 | courtyard (7/3) | 480 x 240 | the unlock board (our art, x 40-87, y 128-167; A in (36, 164)-(92, 204): CoopCall 28, 1); a fan | gate (216, 212)-(264, 240) Down -> street (240, 64); stadium door (216, 56)-(264, 100) Up -> infirmary (400, 120) |
| 14 | infirmary (7/4) | 480 x 160 | a nurse | right (376, 136)-(424, 160) Down -> courtyard (240, 116); left (32, 136)-(56, 160) Down -> waiting room (360, 112) |
| 15 | waiting room (7/5), the hub | 400 x 400 | save pad on Buu's pad (24, 264)-(65, 305); the director (168, 328) at the ring door; today's fighters (six spots) | top (336, 56)-(384, 100) Up -> infirmary (44, 120); ring door (96, 330)-(144, 360) Down -> ring walkway (360, 192) |
| 16 | ring (7/6) | 720 x 720 | the platform x 232-487, y 256-511, closed all round; the walkway's end says "The ring is for fighters in a match" | walkway door (336, 120)-(384, 170) Up -> waiting room (120, 312) |

Our calls (the grounds piece, Ryan: "you make the decisions"):

- **The landing is at (1260, 1820)**, rect (1204, 1760)-(1317, 1881), not (1100-1200, 2000): on our world map that
  spot is brown desert (checked on screen with the test warp); 180 px north is the island sea, and at (1260, 1820)
  the flyer's shadow sits on a small island among rocks. No overlap with any landing rect (the nearest are West
  City (1440, 1528)-(1553, 1649) and (840, 1416)-(953, 1537)). Open from the start (its only condition: the grounds' maps are in the build, `CoopCall 30, 2`; `BUU=0` has no
  landing), banner
  "Papaya Island" (ui_strings 357), the plain marker at (181, 31) on the minimap.
- **The infirmary is in**, as Buu's Fury chains it (courtyard -> infirmary -> waiting room); area 17 stays free
  (the smashed ring 7/7 is not imported).
- **The pads keep Buu's art:** its Capsule Corp pad is the same pad LoG2 draws (zone 7 area 9), so the save pad
  trigger sits on it instead of stamping LoG2's pad over it.
- **The ring stays sealed from the walkway** (Buu's collision: one blocked row between them). Players who walk in
  from the waiting room see the ring from the walkway; fighters are warped onto the platform by the engine, and no
  door or edge can be reached from the platform (no ring-outs, no walking out of a match).
- **Names:** "Papaya Island" (357) outdoors, "World Tournament" (358) indoors and the ring. Music: West City's
  (41) outdoors, Roshi's dojo (29) indoors, the Cell Games arrival (6) in the ring. Town flags (0) everywhere but
  the ring (1).
- **People are stock townspeople** on stock sprites (83, 85, 86, 88, 89, 111, 150, 152, 153; the director 100, the
  announcer's stand-in 99), speaker 0 with the name in the line (no portraits needed).
- **The waiting room's fighters** are town NPCs made in C from one scratch record in `.tny.gr` (the NPC keeps a
  pointer to the record's behaviour words, which never change); six spots, a giant takes two, shuffled each
  visit, a fighter whose sprite is not in the build is skipped. Their greetings live in `tny_grounds.c` (one per
  opponent, by name). `Character_GetSpriteRecord` reads `sprite_index` without a bound, so the grounds check the
  id against `sprite_index_count` (new, `tools/spriteclone.py`); **`tny_fighter_ok` in `tourney_data.c` should do
  the same** (engine): an id past the table returns garbage, not 0.
- **The director and the board** branch on what their CoopCall pushes (a sentinel 0 under it): nothing pushed (the
  weak default) shows the fallback line ("Sorry, the tournament isn't ready yet!" / "the posters haven't gone up
  yet!"); any pushed value ends the talk (1 = took over; 0 = declined, quietly).

`tny_ring.h` (the engine includes it): `TNY_RING_ZONE` / `AREA` 15 / 16; the platform `TNY_RING_X0..X1` 232..488,
`Y0..Y1` 256..512 (exclusive ends; an entity stops about 16-20 px short of a blocked edge); entry points
`TNY_RING_P1` (280, 360), `TNY_RING_P2` (280, 416) on the left, `TNY_RING_OPP` (440, 384) on the right (more
opponents step 56 px in y); camera centre `TNY_RING_CENTER` (360, 384); the waiting room's return point
`TNY_WAIT_*` (15 / 15, (200, 300), in front of the director). Test: `tools/headless/tny_grounds.txt`
(`docs/testing.md` "Tournament grounds").
## 9b. art-new: the five fighters built from existing bodies (built 2026-10-25, branch `tny-art-new`)

King Cold 185, Master Roshi 186, Chi-Chi 187, Videl 188, Chiaotzu 189 (`tools/tnysprites.py`, `docs/sprites.md`
"Tournament fighters (art-new)"): each one record in the player layout whose groups 0-2 and 5-8 also serve the melee
boss AI set 21, tested headless as an opponent (10 s on set 21) and, for the four normal bodies, as player 1's body
(`tools/headless/tny_artnew.txt`). Portraits: King Cold 70, Roshi 80, Chi-Chi 43, Chiaotzu 42 (stock, in
`tourney_data.c`), Videl 127 (new: Buu's Fury's). Our calls, made while building:

- **Chi-Chi wears her LoG2 outfit** (the orange tabard over white sleeves of her NPC sprite 33), not a red / purple
  qipao: the game already draws her that way, and her head is her own NPC head.
- **Videl has her short hair** (the 25th Budokai look; Buu's Fury's own Videl, record 276, and portrait 153 have it),
  not the pigtails: the tournament is where she cut it.
- **Chiaotzu dresses as LoG2's Chiaotzu** (sprite 32: light-blue shirt, red sash, black trousers, white face, the cap
  with the red ball), not in green: the stock NPC is the reference players know.
- **Roshi fights in the orange turtle gi** (Krillin's body), with his own head, beard and sunglasses from his NPC
  sprite 88 and the shell on his back.
- **King Cold keeps Cooler's size** (about 44 px tall against the 62 px of his cutscene sprite 74): the frames are
  Cooler's (his attacks, the death pose), only head, colours and cape are Cold's. A 64 x 64 body all the same, so he
  counts as a giant (`TR_GIANT`).
- The sprite ids and portrait 127 are Makefile knobs (`KING_COLD_SPRITE` ... `VIDEL_PORTRAIT`) matching `tourney.h`.

For the other pieces: the records carry groups 16 (special melee) and 18 (beam pose, 7 frames) for the playables' kits
(Roshi and Chiaotzu have real beam poses from Goku / kid Gohan, the others their charge pose). King Cold as a player
body needs a display row (the playables piece): his OBJ VRAM slot must come from his record (0x1000), so poking his
record into a bound player overflows. Observed while testing (for the engine's Rumble, where fighters step in as
others fall): a body spawned with test command 8 on the same map right after another fighter's 10-s fight and
knock-out showed in pieces or not at all for one to two seconds; not reproduced after a short fight, cause not found
(suspects: the dead one's boss bar, which stays in the entity list, or OBJ VRAM held by the fight's effects).
`docs/testing.md` "Tournament fighters (art-new)".

## 9. Build order (once the plan is done)

1. Buu map lister; the grounds imported; the world-map landing; the save pads; the director with the
   division list; the pop-up path.
2. The fighter table, tiers and triggers; the fight engine; the 1v1 bracket in 1P (solo rounds), heal
   between rounds; the transform rule.
3. The select and bracket screens, with the animations and the announcer.
4. Co-op 1v1 (duel final), then the Rumble (NPC-vs-NPC hook and target weights), then 2v2.
5. The move library's new kinds (teleport, stun, drain, regen, summon), then kits fighter by fighter.
6. Roster imports (Buu's Fury), playable villains, portraits.
7. Unlocks: beat-to-unlock, the challenger ladder with its cutscenes, the global record, the pop-ups.

## 10. The playables (built 2026-10-25, branch `tny-playables`)

Every `TR_BEAT` / `TR_CHALLENGER` fighter of the roster (38 today) plays on a display row of his own. Code:
`src/coop/tny_play.c` (`tny_play_row`, `tny_play_entry`, the kits), `tools/tnyplayables.py` (the bodies and the
rows), test `tools/headless/tny_play.txt` (generated by `tny_play_gen.py`; `docs/testing.md` "Tournament playables").

**How the engine uses it:** for a match it borrows a party slot (7, as 16 and the duel clone do), writes
`tny_play_entry(id, &slot, &lead)` there and binds the player to it, then warps into the ring. The entry: row
`tny_play_row(id)`, the lead's EXP (his level), HP / EP / STR / POW / END as the fighter's percentages of the lead's
(full HP and EP), flags 0x7B (in party, available, all four moves learned), four abilities, ability index 0. A
fighter that is not a tournament playable gets a copy of the lead's entry (story playables play from their slot).
Player 2 rebound to a body of another OBJ size (a 64 x 64 one) is rebuilt by `player.c` (`p2_bind`); player 1 must
change body through a map load (a warp), never in place: `PlayerEntity_RefreshSprite` keeps the old VRAM slot.

**Decisions (ours):**
- *Rows by table order.* Row = 29 + the fighter's place among the table's playables in `TFighterId` order; the rows
  and the map are generated from `tourney_data.c` at build time, so a role change in the table moves the rows with
  it (both sides regenerate). Rows past the last fighter (67-79 today) are Hercule's row as filler.
- *Missing art degrades, never crashes.* A row whose `play_sprite` record is not in the build (art-new / art-buu not
  merged, `BUU=0`) shows Hercule (72); Super Android 13's 180 is Perfect Cell's enemy groups with `BUU=0`, so his row
  uses the Perfect Cell player body (243) then. Broly (159) and Super 13 (180) play on their own records with
  `BUU=1` (Buu imports are already in the player layout; giants need nothing more).
- *Portraits:* row byte 2 = the table's dialogue portrait (Hercule's 67 when the table has 0), byte 3 = Hercule's
  status portrait (the table holds no status portraits; a dialogue portrait there would show in the wrong palette).
  When the art pieces give the table real portrait ids the rows pick them up at the next build.
- *Bodies (sprites 235-244):* Goku's player header, groups filled the way Hercule's set is (0-8, 11, 12, 13, 17, 20)
  from the body's enemy groups, every array padded to Goku's frame count, frames stay the ROM's (Android 13's are
  rebuilt from `bossmodels.build_a13`); all of it `TNY_ART` (52 KB). Giants keep their 0x1000 VRAM size and hurt boxes.
- *No new abilities (24-40 stay free)* (since section 21: 24 is Energy Steal, Spopovich's and Yamu's). Every kit is built from the table: beam abilities need the beam groups
  18 / 19, so only Yamcha, Perfect Cell and Cooler (whose records have a group 18: Cell's own, the others' charge
  pose) get the Kamehameha; Special Beam Cannon, Spirit Bomb, Sword Blast and Energy Punch are on nobody. Signature
  moves the table lacks (Death Beam, Tri-Beam, Solar Flare, Candy Beam, energy steal...) are stand-ins below; the
  moves piece's library (section 5) can replace a stand-in later by giving the fighter a new ability id from 24-40.

**Bodies with missing or odd groups** (source group -> player group):

| body | what is borrowed |
|---|---|
| Yamcha 156, Android 18 11 | nothing (full enemy set) |
| Tien 121 | no attack 7: player 7 = his attack 8 |
| Frieza 48 | no knock-back, death fall or shout: knock-down, lying and tumble = the hit-stun; shout = the charge pose |
| Mecha Frieza 49 | no hit-stun and no attacks: Frieza's (48, the same body without the metal); his group 3 is his death (Trunks' sword cuts him in two), so his knock-down, lying and tumble are Frieza's hit-stun too |
| Tao 50 | one attack only (6 = 7 = 8); no knock-back or death fall (hit-stun). The body looks like a red-armoured robot more than Tao: the roster's art call, to confirm by eye |
| Android 13, 17, Perfect Cell 30 | no shout: the charge pose |
| Cooler 34 | no knock-back: knock-down and tumble = his death fall (12); shout = the charge pose |
| Broly 159, Super 13 180 | Buu-import player layout: nothing (180 with `BUU=0`: see above) |

**Kits and multipliers** (percent of the lead's HP / EP / STR / POW / END; ability ids after the names):

| fighter | abilities | HP EP STR POW END |
|---|---|---|
| Yamcha | Afterimage Rush (Wolf Fang Fist) 19, Kamehameha 3, Big Bang (Spirit Ball) 0, Ki Blast 2 | 100 100 100 95 100 |
| Tien | Burning Attack (Tri-Beam) 1, Shockwave Punch (Solar Flare) 20, Big Bang 0, Ki Blast 2 | 105 100 100 105 105 |
| Frieza | Ki Blast (Death Beam) 2, Big Bang (Death Ball) 0, Scatter Shot 8, Afterimage Rush 19 | 120 120 115 125 115 |
| Mecha Frieza | Ki Blast 2, Afterimage Rush 19, Big Bang 0, Shockwave Punch 20 | 125 110 120 115 125 |
| King Cold | Megaton Punch 16, Ki Blast (Death Beam) 2, Big Bang 0, Shockwave Punch 20 | 140 100 125 105 125 |
| Master Roshi | Masenko (his Kamehameha wave) 4, Afterimage Rush (Drunken Fist) 19, Shockwave Punch (Thunder Shock) 20, Ki Blast 2 | 85 110 85 95 85 |
| Mercenary Tao | Ki Blast (Dodon Ray) 2, Afterimage Rush (Crane rush) 19, Shockwave Punch 20, Big Bang 0 | 100 100 105 100 100 |
| Spopovich | Rolling Hercule Punch 14, Megaton Punch 16, Energy Steal 24, Shockwave Punch 20 | 140 80 125 75 135 (section 21) |
| Yamu | Afterimage Rush 19, Energy Steal 24, Ki Blast 2, Shockwave Punch 20 | 100 110 105 105 95 (section 21) |
| Chi-Chi | Rolling Hercule Punch (the frying pan) 14, Dynamite Kick 15, Afterimage Rush 19, Taunt 17 | 80 70 90 70 80 |
| Videl | Afterimage Rush (Hawk Arrow) 19, Dynamite Kick 15, Rolling Hercule Punch 14, Ki Blast 2 | 85 80 95 80 85 |
| Chiaotzu | Ki Blast (Dodon Ray) 2, Shockwave Punch (psychic hold) 20, Masenko 4, Big Bang 0 | 75 110 70 105 75 |
| Android 13 | Big Bang (S.S. Deadly Bomber) 0, Afterimage Rush 19, Ki Blast 2, Shockwave Punch 20 | 125 115 120 115 125 |
| Android 17 | Big Bang (Power Blitz) 0, Shockwave Punch (Energy Barrier) 20, Afterimage Rush 19, Ki Blast 2 | 120 120 120 120 115 |
| Android 18 | Big Bang (Power Blitz) 0, Ki Blast (Finger Beam) 2, Scatter Shot (Infinity Bullet) 8, Afterimage Rush 19 | 115 120 115 120 110 |
| Perfect Cell | Kamehameha 3, Shockwave Punch (Solar Flare) 20, Big Bang 0, Ki Blast 2 | 145 130 140 140 140 |
| Cooler | Kamehameha (Death Flash) 3, Big Bang (Supernova) 0, Scatter Shot 8, Ki Blast 2 | 140 125 135 135 135 |
| Broly | Big Bang (Eraser Cannon) 0, Scatter Shot (Blaster Shell) 8, Super Saiyan 2 Burst (Gigantic Meteor) 23, Afterimage Rush 19 | 160 120 150 135 150 |
| Super Android 13 | Super Saiyan 2 Burst (the nova) 23, Big Bang (Deadly Bomber) 0, Scatter Shot (volley) 8, Ki Blast 2 | 155 120 145 140 145 |
| Goku (Buu Saga) | Burning Attack (Kamehameha stand-in) 1, Afterimage Rush (Dragon Fist) 19, Super Saiyan 2 Burst 23, Ki Blast 2 | 140 140 140 145 135 |
| Majin Vegeta | Big Bang 0, Burning Attack (Final Flash) 1, Super Saiyan 2 Burst (Final Explosion) 23, Ki Blast 2 | 140 135 145 140 135 |
| Mystic Gohan | Masenko 4, Burning Attack (Kamehameha stand-in) 1, Afterimage Rush 19, Ki Blast 2 | 140 140 145 145 135 |
| Great Saiyaman | Masenko 4, Dynamite Kick 15, Taunt (justice poses) 17, Ki Blast 2 | 110 110 105 105 105 |
| Super Buu | Scatter Shot (Human Extinction) 8, Shockwave Punch (Vice Shout) 20, Big Bang 0, Super Saiyan 2 Burst 23 | 155 140 150 150 145 |
| Pui Pui | Scatter Shot 8, Rolling Hercule Punch 14, Ki Blast 2, Afterimage Rush 19 | 100 100 100 100 100 |
| Goten | Afterimage Rush 19, Masenko (Kamekameha stand-in) 4, Ki Blast 2, Dynamite Kick 15 | 105 110 110 110 100 |
| Kid Trunks | Big Bang (Big Tree Cannon) 0, Burning Attack 1, Afterimage Rush 19, Ki Blast 2 | 105 110 110 110 100 |
| Yakon | Rolling Hercule Punch (claws) 14, Megaton Punch 16, Shockwave Punch 20, Afterimage Rush 19 | 130 90 120 90 125 |
| Pikkon | Burning Attack (Thunder Flash) 1, Shockwave Punch (Hyper Tornado) 20, Big Bang (Burning Shoot) 0, Ki Blast 2 | 125 120 125 120 120 |
| Dabura | Scatter Shot (Evil Flame) 8, Ki Blast (Stone Spit) 2, Megaton Punch 16, Shockwave Punch 20 | 135 125 130 130 130 |
| Evil Buu | Burning Attack (mouth blast) 1, Big Bang (Vanishing Ball) 0, Afterimage Rush 19, Ki Blast 2 | 130 125 130 130 125 |
| Kibito | Rolling Hercule Punch 14, Shockwave Punch 20, Ki Blast 2, Afterimage Rush 19 | 115 110 110 110 115 |
| Fat Buu | Ki Blast (Candy Beam) 2, Super Saiyan 2 Burst (Angry Explosion) 23, Burning Attack (mouth blast) 1, Megaton Punch 16 | 150 120 135 125 140 |
| Super Janemba | Afterimage Rush (Rakshasa's Claw) 19, Big Bang (Hell's Gate) 0, Scatter Shot 8, Rolling Hercule Punch 14 | 145 130 140 135 140 |
| Kid Buu | Big Bang (Planet Burst) 0, Scatter Shot 8, Afterimage Rush (mad rush) 19, Super Saiyan 2 Burst 23 | 145 140 145 150 135 |
| Gotenks | Destructo Disc (Galactic Donut) 21, Big Bang (the ghosts) 0, Afterimage Rush 19, Ki Blast 2 | 130 130 135 135 125 |
| Uub | Scatter Shot 8, Afterimage Rush 19, Ki Blast 2, Shockwave Punch 20 | 130 125 135 130 125 |
| Vegito | Burning Attack (Final Kamehameha) 1, Afterimage Rush (Spirit Sword) 19, Big Bang 0, Super Saiyan 2 Burst 23 | 160 150 155 155 150 |

The art pieces' bodies (King Cold, Roshi, Chi-Chi, Videl, Chiaotzu, every Buu import) have no beam group as far as
the contract says, so their kits avoid the beams; if a body arrives with a group 18, its fighter can take the
Kamehameha (3) in `tny_play.c` (Roshi first). Tested on this branch with those bodies degraded to Hercule's.
## 11. The moves (built 2026-10-25, branch `tny-moves`)

`src/coop/tny_moves.c`; tests `tools/headless/tny_moves.txt` and `tny_rumble.txt` (`docs/testing.md` "Tournament
moves"). What it does and the calls we made:

- **`tny_moves_attach(e, fighter)`** works on any enemy entity (the ROM's vtable, the co-op copy, Android 16's or
  Super 13's private one). The fighter gets a private vtable copy of the one he had (destroy, update, messages
  wrapped; the rest untouched, so the co-op pair / both-down / rage rules still run) and a private stat copy
  (`e->behaviour`). `enemies.c` counts him as before: `is_coop_enemy` / `is_enemy` also ask `tny_moves_base(e)`
  (the one edit outside the piece, two lines). Up to 8 fighters with a kit at once; state on the heap, the
  registry in `.tny.mv` (magic "MOVE"), dropped when a fighter leaves the entity list (a map change).
- **The kit table `tny_kits[TF_COUNT]`**: one row per fighter, up to six moves `{kind, a, b, flags, power, cd}`;
  every TR_OPPONENT and TR_CHALLENGER fighter has a row (sections 4a / 4b). Numbers are ours, to tune in play:
  `power` is the 10.10 multiplier on the stock damage formula with the fighter's own stat (`stat_ranged` for
  blasts, `stat_melee` for rush hits), `cd` the move's own cooldown in frames, plus a pace between specials (150-225
  frames). A special starts only from idle or walking, with the target within 170 px; beams only when the target
  is in line (they fire along the four facings, as the players' do).
- **The library's kinds** (section 5), each written once: beam (the players' Kamehameha beam object with the
  fighter as owner: an enemy's hit-type byte +0x16C is 0, so it hurts players only; thin = 112 px, stun = Candy
  Beam, cost = Tri-Beam's 5 % HP), ball (Super 13's orbs; the large one is held over the head and thrown slowly
  homing), homing (Android 16's rockets, or orbs: Spirit Ball, Gotenks' ghosts), volley (small orbs or Krillin's
  blue ki blasts; converge = Hellzone Grenade), wave (Super 13's), nova (Super 13's; self-KO = Chiaotzu at 10 %,
  Final Explosion at 10 %: the fighter takes his own remaining HP through his OnHit, so the engine sees a normal
  defeat), rush (a dash with map collision and N hits; stun = a grab, a frying pan, a throw), teleport (a flash,
  behind the target, a strike; dodge = hop away), stun (a flash round him: Solar Flare; the target at range:
  psychic hold; a projectile: Stone Spit 3 s, Galactic Donut), heal (Kibito: the most hurt fighter of his side),
  drain (passive: his melee hits heal him, or take a player's ki; Spopovich and Yamu instead have Energy Steal, a rush
  that only fires from half HP and turns the ki it takes into HP: section 21), regen (passive, permille of max a second),
  transform (at N % HP, once: a power-up pose, the new body, + attack / defence), summon (helpers beside him; at N %
  HP a copy of himself with half his health: Multi-Form).
- **Knock-back and the fall** (section 21): a player's hit pushes him away (the finisher far); at HP 0 he falls and
  stays down, out of the fight (no AI, no hits, no target) until the engine clears the body.
- **Stuns**: a stunned player's pad is dead (both pads read 0 in the input poll) with a ring of orbs over his head; a
  stunned fighter stands in his hurt pose with no AI. A special cannot be stunned out of, and a hit during one
  only does damage (Android 16's rule); KF_POISE fighters (giants, bruisers, Android 17, Super Perfect Cell...)
  also get a 75-frame poise window after a stun.
- **Transforms check the body** (our call): a body must have every pose group its AI set plays
  (`tny_moves_body_fits(sprite, ai_set)`, the table in `docs/sprites.md`); Mecha Frieza's stock sprite 49 has only
  groups 0, 1, 3, 5, and under Frieza's set 21 the first swing jumps to C002E560 (found by `tny_rumble.txt`). So a
  transform takes the sprite if it fits, else the play body of the fighter shown by that sprite (Mecha Frieza ->
  the playables' 238), else stays a stat boost on his own body. Same rule for spawns: test command 26 and the
  summons skip a fighter whose body does not fit (`mv.spawn_fail` 4). **The engine should do the same** before it
  spawns an opponent (Mecha Frieza needs TS_P_MECHA; sprite 49 as an opponent crashes).
- **Summons** (our call): Cell Jr. helpers only in a Rumble or a 2v2 (`MF_TEAMONLY`, from `tny_bracket()`'s
  division), Multi-Form copies in every division; helpers and copies never summon or transform. They spawn through
  `enemy_spawn_hook` with co-op twins off and get a kit of their own, on the summoner's side.
- **Rumble mode (`tny_rumble_mode`)**: a fighter's message wrapper takes a type-0 hit from a fighter of another side
  as a player's (type 1) and gives it back as type 0 to the next receiver; his own blasts and projectiles never hit
  him; helpers share their summoner's side. Targeting is done in the fighter's own update, not in
  `coop_target()` (our call: `entities.c` only asks `coop_target()` when player 2 is present, and the Rumble must
  work alone too): the nearest player 70 %, the nearest other fighter 30 %, re-rolled every 3 s, always a player
  within 40 px; `g_PlayerObject` points at the target while the stock AI runs (as `entities.c` does for player 2).
  **Prototype result**: every AI set of the roster ran against other fighters for 20 s per pack with no crash, in
  co-op and alone (`tny_rumble.txt`: sets 21, 98, 129, 29, 80, 116, 148, the Namekian's, Paragus', Broly, Cooler,
  Android 16, Dr. Gero, Android 13, Super 13, Cell Jr., old Hercule); no AI set had to stay player-only.
  Android 16 and Super 13 keep their own specials and aim them at players only (natives: `KF_NATIVE`); they take
  Rumble hits like everyone.
- **Kill credit**: an NPC's killing blow gives nobody EXP (the stat copy's EXP is 0 during that hit);
  `mv.npc_kos` counts NPC-vs-NPC eliminations, and `tny_moves_last_hitter(e)` gives the engine the player or fighter
  behind the last hit a fighter took (beat-to-unlock in a Rumble: the last hit).
- **Not done here**: no twin rule for opponents (the engine adds it; our test spawns set `COOP_OFF_TWINS` around the
  spawn); speed changes on transform (the chase modules are built once; only attack and defence rise); Gohan /
  Goku / Vegeta / Trunks / Piccolo transforms (story playables, never opponents); the engine's stats (160 + id) are
  used when a build has them, else a stand-in stock stat per body (`stand_in_stat[]`).

## 12. The screens (built 2026-10-25, branch `tny-ui`)

`src/coop/tny_ui.c` (the screen framework, the list box, the pop-up, CoopCall 28-29, test commands 28-29),
`tny_ui_grid.c` (the character select and the unlock board), `tny_ui_bracket.c` (the bracket and the Rumble board),
`tny_ui.h` (internal). Test: `tools/headless/tny_ui.txt` (`docs/testing.md` "Tournament screens").

**How a screen runs** (the save pad's switch-screen recipe, decompiled 2026-10-25): stock fade-out (screen object
vtable 0x0802462C, `{vt, 0x100, 0x10}`) -> our screen object through `Modal_Run` (0x0801F2F4: slot 1 open, 2 close,
3 update, 4 vblank, 5 "done?") -> `0x08005F1A(world 0x03001090, 1)` puts the map's tiles, maps, palettes and display
registers back -> stock fade-in (0x08024334, `{vt, 0, 0x10}`). Our object carries a relative vtable built at run
time in its own block. `EntityList_Begin` / `End` (0x08006F4A / 0x08006FA4) set the map's entities **and the OBJ VRAM
allocator** (it lives in the entity list at +0x694) aside and restore them, broadcasting message 9 so every entity
re-uploads its frames: nothing the screen allocates can leak (the test checks the largest free block before and
after every screen: equal). Display while a screen is up: BG3 the switch screen's scrolling checker (tiles 1-4,
its slot-4 scroller 0x08000BE4), BG1 a 240 x 160 8bpp "bitmap" (tiles 16-615, map 0xE800) the screens draw panels,
lines and text into, OBJ faces at priority 1, our own OAM entries appended after the entity list's (the 64 x 64
dialogue portrait, "?" marks at priority 0). Palettes: the menu BG palette (0x081D4F50) with ours in 224-239, the
OBJ palette 0x081DA6C8. Text is the dialogue font through the stock canvas (`Canvas_Text` 0x0801F636; the font is
capitals only). Faces are stock `Portrait` entities (0x08007C70, the switch screen's), centred on their idle frame's
box; looks through the entity's colour remap (+0x28, 0x080076EE): 0 full colour, 0x086D0ED0 dim, 0x086D0DD0 grey,
ours all-black for silhouettes. Player 2's HUD panel is re-composed after a screen (its tiles were drawn over).

**Our calls (ui piece):**
- Every screen is full-screen (the map uses all four BG layers and both char blocks, so an overlay would need the
  same restore); the list box is a centred box over the checker. Fades in / out by `BLDY` (16 frames each way).
- **List box** (`tny_ui_choose`): title in gold, up to 7 lines, Up / Down wrap, A answers the index, B -1. Either
  pad drives it.
- **Select** (`tny_ui_select`): tabs Z Fighters / Tourney / Villains / Buu Saga (L / R), eight faces a page (4 x 2;
  Left / Right past the edge turns the page), the right panel: the dialogue portrait (or the standing body when the
  fighter has portrait 0), the name, the tier. Listed: fighters a player can ever pick (`TR_STORY`, `TR_BEAT`,
  `TR_CHALLENGER`) whose record is in the build; pure opponents (Android 19, Cell Jr. ...) are not shown. Pickable =
  `tny_unlocked(id)` and `tny_tier_open(his tier)`; the rest are black silhouettes with "?", the panel then says why
  (the table's `unlock_text`, "Joins with the story" for a story fighter, "Opens with a higher tier"). The cursor
  starts on the player's own party character. Player 1's pad for player 0, player 2's (`coop.input2`) for player 1
  (the cursor frame is blue for player 2). A on a locked fighter buzzes; A on a pickable one flashes and returns 1;
  B returns 0. The same fighter twice (the violet shadow) is the engine's to apply in the match.
- **Bracket** (`tny_ui_bracket`): a two-sided tree (round 1 down both edges with names, winners one column in per
  round, the champion top centre); Quick one page, Full a page per half and a page for the final. Players' names in
  red (player 1) / blue (player 2), the fallen grey. Phase 0 draws the bracket as it stands; phase 1 first animates
  every result since the screen was last shown (the seats and winners last drawn are kept in `.tny.ui`; when the
  seats differ, every result is new): the players' matches first (the loser greys, a beat, the winner slides to the
  next column), then the round's other matches with quick slides, grouped by page. Then the next match
  (`round` / `match`) lights up, both faces slide to the middle and the banner says "GOHAN vs FRIEZA"; a finished
  bracket slides nobody and the banner says "GOHAN wins!". A (or B / Start) skips the animation, then closes the
  screen; it closes itself after 5 s.
- **Rumble board** (`tny_ui_rumble_board`): up to eight faces with names; the fallen grey one after another with a
  red frame; "Standing: N".
- **Unlock board** (CoopCall 28, a=1; the grounds' courtyard object): every `TR_BEAT` / `TR_CHALLENGER` fighter in
  the build, table order, eight a page, a counter "4 / 38" at the top; locked: black with "?", the panel only the
  condition (plus "Lost once: win again" for a challenger with `tny_challenge_tried`); unlocked: colour, portrait,
  name, "Unlocked!". A or B closes it.
- **Pop-up** (`tny_popup_show`): one narration line in the stock dialogue box (the "Journal Updated!" look) with
  the item pop-up's jingle; closed with A. It runs the dialogue screen (0x08023E88) at once: call it from the
  engine's safe frame (`tny_engine_frame`), as the test does from `tny_ui_frame`.
- **CoopCall 28**: a=1 the unlock board, a=2 the bracket (phase b); pushes 1 (the grounds' script ends the talk on
  it). **CoopCall 29**: the select for player a at tier b, pushes the fighter id or -1.
- ui_strings 380-399 and portraits 190-199 stay unused: labels are C strings drawn by the screens.
- The roster's `tny_fighter_ok` read past the sprite index for art not in the build (found here and by grounds;
  fixed on main): the screens also check a record's shape before making a face of it (`ui_sprite_record`).

**For the engine:** call `tny_ui_choose` / `tny_ui_select` / `tny_ui_bracket` / `tny_ui_rumble_board` from a script
step (CoopCall 23-27) or the per-frame tick, not while another modal screen is up (a second call while one runs
returns at once: `select` 0, `choose` -1). `tny_ui_bracket(0)` once when a bracket is drawn up, `(1)` after each
round's results are in (`winner[][]` filled for the whole round, `round` / `match` on the players' next match);
a new bracket (different seats) starts the animation history over. Pop-ups: `tny_popup_show` from
`tny_engine_frame` at the safe frame.

## 13. The engine (built 2026-10-25, branch `tny-engine`)

`src/coop/tny_engine.c` (+ `tny_engine.h`, the hooks other files call) runs every tournament; `tourney_data.c`'s
numbers are the engine's. Our calls (Ryan left them to us) are marked *decided*.

**The flow.** CoopCall 23 (op 0) at the director arms the tournament and pushes 1 (op 1 pushes 1 while one runs).
Everything after that is a state machine: each frame (`tny_engine_frame`, the input poll) watches, and every action
that opens a screen, spawns, warps or shows a line runs as a *step* from that tick, one at a time, only when no scene
runs, player 1 is free and a queued line has run (`say_hold`); the ticks of a step's own screens (their nested input
polls) stay out (`stepping`, checked against the stack depth so a flag left by a reset clears itself). Steps first ran
as entity-list events; with the ui merged, an event queued after a screen's set-aside of the map's entities was lost
(the flow hung), so they moved to the tick, where the ui expects its callers. Lines ("Round 1: ...") are dialogue
events (`coop_run_sequence`). Warps go through the test-warp command (`coop2.cmd` 5), so they happen at the start of
the list update: a map change inside an event fire freed the arena under the event loop (found headless: the game
froze). The director's lists answer B by ending the talk (nothing changed).

1. *The director*: the tier (a list only when more than one is open), the division (1 v 1 / 2 v 2 / Royal Rumble;
   2 v 2 only with two players), Quick / Full for 1 v 1 when the tier's pool has 15 (never Rookie), then
   `tny_ui_select` for player 1 and player 2. B anywhere ends it quietly. A pick that is not pickable (locked, or a
   tournament fighter without a display row) falls back to the player's own character.
2. *The characters* are set once for the whole tournament: a story pick plays from his party slot (player 1 switches
   with the SetActiveCharacter step), a tournament playable's entry (`tny_play_entry`, on the party's best numbers:
   section 21) is
   written into a *carrier* slot: the player's own slot, else the duel clone's slot 7 when nobody is in it (backed
   up in `.tny.eng`, restored on the way out). The same story fighter twice: player 2 gets a copy in a carrier. 16
   after his chapter (flag 421) plays as a copy of his entry (the `a16_join` recipe). Then the warp into the ring:
   the map load builds player 1's body (a body change in place keeps the old OBJ VRAM slot; a 64 x 64 one overflows
   it, the playables piece's finding). Whenever player 1's fighter changes later (a co-op 1 v 1's turns), the ring
   reloads first (`P_RESTAGE`) and the step runs again.
3. *The bracket*: a random draw from the tier's pool (`tny_in_pool` and the fighter's own tier at or below the
   tournament's; no repeats, no story playables, no challengers). Seats are filled unit by unit (a 2 v 2 team is two
   seats), so a short pool leaves whole byes at the end; a bye's opponent advances. Co-op 1 v 1: player 1 in the
   first half, player 2 in the second. **2 v 2** (*decided*): a Quick bracket of 8 seats = 4 teams (seats 2k and
   2k + 1), `rounds` 2, `winner[r][m]` the winning team's first seat; Full is not offered for 2 v 2.
4. *Simulation*: a match without a player resolves off-screen when the players' next match comes up: power p
   (below) per fighter, a team the sum; side A wins with probability pA^2 / (pA^2 + pB^2) (HP x attack scale as p^2;
   *decided*). The engine's own RNG (an LCG seeded from the frame counter, or by test command 21), so the game's
   RNG and lockstep netplay are untouched.
5. *A fight*: both players healed (HP and EP full), placed 40 px left of the ring's centre, the opponent(s) 40 px
   right (`tny_ring.h`), spawned from a RAM enemy record (stat 160 + fighter id, script-spawned: boss rules) through
   `enemy_spawn_hook`, his kit attached (`tny_moves_attach`), a boss bar each in 1 v 1 / 2 v 2 (the second one under
   the first), the announcer's line ("Round 1: Gohan vs. Frieza!", "The final: ..."). Android 16 and Super 13 spawn
   on their own stats (109 / 145) and paths. Co-op 1 v 1: the players take turns, the other sits out (the Cell Games'
   solo modes 1 / 2: player 2's matches are played on player 1's entity as player 2's pick; `coop_solo` holds in the
   ring while the tournament runs); the final between them is a duel (the CoopCall 1 path: `coop2.call_state`, the
   winner's line; the same fighter twice: the duel's shadow). The same fighter twice on the floor (2 v 2, Rumble):
   player 2 is drawn through the duel's dark violet remap (the ui does not tint).
6. *The end of a fight*: all opponents down = won (`fight_frame`). **The 1 HP rule** (`tny_ko_check` from both
   take-damage hooks): a lethal hit leaves the player at 1 HP and he is out of the fight; it holds for the whole stay
   in the ring (a blast still flying after the fight ended killed the player before: found headless). In a fight of
   both players (2 v 2, Rumble) the first lethal hit downs the player through the revive (`revive.c`; the partner may
   revive him; the countdown running out in the ring just leaves him down, no game over); the second player's lethal
   hit loses the fight. 90 frames later: bodies and bars cleared, unlocks, the EXP trickle, the result line, the next
   step.
7. *Royal Rumble*: rounds per tier (section 1; fighters at once, players included), packs drawn from the pool
   without the headliners and without repeats in one Rumble; giants only where the plan puts them (the Z-Fighter's
   last round, the Legend's rounds from the second) and a giant takes two places; the headliner round is the
   headliner(s) (Rookie King Cold, Adept 17 and 18, Z-Fighter 16, Champion Broly, Legend Super Buu; the strongest left
   when his art or story flag is missing) with helpers to three fighters, the headliner spawning last. No bars.
   `tny_rumble_mode(1)` during the rounds. An entrant waits while OBJ VRAM has no block for his body (0x800, 0x1400 for
   a giant) and comes in when one frees; a fallen fighter's body leaves after 60 frames (art-new saw a next fighter
   drawn in pieces while a knocked-out body still held its tiles); with nobody of the fight standing the next one comes
   in regardless. `tny_ui_rumble_board` before each round.
8. *The trophy*: the winner's line, then the prizes (*decided*): the first win of each tournament ever (the global
   record) a +3 capsule whose stat turns with tier and division ((tier + division) % 3: STR / PWR / END, items
   3 / 6 / 9: five of each over the fifteen tournaments), Full a second one ((tier + 1) % 3) the first time, every
   later win a Senzu bean (item 0), the Rumble a second Senzu when no player went down. Items through `item_use_hook`
   (the stock pick-up pop-up).
9. *The hidden challenger*: every still-locked fighter whose `challenger_of` is this tournament and whose art is in
   the build (a pair in the Rookie 2 v 2); Uub only when the Champion Rumble was won as Hercule, Vegito only when the
   Legend 1 v 1 was won as Goku or Vegeta; otherwise the tournament's alternative (section 6, built in the polish: 14). "A challenger appears: Pui Pui!", the fight
   (the winner alone in a co-op 1 v 1; the team otherwise; a lone challenger against two gets the lone-boss HP x1.8):
   won = unlocked ("... joins the tournament fighters!"), lost = the attempt in the record ("Come back when you've
   won again!").
10. *The way out*: entries restored, both players' own slots back, then the warp to where the director talk began.
   Pause and player 2's Select switch are blocked in the ring. A map change away from the ring mid-tournament (a
   reload) drops the tournament; entries are restored only into the running game (never into a freshly loaded party).

**Opponents' numbers** (stats 160-209 = 160 + fighter id; `tny_resolve_stat` from `bosses.c`'s `resolve_stat`, a
`BOSS_NO_TWIN | BOSS_FLAT` rule from `boss_rule`; *decided*, to tune in play): a base per *tournament* tier (Rookie
1400 HP, melee / ranged / defence 12 / 12 / 10; Adept 2800, 22 / 20 / 18; Z-Fighter 4200, 33 / 33 / 34; Champion 5800,
47 / 50 / 52; Legend 8000, 58 / 62 / 58), times the fighter's power (75-140: HP x p %, attack and defence x
(100 + p) / 200 %), the boss class of stat 29 (Cell Jr his own), no kill EXP; then the difficulty's boss HP and
damage as for every boss. AI set: the one that fits the body (21 most, 98 Tien / Hercule, 129 Tao, 29 the androids,
80 / 116 / 148 / 138 the Cells), else the first of 21, 98, 129, 11 whose pose groups the body has. A body without the
regular groups (Mecha Frieza's cutscene sprite 49) uses the fighter's play body, else he is not in the pool. **No
twins** in tournament fights (the rule's `BOSS_NO_TWIN`; `BOSS_FLAT` skips the lone-boss x1.8, which the engine
applies itself to a lone challenger).

**EXP** (*decided*): a won fight pays each fighting player 1/16 of his level's span (to the player's own slot when he
fought as a tournament playable); a Quick run nudges about a fifth of a level.

**The global record** on the save chip: 64 bytes at EEPROM offset 0x1E00 (clear of the three save slots at any
`PARTY_SLOTS`; read from the game's 8 KiB RAM mirror, which boot fills from the chip; written through `Eeprom_Write`),
magic "NTY1", version 1, a checksum: unlocked bits per fighter id, wins per tournament (tier x 3 + division), Full
wins per tier, challenger attempts, prizes paid. Shared by the three save files and kept by a deleted file.
*Decided*: the first-win capsules follow this global record too (a second save file gets Senzu beans for tournaments
already won on another).

**Unlocks**: a `TR_BEAT` fighter is unlocked when he falls in a fight the players won and the last hit on him was a
player's (the damage calculation's note, `tny_note_hit`; the moves piece's `tny_moves_last_hitter` when he has a
kit); a challenger by beating him. "New tournament fighter!" queues.

**Pop-ups**: `tny_notify` queues up to four lines; shown through the ui's `tny_popup_show` from the frame tick when no
scene runs, player 1 is free and no step is pending (idle, or between fights); without the ui a narration line in a
step. The story watch (every 16 frames): tiers open, story playables in the party, tiers whose pool reached 15 ("New
division: Full bracket!"). *Decided*: no notices before the first visit to Papaya Island (the grounds' flag 441; a
party join made the "New tournament fighter!" line eat the button presses of `a16_moves.txt`), none after a load (the
first look after 300 frames without player 1 is a silent baseline), none while a test forces the pool.

**Tiers** (confirmed against the scripts): Adept 85 (the 19 + 13 fight's defeat sequence), Z-Fighter 136
(`ch5_gingertown_end`), Champion 191, Legend 382; pool flags 127 (17 / 18, the road), 146 (16, the island), 301
(Cooler and his Namekian: Cooler beaten, zone 6 area 61's defeat script), 402 (Broly, Paragus), 421 (16 playable).
Mirror rule: Hercule leaves the pool once he is in the party, 16 once he is the players'.

**For the other pieces**: the ui reads `tny_bracket()` (live while a tournament runs) and gets `tny_ui_bracket(0)`
once per draw, `(1)` before each later match and at the end; the grounds' director needs nothing more. RAM:
`.tny.eng` 0x21C of its 0x240 (since the polish). The walk-in, the end poses and the announcer in the ring came with the
polish (section 14); the Rumble's NPC-vs-NPC fighting is the moves piece's.

## 14. The polish (built 2026-10-25, branch `tny-polish`)

The engine's open items: `src/coop/tny_engine.c` (presentation, alternatives, 16 / Super 13), `src/coop/super13.c` (his
end in the ring), `src/coop/tny_moves.c` (`tny_moves_final_form`), `src/coop/tny_ui.c` / `tny_ui_grid.c` /
`tny_ui_bracket.c` (the giants' faces). Test `tools/headless/tny_polish.txt` (`docs/testing.md` "Tournament polish").

**The ring presentation.** Before a bracket match or a challenge the players stand on the left of the platform
(`TNY_RING_P1` / `P2`; in a co-op fight both), the opponents on the right (`TNY_RING_OPP`), the announcer (sprite 230,
an actor facing down at the ring's top edge, (360, 308); the stock townsman 99 when the art is missing) appears, and
everyone walks 40 px to his fighting mark at the same time (the stock script commands: `WalkToPosition`'s command
object queued on each entity and run by `CommitCommandBatch`'s modal, about 40 frames; nothing else moves meanwhile).
Then the announcer calls it with his portrait (speaker 140; 0 = narration without the art): "ANNOUNCER: Round 1:
GOHAN versus FRIEZA!  Begin!" ("The final: ...", "Royal Rumble, round 2!  Last ones standing win!", "And now, the
headliner: BROLY!  Begin!", "PUI PUI challenges you!  Begin!"). A closes the line and he leaves before the first blow;
the boss bars come then. A Rumble round has no walk: the players stand on their marks, he calls the round, leaves, and
only then does the pack come in (he never shares OBJ VRAM with a fight). The co-op final (the duel): he calls it and
leaves when the duel starts. At the end (90 frames after the last blow) the loser lies down in a side facing (the lying table `tny_lie`, else his
lying group 12, else the knock-down 3, else the hit-stun: section 20; Super 13 kneels in his power-up crouch, group 23,
because his 12 is the death light), the winner shows his victory pose facing the crowd (the shout group 4's last frame, else the fidget 14, else the
ki pose 5), the announcer comes back and calls it ("The winner...  GOHAN!", "FRIEZA wins!  GOHAN is out of the
tournament!", "Round cleared!  GOHAN stands tall!", "GOHAN defeats PUI PUI!"); the poses hold while the line shows,
then the bracket screen. The trophy: the players' pose and "Ladies and gentlemen...  GOHAN wins the Rookie
tournament!"; the challenger: "Wait...  a challenger appears: PUI PUI!". About a second of walking and one line each
way.

*Decided / found:*
- **The world is frozen but drawn while a line shows** (decompiled: the dialogue modal's update 0x08004EA8 draws the
  entity list, 0x08006AD8, without updating it), so a pose written into an entity's frame fields (+0x4C the group's
  four facing arrays, +0x14 the frame) holds for the whole line; the engine rewrites it every frame and, outside the
  line, zeroes both pads (never during it: A closes the line).
- **The walk is the stock command system** (decompiled 2026-10-25): `WalkToPosition` (step 53, 0x08009D4A) builds a
  command with 0x08026620(0, {x, y}) and appends it to the entity's command list (+0x150) through 0x0800E04C; outside a
  batch that call runs the command modal at once; `BeginCommandBatch` / `CommitCommandBatch` set / clear the batch word
  0x030020F8, and the commit runs 0x0800DE20's modal (message 0xB starts every queue, message 10 steps them each frame,
  entities without commands are only animated and drawn) until all are done. The engine queues one walk per fighter and
  commits once: they walk together. The walk ends in the idle pose; the facings are set after.
- **Lines with a portrait start with the name** ("ANNOUNCER: ..."), as the stock lines do; names are in capitals.
- **No camera move:** the camera stays on player 1; a body knocked far by the last blow can lie at the view's edge.

**The hidden fight's alternatives**: section 6 (the rule). Super Buu's final form is his kit's two transforms applied
at the spawn (`tny_moves_final_form`: sprite 207, both attack and defence boosts, no transform left to fire). The
roster's unlock lines for Majin Vegeta, Goku (Buu saga) and Super Buu name both ways.

**Android 16 and Super 13 as opponents.** Both spawn on their own paths (stats 109 / 145: `a16_spawn` / `s13_spawn`,
their own AI and specials) and the engine then replaces their story numbers with the tournament's (the stat copy's HP,
attack and defence from `tny_resolve_stat` at the running tier, the difficulty's boss HP on top; 16's island slicing
keeps the same total). Their ends: 16's stage-over rule already left him standing with no scene (the record's action is
none); Super 13's explosion (the light, a dozen blasts, the shake, the empty pose) does not run while
`tny_ring_active()`: he kneels and stays (`s13.state` 3). Both then take the end's poses. 16 headlines the Z-Fighter
Rumble (with the pool's story flag 146 and before he is the players', flag 421), Super 13 is in the Legend pool;
test command 21's `arg2` 0x7E0000NN forces either into a test's first match.

**The giants' faces.** The select grid and the unlock board (cells 34 x 56) show a giant (an idle frame wider than 32 or
taller than 48 px: the 64 x 64 bodies, Kid Buu, Pui Pui) as a 32 x 32 crop: his idle frame decoded (`UiGfx_Decompress`:
8bpp OBJ tiles in 1D order), the four middle tile columns from the first tile row with a visible pixel (head and
shoulders), the silhouette remap applied for a locked one, drawn as our own OBJ. The bracket and the Rumble board, whose
faces slide, draw a giant at half size instead: his face's 64 x 64 OAM entry becomes an affine sprite on matrix 31
(1/2 scale, no double size: the same centre). The select's big panel (a fighter without a portrait) keeps the full body.

**RAM**: `.tny.eng` 0x21C (the presentation's fields fill the old line's 76 bytes; the line moved to the end, 100
characters; the state's magic is now 0x7E6E0003). The record's `extra` word (was padding): bit 0 the final form beaten.

## 15. The tournament runs as scenes (Ryan, 2026-10-25)

"We shouldn't have access to the save pad in between tournament rounds": from the director's last choice to the
trophy (or the players' elimination) the players stay in the ring (zone 15 area 16, which has no save pad); the
waiting room and its pad come back only when the tournament is over. In the ring Start and Select are blocked, and
whenever no fight is running (the walk-in, the announcer's lines, the poses, the bracket's turn, the pause after a
knock-out) the d-pad, B, L and R are masked on both pads too (`tny_engine.c`, the input lock beside `t_nofight`);
A stays free to close the lines. Matches without a player are never played: the bracket screen's animation shows
their winners.

## 16. The grounds' people, Ryan's first look (2026-10-25)

- The street: Master Roshi (his NPC sprite 88, portrait 80) beside the food stall with a short piece of advice
  (`tny_roshi_talk`); a vendor behind each stall's counter (gifts 99, food 80, drinks 101); the ZZTV reporter in a suit
  (sprite 16, the suited man at the Cell Games) with his cameraman (sprite 141, red cap, the camera on his shoulder)
  beside their news van, our own map art (`tools/vanart.py`, 64 x 48, its footprint blocked).
- The plaza's fan who looked like a woman but spoke as "KID" is a woman fan now; the infirmary's nurse is a woman
  (sprite 154) instead of the dog-man (89).
- The waiting room's crowd (`tny_grounds.c`), revised after Ryan's second look ("it was more accurate before"): up to
  seven places (one more spot; eight hung the engine test's Champion Rumble after a waiting-room visit, cause not
  found, so seven it is), every member spawned only while his body leaves 8 KB of OBJ VRAM free, a random draw of today's competition: this save's pool minus the fighters a player
  has unlocked and minus Roshi (he stays on the street). The **courtyard** holds the unlocked ones: up to four
  places of beaten fighters and won hidden challengers (each with his greeting), a different few each visit
  (CoopCall 30 op 4, the courtyard's on-load). The newest-tier-first rule of the first pass is gone.
- Sprite 50 is LoG2's own General Tao (the cyborg: the helmet, the red armour; Goku's hidden bonus fight, map 46),
  so the roster calls him "General Tao" (was "Mercenary Tao").
- The infirmary's first bed holds an injured man (our art, `tools/patientart.py`, on BG0 with priority 1); A at the
  bed's foot gives player 1's own line, "<NAME>: Aww man.  This guy's in bad shape." (CoopCall 30 op 3 pushes
  player 1's name in capitals). Tests: `tools/headless/tny_npcs.txt` (the street, the stalls, Roshi's line, the nurse),
  `tools/headless/tny_crowd.txt` (the room and courtyard before and after unlocks, the patient).

## 17. Papaya Island's easter eggs (2026-10-25, Claude's picks after Ryan's "pick some stuff and build it out")

- **Yamcha in the infirmary's last bed**: an actor on his playable body (sprite 235) whose lying frame (group 12) is
  written back every frame while the infirmary is loaded (`tny_grounds_frame`, the engine's pose-holding method; a
  `SetEntityAnimation` from the on-load waited forever for a looping pose and hid the player). A at the bed's foot:
  "YAMCHA: ...I'm fine.  I'm fine!  Just resting my eyes.  Don't tell Bulma."
- **The hidden arena** (zone 15 area 17, `data/maps/papaya_secret.json`): Buu's Fury's Other World Stadium (its zone 1
  area 20) imported whole, only the ring floor walkable. A at a crack behind the courtyard's right flowerbed
  (`tny_secret_in`) squeezes the players through; Pikkon (sprite 200, portrait 147) trains alone on the ring: a hint
  about the Adept tournament (where he is the 1v1 challenger) and one Senzu bean (flag 442, CoopCall 30 op 5 gives
  the item with its pop-up); the ring's bottom edge leads back to the crack. Place name 359 "Hidden Arena".
- CoopCall 30 (grounds) gained op 3 (player 1's name in capitals), op 4 (the courtyard's unlocked crowd), op 5 (an
  item with its pop-up), op 6 (every entity on a sprite plays a group). Test: `tools/headless/tny_eggs.txt`.

## 18. The plaza's flight spot is visible (2026-10-25)

Ryan: "there's no visible indicator ... to go and fly on the world map". LoG2 marks its towns' flight spots with a
light-blue cloud painted into the ground (West City: zone 4 area 3, around (352, 1240), in BG3); the plaza now has
the same cloud (a stamp of those 6 x 4 tiles into the plaza's BG2, which draws over its BG3 paving) on the paving just
above the beach, and the flight trigger sits on it ((216, 344)-(264, 376)). The world-map landing still arrives on the
beach below. (World-map landmarks, the 3D towns seen while flying, are drawn per site by the on-load's
FindCharacterEntity 2-12; Papaya Island has none: adding one needs that landmark table decoded.)

## 19. Overhead HP bars in the ring (2026-10-25)

Ryan: "should we display the hp of the other fighters during the royal rumble?" `src/coop/tny_hpbars.c`, drawn from
the HUD's draw hook next to the revive bars: while `tny_ring_active()` and no scene runs, every kit fighter standing
in the ring (`tny_moves_base(e)`, HP > 0) that has no boss bar bound to it gets a 16 x 8 sprite 36 px above its
origin: a black frame round a 14 x 2 fill, green, yellow under half, red under a quarter. So a Royal Rumble shows one
over every fighter, a 2 v 2 partner of the players gets one, and 1 v 1 / the 2 v 2 opponents keep only their boss bars.
A fighter at 0 HP loses its bar. 4bpp in palette bank 0 (the game's one OBJ palette: 1 black, 4 green, 5 red,
8 yellow, 14 dark grey), 64 bytes a fighter, eight at most (0x200 bytes of OBJ VRAM, taken while bars show and freed
when none do and when the HUD goes). State: `.tny.hp` (8 bytes). Test: `tools/headless/tny_hpbars.txt`.

## 20. Ryan's playtest: the art fixes (2026-10-25, branch `tny-artfix`)

- **"chichi's head has been cut off and shifted to the right"**: her NPC head (sprite 33) sat 1-1.5 px forward of 18's
  body and its front was cut by the 16 px side frames. The head is placed over the body as on her own sprite and frames
  it would stick out of are widened; her tabard covers her side views and her victory pose too. Roshi had the same
  anchor fault (corrected) and a head floating on back-view kicks (settled); Videl, Chiaotzu and King Cold checked
  (King Cold lost a floating tail tip). `docs/sprites.md` "Lying poses, Chi-Chi's head, Spopovich".
- **"the fainting/laying down at the end, you used their flying sprites instead"**: every body now has a real lying
  pose where the engine looks (`tny_lie`, group 12): the Buu imports' lie-down frames or a turned recoil, King Cold,
  Cooler, Frieza, Mecha Frieza, Tao and Cell Jr. turned hit-stuns, the stock playables and the Cells their knock-down's
  last frame. The engine lays the loser in a side facing. Sheet: `tools/tnysheet.py --lying` (`docs/testing.md`
  "Tournament bodies").
- **"spopavich is great, he just has a weird animation"**: his partial set is remapped (full walk cycle, a side-view
  grappler's crouch for the charge and the bear hug, the grab as an attack, his flex for the victory pose, his run for
  the dash; every import's hit-stun is its hurt animation now, not its run). Yamu, Pui Pui and Kibito got the dash /
  hit-stun parts of the same fix.


## 21. Fight fixes from Ryan's playtest (2026-10-25, branch `tny-fightfix`)

Code: `src/coop/tny_moves.c` (knock-back, the fall, Energy Steal for opponents), `src/coop/tny_play.c` (the players'
Energy Steal, the two kits), `src/coop/tny_engine.c` (`lead_entry`), `data/hercule.json` (ability 24). Test
`tools/headless/tny_fightfix.txt` (`docs/testing.md` "Tournament fight fixes").

The fallen lie in the engine's lying pose (`tny_lie_pose()`, tny_engine.c: the lying table `tny_lie` first, then
group 12, 3, 2), in a side facing, from the end of the knock-down to the body's removal, so the pose at the
announcer's line and the one before and after it are the same (Ryan, 2026-10-25: "it quickly turns to the flying
animation again" once the line released the bodies whose group 12 is not a lying pose).

**Knock-back** (Ryan: "im punching yamcha and hes unable to move, but its not actually knocking his player model back.
making it easy for me to corner and keep punching him"). Decompiled: a player's punch (`Player_Attack` 0x08007DE8: hit
message type 1, attack descriptor 2 of the table at 0x086DB2CC, the same for every punch of the A combo) reaches the
enemy message 0x0800E5B8, which runs the damage calculation and hands the hit to the AI's hit tree (0x0801AB2A: the
top AI set's second word). Set 21's tree (every tournament opponent, the dojo Yamcha stat 106 and many bosses) is
"go to set 21" plus set 18's: OnHit, then 0x0801ACD4, which tests the space behind him and pushes AI set 17 (0x0801A946,
a slide of 0x20 away from the hitter while his hit state lasts) or, against a wall, set 16 (0x0801ACB6: the other way,
unhittable). Regular enemies (set 3) push sets 1 / 2 the same way with smaller numbers (wolves: about 2 px a punch). For
a set-21 fighter the slide is 7-12 px on the first punch; then he is held in place about 35 frames and every further
punch lands on a man who does not move. Measured (a tournament Yamcha, player 1 following him up to 16 px before each
punch, test command 27 op 6): before, 276.0 -> 286.0 -> 288.3 px, then 288.3 through the next five punches until he
fell; now, per combo of three punches: 276.0 -> 286.0 -> 296.0 -> 340.3 (the finisher) -> 365.3 settled, the next combo
365.3 -> 373.4 -> 388.3 -> 431.3 -> 452.5. *Decided:* the kit adds its own push on top of the stock reaction: nothing
more on a combo's first punch (the stock slide), 3 px on the second, 40 px over 12 frames on the third melee hit (each
within 45 frames of the last), 3 px for a shot; along the hitter's facing for a blow, away from the shooter for a shot;
a wall behind him turns the push along the wall (out of the corner), boxed in it stops. KF_POISE bodies are pushed
half as far; a special in progress and the poise window still only take damage (no push).

**The fall** (the main session found tournament fighters at HP 0 kept their AI and fought on: zombies in the Rumble;
Ryan: "do people actually die in the royal rumble?"). The kit's update ran the stock update at HP 0 for everyone; the
opponents' record has no action and the boss class (stat 29's) never removes the body. Now a fighter (not Android 16 /
Super 13, whose own code ends them) at HP 0 falls: his special ends, the killing blow pushes him 24 px, his knock-down
(group 3) plays over 18 frames, then the engine's lying pose (group 12's last frame, else group 3's, else the hit-stun:
`pose_hold`'s table), held; no AI, no hits taken (the message returns 0 and the stock "hittable" word +0x168 is
cleared), never a target (`alive_kit`). The engine counts him out at once (`fight_frame`: HP 0) and removes his body
after 60 frames in a Rumble; a fallen helper or Multi-Form copy (not the engine's) leaves after 90 frames. Tested: a
Rookie Rumble with every fighter at 4 HP is finished by player punches and the fighters' own hits, round by round to
"The headliner is down!". Found on the way (`tny_rumble.txt`, a new timing): a thin beam still playing its end after
its fighter left kept his kit's vtable copy (freed with the kit) and the next frame jumped to garbage; the kit's
destroy now hands such a beam the stock beam vtable.

**Energy Steal, opponents** (Ryan: "i like that he drains the other players ki, but maybe he should do it more
strategically? like when he is at half health he does it and all the ki he takes goes into his health?"). Of Babidi's
pair only Yamu drained (a passive: every melee hit took 12 ki); Spopovich had only the bear hug, so the drainer Ryan
saw was Yamu (his later message calls him "yamu (spopavich)"). *Decided:* the rule for both. Above half HP neither
drains; from half HP down a new rush (`STEAL`: Spopovich a 72 px dash, Yamu 80 px, one grab, cooldown 360 / 300 frames)
is picked before his other moves whenever it is ready; each player it grabs loses 25 ki (all he has, if less) and every
point becomes 4 permille of the thief's max HP (25 ki = 10 %, both players at once 20 %), capped at his max. Back above
half he stops: not endless. Shown by the heal's light on him (Kibito's flash) and a spark at the victim. Measured:
Spopovich (1330 HP) at 40 % = 532 grabbed both players: 798 HP, both 150 -> 125 ki; at full HP for 600 frames: no
steal. Yamu (1190) at 40 % = 476: 714.

**Energy Steal, players (ability 24)** (Ryan: "yamu (spopavich) isnt fun once you unlock and actually play as him.
he's weak (like he got a default level 1), and he doesnt have ki draining ability when user plays as him"). A short
dash (24 px, walls stop it), the punch and a grab in front of him (melee, power 0x500); when it lands he gets 20 % of
his max ki back, and Spopovich at half HP or lower also 8 % of his max HP. No ki cost (it only pays when it lands); the
stock ability gate paces it. HUD icon: the stock pair 6. Kits: Spopovich Rolling Hercule Punch, Megaton Punch, Energy
Steal, Shockwave Punch (HP 140 %, ki 80, STR 125, POW 75, END 135: the slow brute; Taunt dropped, the steal refills
ki); Yamu Afterimage Rush, Energy Steal, Ki Blast, Shockwave Punch (100 / 110 / 105 / 105 / 95: the quicker thief, the
dash first). Their walking speed is the standard class's (rows 29+ share Gohan's), as every playable's. Measured:
player 1 as Spopovich at 400 / 1000 HP, 20 / 200 ki: a grab on Yamcha -> 480 HP, 60 ki; above half HP the ki only; a
miss nothing. Player 2 as Yamu, 30 / 150 ki: 60 ki, HP unchanged.

**Why they played weak: the lead.** The engine built a playable from player 1's own slot (`apply_entry`). A weak or
low-level player-1 character gave a weak playable (Hercule's HP is the party's lowest: 493 against Goku's 1216 in the
Map Test party), and player 2's pick read player 1's slot after it already held player 1's pick (Spopovich's ki was
80 % of Yamu's 90 % of the lead's). *Decided:* `lead_entry` takes the party's best, number by number: the highest EXP
(his level), max HP, max ki, strength, power and endurance among the story fighters in the party (slots 0-6), each
read as it was before the tournament (a carrier's backup); the fighter's percentages apply to that. Android 16's
tournament copy uses the same lead. Measured (co-op, Map Test party): Yamu 1216 HP (Goku's), 178 ki (110 % of
Krillin's 162), STR 105 % of Hercule's; Spopovich 1702 HP (140 %), no longer built on Yamu.

**Seen, not fixed (fixed in section 23):** in a co-op 1 v 1 with Yamu as player 1, his body draws as scrambled tiles at
the fight's start. Spopovich's and Yamu's bodies look alike (both bald and big; section 22).


## 22. Sprite leftovers (polish2-art, 2026-10-25, branch `tny-polish2-art`)

Art only (generators and `data/tny_buu.txt`; no engine code). Details and evidence: `docs/sprites.md` "Sprite leftovers".
- **Lying:** Yakon, Broly and Janemba lay as a turned recoil that read as a body sprawled seen from above; now Broly and
  Janemba turn a straighter recoil (on the side / on the back) and Yakon, who has front views only, a turned recoil
  with his arms and legs drawn in toward the body.
- **Knock-down:** King Cold (185) and Cooler's player body (244) fell through Cooler's feet-first death frames (groups 3,
  20); now two frames of the hit-stun's recoil, then their lying frame. The same for the imports whose anim 17 is a
  death glow or a regrowth (Broly 159, Janemba 208, Kid Buu 204) and for Yakon, Pui Pui and Evil Buu (who stood in
  their hurt pose). The stock Cooler opponent (34) has no group 3 and still goes straight to the lying table.
- **Videl's hair, Roshi's fists:** fixed in `tnysprites.py` (a head cut by a 32 px frame now moves inside it; small
  pieces left beside Roshi's head go).
- **Victory pose:** Fat Buu, Kid Buu and the three Super Buus no longer cheer under the steam cloud of their shout.
- **Yamu:** the import was the right record (293; Buu's Fury has no other Yamu: every record checked); he keeps his
  map and gets a painted black hair tuft, so he no longer reads as a second Spopovich.

## 23. Polish pass 2: the tournament as scenes (2026-10-25, branch `tny-polish2`)

Ryan: "add a pass to polish and clean all our animations, transitions, and cutscenes". Every flow was filmed frame
by frame (`tools/headless/tny_flow.py`, `docs/testing.md` "Tournament flow films": 1 v 1, Rumble, 2 v 2, co-op 1 v 1
with the duel final), judged by eye, fixed, filmed again. Code: `src/coop/tny_engine.c` (the curtain, the steps),
`tny_ui.c` (`ui_run` honours the curtain), `tny_ui_bracket.c`, `tny_hpbars.c`, `tools/buu2log2.py`.

**The curtain.** Every change of the ring's scene now happens behind black: the stock fades the ui's screens use
(0x0802462C fades the map's palettes to black, 0x08024334 fades them back in, 16 frames each, modal) close and open it;
while closed, the engine writes the blend darkening (`BLDCNT` 0x00FF, `BLDY` 16) every frame, because a map load or a
screen brings the palettes back; opening first blackens the palettes under the darkening, clears it, then fades in.
`tny_curtain()` (tourney.h) tells the ui: a screen opened behind it neither fades the map out nor back in. The flow:
- *The director:* the waiting room fades out once; the tier / division / size lists and the select screens follow each
  other through black; the warp into the ring happens in the dark (B anywhere: the waiting room fades back in).
  Before: a frame of the waiting room's tiles scrambled by the map load, and a hard cut to the ring.
- *Before a fight:* the bracket (or the Rumble board) opens from black and closes to black; the players are placed on
  their side, the opponents spawned off the right edge, the announcer put in view, 8 dark frames (new bodies decode, the
  camera settles), then the fade in, the walk-in, the call. Before: the bracket faded back into the old ring (the
  announcer of the last line still standing, the player alone), then the opponent popped in and the player snapped
  40 px to his mark.
- *After a fight:* the boss bars slide up and away when the fight is decided (before: an empty bar sat over the end's
  poses and vanished after it); the loser keeps lying and the winner posing through the announcer's line and the
  pop-ups ("New tournament fighter!"), then the curtain closes on them and they leave in the dark. Before: the
  fallen body and the bar popped off the ring with the line, the announcer stayed for the pop-ups. A cleared Rumble
  round keeps its fallen too (unless the announcer needs their OBJ VRAM).
- *The Rumble:* the pack walks in from the right after the announcer leaves (before: popped in on their marks); a
  fallen fighter blinks out over his last 24 frames before his body leaves at 60 (an all-transparent remap at entity
  +0x24 every 4 frames, the field the duel's shadow uses; before: he vanished).
- *The co-op 1 v 1:* a turn change's ring reload happens in the dark (before: a hard cut between two bodies, then 90
  frames of the new one standing alone); the duel final stages player 1 left and player 2 right facing each other,
  opens the curtain, calls it, and only then starts the duel (before: both stood one above the other and the duel was
  live during the call).
- *The trophy:* behind the curtain the winner(s) stand in the middle of the ring facing the crowd (a co-op 1 v 1 winner
  alone, through a reload when he is player 2), then the curtain opens on the pose and the line. *The challenger:* the
  curtain closes after "a challenger appears", opens on the players at their mark and the challenger walking in.
- *The way out:* the curtain closes on the last line, the entries are restored and the warp happens in the dark; the
  waiting room fades in (before: a hard cut, and a body change visible in the ring).

**Smaller fixes:**
- *The announcer in view:* he stands where the camera shows him whole (`ann_fit`: 60 px below the view's top, between
  the ring's top edge and 20 px above the players' row); in a co-op floor or the duel only his feet showed.
- *Input:* A is masked too while no fight runs and no line shows, so a player no longer punches through the 90 frames
  after the knock-out or at a posed body (A still closes every line: it is free while a scene runs).
- *Overhead HP bars* (section 19) only while a fight runs (`tny_fight_on()`): they showed over a walking-in opponent and
  over the 1 v 1 opponent for the frames before his boss bar bound.
- *The bracket screen* no longer blanks for a frame when it draws the next match's boxes on the page it shows (it
  cleared and redrew the whole bitmap; now it draws the boxes, lines and names in place; a Full bracket's page turn
  still rebuilds).
- *Yamu scrambled (section 21's "seen, not fixed"):* his side-facing idle (group 0 facings 2 / 3, and group 17) was a
  single Buu's Fury part copied with its own OAM attributes, 4bpp, while every converted blob is 8bpp; the body drew as
  4bpp garbage wherever he stood facing sideways (co-op or not; it showed at the fight's start). `buu2log2.py` sets the
  8bpp bit on every converted frame; a scan of sprites 150-249 finds no 4bpp descriptor left (Kid Buu's victory pose,
  group 4, had the same fault).

*Found on the way:* the end's poses (rewritten every frame) must let the players go before the fight's EXP: a level-up
plays the player's own animation and waits for it to end, so a held pose hung the game in a co-op Rumble's challenge
(`tny_engine.txt`); the players' poses end there (the fallen keep lying), the rest at the curtain.

**The dialogue seat** (Ryan approved planning pre-fight dialogue; written separately). `int tny_talk_hook(const
TTalkSeat *seat)` (tourney.h, weak default in `tourney_hooks.c` returning 0 = nothing shown) is called twice per
fight with the scene staged and lit, from `tny_engine.c`:
- `TALK_BEFORE`: `step_intro` right after the walk-in and before the announcer's call ("Round 1: ... Begin!"); the
  duel final's `step_duel` after the curtain opens, before the call.
- `TALK_AFTER`: `step_match_end` after the end poses and the announcer's result line, before the results (unlocks,
  EXP) and before the curtain closes; the duel final after its winner line.
It receives `when`, the division and tier, `kind` (`TALK_MATCH` a bracket match, `TALK_RUMBLE` a Rumble round: the
announcer only, `TALK_CHALLENGE` the hidden fight, `TALK_FINAL` the co-op final), the players' fighters on the floor
(`mine`, TF ids, 1 or 2), the other side (`opp`, up to 6; the final: player 2's pick), the round (bracket or Rumble,
0-based) with `last` for the final / the headliner round, and after the fight the `result` (1 won, 2 lost; the final:
the winning player). A hook that starts a dialogue sequence returns 1: the engine's next step waits until no scene
runs, so the call (or the results) follow the dialogue. The poses hold through an after-line. No RAM is taken.

**RAM:** `.tny.eng` keeps its size: one byte of its padding is the curtain (`dark`).

## 24. Open points and the plan to fix them (2026-10-25; next session starts here)

Ryan is play-testing this build and will come back with notes (late October / early November). **Next session: read
his notes first, fix what he found, then work down this list in order** (each item: what is wrong, how to fix it, how
to test it, a rough size). Nothing below is a regression; all of it is tested headless except where it says so.
Build into a private dir, add each fix's test to `tools/headless/regfull.sh`, run it before handing Ryan a build.

### 24.1 The Rumble announcer's calls during a round (dialogue plan section 8)

- **Wrong:** "[Name] is down!", "Two fighters down!", "[Name] is down! Can the partner hold on?" and Super Buu's form
  change aren't called. The game's dialogue box (`say_go`, a dialogue sequence) stops the fight while it shows.
- **Fix:** a **caption** that doesn't pause: one line of text drawn into OBJ tiles at the bottom of the screen for about
  90 frames, then gone.
  - The font is already in hand: `tny_ui.c` renders text with the dialogue font through the stock canvas
    (`ui_text_width`, CV_W 240 x CV_H 16).
  - Copy those pixels into a 240 x 16 OBJ strip, drawn from the HUD hook like the HP bars (`tny_hpbars.c`: tiles
    allocated while shown, freed when gone).
  - New file `src/coop/tny_caption.c`: `tny_caption(const char *line)` queues up to 3 lines, each shown in turn, never
    over a scene. RAM about 0x10 in `.tny`; budget is about 0x50 left of 0x400.
- **Triggers:**
  - **A fighter falls:** `tny_moves.c` `fallen()` first frame -> a weak `tny_on_fall(id)` -> the engine picks
    "[Name] is down!" / "hits the floor!" / "That's it for [Name]!" / "Down goes [Name]!" (frame-counter pick, never
    `rnd()`: it shifts the seeded draws the tests rely on).
  - **Two falls within 60 frames:** "What a flurry!  Two fighters down!" instead.
  - **A player goes down in a co-op Rumble:** `revive.c` `revive_hit` while `tny_ring_active()` -> "[Name] is down!
    Can the partner hold on?".
  - **Super Buu changes form:** the MK_TRANSFORM move firing on his kit (`tny_moves.c`) -> "He's changing!  Look at
    that power!" / "Not again!  Another form!".
- **Test:** `tny_fightfix.txt` part C already finishes a Rumble by real hits; a new `tny_caption.txt` films it
  (captions show, the fight never pauses: player x keeps changing under a caption). Size: half a day.

### 24.2 Dialogue portraits for Android 13, Cell Jr., the Namekian Warrior and Evil Buu

- **Wrong:** their lines show the name with no face (roster portrait 0).
- **Fix:** four new portraits, ids **169-172** (art-buu's unused range 169-189; record them in `docs/ids.md`).
  - Evil Buu: look for his own portrait in Buu's Fury first (art-buu's portrait import, `tools/buu2log2.py` /
    `tny_portraits_data`).
  - The other three: a 64 x 64 portrait made from the sprite's face (the idle facing-down frame's head, cropped 32 x 32,
    scaled x2, on the stock portrait background), the way `tools/oldherculeportrait.py` builds one.
  - Set the roster's portrait field in `src/coop/tourney_data.c`.
- **Test:** `tny_talk.txt` with each of the four opening (test command 31); the select screen still crops its own faces.
  Size: a few hours.

### 24.3 Yakon's lying pose

- **Wrong:** it reads like a crab seen from above (Buu's Fury draws him from the front only; `tools/lying.py` turns a
  front recoil frame).
- **Fix:** try two candidates on `tools/tnysheet.py --lying` and keep the better:
  - (a) A face-down sprawl: the front frame squashed to about 45 % height, the arms spread, a long shadow; LoG2's camera
    looks down at an angle, so a body flat on the floor is short and wide.
  - (b) A hand-painted 32 x 32 lying frame stored as a small pixel table in `tools/lying.py`.
- **Test:** the lying sheet plus `tny_polish.txt`'s end poses. Size: an hour or two.

### 24.4 The announcer walks off and on

- **Wrong:** he vanishes as a fight starts and appears at its end.
- **Fix:** in `tny_engine.c`, after "Begin!", queue a walk command (`Cmd_Walk`, as `walk_in` does) from his mark to the
  ring's edge (40 px past the stands' side), then remove him when he arrives. The fight runs meanwhile; he's an
  actor, not a target.
  - At the end, place him at the edge in the dark (the curtain) and walk him to his mark before the result line.
  - The trophy keeps its own placement (section 23).
- **Test:** `tools/headless/tny_flow.py` films (`docs/testing.md` "Tournament flow films"): no pop at Begin!, he walks.
  Size: a few hours.

### 24.5 The co-op turn change's black screen (about 1.6 s)

- **Wrong:** in a co-op 1 v 1, swapping who fights reloads the ring behind the curtain and holds 90 frames of black.
- **Fix:** find the fixed 90-frame wait after the reload (`P_RESTAGE` in `tny_engine.c` and the curtain's open
  condition, section 23). Replace it with a readiness check: the ring arrived, player 1 controllable, the bodies
  placed, OBJ VRAM settled; then a minimum of about 20 frames.
- **Test:** `tny_flow.py --p2 --div 0` (the turns); `tny_engine.txt`'s co-op parts and `duel_end.txt`. Size: an hour.

### 24.6 Papaya Island's world-map landmark

- **Wrong:** flying over the island shows no town model (other sites draw a 3D landmark).
- **Fix:**
  - Decompile (Ghidra headless) how a world-map site's on-load draws its landmark: the scripts call
    FindCharacterEntity 2-12, one per site.
  - Find the landmark table those ids index (model/sprite, position) and append an entry for Papaya (zone 15 world-map
    site, `tny_grounds.c` / `data/maps`) reusing a fitting town model (Gingertown's or West City's), or a recoloured
    copy.
  - Record the table in `docs/maps.md`.
- **Test:** headless world-map flight past the site (the arctic route test's world-map steps are the model); screenshot.
  Size: a day (mostly the decoding). Offered to Ryan, not asked for: **ask before starting**.

### 24.7 Walking speed per tournament playable

- **Wrong:** every tournament playable walks at Gohan's speed (they share his behaviour class); Spopovich should be
  slow, small fighters quick.
- **Fix:**
  - Decompile the walk and dash control states to find where the speed comes from (the class or the display row).
  - Give `tools/tnyplayables.py` a speed column (about 85 % for the giants, 110 % for Goten, Kid Trunks, Chiaotzu and
    Videl) and emit per-row class copies (or a per-row speed table read by a hook).
- **Test:** `tny_play.txt` measuring x after 30 frames of walking per row. Size: half a day to a day.

### 24.8 Decisions only Ryan can make (dialogue plan section 10, at their defaults now)

- A loser's line after a fight too (now only the winner speaks)?
- 2 v 2 opponent teams: draw known teams together (17 & 18, Goten & Trunks...) when both are in the pool (now random)?
- Start skips the talk: keep?

### 24.9 Housekeeping

- The merged branches' worktrees (`log2-artfix`, `log2-fightfix`, `log2-polish2`, `log2-polish2-art` and the build
  team's) hold the evidence films under their `build/` folders. Once Ryan has played this build, remove them with
  `git worktree remove <dir>` (their branches are merged into main).
- Real-time play of the second polish pass and the dialogue: Ryan's session is the first.

### 24.10 Team combos (Ryan's idea, 2026-10-25: in scope; the details are planned with him when he resumes)

**The idea (Ryan):** in the tournament, when a certain pair of fighters on the players' side both hit the **same
opponent** rapidly within a short time, a team combo fires:
- the frame freezes;
- an animation plays where both fighters hit that one opponent with a special combo move of their own;
- the fight resumes with everyone where the animation left them.

Which pairs get one comes from who fights together most in the anime.

**Where it applies:** co-op only (both players on the floor): 2 v 2, the Rumble, a co-op challenge. 1P has no
partner on the floor.

**What we already have to build on:**
- Each player's hits: the players' hit counters and the last hitter of an opponent in `tny_moves.c`
  (`tny_moves_last_hitter`, the hit messages).
- Freezing the fight: a scene running (the engine already waits on it), and the curtain's fades (section 23).
- Posing bodies frame by frame: `pose()` / `poses_apply` (engine and moves), the walk commands (`Cmd_Walk`, the
  walk-in) to move them, camera shake, sound effects.
- Damage on one target: the kit's hit message (`StealMsg` / HitMsg) aimed at one entity, as Energy Steal does.
- Each pair's identity: the players' picks (`eng.pick[]`) and the 2 v 2 team table (`tny_talk_teams`, dialogue
  plan section 7) as the starting list of pairs.

**Plan (to refine with Ryan):**
1. **Trigger:** each opponent remembers the last hit time from each player. When both players land hits on the same
   opponent within a short window (for example 3 hits each within 90 frames), and this pair has a combo, it fires. A
   cooldown (for example once per fight, or once per 30 s) keeps it special. Open with Ryan: a fixed hit count, a
   meter that fills, or a button press when the prompt shows.
2. **The pair list:** a table in `tny_moves.c` (or generated like the dialogue, from a doc table Ryan can edit):
   pair -> combo id. A first cut from the anime and the dialogue teams: Goku & Vegeta, Goku & Gohan, Goku & Krillin,
   Goku & Piccolo, Gohan & Piccolo, Gohan & Videl, Krillin & 18, Krillin & Yamcha, Tien & Chiaotzu, Tien & Yamcha,
   Vegeta & Trunks, Goten & Kid Trunks, 17 & 18, Hercule & Videl, Hercule & Fat Buu, Great Saiyaman & Videl.
   **A generic combo** for any other pair, so every team has one.
3. **The cinematic (a scripted sequence in C, about 2-3 s):**
   - the fight frozen (AI and pads off);
   - both fighters dash in from their sides of the target;
   - alternate strikes using poses they already have (melee groups 7 / 8, the dash 13, the special melee 16, the beam
     poses 18 / 19);
   - a finisher: a joint beam, a double kick that launches him, a ki volley, with shake, flashes and the hit sparks;
   - a caption naming the move (24.1's caption);
   - the target takes the combo's damage and lands knocked down where the animation ends;
   - control returns.
   Each combo is a short table of steps (who moves where, which pose for how many frames, when the damage lands), so a
   new pair's combo is data, not code.
4. **Positions after:** nobody snaps back; the players end where their last step put them, inside the ring's walls
   (the steps clamp to the floor, the way the knock-back's wall slide does).
5. **Names and moves per pair:** Ryan decides (for example Goku & Vegeta's joint beam, Tien & Chiaotzu's
   psychic-hold-then-Tri-Beam, Goten & Trunks's trading-places rush, 17 & 18's double barrier slam). Planned with him
   like the dialogue: a doc table first, built after he says the plan is done.

**Questions for Ryan when he resumes:**
- The trigger: automatic (hits in a window), a meter, or a button prompt?
- How often: once per fight or on a cooldown?
- Damage: a fixed share of the target's HP, or the players' stats?
- Can opponent pairs (17 & 18 against you) do combos to the players too?
- Does the Rumble get them (many targets), or only 2 v 2 and challenges?
- Which pairs and moves first?

**Size:** big. The trigger and the frozen-fight scene runner is about a day. The generic combo is half a day. Each
pair's own combo is an hour or two of step tables and testing once the runner exists. Test: a headless script that
makes both players hit one opponent (test command 27's punch routine for player 1, the pad for player 2), checks the
combo fires once, films it, and checks the positions and HP after.
