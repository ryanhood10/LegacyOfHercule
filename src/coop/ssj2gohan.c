// Super Saiyan 2 Gohan as his own party member (docs/story.md 4h "Triggers and side content around the
// Games", docs/hercule.md "Super Saiyan 2 Gohan"): party slot 8 (PARTY_SLOTS 9), display row 28 (sprite 58,
// portraits 61 / 12), regular Gohan keeps slot 0.
//  * ssj2_join (CoopCall 20, proposed): at the end of the Cell Games he joins at Gohan's level with Gohan's
//    numbers scaled up (ssj2gohan.h); growth row 8 (data/hercule.json) keeps him far ahead. Until then slot
//    8 is empty (flags 0: not in the party). The calling script shows the join message.
//  * Super Saiyan 2 Burst (ability 23): the ki pose and a wide blast of aura around him.
//  * the status screen draws his technique pages as Gohan's and his title from the slot-8 names (krillin.c),
//    dialogue calls him "Gohan" (krillin.c speaker_slot).
#include "coop.h"
#include "krillin.h"
#include "ssj2gohan.h"

#define ABILITY_GATE() (*(u32 *)(*(u32 *)0x086DB284u + 0xC))   // the stock isReady gate (hercule.c)
#define SFX_KI_HIT   0x22    // the ki blast's impact
#define SFX_BOOM     0x14    // an explosion (android16.c)

static void sfx(int id) { Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + id * 12, 0x40, 0); }

#if PARTY_SLOTS > SSJ2_SLOT
static u16 pct(u32 v, u32 p) { v = v * p / 100; return (u16)(v > 0xFFFF ? 0xFFFF : v); }

void ssj2_join(void) {
    PartyState *p = g_Party;
    CharacterEntry *s = &p->chars[SSJ2_SLOT];
    if (s->flags & 1) return;                       // already joined: keep his own growth
    const CharacterEntry *g = &p->chars[GOHAN_SLOT];
    memset(s, 0, sizeof *s);
    s->display_row = SSJ2_ROW;
    s->exp = g->exp;                                 // Gohan's level
    s->max_hp = s->hp = pct(g->max_hp, SSJ2_HP_PCT);
    s->max_ep = s->ep = pct(g->max_ep, SSJ2_EP_PCT);
    s->str = pct(g->str, SSJ2_STR_PCT);
    s->pow = pct(g->pow, SSJ2_POW_PCT);
    s->end = pct(g->end, SSJ2_END_PCT);
    s->flags = SSJ2_FLAGS;
    static const u8 abilities[4] = SSJ2_ABILITIES;
    for (int i = 0; i < 4; i++) s->ability_list[i] = abilities[i];
}
#endif

// ---- Super Saiyan 2 Burst (ability 23) ------------------------------------------------------------
static const u32 atk_burst[2] = { 0x0400, BURST_POWER };   // melee kind, the heavy knock-back reaction

int ssj2_burst_ready(const CharacterEntry *c) { return ABILITY_GATE() != 0 && (s16)c->ep >= BURST_EP; }
void ssj2_burst_use(CharacterEntry *c, Entity *e) {
    c->ep -= BURST_EP;
    Entity_SetAnimation(e, 5);                       // the ki pose (group 5)
    melee_burst(e, e->x >> 8, (e->y >> 8) - 10, BURST_RADIUS_X, BURST_RADIUS_Y, atk_burst);
    camera_shake_safe(BURST_SHAKE, 80);
    sfx(SFX_BOOM);
    sfx(SFX_KI_HIT);
}

// ---- test triggers -----------------------------------------------------------------------------
#ifdef SSJ2_TEST
ROM_FN(void, PlayerEntity_RefreshSprite, (Entity *e), 0x080086A3);
int p2_bind_slot(int slot);                          // player.c
#endif

// Every input poll (input.c). SSJ2_TEST builds: player 2's pad holds L + R and presses Select -> ssj2_join;
// Start -> Super Saiyan 2 Gohan becomes player 1 (as the save pad's switch does); Up -> player 2 plays him;
// Down -> his EXP one short of his next level (the next kill levels him up).
void ssj2_frame(void) {
#if defined(SSJ2_TEST) && PARTY_SLOTS > SSJ2_SLOT
    const InputSystem *t = &coop.input2;
    if ((t->current & (KEY_L | KEY_R)) != (KEY_L | KEY_R) || !p1_controllable()) return;
    CharacterEntry *s = &g_Party->chars[SSJ2_SLOT];
    if (t->pressed & KEY_SELECT) ssj2_join();
    Entity *p1 = g_PlayerObject;
    if ((t->pressed & KEY_START) && p1 && (s->flags & 1) && g_Party->active_slot != SSJ2_SLOT
        && !(coop.p2 && coop.p2_slot == SSJ2_SLOT)) {
        g_Party->active_slot = SSJ2_SLOT;
        p1->sprite_record = Character_GetSpriteRecord(DISPLAY_DATA[SSJ2_ROW * 0x14 + 1]);
        PlayerEntity_RefreshSprite(p1);
        Entity_SetAnimation(p1, ANIM_IDLE);
    }
    if ((t->pressed & KEY_UP) && coop.p2 && (s->flags & 1) && g_Party->active_slot != SSJ2_SLOT) p2_bind_slot(SSJ2_SLOT);
    if (t->pressed & KEY_DOWN) {
        int lv = Character_GetLevel(s);
        if (lv < 50) s->exp = (u32)Character_LevelExp(lv + 1) - 1;
    }
#endif
}
