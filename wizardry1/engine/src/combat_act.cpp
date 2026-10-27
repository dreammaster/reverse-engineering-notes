// SEGMENTS SWINGASW / CASTASPE (the actions of a round) -- see combat_dos.h.
// Every routine names the p-code proc it ports; `tools/pcode_decomp.py SWINGASW|CASTASPE <n>` prints the original.
#include "wiz/combat_dos.h"

#include <algorithm>

namespace wiz {

namespace {
struct EndAction {};                 // EXIT(9:x) / EXIT(8:x): the action is over
}

// ---- message building (SWINGASW:3..9, CASTASPE:2..3) ---------------------------------------------------

// SWINGASW:3 -- the name of a party member or monster, with its article from template `tmpl`
std::string Combat::nameOf(int g, int u, int tmpl) {
    std::string n;
    if (g == 0) n = chr(u).name;
    else n = d_.str(13000 + 4 * G_[g][3] + (G_[g][0] ? 2 : 0));
    return d_.nameWith(n, tmpl);
}

// SWINGASW:8 / CASTASPE:16 -- dice (count, sides, add) as three words
int Combat::dice(int count, int sides, int add) {
    int total = 0;
    while (count > 0) {
        if (sides > 0) total = total + rnd(sides) + 1;
        --count;
    }
    return total + add;
}

// SWINGASW:9 -- "<what> <who>" as a line added under the page
void Combat::sayAbout(int strNo, int g, int u, int tmpl, int strNo2) {
    std::string out = d_.str(strNo);
    out = Dos::substitute(out, nameOf(g, u, tmpl));
    out = Dos::substituteChar(out, 38, d_.str(strNo2));
    add(out);
}

// ---- SWINGASW -------------------------------------------------------------------------------------------

// SWINGASW:2 / CASTASPE:5 -- a unit dies
void Combat::kill(int g, int u) {
    int &hp = unit(g, u, 3);
    hp = 0;
    if (unit(g, u, 7) < 5) {
        unit(g, u, 7) = 5;
        if (g == 0) d_.sound(5);
        else if (G_[g][3] == 96) d_.sound(6);
    }
}

// SWINGASW:12..14 -- a monster's hit may do more than damage
void Combat::specialHit(int target, int group) {
    Character &c = chr(target);
    auto effect = [&](int kind, int save, int resist, int msg) {       // SWINGASW:14
        int r = rnd(20);
        if (r > int(c.luckSkill[save])) return;
        if (kind == 3) {
            int chance = G_[group][80] * 2;
            if (chance > 50) chance = 50;
            if (rnd(100) > chance) return;
        }
        if (resist > 0 && bit(rawWord(c, 98), resist)) return;
        if (unit(0, target, 7) >= 5) return;
        std::string s = Dos::substitute(d_.str(1102), nameOf(0, target, 1320));
        s = Dos::substituteChar(s, 38, d_.str(msg));
        add(s);
        switch (kind) {
            case 0: if (unit(0, target, 7) < 4) unit(0, target, 7) = 4; break;
            case 1: c.poison = 1; break;
            case 2: if (unit(0, target, 7) < 3) unit(0, target, 7) = 3; break;
            default: kill(0, target); break;
        }
    };
    u16 bits = u16(G_[group][122]);
    if (bit(bits, 1)) effect(1, 0, 3, 1103);
    if (bit(bits, 2)) effect(2, 0, 0, 1104);
    if (bit(bits, 0)) effect(0, 1, 5, 1105);
    if (G_[group][110] > 0) {                                          // SWINGASW:13 -- level drain
        if (!(flag_[target] || bit(rawWord(c, 98), 4))) {
            c.charLevel -= G_[group][110];
            unit(0, target, 6) = c.charLevel;
            d_.con.clearRect(msg_, 0, 3, 38, 1);
            d_.gotoXY(msg_, 0, 3);
            std::string n = Dos::itoa(G_[group][110]);
            std::string s;
            if (G_[group][110] == 1) s = d_.str(1100);
            else s = Dos::substituteChar(d_.str(1101), 35, n);
            s = Dos::substitute(s, d_.nameWith(c.name, 1310));
            add(s);
            if (c.charLevel < 1) {
                c.charLevel = 0;
                unit(0, target, 3) = 0;
                unit(0, target, 7) = 7;
            } else {
                c.hpMax = (c.hpMax / c.maxLevelAcquired) * c.charLevel;
                if (c.hpMax < 1) c.hpMax = 1;
                c.maxLevelAcquired = c.charLevel;
                if (c.hpLeft > c.hpMax) c.hpLeft = c.hpMax;
                flag_[target] = 1;
            }
        }
    }
    if (bit(bits, 3)) effect(3, 0, 0, 1106);
}

// SWINGASW:15 / 16 / 17 / 7 -- which verb a monster's attack uses
void Combat::pickVerb(bool partyStyle) {
    if (partyStyle) { verb_ = 1155 + rnd(5); return; }                  // SWINGASW:7
    switch (G_[grpIdx_][83]) {
        case 6: case 8: case 12: case 13: verb_ = 1150 + rnd(5); break;                       // 16
        case 7: case 9: if (rnd(2) == 1) verb_ = 1150 + rnd(5); else verb_ = 1155 + rnd(5); break;   // 17
        default: verb_ = 1155 + rnd(5); break;                          // 7
    }
}

// SWINGASW:11 -- a monster attacks a party member
void Combat::monsterAttacks() {
    if (unit(0, tgt_, 7) >= 5) return;
    pickVerb(false);
    int t = tgt_;
    if (unit(0, t, 7) >= 5) return;
    Character &c = chr(t);
    int need = 20 - c.armorClass - G_[grpIdx_][80] + g_.protect + unit(0, t, 4) + 2 * (unit(grpIdx_, t, 1) == 0 ? 1 : 0);
    if (need < 1) need = 1; else if (need > 19) need = 19;
    int dmg = 0, hits = 0;
    int attacks = G_[grpIdx_][85];
    for (int k = 1; k <= attacks; ++k) {
        if (rnd(20) >= need) {
            dmg += dice(G_[grpIdx_][86 + 3 * (k - 1)], G_[grpIdx_][87 + 3 * (k - 1)], G_[grpIdx_][88 + 3 * (k - 1)]);
            ++hits;
        }
    }
    if (unit(0, t, 7) == 2) dmg *= 2;
    std::string s = d_.str(dmg == 0 ? 1107 : hits == 1 ? 1108 : 1109);
    s = Dos::substitute(s, nameOf(grpIdx_, 0, 1330));
    s = Dos::substituteChar(s, 35, Dos::itoa(hits));
    s = Dos::substituteChar(s, 38, Dos::itoa(dmg));
    s = Dos::substituteChar(s, 37, nameOf(0, t, 1340));
    s = Dos::substituteChar(s, 63, d_.str(verb_));
    begin(s);
    if (dmg != 0) {
        specialHit(t, grpIdx_);
        unit(0, t, 3) -= dmg;
        if (unit(0, t, 3) <= 0) {
            kill(0, t);
            sayAbout(1110, 0, t, 1350, 1350);
        }
    }
}

// SWINGASW:18 -- a party member attacks a monster group
void Combat::memberAttacks() {
    int g = tgt_;
    if (G_[g][1] == 0) return;
    int t = unitIdx_ % G_[g][1];
    if (unit(g, t, 7) >= 5) return;
    pickVerb(true);
    const Character &c = chr(unitIdx_);
    int need = 21 - G_[g][84] - c.hpCalcMd + unit(g, t, 4) - 3 * g;
    if (need < 1) need = 1; else if (need > 19) need = 19;
    int dmg = 0, hits = 0;
    for (int k = 1; k <= c.swingCnt; ++k) {
        if (rnd(20) >= need) {
            dmg += dice(c.hpDamRc[0], c.hpDamRc[1], c.hpDamRc[2]);
            ++hits;
        }
    }
    if (unit(g, t, 7) == 2) dmg *= 2;
    if (bit(rawWord(c, 99), G_[g][83])) dmg *= 2;
    std::string s = d_.str(dmg == 0 ? 1107 : hits == 1 ? 1108 : 1109);
    s = Dos::substitute(s, nameOf(0, unitIdx_, 1360));
    s = Dos::substituteChar(s, 35, Dos::itoa(hits));
    s = Dos::substituteChar(s, 38, Dos::itoa(dmg));
    s = Dos::substituteChar(s, 37, nameOf(g, t, 1370));
    s = Dos::substituteChar(s, 63, d_.str(verb_));
    begin(s);
    unit(g, t, 3) -= dmg;
    int msg = 1110;
    if (c.critHitM && dmg > 0) {
        int chance = c.charLevel * 2;
        if (chance > 50) chance = 50;
        if (rnd(100) < chance) {
            if (rnd(35) > G_[g][80] + 10) {
                msg = 1111;
                unit(g, t, 3) = 0;
            }
        }
    }
    if (unit(g, t, 3) <= 0) {
        kill(g, t);
        sayAbout(msg, g, t, 1380, 1380);
    }
}

// SWINGASW:19 / 20 -- a monster calls for help
void Combat::callForHelp() {
    int g = grpIdx_;
    auto none = [&]() {
        add(Dos::substitute(d_.str(1112), nameOf(g, 0, 1390)));
        throw EndAction{};
    };
    if (G_[g][1] == 9) none();
    if (rnd(200) > 10 * G_[g][80]) none();
    int keep116 = G_[g][116], keep117 = G_[g][117];
    {
        Bytes b = w_.sc.record(Scenario::Monster, G_[g][3]);
        for (int k = 0; k < 47; ++k) G_[g][76 + k] = k * 2 + 1 < int(b.size()) ? int(i16(rd16(b.p + k * 2))) : 0;
    }
    if (keep117 < G_[g][117]) G_[g][117] = 1 + (G_[g][117] + keep117) / 2;
    if (keep116 < G_[g][116]) G_[g][116] = 1 + (G_[g][116] + keep116) / 2;
    int n = G_[g][1];
    ++G_[g][1];
    ++G_[g][2];
    unit(g, n, 2) = -1;
    unit(g, n, 1) = 0;
    unit(g, n, 5) = unit(grpIdx_, unitIdx_, 5);
    unit(g, n, 4) = 0;
    unit(g, n, 3) = dice(G_[g][80], G_[g][81], G_[g][82]);
    unit(g, n, 6) = G_[g][80];
    unit(g, n, 7) = 0;
    begin(Dos::substitute(d_.str(1113), nameOf(g, 0, 1390)));
}

// SWINGASW:21 -- a monster runs away
void Combat::runAway() {
    begin(Dos::substitute(d_.str(1114), nameOf(grpIdx_, 0, 1400)));
    --G_[grpIdx_][2];
    kill(grpIdx_, unitIdx_);
}

// SWINGASW:22 -- the party dispels undead
void Combat::dispell() {
    const Character &c = chr(unitIdx_);
    int g = tgt_;
    int chance = 50 + 5 * c.charLevel - 10 * G_[g][80];
    switch (int(c.cls)) {
        case 4: chance -= 20; break;
        case 6: chance -= 40; break;
        default: break;
    }
    if (chance < 5) chance = 5;
    int gone = 0;
    for (int u = 0; u <= G_[g][1] - 1; ++u) {
        if (unit(g, u, 7) == 0 && rnd(100) < chance && G_[g][83] == 10) {
            ++gone;
            --G_[g][2];
            kill(g, u);
        }
    }
    std::string s = d_.str(gone == 0 ? 1115 : gone == 1 ? 1116 : 1117);
    s = Dos::substitute(s, nameOf(0, unitIdx_, 1410));
    s = Dos::substituteChar(s, 35, Dos::itoa(gone));
    begin(s);
}

// SWINGASW:1
void Combat::swing() {
    try {
        switch (act_) {
            case -5: dispell(); break;
            case -4: callForHelp(); break;
            case -2: runAway(); break;
            case -1: if (grpIdx_ == 0) memberAttacks(); else monsterAttacks(); break;
            default: break;
        }
    } catch (const EndAction &) {
    }
}

// ---- CASTASPE -----------------------------------------------------------------------------------------------

// CASTASPE:4 -- true if the spell has no effect on the unit (dead, or the monsters resist it)
bool Combat::noEffect(int g, int u) {
    if (unit(g, u, 7) >= 5) return true;
    if (g < 1) return false;
    if (G_[g][120] > rnd(100)) {
        add(Dos::substitute(d_.str(1118), nameOf(g, 0, 1420)));
        return true;
    }
    return false;
}

// CASTASPE:14 -- damage
void Combat::hurt(int g, int u, int amount) {
    if (amount < 0) amount = 0;
    else if (amount == 0) amount = 1;
    if (noEffect(g, u)) return;
    unit(g, u, 3) -= amount;
    if (unit(g, u, 3) <= 0) kill(g, u);
    std::string s = d_.str(unit(g, u, 3) <= 0 ? 1119 : 1120);
    s = Dos::substituteChar(s, 35, Dos::itoa(amount));
    add(Dos::substitute(s, nameOf(g, u, g == 0 ? 1470 : 1460)));
}

// CASTASPE:19 -- damage by dice
void Combat::hurtDice(int g, int u, int count, int sides) { hurt(g, u, dice(count, sides, 0)); }

// CASTASPE:18 -- healing by dice
void Combat::healDice(int g, int u, int count, int sides) {
    if (unit(g, u, 7) >= 5) return;
    int amount = dice(count, sides, 0);
    unit(g, u, 3) += amount;
    if (unit(g, u, 3) > chr(u).hpMax) unit(g, u, 3) = chr(u).hpMax;
    std::string s = d_.str(unit(g, u, 3) == chr(u).hpMax ? 1124 : 1123);
    s = Dos::substituteChar(s, 35, Dos::itoa(amount));
    add(Dos::substitute(s, nameOf(g, u, 1450)));
}

// CASTASPE:15 -- a spell that can be shrugged off: `chance` % to resist, else the effect
void Combat::afflict(int g, int u, int chance, int msgNo, int effect) {
    if (noEffect(g, u)) return;
    if (chance < 5) chance = 5;
    std::string s;
    if (rnd(100) < chance) {
        s = d_.str(1122);
    } else {
        s = d_.str(1121);
        switch (effect) {
            case 0: case 3: unit(g, u, 7) = 2; break;
            case 1: unit(g, u, 5) = rnd(4) + 2; break;
            case 2: kill(g, u); break;
            default: break;
        }
    }
    s = Dos::substitute(s, nameOf(g, u, g == 0 ? 1440 : 1430));
    s = Dos::substituteChar(s, 38, d_.str(msgNo));
    add(s);
}

// CASTASPE:17 -- armour class of a stretch of units
void Combat::armour(int g, int delta, int first, int last) {
    for (int u = first; u <= last; ++u) unit(g, u, 4) += delta;
}

// CASTASPE:25 -- fire / cold / ...: damage dice on every unit of a group
void Combat::blast(int g, int count, int sides, int element) {
    if (count == 10 && sides == 15) d_.sound(4); else d_.sound(3);
    if (G_[g][1] > 0) {
        for (int u = 0; u <= G_[g][1] - 1; ++u) {
            if (g == 0) G_[0][121] = int(rawWord(chr(u), 98));
            if (element != 0 && bit(u16(G_[g][121]), element)) hurtDice(g, u, count / 2 + 1, sides);
            else hurtDice(g, u, count, sides);
        }
    }
}

// CASTASPE:26 -- LOKTOFEIT: the party escapes with nothing
void Combat::loktofeit(int level) {
    int chance = 65 + level;
    if (chance > 95) chance = 95;
    if (rnd(100) > chance) {
        add(d_.str(1130));
        throw EndAction{};
    }
    for (int i = 0; i < partyCount(); ++i) {
        Character &c = chr(i);
        for (int k = 0; k < c.possCount; ++k) c.poss[k] = Possession{};
        c.possCount = 0;
        c.gold.v %= 10000;                                              // only the two high limbs go
    }
    xgoto_ = 10;
    endFight();
}

// CASTASPE:27 -- MAKANITO: the weak monsters die
void Combat::makanito() {
    for (int g = 1; g <= 4; ++g) {
        if (G_[g][1] > 0 && G_[g][83] != 10 && G_[g][80] < 8)
            for (int u = 0; u <= G_[g][1] - 1; ++u) hurt(g, u, unit(g, u, 3));
    }
}

// CASTASPE:28 -- MALOR in a fight
void Combat::malor() {
    g_.x = rnd(20);
    g_.y = rnd(20);
    while (rnd(100) < 30) ++g_.level;
    while (rnd(100) < 10) ++g_.level;
    if (g_.level >= w_.sc.count(Scenario::Maze)) g_.level = 1;
    xgoto_ = g_.level == 0 ? 10 : 9;
    endFight();
}

// CASTASPE:32 -- a monster breathes on the party
void Combat::breathe() {
    begin(Dos::substitute(d_.str(1135), nameOf(grpIdx_, unitIdx_, 1510)));
    for (int i = 0; i < partyCount(); ++i) {
        if (unit(0, i, 7) >= 5) continue;
        int amount = unit(grpIdx_, unitIdx_, 3) / 2;
        if (rnd(20) >= int(chr(i).luckSkill[3])) amount = (amount + 1) / 2;
        if (bit(rawWord(chr(i), 98), G_[grpIdx_][119])) amount = (amount + 1) / 2;
        if (amount == 0) amount = 1;
        hurt(0, i, amount);
    }
    throw EndAction{};
}

// CASTASPE:31 -- the caster cannot cast
void Combat::cannotCast(int strNo, const std::string &spell) {
    std::string s = d_.str(strNo);
    s = Dos::substitute(s, nameOf(grpIdx_, unitIdx_, grpIdx_ == 0 ? 1490 : 1500));
    s = Dos::substituteChar(s, 63, spell);
    begin(s);
    throw EndAction{};
}

// CASTASPE:6..13 -- HAMAN / MAHAMAN: a boon of the caster's choice from three offered
void Combat::wish(int spell) {
    Character &c = chr(unitIdx_);
    if (c.charLevel < 13) { d_.putStrC(msg_, 1063); throw EndAction{}; }
    --c.charLevel;
    flag_[unitIdx_] = 1;
    bool chosen[7] = {};
    int span = 4 + (spell == 7 ? 3 : 0);
    for (int k = 1; k <= 3; ++k) {
        int o = rnd(span);
        while (chosen[o]) o = (o + 1) % 7;
        chosen[o] = true;
    }
    Win *b = d_.con.open(0, 9, 40, 8, 31, true);
    d_.putStrC(b, 1064);
    d_.newline(b);
    int o = -1;
    for (int k = 1; k <= 3; ++k) {
        ++o;
        while (!chosen[o]) ++o;
        d_.putNum(b, k, 1);
        d_.putS(b, ") ");
        d_.putStr(b, 1065 + o);
        d_.newline(b);
    }
    int pick = d_.getNum(3);
    d_.con.close(b, true);
    int which = -1;
    for (int k = 1; k <= pick; ++k) {
        ++which;
        while (!chosen[which]) ++which;
    }
    msgPending_ = false;
    switch (which) {
        case 0:                                                        // CURE PARTY
            for (int i = 0; i < partyCount(); ++i) if (unit(0, i, 7) < 5) unit(0, i, 7) = 0;
            break;
        case 1:                                                        // SILENCE ENEMIES
            for (int g = 1; g <= 3; ++g)
                for (int u = 0; u <= G_[g][1] - 1; ++u) unit(g, u, 5) = 5 + rnd(5);
            break;
        case 2:                                                        // AUGMENT MAGIC
            for (int g = 1; g <= 3; ++g) {
                for (int u = 0; u <= G_[g][1] - 1; ++u) unit(g, u, 6) = 1;
                G_[g][120] = 0;
            }
            break;
        case 3:                                                        // TELEPORT ENEMIES
            for (int g = 1; g <= 4; ++g)
                for (int u = 0; u <= G_[g][1] - 1; ++u) { unit(g, u, 7) = 5; unit(g, u, 3) = 0; }
            break;
        case 4:                                                        // HEAL PARTY
            for (int i = 0; i < partyCount(); ++i) if (unit(0, i, 7) < 5) unit(0, i, 3) = chr(i).hpMax;
            break;
        case 5: g_.protect = 20; break;                                // PROTECT PARTY
        case 6:                                                        // RAISE THE DEAD
            for (int i = 0; i < partyCount(); ++i) if (unit(0, i, 7) != 7) unit(0, i, 7) = 0;
            for (int i = 0; i < partyCount(); ++i) if (unit(0, i, 7) < 5) unit(0, i, 3) = chr(i).hpMax;
            break;
        default: break;
    }
}

// CASTASPE:29 / 30 -- what each spell does, by the hash of its name.  `u` / `g` are CASTASPE:1's L2 / L3
// (for a party caster: the target group and the unit it picks out, or the target member in `g`; for a
// monster: group 0 and the target member in `u`); `level` is the caster's level.
void Combat::spellEffect(int hash, int u, int g, int level) {
    const int me = unitIdx_;
    auto groupAc = [&](int grp, int delta) { armour(grp, delta, 0, G_[grp][1] - 1); };
    switch (hash) {
        // ---- CASTASPE:29 ----
        case 1449: armour(0, 1, 0, partyCount() - 1); break;                          // KALKI
        case 2301: healDice(0, g, 1, 8); break;                                       // DIOS
        case 3675: hurtDice(g, u, 1, 8); break;                                       // BADIOS
        case 2889: g_.light += 30 + rnd(15); break;                                   // MILWA
        case 2287: armour(0, 4, me, me); break;                                       // PORFIC
        case 3139: armour(0, 2, 0, partyCount() - 1); break;                          // MATU
        case 2619:                                                                    // MANIFO
            for (int k = 0; k <= G_[g][1] - 1; ++k)
                if (unit(g, k, 7) <= 2) afflict(g, k, 50 + 5 * (unit(g, k, 6) - level), 1125, 0);
            break;
        case 5970:                                                                    // MONTINO
            for (int k = 0; k <= G_[g][1] - 1; ++k) afflict(g, k, 40 + 5 * (unit(g, k, 6) - level), 1126, 1);
            break;
        case 5333: g_.light = 10000; break;                                           // LOMILWA
        case 2718:                                                                    // DIALKO
            if (unit(0, g, 7) == 3 || unit(0, g, 7) == 2) {
                unit(0, g, 7) = 0;
                add(Dos::substitute(d_.str(1131), nameOf(0, g, 1480)));
            } else {
                add(Dos::substitute(d_.str(1132), nameOf(0, g, 1480)));
            }
            break;
        case 6491:                                                                    // LATUMAPIC
            for (int k = 1; k <= 4; ++k) G_[k][0] = 1;
            g_.identify = 32767;
            break;
        case 5169: armour(0, 4, 0, partyCount() - 1); break;                          // BAMATU
        case 761: healDice(0, g, 2, 8); break;                                        // DIAL
        case 1253: hurtDice(g, u, 2, 8); break;                                       // BADIAL
        case 9463:                                                                    // LATUMOFIS
            add(Dos::substitute(d_.str(1131), nameOf(0, g, 1480)));
            chr(g).poison = 0;
            break;
        case 4322: g_.protect = 2; break;                                             // MAPORFIC
        case 1614: healDice(0, g, 3, 8); break;                                       // DIALMA
        case 2446: hurtDice(g, u, 3, 8); break;                                       // BADIALMA
        case 4396: blast(g, 3, 8, 1); break;                                          // LITOKAN
        case 1185: add(d_.str(1127)); break;                                          // KANDI
        case 180: add(d_.str(1127)); break;                                           // DI
        case 382:                                                                     // BADI
            afflict(g, u, std::max(25, 50 + 5 * (unit(g, u, 6) - level)), 1128, 2);
            break;
        case 4296: blast(g, 6, 6, 0); break;                                          // LORTO
        case 547:                                                                     // MADI
            if (unit(0, g, 7) < 5) {
                unit(0, g, 3) = chr(g).hpMax;
                unit(0, g, 7) = 0;
                chr(g).poison = 0;
                healDice(0, g, 1, 1);
            }
            break;
        case 759: hurt(g, u, unit(g, u, 3) - rnd(8) - 1); break;                      // MABADI
        case 8330: loktofeit(level); break;                                           // LOKTOFEIT
        case 5514: for (int k = 1; k <= 4; ++k) blast(k, 12, 6, 0); break;            // MALIKTO
        case 6673: add(d_.str(1127)); break;                                          // KADORTO
        // ---- CASTASPE:30 ----
        case 4178: hurtDice(g, u, 1, 8); break;                                       // HALITO
        case 2409: armour(0, 2, me, me); break;                                       // MOGREF
        case 3983: sleepGroup(g, level); break;                                       // KATINO
        case 3340: groupAc(g, -2); break;                                             // DILTO
        case 1953: armour(0, 4, me, me); break;                                       // SOPIC
        case 6181: blast(g, 4, 6, 1); break;                                          // MAHALITO
        case 4731: blast(g, 3, 6, 0); break;                                          // MOLITO
        case 4744: groupAc(g, -3); break;                                             // MORLIS
        case 3180: blast(g, 6, 6, 2); break;                                          // DALTO
        case 6156: blast(g, 6, 6, 1); break;                                          // LAHALITO
        case 7525: for (int k = 1; k <= 4; ++k) groupAc(k, -3); break;                // MAMORLIS
        case 6612: makanito(); break;                                                 // MAKANITO
        case 4925: blast(g, 8, 8, 2); break;                                          // MADALTO
        case 6587:                                                                    // LAKANITO
            if (G_[g][83] != 10)
                for (int k = 0; k <= G_[g][1] - 1; ++k)
                    if (unit(g, k, 7) < 5) afflict(g, k, 6 * G_[g][80], 1128, 2);
            break;
        case 4573: if (G_[g][83] == 10) hurtDice(g, u, 10, 200); break;               // ZILWAN
        case 3990: armour(0, 4, 0, partyCount() - 1); break;                          // MASOPIC
        case 1562: wish(6); break;                                                    // HAMAN
        case 3128: malor(); break;                                                    // MALOR
        case 2597: wish(7); break;                                                    // MAHAMAN
        case 11157:                                                                   // TILTOWAIT
            if (grpIdx_ == 0) { for (int k = 1; k <= 4; ++k) blast(k, 10, 15, 0); }
            else blast(0, 10, 15, 0);
            break;
        default: break;
    }
}

// CASTASPE:24 -- KATINO
void Combat::sleepGroup(int g, int level) {
    for (int u = 0; u <= G_[g][1] - 1; ++u) {
        if (unit(g, u, 7) < 2) {
            int chance = 50 + 5 * (unit(g, u, 6) - level);
            if (g == 0) afflict(0, u, chance, 1129, 3);
            else afflict(g, u, chance + 100 * (bit(u16(G_[g][122]), 4) ? 0 : 1), 1129, 3);
        }
    }
}

// CASTASPE:1
void Combat::cast() {
    try {
        if (act_ == -3) breathe();
        int hash = unit(grpIdx_, unitIdx_, 1);
        std::string spell;
        for (int s = 1; s <= 50; ++s)
            if (spellHash(s) == hash) spell = d_.str(5000 + s);
        if (!spell.empty() && spell[0] == '*') spell.erase(0, 1);
        if (unit(grpIdx_, unitIdx_, 5) > 0) cannotCast(1133, spell);
        if (g_.g30 > 0) cannotCast(1134, spell);
        std::string s = d_.str(1136);
        s = Dos::substitute(s, nameOf(grpIdx_, unitIdx_, grpIdx_ == 0 ? 1490 : 1500));
        s = Dos::substituteChar(s, 63, spell);
        begin(s);
        int level = unit(grpIdx_, unitIdx_, 6);
        int tgtGroup, tgtUnit = 0;
        if (grpIdx_ == 0) {
            tgtGroup = unit(0, unitIdx_, 0);
            if (tgtGroup > 0 && tgtGroup < 5 && G_[tgtGroup][1] > 0) tgtUnit = unitIdx_ % G_[tgtGroup][1];
        } else {
            tgtGroup = 0;
            tgtUnit = unit(grpIdx_, unitIdx_, 0);
        }
        spellEffect(hash, tgtUnit, tgtGroup, level);
    } catch (const EndAction &) {
    }
}

// COMBAT:5
void Combat::roundActions() {
    for (time_ = 1; time_ <= 10; ++time_) {
        for (grpIdx_ = 0; grpIdx_ <= 4; ++grpIdx_) {
            int last = G_[grpIdx_][1] - 1;
            for (unitIdx_ = 0; unitIdx_ <= last; ++unitIdx_) {
                if (d_.quit) return;
                if (unit(grpIdx_, unitIdx_, 7) == 0 && unit(grpIdx_, unitIdx_, 2) == time_) {
                    tgt_ = unit(grpIdx_, unitIdx_, 0);
                    act_ = unit(grpIdx_, unitIdx_, 1);
                    if (act_ >= -5 && act_ < 0 && act_ != -3) swing();
                    else if (act_ != 0) cast();
                    if (act_ != 0 && msgPending_) d_.delay(g_.delay);
                }
            }
        }
    }
}

} // namespace wiz
