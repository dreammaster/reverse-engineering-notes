// The maze turn loop + HUD -- RUNNER's RUNMAIN / RUNINIT / PRSTATS / SPECSQAR
// (P010E0E / P010E25 / P010E0B / P010E10).  See docs/maze.md.
#pragma once
#include "wiz/roller_ui.h"          // Ui
#include "wiz/party.h"
#include "wiz/runner.h"

#include <map>
#include <string>

namespace wiz {

class Scenario;
class StringPool;
class Roster;

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

    bool active = false;            // a delve is in progress (resume on relaunch)

    // The interrupted-delve save.  DOS Wizardry has no PLAYER.DATA -- the
    // save is SCENARIO.DATA in place; this is the session state the game
    // keeps in globals (MAZEX/Y/LEV/DIRECTIO/LIGHT/ACMOD2/QUICKPLT).
    bool save(const std::string &path) const;
    bool load(const std::string &path);            // false if absent / bad
};

enum class MazeExit {
    ToTown,        // stairs to level 0, camp, inspect, or the player pressed ESC
    PartyWiped,    // XCEMETRY -- everyone dead
    WindowClosed,
};

// Run the maze until the party leaves.  On entry `st.level`/`st.pos` are the
// spawn point (0,0 NORTH, level 1 when arriving from the Edge of Town).
// `roster` is needed by the `I`nspect command to recover lost characters.
MazeExit runMaze(Ui &ui, Party &party, Roster &roster, const Scenario &sc,
                 const StringPool *sp, Rng &rng, MazeState &st);

} // namespace wiz
