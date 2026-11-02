#ifndef ULTIMA6_FILES_TILE_DATA_H
#define ULTIMA6_FILES_TILE_DATA_H

#include "files/data_file.h"

namespace Ultima6 {

/**
 * ANIMDATA (194 bytes), as read by GAME.EXE's load_tile_attribs: a count, then
 * four parallel arrays of 32 entries.
 */
struct AnimData {
	enum { MAX = 32, FILE_SIZE = 2 + MAX * 2 + MAX * 2 + MAX + MAX };

	uint16_t count;
	uint16_t tileToAnimate[MAX];
	uint16_t firstFrame[MAX];
	uint8_t andMasks[MAX];
	uint8_t shiftValues[MAX];

	bool load(const Bytes &data);
};

/**
 * TILEFLAG (7168 bytes), read by load_tile_attribs into four buffers, one per tile
 * (2048 tiles) except the third, which has 1024 entries (its meaning is not
 * established yet).
 */
struct TileFlags {
	enum { TILES = 2048, BLOCK3 = 1024, FILE_SIZE = TILES + TILES + BLOCK3 + TILES };

	uint8_t flags1[TILES];  // +0x0000
	uint8_t flags2[TILES];  // +0x0800
	uint8_t block3[BLOCK3]; // +0x1000
	uint8_t flags3[TILES];  // +0x1400, top two bits = article

	bool load(const Bytes &data);
};

/**
 * BASETILE (2048 bytes): the first tile of every object type, indexed by the
 * 10-bit object type. A tile of an object is basetile[type] + frame.
 */
struct BaseTile {
	enum { TYPES = 1024, FILE_SIZE = TYPES * 2 };

	uint16_t firstTile[TYPES];

	bool load(const Bytes &data);
};

/**
 * U6PAL: the VGA palette (256 RGB triples, 6 bits per component). The game reads
 * only the first 0x300 bytes of the 1024-byte file.
 */
struct Palette {
	enum { COLORS = 256, USED_SIZE = COLORS * 3 };

	uint8_t rgb[COLORS][3];

	bool load(const Bytes &data);
};

} // namespace Ultima6

#endif
