// SEGMENT ROLLER (DOS p-code segnum 17): the Training Grounds -- character
// creation, inspect / delete / change class / alter password, and the roster
// list.  Ported from the DOS p-code (tools/pcode_decomp.py ROLLER all); it draws
// through wiz::Dos / wiz::Console.  (The older wiz/roller_ui.* is the Apple-derived
// flow and is superseded by this.)
#pragma once
#include "wiz/castle.h"

namespace wiz {

struct RollerWorld {
    Party &party;                       // INSPECT hands the character to CAMP through it
    Roster &roster;
    const Scenario &sc;
    Rng &rng;
    std::string rosterPath, partyPath;
};

struct RollerResult {
    int xgoto = XCASTLE;                // XCASTLE: L)EAVE;  17: INSPECT (CAMP shows the sheet)
    bool windowClosed = false;
};

class RollerScreen {
public:
    RollerScreen(Dos &d, RollerWorld &w) : d_(d), w_(w) {}
    // ROLLER:1.  `xgoto` = 19 when returning from CAMP's character sheet.
    RollerResult run(int xgoto);

private:
    // shared per-session state (the frame of ROLLER:1)
    Character rec_;                     // U1.5   the character being edited
    int slot_ = -1;                     // U1.3   its roster record
    Win *summary_ = nullptr;            // U1.158 the one-line summary window
    Win *title_ = nullptr;              // G12    the "TRAINING GROUNDS" box
    int base_[6] = {};                  // U1.159 racial base scores
    int cur_[6] = {};                   // U1.165 scores being allocated
    bool elig_[8] = {};                 // U1.150 class eligibility

    void summaryLine();                              // ROLLER:3
    bool computeEligible();                          // ROLLER:4/5/6/7
    void setBase();                                  // ROLLER:11/12 (SETXBASE)
    void applyClass(int cls);                        // ROLLER:13
    void rosterList();                               // ROLLER:14
    int  trainingMenu(int xgotoIn);                  // ROLLER:15: 0 back to the menu, 1 INSPECT -> CAMP, 2 closed
    bool confirm(const std::string &verb);           // ROLLER:18
    void deleteChar(bool &leave);                    // ROLLER:19 + 16
    void changeClass();                              // ROLLER:20/21
    void alterPassword();                            // ROLLER:22
    int  pickCharacter();                            // ROLLER:23/24/25/26/27
    void create();                                   // ROLLER:28
    bool findFreeSlot();                             // ROLLER:35
    bool enterName(Win *bottom, Win *mid);           // ROLLER:36
    std::string promptLine(Win *bottom, Win *mid, int strNo, bool pw);  // ROLLER:34
    int  choose(Win *bottom, int strNo, int n);      // ROLLER:29
    void allocate(Win *&bottom, Win *&mid);          // ROLLER:30/31/32/33
    void save();
    void readRec(int slot) { rec_ = w_.roster.slot(slot); slot_ = slot; }
    void writeRec() { w_.roster.slot(slot_) = rec_; save(); }

    Dos &d_;
    RollerWorld &w_;
};

} // namespace wiz
