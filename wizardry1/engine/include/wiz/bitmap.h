// Loaders for the game's raster graphics.
//
// 200/400.TITLE: 320x64, 2-bits-per-pixel CGA layout (each byte = 4 pixels,
// the high pair first), 5120 bytes.  The "Wizardry" script logo -- only
// colour indices 0 and 2 occur.
#pragma once
#include "wiz/surface.h"

namespace wiz {

// Decode a linear 2bpp CGA bitmap into an indexed (0..3) Surface.
Surface loadCga2bpp(Bytes data, int widthPx, int heightPx);

inline Surface loadTitle(Bytes data) { return loadCga2bpp(data, 320, 64); }

// A CGA-ish palette for the 0..3 indices (green-monitor flavour).
extern const Color kCgaPalette[4];

} // namespace wiz
