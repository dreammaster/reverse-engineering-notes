// Maze navigation -- the movement half of RUNNER (P010E01).
//
// Position is (x, y, dir): x east, y north, both wrapping mod 20; dir is
// NORTH/EAST/SOUTH/WEST.  Ports SHFTPOS / MOVEFRWD / FORWRD / KICK / DOTURN
// and the FRWD/LEFT/RIGH view helpers the 3D renderer needs.
#pragma once
#include "wiz/maze.h"

namespace wiz {

struct MazePos {
    int x = 0, y = 0, dir = NORTH;
};

// SHFTPOS: shift (x,y) `right` squares to the party's right and `fwd` squares
// ahead, given a facing of `dir`; wraps mod 20.
inline void shftPos(int &x, int &y, int dir, int right, int fwd) {
    switch (dir & 3) {
        case NORTH: x += right; y += fwd;   break;
        case EAST:  x += fwd;   y -= right; break;
        case SOUTH: x -= right; y -= fwd;   break;
        case WEST:  x -= fwd;   y += right; break;
    }
    x = wrap20(x); y = wrap20(y);
}

// MOVEFRWD: advance one cell along `dir`.
inline void stepForward(MazePos &p) {
    int x = p.x, y = p.y;
    shftPos(x, y, p.dir, 0, 1);
    p.x = x; p.y = y;
}

// DOTURN: lr = 3 turns left, lr = 1 turns right.
inline void turn(MazePos &p, int lr) { p.dir = (p.dir + lr) & 3; }

// FORWRD: a step is allowed only through an OPEN edge.
inline bool canWalk(const MazeLevel &m, const MazePos &p) {
    return m.wall(p.x, p.y, p.dir) == Wall::Open;
}
// KICK: a step is allowed through anything that is not a solid WALL
// (doors, hidden doors).
inline bool canKick(const MazeLevel &m, const MazePos &p) {
    return m.wall(p.x, p.y, p.dir) != Wall::Wall;
}

// FRWDVIEW / LEFTVIEW / RIGHVIEW: the wall the renderer sees `deltaR` squares
// to the party's right of the drawing cursor (x,y), looking forward / left /
// right.
inline Wall viewWall(const MazeLevel &m, int x, int y, int dir, int deltaR, int rel) {
    shftPos(x, y, dir, deltaR, 0);
    return m.wall(x, y, (dir + rel) & 3);
}
inline Wall frwdView(const MazeLevel &m, int x, int y, int dir, int dr) { return viewWall(m, x, y, dir, dr, 0); }
inline Wall leftView(const MazeLevel &m, int x, int y, int dir, int dr) { return viewWall(m, x, y, dir, dr, 3); }
inline Wall righView(const MazeLevel &m, int x, int y, int dir, int dr) { return viewWall(m, x, y, dir, dr, 1); }

inline const char *dirName(int d) {
    static const char *n[4] = {"NORTH", "EAST", "SOUTH", "WEST"};
    return n[d & 3];
}

} // namespace wiz
