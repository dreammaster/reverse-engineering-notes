// SEGMENT RUNNER -- see maze_dos.h.  Every routine names the p-code proc it ports;
// `tools/pcode_decomp.py RUNNER <n>` prints the original.
#include "wiz/maze_dos.h"

#include <algorithm>
#include <cstring>

namespace wiz {

// RUNNER's square types by number (the SQRETYPE values the code switches on).
enum : int {
    ST_NORMAL = 0, ST_STAIRS = 1, ST_PIT = 2, ST_CHUTE = 3, ST_SPINNER = 4, ST_DARK = 5,
    ST_TELEPORT = 6, ST_OUCH = 7, ST_BUTTONS = 8, ST_ROCK = 9, ST_FIZZLE = 10, ST_SCNMSG = 11,
    ST_ENCOUNTER = 12,
};

int MazeScreen::rnd(int n) { return w_.rng.mod(n); }

void MazeScreen::save() {
    if (!w_.rosterPath.empty()) w_.roster.save(w_.rosterPath);
    if (!w_.partyPath.empty()) w_.party.save(w_.partyPath);
}

std::vector<u8> &mazeRecord(MazeGlobals &g, const Scenario &sc, int level) {
    std::vector<u8> &v = g.levelRec[level < 1 ? 1 : level > 10 ? 10 : level];
    if (v.empty()) {
        Bytes b = sc.record(Scenario::Maze, level - 1);
        v.assign(b.p, b.p + std::min<size_t>(b.size(), MazeLevel::kBytes));
        v.resize(MazeLevel::kBytes, 0);
    }
    return v;
}

void MazeScreen::loadLevel() {
    std::vector<u8> &v = mazeRecord(g_, w_.sc, g_.level);
    level_.load(Bytes{v.data(), v.size()});
}

// ============================================================================================
//  the view  (RUNNER:3 and the procs nested in it)
// ============================================================================================

// RUNNER:5 -- UNITWRITE(CONUNIT, DRAWLINE) into WINDOW1 with the current clip columns
void MazeScreen::line(int col, int row, int dCol, int dRow, int count, int glyph) {
    DrawLineArgs a;
    a.clipLo = u16(v_.lo); a.clipHi = u16(v_.hi);
    a.col = u16(col); a.row = u16(row);
    a.dCol = u16(dCol); a.dRow = u16(dRow);
    a.count = u16(count); a.glyph = u16(glyph);
    d_.con.drawLine(w1_, a);
}

// RUNNER:7 -- move (a, b) by `right` / `fwd` squares in the facing direction (wraps at 20)
void MazeScreen::step(int &a, int &b, int right, int fwd) const {
    switch (g_.dir & 3) {
        case 0: a += right; b += fwd; break;
        case 1: a += fwd;   b -= right; break;
        case 2: a -= right; b -= fwd; break;
        default: a -= fwd;  b += right; break;
    }
    a = (a + 20) % 20;
    b = (b + 20) % 20;
}

// RUNNER:8 -- a wedge filling the left side of the next depth
void MazeScreen::fillLeft(int col, int row, int w, int h, int clip) {
    for (int c = col; c <= col + w - 1; ++c) {
        dot(c, row, 104);
        line(c, row + 1, 0, 1, h - 2, 103);
        dot(c, row + h - 1, 106);
        ++row;
        h -= 2;
    }
    v_.lo = clip;
}

// RUNNER:9 -- the mirror image on the right
void MazeScreen::fillRight(int col, int row, int w, int h, int clip) {
    for (int c = col; c <= col + w - 1; ++c) {
        dot(c, row, 102);
        line(c, row + 1, 0, 1, h - 2, 103);
        dot(c, row + h - 1, 105);
        --row;
        h += 2;
    }
    v_.hi = clip;
}

// RUNNER:10 -- a solid block
void MazeScreen::fillBox(int col, int row, int w, int h) {
    for (int c = col; c <= col + w - 1; ++c) line(c, row, 0, 1, h, 103);
}

// RUNNER:11 -- can the square (right, fwd) of the one being drawn be seen into?
bool MazeScreen::visible(int right, int fwd) const {
    int r = v_.row, c = v_.col;
    step(r, c, right, fwd);
    if (int(level_.squareAt(r, c)) == ST_DARK)
        return level_.aux2(level_.squareExtra(r, c)) != 0;
    return true;
}

// RUNNER:12 -- advance the square being drawn one step forward and classify its walls
void MazeScreen::stepForward() {
    step(v_.row, v_.col, 0, 1);
    v_.lit = true;
    int idx = level_.squareExtra(v_.row, v_.col);
    int type = int(level_.squareType(idx));
    if (type == ST_DARK) {
        v_.lit = (level_.aux0(idx) == 1);
    } else if (type == ST_TELEPORT) {
        if (level_.aux0(idx) == g_.level) {          // a same-level teleporter is seen through
            v_.row = level_.aux2(idx);
            v_.col = level_.aux1(idx);
        }
    }
    classify();
}

// RUNNER:13/14/15 -- the wall type on the facing (rel 0), left (rel 3) or right (rel 1) edge
// of the square `shift` squares to the right of the one being drawn
int MazeScreen::wallOf(int rel, int shift) const {
    int r = v_.row, c = v_.col;
    step(r, c, shift, 0);
    return int(level_.wall(r, c, (g_.dir + rel) & 3));
}

// RUNNER:16 -- hidden doors are only seen with light
int MazeScreen::fixHidden(int t) const {
    if (t == 3) return (v_.lit && v_.lamp) ? 2 : 1;
    return t;
}

// RUNNER:17
void MazeScreen::classify() {
    v_.front = fixHidden(wallOf(0, 0));
    v_.left = fixHidden(wallOf(3, 0));
    v_.right = fixHidden(wallOf(1, 0));
    v_.frontVis = visible(0, 1);
    v_.leftVis = v_.leftFwdVis = v_.rightVis = v_.rightFwdVis = true;
    if (v_.left == 0) {
        v_.left = fixHidden(wallOf(0, -1));
        v_.leftVis = visible(-1, 0);
        if (v_.leftVis && v_.left == 0) v_.leftFwdVis = visible(-1, 1);
        v_.lWall = false;
    } else {
        v_.lWall = true;
    }
    if (v_.right == 0) {
        v_.right = fixHidden(wallOf(0, 1));
        v_.rightVis = visible(1, 0);
        if (v_.rightVis && v_.right == 0) v_.rightFwdVis = visible(1, 1);
        v_.rWall = false;
    } else {
        v_.rWall = true;
    }
}

// RUNNER:20 -- a marker (stairs / elevator arrow / message sign) painted on a side wall:
// one row per depth step, each row a left glyph run, a filler run and a right glyph run
void MazeScreen::slant(int g1, int g2, int g3, int col, int row, int depthA, int e11, int e12, int e13, int e14) {
    int rows = 16 - depthA;
    int width = rows + rows + 2;
    int dir = row > 9 ? 1 : -1;
    while (rows > 0) {
        if (row < 0 || row > 19) return;
        --rows;
        int last = col + e11;
        for (int c = col; c <= last; ++c)
            if (c >= 0) dot(c, row, g1 + c - col);
        int c2 = col + e11 + 1;
        if (c2 >= 0) {
            line(c2, row, 1, 0, width, g2);
        } else {
            for (int k = 1; k <= width; ++k) {
                if (c2 >= 0) dot(c2, row, g2);
                ++c2;
            }
        }
        int c3 = col + width + 1;
        for (int c = c3; c <= c3 + e12; ++c) dot(c, row, g3 + c - c3);
        col += e13;
        width += e14;
        row += dir;
    }
}

// RUNNER:19 -- the marker for the square at (mr, mc), if its type has one
void MazeScreen::marker(int x, int y, int mr, int mc, const int g[6], const int e[4], int depthA) {
    int idx = level_.squareExtra(mr, mc);
    switch (int(level_.squareType(idx))) {
        case ST_STAIRS:
            if (g_.level < level_.aux0(idx)) slant(g[0], 98, g[1], x, y, depthA, e[0], e[1], e[2], e[3]);
            else slant(g[2], 98, g[3], x, 19 - y, depthA, e[0], e[1], e[2], e[3]);
            break;
        case ST_BUTTONS:
            if (g_.level < level_.aux1(idx)) slant(g[0], 98, g[1], x, y, depthA, e[0], e[1], e[2], e[3]);
            if (g_.level > level_.aux2(idx)) slant(g[2], 98, g[3], x, 19 - y, depthA, e[0], e[1], e[2], e[3]);
            break;
        case ST_SCNMSG:
            slant(g[4], 103, g[5], x, y, depthA, e[0], e[1], e[2], e[3]);
            break;
        default: break;
    }
}

// RUNNER:18 -- the three markers at one depth: the square on the left, on the right and this one
void MazeScreen::markers(int a1, int a2, int a3) {
    int saveLo = v_.lo, saveHi = v_.hi;
    v_.lo = v_.saveLo;
    v_.hi = v_.saveHi;
    int r = v_.row, c = v_.col;
    step(r, c, -1, 0);
    static const int gl[6] = {107, 101, 111, 99, 115, 106};
    static const int el[4] = {1, 0, -2, 1};
    if (!v_.lWall) marker((a1 - a3) - 1, a2, r, c, gl, el, a1);
    step(r, c, 2, 0);
    static const int gr[6] = {100, 109, 97, 113, 105, 117};
    static const int er[4] = {0, 1, 1, 1};
    if (!v_.rWall) marker(a1 + a3, a2, r, c, gr, er, a1);
    static const int gc[6] = {97, 99, 100, 101, 102, 104};
    static const int ec[4] = {0, 0, -1, 2};
    marker(a1, a2, v_.row, v_.col, gc, ec, a1);
    v_.lo = saveLo;
    v_.hi = saveHi;
}

// ---- depth 0 (the square the party stands on): RUNNER:21 and 22..26 --------------------------
bool MazeScreen::depth0() {
    v_.saveHi = v_.hi;
    v_.saveLo = v_.lo;
    // left (22 wall / 23 open)
    if (v_.lWall) {
        if (v_.left != 0) {
            line(1, 0, 0, 1, 20, 12); line(8, 0, 1, 1, 2, 14); line(9, 2, 0, 1, 16, 12); line(9, 18, -1, 1, 2, 13);
            if (v_.left == 2) { line(3, 0, 0, 1, 20, 12); line(6, 0, 1, 1, 2, 14); line(7, 2, 0, 1, 18, 12); }
            v_.lo = 10;
        }
    } else if (!v_.leftVis) {
        fillBox(0, 0, 8, 20); fillLeft(8, 0, 2, 20, 10);
    } else if (v_.left == 0) {
        if (!v_.leftFwdVis) { fillBox(0, 2, 10, 16); v_.lo = 10; }
    } else {
        line(0, 1, 1, 0, 10, 9); line(0, 18, 1, 0, 10, 10); line(9, 2, 0, 1, 16, 12);
        if (v_.left == 2) { line(0, 3, 1, 0, 8, 9); line(7, 4, 0, 1, 14, 12); }
        v_.lo = 10;
    }
    // right (24 wall / 25 open)
    if (v_.rWall) {
        if (v_.right != 0) {
            line(34, 0, 0, 1, 20, 11); line(27, 0, -1, 1, 2, 13); line(26, 2, 0, 1, 16, 11); line(27, 19, -1, -1, 2, 14);
            if (v_.right == 2) { line(32, 0, 0, 1, 20, 11); line(29, 0, -1, 1, 2, 13); line(28, 2, 0, 1, 18, 11); }
            v_.hi = 25;
        }
    } else if (!v_.rightVis) {
        fillBox(28, 0, 8, 20); fillRight(26, 1, 2, 18, 25);
    } else if (v_.right == 0) {
        if (!v_.rightFwdVis) { fillBox(26, 2, 10, 16); v_.hi = 25; }
    } else {
        line(26, 1, 1, 0, 10, 9); line(26, 18, 1, 0, 10, 10); line(26, 2, 0, 1, 16, 11);
        if (v_.right == 2) { line(28, 3, 1, 0, 8, 9); line(28, 4, 0, 1, 14, 11); }
        v_.hi = 25;
    }
    // ahead (26)
    if (v_.front == 0) {
        if (!v_.frontVis) fillBox(10, 2, 16, 16);
    } else {
        line(10, 1, 1, 0, 16, 9); line(9, 2, 0, 1, 16, 12); line(10, 18, 1, 0, 16, 10); line(26, 2, 0, 1, 16, 11);
        if (v_.front == 2) { line(12, 3, 1, 0, 12, 9); line(11, 4, 0, 1, 14, 12); line(24, 4, 0, 1, 14, 11); }
    }
    markers(10, 19, 18);
    if (v_.front != 0 || !v_.frontVis) { present1(); return true; }   // RUNNER:4 also ends RUNNER:3
    return false;
}

// ---- depth 1: RUNNER:27 and 28..32 --------------------------------------------------------------
bool MazeScreen::depth1() {
    v_.saveHi = v_.hi;
    v_.saveLo = v_.lo;
    if (v_.lWall) {
        if (v_.left != 0) {
            line(9, 2, 0, 1, 16, 12); line(10, 2, 1, 1, 4, 14); line(13, 6, 0, 1, 8, 12); line(10, 17, 1, -1, 4, 13);
            if (v_.left == 2) { line(10, 4, 0, 1, 13, 12); line(11, 4, 1, 1, 2, 14); line(12, 6, 0, 1, 9, 12); }
            v_.lo = 14;
        }
    } else if (!v_.leftVis) {
        fillLeft(10, 2, 4, 16, 14);
    } else if (v_.left == 0) {
        if (!v_.leftFwdVis) { fillBox(6, 6, 8, 8); v_.lo = 14; }
    } else {
        line(6, 5, 1, 0, 8, 9); line(6, 14, 1, 0, 8, 10); line(5, 6, 0, 1, 8, 12); line(13, 6, 0, 1, 8, 12);
        if (v_.left == 2) { line(7, 6, 1, 0, 6, 9); line(6, 7, 0, 1, 7, 12); line(12, 7, 0, 1, 7, 12); }
        v_.lo = 14;
    }
    if (v_.rWall) {
        if (v_.right != 0) {
            line(26, 2, 0, 1, 16, 11); line(25, 2, -1, 1, 4, 13); line(22, 6, 0, 1, 8, 11); line(25, 17, -1, -1, 4, 14);
            if (v_.right == 2) { line(25, 4, 0, 1, 13, 11); line(24, 4, -1, 1, 2, 13); line(23, 6, 0, 1, 9, 11); }
            v_.hi = 21;
        }
    } else if (!v_.rightVis) {
        fillRight(22, 5, 4, 10, 21);
    } else if (v_.right == 0) {
        if (!v_.rightFwdVis) { fillBox(22, 6, 8, 8); v_.hi = 21; }
    } else {
        line(22, 5, 1, 0, 8, 9); line(22, 14, 1, 0, 8, 10); line(22, 6, 0, 1, 8, 11); line(30, 6, 0, 1, 8, 11);
        if (v_.right == 2) { line(23, 6, 1, 0, 6, 9); line(23, 7, 0, 1, 7, 11); line(29, 7, 0, 1, 7, 11); }
        v_.hi = 21;
    }
    if (v_.front == 0) {
        if (!v_.frontVis) fillBox(14, 6, 8, 8);
    } else {
        line(14, 5, 1, 0, 8, 9); line(13, 6, 0, 1, 8, 12); line(14, 14, 1, 0, 8, 10); line(22, 6, 0, 1, 8, 11);
        if (v_.front == 2) { line(15, 6, 1, 0, 6, 9); line(14, 7, 0, 1, 7, 12); line(21, 7, 0, 1, 7, 11); }
    }
    markers(14, 15, 10);
    if (v_.front != 0 || !v_.frontVis) { present1(); return true; }   // RUNNER:4 also ends RUNNER:3
    return false;
}

// ---- depth 2: RUNNER:33 and 34..38 ----------------------------------------------------------------
bool MazeScreen::depth2() {
    if (v_.lWall) {
        if (v_.left != 0) {
            line(13, 6, 0, 1, 8, 12); line(14, 6, 1, 1, 2, 14); line(15, 8, 0, 1, 4, 12); line(15, 12, -1, 1, 2, 13);
            if (v_.left == 2) {
                dot(14, 7, 26); dot(15, 7, 27); line(14, 8, 0, 1, 5, 15); line(15, 8, 0, 1, 4, 28);
                dot(14, 13, 25); dot(15, 12, 25);
            }
            v_.lo = 16;
        }
    } else if (!v_.leftVis) {
        fillLeft(14, 6, 2, 8, 16);
    } else if (v_.left == 0) {
        if (!v_.leftFwdVis) { fillBox(12, 8, 4, 4); v_.lo = 16; }
    } else {
        line(12, 7, 1, 0, 4, 9); line(12, 12, 1, 0, 4, 10); line(11, 8, 0, 1, 4, 12); line(15, 8, 0, 1, 4, 12);
        if (v_.left == 2) { line(13, 8, 1, 0, 2, 9); line(12, 9, 0, 1, 3, 12); line(14, 9, 0, 1, 3, 12); }
        v_.lo = 16;
    }
    if (v_.rWall) {
        if (v_.right != 0) {
            line(22, 6, 0, 1, 8, 11); line(21, 6, -1, 1, 2, 13); line(20, 8, 0, 1, 4, 11); line(21, 13, -1, -1, 2, 14);
            if (v_.right == 2) {
                dot(21, 7, 30); dot(20, 7, 31); line(20, 8, 0, 1, 4, 0); line(21, 8, 0, 1, 5, 19);
                dot(20, 12, 29); dot(21, 13, 29);
            }
            v_.hi = 19;
        }
    } else if (!v_.rightVis) {
        fillRight(20, 7, 2, 6, 19);
    } else if (v_.right == 0) {
        if (!v_.rightFwdVis) { fillBox(20, 8, 4, 4); v_.hi = 19; }
    } else {
        line(20, 7, 1, 0, 4, 9); line(20, 12, 1, 0, 4, 10); line(20, 8, 0, 1, 4, 11); line(24, 8, 0, 1, 4, 11);
        if (v_.right == 2) { line(21, 8, 1, 0, 2, 9); line(21, 9, 0, 1, 3, 11); line(23, 9, 0, 1, 3, 11); }
        v_.hi = 19;
    }
    if (v_.front == 0) {
        if (!v_.frontVis) fillBox(16, 8, 4, 4);
    } else {
        line(16, 7, 1, 0, 4, 9); line(15, 8, 0, 1, 4, 12); line(16, 12, 1, 0, 4, 10); line(20, 8, 0, 1, 4, 11);
        if (v_.front == 2) { line(17, 8, 1, 0, 2, 9); line(16, 9, 0, 1, 3, 12); line(19, 9, 0, 1, 3, 11); }
    }
    if (v_.front != 0 || !v_.frontVis) { present1(); return true; }   // RUNNER:4 also ends RUNNER:3
    return false;
}

// ---- depth 3: RUNNER:39 and 40..44 ----------------------------------------------------------------
bool MazeScreen::depth3() {
    if (v_.lWall) {
        if (v_.left != 0) {
            dot(16, 8, 14); dot(16, 11, 13); line(15, 8, 0, 1, 4, 12); line(16, 9, 0, 1, 2, 12);
            if (v_.left == 2) { dot(16, 8, 24); dot(16, 9, 17); dot(16, 10, 17); dot(16, 11, 23); }
            v_.lo = 17;
        }
    } else if (!v_.leftVis) {
        fillLeft(16, 8, 1, 4, 17);
    } else if (v_.left == 0) {
        if (!v_.leftFwdVis) { fillBox(15, 9, 2, 2); v_.lo = 17; }
    } else {
        line(15, 8, 1, 0, 2, 9); line(15, 11, 1, 0, 2, 10); line(14, 9, 0, 1, 2, 12); line(16, 9, 0, 1, 2, 12);
        if (v_.left == 2) { dot(15, 9, 16); dot(16, 9, 94); dot(15, 10, 15); dot(16, 10, 28); }
        v_.lo = 17;
    }
    if (v_.rWall) {
        if (v_.right != 0) {
            dot(19, 8, 13); dot(19, 11, 14); line(19, 9, 0, 1, 2, 11); line(20, 8, 0, 1, 4, 11);
            if (v_.right == 2) { dot(19, 8, 22); dot(19, 9, 21); dot(19, 10, 21); dot(19, 11, 20); }
            v_.hi = 18;
        }
    } else if (!v_.rightVis) {
        fillRight(19, 8, 1, 4, 18);
    } else if (v_.right == 0) {
        if (!v_.rightFwdVis) { fillBox(19, 9, 2, 2); v_.hi = 18; }
    } else {
        line(19, 8, 1, 0, 2, 9); line(19, 11, 1, 0, 2, 10); line(19, 9, 0, 1, 2, 11); line(21, 9, 0, 1, 2, 11);
        if (v_.right == 2) { dot(19, 9, 92); dot(19, 10, 93); dot(20, 9, 18); dot(20, 10, 19); }
        v_.hi = 18;
    }
    if (v_.front == 0) {
        if (!v_.frontVis) fillBox(17, 9, 2, 2);
    } else {
        line(17, 8, 1, 0, 2, 9); line(17, 11, 1, 0, 2, 10); line(16, 9, 0, 1, 2, 12); line(19, 9, 0, 1, 2, 11);
        if (v_.front == 2) { dot(17, 9, 16); dot(18, 9, 18); dot(17, 10, 15); dot(18, 10, 19); }
    }
    present1();
    return true;
}

// RUNNER:3
void MazeScreen::drawView() {
    w1_ = d_.window1;
    v_ = View{};
    v_.row = g_.x;                       // L6 := G8
    v_.col = g_.y;                       // L5 := G7
    if (g_.identify > 0) --g_.identify;
    if (g_.light > 0) --g_.light;
    v_.lamp = g_.light > 0;
    std::fill(w1_->cell.begin(), w1_->cell.begin() + 36 * 20, u16(0x2020));
    v_.lo = 0; v_.hi = 35; v_.saveLo = 0; v_.saveHi = 35;
    if (!visible(0, 0)) {
        fillBox(0, 0, 36, 20);
        present1();
        return;                                       // RUNNER:4 = present + EXIT(RUNNER:3)
    }
    step(v_.row, v_.col, 0, -1);
    stepForward(); if (depth0()) return;          // RUNNER:4 inside a depth ends the whole view
    stepForward(); if (depth1()) return;
    if (v_.lamp) {
        stepForward(); if (depth2()) return;
        stepForward(); if (depth3()) return;
    }
    present1();
}

// ============================================================================================
//  the HUD
// ============================================================================================

// RUNNER:50 -- the tail of a party row: the HP trend mark and the status / maximum HP
void MazeScreen::rowStatus(int i) {
    const Character &c = w_.party.member(i);
    Win *g10 = w_.g10;
    int trend = c.healPts - c.poison;
    d_.putC(g10, trend == 0 ? ' ' : (trend < 0 ? '-' : '+'));
    if (c.status == Status::OK) {
        okSeen_ = true;
        if (c.poison == 0) d_.putNum(g10, c.hpMax, 4);
        else d_.putStr(g10, 700);
    } else {
        d_.putS(g10, d_.str(10040 + int(c.status)).substr(0, 6));
    }
}

// RUNNER:49 -- redraw the party rows; if nobody is left standing the expedition is over
void MazeScreen::partyRows() {
    Console &con = d_.con;
    Win *g10 = w_.g10;
    okSeen_ = false;
    con.hide(g10, false);
    std::fill(g10->cell.begin() + 38, g10->cell.begin() + 38 + 228, u16(0x2020));
    for (int i = 0; i < w_.party.count(); ++i) {
        const Character &c = w_.party.member(i);
        d_.gotoXY(g10, 0, i + 1);
        d_.putNum(g10, i + 1, 1);
        g10->cx = 2;
        d_.putS(g10, c.name);
        g10->cx = 18;
        d_.putS(g10, d_.str(10009 + int(c.align)).substr(0, 1));
        d_.putC(g10, '-');
        d_.putS(g10, d_.str(10030 + int(c.cls)).substr(0, 3));
        d_.putC(g10, ' ');
        int ac = c.armorClass - g_.protect;
        if (ac > -10) d_.putNum(g10, ac, 2);
        else d_.putStr(g10, 701);
        d_.putNum(g10, c.hpLeft, 5);
        rowStatus(i);
    }
    con.show(g10, true);
    if (!okSeen_) {                                  // everyone is down: back to the castle
        closeHud(false);
        finish(16);
    }
}

// RUNNER:48
void MazeScreen::closeHud(bool showRoster) {
    Console &con = d_.con;
    con.close(bar_, false);
    con.close(effects_, false);
    if (showRoster) con.show(w_.g10, false);
    con.refresh();
}

void MazeScreen::finish(int xgoto) {
    result_.xgoto = xgoto;
    leave_ = true;
}

// RUNNER:72
void MazeScreen::openWindows() {
    Console &con = d_.con;
    bar_ = con.open(0, 0, 40, 3, 10, false);
    effects_ = con.open(14, 2, 12, 5, 11, false);
    con.hide(effects_, false);
    if (g_.g23) {
        con.hide(bar_, true);
    } else {
        con.show(bar_, false);
        con.show(w_.g10, true);
    }
    d_.putStr(bar_, 717);
    if (g_.g22) con.show(w_.g10, false);
    if (!w_.g10->hidden()) partyRows();
    drawView();
    needRefresh_ = g_.g23 != 0;
    redraw_ = false;
    moved_ = true;
}

// RUNNER:73
void MazeScreen::effectLine(int turns, int strNo) {
    d_.putStrC(effects_, turns > 0 ? strNo : 718);
}

// ============================================================================================
//  messages, movement and the squares
// ============================================================================================

// RUNNER:45 -- a one-line message box in the middle of the view
void MazeScreen::showMsgText(const std::string &s) {
    Console &con = d_.con;
    int half = (Dos::visLen(s) + 1) / 2;
    Win *b = con.open(19 - half, 10, 2 + half + half, 3, 15, true);
    d_.putS(b, s);
    con.close(b, false);
    needRefresh_ = true;
}
void MazeScreen::showMsg(int strNo) { showMsgText(d_.str(strNo)); }

// RUNNER:2 -- GETKEY and the command index (1..22), -1 for any other key
void MazeScreen::readKey() {
    d_.getKey();
    std::string keys = d_.str(722);
    lastKey_ = -1;
    for (int i = 0; i < int(keys.size()); ++i)
        if (int(u8(keys[i])) == d_.inchar) { lastKey_ = i + 1; return; }
}

// RUNNER:65 -- one step ahead
void MazeScreen::stepAhead() {
    canWander_ = true;
    redraw_ = true;
    moved_ = true;
    g_.g40 = g_.x; g_.g39 = g_.y; g_.g38 = g_.level;
    switch (g_.dir & 3) {
        case 0: ++g_.y; break;
        case 1: ++g_.x; break;
        case 2: --g_.y; break;
        default: --g_.x; break;
    }
    g_.y = (g_.y + 20) % 20;
    g_.x = (g_.x + 20) % 20;
    d_.sound(1);
}

// RUNNER:67 -- the direction glyph flashed in the middle of the view
void MazeScreen::flashGlyph(int ch) {
    Win *w1 = d_.window1;
    d_.gotoXY(w1, 17, 9); d_.putC(w1, ch); d_.putC(w1, ch);
    d_.gotoXY(w1, 17, 10); d_.putC(w1, ch); d_.putC(w1, ch);
}

// RUNNER:68 -- walk forward (or bump into a wall)
void MazeScreen::move(int glyph) {
    int wall = int(level_.wall(g_.x, g_.y, g_.dir & 3));
    if (wall != 1) stepAhead();
    if (!moved_) {                                   // RUNNER:66
        showMsg(715);
        d_.sound(2);
    } else {
        flashGlyph(glyph);
    }
}

// RUNNER:69
void MazeScreen::turn(int delta, int glyph) {
    redraw_ = true;
    g_.dir = (g_.dir + delta) % 4;
    flashGlyph(glyph);
}

// RUNNER:54 -- go to another level: the mainline reloads everything
void MazeScreen::changeLevel(int lvl) {
    g_.level = lvl;
    closeHud(false);
    finish(9);
}

// RUNNER:57 -- stairs / chute / teleporter: jump to the square's destination
void MazeScreen::teleport() {
    g_.x = level_.aux2(idx_);
    g_.y = level_.aux1(idx_);
    if (g_.level != level_.aux0(idx_)) { changeLevel(level_.aux0(idx_)); return; }
    d_.con.refresh();
}

// RUNNER:58 -- the stairs prompt
void MazeScreen::stairs() {
    Console &con = d_.con;
    Win *b = con.open(9, 10, 22, 4, 15, true);
    d_.putStrC(b, g_.level < level_.aux0(idx_) ? 703 : 704);
    d_.putStrC(b, 705);
    std::string yes = d_.str(706), no = d_.str(707);
    do d_.getKey(); while (!d_.quit && d_.inchar != u8(yes[0]) && d_.inchar != u8(no[0]));
    con.close(b, true);
    if (d_.quit) return;
    if (d_.inchar == u8(yes[0])) teleport();
}

// RUNNER:56 -- spinner
void MazeScreen::spinner() {
    g_.dir = rnd(4);
    drawView();
}

// RUNNER:59 -- an anti-light square
void MazeScreen::darkness() {
    if (level_.aux1(idx_) == 0) g_.light = 0;
}

// RUNNER:60 -- walked into solid rock
void MazeScreen::rock() {
    for (int i = 0; i < w_.party.count(); ++i) w_.party.member(i).status = Status(7);
    showMsg(709);
    d_.pause(20);
    d_.pause(20);
    partyRows();
    if (leave_) return;
    g_.level = 0;
    closeHud(false);
    finish(16);
}

// RUNNER:61 -- a trap / pit / ouch square: each healthy member may take damage
void MazeScreen::damageSquare() {
    for (int i = 0; i < w_.party.count(); ++i) {
        Character &c = w_.party.member(i);
        if (int(c.status) < 5 && rnd(25) + g_.level > c.attrib[AGI]) {
            int dmg = level_.aux0(idx_);
            for (int k = 1; k <= level_.aux2(idx_); ++k) dmg += rnd(level_.aux1(idx_)) + 1;
            c.hpLeft -= dmg;
            if (c.hpLeft < 0) { c.hpLeft = 0; c.status = Status(5); }
        }
    }
    partyRows();
}

// RUNNER:62
void MazeScreen::scnmsg() {
    g_.g9 = idx_;
    g_.g41 = 7;
    closeHud(false);
    finish(23);
}

// RUNNER:63 -- a fixed encounter
void MazeScreen::encSquare() {
    if (level_.aux0(idx_) == 0) return;
    if (!g_.fight[g_.x][g_.y]) return;
    g_.g37 = level_.aux2(idx_);
    if (level_.aux1(idx_) > 1) g_.g37 += rnd(level_.aux1(idx_));
    if (level_.aux0(idx_) > 0) {
        // RUNNER writes the decremented count back into the level record (it is the running
        // total of such encounters left); keep the change in the working copy.
        std::vector<u8> &rec = mazeRecord(g_, w_.sc, g_.level);
        int w = 384 + (idx_ & 15);
        int v = int(i16(rd16(rec.data() + w * 2))) - 1;
        rec[w * 2] = u8(v & 0xFF); rec[w * 2 + 1] = u8((v >> 8) & 0xFF);
        if (v == 0) {
            int wd = 380 + (idx_ & 15) / 4, sh = ((idx_ & 15) % 4) * 4;
            u16 old = rd16(rec.data() + wd * 2);
            old = u16(old & ~(0xF << sh));
            rec[wd * 2] = u8(old & 0xFF); rec[wd * 2 + 1] = u8(old >> 8);
        }
    }
    // RUNNER:63 sets the square's type to 3 while it clears the room (so the room's flood
    // fill is allowed to start on it), then back to 12
    auto setType = [&](int t) {
        std::vector<u8> &rec = mazeRecord(g_, w_.sc, g_.level);
        int wd = 380 + (idx_ & 15) / 4, sh = ((idx_ & 15) % 4) * 4;
        u16 old = rd16(rec.data() + wd * 2);
        old = u16((old & ~(0xF << sh)) | (t << sh));
        rec[wd * 2] = u8(old & 0xFF); rec[wd * 2 + 1] = u8(old >> 8);
    };
    setType(3);
    clearRoom();
    setType(12);
    showMsg(702);
    d_.pause(20);
    g_.g29 = 2;
    closeHud(false);
    finish(8);
}

// RUNNER:64 -- an elevator
void MazeScreen::buttons() {
    Console &con = d_.con;
    int lo = level_.aux2(idx_), hi = level_.aux1(idx_);
    Win *b = con.open(0, 10, 40, 6, 15, true);
    d_.putStrC(b, 710);
    std::string s = d_.str(711);
    size_t p = s.find('^');
    if (p != std::string::npos) s[p] = char(65 + hi - lo);
    d_.putCentered(b, s);
    d_.newline(b);
    d_.putStrC(b, 712);
    con.close(b, false);
    for (;;) {
        d_.getKey();
        if (d_.quit) return;
        if (d_.inchar == 13 || (d_.inchar >= 65 && d_.inchar <= 65 + hi - lo)) break;
    }
    if (d_.inchar == 13) { con.refresh(); return; }
    if (level_.aux0(idx_) > 0) {
        g_.x = rnd(20);
        g_.y = rnd(20);
    }
    changeLevel(lo + d_.inchar - 65);
}

// RUNNER:55 -- whatever is on the square the party stands on
void MazeScreen::square() {
    for (;;) {
        int sx = g_.x, sy = g_.y;
        idx_ = level_.squareExtra(g_.x, g_.y);
        switch (int(level_.squareType(idx_))) {
            case ST_STAIRS: if (moved_) stairs(); break;
            case ST_PIT:
                if (moved_) { showMsg(723); d_.pause(20); damageSquare(); }
                break;
            case ST_CHUTE: showMsg(714); d_.pause(20); teleport(); break;
            case ST_SPINNER: if (moved_) spinner(); break;
            case ST_DARK: darkness(); break;
            case ST_TELEPORT: teleport(); break;
            case ST_OUCH: showMsg(713); d_.pause(20); damageSquare(); break;
            case ST_BUTTONS: buttons(); break;
            case ST_ROCK: rock(); break;
            case ST_FIZZLE: g_.g30 = 1; break;
            case ST_SCNMSG: scnmsg(); break;
            case ST_ENCOUNTER: encSquare(); break;
            default: break;
        }
        if (leave_ || d_.quit) return;
        if (g_.x != sx || g_.y != sy) drawView();
        if (sx == g_.x && sy == g_.y) return;
    }
}

// RUNNER:46/47 -- the room the party stands in is no longer an unfought one
void MazeScreen::clearRoom() {
    bool mark[20][20] = {};
    bool again = true;
    auto take = [&](int x, int y, int wall) {
        if (wall != 0) return;
        x = (x + 20) % 20; y = (y + 20) % 20;
        if (mark[x][y]) return;
        if (int(level_.squareAt(x, y)) == ST_ENCOUNTER) return;
        mark[x][y] = true;
        again = false;
    };
    take(g_.x, g_.y, 0);
    again = true;
    // RUNNER:46 repeats the sweep until a pass marks nothing new
    for (;;) {
        again = true;
        for (int x = 0; x < 20; ++x)
            for (int y = 0; y < 20; ++y)
                if (mark[x][y] && g_.fight[x][y]) {
                    take(x + 1, y, int(level_.wall(x, y, EAST)));
                    take(x - 1, y, int(level_.wall(x, y, WEST)));
                    take(x, y - 1, int(level_.wall(x, y, SOUTH)));
                    take(x, y + 1, int(level_.wall(x, y, NORTH)));
                    g_.fight[x][y] = false;
                }
        if (again) break;
    }
}

// RUNNER:51 -- poison, regeneration and the dead, once per step
void MazeScreen::stepEffects() {
    for (int i = 0; i < w_.party.count(); ++i) {
        if (rnd(8) != 1) continue;
        Character &c = w_.party.member(i);
        if (c.poison > 0) {
            if (c.hpLeft > 50) c.hpLeft -= c.hpLeft / 10;
            else c.hpLeft -= c.poison;
        }
        c.hpLeft += c.healPts;
        if (c.hpLeft <= 0) {
            c.poison = 0;
            if (int(c.status) < 5) {
                c.hpLeft = 0;
                c.status = Status(5);
                partyRows();
                if (leave_) return;
                showMsgText(Dos::substitute(d_.str(727), c.name));
                d_.pause(20);
                d_.con.refresh();
            }
        } else if (c.hpLeft > c.hpMax) {
            c.hpLeft = c.hpMax;
        }
    }
}

// RUNNER:52 -- AN ENCOUNTER: size the group and hand over to COMBAT
void MazeScreen::encounter() {
    showMsg(702);
    d_.pause(20);
    int grp = 1;
    while (rnd(4) == 2 && grp < 3) ++grp;
    EnemyCalc e = level_.enemyCalc(grp);
    int worse = 0;
    while (rnd(100) < e.percWorse && worse < e.worse01) ++worse;
    g_.g37 = e.minEnemy + rnd(e.range0n) + e.multWorse * worse;
    if (g_.g31 == 1) g_.g29 = 2;
    else if (level_.fights(g_.x, g_.y)) g_.g29 = g_.fight[g_.x][g_.y] ? 2 : 1;
    else g_.g29 = 0;
    closeHud(false);
    finish(8);
}

// RUNNER:70 -- the DELAY prompt
void MazeScreen::delayPrompt() {
    Console &con = d_.con;
    Win *b = con.open(0, 10, 40, 3, 15, true);
    d_.putStr(b, 716);
    std::string s = d_.getLine(b, 2);
    int n = 0;
    bool bad = false;
    for (char ch : s) {
        if (ch >= '0' && ch <= '9') n = n * 10 + (ch - '0');
        else { bad = true; break; }
    }
    if (!bad && n > 0 && n <= 99) g_.delay = n;
    con.close(b, true);
}

// RUNNER:74 -- Q)UIT: leave the expedition where the party stands
bool MazeScreen::quitPrompt() {
    Console &con = d_.con;
    Win *b = con.open(4, 10, 32, 3, 31, true);
    d_.putStrC(b, 724);
    std::string yes = d_.str(725), no = d_.str(726);
    do d_.getKey();
    while (!d_.quit && d_.inchar != 13 && d_.inchar != u8(no[0]) && d_.inchar != u8(yes[0]));
    if (d_.quit) return false;
    if (d_.inchar == 13 || d_.inchar == u8(no[0])) { con.close(b, true); return false; }
    con.close(bar_, false);
    con.close(effects_, false);
    con.close(b, true);
    for (int i = 0; i < w_.party.count(); ++i) {
        Character &c = w_.party.member(i);
        c.inMaze = false;
        c.lostY = 100 * g_.x + g_.y;
        c.lostLevel = g_.level;
    }
    w_.party.writeBack(w_.roster);
    w_.party.disbandQuiet();                         // PARTYCNT := 0: they stay where they stand
    save();
    g_.level = 0; g_.x = 0; g_.y = 0;
    finish(9);
    return true;
}

// ============================================================================================
//  the walk-in from the castle (UTILITIE:9, 10) and the main loop (RUNNER:1, 53)
// ============================================================================================

void MazeScreen::enter() {
    Console &con = d_.con;
    if (g_.level == 0) g_.level = 1;
    if (g_.level < 0) g_.level = -g_.level;
    for (int i = 0; i < w_.party.count(); ++i) {                       // UTILITIE:9
        Character &c = w_.party.member(i);
        c.lostY = 100 * g_.x + g_.y;
        c.lostLevel = g_.level;
    }
    w_.party.writeBack(w_.roster);
    save();
    loadLevel();
    // UTILITIE:10 -- seed the "unfought rooms" mask
    for (auto &row : g_.fight) for (bool &b : row) b = false;
    g_.g30 = 0;
    Win *box = con.open(14, 8, 12, 3, 15, true);
    d_.putStr(box, 210);
    con.close(box, false);
    bool mark[20][20] = {};
    auto flood = [&](int sx, int sy) {                               // UTILITIE:12 / 13
        std::vector<std::array<int, 2>> q;
        q.push_back({sx, sy});
        for (size_t h = 0; h < q.size(); ++h) {
            int x = q[h][0], y = q[h][1];
            mark[x][y] = true;
            auto push = [&](int nx, int ny, int wall) {
                if (wall != 0) return;
                nx = (nx + 20) % 20; ny = (ny + 20) % 20;
                if (!level_.fights(nx, ny) || mark[nx][ny]) return;
                q.push_back({nx, ny});
            };
            push(x + 1, y, int(level_.wall(x, y, EAST)));
            push(x - 1, y, int(level_.wall(x, y, WEST)));
            push(x, y - 1, int(level_.wall(x, y, SOUTH)));
            push(x, y + 1, int(level_.wall(x, y, NORTH)));
        }
    };
    bool found = true;
    for (int n = 1; n <= 16 && found; ++n) {
        int a = rnd(20), b = rnd(20);                                // UTILITIE:11: a = L2, b = L1
        found = false;
        for (int k = 1; k <= 400; ++k) {
            if (level_.fights(a, b) && !mark[a][b]) { found = true; break; }
            if (++a > 19) {
                a -= 19;
                if (++b > 19) b -= 19;
            }
        }
        if (found) flood(a, b);
    }
    for (int x = 0; x < 20; ++x)
        for (int y = 0; y < 20; ++y)
            if (int(level_.squareAt(x, y)) == ST_ENCOUNTER && !mark[x][y]) flood(x, y);
    for (int x = 0; x < 20; ++x)
        for (int y = 0; y < 20; ++y) g_.fight[x][y] = mark[x][y];
}

MazeResult MazeScreen::run() {
    result_ = MazeResult{};
    leave_ = false;
    loadLevel();
    if (g_.randomStart)
        while (int(level_.squareAt(g_.x, g_.y)) != 0) {
            g_.x = rnd(20);
            g_.y = rnd(20);
        }
    g_.randomStart = false;
    canWander_ = false;
    clearRoom();                                                     // RUNNER:46(G8, G7)
    lastKey_ = -1;
    partyRows();
    if (leave_) return result_;

    openWindows();
    while (!leave_ && !d_.quit) {
        // the LIGHT / PROTECT / IDENTIFY box
        if (!g_.g23 && (g_.light != 0 || g_.protect != 0 || g_.identify != 0)) {
            if (effects_->hidden()) d_.con.show(effects_, true);
            d_.gotoXY(effects_, 0, 0);
            effectLine(g_.light, 719);
            effectLine(g_.protect, 720);
            effectLine(g_.identify, 721);
        } else if (!effects_->hidden()) {
            d_.con.hide(effects_, true);
        }
        if (redraw_) drawView();
        int type = int(level_.squareAt(g_.x, g_.y));
        if (type != 0 && moved_) {
            if (g_.g41 != 23) square();
            else if (type == ST_STAIRS || type == ST_BUTTONS) square();
        }
        if (leave_ || d_.quit) break;
        if (g_.g23 && !w_.g10->hidden() && !g_.g22) d_.con.hide(w_.g10, false);
        g_.g41 = 7;
        redraw_ = false;
        // the encounter test: both RANDOM calls are always made (LAND / LOR evaluate everything)
        int r99 = rnd(99);
        bool t = (canWander_ && r99 == 35) || g_.g31 == 1 || g_.fight[g_.x][g_.y];
        bool b = canWander_ && moved_ && (lastKey_ == 1 || lastKey_ == 4) && level_.fights(g_.x, g_.y);
        int r8 = rnd(8);
        b = b && r8 == 3;
        if (t || b) {
            encounter();
            break;
        }
        if (moved_) stepEffects();
        if (leave_) break;
        moved_ = false;
        readKey();
        if (d_.quit) break;
        if (needRefresh_) { d_.con.refresh(); needRefresh_ = false; }
        switch (lastKey_) {
            case 1: case 2: case 3: case 4: case 23: move(123); break;
            case 5: case 6: case 7: case 8: turn(3, 122); break;
            case 9: case 10: case 11: case 12: turn(1, 124); break;
            case 13: case 14: case 15: case 16: turn(2, 125); break;
            case 17: {                                              // O)FF
                Console &con = d_.con;
                if (g_.g23) {
                    con.show(bar_, false);
                    if (std::abs(g_.light) + std::abs(g_.protect) + std::abs(g_.identify) > 0) con.show(effects_, false);
                    con.show(w_.g10, true);
                    partyRows();
                    g_.g23 = 0; g_.g22 = 1;
                } else {
                    con.hide(bar_, false); con.hide(effects_, false); con.hide(w_.g10, true);
                    g_.g23 = 1; g_.g22 = 0;
                }
                break;
            }
            case 18:                                                // S)TATUS
                if (w_.g10->hidden()) partyRows();
                else d_.con.hide(w_.g10, true);
                g_.g22 = w_.g10->hidden() ? 0 : 1;
                break;
            case 19: delayPrompt(); break;
            case 20: closeHud(true); finish(12); break;             // C)AMP
            case 21: closeHud(false); finish(26); break;            // I)NSPECT
            case 22: quitPrompt(); break;
            default: break;
        }
    }
    result_.windowClosed = d_.quit;
    return result_;
}

} // namespace wiz
