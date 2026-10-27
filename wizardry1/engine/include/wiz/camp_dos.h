// SEGMENT CAMP (DOS p-code segnum 18): the camp menu, the character sheet (name line, stats,
// spell points, the pack), the R)EAD/E)QUIP/T)RADE/D)ROP/P)OOL GOLD/I)DENTIFY/S)PELL/U)SE
// menu, trading and the camp spells.  Ported from the DOS p-code (tools/pcode_decomp.py
// CAMP all).  What the original hands to other segments comes back as an XGOTO:
//   24 (G36 = 0 READ, 1 IDENTIFY, 2 KANDI, 3 DUMAPIC, 4 MALOR) -> REWARDS / SPECIALS / UTILITIE
//   14 (E)QUIP) and 15 (R)EORDER) -> UTILITIE
#pragma once
#include "wiz/castle.h"
#include "wiz/maze_dos.h"

namespace wiz {

struct CampWorld {
    Party &party;
    Win *&g10;                   // the roster window (shared with the Castle / the maze)
    MazeGlobals &g;              // G6 level, G30 fizzle, G32/G33/G34 spell timers, G41
    const Scenario &sc;
    Rng &rng;
};

struct CampResult {
    int xgoto = 20;              // 20 = L)EAVE (back to the maze / on to RUNNER); 3 / 19: back to the
                                 // Tavern / Training Grounds; 24 / 14 / 15: another segment's job
    int member = -1;             // G9: the character the other segment works on
    int g36 = 0;                 // XGOTO 24's sub-function
    bool windowClosed = false;
};

class Camp {
public:
    Camp(Dos &d, CampWorld &w) : d_(d), w_(w) {}
    // CAMP:1.  `xgoto` = the XGOTO the mainline entered with: 12 / 13 the camp menu, 18 / 27 back
    // from UTILITIE / REWARDS (`member` = G9), 4 the Tavern's INSPECT, 17 the Training Grounds'.
    CampResult run(int xgoto, int member = -1);
    // CAMP:3 -- one roster row
    void row(int i);

private:
    struct Leave { int xgoto; int g36; };          // EXIT(18:1)
    struct ExitCast {};                            // EXIT(18:11)
    struct ExitProc { int proc; };                 // EXIT(18:n) of the other nested procs

    // the sheet (CAMP:5 and the procs nested in it)
    void sheet(int xgoto);                         // CAMP:5
    void drawStats();                              // CAMP:9
    void drawSpellPoints();                        // CAMP:6
    void drawPack();                               // CAMP:7 (+ CAMP:8 compaction)
    void compactPack();                            // CAMP:8
    void drawAwards();                             // CAMP:43
    int  sheetMenu(int mode);                      // CAMP:42
    void command(int sel, int mode);               // CAMP:41
    int  pickMember(int strNo, int exclude);       // CAMP:10
    void tradeAll();                               // CAMP:37
    void tradeGold(int target);                    // CAMP:39
    void tradeItems(int target);                   // CAMP:40
    void dropItem();                               // CAMP:34
    void useItem();                                // CAMP:32
    void identify();                               // CAMP:36
    // messages
    void say(const std::string &s);                // CAMP:4
    // casting (CAMP:11 and the procs nested in it)
    void cast(int hash);                           // CAMP:11
    bool dispatch(int hash);                       // CAMP:24
    int  spellHash(int idx) const;                 // CAMP:26
    std::string spellName(int idx) const;          // CAMP:27
    void need(int level, int spell);               // CAMP:16 (priest) -- fails with "YOU CAN'T CAST THAT"
    void needMage(int level, int spell);
    void spend(int level);                         // CAMP:17
    void spendMage(int level);
    void fizzleCheck();                            // CAMP:14
    int  chooseTarget();                           // CAMP:15
    void heal(int dice, int sides, int level, int spell);   // CAMP:18
    void revive(int level);                        // CAMP:20 / 21
    void endCast(const std::string &msg);          // CAMP:12: message, close, leave CAMP:11

    Dos &d_;
    CampWorld &w_;
    Win *title_ = nullptr;       // G11
    Win *sheet_ = nullptr;       // G15
    Win *back_ = nullptr;        // BACKGND
    int m_ = 0;                  // the character on the sheet (G9)
    bool itemCast_ = false;      // CAMP:11's L1: cast from an item (no charges, no checks)
    int target_ = 0;             // CAMP:11's L5: the chosen target
    int mode_ = 0;               // CAMP:41's L1
    int xgoto_ = 12;
};

} // namespace wiz
