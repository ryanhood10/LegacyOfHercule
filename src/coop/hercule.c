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
// The Gun and the Ki Blast both run the ki blast's state (animation 34), whose enter (0x0800E248) shows group 5 of the
// body's record: the peace sign on 72 and 157 (the Flash's pose). They show their own poses instead (Ryan, 2026-11-06):
// group 22 the Gun (Buu's Fury's Hercule drawing his pistol: aimed, fired), group 23 the Ki Blast (a push: his guard,
// then the palm thrust out); tools/herculesprites.py, docs/sprites.md "Hercule's shooting poses". The state's update
// only moves the frame (+0x14: 0 for 8 frames, 1 from the shot, 1 / 2 for the extra shots of a held fire), so the
// group pointer written after the enter stays until the state ends. A record without the group keeps group 5.
// The stock onUse routines are reused (animation 5 spawns its shockwave object and plays its grunt);
// the EP difference to the stock cost is applied around the call.
#include "coop.h"

#define ABILITY_GATE() (*(u32 *)(*(u32 *)0x086DB284u + 0xC))   // same check as every stock isReady: word 2 of the ROM context record at 0x086DB27C -> object -> +0xC

ROM_FN(void, HerculePunch_OnUse, (CharacterEntry *, Entity *), 0x0801327D);  // ep -= 6, animation 5 + shockwave + SFX 40
ROM_FN(void, KiBlast_OnUse, (CharacterEntry *, Entity *), 0x080131E1);       // ep -= 2, animation 34
ROM_FN(void, BigBang_OnUse, (CharacterEntry *, Entity *), 0x08013179);       // ep -= 10, animation 46

#define TAUNT_EP 8
#define SFX_HERCULE_GRUNT 40   // the stock Hercule punch's voice clip (SFX_TABLE + 40 * 12)

#define POSE_GROUP 0x4C                  // entity +0x4C: the shown group (four facing arrays), +0x14 the frame
#define KIBLAST_GROUP 5                   // what animation 34's enter shows
#define HERC_GUN_GROUP 22                 // herc_record / ssj_record / old Hercule's buu_record_158
#define HERC_KI_GROUP 23                  // herc_record / ssj_record

// show group g of e's record in place of the ki blast's group 5, if the record has it in every facing and e is in the
// state that just showed group 5 (the onUse set the animation)
static void shoot_pose(Entity *e, int g) {
    const u8 *rec = e->sprite_record;
    if (!rec || g >= rec[2]) return;
    const u32 *dirs = (const u32 *)(rec + 0x4C + 0x10 * g);
    if (!dirs[0] || !dirs[1] || !dirs[2] || !dirs[3]) return;
    u32 *shown = (u32 *)((u8 *)e + POSE_GROUP);
    if (*shown != (u32)(rec + 0x4C + 0x10 * KIBLAST_GROUP)) return;
    *shown = (u32)dirs;
}

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
    shoot_pose(e, HERC_KI_GROUP);
}

int hercule_gun_ready(const CharacterEntry *c) { return ready(c, 1); }
void hercule_gun_use(CharacterEntry *c, Entity *e) {
    c->ep += 1;                 // 1 in total: the stock routine takes 2
    KiBlast_OnUse(c, e);
    shoot_pose(e, HERC_GUN_GROUP);
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
