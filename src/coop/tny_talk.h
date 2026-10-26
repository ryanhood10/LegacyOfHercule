// The tournament's fighter dialogue (docs/tournament-dialogue.md: Ryan's approved plan; tny_talk.c; the data is
// generated from the doc by tools/tnytalk.py into build/gen/tny_talk_data.c)
#pragma once
#include "tourney.h"

enum { TALK_OPEN1, TALK_OPEN2, TALK_REPLY, TALK_WIN };
typedef struct { const char *l[4]; } TalkVoice;                                  // a fighter's own lines (0 = none)
typedef struct { u8 id[2]; u8 n; u8 spk[4]; const char *l[4]; } TalkPair;       // spk: which of id[] says l[k]
typedef struct { u8 id[4]; u8 n; u8 spk[4]; const char *l[4]; } TalkTvt;        // id[0..1] one team, id[2..3] the other
extern const TalkVoice tny_talk_voice[TF_COUNT];
extern const TalkPair tny_talk_pairs[];   // meeting yourself, then the rival exchanges; ends with id[0] 0xFF
extern const TalkPair tny_talk_teams[];
extern const TalkTvt tny_talk_tvt[];

// Queue the talk before a fight: `opp` the side that opens (the opponents, a challenger, player 2's pick in a
// player-vs-player final), `mine` the side that answers; 1 or 2 fighter ids each (0xFF = nobody). 1 = a talk runs.
int  tny_talk_pre(const u8 *opp, int nopp, const u8 *mine, int nmine);
// The winner's line after the end pose (1 = it runs)
int  tny_talk_win(int id);
// Every frame (tny_frame): Start skips the rest of a talk
void tny_talk_frame(void);
// Test command 31 (TNY_TALK): arg = the opening side (id | id << 8, 0xFF nobody), arg2 = the answering side; arg
// bit 31: the winner's line of arg's first id instead
void tny_talk_test(u32 cmd, u32 arg, u32 arg2);
