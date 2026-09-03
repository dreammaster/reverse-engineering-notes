#include "wiz/combat.h"
#include "wiz/rewards.h"
#include "wiz/string_pool.h"

#include <algorithm>
#include <cstdint>

namespace wiz {

int MonGroup::firstLiving() const {
    for (int i = 0; i < count; ++i)
        if (int(status[i]) < int(Status::Dead)) return i;
    return -1;
}

// ---- CINIT: build the encounter --------------------------------------

// ENGROUPS: resolve `enmyI` through the ENMYTEAM chain to its "real" (UNIQUE)
// record, store it as group `grp`, then -- deeper into the dungeon and on a
// TEAMPERC roll -- recurse to add an allied group.  DOS group indices are
// 1-based (group 0 = the party), so `grp + 1` is the DOS `ENMYGRUP`.
static void engroups(Battle &bt, const Scenario &sc, int enmyI, int grp,
                     int mazeLevel, Rng &rng) {
    if (grp >= 4) return;
    int nmon = sc.count(Scenario::Monster);

    MonsterRec r{sc.record(Scenario::Monster, enmyI)};
    for (int guard = 0; guard < 8 && r.unique() == 0; ++guard) {
        int t = r.enmyTeam();
        if (t < 0 || t >= nmon || t == enmyI) break;
        enmyI = t;
        r = MonsterRec{sc.record(Scenario::Monster, enmyI)};
    }
    bt.grp[grp].enemyId = enmyI;

    int team = r.enmyTeam();
    if (grp + 1 < 4 && team >= 0 && team < nmon && grp + 1 <= mazeLevel &&
        rng.mod(100) < r.teamPerc())
        engroups(bt, sc, team, grp + 1, mazeLevel, rng);
}

void buildEncounter(Battle &bt, const Scenario &sc, int enemyInx, int mazeLevel, Rng &rng) {
    bt = Battle{};
    bt.mazeLevel = mazeLevel;
    int nmon = sc.count(Scenario::Monster);
    if (enemyInx < 0 || enemyInx >= nmon) enemyInx = 0;

    engroups(bt, sc, enemyInx, 0, mazeLevel, rng);

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

    int chance = 20 - v.armorClass - r.hpDice() + 2   // spellHsh == 0 -> +2
               + bt.pAcMod[victim] + bt.acMod2;
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

// ---- DOBREATH (P010906): a breath weapon hits the whole party -------

void monsterBreath(Battle &bt, const Scenario &sc, Party &party, int gi, int ii,
                   Rng &rng, CombatLog &log) {
    if (gi < 0 || gi >= bt.nGroups) return;
    MonGroup &mg = bt.grp[gi];
    if (ii < 0 || ii >= mg.count) return;
    (void)sc;
    log.push_back("IT BREATHES!");
    int base = mg.hp[ii] / 2;                        // TEMP04[BATI].HPLEFT DIV 2
    for (int i = 0; i < party.count(); ++i) {
        Character &v = party.member(i);
        if (int(v.status) >= int(Status::Dead)) continue;
        int dam = base;
        if (rng.mod(20) >= v.luckSkill[3]) dam = (dam + 1) / 2;   // luck save
        // WEPVSTY3[1][BREATHE] breath-type resist is not modelled
        v.hpLeft -= dam;
        char b[48];
        std::snprintf(b, sizeof b, "%s TAKES %d", v.name.c_str(), dam);
        log.push_back(b);
        if (v.hpLeft <= 0) {
            v.hpLeft = 0;
            if (int(v.status) < int(Status::Dead)) v.status = Status::Dead;
            log.push_back(v.name + " IS SLAIN!");
        }
    }
}

// ---- DORUN (P010912): the monster instance flees -------------------

void monsterFlee(Battle &bt, int gi, int ii, CombatLog &log) {
    if (gi < 0 || gi >= bt.nGroups) return;
    MonGroup &mg = bt.grp[gi];
    if (ii < 0 || ii >= mg.count || int(mg.status[ii]) >= int(Status::Dead)) return;
    mg.status[ii] = Status::Dead;
    mg.hp[ii] = 0;
    mg.alive--;
    mg.fled++;
    log.push_back("IT FLEES!");
}

// ---- YELLHELP (P010910): call an ally into the group --------------

void monsterYell(Battle &bt, const Scenario &sc, int gi, Rng &rng, CombatLog &log) {
    if (gi < 0 || gi >= bt.nGroups) return;
    MonGroup &mg = bt.grp[gi];
    MonsterRec r = mg.rec(sc);
    log.push_back("IT CALLS FOR HELP!");
    if (mg.count >= 9 || rng.mod(200) > 10 * r.hpDice()) {
        log.push_back("BUT NONE COMES!");
        return;
    }
    int k = mg.count;
    mg.hp[k] = std::max(1, calcHp(r.hpDice(), r.hpFac(), r.hpAdd(), rng));
    mg.status[k] = Status::OK;
    mg.acMod[k] = 0;
    ++mg.count;
    ++mg.alive;
    log.push_back("AND IT IS HEARD!");
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

// ---- CASTASPE: spell table + effects --------------------------------

namespace {
// kind codes for the compact table below
enum { K_DMG1, K_DMGG, K_HEAL, K_HEALF, K_ACSELF, K_ACPARTY, K_ACPEN, K_SLEEP,
       K_HOLD, K_SILENCE, K_DEATH, K_CUREPARA, K_UNPOISON, K_SETHP, K_NOP };
struct SpRow { const char *name; int lvl; bool pri; SpTarg targ; bool off;
               int kind, a, b, elem; };
const SpRow kTable[51] = {
    {"",0,0,SP_SELF,0,K_NOP,0,0,0},
    {"HALITO",  1,0,SP_ONE_ENEMY,   1,K_DMG1,   1, 8,1},
    {"MOGREF",  1,0,SP_SELF,        0,K_ACSELF, 2, 0,0},
    {"KATINO",  1,0,SP_ENEMY_GROUP, 1,K_SLEEP,  20,0,0},
    {"DUMAPIC", 1,0,SP_SELF,        0,K_NOP,    0, 0,0},
    {"DILTO",   2,0,SP_ENEMY_GROUP, 1,K_ACPEN, -2, 0,0},
    {"SOPIC",   2,0,SP_SELF,        0,K_ACSELF, 4, 0,0},
    {"MAHALITO",3,0,SP_ENEMY_GROUP, 1,K_DMGG,   4, 6,1},
    {"MOLITO",  3,0,SP_ENEMY_GROUP, 1,K_DMGG,   3, 6,0},
    {"MORLIS",  4,0,SP_ENEMY_GROUP, 1,K_ACPEN, -3, 0,0},
    {"DALTO",   4,0,SP_ENEMY_GROUP, 1,K_DMGG,   6, 6,2},
    {"LAHALITO",4,0,SP_ENEMY_GROUP, 1,K_DMGG,   6, 6,1},
    {"MAMORLIS",5,0,SP_ALL_ENEMIES, 1,K_ACPEN, -3, 0,0},
    {"MAKANITO",5,0,SP_ALL_ENEMIES, 1,K_DEATH,  6, 0,0},
    {"MADALTO", 5,0,SP_ENEMY_GROUP, 1,K_DMGG,   8, 8,2},
    {"LAKANITO",6,0,SP_ENEMY_GROUP, 1,K_DEATH,  6, 0,0},
    {"ZILWAN",  6,0,SP_ONE_ENEMY,   1,K_DMG1,   10,200,0},
    {"MASOPIC", 6,0,SP_PARTY,       0,K_ACPARTY,4, 0,0},
    {"HAMAN",   6,0,SP_ALL_ENEMIES, 1,K_DMGG,   6, 6,0},
    {"MALOR",   7,0,SP_SELF,        0,K_NOP,    0, 0,0},
    {"MAHAMAN", 7,0,SP_ALL_ENEMIES, 1,K_DMGG,   7, 6,0},
    {"TILTOWAIT",7,0,SP_ALL_ENEMIES,1,K_DMGG,   10,15,0},
    {"KALKI",   1,1,SP_PARTY,       0,K_ACPARTY,1, 0,0},
    {"DIOS",    1,1,SP_ONE_ALLY,    0,K_HEAL,   1, 8,0},
    {"BADIOS",  1,1,SP_ONE_ENEMY,   1,K_DMG1,   1, 8,0},
    {"MILWA",   1,1,SP_SELF,        0,K_NOP,    0, 0,0},
    {"PORFIC",  1,1,SP_SELF,        0,K_ACSELF, 4, 0,0},
    {"MATU",    2,1,SP_PARTY,       0,K_ACPARTY,2, 0,0},
    {"CALFO",   2,1,SP_ONE_ENEMY,   1,K_NOP,    0, 0,0},
    {"MANIFO",  2,1,SP_ENEMY_GROUP, 1,K_HOLD,   0, 0,0},
    {"MONTINO", 2,1,SP_ENEMY_GROUP, 1,K_SILENCE,0, 0,0},
    {"LOMILWA", 3,1,SP_SELF,        0,K_NOP,    0, 0,0},
    {"DIALKO",  3,1,SP_ONE_ALLY,    0,K_CUREPARA,0,0,0},
    {"LATUMAPI",3,1,SP_ALL_ENEMIES, 0,K_NOP,    0, 0,0},
    {"BAMATU",  4,1,SP_PARTY,       0,K_ACPARTY,4, 0,0},
    {"DIAL",    4,1,SP_ONE_ALLY,    0,K_HEAL,   2, 8,0},
    {"BADIAL",  4,1,SP_ONE_ENEMY,   1,K_DMG1,   2, 8,0},
    {"LATUMOFI",4,1,SP_ONE_ALLY,    0,K_UNPOISON,0,0,0},
    {"MAPORFIC",4,1,SP_PARTY,       0,K_ACPARTY,-99,0,0},   // -99 -> acMod2 := 2
    {"DIALMA",  5,1,SP_ONE_ALLY,    0,K_HEAL,   3, 8,0},
    {"BADIALMA",5,1,SP_ONE_ENEMY,   1,K_DMG1,   3, 8,0},
    {"LITOKAN", 5,1,SP_ENEMY_GROUP, 1,K_DMGG,   3, 8,1},
    {"KANDI",   5,1,SP_SELF,        0,K_NOP,    0, 0,0},
    {"DI",      5,1,SP_ONE_ALLY,    0,K_NOP,    0, 0,0},
    {"BADI",    5,1,SP_ONE_ENEMY,   1,K_DEATH,  10,0,0},
    {"LORTO",   6,1,SP_ENEMY_GROUP, 1,K_DMGG,   6, 6,0},
    {"MADI",    6,1,SP_ONE_ALLY,    0,K_HEALF,  0, 0,0},
    {"MABADI",  6,1,SP_ONE_ENEMY,   1,K_SETHP,  1, 8,0},
    {"LOKTOFEI",6,1,SP_SELF,        0,K_NOP,    0, 0,0},
    {"MALIKTO", 7,1,SP_ALL_ENEMIES, 1,K_DMGG,   12,6,0},
    {"KADORTO", 7,1,SP_ONE_ALLY,    0,K_NOP,    0, 0,0},
};

// DOHITS: `dice`d`sides` to monster (group g, instance i), UNAFFCT resist roll.
void doHits(Battle &bt, const Scenario &sc, int g, int i, int dice, int sides,
           Rng &rng, CombatLog &log) {
    MonGroup &mg = bt.grp[g];
    if (i < 0 || i >= mg.count || int(mg.status[i]) >= int(Status::Dead)) return;
    int pts = calcHp(dice, sides, 0, rng);
    MonsterRec r = mg.rec(sc);
    if (r.unaffct() > 0 && rng.mod(100) < r.unaffct()) pts = 0;
    if (pts == 0) { log.push_back("IS UNAFFECTED!"); return; }
    mg.hp[i] -= pts;
    char b[48]; std::snprintf(b, sizeof b, "TAKES %d DAMAGE", pts);
    log.push_back(b);
    if (mg.hp[i] <= 0) { mg.hp[i] = 0; mg.status[i] = Status::Dead; mg.alive--;
                         log.push_back("DIES!"); }
}
void hitGroup(Battle &bt, const Scenario &sc, int g, int dice, int sides, Rng &rng, CombatLog &log) {
    MonGroup &mg = bt.grp[g];
    for (int i = 0; i < mg.count; ++i)
        if (int(mg.status[i]) < int(Status::Dead)) doHits(bt, sc, g, i, dice, sides, rng, log);
}
// ISISNOT-style status roll on a monster.
void statusRoll(Battle &bt, int g, int i, int isNotChance, Status st, Rng &rng, CombatLog &log) {
    MonGroup &mg = bt.grp[g];
    if (i < 0 || i >= mg.count || int(mg.status[i]) >= int(Status::Dead)) return;
    if (rng.mod(100) < isNotChance) return;                 // "IS NOT ..."
    if (int(mg.status[i]) < int(st)) { mg.status[i] = st;
        log.push_back(st == Status::Dead ? "ONE IS SLAIN!" :
                      st == Status::Asleep ? "ONE IS SLEPT" : "ONE IS HELD"); }
    if (st == Status::Dead) mg.alive--;
}
} // namespace

const SpellDef *spellDef(int no) {
    static SpellDef d;
    if (no < 1 || no > 50) return nullptr;
    const SpRow &r = kTable[no];
    d = SpellDef{r.name, r.lvl, r.pri, r.targ, r.off};
    return &d;
}

void castSpell(Battle &bt, const Scenario &sc, Party &party, bool casterMon,
               int casterLevel, int no, int tgGroup, int tgInst, int tgAlly,
               Rng &rng, CombatLog &log) {
    if (no < 1 || no > 50) return;
    const SpRow &s = kTable[no];

    if (casterMon) {
        // monster casts at the party -- damage / status land on party members
        int who[Party::kMax], n = livingParty(party, who);
        if (n == 0) return;
        switch (s.kind) {
            case K_DMG1: case K_DMGG: {
                int hit = (s.kind == K_DMGG) ? n : 1;
                for (int k = 0; k < hit; ++k) {
                    Character &v = party.member(who[k]);
                    int pts = calcHp(s.a, s.b, 0, rng);
                    v.hpLeft -= pts;
                    char b[48]; std::snprintf(b, sizeof b, "%s TAKES %d", v.name.c_str(), pts);
                    log.push_back(b);
                    if (v.hpLeft <= 0) { v.hpLeft = 0; v.status = Status::Dead;
                                         log.push_back(v.name + " IS SLAIN!"); }
                }
                break;
            }
            case K_SLEEP: case K_DEATH: {
                Status st = (s.kind == K_DEATH) ? Status::Dead : Status::Asleep;
                for (int k = 0; k < n; ++k) {
                    Character &v = party.member(who[k]);
                    if (rng.mod(20) > v.luckSkill[0]) continue;
                    if (int(v.status) < int(st)) {
                        v.status = st;
                        if (st == Status::Dead) v.hpLeft = 0;
                        log.push_back(v.name + (st == Status::Dead ? " IS SLAIN" : " SLEEPS"));
                    }
                }
                break;
            }
            default: log.push_back("THE SPELL HAS NO EFFECT"); break;
        }
        return;
    }

    // a party member casts
    switch (s.kind) {
        case K_DMG1:
            if (tgGroup >= 0 && tgGroup < bt.nGroups) {
                int i = tgInst >= 0 ? tgInst : bt.grp[tgGroup].firstLiving();
                doHits(bt, sc, tgGroup, i, s.a, s.b, rng, log);
            }
            break;
        case K_DMGG:
            if (s.targ == SP_ALL_ENEMIES)
                for (int g = 0; g < bt.nGroups; ++g) hitGroup(bt, sc, g, s.a, s.b, rng, log);
            else if (tgGroup >= 0 && tgGroup < bt.nGroups)
                hitGroup(bt, sc, tgGroup, s.a, s.b, rng, log);
            break;
        case K_DEATH:
            if (s.targ == SP_ALL_ENEMIES) {
                for (int g = 0; g < bt.nGroups; ++g)
                    for (int i = 0; i < bt.grp[g].count; ++i)
                        statusRoll(bt, g, i, 6 * bt.grp[g].rec(sc).hpDice(), Status::Dead, rng, log);
            } else if (tgGroup >= 0 && tgGroup < bt.nGroups) {
                int i = tgInst >= 0 ? tgInst : bt.grp[tgGroup].firstLiving();
                statusRoll(bt, tgGroup, i, 10 * bt.grp[tgGroup].rec(sc).hpDice(), Status::Dead, rng, log);
            }
            break;
        case K_SLEEP: case K_HOLD:
            if (tgGroup >= 0 && tgGroup < bt.nGroups) {
                MonGroup &mg = bt.grp[tgGroup];
                Status st = (s.kind == K_SLEEP) ? Status::Asleep : Status::Paralyzed;
                for (int i = 0; i < mg.count; ++i)
                    statusRoll(bt, tgGroup, i, 20 * mg.rec(sc).hpDice(), st, rng, log);
            }
            break;
        case K_SILENCE:
            log.push_back("THE MONSTERS ARE SILENCED");
            break;
        case K_ACPEN:
            if (s.targ == SP_ALL_ENEMIES)
                for (int g = 0; g < bt.nGroups; ++g)
                    for (int i = 0; i < 9; ++i) bt.grp[g].acMod[i] += s.a;
            else if (tgGroup >= 0 && tgGroup < bt.nGroups)
                for (int i = 0; i < 9; ++i) bt.grp[tgGroup].acMod[i] += s.a;
            log.push_back("THE ENEMY IS EXPOSED");
            break;
        case K_ACSELF:
            bt.pAcMod[tgAlly] += s.a;
            log.push_back("A SHIELD FORMS");
            break;
        case K_ACPARTY:
            if (s.a == -99) { bt.acMod2 = 2; }
            else for (int i = 0; i < party.count(); ++i) bt.pAcMod[i] += s.a;
            log.push_back("THE PARTY IS SHIELDED");
            break;
        case K_HEAL: case K_HEALF: {
            if (tgAlly < 0 || tgAlly >= party.count()) break;
            Character &v = party.member(tgAlly);
            int pts = (s.kind == K_HEALF) ? v.hpMax : calcHp(s.a, s.b, 0, rng);
            v.hpLeft = std::min(v.hpMax, v.hpLeft + pts);
            if (s.kind == K_HEALF && int(v.status) < int(Status::Dead)) { v.status = Status::OK; v.poison = 0; }
            log.push_back(v.name + " IS HEALED");
            break;
        }
        case K_CUREPARA: {
            if (tgAlly < 0 || tgAlly >= party.count()) break;
            Character &v = party.member(tgAlly);
            if (v.status == Status::Paralyzed || v.status == Status::Asleep) {
                v.status = Status::OK; log.push_back(v.name + " IS CURED!");
            } else log.push_back(v.name + " IS NOT HELPED!");
            break;
        }
        case K_UNPOISON:
            if (tgAlly >= 0 && tgAlly < party.count()) {
                party.member(tgAlly).poison = 0;
                log.push_back(party.member(tgAlly).name + " IS UNPOISONED!");
            }
            break;
        case K_SETHP:
            if (tgGroup >= 0 && tgGroup < bt.nGroups) {
                int i = tgInst >= 0 ? tgInst : bt.grp[tgGroup].firstLiving();
                if (i >= 0) { bt.grp[tgGroup].hp[i] = 1 + rng.mod(8);
                              log.push_back("IT IS HIT BY MABADI!"); }
            }
            break;
        default:
            log.push_back("NOTHING HAPPENS");
            break;
    }
    (void)casterLevel;
}

int distributeRewards(const Battle &bt, const Scenario &sc, const StringPool *sp,
                      Party &party, int attk012, Rng &rng, CombatLog &log) {
    giveExp(bt, sc, party, log);
    std::vector<ItemGrant> grants;
    bool hasChest = false;
    return rollTreasure(bt, sc, sp, party, attk012, rng, log, grants, hasChest);
}

} // namespace wiz
