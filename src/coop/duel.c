// Player-vs-player duel (docs/story.md: Vegeta vs his clone in space, Vegeta vs Trunks in the Hyperbolic
// Time Chamber). While a duel runs, a player attack (hit message type 1, which players normally
// ignore) that comes from the opponent is handled like an enemy hit (type 0) by the other player's
// message handler: box test, damage calculation (the attacker's own combat bytes), knock-back, OnHit.
// The final blow never kills: the loser is left at 1 HP and the duel ends (no game over), the winner
// is recorded, both players are back to full HP and ki (Ryan, 2026-10-20; vmcall.c then shows "Player N wins
// the duel!" before the duel's end sequence), the pads swap when player 2 won and the duel asked for it, and the duel's end script
// (a mapScripts index, TriggerMapEvent style) runs.
//
// Clone duels: player 2 plays a copy of player 1's character kept in party slot 7 (DUEL_SLOT, rom.h),
// drawn as a shadow (a darkening colour remap on the entity). Hooks: player vtable 0x08025604 slot 9
// (message handler 0x08008945) and slot 16 (player 1's take-damage 0x08008785); player 2's private
// vtable uses the same slot 9 and its own slot 16 (player.c).
#include "coop.h"
#include "revive.h"
#include "tny_engine.h"

#define PLAYER_MSG        0x08008945u   // player vtable slot 9 (entity, message id, message)
#define P1_TAKE_DAMAGE    0x08008785u   // player vtable slot 16 (entity, hit): HP -= hit[+8], death at 0
#define OBJ_PALETTE       ((volatile const u16 *)0x05000200)

void *memcpy(void *dst, const void *src, unsigned n);

int p2_bind_slot(int slot);                          // player.c: bind player 2 to a party slot

static Entity *player1(void) { return coop2.p1_frame ? coop2.p1_frame : g_PlayerObject; }
static Entity *opponent_of(const Entity *e) {
    Entity *p1 = player1();
    if (e == coop.p2) return p1;
    if (e == p1) return coop.p2;
    return 0;
}

// The attacker of a hit message is the attacking entity, or a projectile whose owner (+0x1C) is.
static int from_entity(const u8 *msg, const Entity *who) {
    const Entity *a = *(Entity *const *)(msg + 0x20);
    if (!a || !who) return 0;
    return a == who || *(Entity *const *)((const u8 *)a + 0x1C) == who;
}

int player_msg_hook(Entity *e, int id, u8 *msg) {
    int (*orig)(Entity *, int, u8 *) = (int (*)(Entity *, int, u8 *))PLAYER_MSG;
    if (coop2.duel_active && (id == 0 || id == 1) && msg[0] == 1) {
        Entity *opp = opponent_of(e);
        if (opp && from_entity(msg, opp)) {
            msg[0] = 0;              // an enemy-style hit for this player only
            int r = orig(e, id, msg);
            msg[0] = 1;              // the rest of the broadcast still sees a player attack
            if (r) coop2.duel_hits++;
            return r;
        }
    }
    return orig(e, id, msg);
}

static void duel_end(int loser);

// Player 1's take-damage: in a duel the final blow leaves 1 HP and ends the duel.
int p1_take_damage_hook(Entity *e, u8 *hit) {
    int (*orig)(Entity *, u8 *) = (int (*)(Entity *, u8 *))P1_TAKE_DAMAGE;
    if (coop2.duel_active) {
        CharacterEntry *c = &g_Party->chars[g_Party->active_slot];
        s32 *dmg = (s32 *)(hit + 8);
        if (*dmg >= c->hp) {
            s32 full = *dmg;
            *dmg = c->hp > 1 ? c->hp - 1 : 0;
            int r = *dmg ? orig(e, hit) : 0;
            *dmg = full;
            duel_end(1);
            return r;
        }
    }
    if (tny_ko_check(0, *(s32 *)(hit + 8))) {   // a tournament fight: 1 HP and out (tny_engine.c)
        CharacterEntry *c = &g_Party->chars[g_Party->active_slot];
        s32 *dmg = (s32 *)(hit + 8), full = *dmg;
        *dmg = c->hp > 1 ? c->hp - 1 : 0;
        int r = *dmg ? orig(e, hit) : 0;
        *dmg = full;
        return r;
    }
    if (revive_hit(e, hit, 0)) return 1;   // downed, getting up, or the body (revive.c): no hurt state
    return orig(e, hit);
}

// Player 2's pad went away during a duel (player.c p2_drop_out): player 1 wins, the end script runs.
void duel_p2_left(void) { duel_end(2); }

// Player 2's side (player.c p2_take_damage asks first): returns 1 when the duel took the hit.
int duel_p2_final_blow(Entity *e, int dmg) {
    (void)e;
    if (!coop2.duel_active) return 0;
    CharacterEntry *c = &g_Party->chars[coop.p2_slot];
    if (dmg < c->hp) return 0;
    c->hp = 1;
    duel_end(2);
    return 1;
}

// ---- shadow tint ----------------------------------------------------------------------------------
// A remap toward the palette colour nearest to 45 % brightness with a violet cast: the clone reads as
// Vegeta's shadow but keeps his shading.
static u8 *shadow_table(void) {
    if (coop2.duel_tint) return coop2.duel_tint;
    u8 *t = Arena_Alloc(256);
    if (!t) return 0;
    t[0] = 0;
    for (int i = 1; i < 256; i++) {
        u16 c = OBJ_PALETTE[i];
        int r = (c & 31) * 45 / 100, g = ((c >> 5) & 31) * 40 / 100, b = ((c >> 10) & 31) * 55 / 100 + 2;
        int best = i, bestd = 0x7FFFFFFF;
        for (int j = 1; j < 256; j++) {
            u16 d = OBJ_PALETTE[j];
            int dr = (d & 31) - r, dg = ((d >> 5) & 31) - g, db = ((d >> 10) & 31) - b;
            int dist = dr * dr + dg * dg + db * db;
            if (dist < bestd) { bestd = dist; best = j; }
        }
        t[i] = (u8)best;
    }
    coop2.duel_tint = t;
    return t;
}

static void set_tint(Entity *e, const u8 *table) {
    if (!e) return;
    *(const u8 **)((u8 *)e + 0x24) = table;
    *(u32 *)((u8 *)e + 0x20) = 0;   // drop the cached frame so the next one is decoded through the remap
}

// ---- start / end ----------------------------------------------------------------------------------
// p2_slot < 0 = clone of player 1's character. end_script = mapScripts index to run at the end (-1 none).
void duel_start(int p2_slot, int flags, int end_script) {
    if (!coop.p2 || !g_PlayerObject) return;
    coop2.duel_saved_slot = coop.p2_slot;
    coop2.duel_flags = (u8)flags;
    coop2.duel_end_script = (s16)end_script;
    coop2.duel_winner = 0;
    coop2.duel_hits = 0;
    if (p2_slot < 0) {
        CharacterEntry *src = &g_Party->chars[g_Party->active_slot];
        memcpy(&g_Party->chars[DUEL_SLOT], src, sizeof *src);
        p2_slot = DUEL_SLOT;
        coop2.duel_flags |= DUEL_CLONE;
    }
    p2_bind_slot(p2_slot);
    if (coop2.duel_flags & DUEL_SHADOW) set_tint(coop.p2, shadow_table());
    coop2.duel_active = 1;
}

// both duelists back to full HP and ki: player 1's slot and player 2's (the clone's slot in a clone duel)
static void duel_refill(void) {
    int slots[2] = { g_Party->active_slot, coop.p2 ? coop.p2_slot : -1 };
    for (int k = 0; k < 2; k++) {
        if (slots[k] < 0 || slots[k] >= PARTY_SLOTS) continue;
        CharacterEntry *c = &g_Party->chars[slots[k]];
        c->hp = c->max_hp;
        c->ep = c->max_ep;
    }
}

static void duel_end(int loser) {
    if (!coop2.duel_active) return;
    coop2.duel_active = 0;
    duel_refill();
    coop2.duel_winner = (u8)(3 - loser);
    if (coop2.duel_winner == 2 && (coop2.duel_flags & DUEL_SWAP_ON_P2_WIN)) coop2.pad_swap ^= 1;
    if (coop2.duel_end_script >= 0) Map_SpawnScript(g_MapState, coop2.duel_end_script);
}

// Undo a clone duel's slot and tint (the story's end script decides when: after its cutscene).
void duel_clear(void) {
    if (coop.p2) set_tint(coop.p2, 0);
    if (coop2.duel_flags & DUEL_CLONE) {
        int back = coop2.duel_saved_slot;
        if (coop.p2 && coop.p2_slot == DUEL_SLOT && back != DUEL_SLOT) p2_bind_slot(back);
        u8 *d = (u8 *)&g_Party->chars[DUEL_SLOT];
        for (unsigned i = 0; i < sizeof(CharacterEntry); i++) d[i] = 0;
    }
    if (coop2.duel_tint) { Arena_Free(coop2.duel_tint); coop2.duel_tint = 0; }
    coop2.duel_flags = 0;
}

// Every input poll: the clone's shadow (the hit flash borrows the same remap field and resets it to 0,
// so it is put back), and no energy drain during a duel (Super Saiyan stays up, as Ryan asked).
void duel_tick(void) {
    if ((coop2.duel_flags & DUEL_SHADOW) && coop2.duel_tint && coop.p2 && coop.p2_slot == DUEL_SLOT
        && *(u8 **)((u8 *)coop.p2 + 0x24) == 0)
        set_tint(coop.p2, coop2.duel_tint);
    if (!coop2.duel_active || !(coop2.duel_flags & DUEL_FULL_EP)) return;
    CharacterEntry *a = &g_Party->chars[g_Party->active_slot];
    CharacterEntry *b = &g_Party->chars[coop.p2_slot];
    a->ep = a->max_ep;
    b->ep = b->max_ep;
}
