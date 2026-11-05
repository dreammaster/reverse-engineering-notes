#ifndef ULTIMA6_ULTIMA6_VIEW_H
#define ULTIMA6_ULTIMA6_VIEW_H

#include "ultima6/world.h"

namespace Ultima6 {
namespace Ultima6 {

/**
 * What the party sees of the 11 x 11 squares around it: the line of sight and the light, as GAME.EXE's sub_18F86 (the terrain of every square), sub_18DB1 (a light or a view
 * spreading out from a square), sub_18C8B (whether light passes a square) and sub_18D4F (the queue of squares) work them out, and the squares of sub_1CB82 that hide their things.
 *
 * The game keeps a 24 x 24 window of the map in grids 40 squares wide (columns and rows 8 to 31, the party stands at 16-23); here the same grids are used so that the quirks of the
 * original (the squares past the edge of a row, the order the squares are visited in) come out the same.
 */
struct ViewGrid {
	enum { CELLS = 11, UNSEEN = 0xFF, DARK = 0x1BC };

	uint16_t tile[CELLS][CELLS]; // what the terrain looks like: UNSEEN (black, nothing on it is drawn), DARK (seen, but black unless something warm stands there), else a tile
	uint8_t light[CELLS][CELLS]; // the light on the square: 0 dark, 1 to 3 shaded (the overlay tile 0x1BC + n), 4 and over bright
	bool hideThings[CELLS][CELLS]; // a wall whose east or south neighbour is not seen: what stands on it is not drawn
};

class ViewBuilder {
public:
	explicit ViewBuilder(const World &world) : _world(world) {
	}

	/** The view of a party at (x, y, z) with the ambient light `ambient` (byte_404A5: 0 to 7). */
	void build(int x, int y, int z, int ambient, ViewGrid &out);

private:
	enum { WIDTH = 40, SIZE = WIDTH * WIDTH + 2 * WIDTH };

	bool passes(int x, int y) const;             // sub_18C8B
	void enqueue(int x, int y);                  // sub_18D4F
	void spread(int mode, int level, int y, int x); // sub_18DB1
	int distance(int dx, int dy) const;          // the table at ds:05EA

	const World &_world;
	uint8_t _flags[SIZE];    // the grid at ds:94B5: 0x80 seen, 0x40 queued, 0x20 and 0x10 and 0x08 and 0x04 what stands on the square, 0x03 the light it gives
	uint8_t _light[SIZE];    // the grid at ds:ACAB
	uint16_t _terrain[SIZE]; // the grid at ds:8E51
	uint8_t _queueX[256], _queueY[256];
	uint8_t _tail = 0, _head = 0; // byte_3DE7A, byte_3DE7B
	int _ox = 0, _oy = 0;         // word_3DE34, word_3DE36: where the 11 x 11 window starts in the grids
	int _cell = 0;                // word_4961C
};

} // namespace Ultima6
} // namespace Ultima6

#endif
