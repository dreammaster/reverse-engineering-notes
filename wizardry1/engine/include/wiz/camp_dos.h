// SEGMENT CAMP (DOS p-code segnum 18): the camp menu -- the roster rows, the CAMP title
// box and the #)INSPECT / R)EORDER / E)QUIP / L)EAVE menu (CAMP:1, 3).  Ported from the
// DOS p-code (tools/pcode_decomp.py CAMP all).  The character sheet (CAMP:5) and the
// R)EORDER / E)QUIP screens are not ported yet: they come back to the caller as XGOTO
// values so it can bridge to the legacy screens.
#pragma once
#include "wiz/castle.h"

namespace wiz {

struct CampWorld {
    Party &party;
    Win *&g10;                   // the roster window (shared with the Castle / the maze)
    int &protect;                // G34  PROTECT bonus shown in the AC column
    int &g41;                    // G41  the XGOTO CAMP returns to when it was entered from the maze
};

struct CampResult {
    int xgoto = 20;              // 20 = L)EAVE; 14 EQUIP; 15 REORDER; 3 / 19: back to the castle / training grounds
    int member = -1;             // INSPECT: the party index (G9)
    bool inspect = false;        // the character sheet was asked for
    bool windowClosed = false;
};

class Camp {
public:
    Camp(Dos &d, CampWorld &w) : d_(d), w_(w) {}
    // CAMP:1.  `xgoto` = the XGOTO the mainline entered with (12 / 13: the camp menu).
    CampResult run(int xgoto);
    // CAMP:3 -- one roster row
    void row(int i);
    // the "CAMP" title box (G11) is closed on L)EAVE
    void closeTitle() { if (title_) d_.con.close(title_, false); }

private:
    Dos &d_;
    CampWorld &w_;
    Win *title_ = nullptr;       // G11
};

} // namespace wiz
