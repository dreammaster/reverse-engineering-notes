// SEGMENT RUNNER (DOS p-code segnum 20): the maze -- the first-person wireframe
// view, the HUD windows, movement and the special squares.  Ported from the DOS
// p-code (tools/pcode_decomp.py RUNNER all) and drawn through wiz::Console
// exactly as the original does: every line of the view is a CONUNIT DRAWLINE
// into WINDOW1 (docs/drawing.md).
//
// Naming: the original's globals G5..G8 are `dir`, `level`, `y`, `x` here
// (G8 = the first array index = east, G7 = the second = north, matching
// MazeLevel::wall(x, y, dir)).
#pragma once
#include "wiz/castle.h"
#include "wiz/maze.h"

#include <array>

namespace wiz {

// The WIZARDRY globals the maze reads and writes.
struct MazeGlobals {
    int dir = 0;                 // G5   0 N, 1 E, 2 S, 3 W
    int level = 1;               // G6   1..10 (negative: just walked in from the castle)
    int y = 0, x = 0;            // G7, G8
    int g9 = 0;                  // G9   scratch handed to the next segment (SCNMSG index)
    int g22 = 1;                 // G22  the roster window is wanted
    int g23 = 0;                 // G23  the HUD is switched off (O)FF
    int delay = 20;              // G28  DELAY (1-99)
    int g29 = 0;                 // G29  combat context: 0 normal, 1 fight-room, 2 ambush
    int g30 = 0;                 // G30  FIZZLE flag
    int g31 = 0;                 // G31  forced-encounter flag
    int light = 0;               // G33  LIGHT turns
    int protect = 0;             // G34  PROTECT (AC bonus)
    int identify = 0;            // G32  IDENTIFY turns
    int g37 = 0;                 // G37  encounter size handed to COMBAT
    int xgoto2 = 1;              // XGOTO2: 1 a wandering encounter (can be fled), 0 a fixed one
    int g35 = 0;                 // G35  the monsters' strength this round (combat)
    int g36 = 0;                 // G36  XGOTO 24's sub-function (CAMP's jobs for other segments)
    int g38 = 0, g39 = 0, g40 = 0;   // G38..G40 the square the party just left
    int g41 = 7;                 // G41  the XGOTO to return to after CAMP
    bool randomStart = false;    // G54  start at a random open square
    bool fight[20][20] = {};     // G323 the "unfought room" mask
    // the maze records as the game has changed them (the original rewrites them on disk):
    // fixed-encounter counters, used-up special squares
    std::vector<u8> levelRec[11];
};

// the working copy of maze level `level` (1..10), loaded from the scenario on first use
std::vector<u8> &mazeRecord(MazeGlobals &g, const Scenario &sc, int level);

struct MazeWorld {
    Party &party;
    Roster &roster;
    const Scenario &sc;
    Rng &rng;
    std::string rosterPath, partyPath;
    MazeGlobals &g;
    Win *&g10;                   // the roster window (shared with the Castle / CAMP)
};

struct MazeResult {
    int xgoto = 0;               // the WIZARDRY mainline's next XGOTO (8 COMBAT, 9 level change /
                                 // quit, 12 CAMP, 16 party gone, 23 SPECIALS, 26 INSPECT)
    bool windowClosed = false;
};

class MazeScreen {
public:
    MazeScreen(Dos &d, MazeWorld &w) : d_(d), w_(w), g_(w.g) {}

    // UTILITIE:9/10 -- the walk-in from the castle: records the party's whereabouts and
    // builds the "unfought room" mask (consumes the RNG exactly as the original).
    void enter();
    // RUNNER:1
    MazeResult run();
    // RUNNER:3: draw the first-person view into WINDOW1
    void drawView();

private:
    // ---- the view (RUNNER:3 and the procs nested in it) ----
    struct View {
        int lo = 0, hi = 35;               // L4, L3: the clip columns
        int saveHi = 35, saveLo = 0;       // L1, L2: the clip when this depth began
        int row = 0, col = 0;              // L6, L5: the square being drawn
        int right = 0, front = 0, left = 0;    // L7, L8, L9 wall types
        bool lit = true;                   // L11
        bool lamp = false;                 // L12
        bool lWall = false, rWall = false; // L14, L13
        bool leftVis = true, leftFwdVis = true, rightVis = true, rightFwdVis = true, frontVis = true;  // L19 L16 L18 L15 L17
    } v_;
    Win *w1_ = nullptr;

    void line(int col, int row, int dCol, int dRow, int count, int glyph);   // RUNNER:5
    void dot(int col, int row, int glyph) { line(col, row, 0, 0, 1, glyph); }   // RUNNER:6
    void step(int &a, int &b, int right, int fwd) const;                    // RUNNER:7
    void fillLeft(int col, int row, int w, int h, int clip);                // RUNNER:8
    void fillRight(int col, int row, int w, int h, int clip);               // RUNNER:9
    void fillBox(int col, int row, int w, int h);                           // RUNNER:10
    bool visible(int right, int fwd) const;                                 // RUNNER:11
    void stepForward();                                                     // RUNNER:12
    int  wallOf(int rel, int shift) const;                                  // RUNNER:13/14/15
    int  fixHidden(int t) const;                                            // RUNNER:16
    void classify();                                                        // RUNNER:17
    void slant(int g1, int g2, int g3, int col, int row, int depthA, int e11, int e12, int e13, int e14);  // RUNNER:20
    void marker(int x, int y, int mr, int mc, const int g[6], const int e[4], int depthA);    // RUNNER:19
    void markers(int a1, int a2, int a3);                                   // RUNNER:18
    // RUNNER:21 / 27 / 33 / 39.  true = RUNNER:4 ran: the view is presented and drawing ends
    bool depth0(); bool depth1(); bool depth2(); bool depth3();
    void present1() { d_.con.present(w1_); }                                // RUNNER:4 (the present half)

    // ---- the loop and the squares ----
    void openWindows();                                                     // RUNNER:72
    void partyRows();                                                       // RUNNER:49
    void rowStatus(int i);                                                  // RUNNER:50
    void closeHud(bool showRoster);                                         // RUNNER:48
    void finish(int xgoto);                                                 // EXIT(20:1) with XGOTO
    void effectLine(int turns, int strNo);                                  // RUNNER:73
    void readKey();                                                         // RUNNER:2
    void stepAhead();                                                       // RUNNER:65
    void flashGlyph(int ch);                                                // RUNNER:67
    void move(int glyph);                                                   // RUNNER:68/66
    void turn(int delta, int glyph);                                        // RUNNER:69
    void showMsg(int strNo);                                                // RUNNER:45
    void showMsgText(const std::string &s);
    void stepEffects();                                                     // RUNNER:51
    void encounter();                                                       // RUNNER:52
    void clearRoom();                                                       // RUNNER:46/47
    void square();                                                          // RUNNER:55
    void stairs();                                                          // RUNNER:58
    void teleport();                                                        // RUNNER:57
    void spinner();                                                         // RUNNER:56
    void darkness();                                                        // RUNNER:59
    void rock();                                                            // RUNNER:60
    void damageSquare();                                                    // RUNNER:61
    void scnmsg();                                                          // RUNNER:62
    void encSquare();                                                       // RUNNER:63
    void buttons();                                                         // RUNNER:64
    void delayPrompt();                                                     // RUNNER:70
    bool quitPrompt();                                                      // RUNNER:74
    void changeLevel(int lvl);                                              // RUNNER:54
    void loadLevel();
    void save();
    int  rnd(int n);

    Dos &d_;
    MazeWorld &w_;
    MazeGlobals &g_;
    MazeLevel level_;
    int idx_ = 0;                   // the SQREXTRA index of the square being handled (RUNNER:55's L3)
    bool okSeen_ = false;           // RUNNER:49's L108: somebody is still on their feet
    // RUNNER:1's frame words 1..4, 23
    bool moved_ = false, canWander_ = false, redraw_ = false, needRefresh_ = false;
    int lastKey_ = -1;
    Win *bar_ = nullptr;            // U2.6  the command bar (0,0,40,3)
    Win *effects_ = nullptr;        // U2.5  LIGHT / PROTECT / IDENTIFY box
    MazeResult result_;
    bool leave_ = false;
};

} // namespace wiz
