#ifndef ULTIMA6_ULTIMA6_MAP_RENDERER_H
#define ULTIMA6_ULTIMA6_MAP_RENDERER_H

#include "engine/framebuffer.h"
#include "ultima6/world.h"

namespace Ultima6 {
namespace Ultima6 {

/**
 * Draws the map view the way GAME.EXE's redraw_screen (sub_1297E) composes it: an 11 x 11 grid of
 * 16 x 16 cells (a 176 x 176 buffer) around the party, terrain first and then the objects of each cell
 * from the bottom of the pile up, copied to the screen as the 160 x 160 pixel region (8,8)-(167,167)
 * of the buffer (so half a cell of each outer ring shows). Tiles of objects flagged double width or
 * double height in TILEFLAG's second block (bits 0x80 and 0x40) are completed with the tiles before them.
 *
 * Not done yet: tile animation, darkness and line of sight, and the effects tints.
 */
class MapRenderer {
public:
	enum { CELLS = 11, CELL_SIZE = 16, BUFFER_SIZE = CELLS * CELL_SIZE, SCREEN_X = 8, SCREEN_Y = 8, VIEW_SIZE = 160 };

	explicit MapRenderer(const World &world) : _world(world) {
		_buffer.assign(BUFFER_SIZE * BUFFER_SIZE, 0);
	}

	/** Draws the view centred on tile (x, y, z) onto the screen. */
	void draw(Framebuffer &screen, int x, int y, int z);

private:
	void drawTile(int tile, int px, int py);
	void drawObject(const World::Obj &o, int cellX, int cellY);

	const World &_world;
	std::vector<uint8_t> _buffer;
};

} // namespace Ultima6
} // namespace Ultima6

#endif
