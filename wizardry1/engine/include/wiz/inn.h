// Adventurer's Inn rules -- rest, spell refill, and level-up.
//
// Ports ADVNTINN's numeric guts (P010A0F..P010A25) from the Apple source,
// cross-checked against DOS CASTLE procs 20/40/41: the DOS Inn additionally
// ages the character +1 week per healing week (HEALHP: AGE := AGE + 1).
// See docs/town.md.  UI lives in wiz/town_ui.cpp.
#pragma once
#include "wiz/character.h"
#include "wiz/rng.h"
#include "wiz/scenario.h"
#include <string>
#include <vector>

namespace wiz {

using InnLog = std::vector<std::string>;

// ---- spell slots (SETSPELS / MINMAG / MINPRI / SPLPERLV) -------------------

// SPLPERLV: raise each of the 7 spell-group counts toward (level - levelMod),
// stepping down by levMod2 per group; cap 9.  `g` is used [1..7].
inline void splPerLv(int g[8], int charLevel, int levelMod, int levMod2) {
    int cnt = charLevel - levelMod;
    if (cnt <= 0) return;
    for (int i = 1; i <= 7 && cnt > 0; ++i) {
        if (cnt > g[i]) g[i] = cnt;
        cnt -= levMod2;
    }
    for (int i = 1; i <= 7; ++i)
        if (g[i] > 9) g[i] = 9;
}

// MINSPCNT: group count = number of known spells in [lo,hi].
inline void minSpCnt(const Character &c, int g[8], int group, int lo, int hi) {
    int n = 0;
    for (int s = lo; s <= hi; ++s)
        if (c.spellKnown[s]) ++n;
    g[group] = n;
}

inline void minMag(Character &c) {
    minSpCnt(c, c.mageSpells, 1, 1, 4);   minSpCnt(c, c.mageSpells, 2, 5, 6);
    minSpCnt(c, c.mageSpells, 3, 7, 8);   minSpCnt(c, c.mageSpells, 4, 9, 11);
    minSpCnt(c, c.mageSpells, 5, 12, 14); minSpCnt(c, c.mageSpells, 6, 15, 18);
    minSpCnt(c, c.mageSpells, 7, 19, 21);
}
inline void minPri(Character &c) {
    minSpCnt(c, c.priestSpells, 1, 22, 26); minSpCnt(c, c.priestSpells, 2, 27, 30);
    minSpCnt(c, c.priestSpells, 3, 31, 34); minSpCnt(c, c.priestSpells, 4, 35, 38);
    minSpCnt(c, c.priestSpells, 5, 39, 44); minSpCnt(c, c.priestSpells, 6, 45, 48);
    minSpCnt(c, c.priestSpells, 7, 49, 50);
}

// SETSPELS: recompute the per-group spell-slot maxima (== the current casts,
// i.e. resting refills spells).
inline void setSpells(Character &c) {
    minPri(c);
    minMag(c);
    switch (c.cls) {
        case Class::Priest:  splPerLv(c.priestSpells, c.charLevel, 0, 2); break;
        case Class::Mage:    splPerLv(c.mageSpells,   c.charLevel, 0, 2); break;
        case Class::Bishop:  splPerLv(c.priestSpells, c.charLevel, 3, 4);
                             splPerLv(c.mageSpells,   c.charLevel, 0, 4); break;
        case Class::Lord:    splPerLv(c.priestSpells, c.charLevel, 3, 2); break;
        case Class::Samurai: splPerLv(c.mageSpells,   c.charLevel, 3, 3); break;
        default: break;
    }
}

// ---- HP on level-up (MOREHP) ---------------------------------------------

inline int moreHp(const Character &c, Rng &rng) {
    int hp;
    switch (c.cls) {
        case Class::Fighter: case Class::Lord:                    hp = rng.mod(10); break;
        case Class::Priest:  case Class::Samurai:                 hp = rng.mod(8);  break;
        case Class::Thief:   case Class::Bishop: case Class::Ninja: hp = rng.mod(6); break;
        case Class::Mage:                                         hp = rng.mod(4);  break;
        default:                                                  hp = rng.mod(8);  break;
    }
    hp += 1;
    switch (c.attrib[VIT]) {
        case 3:            hp -= 2; break;
        case 4: case 5:    hp -= 1; break;
        case 16:           hp += 1; break;
        case 17:           hp += 2; break;
        case 18:           hp += 3; break;
    }
    return hp < 1 ? 1 : hp;
}

// ---- new spells on level-up (TRYLEARN / TRY2LRN) ------------------------

inline void try2Lrn(Character &c, int lo, int hi, Attr iqPiety, Rng &rng, bool &learned) {
    bool known = false;
    for (int s = lo; s <= hi; ++s) known = known || c.spellKnown[s];
    for (int s = lo; s <= hi; ++s) {
        if (c.spellKnown[s]) continue;
        if (rng.mod(30) < c.attrib[iqPiety] || !known) {
            learned = true;
            known = true;
            c.spellKnown[s] = true;
        }
    }
}

inline void tryLearn(Character &c, Rng &rng, InnLog &log) {
    static const int mg[7][2] = {{1,4},{5,6},{7,8},{9,11},{12,14},{15,18},{19,21}};
    static const int pg[7][2] = {{22,26},{27,30},{31,34},{35,38},{39,44},{45,48},{49,50}};
    bool learned = false;
    for (int g = 1; g <= 7; ++g)
        if (c.mageSpells[g] > 0) try2Lrn(c, mg[g-1][0], mg[g-1][1], IQ, rng, learned);
    for (int g = 1; g <= 7; ++g)
        if (c.priestSpells[g] > 0) try2Lrn(c, pg[g-1][0], pg[g-1][1], PIETY, rng, learned);
    if (learned) log.push_back("YOU LEARNED NEW SPELLS!!!!");
    setSpells(c);
}

// ---- age-driven attribute drift (GAINLOST / OLDAGE) --------------------

inline const char *attrName(Attr a) {
    switch (a) {
        case STR: return "STRENGTH"; case IQ: return "I.Q."; case PIETY: return "PIETY";
        case VIT: return "VITALITY"; case AGI: return "AGILITY"; default: return "LUCK";
    }
}

inline void gainLost(Character &c, Rng &rng, InnLog &log) {
    for (int ai = STR; ai <= LUCK; ++ai) {
        Attr a = Attr(ai);
        if (rng.mod(4) == 0) continue;
        int v = c.attrib[a];
        if (rng.mod(130) < c.age / 52) {                 // getting old: lose
            if (v == 18 && rng.mod(6) != 4) {
                // nothing
            } else {
                v -= 1;
                log.push_back(std::string("YOU LOST ") + attrName(a));
                if (a == VIT && v == 2) {                 // OLDAGE
                    log.push_back("** YOU HAVE DIED OF OLD AGE **");
                    c.status = Status::Lost;
                    c.hpLeft = 0;
                    c.attrib[a] = v;
                    return;
                }
            }
        } else if (v != 18) {                             // still young: gain
            v += 1;
            log.push_back(std::string("YOU GAINED ") + attrName(a));
        }
        c.attrib[a] = v;
    }
}

// ---- level-up (CHNEWLEV / MADELEV) ------------------------------------

inline void madeLevel(Character &c, Rng &rng, InnLog &log) {
    log.push_back("YOU MADE A LEVEL!");
    c.charLevel += 1;
    if (c.charLevel > c.maxLevelAcquired) c.maxLevelAcquired = c.charLevel;
    setSpells(c);
    tryLearn(c, rng, log);
    gainLost(c, rng, log);

    int newMax = 0;
    for (int L = 1; L <= c.charLevel; ++L) newMax += moreHp(c, rng);
    if (c.cls == Class::Samurai) newMax += moreHp(c, rng);
    if (newMax <= c.hpMax) newMax = c.hpMax + 1;
    c.hpMax = newMax;
}

// CHNEWLEV: one level per call, or report the XP shortfall.
inline void checkNewLevel(Character &c, const ExpTable &exp, Rng &rng, InnLog &log) {
    int cls = int(c.cls);
    int64_t thr;
    if (c.charLevel <= 12) {
        thr = exp.threshold(cls, c.charLevel).value();
    } else {
        thr = exp.threshold(cls, 12).value();
        for (int L = 13; L <= c.charLevel; ++L) thr += exp.threshold(cls, 0).value();
    }
    if (thr <= c.exp.v) {
        madeLevel(c, rng, log);
    } else {
        int64_t need = thr - c.exp.v;
        log.push_back("YOU NEED " + std::to_string(need) + " MORE");
        log.push_back("EXPERIENCE POINTS TO MAKE LEVEL");
    }
}

// ---- a stay at the Inn (TAKENAP) --------------------------------------

struct RoomTier { const char *name; int hpPerWeek; int goldPerWeek; };
inline const RoomTier kRooms[5] = {
    {"THE STABLES (FREE!)",           0,   0},
    {"COTS. 10 GP/WEEK.",             1,  10},
    {"ECONOMY ROOMS. 50 GP/WEEK.",    3,  50},
    {"MERCHANT SUITES. 200 GP/WEEK.", 7, 200},
    {"ROYAL SUITES. 500 GP/WEEK.",   10, 500},
};

} // namespace wiz
