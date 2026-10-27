// SEGMENT CUTIL (the round: the picture panel, the party rows, the enemy list, the orders) -- see combat_dos.h.
// Every routine names the p-code proc it ports; `tools/pcode_decomp.py CUTIL <n>` prints the original.
#include "wiz/combat_dos.h"

#include <algorithm>
#include <cstring>

namespace wiz {

namespace {
struct EndInput {};                  // EXIT(11:2)
struct EndSpell {};                  // EXIT(11:19)
struct EndUse {};                    // EXIT(11:10)
struct EndPick {};                   // EXIT(11:9)
} // namespace

// ---- the picture panel (CUTIL:37..43) --------------------------------------------------------------

// CUTIL:37 -- a monster shakes off sleep / fear / paralysis
void Combat::wake(int g, int u, int pct) {
    if (pct > 50) pct = 50;
    if (rnd(100) <= pct) unit(g, u, 7) = 0;
}

// CUTIL:42 / 41 / 40 -- the pictures sink: the attribute bytes of the tile rows 9..13 shift down a row
// at a time, with a present and a short beat after each step
void Combat::slidePicture(int which, int total) {
    Win *w1 = d_.window1;
    int rowBytes = 2 * (w1->w - 2);
    int x;
    switch (total) {
        case 1: x = 15; break;
        case 2: x = 11 + 8 * which; break;
        case 3: x = 7 + 8 * which; break;
        default: x = 3 + 8 * which; break;
    }
    int xb = x * 2;
    auto attr = [&](int byteIdx) -> u8 & {                       // (WINDOW1+5w).b[]
        u16 &cell = w1->cell[size_t(byteIdx / 2)];
        static u8 dummy;
        (void)dummy;
        return reinterpret_cast<u8 *>(&cell)[byteIdx & 1];
    };
    for (int top = 9; top <= 13; ++top) {
        for (int row = 13; row >= top + 1; --row) {
            int base = rowBytes * row + xb;
            for (int k = 0; k <= 5; ++k) attr(base + k + k + 1) = attr(base + k + k + 1 - rowBytes);
        }
        int base = rowBytes * top + xb + 1;
        for (int k = 0; k <= 5; ++k) attr(base + k + k) = 0;
        d_.con.present(w1);                                        // CUTIL:41
        d_.pause(1);
    }
}

// CUTIL:39 -- pictures that are gone leave the screen
void Combat::dropPictures(const u8 *now) {
    int total = 0;                                                  // CUTIL:43: how many were up
    for (int i = 4; i >= 1; --i)
        if (pic_[i - 1] != 255) { total = i; break; }
    int k = 0;
    for (int i = 0; i <= 3; ++i) {
        if (pic_[i] == now[k]) ++k;
        else slidePicture(i, total);
    }
}

// CUTIL:38 -- close the ranks, let the monsters recover, and show whoever is left
void Combat::tidyGroups() {
    for (int g = 1; g <= 4; ++g) {
        if (G_[g][1] <= 0) continue;
        int j = 0;
        for (int i = 0; i < G_[g][1]; ++i) {
            for (int k = 0; k < 8; ++k) unit(g, j, k) = unit(g, i, k);
            if (unit(g, j, 7) < 5) {
                switch (unit(g, j, 7)) {
                    case 1: wake(g, j, 10 * G_[g][80]); break;
                    case 2: wake(g, j, 20 * G_[g][80]); break;
                    case 3: wake(g, j, 7 * G_[g][80]); break;
                    default: break;
                }
                unit(g, j, 3) += G_[g][111];
                ++j;
            }
        }
        G_[g][1] = j;
    }
    for (int a = 1; a <= 3; ++a)
        for (int b = a + 1; b <= 4; ++b)
            if (G_[a][1] == 0 && G_[b][1] > 0)
                for (int k = 0; k < 123; ++k) std::swap(G_[a][k], G_[b][k]);
    u8 now[4] = {255, 255, 255, 255};
    int n = 0;
    for (int g = 1; g <= 4; ++g)
        if (G_[g][1] > 0) now[n++] = u8(G_[g][76]);
    done_ = n == 0;
    if (pic_[0] != 255) dropPictures(now);
    bool differs = false;
    for (int i = 0; i <= 3; ++i)
        if (pic_[i] != now[i]) differs = true;
    if (differs) {
        d_.con.clearAttr(d_.window1);
        if (!done_) d_.con.blit13(d_.window1, now);
        d_.con.present(d_.window1);
    }
    std::memcpy(pic_, now, 4);
}

// CUTIL:44 -- the party's poison, regeneration and recovery
void Combat::regenerate() {
    for (int i = 0; i < partyCount(); ++i) {
        if (unit(0, i, 7) >= 5) continue;
        const Character &c = chr(i);
        if (c.poison > 0) {
            if (unit(0, i, 3) > 50) unit(0, i, 3) -= unit(0, i, 3) / 10;
            else unit(0, i, 3) -= c.poison;
        }
        unit(0, i, 3) += c.healPts;
        if (unit(0, i, 3) > c.hpMax) unit(0, i, 3) = c.hpMax;
        if (unit(0, i, 3) <= 0) { unit(0, i, 7) = 5; unit(0, i, 3) = 0; }
        switch (unit(0, i, 7)) {
            case 1: wake(0, i, 5 * c.charLevel); break;
            case 2: wake(0, i, 10 * c.charLevel); break;
            default: break;
        }
    }
    for (int i = 0; i < partyCount(); ++i) {
        chr(i).hpLeft = unit(0, i, 3);
        chr(i).status = Status(unit(0, i, 7));
    }
}

// CUTIL:45 / 46 -- the hold counters run down
void Combat::tickTimers() {
    auto tick = [&](int g, int n) {
        for (int u = 0; u < n; ++u) if (unit(g, u, 5) > 0) --unit(g, u, 5);
    };
    tick(0, partyCount());
    for (int g = 1; g <= 3; ++g) tick(g, G_[g][1]);
}

// CUTIL:36
void Combat::redraw() {
    tidyGroups();
    regenerate();
    tickTimers();
}

// CUTIL:47 -- the enemy list: "n) 4 SCRUFFY MEN (4)"
void Combat::enemyList() {
    g_.g35 = 0;
    d_.con.clear(list_, true);
    for (int g = 1; g <= 4; ++g) {
        if (G_[g][1] <= 0) continue;
        int awake = 0;
        for (int u = 0; u < G_[g][1]; ++u) if (unit(g, u, 7) == 0) ++awake;
        g_.g35 += awake * G_[g][80];
        d_.putNum(list_, g, 1);
        d_.putS(list_, ") ");
        int base = 13000 + G_[g][3] * 4;
        std::string name = d_.str(G_[g][0] ? (G_[g][1] > 1 ? base + 3 : base + 2) : (G_[g][1] > 1 ? base + 1 : base));
        name = d_.nameWith(name, 1300);
        name = Dos::substituteChar(name, 35, Dos::itoa(G_[g][1]));
        d_.putS(list_, name);
        while (list_->cx < list_->w - 6) d_.putC(list_, ' ');
        d_.putS(list_, " (");
        d_.putNum(list_, awake, 1);
        d_.putC(list_, ')');
    }
}

// CUTIL:50 -- swap two party members (and everything the fight keeps about them)
void Combat::swapMembers(int a, int b) {
    w_.party.swapMembers(a, b);                                    // the character copies and CHARDISK
    std::swap(flag_[a], flag_[b]);
    for (int k = 0; k < 8; ++k) std::swap(unit(0, a, k), unit(0, b, k));
}

// CUTIL:49 -- the end of a party row: the trend mark, then the status or the maximum HP
void Combat::partyStatusTail(int i, bool &active) {
    const Character &c = chr(i);
    int trend = c.healPts - c.poison;
    d_.putC(w_.g10, trend == 0 ? ' ' : (trend < 0 ? '-' : '+'));
    active = active || int(c.status) <= 2;
    if (c.status == Status::OK) {
        if (c.poison == 0) d_.putNum(w_.g10, c.hpMax, 4);
        else d_.putStr(w_.g10, 1005);
    } else {
        d_.putS(w_.g10, d_.str(10040 + int(c.status)).substr(0, 6));
    }
}

// CUTIL:48 -- put the party in order and redraw its rows
void Combat::sortParty() {
    int n = partyCount();
    for (int a = 0; a <= n - 2; ++a)
        for (int b = a + 1; b <= n - 1; ++b)
            if (disk_[a] == w_.party.rosterSlot(b)) swapMembers(a, b);
    for (int a = 0; a <= n - 2; ++a)
        for (int b = a + 1; b <= n - 1; ++b)
            if (int(chr(a).status) > int(chr(b).status)) swapMembers(a, b);
    g_.g36 = 0;
    G_[0][1] = 0;
    for (int i = 0; i < n; ++i) {
        if (chr(i).status == Status::OK) g_.g36 += chr(i).charLevel;
        if (int(chr(i).status) < 5) ++G_[0][1];
    }
    bool active = false;
    for (int i = 0; i < n; ++i) {
        const Character &c = chr(i);
        int r = rnd(99);
        if (r < c.attrib[IQ] + c.attrib[PIETY] + c.charLevel) G_[rnd(4) + 1][0] = 1;
        Win *g10 = w_.g10;
        d_.gotoXY(g10, 0, i + 1);
        d_.putNum(g10, i + 1, 1);
        d_.putC(g10, ' ');
        d_.putS(g10, c.name);
        while (g10->cx < 18) d_.putC(g10, ' ');
        d_.putS(g10, d_.str(10009 + int(c.align)).substr(0, 1));
        d_.putC(g10, '-');
        d_.putS(g10, d_.str(10030 + int(c.cls)).substr(0, 3));
        d_.putC(g10, ' ');
        int ac = c.armorClass - g_.protect - unit(0, i, 4);
        if (ac > -10) d_.putNum(g10, ac, 2);
        else d_.putStr(g10, 1006);
        d_.putNum(g10, c.hpLeft, 5);
        partyStatusTail(i, active);
        while (g10->cx != 0) d_.putC(g10, ' ');
    }
    if (!active) endFight();
}

// CUTIL:1
void Combat::cutil() {
    redraw();
    sortParty();
    enemyList();
    if (done_) return;
    inputPhase();
}

// ============================================================================================
//  the orders (CUTIL:2 and the procs nested in it)
// ============================================================================================

namespace {
// the spell tables of the scenario (TOC words 234 (3-bit groups) and 245 (2-bit target kinds))
int spellGroupOf(const Scenario &sc, int s) { return (sc.tocWord(234 + s / 5) >> ((s % 5) * 3)) & 7; }
int spellKindOf(const Scenario &sc, int s) { return (sc.tocWord(245 + s / 8) >> ((s % 8) * 2)) & 3; }
int hashOfName(const std::string &t) {
    int h = int(t.size());
    for (size_t i = 1; i <= t.size(); ++i)
        if (h < 20000) { int v = u8(t[i - 1]) - 64; h += v * v * int(i); }
    return h;
}
} // namespace

std::string Combat::spellName(int idx) const {                     // CUTIL:27
    std::string s = d_.str(5000 + idx);
    if (!s.empty() && s[0] == '*') s.erase(0, 1);
    return s;
}

int Combat::spellHash(int idx) const { return hashOfName(spellName(idx)); }

// CUTIL:3 -- a "** MESSAGE **" page
void Combat::headline(int strNo) {
    d_.con.clear(msg_, true);
    d_.newline(msg_);
    d_.putCentered(msg_, "** " + d_.str(strNo) + " **");
}

// CUTIL:4 -- the order is echoed in the member's row
void Combat::echoOrder(int member, const std::string &text) {
    if (unit(0, member, 1) == -999) return;
    std::string s = text;
    if (!s.empty() && s[0] == '*') s.erase(0, 1);
    d_.gotoXY(w_.g10, 32, member + 1);
    for (size_t i = 0; i < s.size() && w_.g10->cx != 0; ++i) d_.putC(w_.g10, u8(s[i]));   // CUTIL:5
    while (w_.g10->cx != 0) d_.putC(w_.g10, ' ');
}

// CUTIL:7 -- a key typed ahead that is one of `keys` ('/' stands for RETURN)
bool Combat::typedAhead(const std::string &keys) {
    while (d_.keyReady() && !d_.quit) {
        d_.getKey();
        for (char ch : keys) {
            if (ch == '/') { if (d_.inchar == 13) return true; }
            else if (d_.inchar == u8(ch)) return true;
        }
    }
    return false;
}

// CUTIL:9 -- "WHICH GROUP (#) ?": a group number and the action to take on it
void Combat::pickGroup(int member, int promptNo, int action) {
    if (G_[2][1] == 0) {                                           // only one group
        unit(0, member, 0) = 1;
        unit(0, member, 1) = action;
        return;
    }
    if (!typedAhead("1234/")) {
        d_.con.clearRect(msg_, 0, 2, 38, 2);
        d_.gotoXY(msg_, 0, 2);
        d_.putStrC(msg_, promptNo);
        d_.putStrC(msg_, 1010);
        d_.inchar = 48 + d_.getNum(4);
        if (d_.inchar == 48) d_.inchar = 13;
    }
    if (d_.inchar == 13 || G_[d_.inchar - 48][1] == 0) { unit(0, member, 1) = -999; return; }
    unit(0, member, 0) = d_.inchar - 48;
    unit(0, member, 1) = action;
}

// CUTIL:32 / 33 -- RUN
void Combat::tryRun(int) {
    if (!g_.xgoto2) giveUpRunning();
    int chance = 75;
    if (partyCount() < 4) chance += 20 - 5 * partyCount();
    if (g_.g36 > g_.g35) chance += 20;
    if (rnd(100) > chance) giveUpRunning();
    for (int g = 1; g <= 4; ++g) { G_[g][1] = 0; G_[g][2] = 0; }
    xgoto_ = 22;
    done_ = true;
    throw EndInput{};
}

void Combat::giveUpRunning() {                                     // CUTIL:33
    for (int i = 0; i < partyCount(); ++i) unit(0, i, 2) = -1;
    throw EndInput{};
}

// CUTIL:15..18 -- a used item may crumble into another
void Combat::itemSideEffects(int member, int slot, int changeTo, int chance) {
    if (rnd(100) >= chance) return;
    code_[member] = -changeTo;
    slot_[member] = slot;
}

// CUTIL:10..18 -- U)SE an item
void Combat::useItem(int member) {
    Character &c = chr(member);
    if (c.possCount == 0) return;
    // CUTIL:13 -- the usable items in a box
    Win *box = nullptr;
    int listed = 0;
    for (int k = 1; k <= c.possCount; ++k) {
        const Possession &p = c.poss[k - 1];
        ObjectRec o{w_.sc.record(Scenario::Object, p.itemIndex)};
        if (o.spellPwr() > 0 && (int(o.type()) >= 5 || p.equipped)) {
            if (!box) box = d_.con.open(9, 13, 22, 10, 31, true);
            ++listed;
            d_.putNum(box, k, 1);
            d_.putS(box, ") ");
            d_.putStr(box, 14000 + 2 * p.itemIndex + (p.identified ? 1 : 0));
            d_.newline(box);
        }
    }
    if (box) d_.con.close(box, false);
    if (listed == 0) { d_.con.refresh(); return; }
    d_.con.clearRect(msg_, 0, 2, 38, 2);
    d_.gotoXY(msg_, 0, 2);
    d_.putStrC(msg_, 1013);
    d_.putStrC(msg_, 1014);
    int k;
    ObjectRec o{};
    for (;;) {
        k = d_.getNum(c.possCount);
        if (k == 0 || d_.quit) { d_.con.refresh(); return; }
        o = ObjectRec{w_.sc.record(Scenario::Object, c.poss[k - 1].itemIndex)};
        if (o.spellPwr() != 0 && (int(o.type()) >= 5 || c.poss[k - 1].equipped)) break;
    }
    d_.con.clearRect(msg_, 0, 2, 38, 2);
    d_.gotoXY(msg_, 0, 2);
    d_.con.refresh();
    int spell = o.spellPwr();
    int hash = spellHash(spell);
    switch (spellKindOf(w_.sc, spell)) {
        case 0:                                                    // CUTIL:16
            unit(0, member, 1) = hash;
            unit(0, member, 0) = -1;
            itemSideEffects(member, k, o.changeTo(), o.chgChance());
            break;
        case 1: {                                                  // CUTIL:17
            d_.putStrC(msg_, 1011);
            int who;
            for (;;) {
                who = d_.getNum(partyCount()) - 1;
                if (who == -1 || d_.quit) { d_.con.refresh(); return; }
                if (who >= 0) break;
            }
            unit(0, member, 0) = who;
            unit(0, member, 1) = hash;
            itemSideEffects(member, k, o.changeTo(), o.chgChance());
            break;
        }
        default:                                                   // CUTIL:18
            pickGroup(member, 1012, hash);
            itemSideEffects(member, k, o.changeTo(), o.chgChance());
            break;
    }
    pendingText_ = 5000 + spell;
    d_.con.refresh();
}

// CUTIL:19..31 -- S)PELL: a name typed and matched, then the target
void Combat::castSpell(int member) {
    Character &c = chr(member);
    d_.con.clearRect(msg_, 0, 2, 38, 2);
    d_.gotoXY(msg_, 0, 2);
    d_.putStr(msg_, 1019);
    int x = msg_->cx, y = msg_->cy;
    std::string typed = d_.getLine(msg_, 12);
    if (typed.empty()) return;
    // CUTIL:25 / 29: an exact name, else the known spells it is the start of
    int hash = -1;
    int th = hashOfName(typed);
    bool exact = false;
    for (int s = 1; s <= 50 && !exact; ++s)
        if (th == spellHash(s) && spellName(s) == typed) exact = true;
    if (exact) {
        hash = th;
    } else {
        std::vector<int> list;
        for (int s = 1; s <= 50; ++s)
            if (spellName(s).compare(0, typed.size(), typed) == 0 && c.spellKnown[s]) list.push_back(s);
        typed.clear();
        if (list.size() == 1) {
            hash = spellHash(list[0]);
            typed = spellName(list[0]);
        } else if (list.size() > 1) {                              // CUTIL:30
            int n = int(list.size());
            Win *m = d_.con.open(7, 9 - (n + 1) / 2, 26, 6 + n, 31, true);
            d_.putStrC(m, 1040);
            d_.newline(m);
            for (int k = 0; k < n; ++k) {
                d_.gotoXY(m, 0, k + 2);
                d_.putC(m, 65 + k);
                d_.putS(m, ") ");
                d_.putS(m, spellName(list[k]));
                while (m->cx < 21) d_.putC(m, ' ');
                d_.putS(m, "(");
                int lvl = spellGroupOf(w_.sc, list[k]);
                d_.putNum(m, list[k] < 22 ? c.mageSpells[lvl] : c.priestSpells[lvl], 1);
                d_.putC(m, ')');
            }
            d_.gotoXY(m, 0, n + 3);
            d_.putStrC(m, 1041);
            for (;;) {
                d_.getKey();
                if (d_.quit) break;
                if (d_.inchar == 13) { d_.con.close(m, true); break; }
                int k = d_.inchar - 65;
                if (k >= 0 && k < n) {
                    hash = spellHash(list[k]);
                    typed = spellName(list[k]);
                    d_.con.close(m, true);
                    break;
                }
                d_.sound(0);
            }
        }
    }
    d_.gotoXY(msg_, x, y);
    d_.putS(msg_, typed);
    d_.pause(2);
    if (hash == -1) return;
    // CUTIL:20
    for (int s = 0; s <= 50; ++s) {
        if (hash != (s == 0 ? hashOfName(spellName(0)) : spellHash(s))) continue;
        pendingText_ = 5000 + s;
        d_.con.clearRect(msg_, 0, 2, 38, 2);
        d_.gotoXY(msg_, 0, 2);
        int group = spellGroupOf(w_.sc, s);
        // CUTIL:21 -- the points must be there
        auto reserve = [&]() {
            if (c.spellKnown[s]) {
                if (s < 22) {
                    if (c.mageSpells[group] > 0) { code_[member] = 1; slot_[member] = group; return true; }
                } else if (c.priestSpells[group] > 0) { code_[member] = 2; slot_[member] = group; return true; }
            }
            headline(c.spellKnown[s] ? 1015 : 1016);
            d_.pause(20);
            return false;
        };
        switch (spellKindOf(w_.sc, s)) {
            case 0:                                                // CUTIL:22
                if (!reserve()) return;
                unit(0, member, 1) = hash;
                unit(0, member, 0) = -1;
                break;
            case 1: {                                              // CUTIL:23
                if (!reserve()) return;
                d_.putStrC(msg_, 1017);
                int who;
                for (;;) {
                    who = d_.getNum(partyCount()) - 1;
                    if (who == -1 || d_.quit) return;
                    if (who >= 0) break;
                }
                unit(0, member, 0) = who;
                unit(0, member, 1) = hash;
                break;
            }
            default:                                               // CUTIL:24
                if (!reserve()) return;
                pickGroup(member, 1018, hash);
                break;
        }
        break;
    }
}

// CUTIL:2
void Combat::inputPhase() {
    Console &con = d_.con;
    try {
        // CUTIL:34 -- who was surprised
        if (ambush_ == 1) headline(1020);
        else if (ambush_ == 2) headline(1021);
        if (ambush_ > 0) d_.pause(20);
        d_.pause(20);
        for (;;) {
            for (int i = 0; i < partyCount(); ++i) {              // CUTIL:6
                if (chr(i).status == Status::OK) {
                    d_.gotoXY(w_.g10, 32, i + 1);
                    d_.putS(w_.g10, "??????");
                }
                code_[i] = 99;
            }
            for (int m = 0; m < partyCount(); ++m) {
                if (!(unit(0, m, 7) == 0 && ambush_ != 2)) { unit(0, m, 2) = -1; continue; }
                unit(0, m, 1) = -999;
                do {
                    const Character &c = chr(m);
                    int init = rnd(10);
                    switch (c.attrib[AGI]) {
                        case 3: init += 3; break;
                        case 4: case 5: init += 2; break;
                        case 6: case 7: init += 1; break;
                        case 15: init -= 1; break;
                        case 16: init -= 2; break;
                        case 17: init -= 3; break;
                        case 18: init -= 4; break;
                        default: break;
                    }
                    if (init < 1) init = 1; else if (init > 10) init = 10;
                    unit(0, m, 2) = init;
                    bool canDispel = false;
                    std::string menu, keys;
                    auto option = [&](int strNo) {                 // CUTIL:35
                        std::string s = d_.str(strNo);
                        menu += s;
                        keys += s.substr(0, 1);
                    };
                    if (m < 3) option(1022);
                    int cls = int(c.cls);
                    if (cls == 2 || (cls == 6 && c.charLevel > 8) || (cls == 4 && c.charLevel > 3)) { canDispel = true; option(1023); }
                    if (ambush_ == 0) option(1024);
                    keys += d_.str(1026);
                    con.clearRect(msg_, 0, 0, 38, 2);
                    d_.gotoXY(msg_, 0, 0);
                    d_.putCentered(msg_, c.name + d_.str(1027));
                    int sel;
                    if (!typedAhead(keys)) {
                        con.clearRect(msg_, 0, 1, 38, 3);
                        d_.newline(msg_);
                        sel = d_.menu(msg_, menu + d_.str(1025));
                        if (d_.quit) return;
                    } else {
                        sel = 0;
                        while (sel < int(keys.size()) && u8(keys[sel]) != d_.inchar) ++sel;
                    }
                    if (m > 2) ++sel;
                    if (!canDispel && sel > 0) ++sel;
                    if (ambush_ != 0 && sel > 1) ++sel;
                    pendingText_ = -1;
                    try {
                        switch (sel) {
                            case 0:                                // F)IGHT
                                if (m < 3) { pickGroup(m, 1030, -1); echoOrder(m, d_.str(1031)); }
                                break;
                            case 1:                                // D)ISPELL
                                if (canDispel) { pickGroup(m, 1028, -5); echoOrder(m, d_.str(1029)); }
                                break;
                            case 2:                                // S)PELL
                                if (ambush_ == 0) {
                                    castSpell(m);
                                    if (pendingText_ >= 0) echoOrder(m, d_.str(pendingText_));
                                }
                                break;
                            case 3:                                // U)SE
                                useItem(m);
                                if (pendingText_ >= 0) echoOrder(m, d_.str(pendingText_));
                                break;
                            case 4:                                // P)ARRY
                                unit(0, m, 1) = 0;
                                echoOrder(m, d_.str(1032));
                                break;
                            case 5:                                // R)UN
                                for (int i = 0; i < partyCount(); ++i) {
                                    if (chr(i).status == Status::OK) {
                                        d_.gotoXY(w_.g10, 32, i + 1);
                                        d_.putS(w_.g10, "??????");
                                    }
                                    code_[i] = 99;
                                }
                                tryRun(m);
                                break;
                            default:                               // T)AKE BACK
                                unit(0, m, 1) = -1;
                                m = -1;
                                for (int i = 0; i < partyCount(); ++i) {
                                    if (chr(i).status == Status::OK) {
                                        d_.gotoXY(w_.g10, 32, i + 1);
                                        d_.putS(w_.g10, "??????");
                                    }
                                    code_[i] = 99;
                                }
                                break;
                        }
                    } catch (const EndSpell &) {
                    } catch (const EndUse &) {
                    }
                } while (m != -1 && unit(0, m, 1) == -999);
            }
            if (ambush_ != 2) {
                con.clear(msg_, true);
                d_.newline(msg_);
                d_.putStrC(msg_, 1035);
                d_.putStrC(msg_, 1036);
                std::string t = d_.str(1037);
                do d_.getKey(); while (!d_.quit && d_.inchar != 13 && d_.inchar != u8(t[0]));
                if (d_.quit) return;
            } else {
                d_.inchar = 13;
            }
            if (d_.inchar != 13) continue;                         // T)AKE BACK ORDERS
            for (int i = 0; i < partyCount(); ++i) {               // commit the spell points / items
                Character &c = chr(i);
                if (code_[i] <= 0) {
                    Possession &p = c.poss[slot_[i] - 1];
                    p.itemIndex = std::abs(code_[i]);
                    p.identified = false;
                } else if (code_[i] == 1) {
                    --c.mageSpells[slot_[i]];
                } else if (code_[i] == 2) {
                    --c.priestSpells[slot_[i]];
                }
            }
            return;
        }
    } catch (const EndInput &) {
    }
}

} // namespace wiz
