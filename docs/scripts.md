# Story scripts: formats, tools, and the chapters built with them

How the game's map scripts, dialogue and map records work (decompiled from the ROM, checked by
assembling every stock script back to identical bytes), the text form we edit them in, the map
patcher that applies story specs, and the chapters implemented so far. Related: `docs/maps.md` (map
format, new maps), `docs/story.md` (what the story is), `docs/testing.md` (how chapters are tested).

## 1. Tools

| tool | what it does |
|---|---|
| `tools/script_asm.py` | the lscript text form: `dump map ZONE AREA` / `dump script ADDR` / `dump seq ADDR` write it, `asm FILE --free OFF` assembles a file into free ROM, `roundtrip` checks every stock script and sequence |
| `tools/mapedit.py` | applies story specs (`data/story/*.json`): triggers, records, map scripts, line edits; `show SPEC...` prints the records it would write |
| `tools/romdata.py` | a file in `data/hercule.json`'s `include` list that holds an object (not a list) is a story spec: it goes through `mapedit` (all specs share one editor, in include order) into `"story_free"` (0xB00000..0xC00000) |
| `tools/script_dump.py` | the older read-only dumper (byte offsets beside each instruction) |
| `tools/headless/st_headless.c` | headless test runner on the fork's libmgba (section 7, `docs/testing.md`) |

Round trip (`python tools/script_asm.py roundtrip`): 563 scripts and 367 dialogue sequences (3068
entries: 1239 script, 953 text, 781 compressed text, 39 format, 51 compressed format, 5 jump, plus
328 segment stops) reached
from the map table (name, music, variation, on-load, on-leave scripts, trigger conditions and
payloads, record conditions, NPC dialogue, enemy defeat actions, item scripts, chest dialogue) and
the quest table: dump, parse, assemble, compare: **0 failures**.

## 2. Formats

### 2.1 Bytecode (`BytecodeVM_ExecuteScript` 0x080092E4)

Main dispatch (0x083B5C00): 0x00 PushByte u8, 0x01 PushVarint, 0x02 Step u8 (game action table
0x083B5D28, 145 entries; names in `script_dump.py STEP_NAMES`, from the reference notes), 0x03 TypeOp
u8, 0x04-0x10 And Not Negate Add Sub Mul Div CmpEq CmpNe CmpGt CmpGe CmpLt CmpLe, **0x11 END**,
0x12 Jump, 0x13 JumpIfFalse, 0x14 Pop, 0x15-0x1A StepTypePost_* u8 u8, 0x1B PushToAltStack (loop
counter), 0x1C LoopOrJump, 0x1D PushMulti u8 count + varints.

- **Jumps** (decompiled 0x080090FE, 0x0800910A, 0x0800926C): the operand is an **unsigned** byte.
  Jump and JumpIfFalse go **forward** to (address after the operand) + byte; LoopOrJump decrements
  the counter pushed by PushToAltStack and, unless it reached 0, goes **back** to (address after the
  operand) - byte. A script can continue past an END when a jump targets beyond it.
- **0x05 is a binary OR** (decompiled 0x08008FE6: pops two values, pushes 1 when either is nonzero),
  though the tools print it `Not` (the reference notes' name); stock conditions join cases with it
  (`A; B; Not` = A or B). There is no unary NOT: use the `StackTestStoryFlagNot` form.
- **Varints**: 7-bit groups MSB first, bit 7 = more; then u even -> u/2, u odd -> -(u/2). Every stock
  varint is minimal (the assembler can rely on it; the disassembler would print `.bytes` otherwise).
- **Arguments**: actions pop their arguments; the stock scripts push them with PushByte (one value
  0-255), PushVarint (one other value) or PushMulti (several), then Step. The lscript macro form
  writes exactly that.
- A script entry (mode 0) or a condition script leaves its result on the stack.

Useful action facts found on the way (decompiled):

| action | fact |
|---|---|
| 104 DespawnCharacterAndRecordPosition (char, record) | removes the entity showing sprite `char` and spawns map record `record` (any handler, its condition ignored) at its position and facing: how a stand-in actor becomes the boss |
| 84 TriggerMapEvent (record) | spawns map record `record` ignoring its condition (records with condition 1 exist only for this) |
| 73 SetCharacterTransformation (slot, sprite) | the second argument is a **sprite id**: the display row showing that sprite becomes the slot's row (assertion "unable to find sprite index in character display data" when none does) |
| 79 / 91 SetCharacterInPartyFlag / ClearCharacterInPartyFlag (slot) | party entry flags bit 0 |
| 80 / 81 SetCharacterFlag_Bit1 / ClearCharacterFlag_Bit1 (slot) | bit 1 = available (switchable); stock chapters lock characters with it |
| 90 DrawBossHealthBarRange (char, lo, hi) | `BossHealthBar_Create(0, entity, lo, hi)`: the bar draws the entity's HP over [lo, hi] % |
| 86 SetPlayerVisible (b) | not the player's visibility: `0x080040AA` sets party flags +1 bit 1, the HUD meter shown; both players stay on screen in scenes (stock scenes spawn a character's actor only when the active character is someone else) |
| 31 PanCameraToPosition (x, y, speed) | x, y is the view's **top-left** corner, not its centre (the camera starts at spawn - (112, 76)) |
| 52 DespawnEntity (sprite) and the other sprite-addressed steps | no entity showing that sprite = a write through a null pointer (`0x0800E04C` appends to entity + 0x150): guard them as the spawns are guarded |
| 137 NPCFireKamehameha (char, frames) | the entity showing sprite `char` (0 = player 1) plays animation 30 (sprite group 18: frame 0 for 8 frames, then 1 / 2 every 8) and fires the Kamehameha beam 8 frames in from its position + a per-facing offset (left -9, +2; right +10, 0; up / down 0, 0) for `frames` frames; Hercule (sprite 72) has the pose since `tools/herculesprites.py` (`docs/sprites.md` "Hercule's beam pose") |
| 38 / 39 WarpToMapWithEntities / NoEntities (zone, area, variation, x, y) | NoEntities loads the map without its records (the island's area 5 has no 16 after the stage-1 warp) |

### 2.2 Dialogue sequences and entries

A sequence is an array of entry pointers; a word whose top 16 bits are 0 stops the conversation. An
entry starts `u16 next, u8 mode`; `next` is the index run after it. Entries are 2-aligned; modes 2-5
need 4. **A script entry that leaves one value on the stack sets the next index instead**: stock
talk scripts branch with it (Chi-Chi's returns 1, 3, 5 or 7 by the story flags), so **an array
holds several segments**, each ended by a stop word, reached only through such returned indexes or
`next` fields. The reader follows both and keeps reading past a stop word while a reachable index
lies beyond it (lscript `stop N` lines); the last stop word is the sequence's `end=`. Indexes inside
scripts are absolute: a `stockseq` that moves a multi-segment range would break them (none of the
chapters does).

| mode | layout after the 3-byte header |
|---|---|
| 0 script | bytecode to END (no text box, runs at once) |
| 1 text | u8 speaker, NUL-terminated text |
| 2 compressed text | u8 speaker, JCALG1 stream (word-aligned; `tools/mapchunks.py decompress`) |
| 3 format | u8 speaker, u32 pointer to the format string (`%s`, `%d`), the argument script inline at +8 |
| 4 compressed format | u8 speaker, **u32 pointer to the argument script**, the compressed format text inline at +8 (decompiled 0x0800B396; the reference notes had +4 and +8 the other way round) |
| 5 jump | u8 pad, u32 target |

Speaker = portrait and name box (0 narration, 1 the active character's box, 3 / 90 Piccolo, 26 16,
28 17, 29 18, 38 Cell, 44 Cooler, 49 Frieza, 57 Future Gohan, 63 / 66 Goku, 67 Hercule, 73 Krillin,
80 Roshi, 81 Mayor, 95 Tien, 98 Trunks). The first character picks the box position: `!` top, `#`,
`@` and `@^` (captions) as the stock lines use them. `\x0b` is a page break; a page holds about 60
characters in three lines.

### 2.2b CoopCall, the script -> C bridge (`src/coop/vmcall.c`)

Game action 59 (`Deprecated_CharacterSpecialAttack`, an assert that no stock script calls) is repointed by a `data/hercule.json`
patch to `vm_coop_call`. `CoopCall fn, a, b, c` always pops four values; the queries push one result
(use them only where the value is consumed: a value left at a dialogue script's END is a next index).
The bridge first sat on action 130 (`sub_0800AFD4`), wrongly taken for unused: nine stock on-load
scripts (maps in zones 2, 7, 8, 10, 12, 13, e.g. Gero's area 8/13) call it to add an object made by
`0x0801C4B0` to the scene, and with the repoint they popped an empty stack. Unused actions (no stock
script, `walk_corpus` histogram): 1-3, 6, 7, 10, 18, 23, 32, 48, 59, 61, 66, 68, 105-108, 118, 120, 128,
131-133, 138; 59, 61 and 66 are the engine's own "deprecated" asserts.

| fn | call | does |
|---|---|---|
| 1 | DuelStart slot, flags, seq:END | player-vs-player duel (`duel.c`): slot = player 2's party slot, or -1 = a clone of player 1 (the clone slot is made available so player 2 spawns into it); flags 1 clone, 2 shadow tint, 4 swap the pads when player 2 wins, 8 full energy; END = sequence run when the duel ends (queued like an enemy's defeat dialogue, outside the damage code). Without a player 2, player 1 wins and END runs at once |
| 2 | DuelClear | after the end scene: player 2's slot and colours back (`duel_clear`) |
| 3 | DuelWinner -> 1 / 2 | winner of the last duel: `CoopCall 3, 0, 0, 0; PushByte 1; CmpEq; JumpIfFalse L1; PushByte 1; END; L1: PushByte 2; END` picks entry 1 or 2 |
| 4 | HerculeAwaken | Super Hercule into his abilities (`ssj.c`, the Buu phase) |
| 5 | P2Present -> 0 / 1 | is player 2 there: its entity or its pad (`p2_present()`, what the co-op twins spawn by; a warp without entities removes the entity until player 1 is free) |
| 6 | P2Bind slot | player 2's entity plays that slot now (the entity survives warps with entities, so after a duel the story rebinds it); after a warp without entities player 2 is gone until player 1 is free again, and the respawn takes this slot when it is selectable |
| 7 | PadSwap | the two pads trade players (`coop2.pad_swap` toggles, as when player 2 wins a duel with the swap flag): Perfect Cell, where the pad that played Vegeta takes Krillin and Trunks' pad becomes player 1 |
| 8 | P2Slot -> slot / -1 | player 2's party slot (the last bound one while a warp has the entity despawned), -1 without player 2: Roshi's gym checks whether either player is Hercule or Krillin |
| 9 | Stage op, b, c | Broly chapter staging: op 0 = `b` (seq:NAME) is the dialogue a `BOSS_RAGE` survivor runs when its partner falls (kept in `call_end_seq`, which a duel start overwrites); op 1 = player 2 stands at (b, c) px facing up |
| 10 | PartyKeep op [-> slot] | op 0 keeps the party's selectable bits (slots 0-7), Gohan's in-party bit and who players 1 / 2 are in story flags 404-419; op 1 puts the bits back and pushes player 1's kept slot (follow with `Step SetActiveCharacter`); op 2 binds player 2 to its kept slot |
| 11 | A16Join | "Gero's computer" (`a16player.c`): Android 16 joins in the last party slot (row 25, Krillin's EXP, stats scaled from Krillin's, abilities 22 / 19 / 0 / 2), every other slot but Krillin's loses its available bit (saved in flags 430 + slot), flag 420. The script then sets `SetActiveCharacterNoReset 6` and `CoopCall 6, 7` |
| 12 | A16Leave | 16 leaves the party for good (slot flags 0), the saved bits come back, Krillin and Hercule in the party and available, flag 421. The script then sets Krillin player 1 and `CoopCall 6, 5` |
| 13 | WaveStart seq:ROUND, first | the Time Chamber training (`waves.c`): from round `first` (0 / 1 = the first); ROUND = the sequence queued before every round and once after the last (it asks fn 14 which) |
| 14 | Wave op | op 0 -> the round being announced / fought (11 = completed); op 1 spawns the announced round (no result); op 2 -> player 2's character name (a `%s` argument: "%s wins the sparring match!") |
| 20 | Ssj2Join | **proposed number** (the Super Saiyan 2 Gohan piece; the main session assigns it): Super Saiyan 2 Gohan joins in party slot 8 (`ssj2gohan.c` `ssj2_join`: row 28, Gohan's EXP, Gohan's HP / EP x1.5 and str / pow / end x1.7, flags 0x7F, abilities 3 / 4 / 23 / 2; a second call does nothing). The script then shows the message: `CoopCall 20, 0, 0, 0` in a `script` entry, then `text 0 "@^Super Saiyan 2 Gohan is now an available character!"` (`tools/headless/ssj2_join_test.lscript`) |

### 2.3 Map records and triggers (`Map_LoadInternal` 0x0800C390)

MapEntry (0x38 bytes, table 0x08409368; `docs/maps.md` has every field): +3 trigger count, +0x10
triggers, +4 record count, +0x14 records, +0xC name, +0x24 music, +0x28 variation script, +0x30
on-load script, +0x34 on-leave script (run when the next map loads).

**Condition word** (records +4, triggers' first word, items, chests; `FUN_08003F06`, re-implemented
in party.c): 0 always; 1..0xFFFF never (1 = "spawned by a script"); 0x1nnnn flag n set; 0x2nnnn flag
n clear; 0x3nnnn flag n set and consumed; else a condition script.

**Triggers**: `{u32 condition, data*}`; data = `{create fn, activation getter, s16 x1, y1, x2, y2,
payload}`, created at map load when the condition holds (so flags must be set before the map loads).

| create / activation | kind |
|---|---|
| 0x0800D7E1 / 0x0800D765 | dialogue, on entering the rect |
| 0x0800C2C3 / 0x0800D765 or 0x0801087B / 0x0801087F | script, on entering the rect |
| 0x0800C2C3 / 0x0800D839 | script, **A pressed inside the rect** (the island's area-5 exit) |
| 0x0800D7E1 / 0x0800D87F | dialogue, **A pressed while facing the activation object's direction** (object 0x0300209C, byte +4; check 0x0800D844): doors such as the Lookout's chamber door (zone 9 area 3 trigger 0) |
| 0x0800D7E1 / 0x0800D765 via mapedit `"kind": "dialogue"` | a new dialogue trigger on a map that has none to copy (`add_triggers`) |
| 0x08010AC7 / 0x0800D839 | save pad (16 bytes, no payload) |
| 0x0800C233, 0x0800D9C3 | exits (door / walk-off); the map's always-on exits are the variation's (`docs/maps.md`) |

**Records** (handler first): town NPC 0x0800B711 `{handler, condition, x, y, dialogue*, sprite,
count, behaviours[]}` (talkable); actor 0x0800D6DF `{handler, condition, x, y, sprite, count, ptr}`
with its behaviour inline at +0x18; enemy 0x0800E77F `{handler, condition, stat, x, y, sprite,
action*, count, behaviours[]}`, action = `{0x0801069B, script}` / `{0x080106B3, sequence}` /
`{0x08010699, -}`, fired by the death class. Behaviour `{0x0800CB0F, facing << 16 | ticks}` = face and
wait. Scripts find entities by **sprite id** and spawn records by **index**, so records are never
deleted (mapedit disables them) and new ones are appended.

### 2.4 Party flags and player 2

Party entry +0x14 flags: bit 0 in the party, bit 1 available, bit 2 the special melee page, bits 3 / 4 the
two energy technique pages, bit 5 the charged A attack, 0x40 on every stock row. Player 2 (`src/coop/player.c`) is any
party member with both bits other than player 1's character, keeping its previous choice, else
Hercule, else the first; it is chosen when player 2's entity spawns (map load). So the story picks
player 2 with the flag opcodes (old Hercule in the prologue, Krillin from his visit, the jungle
lock), and a stock chapter lock applies to player 2 too.

### 2.5 Story flags

Stock scripts use flags up to 306. `Party_Init` clears flags 0-399 at a new game and `party.c` also
clears 400-479 (the ten-day wait's side chapters); flags 480+ survive a new game, so avoid them.
**The allocation by range and every flag's meaning are in `docs/ids.md` "Story flags"**; take new
flags there. Chapter-specific notes: 360 Krillin has visited, 361 inside Triceratops Jungle, 362
Hercule's deal, 363-375 Roshi's gym rounds, 380 the arctic barrier, 400-419 the Broly chapter (404-419
the party kept by CoopCall 10), 420-437 Gero's computer, 440 Roshi's dojo booked for a spar. The bank
is at `0x0203F00C + PARTY_SLOTS * 0x1C` (0x0203F108 with the default 9 slots; headless `FLAGS`).

## 3. lscript, the text form

```
; comment to the end of the line (outside strings)
script NAME [@08xxxxxx]                    a bytecode script; @ADDR = where the stock copy lives
    WalkToPosition 0, 96, 176              macro: push the arguments, then Step WalkToPosition
    PushByte 60 / PushVarint -3 / PushMulti 1, 2, 3 / Step NAME|N / TypeOp N
    And .. CmpLe, Pop, PushToAltStack, StepTypePost_* NAME, T, END, .bytes HEX
  loop:                                    labels; Jump / JumpIfFalse LABEL (forward), LoopOrJump LABEL (back)
    JumpIfFalse loop
end                                        END is appended when the last instruction is not END

sequence NAME [@08xxxxxx] [end=N]
  [0] script [-> NEXT] [@ADDR]             entry lines: optional [index] (checked), KIND, ARGS,
      SetPlayerVisible 0                     -> NEXT (default index + 1), @ADDR; instructions follow
  text 63 "GOKU: Oh, hi!"                    text SPEAKER "..."              mode 1
  textz 1 @083b9410 "..."                    textz SPEAKER @A "..."          mode 2 (stock)
  fmt 113 fmt=@083b5fc0 "Hi, %s!"            fmt SPEAKER [fmt=@A] "..."      mode 3, argument script below
  fmtz 1 @A args=@A "..."                    fmtz SPEAKER @A args=@A "..."   mode 4, argument script below
  jump 0x83bb5f4                             jump TARGET [pad=N]             mode 5
  stop 0x3                                   a stop word between segments (default: the next index)
  stock @083b9d16                            the ROM's entry at that address, with this sequence's next index
  stockseq @083e6078 1 [71]                  entries FROM..TO of a stock sequence, next indexes shifted
end
```

Instruction arguments may be `seq:NAME` / `script:NAME`: the address of that block (mapedit places it
first), e.g. `CoopCall 1, -1, 15, seq:ch3_space_end`. Strings take `\xNN` (`\x0b` page break), `\"`, `\\`. The assembler never compresses: a `textz` /
`fmtz` whose text still equals the stock entry at its `@ADDR` keeps the stock bytes, an edited one
becomes mode 1 / mode 3. Anything whose assembled bytes equal the stock bytes at its `@ADDR` is not
written again. **Committed lscript files restate no stock text**: unchanged lines are `stock` /
`stockseq` references; only our new lines and the scripts we edit are spelled out.

## 4. Story specs (`tools/mapedit.py`)

```json
{
  "sources": ["data/story/x.lscript"],
  "maps": [{
    "map": {"zone": 11, "area": 4},                  or {"index": 296}, "which" for duplicates
    "onload": "script:NAME",                         also name, music, variation, onleave
    "triggers": {"0": {"payload": "seq:NAME", "rect": [x1, y1, x2, y2], "condition": "flag:12"}},
    "add_triggers": [{"like": 0, "rect": [...], "payload": "...", "condition": "..."}],
    "records": {"1": {"stat": 109, "sprite": 160, "action": "seq:NAME"}, "4": "disable"},
    "add_records": [{"kind": "actor", "sprite": 158, "x": 240, "y": 456, "facing": 0, "condition": "noflag:26"},
                    {"like": 4, "sprite": 72, "talk": "seq:NAME"}],
    "pre_edits": [...],
    "edits": [{"at": "trigger:0/entry:48", "replace": [["SetEntityFollow 76, 128", ""]]}]
  }]
}
```

- `add_triggers` entries copy trigger N of the same map (`"like"`), or build a door exit from scratch:
  `{"kind": "exit", "rect": [...], "exit": [DIR, ZONE, AREA, VAR, X, Y], "condition": ...}` (create
  0x0800C233, activation 0x0800D8AF, the 8-byte payload inline; `docs/maps.md` 7). A map without
  triggers gets its first one this way (area 47).
- References: `seq:NAME`, `script:NAME` (assembled once, on first use), `@08xxxxxx`, `none`.
  Conditions: `always`, `never`, `flag:N`, `noflag:N`, `once:N`, `script:NAME`, a number.
- Record fields: `condition`, `x`, `y`, `sprite`; enemies `stat`, `action` (a sequence = defeat
  dialogue, a script = defeat script, `none`); NPCs `talk`. New records: `kind` actor / npc / enemy
  (one face-and-wait behaviour) or `like` (a copy of record N, or `{"map": ..., "record": N}`).
  Growing a list copies the pointer array to free ROM and updates the count.
- Edit paths: `trigger:N`, `record:N/defeat`, `record:N/talk`, `onload` (and the other map
  scripts), plus `/entry:K`. `replace` pairs match whole lines as `dump` prints them
  (whitespace-normalised); `""` deletes the line, a replacement with newlines inserts several.
  An edited script goes to free ROM and its pointer is repointed; an edited entry is re-placed and its
  sequence slot repointed in place (sequences are shared). `pre_edits` run before the replacements,
  and sources are parsed against the ROM as edited so far, so a `stockseq` over a stock sequence
  picks up `pre_edits` made to its entries (`ch1_opening.json`, zone 3 area 6).
- Edits land at the stock table's offsets; `patch_rom.py --maps` relocates the table afterwards and
  carries them over.

## 5. Chapters implemented

| spec | maps | what |
|---|---|---|
| `ch1_opening` | 0, 1, 3 (zone 1), 14, 19 (zone 2), 52, 54 (zone 3) | old Hercule (slot 5, row 24 / sprite 158) is player 2 in the future prologue with three comic lines and leaves the party at "Present day"; Krillin walks in instead of phoning (the stock call lines in person) and joins (slot 6); the forest spirit fight (the dream's clearing, Frieza + Cooler as a `BOSS_PAIR`), the warp to the armor cave and the voice's explanation; Krillin's actor and NPC gone from both landing-site scenes; old Hercule beside Trunks, two lines, leaves with him |
| `ch2_west_city` | 65, 97, 98, 105 (zone 4) | the mansion deal where Piccolo joins, Hercule joins; Triceratops Jungle locks Gohan and Krillin (flag 361) so player 2 is Hercule, restored at the entrance; Hercule claims the dinosaur peace |
| `ch3_duels` | 143 (zone 5), 166, 168 (zone 6), 263 (zone 9) | Amenbo's "Meanwhile" also locks Goku, Hercule and Krillin; space: Vegeta vs his clone (actors 137 / 140, the monologue, the transformation), the shadow-clone duel (`CoopCall 1, -1, 15`), "I've won. I'm a Super Saiyan", the tutorial, the warp; the landing: Hercule meets Vegeta and becomes player 2's only choice; the Lookout: Vegeta and Trunks walk in, the chamber (zone 15 area 2) with both as Super Saiyans, the duel (`CoopCall 1, 3, 12`), the winner's line, back to the Lookout with Piccolo as player 1 and Hercule as player 2 (Krillin selectable too), Goku sends Piccolo off |
| `ch3_zone7` | 7 (zone 2), 177, 191 (zone 7) | the three-year skip also raises Hercule (5) and Krillin (6) one level with a full refill; Android 13 (actor 165, speaker 0: no portrait) stands with 19 and 20 in the zone 7 cutaway with two lines and joins 19's fight ("Not by yourself, you don't"); 19 + 13 are a `BOSS_PAIR` (13 in single-player too); the defeat: 19 explodes as stock, 13 swears revenge and says not even the androids know Dr. Gero's true plan, flies off (by sprite 165, downed or last standing), then the stock Vegeta / 20 / Piccolo scene |
| `ch4_piccolo` | 215, 229, 242, 248, 251 (zone 8 areas 13, 27, 40, 46, 49) | Android 13 beside Dr. Gero in all three of his stages ("Told ya I'd be back"; stat 7 is a `BOSS_PAIR` with 13's stat 112), flying off in each defeat script; at the lab door (whichever generator comes first) Hercule offers to negotiate ("How about an autographed photo?"), Piccolo: "Get behind me", 13's offer; the generator sequences are restated because their returned index is absolute; the lab cutaway: 13 watches 17 and 18 turn on Gero, whispers that he will fulfil his destiny, walks out; the road: in co-op (`CoopCall 5`) Hercule bows out, Vegeta arrives ("Out of my way, clown") and player 2 plays him (`CoopCall 6, 2`, only his bit) against 18 and 17 (her twin), 17 picks Vegeta; after 18's blast player 2 is Hercule again (during the fade), who backs Krillin's plea; stock ending. Single-player keeps the stock road fight |
| `ch5_lookout` | 266 (zone 9 area 6), 283, 284, 287 (zone 10 areas 16, 17, 20) | the first Lookout visit: Hercule asks whether they wait up here, Piccolo leaves him ("don't touch anything"); Hercule and Krillin are locked with Piccolo, player 2 is Gohan in Trunks' chapter (`CoopCall 6, 0` after the warp, applied at player 2's respawn); before the fusion Kami calls Hercule over, thanks him for his kindness to Piccolo and draws out his potential "as the Grand Elder of Namek once did" (+3 levels, full refill, a level line); after the fusion Hercule is selectable again, Gohan and Trunks locked, player 2 Hercule for Gingertown; in co-op Cell splits ("Two of you?  Then two of me!"), Piccolo names Tien's Multi-Form, Hercule panics; at the defeat the fallen body is taken back in (one Cell despawned, the other stood up) before the stock scene; at the end Trunks and Krillin are the pair (player 2 Krillin), Hercule flies on with Piccolo |
| `ch6_basement` | 250, 255-257 (zone 8 areas 48, 61-63), 295 (zone 11 area 3) | the lab basement with player 2 as Krillin: every scene leaves Krillin's NPC (sprite 76: `TriggerMapEvent` spawns, walks, the area 48 actor through a condition script) out when `CoopCall 5` says player 2 is there and keeps it in single-player; Piccolo's selectable bit is no longer set at the lab exit (he is a scene actor at Kame House); upstairs at Kame House Krillin says he will stay and keep watch, and his bit is cleared when Goku leaves. With `ch3_duels`' chamber end Piccolo, Hercule and Krillin are selectable for the island |
| `a16_island` | 293, 296-298 (zone 11 areas 1, 4-6) | Kame House: 16 furious that Goku is absent, Piccolo's deal ("beat me on that island and I'll tell you where Goku is"); 16 (stat 109, sprite 160) replaces 17 in the two stock stages (bar 75-100 in area 4, 50-75 in area 6), stops at half health standing (`android16.c`), replaced by his stand-in actor (new record 6), Cell's stock scene follows (`stockseq`) |
| `ch7_cell` | 263 (zone 9 area 3, trigger 2 edit), 302, 304 (zone 12 areas 4, 6) | out of the chamber Vegeta is player 1 and Trunks the only player-2 choice; Semi-Perfect Cell splits in co-op ("Two Saiyans against me?  Then I'll even the odds"; Trunks recalls Gingertown) and takes the fallen body back after the fight; Trunks' stock fly-in only in single-player; after Perfect Cell's kick Trunks is player 1 (stock) and in co-op Krillin takes player 2 with the pads traded (`CoopCall 6, 6`, `CoopCall 7`), flying in ("Two of us have a better shot than one!"); the transformation check's returned index restated; at the end of the second Perfect Cell stage Krillin rushes him and is knocked out in one hit (co-op), then the stock tournament speech; Trunks + Krillin by default, Vegeta, Piccolo and Hercule selectable |
| `ch8_gym` | 175 (zone 6 area 70) | Roshi's sparring room as duos: Yamcha + Tien (stat 106 pairs with Tien's model and stat 98), then Krillin + Hercule (stat 64 pairs with Hercule's sprite 72), `BOSS_PAIR` rules so single-player gets the duos too; when either player is Hercule or Krillin (`CoopCall 8`) the second duo refuses ("Master Roshi says I can only fight duos with Krillin on my team right now... Come back with a different team!") and only the other of the pair stands (record condition scripts); the finale: Roshi's animal ninjas, eleven new enemy records (copies of Yamcha's: alligator, hawk, ladybug, pterodactyl, saber-toothed tiger, scorpion, snake, T-Rex, triceratops, two Ninjas; stats from the bestiary table 0x086F8E40, `{name, description, sprite, portrait, stat}` x 128) in three rounds (all at once ran out of OBJ tiles), flags 363-375, the last defeat pays Tien's +3 Endurance Capsule and flag 214 |
| `ch8_cooler` | 172 (zone 6 area 61) | Cooler's fight: the three stock openings (Goku, Trunks, anyone else) meet at a co-op check (`stock @A -> 10`), where Cooler brings "a Namekian warrior my father took from Namek long ago" ("...As you command, Lord Cooler.") before the fight with his servant (bosses.c: sprite 166, stat 113); the Piccolo gates are unchanged (zone 4 area 50 needs flag 60 from chapter 2, area 62's barrier Piccolo) |
| `wait_chamber` | 263 (zone 9 area 3), zone 15 area 2 | the ten-day wait's Time Chamber training: the chamber door (trigger 0) lets the players in from flag 157 (fade, warp, `CoopCall 13`); `lk_tc_round` before every round ("Round %d" with `CoopCall 14, 0`, then `CoopCall 14, 1` spawns) and "Training completed!"; Mr. Popo in front of the door until flag 157 (record 0's condition), beside it from 157 with his chamber line (new record 9). Enemies per round in `src/coop/waves.c`, table in `docs/story.md` 4g |
| `wait_spar` | 147 (zone 6 area 3), 175 (zone 6 area 70) | Roshi's dojo sparring: his talk sequence restated (`stockseq` 0-9 and 11-30 at their own indexes, so the returned indexes hold) with entry 10 branching to the offer once flag 214 is set; in co-op the offer books the dojo (flag 440), a new mat trigger (condition 214 and 440) starts `CoopCall 1` with player 2's slot (`CoopCall 8` pushed as the argument); the end: the winner by name (fn 14 op 2 for player 2), HP / EP refilled, back to the atrium |
| `broly` | 160, 165 (zone 6 areas 16, 21) and zone 15 area 3 (through `data/maps/broly_arena.json`'s `"sources"`) | the Broly side chapter (docs/story.md 4g): after Cell's broadcast (trigger 1's sequence restated with Bulma's entries appended as 9-11) Bulma walks in about the Saiyan ship (flag 400); the backyard's new ship actor (171) and a new dialogue trigger: without Vegeta as a player a hint, with him Paragus (actor 170, speaker 120), the trip's others walk in (each only when no player plays him), everyone boards, the ship lifts off, CoopCall 10 keeps the party, the five are selectable, warp to the landing zone; the rim trigger (once, flag 403, then straight to the fight): Paragus' betrayal, Broly, `DespawnCharacterAndRecordPosition 159, 2` (stat 115 + Paragus 116 as a `BOSS_PAIR`), CoopCall 9 sets the rage dialogue; the defeat: the explosion (19's effects), the warp home, the debrief, CoopCall 10 restores the party, `ReloadMap` |
| `ch9_gero` | 149 (zone 6 area 5), 176 (world map), 229, 242, 249, 250, 257, 258 (zone 8 areas 27, 40, 47, 48, 63, 64) | "Gero's computer" (`docs/story.md` 4g): 16 stands in Dr. Briefs' workshop during the wait (new actor, sprite 8); after the memory scene, the TV announcement and the Scouter Note (155, 157, item 22 / 23 / 24 or flag 156) the workshop scene: Briefs (16 repaired, bomb out), 16 wakes and picks up the computer's signal, Krillin walks in unless a player is Krillin, `CoopCall 11`, Krillin player 1 and 16 player 2, a warp into the workshop clears the actors; the world-map star at the zone 8 landing; the two generators under the waterfalls of areas 27 / 40 (flags 423 / 424, robots back online are the stock records); area 47's top exit fixed; the rubble at the cave mouth (16 punches through once both generators are down, flag 422); the basement door to the intact computer room; the bunker: "assembly complete" captions, two Android 19 spare bodies (record 1, stat 120, `BOSS_PAIR` with stat 121, single-player too), both down: the Android 13 file ("UNFINISHED. FINAL STAGE ON HOLD", absorbing two androids), the computer smashed (flag 425), outside 16 watches two hawks (sprites 63 / 64) and says he will go to the Cell Games, `CoopCall 12`, the warp to the workshop with Krillin player 1 and Hercule player 2 |
| `ch8_arctic` (**removed 2026-10-18**: not in `data/hercule.json`'s include, the trigger and record are gone from `arctic.json`; kept for 13's return after Cell) | zone 15 area 1 (built by `tools/mapbuild.py` from `data/maps/arctic.json`, which names this file in `"sources"`) | Android 13 at the cave mouth once Vegeta has opened the barrier (flag 380, encounter flag 376 clear): he was meant to absorb 17 and 18, Cell stole his perfection, he would kill Cell himself; Vegeta: he won't get the chance; a half-health fight (stat 114, alone until Ryan picks his partner), 13 flies off, flag 376 for the post-Cell variant |

Engine support added with them: `A16_END_PCT` and bar-range HP slices (`android16.c`), the
`BOSS_PAIR` rule flag and Cooler's spirit stat 111 (`bosses.c`; 19 + 13 and Gero + 13 are pairs too), player 2 by party flags
(`player.c`), the `StoryTest` preset and `COOP_CMD_STORY_WIN` for tests, and two engine fixes: the
`BossHealthBar_Create` entry jump now goes through r12 (the r3 trampoline had clobbered the bar's
upper percent), and a `Camera_Shake` hook never links the camera into its own shake chain.
Later chapters added `BOSS_RAGE` (a pair's survivor refills and grows stronger once, Broly), CoopCall
9 / 10 (`PartyKeep`, Broly's trip), `a16player.c` (Android 16 joins and leaves, Rocket Punch), `waves.c`
(the Time Chamber rounds, CoopCall 13 / 14), mapedit `"kind": "dialogue"` and `"kind": "exit"` triggers,
and world-map icons (`docs/maps.md` 9b).

## 6. Writing a chapter

1. `python tools/script_asm.py dump map Z A > build/story/zZaA.lscript` for every map involved.
2. Write `data/story/NAME.lscript`: new lines as `text`, unchanged ones as `stock` / `stockseq`,
   edited scripts in full. Write `data/story/NAME.json` and add it to `data/hercule.json`'s include.
3. `python tools/mapedit.py show data/story/NAME.json` (errors name the line), then `make`.
4. Test headless (section 7): Map Test the map with the chapter's flags, mash through, screenshot.

## 7. Testing chapters

`docs/testing.md` "Story chapters, headless" has the recipe. Short form: build a `DEBUG_MENU=1` ROM,
make a debug-menu savestate once per build (`tools/headless/mkmenu.txt`), then scripts like

```
p2 0                      # player 2 present (any build except STORY_TEST)
preset +4                 # story flags Party_Init applies at the Map Test
maptest 19                # map entry index (script_dump.py maps)
mash 9 110                # A nine times, 110 frames apart
poke32 0x0203FD08 7       # COOP_CMD_STORY_WIN: every boss fight on the map is won
mashmap 11 5 30           # A until the map changes
shot NAME                 # screenshot
```

run by `python tools/headless/st_test.py SCRIPT ROM STATE`, which tiles the screenshots.

## 8. Limits and open points

- Old Hercule's lines use Hercule's portrait (67); a portrait of his own is art.
- The Map Test party has everyone in it, so a Map Test does not show who player 2 would be in a new
  game; the party flags were checked by peeking the party block.
- Gohan's later chapters still show Krillin as an NPC where the stock game has him (18 maps); the
  duplicate-NPC rule (`actors.c`) hides town NPCs, cutscene actors need per-scene edits like zone 3's.
- Mode 5 (jump) entries round-trip but their runtime meaning is not traced.
- The duels' real endings (a final blow) are `duel.c`'s, verified by the main session; the chapter
  tests ended the duels by setting `coop2.duel_active` 0 and `duel_winner` (0x0203FD5C, 0x0203FD5D).
