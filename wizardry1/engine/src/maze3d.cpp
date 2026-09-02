#include "wiz/maze3d.h"

namespace wiz {
namespace {

// DRAWLINE(x, y, dh, dv, len): plot `len` points from (x,y) stepping (dh,dv),
// clipped to x in [clipLo, clipHi] and the 82x79 picture.
struct Pic {
    Surface &s;
    int clipLo = 0, clipHi = kPicW - 1;
    u8 ink;
    void line(int x, int y, int dh, int dv, int len) {
        for (int i = 0; i < len; ++i) {
            if (x >= clipLo && x <= clipHi && x >= 0 && x < kPicW && y >= 0 && y < kPicH)
                s.set(x, y, ink);
            x += dh; y += dv;
        }
    }
};

struct Geom { int ul, lr, ww, wh, dw, df; };

// EXIT test shared by DRAWLEFT/DRAWRIGH/DRAWFRNT: draw the door cut-out only
// for a real DOOR, or a HIDEDOOR that is revealed (lit, or a 1-in-6 glimpse).
bool noDoor(Wall w, bool gotLight, Rng &rng) {
    if (w == Wall::Open || w == Wall::Wall) return true;
    if (w == Wall::HiddenDoor) return !(gotLight || rng.mod(6) == 3);
    return false;                                   // Wall::Door
}

void drawLeft(Pic &p, const Geom &g, Wall wt, bool gotLight, Rng &rng, int &xLower) {
    xLower = g.ul;
    p.line(g.ul, g.ul, -1, -1, g.ww);
    p.line(g.ul, g.ul, 0, 1, g.wh);
    p.line(g.ul, g.lr, -1, 1, g.ww);
    p.line(g.ul - g.ww, g.ul - g.ww, 0, 1, g.wh + g.wh);
    if (noDoor(wt, gotLight, rng)) return;
    p.line(g.ul - g.df, g.ul, -1, -1, g.dw);
    p.line(g.ul - g.df, g.ul, 0, 1, g.wh + g.df);
    p.line(g.ul - g.df - g.dw, g.ul - g.dw, 0, 1, g.wh + g.ww + g.df);
}

void drawRigh(Pic &p, const Geom &g, Wall wt, bool gotLight, Rng &rng, int &xUpper) {
    xUpper = g.lr;
    p.line(g.lr, g.ul, 1, -1, g.ww);
    p.line(g.lr, g.ul, 0, 1, g.wh);
    p.line(g.lr, g.lr, 1, 1, g.ww);
    p.line(g.lr + g.ww, g.ul - g.ww, 0, 1, g.wh + g.wh);
    if (noDoor(wt, gotLight, rng)) return;
    p.line(g.lr + g.df, g.ul, 1, -1, g.dw);
    p.line(g.lr + g.df, g.ul, 0, 1, g.wh + g.df);
    p.line(g.lr + g.df + g.dw, g.ul - g.dw, 0, 1, g.wh + g.ww + g.df);
}

void drawFrnt(Pic &p, const Geom &g, Wall wt, int lrCent, bool gotLight, Rng &rng) {
    p.line(g.ul + lrCent, g.ul, 1, 0, g.wh);
    p.line(g.ul + lrCent, g.ul, 0, 1, g.wh);
    p.line(g.ul + lrCent + g.wh, g.ul, 0, 1, g.wh + 1);
    p.line(g.ul + lrCent, g.ul + g.wh, 1, 0, g.wh);
    if (noDoor(wt, gotLight, rng)) return;
    p.line(g.ul + lrCent + g.df, g.lr, 0, -1, g.ww + g.dw + g.df);
    p.line(g.ul + lrCent + g.ww + g.dw + g.df, g.lr, 0, -1, g.ww + g.dw + g.df);
    p.line(g.ul + lrCent + g.df, g.lr - g.ww - g.dw - g.df, 1, 0, g.ww + g.dw + 1);
}

} // namespace

void drawMazeView(Surface &pic, const MazeLevel &m, const MazePos &pos,
                  int level, int &light, bool quickPlot, Rng &rng, u8 ink) {
    bool gotLight = light > 0;
    int lightDis;
    if (gotLight) { lightDis = quickPlot ? 3 : 5; light -= 1; }
    else lightDis = 2;

    Geom g{8, 72, 32, 64, 16, 8};
    int x4 = pos.x, y4 = pos.y, dir = pos.dir;

    pic.fillRect(0, 0, kPicW, kPicH, 0);            // CLEARPIC
    Pic p{pic, 0, kPicW - 1, ink};
    int xLower = 0, xUpper = kPicW - 1;

    while (lightDis > 0) {
        int sq = m.squareExtra(x4, y4);
        Square st = m.squareType(sq);
        if (st == Square::Darkness) return;
        if (st == Square::Teleport && m.aux0(sq) == level) {
            x4 = wrap20(m.aux2(sq));
            y4 = wrap20(m.aux1(sq));
        }
        p.clipLo = xLower;
        p.clipHi = xUpper;

        Wall wt = leftView(m, x4, y4, dir, 0);
        if (wt != Wall::Open) drawLeft(p, g, wt, gotLight, rng, xLower);
        else {
            wt = frwdView(m, x4, y4, dir, -1);
            if (wt != Wall::Open) { drawFrnt(p, g, wt, -(2 * g.ww), gotLight, rng); xLower = g.ul; }
        }

        wt = righView(m, x4, y4, dir, 0);
        if (wt != Wall::Open) drawRigh(p, g, wt, gotLight, rng, xUpper);
        else {
            wt = frwdView(m, x4, y4, dir, 1);
            if (wt != Wall::Open) { drawFrnt(p, g, wt, 2 * g.ww, gotLight, rng); xUpper = g.lr; }
        }

        wt = frwdView(m, x4, y4, dir, 0);
        if (wt != Wall::Open) { drawFrnt(p, g, wt, 0, gotLight, rng); return; }

        g.ww /= 2;
        g.dw = g.ww / 2;
        g.wh = g.ww * 2;
        g.df = g.ww / 4;
        g.ul += g.ww;
        g.lr -= g.ww;

        int nx = x4, ny = y4;
        shftPos(nx, ny, dir, 0, 1);                 // step forward
        x4 = nx; y4 = ny;
        --lightDis;
    }
}

} // namespace wiz
