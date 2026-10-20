// The Cell Games (docs/story.md 4h, data/story/cell_games.*): solo fights.
//
// Fights 1-2 and the Cell Junior fights are fought by one player while the other watches. CoopCall 15
// (a) sets the mode: 0 both players, 1 player 1 fights alone, 2 player 2 fights alone. While a mode is
// set, player 2's entity is removed and not respawned, the co-op twins do not spawn, and in mode 2
// player 2's pad drives player 1's entity (the fighter the script made player 1) so a loss is the
// normal game over. The mode only holds in the arena (zone 14 areas 2-4): anywhere else it reads 0
// and is cleared, so a game over or a reload never leaves a player hidden.
#include "coop.h"

#define CG_MAGIC 0xCE11CA3Eu
#define CG_ZONE  14

typedef struct CgState {
    u32 magic;      // partyram is not cleared at boot: the mode only counts with the magic set
    u8 solo;        // 0 both, 1 player 1 alone, 2 player 2 alone (on player 1's entity)
    u8 _pad[3];     // tests: [0] stat to win + 1, [1] frames ready, [2] who fought the last win
    Entity *owner[2];   // CoopCall 16: the entities holding a beam and their beam objects
    void *beam[2];
} CgState;

CgState cg_state __attribute__((section(".party.cg"), used));

static int in_arena(void) {
    const u8 *m = (const u8 *)g_MapState;   // map state: +0 zone, +1 area
    return m && m[0] == CG_ZONE && m[1] >= 2 && m[1] <= 4;
}

int coop_solo(void) {
    if (cg_state.magic != CG_MAGIC || !cg_state.solo) return 0;
    if (!in_arena()) { cg_state.solo = 0; cg_state.owner[0] = cg_state.owner[1] = 0; return 0; }
    return cg_state.solo;
}

// COOP_CMD_WIN_STAT (chapter tests): the stat whose fight wins itself once an enemy of it stands and player 1
// is free to move (the fight has started, not a scene that only shows it); cg_state's spare bytes hold it
u8 *cg_test_slot(void) {
    if (cg_state.magic != CG_MAGIC) { cg_state.magic = CG_MAGIC; cg_state.solo = 0; cg_state._pad[0] = cg_state._pad[1] = cg_state._pad[2] = 0;
                                     cg_state.owner[0] = cg_state.owner[1] = 0; cg_state.beam[0] = cg_state.beam[1] = 0; }
    return cg_state._pad;
}

// CoopCall 16 Beam (sprite, on): a Kamehameha held by the entity showing that sprite, driven from C the way the
// beam command of script step 137 (NPCFireKamehameha, 0x0801D788 / 0x0801D7DC) drives it, but without the
// command chain: step 137 and batch commits wait for the entity's commands, so a beam held through the dialogue
// would stall the scene. on 0 ends that entity's beam. The beam struggle gives Hercule and the second Cell
// their beams this way (the stock clash beam between Gohan and Cell is a decoration of map 326).
ROM_FN(Entity *, Vm_FindCharEntity, (int id), 0x08009371);   // 0 = player 1, 1-6 party slots, else a sprite id
ROM_FN(void, Entity_GetBox, (s32 *out, Entity *e), 0x080079B9);
ROM_FN(void, Entity_BeamPoint, (s32 *out, Entity *e), 0x08007A27);
ROM_FN(void *, Beam_Init, (void *mem, s32 *pos, int a, Entity *owner), 0x0801D84D);
ROM_FN(void, Beam_Fire, (void *beam, int facing, s32 *pos), 0x08014479);
ROM_FN(void, Beam_End, (void *beam), 0x0801451D);
#define BEAM_OFFSETS ((const s32 *)0x08701DF0)   // per facing {dx, dy}: down, up, left (-9, 2), right (10, 0)

void cg_beam(int id, int on) {
    Entity *e = Vm_FindCharEntity(id);
    if (!e) return;
    int k;
    for (k = 0; k < 2 && cg_state.owner[k] != e; k++) {}
    if (!on) {
        if (k < 2) { Beam_End(cg_state.beam[k]); cg_state.owner[k] = 0; cg_state.beam[k] = 0; Entity_SetAnimation(e, 0); }
        return;
    }
    if (k < 2) return;                       // already holding one
    for (k = 0; k < 2 && cg_state.owner[k]; k++) {}
    if (k == 2) return;
    s32 box[4], pos[2];
    Entity_GetBox(box, e);
    pos[0] = (box[0] + box[2]) >> 1;
    pos[1] = (box[1] + box[3]) >> 1;
    void *beam = Beam_Init(0, pos, 0, e);
    if (!beam) return;
    EntityList_Add(g_EntityList, (Entity *)beam);
    Entity_SetAnimation(e, 30);              // the beam pose (group 18)
    int f = e->facing & 3;
    Entity_BeamPoint(pos, e);
    pos[0] += BEAM_OFFSETS[f * 2];
    pos[1] += BEAM_OFFSETS[f * 2 + 1];
    Beam_Fire(beam, f, pos);
    cg_state.magic = CG_MAGIC;
    cg_state.owner[k] = e;
    cg_state.beam[k] = beam;
}

void cg_set_solo(int mode) {
    if (mode < 0 || mode > 2) mode = 0;
    cg_state.magic = CG_MAGIC;
    cg_state.solo = (u8)mode;
    if (mode && coop.p2) EntityList_Remove(g_SceneTable[1], coop.p2, 0);   // its destroy clears coop.p2
}
