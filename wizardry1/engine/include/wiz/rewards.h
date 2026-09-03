// REWARDS  (P010D01)  -- experience + treasure after a won fight.
//
//   GIVEEXP   real per-kill XP (CALCKILL); EXPAMT in the record is unused
//             (the source literally comments  "KILLEXP := ENEMYREC.EXPAMT; LOL")
//   CHSTGOLD  ENMYREWD -> pick the ZREWARD record, run its reward list,
//             split the gold among the survivors, drop items on random members
//   ACHEST    the chest / trap mini-game (GTTRAPTY + DOTRAPDM)  -- data +
//             resolution here, the O)PEN/I)NSPECT/D)ISARM/C)ALFO loop lives in
//             combat_ui.cpp
//
// See docs/combat.md.
#pragma once
#include "wiz/scenario.h"
#include "wiz/party.h"
#include "wiz/rng.h"
#include <cstdint>
#include <string>
#include <vector>

namespace wiz {

class StringPool;
struct Battle;

// ---- ZREWARD record (TREWARD, 168 B / 84 words) ------------------------
// word 0  BCHEST                 word 1  BTRAPTYP  packed bool[0..7]
// word 2  REWRDCNT               words 3.. REWARDXX[1..9], 9 words each
struct RewardEntry {              // TREWARDX
    const u8 *p = nullptr;
    u16 w(int i) const { return rd16(p + i * 2); }
    int  perc() const   { return i16(w(0)); }        // apply if perc >= rand%100
    bool isItem() const { return i16(w(1)) != 0; }   // BITEM  (0 = gold)
    // gold sub-record
    int  gTries()  const { return i16(w(2)); }
    int  gAve()    const { return i16(w(3)); }
    int  gMin()    const { return i16(w(4)); }
    int  gMult()   const { return i16(w(5)); }
    int  gTries2() const { return i16(w(6)); }
    int  gAve2()   const { return i16(w(7)); }
    int  gMin2()   const { return i16(w(8)); }
    // item sub-record
    int  iMinIdx()  const { return i16(w(2)); }
    int  iFactor()  const { return i16(w(3)); }
    int  iMaxTimes()const { return i16(w(4)); }
    int  iRange()   const { return i16(w(5)); }
    int  iPercBigr()const { return i16(w(6)); }
};

struct RewardRec {
    Bytes b;
    u16 w(int i) const { return rd16(b.p + i * 2); }
    bool chest() const          { return i16(w(0)) != 0; }
    bool trapPossible(int t) const { return (w(1) >> (t & 7)) & 1; }   // t 0..7
    int  count() const          { return i16(w(2)); }                 // 0..9
    RewardEntry entry(int i) const {                                  // i 1..9
        return { b.p + (3 + (i - 1) * 9) * 2 };
    }
};

// ---- experience -------------------------------------------------------
// CALCKILL: XP for one kill of monster `m` (no EXPAMT involved).
int64_t killExp(const MonsterRec &m);

// GIVEEXP: sum killExp * (monsters killed) over the groups, divide by the
// number of conscious members, add to each.  Logs the "EACH SURVIVOR GETS n"
// line.  Returns the per-survivor amount.
int64_t giveExp(const Battle &bt, const Scenario &sc, Party &party,
                std::vector<std::string> &log);

// ---- treasure --------------------------------------------------------
struct ItemGrant { int who; int itemIndex; };

// CHSTGOLD (minus the chest loop): ENMYREWD picks REWARD1/REWARD2 from group
// 0's monster by `attk012` (0 = wandering, 1 = set-piece first visit -> x2
// gold, 2 = re-fight / scripted -> REWARD2), then walks the reward list.
// Gold is split among the conscious members; items are added to random
// members' packs and appended to `grants`.  Returns the reward index used
// (-1 if the monster has no reward), and sets `hasChest`.
int rollTreasure(const Battle &bt, const Scenario &sc, const StringPool *sp,
                 Party &party, int attk012, Rng &rng,
                 std::vector<std::string> &log, std::vector<ItemGrant> &grants,
                 bool &hasChest);

// Load a ZREWARD record (for the chest loop, which needs it before rolling).
bool loadReward(const Scenario &sc, int rewardIdx, RewardRec &out);
int  chooseRewardIndex(const Battle &bt, const Scenario &sc, int attk012,
                       int &oneOrTwo);

// ---- chest / traps (GTTRAPTY, PRTRAPTY, DOTRAPDM) -------------------
// trap type 0 trapless, 1 poison needle, 2 gas bomb, 3 (see trap3),
// 4 teleporter, 5 anti-mage, 6 anti-priest, 7 alarm
struct ChestTrap { int type = 0; int trap3 = 0; };

// GTTRAPTY: roll the actual trap for this chest on this level.
ChestTrap pickChestTrap(const RewardRec &rw, int mazeLevel, Rng &rng);
std::string trapName(const ChestTrap &t);

// DOTRAPDM: the trap goes off.  Mutates party HP / status / poison; sets
// `teleport` (caller relocates) and `alarm` (caller starts a fight).  Returns
// false if the whole party died (caller -> cemetery).
struct TrapOutcome { bool teleport = false; bool alarm = false; bool wiped = false; };
TrapOutcome springTrap(const ChestTrap &t, Party &party, int chestChar,
                       int mazeLevel, Rng &rng, std::vector<std::string> &log);

} // namespace wiz
