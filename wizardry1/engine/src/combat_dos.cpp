// SEGMENTS COMBAT / CINIT / CUTIL (display and bookkeeping) -- see combat_dos.h.
// Every routine names the p-code proc it ports; `tools/pcode_decomp.py <SEG> <n>` prints the original.
#include "wiz/combat_dos.h"

#include <algorithm>
#include <cstring>

namespace wiz {

// ============================================================================================
//  COMBAT
// ============================================================================================

// COMBAT:7 / 8 / 6 -- a message, word-wrapped to the window and centred a line at a time
void Combat::say(const std::string &text) {
    std::string s = text;
    int width = msg_->w - 2;
    while (!s.empty()) {
        // COMBAT:7: the last blank within `width` columns (or the end of the string)
        int brk = 0, pos = 1, len = int(s.size());
        for (int col = 1; col <= width; ++col) {
            if (pos > len) { brk = len + 1; break; }
            if (s[pos - 1] == ' ') brk = pos;
            if (u8(s[pos - 1]) >= 129 && u8(s[pos - 1]) <= 159) ++pos;
            ++pos;
        }
        // COMBAT:8: skip the leading blanks
        int first = brk;
        {
            int lim = brk - 1, i = 1;
            first = lim + 1;
            for (; i <= lim; ++i)
                if (s[i - 1] != ' ') { first = i; break; }
        }
        if (first < brk) d_.putCentered(msg_, s.substr(first - 1, brk - first));
        if (brk >= len) break;
        s = s.substr(brk);
    }
}

// COMBAT:4 -- a fresh message page
void Combat::begin(const std::string &s) {
    d_.flushKeys();
    d_.con.clear(msg_, true);
    d_.gotoXY(msg_, 0, 0);
    say(s);
    msgRow_ = msg_->cy;
    msgPending_ = true;
}

// COMBAT:3 -- one more line under it, then the DELAY beat
void Combat::add(const std::string &s) {
    d_.con.clearRect(msg_, 0, msgRow_, 38, 4 - msgRow_);
    d_.gotoXY(msg_, 0, msgRow_);
    say(s);
    d_.delay(g_.delay);
    msgPending_ = false;
}

// COMBAT:2
void Combat::endFight() {
    d_.con.close(list_, false);
    d_.con.close(msg_, false);
    throw EndCombat{};
}

// COMBAT:1
CombatOutcome Combat::run() {
    Console &con = d_.con;
    CombatOutcome res;
    std::memset(G_, 0, sizeof G_);
    {
        std::vector<u8> &b = monsterRecord(g_, w_.sc, g_.g37);
        for (int k = 0; k < 47; ++k) G_[1][76 + k] = int(i16(rd16(b.data() + k * 2)));
    }
    con.show(w_.g10, false);
    list_ = con.open(2, 0, 36, 6, 6, false);
    msg_ = con.open(0, 5, 40, 6, 7, true);
    done_ = false;
    phase_ = 0;
    xgoto_ = 11;
    try {
        cinit();
        for (;;) {
            cutil();
            if (d_.quit) break;
            if (!done_) {
                phase_ = -1;
                cinit();
                roundActions();
            }
            if (done_) break;
        }
        if (d_.quit) endFight();
        phase_ = 2;
        cinit();
        endFight();
    } catch (const EndCombat &) {
    }
    res.xgoto = xgoto_;
    res.windowClosed = d_.quit;
    return res;
}

// ============================================================================================
//  CINIT
// ============================================================================================

void Combat::cinit() {                                    // CINIT:1
    if (phase_ == -1) cinitMonsters();
    else if (phase_ == 0) cinitStart();
    else cinitEnd();
}

// CINIT:15 -- dice rolled from a (dice, sides, add) triple
int Combat::roll(int dice, int sides, int add) {
    int total = add;
    while (dice > 0) {
        if (sides > 0) total = total + rnd(sides) + 1;
        --dice;
    }
    return total;
}

// CINIT:14 -- put monster `monster` into group `group` (and maybe a companion group after it)
void Combat::loadGroup(int monster, int group) {
    G_[group][3] = monster;
    std::vector<u8> &b = monsterRecord(g_, w_.sc, monster);
    for (int k = 0; k < 47; ++k) rec(group, k) = int(i16(rd16(b.data() + k * 2)));
    if (group < 4 && (group - 1) < g_.level && rec(group, 38) > 0) {
        if (rnd(100) < rec(group, 39)) loadGroup(rec(group, 38), group + 1);
    }
}

// CINIT:13 -- size and hit points of every group
void Combat::makeEnemies() {
    for (int g = 1; g <= 4; ++g) { G_[g][2] = 0; G_[g][1] = 0; G_[g][3] = -1; }
    loadGroup(g_.g37, 1);
    for (int g = 1; g <= 4; ++g) {
        if (G_[g][3] == -1) continue;
        int n = roll(rec(g, 1), rec(g, 2), rec(g, 3));
        if (n > 4 + g_.level) n = 4 + g_.level;
        if (n > 9) n = 9;
        if (n < 1) n = 1;
        G_[g][2] = n;
        G_[g][1] = n;
        G_[g][0] = g_.identify > 0;
        for (int u = 0; u < n; ++u) {
            unit(g, u, 6) = rec(g, 4);
            unit(g, u, 4) = 0;
            unit(g, u, 5) = 0;
            unit(g, u, 3) = roll(rec(g, 4), rec(g, 5), rec(g, 6));
            unit(g, u, 7) = 0;
        }
    }
}

// CINIT:16 -- the party as group 0
void Combat::setupParty() {
    G_[0][2] = partyCount();
    G_[0][1] = partyCount();
    for (int i = 0; i < partyCount(); ++i) {
        Character &c = chr(i);
        unit(0, i, 6) = c.charLevel;
        unit(0, i, 4) = 0;
        unit(0, i, 5) = 0;
        unit(0, i, 3) = c.hpLeft;
        unit(0, i, 7) = int(c.status);
        c.raw[96 * 2] = c.raw[95 * 2]; c.raw[96 * 2 + 1] = c.raw[95 * 2 + 1];     // the current resists := the base
        c.raw[98 * 2] = c.raw[97 * 2]; c.raw[98 * 2 + 1] = c.raw[97 * 2 + 1];
    }
}

// CINIT:19 -- remember where the party stands
void Combat::saveLocations() {
    for (int i = 0; i < partyCount(); ++i) {
        chr(i).lostY = 100 * g_.x + g_.y;
        chr(i).lostLevel = g_.level;
    }
}

// CINIT:18 -- a friendly encounter can turn a good character evil (and the other way round)
void Combat::alignDrift(bool leaving, bool fighting) {
    for (int i = 0; i < partyCount(); ++i) {
        if (rnd(100) < 5) {
            Character &c = chr(i);
            if (c.align == Align::Good && fighting) c.align = Align::Evil;
            else if (c.align == Align::Evil && leaving) c.align = Align::Good;
        }
    }
}

// CINIT:17 -- "A FRIENDLY GROUP OF ..."
void Combat::friendly() {
    if (bit(u16(rec(1, 45)), 0)) return;
    if (G_[1][3] == 96) return;
    bool anyGood = false, anyEvil = false;
    for (int i = 0; i < partyCount(); ++i) {
        anyGood = anyGood || chr(i).align == Align::Good;
        anyEvil = anyEvil || chr(i).align == Align::Evil;
    }
    if (!(anyGood || anyEvil)) return;
    int r = rnd(100);
    int limit = 50;
    switch (rec(1, 7)) {
        case 0: limit = 10; break;
        case 1: limit = 5; break;
        case 2: limit = 15; break;
        case 3: limit = 3; break;
        case 4: limit = 30; break;
        case 7: limit = 25; break;
        default: break;
    }
    if (r > limit) return;
    for (int g = 1; g <= 4; ++g) G_[g][0] = 1;
    d_.con.clear(msg_, true);
    d_.putStrC(msg_, 1000);
    d_.putStrC(msg_, 13003 + G_[1][3] * 4);
    d_.newline(msg_);
    d_.putStrC(msg_, 1001);
    ambush_ = 0;
    std::string fight = d_.str(1002), leave = d_.str(1003);
    do d_.getKey(); while (!d_.quit && d_.inchar != u8(fight[0]) && d_.inchar != u8(leave[0]));
    bool leaving = d_.inchar == u8(leave[0]);
    bool fighting = d_.inchar == u8(fight[0]);
    alignDrift(leaving, fighting);
    if (leaving) {
        xgoto_ = 7;
        endFight();
    }
}

// CINIT:12 -- the fight begins
void Combat::cinitStart() {
    saveLocations();
    std::memset(pic_, 255, sizeof pic_);
    makeEnemies();
    setupParty();
    std::memset(flag_, 0, sizeof flag_);
    for (int i = 0; i < partyCount(); ++i) disk_[i] = w_.party.rosterSlot(i);
    if (rnd(100) > 80) ambush_ = 1;
    else if (rnd(100) > 80) ambush_ = 2;
    else ambush_ = 0;
    if (g_.level != w_.sc.count(Scenario::Maze)) friendly();
}

// CINIT:2 (+3..10) -- what each monster does this round
void Combat::cinitMonsters() {
    int target = 0, group = 0, action = 0;
    // CINIT:3 -- the target does not shrug the monster's class off (or fails to half the time).
    // LOR evaluates both operands, so the RANDOM call is always made.
    auto canAffect = [&]() {
        bool resists = bit(rawWord(chr(target), 96), G_[group][83]);
        int r = rnd(100);
        return !resists || r < 50;
    };
    // CINIT:5 -- casting uses a spell point up now and then
    auto spendPoint = [&](int word) { if (rnd(G_[group][1] + 2) == 1) --G_[group][word]; };
    auto mageSpell = [&](int level) {                                  // CINIT:6
        for (;;) {                                                     // (a1 > 1) and (RANDOM > 70): both evaluated
            bool more = level > 1;
            int r = rnd(100);
            if (more && r > 70) --level; else break;
        }
        bool alt = rnd(100) > 33;
        spendPoint(116);
        switch (level) {
            case 1: action = alt ? 3983 : 4178; break;
            case 2: action = alt ? 3340 : 4178; break;
            case 3: action = alt ? 4731 : 6181; break;
            case 4: action = alt ? 3180 : 6156; break;
            case 5: action = alt ? 6156 : 4925; break;
            case 6: action = alt ? 4925 : 4573; break;
            default: action = 11157; break;
        }
    };
    auto priestSpell = [&](int level) {                                // CINIT:7
        bool alt = rnd(100) > 33;
        spendPoint(117);
        switch (level) {
            case 1: action = 3675; break;
            case 2: action = 5970; break;
            case 3: action = alt ? 3675 : 1253; break;
            case 4: action = 1253; break;
            case 5: action = alt ? 2446 : 382; break;
            case 6: action = alt ? 4296 : 759; break;
            default: action = 759; break;
        }
    };
    for (group = 1; group <= 4; ++group) {
        if (G_[group][1] <= 0) continue;
        for (int u = 0; u <= G_[group][1] - 1; ++u) {
            if (!(unit(group, u, 7) == 0 && ambush_ != 1)) { unit(group, u, 2) = -1; continue; }
            unit(group, u, 2) = rnd(8) + 2;
            if (partyCount() == 1) {
                target = 0;
            } else {
                target = partyCount() - 1;
                while (unit(0, target, 7) >= 5) --target;
                target = rnd(target + 1);
            }
            unit(group, u, 0) = target;
            unit(group, u, 1) = 0;
            action = 0;
            if (canAffect()) {
                if (ambush_ == 0) {                                    // CINIT:4
                    if (G_[group][116] > 0) {
                        int r = rnd(100);
                        if (G_[group][83] == 1 || r < 75) mageSpell(G_[group][116]);
                    }
                    if (action == 0 && G_[group][117] > 0) {
                        int r = rnd(100);
                        if (G_[group][83] == 2 || r < 75) priestSpell(G_[group][117]);
                    }
                }
                if (action == 0 && G_[group][119] > 0 && rnd(100) < 60) action = -3;                            // CINIT:10
                if (action == 0 && bit(u16(G_[group][122]), 6) && G_[group][1] < 8 && rnd(100) < 75) action = -4;   // CINIT:8
                if (action == 0 && bit(u16(G_[group][122]), 5) && g_.g36 > g_.g35 && rnd(100) < 65) action = -2;    // CINIT:9
                if (action > 0 && bit(rawWord(chr(target), 98), 6)) unit(group, u, 2) = -1;
                if (action == 0) {
                    bool front = u <= 4 - group;
                    int r = rnd(100);
                    if (front || (60 - 10 * group) <= r) {
                        target = target % 3;
                        if (canAffect()) { action = -1; unit(group, u, 0) = target; }
                        else unit(group, u, 2) = -1;
                    }
                }
            }
            unit(group, u, 1) = action;
        }
    }
    ambush_ = 0;
}

// CINIT:11 -- the fight is over
void Combat::cinitEnd() {
    for (int i = 0; i < partyCount(); ++i)
        if (int(chr(i).status) < 3) chr(i).status = Status::OK;
    for (int i = 0; i < 4; ++i) {
        g_.fightMonster[i] = G_[i + 1][3];
        g_.fightCount[i] = G_[i + 1][2];
    }
    for (int i = 0; i < 6; ++i) g_.fightFlag[i] = flag_[i];
}

} // namespace wiz
