// DRAWMAZE -- the first-person wireframe (RUNNER P010E02).
//
// Ported from the Apple source: a receding stack of trapezoidal wall panels
// drawn with DRAWLINE (runs of points along one of 8 directions) into an
// 82x79 "picture" area.  The DOS build precomputes the same shape into 4
// fixed depth layers (CONUNIT UNITWRITE subfn 5); this reproduces the Apple
// halving loop, which is equivalent and fully specified.
#pragma once
#include "wiz/surface.h"
#include "wiz/maze.h"
#include "wiz/runner.h"
#include "wiz/rng.h"

namespace wiz {

// The picture area the view is drawn into.
constexpr int kPicW = 82, kPicH = 79;

// Render the view from `pos` on `level` into `pic` (>= 82x79, index 0 = black,
// `ink` = wall colour).  `light` is the party's LIGHT counter; on a lit draw
// it is decremented and the view reaches further.  `rng` drives the
// hidden-door reveal roll.
void drawMazeView(Surface &pic, const MazeLevel &m, const MazePos &pos,
                  int level, int &light, bool quickPlot, Rng &rng,
                  u8 ink = 2);

} // namespace wiz
