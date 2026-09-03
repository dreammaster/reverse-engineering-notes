#include "wiz/rewards.h"
#include "wiz/combat.h"
#include "wiz/string_pool.h"

#include <algorithm>

namespace wiz {
namespace {

// MLTADDKX: 0 if mult == 0, else amount * 2^(mult-1)  (repeated ADDLONGS).
int64_t mltAdd(int mult, int amount) {
    if (mult <= 0) return 0;
    int64_t v = amount;
    for (int i = 1; i < mult && i < 40; ++i) v += v;
    return v;
}

// CALCULAT: min + Σ_tries (rand mod ave + 1).
int64_t calculat(int tries, int ave, int min, Rng &rng) {
    int64_t t = min;
    for (int i = 0; i < tries; ++i) t += rng.mod(ave) + 1;
    return t;
}

int countOk(const Party &p) {
    int n = 0;
    for (int i = 0; i < p.count(); ++i) if (p.member(i).status == Status::OK) ++n;
    return n;
}

} // namespace

// ---- CALCKILL -------------------------------------------------------

int64_t killExp(const MonsterRec &m) {
    int64_t e = int64_t(m.hpDice()) * m.hpFac();       // HPREC.LEVEL * HPREC.HPFAC
    e *= (m.breathe() == 0 ? 20 : 40);
    e += mltAdd(m.magSpels(), 35);
    e += mltAdd(m.priSpels(), 35);
    e += mltAdd(m.drainAmt(), 200);
    e += mltAdd(m.healPts(), 90);
    e += 40 * (11 - m.ac());
    if (m.recsN() > 1) e += mltAdd(m.recsN(), 30);
    if (m.unaffct() > 0) e += mltAdd(m.unaffct() / 10 + 1, 40);
    int wep = 0;
    for (int i = 1; i <= 6; ++i) if ((m.wepVsty3() >> i) & 1) ++wep;
    e += mltAdd(wep, 35);
    int spp = 0;
    for (int i = 0; i <= 6; ++i) if ((m.sppc() >> i) & 1) ++spp;
    e += mltAdd(spp, 40);
    return e < 0 ? 0 : e;
}

int64_t giveExp(const Battle &bt, const Scenario &sc, Party &party,
                std::vector<std::string> &log) {
    int alive = countOk(party);
    if (alive == 0) return 0;

    int64_t total = 0;
    for (int g = 0; g < bt.nGroups; ++g) {
        const MonGroup &mg = bt.grp[g];
        if (mg.enemyId < 0) continue;
        int killed = mg.count - mg.alive - mg.fled - mg.dispelled;  // fled / dispelled earn nothing
        if (killed <= 0) continue;
        total += killExp(mg.rec(sc)) * killed;
    }
    int64_t per = total / alive;
    for (int i = 0; i < party.count(); ++i)
        if (party.member(i).status == Status::OK) party.member(i).exp.v += per;

    log.push_back("FOR KILLING THE MONSTERS");
    log.push_back("EACH SURVIVOR GETS " + std::to_string(per) + " E.P.");
    return per;
}

// ---- ENMYREWD / RDREWARD -------------------------------------------

int chooseRewardIndex(const Battle &bt, const Scenario &sc, int attk012,
                      int &oneOrTwo) {
    oneOrTwo = 1;
    if (bt.nGroups <= 0 || bt.grp[0].enemyId < 0) return -1;
    MonsterRec m0 = bt.grp[0].rec(sc);
    if (attk012 == 0) return m0.reward1();
    if (attk012 == 1) { oneOrTwo = 2; return m0.reward1(); }
    return m0.reward2();
}

bool loadReward(const Scenario &sc, int rewardIdx, RewardRec &out) {
    if (rewardIdx < 0 || rewardIdx >= sc.count(Scenario::Reward)) return false;
    out = RewardRec{ sc.record(Scenario::Reward, rewardIdx) };
    return true;
}

// ---- CHSTGOLD (reward list + GIVEGOLD) -----------------------------

int rollTreasure(const Battle &bt, const Scenario &sc, const StringPool *sp,
                 Party &party, int attk012, Rng &rng,
                 std::vector<std::string> &log, std::vector<ItemGrant> &grants,
                 bool &hasChest) {
    hasChest = false;
    int oneOrTwo = 1;
    int idx = chooseRewardIndex(bt, sc, attk012, oneOrTwo);
    RewardRec rw;
    if (!loadReward(sc, idx, rw)) return -1;
    hasChest = rw.chest();

    int nObj = sc.count(Scenario::Object);
    int64_t goldPot = 0;

    for (int i = 1; i <= rw.count() && i <= 9; ++i) {
        RewardEntry e = rw.entry(i);
        if (e.perc() < rng.mod(100)) continue;             // GETREWRD probability

        if (!e.isItem()) {                                 // GOLDREWD
            int64_t g = calculat(e.gTries(), e.gAve(), e.gMin(), rng);
            g *= e.gMult();
            g *= calculat(e.gTries2(), e.gAve2(), e.gMin2(), rng);
            g *= oneOrTwo;
            goldPot += g;
            continue;
        }

        // ITEMREWD -- a random conscious member with room in their pack
        if (party.count() == 0) continue;
        int who = rng.mod(party.count()), guard = 0;
        while (party.member(who).status != Status::OK && guard < party.count()) {
            who = (who + 1) % party.count();
            ++guard;
        }
        Character &c = party.member(who);
        if (c.status != Status::OK || c.possCount >= 8) continue;

        int times = 0;
        while (calculat(1, 100, 1, rng) < e.iPercBigr() && times < e.iMaxTimes())
            ++times;
        int itemIdx = e.iMinIdx() + int(calculat(1, e.iRange(), 1, rng))
                    + e.iFactor() * times;
        itemIdx = std::clamp(itemIdx, 0, nObj > 0 ? nObj - 1 : 0);

        c.poss[c.possCount] = Possession{false, false, false, itemIdx};
        ++c.possCount;
        grants.push_back({who, itemIdx});

        std::string nm;
        bool ok = false;
        if (sp) nm = sp->get(StringPool::objectNameKey(itemIdx, 0), &ok);
        if (!ok) nm = "ITEM #" + std::to_string(itemIdx);
        log.push_back(c.name + " FOUND - " + nm);
    }

    // GIVEGOLD
    int alive = countOk(party);
    if (alive > 0 && goldPot > 0) {
        int64_t share = goldPot / alive;
        for (int i = 0; i < party.count(); ++i)
            if (party.member(i).status == Status::OK) party.member(i).gold.v += share;
        log.push_back("EACH SHARE IS WORTH " + std::to_string(share) + " GP!");
    }
    return idx;
}

// ---- GTTRAPTY / PRTRAPTY -------------------------------------------

ChestTrap pickChestTrap(const RewardRec &rw, int mazeLevel, Rng &rng) {
    ChestTrap ct;
    bool any = false;
    for (int t = 0; t <= 7; ++t) any = any || rw.trapPossible(t);
    ct.trap3 = rng.mod(5);

    if (!any) { ct.type = 0; return ct; }
    if (rng.mod(15) > 4 + mazeLevel) { ct.type = 0; return ct; }

    int z = rng.mod(100), type = 0, guard = 0;
    while (z > 0 && guard < 4000) {
        do {
            if (type < 7) { z -= (type == 3) ? 5 : 1; type += 1; }
            else          { type = 1; }
            if (++guard >= 4000) break;
        } while (!rw.trapPossible(type));
    }
    ct.type = rw.trapPossible(type) ? type : 0;
    return ct;
}

std::string trapName(const ChestTrap &t) {
    switch (t.type) {
        case 0: return "TRAPLESS CHEST";
        case 1: return "POISON NEEDLE";
        case 2: return "GAS BOMB";
        case 3:
            switch (t.trap3) {
                case 0: return "CROSSBOW BOLT";
                case 1: return "EXPLODING BOX";
                case 2: return "SPLINTERS";
                case 3: return "BLADES";
                default: return "STUNNER";
            }
        case 4: return "TELEPORTER";
        case 5: return "ANTI-MAGE";
        case 6: return "ANTI-PRIEST";
        default: return "ALARM";
    }
}

// ---- DOTRAPDM ------------------------------------------------------
namespace {

bool allDead(const Party &p) {
    for (int i = 0; i < p.count(); ++i)
        if (int(p.member(i).status) < int(Status::Dead)) return false;
    return true;
}

void hpDamage(Party &party, int victim, int hitCnt, int hitDam, Rng &rng,
              std::vector<std::string> &log) {
    if (victim < 0 || victim >= party.count()) return;
    Character &v = party.member(victim);
    int tot = 0;
    for (int i = 0; i < hitCnt; ++i) tot += rng.mod(hitDam) + 1;
    v.hpLeft -= tot;
    if (v.hpLeft < 1) {
        v.hpLeft = 0;
        v.status = Status::Dead;
        log.push_back(v.name + " DIES!");
    }
}

void hpDamAll(Party &party, int chance, int hitCnt, int hitDam, Rng &rng,
              std::vector<std::string> &log) {
    for (int i = 0; i < party.count(); ++i) {
        if (rng.mod(100) < chance)      hpDamage(party, i, hitCnt, hitDam, rng, log);
        else if (rng.mod(100) < chance) hpDamage(party, i, hitCnt, hitDam / 2 + 1, rng, log);
    }
}

void antiPM(Party &party, bool mageDam, Rng &rng) {
    for (int i = 0; i < party.count(); ++i) {
        Character &c = party.member(i);
        bool plyz = rng.mod(20) < c.luckSkill[4];
        auto stone = [&] { if (int(c.status) < int(Status::Stoned)) c.status = Status::Stoned; };
        auto para  = [&] { if (int(c.status) < int(Status::Paralyzed)) c.status = Status::Paralyzed; };
        switch (c.cls) {
            case Class::Mage:    if (mageDam)  { plyz ? para() : stone(); } break;
            case Class::Samurai: if (mageDam && !plyz) para();             break;
            case Class::Priest:  if (!mageDam) { plyz ? para() : stone(); } break;
            case Class::Bishop:  if (!mageDam && !plyz) para();            break;
            default: break;
        }
    }
}

} // namespace

TrapOutcome springTrap(const ChestTrap &t, Party &party, int chestChar,
                       int mazeLevel, Rng &rng, std::vector<std::string> &log) {
    TrapOutcome out;
    switch (t.type) {
        case 0:
            break;
        case 1:                                            // POISON NEEDLE
            if (chestChar >= 0 && chestChar < party.count())
                party.member(chestChar).poison += 1;
            break;
        case 2:                                            // GAS BOMB
            for (int i = 0; i < party.count(); ++i)
                if (rng.mod(20) < party.member(i).luckSkill[3])
                    party.member(i).poison = 1;
            break;
        case 3:                                            // TYPE3DAM
            switch (t.trap3) {
                case 0: hpDamage(party, chestChar, mazeLevel, 8, rng, log); break;
                case 1: hpDamAll(party, 50, mazeLevel, 8, rng, log);        break;
                case 2: hpDamAll(party, 70, mazeLevel, 6, rng, log);        break;
                case 3: hpDamAll(party, 30, mazeLevel, 12, rng, log);       break;
                case 4: if (chestChar >= 0 && chestChar < party.count())
                            party.member(chestChar).status = Status::Paralyzed;
                        break;
            }
            break;
        case 4:                                            // TELEPORTER
            out.teleport = true;
            break;
        case 5: antiPM(party, true, rng);  break;           // ANTI-MAGE
        case 6: antiPM(party, false, rng); break;           // ANTI-PRIEST
        case 7: out.alarm = true;          break;           // ALARM
    }
    out.wiped = allDead(party);
    return out;
}

} // namespace wiz
