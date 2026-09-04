#include "wiz/maze3d.h"

#include <algorithm>

namespace wiz {
namespace {

// The DOS wireframe is chunky character-cell line-art (CHARSET glyphs) in a
// 36x20 grid, not thin pixel lines.  We keep the verified Apple DRAWMAZE
// geometry (receding trapezoids, halving each depth) and render its edges as
// glyph runs: vertical -> 15, horizontal -> 7, the two diagonals -> 13/14,
// door cut -> 17/19.
enum : u8 { GV = 15, GH = 7, GDU = 13, GDD = 14, GDOOR = 17 };

struct Cells {
    u8 (*g)[kMazeCols];
    int clipLo = 0, clipHi = kMazeCols - 1;
    void put(int c, int r, u8 gl) {
        if (r >= 0 && r < kMazeRows && c >= clipLo && c <= clipHi &&
            c >= 0 && c < kMazeCols && g[r][c] == 0)
            g[r][c] = gl;
    }
    // Bresenham from (c0,r0) to (c1,r1) inclusive.
    void line(int c0, int r0, int c1, int r1, u8 gl) {
        int dc = std::abs(c1 - c0), dr = -std::abs(r1 - r0);
        int sc = c0 < c1 ? 1 : -1, sr = r0 < r1 ? 1 : -1, err = dc + dr;
        for (;;) {
            put(c0, r0, gl);
            if (c0 == c1 && r0 == r1) break;
            int e2 = 2 * err;
            if (e2 >= dr) { err += dr; c0 += sc; }
            if (e2 <= dc) { err += dc; r0 += sr; }
        }
    }
    void vline(int c, int r0, int r1, u8 gl) { line(c, r0, c, r1, gl); }
    void hline(int c0, int c1, int r, u8 gl) { line(c0, r, c1, r, gl); }
};

// Wall geometry for one depth, in cell units.  `ulX`/`lrX` = the near-edge
// columns (left / right walls sit here); `topY`/`botY` = the near-edge rows;
// the far edge is the next depth's `ulX`/`lrX` inset toward the centre.
struct Geom { int ulX, lrX, topY, botY, ww; };

bool wallOpen(Wall w) { return w == Wall::Open; }
bool wallDoor(Wall w, bool lit, Rng &rng) {
    if (w == Wall::Door) return true;
    if (w == Wall::HiddenDoor) return lit || rng.mod(6) == 3;
    return false;
}

// LEFT wall: near vertical edge at g.ulX (full height) receding to the far
// edge n.ulX; top + bottom diagonals close the trapezoid.  DOS DRAWLEFT.
void drawLeft(Cells &p, const Geom &g, const Geom &n, Wall w, bool lit, Rng &rng) {
    p.vline(g.ulX, g.topY, g.botY, GV);
    p.vline(n.ulX, n.topY, n.botY, GV);
    p.line(g.ulX, g.topY, n.ulX, n.topY, GDD);
    p.line(g.ulX, g.botY, n.ulX, n.botY, GDU);
    if (!wallDoor(w, lit, rng)) return;
    int a = g.ulX + (n.ulX - g.ulX) / 3, b = g.ulX + 2 * (n.ulX - g.ulX) / 3;
    int t = (g.topY + n.topY) / 2, u = (g.botY + n.botY) / 2;
    p.vline(a, t, g.botY, GDOOR);
    p.vline(b, (t + n.topY) / 2, u, GDOOR);
}

void drawRigh(Cells &p, const Geom &g, const Geom &n, Wall w, bool lit, Rng &rng) {
    p.vline(g.lrX, g.topY, g.botY, GV);
    p.vline(n.lrX, n.topY, n.botY, GV);
    p.line(g.lrX, g.topY, n.lrX, n.topY, GDU);
    p.line(g.lrX, g.botY, n.lrX, n.botY, GDD);
    if (!wallDoor(w, lit, rng)) return;
    int a = g.lrX - (g.lrX - n.lrX) / 3, b = g.lrX - 2 * (g.lrX - n.lrX) / 3;
    int t = (g.topY + n.topY) / 2, u = (g.botY + n.botY) / 2;
    p.vline(a, t, g.botY, GDOOR);
    p.vline(b, (t + n.topY) / 2, u, GDOOR);
}

// FRONT wall: the rectangle at depth `g` (already the far geom).
void drawFrnt(Cells &p, const Geom &g, Wall w, bool lit, Rng &rng) {
    p.hline(g.ulX, g.lrX, g.topY, GH);
    p.hline(g.ulX, g.lrX, g.botY, GH);
    p.vline(g.ulX, g.topY, g.botY, GV);
    p.vline(g.lrX, g.topY, g.botY, GV);
    if (!wallDoor(w, lit, rng)) return;
    int cx = (g.ulX + g.lrX) / 2, dw = std::max(1, (g.lrX - g.ulX) / 5);
    int dy = g.topY + (g.botY - g.topY) / 3;
    p.vline(cx - dw, dy, g.botY, GV);
    p.vline(cx + dw, dy, g.botY, GV);
    p.hline(cx - dw, cx + dw, dy, GH);
}

// The receding-depth Geoms (cell units, centre column 17-18, 20 rows).
const Geom kDepth[5] = {
    { 1, 34,  0, 19, 8 },      // depth 0  -- fills the frame
    { 6, 29,  4, 15, 6 },      // depth 1
    { 9, 26,  7, 12, 4 },      // depth 2
    {11, 24,  9, 10, 2 },      // depth 3
    {12, 23, 10, 10, 1 },      // vanishing point
};

} // namespace

void renderMazeCells(u8 grid[kMazeRows][kMazeCols], const MazeLevel &m,
                     const MazePos &pos, int level, int &light,
                     bool quickPlot, Rng &rng) {
    for (int r = 0; r < kMazeRows; ++r)
        for (int c = 0; c < kMazeCols; ++c) grid[r][c] = 0;

    bool lit = light > 0;
    int maxDepth;
    if (lit) { maxDepth = quickPlot ? 3 : 4; light -= 1; }
    else     maxDepth = 2;

    Cells p{grid};
    int x4 = pos.x, y4 = pos.y, dir = pos.dir;

    for (int depth = 0; depth < maxDepth; ++depth) {
        int sq = m.squareExtra(x4, y4);
        Square st = m.squareType(sq);
        if (st == Square::Darkness) break;
        if (st == Square::Teleport && m.aux0(sq) == level) {
            x4 = wrap20(m.aux2(sq));
            y4 = wrap20(m.aux1(sq));
        }

        const Geom &g = kDepth[depth], &n = kDepth[depth + 1];
        p.clipLo = g.ulX; p.clipHi = g.lrX;

        Wall lw = leftView(m, x4, y4, dir, 0);
        if (!wallOpen(lw)) drawLeft(p, g, n, lw, lit, rng);

        Wall rw = righView(m, x4, y4, dir, 0);
        if (!wallOpen(rw)) drawRigh(p, g, n, rw, lit, rng);

        Wall fw = frwdView(m, x4, y4, dir, 0);
        if (!wallOpen(fw)) { drawFrnt(p, n, fw, lit, rng); break; }

        int nx = x4, ny = y4;
        shftPos(nx, ny, dir, 0, 1);
        x4 = nx; y4 = ny;
    }
}

} // namespace wiz
