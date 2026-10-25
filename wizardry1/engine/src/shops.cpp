#include "wiz/shops.h"

#include <algorithm>

namespace wiz {

static const long long kMaxGold = 999999999999LL;

void Shops::save() {
    if (!w_.rosterPath.empty()) w_.roster.save(w_.rosterPath);
    if (!w_.partyPath.empty()) w_.party.save(w_.partyPath);
    if (!w_.shopPath.empty()) w_.shop.save(w_.shopPath);
}

// WIZARDRY:44 -- pool every member's gold into `member`
static void poolGold(Party &party, int member) {
    long long sum = 0;
    for (int i = 0; i < party.count(); ++i) {
        sum = std::min(kMaxGold, sum + party.member(i).gold.v);
        party.member(i).gold.v = 0;
    }
    party.member(member).gold.v = sum;
}

// ---- Edge of Town (SHOPS:30) -----------------------------------------------------------
ShopsResult Shops::edgeOfTown() {
    Console &con = d_.con;
    ShopsResult res;
    int sel;
    for (;;) {
        d_.partyCount = w_.party.count();
        std::string spec = d_.str(502);
        Win *m = d_.centerBox(d_.menuWidth(spec), 4, 7, 6, true);
        sel = d_.menu(m, spec);
        con.close(m, true);
        if (d_.quit) { res.windowClosed = true; return res; }
        if (!(sel == 1 && w_.party.count() == 0)) break;
    }
    switch (sel) {
        case 0: castle_.closeWindows(); res.xgoto = XROLLER; break;
        case 1: enterMaze(); res.xgoto = XMAZE_ENTRY; return res;
        case 2: res.xgoto = XCASTLE_FROM_EDGE; return res;
        case 3: castle_.closeWindows(); res.xgoto = XUTILITIES; break;
        default: castle_.closeWindows(); res.xgoto = XQUIT; break;
    }
    disbandParty();                                                  // SHOPS:39
    return res;
}

void Shops::enterMaze() {                                            // SHOPS:31
    Console &con = d_.con;
    castle_.closeTitleWindow();
    Win *w = con.open(0, 10, 40, 4, 6, true);
    d_.putStrC(w, 500);
    d_.putStrC(w, 501);
    con.close(w, false);
    for (int i = 0; i < w_.party.count(); ++i) recalcSpellCounts(w_.party.member(i));   // SHOPS:32
    w_.party.writeBack(w_.roster);
    save();
}

void Shops::disbandParty() {                                         // SHOPS:39
    for (int i = 0; i < w_.party.count(); ++i) {
        Character &c = w_.party.member(i);
        c.inMaze = false;
        c.lostY = 0;
        c.lostLevel = 0;
    }
    w_.party.disband(w_.roster);
    save();
}

// ---- back from the maze ---------------------------------------------------------------------
void Shops::leaveMaze(Win *&g10) {                                   // SHOPS:41
    Console &con = d_.con;
    Party &party = w_.party;
    con.show(g10, false);
    bool amulet = false;
    for (int i = 0; i < party.count(); ++i) {
        Character &c = party.member(i);
        for (int k = 0; k < c.possCount; ++k)
            if (c.poss[k].itemIndex == 94) { c.poss[k].itemIndex = 0; amulet = true; }
        c.poison = 0;                                                // LOSTXYL words 100..102 := 0
        c.lostX = c.lostY = c.lostLevel = 0;
    }
    if (amulet) victory(g10);
    for (int i = 0; i < party.count(); ++i) party.member(i).inMaze = party.member(i).status == Status::OK;
    party.writeBack(w_.roster);
    party.keepOnlyOk();
    save();
}

void Shops::victory(Win *&g10) {                                     // SHOPS:42
    Console &con = d_.con;
    con.close(g10, false);
    con.clear(d_.window1, true);
    int n = 1600;
    while (d_.str(n) != "!END!") d_.putCentered(d_.window1, d_.str(n++));
    d_.sound(7);
    d_.gotoXY(d_.window1, 0, 19);
    d_.putStrC(d_.window1, n + 1);
    d_.waitCR();
    for (int i = 0; i < w_.party.count(); ++i) {
        Character &c = w_.party.member(i);
        c.exp.v += 50000;
        c.gold.v = std::min(kMaxGold, c.gold.v + 50000);
        c.raw[103 * 2] |= 1;                                         // AWARDS: the Amulet bit
    }
}

void Shops::partyWiped(Win *&g10, int level, int x, int y) {         // SHOPS:27
    Console &con = d_.con;
    Party &party = w_.party;
    for (int i = 0; i < party.count(); ++i) {                        // SHOPS:28
        Character &c = party.member(i);
        if (int(c.status) < 5) c.status = Status::Dead;
        c.inMaze = false;
        c.gold.v /= 2;
        for (int k = 0; k < c.possCount; ++k)                        // SHOPS:29: unlucky gear is lost
            if (!c.poss[k].cursed && w_.rng.mod(21) > c.attrib[LUCK]) c.poss[k].itemIndex = 0;
        int kept = 0;
        for (int k = 0; k < c.possCount; ++k)
            if (c.poss[k].itemIndex > 0) c.poss[kept++] = c.poss[k];
        c.possCount = kept;
        if (w_.rng.mod(50) < level) {
            c.status = Status::Lost;
        } else {
            c.lostY = 100 * x + y;
            c.lostLevel = level;
        }
    }
    party.writeBack(w_.roster);
    con.close(g10, false);
    con.clear(d_.window1, true);
    Win *b = con.open(0, 20, 40, 4, 1, true);
    d_.putStrC(b, 510);
    d_.putStrC(b, 511);
    const u8 pics[4] = {3, 3, 3, 3};
    con.blit13(d_.window1, pics);
    con.present(d_.window1);
    con.close(b, false);
    d_.waitCR();
    party.disbandQuiet();
    save();
}

// ---- Temple of Cant (SHOPS:2) -------------------------------------------------------------
void Shops::temple() {
    Console &con = d_.con;
    Win *win = con.open(2, 4, 36, 8, 6, true);
    for (;;) {
        w_.party.writeBack(w_.roster);                               // SHOPS:12
        if (!templeSelect(win)) { con.close(win, false); return; }   // SHOPS:6 (CR leaves)
        if (d_.quit) return;
        templeTreat(win);                                            // SHOPS:8
        w_.party.resyncFromRoster(w_.roster);                        // SHOPS:13
        save();
        if (d_.quit) return;
    }
}

bool Shops::templeSelect(Win *win) {                                 // SHOPS:6 / 7
    Console &con = d_.con;
    con.clear(win, true);
    Win *list = con.open(0, 9, 40, 12, 31, true);
    int slotOf[20];
    std::fill(slotOf, slotOf + 20, -1);
    int n = 0;
    for (int r = 0; r < w_.roster.count(); ++r) {
        const Character &c = w_.roster.slot(r);
        d_.gotoXY(list, 19 * (n / 10), n % 10);
        if (c.status != Status::Lost && c.lostLevel == 0 && c.status != Status::OK) {
            d_.putC(list, 65 + n);
            d_.putS(list, ") ");
            d_.putS(list, c.name);
            slotOf[n++] = r;
        }
    }
    con.close(list, false);
    d_.putStrC(win, 512);
    d_.putStrC(win, 513);
    d_.putStrC(win, 514);
    for (;;) {
        d_.getKey();
        if (d_.quit) return false;
        if (d_.inchar == 13) return false;                           // SHOPS:3
        int l = d_.inchar - 65;
        if (l >= 0 && l <= 19 && slotOf[l] != -1) {
            patient_ = slotOf[l];
            patientRec_ = w_.roster.slot(patient_);
            con.refresh();
            return true;
        }
    }
}

void Shops::templeMessage(Win *win, const std::string &name, int strNo) {   // SHOPS:5
    d_.gotoXY(win, 0, 5);
    std::string s = Dos::substitute(d_.str(strNo), name);
    d_.putCentered(win, "* " + s + " *");
    d_.pause(20);
}

bool Shops::templePay(Win *win) {                                    // SHOPS:9 (GETPAYER)
    Character &p = patientRec_;
    long long fee = 0;
    switch (int(p.status)) {
        case 3: fee = 100; break;
        case 4: fee = 200; break;
        case 5: fee = 250; break;
        case 6: fee = 500; break;
        default: break;
    }
    fee *= p.charLevel;
    d_.con.clear(win, true);
    d_.putStr(win, 519);
    d_.putS(win, Dos::long2str(fee));
    d_.newline(win);
    d_.newline(win);
    d_.putStrC(win, 520);
    d_.putStrC(win, 521);
    payer_ = d_.getNum(w_.party.count()) - 1;
    if (payer_ < 0) return false;                                    // EXIT(14:4)
    Character &pay = w_.party.member(payer_);
    if (fee > pay.gold.v) {
        poolGold(w_.party, payer_);
        if (fee > pay.gold.v) {
            templeMessage(win, "", 522);
            return false;
        }
    }
    pay.gold.v = std::max<long long>(0, pay.gold.v - fee);
    return true;
}

void Shops::templeTreat(Win *win) {                                  // SHOPS:8 + 10 + 11
    if (!templePay(win)) return;
    Console &con = d_.con;
    Character &p = patientRec_;
    con.clear(win, true);
    d_.putStr(win, 525); d_.pause(20);
    d_.putStr(win, 526); d_.pause(20);
    d_.putStr(win, 527); d_.pause(20);
    d_.putStr(win, 528);
    d_.newline(win);
    auto worsen = [&] {                                              // SHOPS:11
        p.status = (p.status == Status::Dead) ? Status::Ashes : Status::Lost;
        w_.roster.slot(patient_) = p;
        templeMessage(win, p.name, p.status == Status::Lost ? 523 : 524);
    };
    auto roll = [&](int base) {
        int r = w_.rng.mod(100);
        return r > base + 3 * p.attrib[VIT];
    };
    if (p.status == Status::Dead) {
        if (roll(50)) { worsen(); return; }
        p.hpLeft = 1;
    } else if (p.status == Status::Ashes) {
        if (roll(40)) { worsen(); return; }
        p.hpLeft = p.hpMax;
    }
    p.age += w_.rng.mod(52) + 1;
    p.lostX = p.lostY = p.lostLevel = 0;
    p.poison = 0;
    p.status = Status::OK;
    w_.roster.slot(patient_) = p;
    d_.newline(win);
    templeMessage(win, p.name, 529);
}

// ---- Boltac's Trading Post (SHOPS:14) --------------------------------------------------------
void Shops::boltac() {
    Console &con = d_.con;
    Win *win = con.open(0, 2, 40, 14, 8, true);
    for (;;) {
        con.clear(win, true);
        d_.putStrC(win, 577);
        d_.newline(win);
        d_.putStrC(win, 578);
        d_.putStrC(win, 579);
        int who = d_.getNum(w_.party.count()) - 1;
        if (who < 0 || d_.quit) { con.close(win, false); return; }
        boltacMember(win, who);
        if (d_.quit) return;
    }
}

void Shops::boltacMember(Win *win, int m) {                          // SHOPS:15
    for (;;) {
        Character &c = w_.party.member(m);
        d_.con.clear(win, true);
        d_.putS(win, Dos::substitute(d_.str(573), c.name));
        d_.newline(win);
        d_.putStr(win, 574);
        d_.putS(win, Dos::long2str(c.gold.v));
        d_.putStr(win, 575);
        d_.newline(win);
        d_.newline(win);
        int g = d_.menuStr(win, 576);
        if (d_.quit) return;
        switch (g) {
            case 0: buy(win, m); break;
            case 1: trade(win, m, 0); break;
            case 2: trade(win, m, 1); break;
            case 3: trade(win, m, 2); break;
            case 4: poolGold(w_.party, m); break;
            default: return;                                         // EXIT(14:15)
        }
        save();
        if (d_.quit) return;
    }
}

void Shops::buyRow(Win *win, int m, int row, int dir) {             // SHOPS:17
    int nobj = w_.sc.count(Scenario::Object);
    d_.con.clearRect(win, 0, row - 1, 38, 1);
    d_.gotoXY(win, 0, row - 1);
    for (;;) {
        cursor_ += dir;
        if (cursor_ >= nobj) cursor_ = 1;
        else if (cursor_ < 1) cursor_ = nobj - 1;
        ObjectRec rec{w_.sc.record(Scenario::Object, cursor_)};
        int n = w_.shop.stock(cursor_);
        if (n > -2 && !rec.cursed() && n != 0) break;
    }
    ObjectRec rec{w_.sc.record(Scenario::Object, cursor_)};
    shown_[row - 1] = cursor_;
    d_.putNum(win, row, 1);
    d_.putC(win, ')');
    d_.putStr(win, 14000 + cursor_ + cursor_ + 1);
    d_.gotoXY(win, 15 + 3, row - 1);
    d_.putPadded(win, rec.price().value());
    if (!rec.classUse(int(w_.party.member(m).cls))) d_.putStr(win, 550);
}

void Shops::boltacMsg(Win *win, int strNo, int row) {                // SHOPS:21 (row 9) / 26 (row 11)
    d_.gotoXY(win, 0, row);
    d_.putCentered(win, "* " + d_.str(strNo) + " *");
    d_.pause(20);
}

void Shops::buy(Win *win, int m) {                                   // SHOPS:16
    Console &con = d_.con;
    cursor_ = 1;
    bool redraw = true;
    int dir = 1;
    shown_[0] = 1;
    shown_[5] = 1;
    con.clear(win, true);
    for (;;) {
        if (redraw) {
            if (dir == 1) {                                          // SHOPS:18
                cursor_ = shown_[5] - 1;
                for (int r = 1; r <= 6; ++r) buyRow(win, m, r, 1);
            } else {                                                 // SHOPS:19
                cursor_ = shown_[0] + 1;
                for (int r = 6; r >= 1; --r) buyRow(win, m, r, -1);
            }
        }
        redraw = true;
        dir = 1;
        con.clearRect(win, 0, 7, 38, 5);
        d_.gotoXY(win, 0, 7);
        d_.putS(win, Dos::long2str(w_.party.member(m).gold.v));
        d_.putStr(win, 561);
        d_.gotoXY(win, 0, 10);
        int g = d_.menuStr(win, 562);
        if (d_.quit) return;
        con.clearRect(win, 0, 7, 38, 5);
        switch (g) {
            case 0: redraw = false; purchase(win, m); break;         // SHOPS:20 (sets L1 := 0)
            case 2: dir = -1; break;
            case 3: shown_[5] = 1; break;
            default: break;
        }
        if (g == 4) return;
    }
}

void Shops::restock(int obj) {                                       // SHOPS:22
    int nobj = w_.sc.count(Scenario::Object);
    int l = obj;
    do {
        ++l;
        if (l == nobj) l = 0;
        int n = w_.shop.stock(l);
        if (n < -1) { w_.shop.setStock(l, std::abs(n)); return; }
    } while (l != obj);
}

void Shops::purchase(Win *win, int m) {                              // SHOPS:20 (AASTRAA)
    Character &c = w_.party.member(m);
    d_.gotoXY(win, 0, 7);
    d_.putStr(win, 551);
    int row = d_.getNum(6);
    if (d_.quit) return;
    if (row == 0) { boltacMsg(win, 552, 9); d_.con.clearRect(win, 0, 7, 38, 3); return; }
    auto bail = [&](int strNo) {                                     // SHOPS:21
        boltacMsg(win, strNo, 9);
        d_.con.clearRect(win, 0, 7, 38, 3);
    };
    int obj = shown_[row - 1];
    ObjectRec rec{w_.sc.record(Scenario::Object, obj)};
    int stock = w_.shop.stock(obj);
    if (stock == 0) { bail(553); return; }
    if (c.possCount == 8) { bail(554); return; }
    if (c.gold.v < rec.price().value()) { bail(555); return; }
    if (!rec.classUse(int(c.cls))) {
        d_.gotoXY(win, 0, 8);
        d_.putStr(win, 556);
        std::string yes = d_.str(557), no = d_.str(558);
        do {
            d_.getKey();
            if (d_.quit) return;
        } while (d_.inchar != (unsigned char)yes[0] && d_.inchar != (unsigned char)no[0]);
        if (d_.inchar == (unsigned char)no[0]) { bail(559); return; }
    } else {
        d_.inchar = 32;
    }
    c.gold.v = std::max<long long>(0, c.gold.v - rec.price().value());
    Possession &p = c.poss[c.possCount];
    p.equipped = false;
    p.identified = true;
    p.cursed = false;
    p.itemIndex = obj;
    ++c.possCount;
    if (stock > 0) w_.shop.setStock(obj, stock - 1);
    if (w_.shop.stock(obj) == 0) restock(obj);
    bail(560);
}

// SHOPS:24 -- the member's possessions, numbered, with sale prices
bool Shops::listPossessions(Win *win, int m, int mode) {
    Character &c = w_.party.member(m);
    d_.con.clear(win, true);
    possCount_ = c.possCount;
    for (int i = 1; i <= possCount_; ++i) {
        const Possession &p = c.poss[i - 1];
        possShown_[i - 1] = p.itemIndex;
        ObjectRec rec{w_.sc.record(Scenario::Object, p.itemIndex)};
        d_.putNum(win, i, 1);
        d_.putC(win, ')');
        if (p.equipped) d_.putC(win, p.cursed ? '-' : '*');
        else if (p.identified) d_.putC(win, rec.classUse(int(c.cls)) ? ' ' : '#');
        else d_.putC(win, '?');
        d_.putStr(win, 14000 + 2 * p.itemIndex + (p.identified ? 1 : 0));
        d_.gotoXY(win, 15 + 4, win->cy);
        long long price = rec.price().value() / 2;                   // LONG2BCD(price, HALFPRIC)
        if (mode == 0) {
            if (!p.identified) price = 0;
        } else if (price == 0) {
            price = 15LL * 10000;                                    // mid limb := 15 (as shipped)
        }
        d_.putPadded(win, price);
        d_.newline(win);
    }
    return true;
}

void Shops::trade(Win *win, int m, int mode) {                       // SHOPS:23
    listPossessions(win, m, mode);
    for (;;) {
        if (possCount_ == 0) return;
        d_.gotoXY(win, 0, 9);
        d_.con.clearRect(win, 0, 9, 38, 3);
        d_.putStr(win, mode == 0 ? 569 : mode == 1 ? 570 : 571);
        d_.putStr(win, 572);
        int sel = d_.getNum(possCount_);
        if (sel == 0 || d_.quit) return;
        transact(win, m, mode, sel);
        if (d_.quit) return;
    }
}

void Shops::transact(Win *win, int m, int mode, int sel) {           // SHOPS:25
    Character &c = w_.party.member(m);
    Possession &p = c.poss[sel - 1];
    ObjectRec rec{w_.sc.record(Scenario::Object, possShown_[sel - 1])};
    long long price = rec.price().value() / 2;
    auto bail = [&](int strNo) { boltacMsg(win, strNo, 11); };       // SHOPS:26 (EXIT 25)
    if (mode == 1 || mode == 2) {
        if (price == 0) price = 15LL * 10000;
    }
    if (mode == 0) {
        if (!p.identified) price = 0;
        if (price == 0) { bail(563); return; }
        if (p.cursed) { bail(564); return; }
        if (p.equipped) {
            d_.gotoXY(win, 0, 8);
            d_.putStr(win, 580);
            std::string yn = d_.str(581);
            do {
                d_.getKey();
                if (d_.quit) return;
            } while (d_.inchar != (unsigned char)yn[0] && d_.inchar != (unsigned char)yn[1]);
            if (d_.inchar == (unsigned char)yn[1]) { bail(559); return; }
        }
    } else {
        if (!p.cursed && mode == 1) { bail(565); return; }
        if (p.identified && mode == 2) { bail(566); return; }
        if (c.gold.v < price) { bail(567); return; }
    }
    if (mode == 0) c.gold.v = std::min(kMaxGold, c.gold.v + price);
    else c.gold.v = std::max<long long>(0, c.gold.v - price);
    if (mode == 2) {
        p.identified = true;
    } else {
        for (int i = sel; i < c.possCount; ++i) c.poss[i - 1] = c.poss[i];
        --c.possCount;
        if (mode == 0) {
            int obj = possShown_[sel - 1];
            int n = w_.shop.stock(obj);
            if (n < -1) w_.shop.setStock(obj, 1 - n);
            else if (n > -1) w_.shop.setStock(obj, n + 1);
        }
    }
    d_.gotoXY(win, 0, 11);
    d_.putStrC(win, 568);
    d_.pause(20);
    listPossessions(win, m, mode);
}

} // namespace wiz
