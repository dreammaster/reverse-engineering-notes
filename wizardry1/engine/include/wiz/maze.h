// TMAZE -- one maze level (894 bytes / 447 words).
//
// Layout confirmed from DOS RUNNER p-code (procs 13/19) and the UCSD packed-
// array rules; see docs/file-formats.md.  All grids are [x][y], x = MAZEX
// (east), y = MAZEY (north); both wrap mod 20.
//
//   word  field
//      0  W[x][y]   2 bits, 3 words/row   (TWALL: Open/Wall/Door/HiddenDoor)
//     60  S[x][y]   "
//    120  E[x][y]   "
//    180  N[x][y]   "
//    240  FIGHTS[x][y]   1 bit, 2 words/row
//    280  SQREXTRA[x][y] 4 bits, 5 words/row  -> index 0..15
//    380  SQRETYPE[0..15] 4 bits              -> TSQUARE
//    384  AUX0[0..15]  i16
//    400  AUX1[0..15]
//    416  AUX2[0..15]
//    432  ENMYCALC[1..3] x {MINENEMY,MULTWORS,WORSE01,RANGE0N,PERCWORS}
#pragma once
#include "wiz/types.h"

namespace wiz {

enum Dir : int { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 };

inline int wrap20(int v) { return ((v % 20) + 20) % 20; }

struct EnemyCalc {
    int minEnemy = 0, multWorse = 0, worse01 = 0, range0n = 1, percWorse = 0;
};

class MazeLevel {
public:
    static constexpr int kBytes = 894;

    // `rec` must be >= 894 bytes (a ZMAZE scenario record).
    bool load(Bytes rec);

    // Wall on edge `dir` of cell (x,y).
    Wall wall(int x, int y, int dir) const;

    // The special-square descriptor index (0..15) for a cell, and the type it
    // maps to.
    int squareExtra(int x, int y) const;
    Square squareType(int idx) const;
    Square squareAt(int x, int y) const { return squareType(squareExtra(x, y)); }

    bool fights(int x, int y) const;

    int aux0(int idx) const { return aux_(0, idx); }
    int aux1(int idx) const { return aux_(1, idx); }
    int aux2(int idx) const { return aux_(2, idx); }

    EnemyCalc enemyCalc(int group) const;   // group 1..3

private:
    const u8 *p_ = nullptr;
    u16 w_(int word) const { return rd16(p_ + word * 2); }
    int packed_(int baseWord, int rowWords, int perWord, int bits, int x, int y) const;
    int aux_(int which, int idx) const;
};

} // namespace wiz
