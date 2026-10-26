# Tournament dialogue: the plan for Ryan's review (2026-10-25)

Status: **plan done (Ryan, 2026-10-25: "done as written"); to be built after the polish pass merges.** Section 10's
questions stand as the plan has them: the winner's line only, 2 v 2 opponent teams drawn at random, Start skips.
Built so far: `src/coop/tny_talk.c` (choosing and showing the lines), the data generated from this doc's tables by
`tools/tnytalk.py` (an edit here is an edit in the game: the build reads sections 4-7), test command 31 and
`tools/headless/tny_talk.txt`. Still to do: calling it from the engine's talk hook (the polish pass's) and the
Rumble announcer's calls (section 8).
Original brief: Ryan asked for Budokai-style lines between the
fighters on the ring, planned for every combination in 1 v 1 and 2 v 2, and **the announcer only in the Royal
Rumble**. Mark any line to change, cut or replace. Lines you dictate go in word for word. When you say the plan is
done, it gets built into the hook the polish pass leaves in the engine (right after the walk-in, and right after the
end pose).

Contents: 1 when lines play, 2 how a line is chosen, 3 writing rules, 4 every fighter's own lines (layer 1),
5 rival exchanges (layer 2), 6 meeting yourself, 7 2 v 2 team lines (layer 3), 8 the Royal Rumble's announcer,
9 the size of it, 10 questions for you.

## 1. When lines play

| moment | 1 v 1 | 2 v 2 (co-op) | Royal Rumble |
|---|---|---|---|
| after the walk-in, before "Begin!" | 2 lines (3-4 for a rival exchange) | 2-4 lines (one per side, more for a known team or rival teams) | the announcer's round call only (section 8) |
| after the end pose and the announcer's line | the winner's line (1) | one winner's line (1) | the announcer only |
| a hidden challenger's fight | the challenger opens with his own line, your fighter replies | same, the challenger pair's line | the announcer (as now) |
| the co-op 1 v 1 final (player vs player) | both players' fighters, as any 1 v 1 | - | - |

- Each line is one dialogue box with the speaker's portrait. A advances. Pressing Start skips the rest of the talk
  (so a fight you've seen ten times starts at once).
- CPU-vs-CPU results stay as they are: no dialogue, only the bracket.
- Pacing: at most 4 boxes before a fight and 1 after it.

## 2. How a line is chosen (most specific first)

1. **Meeting yourself** (section 6): the same person from another time, such as Goku vs Goku (Buu Saga).
2. **A rival exchange** (section 5) for this exact pair, in either order: it replaces the defaults.
3. **2 v 2 only: a known team** (section 7) speaks its team line; **two known teams with history** use a
   team-vs-team exchange.
4. **The defaults** (section 4): the opponent opens with one of his two openers (picked at random), your fighter
   answers with his reply. After the fight the winner says his win line.

Who speaks first: the opponent (or the challenger) opens and your fighter answers, so the player always gets the
last word before "Begin!". In a player-vs-player final, player 2's fighter opens.

## 3. Writing rules

- About 60 characters fit one box (three short rows), speaker's name included. The lines below aim for 45 or less
  so long names (Super Perfect Cell, Great Saiyaman) still fit; anything longer runs onto a second box.
- Original lines in each character's voice; no quotes copied from the show or other games.
- **Hercule is the loud showman everyone knows** (the champ, the cameras, the bragging). Nothing in these lines
  hints at anything more until the Buu phase.
- Fighters without a dialogue portrait (Android 13, Cell Jr., the Namekian Warrior, Evil Buu) speak with their
  face drawn from their sprite, as the select screen already does.
- The two Hercule entries (the opponent and your party's Hercule) share one voice, and so do the two Android 16
  entries.

## 4. Layer 1: every fighter's own lines

Openers are said when he opens a fight (one of two at random); the reply when he answers; the win line after he
wins. 57 voices, 4 lines each.

### Rookie

| fighter | opener 1 | opener 2 | reply | win |
|---|---|---|---|---|
| Yamcha | Wolf Fang Fist is warmed up. Let's go! | I trained hard for this. Really hard. | Don't count me out just yet! | Ha! Did you see that? Somebody saw that, right? |
| Tien | My third eye sees every move you make. | I fight for discipline, not prizes. | Then show me what you've got. | A good bout. Train harder. |
| Frieza | How amusing. You actually showed up. | I'll make this quick. Do try to scream. | You dare speak to me like that? | Kneel. That's where you belong. |
| Mecha Frieza | Rebuilt, upgraded and in a foul mood. | Every bolt in me wants revenge. | Your insolence will cost you. | Machine or not, I'm still your better. |
| King Cold | Bow before the king of the universe. | My son is careless. I am not. | Such bold words from such a small thing. | That is how royalty fights. |
| Hercule | The champ is here! Get ready to lose! | Cameras on me! This won't take long! | Ha ha ha! You're cute. Real cute. | Hercule wins again! Who's surprised? Nobody! |
| Master Roshi | Heh heh. Don't let the old bones fool you. | Show some respect for your elders, kid. | Oh? This old man's still got it. | Ho ho! Back to the beach for me. |
| General Tao | I've ended better men for less money. | My fee is high. Your end comes free. | The Crane style will finish you. | Another job done. Now pay me. |
| Spopovich | Grrr... crush... you... | Master Babidi... will be pleased... | Hrrrgh! | Heh... heh... more... |
| Yamu | Your energy will do nicely. | Hold still. This won't hurt much. | I'll take every drop you've got. | All that energy, mine. Delicious. |
| Chi-Chi | I didn't come here to lose. Hiya! | Don't think I'll go easy on you! | Oh, you're gonna regret that! | And THAT'S how a lady fights! |
| Videl | I've fought crooks since I was a kid. | Let's go. I'm not holding back. | Big talk. Let's see you back it up. | Not bad. But I'm better. |
| Chiaotzu | Tien says I can win this. I believe him! | Don't look at me like that. I'm strong! | Here I come! Hyaaah! | I did it! Tien, did you see? |

### Adept, Z-Fighter

| fighter | opener 1 | opener 2 | reply | win |
|---|---|---|---|---|
| Android 19 | Your energy reading is acceptable. | I will absorb everything you have. | Analysis complete. You will lose. | Energy absorbed. Thank you. |
| Dr. Gero | Every flaw in your style is on record. | My calculations predict your defeat. | Insolent. You'll make good data. | Exactly as my data predicted. |
| Android 13 | Target confirmed. Starting combat. | Orders are orders. Nothing personal. | Ha! You'll be scrap in a minute. | Target down. Mission complete. |
| Android 17 | Sure, I'll play. Nothing better to do. | Let's make this interesting, at least. | You're more fun than I expected. | Too easy. Try again sometime. |
| Android 18 | Let's get this over with. I'm busy. | Don't expect me to go easy. | Cute. Now try to keep up. | Was that all? How boring. |
| Imperfect Cell | Your power... I can taste it already. | Every fight makes me stronger. | Hsss... interesting. Very interesting. | Another step toward perfection. |
| Android 16 | I do not wish to fight. But I will. | My programming says: protect. Then win. | Understood. Engaging. | It is over. Rest now. |

### Champion

| fighter | opener 1 | opener 2 | reply | win |
|---|---|---|---|---|
| Semi-Perfect Cell | So close to perfection. Watch me. | You'll do nicely while I wait. | Heh. Struggle all you like. | Not perfect yet. But good enough. |
| Perfect Cell | Welcome to my stage. Entertain me. | Perfection fears nothing. Begin. | Excellent. Show me everything. | Perfect, as expected. |
| Cell Jr. | Hee hee! Papa says I get to play! | Play with me! Play with me! | Hee hee! Gotcha! | Again! Again! |
| Cooler | My brother was soft. I am not. | Let me show you the family's real power. | You'll regret raising a fist to me. | Remember the name. Cooler. |
| Namekian Warrior | I fight to honor the Grand Elder. | Namek's warriors never back down. | Very well. Prepare yourself. | Honor is satisfied. |
| Broly | Kakarot... where is Kakarot?! | I'll crush you. Then everyone. | Hrrraaagh! | Is that all? Bring me more! |
| Paragus | Pray my son stays calm today. | A Saiyan elite does not lose to you. | Insolence! I'll teach you manners. | My son will rule. You were practice. |

### Legend

| fighter | opener 1 | opener 2 | reply | win |
|---|---|---|---|---|
| Super Perfect Cell | Back from the brink, stronger than ever. | I've been dying to show off. Literally. | Oh, you won't be getting up. | Even better than perfect. |
| Super Android 13 | Merged and upgraded. You're finished. | Target: everything. Starting with you. | Resistance is pointless. | Mission accomplished. Next target. |
| Goku (Buu Saga) | This is gonna be fun! Ready? | I came back just for a fight like this! | Alright! Now we're talking! | Great match! Let's do it again! |
| Majin Vegeta | Babidi's mark gives me what I need. | No more holding back. Not for anyone. | Good. Make me fight for real. | You were never at my level. |
| Mystic Gohan | I've unlocked everything I've got. | I'll end this before it gets ugly. | Okay. Then I won't hold back. | Sorry. I had to take that seriously. |
| Great Saiyaman | Justice has arrived! Great Saiyaman! | Evildoers, prepare to be stopped! | Your crimes end here, villain! | Justice prevails once again! |
| Hercule (Buu Saga) | Still the champ! Some things never change! | My fans are watching! Let's give 'em a show! | Ha! I beat tougher guys before breakfast. | Champ of the world! Write it down! |
| Piccolo (Buu Saga) | Hmph. Let's see if you've improved. | Don't waste my time. Fight seriously. | Then come at me with everything. | Train harder. Then come back. |
| Super Buu | Buu is hungry. You look like candy. | Buu wants to play. You play now. | Hee hee hee! Fun! | Buu wins! Buu always wins! |

### Hidden challengers

| fighter | opener 1 | opener 2 | reply | win |
|---|---|---|---|---|
| Pui Pui | Babidi sent me. You'll be my trophy. | On my world gravity is ten times this! | Ha! A weakling with a big mouth. | Too easy. Tell Babidi I'm ready. |
| Goten | Hi! Can we fight now? Please? | Mom said I'm not allowed to lose! | Okay! Here I come! | Yay! I won! I'm telling Trunks! |
| Kid Trunks | This'll be quick. I've got stuff to do. | My dad's Vegeta. You've already lost. | Heh. You're gonna regret that. | Told you. Easy. |
| Yakon | Your light looks delicious. | In the dark, you're mine. Grrah! | Ghhh... I'll eat your energy. | More light! Feed me more! |
| Pikkon | I trained in the afterlife for this. | Show me your best. I'll show you mine. | Good. Come at me. | A fine fight. Train well. |
| Dabura | The demon king greets you. Briefly. | Look into my eyes. Turn to stone. | How refreshingly foolish. | A pleasure. For me, anyway. |
| Evil Buu | Hehh... nothing will be left of you. | Pure evil needs no reason. | Grrrh... | Hehh. Next. |
| Kibito | In the Supreme Kai's name, fight. | I'll test your strength. Ready? | Very well. Let us begin. | Your heart is good. Keep training. |
| Fat Buu | Buu fight! Then Buu eat candy! | You make Buu mad? Okay! Fight! | Hee hee! Buu ready! | Buu win! Buu go eat now! |
| Super Janemba | Jaaa... nemba... | Grrraaah! Janemba! | Kehehehe... | Janemba! Janemba! |
| Kid Buu | Hee hee hee! Aaaaagh! | Gyaaa! Gyaha ha ha! | Rrrraaaagh! | Kyaaa ha ha ha ha! |
| Gotenks | The super-awesome Gotenks has arrived! | Get ready for my best attack ever! | Bring it! I'm way cooler than you! | Gotenks is the greatest! Duh! |
| Uub | I want to get stronger. Please fight me! | I came a long way for this. Let's go! | Okay! I won't lose! | I did it! I'm getting stronger! |
| Vegito | Two of the best, in one. Bad luck for you. | Relax. I'll make this quick. | Cute. Try harder. | Never had a chance. Don't feel bad. |

### Your party (the story's fighters)

| fighter | opener 1 | opener 2 | reply | win |
|---|---|---|---|---|
| Gohan | I don't like fighting. But I'll win. | Dad would want me to do my best. | Alright. I'm ready. | Sorry! Are you okay? |
| Krillin | Okay, okay. I can do this. Probably. | Don't let the height fool ya! | Heh. Bring it on! | Wait, I won? I won! Woo! |
| Piccolo | Hmph. Don't waste my time. | I'll show you a real warrior. | Talk is cheap. Fight. | Go home and train. |
| Vegeta | You stand before the prince of Saiyans. | I'll make this painful. And short. | Hah! You dare challenge me? | Know your place. Beneath me. |
| Trunks | I crossed time for battles like this. | No time to lose. Let's go. | Alright. Don't hold back. | Good fight. You made me work for it. |
| Goku | Hey! You look strong! Let's fight! | Whew, I'm getting excited! Ready? | Alright! Let's go all out! | That was awesome! Let's go again! |
| Hercule, Android 16 | (as above) | | | |
| SSJ2 Gohan | I'm not holding back anymore. | You picked the wrong fight. | ...Fine. Then it's over. | Stay down. |

## 5. Layer 2: rival exchanges (either order; the first speaker is the one named first)

When the pair meets in 1 v 1 (or faces each other in a 2 v 2), these replace the defaults. 90 pairs.

### Goku's

| pair | lines |
|---|---|
| Goku, Vegeta | V: Kakarot. Today I finally settle this. / G: Ha! I was hoping you'd say that! / V: Wipe that grin off your face! |
| Goku, Frieza | F: You again. I should've finished you. / G: You had your chance, Frieza. / F: Then I'll take another. |
| Goku, Piccolo | P: Don't think I've forgotten our deal. / G: Heh, I know. Let's make it a good one! |
| Goku, Krillin | K: Goku! Go easy on me, okay? / G: No way, buddy! You'd never forgive me! / K: ...Yeah, that's fair. |
| Goku, Chi-Chi | C: Goku! You promised you'd study with Gohan! / G: Uh... after this fight? / C: Oh, you are SO in trouble! |
| Goku, Perfect Cell | C: Goku. I've waited for a real opponent. / G: Then let's give 'em a show, Cell. |
| Goku, Broly | B: KAKAROT! / G: Whoa! Easy, big guy! / B: Kakarot... Kakarot! |
| Goku, Majin Vegeta | V: No more crowd, no more rules. Just us. / G: Vegeta... you didn't have to do it like this. |
| Goku, Kid Buu | B: Heeee hee hee! / G: This guy's trouble. Gotta finish him fast. |
| Goku, Yamcha | Y: I've been training! You'll see! / G: Great! Show me, Yamcha! |
| Goku, Tien | T: I've closed the gap. Let's find out. / G: Ha! I knew you would, Tien! |
| Goku, Master Roshi | R: Show your old master what you've learned. / G: You got it, Master Roshi! |
| Goku, Pikkon | P: I've heard about you in the afterlife. / G: Same here! Let's see who's better! |
| Goku, Uub | U: Are you... really going to fight me? / G: Sure am! Show me what you've got, kid! |
| Goku, Cooler | C: You humiliated my family name. / G: Your brother did that himself. |
| Goku, Hercule | H: You! You're that guy with the hair! / G: Heh. Nice to meet you too! / H: Prepare to be dazzled! |

### Vegeta's

| pair | lines |
|---|---|
| Vegeta, Frieza | F: My, my. The little prince. / V: I'm not your soldier anymore, Frieza! |
| Vegeta, Broly | B: Vegeta... / V: Tch. Don't think I'm afraid of you. |
| Vegeta, Perfect Cell | C: Shall we try that again, Vegeta? / V: This time I won't give you anything! |
| Vegeta, Trunks | T: Father. I won't hold back. / V: Good. Then prove you're my son. |
| Vegeta, Kid Trunks | K: Dad! You promised you'd fight me! / V: Hmph. Show me your stance. Now. |
| Vegeta, Android 18 | A: You again. Still nursing that arm? / V: I'll break more than your smile! |
| Vegeta, Android 19 | A: Prince Vegeta. Your energy, please. / V: Come and take it, machine! |
| Vegeta, Dr. Gero | G: The prince. A predictable specimen. / V: Predict THIS, old man! |
| Vegeta, Goku (Buu Saga) | G: Vegeta! Just like old times! / V: Old times end today, Kakarot! |
| Vegeta, Dabura | D: The prince of a dead race. Charming. / V: You'll regret those words. |

### Gohan's (story Gohan and SSJ2 Gohan both use these)

| pair | lines |
|---|---|
| Gohan, Perfect Cell | C: Show me the power you hide, Gohan. / G: You really want to see it? |
| Gohan, Super Perfect Cell | C: You won't catch me off guard twice. / G: I don't need to. |
| Gohan, Cell Jr. | J: Hee hee! You look fun! / G: Sorry. You're not hurting anyone. |
| Gohan, Piccolo | P: Show me how much you've grown. / G: I'll do my best, Mr. Piccolo! |
| Gohan, Videl | V: You're stronger than you look. Fight! / G: Uh... okay, but go easy? |
| Gohan, Frieza | F: The little monkey from Namek. / G: I'm not little anymore. |
| Gohan, Super Buu | B: Buu remember you. You were tasty. / G: Not this time. |
| Gohan, Spopovich | S: Grrr... energy... / G: You're not taking anything from me. |
| Gohan, Chi-Chi | C: Gohan! Fighting instead of studying?! / G: Mom! It's just one match, I promise! |

### Krillin's, Piccolo's, Trunks's

| pair | lines |
|---|---|
| Krillin, Android 18 | A: You. You're the short one. / K: ...Can we not fight, maybe? / A: No. |
| Krillin, Android 17 | A: Hey, baldy. Still scared of me? / K: Terrified! But I'm here anyway! |
| Krillin, Frieza | F: You. I remember you. / K: Yeah... I remember YOU too. |
| Krillin, Yamcha | Y: Krillin, no hard feelings, right? / K: Sure! Just like the old days! |
| Krillin, Master Roshi | R: My student! Show me your training! / K: Yes, Master! ...Please go easy! |
| Krillin, Tien | T: You've grown, Krillin. / K: Not taller, though. Let's go! |
| Piccolo, Imperfect Cell | C: Piccolo. I have your cells, you know. / P: Then you know how this ends. |
| Piccolo, Android 17 | A: The green guy again? Fine. / P: This time you won't walk away. |
| Piccolo, Dr. Gero | G: Namekian. Fascinating biology. / P: Study this, old man. |
| Piccolo, Goten | G: Mr. Piccolo! You're on! / P: Hmph. Brat. Let's see you. |
| Trunks, Frieza | F: You! The boy with the sword! / T: You remember. Good. |
| Trunks, Mecha Frieza | F: I've been rebuilt for you, boy. / T: Then I'll take you apart again. |
| Trunks, King Cold | C: You cut down my son. / T: And you're next in line. |
| Trunks, Android 17 | A: You look like you hate me. / T: In my time, you took everything. |
| Trunks, Android 18 | A: Do I know you? / T: Not in this time. Lucky you. |
| Trunks, Imperfect Cell | C: You came back in time, didn't you? / T: To stop you. Every version. |
| Trunks, Perfect Cell | C: Ah. The boy who ran from me. / T: I'm not running anymore. |

### The Tournament crowd's

| pair | lines |
|---|---|
| Tien, General Tao | Tao: My old pupil. Back for a lesson? / Tien: You taught me to hate. I chose not to. |
| Tien, Chiaotzu | C: Tien! Don't go easy on me! / T: Never, Chiaotzu. Never. |
| Tien, Yamcha | Y: Tien! Rematch time! / T: You never learn, Yamcha. |
| Tien, Imperfect Cell | C: Your energy smells familiar. / T: Then you know how stubborn it is. |
| Tien, Master Roshi | R: The Crane's student! Come at me! / T: With respect, I won't hold back. |
| Chiaotzu, General Tao | T: The little one. Still Tien's pet? / C: I'm not scared of you anymore! |
| Videl, Spopovich | S: Grrr... girl... break you... / V: Not this time, you creep! |
| Videl, Great Saiyaman | S: Citizen! Surrender peacefully! / V: Gohan, I KNOW it's you. / S: ...Who? |
| Videl, Hercule | H: My girl! Don't hurt your old man! / V: Dad! Fight me for real, okay? |
| Chi-Chi, Master Roshi | R: Chi-Chi! My, how you've grown... / C: Watch it, old man! |
| Chi-Chi, Hercule | H: A lady? Ha! I'll go easy on you! / C: You'll WHAT?! |
| Yamcha, Dr. Gero | G: Yamcha. I already know your moves. / Y: Then you know they're new! |
| Hercule, Perfect Cell | C: The world champion. Show me. / H: I, uh... have a stomachache... / C: Begin. |
| Hercule, Spopovich | S: Grrr... / H: Whoa! Big guy! Let's keep it clean! |
| Hercule, Great Saiyaman | S: Champion! Fight me for justice! / H: Justice? I fight for the belt! |
| Hercule, Fat Buu | B: Buu like you! / H: Aw, buddy! Don't make me hurt ya! |
| Hercule, Super Buu | B: Buu remember. You funny. / H: Ha ha! Funny AND strong! ...Right? |
| Hercule, Android 16 | H: A robot? I eat robots for lunch! / A: I am not edible. |

### Villains', Buu's and the androids'

| pair | lines |
|---|---|
| Frieza, King Cold | C: Son. Show your father your best. / F: Don't patronize me, Father. |
| Frieza, Cooler | C: Little brother. Still sloppy, I see. / F: You always were insufferable. |
| King Cold, Cooler | K: My eldest. Make me proud. / C: I always do, Father. |
| Broly, Paragus | P: Broly! Calm yourself! / B: Father... out of my way. |
| Perfect Cell, Cell Jr. | J: Papa! Play with me! / C: Very well. Don't cry when you lose. |
| Perfect Cell, Android 16 | C: You're a relic, 16. / A: A relic that will stop you. |
| Imperfect Cell, Android 17 | C: Number 17. You're mine. / A: Back off, bug. |
| Imperfect Cell, Android 18 | C: Number 18. Come to me. / A: Not in a million years. |
| Android 16, Dr. Gero | G: 16! You'll obey me! / A: No. |
| Android 17, Android 18 | 18: Don't get cocky, brother. / 17: Who, me? Never. |
| Android 13, Super Android 13 | 13: That's... me? / S13: The better you. |
| Kid Buu, Uub | B: Rrraagh! / U: I feel like I know you... |
| Super Buu, Gotenks | G: You again! Round two! / B: Buu eat you this time. |
| Super Buu, Vegito | B: You fused! Buu absorb you! / V: Cute. Try it. |
| Super Buu, Mystic Gohan | B: Gohan! Buu want Gohan! / G: Come and get me. |
| Kid Trunks, Goten | G: Trunks! For real this time! / T: You always say that! |
| Dabura, Kibito | D: The Supreme Kai's lapdog. / K: You won't pass, demon. |
| Pikkon, Super Janemba | P: The monster from the afterlife. / J: Jaaaa... nemba! |
| Yakon, Goku | Y: Your light! Give it! / G: Sure, have all you want! |
| Pui Pui, Vegeta | P: A Saiyan? Babidi will love this. / V: You won't live to tell him. |

## 6. Meeting yourself

| pair | lines |
|---|---|
| Goku, Goku (Buu Saga) | B: Whoa, it's me! / G: This is the weirdest fight ever! / B: Yeah, isn't it great?! |
| Vegeta, Majin Vegeta | M: Weakling. That's what I used to be? / V: That's what you're about to be. |
| Gohan, Mystic Gohan | M: You've got a long way to go. / G: Then show me where I end up! |
| Gohan, Great Saiyaman | S: Young citizen! Prepare for justice! / G: ...Please tell me I don't wear that. |
| SSJ2 Gohan, Mystic Gohan | M: You're angry. I remember. / G: Then you know what's coming. |
| Piccolo, Piccolo (Buu Saga) | B: You're slow. / P: You're arrogant. / B: We both are. Fight. |
| Trunks, Kid Trunks | K: Who are you? / T: ...Someone who knows you very well. |
| Hercule, Hercule (Buu Saga) | B: Hey! Nice moustache! / H: I was gonna say the same thing! / B: Champ vs champ! |

## 7. Layer 3: 2 v 2 team lines

When both members of a known team are on the same side, they open together (one line each) instead of the
default openers. Any other pair uses each member's layer-1 lines.

| team | lines |
|---|---|
| Goku, Vegeta | G: Ready, Vegeta? / V: Don't slow me down, Kakarot. |
| Goku, Gohan | G: Let's show 'em, Gohan! / Gohan: Right behind you, Dad! |
| Goku, Krillin | K: Like the old days, Goku! / G: You bet, Krillin! |
| Goku, Piccolo | P: Don't get in my way. / G: Wouldn't dream of it! |
| Gohan, Piccolo | G: Ready, Mr. Piccolo? / P: Always. Stay sharp. |
| Gohan, Videl | V: Try to keep up, Gohan. / G: I'll... do my best! |
| Krillin, Android 18 | K: We make a pretty good team, huh? / 18: Just don't trip. |
| Krillin, Yamcha | Y: The old gang's back! / K: Two of us, at least! |
| Tien, Chiaotzu | C: Together, Tien! / T: Always, Chiaotzu. |
| Tien, Yamcha | Y: Try not to steal my spotlight. / T: What spotlight? |
| Vegeta, Trunks | T: Father, follow my lead. / V: Hah! You follow MINE. |
| Goten, Kid Trunks | G: Let's do this, Trunks! / T: Don't mess it up, Goten! |
| Android 17, Android 18 | 17: You take the left one. / 18: You're not the boss of me. |
| Android 16, Android 17 | 17: Big guy, try to keep up. / 16: I will protect you. / 17: ...Sure. |
| Android 19, Dr. Gero | G: 19. Follow your programming. / 19: Understood, Doctor. |
| Spopovich, Yamu | Y: Leave some energy for me. / S: Grrr... crush... |
| Frieza, King Cold | C: Let's teach them manners, my son. / F: Try to keep up, Father. |
| Frieza, Cooler | C: Don't embarrass me, brother. / F: Don't embarrass yourself. |
| Broly, Paragus | P: Broly! Destroy them! / B: ...Hrrraagh! |
| Perfect Cell, Cell Jr. | J: Papa! Can I break them? / C: Go ahead. Have fun. |
| Hercule, Videl | H: Daddy and daughter, unbeatable! / V: Just don't hog the cameras, Dad. |
| Hercule, Fat Buu | B: Buu and Mister Hercule, best friends! / H: Ha ha! Let's go, buddy! |
| Great Saiyaman, Videl | S: Justice duo, assemble! / V: Do NOT call us that. |
| Goku, Chi-Chi | C: Goku, you'd better not lose. / G: Yes, dear! |
| Master Roshi, Krillin | R: Like the old days, my boy! / K: Yes, Master! Let's go! |
| Goku (Buu Saga), Majin Vegeta | G: Together again, huh? / V: Tch. Only this once. |
| Mystic Gohan, Great Saiyaman | S: Two of me! Twice the justice! / G: ...Twice the embarrassment. |
| Pui Pui, Yakon | P: Babidi's finest. / Y: Your light is mine! |

**Team vs team** (both sides known teams with history; replaces both team lines):

| teams | lines |
|---|---|
| Goku & Vegeta vs Frieza & King Cold | F: The monkeys have teamed up. / V: And you two are finished! |
| Goten & Kid Trunks vs Android 17 & 18 | 17: Kids? Seriously? / T: We'll beat you in five minutes! |
| Tien & Chiaotzu vs Spopovich & Yamu | Y: Two little ones. Plenty of energy. / T: You'll find us hard to swallow. |
| Krillin & 18 vs Android 17 & 16 | 17: Sis, really? Him? / 18: Mind your own business. |
| Gohan & Piccolo vs Perfect Cell & Cell Jr. | C: Teacher and student. How sweet. / P: Gohan, don't hold back. |
| Hercule & Videl vs Spopovich & Yamu | S: Grrr... / H: Don't worry, honey! Daddy's got this! / V: Dad, behind you! |

## 8. The Royal Rumble: the announcer only

No fighter lines in a Rumble. The announcer calls the action from short pools (a different line each time where
possible). Existing calls stay; these are added.

| moment | calls |
|---|---|
| round 1 | Royal Rumble, round 1! Last ones standing win! / Into the ring! Every fighter for themselves! |
| a middle round | Round [N]! The pack gets tougher! / Round [N]! Fresh fighters, fresh trouble! |
| a fighter goes down | [Name] is down! / [Name] hits the floor! / That's it for [Name]! / Down goes [Name]! |
| two down in a row | What a flurry! Two fighters down! |
| a player goes down (co-op) | [Name] is down! Can the partner hold on? |
| round cleared | Round cleared! [Player] stands tall! / The ring is clear! On to the next round! |
| headliner: King Cold (Rookie) | The king of the universe himself! King Cold! |
| headliner: 17 & 18 (Adept) | Twin terrors! Android 17 and Android 18! |
| headliner: Android 16 (Z-Fighter) | The silent giant! Android 16! |
| headliner: Broly (Champion) | The Legendary Super Saiyan, Broly! |
| headliner: Super Buu (Legend) | The final menace! Super Buu! |
| Super Buu changes form | He's changing! Look at that power! / Not again! Another form! |
| the players win | The headliner is down! [Player] wins it all! |
| the players lose | [Player] is out! What a battle! |

## 9. The size of it

| layer | count | boxes |
|---|---|---|
| 1 every fighter | 57 voices x 4 | 228 |
| 2 rival exchanges | 90 pairs | about 220 |
| 6 meeting yourself | 8 | about 20 |
| 3 team lines | 28 teams + 6 team-vs-team | about 75 |
| 8 announcer | about 35 calls | 35 |

About 580 lines of text, roughly 25 KB of ROM. Any of the 1,700 possible 1 v 1 pairs and every 2 v 2 team gets
a sensible exchange; the ones fans care about get their own.

## 10. Questions for you

1. **After a fight:** only the winner's line (this plan), or a loser's line too?
2. **2 v 2 opponent teams** are drawn at random from the pool now, so a known team (17 & 18, Goten & Trunks) only
   faces you by chance. Should the draw put known teams together when both are in the pool?
3. **Start to skip** the talk: keep it?
