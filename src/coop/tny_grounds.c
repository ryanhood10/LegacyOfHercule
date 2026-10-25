// The tournament grounds (Papaya Island, docs/tournament.md "The grounds"): CoopCall 30 and test command 30.
//
// CoopCall 30, op, b, c:
//   op 0  the waiting room's fighters (its on-load script): up to six of today's pool (tny_in_pool) stand on
//         fixed spots of open floor as talkable NPCs, each with a one-line greeting by name. A giant (TR_GIANT)
//         takes two of the six (OBJ VRAM: 32 KB of tiles, docs/tournament.md section 8). A fighter whose sprite
//         is not in this build is skipped (gr_sprite_ok). Pushes nothing.
//   op 1  pushes 1 when sprite b has a record in this build, else 0 (the plaza's announcer, sprite 230, or his
//         stand-in: record conditions in data/story/papaya.lscript).
//   op 2  pushes 1 when the grounds' maps are in this build (zone 15 area 11 is in the map table; they need
//         roms/buu.gba): the world-map landing's conditions, so a BUU=0 build has no landing into a missing map.
// Test command 30 (coop2.cmd = 30, cmd_arg): 0 the real pool again, 1 force the pool to the Rookie opponents
// (a stand-in while the engine's tny_in_pool is the weak default), 2 spawn the fighters now beside the rest.
#include "tourney.h"
#include "tny_ring.h"

extern const unsigned sprite_index_count;   // build/gen/sprites_data.c (tools/spriteclone.py)

#define GR_MAGIC 0x50415041u                // "PAPA"
#define GR_MAX_SLOTS 6
#define NPC_HANDLER 0x0800B711u            // town NPC record handler (talkable; MapScript create, returns the entity)

typedef struct NpcRecord {                 // map script record of a town NPC (docs/scripts.md 2.3)
    u32 handler;                           // +0x00
    u32 condition;                         // +0x04 (already decided: we create it ourselves)
    s16 x, y;                              // +0x08
    const void *talk;                      // +0x0C dialogue sequence
    s32 sprite;                            // +0x10
    s32 count;                             // +0x14 behaviours; the entity keeps a pointer to +0x14,
    const void *behaviour;                 // +0x18 so this record stays put (RAM below) and these two never change
} NpcRecord;

static struct {
    u32 magic;
    u8 force_rookie;                       // test command 30, 1
    u8 _pad[3];
    NpcRecord rec;                         // the scratch record every spawned fighter is created from
} gr TNY_SECTION("gr");
_Static_assert(sizeof(gr) <= 0x40, ".tny.gr is 0x40 bytes");

static const u32 face_down_wait[2] = { 0x0800CB0Fu, 0x7D00 };   // NpcBehavior_FaceAndWait: face down, ~9 min

// Open floor in the waiting room (400 x 400; data/maps/papaya_waiting.json): clear of the table (160..240,
// 168..224), the chairs, the save pad (24..65, 264..305), the doors and the director (168, 328).
static const s16 spots[GR_MAX_SLOTS][2] = {
    { 80, 120 }, { 296, 104 }, { 344, 184 }, { 296, 264 }, { 80, 216 }, { 224, 128 },
};

// ---- greetings: one dialogue entry each (u16 next = 1, mode 1 text, speaker 0), then the stop word ----------
#define GREET(name, txt) \
    static const char gl_##name[] __attribute__((aligned(4))) = "\x01\x00\x01\x00" txt; \
    static const void *const gs_##name[2] = { gl_##name, 0 };

GREET(yamcha,   "YAMCHA: Yamcha's the name.  Don't blink,\x0b" "you might miss my Wolf Fang Fist!")
GREET(tien,     "TIEN: I am Tien.  Fight me with everything\x0b" "you have.")
GREET(frieza,   "FRIEZA: Lord Frieza, in a tournament for monkeys?\x0b" "How quaint.  I'll play along.")
GREET(mecha,    "MECHA FRIEZA: They rebuilt me better.\x0b" "Frieza's comeback starts in this ring!")
GREET(kingcold, "KING COLD: I am King Cold.  My sons were\x0b" "mere children compared to me.")
GREET(hercule,  "HERCULE: Ha ha ha!  The champ is here!\x0b" "Autographs after the tournament, please!")
GREET(roshi,    "MASTER ROSHI: Ho ho!  Old Roshi still has a few\x0b" "tricks up his sleeve, youngster.")
GREET(tao,      "MERCENARY TAO: Mercenary Tao.  I've killed men\x0b" "for less than a trophy.")
GREET(spopovich,"SPOPOVICH: SPOPOVICH...  CRUSH!")
GREET(yamu,     "YAMU: I'm Yamu.  Your energy looks...\x0b" "delicious.")
GREET(chichi,   "CHI-CHI: Chi-Chi, Ox-King's daughter!\x0b" "Don't you dare go easy on me!")
GREET(videl,    "VIDEL: I'm Videl.  I've been training hard\x0b" "for this.  Let's have a good match!")
GREET(chiaotzu, "CHIAOTZU: I'm Chiaotzu!  Tien says\x0b" "I have to be brave today.")
GREET(a19,      "ANDROID 19: Android 19.  Your energy\x0b" "readings are...  acceptable.")
GREET(gero,     "DR. GERO: I am Gero.  I came to gather data\x0b" "on the world's strongest fighters.")
GREET(a13,      "ANDROID 13: Name's 13.  Ya better not\x0b" "waste my time, punk.")
GREET(a17,      "ANDROID 17: 17.  This beats driving around\x0b" "looking for Goku.")
GREET(a18,      "ANDROID 18: 18.  I'm only here for the prize money.")
GREET(icell,    "CELL: Cell.  So many strong fighters\x0b" "in one room...  delicious.")
GREET(a16,      "ANDROID 16: I am Android 16.  I will not\x0b" "hold back.  Neither should you.")
GREET(scell,    "CELL: Cell, nearly perfect.  Nearly.\x0b" "A few good fights should help.")
GREET(pcell,    "PERFECT CELL: Perfect Cell.  This tournament\x0b" "is a warm-up for my own Games.")
GREET(celljr,   "CELL JR.: Hee hee!  Cell Jr. wants to play!")
GREET(cooler,   "COOLER: Cooler.  I am nothing like\x0b" "my little brother.")
GREET(namek,    "NAMEKIAN WARRIOR: I fight for Lord Cooler.\x0b" "Forgive me in advance.")
GREET(broly,    "BROLY: Broly...  KAKAROT...")
GREET(paragus,  "PARAGUS: Paragus.  Please keep your distance\x0b" "from my son.")
GREET(spcell,   "SUPER PERFECT CELL: Super Perfect Cell.\x0b" "I came back from the brink stronger!")
GREET(super13,  "SUPER 13: Android 13, complete!\x0b" "Time to crush some bugs!")
GREET(bgoku,    "GOKU: Hey, I'm Goku!  Let's both give it\x0b" "everything we've got!")
GREET(mvegeta,  "MAJIN VEGETA: Vegeta, Prince of all Saiyans.\x0b" "Stay out of my way.")
GREET(mgohan,   "GOHAN: I'm Gohan.  Let's have a fair fight!")
GREET(saiyaman, "GREAT SAIYAMAN: Champion of justice,\x0b" "the Great Saiyaman!  ...Too much?")
GREET(bhercule, "HERCULE: The world champion, Hercule!\x0b" "Still the greatest, ha ha ha!")
GREET(bpiccolo, "PICCOLO: Piccolo.  Concentrate.  The ring\x0b" "doesn't forgive mistakes.")
GREET(superbuu, "SUPER BUU: Buu!  Buu fight!  Buu win!")

static const void *const *const greet[TF_COUNT] = {
    [TF_YAMCHA] = gs_yamcha, [TF_TIEN] = gs_tien, [TF_FRIEZA] = gs_frieza, [TF_MECHA_FRIEZA] = gs_mecha,
    [TF_KING_COLD] = gs_kingcold, [TF_HERCULE] = gs_hercule, [TF_ROSHI] = gs_roshi, [TF_TAO] = gs_tao,
    [TF_SPOPOVICH] = gs_spopovich, [TF_YAMU] = gs_yamu, [TF_CHICHI] = gs_chichi, [TF_VIDEL] = gs_videl,
    [TF_CHIAOTZU] = gs_chiaotzu, [TF_A19] = gs_a19, [TF_GERO] = gs_gero, [TF_A13] = gs_a13, [TF_A17] = gs_a17,
    [TF_A18] = gs_a18, [TF_IMPERFECT_CELL] = gs_icell, [TF_A16] = gs_a16, [TF_SEMI_CELL] = gs_scell,
    [TF_PERFECT_CELL] = gs_pcell, [TF_CELL_JR] = gs_celljr, [TF_COOLER] = gs_cooler, [TF_NAMEKIAN] = gs_namek,
    [TF_BROLY] = gs_broly, [TF_PARAGUS] = gs_paragus, [TF_SUPER_PERFECT_CELL] = gs_spcell,
    [TF_SUPER13] = gs_super13, [TF_BGOKU] = gs_bgoku, [TF_MAJIN_VEGETA] = gs_mvegeta,
    [TF_MYSTIC_GOHAN] = gs_mgohan, [TF_SAIYAMAN] = gs_saiyaman, [TF_BHERCULE] = gs_bhercule,
    [TF_BPICCOLO] = gs_bpiccolo, [TF_SUPER_BUU] = gs_superbuu,
};

static void gr_init(void) {
    if (gr.magic == GR_MAGIC) return;
    gr.magic = GR_MAGIC;
    gr.force_rookie = 0;
}

// The sprite has a record in this build. Character_GetSpriteRecord reads sprite_index without a bound, so
// an id past the table (art not merged) is checked here first.
static int gr_sprite_ok(int sprite) {
    return sprite >= 7 && (unsigned)sprite < sprite_index_count && Character_GetSpriteRecord(sprite) != 0;
}

static int gr_in_pool(int id) {
    const TFighter *f = &tny_fighters[id];
    if (!(f->role & TR_OPPONENT)) return 0;
    if (gr.force_rookie) return f->tier == TIER_ROOKIE;
    return tny_in_pool(id);
}

static Entity *gr_spawn_npc(int sprite, int x, int y, const void *talk) {
    NpcRecord *r = &gr.rec;
    r->handler = NPC_HANDLER;
    r->condition = 0;
    r->x = (s16)x;
    r->y = (s16)y;
    r->talk = talk;
    r->sprite = sprite;
    r->count = 1;
    r->behaviour = face_down_wait;
    Entity *e = ((Entity *(*)(const NpcRecord *))NPC_HANDLER)(r);
    if (e) EntityList_Add(g_SceneTable[1], e);
    return e;
}

// Today's fighters in the waiting room: a random draw from the pool, at most six slots (a giant takes two).
static int gr_spawn_fighters(void) {
    u8 ids[TF_COUNT];
    int n = 0;
    for (int id = 0; id < TF_COUNT; id++)
        if (greet[id] && gr_in_pool(id) && gr_sprite_ok(tny_fighters[id].sprite)) ids[n++] = (u8)id;
    for (int i = n - 1; i > 0; i--) {              // shuffle: a different crowd each visit
        int j = Rand_Range(i + 1);
        u8 t = ids[i]; ids[i] = ids[j]; ids[j] = t;
    }
    int slots = 0, placed = 0;
    for (int i = 0; i < n && slots < GR_MAX_SLOTS; i++) {
        const TFighter *f = &tny_fighters[ids[i]];
        int cost = (f->role & TR_GIANT) ? 2 : 1;
        if (slots + cost > GR_MAX_SLOTS) continue;
        if (gr_spawn_npc(f->sprite, spots[placed][0], spots[placed][1], greet[ids[i]])) {
            placed++;
            slots += cost;
        }
    }
    return placed;
}

// The map table has zone 15 area 11 (Map_FindEntry is a fatal error screen for a missing map).
static int gr_maps_built(void) {
    const u8 *row = *(const u8 *const *)0x0800EBD8;            // MapEntry table literal (relocated by --maps)
    int n = ((int (*)(void))0x0800E0ADu)();                    // Map_GetCount
    for (int i = 0; i < n; i++, row += 0x38)
        if (row[0] == 15 && row[1] == TNY_PLAZA_AREA) return 1;
    return 0;
}

int tny_grounds_call(int fn, s32 a, s32 b, s32 c, s32 *push) {
    (void)fn; (void)c;
    gr_init();
    switch (a) {
    case 0:
        gr_spawn_fighters();
        return 0;
    case 1:
        *push = gr_sprite_ok((int)b);
        return 1;
    case 2:
        *push = gr_maps_built();
        return 1;
    }
    return 0;
}

void tny_grounds_test(u32 cmd, u32 arg, u32 arg2) {
    (void)cmd; (void)arg2;
    gr_init();
    if (arg == 0 || arg == 1) gr.force_rookie = (u8)arg;
    else if (arg == 2) gr_spawn_fighters();
}
