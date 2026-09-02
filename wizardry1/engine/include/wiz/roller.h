// Character creation rules -- the "Training Grounds" roller.
//
// Ported from the released Apple Pascal source (SEGMENT PROCEDURE ROLLER,
// P010B01..) which the 1987 DOS build follows closely.  These are the pure
// rules; the interactive menu flow (GETKEY / GOTOXY) belongs to the platform
// layer and is not here.
#pragma once
#include "wiz/character.h"
#include "wiz/rng.h"

namespace wiz {

// Racial base attributes (STR,IQ,PIETY,VIT,AGI,LUCK) -- SETBASE / SETXBASE.
inline void raceBaseAttrs(Race r, int out[ATTR_COUNT]) {
    static const int T[6][ATTR_COUNT] = {
        {0, 0, 0, 0, 0, 0},        // NoRace
        {8, 8, 5, 8, 8, 9},        // Human
        {7, 10, 10, 6, 9, 6},     // Elf
        {10, 7, 10, 10, 5, 6},    // Dwarf
        {7, 7, 10, 8, 10, 7},     // Gnome
        {5, 7, 7, 6, 10, 15},     // Hobbit
    };
    for (int i = 0; i < ATTR_COUNT; ++i) out[i] = T[int(r)][i];
}

// GTCHGLST: which classes the attribute set + alignment qualifies for.
// `elig[8]` indexed by Class.
inline bool classEligibility(const int a[ATTR_COUNT], Align al, bool elig[8]) {
    elig[int(Class::Fighter)] = a[STR] >= 11;
    elig[int(Class::Mage)]    = a[IQ] >= 11;
    elig[int(Class::Priest)]  = a[PIETY] >= 11 && al != Align::Neutral;
    elig[int(Class::Thief)]   = a[AGI] >= 11 && al != Align::Good;
    elig[int(Class::Bishop)]  = a[IQ] >= 12 && a[PIETY] >= 12 && al != Align::Neutral;
    elig[int(Class::Samurai)] = a[STR] >= 15 && a[IQ] >= 11 && a[PIETY] >= 10 &&
                                a[VIT] >= 14 && a[AGI] >= 10 && al != Align::Evil;
    elig[int(Class::Lord)]    = a[STR] >= 15 && a[IQ] >= 12 && a[PIETY] >= 12 &&
                                a[VIT] >= 15 && a[AGI] >= 14 && a[LUCK] >= 15 &&
                                al == Align::Good;
    elig[int(Class::Ninja)]   = a[STR] >= 17 && a[IQ] >= 17 && a[PIETY] >= 17 &&
                                a[VIT] >= 17 && a[AGI] >= 17 && a[LUCK] >= 17 &&
                                al == Align::Evil;
    for (int c = 0; c < 8; ++c) if (elig[c]) return true;
    return false;
}

// PTSMENU: bonus points to distribute.  7 + rand%4, with rare +10 stacks.
inline int rollBonusPoints(Rng &rng) {
    int pts = 7 + rng.mod(4);
    while (pts < 20 && rng.mod(11) == 10) pts += 10;
    return pts;
}

// INITCHAR.  The DOS build sets AGE to a flat 500 weeks (Apple rolled
// 18*52 + rand%300); gold is 100 + rand%100 (Apple used 90).
inline int rollAge(Rng &)     { return 500; }
inline int rollGold(Rng &rng) { return 100 + rng.mod(100); }

// KEEPCHYN: starting HP from class + vitality, with two 10% shrink rolls.
inline int rollHp(Class cls, int vitality, Rng &rng) {
    int base;
    switch (cls) {
        case Class::Fighter: case Class::Lord:                  base = 10; break;
        case Class::Priest:                                     base = 8;  break;
        case Class::Thief: case Class::Bishop: case Class::Ninja: base = 6; break;
        case Class::Mage:                                       base = 4;  break;
        case Class::Samurai:                                    base = 16; break;
        default:                                                base = 8;  break;
    }
    int vmod = 0;
    switch (vitality) {
        case 3: vmod = -2; break;
        case 4: case 5: vmod = -1; break;
        case 16: vmod = 1; break;
        case 17: vmod = 2; break;
        case 18: vmod = 3; break;
    }
    int hp = base + vmod;
    for (int i = 0; i < 2; ++i)
        if (rng.mod(2) == 1) hp = (9 * hp) / 10;
    return hp < 2 ? 2 : hp;
}

// KEEPCHYN: the free first-level spells a fresh caster starts with.
inline void startingSpells(Character &c) {
    if (c.cls == Class::Mage || c.cls == Class::Bishop) {
        c.spellKnown[3] = c.spellKnown[1] = true;   // KATINO, HALITO
        c.mageSpells[1] = 2;
    }
    if (c.cls == Class::Priest) {
        c.spellKnown[23] = c.spellKnown[24] = true;  // DIOS, BADIOS
        c.priestSpells[1] = 2;
    }
}

// UTILITIE proc 25 (P010119) -- recompute the combat tail from level / class /
// strength (unarmed; the engine does not model equipment yet).
inline void deriveStats(Character &c) {
    bool fighty = c.cls == Class::Fighter || c.cls == Class::Priest ||
                  int(c.cls) >= int(Class::Samurai);
    c.hpCalcMd = fighty ? 2 + c.charLevel / 3 : c.charLevel / 5;
    c.hpDamRc[0] = 2; c.hpDamRc[1] = 2; c.hpDamRc[2] = 0;
    int str = c.attrib[STR];
    if (str > 15) { c.hpCalcMd += str - 15; c.hpDamRc[2] = str - 15; }
    else if (str < 6) c.hpCalcMd += str - 6;
    c.healPts = 0;
    c.critHitM = c.cls == Class::Ninja;
    c.swingCnt = 1;
    if (c.cls == Class::Ninja) c.hpDamRc[1] = 4;
    c.armorClass = 10;
    if (c.cls == Class::Fighter || int(c.cls) >= int(Class::Samurai))
        c.swingCnt += c.charLevel / 5 + (c.cls == Class::Ninja ? 1 : 0);
    if (c.swingCnt > 10) c.swingCnt = 10;
    c.wepSlay = 0;
}

} // namespace wiz
