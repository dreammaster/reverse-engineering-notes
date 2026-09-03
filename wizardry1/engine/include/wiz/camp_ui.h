// CAMP (P010C01) -- the in-maze camp menu: the party roster, a full
// character sheet (DSPSTATS + DSPSPELS + DSPITEMS), REORDER, DROP an item,
// READ spell books, and DISBAND.  See docs/maze.md.
#pragma once
#include "wiz/roller_ui.h"          // Ui
#include "wiz/party.h"

namespace wiz {

class Scenario;
class StringPool;

class Roster;

enum class CampExit { ToMaze, Disbanded, WindowClosed };

// Runs the camp menu until the player leaves (L) or disbands (D).  The party
// is mutated in place (reorder / drop).  On DISBAND every member is left as a
// body in the dungeon at (mazeX, mazeY, mazeLevel) with INMAZE cleared and
// age +25 weeks -- a later party can `I`nspect the room to recover them.
CampExit runCamp(Ui &ui, Party &party, Roster &roster, const Scenario &sc,
                 const StringPool *sp, Rng &rng,
                 int mazeX, int mazeY, int mazeLevel);

} // namespace wiz
