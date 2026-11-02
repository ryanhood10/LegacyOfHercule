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
// For player 1 the replacements do exactly what the stock does. The base class's update (0x08024004 slot 1)
// is rewritten too, so that a Super Saiyan partner gets no regeneration from the other player's class.
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
static int transformed_class(const u8 *row) { return *(u32 *)(row + 0x10) == 0x030021ECu; }

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
    // one sound channel: a player turning back silenced the other's hum too
    if (!up && coop.p2) {
        int other = is_p2_entity(e) ? g_Party->active_slot : coop.p2_slot;
        if (other < PARTY_SLOTS && transformed_class(row_of(other))) class_call(row_of(other), 2);
    }
    if (up) { ssj_state.ups++; ssj_state.last_up_slot = (u8)slot; } else ssj_state.downs++;
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

// The base class's update (vtable 0x08024004 slot 1, stock 0x08017028), rewritten with one rule added: a slot
// on a transformed row gets no regeneration. Stock, per frame: the party meter -1 (to 0); the entity's +0x184
// accumulator gains 0x6E and each overflow (about every 596 frames) gives the active slot EP + max_ep * 0x7AE
// >> 16 (3 %) and every other stock slot (0-5) EP and HP + max * 0xCCD >> 16 (5 %), capped at the maximum.
// Alone, no transformed slot is ever regenerated (the active one runs the Super Saiyan class, the others are
// turned back); in co-op each player's base class regenerates the other player's slot, which would have
// fed a Super Saiyan partner.
static void regen(CharacterEntry *c, int ep_rate, int hp_rate) {
    if (DISPLAY_DATA[c->display_row * 0x14] & 1) return;
    s32 ep = (s16)c->ep + (((s16)c->max_ep * ep_rate) >> 16);
    c->ep = (u16)(ep > (s16)c->max_ep ? (s16)c->max_ep : ep);
    if (!hp_rate) return;
    s32 hp = (s16)c->hp + (((s16)c->max_hp * hp_rate) >> 16);
    c->hp = (u16)(hp > (s16)c->max_hp ? (s16)c->max_hp : hp);
}
void ssj_base_update(void *cls, Entity *e) {
    (void)cls;
    PartyState *p = g_Party;
    if (p->meter) p->meter--;
    u32 *acc = (u32 *)((u8 *)e + 0x184);
    *acc += 0x6E;
    if (*acc >> 16) {
        regen(&p->chars[p->active_slot], 0x7AE, 0);
        for (int i = 0; i < PARTY_SLOTS_STOCK; i++)
            if (i != p->active_slot) regen(&p->chars[i], 0xCCD, 0xCCD);
    }
    *acc &= 0xFFFF;
    // resting heals the entity's own slot to 2/3 (regen.c; the companion is nobody's slot, companion.c)
    if (coop.p2 && e == coop.p2) { if (!companion_active()) regen_tick(e, &p->chars[coop.p2_slot], 1); }
    else regen_tick(e, &p->chars[p->active_slot], 0);
}

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
void hercule_lock(CharacterEntry *c) {
#if !SSJ_HERCULE_UNLOCK
    if (c->ability_list[2] == SSJ_ABILITY) c->ability_list[2] = 16;
    if (c->display_row == 0x16) c->display_row = 0x15;
#else
    (void)c;
#endif
}
