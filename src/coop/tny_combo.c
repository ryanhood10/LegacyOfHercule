// The team combos' pair table (docs/tournament.md 24.10: Ryan's picks, 2026-10-31). Only these pairs have a combo bar;
// most teams have none. The androids' pairs and Spopovich & Yamu share the generic barrage.
#include "tny_combo.h"

static const struct { u8 a, b, combo; } pairs[] = {
    { TF_GOHAN,        TF_KRILLIN,      COMBO_GOHAN_KRILLIN },
    { TF_PICCOLO,      TF_P_HERCULE,    COMBO_PICCOLO_HERCULE },
    { TF_GOKU,         TF_VEGETA,       COMBO_GOKU_VEGETA },
    { TF_GOKU,         TF_PICCOLO,      COMBO_GOKU_PICCOLO },
    { TF_GOKU,         TF_GOHAN,        COMBO_FATHER_SON },
    { TF_P_HERCULE,    TF_SSJ2_GOHAN,   COMBO_FATHER_SON },     // Hercule poses beside Gohan; the beam is Gohan's
    { TF_FRIEZA,       TF_COOLER,       COMBO_FRIEZA_COOLER },
    { TF_BROLY,        TF_PARAGUS,      COMBO_BROLY_PARAGUS },
    { TF_MECHA_FRIEZA, TF_KING_COLD,    COMBO_MECHA_COLD },
    { TF_PERFECT_CELL, TF_CELL_JR,      COMBO_CELL_JR },
    { TF_SPOPOVICH,    TF_YAMU,         COMBO_GENERIC },
};
// any two of these: the generic barrage
static const u8 androids[] = { TF_GERO, TF_A19, TF_A13, TF_A17, TF_A18 };

// Who a fighter is for the pairs: Super Saiyan 2 Gohan pairs as Gohan (but Hercule & SSJ2 Gohan is its own entry,
// matched first); the opponents' Hercule as the party's
static int person(int id) {
    if (id == TF_SSJ2_GOHAN) return TF_GOHAN;
    if (id == TF_HERCULE) return TF_P_HERCULE;
    return id;
}
static int android(int id) {
    for (unsigned i = 0; i < sizeof androids; i++) if (androids[i] == id) return 1;
    return 0;
}

// The table row for a, b: exact ids first, then by person; *swap = 1 when b is the row's first fighter
static int find(int a, int b, int *swap) {
    for (int loose = 0; loose < 2; loose++) {
        int x = loose ? person(a) : a, y = loose ? person(b) : b;
        for (unsigned i = 0; i < sizeof pairs / sizeof pairs[0]; i++) {
            if (pairs[i].a == x && pairs[i].b == y) { *swap = 0; return (int)i; }
            if (pairs[i].a == y && pairs[i].b == x) { *swap = 1; return (int)i; }
        }
    }
    return -1;
}

int tny_combo_of(int a, int b) {
    if (a < 0 || b < 0 || a >= TF_COUNT || b >= TF_COUNT || a == b) return -1;
    int swap, i = find(a, b, &swap);
    if (i >= 0) return pairs[i].combo;
    if (android(a) && android(b)) return COMBO_GENERIC;
    return -1;
}

int tny_combo_lead(int a, int b) {
    int swap;
    return find(a, b, &swap) >= 0 ? swap : 0;
}
