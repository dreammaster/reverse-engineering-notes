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

	/**
	 * The frame counter (word_3DB88 of sub_12023, which counts the redraws): the tiles of ANIMDATA show the frame (counter & mask) >> shift after their first frame; a tile whose
	 * entry is switched off shows its first frame. Ultima VI's bell, the 0x120 and the 0x125 only move when something makes them (sub_11FF9), which is not done: they stand still.
	 */
	void setFrame(unsigned counter) {
		_frame = counter;
	}

	/** Draws the view centred on tile (x, y, z) onto the screen. */
	void draw(Framebuffer &screen, int x, int y, int z, const uint8_t *darkness = nullptr);

	/**
	 * How dark each of the 11 x 11 cells is for a party at (x, y, z) at an hour (0 lit, 1 dim, 2 black): the surface is dark at night, dungeons always;
	 * what the party carries (a lit torch, the Light spell) lights a few squares around it and lit lamps, torches, candles and campfires on the
	 * map light their surroundings. An approximation of the original's light levels.
	 */
	void computeDarkness(int x, int y, int z, int hour, int partyLightRadius, uint8_t *out) const;

private:
	void drawTile(int tile, int px, int py);
	void drawObject(const World::Obj &o, int cellX, int cellY);

	void buildRemap();

	const World &_world;
	std::vector<uint8_t> _buffer;
	unsigned _frame = 0;
	uint16_t _remap[TileSet::COUNT];
};

} // namespace Ultima6
} // namespace Ultima6

#endif
