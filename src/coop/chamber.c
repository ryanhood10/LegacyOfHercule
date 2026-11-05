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
#define CAPSULE_PLACE .look = LOOK_UP, .mark = { { 120, 200 }, { 200, 200 } }, .seal = { 144, 296, 176, 304 }, .line = 290
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
    const void *const *crew_parts;   // RF_CREW: the scene's parts when the crew is down (ch_crew_parts, CH_INTRO's b)
} ChState;
_Static_assert(sizeof(ChState) <= 0x30, "docs/ids.md: .combo.ch at most 0x30 bytes");
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

static void open_tick(void) {
    if (ch.frozen || scene_running() || !p1_controllable()) return;
    Entity *p1 = g_PlayerObject;
    if (!p1 || !in_list(p1)) return;
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

void chamber_tick(void) {
    state_check();
    const u8 *m = map_here();
    if (!m || m[0] != CH_ZONE) { ch.state = R_IDLE; return; }
    if (m[1] == CH_OPEN_AREA) { open_tick(); return; }      // a lever's cut-away too: a scene, open_tick does nothing
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
};
_Static_assert(CH_FIGHT == 13 && CH_OPEN == 20 && CH_STEP_Y == 22, "the op numbers data/story/chamber.lscript uses");

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
        }
        return 0;
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
    case 0:                                         // arg2 bits 0-5: which levers are thrown
        for (int i = 0; i < CH_ROOMS; i++) {
            if ((arg2 >> i) & 1) flag_set_hook(g_Party, CH_FLAG_LEVER0 + i);
            else flag_clear_hook(g_Party, CH_FLAG_LEVER0 + i);
        }
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
