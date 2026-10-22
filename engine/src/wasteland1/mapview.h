#pragma once
// Map window: 19x9 tiles of the current map around the party, clipped to the frame (pixels x 8..295, y 9..135).
#include "wasteland1/assets.h"
#include "wasteland1/video.h"

namespace wl {

// vx, vy: map coordinates of the top-left visible tile (party x - 9, y - 4). Cells outside the map show the
// background tile; sprite tiles (< 10) are drawn over the background tile.
void drawMapView(Video &v, const Assets &a, const GameMap &map, int vx, int vy, bool partyMarker);

} // namespace wl
