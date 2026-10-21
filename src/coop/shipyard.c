// The Capsule Corp backyard with Paragus' ship parked in it (Ryan, 2026-10-21: the ship much bigger, with a
// door). The ship is map art, not sprites (256 x 160 px would take all of OBJ VRAM): data/maps/yard_ship.json
// builds zone 15 area 10, the backyard (zone 6 area 21) with the ship painted in. Every variation lookup goes
// through FUN_0800EB4C (entry, v) -> entry->variations[v - 1] (Map_LoadInternal, the exits, the debug Map Test,
// the new-game map); while the ship is parked it hands out that map's variation for the backyard instead, so
// the backyard keeps its own MapEntry (records, triggers, on-load: data/story/broly.json).
#include "coop.h"

#define SHIP_NEWS   400     // Bulma's news: the ship is in the backyard
#define SHIP_TRIP   401     // the trip under way (the ship is away) ...
#define SHIP_HOME   402     // ... until Broly falls and it flies everyone home
#define YARD_ZONE 6
#define YARD_AREA 21
#define SHIP_ZONE 15
#define SHIP_AREA 10

ROM_FN(int, Map_GetCount, (void), 0x0800E0AD);
#define MAP_TABLE (*(const u8 *const *)0x0800EBD8)   // Map_FindEntry's literal (repointed by patch_rom --maps)
#define MAP_ENTRY_SIZE 0x38

static int flag(int n) { return (g_Party->event_flags[n >> 3] >> (n & 7)) & 1; }

static int ship_parked(void) {
    return flag(SHIP_NEWS) && (!flag(SHIP_TRIP) || flag(SHIP_HOME));
}

static const void *const *variations(const u8 *entry) { return *(const void *const *const *)(entry + 0x2C); }

const void *map_variation_hook(const u8 *entry, int v) {
    if (entry[0] == YARD_ZONE && entry[1] == YARD_AREA && ship_parked()) {
        const u8 *e = MAP_TABLE;
        for (int n = Map_GetCount(); n > 0; n--, e += MAP_ENTRY_SIZE)
            if (e[0] == SHIP_ZONE && e[1] == SHIP_AREA)   // not built (MAPS=0): the stock backyard
                return variations(e)[0];
    }
    return variations(entry)[v - 1];
}
