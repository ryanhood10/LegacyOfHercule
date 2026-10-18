# The story as the ROM tells it, and how "The Legacy of Hercule" could bend it

Source: the game's own caption cards and dialogue (ROM order follows play order closely), plus the
quest table. Chapter names are ours.

## 1. What is actually in the game (Android and Cell sagas)

1. **Prologue, Future Trunks.** "Present day Earth": Frieza and King Cold land; Trunks cuts them down
   and warns Goku about the Androids. "Sixteen years from the present day": the ruined future with
   Androids 17 and 18. The player is Trunks (Flight Circle tutorial).
2. **Three years later, Gohan.** Gohan at home (phone, Chi-Chi, the Mathbook), West City and Capsule
   Corp (Bulma, Dr. Briefs), the woods, Korin and Yajirobe (Senzu beans), Krillin's lines ("You let me
   win, didn't you?"). Errands: missing children, fishing bait, a newspaper, Hercule's autograph (!),
   Scouter parts, the Dragon Radar. Goku arrives ("Sorry I'm late... left turn at Mars").
3. **Androids.** Dr. Gero's lab with Blue/Red access cards and computer-room access, Androids 19/20,
   then 16/17/18 released; "You are now playing as Vegeta"; Vegeta levels; Piccolo joins (Namek
   lines, "Only Vegeta is allowed through this barrier" gates per character).
4. **Cell.** Ginger Town, Imperfect Cell, the absorptions, Perfect Cell; Cell Games with Hercule's
   public "victory"; Gohan's win; Cell's defeat dialogue, after which the script removes Goku and makes
   Hercule available (both one-byte-ish facts in the script).
5. **Side content.** Missing Nameks, Golden Capsules, "Eyes of the Lion" book, the Scouter minigame,
   Cooler's bonus area, technique unlocks per character (Super Kick, Spin Punch, Two Handed Smash).

Buu's Fury (separate ROM) continues seven years later through Majin Buu.

## 2. Decision (Ryan, 2026-10-17): "Canon plus Hercule"

The DBZ story stays beat for beat; Hercule is player 2 and is written into every scene as the man
who was also there. The twist is kept for the end: a surprise Super Saiyan Hercule beats Majin Buu,
and against Buu the fusion is Goku + Hercule instead of Goku + Vegeta. Vegito stays for Janemba
(Ryan's call; the film has Gogeta against Janemba, and whether Buu's Fury has a Janemba fight at all
is to be checked in that ROM). Hercule gains; nobody else loses their moment.

## 3. Beat sheet (LoG2 ROM) with the mechanism each beat uses

Cost tags: **text** = dialogue swap through `tools/script_dump.py` + `data/hercule-text.json`;
**actor** = place/replace a sprite in a map script; **byte** = one script flag such as Keep Goku;
**code** = new C in `src/coop`; **art** = new frames.

| chapter | Hercule's part | cost |
|---|---|---|
| Prologue (Trunks vs Frieza, King Cold) | Hercule is in the crowd at the landing site as player 2, "covering the story for the press"; his first line is that he had it handled. Open question: he replaces Trunks for player 1, or the game opens on him watching the news. | actor, text |
| Gohan at home / West City | The autograph errand becomes the running gag: the autograph is Hercule's, and he signs it himself in person. Krillin's "You let me win" line gets a Hercule reply. | text |
| Androids (Gero's lab, 19/20, 16/17/18) | Hercule "disarms" the lab door (the access cards are his idea); Android 18 ignores him, setting up Buu's Fury. Twin-boss excuse: Gero's spare unit. | text, actor |
| Vegeta chapter | Hercule follows Vegeta around as his "manager", Vegeta's lines sharpen. | text |
| Cell (Ginger Town, absorptions) | Townsfolk cheer Hercule, who claims each win; the Scouter minigame reads him at 5. | text |
| Cell Games | His public "victory" becomes partly true: he throws Android 16's head to Gohan (the beat the series already gives him), written so the player does it. Cell Jr. twins. | text, actor |
| After Cell | Goku stays (Keep-Goku byte). Hercule is unlocked as in the stock game and leads the credits scene. The transformation is gated by a story flag, not a level: the aura is a "gift" he does not understand yet. | byte, code |
| Side content | Animal mini-quest (sprite 136 row), Satan Dollars counter text, enemy taunts about the champion, manager phone calls through the Capsule Corp phone gag. | text, code |

## 4. Buu's Fury (phase 2, separate ROM)

- Hercule and Buu's friendship is already in the stock story; the surprise is that the aura from the
  Cell Games is real. When Goku needs a fusion partner against Buu, Hercule is the one who is there
  (as he is in the series, where Goku considers it): the fused form is a new row and sprite in the
  Buu ROM, built with the same cloner once that ROM has its decompile pass. Vegito stays for Janemba.
- The final blow on Buu is Super Saiyan Hercule's, with the Spirit Bomb beat kept as his crowd call.
- Needs: Buu's Fury decompile pass (party, display rows, saves), save conversion from LoG2, the
  fusion scene as a script edit, two new sprites (SSJ Hercule in the Buu ROM, the fused form).

## 5. Still open

- Prologue: Hercule replaces Trunks as player 1, or the game opens with Hercule watching the news?
- Super Saiyan look: gold afro (what we have) or a different "Super Human" look?
- Which Z fighters stay as player 1 per chapter (the Gohan chapters are most of the game).
- Name of the Goku + Hercule fusion in our text ("Gokule" is the fan name).
