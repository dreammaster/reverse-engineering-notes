// CAMP (P010C01) -- the in-maze camp menu: the party roster, a full
// character sheet (DSPSTATS + DSPSPELS + DSPITEMS), REORDER, DROP an item,
// READ spell books, and DISBAND.  See docs/maze.md.
#pragma once
#include "wiz/roller_ui.h"          // Ui
#include "wiz/party.h"

namespace wiz {

class Scenario;
class StringPool;

enum class CampExit { ToMaze, Disbanded, WindowClosed };

// Runs the camp menu until the player leaves (L) or disbands (D).  The party
// is mutated in place (reorder / drop).
CampExit runCamp(Ui &ui, Party &party, const Scenario &sc, const StringPool *sp,
                 Rng &rng);

} // namespace wiz
