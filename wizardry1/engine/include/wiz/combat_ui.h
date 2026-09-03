// The combat screen + round loop (MELEE's RUNMAIN-equivalent).
// See docs/combat.md.
#pragma once
#include "wiz/roller_ui.h"          // Ui
#include "wiz/party.h"
#include <string>
#include <vector>

namespace wiz {

class Scenario;
class StringPool;

enum class CombatResult { Won, Fled, PartyWiped, WindowClosed, Friendly, Recalled };

// Run one fight against scenario monster index `enemyInx` on maze level
// `mazeLevel`.  The party's HP / status are mutated in place; on `Won` the
// spoils are added to the survivors.  If `transcript` is non-null every log
// line shown during the fight is also appended to it (for headless tests).
// `attk012`: 0 wandering monster, 1 set-piece room first visit (double gold),
// 2 re-fought room / scripted encounter (uses the monster's REWARD2 table).
// `parleyThresh` >= 0 overrides the FRIENDLY class-based threshold (a test
// hook -- the weak PC RNG rarely lands the roll on its own).
CombatResult runCombat(Ui &ui, Party &party, const Scenario &sc,
                       const StringPool *sp, Rng &rng, int enemyInx, int mazeLevel,
                       int attk012 = 2,
                       std::vector<std::string> *transcript = nullptr,
                       int parleyThresh = -1);

} // namespace wiz
