// DRAWMAZE -- the DOS first-person wireframe (RUNNER proc 3 + the depth-layer
// procs 21-41).
//
// The DOS build draws the view as CHARSET line-art glyphs into a 36x20
// character grid (WINDOW1), NOT as pixel lines: each depth layer is a fixed
// table of DRAWLINE(glyph, count, dRow, dCol, row, col) cell-runs, gated by
// the walls the party can see (leftView / frwdView / righView + doors).
// See docs/maze.md.
#pragma once
#include "wiz/types.h"
#include "wiz/maze.h"
#include "wiz/runner.h"
#include "wiz/rng.h"

namespace wiz {

// WINDOW1 is 36 wide; the DRAWLINE tables address rows 0..20 (the DOS
// buffer's h-2 sentinel), so render a 22-row grid and display the top 20.
constexpr int kMazeCols = 36, kMazeRows = 22, kMazeView = 20;

// Render the view from `pos` on `level` into `grid` (36 wide x 20 tall) as
// CHARSET glyph codes; 0 = empty.  `light` is the LIGHT counter -- a lit
// draw decrements it and reaches two squares further; `rng` drives the
// hidden-door reveal roll.
void renderMazeCells(u8 grid[kMazeRows][kMazeCols], const MazeLevel &m,
                     const MazePos &pos, int level, int &light,
                     bool quickPlot, Rng &rng);

} // namespace wiz
