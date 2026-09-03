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

enum class CombatResult { Won, Fled, PartyWiped, WindowClosed };

// Run one fight against scenario monster index `enemyInx` on maze level
// `mazeLevel`.  The party's HP / status are mutated in place; on `Won` the
// spoils are added to the survivors.  If `transcript` is non-null every log
// line shown during the fight is also appended to it (for headless tests).
CombatResult runCombat(Ui &ui, Party &party, const Scenario &sc,
                       const StringPool *sp, Rng &rng, int enemyInx, int mazeLevel,
                       std::vector<std::string> *transcript = nullptr);

} // namespace wiz
