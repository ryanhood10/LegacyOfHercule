// Android 16 as a player for the "Gero's computer" chapter (docs/story.md 4g): ids shared by
// src/coop/a16player.c, the generated tables (tools/a16sprites.py a16p_record, tools/spriteclone.py row 25,
// tools/krillinsprites.py status portrait 121 and slot-7 names) and data/story/ch9_gero.*.
#pragma once

// Party slot 7, the duel clone's slot (DUEL_SLOT, rom.h; slot 8 is Super Saiyan 2 Gohan's).
// The clone duels that write it (chapters 3 and 7) come before the chapter, so 16 borrows it: CoopCall 11
// writes his entry there when the chapter starts; he stays selectable on the save pads after it (Ryan,
// 2026-10-20: the players trade Krillin and 16 at a pad) until CoopCall 12, 1 takes him out for good at the
// Cell Games, where he dies (the dojo's duel uses player 2's own slot, never this one).
#define A16_SLOT            DUEL_SLOT
#define A16_ROW             25      // display row (tools/spriteclone.py)
#ifndef A16P_SPRITE
#define A16P_SPRITE         175     // Makefile A16P_SPRITE: a16p_record
#endif

// Story flags (420-439 are this chapter's; docs/scripts.md 2.5)
#define A16_FLAG_ACTIVE     420     // the chapter runs: 16 is in the party (set by CoopCall 11)
#define A16_FLAG_DONE       421     // the chapter is over: the party lock is lifted (set by CoopCall 12)
#define A16_FLAG_RUBBLE     422     // 16 punched through the rubble at the cave mouth (zone 8 area 48)
#define A16_FLAG_GEN_WEST   423     // the generator in zone 8 area 27 is shut down again
#define A16_FLAG_GEN_EAST   424     // the generator in zone 8 area 40 is shut down again
#define A16_FLAG_BUNKER     425     // the two Android 19s are beaten and the computer is smashed
#define A16_FLAG_SAVED      430     // 430 + slot: that slot's "available" bit before the chapter's lock (slots 0-7)

// Stats: 16 joins at Krillin's level (same EXP) with Krillin's numbers scaled (stronger than Krillin:
// Hercule and Krillin stay the weaker fighters)
#define A16_HP_PCT          160
#define A16_EP_PCT          110
#define A16_STR_PCT         150
#define A16_POW_PCT         110
#define A16_END_PCT         150
#define A16_FLAGS           0x43    // in party, available, 0x40 like every stock row
#define ROCKET_ABILITY      22      // ability table entry (data/hercule.json)

// Rocket Punch (ability 22): the forearm flies along his facing (the boss's fist frames), explodes on
// the first enemy it touches (one hit) or at the end of its range, hurting every other enemy in the blast
#define ROCKET_EP           6
#define ROCKET_COOLDOWN     70      // frames between punches
#define ROCKET_SPEED        0x500   // 8.8 px per frame
#define ROCKET_RANGE        40      // frames of flight
#define ROCKET_POWER        0x500   // attack base (the ki blast's is 0x400); a melee-kind hit: 19 absorbs ranged ones
#define ROCKET_RADIUS       20      // blast half-size (px)

void a16_join(void);     // CoopCall 11
void a16_leave(void);    // CoopCall 12 (0): the chapter's end: the lock lifts, 16 stays selectable
void a16_gone(void);     // CoopCall 12, 1: the Cell Games: 16 leaves the party for good
