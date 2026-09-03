#include "wiz/camp_ui.h"
#include "wiz/scenario.h"
#include "wiz/string_pool.h"

#include <cstdio>
#include <string>

namespace wiz {
namespace {

const char *nameOf(const std::vector<std::string> &v, int i) {
    return i >= 0 && i < int(v.size()) ? v[i].c_str() : "?";
}

struct CampCtx {
    Ui &ui;
    Party &party;
    const Scenario &sc;
    const StringPool *sp;
    Rng &rng;
    TextScreen &t() { return ui.ts(); }
};

std::string objName(const CampCtx &c, int idx, bool identified) {
    if (c.sp) {
        bool ok = false;
        std::string s = c.sp->get(StringPool::objectNameKey(idx, identified ? 1 : 0), &ok);
        if (ok && !s.empty()) return s;
    }
    return "ITEM #" + std::to_string(idx);
}

// ---- DSPSPELS: the 7 mage / 7 priest spell-slot counts ---------------
void dspSpels(CampCtx &c, const Character &ch) {
    auto &t = c.t();
    char b[48];
    std::string mage = "  MAGE ", pri = "PRIEST ";
    for (int i = 1; i <= 7; ++i) {
        std::snprintf(b, sizeof b, "%d%s", ch.mageSpells[i], i < 7 ? "/" : "");
        mage += b;
        std::snprintf(b, sizeof b, "%d%s", ch.priestSpells[i], i < 7 ? "/" : "");
        pri += b;
    }
    t.gotoXY(6, 9);  t.write(mage);
    t.gotoXY(6, 10); t.write(pri);
}

// ---- DSPITEMS: the pack, two columns, with the flag glyph -----------
void dspItems(CampCtx &c, const Character &ch) {
    auto &t = c.t();
    t.gotoXY(0, 12); t.write("*=EQUIP -=CURSED ?=UNKNOWN #=UNUSABLE");
    for (int r = 14; r <= 17; ++r) t.clearRect(0, r, 40, 1);
    for (int i = 1; i <= ch.possCount; ++i) {
        const Possession &p = ch.poss[i - 1];
        ObjectRec o{c.sc.record(Scenario::Object, p.itemIndex)};
        bool canUse = o.classUse(int(ch.cls));
        char flag = '?';
        if (p.equipped)          flag = p.cursed ? '-' : '*';
        else if (p.identified)   flag = canUse ? ' ' : '#';
        int col = 20 - 20 * (i % 2), row = 14 + (i - 1) / 2;
        char b[40];
        std::snprintf(b, sizeof b, "%d)%c%s", i, flag,
                      objName(c, p.itemIndex, p.identified).c_str());
        t.gotoXY(col, row); t.write(std::string(b).substr(0, 19));
    }
}

// ---- DSPSTATS: the full character sheet ----------------------------
void dspStats(CampCtx &c, const Character &ch) {
    auto &t = c.t();
    t.resetWindow();
    t.putChar(12);
    char b[64];

    std::string hdr = ch.name + "  " + nameOf(c.sc.races(), int(ch.race)) + " " +
                      std::string(1, nameOf(c.sc.aligns(), int(ch.align))[0]) + "-" +
                      nameOf(c.sc.classes(), int(ch.cls));
    t.gotoXY(0, 0); t.write(hdr);

    static const char *lbl[6] = {"STRENGTH", "I.Q.", "PIETY", "VITALITY", "AGILITY", "LUCK"};
    for (int i = 0; i < 6; ++i) {
        t.gotoXY(0, 2 + i); t.writeField(lbl[i], 10);
        std::snprintf(b, sizeof b, "%3d", ch.attrib[i]); t.write(b);
    }
    t.gotoXY(16, 2); std::snprintf(b, sizeof b, "GOLD %lld", (long long)ch.gold.v); t.write(b);
    t.gotoXY(16, 3); std::snprintf(b, sizeof b, "EXP  %lld", (long long)ch.exp.v);  t.write(b);
    t.gotoXY(16, 5); std::snprintf(b, sizeof b, "LEVEL %-3d AGE %d", ch.charLevel, ch.age / 52);
    t.write(b);
    t.gotoXY(16, 6); std::snprintf(b, sizeof b, "HITS %d/%d  AC %d", ch.hpLeft, ch.hpMax, ch.armorClass);
    t.write(b);
    t.gotoXY(16, 7);
    std::string st = std::string("STATUS ") + nameOf(c.sc.statuses(), int(ch.status));
    if (ch.poison > 0) st += " & POISONED";
    t.write(st.substr(0, 23));

    dspSpels(c, ch);
    dspItems(c, ch);
}

// ---- READ SPELL BOOKS: list the character's known spells -----------
void readBooks(CampCtx &c, const Character &ch) {
    auto &t = c.t();
    t.putChar(12);
    t.gotoXY(0, 0); t.write(ch.name + " KNOWS:");
    int row = 2, col = 0;
    for (int s = 1; s <= 50; ++s) {
        if (!ch.spellKnown[s]) continue;
        std::string nm;
        if (c.sp) { bool ok = false; nm = c.sp->get(StringPool::spellNameKey(s), &ok); if (!ok) nm.clear(); }
        if (nm.empty()) nm = "SPELL " + std::to_string(s);
        if (!nm.empty() && nm[0] == '*') nm.erase(0, 1);   // UI damage marker
        t.gotoXY(col, row); t.write(nm.substr(0, 18));
        std::printf("BOOK| %s knows %s\n", ch.name.c_str(), nm.c_str());
        row++;
        if (row > 21) { row = 2; col += 20; }
    }
    t.gotoXY(0, 23);
    c.ui.pressAnyKey("PRESS ANY KEY");
}

// ---- DROPITEM -----------------------------------------------------
void dropItem(CampCtx &c, Character &ch) {
    auto &t = c.t();
    for (;;) {
        t.gotoXY(0, 18); t.putChar(11);
        t.write("DROP ITEM (0=EXIT) ? >");
        int k = c.ui.getKey();
        if (c.ui.quit()) return;
        int n = k - '0';
        if (n == 0) return;
        if (n < 1 || n > ch.possCount) continue;
        Possession &p = ch.poss[n - 1];
        if (p.cursed)   { c.ui.pressAnyKey("** CURSED **"); return; }
        if (p.equipped) { c.ui.pressAnyKey("** EQUIPPED **"); return; }
        for (int j = n; j < ch.possCount; ++j) ch.poss[j - 1] = ch.poss[j];
        --ch.possCount;
        dspItems(c, ch);
        c.ui.pressAnyKey("** DROPPED **");
        return;
    }
}

// ---- REORDER (UTILITIE proc 27) ---------------------------------
void reorder(CampCtx &c) {
    if (c.party.count() < 2) return;
    auto &t = c.t();
    t.putChar(12);
    t.gotoXY(8, 1); t.write("REORDERING");
    for (int i = 0; i < c.party.count(); ++i) {
        t.gotoXY(0, 3 + i);
        char b[40];
        std::snprintf(b, sizeof b, "%d) %s", i + 1, c.party.member(i).name.c_str());
        t.write(b);
    }
    // LIST[oldPos] = newPos, entered as a permutation of current slots.
    int list[Party::kMax];
    for (int i = 0; i < c.party.count(); ++i) list[i] = 99;
    for (int newPos = 0; newPos < c.party.count() - 1; ++newPos) {
        for (;;) {
            t.gotoXY(0, 12); t.putChar(11);
            t.write("NEW #" + std::to_string(newPos + 1) + " IS OLD #? >");
            int k = c.ui.getKey();
            if (c.ui.quit()) return;
            int old = k - '1';
            if (old >= 0 && old < c.party.count() && list[old] == 99) {
                list[old] = newPos;
                break;
            }
        }
    }
    for (int i = 0; i < c.party.count(); ++i)
        if (list[i] == 99) list[i] = c.party.count() - 1;
    // selection sort by LIST, swapping party members to match
    for (int i = 0; i < c.party.count() - 1; ++i)
        for (int j = i + 1; j < c.party.count(); ++j)
            if (list[j] < list[i]) {
                c.party.swapMembers(i, j);
                std::swap(list[i], list[j]);
            }
}

// ---- CAMPMENU: one character's inspect sub-menu --------------------
// Returns false only if the window closed.
bool inspectChar(CampCtx &c, int idx) {
    for (;;) {
        Character &ch = c.party.member(idx);
        dspStats(c, ch);
        bool ok = ch.status == Status::OK;
        auto &t = c.t();
        t.gotoXY(0, 18); t.putChar(11);
        if (ok) t.write("E)QUIP D)ROP T)RADE R)EAD S)PELL U)SE I)DENT L)EAVE");
        else    t.write("E)QUIP D)ROP T)RADE R)EAD L)EAVE");
        int k = c.ui.getKey();
        if (c.ui.quit()) return false;
        if (k == 'L') return true;
        if (k == 'R') { readBooks(c, ch); continue; }
        if (k == 'D') { dropItem(c, ch); continue; }
        if (k == 'E' || k == 'T' || (ok && (k == 'S' || k == 'U' || k == 'I'))) {
            c.ui.pressAnyKey("-- NOT AVAILABLE IN CAMP YET --");
            continue;
        }
    }
}

// ---- CAMPMEN2: the top camp screen -------------------------------
void campList(CampCtx &c) {
    auto &t = c.t();
    t.resetWindow();
    t.putChar(12);
    t.gotoXY(18, 0); t.write("CAMP");
    t.gotoXY(0, 2);  t.write(" # CHARACTER NAME  CLASS AC HITS STATUS");
    for (int i = 0; i < c.party.count(); ++i) {
        const Character &ch = c.party.member(i);
        char b[80];
        std::snprintf(b, sizeof b, "%d %-15.15s %c-%-3.3s %3d %4d",
                      i + 1, ch.name.c_str(),
                      nameOf(c.sc.aligns(), int(ch.align))[0],
                      nameOf(c.sc.classes(), int(ch.cls)),
                      ch.armorClass, ch.hpLeft);
        t.gotoXY(0, 3 + i); t.write(b);
        std::string tail;
        if (ch.status == Status::OK)
            tail = ch.poison ? " POISON" : ("/" + std::to_string(ch.hpMax));
        else
            tail = std::string(" ") + nameOf(c.sc.statuses(), int(ch.status));
        t.write(tail);
    }
    t.gotoXY(0, 12); t.write("YOU MAY R)EORDER, E)QUIP, D)ISBAND,");
    t.gotoXY(8, 13); t.write("#) TO INSPECT, OR");
    t.gotoXY(8, 14); t.write("L)EAVE THE CAMP.");
}

} // namespace

CampExit runCamp(Ui &ui, Party &party, const Scenario &sc, const StringPool *sp,
                 Rng &rng) {
    CampCtx c{ui, party, sc, sp, rng};
    for (;;) {
        campList(c);
        int k = ui.getKey();
        if (ui.quit()) return CampExit::WindowClosed;
        if (k == 'L') return CampExit::ToMaze;
        if (k >= '1' && k <= '0' + party.count()) {
            if (!inspectChar(c, k - '1')) return CampExit::WindowClosed;
            continue;
        }
        if (k == 'R') { reorder(c); continue; }
        if (k == 'E') { ui.pressAnyKey("-- EQUIP NOT PORTED YET --"); continue; }
        if (k == 'D') {
            campList(c);
            c.t().gotoXY(0, 18); c.t().putChar(11);
            c.t().write("DISBAND THE PARTY?  CONFIRM (Y/N) ?");
            if (ui.menu("YN") != 'Y') continue;
            c.t().gotoXY(0, 19); c.t().write("RE-CONFIRM (Y/N) ?");
            if (ui.menu("YN") != 'Y') continue;
            return CampExit::Disbanded;
        }
    }
}

} // namespace wiz
