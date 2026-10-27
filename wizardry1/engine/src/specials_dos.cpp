// SEGMENT SPECIALS -- see specials_dos.h.  Every routine names the p-code proc it ports.
#include "wiz/specials_dos.h"

#include <algorithm>

namespace wiz {

// ---- the message window ----------------------------------------------------------------------------

// SPECIALS:11 -- a window across the top of the view, `lines` rows of text
void Specials::openMsg(int lines) {
    Console &con = d_.con;
    lines = std::max(1, std::min(13, lines));
    if (msg_) {
        if (msg_->h == lines + 2) { con.clear(msg_, true); return; }
        con.close(msg_, false);
    }
    msg_ = con.open(0, 0, 40, lines + 2, 27, true);
}

// SPECIALS:18 -- the longest page of the message that starts at string `strNo`
int Specials::countLines(int strNo) {
    int page = 0, most = 1;
    for (;; ++strNo) {
        std::string s = d_.str(strNo);
        if (s == "**ERR**") return most;
        if (!s.empty() && s[0] == '@') page = 0;
        ++page;
        if (page > most) most = page;
    }
}

// SPECIALS:15 -- the blinking "PRESS RETURN" box
void Specials::pressReturn() {
    Console &con = d_.con;
    std::string s = d_.str(101);
    Win *b = d_.centerBox(Dos::visLen(s), msg_->y + msg_->h + 1, 3, 30, true);
    d_.putCentered(b, s);
    for (;;) {
        d_.pause(5);
        if (d_.keyReady()) {
            d_.getKey();
            if (d_.inchar == 13 || d_.quit) { con.close(b, false); return; }
        }
        if (b->hidden()) con.show(b, true);
        else con.hide(b, true);
    }
}

// SPECIALS:17
void Specials::pageBreak() {
    pressReturn();
    d_.con.clear(msg_, true);
    row_ = 0;
}

// SPECIALS:16 -- one line of the message; true once the message has run out
bool Specials::printLine() {
    std::string s = d_.str(strNo_);
    if (s == "**ERR**") return true;
    if (!s.empty() && s[0] == '@') { s.erase(0, 1); pageBreak(); }
    if (row_ == msg_->h - 2) pageBreak();
    if (s.empty() || s == " ") {
        // an empty line: nothing to position
    } else if (s[0] == '$') {                                       // flush left
        s.erase(0, 1);
        d_.gotoXY(msg_, 0, row_);
        lineLen_ = 38;
    } else if (s[0] == '^') {                                       // centred under the previous line
        s.erase(0, 1);
        d_.gotoXY(msg_, ((msg_->w - 2) / 2) - ((1 + lineLen_) / 2), row_);
    } else {
        d_.gotoXY(msg_, 0, row_);
        lineLen_ = Dos::visLen(s);
        d_.gotoXY(msg_, ((msg_->w - 2) / 2) - ((1 + lineLen_) / 2), row_);
    }
    d_.putS(msg_, s);
    ++strNo_;
    ++row_;
    return false;
}

// SPECIALS:14 -- show message number `n` (strings 15000 + 50 * n ...)
void Specials::showMessage(int n) {
    strNo_ = 15000 + 50 * n;
    row_ = 0;
    openMsg(countLines(strNo_));
    d_.con.clear(msg_, true);
    while (!printLine() && !d_.quit) {}
    pressReturn();
}

// SPECIALS:13 -- a Y/N question under the message
bool Specials::askYesNo(const std::string &q) {
    d_.newline(msg_);
    d_.putCentered(msg_, q + d_.str(100));
    std::string yes = d_.str(115), no = d_.str(116);
    do d_.getKey(); while (!d_.quit && d_.inchar != u8(yes[0]) && d_.inchar != u8(no[0]));
    return d_.inchar == u8(yes[0]);
}

// ---- helpers shared by the kinds ---------------------------------------------------------------------------

// SPECIALS:26 -- one step back the way the party came (it was not let through)
void Specials::pushBack() {
    switch (g_.dir & 3) {
        case 0: --g_.y; break;
        case 1: --g_.x; break;
        case 2: ++g_.y; break;
        default: ++g_.x; break;
    }
    g_.y = (g_.y + 20) % 20;
    g_.x = (g_.x + 20) % 20;
    if (a1_ >= 0) showMessage(a1_);
}

// SPECIALS:38 -- descriptor 0 is the "out of bounds" sign: back to where the party stood
void Specials::backOut() {
    g_.g41 = 8;
    xgoto_ = g_.g38 == g_.level ? 7 : 9;
    std::swap(g_.x, g_.g40);
    std::swap(g_.y, g_.g39);
    std::swap(g_.level, g_.g38);
    throw End10{};
}

// SPECIALS:19 -- try to hand `item` to a member; false if their pack is full or they already have it
bool Specials::giveItem(int member, int item) {
    Character &c = w_.party.member(member);
    if (c.possCount == 8) return false;
    for (int k = 0; k < c.possCount; ++k)
        if (c.poss[k].itemIndex == item) return false;
    d_.con.clear(msg_, true);
    d_.putCentered(msg_, Dos::substitute(d_.str(102), c.name));
    Possession &p = c.poss[c.possCount++];
    p = Possession{};
    p.itemIndex = item;
    return true;
}

// SPECIALS:20 (kind 2) -- an item lying about: the first member who can carry it takes it
void Specials::kindItem() {
    openMsg(1);
    bool taken = false;
    for (int i = 0; i < w_.party.count() && !taken; ++i) taken = giveItem(i, a0_);
}

// SPECIALS:21 / 22 (kind 3) -- somebody has to wade in
void Specials::kindWade() {
    openMsg(2);
    d_.putStrC(msg_, 103);
    d_.putStrC(msg_, 104);
    int m = d_.getNum(w_.party.count()) - 1;
    if (m < 0 || d_.quit) return;
    if (a0_ == -1) a0_ = w_.rng.mod(7);
    Character &c = w_.party.member(m);
    auto atLeast = [&](int st) { if (st > int(c.status)) c.status = Status(st); };     // SPECIALS:22
    switch (a0_) {
        case 0:
            if (int(c.status) < 5) {
                c.status = Status::OK;
                c.hpMax -= 8;
                c.hpLeft = c.hpMax;
                if (c.hpMax <= 0) atLeast(5);
            }
            break;
        case 1:
            if (c.attrib[IQ] == 3 || c.attrib[PIETY] == 3) {
                atLeast(5);
            } else {
                c.age -= 52;
                --c.attrib[IQ];
                --c.attrib[PIETY];
            }
            break;
        case 2: c.poison = 1; break;
        case 3: atLeast(2); break;
        case 4: atLeast(3); break;
        case 5: atLeast(4); break;
        default:
            if (c.status == Status::Dead) {
                if (w_.rng.mod(10) < 3) { c.status = Status::OK; c.hpLeft = c.hpMax; }
                else c.status = Status::Ashes;
            }
            break;
    }
}

// SPECIALS:24 / 25 (kind 4, negative AUX0 >= 20000) -- swap one item for another
void Specials::kindTrade() {
    d_.con.clear(msg_, true);
    int v = a0_ - 20000;
    int give = v / 100;
    int get = v % 100;
    for (int i = 0; i < w_.party.count(); ++i) {                    // SPECIALS:25: the first holder trades
        Character &c = w_.party.member(i);
        for (int k = 0; k < c.possCount; ++k)
            if (c.poss[k].itemIndex == give) {
                c.poss[k].itemIndex = get;
                c.poss[k].equipped = false;
                c.poss[k].identified = false;
                d_.putCentered(msg_, Dos::substitute(d_.str(105), c.name));
                d_.pause(20);
                return;                                              // EXIT(15:24)
            }
    }
    d_.putStrC(msg_, 106);
}

// SPECIALS:23 (kind 4) -- "WILL YOU SEARCH": a fight (AUX0 > 0 = monster number) or an item
void Specials::kindSearch() {
    openMsg(1);
    if (!askYesNo(d_.str(107))) throw End10{};
    if (a0_ > 0) {
        g_.g29 = 0;
        g_.g37 = a0_;
        xgoto_ = 8;
    } else {
        a0_ = -a0_;
        if (a0_ >= 20000) kindTrade();
        else kindItem();
    }
}

// SPECIALS:27 (kind 5) -- a way through only for whoever carries item AUX0
void Specials::kindNeedItem() {
    for (int i = 0; i < w_.party.count(); ++i) {
        const Character &c = w_.party.member(i);
        for (int k = 0; k < c.possCount; ++k)
            if (c.poss[k].itemIndex == a0_) throw End10{};
    }
    pushBack();
}

// SPECIALS:28 (kind 6) -- a way through only for some alignments / classes
void Specials::kindGate() {
    for (int i = 0; i < w_.party.count(); ++i) {
        int al = int(w_.party.member(i).align);
        bool stop = false;
        if (al == 1) stop = a0_ == 0 || a0_ == 2 || a0_ == 4 || a0_ == 6;
        else if (al == 2) stop = a0_ == 0 || a0_ == 1 || a0_ == 4 || a0_ == 5;
        else if (al == 3) stop = a0_ < 4;
        if (stop) pushBack();
    }
}

// SPECIALS:29 (kind 7) -- a field of light or of protection
void Specials::kindLight() {
    if (a0_ == 99) g_.light += 50;
    else if (a0_ == -99) g_.light = 0;
    else g_.protect = a0_;
}

// SPECIALS:30 (kind 8) -- up and out
void Specials::kindToTown() {
    g_.level = 0;
    xgoto_ = 9;
}

// SPECIALS:31 / 32 (kind 10) -- a riddle
void Specials::kindRiddle() {
    openMsg(2);
    d_.putStrC(msg_, 108);
    d_.gotoXY(msg_, 4, 1);
    std::string answer = d_.getLine(msg_, 38);
    int n = 15000 + 50 * a0_;
    d_.con.clear(msg_, true);
    for (;; ++n) {
        std::string s = d_.str(n);
        if (s == "**ERR**") break;
        bool right = false;
        if (!s.empty() && s[0] == '$') {
            s.erase(0, 1);
            right = answer.find(s) != std::string::npos;
        } else {
            right = s == answer;
        }
        if (right) {
            d_.putStrC(msg_, 110);                                // SPECIALS:32 (CORRECT!)
            return;
        }
    }
    a1_ = -1;
    d_.putStrC(msg_, 109);
    pushBack();
}

// SPECIALS:33..36 (kind 11) -- a toll; the first line of message AUX0 holds the fee (and the
// destination letter), the descriptor it names says where the toll buys passage to
void Specials::kindToll() {
    openMsg(4);
    std::string fee = d_.str(15000 + 50 * a0_);
    int dest = 0;                                                   // SPECIALS:34
    if (!fee.empty() && u8(fee[0]) >= 64) { dest = fee[0] - 65 + 1; fee.erase(0, 1); }
    long long amount = 0;
    for (char ch : fee) amount = amount * 10 + (ch - '0');
    d_.con.clear(msg_, true);
    d_.putCentered(msg_, d_.str(113) + fee);
    a1_ = -1;
    if (!askYesNo(d_.str(114))) {
        if (dest == 0) pushBack();
        throw End10{};
    }
    d_.con.clear(msg_, true);
    long long have = 0;                                             // SPECIALS:35
    for (int i = 0; i < w_.party.count(); ++i) have += w_.party.member(i).gold.v;
    if (have < amount) {
        d_.putStrC(msg_, 111);
        if (dest == 0) pushBack();
        throw End10{};
    }
    for (int i = 0; i < w_.party.count(); ++i) {                    // SPECIALS:36
        if (amount == 0) break;
        Character &c = w_.party.member(i);
        if (amount > c.gold.v) { amount -= c.gold.v; c.gold.v = 0; }
        else { c.gold.v -= amount; amount = 0; }
    }
    d_.putStrC(msg_, 112);
    if (dest > 0) {
        idx_ = dest;
        std::vector<u8> &rec = mazeRecord(g_, w_.sc, g_.level);
        MazeLevel lv;
        lv.load(Bytes{rec.data(), rec.size()});
        g_.x = lv.aux2(dest);
        g_.y = lv.aux1(dest);
        g_.level = lv.aux0(dest);
        xgoto_ = 9;
    }
}

// SPECIALS:37 (kind 9) -- a nest: every room within AUX0 squares is marked unfought
void Specials::kindNest() {
    for (int dx = -a0_; dx <= a0_; ++dx)
        for (int dy = -a0_; dy <= a0_; ++dy)
            g_.fight[(g_.x + dx + 20) % 20][(g_.y + dy + 20) % 20] = true;
    g_.fight[g_.x][g_.y] = false;
}

// SPECIALS:10 -- the square's handler
void Specials::scnmsg() {
    std::vector<u8> &rec = mazeRecord(g_, w_.sc, g_.level);
    MazeLevel lv;
    lv.load(Bytes{rec.data(), rec.size()});
    idx_ = g_.g9;
    if (idx_ == 0) backOut();
    g_.g41 = 23;
    a2_ = lv.aux2(idx_);
    a1_ = lv.aux1(idx_);
    a0_ = lv.aux0(idx_);
    xgoto_ = 7;
    if (a2_ == 0) return;
    if (a2_ == 1 || a2_ == 4 || a2_ == 8) {                        // limited-use squares
        if (a0_ == 0) return;
        auto setAux0 = [&](int v) { rec[(384 + (idx_ & 15)) * 2] = u8(v & 0xFF); rec[(384 + (idx_ & 15)) * 2 + 1] = u8((v >> 8) & 0xFF); };
        if (a2_ != 4) {
            if (a0_ > 0) setAux0(a0_ - 1);
            if (a0_ == 1) {                                        // used up: the square is plain again
                int wd = 380 + (idx_ & 15) / 4, sh = ((idx_ & 15) % 4) * 4;
                u16 old = rd16(rec.data() + wd * 2);
                old = u16(old & ~(0xF << sh));
                rec[wd * 2] = u8(old & 0xFF); rec[wd * 2 + 1] = u8(old >> 8);
            }
        } else if (a0_ < 0) {
            if (a0_ > -1000) setAux0(0);
            else if (a0_ > -20000) a0_ += 1000;
        }
    }
    try {
        if (!(a2_ > 12 || a2_ == 5 || a2_ == 6)) showMessage(a1_);
        switch (a2_) {
            case 2: kindItem(); break;
            case 3: kindWade(); break;
            case 4: kindSearch(); break;
            case 5: kindNeedItem(); break;
            case 6: kindGate(); break;
            case 7: kindLight(); break;
            case 8: kindToTown(); break;
            case 9: kindNest(); break;
            case 10: kindRiddle(); break;
            case 11: kindToll(); break;
            default: break;
        }
    } catch (const End10 &) {
    }
    if (msg_) d_.con.close(msg_, false);
}

// ---- I)NSPECT (SPECIALS:2..8) --------------------------------------------------------------------------------------

// SPECIALS:7 -- the squares the party can walk to through open edges
void Specials::markRegion(bool (&reach)[20][20]) {
    std::vector<u8> &rec = mazeRecord(g_, w_.sc, g_.level);
    MazeLevel lv;
    lv.load(Bytes{rec.data(), rec.size()});
    d_.putStr(msg_, 10);
    bool done[20][20] = {};
    reach[g_.x][g_.y] = true;
    for (;;) {
        d_.putC(msg_, '.');
        bool finished = true;
        for (int x = 0; x < 20; ++x)
            for (int y = 0; y < 20; ++y)
                if (reach[x][y] && !done[x][y]) {
                    auto take = [&](int nx, int ny, int wall) {            // SPECIALS:8
                        if (wall != 0) return;
                        nx = (nx + 20) % 20; ny = (ny + 20) % 20;
                        if (reach[nx][ny]) return;
                        finished = false;
                        reach[nx][ny] = true;
                    };
                    take(x + 1, y, int(lv.wall(x, y, EAST)));
                    take(x - 1, y, int(lv.wall(x, y, WEST)));
                    take(x, y - 1, int(lv.wall(x, y, SOUTH)));
                    take(x, y + 1, int(lv.wall(x, y, NORTH)));
                    done[x][y] = true;
                }
        if (finished) return;
    }
}

void Specials::inspect() {
    Console &con = d_.con;
    msg_ = con.open(9, 2, 22, 16, 7, true);
    bool reach[20][20] = {};
    markRegion(reach);
    // SPECIALS:4 -- who lies here
    int found[5];
    int n = 0;
    con.clear(msg_, true);
    d_.putStr(msg_, 6);
    d_.gotoXY(msg_, 0, 2);
    for (int i = 0; i < w_.roster.count(); ++i) {
        const Character &c = w_.roster.slot(i);
        if (c.status != Status::Lost && !c.inMaze && c.lostLevel == g_.level &&
            c.lostY / 100 >= 0 && c.lostY / 100 < 20 && reach[c.lostY / 100][c.lostY % 100] && n < 5) {
            found[n++] = i;                                            // SPECIALS:5
            d_.putNum(msg_, n, 1);
            d_.putS(msg_, ") ");
            d_.putS(msg_, c.name);
            d_.newline(msg_);
        }
    }
    if (n == 0) {
        d_.putStr(msg_, 7);
        con.close(msg_, false);                                        // SPECIALS:3
        xgoto_ = 7;
        throw EndAll{};
    }
    for (;;) {
        d_.gotoXY(msg_, 0, 12);
        con.clearRect(msg_, 0, 12, 20, 2);
        int sel = d_.menuStr(msg_, 11);
        if (d_.quit) throw EndAll{};
        if (sel == 0) {
            if (w_.party.count() < 6) {                                // SPECIALS:6: G)ET
                con.clearRect(msg_, 0, 12, 20, 2);
                d_.gotoXY(msg_, 0, 12);
                d_.putStr(msg_, 8);
                d_.newline(msg_);
                d_.putStr(msg_, 9);
                d_.gotoXY(msg_, 5, 12);
                int k = d_.getNum(n);
                if (k > 0 && found[k - 1] != -1) {
                    int slot = found[k - 1];
                    Character c = w_.roster.slot(slot);
                    const Character &last = w_.party.member(w_.party.count() - 1);
                    c.lostY = last.lostY;
                    c.lostLevel = last.lostLevel;
                    c.inMaze = true;
                    w_.roster.slot(slot) = c;
                    w_.party.appendRaw(c, slot);
                    found[k - 1] = -1;
                    con.clearRect(msg_, 0, 1 + k, 20, 1);
                }
            } else {
                d_.sound(0);
            }
        } else {                                                       // l)EAVE
            con.close(msg_, false);
            xgoto_ = 7;
            throw EndAll{};
        }
    }
}

// SPECIALS:9 -- DUMAPIC: where the party is
void Specials::position() {
    Console &con = d_.con;
    xgoto_ = 27;
    Win *b = con.open(1, 7, 38, 9, 16, true);
    d_.putStrC(b, 21);
    d_.newline(b);
    d_.putCentered(b, Dos::substitute(d_.str(22), d_.str(23 + g_.dir)));
    d_.newline(b);
    std::string s = d_.str(27);
    auto sub = [&](char ch, int v) {                               // WIZARDRY:56
        size_t p = s.find(ch);
        if (p != std::string::npos) s = s.substr(0, p) + Dos::itoa(v) + s.substr(p + 1);
    };
    sub(char(33), g_.x);
    sub(char(34), g_.y);
    sub(char(35), g_.level);
    d_.putCentered(b, s);
    d_.newline(b);
    d_.putStrC(b, 250);
    con.close(b, false);
    d_.waitCR();
    d_.inchar = 65;
    throw EndAll{};
}

// SPECIALS:1
SpecialsResult Specials::run(int xgoto) {
    SpecialsResult res;
    msg_ = nullptr;
    xgoto_ = 7;
    try {
        if (xgoto == 26) inspect();
        else if (xgoto == 24) position();
        xgoto_ = g_.g41;                                           // XGOTO := G41
        scnmsg();
    } catch (const EndAll &) {
    } catch (const End10 &) {
    }
    res.xgoto = xgoto_;
    res.windowClosed = d_.quit;
    return res;
}

} // namespace wiz
