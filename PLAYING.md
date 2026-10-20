# Playing The Legacy of Hercule

A short guide for the people playing it. How it works inside is in `README.md` and `docs/`.

## 1. Build the game

From Git Bash in the repo (the toolchain line is in `CLAUDE.md` "Build"):

```
make
```

That writes `build/log2-coop.gba`. It needs `roms/log2.gba` (Legacy of Goku II, USA). Put
`roms/buu.gba` (Buu's Fury) next to it too: without it, old Hercule, Broly and Super 13 fall back to
stand-in art.

Play the plain `make` build. The test flags (`DEBUG_MENU`, `WIDE`, `P2_MODE=1/2`, `SSJ_TEST`, `A16_TEST`,
`KR_TEST`, `SSJ2_TEST`, `STORY_TEST`, `HERCULE=0`) are for development only.

## 2. Connect the second player

**Same PC (two people, one screen).** In mGBA 0.10.5:

1. File > Load ROM: `build/log2-coop.gba`.
2. Tools > Scripting, then run `dofile("F:/MeltyCreations/log2/tools/mgba/coop.lua")`.
3. In a terminal: `python tools\p2_keys.py`.

Player 1 uses mGBA's own keys or gamepad. Player 2 plays on the keyboard: I J K L move, O = A,
U = B, `[` `]` = L / R, N = Select, M = Start. The keys work while any window has focus. For a second
gamepad, map it to those keys with a gamepad-to-keyboard tool.

**Two PCs (online).** Use lockstep netplay; the steps are in `README.md` "Playing online". Both
players must build with the same flags. The host's save is the one that counts.

**1 or 2 players.** Every time a game starts (new or loaded), the first thing it asks is "How many players?"
(A: 1 Player, B: 2 Players). 1 Player keeps player 2 out even if a second controller is connected. With 2
Players, player 2 joins whenever their controller sends input, and leaves a few seconds after it stops.

## 3. Starting a new game

After the players question, a new game asks for the difficulty. It is set for the whole game.

| | Easy | Normal | Hard |
|---|---|---|---|
| enemies (two players) | normal HP, hit softer, no twin grunts | 1.5x HP, hit 1.25x, twins | 2x HP, hit 1.5x, twins |
| enemies (alone) | hit softer | as the original game | 1.3x HP, hit 1.2x |
| bosses | 80 % HP | as tuned | 125 % HP |
| time to revive a partner | 20 s | 10 s | 6 s |
| item drops | more | normal | fewer |

## 4. Controls

| button | does |
|---|---|
| A | punch, talk, confirm. Hold it beside a downed partner to revive them |
| B | the selected ability (ki blasts, beams...) |
| a direction twice | dash |
| L (player 2) | cycle player 2's ability; player 1 picks abilities as in the original game |
| Select (player 2) | switch player 2 to the next available character (while player 1 is free to move) |
| Start | pause menu: status, items, journal (L / R to change page) |
| A + B + Select + Start | soft reset |

Avoid holding L + R for the first ten seconds after switching the game on: that's a hidden mode for a
link-cable controller.

**Save pads** offer Save and Switch Characters. Player 1 chooses there. Taking player 2's character
swaps the two of you.

## 5. Co-op rules

- **Twins:** with two players, most enemies come with a twin, and bosses have a partner.
- **Going down:** at 0 HP you collapse instead of dying. A red bar with the seconds left counts down
  over your body. Your partner stands next to you and holds A for 3 seconds; a yellow bar fills and you
  glow. Then you're back up with 30 % HP. If the partner lets go or gets hit, they have to start again.
  The countdown running out, or both of you down, is a game over. Alone, 0 HP is the normal game over.
- **Story pairs:** some fights are always two bosses: the twin Cells, two Android 19s, Broly and
  Paragus, and others. Alone, each of them has half its health.
- **The world map:** whoever steps on a flight sign flies, and the other rides along.
- **Duels:** some story scenes pit player 1 against player 2. If player 2 wins, the controllers swap
  roles from then on.

## 6. The story, briefly

The future prologue with old Hercule. West City, where Hercule joins. Vegeta's duel with his clone. The
Androids. Piccolo and Kami. Gingertown and Cell. The Time Chamber. Then the ten-day wait before the Cell
Games, with optional side trips: Roshi's gym, Cooler on New Namek, Broly, Time Chamber training, and
sparring at Roshi's dojo. Krillin and Android 16 go back to Dr. Gero's lab. Then the Cell Games, the
wishes, a climb through the Arctic, and a final fight. Afterwards Dende asks whether to begin
"Peace Times": answering yes twice plays the ending. The journal (Start, then the Journal page) always
says where to go next, and a star on the world map marks the place.

## 7. Good to know

- Save often. The Cell Games run fight after fight with no save pad and no healing between fights; break
  the white rubble in the arena for health and energy.
- The Broly trip has no save pad and no way back until Broly falls. Go when you're ready.
- Losing to the Arctic's final boss sends you back to the last save pad, before the cutscene.
- Alone, characters who would be player 2 still talk in some scenes, off screen.
- This has only been tested by automated runs. Note anything odd (what you did, where you were) and
  report it.
