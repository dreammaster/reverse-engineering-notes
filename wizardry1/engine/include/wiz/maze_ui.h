// The maze turn loop + HUD -- RUNNER's RUNMAIN / RUNINIT / PRSTATS / SPECSQAR
// (P010E0E / P010E25 / P010E0B / P010E10).  See docs/maze.md.
#pragma once
#include "wiz/roller_ui.h"          // Ui
#include "wiz/party.h"
#include "wiz/runner.h"

#include <map>

namespace wiz {

class Scenario;
class StringPool;

// Persistent maze position + party state carried between the town and the
// maze, and across levels.
struct MazeState {
    int level = 1;                  // MAZELEV
    MazePos pos{0, 0, NORTH};       // MAZEX / MAZEY / DIRECTIO
    int light = 0;                  // LIGHT counter (spell)
    int protect = 0;                // ACMOD2 (spell)
    bool quickPlot = false;         // QUICKPLT

    // SCNMSG one-shot / countdown bookkeeping (the engine holds SCENARIO.DATA
    // read-only, so the AUX0 write-back is tracked here instead).
    // key = level*100 + descriptor index -> times already fired.
    std::map<int, int> scnMsgFired;
};

enum class MazeExit {
    ToTown,        // stairs to level 0, camp, inspect, or the player pressed ESC
    PartyWiped,    // XCEMETRY -- everyone dead
    WindowClosed,
};

// Run the maze until the party leaves.  On entry `st.level`/`st.pos` are the
// spawn point (0,0 NORTH, level 1 when arriving from the Edge of Town).
MazeExit runMaze(Ui &ui, Party &party, const Scenario &sc, const StringPool *sp,
                 Rng &rng, MazeState &st);

} // namespace wiz
