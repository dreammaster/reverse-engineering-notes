#include "wiz/town_ui.h"
#include "wiz/scenario.h"
#include "wiz/roster.h"
#include "wiz/string_pool.h"
#include "wiz/inn.h"
#include "wiz/temple.h"

#include <cstdio>
#include <string>

namespace wiz {
namespace {

const char *raceName(const Scenario &sc, Race r) {
    return int(r) < int(sc.races().size()) ? sc.races()[int(r)].c_str() : "?";
}
const char *className(const Scenario &sc, Class c) {
    return int(c) < int(sc.classes().size()) ? sc.classes()[int(c)].c_str() : "?";
}
const char *alignName(const Scenario &sc, Align a) {
    return int(a) < int(sc.aligns().size()) ? sc.aligns()[int(a)].c_str() : "?";
}
const char *statusName(const Scenario &sc, Status s) {
    return int(s) < int(sc.statuses().size()) ? sc.statuses()[int(s)].c_str() : "?";
}

const std::string kBorder     = "+--------------------------------------+";
const std::string kPartyBorder = "+----------- CURRENT PARTY: -----------+";

struct TownCtx {
    Ui &ui;
    Party &party;
    Roster &roster;
    Shop &shop;
    const Scenario &sc;
    const StringPool *sp;
    Rng &rng;
    const std::string &rosterPath;
    const std::string &partyPath;
    const std::string &shopPath;

    TextScreen &ts() { return ui.ts(); }
    void save() {
        if (!rosterPath.empty()) roster.save(rosterPath);
        if (!partyPath.empty()) party.save(partyPath);
        if (!shopPath.empty()) shop.save(shopPath);
    }

    // objectNameKey(idx, field): field 1 = identified, 0 = unidentified.
    std::string itemName(int idx, bool identified) const {
        if (sp) {
            bool ok = false;
            std::string s = sp->get(StringPool::objectNameKey(idx, identified ? 1 : 0), &ok);
            if (ok && !s.empty()) return s;
        }
        return "ITEM " + std::to_string(idx);
    }
};

// DSPTITLE -- row 1 banner.
void dspTitle(TownCtx &c, const std::string &title) {
    auto &t = c.ts();
    t.gotoXY(0, 1);
    t.write("! CASTLE");
    t.writeField(title, 30);
    t.write(" !");
}

// CHARINFO -- one party row at y = 5 + x.
void charInfo(TownCtx &c, int x) {
    auto &t = c.ts();
    const Character &ch = c.party.member(x);
    t.gotoXY(0, 5 + x);
    t.putChar(29);                                   // clear to end of line
    t.writeField(std::to_string(x + 1), 2);
    t.write(" ");
    t.write(ch.name);
    t.gotoXY(19, 5 + x);
    t.write(std::string(1, alignName(c.sc, ch.align)[0]));
    t.write("-");
    t.write(std::string(className(c.sc, ch.cls)).substr(0, 3));
    t.write(" ");
    if (ch.armorClass > -10) t.writeField(std::to_string(ch.armorClass), 2);
    else                     t.write("LO");
    t.writeField(std::to_string(ch.hpLeft), 5);
    t.write(" ");
    if (ch.status == Status::OK) t.writeField(std::to_string(ch.hpMax), 4);
    else                         t.write(statusName(c.sc, ch.status));
}

// DSPPARTY -- the box across the top of the screen (rows 0..11).
void dspParty(TownCtx &c, const std::string &title) {
    auto &t = c.ts();
    t.resetWindow();
    t.putChar(12);
    t.gotoXY(0, 0); t.write(kBorder);
    dspTitle(c, title);                              // row 1
    t.gotoXY(0, 2); t.write(kPartyBorder);
    t.gotoXY(0, 4); t.write(" # CHARACTER NAME  CLASS AC HITS STATUS");
    for (int i = 0; i < c.party.count(); ++i) charInfo(c, i);   // rows 5..10
    t.gotoXY(0, 11); t.write(kBorder);
    t.gotoXY(0, 13); t.putChar(11);                  // clear the menu area
}

// GETCHARX -- pick a party member by number; -1 on RETURN.
int getCharX(TownCtx &c, bool dspNames, const std::string &solicit) {
    auto &t = c.ts();
    t.setWindow(0, 0, 40, 24);
    t.gotoXY(0, 18); t.putChar(11);
    if (dspNames)
        for (int i = 0; i < c.party.count(); ++i) {
            t.gotoXY(20 * (i % 2), 20 + i / 2);
            char b[40];
            std::snprintf(b, sizeof b, "%d) %s", i + 1, c.party.member(i).name.c_str());
            t.write(b);
        }
    for (;;) {
        t.gotoXY(0, 18); t.putChar(29);
        t.write(solicit); t.write(" ([RETURN] EXITS) >");
        int k = c.ui.getKey();
        if (c.ui.quit()) return -1;
        if (k == KEY_RETURN) return -1;
        int n = k - '0';
        if (n >= 1 && n <= c.party.count()) return n - 1;
    }
}

// A compact character sheet -- Gilgamesh "#) SEE A MEMBER".
void seeMember(TownCtx &c, const Character &ch) {
    auto &t = c.ts();
    t.resetWindow();
    t.putChar(12);
    char b[48];
    t.gotoXY(0, 0); t.writeField("NAME ", 10); t.write(ch.name);
    t.gotoXY(0, 1); t.writeField("RACE", 9);  t.write(std::string(" ") + raceName(c.sc, ch.race));
    t.gotoXY(0, 2); t.writeField("CLASS", 9); t.write(std::string(" ") + className(c.sc, ch.cls));
    t.gotoXY(0, 3); t.writeField("ALIGN", 9); t.write(std::string(" ") + alignName(c.sc, ch.align));
    t.gotoXY(0, 4); t.writeField("LEVEL", 9);
    std::snprintf(b, sizeof b, " %d", ch.charLevel); t.write(b);
    static const char *kAttr[6] = {"STRENGTH", "I.Q.", "PIETY", "VITALITY", "AGILITY", "LUCK"};
    for (int i = 0; i < 6; ++i) {
        t.gotoXY(0, 6 + i); t.writeField(kAttr[i], 9);
        std::snprintf(b, sizeof b, " %2d", ch.attrib[i]); t.write(b);
    }
    t.gotoXY(0, 13); t.writeField("HITS", 9);
    std::snprintf(b, sizeof b, " %d/%d", ch.hpLeft, ch.hpMax); t.write(b);
    t.gotoXY(0, 14); t.writeField("A.C.", 9);
    std::snprintf(b, sizeof b, " %d", ch.armorClass); t.write(b);
    t.gotoXY(0, 15); t.writeField("GOLD", 9);
    std::snprintf(b, sizeof b, " %lld", (long long)ch.gold.v); t.write(b);
    t.gotoXY(0, 22);
    c.ui.pressAnyKey();
}

// centered one-liner in the menu area, then wait.
void notice(TownCtx &c, const std::string &msg) {
    auto &t = c.ts();
    t.setWindow(0, 13, 40, 11);
    t.putChar(11);
    t.writeCentered(msg, 2);
    t.resetWindow();
    t.gotoXY(0, 22);
    c.ui.pressAnyKey();
}

// ADDPARTY (P010A09).
void addParty(TownCtx &c) {
    auto &t = c.ts();
    t.setWindow(0, 0, 40, 24);
    t.gotoXY(0, 19); t.write("WHO WILL JOIN ? >");
    std::string who = c.ui.getLine(15);
    if (c.ui.quit() || who.empty()) return;

    int slot = c.roster.findByName(who);
    if (slot < 0) { notice(c, "** WHO? **"); return; }
    const Character &rc = c.roster.slot(slot);
    if (rc.inMaze)  { notice(c, "** OUT **"); return; }

    Align pa = c.party.align();
    if (pa != Align::Neutral && rc.align != Align::Neutral && rc.align != pa) {
        notice(c, "** BAD ALIGNMENT **");
        return;
    }

    t.gotoXY(0, 20); t.write("ENTER PASSWORD  >");
    std::string pw = c.ui.getPass(15, c.rng);
    if (c.ui.quit()) return;
    t.gotoXY(0, 21);
    if (pw != rc.password) { notice(c, "** THATS NOT IT **"); return; }

    c.party.add(c.roster, slot);
    c.save();
    charInfo(c, c.party.count() - 1);
}

// REMOVE (P010A0B).
void removeMember(TownCtx &c) {
    int i = getCharX(c, false, "WHO WILL LEAVE");
    if (i < 0) return;
    c.party.remove(c.roster, i);
    c.save();
}

// GILGMENU (P010A08).
void gilgMenu(TownCtx &c) {
    auto &t = c.ts();
    t.setWindow(0, 0, 40, 24);
    t.gotoXY(0, 13); t.putChar(11);
    t.write("YOU MAY ");
    if (!c.party.full()) {
        t.write("A)DD A MEMBER");
        t.writeln(c.party.empty() ? "" : ",");
        t.write("        ");
    }
    if (!c.party.empty()) {
        t.writeln("R)EMOVE A MEMBER,");
        t.write("        ");
        t.writeln("#) SEE A MEMBER,");
    } else {
        t.writeln(""); t.writeln("");
    }
    t.writeln("");
    t.writeln("OR PRESS [RETURN] TO LEAVE");
}

// GILGAMSH (P010A06).
void gilgamesh(TownCtx &c) {
    dspTitle(c, "TAVERN");
    for (;;) {
        gilgMenu(c);
        int k = c.ui.getKey();
        if (c.ui.quit() || k == KEY_RETURN) return;
        if (k == 'A' && !c.party.full()) addParty(c);
        else if (k == 'R' && !c.party.empty()) removeMember(c);
        else if (k >= '1' && k <= '6' && !c.party.empty()) {
            int i = k - '1';
            if (i < c.party.count()) seeMember(c, c.party.member(i));
        }
        dspParty(c, "TAVERN");
        dspTitle(c, "TAVERN");
    }
}

// The Castle hub menu text (P010A26).
void hubMenu(TownCtx &c) {
    auto &t = c.ts();
    t.setWindow(0, 0, 40, 24);
    t.gotoXY(0, 13); t.putChar(11);
    t.gotoXY(13, 13); t.writeln("YOU MAY GO TO:");
    t.writeln("");
    t.writeln("THE A)DVENTURER'S INN, G)ILGAMESH'");
    t.writeln("TAVERN, B)OLTAC'S TRADING POST, THE");
    t.writeln("TEMPLE OF C)ANT, OR THE E)DGE OF TOWN.");
}

// EDGETOWN (P01021A, in SHOPS).  Returns the town's next destination, or
// nullopt to go back to the Castle hub.
bool edgeOfTown(TownCtx &c, TownExit &out) {
    dspParty(c, "EDGE OF TOWN");
    auto &t = c.ts();
    t.setWindow(0, 0, 40, 24);
    t.gotoXY(0, 13); t.putChar(11);
    if (c.party.empty()) {
        t.writeln("YOU MAY GO TO THE T)RAINING GROUNDS,");
        t.writeln("RETURN TO THE C)ASTLE, OR L)EAVE THE");
        t.writeln("GAME.");
    } else {
        t.writeln("YOU MAY ENTER THE M)AZE, THE T)RAINING");
        t.writeln("GROUNDS, C)ASTLE,  OR L)EAVE THE GAME.");
    }
    for (;;) {
        int k = c.ui.getKey();
        if (c.ui.quit()) { out = TownExit::WindowClosed; return true; }
        if (k == 'M' && !c.party.empty()) {
            t.gotoXY(0, 13); t.putChar(11);
            t.writeCentered("ENTERING", 0);
            t.writeCentered(c.sc.gameName(), 1);
            out = TownExit::ToMaze;
            return true;
        }
        if (k == 'T') { c.party.disband(c.roster); c.save(); out = TownExit::ToRoller; return true; }
        if (k == 'L') { c.party.disband(c.roster); c.save(); out = TownExit::LeaveGame; return true; }
        if (k == 'C') return false;                   // back to the hub
    }
}

// ---- Adventurer's Inn (ADVNTINN P010A0F) --------------------------------

// INNMENU (P010A11): the room list.
void innMenu(TownCtx &c, const Character &ch) {
    auto &t = c.ts();
    t.setWindow(0, 0, 40, 24);
    t.gotoXY(0, 13); t.putChar(11);
    t.write("   WELCOME ");
    t.write(ch.name);
    t.writeln(". WE HAVE:");
    t.writeln("");
    for (int i = 0; i < 5; ++i) {
        char b[64];
        std::snprintf(b, sizeof b, "[%c] %s", 'A' + i, kRooms[i].name);
        t.writeln(b);
    }
    t.write("    OR [RETURN] TO LEAVE");
}

// HEALHP one week's frame.
void healFrame(TownCtx &c, const Character &ch) {
    auto &t = c.ts();
    t.setWindow(0, 0, 40, 24);
    t.gotoXY(0, 13); t.putChar(11);
    t.write(ch.name); t.writeln(" IS HEALING UP");
    t.writeln("");
    char b[48];
    std::snprintf(b, sizeof b, "         HIT POINTS (%d/%d)", ch.hpLeft, ch.hpMax);
    t.writeln(b);
    t.writeln("");
    std::snprintf(b, sizeof b, "               GOLD  %lld", (long long)ch.gold.v);
    t.write(b);
}

// TAKENAP (P010A23): rest in `room`, then check for a level and refill spells.
void takeNap(TownCtx &c, int partyX, int room) {
    Character &ch = c.party.member(partyX);
    const RoomTier &rt = kRooms[room];
    InnLog log;
    auto &t = c.ts();
    t.setWindow(0, 0, 40, 24);
    t.gotoXY(0, 13); t.putChar(11);

    if (rt.hpPerWeek > 0) {
        while (ch.gold.v >= rt.goldPerWeek && ch.hpLeft < ch.hpMax) {
            ch.hpLeft += rt.hpPerWeek;
            if (ch.hpLeft > ch.hpMax) ch.hpLeft = ch.hpMax;
            ch.gold.v -= rt.goldPerWeek;
            ch.age += 1;                          // DOS HEALHP ages a week
            healFrame(c, ch);
            c.ui.refresh();
            if (c.ui.pollKey() != KEY_NONE) break;   // KEYAVAIL -> stop resting
            c.ui.delayMs(60);
        }
    } else {
        t.write(ch.name); t.write(" IS NAPPING");
    }

    ExpTable exp{c.sc.record(Scenario::Exp, 0)};
    checkNewLevel(ch, exp, c.rng, log);
    setSpells(ch);                                // resting refills spell slots

    t.gotoXY(0, 13); t.putChar(11);
    int row = 0;
    for (const auto &m : log) { t.gotoXY(0, 13 + row++); t.write(m); }
    t.gotoXY(0, 23); t.write("PRESS [RETURN] TO LEAVE");
    for (;;) {
        int k = c.ui.getKey();
        if (c.ui.quit() || k == KEY_RETURN) break;
    }
    c.save();
}

void advntInn(TownCtx &c) {
    for (;;) {                                    // REPEAT GETWHO ... UNTIL FALSE
        dspParty(c, "INN");
        dspTitle(c, "INN");
        int partyX = getCharX(c, false, "WHO WILL STAY");
        if (c.ui.quit() || partyX < 0) return;
        Character &ch = c.party.member(partyX);
        if (ch.status != Status::OK) continue;

        int k = 0;
        do {
            innMenu(c, ch);
            k = c.ui.getKey();
            if (c.ui.quit()) return;
            if (k == KEY_RETURN) break;
            int room = k - 'A';
            if (room >= 0 && room < 5) takeNap(c, partyX, room);
            dspParty(c, "INN");
            dspTitle(c, "INN");
        } while (k != KEY_RETURN && ch.status == Status::OK);
    }
}

// ---- Boltac's Trading Post (BOLTAC P01020A) -----------------------------

enum ShopAction { SA_SELL = 0, SA_UNCURSE = 1, SA_IDENTIFY = 2 };

// AASTRAA / CENTSTR: a centred one-liner in the menu area, then a key.
void aastraa(TownCtx &c, const std::string &msg) {
    auto &t = c.ts();
    t.setWindow(0, 13, 40, 11);
    t.putChar(11);
    t.writeCentered(msg, 4);
    t.resetWindow();
    t.gotoXY(0, 23);
    c.ui.pressAnyKey();
}

// Fill objList[0..5] with the next / previous 6 shelvable object indices,
// wrapping 1..count-1 and skipping anything not on the shelf (SCROLPOS/NEG).
bool scrollPage(TownCtx &c, int objList[6], bool forward) {
    int rec = c.sc.count(Scenario::Object);
    int onShelf = 0;
    for (int i = 1; i < rec; ++i) if (c.shop.onShelf(c.sc, i)) ++onShelf;
    if (onShelf == 0) return false;

    if (forward) {
        int inv = objList[5] - 1;
        for (int x = 0; x < 6; ++x) {
            do { if (++inv >= rec) inv = 1; } while (!c.shop.onShelf(c.sc, inv));
            objList[x] = inv;
        }
    } else {
        int inv = objList[0] + 1;
        for (int x = 5; x >= 0; --x) {
            do { if (--inv < 1) inv = rec - 1; } while (!c.shop.onShelf(c.sc, inv));
            objList[x] = inv;
        }
    }
    return true;
}

void drawShopPage(TownCtx &c, const int objList[6], const Character &ch) {
    auto &t = c.ts();
    for (int x = 0; x < 6; ++x) {
        int obj = objList[x];
        ObjectRec o{c.sc.record(Scenario::Object, obj)};
        t.gotoXY(0, 13 + x + 1);
        t.putChar(29);
        char b[64];
        std::snprintf(b, sizeof b, "%d)%-15s %lld", x + 1,
                      c.itemName(obj, true).substr(0, 15).c_str(),
                      (long long)o.price().value());
        t.write(b);
        if (!o.classUse(int(ch.cls))) t.write(" UNUSABLE");
    }
}

// PURCHASE (P01020F).
void purchase(TownCtx &c, int objList[6], int charIdx) {
    auto &t = c.ts();
    Character &ch = c.party.member(charIdx);
    int buyX;
    for (;;) {
        t.setWindow(0, 0, 40, 24);
        t.gotoXY(0, 21); t.putChar(11);
        t.write("PURCHASE WHICH ITEM ([RETURN] EXITS) ? >");
        int k = c.ui.getKey();
        if (c.ui.quit() || k == KEY_RETURN) return;
        buyX = k - '1';
        if (buyX >= 0 && buyX < 6) break;
    }
    int obj = objList[buyX];
    ObjectRec o{c.sc.record(Scenario::Object, obj)};

    if (c.shop.stock(obj) == 0)       { aastraa(c, "** YOU BOUGHT THE LAST ONE **"); return; }
    if (ch.possCount == 8)            { aastraa(c, "** YOU CANT CARRY ANYTHING MORE **"); return; }
    if (ch.gold.v < o.price().value()){ aastraa(c, "** YOU CANNOT AFFORD IT **"); return; }

    bool confirmedUnusable = false;
    if (!o.classUse(int(ch.cls))) {
        t.setWindow(0, 0, 40, 24);
        t.gotoXY(0, 22); t.putChar(11);
        t.write("UNUSABLE ITEM - CONFIRM BUY (Y/N) ? >");
        int k = c.ui.menu("YN");
        if (c.ui.quit() || k == 'N') { aastraa(c, "** WE ALL MAKE MISTAKES **"); return; }
        confirmedUnusable = true;
    }

    ch.gold.v -= o.price().value();
    ch.poss[ch.possCount] = Possession{false, false, true, obj};
    ch.possCount += 1;
    int n = c.shop.stock(obj);
    if (n > 0) c.shop.setStock(obj, n - 1);
    c.save();
    aastraa(c, confirmedUnusable ? "** ITS YOUR MONEY **" : "** JUST WHAT YOU NEEDED **");
}

// DOBUY (P01020C).
void doBuy(TownCtx &c, int charIdx) {
    auto &t = c.ts();
    const Character &ch = c.party.member(charIdx);
    int objList[6] = {1, 0, 0, 0, 0, 1};
    bool reScroll = true, forward = true;

    for (;;) {
        if (reScroll) {
            if (!scrollPage(c, objList, forward)) { aastraa(c, "** BOLTAC HAS NOTHING TO SELL **"); return; }
        }
        reScroll = true;
        forward = true;

        t.setWindow(0, 0, 40, 24);
        t.gotoXY(0, 13); t.putChar(11);
        drawShopPage(c, objList, ch);
        t.gotoXY(0, 20); t.putChar(11);
        char b[48];
        std::snprintf(b, sizeof b, "YOU HAVE %lld GOLD", (long long)ch.gold.v);
        t.writeln(b);
        t.writeln("YOU MAY P)URCHASE, SCROLL");
        t.writeln("        F)ORWARD OR B)ACK, GO TO THE");
        t.write("        S)TART, OR L)EAVE");

        int k = c.ui.menu("PFBSL");
        if (c.ui.quit() || k == 'L') return;
        if (k == 'P') { purchase(c, objList, charIdx); reScroll = false; }
        else if (k == 'S') { objList[5] = 1; }
        else if (k == 'B') { forward = false; }
        // 'F' -> reScroll stays true, forward stays true
    }
}

// LISTPOSS (P010212): the character's carried items with Boltac's prices.
void listPoss(TownCtx &c, int charIdx, int action) {
    auto &t = c.ts();
    const Character &ch = c.party.member(charIdx);
    t.setWindow(0, 0, 40, 24);
    t.gotoXY(0, 13); t.putChar(11);
    for (int i = 0; i < ch.possCount; ++i) {
        const Possession &p = ch.poss[i];
        ObjectRec o{c.sc.record(Scenario::Object, p.itemIndex)};
        int64_t price = o.price().value() / 2;
        if (action == SA_SELL && !p.identified) price = 1;
        char b[64];
        std::snprintf(b, sizeof b, "%d)%-15s %lld", i + 1,
                      c.itemName(p.itemIndex, p.identified).substr(0, 15).c_str(),
                      (long long)price);
        t.gotoXY(0, 13 + i); t.write(b);
    }
}

// TRANSACT (P010213).
void transact(TownCtx &c, int charIdx, int x, int action) {
    Character &ch = c.party.member(charIdx);
    Possession &p = ch.poss[x];
    int obj = p.itemIndex;
    ObjectRec o{c.sc.record(Scenario::Object, obj)};
    int64_t price = o.price().value() / 2;

    if (action == SA_SELL) {
        if (!p.identified) price = 1;
        if (p.cursed) { aastraa(c, "** WE DONT BUY CURSED ITEMS **"); return; }
    } else {
        if (!p.cursed && action == SA_UNCURSE) { aastraa(c, "** THAT IS NOT A CURSED ITEM **"); return; }
        if (p.identified && action == SA_IDENTIFY) { aastraa(c, "** THAT HAS BEEN IDENTIFIED **"); return; }
        if (ch.gold.v < price) { aastraa(c, "** YOU CANT AFFORD THE FEE **"); return; }
    }

    if (action == SA_SELL) ch.gold.v += price;
    else                   ch.gold.v -= price;

    if (action == SA_IDENTIFY) {
        p.identified = true;
    } else {                                   // SELL or UNCURSE: lose the item
        for (int j = x + 1; j < ch.possCount; ++j) ch.poss[j - 1] = ch.poss[j];
        ch.possCount -= 1;
        if (action == SA_SELL) {
            int n = c.shop.stock(obj);
            if (n > -1) c.shop.setStock(obj, n + 1);
        }
    }
    c.save();
    aastraa(c, "** ANYTHING ELSE, SIRE? **");
}

// SELLIDUN (P010211).
void sellIdun(TownCtx &c, int charIdx, int action) {
    auto &t = c.ts();
    const Character &ch = c.party.member(charIdx);
    for (;;) {
        listPoss(c, charIdx, action);
        if (ch.possCount == 0) return;
        t.setWindow(0, 0, 40, 24);
        t.gotoXY(0, 22); t.putChar(11);
        t.write(action == SA_SELL    ? "WHICH DO YOU WISH TO SELL ? >"
              : action == SA_UNCURSE ? "WHICH DO YOU WISH UNCURSED ? >"
                                     : "WHICH DO YOU WISH IDENTIFIED ? >");
        int k = c.ui.getKey();
        if (c.ui.quit() || k == KEY_RETURN) return;
        int x = k - '1';
        if (x >= 0 && x < ch.possCount) transact(c, charIdx, x, action);
    }
}

// DOPLAYER (P01020B).
void doPlayer(TownCtx &c, int charIdx) {
    auto &t = c.ts();
    const Character &ch = c.party.member(charIdx);
    for (;;) {
        t.setWindow(0, 0, 40, 24);
        t.gotoXY(0, 13); t.putChar(11);
        char b[48];
        std::snprintf(b, sizeof b, "      WELCOME %s", ch.name.c_str());
        t.writeln(b);
        std::snprintf(b, sizeof b, "     YOU HAVE %lld GOLD", (long long)ch.gold.v);
        t.writeln(b);
        t.writeln("");
        t.writeln("YOU MAY B)UY  AN ITEM,");
        t.writeln("        S)ELL AN ITEM, HAVE AN ITEM");
        t.writeln("        U)NCURSED,  OR HAVE AN ITEM");
        t.write("        I)DENTIFIED, OR L)EAVE");
        int k = c.ui.menu("BSUIL");
        if (c.ui.quit() || k == 'L') return;
        if (k == 'B') doBuy(c, charIdx);
        else if (k == 'S') sellIdun(c, charIdx, SA_SELL);
        else if (k == 'U') sellIdun(c, charIdx, SA_UNCURSE);
        else if (k == 'I') sellIdun(c, charIdx, SA_IDENTIFY);
    }
}

// BOLTAC (P01020A).
void boltac(TownCtx &c) {
    for (;;) {
        dspParty(c, "SHOP");
        dspTitle(c, "SHOP");
        auto &t = c.ts();
        t.setWindow(0, 0, 40, 24);
        t.gotoXY(0, 13); t.putChar(11);
        t.writeln("       WELCOME TO THE TRADING POST");
        int charIdx = getCharX(c, false, "WHO WILL ENTER");
        if (c.ui.quit() || charIdx < 0) return;
        doPlayer(c, charIdx);
    }
}

// ---- Temple of Radiant Cant (CANT P010202) ------------------------------

// WELCOME (P010205 / DOS proc 6+7): list the roster characters the Temple can
// treat (present, status PLYZE / STONED / DEAD / ASHES) and pick one.
// Returns the roster slot, or -1 to leave the Temple.
int templeWelcome(TownCtx &c) {
    auto &t = c.ts();
    t.setWindow(0, 0, 40, 24);
    t.gotoXY(0, 13); t.putChar(11);
    t.writeln(" WELCOME TO THE TEMPLE OF RADIANT CANT!");

    int slot[26], n = 0;
    for (int i = 0; i < c.roster.count() && n < 20; ++i) {
        const Character &r = c.roster.slot(i);
        if (r.name.empty() || r.inMaze || !templeTreatable(r.status)) continue;
        slot[n] = i;
        t.gotoXY(20 * (n % 2), 15 + n / 2);
        char b[40];
        std::snprintf(b, sizeof b, "%c) %-9s %s", 'A' + n, r.name.substr(0, 9).c_str(),
                      statusName(c.sc, r.status));
        t.write(b);
        ++n;
    }
    t.gotoXY(0, 22);
    t.write("WHO ARE YOU HELPING ([RETURN] EXITS) ? >");
    for (;;) {
        int k = c.ui.getKey();
        if (c.ui.quit() || k == KEY_RETURN) return -1;
        int p = k - 'A';
        if (p >= 0 && p < n) return slot[p];
    }
}

// PAYCANT (P010206) -> GETPAYER (P010207) + DOCANT (P010208), for the roster
// character in `patientSlot`.
void cantShop(TownCtx &c, int patientSlot) {
    auto &t = c.ts();
    Character &who = c.roster.slot(patientSlot);

    // GETPAYER
    int64_t fee = templeFee(who.status, who.charLevel);
    t.setWindow(0, 0, 40, 24);
    t.gotoXY(0, 17); t.putChar(11);
    char b[48];
    std::snprintf(b, sizeof b, "THE DONATION WILL BE %lld", (long long)fee);
    t.writeln(b);
    int payer = getCharX(c, false, "WHO WILL TITHE");
    if (c.ui.quit() || payer < 0) return;                     // EXIT(CANTSHOP)
    Character &pc = c.party.member(payer);
    if (pc.gold.v < fee) { aastraa(c, "** CHEAP APOSTATES! OUT! **"); return; }
    pc.gold.v -= fee;

    // DOCANT
    t.setWindow(0, 0, 40, 24);
    t.gotoXY(0, 17); t.putChar(11);
    for (const char *step : {"MURMUR - ", "CHANT - ", "PRAY - "}) {
        t.write(step); c.ui.refresh(); c.ui.delayMs(350);
    }
    t.writeln("INVOKE!");

    InnLog log;
    doCant(who, c.rng, log);
    c.party.resyncFromRoster(c.roster);
    c.save();

    t.gotoXY(0, 19);
    for (const auto &m : log) t.writeln(m);
    aastraa(c, "** " + (log.empty() ? std::string("...") : log.back()) + " **");
}

// CANT (P010202): REPEAT { WELCOME; CANTSHOP } -- the only exit is [RETURN] at
// WELCOME.
void temple(TownCtx &c) {
    for (;;) {
        dspParty(c, "TEMPLE");
        dspTitle(c, "TEMPLE");
        int patientSlot = templeWelcome(c);
        if (patientSlot < 0) return;
        cantShop(c, patientSlot);
    }
}

} // namespace

TownExit runTown(Ui &ui, TownWorld &w) {
    TownCtx c{ui, w.party, w.roster, w.shop, w.sc, w.sp, w.rng,
              w.rosterPath, w.partyPath, w.shopPath};

    for (;;) {
        dspParty(c, "MARKET");
        hubMenu(c);

        int k = 0;
        for (;;) {
            k = ui.getKey();
            if (ui.quit()) return TownExit::WindowClosed;
            bool valid = k == 'A' || k == 'G' || k == 'B' || k == 'C' || k == 'E';
            if (!valid) continue;
            if (c.party.count() > 0 || k == 'E' || k == 'G') break;
        }

        if (k == 'G') {
            gilgamesh(c);
        } else if (k == 'A') {
            advntInn(c);
        } else if (k == 'B') {
            boltac(c);
        } else if (k == 'C') {
            temple(c);
        } else if (k == 'E') {
            TownExit out;
            if (edgeOfTown(c, out)) return out;
        }
    }
}

} // namespace wiz
