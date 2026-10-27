// SEGMENT REWARDS -- see rewards_dos.h.  Every routine names the p-code proc it ports;
// `tools/pcode_decomp.py REWARDS <n>` prints the original.
#include "wiz/rewards_dos.h"

#include <algorithm>

namespace wiz {

namespace {
const long long kMaxLong = 99999999999999LL;            // ADDLONGS saturates at 9999 / 9999 / 9999
long long addLongs(long long a, long long b) { return std::min(kMaxLong, a + b); }
}

// ---- helpers ---------------------------------------------------------------------------------------------

// REWARDS:29 -- count dice of `sides` plus `add`
int Rewards::rollDice(int count, int sides, int add) {
    int total = add;
    while (count > 0) {
        if (sides > 0) total = total + w_.rng.mod(sides) + 1;
        --count;
    }
    return total;
}

// REWARDS:2 -- "WHO (#) WILL <verb> ?"; the member's index, or -1
int Rewards::pickMember(int verbStr) {
    d_.con.clearRect(back_, 0, 2, 38, 4);
    d_.gotoXY(back_, 0, 2);
    d_.putCentered(back_, Dos::substitute(d_.str(800), d_.str(verbStr)));
    d_.putStrC(back_, 801);
    return d_.getNum(w_.party.count()) - 1;
}

// REWARDS:3 -- one picture (the chest, the treasure) in the view, or none
void Rewards::picture(int pic) {
    u8 none[4] = {255, 255, 255, 255};
    d_.con.clearAttr(d_.window1);
    d_.con.present(d_.window1);
    u8 one[4] = {u8(pic), 255, 255, 255};
    d_.con.blit13(d_.window1, one);
    d_.con.present(d_.window1);
    (void)none;
}

// REWARDS:4 / 7 -- which treasure record the monster carries
void Rewards::loadRewardRecord() {
    std::vector<u8> &mon = monsterRecord(g_, w_.sc, g_.g37);
    int word42 = int(i16(rd16(mon.data() + 42 * 2)));
    if (word42 > 0) {                                                  // a unique monster is used up
        --word42;
        mon[84] = u8(word42 & 0xFF); mon[85] = u8((word42 >> 8) & 0xFF);
    }
    kind_ = 1;
    int r1 = int(i16(rd16(mon.data() + 36 * 2))), r2 = int(i16(rd16(mon.data() + 37 * 2)));
    if (g_.g29 == 0) rewardIdx_ = r1;
    else if (g_.g29 == 1) { rewardIdx_ = r1; kind_ = 2; }
    else rewardIdx_ = r2;
    Bytes b = w_.sc.record(Scenario::Reward, rewardIdx_);                // REWARDS:7
    for (int k = 0; k < 84; ++k) rec_[k] = k * 2 + 1 < int(b.size()) ? int(i16(rd16(b.p + k * 2))) : 0;
}

// REWARDS:5 FOUNDITM
void Rewards::foundItem(int member, int item) {
    int items = w_.sc.count(Scenario::Object);
    if (item >= items) item = items - 1;
    Character &c = w_.party.member(member);
    Win *b = d_.con.open(0, 2, 40, 3, 31, true);
    std::string s = Dos::substitute(d_.str(802), c.name);
    s = Dos::substituteChar(s, 35, d_.str(14000 + 2 * item));
    d_.putCentered(b, s);
    d_.con.close(b, false);
    Possession &p = c.poss[c.possCount++];
    p = Possession{};
    p.itemIndex = item;
}

// ---- gold and items (REWARDS:28..32) -------------------------------------------------------------------------------

// REWARDS:30 -- (dice1 * multiplier) * dice2 * kind
void Rewards::dropGold(const int *t) {
    long long amount = rollDice(t[2], t[3], t[4]);
    amount *= t[5];
    long long second = rollDice(t[6], t[7], t[8]);
    amount *= second;
    amount *= kind_;
    gold_ = addLongs(gold_, amount);
}

// REWARDS:31 -- an item: the first member with room takes it
void Rewards::dropItem(const int *t) {
    int n = w_.party.count();
    int first = w_.rng.mod(n), m = first;
    int stop = first;
    do {
        Character &c = w_.party.member(m);
        if (c.status == Status::OK && c.possCount != 8) {
            int steps = 0;
            while (rollDice(1, 100, 0) <= t[6] && steps < t[4]) ++steps;
            int item = t[2] + t[3] * steps;
            if (t[5] > 0) item += w_.rng.mod(t[5]);
            int items = w_.sc.count(Scenario::Object);
            if (item >= items) item = items - 1;
            foundItem(m, item);
            d_.pause(20);
            return;
        }
        m = (m + 1) % n;
    } while (m != stop);
}

// REWARDS:28
void Rewards::dropOne(const int *t) {
    if (t[0] < w_.rng.mod(100)) return;
    if (t[1] == 0) dropGold(t);
    else dropItem(t);
}

// REWARDS:32 DROPLEVL -- the gold is shared among the survivors
void Rewards::shareGold() {
    long long share = alive_ > 0 ? gold_ / alive_ : 0;
    Win *b = d_.con.open(0, 2, 40, 3, 31, true);
    d_.putCentered(b, d_.str(827) + Dos::long2str(share) + d_.str(828));
    d_.con.close(b, false);
    for (int i = 0; i < w_.party.count(); ++i) {
        Character &c = w_.party.member(i);
        if (c.status == Status::OK) c.gold.v = addLongs(c.gold.v, share);
    }
    d_.pause(20);
}

// ---- the chest -------------------------------------------------------------------------------------------------------------

// REWARDS:12 PRTRAPTY
void Rewards::trapName(std::string &out, int type, int variant) const {
    switch (type) {
        case 0: out = d_.str(803); break;
        case 1: out = d_.str(804); break;
        case 2: out = d_.str(805); break;
        case 3: out = d_.str(806 + (variant < 0 ? 0 : variant > 2 ? 2 : variant)); break;
        case 4: out = d_.str(809); break;
        case 5: out = d_.str(810); break;
        case 6: out = d_.str(811); break;
        default: out = d_.str(812); break;
    }
}

// REWARDS:13 -- the trap's name, centred, and a beat
void Rewards::showTrap(int type, int variant) {
    std::string s;
    trapName(s, type, variant);
    d_.putCentered(back_, s);
    d_.pause(20);
}

// REWARDS:9 -- the chest is dealt with
void Rewards::chestDone() {
    d_.con.close(back_, true);
    throw EndProc{8};
}

// REWARDS:15 -- a trap does damage to one member
void Rewards::damage(int member, int count, int sides) {
    if (sides < 1) sides = 1;
    int total = 0;
    while (count > 0) { total = total + w_.rng.mod(sides) + 1; --count; }
    Character &c = w_.party.member(member);
    c.hpLeft -= total;
    if (c.hpLeft < 1) {
        c.hpLeft = 0;
        c.status = Status::Dead;
        d_.con.clear(back_, true);
        d_.gotoXY(back_, 0, 1);
        d_.putCentered(back_, Dos::substitute(d_.str(813), c.name));
        d_.pause(20);
    }
}

// REWARDS:20 -- a blast that may hit each member (and half-hit on a second chance)
void Rewards::groupDamage(int chance, int count, int sides) {
    for (int i = 0; i < w_.party.count(); ++i) {
        if (w_.rng.mod(100) < chance) damage(i, count, sides);
        else if (w_.rng.mod(100) < chance) damage(i, count, sides / 2 + 1);
    }
}

// REWARDS:19 -- the third trap kind, by variant
void Rewards::crossbow() {
    switch (variant_) {
        case 0: damage(member_, g_.level, 8); break;
        case 1: groupDamage(50, g_.level, 8); break;
        default: w_.party.member(member_).status = Status::Paralyzed; break;
    }
}

// REWARDS:16 / 17 / 18 -- the blasters
void Rewards::blaster(bool mage) {
    for (int i = 0; i < w_.party.count(); ++i) {
        Character &c = w_.party.member(i);
        bool saved = w_.rng.mod(20) < c.luckSkill[4];
        auto stone = [&]() { if (int(c.status) < 4) c.status = Status(4); };
        auto paralyse = [&]() { if (int(c.status) < 3) c.status = Status(3); };
        switch (int(c.cls)) {
            case 1: if (mage) { if (saved) paralyse(); else stone(); } break;
            case 2: if (!mage) { if (saved) paralyse(); else stone(); } break;
            case 4: if (!mage && !saved) paralyse(); break;
            case 5: if (mage && !saved) paralyse(); break;
            default: break;
        }
    }
}

// REWARDS:14 -- the trap goes off
void Rewards::triggerTrap() {
    d_.con.clearRect(back_, 0, 2, 38, 4);
    d_.gotoXY(back_, 0, 2);
    if (trap_ != 0) {
        d_.putStrC(back_, 814);
        showTrap(trap_, variant_);
    } else {
        d_.putStrC(back_, 815);
    }
    d_.pause(20);
    d_.con.clearRect(back_, 0, 2, 38, 4);
    d_.gotoXY(back_, 0, 2);
    switch (trap_) {
        case 1: ++w_.party.member(member_).poison; break;
        case 2:
            for (int i = 0; i < w_.party.count(); ++i)
                if (w_.rng.mod(20) < w_.party.member(i).attrib[VIT]) w_.party.member(i).poison = 1;
            break;
        case 3: crossbow(); break;
        case 4:
            g_.x = w_.rng.mod(20);
            g_.y = w_.rng.mod(20);
            g_.dir = w_.rng.mod(4);
            g_.randomStart = true;
            break;
        case 5: blaster(true); break;
        case 6: blaster(false); break;
        case 7:
            g_.g31 = 1;
            d_.con.close(back_, true);
            throw EndAll{};
        default: break;
    }
    chestDone();
}

// REWARDS:21 / 22 -- the trap named truly / by a guess
void Rewards::revealTrap() {
    d_.con.clearRect(back_, 0, 2, 38, 4);
    d_.gotoXY(back_, 0, 2);
    showTrap(trap_, variant_);
}

void Rewards::guessTrap() {
    int guess = 0;
    int n = w_.rng.mod(50);
    for (int k = 1; k <= n; ++k) guess = guess == 7 ? 0 : guess + 1;
    d_.con.clearRect(back_, 0, 2, 38, 4);
    d_.gotoXY(back_, 0, 2);
    showTrap(guess, w_.rng.mod(3));
}

// REWARDS:23 -- I)NSPECT
void Rewards::inspectChest() {
    int m = pickMember(816);
    if (m < 0 || m >= w_.party.count()) return;
    Character &c = w_.party.member(m);
    if (c.status != Status::OK) return;
    if (looked_[m]) {
        d_.con.clearRect(back_, 0, 2, 38, 4);
        d_.gotoXY(back_, 0, 2);
        d_.putStrC(back_, 817);
        d_.pause(20);
        return;
    }
    looked_[m] = true;
    int skill = c.attrib[AGI];
    if (int(c.cls) == 3) skill *= 6;
    else if (int(c.cls) == 7) skill *= 4;
    if (skill > 95) skill = 95;
    member_ = m;
    if (w_.rng.mod(100) < skill) revealTrap();
    else if (w_.rng.mod(20) > c.attrib[AGI]) triggerTrap();
    else guessTrap();
}

// REWARDS:24 -- C)ALFO
void Rewards::calfoChest() {
    int m = pickMember(818);
    if (m < 0 || m >= w_.party.count()) return;
    Character &c = w_.party.member(m);
    if (!c.spellKnown[28]) return;
    if (c.priestSpells[2] == 0) return;
    if (c.status != Status::OK) return;
    member_ = m;
    --c.priestSpells[2];
    if (w_.rng.mod(100) < 95) revealTrap();
    else guessTrap();
}

// REWARDS:26 -- the attempt to disarm
void Rewards::disarmTry() {
    const Character &c = w_.party.member(member_);
    d_.con.clearRect(back_, 0, 2, 38, 4);
    d_.gotoXY(back_, 0, 2);
    int thief = (int(c.cls) == 3 || int(c.cls) == 7) ? 50 : 0;
    if (w_.rng.mod(70) < (c.charLevel - g_.level - 7) + thief) {
        d_.putStrC(back_, 819);
        d_.pause(20);
        chestDone();
    } else if (w_.rng.mod(20) < c.attrib[AGI]) {
        d_.putStrC(back_, 820);
        d_.pause(20);
        throw EndProc{25};
    } else {
        d_.putStrC(back_, 821);
        d_.pause(20);
        triggerTrap();
    }
}

// REWARDS:25 -- D)ISARM: the trap has to be named
void Rewards::disarmChest() {
    int m = pickMember(822);
    if (m < 0 || m >= w_.party.count()) return;
    if (w_.party.member(m).status != Status::OK) return;
    member_ = m;
    d_.con.clearRect(back_, 0, 2, 38, 4);
    d_.gotoXY(back_, 1, 2);
    d_.putStr(back_, 823);
    std::string typed = d_.getLine(back_, 80);
    if (typed.empty()) return;
    std::string name;
    trapName(name, trap_, variant_);
    try {
        if (typed == name) disarmTry();
        else triggerTrap();
    } catch (const EndProc &e) {
        if (e.proc != 25) throw;
    }
}

// REWARDS:27 -- O)PEN
void Rewards::openChest() {
    int m = pickMember(824);
    if (m < 0 || m >= w_.party.count()) return;
    if (w_.party.member(m).status != Status::OK) return;
    member_ = m;
    if (trap_ == 0) chestDone();
    if (w_.rng.mod(1000) < w_.party.member(m).charLevel) chestDone();
    triggerTrap();
}

// REWARDS:10 -- which trap (if any) this chest has
void Rewards::chooseTrap() {
    bool any = false;
    for (int t = 0; t <= 7; ++t) if ((rec_[1] >> t) & 1) any = true;
    variant_ = w_.rng.mod(3);
    if (!any) { trap_ = 0; return; }
    if (w_.rng.mod(15) > 11 + g_.level) { trap_ = 0; return; }
    int left = w_.rng.mod(100);
    trap_ = 0;
    while (left > 0) {
        do {
            if (trap_ < 7) {
                left -= trap_ == 3 ? 3 : 1;
                ++trap_;
            } else {
                trap_ = 1;
            }
        } while (!((rec_[1] >> trap_) & 1));
    }
}

// REWARDS:8 -- "A CHEST!  YOU MAY:"
void Rewards::chest() {
    picture(2);
    for (bool &b : looked_) b = false;
    chooseTrap();
    back_ = d_.con.open(0, 2, 40, 6, 31, true);
    d_.putStrC(back_, 825);
    try {
        for (;;) {
            d_.con.clearRect(back_, 0, 2, 38, 4);
            d_.gotoXY(back_, 0, 2);
            int sel = d_.menuStr(back_, 826);
            if (d_.quit) throw EndAll{};
            try {
                switch (sel) {
                    case 0: openChest(); break;
                    case 1: inspectChest(); break;
                    case 2: calfoChest(); break;
                    case 3: disarmChest(); break;
                    default:                                           // REWARDS:11 -- leave it alone
                        d_.con.close(back_, true);
                        throw EndAll{};
                }
            } catch (const EndProc &e) {
                if (e.proc != 8) throw;
                return;
            }
        }
    } catch (const EndProc &e) {
        if (e.proc != 8) throw;
    }
}

// REWARDS:6 -- the chest, then the gold and items
void Rewards::treasure() {
    gold_ = 0;
    loadRewardRecord();
    d_.flushKeys();
    if (rec_[0] && g_.g31 != 1) chest();
    else g_.g31 = 0;
    picture(1);
    for (int k = 1; k <= rec_[2]; ++k) dropOne(&rec_[3 + 9 * (k - 1)]);
    shareGold();
    u8 none[4] = {255, 255, 255, 255};
    (void)none;
    d_.con.clearAttr(d_.window1);
    d_.con.present(d_.window1);
}

// ---- experience points -----------------------------------------------------------------------------------------------------

// REWARDS:43 -- anyone left?
void Rewards::checkAlive() {
    alive_ = 0;
    for (int i = 0; i < w_.party.count(); ++i)
        if (w_.party.member(i).status == Status::OK) ++alive_;
    if (alive_ == 0) {
        xgoto_ = 16;
        throw EndAll{};
    }
}

// REWARDS:46..48 -- what one monster of this kind is worth
long long Rewards::monsterExperience(const int *r) const {
    auto times = [](int count, int per) -> long long {                // REWARDS:48
        if (count == 0) return 0;
        long long v = per;
        while (count > 1) { --count; v += v; }
        return v;
    };
    long long total = 0;
    long long first = (long long)(r[4]) * r[5];                       // REWARDS:47(hit dice * sides)
    first *= (r[43] == 0) ? 20 : 40;
    total = addLongs(total, first);
    total = addLongs(total, times(r[40], 35));
    total = addLongs(total, times(r[41], 35));
    total = addLongs(total, times(r[34], 200));
    total = addLongs(total, times(r[35], 90));
    total = addLongs(total, (long long)40 * (11 - r[8]));
    if (r[9] > 1) total = addLongs(total, times(r[9], 30));
    if (r[44] > 0) total = addLongs(total, times(r[44] / 10 + 1, 40));
    int bits1 = 0;
    for (int k = 1; k <= 6; ++k) if ((r[45] >> k) & 1) ++bits1;
    total = addLongs(total, times(bits1, 35));
    int bits2 = 0;
    for (int k = 0; k <= 6; ++k) if ((r[46] >> k) & 1) ++bits2;
    total = addLongs(total, times(bits2, 40));
    return total;
}

// REWARDS:44 / 45 -- each survivor's share of the experience
void Rewards::totalExperience() {
    long long total = 0;
    for (int g = 1; g <= 4; ++g) {
        if (fightMonster_[g - 1] < 0) continue;
        std::vector<u8> &b = monsterRecord(g_, w_.sc, fightMonster_[g - 1]);
        int r[47];
        for (int k = 0; k < 47; ++k) r[k] = int(i16(rd16(b.data() + k * 2)));
        total = addLongs(total, monsterExperience(r) * fightCount_[g - 1]);
    }
    expEach_ = alive_ > 0 ? total / alive_ : 0;
}

// REWARDS:49 / 50 -- a drained character's experience falls back to what the lower level needs
void Rewards::refundDrain() {
    Bytes tb = w_.sc.record(Scenario::Exp, 0);
    ExpTable table{tb};
    for (int i = 0; i < w_.party.count(); ++i) {
        if (!fightFlag_[i]) continue;
        Character &c = w_.party.member(i);
        Win *b = d_.con.open(0, 2, 40, 3, 31, true);
        d_.putCentered(b, Dos::substitute(d_.str(925), c.name));
        d_.con.close(b, false);
        int lvl = c.charLevel - 1;
        long long v;
        if (lvl == 0) {
            v = 0;
        } else if (lvl < 13) {
            v = table.threshold(int(c.cls), lvl).value();
        } else {
            v = table.threshold(int(c.cls), 12).value();
            for (int k = 13; k <= lvl; ++k) v = addLongs(v, table.threshold(int(c.cls), 0).value());
        }
        c.exp.v = v;
        d_.pause(20);
    }
}

// REWARDS:42
void Rewards::experience() {
    for (int g = 0; g < 4; ++g) { fightCount_[g] = g_.fightCount[g]; fightMonster_[g] = g_.fightMonster[g]; }
    for (int i = 0; i < 6; ++i) fightFlag_[i] = g_.fightFlag[i];
    checkAlive();
    refundDrain();
    if (xgoto_ == 22) return;
    bool someone = false;
    for (int i = 0; i < w_.party.count(); ++i) if (w_.party.member(i).status == Status::OK) someone = true;
    if (!someone) { xgoto_ = 16; throw EndAll{}; }
    totalExperience();
    Win *b = d_.con.open(0, 2, 40, 3, 31, true);
    d_.con.clear(b, true);
    d_.putCentered(b, d_.str(926) + Dos::long2str(expEach_) + d_.str(927));
    d_.con.close(b, false);
    d_.pause(20);
    for (int i = 0; i < w_.party.count(); ++i) {
        Character &c = w_.party.member(i);
        if (c.status == Status::OK) c.exp.v = addLongs(c.exp.v, expEach_);
    }
}

// ---- the camp jobs (XGOTO 24) ----------------------------------------------------------------------------------------------

// REWARDS:33..35 -- R)EAD the spell books
void Rewards::readBooks() {
    Console &con = d_.con;
    int member = g_.g9;
    Win *w = con.open(0, 7, 40, 17, 15, false);
    Character &c = w_.party.member(member);
    auto counts = [&](int strNo, const int *cnt) {                      // REWARDS:35
        d_.putStr(w, strNo);
        d_.putStr(w, 901);
        d_.putNum(w, cnt[1], 1);
        for (int k = 2; k <= 7; ++k) { d_.putC(w, '/'); d_.putNum(w, cnt[k], 1); }
        d_.newline(w);
    };
    auto book = [&](int titleNo, int lo, int hi) {                      // REWARDS:34
        con.clear(w, true);
        int n = 0, x = 0, y = 0;
        for (int s = lo; s <= hi; ++s) {
            std::string name = d_.str(5000 + s);
            if (!name.empty() && name[0] == '*') {
                x = 9 * (n % 4);
                y = 7 * (n / 4);
                ++n;
                if (n == 4) ++x;
                name.erase(0, 1);
            }
            if (c.spellKnown[s]) {
                d_.gotoXY(w, x, y);
                d_.putS(w, name);
                ++y;
            }
        }
        int yy = w->h - 2 - 2, xx = w->w - 2;
        std::string t = d_.str(titleNo);
        d_.gotoXY(w, xx - Dos::visLen(t), yy);
        d_.putS(w, t);
        std::string p = d_.str(900);
        d_.gotoXY(w, xx - Dos::visLen(p), yy + 1);
        d_.putS(w, p);
        d_.waitCR();
        d_.inchar = 0;
    };
    for (;;) {
        con.clear(w, true);
        counts(902, c.mageSpells);
        counts(903, c.priestSpells);
        d_.gotoXY(w, 8, 4);
        int sel = d_.menuStr(w, 904);
        if (d_.quit) return;
        if (sel == 0) book(905, 1, 21);
        else if (sel == 1) book(906, 22, 50);
        else break;
    }
    d_.inchar = 0;
    con.close(w, false);
    xgoto_ = 27;
}

// REWARDS:36..39 -- KANDI: where is that body?
void Rewards::locateBody() {
    Console &con = d_.con;
    Win *b = con.open(1, 7, 38, 6, 17, true);
    d_.putStrC(b, 916);
    d_.newline(b);
    d_.putStr(b, 917);
    std::string who = d_.getName(b, 15);
    auto leave = [&]() {                                                // REWARDS:37
        con.close(b, false);
        d_.inchar = 65;
        xgoto_ = 27;
        throw EndAll{};
    };
    auto pressToLeave = [&]() {                                         // REWARDS:38
        d_.gotoXY(b, 8, b->h - 3);
        d_.putStrC(b, 907);
        d_.waitCR();
        leave();
    };
    if (who.empty()) leave();
    con.clear(b, true);
    d_.putCentered(b, Dos::substitute(d_.str(918), who));
    for (int i = 0; i < w_.roster.count(); ++i) {
        const Character &c = w_.roster.slot(i);
        if (c.name != who) continue;
        if (c.status == Status::Lost) continue;                         // REWARDS:39
        if (c.lostY == 0 && c.lostLevel == 0) {
            d_.putStrC(b, 909);
        } else {
            int x = c.lostY / 100, y = c.lostY % 100;
            std::string s = d_.str(910) + d_.str(y > 9 ? 911 : 912) + d_.str(x > 9 ? 913 : 914) + d_.str(915);
            d_.putCentered(b, Dos::substitute(s, Dos::itoa(c.lostLevel)));
        }
        pressToLeave();
    }
    d_.putStrC(b, 919);
    pressToLeave();
}

// REWARDS:40 -- I)DENTIFY an item (the Bishop's skill)
void Rewards::identifyItem() {
    Console &con = d_.con;
    int m = g_.g9;
    xgoto_ = 27;
    Character &c = w_.party.member(m);
    Win *b = con.open(13, 8, 26, 5, 15, false);
    con.clear(b, true);
    d_.putStrC(b, 920);
    d_.putStrC(b, 921);
    d_.putStrC(b, 922);
    int k = d_.getNum(c.possCount);
    auto done = [&]() {                                                 // REWARDS:41
        con.close(b, false);
        g_.g9 = m;
        throw EndAll{};
    };
    if (k == 0 || d_.quit) done();
    con.clear(b, true);
    d_.newline(b);
    Possession &p = c.poss[k - 1];
    if (p.identified) done();
    p.identified = w_.rng.mod(100) > (90 - 5 * c.charLevel);
    d_.putStrC(b, p.identified ? 923 : 924);
    if (w_.rng.mod(100) < 35 - 3 * c.charLevel) {
        ObjectRec o{w_.sc.record(Scenario::Object, p.itemIndex)};
        p.cursed = o.cursed();
        if (p.cursed) xgoto_ = 14;
    }
    d_.pause(20);
    done();
}

// ---- REWARDS:1 -------------------------------------------------------------------------------------------------------------------

RewardsResult Rewards::run(int xgoto) {
    RewardsResult res;
    xgoto_ = xgoto;
    try {
        if (xgoto == 11) {
            xgoto_ = 7;
            try {
                experience();
                treasure();
            } catch (const EndProc &) {
            }
        } else if (xgoto == 22) {
            experience();
            g_.g9 = 0;
            xgoto_ = 23;
        } else if (xgoto == 24) {
            if (g_.g36 == 0) readBooks();
            else if (g_.g36 == 2) locateBody();
            else identifyItem();
        }
    } catch (const EndAll &) {
    }
    res.xgoto = xgoto_;
    res.windowClosed = d_.quit;
    return res;
}

} // namespace wiz
