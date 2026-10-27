// SEGMENT UTILITIE (DOS p-code segnum 13), the camp-side half: a character's derived scores and what the
// equipment does to them (UTILITIE:15, 22..28), the E)QUIP dialogues (25, 26, 16, 17, 30, 31), R)EORDER (32)
// and MALOR's teleport prompt (2..8).  The maze walk-in (UTILITIE:9..14) is MazeScreen::enter.
// Ported from the DOS p-code (tools/pcode_decomp.py UTILITIE all).
#pragma once
#include "wiz/maze_dos.h"

namespace wiz {

struct UtilitiesWorld {
    Party &party;
    Roster &roster;
    const Scenario &sc;
    Rng &rng;
    MazeGlobals &g;
    Win *&campTitle;                 // CAMP's G11 (UTILITIE:3 closes it when the party is teleported)
    Win *&campSheet;                 // CAMP's G15
};

struct UtilitiesResult {
    int xgoto = 12;
    int member = -1;                 // G9
    bool windowClosed = false;
};

class Utilities {
public:
    Utilities(Dos &d, UtilitiesWorld &w) : d_(d), w_(w), g_(w.g) {}
    // UTILITIE:1 for XGOTO 13 / 20 (the scores of the whole party are recomputed silently), 14 (E)QUIP: member
    // G9, or everybody when G9 < 0), 15 (R)EORDER) and 24 (MALOR's destination prompt)
    UtilitiesResult run(int xgoto);

    // UTILITIE:15 -- a character's derived scores.  `silent` = the equipment is applied as it is worn;
    // otherwise the character is asked what to equip.
    void derive(int member, bool silent);

private:
    struct EndAll {};            // EXIT(13:1)

    void saveTier(int member, int idx, int amount);               // UTILITIE:27
    void clearResists(Character &c);                              // UTILITIE:28
    void scanItems(int member, bool avail[8]);                    // UTILITIE:22
    void wear(int member, int slot, int item);                    // UTILITIE:23
    void wearAll(int member);                                     // UTILITIE:24
    void chooseOne(int member, int type, const bool avail[8]);    // UTILITIE:25 / 26
    void specials(int member);                                    // UTILITIE:16
    void power(int member, int slot, int item, const ObjectRec &o);   // UTILITIE:17 / 18 / 19 / 20
    void equipEveryone();                                         // UTILITIE:31
    void reorder();                                               // UTILITIE:32
    void teleportPrompt();                                        // UTILITIE:2
    void teleportApply(int east, int north, int up, Win *menu);   // UTILITIE:3..8
    void setRaw(Character &c, int word, u16 v);

    Dos &d_;
    UtilitiesWorld &w_;
    MazeGlobals &g_;
    int xgoto_ = 12;
    Win *back_ = nullptr;        // BACKGND
};

} // namespace wiz
