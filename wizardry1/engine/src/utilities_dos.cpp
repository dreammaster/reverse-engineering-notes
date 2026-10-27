// SEGMENT UTILITIE (the camp-side half) -- see utilities_dos.h.  Every routine names the p-code proc it
// ports; `tools/pcode_decomp.py UTILITIE <n>` prints the original.
#include "wiz/utilities_dos.h"

#include <algorithm>

namespace wiz {

namespace {
u16 rawWord(const Character &c, int word) { return u16(c.raw[word * 2] | (c.raw[word * 2 + 1] << 8)); }
bool bitAt(u16 v, int i) { return (v >> i) & 1; }
}

// the words 95..99 of the record (the resists and the slay mask) are not modelled field by field
void Utilities::setRaw(Character &c, int word, u16 v) {
    c.raw[word * 2] = u8(v & 0xFF);
    c.raw[word * 2 + 1] = u8(v >> 8);
    if (word == 98) c.wepSlay = v;
}

// UTILITIE:27 -- one saving throw is `amount` better (it never goes below 1)
void Utilities::saveTier(int member, int idx, int amount) {
    Character &c = w_.party.member(member);
    int v = c.luckSkill[idx] - amount;
    if (v < 1) v = 1;
    c.luckSkill[idx] = v;
}

// UTILITIE:28 -- start from nothing
void Utilities::clearResists(Character &c) {
    for (int k = 0; k <= 13; ++k) {
        setRaw(c, 95, u16(rawWord(c, 95) & ~(1 << k)));
        setRaw(c, 96, u16(rawWord(c, 96) & ~(1 << k)));
        setRaw(c, 99, u16(rawWord(c, 99) & ~(1 << k)));
    }
    for (int k = 0; k <= 6; ++k) {
        setRaw(c, 97, u16(rawWord(c, 97) & ~(1 << k)));
        setRaw(c, 98, u16(rawWord(c, 98) & ~(1 << k)));
    }
}

// UTILITIE:22 -- what the pack offers: which kinds of item the character could wear, the best heal rate
// and the resists the items carry
void Utilities::scanItems(int member, bool avail[8]) {
    Character &c = w_.party.member(member);
    for (int k = 0; k < 8; ++k) avail[k] = false;
    for (int s = 1; s <= c.possCount; ++s) {
        ObjectRec o{w_.sc.record(Scenario::Object, c.poss[s - 1].itemIndex)};
        if (o.classUse(int(c.cls))) avail[int(o.type()) & 7] = true;
        if (c.healPts < o.healPts()) c.healPts = o.healPts();
        for (int k = 0; k <= 13; ++k)
            if (bitAt(o.wepVsty2(), k)) setRaw(c, 95, u16(rawWord(c, 95) | (1 << k)));
        for (int k = 0; k <= 6; ++k)
            if (bitAt(o.wepVsty3(), k)) setRaw(c, 97, u16(rawWord(c, 97) | (1 << k)));
    }
}

// UTILITIE:23 -- the item in slot `slot` is worn: it shows whether it is cursed and changes the scores
void Utilities::wear(int member, int slot, int itemIdx) {
    Character &c = w_.party.member(member);
    ObjectRec o{w_.sc.record(Scenario::Object, itemIdx)};
    c.poss[slot - 1].cursed = o.cursed();
    if (int(o.align()) == 0 || int(o.align()) == int(c.align)) {
        if (o.xtraSwng() > c.swingCnt) c.swingCnt = o.xtraSwng();
        c.armorClass -= o.armorMod();
        c.hpCalcMd += o.wepHitMd();
        if (int(o.type()) == 0) {
            int keep = c.hpDamRc[2];
            int dice, fac, add;
            o.wepHpDam(dice, fac, add);
            c.hpDamRc[0] = dice; c.hpDamRc[1] = fac; c.hpDamRc[2] = add + keep;
            c.critHitM = c.critHitM || o.critHitM();
            setRaw(c, 99, o.wepVstyp());
        }
    } else {                                                           // the wrong alignment: a penalty and the curse
        --c.hpCalcMd;
        ++c.armorClass;
        c.critHitM = false;
        c.poss[slot - 1].cursed = true;
    }
}

// UTILITIE:24 -- every item already worn takes effect
void Utilities::wearAll(int member) {
    Character &c = w_.party.member(member);
    for (int s = 1; s <= c.possCount; ++s)
        if (c.poss[s - 1].equipped) wear(member, s, c.poss[s - 1].itemIndex);
}

// UTILITIE:25 / 26 -- "SELECT WEAPON" etc.: pick what to wear of one kind
void Utilities::chooseOne(int member, int type, const bool avail[8]) {
    if (!avail[type & 7]) return;
    Console &con = d_.con;
    Character &c = w_.party.member(member);
    Win *box = con.open(8, 8, 24, 15, 16, true);
    static const int nameNo[7] = {259, 260, 261, 262, 263, 259, 264};
    d_.putCentered(box, Dos::substitute(d_.str(258), d_.str(nameNo[type])));
    d_.newline(box);
    int list[8];
    int n = 0;
    for (int s = 1; s <= c.possCount; ++s) {
        int item = c.poss[s - 1].itemIndex;
        g_.g9 = item;
        if (item <= 0) continue;
        ObjectRec o{w_.sc.record(Scenario::Object, item)};
        if (int(o.type()) == type && o.classUse(int(c.cls))) {
            list[n++] = s;
            d_.putNum(box, n, 1);
            d_.putC(box, ')');
            if (c.poss[s - 1].cursed) d_.putC(box, '-');
            else if (c.poss[s - 1].identified) d_.putC(box, ' ');
            else d_.putC(box, '?');
            d_.putStr(box, 14000 + 2 * item + (c.poss[s - 1].identified ? 1 : 0));
            d_.newline(box);
        }
    }
    auto flagged = [&]() {                                             // the last listed item whose flag word +1 is set
        int pick = 0;
        for (int k = 1; k <= n; ++k) if (c.poss[list[k - 1] - 1].cursed) pick = k;
        return pick;
    };
    if (flagged() == 0) {                                              // UTILITIE:26
        d_.gotoXY(box, 0, box->cy + 1);
        d_.putStrC(box, 256);
        d_.putStrC(box, 257);
        int k = d_.getNum(n);
        if (k > 0 && !d_.quit) {
            c.poss[list[k - 1] - 1].equipped = true;
            wear(member, list[k - 1], c.poss[list[k - 1] - 1].itemIndex);
        }
    }
    int pick = flagged();
    if (pick > 0 && !d_.quit) {                                        // cursed: it cannot be taken off
        con.clear(box, true);
        d_.gotoXY(box, 0, 4);
        d_.putStrC(box, 265);
        d_.sound(0); d_.sound(0); d_.sound(0);
        d_.pause(20);
        c.poss[list[pick - 1] - 1].equipped = true;
        wear(member, list[pick - 1], c.poss[list[pick - 1] - 1].itemIndex);
    }
    con.close(box, false);
}

// UTILITIE:17 -- "WILL YOU INVOKE THE SPECIAL POWER OF YOUR ...?"
void Utilities::power(int member, int slot, int item, const ObjectRec &o) {
    Console &con = d_.con;
    Character &c = w_.party.member(member);
    Win *box = con.open(4, 12, 32, 8, 18, true);
    d_.putStrC(box, 251);
    std::string name = d_.str(14000 + 2 * item + (c.poss[slot - 1].identified ? 1 : 0));
    d_.putCentered(box, Dos::substitute(d_.str(252), name));
    d_.newline(box);
    d_.putStrC(box, 253);
    std::string yes = d_.str(254), no = d_.str(255);
    do d_.getKey(); while (!d_.quit && d_.inchar != u8(yes[0]) && d_.inchar != u8(no[0]));
    if (d_.quit) return;
    if (d_.inchar == u8(yes[0])) {
        if (w_.rng.mod(100) < o.chgChance()) c.poss[slot - 1].itemIndex = o.changeTo();
        int sp = o.special();
        auto attribute = [&](int which, int delta) {                   // UTILITIE:18
            int v = c.attrib[which - 1] + delta;
            if (v > 2 && v < 19) c.attrib[which - 1] = v;
        };
        auto fillSpellPoints = [&]() {                                 // UTILITIE:19
            for (g_.g9 = 1; g_.g9 <= 7; ++g_.g9) { c.mageSpells[g_.g9] = 9; c.priestSpells[g_.g9] = 9; }
        };
        if (sp < 7) attribute(sp, 1);
        else if (sp < 13) attribute(sp - 6, -1);
        else {
            switch (sp) {
                case 13: if (c.age > 1040) c.age -= 52; break;
                case 14: c.age += 52; break;
                case 15: c.cls = Class(5); break;
                case 16: c.cls = Class(6); break;
                case 17: c.cls = Class(7); break;
                case 18: c.gold.v += 50000; break;
                case 19: c.exp.v += 50000; break;
                case 20: c.status = Status(7); break;
                case 21: c.status = Status::OK; c.hpLeft = c.hpMax; c.poison = 0; break;
                case 22: ++c.hpMax; break;
                case 23: for (int i = 0; i < w_.party.count(); ++i) w_.party.member(i).hpLeft = w_.party.member(i).hpMax; break;
                case 24: c.cls = Class(5 + w_.rng.mod(3)); break;
                case 25: c.cls = Class(w_.rng.mod(5)); c.status = Status(5); c.hpLeft = 0; break;
                case 26: fillSpellPoints(); break;
                case 27:                                               // UTILITIE:20
                    fillSpellPoints();
                    for (int k = 1; k <= 50; ++k) c.spellKnown[k] = false;
                    break;
                default: break;
            }
        }
    }
    con.close(box, true);
}

// UTILITIE:16
void Utilities::specials(int member) {
    Character &c = w_.party.member(member);
    for (int s = 1; s <= c.possCount; ++s) {
        int item = c.poss[s - 1].itemIndex;
        g_.g9 = item;
        if (item <= 0) continue;
        ObjectRec o{w_.sc.record(Scenario::Object, item)};
        if (o.special() > 0 && o.classUse(int(c.cls))) power(member, s, item, o);
    }
}

// UTILITIE:15
void Utilities::derive(int member, bool silent) {
    Character &c = w_.party.member(member);
    int base = 20 - c.charLevel / 5 - c.attrib[5] / 6;
    if (base < 1) base = 1;
    for (int k = 0; k <= 4; ++k) c.luckSkill[k] = base;
    switch (int(c.cls)) {
        case 0: saveTier(member, 0, 3); break;
        case 1: saveTier(member, 4, 3); break;
        case 2: saveTier(member, 1, 3); break;
        case 3: saveTier(member, 3, 3); break;
        case 4: saveTier(member, 2, 2); saveTier(member, 4, 2); saveTier(member, 1, 2); break;
        case 5: saveTier(member, 0, 2); saveTier(member, 4, 2); break;
        case 6: saveTier(member, 0, 2); saveTier(member, 1, 2); break;
        case 7: saveTier(member, 0, 3); saveTier(member, 1, 2); saveTier(member, 2, 4); saveTier(member, 3, 3); saveTier(member, 4, 2); break;
        default: break;
    }
    switch (int(c.race)) {
        case 1: saveTier(member, 0, 1); break;
        case 2: saveTier(member, 2, 2); break;
        case 3: saveTier(member, 3, 4); break;
        case 4: saveTier(member, 1, 2); break;
        case 5: saveTier(member, 4, 3); break;
        default: break;
    }
    if (!silent) for (int s = 0; s < 8; ++s) c.poss[s].equipped = false;
    int cls = int(c.cls);
    if (cls == 2 || cls == 0 || cls >= 5) c.hpCalcMd = 2 + c.charLevel / 3;
    else c.hpCalcMd = c.charLevel / 5;
    c.hpDamRc[0] = 2; c.hpDamRc[1] = 2; c.hpDamRc[2] = 0;
    if (c.attrib[0] > 15) {
        c.hpCalcMd = c.hpCalcMd + c.attrib[0] - 15;
        c.hpDamRc[2] = c.attrib[0] - 15;
    } else if (c.attrib[0] < 6) {
        c.hpCalcMd = c.hpCalcMd + c.attrib[0] - 6;
    }
    c.healPts = 0;
    c.critHitM = cls == 7;
    c.swingCnt = 1;
    if (cls == 7) c.hpDamRc[1] = 4;
    c.armorClass = 10;
    if (cls == 0 || cls >= 5) c.swingCnt = c.swingCnt + c.charLevel / 5 + (cls == 7 ? 1 : 0);
    if (c.swingCnt > 10) c.swingCnt = 10;
    clearResists(c);
    bool avail[8];
    scanItems(member, avail);
    if (!silent) {
        for (int type = 0; type <= 4; ++type) chooseOne(member, type, avail);
        chooseOne(member, 6, avail);
        specials(member);
    } else {
        wearAll(member);
    }
    if (cls == 7) c.armorClass = c.armorClass - c.charLevel / 3 - 2;
}

// UTILITIE:31 -- E)QUIP from the camp menu: each member in turn
void Utilities::equipEveryone() {
    Console &con = d_.con;
    Win *w = con.open(0, 4, 40, 3, 7, true);
    for (int i = 0; i < w_.party.count(); ++i) {
        const Character &c = w_.party.member(i);
        con.clear(w, true);
        d_.putS(w, c.name);
        while (w->cx <= 15) d_.putC(w, ' ');
        d_.putC(w, 'L');
        d_.putNum(w, c.charLevel, 3);
        d_.putC(w, ' ');
        d_.putS(w, d_.str(10009 + int(c.align)).substr(0, 1));
        d_.putC(w, '-');
        d_.putS(w, d_.str(10030 + int(c.cls)).substr(0, 3));
        d_.putC(w, ' ');
        d_.putStr(w, 9999 + int(c.race));
        derive(i, false);                                              // UTILITIE:30
        xgoto_ = 18;
        g_.g9 = i;
        if (d_.quit) break;
    }
    xgoto_ = 12;
    con.close(w, true);
}

// UTILITIE:32 -- R)EORDER
void Utilities::reorder() {
    xgoto_ = 12;
    int n = w_.party.count();
    if (n < 2) return;
    Console &con = d_.con;
    Win *w = con.open(8, 4, 24, 10, 15, true);
    d_.putStrC(w, 266);
    int order[6];
    for (int i = 0; i < n; ++i) {
        order[i] = 99;
        d_.gotoXY(w, 0, i + 2);
        d_.putNum(w, i + 1, 1);
        d_.putC(w, ')');
    }
    for (int i = 0; i <= n - 2; ++i) {
        int pick;
        for (;;) {
            d_.gotoXY(w, 1, 2 + i);
            d_.putS(w, "   ");
            d_.gotoXY(w, 1, 2 + i);
            d_.putS(w, ")>");
            do pick = d_.getNum(n) - 1; while (pick < 0 && !d_.quit);
            if (d_.quit) return;
            if (order[pick] == 99) { order[pick] = i; break; }
        }
        d_.gotoXY(w, 1, 2 + i);
        d_.putS(w, ") ");
        d_.putS(w, w_.party.member(pick).name);
    }
    for (int i = 0; i <= n - 2; ++i)
        for (int k = i + 1; k <= n - 1; ++k)
            if (order[k] < order[i]) {
                w_.party.swapMembers(i, k);
                std::swap(order[i], order[k]);
            }
    d_.gotoXY(w, 1, n + 1);
    d_.putS(w, ") ");
    d_.putS(w, w_.party.member(n - 1).name);
    con.close(w, false);
}

// UTILITIE:3..8 -- MALOR: the party arrives somewhere
void Utilities::teleportApply(int east, int north, int up, Win *menu) {
    Console &con = d_.con;
    con.close(menu, false);
    con.close(w_.campSheet, false);
    con.close(w_.campTitle, false);
    con.clear(back_, true);
    xgoto_ = 9;
    int last = w_.sc.count(Scenario::Maze);
    auto leave = [&](bool refresh) { con.close(back_, refresh); throw EndAll{}; };
    if (g_.level - up == last) {                                       // UTILITIE:8: it cannot go any deeper
        d_.putStrC(back_, 206);
        leave(false);
    }
    g_.x += east;
    g_.y += north;
    g_.level -= up;
    if ((g_.x < 0 || g_.x > 19 || g_.y < 0 || g_.y > 19 || g_.level > last) && g_.level > 0) {
        d_.putStrC(back_, 200);                                        // UTILITIE:4: in solid rock
        for (int i = 0; i < w_.party.count(); ++i) {
            w_.party.member(i).inMaze = false;
            w_.party.member(i).status = Status(7);
        }
        xgoto_ = 16;
        leave(false);
    } else if (g_.level < 0) {                                         // UTILITIE:5: fell from the sky
        d_.putStrC(back_, 201);
        d_.putStrC(back_, 202);
        d_.putStrC(back_, 203);
        for (int i = 0; i < w_.party.count(); ++i) w_.party.member(i).status = Status(5);
        g_.x = g_.y = g_.level = 0;
        xgoto_ = 10;
        leave(false);
    } else if (g_.level == 0) {
        if (g_.x == 0 && g_.y == 0) {                                  // UTILITIE:7: the castle gates
            xgoto_ = 10;
            leave(false);
        }
        d_.putStrC(back_, 204);                                        // UTILITIE:6: the moat
        d_.putStrC(back_, 205);
        for (int i = 0; i < w_.party.count(); ++i) {
            Character &c = w_.party.member(i);
            if (int(c.status) < 5 && w_.rng.mod(25) > c.attrib[4]) c.status = Status(5);
        }
        g_.x = g_.y = g_.level = 0;
        xgoto_ = 10;
        leave(false);
    }
    leave(true);
}

// UTILITIE:2 -- PARTY TELEPORT
void Utilities::teleportPrompt() {
    Console &con = d_.con;
    int member = g_.g9;
    back_ = con.open(0, 7, 40, 6, 16, true);
    d_.putStrC(back_, 207);
    d_.newline(back_);
    d_.putStr(back_, 208);
    Win *m = con.open(5, 12, 12, 10, 17, true);
    int up = 0, north = 0, east = 0;
    int sel;
    do {
        d_.gotoXY(back_, 8, 2); d_.putNum(back_, east, 4);
        d_.gotoXY(back_, 22, 2); d_.putNum(back_, north, 4);
        d_.gotoXY(back_, 33, 2); d_.putNum(back_, up, 4);
        d_.gotoXY(m, 0, 0);
        sel = d_.menuStr(m, 209);
        if (d_.quit) return;
        switch (sel) {
            case 0: ++east; break;
            case 1: --east; break;
            case 2: ++north; break;
            case 3: --north; break;
            case 4: ++up; break;
            case 5: --up; break;
            case 6: teleportApply(east, north, up, m); break;          // UTILITIE:3 (does not return)
            default: break;
        }
    } while (sel <= 5);
    con.close(m, false);
    con.close(back_, true);
    xgoto_ = 27;
    g_.g9 = member;
}

// UTILITIE:1
UtilitiesResult Utilities::run(int xgoto) {
    UtilitiesResult res;
    xgoto_ = xgoto;
    try {
        if (xgoto == 13 || xgoto == 20) {                              // UTILITIE:29
            for (int i = 0; i < w_.party.count(); ++i) derive(i, true);
            xgoto_ = xgoto == 13 ? 12 : 7;
        } else if (xgoto == 14) {
            if (g_.g9 >= 0) {                                          // UTILITIE:30
                int member = g_.g9;
                derive(member, false);
                xgoto_ = 18;
                g_.g9 = member;
            } else {
                equipEveryone();
            }
        } else if (xgoto == 15) {
            reorder();
        } else if (xgoto == 24) {
            teleportPrompt();
        }
    } catch (const EndAll &) {
    }
    res.xgoto = xgoto_;
    res.member = g_.g9;
    res.windowClosed = d_.quit;
    return res;
}

} // namespace wiz
