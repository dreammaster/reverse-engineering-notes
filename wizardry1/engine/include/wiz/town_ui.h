// The Castle and its establishments -- the "town".
//
// Ports the menu flow of CASTLE (P010A01) and the Edge of Town (EDGETOWN,
// P01021A in SHOPS); see docs/town.md.  The Adventurer's Inn, Boltac's
// Trading Post and the Temple of Cant are stubbed for now.
#pragma once
#include "wiz/roller_ui.h"          // Ui
#include "wiz/party.h"

namespace wiz {

class Scenario;

// Where the town hands control next (the WIZARDRY-mainline XGOTO, trimmed to
// what the standalone engine currently reaches).
enum class TownExit {
    ToRoller,        // XTRAININ  -- Training Grounds
    ToMaze,          // XNEWMAZE  -- enter the maze
    LeaveGame,       // XDONE
    WindowClosed,    // the SDL window was closed
};

// Run the Castle hub until the player leaves via the Edge of Town.  `roster`
// and `party` are persisted to their paths on every change (empty path =
// no autosave).
TownExit runTown(Ui &ui, Party &party, Roster &roster, const Scenario &sc,
                 Rng &rng, const std::string &rosterPath,
                 const std::string &partyPath);

} // namespace wiz
