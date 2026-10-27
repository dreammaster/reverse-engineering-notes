// SEGMENT CAMP -- see camp_dos.h.  Every routine names the p-code proc it ports.
#include "wiz/camp_dos.h"

#include "wiz/scenario.h"

#include <algorithm>

namespace wiz {

// ---- helpers ---------------------------------------------------------------------------
static std::string firstChars(const std::string &s, size_t n) { return s.substr(0, n); }

// ---- CAMP:3 -- one roster row -------------------------------------------------------------
void Camp::row(int i) {
    const Character &c = w_.party.member(i);
    Win *g = w_.g10;
    d_.gotoXY(g, 0, i + 1);
    d_.putNum(g, i + 1, 1);
    d_.putC(g, ' ');
    d_.putS(g, c.name);
    while (g->cx < 18) d_.putC(g, ' ');
    d_.putS(g, firstChars(d_.str(10009 + int(c.align)), 1));
    d_.putC(g, '-');
    d_.putS(g, firstChars(d_.str(10030 + int(c.cls)), 3));
    d_.putC(g, ' ');
    int ac = c.armorClass - w_.g.protect;
    if (ac > -10) d_.putNum(g, ac, 2);
    else d_.putStr(g, 601);
    d_.putNum(g, c.hpLeft, 5);
    d_.putC(g, ' ');
    if (c.status == Status::OK) {
        if (c.poison != 0) d_.putStr(g, 602);
        else d_.putNum(g, c.hpMax, 4);
    } else {
        d_.putS(g, firstChars(d_.str(10040 + int(c.status)), 6));
    }
    while (g->cx != 0) d_.putC(g, ' ');
}

// ---- CAMP:4 -- "* message *" in the small box, then a beat ---------------------------------------
void Camp::say(const std::string &s) {
    if (s.empty() || s == " ") return;
    d_.con.clear(back_, true);
    d_.gotoXY(back_, 0, 1);
    d_.putCentered(back_, "* " + s + " *");
    d_.pause(20);
}

// ---- CAMP:43 -- the awards in quotes ("*" per bit of LOSTXYL.AWARDS) -------------------------------
void Camp::drawAwards() {
    const Character &c = w_.party.member(m_);
    unsigned bits = c.raw[103 * 2] | (c.raw[103 * 2 + 1] << 8);
    static const char *marks = ">!$#&*<?BCPKODG@";
    d_.putC(sheet_, '"');
    for (int i = 0; i < 16; ++i)
        if ((bits >> i) & 1) d_.putC(sheet_, marks[i]);
    d_.putC(sheet_, '"');
}

// ---- CAMP:9 -- the attribute block ----------------------------------------------------------------
void Camp::drawStats() {
    const Character &c = w_.party.member(m_);
    Win *s = sheet_;
    d_.gotoXY(s, 0, 2);
    d_.putStr(s, 605); d_.putNum(s, c.attrib[STR], 3);
    d_.putStr(s, 606); d_.putPadded(s, c.gold.v);
    d_.putStr(s, 607); d_.putNum(s, c.charLevel, 4);
    d_.putStr(s, 608); d_.putNum(s, c.attrib[IQ], 3);
    d_.putStr(s, 609); d_.putPadded(s, c.exp.v);
    d_.putStr(s, 610); d_.putNum(s, c.age / 52, 4);
    d_.putStr(s, 611); d_.putNum(s, c.attrib[PIETY], 3);
    d_.newline(s);
    d_.putStr(s, 612); d_.putNum(s, c.attrib[VIT], 3);
    d_.putStr(s, 613); d_.putNum(s, c.hpLeft, 5); d_.putC(s, '/'); d_.putNum(s, c.hpMax, 4);
    d_.putStr(s, 614); d_.putNum(s, c.armorClass - w_.g.protect, 3);
    d_.putStr(s, 615); d_.putNum(s, c.attrib[AGI], 3);
    d_.newline(s);
    d_.putStr(s, 616); d_.putNum(s, c.attrib[LUCK], 3);
    d_.putStr(s, 617); d_.putStr(s, 10040 + int(c.status));
    if (c.poison > 0) d_.putStr(s, 618);
    while (s->cx != 0) d_.putC(s, ' ');
}

// ---- CAMP:6 -- the spell points line ----------------------------------------------------------------
void Camp::drawSpellPoints() {
    const Character &c = w_.party.member(m_);
    Win *s = sheet_;
    d_.gotoXY(s, 0, 9);
    d_.putStr(s, 603);
    for (int l = 1; l <= 7; ++l) { d_.putNum(s, c.mageSpells[l], 1); if (l < 7) d_.putC(s, '/'); }
    d_.putStr(s, 604);
    for (int l = 1; l <= 7; ++l) { d_.putNum(s, c.priestSpells[l], 1); if (l < 7) d_.putC(s, '/'); }
}

// ---- CAMP:8 -- squeeze the empty slots out of the pack ---------------------------------------------------
void Camp::compactPack() {
    Character &c = w_.party.member(m_);
    int kept = 0;
    for (int k = 0; k < c.possCount; ++k) {
        c.poss[kept] = c.poss[k];
        if (c.poss[kept].itemIndex != 0) ++kept;
    }
    c.possCount = kept;
}

// ---- CAMP:7 -- the pack, two columns -------------------------------------------------------------------------
void Camp::drawPack() {
    compactPack();
    Character &c = w_.party.member(m_);
    Win *s = sheet_;
    for (int i = 1; i <= 8; ++i) {
        int right = 19 + 19 * ((i - 1) % 2);
        d_.gotoXY(s, right - 19, 11 + (i - 1) / 2);
        if (i <= c.possCount && c.poss[i - 1].itemIndex >= 0) {
            const Possession &p = c.poss[i - 1];
            ObjectRec o{w_.sc.record(Scenario::Object, p.itemIndex)};
            bool cursed = o.cursed();                            // U2.10: the record's CURSED word
            bool usable = o.classUse(int(c.cls));                // U2.11
            d_.putNum(s, i, 1);
            d_.putC(s, ')');
            if (p.equipped) d_.putC(s, cursed ? '-' : '*');
            else if (p.identified) d_.putC(s, usable ? ' ' : '#');
            else d_.putC(s, '?');
            d_.putS(s, d_.str(14000 + 2 * p.itemIndex + (p.identified ? 1 : 0)));
        }
        if (s->cx == 0) d_.putC(s, ' ');
        while (s->cx != 0 && s->cx < right) d_.putC(s, ' ');
    }
}

// ---- CAMP:10 -- "WHOM (#) ?" with the roster brought to the front ---------------------------------------------
int Camp::pickMember(int strNo, int exclude) {
    Console &con = d_.con;
    Win *b = con.open(13, 8, 26, 5, 16, true);
    con.clear(b, true);
    con.show(w_.g10, false);
    con.setPrio(w_.g10, 30, true);
    d_.newline(b);
    d_.putStrC(b, strNo);
    d_.putStrC(b, 600);
    int n;
    do {
        n = d_.getNum(w_.party.count()) - 1;
        if (d_.quit) break;
    } while (n == exclude);
    con.setPrio(w_.g10, 6, false);
    con.hide(w_.g10, false);
    con.close(b, true);
    return n;
}

// ---- CAMP:39 / 40 / 37 -- trading ---------------------------------------------------------------------------------
void Camp::tradeGold(int target) {                                 // TRADGOLD
    Character &src = w_.party.member(m_), &dst = w_.party.member(target);
    d_.con.clear(back_, true);
    d_.putStrC(back_, 640);
    d_.putStrC(back_, 600);
    d_.putStr(back_, 641);
    std::string s = d_.getLine(back_, 12);
    if (s.empty()) return;                                         // EXIT(18:39)
    long long amount = 0;
    bool bad = false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9' || i >= 12 || bad) bad = true;
        else amount = amount * 10 + (s[i] - '0');
    }
    auto fail = [&](int strNo) {                                   // CAMP:38
        say(d_.str(strNo));
        d_.con.close(back_, true);
        drawStats();
        throw ExitProc{37};
    };
    if (bad) fail(642);
    if (src.gold.v - amount < 0) fail(643);
    dst.gold.v += amount;
    src.gold.v -= amount;
}

void Camp::tradeItems(int target) {                                // CAMP:40
    Character &src = w_.party.member(m_), &dst = w_.party.member(target);
    for (;;) {
        d_.con.clear(back_, true);
        d_.putStrC(back_, 644);
        d_.putStrC(back_, 600);
        int n = d_.getNum(src.possCount);
        if (n <= 0) return;                                        // EXIT(18:40)
        auto fail = [&](int strNo) {                               // CAMP:38
            say(d_.str(strNo));
            d_.con.close(back_, true);
            drawStats();
            throw ExitProc{37};
        };
        if (dst.possCount == 8) fail(645);
        if (src.poss[n - 1].cursed) fail(646);
        if (src.poss[n - 1].equipped) fail(647);
        dst.poss[dst.possCount++] = src.poss[n - 1];
        for (int k = n; k < src.possCount; ++k) src.poss[k - 1] = src.poss[k];
        --src.possCount;
        drawPack();
    }
}

void Camp::tradeAll() {                                            // CAMP:37
    int target = pickMember(648, m_);
    if (target == -1 || d_.quit) return;
    back_ = d_.con.open(13, 8, 26, 5, 15, true);
    try {
        tradeGold(target);
        tradeItems(target);
    } catch (const ExitProc &e) {
        if (e.proc != 37) throw;
        return;                                                    // CAMP:38 already closed the box
    }
    d_.con.close(back_, true);
    drawStats();
}

// ---- CAMP:34 -- DROP --------------------------------------------------------------------------------------------------
void Camp::dropItem() {
    Character &c = w_.party.member(m_);
    back_ = d_.con.open(13, 8, 26, 5, 15, true);
    d_.putStrC(back_, 636);
    d_.putStrC(back_, 600);
    auto done = [&](const std::string &msg) {                      // CAMP:35
        say(msg);
        d_.con.close(back_, true);
    };
    int n = d_.getNum(c.possCount);
    if (n <= 0) { done(d_.str(9999)); return; }
    if (c.poss[n - 1].cursed) { done(d_.str(637)); return; }
    if (c.poss[n - 1].equipped) { done(d_.str(638)); return; }
    for (int k = n; k < c.possCount; ++k) c.poss[k - 1] = c.poss[k];
    --c.possCount;
    drawPack();
    done(d_.str(639));
}

// ---- CAMP:36 -- I)DENTIFY is the Bishop's: another segment does it ---------------------------------------------
void Camp::identify() {
    if (int(w_.party.member(m_).cls) != 4) return;
    throw Leave{24, 1};
}

// ---- CAMP:26 / 27 -- spell names and their hashes ---------------------------------------------------------------------
std::string Camp::spellName(int idx) const {                       // CAMP:27
    if (idx == 0) return "";
    std::string s = d_.str(5000 + idx);
    if (!s.empty() && s[0] == '*') s.erase(0, 1);
    return s;
}

int Camp::spellHash(int idx) const {
    std::string s = spellName(idx);
    int h = int(s.size());
    for (size_t i = 1; i <= s.size(); ++i)
        if (h < 20000) {
            int v = (u8(s[i - 1]) - 64);
            h += v * v * int(i);
        }
    return h;
}

// ---- casting -------------------------------------------------------------------------------------------------------------------------
void Camp::endCast(const std::string &msg) {                       // CAMP:12
    say(msg);
    d_.con.close(back_, true);
    drawSpellPoints();
    throw ExitCast{};
}

void Camp::need(int level, int spell) {                            // CAMP:16: a priest spell
    if (itemCast_) return;
    const Character &c = w_.party.member(m_);
    if (c.priestSpells[level] <= 0 || !c.spellKnown[spell]) endCast(d_.str(619));
}

void Camp::needMage(int level, int spell) {                        // CAMP:22 / 23's check
    if (itemCast_) return;
    const Character &c = w_.party.member(m_);
    if (c.mageSpells[level] <= 0 || !c.spellKnown[spell]) endCast(d_.str(619));
}

void Camp::spend(int level) {                                      // CAMP:17
    if (!itemCast_) --w_.party.member(m_).priestSpells[level];
    drawSpellPoints();
    fizzleCheck();
}

void Camp::spendMage(int level) {
    if (!itemCast_) --w_.party.member(m_).mageSpells[level];
    drawSpellPoints();
    fizzleCheck();
}

void Camp::fizzleCheck() {                                         // CAMP:14
    if (w_.g.g30 > 0) endCast(d_.str(620));
}

int Camp::chooseTarget() {                                         // CAMP:15
    d_.con.clear(back_, false);
    target_ = pickMember(621, -999);
    if (target_ == -1) endCast(d_.str(622));
    return target_;
}

void Camp::heal(int dice, int sides, int level, int spell) {       // CAMP:18
    need(level, spell);
    chooseTarget();
    spend(level);
    Character &t = w_.party.member(target_);
    int amount = 0;
    if (dice == -1) {
        amount = t.hpMax;
        t.poison = 0;
        if (int(t.status) < 5) t.status = Status::OK;
    } else {
        while (dice > 0) {
            amount += w_.rng.mod(sides) + 1;
            --dice;
        }
    }
    t.hpLeft += amount;
    if (t.hpLeft > t.hpMax) t.hpLeft = t.hpMax;
    d_.con.clear(back_, true);
    d_.newline(back_);
    d_.putStr(back_, 623);
    d_.putNum(back_, t.hpLeft, 5);
    d_.putC(back_, '/');
    d_.putNum(back_, t.hpMax, 4);
    d_.pause(20);
    drawSpellPoints();
    if (target_ == m_) drawStats();
    row(target_);
    d_.con.close(back_, true);
    throw ExitCast{};
}

void Camp::revive(int level) {                                     // CAMP:20 / 21: DI and KADORTO
    need(level, level == 5 ? 43 : 50);
    chooseTarget();
    Character &t = w_.party.member(target_);
    if (int(t.status) < 5) endCast(d_.str(626));
    if (t.status == Status::Lost) endCast(d_.str(627));
    spend(level);
    auto attempt = [&]() {                                         // CAMP:21
        if (w_.rng.mod(100) <= 4 * t.attrib[VIT]) {
            t.status = Status::OK;
            t.hpLeft = level == 5 ? 1 : t.hpMax;
            if (t.attrib[VIT] == 3) t.status = Status::Lost;
            else --t.attrib[VIT];
        }
        row(target_);
        if (t.status == Status::OK) {
            endCast(d_.str(624));
        } else {
            t.status = Status(int(t.status) + 1);
            row(target_);
            endCast(d_.str(625));
        }
    };
    if (level == 5) {
        if (t.status == Status::Dead) attempt();
        else if (t.status == Status::Ashes) endCast(d_.str(628));
    } else {
        attempt();
    }
}

bool Camp::dispatch(int hash) {                                    // CAMP:24
    Character &c = w_.party.member(m_);
    switch (hash) {
        case 6491:                                                 // LATUMAPIC
            need(3, 33); spend(3); w_.g.identify = 10000; return true;
        case 2301: heal(1, 8, 1, 23); return true;                 // DIOS
        case 2889:                                                 // MILWA
            need(1, 25); spend(1); w_.g.light += 30 + w_.rng.mod(15); return true;
        case 3245:                                                 // DUMAPIC
            if (!itemCast_ && (c.mageSpells[1] == 0 || !c.spellKnown[4])) endCast(d_.str(619));
            if (w_.g.level == w_.sc.count(Scenario::Maze)) endCast(d_.str(620));
            if (!itemCast_) --c.mageSpells[1];
            drawSpellPoints(); fizzleCheck();
            d_.con.close(back_, false);
            throw Leave{24, 3};
        case 1185:                                                 // KANDI
            need(5, 42); spend(5);
            d_.con.close(back_, false);
            throw Leave{24, 2};
        case 5333:                                                 // LOMILWA
            need(3, 31); spend(3); w_.g.light = 10000; return true;
        case 9463:                                                 // LATUMOFIS
            need(4, 37); chooseTarget(); spend(4);
            w_.party.member(target_).poison = 0;
            row(target_);
            return true;
        case 2718: {                                               // DIALKO
            need(3, 32); chooseTarget(); spend(3);
            Character &t = w_.party.member(target_);
            if (t.status == Status::Paralyzed || t.status == Status::Asleep) t.status = Status::OK;
            row(target_);
            return true;
        }
        case 761: heal(2, 8, 4, 35); return true;                  // DIAL
        case 4322:                                                 // MAPORFIC
            need(4, 38); spend(4); w_.g.protect = 2;
            for (int i = 0; i < w_.party.count(); ++i) row(i);
            return true;
        case 1614: heal(3, 8, 5, 39); return true;                 // DIALMA
        case 180: revive(5); return true;                          // DI
        case 547: heal(-1, -1, 6, 46); return true;                // MADI
        case 6673: revive(7); return true;                         // KADORTO
        case 3128:                                                 // MALOR
            if (!itemCast_ && (c.mageSpells[7] == 0 || !c.spellKnown[19])) endCast(d_.str(619));
            if (!itemCast_) --c.mageSpells[7];
            drawSpellPoints(); fizzleCheck();
            d_.con.close(back_, false);
            throw Leave{24, 4};
        default: return false;
    }
}

// CAMP:11
void Camp::cast(int hash) {
    Character &c = w_.party.member(m_);
    back_ = d_.con.open(13, 8, 26, 5, 15, true);
    itemCast_ = hash > 0;
    try {
        if (hash == -1) {
            d_.putStrC(back_, 629);
            d_.putStrC(back_, 600);
            d_.putStr(back_, 630);
            int x = back_->cx, y = back_->cy;
            std::string typed = d_.getLine(back_, 15);
            if (typed.empty()) endCast(d_.str(9999));
            // CAMP:25 / 29: an exact spell name, else the known spells it is the start of
            hash = -1;
            auto hashOf = [](const std::string &t) {                // CAMP:26
                int h = int(t.size());
                for (size_t i = 1; i <= t.size(); ++i)
                    if (h < 20000) { int v = u8(t[i - 1]) - 64; h += v * v * int(i); }
                return h;
            };
            int th = hashOf(typed);
            bool exact = false;
            for (int s = 1; s <= 50 && !exact; ++s)
                if (th == spellHash(s) && spellName(s) == typed) exact = true;
            if (exact) {
                hash = th;
            } else {
                std::vector<int> list;
                for (int s = 1; s <= 50; ++s)
                    if (spellName(s).compare(0, typed.size(), typed) == 0 && c.spellKnown[s]) list.push_back(s);
                typed.clear();                                      // CAMP:25 empties the typed text
                if (list.size() == 1) {
                    hash = spellHash(list[0]);
                    typed = spellName(list[0]);
                } else if (list.size() > 1) {
                    // CAMP:30 -- several spells start that way: pick one
                    int n = int(list.size());
                    Win *m = d_.con.open(7, 9 - (n + 1) / 2, 26, 6 + n, 31, true);
                    d_.putStrC(m, 1040);
                    d_.newline(m);
                    static const int mgEnd[7] = {4, 6, 8, 11, 14, 18, 21}, prEnd[7] = {26, 30, 34, 38, 44, 48, 50};
                    for (int k = 0; k < n; ++k) {
                        d_.gotoXY(m, 0, k + 2);
                        d_.putC(m, 65 + k);
                        d_.putS(m, ") ");
                        d_.putS(m, spellName(list[k]));
                        while (m->cx < 21) d_.putC(m, ' ');
                        d_.putS(m, "(");
                        int lvl = 7;
                        for (int g = 6; g >= 0; --g) if (list[k] <= (list[k] < 22 ? mgEnd[g] : prEnd[g])) lvl = g + 1;
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
            d_.gotoXY(back_, x, y);
            d_.putS(back_, typed);
            d_.pause(2);
            if (hash == -1) endCast(d_.str(9999));
        }
        if (dispatch(hash)) endCast(d_.str(632));
        endCast(d_.str(631));
    } catch (const ExitCast &) {
    }
}

// CAMP:32 -- U)SE an item
void Camp::useItem() {
    Character &c = w_.party.member(m_);
    back_ = d_.con.open(13, 8, 26, 5, 15, true);
    d_.putStrC(back_, 633);
    d_.putStrC(back_, 600);
    auto done = [&](const std::string &msg) {                      // CAMP:33
        say(msg);
        d_.con.close(back_, true);
    };
    int n = d_.getNum(c.possCount);
    if (n <= 0) { done(d_.str(9999)); return; }
    ObjectRec o{w_.sc.record(Scenario::Object, c.poss[n - 1].itemIndex)};
    if (o.spellPwr() == 0) { done(d_.str(634)); return; }
    if (int(o.type()) != 5 && !c.poss[n - 1].equipped) { done(d_.str(635)); return; }
    if (w_.rng.mod(100) < o.chgChance()) c.poss[n - 1].itemIndex = o.changeTo();
    int spell = o.spellPwr();
    d_.con.close(back_, false);
    cast(spellHash(spell));
    drawPack();
}

// ---- CAMP:42 / 41 -- the sheet's command menu ---------------------------------------------------------------------------
int Camp::sheetMenu(int mode) {                                    // CAMP:42
    const Character &c = w_.party.member(m_);
    (void)c;
    Win *b = d_.con.open(0, 21, 40, 4, 8, true);
    int r;
    if (mode == 2) { r = d_.menuStr(b, 649); if (r == 8) r = -1; }
    else if (mode == 1) { r = d_.menuStr(b, 650); if (r == 6) r = -1; }
    else { r = d_.menuStr(b, 651); if (r == 1) r = -1; }
    d_.con.close(b, false);
    return r;
}

void Camp::command(int sel, int mode) {                            // CAMP:41
    Character &c = w_.party.member(m_);
    switch (sel) {
        case 0: throw Leave{24, 0};                                // R)EAD
        case 1: if (mode > 0) throw Leave{14, 0}; break;           // E)QUIP
        case 2: tradeAll(); break;
        case 3: if (mode > 0) dropItem(); break;
        case 4: {                                                  // P)OOL GOLD (WIZARDRY:44)
            long long sum = 0;
            for (int i = 0; i < w_.party.count(); ++i) {
                sum = std::min(999999999999LL, sum + w_.party.member(i).gold.v);
                w_.party.member(i).gold.v = 0;
            }
            c.gold.v = sum;
            drawStats();
            break;
        }
        case 5: if (c.status == Status::OK && mode != 0) identify(); break;
        case 6: if (mode == 2) cast(-1); break;
        case 7: if (mode == 2) useItem(); break;
        default: break;
    }
}

// ---- CAMP:5 -- the character sheet -----------------------------------------------------------------------------------------
void Camp::sheet(int xgoto) {
    Console &con = d_.con;
    xgoto_ = xgoto;
    con.hide(w_.g10, false);
    if (!sheet_) {
        const Character &c = w_.party.member(m_);
        sheet_ = con.open(0, 4, 40, 17, 7, true);
        d_.putS(sheet_, c.name);
        d_.putC(sheet_, ' ');
        d_.putC(sheet_, 'L');
        d_.putNum(sheet_, c.charLevel, 3);
        d_.putC(sheet_, ' ');
        d_.putS(sheet_, firstChars(d_.str(10009 + int(c.align)), 1));
        d_.putC(sheet_, '-');
        d_.putS(sheet_, firstChars(d_.str(10030 + int(c.cls)), 3));
        d_.putC(sheet_, ' ');
        d_.putS(sheet_, d_.str(9999 + int(c.race)));
        d_.putC(sheet_, ' ');
        drawAwards();
    }
    drawStats();
    drawSpellPoints();
    drawPack();
    w_.g.g41 = xgoto;
    for (;;) {
        // CAMP:42 picks the menu: 0 Training Grounds (read only), 1 Tavern or the not-OK, 2 a live camper
        int mode;
        if (xgoto == 17) mode = 0;
        else if (xgoto == 4) mode = 1;
        else mode = w_.party.member(m_).status == Status::OK ? 2 : 1;
        int sel = sheetMenu(mode);
        if (d_.quit || sel == -1) break;
        try {
            command(sel, mode);
        } catch (const ExitProc &) {
        }
        if (d_.quit) break;
    }
    con.close(sheet_, false);
    con.show(w_.g10, false);
}

// ---- CAMP:1 ---------------------------------------------------------------------------------------------------------------------------
CampResult Camp::run(int xgoto, int member) {
    CampResult res;
    m_ = member < 0 ? 0 : member;
    try {
        if (xgoto == 18 || xgoto == 27) {                          // back from another segment
            xgoto = w_.g.g41;
            if (xgoto == 12) sheet(xgoto);
        }
        if (xgoto == XCAMP) {                                      // the Tavern's INSPECT
            sheet(4);
            res.xgoto = XCASTLE2;
            return res;
        }
        if (xgoto == 17) {                                         // the Training Grounds' INSPECT
            m_ = 0;
            sheet(17);
            res.xgoto = 19;
            return res;
        }
        for (int i = 0; i < w_.party.count(); ++i) row(i);
        if (!title_) title_ = d_.boxMsg(652, 0, 1, 1);
        for (;;) {
            d_.partyCount = w_.party.count();
            std::string spec = d_.str(653);
            Win *m = d_.centerBox(d_.menuWidth(spec), 4, 6, 1, true);
            int sel = d_.menu(m, spec);
            d_.con.close(m, false);
            if (d_.quit) { res.windowClosed = true; return res; }
            if (sel >= 0 && sel <= 5) {                            // #)INSPECT
                m_ = sel;
                sheet(12);
                if (d_.quit) { res.windowClosed = true; return res; }
            } else if (sel == 6) { res.xgoto = 15; return res; }   // R)EORDER
            else if (sel == 7) { res.xgoto = 14; res.member = -1; return res; }   // E)QUIP
            else if (sel == 8) {                                   // L)EAVE
                d_.con.refresh();
                if (title_) d_.con.close(title_, false);
                title_ = nullptr;
                w_.g.g41 = 23;
                res.xgoto = 20;
                return res;
            }
        }
    } catch (const Leave &l) {                                     // EXIT(18:1) with an XGOTO for another segment
        res.xgoto = l.xgoto;
        res.g36 = l.g36;
        res.member = m_;
        return res;
    }
}

} // namespace wiz
