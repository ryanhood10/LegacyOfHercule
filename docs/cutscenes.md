# Cutscenes: the guidelines (Ryan, 2026-11-06)

The contract for everyone who researches, writes or changes a cutscene. `docs/howto.md` "Stage a scene" has the
mechanics (rules 1-9); this page says what a good scene is, how a scene is improved, and how the work is split.
Per-chapter research lives in `docs/cutscenes/` (one file a batch); the toolkit of moves is section 5 (its "Cine" table: the
dash, strikes, the exchange, charges, blasts, the beam struggle, one call each).

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
(presence), `31` (player 2's body), `9` (place player 2), **`37` the cinematic helpers below**. (`EntitySetVelocity` is
misnamed in the reference notes: decompiled, it plays the dust explosion on the entity at an offset and moves nobody;
`LockEntityPose` sets his draw priority: `docs/scripts.md` 2.1. Knock-backs are Cine's op 5.)

**The cinematic helpers: "Cine"** (`src/coop/cine.c`, `cine.h`; built 2026-11-06, branch `nb-cine`): the moves scripts
cannot do well on their own. One bridge call, **`CoopCall 37, op, b, c`**. Scene writers use these instead of hand-rolling
them, so every scene's hits look alike and are tested once (`tools/headless/cine.txt`). The demo scene that plays every
op is `data/story/cine_demo.lscript` (filmed: `tools/headless/cine_demo.txt`, `docs/testing.md` "Cinematic helpers").

**How a move behaves in a script.**

- **It blocks** like a walk: the script goes on when the move is over (the frames column). Inside
  `BeginCommandBatch` / `CommitCommandBatch` one move is queued and plays with the batch's walks (one a batch; a second
  is dropped).
- **The cast first.** `CoopCall 37, 1, A, B` names body A and body B; every move after it is A's on B until the next
  cast. Bodies are looked up at each move, so a cast may be set before its actors are spawned.
- **Body codes:** `0` player 1 · `-1` player 2 (nobody when he does not stand) · `-10 - slot` the player who plays that
  party slot (`-10` Gohan, `-11` Piccolo, `-12` Vegeta, `-13` Trunks, `-14` Goku, `-15` Hercule, `-16` Krillin, `-17`
  16, `-18` Super Saiyan 2 Gohan; nobody when neither plays him) · a sprite id (7 or more): the first actor, NPC or
  enemy showing it, **never a player** · `-2` the point set by op 2 (a target only) · `-3` nobody.
- **Nothing crashes on a missing body.** Without A nothing plays and the script goes on at once. Without B a strike
  swings at the air and a blast flies 110 px ahead; a dash beside B, an exchange and a struggle do not play. Branch on
  presence with op 16 where the scene needs to (`CoopCall 37, 16, code, 0` then `JumpIfFalse`).
- **Poses come from the body's own record**, the first group of a list it has, else the idle stance: no move can
  animate a missing group (the table "What bodies lack" below).
- **Spots are fitted:** on walkable floor, the whole body in the camera's view and 30 px clear of its top (the HUD)
  unless op 13 has the camera follow. A knock-back against a wall or the view's edge is shorter.
- **Who stays down.** A heavy strike's, a thrown blast's and a struggle's loser, and op 10's kneel / lie, leave the body
  **held** in that pose after the move. It stays until op 10 rise, op 0 (everything ends), another move that uses it,
  or a script step that walks or animates it. A held player is freed by himself after 20 frames of live play, so a
  scene cannot cost a player his control; a held body does not turn in a look.
- **Start (player 1) skips the move in progress**: its timeline is run at once without effects, so everybody ends
  where and how they would have. A press skips one move.
- **Budget:** nothing runs per frame outside a move, a held charge or a flash. Effects are the game's own objects
  (0x400 of OBJ VRAM each while they play) and are left out when OBJ VRAM's largest free block is under 0xC00; an
  afterimage takes the body's frame size (0x400, 0x1000 for a 64 x 64 body). Everything a move makes is gone at its end.

**The ops** (`cine.h`). Frames are at 60 a second; "lunge" is 0 to 12 frames of A closing in when he is not beside B.

| op | call | what plays | frames |
|---|---|---|---|
| 1 cast | `CoopCall 37, 1, A, B` | sets body A and body B | 0 |
| 2 point | `CoopCall 37, 2, x, y` | sets the point (map px): cast it as B with `-2` | 0 |
| 3 dash to | `CoopCall 37, 3, x, y` | A flickers out, is at (x, y) in a white flash, his afterimage flickering where he stood for 9 frames; the take-off whoosh; he faces B there when B is a body | 15 |
| 4 dash by | `CoopCall 37, 4, side, gap` | the same to a spot beside B (a body or the point). side: 0 the side A is on, 1 left of B, 2 right, 3 above, 4 below, 5 behind B (away from where B looks), 6 in front of him; + 16 no afterimage. gap: px between their centres, 0 = touching | 15 |
| 5 strike | `CoopCall 37, 5, flags, back` | A lunges to B's side and hits him. Light (flags 0): the attack pose, the yellow star on B, B white for 4 frames in his hurt pose, the impact sound, a small shake, B pushed 6 px. flags 1 heavy: a 5-frame wind-up, the star and a white ring, a screen flash, a hard shake and the boom, 5 frames of hit-stop, then B flies `back` px (0 = 56) in an arc, lands in smoke with a thud and a shake, slides, **lies** (held). + 2 a kick instead of a punch; + 4 B ends **on a knee** instead of lying | light: lunge + 21; heavy: lunge + 48 |
| 6 exchange | `CoopCall 37, 6, frames, end` | the flurry: both vanish and meet in the middle (step in when close), then trade blows in beats of 4 to 8 frames: punches and kicks in turn, a star at each, every third lands (white, hurt pose, a shake), both shifting with the blows; a clash now and then (both swing, a white ring, they are driven apart and close again); in one of 100 frames or more both vanish with afterimages and meet again a little way off, about every 50 frames. frames: the flurry's length (0 = 120, 24 to 900). end: 0 they leap apart 24 px each in smoke, facing each other; 1 A lands the last blow, a heavy strike (B lies, held); 2 B does | about frames + 35 (end 0), + 65 (end 1 / 2); + 15 when they start apart |
| 7 charge | `CoopCall 37, 7, frames, level` | A powers up: his power-up pose (the stance before a transformation where his record has it, else the ki pose), the hum, white flickers that come faster, a shake that grows, smoke at his feet, yellow rings on the ground (level 1, 2), then the burst: a screen flash, a white ring, the transformation's burst sound. level 0 to 2 | frames + 12 |
| 7 held charge | `CoopCall 37, 7, 0, level` then `CoopCall 37, 7, -1, 0` | **does not block**: the pose, the flicker, a light rumble, smoke and the hum go on alongside the script, through text boxes too, until `-1` ends it with the burst. Do not move that body with another move meanwhile (it ends the charge) | 0 |
| 7 charge beside a transformation | `CoopCall 37, 7, 0, level + 4` then the body's own power-up, then `CoopCall 37, 7, -1, 0` | **the way a transformation is played** (Ryan, 2026-11-06: "you removed the normal sprite animation of them transforming super sayain (normal pace), add that back in alongside the explosions"): the held charge's effects on a 160-frame ramp (white flickers coming faster, a rumble growing, smoke at his feet by turns, yellow rings at the start (level 1) and every 30 frames (level 2)) while the body plays its own stock power-up at its own pace: an actor `SetEntityAnimation <base sprite>, 9` (with the stock's `PlayAudio 55` before and `PlayAudio 23` after, then the swap to the gold sprite), a player `SetEntityAnimation 0, 35` (the Transformation ability's own state: the hum, the gold, the form). No pose, no hold and no sound of its own; `-1` is the burst's light and shake without its sound. Only on bodies with group 9 (`docs/art-audit.md` section 1) | 0 |
| 8 blast | `CoopCall 37, 8, kind, n` | A fires at B (a body, the point; ahead without one). kind 0: a wind-up in the ki pose, one ki blast, the fire burst on B, B white and pushed 5 px. 1: a volley of n shots (0 = 5, at most 8), one every 6 frames from either hand, a burst each, the last one an explosion (dust, a ring, a flash, the boom). 2: a beam held on him for n frames (0 = 60): A steps level with B (up to 40 px), the charge pose, the Kamehameha out of his hands onto B, rings and bursts on B and shakes while it holds, B pushed back a pixel at a time; **a body without the beam pose (group 18) fires kind 3 instead**. 3: a big blast: the ball grows in his palm for 24 frames (rings, flickers), flies, explodes. kind + 16: B blocks (an arm out, no white, smoke at his feet); kind + 32: B is thrown (the knock-back; he lies, held) | 0: about 45; 1: about 40 + 6 n; 2: about 50 + n; 3: about 70; + 20 thrown |
| 9 struggle | `CoopCall 37, 9, frames, winner` | the beam struggle: the two dash apart to 116 px and level, charge with rings at their feet, fire; the beams meet and the clash sways back and forth with white rings, stars, a growing shake, flashes, for `frames` (0 = 180). winner 1: A's beam drives the clash onto B, an explosion, B is thrown and lies (held); 2: B's wins; 0: it blows up in the middle and both are pushed back, standing. Any two bodies can (the ki pose stands in for the beam pose) | frames + 95 (a winner), + 65 (0) |
| 10 pose | `CoopCall 37, 10, who, pose` | who 0 A, 1 B. pose 0 **rise**: he gets up from lying or from a knee (the fall played backwards) and stands idle; nothing when he is not held. 1 **kneel**: down on a knee, held. 2 **fall**: he falls where he stands, smoke and a thud, lies, held | 8 to 23; 10; 22 |
| 11 look | `CoopCall 37, 11, speaker, to` | everyone on screen but the speaker turns to him, players included; the speaker turns to `to` (a body, `-2`, or `-3` to leave him). Howto rule 7 in one call, before the line's text entry | 0 |
| 12 look at | `CoopCall 37, 12, target, except` | everyone on screen but `except` (`-3`: nobody is left out) turns to the target (a body or `-2`) | 0 |
| 13 camera | `CoopCall 37, 13, on, 0` | on 1: during the moves the camera follows the cast (the middle of A and B, the game's own smooth follow) and spots are no longer kept in the view; 0: it stays | 0 |
| 14 pan | `CoopCall 37, 14, target, speed` | the camera pans to centre on a body or the point (blocking; speed px a frame, 0 = 4): `CenterCameraOnChar` for any body code. Not inside a batch | by distance |
| 15 jolt | `CoopCall 37, 15, frames, flash` | does not block: a shake of `frames` (0 = 10), stronger with flash 0 to 3, and a white screen flash for 1 to 3 | 0 |
| 16 has | `CoopCall 37, 16, code, 0` | **pushes** 1 when that body stands, else 0 | 0 |
| 17 held | `CoopCall 37, 17, 0, 0` | **pushes** how many bodies are held in a pose of ours | 0 |
| 0 end | `CoopCall 37, 0, 0, 0` | a held charge ends (no burst), the flash, the camera's follow; every held body stands idle. Good manners at a scene's end | 0 |

**Copy and paste** (inside a `script` entry; `137` Vegeta's actor, `9` Android 17's, `159` Broly's):

```
; a line: everyone looks at 17, he looks at Vegeta (then the text entry)
        CoopCall 37, 11, 9, 137
; Vegeta powers up through his own line (a script entry before the text, one after)
        CoopCall 37, 1, 137, 9
        CoopCall 37, 7, 0, 1
        ...                             ; text 106 "VEGETA: ..."
        CoopCall 37, 7, -1, 0
; he vanishes, is beside 17, a punch, a kick
        CoopCall 37, 4, 0, 0
        CoopCall 37, 5, 0, 0
        CoopCall 37, 5, 2, 0
; a flurry of 170 frames, they break apart
        CoopCall 37, 6, 170, 0
; 17 fires five blasts, Vegeta blocks
        CoopCall 37, 1, 9, 137
        CoopCall 37, 8, 17, 5
; Vegeta appears behind him and knocks him 64 px; he lies; later he gets up
        CoopCall 37, 1, 137, 9
        CoopCall 37, 4, 5, 0
        CoopCall 37, 5, 1, 64
        WaitFrames 30
        CoopCall 37, 10, 1, 0
; a transformation: Goku's stock power-up (his animation 9) with the charge's effects alongside, the burst, the swap
        CoopCall 37, 1, 60, 12
        CoopCall 37, 7, 0, 6
        PlayAudio 55
        SetEntityAnimation 60, 9
        CoopCall 37, 7, -1, 0
        PlayAudio 23
        DespawnEntity 60
        SpawnCharacterEntity 61, 432, 408
        WhiteFlash 6
; a 4-second power-up, level 2, blocking
        CoopCall 37, 7, 240, 2
; a beam held 60 frames that throws him; a big blast at a point of the map
        CoopCall 37, 8, 34, 60
        CoopCall 37, 2, 420, 300
        CoopCall 37, 1, 137, -2
        CoopCall 37, 8, 3, 0
; the beam struggle, 150 frames, Vegeta's wins
        CoopCall 37, 1, 137, 9
        CoopCall 37, 9, 150, 1
; a villain's blast throws player 1; player 2 goes for him and is put on a knee; both get up
        CoopCall 37, 1, 159, 0
        CoopCall 37, 8, 35, 0
        CoopCall 37, 1, -1, 159
        CoopCall 37, 4, 0, 0
        CoopCall 37, 5, 2, 0
        CoopCall 37, 1, 159, -1
        CoopCall 37, 5, 5, 40
        CoopCall 37, 1, 0, -1
        CoopCall 37, 10, 0, 0
        CoopCall 37, 10, 1, 0
; only when Vegeta is played (party slot 2): he steps in himself
        CoopCall 37, 16, -12, 0
        JumpIfFalse novegeta
        CoopCall 37, 1, -12, 9
        CoopCall 37, 5, 1, 0
      novegeta:
; a body falls where it stands; another drops to a knee
        CoopCall 37, 1, 9, 137
        CoopCall 37, 10, 0, 2
        CoopCall 37, 10, 1, 1
; the camera follows a long exchange, then stays
        CoopCall 37, 13, 1, 0
        CoopCall 37, 6, 300, 2
        CoopCall 37, 13, 0, 0
; a move with a walk: player 1 walks while Vegeta dashes to 17's left
        BeginCommandBatch
        WalkToPosition 0, 314, 398
        CoopCall 37, 4, 1, 0
        CommitCommandBatch
; everyone looks at where the explosion was; a jolt; the camera on player 1; the end
        CoopCall 37, 2, 330, 300
        CoopCall 37, 12, -2, -3
        CoopCall 37, 15, 20, 2
        CoopCall 37, 14, 0, 4
        CoopCall 37, 0, 0, 0
```

**What bodies lack** (`python -I tools/cineposes.py build/<dir>/log2-coop.gba` lists every sprite of a build; a missing
pose is the next of its list, at last the idle stance, never a crash):

| bodies | what they show |
|---|---|
| player layouts with everything: Gohan 53 / 56 / 59, Goku 60, Hercule 72, Piccolo 103, Trunks 128 / 132, Vegeta 137 / 138, Paragus 170, haloed Goku 176 | every pose; the charge is the stance before the transformation (group 9's first two frames) |
| the other player layouts: 55, 58, 61, 104, 134, 139, 140, Krillin 161, old Hercule 158, Broly 159, Mr. Popo 163, the Namekian 166, 16 the player 175, 177, Super 13 180, the tournament bodies 185-189 and 235-244 | the charge is the ki pose (group 5); **no beam pose on 55, 57, 135, 158, 159, 163, 175, 180**: their "beam" is the big blast (in a struggle they fire a beam from the ki pose) |
| the androids and bosses on enemy sets: 17 (9), 18 (11), 19 (12), Gero 14, Cooler 34, 13 (165), 16 the boss 160, Yamcha 156, Tien 121, Krillin's actor 76 | punches, kicks, the ki pose, the fall, lying; no dash pose (they run), no beam pose (a big blast); 76 and 121 punch with group 6; 19 and Cooler have no fall (they reel in the hit-stun, then lie) |
| the Cells: 28, 29, 30, the second Cell 162 | everything but the dash pose (they run); the charge of 28 and 29 is the stance before their own transformation (group 9's first two frames, a front view), of 30 and 162 the ki pose |
| Android 16's actor 8 | one attack (group 6) for punch, kick and ki; no charge pose (idle), no beam |
| Cell Jr. 31, Frieza 48 | no fall (the hit-stun stands in: he reels, then "lies" in the hit-stun's first frame) |
| King Cold 74, Mecha Frieza 49 | no attack and no hurt pose: they stand through a strike (the effects still play); they fall and lie (group 3); King Cold has no ki pose either |
| Gero's actor 15, haloed 62, the announcer 230, town NPCs and animals (1 to 7 groups) | the idle stance for everything; birds 63 / 64 and animals strike with their one attack (group 6). Use them as targets and onlookers, not as fighters |

**Sounds** used (stock ids): the dash's whoosh 26 (the stock flights' take-off), impacts 34-36, the boom 20, a ki
launch 13 / 14, the power-up hum 55 (stopped at the charge's end), the transformation's burst 23. Nobody has listened to
them in place yet (the tests are headless): a scene writer who hears a wrong one changes the `SFX_*` line in `cine.c`.

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
