#include "wiz/roller_dos.h"

#include <algorithm>
#include <array>

namespace wiz {

void RollerScreen::save() {
    if (!w_.rosterPath.empty()) w_.roster.save(w_.rosterPath);
}

// ---- helpers shared by the sub-screens -----------------------------------------------------------------
void RollerScreen::summaryLine() {                                   // ROLLER:3
    Win *w = summary_;
    d_.gotoXY(w, 0, 0);
    d_.putS(w, rec_.name);
    while (w->cx <= 15) d_.putC(w, ' ');
    d_.putC(w, 'L');
    d_.putNum(w, rec_.charLevel, 3);
    d_.putC(w, ' ');
    if (int(rec_.align) == 0) d_.putC(w, '?');
    else d_.putS(w, d_.str(10009 + int(rec_.align)).substr(0, 1));
    d_.putC(w, '-');
    if (int(rec_.cls) == 8) d_.putS(w, "???");
    else d_.putS(w, d_.str(10030 + int(rec_.cls)).substr(0, 3));
    d_.putC(w, ' ');
    if (int(rec_.race) == 0) d_.putS(w, "???");
    else d_.putStr(w, 9999 + int(rec_.race));
    while (w->cx != 0) d_.putC(w, ' ');
}

bool RollerScreen::computeEligible() {                               // ROLLER:4 (+ 5/6/7)
    static const int req[8][6] = {{11, 0, 0, 0, 0, 0},   {0, 11, 0, 0, 0, 0},  {0, 0, 11, 0, 0, 0},
                                  {0, 0, 0, 0, 11, 0},   {0, 12, 12, 0, 0, 0}, {15, 11, 10, 14, 10, 0},
                                  {15, 12, 12, 15, 14, 15}, {17, 17, 17, 17, 17, 17}};
    static const bool al[8][3] = {{1, 1, 1}, {1, 1, 1}, {1, 0, 1}, {0, 1, 1},
                                  {1, 0, 1}, {1, 1, 0}, {1, 0, 0}, {0, 0, 1}};
    bool any = false;
    for (int c = 0; c < 8; ++c) {
        bool ok = true;
        for (int i = 0; i < 6; ++i) ok = ok && cur_[i] >= req[c][i];
        int a = int(rec_.align);
        ok = ok && ((al[c][0] && a == 1) || (al[c][1] && a == 2) || (al[c][2] && a == 3));
        elig_[c] = ok;
        any = any || ok;
    }
    return any;
}

void RollerScreen::setBase() {                                       // ROLLER:11 (SETXBASE) / 12
    static const char *digits[6] = {"", "885889", "7::696", ":7::56", "77:8:7", "5776:?"};
    int r = int(rec_.race);
    if (r < 1 || r > 5) return;                                      // out of range: untouched
    for (int i = 0; i < 6; ++i) base_[i] = digits[r][i] - 48;
}

void RollerScreen::applyClass(int cls) {                             // ROLLER:13
    setBase();
    for (int i = 0; i < 6; ++i) rec_.attrib[i] = base_[i];
    rec_.cls = Class(cls);
    rec_.charLevel = 1;
    rec_.exp.v = 0;
    rec_.age += 52 * w_.rng.mod(3) + 252;
    if (cls == 4 || cls == 1) {
        rec_.spellKnown[3] = true;
        rec_.spellKnown[1] = true;
    } else if (cls == 2) {
        rec_.spellKnown[23] = true;
        rec_.spellKnown[24] = true;
    }
    for (int l = 1; l <= 7; ++l) { rec_.mageSpells[l] = 0; rec_.priestSpells[l] = 0; }
    for (int l = 0; l < rec_.possCount; ++l)
        if (!rec_.poss[l].cursed) rec_.poss[l].equipped = false;
    writeRec();
}

// ---- ROLLER:1 ----------------------------------------------------------------------------------------------
RollerResult RollerScreen::run(int xgoto) {
    Console &con = d_.con;
    RollerResult res;
    for (;;) {
        if (xgoto == 19) {                                           // back from CAMP's sheet
            int t = trainingMenu(19);
            if (t == 2) { res.windowClosed = true; return res; }
            if (t == 1) { res.xgoto = 17; return res; }
            xgoto = 0;
        }
        con.clear(d_.window1, false);
        if (!title_) title_ = d_.boxMsg(333, 0, 1, 15);
        std::string spec = d_.str(334);
        Win *m = d_.centerBox(d_.menuWidth(spec), 8, 6, 25, true);
        int g = d_.menuStr(m, 334);
        con.close(m, false);
        if (d_.quit) { res.windowClosed = true; return res; }
        switch (g) {
            case 0: create(); break;
            case 1: {
                int t = trainingMenu(1);
                if (t == 2) { res.windowClosed = true; return res; }
                if (t == 1) { res.xgoto = 17; return res; }
                break;
            }
            case 2: rosterList(); break;
            case 3:
                con.close(title_, false);
                res.xgoto = XCASTLE;
                return res;
            default: break;
        }
        if (d_.quit) { res.windowClosed = true; return res; }
    }
}

// ---- the roster list (ROLLER:14) ---------------------------------------------------------------------------------
void RollerScreen::rosterList() {
    Console &con = d_.con;
    con.clear(d_.window1, false);
    con.hide(title_, true);
    Win *prompt = d_.centerBox(Dos::visLen(d_.str(303)), 21, 3, 30, false);
    Win *w = d_.window1;
    int row = 0;
    for (int r = 0; r < w_.roster.count(); ++r) {
        const Character &c = w_.roster.slot(r);
        if (int(c.status) == 7) continue;
        d_.gotoXY(w, 0, row++);
        d_.putS(w, c.name);
        while (w->cx <= 15) d_.putC(w, ' ');
        d_.putC(w, 'L');
        d_.putNum(w, c.charLevel, 3);
        d_.putC(w, ' ');
        d_.putS(w, d_.str(10009 + int(c.align)).substr(0, 1));
        d_.putC(w, '-');
        d_.putS(w, d_.str(10030 + int(c.cls)).substr(0, 3));
        d_.putC(w, ' ');
        d_.putS(w, d_.str(9999 + int(c.race)).substr(0, 3));
        d_.putC(w, ' ');
        bool out = c.lostLevel != 0 || c.inMaze;
        if (int(c.status) >= 5) d_.putStr(w, out ? 300 : 301);
        else if (out) d_.putStr(w, 302);
    }
    for (;;) {
        for (int i = 0; i <= 10; ++i) {
            d_.pause(1);
            if (d_.keyReady()) {
                d_.getKey();
                if (d_.quit) return;
                if (d_.inchar == 13) {
                    d_.inchar = 0;
                    con.close(prompt, false);
                    con.clear(d_.window1, false);
                    con.show(title_, true);
                    return;
                }
            }
        }
        if (prompt->prio > 127) {
            con.show(prompt, true);
            d_.gotoXY(prompt, 0, 0);
            d_.putStrC(prompt, 303);
        } else {
            con.hide(prompt, true);
        }
    }
}

// ---- picking a character (ROLLER:23..27) --------------------------------------------------------------------------
int RollerScreen::pickCharacter() {
    Console &con = d_.con;
    Win *prompt = con.open(2, 6, 36, 4, 7, false);                   // BACKGND
    Win *list = con.open(0, 9, 40, 12, 31, true);                    // ROLLER:24
    int slotOf[20];
    std::fill(slotOf, slotOf + 20, -1);
    int n = 0;
    for (int r = 0; r < w_.roster.count(); ++r) {
        const Character &c = w_.roster.slot(r);
        d_.gotoXY(list, 19 * (n / 10), n % 10);
        if (int(c.status) != 7) {
            d_.putC(list, 65 + n);
            d_.putS(list, ") ");
            d_.putS(list, c.name);
            slotOf[n++] = r;
        }
    }
    con.clear(prompt, true);
    d_.gotoXY(prompt, 0, 0);
    d_.putStrC(prompt, 406);
    auto closeAll = [&] {                                            // ROLLER:25
        con.close(list, false);
        con.close(prompt, false);
    };
    for (;;) {
        d_.getKey();
        if (d_.quit) return -1;
        if (d_.inchar == 13) { closeAll(); return -1; }
        int l = d_.inchar - 65;
        if (l >= 0 && l <= 19 && slotOf[l] != -1) {                  // ROLLER:26
            const Character &c = w_.roster.slot(slotOf[l]);
            if (!c.password.empty()) {
                d_.gotoXY(list, 19 * (l / 10), l % 10);
                d_.putS(list, "-->");
                con.clear(prompt, true);
                d_.gotoXY(prompt, 0, 0);
                d_.putStr(prompt, 410);
                std::string pw = d_.getPassword(prompt, w_.rng);
                if (d_.quit) return -1;
                if (pw != c.password) {
                    d_.gotoXY(list, 19 * (l / 10), l % 10);
                    d_.putC(list, 65 + l);
                    d_.putS(list, ") ");
                    con.clear(prompt, true);                         // ROLLER:27
                    d_.putStrC(prompt, 411);
                    d_.pause(20);
                    closeAll();
                    return -1;
                }
            }
            int s = slotOf[l];
            closeAll();
            return s;
        }
        // PRINTBEL
    }
}

// ---- the Training Grounds menu for one character (ROLLER:15) ----------------------------------------------------------
int RollerScreen::trainingMenu(int xgotoIn) {
    Console &con = d_.con;
    if (xgotoIn != 19) {
        int s = pickCharacter();
        if (d_.quit) return 2;
        if (s == -1) return 0;
        readRec(s);
    } else {                                                         // restore what CAMP handed back
        rec_ = w_.party.member(0);
        slot_ = w_.party.rosterSlot(0);
        w_.party.disbandQuiet();
    }
    summary_ = con.open(0, 4, 40, 3, 7, true);
    for (;;) {
        summaryLine();
        con.clear(d_.window1, false);
        std::string spec = d_.str(318);
        Win *m = d_.centerBox(d_.menuWidth(spec), 8, 7, 6, true);
        int g = d_.menu(m, spec);
        con.close(m, true);
        if (d_.quit) return 2;
        switch (g) {
            case 0:                                                  // ROLLER:17 -- CAMP shows the sheet
                w_.party.setSingle(rec_, slot_);
                con.close(summary_, false);
                return 1;
            case 1: {                                                // ROLLER:19
                if (!confirm(d_.str(308))) break;
                rec_.status = Status::Lost;                          // ROLLER:16
                rec_.inMaze = false;
                writeRec();
                con.close(summary_, false);
                return 0;
            }
            case 2: changeClass(); break;
            case 3: alterPassword(); break;
            default:
                con.close(summary_, false);
                return 0;
        }
        if (d_.quit) return 2;
    }
}

bool RollerScreen::confirm(const std::string &verb) {                // ROLLER:18
    Console &con = d_.con;
    con.clear(d_.window1, true);
    d_.gotoXY(d_.window1, 0, 10);
    d_.putCentered(d_.window1, Dos::substitute(d_.str(304), verb));
    con.refresh();
    std::string yes = d_.str(306), no = d_.str(307);
    do {
        d_.getKey();
        if (d_.quit) return false;
    } while (d_.inchar != (unsigned char)yes[0] && d_.inchar != (unsigned char)no[0]);
    return d_.inchar == (unsigned char)yes[0];
}

void RollerScreen::changeClass() {                                   // ROLLER:20/21
    Console &con = d_.con;
    for (int i = 0; i < 6; ++i) cur_[i] = rec_.attrib[i];
    con.clear(d_.window1, false);
    computeEligible();
    elig_[int(rec_.cls) & 7] = false;
    bool any = false;
    for (int c = 0; c < 8; ++c) any = any || (elig_[c] && c != int(rec_.cls));
    if (!any) {                                                      // ROLLER:21
        Win *w = con.open(0, 10, 40, 4, 2, true);
        d_.putStrC(w, 309);
        d_.putStrC(w, 310);
        d_.waitCR();
        con.close(w, false);
        return;
    }
    Win *list = con.open(13, 11, 14, 10, 2, true);
    for (int c = 0; c < 8; ++c)
        if (elig_[c] && c != int(rec_.cls)) {
            d_.putC(list, 65 + c);
            d_.putS(list, ") ");
            d_.putS(list, w_.sc.classes()[size_t(c)]);
            d_.newline(list);
        }
    d_.gotoXY(d_.window1, 0, 7);
    d_.putStrC(d_.window1, 311);
    con.close(list, false);
    for (;;) {
        d_.getKey();
        if (d_.quit) return;
        if (d_.inchar == 13) return;
        if (d_.inchar >= 65 && d_.inchar <= 72) {
            int c = d_.inchar - 65;
            if (elig_[c] && c != int(rec_.cls)) { applyClass(c); return; }
        }
    }
}

void RollerScreen::alterPassword() {                                 // ROLLER:22
    Console &con = d_.con;
    Win *w1 = d_.window1;
    con.clear(w1, true);
    d_.gotoXY(w1, 0, 8);
    d_.putStrC(w1, 312);
    d_.gotoXY(w1, 10, 10);
    std::string a = d_.getPassword(w1, w_.rng);
    d_.gotoXY(w1, 0, 12);
    d_.putStrC(w1, 313);
    d_.gotoXY(w1, 10, 14);
    std::string b = d_.getPassword(w1, w_.rng);
    d_.gotoXY(w1, 0, 16);
    if (a == b) {
        rec_.password = a;
        writeRec();
        d_.putStrC(w1, 314);
    } else {
        d_.putStr(w1, 315);
        d_.putStr(w1, 316);
    }
    d_.waitCR();
}

// ---- creating a character (ROLLER:28) -------------------------------------------------------------------------------------
bool RollerScreen::findFreeSlot() {                                  // ROLLER:35
    for (int i = 0; i < w_.roster.count(); ++i)
        if (int(w_.roster.slot(i).status) == 7) { slot_ = i; return true; }
    Console &con = d_.con;
    std::string s = d_.str(335);
    Win *w = d_.centerBox(Dos::visLen(s), 12, 3, 31, true);
    d_.putCentered(w, s);
    con.close(w, false);
    d_.waitCR();
    return false;
}

std::string RollerScreen::promptLine(Win *bottom, Win *mid, int strNo, bool) {   // ROLLER:34
    Console &con = d_.con;
    con.clear(bottom, false);
    con.clear(mid, true);
    d_.putCentered(bottom, d_.str(strNo));
    d_.putC(mid, '>');
    return d_.getPassword(mid, w_.rng);
}

int RollerScreen::choose(Win *bottom, int strNo, int n) {            // ROLLER:29
    d_.con.clear(bottom, true);
    d_.putCentered(bottom, d_.str(strNo));
    for (;;) {
        d_.getKey();
        if (d_.quit) return 0;
        int l = d_.inchar - 65;
        if (l >= 0 && l < n) { d_.con.clear(bottom, true); return l; }
    }
}

bool RollerScreen::enterName(Win *bottom, Win *mid) {                // ROLLER:36
    Console &con = d_.con;
    for (;;) {
        con.clear(bottom, false);
        con.clear(mid, true);
        d_.putStrC(bottom, 337);
        d_.putC(mid, '>');
        std::string name = d_.getName(mid, 15);
        if (d_.quit) return false;
        if (name.empty()) {
            con.close(summary_, false);
            con.close(bottom, false);
            con.close(mid, false);
            return false;
        }
        bool taken = false;
        for (int g = 0; g < w_.roster.count(); ++g) {
            const Character &c = w_.roster.slot(g);
            taken = taken || (int(c.status) != 7 && c.name == name);
        }
        if (taken) {
            std::string s = d_.str(336);
            Win *w = d_.centerBox(Dos::visLen(s), 14, 3, 31, true);
            d_.putCentered(w, s);
            con.close(w, false);
            d_.waitCR();
            continue;
        }
        rec_.name = name;
        summaryLine();
        return true;
    }
}

void RollerScreen::create() {
    Console &con = d_.con;
    if (!findFreeSlot()) return;
    // a blank record
    std::array<u8, Character::kRecordBytes> zero{};
    rec_ = Character();
    rec_.read({zero.data(), zero.size()});
    rec_.name = "???";
    rec_.age = 500;
    rec_.gold.v = 100 + w_.rng.mod(100);
    rec_.status = Status::OK;
    rec_.hpLeft = rec_.hpMax = 8;
    rec_.cls = Class(8);
    rec_.charLevel = 1;
    rec_.armorClass = 10;
    con.clear(d_.window1, true);
    con.close(title_, false);
    summary_ = con.open(0, 0, 40, 3, 7, false);
    Win *bottom = con.open(0, 21, 40, 3, 11, false);
    Win *mid = con.open(0, 10, 40, 3, 11, true);
    summaryLine();
    if (!enterName(bottom, mid)) {                                   // empty name: back to the menu
        title_ = nullptr;
        return;
    }
    std::string pw, again;
    for (;;) {
        pw = promptLine(bottom, mid, 329, true);
        if (d_.quit) return;
        if (pw.empty()) again.clear();
        else again = promptLine(bottom, mid, 330, true);
        if (d_.quit) return;
        if (again == pw) break;
    }
    rec_.password = pw;
    con.close(mid, false);
    mid = con.open(13, 8, 14, 7, 12, true);
    for (int r = 1; r <= 5; ++r) {
        d_.gotoXY(mid, 0, r - 1);
        d_.putC(mid, 64 + r);
        d_.putS(mid, ") ");
        d_.putStr(mid, 9999 + r);
    }
    rec_.race = Race(1 + choose(bottom, 331, 5));
    if (d_.quit) return;
    con.close(mid, false);
    mid = con.open(13, 8, 14, 5, 12, true);
    summaryLine();
    for (int a = 1; a <= 3; ++a) {
        d_.gotoXY(mid, 0, a - 1);
        d_.putC(mid, 64 + a);
        d_.putS(mid, ") ");
        d_.putStr(mid, 10009 + a);
    }
    rec_.align = Align(1 + choose(bottom, 332, 3));
    if (d_.quit) return;
    summaryLine();
    con.close(mid, false);
    allocate(bottom, mid);
    title_ = nullptr;
}

// ROLLER:30 -- hand out the bonus points, then pick a class
void RollerScreen::allocate(Win *&bottom, Win *&mid) {
    Console &con = d_.con;
    setBase();
    for (int i = 0; i < 6; ++i) cur_[i] = base_[i];
    con.close(bottom, false);
    bottom = con.open(0, 20, 40, 4, 11, false);
    Win *attrs = con.open(7, 6, 13, 10, 12, false);
    Win *classes = con.open(20, 6, 13, 10, 12, true);
    auto line = [&](const std::string &s) {                          // ROLLER:31
        for (int n = int(s.size()); n <= 7; ++n) d_.putC(attrs, ' ');
        d_.putS(attrs, s);
        d_.newline(attrs);
    };
    for (int n = 10020; n <= 10025; ++n) line(d_.str(n));
    line("");
    line(d_.str(323));
    con.clear(bottom, true);
    d_.putStrC(bottom, 324);
    d_.putStrC(bottom, 325);
    int row = 0;
    int bonus = 5 + w_.rng.mod(5);
    if (w_.rng.mod(20) == 10) bonus += 10;
    if (w_.rng.mod(20) == 10) bonus += 10;
    for (int i = 0; i < 6; ++i) { d_.gotoXY(attrs, 9, i); d_.putNum(attrs, cur_[i], 2); }
    d_.gotoXY(attrs, 9, 7);
    d_.putNum(attrs, bonus, 2);
    d_.gotoXY(attrs, 8, 0);
    d_.putC(attrs, '_');
    bool oldElig[8] = {};
    bool anyElig = false;
    auto change = [&](int delta) {                                   // ROLLER:33
        int v = cur_[row] + delta;
        if (v >= base_[row] && v <= 18 && bonus - delta >= 0) {
            cur_[row] = v;
            bonus -= delta;
            d_.gotoXY(attrs, 9, row);
            d_.putNum(attrs, cur_[row], 2);
            d_.gotoXY(attrs, 9, 7);
            d_.putNum(attrs, bonus, 2);
            anyElig = computeEligible();
            for (int c = 0; c < 8; ++c) {
                if (!oldElig[c] && elig_[c]) {
                    d_.gotoXY(classes, 0, c);
                    d_.putC(classes, 65 + c);
                    d_.putS(classes, ") ");
                    d_.putStr(classes, 10030 + c);
                } else if (!elig_[c] && oldElig[c]) {
                    con.clearRect(classes, 0, c, 11, 1);
                }
            }
            std::copy(elig_, elig_ + 8, oldElig);
        } else {
            // PRINTBEL
        }
    };
    for (;;) {
        d_.getKey();
        if (d_.quit) return;
        int k = d_.inchar;
        if (k == 13) {
            d_.gotoXY(attrs, 8, row); d_.putC(attrs, ' ');
            row = (row == 5) ? 0 : row + 1;
            d_.gotoXY(attrs, 8, row); d_.putC(attrs, '_');
        } else if (k == 8) {
            d_.gotoXY(attrs, 8, row); d_.putC(attrs, ' ');
            row = (row == 0) ? 5 : row - 1;
            d_.gotoXY(attrs, 8, row); d_.putC(attrs, '_');
        } else if (k == 11 || k == 43 || k == 61) {
            change(1);
        } else if (k == 10 || k == 45) {
            change(-1);
        } else if (k == 67 || k == 27) {
            if (anyElig && bonus == 0) {                             // ROLLER:32 -- choose the class
                con.close(bottom, false);
                bottom = con.open(0, 21, 40, 3, 11, true);
                d_.putStrC(bottom, 319);
                int cls;
                for (;;) {
                    int l;
                    do {
                        d_.getKey();
                        if (d_.quit) return;
                        l = d_.inchar - 65;
                    } while (l < 0 || l > 5);
                    cls = l;
                    if (elig_[cls]) break;
                }
                rec_.cls = Class(cls);
                summaryLine();
                con.clear(bottom, true);
                d_.putStrC(bottom, 320);
                std::string yes = d_.str(321), no = d_.str(322);
                do {
                    d_.getKey();
                    if (d_.quit) return;
                } while (d_.inchar != (unsigned char)yes[0] && d_.inchar != (unsigned char)no[0]);
                if (d_.inchar == (unsigned char)yes[0]) {
                    applyClass(cls);
                    rec_.maxLevelAcquired = 1;
                    for (int i = 0; i < 6; ++i) rec_.attrib[i] = cur_[i];
                    writeRec();
                }
                con.close(summary_, false);
                con.close(bottom, false);
                con.close(attrs, false);
                con.close(classes, true);
                return;
            }
            // PRINTBEL
        }
    }
}

} // namespace wiz
