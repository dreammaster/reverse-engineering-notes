// SEGMENT CASTLE (DOS p-code segnum 16): the Castle hub, Gilgamesh's Tavern
// and the Adventurer's Inn -- ported from the DOS p-code (tools/pcode_decomp.py
// CASTLE all), drawing through wiz::Dos / wiz::Console.
//
// CASTLE:1 is the hub; the WIZARDRY mainline then dispatches on XGOTO to the
// other segments.  runCastle returns that XGOTO (and the Tavern's "inspect
// member" index) so the caller can dispatch.
#pragma once
#include "wiz/dos.h"
#include "wiz/party.h"
#include "wiz/roster.h"
#include "wiz/rng.h"
#include "wiz/scenario.h"

namespace wiz {

// XGOTO values the Castle sets (WIZARDRY:1's dispatch table).
enum XGoto : int {
    XTRAININ = 1,       // ROLLER
    XCASTLE = 2,        // CASTLE hub
    XCASTLE2 = 3,       // CASTLE, straight into the Tavern
    XCAMP = 4,          // CAMP (the Tavern's Inspect, G9 = member)
    XSHOPS = 5,         // SHOPS: Boltac's
    XTEMPLE = 6,        // SHOPS/UTILITIE: Temple of Cant
    XEDGE = 25,         // SHOPS: Edge of Town
};

struct CastleWorld {
    Party &party;
    Roster &roster;
    const Scenario &sc;
    Rng &rng;
    std::string rosterPath, partyPath;     // empty = no autosave
};

struct CastleResult {
    int xgoto = XCASTLE;
    int member = -1;                       // XCAMP: party index to inspect
    bool windowClosed = false;
};

// KANJIREA:37 -- S)TART GAME: every character still flagged INMAZE from the last
// session walks back to town (INMAZE := false; a character not lost in the
// dungeon also has LOCATION[2] zeroed).
void startupResetInMaze(Roster &roster);

class Castle {
public:
    Castle(Dos &d, CastleWorld &w) : d_(d), w_(w) {}

    // Runs until a menu choice leaves the Castle.  `entry` = XGOTO on entry
    // (XCASTLE2 opens in the Tavern, as CASTLE:1 does).
    CastleResult run(int entry);

    // Drop the Castle's persistent windows (G10/G13) -- the maze / roller do this.
    void closeWindows();

private:
    // CASTLE:3/6/7 -- the roster window
    void charRow(int i);
    void header();
    void rows();
    // CASTLE:8/9..17 -- Gilgamesh's Tavern
    CastleResult tavern();
    void addMember();
    void removeMember();
    void divvyGold();
    int  partyAlign() const;
    // CASTLE:20..41 -- the Adventurer's Inn
    void inn();
    void nap(Win *win, int member, int hpPerWeek, int goldPerWeek);
    void innWelcome(Win *win, int member);
    void checkLevel(Win *win, int member);
    void levelUp(Win *win, int member);
    void learnSpells(Win *win, int member, bool &learned);
    void gainLose(Win *win, int member);
    void attrMessage(Win *win, int strNo, int attr);
    int  hpRoll(int member);
    void save();
    int  rnd(int n);

    Dos &d_;
    CastleWorld &w_;
    Win *roster_ = nullptr;        // G10
    Win *title_ = nullptr;         // G13
};

} // namespace wiz
