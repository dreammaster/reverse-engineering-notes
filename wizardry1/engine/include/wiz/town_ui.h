// The Castle and its establishments -- the "town".
//
// Ports the menu flow of CASTLE (P010A01) and SHOPS (P010201); see
// docs/town.md.  Ported: the Castle hub, Gilgamesh's Tavern, the Adventurer's
// Inn, Boltac's Trading Post, the Temple of Radiant Cant (wiz/temple.h), and
// the Edge of Town.
#pragma once
#include "wiz/roller_ui.h"          // Ui
#include "wiz/party.h"
#include "wiz/shop.h"

namespace wiz {

class Scenario;
class StringPool;

// Where the town hands control next (the WIZARDRY-mainline XGOTO, trimmed to
// what the standalone engine currently reaches).
enum class TownExit {
    ToRoller,        // XTRAININ  -- Training Grounds
    ToMaze,          // XNEWMAZE  -- enter the maze
    LeaveGame,       // XDONE
    WindowClosed,    // the SDL window was closed
};

// The persistent state the town reads and writes.  Empty paths = no autosave.
struct TownWorld {
    Party &party;
    Roster &roster;
    Shop &shop;
    const Scenario &sc;
    const StringPool *sp = nullptr;     // item names; null -> "ITEM n"
    Rng &rng;
    std::string rosterPath, partyPath, shopPath, mazePath;
};

// Run the Castle hub until the player leaves via the Edge of Town.
TownExit runTown(Ui &ui, TownWorld &world);

// CHK4WIN (SHOPS P01021D / CONGRATS P01021E) -- call once the party is back
// in the Castle.  If any member carries the Amulet of Werdna (object 94) the
// quest is complete: show the CONGRATULATIONS screen, grant +250,000 XP each,
// and take the party's equipment and all but a little gold as the price of
// initiation into the Overlord's Honor Guard.  Returns true if it fired.
bool endgameCheck(Ui &ui, Party &party);

} // namespace wiz
