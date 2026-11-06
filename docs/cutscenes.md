# Cutscenes: the guidelines (Ryan, 2026-11-06)

The contract for everyone who researches, writes or changes a cutscene. `docs/howto.md` "Stage a scene" has the
mechanics (rules 1-9); this page says what a good scene is, how a scene is improved, and how the work is split.
Per-chapter research lives in `docs/cutscenes/` (one file a batch); the toolkit of moves is section 5.

## 1. Why

Ryan: "i think whats REALLY gonna draw attention to the game is captivating, improved cutscenes ... people moving
and not always stagnant, turning to face towards whos talking or what makes sense, a big bonus for animations like
energy blasts, a fight thats NOT basic, like a REALLY cool cutscene fight ... we can take some of the current
cutscenes that are already in the game and spruce them up - as some of them are bland ... REALLY put some effort into
it ... the cutscenes need to be dynamic depending on which player or players have been selected ... i dont want
anything tacky or cheesy - i want it true to the badass feel of Dragon Ball Z and keep the characters true to the
anime." He shows scenes to fans as YouTube Shorts before the game is out, so a scene should also read well as a
20-40 second clip. The Time Chamber's opening scene is the standard he liked: clean, polished, it shows instead of
tells.

## 2. Improve, do not replace

1. **The story stays.** No plot change, no new character in a scene the story does not put there, no change to who
   wins, who leaves, what flags are set. A scene is improved in how it is shown.
2. **Stock lines stay as written; Ryan's lines stay word for word** (`docs/story.md`, `docs/tournament-dialogue.md`
   mark them). A line may be moved to a present speaker by the presence rule only as that rule allows.
3. **Add by showing.** Prefer a move, a look, a blast, a camera pan to a new line. A new line is rare, short, in the
   character's voice, and marked DRAFT in a comment and in the batch's research file.
4. **Keep what works.** A beat that already reads well is left alone. Do not lengthen a scene for its own sake: a
   scene may grow by a few seconds for a real moment, never by a wait.
5. **Every change is one commit per scene** (script + its test), so Ryan can drop any one of them.

## 3. What makes it Dragon Ball Z, not cheesy

- **Characters are themselves.** Vegeta is proud and curt, never chatty or silly. Piccolo is dry and watchful.
  Goku is warm and eager for a fight. Gohan is polite until pushed. Trunks is earnest and tense. Krillin is scared
  and goes anyway. 16 says little. 17 is amused, 18 is bored, both dangerous. Cell is theatrical and polite. Frieza
  is cruel with manners. Broly barely speaks. 13 is our truck-driver brawler. Mr. Popo speaks of himself by name.
  **Hercule is the comic coward and braggart in everyone's eyes; nothing reveals him as the hero before the Buu
  phase.** His comedy is his own bluster, never slapstick put on the others.
- **Power is shown, then spoken.** The ground shakes before the villain speaks. A fighter powers up (the flash, the
  aura sound, a shake) before his threat. Silence and a held look are DBZ; a quip after every line is not.
- **Hits have weight.** A blow is a dash, an impact (flash, hit effect, shake), a knock-back, dust where he lands. A
  blast is a wind-up pose, the shot, the impact. Nobody slides politely to a mark and then announces he was hit.
- **Speed is DBZ's signature:** vanish and reappear (the afterimage dash), exchanges too fast to follow, then a hold.
- **No jokes with the fourth wall, no memes, no modern slang, no winking at the player.** Nothing "cute" on villains.
- **Reference the anime's own staging** for a scene the anime has (who stood where, who struck first, the famous
  pose). When the game's stock scene departs from the anime, the game's story wins; borrow the staging only.

## 4. Dynamic by who plays (the hard part)

- **Presence (howto rule 2):** a party member nobody plays is not in the scene. One player: only his character. Two
  players: their two. Story characters the plot puts there always are.
- **Every scene lists its lineups** (from the `SetCharacterFlag_Bit1` / `P2Bind` calls before it) and is written for
  each: who takes which beat, which lines fall away, where each player's body stands and what it does (players are
  moved as char 0 and through `CoopCall 31` / player 2's entity, never by a sprite a player may show).
- **A scene's big moment never depends on an absent character.** If the anime's beat belongs to someone who may not
  be played, the villain's side of it carries the scene (he still powers up, still fires), or the played character
  takes a version of it that fits him.
- **Players are part of the choreography**: they turn, step back from a blast, brace, get knocked down and rise when
  the story has them hit; they are not parked at the bottom of the screen.
- **A cutscene fight never plays the player's fight for him.** Choreographed exchanges are between story actors, or
  a short opening / closing exchange around the real fight (the first clash, the finishing blow). Control comes
  back where it does today.

## 5. The toolkit

Script steps (lscript names): camera `PanCameraToPosition`, `CenterCameraOnChar`; light `WhiteFlash`,
`FadeToWhite` / `FadeFromWhite`, `FadeOut`; `ScreenShake`; sound `PlayAudio`, `PlayMusic`, `StopMusic`; bodies
`SpawnCharacterEntity`, `WalkToPosition`, `FlyToPosition`, `SetEntityFacing`, `SetEntityAnimation`,
`SetEntityPositionInstant`, `EntitySetVelocity` (knock-backs), `EntityStopMovement`, `BeginCommandBatch` /
`CommitCommandBatch` (together), `EntityWaitTouchPlayer`; effects `PlayDamageEffect`, `SpawnDustCloud`,
`NPCFireKiBlast`, `NPCFireKiBlastAtEntity`, `NPCFireKamehameha`; C calls `CoopCall 16` (a held beam), `22`
(presence), `31` (player 2's body), `9` (place player 2).

**The cinematic helpers** (`src/coop/cine.c`, CoopCall in `docs/ids.md`; built 2026-11-06, see "Cine" below once it
lands): the moves scripts cannot do well on their own: the afterimage dash, a melee exchange between two bodies, a
charge-up, a beam struggle, a struck body's flight and landing. Scene writers use these instead of hand-rolling them,
so every scene's hits look alike and are tested once.

**Traps (all have bitten us):** an animation whose group the sprite lacks overwrites the heap and crashes later
(check groups in `docs/sprites.md`; test `ents` vtables with `staging_run.py`); a sprite-addressed step on a sprite
nobody shows is a null write; a reload of a map mid-scene drops actors (spawn them again); `stop` counts as an entry
and a trigger's last stop must be `stop 0x0`; stock indexes are absolute (added entries go after the end); in C
`"\x0bD"` eats the letter; a text box on a full map can take bodies off the list while it is open; timing changes
shift tests that sit near a hit (step through the pick-up box).

## 6. How the work is split

- **Research** (`docs/cutscenes/<batch>.md`): for every scene of a batch: where it is (file, sequence, map), the
  beats today (from a headless film), the lineups, how the anime staged it, a rating (leave / touch up / rework /
  showcase), and for the ones to change a shot list in toolkit terms with each lineup's variant, its risks, its
  size. Research changes no game data.
- **Development**: one agent a batch, in its own worktree, builds the research's shot lists in priority order
  (showcase first), one commit a scene, with a lineup test and frames looked at, full regression at the end. A
  developer who finds the research wrong fixes the research file in the same commit.
- **Review**: the main session merges, runs the combined regression, and lists every changed scene for Ryan with how
  to see it (`docs/review-*.md`, the test drop-ins).

## 7. Done means

Every lineup of the scene filmed and looked at; `ents` clean; no new wait before control; A / Start skip what they
skipped; the chapter's staging test extended; full regression clean; the scene's row in `docs/scripts.md` and the
batch file updated; DRAFT lines listed.
