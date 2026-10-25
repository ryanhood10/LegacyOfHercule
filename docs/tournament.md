# Tournament mode: design in progress (Ryan, 2026-10-24)

Status: **plan done (Ryan, 2026-10-25), nothing built yet.** Every decision below is Ryan's unless marked "our
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
- **The announcer** (Buu record 207) is imported and calls every match; the crowd is LoG2's townspeople.
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
| **King Cold** | to build: Cooler's 64 x 64 fight set (34) recoloured and re-headed (the Paragus method), Cold's armour and cape; 74 keeps the cutscenes | O, **B** (villain) | start (Ryan: in the Rookie bracket) | melee giant with reach, Death Beam, a throw | moderate-big (a recolour + head; new frames only if the body reads wrong) |
| Mercenary Tao | 50, stat 44 (AI 129) | O, **B** | start (a 23rd Budokai finalist) | Dodon Ray (thin beam), Crane rush, the hidden blade (a one-off heavy hit); the ROM's own AI | moderate (4 groups) |
| **Master Roshi** | to build: a stock human fight set (Krillin's 76 or Hercule's 72) re-headed with Roshi's head (bald, beard, sunglasses, 75 for the face), orange shirt | O, **B** | start | **MAX Power** (bulks up at 50 %: +attack, slower), a slow heavy Kamehameha, Thunder Shock Surprise (a short stun), Drunken Fist (a rush) | moderate-big |
| Chi-Chi | to build: Android 18's fight set (11) re-headed, Chi-Chi's dress; Buu's 55 for the face | O, **B** | start | a frying-pan heavy hit (stun), a fast combo, no ki | moderate-big |
| Videl | to build: Android 18's set (11) re-headed; Buu's 56 for the face | O, **B** | start (a 25th Budokai fighter) | fast combo, Hawk Arrow (a rush), flight dodge | moderate-big |
| Chiaotzu | 102 (tiny NPC) + drawn poses | O, **B** | start | Dodon Ray, **psychic hold** (a stun), self-destruct at 10 % (a nova that costs him the match) | big (tiny body, poses from scratch) |
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
| Goten (SSJ) | 92 | O, **B** | Kamekameha (short beam), rush; small, fast | moderate |
| Kid Trunks (SSJ) | 93 | O, **B** | Big Tree Cannon (ball), Galick-style beam; small, fast | moderate |
| Gotenks | 277 base, 119 SSJ, 120 SSJ3 (confirmed by eye, `build/wm/buu_ids_x2.png`) | **C** (Champion 2v2: the fusion alone against your team) | **Super Ghost Kamikaze Attack** (three homing ghosts: the rocket class), Galactic Donut (a ring that stuns), SSJ3 at 40 % | moderate; ghosts big-ish |
| Vegito | 273 (SSJ) (?) | **C** (Legend 1v1) | Final Kamehameha (beam), Spirit Sword (long melee), Big Bang Kamehameha | moderate |
| Uub | 263 | **C** (Champion Rumble, won as Hercule: "the champion's student") | ki volley, rush; Kid Buu's numbers in a boy | moderate |
| Hercule (Buu saga) | 128 (= old Hercule's body, row 24 today) | O (Legend "the champion") | his four moves; a comic headliner | cheap |
| Piccolo (Buu saga) | 212 (cape), 214 / 215 (?) | O | Special Beam Cannon, Hellzone Grenade (volley that converges) | cheap |
| Dabura | 63 (giant, winged) | **C** (Adept 2v2: alone against your team) | Evil Flame (a fire cone: Super 13's wave), **Stone Spit** (a projectile that petrifies for 3 s), Darkness Sword (reach) | moderate; petrify big-ish |
| Fat Buu | 33 (giant) | O, **C** (Z-Fighter 2v2, with Hercule) | **Candy Beam** (a thin beam that stuns), Angry Explosion (nova), **regeneration** (HP regen rule), Mouth Blast | moderate; regen cheap |
| Super Buu | 39 / 40 base, 42 (Gotenks absorbed, vest) (?), 43 (Gohan absorbed, gi) (?) | O (Legend headliner), **B** (villain) | **Human Extinction Attack** (a volley), Vice Shout, absorb (heals on hit); **the Buu gauntlet: 40 -> 42 at 60 % -> 43 at 30 %**, each form stronger | moderate; the chain cheap once transforms exist |
| Kid Buu | 37 (32 x 64) | **C** (Champion 1v1) | **Planet Burst** (a huge ball), Vanishing Ball, mad rush, regeneration | moderate |
| Super Janemba | 144 (giant, horned, tail) | **C** (Z-Fighter Rumble) | Dimension Sword (reach), Rakshasa's Claw (teleport behind: Instant Transmission), Hell's Gate (a portal projectile) | moderate; portal big-ish |
| Evil Buu | 62 (grey, thin, the cape frame is his spit) | O, **C** (Adept Rumble) | Mouth blast (beam), Vanishing Ball, regeneration; thinner and faster than Fat Buu | moderate |
| Pikkon | 132 / 179 (two palettes; cap, red vest, green pants) | O, **C** (Adept 1v1) | Thunder Flash (beam), Hyper Tornado (a spin that hits around him), Burning Shoot | moderate |
| Kibito | 32 (giant, partial set) | **C** (Z-Fighter 1v1) | melee, **Heal** (restores his teammate in 2v2 / a Rumble ally) | moderate |
| Pui Pui | 224 (32 x 64, partial set) | **C** (Rookie 1v1) | ki volley, melee | cheap |
| Yakon | 290 (giant beast, partial set) | **C** (Rookie Rumble: a monster, story-neutral) | **Darkness Eater** (his hits drain your EP), claws | moderate |
| Buu-era Vegeta base / Goku variants | 116 (Vegeta base), 269-272, 107, 109, 114; 99 (Goku's 64 x 64 Spirit Bomb / big-beam frames); 97 / 102 Gohan outfits; 113 Goku with the halo | skins and forms of the above | | |

Not fighters (NPC grade in Buu's Fury): Supreme Kai 149, Babidi, Videl 56, Android 18 (Buu) 6, Future Trunks 4,
the announcer 207, Yamu 21. Not in either ROM: Olibu, Bojack, Hirudegarn, Pan. Records 259 / 260 are a lavender-haired kid in Goku's gi
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
| Champion | Kid Buu | Gotenks, the fusion alone against your team | Uub (won as Hercule: "the champion's student"; else Majin Vegeta's form for playable Vegeta) |
| Legend | Vegito (won as Goku or Vegeta; else SSJ3 for playable Goku) | the Legend trophy: the SSJ3 / Majin forms for whoever won | Super Buu's final form as the headliner; the prize is the record's last line |

Each challenger fight is its own cutscene in the ring after the trophy; a loss ends with "come back when you've
won again" and the record notes the attempt. The pairings are Ryan's to shuffle; the structure (one hidden
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
  Buu's Fury's announcer (record 207, NPC grade) is a cheap import.
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

## 10. Built: the moves piece (branch `tny-moves`, 2026-10-25)

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
  drain (passive: his melee hits heal him, or take a player's ki), regen (passive, permille of max a second),
  transform (at N % HP, once: a power-up pose, the new body, + attack / defence), summon (helpers beside him; at N %
  HP a copy of himself with half his health: Multi-Form).
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
