// SEGMENT SPECIALS (DOS p-code segnum 15): the scripted squares of the maze (SCNMSG: signs, riddles,
// tolls, traps, doors that want an item, items lying about, fixed fights), the I)NSPECT command
// that picks up the bodies of fallen characters, and DUMAPIC's position report.  Ported from the
// DOS p-code (tools/pcode_decomp.py SPECIALS all).
#pragma once
#include "wiz/maze_dos.h"

namespace wiz {

struct SpecialsWorld {
    Party &party;
    Roster &roster;
    const Scenario &sc;
    Rng &rng;
    std::string rosterPath, partyPath;
    MazeGlobals &g;
};

struct SpecialsResult {
    int xgoto = 7;                   // the mainline's next XGOTO (7 RUNNER, 8 COMBAT, 9 level change, 27 CAMP)
    bool windowClosed = false;
};

class Specials {
public:
    Specials(Dos &d, SpecialsWorld &w) : d_(d), w_(w), g_(w.g) {}
    // SPECIALS:1.  `xgoto` = 23 (a SCNMSG square, descriptor G9), 26 (I)NSPECT) or 24 (DUMAPIC's report).
    SpecialsResult run(int xgoto);

private:
    struct End10 {};                 // EXIT(15:10): the square's handler ends
    struct EndAll {};                // EXIT(15:1): back to the mainline

    // ---- the message window (SPECIALS:11..18) ----
    void openMsg(int lines);                         // 11
    int  countLines(int strNo);                      // 18
    bool printLine();                                // 16: false while there are more lines
    void pageBreak();                                // 17
    void pressReturn();                              // 15
    void showMessage(int n);                         // 14
    bool askYesNo(const std::string &q);             // 13
    // ---- the SCNMSG kinds (SPECIALS:10) ----
    void scnmsg();                                   // 10
    void backOut();                                  // 38
    bool giveItem(int member, int item);             // 19
    void kindItem();                                 // 20
    void kindWade();                                 // 21 (+22)
    void kindSearch();                               // 23
    void kindTrade();                                // 24 (+25)
    void kindNeedItem();                             // 27
    void kindGate();                                 // 28
    void kindLight();                                // 29
    void kindToTown();                               // 30
    void kindRiddle();                               // 31
    void kindToll();                                 // 33 (+34, 35, 36)
    void kindNest();                                 // 37
    void pushBack();                                 // 26
    // ---- I)NSPECT, DUMAPIC ----
    void inspect();                                  // 2 (+3..8)
    void position();                                 // 9
    void markRegion(bool (&reach)[20][20]);          // 7

    Dos &d_;
    SpecialsWorld &w_;
    MazeGlobals &g_;
    Win *msg_ = nullptr;             // SPECIALS:10's L1
    int row_ = 0;                    // SPECIALS:10's L2: the next line of the message window
    int lineLen_ = 0;                // SPECIALS:14's L1: the length of the line just centred
    int strNo_ = 0;                  // SPECIALS:14's a1: the next string of the message
    int idx_ = 0;                    // the descriptor index (G9)
    int a0_ = 0, a1_ = 0, a2_ = 0;   // its AUX0 / AUX1 / AUX2
    int xgoto_ = 7;
};

} // namespace wiz
