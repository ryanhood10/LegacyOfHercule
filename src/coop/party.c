// The party block, relocated out of IWRAM so it can hold PARTY_SLOTS entries (the stock block at
// 0x03000E90 is followed by other globals). tools/patch_rom.py repoints every ROM literal that held the
// stock address at g_party_block; the static initialiser at 0x08022EE0 then runs Party_Init on it at boot.
#include "coop.h"

PartyState g_party_block __attribute__((section(".party"), aligned(4)));
