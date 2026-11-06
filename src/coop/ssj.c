// Super Saiyan Hercule (docs/hercule.md 3b, docs/engine.md "Super Saiyan transformation").
// Ability 18 is the stock Transformation (ability 12) on Hercule's rows: row 21 links to row 22 (sprite 157),
// row 22 is a transformed row (flag bit 0, link back to 21) with the stock Super Saiyans' behaviour class, so
// the stock rules run for him: ready at 10 EP or more (no cost), the power-up (anim 0x23, herc_record's group
// 9) and its sounds, +10 combat bytes and faster legs while transformed, 1 EP every 20 frames, back to row 21
// at 0 EP or when the ability is used again (anim 0x24, ssj_record's group 21).
//
// The stock code serves only the active party slot in three places, so it is replaced here for every
// transformation (Goku's, Gohan's, ... too), each reading the slot of the entity it runs for:
//   * the transform's end (states 0x23 / 0x25, vtable slot 5, stock 0x0801563D -> 0x08014D58),
//   * the detransform's end (state 0x24, slot 5, stock 0x080157AF -> 0x08014D58),
//   * the transformed class's update (0x08024C58 slot 1, stock 0x08017307: the drain and the revert).
// For player 1 the replacements do exactly what the stock does. The base class's update (0x08024004 slot 1) is
// regen.c's now, and the other forms' classes run on the entity's own slot (form_update below).
#include "coop.h"
#include "krillin.h"

#define ANIM_DETRANSFORM 0x24
#define DRAIN_STEP 0xCCD            // stock: 16.16 accumulator, 1 EP per overflow (every 20 frames)

// The stock Transformation B (ability 12; Goku's, Gohan's, Vegeta's, Trunks'): isReady 0x080134F2 (bit 1 of
// the row's flags: never; bit 0: always, to turn back; else the ability gate, the party meter at 0 and
// EP >= 10), onUse 0x0801353C (bit 0: anim 0x24, else anim 0x23).
ROM_FN(int, Transform_IsReady, (const CharacterEntry *c), 0x080134F3);
ROM_FN(void, Transform_Use, (CharacterEntry *c, Entity *e), 0x0801353D);
ROM_FN(void, PlayerEntity_RefreshSprite, (Entity *e), 0x080086A3);   // the active slot's row: class + init

void p2_refresh_sprite(void);   // player.c: re-read player 2's row (sprite record + behaviour)

// .party.ssj (docs/ids.md: 0x10 bytes): test counters and the slot player 2 played last
typedef struct {
    u32 magic;          // +0x0 SSJ_MAGIC once initialised
    u8 last_p2;         // +0x4 player 2's slot last frame (0xFF: no player 2)
    u8 last_up_slot;    // +0x5 the slot of the last transformation's end
    u16 ups;            // +0x6 transformations completed (either player)
    u16 downs;          // +0x8 turned back by the detransform state (0 EP, or the ability again)
    u16 handoffs;       // +0xA slots turned back because player 2 left them (Party_DetransformOthers' rule)
    u16 drains;         // +0xC EP taken by the drain (either player)
    u16 p2_drains;      //  +0xE ... of them from player 2's slot
} SsjState;
_Static_assert(sizeof(SsjState) == 0x10, ".party.ssj is 0x10 bytes");
#define SSJ_MAGIC 0x53534A48u   // "HJSS"
SsjState ssj_state __attribute__((section(".party.ssj"), used));

static int is_p2_entity(const Entity *e) { return coop.p2 && e == coop.p2; }
static int slot_of_entity(const Entity *e) { return is_p2_entity(e) ? coop.p2_slot : g_Party->active_slot; }
static const u8 *row_of(int slot) { return DISPLAY_DATA + g_Party->chars[slot].display_row * 0x14; }

// a display row's behaviour class (row +0x10: IWRAM cell -> vtable); slot 2 starts the form's sound (the
// Super Saiyans' aura hum, SFX_TABLE + 0x294), slot 3 stops it; the stock calls them with only the cell
static void class_call(const u8 *row, int slot) {
    u32 *cell = *(u32 **)(row + 0x10);
    const u32 *vt = (const u32 *)*cell;
    ((void (*)(u32 *))((u32)vt + vt[slot]))(cell);
}

// The hum: three classes play it (the Super Saiyans', rows 10 and 14's, Super Saiyan 2's). A class's slot 2 starts a
// channel of its own and slot 3 stops the first it finds (0x0801FC9C, 0x0801FC22: 16 channels of 0x40 bytes, +0x10 bit 0
// on, +0x28 the sound). Two players need one hum: two Super Saiyans started two and a turn-back's restart left one
// looping after both had turned back; a slot player 2 left kept his, and a save pad's screen gives back what it found.
#define HUM_SFX      (SFX_TABLE + 0x294)
#define SFX_CHANNELS (g_AudioMixer + 0x5C4)
ROM_FN(void, Sfx_Stop, (void *channel, const void *sfx), 0x0801FC23);

// Test command 46 answers this
int ssj_hum_count(void) {
    int on = 0;
    for (int i = 0; i < 16; i++) {
        const u8 *ch = SFX_CHANNELS + i * 0x40;
        if ((ch[0x10] & 1) && *(const u8 *const *)(ch + 0x28) == HUM_SFX) on++;
    }
    return on;
}

static int hums(const u8 *row) {
    u32 cell = *(u32 *)(row + 0x10);
    return cell == 0x030021ECu || cell == 0x030021E8u || cell == 0x030021E4u;
}

static void hum_settle(void) {
    int want = hums(row_of(g_Party->active_slot)) || (coop.p2 && hums(row_of(coop.p2_slot)));
    int on = ssj_hum_count();
    for (; on > want; on--) Sfx_Stop(SFX_CHANNELS, HUM_SFX);
    if (on < want) Sfx_Play(SFX_CHANNELS, HUM_SFX, 0x40, 0);
}

// 0x08014D58 for the entity's own slot: the old class's stop, the row's link (+4 transformed / +5 back), the
// new sprite and class, idle, the new class's start.
static void apply(Entity *e, int up) {
    int slot = slot_of_entity(e);
    if (slot < 0 || slot >= PARTY_SLOTS) return;
    CharacterEntry *c = &g_Party->chars[slot];
    const u8 *row = row_of(slot);
    class_call(row, 3);
    c->display_row = up ? row[4] : row[5];
    row = row_of(slot);
    if (is_p2_entity(e)) {
        p2_refresh_sprite();                       // record, class init as its own slot, idle
    } else {
        e->sprite_record = Character_GetSpriteRecord(row[1]);
        Entity_SetAnimation(e, ANIM_IDLE);
        PlayerEntity_RefreshSprite(e);
    }
    class_call(row, 2);
    hum_settle();
    if (up) { ssj_state.ups++; ssj_state.last_up_slot = (u8)slot; } else ssj_state.downs++;
    regen_ki_used(is_p2_entity(e));   // regen.c: a form burns ki; the 30 s count from when it ends
}

// transform states 0x23 / 0x25, vtable slot 5 (stock 0x0801563C): the transform's sound, then the form
void ssj_transform_done(void *state, Entity *e) {
    (void)state;
    Sfx_Play(g_AudioMixer + 0x5C4, SFX_TABLE + 0x114, 0x40, 0);
    apply(e, 1);
}

// detransform state 0x24, vtable slot 5 (stock 0x080157AE)
void ssj_detransform_done(void *state, Entity *e) {
    (void)state;
    apply(e, 0);
}

// The death state's enter (vtable 0x080254D8 slot 0, stock 0x0800833E): the state's bookkeeping, no ranged or melee
// resistance, and the form turned back: the stock's is the active slot's whoever fell, so player 2 going down took player
// 1's row back while his body stayed transformed and kept draining, and left his own form on (found by the regression,
// 2026-11-02; docs/perfect-pass.md 9). Now the fallen one's own, as apply() does. His body stays as it lies until he is
// up: p2_sync_combat rebinds him then.
ROM_FN(void, DeathState_Base, (void *state, Entity *e), 0x0800E131);
void ssj_death_enter(void *state, Entity *e) {
    DeathState_Base(state, e);
    *((u8 *)e + 0x163) = 0;
    *((u8 *)e + 0x164) = 0;
    int slot = slot_of_entity(e);
    if (slot < 0 || slot >= PARTY_SLOTS) return;
    const u8 *row = row_of(slot);
    if (!(row[0] & 1)) return;
    class_call(row, 3);
    g_Party->chars[slot].display_row = row[5];
    if (coop.p2) hum_settle();                     // the class's stop took the first hum it found: the other's stays
}

// A body knocked down out of his form in a scene (cine.c, a strike's flag 8): a player on a transformed row turns back as
// the turn-back's end does (apply: 1); an actor showing a transformed row's sprite takes the sprite of the row it turns back
// to (140 -> 137, 61 -> 60, 134 -> 128, 59 -> 56, ...: 2), so the base body falls and lies as the next cut shows him.
// 0: neither (an untransformed player, any other body).
#define DISPLAY_ROW_COUNT 80        // the patched row limit (docs/ids.md "Display rows")
int ssj_turn_back(Entity *e) {
    if (!e) return 0;
    if (e == g_PlayerObject || is_p2_entity(e)) {
        int slot = slot_of_entity(e);
        if (slot < 0 || slot >= PARTY_SLOTS || !(row_of(slot)[0] & 1)) return 0;
        apply(e, 0);
        return 1;
    }
    for (int r = 0; r < DISPLAY_ROW_COUNT; r++) {
        const u8 *row = DISPLAY_DATA + r * 0x14;
        if (!(row[0] & 1) || Character_GetSpriteRecord(row[1]) != e->sprite_record) continue;
        const u8 *base = DISPLAY_DATA + row[5] * 0x14;
        if (base[0] & 1) return 0;
        e->sprite_record = Character_GetSpriteRecord(base[1]);
        *(u32 *)((u8 *)e + 0x20) = 0;              // the cached frame: the next one is read from the new record
        return 2;
    }
    return 0;
}

// the Super Saiyans' class update (vtable 0x08024C58 slot 1, stock 0x08017306): not while turning back; the
// entity's +0x184 accumulator gains 0xCCD a frame and each overflow takes 1 EP from its own slot; at 0 EP
// (alive) it turns back
void ssj_class_update(void *cls, Entity *e) {
    (void)cls;
    if (((int (*)(Entity *, int))vslot(e, 11))(e, ANIM_DETRANSFORM)) return;
    u32 *acc = (u32 *)((u8 *)e + 0x184);
    *acc += DRAIN_STEP;
    if (!(*acc >> 16)) return;
    int slot = slot_of_entity(e);
    if (slot >= 0 && slot < PARTY_SLOTS) {
        CharacterEntry *c = &g_Party->chars[slot];
        if (c->hp == 0) { *acc &= 0xFFFF; return; }   // down: his body is still on the class, his ki stays
        if ((s16)c->ep >= 1) {
            c->ep--;
            ssj_state.drains++;
            if (is_p2_entity(e)) ssj_state.p2_drains++;
        } else {
            c->ep = 0;
        }
        if (c->ep == 0 && c->hp != 0) Entity_SetAnimation(e, ANIM_DETRANSFORM);
    }
    *acc &= 0xFFFF;
}

// The base class's update (vtable 0x08024004 slot 1, stock 0x08017028) and the Super Saiyan 2 class's (rows 4 and 28,
// vtable 0x08024E4C slot 1, stock 0x0801710E: only "the active slot's HP + 1 % every 20 frames", to full, whoever's
// entity it ran for): both are regen.c's rules now. docs/engine.md "Behaviour classes", docs/perfect-pass.md 11.1.
void ssj_base_update(void *cls, Entity *e) {
    (void)cls;
    regen_update(e);
}

// The other two forms' classes read the active slot for every entity, as the Super Saiyans' did (above): rows 10 and
// 14 (vtable 0x0802526C slot 1, stock 0x080171E4: 1 EP every 40 frames, back at 0) and Piccolo's row 7 (vtable
// 0x08024ABC slot 1, stock 0x08017400: 1 % HP for 1 EP every 20 frames, back at 0). Player 2 on one of them fed or
// drained player 1 and never himself. The stock code runs as it is, with player 2's slot presented as the active one.
static void form_update(void *cls, Entity *e, u32 stock) {
    PartyState *p = g_Party;
    u8 active = p->active_slot;
    if (is_p2_entity(e) && coop.p2_slot < PARTY_SLOTS) p->active_slot = coop.p2_slot;
    ((void (*)(void *, Entity *))stock)(cls, e);
    p->active_slot = active;
}
void form_update_drain(void *cls, Entity *e) { form_update(cls, e, 0x080171E5); }
void form_update_heal(void *cls, Entity *e)  { form_update(cls, e, 0x08017401); }

// Every input poll (input.c). Party_DetransformOthers (0x08003ED4, at a load and a save-pad switch) turns back
// every transformed slot but player 1's; in co-op player 2's slot is played too, so the same rule runs when
// player 2 leaves a slot (switches with Select, drops out, or is rebound): that slot turns back unless
// player 1 now plays it.
void ssj_frame(void) {
    if (ssj_state.magic != SSJ_MAGIC) {
        memset(&ssj_state, 0, sizeof ssj_state);
        ssj_state.magic = SSJ_MAGIC;
        ssj_state.last_p2 = 0xFF;
    }
    if (coop.p2 && p1_in_play()) hum_settle();
    // between maps the entity is gone for a while and comes back on the same slot: only a player 2 who
    // left (no controller, or one-player play) counts as gone
    if (!coop.p2 && p2_present() && !coop_solo()) return;
    u8 cur = coop.p2 ? coop.p2_slot : 0xFF;
    u8 old = ssj_state.last_p2;
    if (cur == old) return;
    ssj_state.last_p2 = cur;
    if (old >= PARTY_SLOTS || old == g_Party->active_slot) return;
    CharacterEntry *c = &g_Party->chars[old];
    const u8 *row = row_of(old);
    if (row[0] & 1) {
        c->display_row = row[5];
        ssj_state.handoffs++;
    }
}

// Ability 18 (data/hercule.json "SuperHercule"): the stock Transformation's rules; old Hercule (row 24, the
// future prologue) cannot.
int ssj_ready(CharacterEntry *c) {
    if (c->display_row == OLD_HERCULE_ROW) return 0;
    return Transform_IsReady(c);
}

void ssj_use(CharacterEntry *c, Entity *e) {
    Transform_Use(c, e);
}

// The Super Hercule ability is not in Hercule's starting list (Ryan: nothing reveals Hercule as the hero
// until the Buu phase); the late-story script calls this to put it in his third slot, in place of the
// Megaton Punch (16).
#define HERCULE_SLOT_INDEX 5
#define SSJ_ABILITY 18
// Locked off (Ryan, 2026-11-02, two players: "Hercule's Super Saiyan mode should NOT be an option yet in the game;
// it's MUCH later in development. Fix this so player 2 can actually use Hercule's ki moves again"): CoopCall 4 does
// nothing until SSJ_HERCULE_UNLOCK is 1, and a save that already holds the ability gets Megaton Punch back at the
// load (party_save.c hercule_lock). The transformation itself (ssj_hercule.txt pokes ability 18 in) stays built.
#define SSJ_HERCULE_UNLOCK 0
void hercule_awaken(void) {
#if SSJ_HERCULE_UNLOCK
    CharacterEntry *c = &g_Party->chars[HERCULE_SLOT_INDEX];
    c->ability_list[2] = SSJ_ABILITY;
#endif
}
// A save from the days the ability was granted: Megaton Punch back in slot 2, and his base row if he was transformed.
// A save from before his kit was settled (Ryan, 2026-11-02: the Flash and the Gun, the Ki Blast after the three years, nothing
// else: Ability_Cycle stops only on learned moves, 8 << index, and bit 5 is also the charged A attack): the Taunt's slot gets
// the Gun, the moves are learned as a new game's would be, and his numbers come down to the new growth for his level. The
// flags say whether that has run (a new game's are 0x48).
#define HERCULE_FLASH_BIT 0x08u
#define HERCULE_KI_BIT 0x10u
#define HERCULE_CHARGE_BIT 0x20u
#define THREE_YEARS_FLAG 68       // set by the stock skip, as the two of them learn their moves
#define HERCULE_STR(l) (0x200 + ((l) - 1) * 270)   // data/hercule.json: start + the growth's middle per level
#define HERCULE_POW(l) (0x400 + ((l) - 1) * 310)
#define HERCULE_END(l) (0x180 + ((l) - 1) * 440)
int flag_test_hook(PartyState *p, u32 id);   // party.c
void hercule_lock(PartyState *p) {
    CharacterEntry *c = &p->chars[HERCULE_SLOT_INDEX];
#if !SSJ_HERCULE_UNLOCK
    if (c->ability_list[2] == SSJ_ABILITY) c->ability_list[2] = 16;
    if (c->display_row == 0x16) c->display_row = 0x15;
#endif
    // His endurance came back up (Ryan, 2026-11-02: his punches were the problem, and with 320 a level the androids took 6 hits to
    // put him down): a save that had it cut is raised at the load
    int lv = Character_GetLevel(c);
    if (c->end < HERCULE_END(lv)) c->end = (u16)HERCULE_END(lv);
    if ((c->flags & (HERCULE_FLASH_BIT | HERCULE_CHARGE_BIT)) == HERCULE_FLASH_BIT) return;
    c->flags = (c->flags | HERCULE_FLASH_BIT) & ~(HERCULE_CHARGE_BIT | HERCULE_KI_BIT);
    if (flag_test_hook(p, THREE_YEARS_FLAG)) c->flags |= HERCULE_KI_BIT;
    if (c->ability_list[3] == 17) c->ability_list[3] = HERCULE_GUN_ABILITY;
    c->ability_index = 0;
    int level = Character_GetLevel(c);
    if (c->str > HERCULE_STR(level)) c->str = (u16)HERCULE_STR(level);
    if (c->end > HERCULE_END(level)) c->end = (u16)HERCULE_END(level);
    c->pow = (u16)HERCULE_POW(level);
    c->max_hp = (u16)(c->max_hp * 7 / 5);          // a new game's 70 HP against the old 50
    c->hp = (u16)(c->hp * 7 / 5);
}
