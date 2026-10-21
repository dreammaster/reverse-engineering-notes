#include "wl/mapview.h"

namespace wl {

void drawMapView(Video &v, const Assets &a, const GameMap &map, int vx, int vy, bool partyMarker) {
	const auto &set = a.tilesets.at(map.info.tileset);   // ALLHTDS1 holds sets 0-3, ALLHTDS2 sets 4-8
	auto tilePix = [&](int id, int x, int y) -> uint8_t {
		const int bgIdx = map.info.backgroundTile - 10;
		const uint8_t bg = (bgIdx >= 0 && size_t(bgIdx) < set.size()) ? set[size_t(bgIdx)].at(x, y) : 0;
		if (id < 10) return uint8_t((a.masks[size_t(id)][size_t(y) * 16 + size_t(x)] ? bg : 0) | a.sprites[size_t(id)].at(x, y));
		return size_t(id - 10) < set.size() ? set[size_t(id - 10)].at(x, y) : 0;
	};
	for (int ty = 0; ty < 9; ++ty)
		for (int tx = 0; tx < 19; ++tx) {
			const int mx = vx + tx, my = vy + ty;
			const bool inside = mx >= 0 && mx < map.size && my >= 0 && my < map.size;
			int id = inside ? map.tiles[size_t(my) * size_t(map.size) + size_t(mx)] : map.info.backgroundTile;
			for (int y = 0; y < 16; ++y)
				for (int x = 0; x < 16; ++x) {
					const int px = tx * 16 + x, py = ty * 16 + y;
					if (px < 8 || px >= 296 || py < 9 || py >= 136) continue;
					uint8_t c = tilePix(id, x, y);
					if (partyMarker && tx == 9 && ty == 4) {   // party marker: sprite 7 over the cell
						const uint8_t under = id < 10 ? c : tilePix(id, x, y);
						c = uint8_t((a.masks[7][size_t(y) * 16 + size_t(x)] ? under : 0) | a.sprites[7].at(x, y));
					}
					v.put(px, py, c);
				}
		}
}

} // namespace wl
