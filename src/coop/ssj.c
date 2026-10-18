// Super Saiyan Hercule: the ability pair the ability table points at. Switches the character's party
// slot to the new display row (generated at build time: sprite 157, row 22) through the game's own
// transformation handler, and refreshes player 2's entity when the slot is player 2's.
#include "coop.h"

#define SSJ_SPRITE 157
#define SSJ_ROW 22
#define SSJ_EP_COST 50

// BytecodeVM_SetCharacterTransformation (script opcode 73): stack = {count, slot, spriteId}; finds the
// first display row with that sprite, writes the slot's row and refreshes the active player's entity.
ROM_FN(void, VM_SetCharacterTransformation, (s32 *stack), 0x0800A8ED);

void p2_refresh_sprite(void);   // player.c: re-read player 2's row (sprite record + behaviour)

static int slot_of(const CharacterEntry *c) {
    return (int)(c - g_Party->chars);
}

int ssj_ready(CharacterEntry *c) {
    return c->ep >= SSJ_EP_COST && c->display_row != SSJ_ROW;
}

void ssj_use(CharacterEntry *c, Entity *e) {
    (void)e;
    int slot = slot_of(c);
    if (slot < 0 || slot >= 6) return;
    c->ep = (u16)(c->ep - SSJ_EP_COST);
    s32 stack[3] = { 2, slot, SSJ_SPRITE };
    VM_SetCharacterTransformation(stack);
    if (coop.p2 && slot == coop.p2_slot) p2_refresh_sprite();
}
