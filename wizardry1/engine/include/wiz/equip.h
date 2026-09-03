// EQUIPCHR / ARMORPOW (UTILITIE procs P010119 / P01011E) -- recompute a
// character's combat tail from the items they have equipped.  The interactive
// slot-by-slot picker (DOEQUIP) lives in camp_ui.cpp; this is the pure
// "ARM4CHAR" path that rebuilds the stats from the .equipped flags, used on
// entering the maze and after any inventory change.  See docs/maze.md.
#pragma once
#include "wiz/character.h"
#include "wiz/scenario.h"
#include "wiz/roller.h"          // deriveStats (the base, unarmed stats)

namespace wiz {

// UPLCKSKL: lower one saving-throw skill (lower == better), floor 1.
inline void upLuck(Character &c, int sub, int amt) {
    int x = c.luckSkill[sub] - amt;
    c.luckSkill[sub] = x < 1 ? 1 : x;
}

// EQUIPCHR's LUCKSKIL block: a level/luck base, then class then race mods.
inline void recalcLuck(Character &c) {
    int t = 20 - c.charLevel / 5 - c.attrib[LUCK] / 6;
    if (t < 1) t = 1;
    for (int i = 0; i < 5; ++i) c.luckSkill[i] = t;
    switch (c.cls) {
        case Class::Fighter: upLuck(c, 0, 3); break;
        case Class::Mage:    upLuck(c, 4, 3); break;
        case Class::Priest:  upLuck(c, 1, 3); break;
        case Class::Thief:   upLuck(c, 3, 3); break;
        case Class::Bishop:  upLuck(c, 2, 2); upLuck(c, 4, 2); upLuck(c, 1, 2); break;
        case Class::Samurai: upLuck(c, 0, 2); upLuck(c, 4, 2); break;
        case Class::Lord:    upLuck(c, 0, 2); upLuck(c, 1, 2); break;
        case Class::Ninja:   upLuck(c, 0, 3); upLuck(c, 1, 2); upLuck(c, 2, 4);
                             upLuck(c, 3, 3); upLuck(c, 4, 2); break;
        default: break;
    }
    switch (c.race) {
        case Race::Human:  upLuck(c, 0, 1); break;
        case Race::Elf:    upLuck(c, 2, 2); break;
        case Race::Dwarf:  upLuck(c, 3, 4); break;
        case Race::Gnome:  upLuck(c, 1, 2); break;
        case Race::Hobbit: upLuck(c, 4, 3); break;
        default: break;
    }
}

// ARMORPOW: fold one equipped item `o` (possession slot `slot`) into the
// running stats.  Alignment mismatch curses the item and hurts.
inline void armorPow(Character &c, const ObjectRec &o, int slot) {
    c.poss[slot].cursed = o.cursed();
    bool ok = o.align() == Align::Unalign || o.align() == c.align;
    if (!ok) {
        c.hpCalcMd -= 1;
        c.armorClass += 1;
        c.critHitM = false;
        c.poss[slot].cursed = true;
        return;
    }
    if (o.xtraSwng() > c.swingCnt) c.swingCnt = o.xtraSwng();
    c.armorClass -= o.armorMod();
    c.hpCalcMd  += o.wepHitMd();
    if (o.type() == ObjType::Weapon) {
        int keepAdd = c.hpDamRc[2];               // the STR damage bonus
        int d, f, a; o.wepHpDam(d, f, a);
        c.hpDamRc[0] = d; c.hpDamRc[1] = f; c.hpDamRc[2] = a + keepAdd;
        c.critHitM = c.critHitM || o.critHitM();
        c.wepSlay  = o.wepVstyp();
    }
}

// EQUIPCHR with EQUIPALL = TRUE (== the ARM4CHAR path): rebuild every derived
// stat from the character's current .equipped set.
inline void equipRecalc(Character &c, const Scenario &sc) {
    recalcLuck(c);
    deriveStats(c);                               // base unarmed stats

    // NORMPOW: the best HEALPTS of anything carried (regen while walking).
    for (int i = 0; i < c.possCount; ++i) {
        ObjectRec o{sc.record(Scenario::Object, c.poss[i].itemIndex)};
        if (o.healPts() > c.healPts) c.healPts = o.healPts();
    }

    bool unarmed = true;
    for (int i = 0; i < c.possCount; ++i) {
        if (!c.poss[i].equipped) continue;
        ObjectRec o{sc.record(Scenario::Object, c.poss[i].itemIndex)};
        armorPow(c, o, i);
        unarmed = false;
    }
    if (c.cls == Class::Ninja && unarmed)
        c.armorClass -= c.charLevel / 3 + 2;      // unarmed-ninja AC bonus
}

} // namespace wiz
