// Super Saiyan 2 Gohan as his own party member (docs/story.md 4h, docs/hercule.md "Super Saiyan 2 Gohan"):
// ids shared by src/coop/ssj2gohan.c, src/coop/krillin.c (status screen), the generated tables
// (tools/spriteclone.py row 28, tools/ssj2sprites.py record 58, tools/krillinsprites.py slot-8 names) and
// data/hercule.json (growth row 8, ability 23).
#pragma once

#define SSJ2_SLOT           8       // party slot (PARTY_SLOTS >= 9); regular Gohan keeps slot 0
#define GOHAN_SLOT          0
#define SSJ2_ROW            28      // display row: Gohan's row 3 with row 4's sprite 58, portraits 61 / 12 and Super Saiyan 2 class
#define SSJ2_SPRITE         58      // the stock record, repointed to ssj2_record (hurt / lying / tumble filled)
#define SSJ2_BURST_ABILITY  23      // ability table entry (data/hercule.json)

// Joining (ssj2_join, the end of the Cell Games): Gohan's EXP (his level) and Gohan's numbers scaled.
// Growth row 8 in data/hercule.json keeps him far ahead as he levels.
#define SSJ2_HP_PCT         150
#define SSJ2_EP_PCT         150
#define SSJ2_STR_PCT        170
#define SSJ2_POW_PCT        170
#define SSJ2_END_PCT        170
#define SSJ2_FLAGS          0x7F    // in party, available, the charged attack and every technique page, 0x40
// Kamehameha, Masenko, Super Saiyan 2 Burst, Ki Blast (Gohan's list without his transformation: sprite 58
// has no transformation groups)
#define SSJ2_ABILITIES      { 3, 4, SSJ2_BURST_ABILITY, 2 }

// Super Saiyan 2 Burst (ability 23): the ki pose and a blast of aura that knocks back everything around him
#define BURST_EP            10
#define BURST_RADIUS_X      48      // half-size of the hit box around him (px)
#define BURST_RADIUS_Y      40
#define BURST_POWER         0x900   // attack base (Krillin's Shockwave Punch: 0x500)
#define BURST_SHAKE         16      // camera shake frames

#if PARTY_SLOTS > SSJ2_SLOT
void ssj2_join(void);           // the join (CoopCall 20): slot 8 enters the party
#endif
void ssj2_frame(void);          // every input poll (SSJ2_TEST triggers)
