#include "ultima6/map_renderer.h"

namespace Ultima6 {
namespace Ultima6 {

void MapRenderer::drawTile(int tile, int px, int py) {
	if (tile < 0 || tile >= TileSet::COUNT)
		return;
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

void MapRenderer::draw(Framebuffer &screen, int x, int y, int z) {
	int left = x - CELLS / 2, top = y - CELLS / 2;
	int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;

	for (int cy = 0; cy < CELLS; ++cy)
		for (int cx = 0; cx < CELLS; ++cx)
			drawTile(_world.map.tile(z, (left + cx) & mask, (top + cy) & mask), cx * CELL_SIZE, cy * CELL_SIZE);

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
					bool topTile = _world.flags.flags2[tile & (TileSet::COUNT - 1)] & 0x10;
					if ((pass == 1) == topTile)
						drawObject(o, cx, cy);
				}
			}
		}
	}

	screen.blit(SCREEN_X, SCREEN_Y, &_buffer[SCREEN_Y * BUFFER_SIZE + SCREEN_X], VIEW_SIZE, VIEW_SIZE, BUFFER_SIZE);
}

} // namespace Ultima6
} // namespace Ultima6
