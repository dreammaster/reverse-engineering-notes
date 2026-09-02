#include "wiz/combat.h"
#include "wiz/string_pool.h"

#include <algorithm>

namespace wiz {

int MonGroup::firstLiving() const {
    for (int i = 0; i < count; ++i)
        if (int(status[i]) < int(Status::Dead)) return i;
    return -1;
}

// ---- CINIT: build the encounter --------------------------------------

static void engroups(Battle &bt, const Scenario &sc, int enmyI, int grp, Rng &rng) {
    if (grp >= 4) return;
    int nmon = sc.count(Scenario::Monster);
    // ENGROUPS' ENMYTEAM chain to the "real" (UNIQUE) record.
    MonsterRec r{sc.record(Scenario::Monster, enmyI)};
    for (int guard = 0; guard < 8 && r.unique() == 0; ++guard) {
        int t = r.enmyTeam();
        if (t < 0 || t >= nmon || t == enmyI) break;
        enmyI = t;
        r = MonsterRec{sc.record(Scenario::Monster, enmyI)};
    }
    bt.grp[grp].enemyId = enmyI;
    // Allied groups: the exact TENEMY tail offsets (ENMYTEAM/TEAMPERC) are not
    // yet confirmed, so multi-group encounters are only built from the maze's
    // own descriptors, not by chasing ENMYTEAM here.
    (void)rng;
}

void buildEncounter(Battle &bt, const Scenario &sc, int enemyInx, int mazeLevel, Rng &rng) {
    bt = Battle{};
    bt.mazeLevel = mazeLevel;
    int nmon = sc.count(Scenario::Monster);
    if (enemyInx < 0 || enemyInx >= nmon) enemyInx = 0;

    engroups(bt, sc, enemyInx, 0, rng);

    int cap = std::min(9, 4 + mazeLevel);
    for (int g = 0; g < 4; ++g) {
        MonGroup &mg = bt.grp[g];
        if (mg.enemyId < 0) break;
        bt.nGroups = g + 1;
        MonsterRec r = mg.rec(sc);
        int n = calcHp(r.cntDice(), r.cntFac(), r.cntAdd(), rng);
        n = std::clamp(n, 1, cap);
        mg.count = mg.alive = n;
        for (int i = 0; i < n; ++i) {
            mg.hp[i] = std::max(1, calcHp(r.hpDice(), r.hpFac(), r.hpAdd(), rng));
            mg.status[i] = Status::OK;
        }
    }
}

std::string groupName(const Scenario &sc, const StringPool *sp, const MonGroup &g) {
    bool plural = g.alive > 1;
    int field = g.identified ? (plural ? 3 : 2) : (plural ? 1 : 0);
    std::string s;
    if (sp) {
        bool ok = false;
        s = sp->get(StringPool::monsterNameKey(g.enemyId, field), &ok);
        if (!ok) s.clear();
    }
    if (s.empty()) s = "MONSTER " + std::to_string(g.enemyId);
    return (plural ? std::to_string(g.alive) + " " : std::string()) + s;
}

// ---- DAM2ENMY: a party member attacks a monster group ----------------

void partyAttack(Battle &bt, const Scenario &sc, Character &atk, int tg,
                 Rng &rng, CombatLog &log) {
    if (tg < 0 || tg >= bt.nGroups) return;
    MonGroup &mg = bt.grp[tg];
    int inst = mg.firstLiving();
    if (inst < 0) return;
    MonsterRec r = mg.rec(sc);

    int chance = 21 - r.ac() - atk.hpCalcMd + mg.acMod[inst] - 3 * (tg + 1);
    chance = std::clamp(chance, 1, 19);

    int dmg = 0, hits = 0;
    for (int s = 0; s < atk.swingCnt; ++s)
        if (rng.mod(20) >= chance) {
            dmg += calcHp(atk.hpDamRc[0], atk.hpDamRc[1], atk.hpDamRc[2], rng);
            ++hits;
        }
    if (mg.status[inst] == Status::Asleep) dmg *= 2;
    if ((atk.wepSlay >> (r.cls() & 15)) & 1) dmg *= 2;

    if (dmg == 0) { log.push_back(atk.name + " MISSES"); return; }

    // critical-hit weapon
    bool killed = false;
    if (atk.critHitM) {
        int t = std::min(50, atk.charLevel * 2);
        if (rng.mod(100) < t && rng.mod(35) > r.hpDice() + 10) {
            killed = true;
            log.push_back(atk.name + " -- A CRITICAL HIT!");
        }
    }
    mg.hp[inst] -= dmg;
    if (killed) mg.hp[inst] = 0;
    char b[64];
    std::snprintf(b, sizeof b, "%s HITS %d TIMES FOR %d", atk.name.c_str(), hits, dmg);
    log.push_back(b);
    if (mg.hp[inst] <= 0) {
        mg.hp[inst] = 0;
        if (int(mg.status[inst]) < int(Status::Dead)) { mg.status[inst] = Status::Dead; mg.alive--; }
        log.push_back(atk.name + " KILLS ONE!");
    }
}

// ---- DAM2ME: a monster attacks the party -----------------------------

namespace {
int livingParty(const Party &p, int who[Party::kMax]) {
    int n = 0;
    for (int i = 0; i < p.count(); ++i)
        if (int(p.member(i).status) < int(Status::Dead)) who[n++] = i;
    return n;
}
}

void monsterAttack(Battle &bt, const Scenario &sc, Party &party, int gi, int ii,
                   Rng &rng, CombatLog &log) {
    if (gi < 0 || gi >= bt.nGroups) return;
    MonGroup &mg = bt.grp[gi];
    if (ii < 0 || ii >= mg.count || int(mg.status[ii]) >= int(Status::Dead)) return;
    if (mg.status[ii] == Status::Asleep || mg.status[ii] == Status::Paralyzed) return;
    MonsterRec r = mg.rec(sc);

    int who[Party::kMax];
    int n = livingParty(party, who);
    if (n == 0) return;
    int victim = who[rng.mod(std::min(3, n))];       // monsters favour the front
    Character &v = party.member(victim);

    int chance = 20 - v.armorClass - r.hpDice() + 2;  // spellHsh == 0 -> +2
    chance = std::clamp(chance, 1, 19);

    int dmg = 0, hits = 0;
    for (int a = 1; a <= r.recsN(); ++a) {
        int d, f, ad;
        r.atkDice(a, d, f, ad);
        if (rng.mod(20) >= chance) { dmg += calcHp(d, f, ad, rng); ++hits; }
    }
    if (v.status == Status::Asleep) dmg *= 2;

    std::string nm = groupName(sc, nullptr, mg);
    if (dmg == 0) { log.push_back("A MONSTER MISSES " + v.name); return; }

    v.hpLeft -= dmg;
    char b[64];
    std::snprintf(b, sizeof b, "A MONSTER HITS %s %d TIMES FOR %d", v.name.c_str(), hits, dmg);
    log.push_back(b);

    // CASEDAMG -- status effects from SPPC (0 stone, 1 poison, 2 paralyze, 3 crit)
    u16 sppc = r.sppc();
    auto resist = [&](int ls) { return rng.mod(20) > v.luckSkill[ls]; };
    if ((sppc & 2) && !resist(0) && int(v.status) < int(Status::Dead)) {
        v.poison = 1; log.push_back(v.name + " IS POISONED");
    }
    if ((sppc & 4) && !resist(0) && int(v.status) < int(Status::Paralyzed)) {
        v.status = Status::Paralyzed; log.push_back(v.name + " IS PARALYZED");
    }
    if ((sppc & 1) && !resist(1) && int(v.status) < int(Status::Stoned)) {
        v.status = Status::Stoned; log.push_back(v.name + " IS STONED");
    }
    if (r.drainAmt() > 0 && int(v.status) < int(Status::Dead)) {
        v.charLevel -= r.drainAmt();
        log.push_back(v.name + " IS DRAINED!");
        if (v.charLevel < 1) { v.charLevel = 0; v.hpLeft = 0; v.status = Status::Lost; }
    }
    if ((sppc & 8)) {
        int cb = std::min(50, r.hpDice() * 2);
        if (rng.mod(20) <= v.luckSkill[0] && rng.mod(100) <= cb) {
            v.hpLeft = 0; log.push_back(v.name + " IS CRITICALLY HIT!");
        }
    }
    if (v.hpLeft <= 0) {
        v.hpLeft = 0;
        if (int(v.status) < int(Status::Dead)) v.status = Status::Dead;
        log.push_back(v.name + " IS SLAIN!");
    }
}

// ---- outcome / rewards ----------------------------------------------

bool allMonstersDead(const Battle &bt) {
    for (int g = 0; g < bt.nGroups; ++g)
        if (bt.grp[g].alive > 0) return false;
    return true;
}

bool partyCanFight(const Party &party) {
    for (int i = 0; i < party.count(); ++i) {
        Status s = party.member(i).status;
        if (s == Status::OK || s == Status::Afraid) return true;
    }
    return false;
}

void distributeRewards(const Battle &bt, const Scenario &sc, Party &party,
                       Rng &rng, CombatLog &log) {
    int64_t xp = 0, gold = 0;
    for (int g = 0; g < bt.nGroups; ++g) {
        MonsterRec r = bt.grp[g].rec(sc);
        // EXPAMT is 0 in this scenario; the real GIVEEXP reads the ZREWARD
        // record.  Until that is decoded, approximate from the monster's HD
        // and reward tier.
        int64_t per = r.expAmt().value();
        if (per == 0) per = int64_t(r.hpDice()) * r.hpFac() * 12 + r.reward2() * 8 + 5;
        xp += per * bt.grp[g].count;
        gold += int64_t(bt.grp[g].count) * (rng.mod(bt.mazeLevel * 20 + 10) + 1);
    }
    int living = 0;
    for (int i = 0; i < party.count(); ++i)
        if (int(party.member(i).status) < int(Status::Dead)) ++living;
    if (living == 0) return;
    int64_t eachXp = xp / living, eachGp = gold / living;
    for (int i = 0; i < party.count(); ++i) {
        Character &c = party.member(i);
        if (int(c.status) >= int(Status::Dead)) continue;
        c.exp.v += eachXp;
        c.gold.v += eachGp;
    }
    char b[80];
    std::snprintf(b, sizeof b, "EACH SURVIVOR GAINS %lld E.P. AND %lld GOLD",
                  (long long)eachXp, (long long)eachGp);
    log.push_back(b);
}

} // namespace wiz
