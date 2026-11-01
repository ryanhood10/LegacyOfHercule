# Buu's Fury: what the stock game does (research notes, 2026-11-01)

Facts page for the Buu campaign, the way `docs/story-rom.md` is for LoG2's. Decided 2026-11-01 (Ryan): the Buu
campaign is **chapters inside our ROM** (Buu's Fury's maps, bodies and story brought in as LoG2's were; no second
ROM, no save conversion). Its plan is not started: Ryan wants the LoG2 half perfected first. Sources: the
walkthrough by Tom Hayes (gamerstemple.com), StrategyWiki's contents, Kanzenshuu's wiki, GameFAQs boards, and our own
map list (`python tools/mapexport.py --buu --list`). A script dump of the ROM (the dumper does not read Buu's Fury
yet) will replace the wiki facts when the plan starts.

## The twelve chapters

| # | chapter | plays as | where (zone / maps) | what happens | bosses |
|---|---|---|---|---|---|
| 1 | The Other World | Goku (dead, with King Kai) | Snake Way, Yenma's office, Grand Kai's planet, Cave of the Ancients, Other World Stadium (zone 1) | the Other World tournament; Goku and Pikkon disqualified in the final | Kais, Olibu, Pikkon |
| 2 | Great Saiyaman | Goku, then Gohan | East District 439 (zone 2), Hercule City and Orange Star High (zone 3), Capsule Corp (zone 4), Musuka Circus | Gohan's school and disguise; Goku allowed one day on Earth; Videl finds him out | Red Shark Gang, the circus tank |
| 3 | World Tournament | Goku, Trunks (starts at level 60), Gohan, Videl | the forest (Goten's training), the Gravity Chamber, Papaya Island (zone 7), the Lookout | the junior division (Trunks wins; Hercule fakes a loss), Shin is the Supreme Kai, Spopovich beats Videl, Kibito's Buu backstory | Goten, Vegeta, Idasa, Hercule (20 HP), Spopovich and Yamu, Kibito |
| 4 | Babidi | Goku, Gohan, Vegeta | Southwest Forest (zone 8), Babidi's ship stages 1-3 | Spopovich and Yamu killed; Dabura turns Krillin and Piccolo to stone; Vegeta taken | Destroyer, PuiPui, Yakon, Dabura (10 000 HP) |
| 5 | Majin Vegeta | Goku | Papaya Island stadium, Rocky Canyon (zone 8), Southwest Forest | the Majin Vegeta duel in four stages; Buu hatches; Dabura eaten; Vegeta's self-destruct | Majin Vegeta, Majin Buu |
| 6 | Dragon Ball | Gohan, Trunks, Goku, Goten | the Lookout; Capsule Corp (the Radar); seven dungeons: Thieves Den (zone 9), Diablo Desert and its train (zone 11), Pilaf's castle, the Airship (zone 12), Fukurou Forest and the Ninja Stronghold (zone 13), Kyodai Pyramid (zone 14), Nataday Village and the Volcano (zone 15) | the seven Dragon Balls; **Broly** guards the 4-star at Volcano Summit | Pilaf's Guardian, the Airship Warlord, the Ninja Boss, Totenhotep, Broly |
| 7 | Majin Buu | Goku (SS3), Gotenks | the Lookout, West City (ZZTV roof, over Capsule Corp) | the Fusion Dance taught; Goku's day ends; Gotenks loses | Majin Buu twice |
| 8 | Janemba (movie 12) | **Hercule**, Goku, Vegeta, Gogeta | Hercule's town and Buu's house, Snake Way, HFIL (zone 18), Yenma's office | **Hercule befriends Buu** (the bomb, the puppy); Janemba takes the Other World; Hercule shot, Buu revives him; Evil Buu becomes Super Buu | Janemba, Super Janemba (Gogeta) |
| 9 | Super Buu | Goku, Goten, Trunks, Gotenks, Gohan | the Planet of the Kais (zone 19), Capsule Corp, the Lookout, the **Hyperbolic Time Chamber** (zone 6), Barren Wasteland (zone 20) | the Z-Sword and the Old Kai; Super Buu's hour; Gotenks in the chamber; the Lookout eaten; Mystic Gohan; Gohan absorbed; the Potara | Goten, Super Buu four times |
| 10 | Fusion | Goku, Vegito, Vegeta | Barren Wasteland, inside Buu (zone 21: Stomach, Intestines, Brain) | Vegeta agrees; Vegito; inside Buu the fakes and the pods | Super Buu outside and inside |
| 11 | Kid Buu | Goku, Vegeta (recommended level 140) | the Planet of the Kais, Earth (destroyed), New Namek | Earth restored by the wishes; **Hercule talks the Earth into the Spirit Bomb** and saves Vegeta; Goku throws it | Kid Buu (split, two phases) |
| 12 | A New Beginning | Goku | Capsule Corp, Papaya Island ten years on | Pan, Uub, Goku leaves | Uub |

Also in the ROM: zone 0 West City / Rogue Submarine / Stolen Airship / Z-Mart (post-game: the airship to the Shadow
Colossus), zone 5 Kame House, zone 22 Heaven, zone 8 Demon World (Janemba's). 452 maps in all.

## Systems (what LoG2 does not have)

- **Levels:** Buu's Fury's cast starts around 50-60 (Trunks at 60) and the story gates by level: 65 (ch1 Goku), 70
  and 85 (ch4 Goku, Gohan), 80 (ch6 Trunks), 100 (ch6 Gohan), 120 (a forest door), 140 (Kid Buu), 135-145 (post-game
  gates). The **cap is 200** (GameFAQs), not 100. Mystic Gohan is +25 levels. LoG2's cap is 50 (`docs/hercule.md`:
  50-word EXP table; HP growth is multiplicative, 5.5-7.5 % a level, so a cap past 50 needs a flatter growth too).
- **Stat points:** 3 a level, spent by hand (Strength / Power / Endurance; Speed fixed); a stat never above the level.
- **Equipment:** body, hands, feet, accessory; from shops, drops, bosses. Money. Supplies (HP / EP).
- **Scouter:** records enemies' stats and visited maps; upgraded in ch9.
- **Exhibits:** 25 Hercule Exhibits (shown at the Hercule Cafe in Hercule City) and 15 Z-Fighter Exhibits (Capsule
  Corp's exhibit hall); all 40 unlock Super Saiyan Gogeta.
- **Fusion:** the Fusion Dance as a button mini-game (Gotenks from ch7, Gogeta in ch8); the Potara (Vegito, ch10 only).
- **Transformations:** Super Saiyan for everyone, Super Saiyan 3 for Goku and Gotenks, Majin Vegeta (ch5), Mystic Gohan.
- **Playables:** Goku, Gohan, Goten, kid Trunks, Vegeta; one-offs Videl (ch3), Hercule (ch8), Gotenks, Gogeta,
  Vegito. Piccolo and Krillin are NPCs (stone in ch4-5).

## Hercule in the stock game

The champion with a city, a cafe and exhibits named after him; fakes a loss to Trunks (ch3); playable against Buu
(ch8: the bomb, the puppy, the friendship); shot and revived (ch8); the one the Earth listens to for the Spirit Bomb
(ch11); Buu lets him win the tournament (ch12). The canon Buu saga is already his story: our reveal
(`docs/story.md` 2 and 6: the fusion is Goku + Hercule, the final blow Super Saiyan Hercule's) has its places in
chapters 8, 10 and 11.

## Already ours

Buu's Fury's sprite and map formats (`docs/sprites.md` "Buu's Fury", `docs/maps.md` 10); imported so far: the
tournament grounds (zone 7), the Time Chamber room (6/6, as the Saiyan ship's interior), Broly's crater (8/71), about
thirty fighter bodies and portraits for the tournament, Broly, old Hercule, the ship. The tournament's roster already
holds Goten, kid Trunks, Gotenks, the Buus, Dabura, Pikkon, Janemba, Majin Vegeta and Videl as fighters.

## Threads to decide when the plan starts (research done, nothing decided)

The seven-year gap and who stays (Future Trunks); the cast and party slots (9 now; the check allows up to 15); when
the level cap lifts and how growth flattens; the Time Chamber as more than a grinder; which systems to keep (the
exhibits and the fusion dance look worth it, the stat points, equipment, money and Scouter do not); the world map (one
Earth with new landings, the Other World by the Instant Transmission circle, rather than a second world map); what to
cut (ch6's seven dungeons is the long part).

## Ryan's ideas for the Buu phase (noted, not planned)

- 2026-11-01: after the Peace Times the sequel opens from the villains' side. The first chapters are played as the
  villains only, or as specific villains: Buu, Dabura, Babidi, Spopovich, Yamu and others. Chapters with the heroes come
  later, leading down to their final face-offs. To take up when the Buu planning starts; nothing is built for it now.
