// The Hyperbolic Time Chamber as a game mode (docs/time-chamber.md, Ryan's approved plan; section 12: the state of the
// build and how to add a room). A very large white void (zone 15 area 18) with six rooms in it. Beat a room's boss, throw
// its lever, one light of the gate goes out and everything outside gets harder.
//
// The open area. Way B of 7.3: the map has "spot" records of stat CH_SPOT_STAT; enemy_spawn_hook hands each to
// chamber_spot, which gives it the current tier's stat, sprite and EXP (tiers[], indexed by lights out, section 4). The map
// is far bigger than the bodies that fit OBJ VRAM (measured: docs/time-chamber.md 12), so the bodies follow the players:
// every CH_TICK frames one body far from both players leaves and, while fewer than the target stand, a new record comes
// in out of view around them (open_tick). A spawn here is the same call the loader makes.
//
// A room. One row of rooms[] plus a map (data/maps/chamber_dojo.json is the pattern) makes a room; everything else is shared:
//   the on-load (ch_room_onload)      CH_ROOM_LOAD: state clear; 1 when its lever is thrown: the plaque shows ON
//   the actors (records of the map)   conditions ch_boss_cond / ch_partner_cond
//   walking in (trigger ch_room_intro) CH_INTRO builds the scene: the boss and partner walk to their marks, the row's lines
//                                     and each player's reply, then CH_FIGHT: the door seals, and at the scene's end the
//                                     actors become fighters with their kits on the same spots, with boss bars
//   the watcher (room_tick)           every fighter down: the bars leave, his last line, the bodies blink out
//   the lever (trigger ch_lever)      CH_LEVER answers refuse (the click) or throw; the script cuts to the gate
//                                     (LoadMapNoEntities, the room's entity list is kept: FUN_08006F4A), puts the light out,
//                                     sets the flag, all six: the beams fall; RestoreMap; CH_OPEN opens the door
// The next load of the open area has the next tier. The scenes' lines are Claude's drafts, marked DRAFT below.
// Two rooms add a step (phase 4): RF_CREW (the airship) turns the crew's actors into regular enemies when the intro ends
// (R_CREW); when the last is down the lead says his line and steps up (ch_crew_parts) and only then fights. RF_RAGE (the Saiyan
// ship) enrages the lead when his partner falls first (tny_moves_rage: full HP, his power-up, his line). A build without
// Buu's Fury's art (BUU=0) stands each room on its row's `plain` twin (fallback maps, other fighters).
#include "tourney.h"
#include "chamber.h"
#include "revive.h"
#include "tny_talk.h"
#include "popo.h"         // the hidden room and Mr. Popo, the loop's times round (popo.c)

#define CH_MAGIC        0x4348414Du   // "CHAM"
#define CH_TICK         16            // frames between the open area's checks
#define CH_FAR_X        300           // px from the players' midpoint: a body beyond this leaves
#define CH_FAR_Y        220
#define CH_TARGET       7             // records standing with two players (14 bodies, each with his twin): the measures, section 12
#define CH_TARGET_ONE   10            // bodies standing with one player
#define CH_REFILL_WAIT  90            // frames after a kill before a record comes in
#define CH_MIN_FREE     0x1800        // OBJ VRAM's largest free block a spawn asks for: two bodies and room for blasts and drops
#define CH_FALL_FRAMES  48            // a fallen boss lies this long before his last line
#define CH_BLINK        24            // ... and blinks this long before he goes
#define CH_MAP_W        2048
#define CH_MAP_H        2048
#define WANDER          0x083EE430u   // the stock wild-wolf wander (zone 2 area 7 record 0): the chase AI takes over near a player

#define STEP_FLAG_SHOW  0x08009741u   // step 29: decoration N shows (id << 1 | 1 to every entity)
#define STEP_FLAG_HIDE  0x0800975Fu   // step 30: ... hides
#define STEP_WALK_RECT  0x0800A7A1u   // step 71 SetCollisionRect: walkable (walk and the enemies' copy)
#define STEP_BLOCK_RECT 0x0800A7F9u   // step 72 ClearCollisionRect: blocked
#define STEP_SPAWN_ITEM 0x0800ABA5u   // step 87 SpawnItem: at the last break's spot (0x03001FD0), when OBJ VRAM has 1 KB
#define COLLISION_MAP   ((const void *)g_Ctx[4])
#define LOOK_UP         1
ROM_FN(int, ObjVram_Largest, (void *list), 0x08006D71);

Entity *enemy_spawn_hook(const EnemyRecord *rec);               // enemies.c
BossBar *bossbar_create_hook(void *mem, Entity *e, int lo, int hi);
int flag_test_hook(PartyState *p, u32 id);                      // party.c
void flag_set_hook(PartyState *p, u32 id);
void flag_clear_hook(PartyState *p, u32 id);
Entity *tny_moves_room_spawn(int fighter, s32 x, s32 y);        // tny_moves.c: a fighter with his kit, off any map's record
void tny_moves_rage(Entity *e);                                 // tny_moves.c: full HP and his power-up now (RF_RAGE)
int is_any_enemy(const Entity *e);                              // enemies.c
void wave_start(const void *seq, int first_round);              // waves.c

// ---- the open area's tiers (section 4, by lights out) --------------------------------------------------------------
// {stat, sprite, EXP per kill}. Stock EXP is uneven and its top amounts level far too fast with a refill (risk 4): the
// third column is a ladder of its own, at about 35 kills a level at the level each tier is met (the table in section 12).
typedef struct ChFoe { u8 stat, sprite; u16 exp; } ChFoe;
#define KINDS 3
#define DESTROYER_SPRITE 38
#define BEAR_0 {71, 92, 3000}
static const ChFoe tiers[CH_ROOMS + 1][KINDS] = {
    { BEAR_0,          {14, 116, 3000},  BEAR_0 },           // 0 Kuma Mercenary (bear), snake
    { {57, 148, 4000}, {107, 64, 4000},  {92, 108, 4000} },  // 1 wolf, hawk, sabre-toothed tiger
    { {70, 91, 6000},  {12, 142, 6000},  {70, 91, 6000} },   // 2 the big Kuma, Warlord's Henchman (the "grenadiers")
    { {75, 96, 9000},  {56, 145, 9000},  {51, 143, 9000} },  // 3 ninja, warthog, stronger Henchman
    { {10, 19, 13000}, {84, 65, 13000},  {40, 47, 13000} },  // 4 Tiger Bandit, stronger hawk, Eggbot (no twin)
    { {86, 98, 18000}, {89, 110, 18000}, {47, 112, 18000} }, // 5 stronger ninja, stronger tiger, scorpion
    { {48, 117, 25000}, {50, 147, 25000}, {32, DESTROYER_SPRITE, 25000} },   // 6 the top snake, wolf, one Destroyer (4 KB)
};

// ---- the rooms -----------------------------------------------------------------------------------------------------
// One row per room: docs/time-chamber.md 12 "Adding a room".
#define NONE 0xFF
#define CH_LINE_SLOTS 9         // the lines by party slot (Gohan .. Super Saiyan 2 Gohan), whatever PARTY_SLOTS a build has
typedef struct ChBoss { u8 fighter; s16 rating; u8 hp_pct; const char *won; } ChBoss;
typedef struct ChRoom {
    u8 area;                    // zone 15 area of its map
    u8 light;                   // its lever's flag is CH_FLAG_LEVER0 + light; decoration `light` of the open map goes out
    u8 flags;                   // RF_*
    u8 look;                    // the way the players face at the intro (LOOK_UP: the bosses stand above; LOOK_LEFT)
    ChBoss alone;               // one player
    ChBoss pair[2];             // two players (pair[1].fighter NONE: pair[0] alone, its hp_pct for two players)
    s16 mark[2][2];             // where the actors walk to before the fight (x, y): [0] the lead, [1] the partner
    s16 seal[4];                // the door's rect: blocked during the fight (x1, y1, x2, y2)
    s16 line;                   // a player past this (below it; LOOK_LEFT: right of it) when the scene starts is brought up
                                // beside the other (the door's strip seals: nobody may stand behind it)
    const char *open;           // the boss's opening line (a `%` is a format here: none)
    const char *const *open_by; // CH_LINE_SLOTS lines by party slot: player 1's character's, else player 2's (0: open)
    const char *partner_line;   // the partner's line (with two players)
    u8 crew_sprite;             // RF_CREW: the crew's actors show this sprite; they become enemies when the intro ends
    const char *crew_line;      // RF_CREW: the lead's line when the crew is down, as he steps up
    const char *rage_line;      // RF_RAGE: the lead's line when his partner falls first
    const struct ChRoom *plain; // the row a build without Buu's Fury's art uses (BUU=0: its fallback map, other fighters)
    // (docs/time-chamber.md 13, Ryan's first play)
    s16 stand[2][2];            // where the lead and the partner stand at the far end (the map's actor records): the opening
    s16 shot[2][2];             // the opening's two views of the room (top-left): from the lit plaque to the boss
    s16 ways[2][4];             // side rooms' doorways, blocked until the lever is thrown (x1, y1, x2, y2; 0 = none)
} ChRoom;
#define RF_CREW 0x01            // a crew of regular enemies first (the airship's Warlord's Henchmen), then the lead
#define RF_RAGE 0x02            // the partner falling first enrages the lead (the Broly chapter's rule: full HP, a power-up)
#define LOOK_LEFT 2
#define RATING_STEP 25          // rating per light already out: the last room is the hardest whichever it is (decision 3)
#define PAIR_HP 70              // a pair's HP in percent each (bosses.h COOP_PAIR_HP_PCT)
#define LONE_HP 180             // a lone boss against two players (COOP_LONE_BOSS_HP_PCT)
#define DECO_PLAQUE_ON 0        // a room map's decorations (data/maps/chamber_dojo.json)
#define DECO_DOOR_BEAM 1
#define CREW_STAT        12     // the Warlord's Henchman (sprite 142); from three lights out the stronger one (51 / 143),
#define CREW_STAT_HARD   51     // as the open area's tiers 2 and 3
#define CREW_HARD_SPRITE 143
#define CREW_HARD_LIGHTS 3
#define CREW_PAUSE       40     // frames after the crew's last body falls before the lead steps up

// Every line below is Claude's draft (DRAFT), not dictated. A fighter meeting himself, or a player's own story people, get
// a line of their own (open_by).
static const char *const roshi_by[CH_LINE_SLOTS] = {
    [0] = "ROSHI: Little Gohan! Not so little any more.\x0b" "Show this old man what your father taught you.",
    [1] = "ROSHI: Piccolo, eh? The last of your kind I met cost me my life.\x0b" "I've had time to practise.",
    [2] = "ROSHI: The Saiyan prince in my dojo!\x0b" "Wipe your boots and mind the furniture.",
    [3] = "ROSHI: Bulma's boy! Give her my love, won't you?\x0b" "Hoho! After I've thrashed you, of course.",
    [4] = "ROSHI: Goku! Come to see your old master?\x0b" "Let's see how far you've come, then.",
    [5] = "ROSHI: The champion himself!\x0b" "Those belts look heavy, my boy. Let's see what's under them.",
    [6] = "ROSHI: Krillin! Been training, or chasing girls?\x0b" "Don't answer. I can tell.",
    [7] = "ROSHI: An android who likes birds, they tell me.\x0b" "No hard feelings, tin man.",
    [8] = "ROSHI: Little Gohan! Not so little any more.\x0b" "Show this old man what your father taught you.",
};
#define ROSHI_WON "ROSHI: Oof! My back... Enough, enough!\x0b" "Go on, throw that switch on the wall."

static const char *const tao_by[CH_LINE_SLOTS] = {
    [4] = "TAO: Goku. The brat who broke my Dodon Ray.\x0b" "I've been rebuilt since. Men!",
    [5] = "TAO: The World Champion? I've killed for less.\x0b" "Men! Make it quick.",
    [6] = "TAO: The little monk from the tournament.\x0b" "You haven't grown an inch. Men!",
    [7] = "TAO: Gero's work. I took the Red Ribbon's money too, once.\x0b" "Men! Scrap it.",
};
#define TAO_OPEN "TAO: General Tao. Someone has paid well for your heads.\x0b" "Men! Deal with them."
#define TAO_CREW "TAO: Useless, the lot of you!\x0b" "Very well. I'll kill them myself."
#define TAO_WON  "TAO: Beaten... again...\x0b" "Take your switch, and be gone."

static const char *const frieza_by[CH_LINE_SLOTS] = {
    [0] = "FRIEZA: The little Saiyan from Namek.\x0b" "You've grown. It won't save you.",
    [1] = "FRIEZA: Another Namekian. Your people died screaming.\x0b" "Shall I make it one more?",
    [2] = "FRIEZA: Vegeta. Still playing the proud little prince?\x0b" "I killed you once. I can do it again.",
    [3] = "FRIEZA: You! The boy with the sword...\x0b" "This time, I do the cutting.",
    [4] = "FRIEZA: Goku... even here you haunt me.\x0b" "I have waited a long time for this.",
    [5] = "FRIEZA: And what is this? A clown?\x0b" "Do try to entertain me.",
    [6] = "FRIEZA: The bald one! I remember you.\x0b" "You made such a lovely explosion.",
    [8] = "FRIEZA: The little Saiyan from Namek.\x0b" "You've grown. It won't save you.",
};
#define FRIEZA_OPEN "FRIEZA: A monkey's training room. How quaint.\x0b" "Let me show you what real power looks like."
#define FRIEZA_WON  "FRIEZA: Impossible! Beaten by the likes of you?\x0b" "Fine. Pull your little switch."
#define COLD_LINE   "KING COLD: Careful, my son.\x0b" "Do leave a little for your father."

static const char *const piccolo_by[CH_LINE_SLOTS] = {
    [0] = "PICCOLO: Gohan. Don't hold back because it's me.\x0b" "I didn't train you to be gentle.",
    [1] = "PICCOLO: ...Me? Heh. Popo has a sense of humour.\x0b" "Let's see which of us is the real one.",
    [2] = "PICCOLO: Vegeta. All that pride, and still second best.\x0b" "Prove me wrong.",
    [4] = "PICCOLO: Goku. I always said I'd beat you one day.\x0b" "Today's as good a day as any.",
    [5] = "PICCOLO: ...You. Do you even know where you are?\x0b" "Never mind. Brace yourself.",
    [8] = "PICCOLO: Gohan. Don't hold back because it's me.\x0b" "I didn't train you to be gentle.",
};
#define PICCOLO_OPEN "PICCOLO: So this is what Popo dreamed up.\x0b" "Show me you've been training, or get out."
#define PICCOLO_WON  "PICCOLO: Hmph. Not bad.\x0b" "Go on. Throw the switch."
#define NAMEK_LINE   "NAMEKIAN: For Namek!\x0b" "I fight beside you, Piccolo."
static const char *const namek_by[CH_LINE_SLOTS] = {
    [1] = "NAMEKIAN: Piccolo! Kami and Nail, both in one.\x0b" "Then show me what a Namekian can be.",
};
#define NAMEK_OPEN "NAMEKIAN: An outsider, in the elders' ship.\x0b" "Prove your strength, or leave."
#define NAMEK_WON  "NAMEKIAN: Enough... You have earned it.\x0b" "The switch is yours."

static const char *const a18_by[CH_LINE_SLOTS] = {
    [2] = "ANDROID 18: Vegeta. I broke your arm last time.\x0b" "Want to try for the other one?",
    [3] = "ANDROID 18: The boy from the future who hates us so much.\x0b" "Let's give you a reason.",
    [4] = "ANDROID 18: Son Goku. Gero built us to kill you.\x0b" "Let's get it over with.",
    [5] = "ANDROID 18: The World Champion. Cute belts.\x0b" "Let's see if they're real.",
    [6] = "ANDROID 18: You again. The one with the remote.\x0b" "Don't look at me like that.",
    [7] = "ANDROID 18: Sixteen? Whose side are you on now?\x0b" "Never mind. Out of my way.",
};
#define A18_OPEN "ANDROID 18: So I'm a training dummy now. Fine.\x0b" "Don't expect me to go easy."
#define A18_WON  "ANDROID 18: Tch. Fine, you win.\x0b" "The switch is over there."
#define A17_LINE "ANDROID 17: Relax, sis.\x0b" "I'll take the other one."

static const char *const broly_by[CH_LINE_SLOTS] = {
    [0] = "BROLY: Kakarot's... son.\x0b" "You will suffer first.",
    [2] = "BROLY: Prince... Vegeta.\x0b" "You will kneel.",
    [4] = "BROLY: Ka... ka... rot...\x0b" "KAKAROT!!",
    [8] = "BROLY: Kakarot's... son.\x0b" "You will suffer first.",
};
#define BROLY_OPEN "BROLY: ...More of you.\x0b" "I will crush you all."
#define BROLY_WON  "BROLY: Impossible...\x0b" "I am... the strongest..."
#define BROLY_RAGE "BROLY: Pa... ra... gus...?\x0b" "RRRAAAAAAGH!!"
#define PARAGUS_LINE "PARAGUS: Steady, Broly.\x0b" "Show them what a true Saiyan is."
static const char *const paragus_by[CH_LINE_SLOTS] = {
    [2] = "PARAGUS: My prince. Your father wronged my son.\x0b" "I will settle that debt with you.",
};
#define PARAGUS_OPEN "PARAGUS: Broly is not here. I will have to do.\x0b" "Come, then."
#define PARAGUS_WON  "PARAGUS: Enough... You are strong.\x0b" "Take your prize."

// A build without roms/buu.gba (BUU=0) has no Buu-saga Piccolo, no Broly and none of Buu's Fury's interiors: these rows stand
// on the fallback maps (data/maps/chamber_*_plain.json, "unless": "buu": LoG2's Capsule Ship 3 recoloured).
#define CAPSULE_PLACE .look = LOOK_UP, .mark = { { 120, 200 }, { 200, 200 } }, .seal = { 144, 296, 176, 304 }, .line = 290, \
                      .stand = { { 120, 176 }, { 200, 176 } }, .shot = { { 40, 60 }, { 40, 100 } }
static const ChRoom pirate_plain = {
    .area = 20, .light = 1, .flags = RF_CREW, CAPSULE_PLACE,
    .alone = { TF_TAO, 360, 100, TAO_WON }, .pair = { { TF_TAO, 360, LONE_HP, TAO_WON }, { NONE, 0, 0, 0 } },
    .open = TAO_OPEN, .open_by = tao_by, .crew_sprite = 142, .crew_line = TAO_CREW,
};
static const ChRoom frieza_plain = {
    .area = 21, .light = 2, CAPSULE_PLACE,
    .alone = { TF_FRIEZA, 360, 100, FRIEZA_WON },
    .pair = { { TF_FRIEZA, 360, PAIR_HP, FRIEZA_WON }, { TF_KING_COLD, 340, PAIR_HP, 0 } },
    .open = FRIEZA_OPEN, .open_by = frieza_by, .partner_line = COLD_LINE,
};
static const ChRoom namek_plain = {
    .area = 22, .light = 3, CAPSULE_PLACE,
    .alone = { TF_NAMEKIAN, 370, 100, NAMEK_WON }, .pair = { { TF_NAMEKIAN, 370, LONE_HP, NAMEK_WON }, { NONE, 0, 0, 0 } },
    .open = NAMEK_OPEN, .open_by = namek_by,
};
static const ChRoom saiyan_plain = {
    .area = 24, .light = 5, CAPSULE_PLACE,
    .alone = { TF_PARAGUS, 360, 100, PARAGUS_WON }, .pair = { { TF_PARAGUS, 360, LONE_HP, PARAGUS_WON }, { NONE, 0, 0, 0 } },
    .open = PARAGUS_OPEN, .open_by = paragus_by,
};

static const ChRoom rooms[] = {
    {   // Roshi's dojo (data/maps/chamber_dojo.json)
        .area = 19, .light = 0, .look = LOOK_UP,
        .alone = { TF_ROSHI, 350, 100, ROSHI_WON },
        .pair = { { TF_ROSHI, 350, PAIR_HP, ROSHI_WON }, { TF_CHIAOTZU, 330, PAIR_HP, 0 } },
        .mark = { { 124, 136 }, { 164, 136 } },
        .seal = { 120, 272, 160, 280 },
        .line = 256,
        .open = "ROSHI: Hoho! So Popo set me up in here, did he?\x0b" "Come on, then. Let's see what you've learned.",
        .open_by = roshi_by,
        .partner_line = "CHIAOTZU: I'll help, Master Roshi.\x0b" "Tien says I have to get more practice.",
        .stand = { { 124, 100 }, { 164, 100 } }, .shot = { { 20, 0 }, { 20, 40 } },
    },
    {   // the pirate airship (data/maps/chamber_pirate.json, Buu's Fury's deck 12/1): the Warlord's Henchmen, then Tao. The
        // players come in by the gangway on the right; a lone Tao against two (his crew has twins)
        .area = 20, .light = 1, .flags = RF_CREW, .look = LOOK_LEFT,
        .alone = { TF_TAO, 360, 100, TAO_WON }, .pair = { { TF_TAO, 360, LONE_HP, TAO_WON }, { NONE, 0, 0, 0 } },
        .mark = { { 136, 136 }, { 136, 136 } },
        .seal = { 352, 112, 360, 144 },
        .line = 344,
        .open = TAO_OPEN, .open_by = tao_by, .crew_sprite = 142, .crew_line = TAO_CREW,
        .plain = &pirate_plain,
        .stand = { { 80, 136 }, { 80, 136 } }, .shot = { { 0, 0 }, { 0, 60 } },
    },
    {   // Frieza's ship (data/maps/chamber_frieza.json, Buu's Fury's 8/19, the crystal room): Frieza alone turns Mecha at 40 %
        // (his kit); with two players Frieza and King Cold come round the crystal
        .area = 21, .light = 2, .look = LOOK_UP,
        .alone = { TF_FRIEZA, 360, 100, FRIEZA_WON },
        .pair = { { TF_FRIEZA, 360, PAIR_HP, FRIEZA_WON }, { TF_KING_COLD, 340, PAIR_HP, 0 } },
        .mark = { { 104, 296 }, { 296, 296 } },
        .seal = { 184, 336, 216, 344 },
        .line = 320,
        .open = FRIEZA_OPEN, .open_by = frieza_by, .partner_line = COLD_LINE,
        .plain = &frieza_plain,
        .stand = { { 104, 240 }, { 296, 232 } }, .shot = { { 0, 16 }, { 80, 150 } },
        .ways = { { 184, 56, 216, 104 } },          // the top doorway: the throne room (15/29)
    },
    {   // the Namek ship (data/maps/chamber_namek.json, Capsule Ship 3 in green): Buu-saga Piccolo, the Namekian Warrior
        .area = 22, .light = 3, CAPSULE_PLACE,
        .alone = { TF_BPICCOLO, 380, 100, PICCOLO_WON },
        .pair = { { TF_BPICCOLO, 370, PAIR_HP, PICCOLO_WON }, { TF_NAMEKIAN, 350, PAIR_HP, 0 } },
        .open = PICCOLO_OPEN, .open_by = piccolo_by, .partner_line = NAMEK_LINE,
        .plain = &namek_plain,
    },
    {   // the lab (data/maps/chamber_lab.json, Dr. Gero's computer room 8/63): 18 alone, 18 and 17
        .area = 23, .light = 4, .look = LOOK_UP,
        .alone = { TF_A18, 360, 100, A18_WON },
        .pair = { { TF_A18, 360, PAIR_HP, A18_WON }, { TF_A17, 370, PAIR_HP, 0 } },
        .mark = { { 128, 248 }, { 240, 248 } },
        .seal = { 200, 336, 232, 344 },
        .line = 320,
        .open = A18_OPEN, .open_by = a18_by, .partner_line = A17_LINE,
        .stand = { { 128, 200 }, { 240, 200 } }, .shot = { { 30, 60 }, { 40, 110 } },
    },
    {   // the Saiyan ship (data/maps/chamber_saiyan.json, the Broly chapter's round room): Broly (his rage at 50 %, his kit);
        // with two players Paragus too, and Paragus falling first enrages Broly (RF_RAGE)
        .area = 24, .light = 5, .flags = RF_RAGE, .look = LOOK_UP,
        .alone = { TF_BROLY, 390, 100, BROLY_WON },
        .pair = { { TF_BROLY, 380, PAIR_HP, BROLY_WON }, { TF_PARAGUS, 330, PAIR_HP, 0 } },
        .mark = { { 64, 168 }, { 192, 176 } },
        .seal = { 104, 232, 136, 240 },
        .line = 216,
        .open = BROLY_OPEN, .open_by = broly_by, .partner_line = PARAGUS_LINE, .rage_line = BROLY_RAGE,
        .plain = &saiyan_plain,
        .stand = { { 56, 136 }, { 200, 150 } }, .shot = { { 0, 0 }, { 0, 60 } },
        .ways = { { 0, 136, 32, 184 }, { 208, 136, 240, 184 } },   // the side openings: the bedroom (15/27), the store room (15/28)
    },
};
#define ROOM_COUNT (int)(sizeof rooms / sizeof rooms[0])

// A player's reply when a boss has spoken, by party slot. DRAFT lines, Claude's.
static const char *const replies[CH_LINE_SLOTS] = {
    [0] = "GOHAN: I won't hold back.",
    [1] = "PICCOLO: Enough talk.",
    [2] = "VEGETA: Another phantom. Get on with it.",
    [3] = "TRUNKS: I have to get stronger. Here I come!",
    [4] = "GOKU: Alright! This'll be fun!",
    [5] = "HERCULE: Uh... is it too late to ask for a rain check?",
    [6] = "KRILLIN: Why is it always me?",
    [7] = "ANDROID 16: ...Very well.",
    [8] = "GOHAN: Let's finish this quickly.",
};
static const u8 slot_fighter[CH_LINE_SLOTS] = {
    TF_GOHAN, TF_PICCOLO, TF_VEGETA, TF_TRUNKS, TF_GOKU, TF_P_HERCULE, TF_KRILLIN, TF_P_A16, TF_SSJ2_GOHAN,
};

// ---- state -----------------------------------------------------------------------------------------------------------
enum { R_IDLE, R_ARMED, R_FIGHT, R_FALL, R_TALK, R_CLEAR, R_WON, R_CREW, R_CREW_TALK };
typedef struct ChState {
    u32 magic;
    u8 state;              // R_*
    u8 timer;              // frames in R_FALL / R_CLEAR
    u8 target;             // the open area's records standing with two players (CH_TARGET; test command 48 op 2)
    u8 frozen;             // tests: the open area's tick does nothing
    Entity *boss[2];       // the room's fighters
    const u32 *action;     // the open area's drop (the spot records'): the refill's records use it
    u16 obj_low;           // the smallest largest-free OBJ VRAM block seen since the last reset (bytes / 32)
    u16 spawned, removed;  // the refill's counts
    u8 tick, kind;         // frames to the next check; the next kind of the tier
    s16 wait;              // frames before the next record may come in
    u8 standing;           // bodies at the last check
    u8 rate_on;            // chamber_rating answers while a room spawns
    u8 rate_id, rate_pct;
    s16 rate;
    u8 cut;                // the room whose lever's cut-away runs (rooms[] index)
    u8 crew;               // RF_CREW: 1 once the crew stood up (the lead fights after them)
    u8 raged;              // RF_RAGE: 1 once the lead raged
    u8 prize;              // the item the last lever paid (its box: CH_PRIZE_TEXT)
    const void *const *crew_parts;   // RF_CREW: the scene's parts when the crew is down (ch_crew_parts, CH_INTRO's b)
    const void *open_seq;  // the open area's scenes (ch_open_scenes: the opening, the way out, the training's end)
    u8 scene;              // SC_* | SC_RUNNING | SC_SKIP: the open area's scene queued or running
    u8 bits;               // B_*
    u8 anim;               // the portal's frames
    u8 wait2;              // frames before a queued scene starts (player 1 free)
} ChState;
_Static_assert(sizeof(ChState) <= 0x34, "docs/ids.md: .combo.ch at most 0x34 bytes");
enum { SC_NONE, SC_OPENING, SC_EXIT, SC_DONE, SC_PORTAL };   // the open area's scenes (ch_open_scenes' routes)
#define SC_KIND     0x0F
#define SC_RUNNING  0x40
#define SC_SKIP     0x80
#define B_EXIT_HELD 0x01   // a player stood in the door's strip: no question until both have left it
#define B_DONE_SEEN 0x02   // "Training complete!" shown this visit
#define B_PORTAL    0x04   // the portal is open (its two frames shown in turn)
#define B_PHASE     0x08
static ChState ch __attribute__((section(".combo.ch")));

static void state_check(void) {
    if (ch.magic == CH_MAGIC) return;
    memset(&ch, 0, sizeof ch);
    ch.magic = CH_MAGIC;
    ch.target = CH_TARGET;
    ch.obj_low = 0xFFFF;
}

static const u8 *map_here(void) { return (const u8 *)g_MapState; }
// Buu's Fury's art is in this build (BUU=1): its maps were built with it, so the rooms stand on them (else ChRoom.plain)
static int have_buu(void) { return tny_fighter_ok(TF_BPICCOLO); }
static int room_index_here(void) {
    const u8 *m = map_here();
    if (!m || m[0] != CH_ZONE) return -1;
    for (int i = 0; i < ROOM_COUNT; i++) if (rooms[i].area == m[1]) return i;
    return -1;
}
static const ChRoom *room_here(void) {
    int i = room_index_here();
    if (i < 0) return 0;
    return rooms[i].plain && !have_buu() ? rooms[i].plain : &rooms[i];
}
static int in_list(const Entity *e) {
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) if (l->items[i] == e) return 1;
    return 0;
}
static int lever_flag(const ChRoom *r) { return CH_FLAG_LEVER0 + r->light; }
static int lever_thrown(const ChRoom *r) { return flag_test_hook(g_Party, lever_flag(r)); }

int chamber_lights(void) {
    int n = 0;
    for (int i = 0; i < CH_ROOMS; i++) n += flag_test_hook(g_Party, CH_FLAG_LEVER0 + i);
    return n;
}

static void step4(u32 fn, s32 a, s32 b, s32 c, s32 d) {
    s32 st[6] = { 4, a, b, c, d, 0 };
    ((void (*)(s32 *))fn)(st);
}
static void step1(u32 fn, s32 a) {
    s32 st[3] = { 1, a, 0 };
    ((void (*)(s32 *))fn)(st);
}

// ---- the open area ---------------------------------------------------------------------------------------------------
static const ChFoe *foe_next(void) {
    const ChFoe *row = tiers[chamber_lights()];
    for (int i = 0; i < KINDS; i++) {
        const ChFoe *f = &row[ch.kind++ % KINDS];
        if (f->sprite == DESTROYER_SPRITE) {        // one 4 KB body at most
            const void *rec = Character_GetSpriteRecord(DESTROYER_SPRITE);
            int have = 0;
            EntityList *l = g_EntityList;
            for (int k = 0; k < l->count && !have; k++) have = l->items[k] && l->items[k]->sprite_record == rec;
            if (have || ObjVram_Largest(g_EntityList) < 0x1400) continue;
        }
        return f;
    }
    return &row[0];
}

void chamber_spot(const EnemyRecord *in, EnemyRecord *out, s32 *exp) {
    state_check();
    const ChFoe *f = foe_next();
    memcpy(out, in, 0x1C);
    out->flags = 0;
    out->stat_index = f->stat;
    out->sprite_id = f->sprite;
    out->behaviour_count = 1;
    out->behaviours[0] = (const void *)WANDER;
    ch.action = (const u32 *)in->action;
    *exp = f->exp;
}

// Spots round the players' midpoint, out of the 240 x 160 view whichever player the camera favours
static const s16 ring[12][2] = {
    { 170, 0 }, { -170, 0 }, { 170, 70 }, { -170, 70 }, { 170, -70 }, { -170, -70 },
    { 0, 125 }, { 0, -125 }, { 90, 125 }, { -90, 125 }, { 90, -125 }, { -90, -125 },
};

static int open_ground(const Entity *like, s32 x, s32 y) {
    if (x < 24 || y < 24 || x > CH_MAP_W - 56 || y > CH_MAP_H - 32) return 0;
    s32 px[2] = { x, y };
    return Collision_Check(COLLISION_MAP, px, (u8 *)like + 0x154);
}

static void spawn_near(Entity *p1, s32 cx, s32 cy) {
    for (int i = 0; i < 4; i++) {
        const s16 *o = ring[Rand_Range(12)];
        s32 x = cx + o[0], y = cy + o[1];
        if (!open_ground(p1, x, y)) continue;
        struct { EnemyRecord r; const void *beh[1]; } rec;
        memset(&rec, 0, sizeof rec);
        rec.r.handler = (void *)0x0800E77Fu;
        rec.r.stat_index = CH_SPOT_STAT;
        rec.r.x = (s16)x;
        rec.r.y = (s16)y;
        rec.r.action = (void *)ch.action;
        rec.r.behaviour_count = 1;
        rec.beh[0] = (const void *)WANDER;
        Entity *e = enemy_spawn_hook(&rec.r);
        if (!e) return;
        EntityList_Add(g_SceneTable[1], e);         // the loader's add; the hook adds the twin itself
        ch.spawned++;
        return;
    }
}

static void exit_watch(void);
static int training_done(void) { return flag_test_hook(g_Party, CH_FLAG_ROUND_WON); }

static void open_tick(void) {
    if (scene_running() || !p1_controllable()) return;
    Entity *p1 = g_PlayerObject;
    if (!p1 || !in_list(p1)) return;
    exit_watch();
    if (ch.frozen || training_done()) return;      // Popo beaten this time round: the training is complete, nobody comes
    if (ch.wait > 0) ch.wait--;
    if (ch.tick++ < CH_TICK) return;
    ch.tick = 0;
    int largest = ObjVram_Largest(g_EntityList);
    if ((largest >> 5) < ch.obj_low) ch.obj_low = (u16)(largest >> 5);
    s32 cx = p1->x >> 8, cy = p1->y >> 8;
    if (coop.p2) { cx = (cx + (coop.p2->x >> 8)) / 2; cy = (cy + (coop.p2->y >> 8)) / 2; }
    int n = 0;
    Entity *far = 0;
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) {
        Entity *e = l->items[i];
        if (!is_any_enemy(e) || e->hp <= 0) continue;
        n++;
        s32 dx = (e->x >> 8) - cx, dy = (e->y >> 8) - cy;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        if (dx > CH_FAR_X || dy > CH_FAR_Y) far = e;
    }
    if (far) {                                      // one a check: its block goes back to OBJ VRAM
        EntityList_Broadcast(g_SceneTable[1], 0xF, far);
        EntityList_Remove(g_SceneTable[1], far, 0);
        ch.removed++;
        n--;
    }
    if ((u8)n < ch.standing && !far) ch.wait = CH_REFILL_WAIT;   // a kill: a breath before the next one comes
    ch.standing = (u8)n;
    int bodies = (p2_coming() && !coop_solo()) ? ch.target * 2 : ch.target + CH_TARGET_ONE - CH_TARGET;
    if (n < bodies && ch.wait <= 0 && largest >= CH_MIN_FREE) {
        spawn_near(p1, cx, cy);
        ch.standing = (u8)(n + 1);                  // a twin's count comes with the next check
    }
}

// The stock drops read player 1's bars only (docs/time-chamber.md 2.4): the chamber's read both players'. Item ids: HP
// 11 / 12 / 13 (10, 20, 30 %), EP 14 / 15 / 16.
static int bar_short(const CharacterEntry *c, int ep) { return ep ? c->ep < c->max_ep : c->hp < c->max_hp; }
static void drop_item(int roll_of_100, int bigger) {
    const CharacterEntry *a = &g_Party->chars[g_Party->active_slot];
    const CharacterEntry *b = (coop.p2 && coop.p2_slot < PARTY_SLOTS) ? &g_Party->chars[coop.p2_slot] : a;
    int hp = bar_short(a, 0) || bar_short(b, 0), ep = bar_short(a, 1) || bar_short(b, 1);
    if (!hp && !ep) return;                         // both full: nothing, as the stock drops
    int want_ep = hp && ep ? Rand_Range(2) : ep;
    int size = roll_of_100 < 50 ? 0 : roll_of_100 < (bigger ? 83 : 90) ? 1 : 2;
    step1(STEP_SPAWN_ITEM, (want_ep ? 14 : 11) + size);
}

// ---- a room ----------------------------------------------------------------------------------------------------------
static int two_players(void) { return p2_coming() && !coop_solo(); }
static const ChBoss *lead_of(const ChRoom *r) { return two_players() ? &r->pair[0] : &r->alone; }
static const ChBoss *partner_of(const ChRoom *r) {
    return two_players() && r->pair[1].fighter != NONE ? &r->pair[1] : 0;
}

int chamber_rating(int fighter, int *hp_pct) {
    if (ch.magic != CH_MAGIC || !ch.rate_on || ch.rate_id != fighter) return 0;
    *hp_pct = ch.rate_pct;
    return ch.rate;
}

static Entity *actor_of(int sprite) {
    const void *rec = Character_GetSpriteRecord(sprite);
    EntityList *l = g_EntityList;
    for (int i = l->count - 1; i >= 0; i--) {
        Entity *e = l->items[i];
        if (e && e != g_PlayerObject && e != coop.p2 && e->sprite_record == rec && !is_any_enemy(e)) return e;
    }
    return 0;
}

// The actor becomes the fighter: same spot, same facing, the body comes from the same sprite.
static Entity *swap_in(const ChBoss *b, const ChRoom *r, int i) {
    Entity *a = actor_of(tny_fighters[b->fighter].sprite);
    s32 x = a ? a->x >> 8 : r->mark[i][0], y = a ? a->y >> 8 : r->mark[i][1];
    int facing = a ? a->facing : 0;
    if (a) EntityList_Remove(g_SceneTable[1], a, 0);
    ch.rate = (s16)(b->rating + RATING_STEP * chamber_lights() + CH_LOOP_STEP * chamber_round());   // + the loop's (popo.h)
    ch.rate_id = b->fighter;
    ch.rate_pct = b->hp_pct;
    ch.rate_on = 1;
    Entity *e = tny_moves_room_spawn(b->fighter, x, y);
    ch.rate_on = 0;
    if (e) e->facing = (u8)facing;
    return e;
}

static void fight_begin(const ChRoom *r) {
    tny_rumble_mode(0);
    const ChBoss *lead = lead_of(r), *part = partner_of(r);
    // who stands decides, not the pad now: player 2 may have dropped out (or in) since the load stood the actors
    if (r->pair[1].fighter != NONE) {
        int stands = actor_of(tny_fighters[r->pair[1].fighter].sprite) != 0;
        part = stands ? &r->pair[1] : 0;
        lead = stands ? &r->pair[0] : &r->alone;
    }
    ch.boss[0] = swap_in(lead, r, 0);
    ch.boss[1] = part ? swap_in(part, r, 1) : 0;
    int nbar = 0;
    for (int i = 0; i < 2; i++) {
        if (!ch.boss[i]) continue;
        BossBar *bar = bossbar_create_hook(0, ch.boss[i], 0, 100);
        if (!bar) continue;
        if (nbar == 1) bar->y_target = 20;          // the second under the first, as the ring's
        EntityList_Add(g_SceneTable[1], (Entity *)bar);
        nbar++;
    }
    ch.state = R_FIGHT;
}

static void bars_out(void) {
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) {
        BossBar *b = (BossBar *)l->items[i];
        if (b && (b->vtable == (u32 *)BOSSBAR_VTABLE || b->vtable == coop2.bar_vtable)) b->step = -1;
    }
}

// A fallen fighter blinks out, through the transparent remap the Rumble's use (tny_engine.c blink)
static const u8 clear_remap[256];
static void blink(Entity *e, int off) {
    const u8 *want = off ? clear_remap : 0, *now = *(const u8 **)((u8 *)e + 0x24);
    if (now == want || (now && now != clear_remap)) return;
    *(const u8 **)((u8 *)e + 0x24) = want;
    *(u32 *)((u8 *)e + 0x20) = 0;                   // the next frame is decoded through it
}

static void bodies_gone(void) {
    for (int i = 0; i < 2; i++) {
        Entity *e = ch.boss[i];
        ch.boss[i] = 0;
        if (!e || !in_list(e)) continue;
        EntityList_Broadcast(g_SceneTable[1], 0xF, e);
        EntityList_Remove(g_SceneTable[1], e, 0);
    }
}

static void won_talk(const ChRoom *r) {
    const ChBoss *lead = lead_of(r);
    const u8 portrait = tny_fighters[lead->fighter].portrait;
    const char *line = lead->won;
    tny_talk_scene(0, &portrait, &line, line ? 1 : 0, 0);
}

// RF_CREW: the crew's actors become regular enemies on their spots (twins with two players, as everywhere); the lead stays
// an actor and watches
static void crew_begin(const ChRoom *r) {
    static const u32 no_action[2] = { ENEMY_ACTION_NONE, 0 };
    int hard = chamber_lights() >= CREW_HARD_LIGHTS;
    const void *rec_of = Character_GetSpriteRecord(r->crew_sprite);
    ch.crew = 1;
    ch.state = R_CREW;
    ch.timer = 0;
    Entity *crew[6];
    int n = 0;
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count && n < 6; i++) {
        Entity *a = l->items[i];
        if (a && a != g_PlayerObject && a != coop.p2 && a->sprite_record == rec_of && !is_any_enemy(a)) crew[n++] = a;
    }
    for (int i = 0; i < n; i++) {
        Entity *a = crew[i];
        // on the map's heap, not the stack: alone, a regular enemy with the ROM's stat is the stock Enemy_Create, which keeps
        // the record's address (the heap goes with the map, as the bodies do)
        struct CrewRec { EnemyRecord r; const void *beh[1]; } *rec = Arena_Alloc(sizeof(struct CrewRec));
        if (!rec) break;
        memset(rec, 0, sizeof *rec);
        rec->r.handler = (void *)0x0800E77Fu;
        rec->r.stat_index = hard ? CREW_STAT_HARD : CREW_STAT;
        rec->r.sprite_id = hard ? CREW_HARD_SPRITE : r->crew_sprite;
        rec->r.x = (s16)(a->x >> 8);
        rec->r.y = (s16)(a->y >> 8);
        rec->r.action = ch.action ? (void *)ch.action : (void *)no_action;   // the open area's drop (ch_enemy_drop)
        rec->r.behaviour_count = 1;
        rec->beh[0] = (const void *)WANDER;
        EntityList_Remove(g_SceneTable[1], a, 0);
        Entity *e = enemy_spawn_hook(&rec->r);
        if (e) EntityList_Add(g_SceneTable[1], e);  // the hook adds the twin itself
    }
}

static int enemies_standing(void) {
    int n = 0;
    EntityList *l = g_EntityList;
    for (int i = 0; i < l->count; i++) if (is_any_enemy(l->items[i]) && l->items[i]->hp > 0) n++;
    return n;
}

// The crew is down: the lead's line as he steps up (ch_crew_parts: its last entry is CH_FIGHT again, so R_ARMED follows)
static void crew_talk(const ChRoom *r) {
    const void *const *parts = ch.crew_parts;
    const u8 portrait = tny_fighters[lead_of(r)->fighter].portrait;
    const char *line = r->crew_line;
    ch.state = R_CREW_TALK;
    ch.timer = 0;
    if (!parts || !tny_talk_scene(parts[0], &portrait, &line, line ? 1 : 0, parts[1])) ch.state = R_ARMED;
}

// RF_RAGE: his partner fell first. The lead rages once: full HP and a power-up (tny_moves_rage), and his line
static void rage_check(const ChRoom *r) {
    Entity *lead = ch.boss[0], *part = ch.boss[1];
    if (ch.raged || !part || !lead || !in_list(lead) || lead->hp <= 0 || (in_list(part) && part->hp > 0)) return;
    ch.raged = 1;
    tny_moves_rage(lead);
    if (r->rage_line) {
        const u8 portrait = tny_fighters[lead_of(r)->fighter].portrait;
        const char *line = r->rage_line;
        tny_talk_scene(0, &portrait, &line, 1, 0);
    }
}

static void room_tick(const ChRoom *r) {
    switch (ch.state) {
    case R_ARMED:                                   // the scene that ends with CH_FIGHT has ended
        if (scene_running()) break;
        if ((r->flags & RF_CREW) && !ch.crew) crew_begin(r);
        else fight_begin(r);
        break;
    case R_CREW:                                    // the crew fights; the lead waits for the last of them
        if (enemies_standing()) { ch.timer = 0; break; }
        if (scene_running() || ++ch.timer < CREW_PAUSE) break;
        crew_talk(r);
        break;
    case R_CREW_TALK:                               // its last entry sets R_ARMED; a failsafe if it never ran
        if (!scene_running() && ++ch.timer > 240) ch.state = R_ARMED;
        break;
    case R_FIGHT: {
        if (r->flags & RF_RAGE) rage_check(r);
        int alive = 0;
        for (int i = 0; i < 2; i++) if (ch.boss[i] && in_list(ch.boss[i]) && ch.boss[i]->hp > 0) alive++;
        if (!alive) { ch.state = R_FALL; ch.timer = 0; bars_out(); }
        break;
    }
    case R_FALL:
        if (scene_running() || ++ch.timer < CH_FALL_FRAMES) break;
        ch.state = R_TALK;
        ch.timer = 0;
        won_talk(r);
        break;
    case R_TALK:                                    // queued, then on screen: its end brings the bodies' blink
        if (scene_running()) { ch.timer = 1; break; }
        if (ch.timer == 0 && lead_of(r)->won && ++ch.tick < 120) break;   // not on screen yet (a failsafe of 2 s)
        ch.state = R_CLEAR;
        ch.timer = 0;
        break;
    case R_CLEAR:
        ch.timer++;
        for (int i = 0; i < 2; i++) if (ch.boss[i] && in_list(ch.boss[i])) blink(ch.boss[i], (ch.timer >> 2) & 1);
        if (ch.timer >= CH_BLINK) { bodies_gone(); ch.state = R_WON; }
        break;
    default:
        break;
    }
}

// ---- the open area's scenes (docs/time-chamber.md 13): the opening of a run, the question at the door home, the training's end
// One sequence (data/story/chamber.lscript ch_open_scenes, handed over by the open area's on-load); its entry 0 asks
// CH_SCENE_ROUTE where to start. A queued scene starts once player 1 is free (as Popo's do), through coop2's action object.
#define SCENE_WAIT     20            // frames of player 1 free before a queued scene starts
#define DOOR_X1        976           // the quarters' door: a player here pressing Up is leaving
#define DOOR_X2        1048
#define DOOR_Y2        1250          // (the door's wall stops a player at about y 1234)
#define PORTAL_DECO    25            // the open map's decorations 25 / 26: the portal's swirl and its half-turned copy
#define PORTAL_FRAMES  10
#define CMD_BATCH      ((u32 *)0x030020F8)
ROM_FN(void *, Cmd_Walk, (void *mem, const s32 *xy), 0x08026621);
ROM_FN(void, Cmd_Queue, (u32 *batch, void *cmd, Entity *e), 0x0800E04D);

static Entity *p2_here(void) { return coop.p2 && in_list(coop.p2) ? coop.p2 : 0; }
static void queue_scene(int kind) { ch.scene = (u8)kind; ch.wait2 = 0; }

static void walk_to(Entity *e, s32 x, s32 y) {      // inside the script's command batch
    if (!e || !in_list(e) || *(u32 *)((u8 *)e + 0x150)) return;
    s32 xy[2] = { x, y };
    void *c = Cmd_Walk(0, xy);
    if (c) Cmd_Queue(CMD_BATCH, c, e);
}

static int at_door(const Entity *e, const InputSystem *in) {
    if (!e) return 0;
    s32 x = e->x >> 8, y = e->y >> 8;
    return x >= DOOR_X1 && x < DOOR_X2 && y < DOOR_Y2 && y > DOOR_Y2 - 48 && (in->current & KEY_UP);
}
static int near_door(const Entity *e) {
    if (!e) return 0;
    s32 x = e->x >> 8, y = e->y >> 8;
    return x >= DOOR_X1 - 8 && x < DOOR_X2 + 8 && y < DOOR_Y2 + 4 && y > DOOR_Y2 - 48;
}

// Either player walking up into the quarters' door: the question (or, the training complete, the way out at once). Asked
// again only once both have stepped away from the door.
static void exit_watch(void) {
    Entity *p1 = g_PlayerObject, *p2 = p2_here();
    if (ch.scene) return;
    if (ch.bits & B_EXIT_HELD) {
        if (!near_door(p1) && !near_door(p2)) ch.bits &= (u8)~B_EXIT_HELD;
        return;
    }
    if (at_door(p1, g_InputSystem) || (p2 && !coop_solo() && at_door(p2, &coop.input2))) {
        ch.bits |= B_EXIT_HELD;
        queue_scene(SC_EXIT);
    }
}

static void bodies_clear(void) {                    // the void's regular enemies leave (a scene over the whole map)
    EntityList *l = g_EntityList;
    for (int i = l->count - 1; i >= 0; i--) {
        Entity *e = l->items[i];
        if (!is_any_enemy(e)) continue;
        EntityList_Broadcast(g_SceneTable[1], 0xF, e);
        EntityList_Remove(g_SceneTable[1], e, 0);
    }
    ch.wait = CH_REFILL_WAIT;
}

static void portal_tick(void) {
    if (!(ch.bits & B_PORTAL) || ++ch.anim < PORTAL_FRAMES) return;
    ch.anim = 0;
    ch.bits ^= B_PHASE;
    int b = (ch.bits & B_PHASE) != 0;
    step1(b ? STEP_FLAG_HIDE : STEP_FLAG_SHOW, PORTAL_DECO);
    step1(b ? STEP_FLAG_SHOW : STEP_FLAG_HIDE, PORTAL_DECO + 1);
}

// input.c, every poll: during the opening Start or B (either pad) asks for the skip; from then on the pads press A every other
// frame and nothing else, so a box on screen closes and the scene runs to its end (CH_GO / CH_GOING route it there)
void chamber_input(u16 *p1, u16 *p2) {
    if (ch.magic != CH_MAGIC || (ch.scene & (SC_RUNNING | SC_KIND)) != (SC_RUNNING | SC_OPENING)) return;
    if ((*p1 | *p2) & (KEY_START | KEY_B)) ch.scene |= SC_SKIP;
    if (!(ch.scene & SC_SKIP)) return;
    *p1 = (g_FrameCounter & 2) ? KEY_A : 0;
    *p2 = 0;
}

static void scene_tick(void) {
    int kind = ch.scene & SC_KIND;
    if (!kind) return;
    if (ch.scene & SC_RUNNING) {                    // the scene's last entry clears it (CH_SCENE_END); a failsafe if it never
        if (scene_running()) ch.wait2 = 0;          // got there: two seconds with no scene, and the door is watched again
        else if (++ch.wait2 > 120) ch.scene = 0;
        return;
    }
    if (scene_running() || !p1_controllable() || !ch.open_seq) { ch.wait2 = 0; return; }
    if (++ch.wait2 < ((ch.scene & SC_KIND) == SC_PORTAL ? 2 : SCENE_WAIT)) return;
    ch.scene |= SC_RUNNING;
    ch.wait2 = 0;
    coop_run_sequence(coop2.call_action, ch.open_seq);
}

void chamber_tick(void) {
    state_check();
    const u8 *m = map_here();
    if (!m || m[0] != CH_ZONE) { ch.state = R_IDLE; ch.scene = 0; ch.bits &= (u8)~B_PORTAL; return; }
    if (m[1] == CH_OPEN_AREA) {                     // a lever's cut-away too: a scene, open_tick does nothing
        portal_tick();
        scene_tick();
        open_tick();
        return;
    }
    ch.bits &= (u8)~B_PORTAL;
    if (!(ch.scene & SC_RUNNING)) ch.scene = 0;     // the opening's views of the rooms keep it
    if (m[1] == POPO_AREA) { ch.state = R_IDLE; popo_tick(); return; }   // the hidden room (popo.c)
    const ChRoom *r = room_here();
    if (!r) { ch.state = R_IDLE; return; }
    if (!g_PlayerObject || !in_list(g_PlayerObject)) return;   // a game over's title, a load: nothing below is valid
    room_tick(r);
}

// ---- the script's calls (CoopCall 35, data/story/chamber.lscript) --------------------------------------------------------
enum {
    CH_ROOM_LOAD = 1,   // the room's on-load: -> 1 when its lever is thrown
    CH_BOSS_COND,       // a boss actor's record condition: the lever not thrown
    CH_PARTNER_COND,    // the partner actor's: and two players
    CH_INTRO,           // a = seq:ch_room_parts: the scene (walk-in, lines, fight)
    CH_STAGE,           // the walk-in's first step: the players turn to the far end, player 2 comes up past the door
    CH_LEAD_SPRITE, CH_LEAD_X, CH_LEAD_Y,          // -> the lead's sprite, his mark
    CH_HAS_PARTNER,     // -> 1 when a partner stands
    CH_PARTNER_SPRITE, CH_PARTNER_X, CH_PARTNER_Y,
    CH_FIGHT,           // the door seals, the fight starts when the scene ends
    CH_LEVER,           // -> the next entry of ch_lever: 1 the click, 3 throw
    CH_ALL_SIX,         // -> 1 when all six are out
    CH_LIGHT,           // -> the decoration of this room's light (in the cut-away)
    CH_JAR,             // a jar's drop
    CH_DROP,            // a regular enemy's drop
    CH_SET_FLAG,        // the lever's flag
    CH_OPEN,            // the door opens (back in the room)
    CH_STEP_X, CH_STEP_Y,   // -> where the lead steps up to after his crew (RF_CREW, ch_crew_parts)
    // docs/time-chamber.md 13 (Ryan's first play)
    CH_OPEN_LOAD,       // 23 the open area's on-load: a = seq:ch_open_scenes; the opening (a new run) or the training's end queued
    CH_SCENE_ROUTE,     // 24 -> ch_open_scenes' entry for the queued scene
    CH_OPEN_STAGE,      // 25 the opening starts: the run is under way (flag 510), the void's bodies leave, the players stand
    CH_LOOK,            // 26 the players face a
    CH_GO,              // 27 -> a, or b once the player skipped the opening (Start or B)
    CH_SHOW,            // 28 the opening's view of room a: its lead (and partner with two players) stand at the far end
    CH_SHOT_X, CH_SHOT_Y,   // 29 / 30 -> room a's view b (0 the plaque, 1 the boss), top-left
    CH_AREA,            // 31 -> room a's map area
    CH_GOING,           // 32 -> 1 while the opening is not skipped
    CH_SCENE_END,       // 33 the open area's scene is over
    CH_EXIT_STAGE,      // 34 the players face the door
    CH_EXIT_ROUTE,      // 35 -> the way out: the question (progress to lose, or none), or out at once (the training complete)
    CH_STEP_BACK,       // 36 (in a command batch) the players step back from the door
    CH_RESET_RUN,       // 37 leaving: the six lights on again, the run over (flags 504-510, 530)
    CH_DONE_STAGE,      // 38 the training's end: seen this visit; the players face down
    CH_PORTAL,          // 39 a = 1: the portal opens now (the sixth lever's cut-away, the on-load with six out)
    CH_CUT_X, CH_CUT_Y, // 40 / 41 -> the lever's cut-away starts at its room's entrance (the view's top-left)
    CH_LEG,             // 42 -> 1 when leg a of the way to the gate exists
    CH_LEG_X, CH_LEG_Y, // 43 / 44 -> leg a's end (the view's top-left)
    CH_LEG_WAIT,        // 45 -> leg a's frames
    CH_WAYS,            // 46 the room's side doorways open (a room's on-load once its lever is thrown)
    CH_PRIZE_OWED,      // 47 -> 1 when the lever pays a prize (popo.c's rule: the first win, or any win once round)
    CH_PRIZE_ITEM,      // 48 -> its item (and it is paid)
    CH_PRIZE_TEXT,      // 49 -> ch_lever's entry of its box
    CH_GOHAN,           // 50 -> ch_lever's entry: Gohan's Super Saiyan now (Gohan in the party, before the Games), else the end
    CH_FIND,            // 51 -> ch_find's entry: the side room's find (its first time) or the look
    CH_FIND_ITEM,       // 52 -> the side room's item (the find is taken: flag 531 + n)
    CH_PORTAL_GO,       // 53 the portal's trigger: with all six out, the warp into the hidden room is queued (a scene of its own)
};
_Static_assert(CH_FIGHT == 13 && CH_OPEN == 20 && CH_STEP_Y == 22 && CH_OPEN_LOAD == 23 && CH_PORTAL == 39 && CH_PORTAL_GO == 53,
               "the op numbers data/story/chamber.lscript uses");

// ch_open_scenes' entries (data/story/chamber.lscript; keep the `[n]` checks there)
#define E_OPENING      1
#define E_EXIT         28
#define E_EXIT_LOSE    29
#define E_EXIT_PLAIN   31
#define E_EXIT_NOW     34
#define E_DONE         36
#define E_PORTAL       42
// ch_lever's entries
#define E_PRIZE_SENZU  14            // 14-17: the Senzu, the Strength, Power, Endurance capsules' boxes
#define E_GOHAN        19
#define E_LEVER_END    21
// ch_find's
#define E_FIND0        1             // 1 + 2n: room n's find; 2 + 2n: its look once taken
#define ITEM_SENZU     0
#define ITEM_STR3      3
#define ITEM_PWR3      6
#define ITEM_END3      9

// The way from each room's entrance along the red carpet to the gate (centres, px; tools/chamberpath.py SEGMENTS):
// rooms[] order (light 0-5). The cut-away starts on the entrance and pans leg by leg (Ryan: "i literally couldnt find the
// gate or where to go after i beat 18").
#define GATE_X 1028
#define GATE_Y 360
#define CUT_SPEED 8                  // px a frame (PanCameraToPosition's speed)
static const s16 way[CH_ROOMS][8][2] = {
    { { 1616, 1704 }, { 1616, 1792 }, { 1016, 1792 }, { 1016, 1304 }, { 1160, 1304 }, { 1160, 1000 }, { 1028, 1000 }, { GATE_X, GATE_Y } },
    { { 1808, 1096 }, { 1808, 1304 }, { 1616, 1304 }, { 1616, 616 }, { 1028, 616 }, { GATE_X, GATE_Y } },
    { { 1616, 520 }, { 1616, 616 }, { 1028, 616 }, { GATE_X, GATE_Y } },
    { { 432, 520 }, { 432, 616 }, { 1028, 616 }, { GATE_X, GATE_Y } },
    { { 432, 1704 }, { 432, 1792 }, { 1016, 1792 }, { 1016, 1304 }, { 880, 1304 }, { 880, 1000 }, { 1028, 1000 }, { GATE_X, GATE_Y } },
    { { 240, 1096 }, { 240, 1304 }, { 432, 1304 }, { 432, 616 }, { 1028, 616 }, { GATE_X, GATE_Y } },
};
static s32 view_of(s32 c, int y) {
    s32 v = c - (y ? 80 : 120), max = y ? CH_MAP_H - 160 : CH_MAP_W - 240;
    return v < 0 ? 0 : v > max ? max : v;
}
static int way_len(int room) { int n = 0; while (n < 8 && way[room][n][0]) n++; return n; }

static const ChRoom *room_k(int k) {             // the opening's rooms: rooms[] order, the build's own row
    if (k < 0 || k >= ROOM_COUNT) k = 0;
    return rooms[k].plain && !have_buu() ? rooms[k].plain : &rooms[k];
}

static void stand_actor(int sprite, s32 x, s32 y) {
    static const u32 face_wait[2] = { 0x0800CB0Fu, 0x7D00 };    // NpcBehavior_FaceAndWait: face down
    ActorRecord *rec = Arena_Alloc(sizeof(ActorRecord));      // the map's heap: it goes with the map
    if (!rec || !Character_GetSpriteRecord(sprite)) return;
    memset(rec, 0, sizeof *rec);
    rec->handler = (void *)0x0800D6DFu;
    rec->x = (s16)x;
    rec->y = (s16)y;
    rec->sprite_id = sprite;
    rec->behaviour_count = 1;
    rec->behaviours[0] = face_wait;
    Entity *n = MapScript_CreateCharacter(rec);
    if (n) { EntityList_Add(g_SceneTable[1], n); n->facing = 0; }
}

static void ways(const ChRoom *r, int open) {
    for (int i = 0; i < 2; i++) {
        const s16 *w = r->ways[i];
        if (!w[2]) continue;
        step4(open ? STEP_WALK_RECT : STEP_BLOCK_RECT, w[0], w[1], w[2], w[3]);
    }
}

static int find_room(void) {
    const u8 *m = map_here();
    int n = m && m[0] == CH_ZONE ? m[1] - CH_FIND_AREA0 : -1;
    return n >= 0 && n < 3 ? n : -1;
}

static void reset_run(void) {
    for (int i = 0; i < CH_ROOMS; i++) flag_clear_hook(g_Party, CH_FLAG_LEVER0 + i);
    flag_clear_hook(g_Party, CH_FLAG_RUN);
    flag_clear_hook(g_Party, CH_FLAG_ROUND_WON);
    ch.bits &= (u8)~B_DONE_SEEN;
}

// The room whose lever runs: the map says while in the room; in the cut-away the map is the open area, so CH_LEVER notes it
static const ChRoom *cut_room(void) { return &rooms[ch.cut < ROOM_COUNT ? ch.cut : 0]; }
int chamber_cut_light(void) { state_check(); return cut_room()->light; }   // popo.c: the room prize after the lever

int chamber_call(s32 op, s32 a, s32 b, s32 *push) {
    state_check();
    const ChRoom *r = room_here();
    switch (op) {
    case CH_ROOM_LOAD:
        ch.state = R_IDLE;
        ch.boss[0] = ch.boss[1] = 0;
        ch.crew = ch.raged = 0;
        ch.crew_parts = 0;
        *push = r && lever_thrown(r);
        return 1;
    case CH_WAYS: if (r) ways(r, 1); return 0;
    case CH_BOSS_COND:
        *push = r && !lever_thrown(r);
        return 1;
    case CH_PARTNER_COND:
        *push = r && !lever_thrown(r) && partner_of(r) != 0;
        return 1;
    case CH_INTRO: {
        if (!r || ch.state != R_IDLE) return 0;
        const void *const *parts = (const void *const *)a;
        ch.crew_parts = (const void *const *)b;     // seq:ch_crew_parts (RF_CREW rooms use it)
        const ChBoss *lead = lead_of(r), *part = partner_of(r);
        int slot = g_Party->active_slot;
        const char *lines[4];
        u8 faces[4];
        int n = 0;
        const char *open = r->open_by && slot < CH_LINE_SLOTS ? r->open_by[slot] : 0;
        if (!open && r->open_by && two_players() && coop.p2 && coop.p2_slot < CH_LINE_SLOTS) open = r->open_by[coop.p2_slot];
        if (!open) open = r->open;
        faces[n] = tny_fighters[lead->fighter].portrait; lines[n++] = open;
        if (part && r->partner_line) { faces[n] = tny_fighters[part->fighter].portrait; lines[n++] = r->partner_line; }
        if (slot < CH_LINE_SLOTS && replies[slot]) { faces[n] = tny_fighters[slot_fighter[slot]].portrait; lines[n++] = replies[slot]; }
        if (coop.p2 && coop.p2_slot < CH_LINE_SLOTS && two_players() && replies[coop.p2_slot]) {
            faces[n] = tny_fighters[slot_fighter[coop.p2_slot]].portrait;
            lines[n++] = replies[coop.p2_slot];
        }
        tny_talk_scene(parts[0], faces, lines, n, parts[1]);
        return 0;
    }
    case CH_STAGE: {
        Entity *p1 = g_PlayerObject, *p2 = coop.p2;
        int look = r ? r->look : LOOK_UP;
        if (p1) p1->facing = (u8)look;
        if (p2) p2->facing = (u8)look;
        if (r && p1 && p2) {                        // the door's strip seals: he must not be behind it
            if (look == LOOK_LEFT && (p2->x >> 8) > r->line) {
                s32 dy = open_ground(p1, p1->x >> 8, (p1->y >> 8) + 24) ? 24 : -24;
                p2->x = p1->x;
                p2->y = p1->y + (dy << 8);
            } else if (look != LOOK_LEFT && (p2->y >> 8) > r->line) {
                s32 dx = open_ground(p1, (p1->x >> 8) + 24, p1->y >> 8) ? 24 : -24;
                p2->x = p1->x + (dx << 8);
                p2->y = p1->y;
            }
        }
        return 0;
    }
    case CH_LEAD_SPRITE: *push = r ? tny_fighters[lead_of(r)->fighter].sprite : 0; return 1;
    case CH_LEAD_X: *push = r ? r->mark[0][0] : 0; return 1;
    case CH_LEAD_Y: *push = r ? r->mark[0][1] : 0; return 1;
    case CH_HAS_PARTNER: *push = r && partner_of(r) != 0; return 1;
    case CH_PARTNER_SPRITE: *push = r && partner_of(r) ? tny_fighters[partner_of(r)->fighter].sprite : 0; return 1;
    case CH_PARTNER_X: *push = r ? r->mark[1][0] : 0; return 1;
    case CH_PARTNER_Y: *push = r ? r->mark[1][1] : 0; return 1;
    case CH_STEP_X:                                 // the lead's step up after his crew: 24 px towards the players
        *push = r ? r->mark[0][0] + (r->look == LOOK_LEFT ? 24 : 0) : 0;
        return 1;
    case CH_STEP_Y: *push = r ? r->mark[0][1] + (r->look == LOOK_LEFT ? 0 : 24) : 0; return 1;
    case CH_FIGHT:
        if (r) {
            step4(STEP_BLOCK_RECT, r->seal[0], r->seal[1], r->seal[2], r->seal[3]);
            step1(STEP_FLAG_SHOW, DECO_DOOR_BEAM);
            ch.state = R_ARMED;
        }
        return 0;
    case CH_LEVER:
        *push = 1;
        if (r && ch.state == R_WON && !lever_thrown(r)) {
            ch.cut = (u8)room_index_here();         // for the cut-away (cut_room)
            *push = 3;
        }
        return 1;
    case CH_LIGHT: *push = cut_room()->light; return 1;
    case CH_SET_FLAG: flag_set_hook(g_Party, lever_flag(cut_room())); return 0;
    case CH_ALL_SIX: *push = chamber_lights() == CH_ROOMS; return 1;
    case CH_OPEN:
        if (r) {
            step4(STEP_WALK_RECT, r->seal[0], r->seal[1], r->seal[2], r->seal[3]);
            step1(STEP_FLAG_HIDE, DECO_DOOR_BEAM);
            ways(r, 1);                             // the side rooms (Broly's ship, Frieza's throne room)
        }
        return 0;
    // ---- docs/time-chamber.md 13 ----
    case CH_OPEN_LOAD: {
        ch.open_seq = (const void *)a;
        ch.bits &= (u8)~(B_PORTAL | B_EXIT_HELD);
        if (ch.scene & SC_RUNNING) return 0;        // the opening's way back from its views of the rooms
        ch.scene = 0;
        if (!flag_test_hook(g_Party, CH_FLAG_RUN)) queue_scene(SC_OPENING);
        else if (training_done() && !(ch.bits & B_DONE_SEEN)) queue_scene(SC_DONE);
        Entity *p1 = g_PlayerObject;               // arriving by the door (the Lookout's) stands in its strip: no question
        if (p1 && near_door(p1)) ch.bits |= B_EXIT_HELD;
        return 0;
    }
    case CH_SCENE_ROUTE: {
        int k = ch.scene & SC_KIND;
        *push = k == SC_EXIT ? E_EXIT : k == SC_DONE ? E_DONE : k == SC_PORTAL ? E_PORTAL : E_OPENING;
        return 1;
    }
    case CH_OPEN_STAGE:
        flag_set_hook(g_Party, CH_FLAG_RUN);
        bodies_clear();
        __attribute__((fallthrough));               // the players face down, towards the camera
    case CH_LOOK: {
        Entity *p2 = p2_here();
        int f = op == CH_OPEN_STAGE ? 0 : a;
        if (g_PlayerObject) g_PlayerObject->facing = (u8)f;
        if (p2) p2->facing = (u8)f;
        return 0;
    }
    case CH_GO: *push = (ch.scene & SC_SKIP) ? b : a; return 1;
    case CH_GOING: *push = !(ch.scene & SC_SKIP); return 1;
    case CH_SHOW: {
        const ChRoom *k = room_k(a);
        const ChBoss *lead = lead_of(k), *part = partner_of(k);
        stand_actor(tny_fighters[lead->fighter].sprite, k->stand[0][0], k->stand[0][1]);
        if (part) stand_actor(tny_fighters[part->fighter].sprite, k->stand[1][0], k->stand[1][1]);
        return 0;
    }
    case CH_SHOT_X: {                               // the boss's view: alone, centred on him (Frieza stands far left)
        const ChRoom *k = room_k(a);
        s32 x = k->shot[b & 1][0];
        if ((b & 1) && !partner_of(k)) { x = k->stand[0][0] - 120; if (x < 0) x = 0; }
        *push = x;
        return 1;
    }
    case CH_SHOT_Y: *push = room_k(a)->shot[b & 1][1]; return 1;
    case CH_AREA: *push = room_k(a)->area; return 1;
    case CH_SCENE_END: ch.scene = 0; return 0;
    case CH_EXIT_STAGE: {
        Entity *p2 = p2_here();
        if (g_PlayerObject) g_PlayerObject->facing = LOOK_UP;
        if (p2) p2->facing = LOOK_UP;
        return 0;
    }
    case CH_EXIT_ROUTE: *push = training_done() ? E_EXIT_NOW : chamber_lights() ? E_EXIT_LOSE : E_EXIT_PLAIN; return 1;
    case CH_STEP_BACK: {
        Entity *p1 = g_PlayerObject, *p2 = p2_here();
        if (p1) walk_to(p1, p1->x >> 8, (p1->y >> 8) + 24);
        if (p2) walk_to(p2, p2->x >> 8, (p2->y >> 8) + 24);
        return 0;
    }
    case CH_RESET_RUN: reset_run(); return 0;
    case CH_DONE_STAGE: {
        Entity *p2 = p2_here();
        ch.bits |= B_DONE_SEEN;
        if (g_PlayerObject) g_PlayerObject->facing = 0;
        if (p2) p2->facing = 0;
        return 0;
    }
    case CH_PORTAL_GO: if (chamber_lights() == CH_ROOMS && !(ch.scene & SC_RUNNING)) queue_scene(SC_PORTAL); return 0;
    case CH_PORTAL:
        if (a && !(ch.bits & B_PORTAL)) { ch.bits |= B_PORTAL; ch.bits &= (u8)~B_PHASE; ch.anim = PORTAL_FRAMES; }
        return 0;
    case CH_CUT_X: *push = view_of(way[cut_room()->light][0][0], 0); return 1;
    case CH_CUT_Y: *push = view_of(way[cut_room()->light][0][1], 1); return 1;
    case CH_LEG: *push = a >= 0 && a + 1 < way_len(cut_room()->light); return 1;
    case CH_LEG_X: case CH_LEG_Y: case CH_LEG_WAIT: {
        int n = way_len(cut_room()->light), i = a < 0 ? 0 : a + 1 >= n ? n - 1 : a + 1;
        const s16 *to = way[cut_room()->light][i], *from = way[cut_room()->light][i ? i - 1 : 0];
        if (op == CH_LEG_X) *push = view_of(to[0], 0);
        else if (op == CH_LEG_Y) *push = view_of(to[1], 1);
        else {
            s32 dx = view_of(to[0], 0) - view_of(from[0], 0), dy = view_of(to[1], 1) - view_of(from[1], 1);
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            *push = (dx > dy ? dx : dy) / CUT_SPEED + 4;
        }
        return 1;
    }
    case CH_PRIZE_OWED: { s32 v = 0; popo_call(19, 0, 0, &v); *push = v; return 1; }      // popo.c POPO_ROOM_OWED
    case CH_PRIZE_ITEM: { s32 v = 0; popo_call(20, 0, 0, &v); ch.prize = (u8)v; *push = v; return 1; }   // POPO_ROOM_PRIZE
    case CH_PRIZE_TEXT:
        *push = ch.prize == ITEM_STR3 ? E_PRIZE_SENZU + 1 : ch.prize == ITEM_PWR3 ? E_PRIZE_SENZU + 2
              : ch.prize == ITEM_END3 ? E_PRIZE_SENZU + 3 : E_PRIZE_SENZU;
        return 1;
    case CH_GOHAN:      // Gohan's Super Saiyan (decision 6): at a lever before the Cell Games, only while Gohan is in the party
        *push = !flag_test_hook(g_Party, 468) && !flag_test_hook(g_Party, 193) && (g_Party->chars[0].flags & 1)
              ? E_GOHAN : E_LEVER_END;
        return 1;
    case CH_FIND: {
        int n = find_room();
        *push = n < 0 ? 0 : E_FIND0 + 2 * n + (flag_test_hook(g_Party, CH_FLAG_FIND0 + n) ? 1 : 0);
        return 1;
    }
    case CH_FIND_ITEM: {
        static const u8 item[3] = { ITEM_END3, ITEM_SENZU, ITEM_PWR3 };   // the bed, the cold box, Frieza's pod
        int n = find_room();
        if (n < 0) { *push = ITEM_SENZU; return 1; }
        flag_set_hook(g_Party, CH_FLAG_FIND0 + n);
        *push = item[n];
        return 1;
    }
    case CH_JAR: drop_item(Rand_Range(100), 1); return 0;
    case CH_DROP: if (Rand_Range(100) < 25) drop_item(Rand_Range(100), 0); return 0;
    default:
        return 0;
    }
}

// ---- test command 48 (docs/ids.md) ----------------------------------------------------------------------------------------
void chamber_test(u32 arg, u32 arg2) {
    state_check();
    switch (arg) {
    case 0:                                         // arg2 bits 0-5: which levers are thrown; the run is under way
        for (int i = 0; i < CH_ROOMS; i++) {
            if ((arg2 >> i) & 1) flag_set_hook(g_Party, CH_FLAG_LEVER0 + i);
            else flag_clear_hook(g_Party, CH_FLAG_LEVER0 + i);
        }
        flag_set_hook(g_Party, CH_FLAG_RUN);
        break;
    case 7:                                         // flag 510 (a run under way: no opening at the next load) := arg2
        if (arg2) flag_set_hook(g_Party, CH_FLAG_RUN);
        else flag_clear_hook(g_Party, CH_FLAG_RUN);
        break;
    case 8:                                         // cmd_arg := the open area's scene | bits << 8 | the last prize << 16
        coop2.cmd_arg = (u32)ch.scene | (u32)ch.bits << 8 | (u32)ch.prize << 16;
        break;
    case 1: coop2.cmd_arg = (u32)ch.standing | (u32)ch.obj_low << 16; break;
    case 2: ch.target = (u8)arg2; break;            // records standing
    case 3: ch.frozen = (u8)arg2; break;            // the open area's tick stops
    case 4: ch.obj_low = 0xFFFF; break;
    case 5: wave_start(0, (int)arg2); break;        // the old rounds, without their captions, on zone 15 area 2 (waves.c)
    case 6: coop2.cmd_arg = (u32)chamber_lights() | (u32)ch.state << 8 | (u32)ch.raged << 16 | (u32)ch.crew << 24; break;
    default: break;
    }
}
