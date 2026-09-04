// 200.MONSTERS -- DOS monster combat portraits.  See /docs/file-formats.md.
//
// A record is 512 bytes; `PIC` is 1-based, so record n occupies bytes
// (PIC-1)*512 .. +512.  Each record is a 6-wide x 5-tall grid of 16x8 px
// 1bpp tiles, row-major (tile t = row*6 + col, at record offset t*16), two
// bytes per tile row, MSB = leftmost pixel.  The composed portrait is
// 96 x 40 px.  Geometry from SYSTEM.INTERP's CONUNIT blitter (0x1E9A),
// confirmed live in DOSBox and by rendering all of WIZ1's portraits
// (2026-09-04).
#pragma once
#include "wiz/types.h"

namespace wiz {

class Surface;

constexpr int kPortraitW = 96, kPortraitH = 40;   // px, 6x5 tiles of 16x8

// Blit the portrait held in the 512-byte record `rec` into `dst` at (dx,dy):
// set bits become colour `fg`; clear bits are left alone unless `bg >= 0`.
// A record shorter than 512 bytes (e.g. Scenario::monsterArtRecord of an
// unloaded / blank PIC) draws nothing.
void blitPortrait(Surface &dst, Bytes rec, int dx, int dy, u8 fg, int bg = -1);

} // namespace wiz
