#include "wiz/camp_ui.h"
#include "wiz/roster.h"
#include "wiz/scenario.h"
#include "wiz/string_pool.h"
#include "wiz/equip.h"

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
    Roster &roster;
    const Scenario &sc;
    const StringPool *sp;
    Rng &rng;
    MazeState &st;
    bool done = false;                  // MALOR ended the delve
    CampExit exit = CampExit::ToMaze;
    TextScreen &t() { return ui.ts(); }
};

// GETCHARX: pick a party member by number (1..count); -1 on [RET] / quit.
int pickMember(CampCtx &c, const char *prompt) {
    auto &t = c.t();
    t.gotoXY(0, 18); t.putChar(11);
    t.write(std::string(prompt) + " (#) >");
    int k = c.ui.getKey();
    if (c.ui.quit() || k == KEY_RETURN) return -1;
    int i = k - '1';
    return (i >= 0 && i < c.party.count()) ? i : -1;
}

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

// ---- DOEQUIP: pick one item of a slot type to equip ---------------
void doEquipSlot(CampCtx &c, Character &ch, ObjType ot, const char *label) {
    int list[8], n = 0, cursed = -1;
    for (int i = 0; i < ch.possCount; ++i) {
        ObjectRec o{c.sc.record(Scenario::Object, ch.poss[i].itemIndex)};
        if (o.type() != ot || !o.classUse(int(ch.cls))) continue;
        list[n++] = i;
        if (ch.poss[i].cursed) cursed = i;
    }
    if (n == 0) return;

    auto &t = c.t();
    t.putChar(12);
    t.gotoXY(0, 0); t.write(std::string("SELECT ") + label + " FOR " + ch.name);
    for (int j = 0; j < n; ++j) {
        const Possession &p = ch.poss[list[j]];
        char flag = p.cursed ? '-' : (p.identified ? ' ' : '?');
        char b[48];
        std::snprintf(b, sizeof b, "  %d)%c%s", j + 1, flag,
                      objName(c, p.itemIndex, p.identified).c_str());
        t.gotoXY(0, 3 + j); t.write(std::string(b).substr(0, 39));
    }

    if (cursed >= 0) {                       // force-equipped, no choice
        ch.poss[cursed].equipped = true;
        c.ui.pressAnyKey("** CURSED **");
        return;
    }
    for (;;) {
        t.gotoXY(0, 15); t.putChar(11);
        t.write("WHICH ONE ([RET] FOR NONE) ? >");
        int k = c.ui.getKey();
        if (c.ui.quit() || k == KEY_RETURN) return;
        int p = k - '0';
        if (p >= 1 && p <= n) { ch.poss[list[p - 1]].equipped = true; return; }
    }
}

// ---- CHSPCPOW (UTILITIE P01011A): invoke an equipped item's SPECIAL ----
// Runs at the tail of a single-character EQUIP.  For each carried item with
// SPECIAL > 0 the player is asked Y/N; on Y the CHGCHANC roll may consume
// the item, then the one-shot effect fires.
void chSpcPow(CampCtx &c, Character &ch) {
    auto &t = c.t();
    for (int pi = 0; pi < ch.possCount; ++pi) {
        Possession &p = ch.poss[pi];
        ObjectRec o{c.sc.record(Scenario::Object, p.itemIndex)};
        int sp = o.special();
        if (sp <= 0) continue;

        t.putChar(12);
        t.gotoXY(0, 0); t.write("WILL YOU INVOKE THE SPECIAL POWER OF");
        t.gotoXY(0, 1); t.write("YOUR " + objName(c, p.itemIndex, p.identified) + " (Y/N) ?");
        c.ui.refresh();
        if (c.ui.menu("YN") != 'Y') continue;

        if (c.rng.mod(100) < o.chgChance()) p.itemIndex = o.changeTo();   // may consume
        std::printf("CHSPCPOW| %s invokes special %d\n", ch.name.c_str(), sp);

        auto bumpAttr = [&](int idx, int d) {
            int v = ch.attrib[idx] + d;
            if (v > 2 && v < 19) ch.attrib[idx] = v;
        };
        if (sp >= 1 && sp <= 6)       bumpAttr(sp - 1, +1);
        else if (sp >= 7 && sp <= 12) bumpAttr(sp - 7, -1);
        else switch (sp) {
            case 13: if (ch.age > 1040) ch.age -= 52; break;
            case 14: ch.age += 52; break;
            case 15: ch.cls = Class::Samurai; break;
            case 16: ch.cls = Class::Lord;    break;
            case 17: ch.cls = Class::Ninja;   break;
            case 18: ch.gold.v += 50000; break;
            case 19: ch.exp.v  += 50000; break;
            case 20: ch.status = Status::Lost; break;
            case 21: ch.status = Status::OK; ch.hpLeft = ch.hpMax; ch.poison = 0; break;
            case 22: ch.hpMax += 1; break;
            case 23:
                for (int m = 0; m < c.party.count(); ++m)
                    c.party.member(m).hpLeft = c.party.member(m).hpMax;
                break;
        }
        c.ui.pressAnyKey("DONE!");
    }
}

// EQUIP1: strip everything, walk the slot types, recompute the tail.
// `single` = the interactive one-character path (also runs CHSPCPOW); the
// party-wide E)QUIP passes false (DOS: ARM4CHAR, no CHSPCPOW).
void runEquip(CampCtx &c, Character &ch, bool single = true) {
    for (int i = 0; i < ch.possCount; ++i) ch.poss[i].equipped = false;
    doEquipSlot(c, ch, ObjType::Weapon,   "WEAPON");
    doEquipSlot(c, ch, ObjType::Armor,    "ARMOR");
    doEquipSlot(c, ch, ObjType::Shield,   "SHIELD");
    doEquipSlot(c, ch, ObjType::Helmet,   "HELMET");
    doEquipSlot(c, ch, ObjType::Gauntlet, "GAUNTLETS");
    doEquipSlot(c, ch, ObjType::Misc,     "MISC. ITEM");
    if (single) chSpcPow(c, ch);
    equipRecalc(ch, c.sc);
    std::printf("EQUIP| %s AC %d swings %d dmg %dd%d+%d slay %#x\n", ch.name.c_str(),
                ch.armorClass, ch.swingCnt, ch.hpDamRc[0], ch.hpDamRc[1],
                ch.hpDamRc[2], ch.wepSlay);
}

// ---- DOTRADE (P010C17): hand gold / items to another member -------
void doTrade(CampCtx &c, int fromIdx) {
    int to = pickMember(c, "TRADE WITH");
    if (to < 0 || to == fromIdx) return;
    Character &from = c.party.member(fromIdx);
    Character &dst  = c.party.member(to);
    auto &t = c.t();

    // TRADGOLD
    t.gotoXY(0, 18); t.putChar(11);
    t.write("AMT OF GOLD ? >");
    std::string g = c.ui.getLine(12);
    int64_t amt = 0;
    bool bad = g.empty();
    for (char ch : g) { if (ch < '0' || ch > '9') { bad = true; break; } amt = amt * 10 + (ch - '0'); }
    if (bad) { c.ui.pressAnyKey("** BAD AMT **"); }
    else if (amt > from.gold.v) { c.ui.pressAnyKey("** NOT ENOUGH $ **"); }
    else if (amt > 0) { from.gold.v -= amt; dst.gold.v += amt; }

    // TRADITEM -- loop until [RET]
    for (;;) {
        dspItems(c, from);
        t.gotoXY(0, 18); t.putChar(11);
        t.write("WHAT ITEM ([RET] EXITS) ? >");
        int k = c.ui.getKey();
        if (c.ui.quit() || k == KEY_RETURN) return;
        int n = k - '0';
        if (n < 1 || n > from.possCount) continue;
        Possession &p = from.poss[n - 1];
        if (dst.possCount >= 8) { c.ui.pressAnyKey("** FULL **"); return; }
        if (p.cursed)   { c.ui.pressAnyKey("** CURSED **"); return; }
        if (p.equipped) { c.ui.pressAnyKey("** EQUIPPED **"); return; }
        dst.poss[dst.possCount++] = p;
        for (int j = n; j < from.possCount; ++j) from.poss[j - 1] = from.poss[j];
        --from.possCount;
    }
}

// ---- CASTSPEL (P010C06): the in-camp (non-combat) spell set -------
// Effect kinds -- the DOS dispatch keyed by spell hash.
enum CampSpellKind { CS_HEAL, CS_FULLHEAL, CS_LIGHT, CS_UNPOISON, CS_CURE,
                     CS_PROTECT, CS_RESURRECT, CS_LOCATE, CS_KANDI, CS_MALOR };
struct CampSpell {
    int no; const char *name; bool priest; int group; CampSpellKind kind; int a; int b;
};
// group = spell level -> mageSpells[]/priestSpells[] pool.  a/b = healing
// dice; CS_LIGHT a = LIGHT value.
const CampSpell kCampSpells[] = {
    { 4, "DUMAPIC",  false, 1, CS_LOCATE,     0, 0},   // DUMAPIC (P01010D)
    {19, "MALOR",    false, 7, CS_MALOR,      0, 0},   // MALOR   (P01010E)
    {23, "DIOS",     true,  1, CS_HEAL,       1, 8},
    {25, "MILWA",    true,  1, CS_LIGHT,     15, 0},
    {31, "LOMILWA",  true,  3, CS_LIGHT,  32000, 0},
    {32, "DIALKO",   true,  3, CS_CURE,       0, 0},
    {35, "DIAL",     true,  4, CS_HEAL,       2, 8},
    {37, "LATUMOFI", true,  4, CS_UNPOISON,   0, 0},
    {38, "MAPORFIC", true,  4, CS_PROTECT,    0, 0},
    {39, "DIALMA",   true,  5, CS_HEAL,       3, 8},
    {42, "KANDI",    true,  5, CS_KANDI,      0, 0},   // KANDIFND (P01010A)
    {43, "DI",       true,  5, CS_RESURRECT,  5, 0},   // from DEAD
    {46, "MADI",     true,  6, CS_FULLHEAL,   0, 0},
    {50, "KADORTO",  true,  7, CS_RESURRECT,  7, 0},   // from DEAD or ASHES
};

int &campPool(Character &c, const CampSpell &s) {
    return s.priest ? c.priestSpells[s.group] : c.mageSpells[s.group];
}

// DODIKADO / DIKADORT: resurrection.  `mode` 5 = DI (-> 1 HP), 7 = KADORTO
// (-> full HP, also from ASHES).
void resurrect(CampCtx &c, Character &v, int mode) {
    if (v.status == Status::Lost) { c.ui.pressAnyKey("** LOST **"); return; }
    if (mode == 5 && v.status != Status::Dead) {
        c.ui.pressAnyKey(v.status == Status::Ashes ? "** KADORTO NEEDED **" : "** NOT DEAD **");
        return;
    }
    if (mode == 7 && v.status != Status::Dead && v.status != Status::Ashes) {
        c.ui.pressAnyKey("** NOT DEAD **"); return;
    }
    int vit = v.attrib[VIT];
    if (c.rng.mod(100) <= 4 * vit) {                       // success
        v.status = Status::OK;
        v.hpLeft = (mode == 5) ? 1 : v.hpMax;
        v.poison = 0;
        if (vit == 3) v.status = Status::Lost;
        else v.attrib[VIT] = vit - 1;
        c.ui.pressAnyKey(v.status == Status::OK ? "EXCELSIOR!" : "OOPS!");
    } else {                                               // botch -> worse
        if (int(v.status) < int(Status::Lost)) v.status = Status(int(v.status) + 1);
        c.ui.pressAnyKey("OOPS!");
    }
}

// ---- DUMAPIC (UTILITIE P01010D): tell the party where it is ---------
void dumapic(CampCtx &c) {
    auto &t = c.t();
    t.putChar(12);
    if (c.st.level >= 10) {
        t.gotoXY(0, 0); t.write("ENCHANTMENTS PREVENT SPELL FROM WORKING");
        c.ui.pressAnyKey("PRESS ANY KEY");
        return;
    }
    static const char *dir[4] = {"NORTH", "EAST", "SOUTH", "WEST"};
    t.gotoXY(0, 0);  t.write("PARTY LOCATION:");
    t.gotoXY(0, 2);  t.write(std::string("THE PARTY IS FACING ") + dir[c.st.pos.dir & 3] + ".");
    char b[64];
    std::snprintf(b, sizeof b, "YOU ARE %d SQUARES EAST AND", c.st.pos.x);
    t.gotoXY(0, 4); t.write(b);
    std::snprintf(b, sizeof b, "%d SQUARES NORTH OF THE STAIRS", c.st.pos.y);
    t.gotoXY(0, 5); t.write(b);
    std::snprintf(b, sizeof b, "TO THE CASTLE, AND %d LEVELS BELOW IT.", c.st.level);
    t.gotoXY(0, 6); t.write(b);
    std::printf("DUMAPIC| L%d (%d,%d) facing %s\n", c.st.level, c.st.pos.x, c.st.pos.y,
                dir[c.st.pos.dir & 3]);
    c.ui.pressAnyKey("L)EAVE WHEN READY");
}

// ---- KANDIFND (UTILITIE P01010A): locate a fallen companion --------
void kandiFind(CampCtx &c) {
    auto &t = c.t();
    t.putChar(12);
    t.gotoXY(0, 0); t.write("LOCATE BODIES");
    t.gotoXY(0, 2); t.write("FIND WHO ? >");
    std::string want = c.ui.getLine(15);
    t.putChar(12);
    t.gotoXY(0, 0); t.write("THE SOUL OF " + want + " IS..");

    std::string res = "LOST FOREVER!";
    for (int i = 0; i < c.roster.count(); ++i) {
        const Character &r = c.roster.slot(i);
        if (r.name != want) continue;
        if (r.status == Status::Lost) { res = "LOST FOREVER!"; break; }
        if (int(r.status) < int(Status::Dead)) { res = "STILL WITH US!"; break; }
        if (r.lostX == 0 && r.lostY == 0 && r.lostLevel == 0) res = "IN THE MORGUE";
        else if (r.lostLevel <= 0) res = "UNREACHABLE!";
        else res = std::string("IN THE ") + (r.lostY > 9 ? "NORTH " : "SOUTH ") +
                   (r.lostX > 9 ? "EAST" : "WEST") + " OF LEVEL " +
                   std::to_string(r.lostLevel);
        break;
    }
    t.gotoXY(0, 2); t.write(res);
    std::printf("KANDI| %s: %s\n", want.c_str(), res.c_str());
    c.ui.pressAnyKey("L)EAVE WHEN READY");
}

// ---- MALOR (UTILITIE P01010E): party teleport by displacement ------
void malor(CampCtx &c) {
    auto &t = c.t();
    const int nLevels = c.sc.count(Scenario::Maze);     // 10 -- MALOR can't reach it
    int de = 0, dn = 0, du = 0;                         // east / north / down
    for (;;) {
        t.putChar(12);
        t.gotoXY(0, 0); t.write("PARTY TELEPORT:");
        t.gotoXY(0, 2); t.write("N S E W U D SET DISPLACEMENT,");
        t.gotoXY(0, 3); t.write("[RETURN] TELEPORT, [ESC] CHICKEN OUT");
        char b[32];
        std::snprintf(b, sizeof b, "# SQUARES EAST  = %d", de); t.gotoXY(0, 5); t.write(b);
        std::snprintf(b, sizeof b, "# SQUARES NORTH = %d", dn); t.gotoXY(0, 6); t.write(b);
        std::snprintf(b, sizeof b, "# SQUARES DOWN  = %d", du); t.gotoXY(0, 7); t.write(b);
        c.ui.refresh();
        int k = c.ui.getKey();
        if (c.ui.quit()) { c.done = true; c.exit = CampExit::WindowClosed; return; }
        if (k == KEY_ESC) return;                       // chicken out -- no move
        if (k == KEY_RETURN) break;
        switch (k) {
            case 'N': ++dn; break;  case 'S': --dn; break;
            case 'E': ++de; break;  case 'W': --de; break;
            case 'D': ++du; break;  case 'U': --du; break;
        }
    }

    if (c.st.level + du == nLevels) {                   // BOUNCE
        c.ui.pressAnyKey("YOU BOUNCED BACK TO WHERE YOU WERE!");
        return;
    }
    int nx = c.st.pos.x + de, ny = c.st.pos.y + dn, nl = c.st.level + du;
    std::printf("MALOR| (%d,%d,L%d) -> (%d,%d,L%d)\n",
                c.st.pos.x, c.st.pos.y, c.st.level, nx, ny, nl);

    auto wipe = [&](const char *l1, const char *l2, Status s) {
        t.putChar(12);
        t.gotoXY(0, 0); t.write(l1);
        t.gotoXY(0, 1); t.write(l2);
        for (int i = 0; i < c.party.count(); ++i) {
            Character &m = c.party.member(i);
            m.inMaze = false;
            if (int(m.status) < int(s)) m.status = s;
        }
        c.ui.pressAnyKey("PRESS ANY KEY");
    };

    if ((nx < 0 || nx > 19 || ny < 0 || ny > 19 || nl > nLevels) && nl > 0) {
        wipe("YOU LANDED IN SOLID ROCK OUTSIDE THE",
             "DUNGEON - YOU ARE LOST FOREVER!", Status::Lost);
        c.done = true; c.exit = CampExit::ToMaze;       // -> loop-top -> CEMETARY
        return;
    }
    if (nl < 0) {
        wipe("YOU MATERIALIZED IN MID-AIR AND FELL", "TO A PAINFUL DEATH!", Status::Dead);
        c.done = true; c.exit = CampExit::ToMaze;
        return;
    }
    if (nl == 0) {
        if (nx == 0 && ny == 0) {
            c.ui.pressAnyKey("YOU RETURN TO THE CASTLE.");
        } else {
            t.putChar(12);
            t.gotoXY(0, 0); t.write("YOU APPEARED IN THE CASTLE MOAT AND");
            t.gotoXY(0, 1); t.write("PROBABLY DROWNED!");
            for (int i = 0; i < c.party.count(); ++i) {
                Character &m = c.party.member(i);
                if (int(m.status) < int(Status::Dead) && c.rng.mod(25) > m.attrib[AGI])
                    m.status = Status::Dead;
            }
            c.ui.pressAnyKey("PRESS ANY KEY");
        }
        c.done = true; c.exit = CampExit::ToTown;
        return;
    }
    c.st.pos.x = nx; c.st.pos.y = ny; c.st.level = nl;  // valid landing (1 .. 9)
    c.done = true; c.exit = CampExit::ToMaze;
}

// `viaItem` -> USE an item: skip the spell-point / known check and cost.
void campCast(CampCtx &c, Character &caster, int forcedNo) {
    struct Avail { const CampSpell *s; };
    Avail av[16]; int n = 0;
    for (const CampSpell &s : kCampSpells) {
        bool viaItem = forcedNo > 0;
        if (viaItem ? s.no != forcedNo
                    : (!caster.spellKnown[s.no] || campPool(caster, s) <= 0))
            continue;
        av[n++] = {&s};
        if (viaItem) break;
    }
    if (n == 0) { c.ui.pressAnyKey("** YOU CAN'T CAST IT **"); return; }

    const CampSpell *sp;
    if (forcedNo > 0) sp = av[0].s;
    else {
        auto &t = c.t();
        t.gotoXY(0, 18); t.putChar(11);
        std::string line = "CAST: ";
        for (int i = 0; i < n; ++i) { line += char('1' + i); line += ')'; line += av[i].s->name; line += ' '; }
        t.write(line.substr(0, 39));
        int k = c.ui.getKey();
        if (c.ui.quit()) return;
        int pick = k - '1';
        if (pick < 0 || pick >= n) return;
        sp = av[pick].s;
        --campPool(caster, *sp);                           // DECPRIEST / DECMAGE
    }
    std::printf("CAMPCAST| %s casts %s\n", caster.name.c_str(), sp->name);

    if (sp->kind == CS_LOCATE) { dumapic(c); return; }     // DUMAPIC
    if (sp->kind == CS_KANDI)  { kandiFind(c); return; }   // KANDIFND
    if (sp->kind == CS_MALOR)  { malor(c); return; }       // MALOR
    if (sp->kind == CS_LIGHT) {
        c.st.light = sp->a + (sp->a < 100 ? c.rng.mod(15) : 0);
        c.ui.pressAnyKey("DONE!");
        return;
    }
    if (sp->kind == CS_PROTECT) { c.st.protect = 2; c.ui.pressAnyKey("DONE!"); return; }

    int who = pickMember(c, "CAST ON WHO");
    if (who < 0) return;
    Character &v = c.party.member(who);
    switch (sp->kind) {
        case CS_HEAL: {
            int h = 0;
            for (int i = 0; i < sp->a; ++i) h += c.rng.mod(sp->b) + 1;
            v.hpLeft = std::min(v.hpMax, v.hpLeft + h);
            c.ui.pressAnyKey(("CURED " + std::to_string(h) + " HP").c_str());
            break;
        }
        case CS_FULLHEAL:
            v.hpLeft = v.hpMax; v.poison = 0;
            if (int(v.status) < int(Status::Dead)) v.status = Status::OK;
            c.ui.pressAnyKey("FULLY HEALED");
            break;
        case CS_UNPOISON: v.poison = 0; c.ui.pressAnyKey("DONE!"); break;
        case CS_CURE:
            if (v.status == Status::Paralyzed || v.status == Status::Asleep) v.status = Status::OK;
            c.ui.pressAnyKey("DONE!");
            break;
        case CS_RESURRECT: resurrect(c, v, sp->a); break;
        default: break;
    }
}

// ---- USEITEM (P010C11): invoke an item's SPELLPWR ----------------
void doUse(CampCtx &c, Character &ch) {
    auto &t = c.t();
    t.gotoXY(0, 18); t.putChar(11);
    t.write("USE ITEM (0=EXIT) ? >");
    int k = c.ui.getKey();
    if (c.ui.quit()) return;
    int n = k - '0';
    if (n < 1 || n > ch.possCount) return;
    Possession &p = ch.poss[n - 1];
    ObjectRec o{c.sc.record(Scenario::Object, p.itemIndex)};
    if (o.spellPwr() == 0) { c.ui.pressAnyKey("** POWERLESS **"); return; }
    if (o.type() != ObjType::Special && !p.equipped) { c.ui.pressAnyKey("** NOT EQUIPPED **"); return; }
    if (c.rng.mod(100) < o.chgChance()) p.itemIndex = o.changeTo();   // item transforms
    campCast(c, ch, o.spellPwr());                        // SPELLPWR -> spell number
}

// ---- IDITEM (P010108): a Bishop identifies one packed item --------
// Dispatched from camp I)DENT (P010C15): CLASS must be BISHOP, then the
// work runs in UTILITIE's XCAMPSTF screen.
void doIdentify(CampCtx &c, Character &ch) {
    if (ch.cls != Class::Bishop) { c.ui.pressAnyKey("** NOT BISHOP **"); return; }
    auto &t = c.t();
    int n;
    for (;;) {
        t.gotoXY(0, 18); t.putChar(11);
        t.write("IDENTIFY WHAT ITEM (0=EXIT) ? >");
        int k = c.ui.getKey();
        if (c.ui.quit()) return;
        n = k - '0';
        if (n == 0) return;
        if (n >= 1 && n <= ch.possCount) break;
    }
    Possession &p = ch.poss[n - 1];
    if (p.identified) { c.ui.pressAnyKey("** ALREADY IDENTIFIED **"); return; }

    p.identified = c.rng.mod(100) < (10 + 5 * ch.charLevel);
    std::printf("IDENT| %s -> %s (%s)\n", ch.name.c_str(),
                objName(c, p.itemIndex, p.identified).c_str(),
                p.identified ? "SUCCESS" : "FAILURE");
    c.ui.pressAnyKey(p.identified ? "SUCCESS!" : "FAILURE");

    // Curse backfire: the item's true CURSED flag bites, and a cursed item
    // sticks to the hand (DOS falls through to the equipment display).
    if (c.rng.mod(100) < (35 - 3 * ch.charLevel)) {
        ObjectRec o{c.sc.record(Scenario::Object, p.itemIndex)};
        p.cursed = o.cursed();
        if (p.cursed) {
            p.equipped = true;
            equipRecalc(ch, c.sc);
            std::printf("IDENT| %s CURSED by the identify!\n", ch.name.c_str());
            c.ui.pressAnyKey("** CURSED! **");
        }
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
        if (k == 'E') { runEquip(c, ch); continue; }
        if (k == 'T') { doTrade(c, idx); continue; }
        if (ok && k == 'S') { campCast(c, ch, 0); if (c.done) return true; continue; }
        if (ok && k == 'U') { doUse(c, ch); if (c.done) return true; continue; }
        if (ok && k == 'I') { doIdentify(c, ch); continue; }
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

CampExit runCamp(Ui &ui, Party &party, Roster &roster, const Scenario &sc,
                 const StringPool *sp, Rng &rng, MazeState &st) {
    CampCtx c{ui, party, roster, sc, sp, rng, st};
    for (;;) {
        campList(c);
        int k = ui.getKey();
        if (ui.quit()) return CampExit::WindowClosed;
        if (k == 'L') return CampExit::ToMaze;
        if (k >= '1' && k <= '0' + party.count()) {
            if (!inspectChar(c, k - '1')) return CampExit::WindowClosed;
            if (c.done) return c.exit;          // MALOR teleported the party
            continue;
        }
        if (k == 'R') { reorder(c); continue; }
        if (k == 'E') {                          // EQUIP1(-1): every member
            for (int i = 0; i < party.count(); ++i) runEquip(c, party.member(i), false);
            continue;
        }
        if (k == 'D') {
            campList(c);
            c.t().gotoXY(0, 18); c.t().putChar(11);
            c.t().write("DISBAND THE PARTY?  CONFIRM (Y/N) ?");
            if (ui.menu("YN") != 'Y') continue;
            c.t().gotoXY(0, 19); c.t().write("RE-CONFIRM (Y/N) ?");
            if (ui.menu("YN") != 'Y') continue;
            // DISBAND: leave every member as a body in the current room.
            for (int i = 0; i < party.count(); ++i) {
                Character &m = party.member(i);
                m.inMaze = false;
                m.lostX = st.pos.x; m.lostY = st.pos.y; m.lostLevel = st.level;
                m.age += 25;
            }
            party.disband(roster);          // persist the bodies + empty the party
            return CampExit::Disbanded;
        }
    }
}

} // namespace wiz
