// Hercule's moveset (Ryan, 2026-11-02): the Flash and the Gun from the start, the Ki Blast after the three years
// (ch3_zone7.lscript); the Megaton Punch, the Taunt and Super Hercule are not his to select (data/hercule.json's flags,
// ssj.c hercule_lock). Abilities added to the (relocated) ability table by data/hercule.json.
// Each is an {isReady, onUse} pair like the stock ones: isReady checks the game's ability gate and
// the EP cost, onUse spends EP and runs an animation (a control state from the table at 0x083B5268)
// that Hercule's sprite record (72) has frames for. Tested in mGBA on sprite 72: animations 5, 34,
// 44, 45, 46 and 47 run (they read the group-5 "ki pose" frames), 43 (Energy Punch) crashes and the
// beam animations 48-51 read groups 18/19 that sprite 72 does not have; the idle fidget (14) needs
// group 14, which Hercule also lacks.
//   14 Flash                  animation 5  (his stock move: the pose, everyone on screen dazed, a grunt), 4 EP
//   15 Ki Blast               animation 34 (one quick projectile), the stock 2 EP
//   16 Megaton Punch          animation 46 (Big Bang: the big slow projectile), 12 EP; tournament fighters only
//   17 Hercule Taunt          animation 31 (the wall-bump recoil, a flinch-pose) + grunt, restores 8 EP; tournament fighters only
//   25 Gun                    animation 34, the ki blast's own flight with a bullet's frames, faster and a shade weaker
//                             (kiblast_spawn_hook in krillin.c), 1 EP
// The stock onUse routines are reused (animation 5 spawns its shockwave object and plays its grunt);
// the EP difference to the stock cost is applied around the call.
#include "coop.h"

#define ABILITY_GATE() (*(u32 *)(*(u32 *)0x086DB284u + 0xC))   // same check as every stock isReady: word 2 of the ROM context record at 0x086DB27C -> object -> +0xC

ROM_FN(void, HerculePunch_OnUse, (CharacterEntry *, Entity *), 0x0801327D);  // ep -= 6, animation 5 + shockwave + SFX 40
ROM_FN(void, KiBlast_OnUse, (CharacterEntry *, Entity *), 0x080131E1);       // ep -= 2, animation 34
ROM_FN(void, BigBang_OnUse, (CharacterEntry *, Entity *), 0x08013179);       // ep -= 10, animation 46

#define TAUNT_EP 8
#define SFX_HERCULE_GRUNT 40   // the stock Hercule punch's voice clip (SFX_TABLE + 40 * 12)

static int ready(const CharacterEntry *c, int cost) {
    return ABILITY_GATE() != 0 && (s16)c->ep >= cost;
}

int hercule_punch_ready(const CharacterEntry *c) { return ready(c, 4); }
void hercule_punch_use(CharacterEntry *c, Entity *e) {
    HerculePunch_OnUse(c, e);   // takes 6
    c->ep += 2;                 // 4 in total
}

int hercule_kick_ready(const CharacterEntry *c) { return ready(c, 2); }
void hercule_kick_use(CharacterEntry *c, Entity *e) {
    KiBlast_OnUse(c, e);
}

int hercule_gun_ready(const CharacterEntry *c) { return ready(c, 1); }
void hercule_gun_use(CharacterEntry *c, Entity *e) {
    c->ep += 1;                 // 1 in total: the stock routine takes 2
    KiBlast_OnUse(c, e);
}

int hercule_megaton_ready(const CharacterEntry *c) { return ready(c, 12); }
void hercule_megaton_use(CharacterEntry *c, Entity *e) {
    c->ep -= 2;                 // 12 in total: the stock routine takes 10
    BigBang_OnUse(c, e);
}

int hercule_taunt_ready(const CharacterEntry *c) { return ABILITY_GATE() != 0 && c->ep < c->max_ep; }
void hercule_taunt_use(CharacterEntry *c, Entity *e) {
    int ep = c->ep + TAUNT_EP;
    c->ep = (u16)(ep > c->max_ep ? c->max_ep : ep);
    Entity_SetAnimation(e, ANIM_BUMP);
    Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + SFX_HERCULE_GRUNT * 12, 0x40, 0);
}
