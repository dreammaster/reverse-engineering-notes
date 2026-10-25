#include "wiz/castle.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace wiz {

void startupResetInMaze(Roster &roster) {                           // KANJIREA:37
    for (int i = 0; i < roster.count(); ++i) {
        Character &c = roster.slot(i);
        if (!c.inMaze) continue;
        c.inMaze = false;
        if (c.lostLevel == 0) c.lostY = 0;
    }
}

int Castle::rnd(int n) {                                            // RANDOM MOD n
    u16 r = w_.rng.next();
    int v = n > 0 ? int(r % u16(n)) : 0;
    if (std::getenv("WIZ1_RNGLOG")) std::fprintf(stderr, "rng %04x mod %d = %d\n", r, n, v);
    return v;
}

// ---- helpers -----------------------------------------------------------------
static const long long kMaxGold = 999999999999LL;   // ADDLONGS caps every limb at 9999
static const int kRosterWinY = 16;            // HIRES400 (LINES.24) would use 15

int Castle::partyAlign() const {                                   // CASTLE:9 (GETALIGN)
    int a = 2;
    for (int i = 0; i < w_.party.count(); ++i)
        if (int(w_.party.member(i).align) != 2) a = int(w_.party.member(i).align);
    return a;
}

void Castle::save() {
    if (!w_.rosterPath.empty()) w_.roster.save(w_.rosterPath);
    if (!w_.partyPath.empty()) w_.party.save(w_.partyPath);
}

void Castle::closeWindows() {
    if (roster_) d_.con.close(roster_, false);
    if (title_) d_.con.close(title_, false);
}

// ---- the roster window (CASTLE:3, 6, 7) ---------------------------------------------
void Castle::charRow(int i) {                                      // CASTLE:3
    const Character &c = w_.party.member(i);
    Win *w = roster_;
    d_.gotoXY(w, 0, i + 1);
    d_.putNum(w, i + 1, 1);
    d_.putC(w, ' ');
    d_.putS(w, c.name);
    while (w->cx < 18) d_.putC(w, ' ');
    d_.putS(w, d_.str(10009 + int(c.align)).substr(0, 1));
    d_.putC(w, '-');
    d_.putS(w, d_.str(10030 + int(c.cls)).substr(0, 3));
    d_.putC(w, ' ');
    if (c.armorClass > -10) d_.putNum(w, c.armorClass, 2);
    else d_.putS(w, d_.str(400));
    d_.putNum(w, c.hpLeft, 5);
    d_.putC(w, ' ');
    if (c.status == Status::OK) {
        if (c.poison != 0) d_.putS(w, d_.str(401));
        else d_.putNum(w, c.hpMax, 4);
    } else {
        d_.putS(w, d_.str(10040 + int(c.status)).substr(0, 6));
    }
    while (w->cx != 0) d_.putC(w, ' ');
}

void Castle::header() {                                            // CASTLE:6
    d_.gotoXY(roster_, 0, 0);
    d_.putStr(roster_, 402);
}

void Castle::rows() {                                              // CASTLE:7
    header();
    for (int i = 0; i <= 5; ++i) {
        if (i < w_.party.count()) charRow(i);
        else d_.con.clearRect(roster_, 0, i + 1, 38, 1);
    }
}

// ---- the hub (CASTLE:1) ------------------------------------------------------------------
CastleResult Castle::run(int entry) {
    Console &con = d_.con;
    d_.partyCount = w_.party.count();
    if (d_.window1) con.clear(d_.window1, false);
    if (!roster_) roster_ = con.open(0, kRosterWinY, 40, 9, 3, false);
    if (!title_) title_ = d_.boxMsg(403, 0, 1, 7);
    header();
    if (w_.party.count() > 0) rows();
    rows();

    CastleResult res;
    if (entry == XCASTLE2) {
        res = tavern();
        if (res.xgoto != XCASTLE) return res;
    }
    for (;;) {
        d_.partyCount = w_.party.count();
        std::string spec = d_.str(404) + d_.str(405);
        Win *m = d_.centerBox(d_.menuWidth(spec), 4, 7, 6, true);
        int sel = d_.menu(m, spec);
        con.close(m, false);
        if (d_.quit) { res.windowClosed = true; return res; }
        switch (sel) {
            case 0:
                res = tavern();
                if (res.xgoto != XCASTLE || res.windowClosed) return res;
                break;
            case 1:
                if (w_.party.count() > 0) inn();
                if (d_.quit) { res.windowClosed = true; return res; }
                break;
            case 2:
                if (w_.party.count() > 0) { res.xgoto = XTEMPLE; return res; }
                break;
            case 3:
                if (w_.party.count() > 0) { res.xgoto = XSHOPS; return res; }
                break;
            case 4:
                res.xgoto = XEDGE;
                return res;
        }
    }
}

// ---- Gilgamesh's Tavern (CASTLE:8) -----------------------------------------------------------
CastleResult Castle::tavern() {
    Console &con = d_.con;
    CastleResult res;
    std::string title = d_.str(415), spec = d_.str(416);
    Win *win = d_.centerBox(std::max(Dos::visLen(title) + 1, d_.menuWidth(spec)), 4, 9, 6, false);
    for (;;) {
        d_.partyCount = w_.party.count();
        con.refresh();
        d_.gotoXY(win, 0, 0);
        d_.putStrC(win, 415);
        d_.newline(win);
        int g9 = d_.menuStr(win, 416);
        if (d_.quit) { res.windowClosed = true; return res; }
        if (g9 == 0) {
            if (w_.party.count() < 6) addMember();
        } else if (g9 == 1) {
            if (w_.party.count() > 0) removeMember();
        } else if (g9 >= 2 && g9 <= 7) {
            if (w_.party.count() > 0) {                            // CASTLE:16 -> XGOTO := CAMP
                con.close(win, false);
                res.xgoto = XCAMP;
                res.member = g9 - 2;
                return res;
            }
        } else if (g9 == 8) {
            if (w_.party.count() > 0) { con.clear(win, true); divvyGold(); }
        } else if (g9 == 9) {
            con.close(win, false);
            return res;                                            // EXIT(16:8)
        }
    }
}

// CASTLE:10..14 -- A)DD
void Castle::addMember() {
    Console &con = d_.con;
    Win *prompt = con.open(2, 6, 36, 4, 7, false);                 // BACKGND
    Win *list = con.open(0, 9, 40, 12, 31, true);                  // CASTLE:11
    int slotOf[20];
    std::fill(slotOf, slotOf + 20, -1);
    int pa = partyAlign();
    int n = 0;
    for (int r = 0; r < w_.roster.count(); ++r) {
        const Character &c = w_.roster.slot(r);
        d_.gotoXY(list, 19 * (n / 10), n % 10);
        if (c.status != Status::Lost && !c.inMaze && c.lostLevel == 0 &&
            (int(c.align) == 2 || pa == int(c.align) || pa == 2)) {
            d_.putC(list, 65 + n);
            d_.putS(list, ") ");
            d_.putS(list, c.name);
            slotOf[n] = r;
            ++n;
        }
    }
    auto leave = [&] {                                              // CASTLE:12
        con.close(list, false);
        con.close(prompt, false);
    };
    while (w_.party.count() < 6) {
        con.clear(prompt, true);
        d_.gotoXY(prompt, 0, 0);
        d_.putStrC(prompt, 406);
        d_.getKey();
        if (d_.quit) return;
        if (d_.inchar == 13) break;
        int idx = d_.inchar - 65;
        if (idx < 0 || idx > 19 || slotOf[idx] == -1) continue;
        // CASTLE:13 (EXITADDP)
        int slot = slotOf[idx];
        const Character &c = w_.roster.slot(slot);
        auto err = [&](int strNo) {                                 // CASTLE:14
            con.clear(prompt, true);
            d_.putStrC(prompt, strNo);
            d_.pause(20);
        };
        int pa2 = partyAlign();
        if (pa2 != 2 && int(c.align) != 2 && pa2 != int(c.align)) { err(409); continue; }
        if (!c.password.empty()) {
            d_.gotoXY(list, 19 * (idx / 10), idx % 10);
            d_.putS(list, "-->");
            con.clear(prompt, true);
            d_.gotoXY(prompt, 0, 0);
            d_.putStr(prompt, 410);
            // CASTLE:2 -- password entry, echoing 1-2 'X' per key
            std::string pw;
            for (;;) {
                d_.getKey();
                if (d_.quit) return;
                if (d_.inchar == 13) break;
                if (pw.size() < 15) {
                    int reps = rnd(2);
                    for (int k = 0; k <= reps; ++k)
                        if (prompt->cx < prompt->w - 3) d_.putC(prompt, 'X');
                    pw.push_back(char(d_.inchar));
                }
            }
            d_.newline(prompt);
            if (pw != c.password) {
                d_.gotoXY(list, 19 * (idx / 10), idx % 10);
                d_.putC(list, 65 + idx);
                d_.putS(list, ") ");
                err(411);
                continue;
            }
        }
        w_.party.add(w_.roster, slot);
        save();
        d_.partyCount = w_.party.count();
        d_.con.clearRect(list, 19 * (idx / 10), idx % 10, 19, 1);
        slotOf[idx] = -1;
        charRow(w_.party.count() - 1);
        // (proc 9 recomputes the party alignment -- partyAlign() does it on demand)
    }
    leave();
}

// CASTLE:15 -- R)EMOVE
void Castle::removeMember() {
    Console &con = d_.con;
    Win *w = con.open(8, 7, 24, 4, 7, true);
    d_.putStrC(w, 413);
    d_.putStrC(w, 414);
    int who = d_.getNum(w_.party.count()) - 1;
    if (d_.quit) { con.close(w, false); return; }
    con.close(w, false);
    if (who < 0) return;
    w_.party.remove(w_.roster, who);
    save();
    d_.partyCount = w_.party.count();
    rows();
}

// CASTLE:17 (labelled OLDAGE by the proc-matcher) -- D)IVVY GOLD
void Castle::divvyGold() {
    long long total = 0;
    int n = w_.party.count();
    for (int i = 0; i < n; ++i) total = std::min(kMaxGold, total + w_.party.member(i).gold.v);
    long long share = total / n;
    for (int i = 0; i < n - 1; ++i) {
        w_.party.member(i).gold.v = share;
        total -= share;
    }
    w_.party.member(n - 1).gold.v = total;
    save();
}

// ---- the Adventurer's Inn (CASTLE:20..41) ---------------------------------------------------------
void Castle::innWelcome(Win *win, int m) {                         // CASTLE:22
    const Character &c = w_.party.member(m);
    d_.con.clear(win, true);
    d_.putCentered(win, Dos::substitute(d_.str(420), c.name));
    d_.newline(win);
    for (int n = 422; n <= 427; ++n) {
        if (n == 427) d_.newline(win);
        d_.putStr(win, n);
        d_.newline(win);
    }
    d_.newline(win);
    d_.putS(win, Dos::substitute(d_.str(443), Dos::long2str(c.gold.v)));
}

void Castle::attrMessage(Win *win, int strNo, int attr) {          // CASTLE:38
    std::string s = Dos::substitute(d_.str(strNo), d_.str(10020 + attr));
    d_.putS(win, s);
    d_.newline(win);
}

int Castle::hpRoll(int m) {                                        // CASTLE:32
    Character &c = w_.party.member(m);
    int hp;
    switch (int(c.cls)) {
        case 0: case 6: hp = rnd(10); break;
        case 1:         hp = rnd(4);  break;
        case 2: case 5: hp = rnd(8);  break;
        default:        hp = rnd(6);  break;
    }
    ++hp;
    switch (c.attrib[VIT]) {
        case 3: hp -= 2; break;
        case 4: case 5: hp -= 1; break;
        case 16: hp += 1; break;
        case 17: hp += 2; break;
        case 18: hp += 3; break;
        default: break;
    }
    return hp < 1 ? 1 : hp;
}

// CASTLE:28/29 + 23: recompute the per-level spell counts
static void minCount(const Character &c, int g[8], int group, int lo, int hi) {   // CASTLE:27
    int n = 0;
    for (int s = lo; s <= hi; ++s) n += c.spellKnown[s] ? 1 : 0;
    g[group] = n;
}
static void perLevel(int g[8], int level, int minus, int step) {                   // CASTLE:24
    int cnt = level - minus;
    if (cnt <= 0) return;
    for (int i = 1; i <= 7 && cnt > 0; ++i) {
        if (cnt > g[i]) g[i] = cnt;
        cnt -= step;
    }
    for (int i = 1; i <= 7; ++i)
        if (g[i] > 9) g[i] = 9;
}
void recalcSpellCounts(Character &c) {                              // CASTLE:23
    static const int mg[7][2] = {{1, 4}, {5, 6}, {7, 8}, {9, 11}, {12, 14}, {15, 18}, {19, 21}};
    static const int pg[7][2] = {{22, 26}, {27, 30}, {31, 34}, {35, 38}, {39, 44}, {45, 48}, {49, 50}};
    for (int g = 1; g <= 7; ++g) minCount(c, c.priestSpells, g, pg[g - 1][0], pg[g - 1][1]);
    for (int g = 1; g <= 7; ++g) minCount(c, c.mageSpells, g, mg[g - 1][0], mg[g - 1][1]);
    switch (int(c.cls)) {
        case 1: perLevel(c.mageSpells, c.charLevel, 0, 2); break;
        case 2: perLevel(c.priestSpells, c.charLevel, 0, 2); break;
        case 4: perLevel(c.priestSpells, c.charLevel, 3, 4); perLevel(c.mageSpells, c.charLevel, 0, 4); break;
        case 5: perLevel(c.mageSpells, c.charLevel, 3, 3); break;
        case 6: perLevel(c.priestSpells, c.charLevel, 3, 3); break;
        default: break;
    }
}

void Castle::learnSpells(Win *win, int m, bool &learned) {         // CASTLE:33/34/35/36
    Character &c = w_.party.member(m);
    static const int mg[7][2] = {{1, 4}, {5, 6}, {7, 8}, {9, 11}, {12, 14}, {15, 18}, {19, 21}};
    static const int pg[7][2] = {{22, 26}, {27, 30}, {31, 34}, {35, 38}, {39, 44}, {45, 48}, {49, 50}};
    auto tryRange = [&](int lo, int hi, int attr) {                 // CASTLE:34
        bool any = false;
        for (int s = lo; s <= hi; ++s) any = any || c.spellKnown[s];
        for (int s = lo; s <= hi; ++s)
            if (!c.spellKnown[s]) {
                bool roll = rnd(30) < c.attrib[attr];
                if (roll || !any) { learned = true; any = true; c.spellKnown[s] = true; }
            }
    };
    learned = false;
    for (int g = 1; g <= 7; ++g) if (c.mageSpells[g] > 0) tryRange(mg[g - 1][0], mg[g - 1][1], IQ);
    for (int g = 1; g <= 7; ++g) if (c.priestSpells[g] > 0) tryRange(pg[g - 1][0], pg[g - 1][1], PIETY);
    if (learned) {
        d_.putStr(win, 428);
        d_.newline(win);
    }
    recalcSpellCounts(c);
}

void Castle::gainLose(Win *win, int m) {                           // CASTLE:37
    Character &c = w_.party.member(m);
    for (int a = 0; a <= 5; ++a) {
        if (rnd(4) == 0) continue;
        int v = c.attrib[a];
        if (rnd(130) < c.age / 52) {
            int r6 = rnd(6);                                       // p-code LAND: both operands run
            if (v == 18 && r6 != 4) {
                // untouched
            } else {
                --v;
                attrMessage(win, 430, a);
                if (a == 3 && v == 2) {                             // CASTLE:39 -- old age
                    d_.putStr(win, 429);
                    d_.newline(win);
                    c.status = Status::Lost;
                    c.hpLeft = 0;
                    return;
                }
            }
        } else if (v != 18) {
            ++v;
            attrMessage(win, 431, a);
        }
        c.attrib[a] = v;
    }
}

void Castle::levelUp(Win *win, int m) {                            // CASTLE:31
    Character &c = w_.party.member(m);
    d_.putStr(win, 432);
    d_.newline(win);
    ++c.charLevel;
    if (c.charLevel > c.maxLevelAcquired) c.maxLevelAcquired = c.charLevel;
    recalcSpellCounts(c);
    bool learned = false;
    learnSpells(win, m, learned);
    gainLose(win, m);
    int total = 0;
    for (int l = 1; l <= c.charLevel; ++l) total += hpRoll(m);
    if (int(c.cls) == 5) total += hpRoll(m);
    if (total <= c.hpMax) total = c.hpMax + 1;
    d_.putS(win, Dos::substitute(d_.str(442), Dos::itoa(total - c.hpMax)));
    d_.newline(win);
    c.hpLeft += total - c.hpMax;
    c.hpMax = total;
}

void Castle::checkLevel(Win *win, int m) {                         // CASTLE:30
    Character &c = w_.party.member(m);
    ExpTable exp{w_.sc.record(Scenario::Exp, 0)};
    long long need;
    int cls = int(c.cls);
    if (c.charLevel <= 12) {
        need = exp.threshold(cls, c.charLevel).value();
    } else {
        need = exp.threshold(cls, 12).value();
        for (int l = 13; l <= c.charLevel; ++l) need += exp.threshold(cls, 0).value();
    }
    if (need <= c.exp.v) {
        levelUp(win, m);
    } else {
        d_.putStr(win, 433);
        d_.putS(win, Dos::long2str(need - c.exp.v));
        d_.putStr(win, 434);
        d_.newline(win);
        d_.putStr(win, 435);
        d_.newline(win);
    }
}

void Castle::nap(Win *win, int m, int hpPerWeek, int goldPerWeek) {   // CASTLE:40 (+41)
    Character &c = w_.party.member(m);
    d_.con.clear(win, true);
    bool birthday = false;
    if (goldPerWeek > 0) {
        while (c.gold.v >= goldPerWeek && c.hpLeft < c.hpMax && !d_.keyReady()) {
            ++c.age;                                                // CASTLE:41
            birthday = birthday || (c.age % 52 == 1);
            c.hpLeft += hpPerWeek;
            if (c.hpLeft > c.hpMax) c.hpLeft = c.hpMax;
            c.gold.v -= goldPerWeek;
            d_.gotoXY(win, 0, 0);
            d_.putCentered(win, Dos::substitute(d_.str(436), c.name));
            d_.newline(win);
            d_.newline(win);
            d_.putStr(win, 437);
            d_.putNum(win, c.hpLeft, 5);
            d_.putC(win, '/');
            d_.putNum(win, c.hpMax, 5);
            d_.putC(win, ')');
            d_.newline(win);
            d_.putStr(win, 438);
            d_.putS(win, Dos::long2str(c.gold.v));
            d_.putC(win, ' '); d_.putC(win, ' '); d_.putC(win, ' ');
            d_.pause(8);
        }
    } else {
        d_.putCentered(win, Dos::substitute(d_.str(439), c.name));
        d_.pause(20);
    }
    if (d_.keyReady()) d_.getKey();
    d_.con.clear(win, true);
    if (birthday) {                                                 // CASTLE:30
        d_.putStr(win, 444);
        d_.newline(win);
    }
    checkLevel(win, m);
    recalcSpellCounts(c);                                                // CASTLE:23
    d_.gotoXY(win, 0, 9);
    d_.putStrC(win, 440);
    d_.waitCR();
    d_.inchar = 0;
}

void Castle::inn() {                                                // CASTLE:20
    Console &con = d_.con;
    std::string keys = d_.str(441);                                 // "ABCDEPL"
    Win *win = con.open(0, 2, 40, 13, 8, true);
    for (;;) {
        // CASTLE:21 -- who will stay?
        con.clear(win, true);
        d_.putStrC(win, 417);
        d_.newline(win);
        d_.putStrC(win, 418);
        d_.putStrC(win, 419);
        int who = d_.getNum(w_.party.count()) - 1;
        if (d_.quit) { con.close(win, false); return; }
        if (who < 0) { con.close(win, false); return; }
        if (w_.party.member(who).status != Status::OK) continue;
        int choice;
        do {
            innWelcome(win, who);
            do {
                d_.getKey();
                if (d_.quit) { con.close(win, false); return; }
                choice = -1;
                for (size_t i = 0; i < keys.size(); ++i)
                    if (d_.inchar == (unsigned char)keys[i]) choice = int(i) + 1;
                if (d_.inchar == 13) choice = 7;
            } while (choice == -1);
            switch (choice) {
                case 1: nap(win, who, 0, 0); break;
                case 2: nap(win, who, 1, 10); break;
                case 3: nap(win, who, 3, 50); break;
                case 4: nap(win, who, 7, 200); break;
                case 5: nap(win, who, 10, 500); break;
                case 6: {                                           // WIZARDRY:44 -- pool gold
                    long long sum = 0;
                    for (int i = 0; i < w_.party.count(); ++i) {
                        sum = std::min(kMaxGold, sum + w_.party.member(i).gold.v);
                        w_.party.member(i).gold.v = 0;
                    }
                    w_.party.member(who).gold.v = sum;
                    break;
                }
                default: break;
            }
            charRow(who);
            save();
        } while (!(choice == 7 || w_.party.member(who).status != Status::OK));
    }
}

} // namespace wiz
