#include "ultima6/map_renderer.h"

#include <algorithm>
#include <cstdlib>

namespace Ultima6 {
namespace Ultima6 {

// sub_12023 12040-12090: for every entry of ANIMDATA the driver is told (call 0x4B) to show frame `first + ((mask & counter) >> shift)` in place of the tile; an entry that is switched
// off shows its first frame (the bell 0xEC, 0x120 and 0x125 of Ultima VI)
void MapRenderer::buildRemap() {
	for (int i = 0; i < TileSet::COUNT; ++i)
		_remap[i] = (uint16_t)i;
	const AnimData &a = _world.anim;
	for (int i = 0; i < (int)a.count && i < AnimData::MAX; ++i) {
		int tile = a.tileToAnimate[i];
		if (tile >= TileSet::COUNT)
			continue;
		bool still = false;
		if (_world.game == GameId::Ultima6)
			for (int type : {0x125, 0x120, 0xEC})
				still |= tile == _world.baseTile.firstTile[type];
		int frame = still ? 0 : (int)((a.andMasks[i] & _frame) >> a.shiftValues[i]);
		_remap[tile] = (uint16_t)((a.firstFrame[i] + frame) & (TileSet::COUNT - 1));
	}
}

void MapRenderer::drawTile(int tile, int px, int py) {
	if (tile < 0 || tile >= TileSet::COUNT)
		return;
	tile = _remap[tile];
	const uint8_t *src = _world.tiles.pixels[tile];
	bool transparent = _world.tiles.transparent(tile);
	for (int j = 0; j < CELL_SIZE; ++j) {
		int y = py + j;
		if (y < 0 || y >= BUFFER_SIZE)
			continue;
		for (int i = 0; i < CELL_SIZE; ++i) {
			int x = px + i;
			if (x < 0 || x >= BUFFER_SIZE)
				continue;
			uint8_t v = src[j * CELL_SIZE + i];
			if (!transparent || v != TileSet::TRANSPARENT_PIXEL)
				_buffer[y * BUFFER_SIZE + x] = v;
		}
	}
}

void MapRenderer::drawObject(const World::Obj &o, int cellX, int cellY) {
	int tile = _world.baseTile.firstTile[o.type] + o.frame;
	int px = cellX * CELL_SIZE, py = cellY * CELL_SIZE;
	uint8_t f2 = _world.flags.flags2[tile & (TileSet::COUNT - 1)];
	bool wide = f2 & 0x80, tall = f2 & 0x40;
	drawTile(tile, px, py);
	// a double-width or double-height tile is drawn together with the tiles that precede it
	if (wide)
		drawTile(tile - 1, px - CELL_SIZE, py);
	if (tall) {
		drawTile(tile - (wide ? 2 : 1), px, py - CELL_SIZE);
		if (wide)
			drawTile(tile - 3, px - CELL_SIZE, py - CELL_SIZE);
	}
}

// APPROX(15): not the original's exact behaviour, see docs/approximations.md
void MapRenderer::computeDarkness(int x, int y, int z, int hour, int partyLightRadius, uint8_t *out) const {
	bool night = z != 0 || hour < 5 || hour >= 19;
	int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
	int left = x - CELLS / 2, top = y - CELLS / 2;
	std::vector<std::pair<int, int>> lights; // in cell coordinates, with radius in the second
	std::vector<int> radius;
	for (int i = 0; i < CELLS * CELLS; ++i)
		out[i] = night ? 2 : 0;
	if (!night)
		return;
	std::vector<int> here;
	for (int cy = -3; cy < CELLS + 3; ++cy)
		for (int cx = -3; cx < CELLS + 3; ++cx) {
			_world.objectsAt((left + cx) & mask, (top + cy) & mask, z, here);
			for (int idx : here) {
				const World::Obj &o = _world.objects[idx];
				bool lit = ((o.type == 90 || o.type == 122 || o.type == 145 || o.type == 164 || o.type == 206) && (o.frame & 1)) || o.type == 253;
				if (lit && o.actor < 0) {
					lights.push_back({cx, cy});
					radius.push_back(3);
				}
			}
		}
	lights.push_back({CELLS / 2, CELLS / 2});
	radius.push_back(partyLightRadius);
	for (int cy = 0; cy < CELLS; ++cy)
		for (int cx = 0; cx < CELLS; ++cx) {
			int best = 2;
			for (size_t n = 0; n < lights.size(); ++n) {
				int d = std::max(std::abs(cx - lights[n].first), std::abs(cy - lights[n].second));
				if (d <= radius[n])
					best = 0;
				else if (d == radius[n] + 1)
					best = std::min(best, 1);
			}
			out[cy * CELLS + cx] = (uint8_t)best;
		}
}

void MapRenderer::draw(Framebuffer &screen, int x, int y, int z, const uint8_t *darkness) {
	composite(screen, x, y, z, nullptr, false, darkness);
}

void MapRenderer::draw(Framebuffer &screen, int x, int y, int z, const ViewGrid &view, bool infravision) {
	composite(screen, x, y, z, &view, infravision, nullptr);
}

// APPROX(14): not the original's exact behaviour, see docs/approximations.md
void MapRenderer::composite(Framebuffer &screen, int x, int y, int z, const ViewGrid *view, bool infravision, const uint8_t *darkness) {
	int left = x - CELLS / 2, top = y - CELLS / 2;
	int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
	buildRemap();

	for (int cy = 0; cy < CELLS; ++cy)
		for (int cx = 0; cx < CELLS; ++cx)
			drawTile(view ? view->tile[cy][cx] : _world.map.tile(z, (left + cx) & mask, (top + cy) & mask), cx * CELL_SIZE, cy * CELL_SIZE);

	// objects: rows from the top so that taller pieces overlap the cell above correctly; the ring one
	// cell outside the view is included because of the double-size tiles
	std::vector<int> here;
	for (int pass = 0; pass < 2; ++pass) {
		for (int cy = -1; cy <= CELLS; ++cy) {
			for (int cx = -1; cx <= CELLS; ++cx) {
				_world.objectsAt((left + cx) & mask, (top + cy) & mask, z, here);
				for (int idx : here) {
					const World::Obj &o = _world.objects[idx];
					if (o.invisible() || o.type == 335) // eggs are never drawn
						continue;
					int tile = _world.baseTile.firstTile[o.type] + o.frame;
					if (view && cx >= 0 && cx < CELLS && cy >= 0 && cy < CELLS) {
						// sub_1C93C: nothing is put on a square that is not seen, nor on a dark one unless it is a warm thing and the party sees by infravision; sub_1CB82 takes
						// the things off a wall that is not seen from the east or the south
						int t = view->tile[cy][cx];
						if (t == ViewGrid::UNSEEN || view->hideThings[cy][cx])
							continue;
						if (t == ViewGrid::DARK && !(infravision && (_world.flags.flags3[tile & (TileSet::COUNT - 1)] & 1)))
							continue;
					}
					bool topTile = _world.flags.flags2[tile & (TileSet::COUNT - 1)] & 0x10;
					if ((pass == 1) == topTile)
						drawObject(o, cx, cy);
				}
			}
		}
	}

	if (view) {
		// the shade of a square that is lit only a little (sub_1CB82 1CF55-1CF8E): the overlay tile 0x1BC + the light, over what is there
		for (int cy = 0; cy < CELLS; ++cy)
			for (int cx = 0; cx < CELLS; ++cx) {
				int l = view->light[cy][cx];
				if (l > 0 && l < 4 && view->tile[cy][cx] != ViewGrid::UNSEEN && view->tile[cy][cx] != ViewGrid::DARK)
					drawTile(ViewGrid::DARK + l, cx * CELL_SIZE, cy * CELL_SIZE);
			}
	}
	if (darkness) {
		for (int cy = 0; cy < CELLS; ++cy)
			for (int cx = 0; cx < CELLS; ++cx) {
				int d = darkness[cy * CELLS + cx];
				if (!d)
					continue;
				for (int j = 0; j < CELL_SIZE; ++j)
					for (int i = 0; i < CELL_SIZE; ++i)
						if (d == 2 || ((i + j) & 1))
							_buffer[(cy * CELL_SIZE + j) * BUFFER_SIZE + cx * CELL_SIZE + i] = 0;
			}
	}
	screen.blit(SCREEN_X, SCREEN_Y, &_buffer[SCREEN_Y * BUFFER_SIZE + SCREEN_X], VIEW_SIZE, VIEW_SIZE, BUFFER_SIZE);
}

} // namespace Ultima6
} // namespace Ultima6
