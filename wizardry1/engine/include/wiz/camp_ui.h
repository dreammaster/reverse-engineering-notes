// CAMP (P010C01) -- the in-maze camp menu: the party roster, a full
// character sheet (DSPSTATS + DSPSPELS + DSPITEMS), REORDER, DROP / TRADE an
// item, READ spell books, cast S)PELLS / U)SE an item, EQUIP, and DISBAND.
// See docs/maze.md.
#pragma once
#include "wiz/roller_ui.h"          // Ui
#include "wiz/party.h"
#include "wiz/maze_ui.h"            // MazeState

namespace wiz {

class Scenario;
class StringPool;
class Roster;

// ToMaze also covers a MALOR teleport that landed on a valid square (the
// caller must reload the level when `st.level` changed); ToTown is a MALOR
// that dumped the party at the castle (or the moat / shops).
enum class CampExit { ToMaze, Disbanded, WindowClosed, ToTown };

// Runs the camp menu until the player leaves (L) or disbands (D).  The party
// is mutated in place (equip / drop / trade / cast).  On DISBAND every member
// is left as a body in the dungeon at (st.pos, st.level) with INMAZE cleared
// and age +25 weeks -- a later party can `I`nspect the room to recover them.
// `st` also carries LIGHT / PROTECT so camp MILWA / MAPORFIC take effect.
CampExit runCamp(Ui &ui, Party &party, Roster &roster, const Scenario &sc,
                 const StringPool *sp, Rng &rng, MazeState &st);

} // namespace wiz
