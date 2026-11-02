// Player 2's agency (agency.c, docs/perfect-pass.md 11.3).
#pragma once
#include "coop.h"
void agency_map_tick(void);                       // input.c, every poll: the barriers for two, once a map
void agency_pads(u16 *p1, u16 *p2);               // input.c: what player 2 fired runs on his pad
void agency_scene_press(InputSystem *in, u16 pad2);   // input.c: his A and B in a scene
void agency_attack(Entity *e);                    // player.c: Player_Attack, noting who punches
int agency_menu_is_p2(void);                      // switch.c: the screen is player 2's
void agency_menu_choose(int slot);                // switch.c: his choice
int flag_cond_hook(u32 cond);                     // party.c
int p2_in_control(void);                          // player.c: player 2 stands, walks or dashes
int p2_body_fits(int slot);                       // player.c: that slot's body fits his entity's VRAM
