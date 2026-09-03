// Combat core -- the COMBAT / CINIT / CUTIL / MELEE / SWINGASW family
// (P010401 .. P010901).  Data model + one-round melee resolution; spell-
// casting and monster spell AI are not ported yet.  See docs/combat.md.
#pragma once
#include "wiz/scenario.h"
#include "wiz/party.h"
#include "wiz/rng.h"
#include <string>
#include <vector>

namespace wiz {

class StringPool;

using CombatLog = std::vector<std::string>;

// One monster group (BATTLERC[1..4]).
struct MonGroup {
    int  enemyId = -1;
    bool identified = false;
    int  count = 0, alive = 0;
    int  hp[9] = {};
    Status status[9] = {};      // OK / Asleep / Dead
    int  acMod[9] = {};

    MonsterRec rec(const Scenario &sc) const { return {sc.record(Scenario::Monster, enemyId)}; }
    int firstLiving() const;
};

struct Battle {
    MonGroup grp[4];
    int  nGroups = 0;
    bool friendly = false;
    int  surprise = 0;              // INITATTK: 1 party surprised, 2 monsters, 0 neither
    int  mazeLevel = 1;
    int  pAcMod[Party::kMax] = {};   // per-battle AC bonus (MOGREF / KALKI / ...)
    int  acMod2 = 0;                 // MAPORFIC party-wide AC
};

// N dice of 1..fac, plus a flat add (CALCHP / ENEMYCNT).
inline int calcHp(int dice, int fac, int add, Rng &rng) {
    int t = add;
    for (int i = 0; i < dice && fac > 0; ++i) t += rng.mod(fac) + 1;
    return t;
}

// CINIT: build the encounter for scenario monster index `enemyInx`.
void buildEncounter(Battle &bt, const Scenario &sc, int enemyInx, int mazeLevel, Rng &rng);

// Display name for a group (identified name vs the unknown name; plural if >1).
std::string groupName(const Scenario &sc, const StringPool *sp, const MonGroup &g);

// DAM2ENMY: `atk` swings at group `tg` (0-based).  Party group index (for the
// −3·group to-hit term) is `tg + 1`.
void partyAttack(Battle &bt, const Scenario &sc, Character &atk, int tg,
                 Rng &rng, CombatLog &log);

// DAM2ME: monster instance (group `gi`, instance `ii`) attacks a random
// living party member.
void monsterAttack(Battle &bt, const Scenario &sc, Party &party, int gi, int ii,
                   Rng &rng, CombatLog &log);

bool allMonstersDead(const Battle &bt);
bool partyCanFight(const Party &party);          // any OK / Afraid member

// ---- CASTASPE ---------------------------------------------------------

enum SpTarg { SP_SELF, SP_ONE_ENEMY, SP_ENEMY_GROUP, SP_ONE_ALLY, SP_PARTY, SP_ALL_ENEMIES };

struct SpellDef {
    const char *name;
    int   level;         // 1..7
    bool  priest;        // else mage
    SpTarg targ;
    bool  offensive;     // true = targets an enemy / enemy group
};
const SpellDef *spellDef(int no);   // no in 1..50, else nullptr

// Apply spell `no` (1..50).  `casterMon` = a monster casts (effects land on
// the party); otherwise a party member casts and enemy effects hit group
// `tgGroup` (`tgInst` a specific monster, `tgAlly` a party member).
void castSpell(Battle &bt, const Scenario &sc, Party &party, bool casterMon,
               int casterLevel, int no, int tgGroup, int tgInst, int tgAlly,
               Rng &rng, CombatLog &log);

// GIVEEXP + CHSTGOLD: real per-kill XP and the ZREWARD treasure list, split
// among the survivors (see wiz/rewards.h).  `attk012`: 0 wandering monster,
// 1 set-piece first visit (double gold), 2 re-fight / scripted (REWARD2).
// `sp` resolves dropped-item names.  Returns the reward index used (or -1).
int distributeRewards(const Battle &bt, const Scenario &sc, const StringPool *sp,
                      Party &party, int attk012, Rng &rng, CombatLog &log);

} // namespace wiz
