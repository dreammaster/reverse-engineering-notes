// SEGMENT SHOPS (DOS p-code segnum 14): the Edge of Town menu, the Temple of
// Cant and Boltac's Trading Post -- ported from the DOS p-code
// (tools/pcode_decomp.py SHOPS all), drawing through wiz::Dos / wiz::Console.
#pragma once
#include "wiz/castle.h"
#include "wiz/shop.h"

namespace wiz {

// XGOTO values the Edge of Town sets.
enum EdgeXGoto : int {
    XQUIT = 0,
    XROLLER = 1,        // T)RAINING
    XCASTLE_FROM_EDGE = 2,
    XMAZE_ENTRY = 9,    // M)AZE (UTILITIE:1 sets the delve up)
    XUTILITIES = 29,    // U)TILITIES
};

struct ShopsWorld {
    Party &party;
    Roster &roster;
    Shop &shop;                         // Boltac's stock (BOLTACXX)
    const Scenario &sc;
    Rng &rng;
    std::string rosterPath, partyPath, shopPath;
};

struct ShopsResult {
    int xgoto = XCASTLE_FROM_EDGE;
    bool windowClosed = false;
};

class Shops {
public:
    Shops(Dos &d, ShopsWorld &w, Castle &castle) : d_(d), w_(w), castle_(castle) {}

    ShopsResult edgeOfTown();           // SHOPS:30 (XGOTO 25)
    void temple();                      // SHOPS:2  (XGOTO 6)
    void boltac();                      // SHOPS:14 (XGOTO 5)

private:
    // ---- Edge of Town (SHOPS:30/31/39/40)
    void enterMaze();
    void disbandParty();
    // ---- Temple of Cant (SHOPS:2..13)
    bool templeSelect(Win *win);        // SHOPS:6/7 -- false = leave the Temple
    void templeTreat(Win *win);         // SHOPS:8/9/10/11
    bool templePay(Win *win);           // SHOPS:9 GETPAYER
    void templeMessage(Win *win, const std::string &name, int strNo);   // SHOPS:5
    // ---- Boltac's (SHOPS:14..26)
    void boltacMember(Win *win, int member);     // SHOPS:15
    void buy(Win *win, int member);              // SHOPS:16
    void buyRow(Win *win, int member, int row, int dir);   // SHOPS:17
    void purchase(Win *win, int member);         // SHOPS:20
    void restock(int obj);                       // SHOPS:22
    void trade(Win *win, int member, int mode);  // SHOPS:23 (0 sell, 1 uncurse, 2 identify)
    bool listPossessions(Win *win, int member, int mode);  // SHOPS:24
    void transact(Win *win, int member, int mode, int sel);// SHOPS:25
    void boltacMsg(Win *win, int strNo, int row);          // SHOPS:21 / 26
    void save();

    Dos &d_;
    ShopsWorld &w_;
    Castle &castle_;

    // Temple state (proc 2's frame)
    int patient_ = -1;                  // roster slot (U2.2)
    Character patientRec_;              // U2.47..
    int payer_ = -1;                    // U2.4
    // Boltac state (proc 14/15's frames)
    int shown_[6] = {1, 1, 1, 1, 1, 1}; // object index per displayed row (proc 15 L1[1..6])
    int cursor_ = 2;                    // U3.2 -- the last object index scanned
    int possShown_[8] = {};             // item indices listed by SELL/UNCURSE/IDENTIFY
    int possCount_ = 0;                 // proc 15 L9
};

} // namespace wiz
