// Difficulty (docs/coop-design.md "Difficulty"): Easy, Normal or Hard, picked once when a new game starts
// (the first scene, data/story/ch1_opening). Stored in story flags 460 (Easy) and 461 (Hard); neither is
// Normal, so a save from before the choice plays Normal.
#ifndef DIFFICULTY_H
#define DIFFICULTY_H
#include "coop.h"

#define DIFF_EASY   0
#define DIFF_NORMAL 1
#define DIFF_HARD   2
#define DIFF_FLAG_EASY 460
#define DIFF_FLAG_HARD 461

//                       Easy  Normal  Hard   (Normal is the co-op tuning from before the setting)
#define DIFF_ENEMY_HP   { 100,  150,  200 }   // regular enemies' HP, percent, while player 2 is there
#define DIFF_DAMAGE     {  75,  125,  150 }   // damage enemies deal the players, percent, while player 2 is there
#define DIFF_SPEED      { 100,  120,  130 }   // regular enemies' walk speed, percent, while player 2 is there
#define DIFF_BOSS_HP    {  80,  100,  125 }   // bosses' HP, percent (every boss: stock, ours, Android 16, Super 13)
#define DIFF_REVIVE_S   {  20,   10,    6 }   // seconds a downed player waits for a revive (revive.c)
// Alone (no player 2) Normal is the stock game; Easy and Hard still change these:
#define DIFF_SOLO_ENEMY_HP { 100, 100, 130 }  // regular enemies' HP, percent, alone
#define DIFF_SOLO_DAMAGE   {  75, 100, 120 }  // damage enemies deal the player, percent, alone
// Drops (alone too): on Easy a regular enemy's drop roll (the five stock drop scripts) runs again when the first roll gave
// nothing; on Hard one roll in three is skipped. Story defeat scripts are never touched.
#define DIFF_HARD_SKIP 3

int difficulty(void);
int diff_enemy_hp_pct(void);                        // co-op or alone, by p2_coming()
int diff_damage_pct(void);                          // co-op or alone, by coop.p2
int diff_speed_pct(void);
int diff_boss_hp_pct(void);
int diff_revive_frames(void);
int diff_regular_twins(void);                       // Easy: only bosses get a twin
void *diff_drop_action(void *action);               // a regular enemy's record action, wrapped when it is a drop roll

#endif
