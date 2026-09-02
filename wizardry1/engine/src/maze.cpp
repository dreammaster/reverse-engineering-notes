#include "wiz/maze.h"
#include "wiz/rng.h"

namespace wiz {

bool MazeLevel::load(Bytes rec) {
    if (rec.size() < kBytes) { p_ = nullptr; return false; }
    p_ = rec.p;
    return true;
}

int MazeLevel::packed_(int baseWord, int rowWords, int perWord, int bits,
                       int x, int y) const {
    x = wrap20(x); y = wrap20(y);
    int word = baseWord + x * rowWords + y / perWord;
    int shift = (y % perWord) * bits;
    return (w_(word) >> shift) & ((1 << bits) - 1);
}

Wall MazeLevel::wall(int x, int y, int dir) const {
    static const int base[4] = {180, 120, 60, 0};   // N, E, S, W
    return Wall(packed_(base[dir & 3], 3, 8, 2, x, y));
}

int MazeLevel::squareExtra(int x, int y) const {
    return packed_(280, 5, 4, 4, x, y);
}

Square MazeLevel::squareType(int idx) const {
    idx &= 15;
    int v = (w_(380 + idx / 4) >> ((idx % 4) * 4)) & 0xF;
    return v <= int(Square::Encounter) ? Square(v) : Square::Normal;
}

bool MazeLevel::fights(int x, int y) const {
    return packed_(240, 2, 16, 1, x, y) != 0;
}

int MazeLevel::aux_(int which, int idx) const {
    static const int base[3] = {384, 400, 416};
    return i16(w_(base[which] + (idx & 15)));
}

// ---- FightMap ----------------------------------------------------------

// FILLROOM: flood-fill a connected run of the level's FIGHTS cells.
void FightMap::fillRoom(const MazeLevel &m, int x, int y) {
    x = wrap20(x); y = wrap20(y);
    if (!m.fights(x, y) || cell[x][y]) return;
    cell[x][y] = true;
    if (m.wall(x, y, NORTH) == Wall::Open) fillRoom(m, x, y + 1);
    if (m.wall(x, y, EAST)  == Wall::Open) fillRoom(m, x + 1, y);
    if (m.wall(x, y, SOUTH) == Wall::Open) fillRoom(m, x, y - 1);
    if (m.wall(x, y, WEST)  == Wall::Open) fillRoom(m, x - 1, y);
}

// CLROOMFG: flood-clear the flags for a room the party has just cleared.
void FightMap::clearRoom(const MazeLevel &m, int x, int y) {
    x = wrap20(x); y = wrap20(y);
    if (!cell[x][y]) return;
    cell[x][y] = false;
    if (m.wall(x, y, NORTH) == Wall::Open) clearRoom(m, x, y + 1);
    if (m.wall(x, y, EAST)  == Wall::Open) clearRoom(m, x + 1, y);
    if (m.wall(x, y, SOUTH) == Wall::Open) clearRoom(m, x, y - 1);
    if (m.wall(x, y, WEST)  == Wall::Open) clearRoom(m, x - 1, y);
}

// FIGHTS: 9 random fight-cell seeds + every ENCOUNTE square, flood-filled.
void FightMap::build(const MazeLevel &m, Rng &rng) {
    for (auto &row : cell) for (bool &b : row) b = false;

    for (int i = 0; i < 9; ++i) {
        int x0 = rng.mod(20), y0 = rng.mod(20);
        int fx = x0, fy = y0;
        do {                                        // FINDSPOT: scan for a
            if (m.fights(fx, fy) && !cell[fx][fy]) break;   // free fight cell
            if (++fx > 19) { fx = 0; if (++fy > 19) fy = 0; }
        } while (fx != x0 || fy != y0);
        if (m.fights(fx, fy) && !cell[fx][fy]) fillRoom(m, fx, fy);
    }
    for (int x = 0; x < 20; ++x)
        for (int y = 0; y < 20; ++y)
            if (m.squareAt(x, y) == Square::Encounter) fillRoom(m, x, y);
}

EnemyCalc MazeLevel::enemyCalc(int group) const {
    EnemyCalc e;
    if (group < 1 || group > 3) return e;
    int b = 432 + (group - 1) * 5;
    e.minEnemy  = i16(w_(b + 0));
    e.multWorse = i16(w_(b + 1));
    e.worse01   = i16(w_(b + 2));
    e.range0n   = i16(w_(b + 3));
    e.percWorse = i16(w_(b + 4));
    if (e.range0n < 1) e.range0n = 1;
    return e;
}

} // namespace wiz
