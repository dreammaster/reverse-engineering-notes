#ifndef ULTIMA6_FILES_MAP_DATA_H
#define ULTIMA6_FILES_MAP_DATA_H

#include "files/data_file.h"

namespace Ultima6 {

/**
 * The world map: MAP (chunk indices) and CHUNKS (the tiles of each chunk), as
 * GAME.EXE's sub_17F17 (loads the map blocks), sub_18B37 (tile position -> chunk) and
 * sub_17E4F / sub_18146 (chunk -> tiles) use them.
 *
 *   MAP     64 superchunks of 0x180 bytes, then 5 dungeon levels of 0x600 bytes.
 *           Chunk indices are 12 bits, two per three bytes.
 *             surface  superchunk = (y >> 7) * 8 + (x >> 7)       (1024 x 1024 tiles)
 *                      index in it = ((y >> 3) & 15) * 16 + ((x >> 3) & 15)
 *             dungeon  level z = 1..5 at 0x6000 + (z - 1) * 0x600 (256 x 256 tiles)
 *                      index in it = ((y >> 3) & 31) * 32 + ((x >> 3) & 31)
 *   CHUNKS  1024 chunks of 8 x 8 tile numbers, row by row.
 */
class MapData {
public:
	enum {
		SURFACE_SIZE = 1024,
		DUNGEON_SIZE = 256,
		DUNGEON_LEVELS = 5,
		SUPERCHUNK_BYTES = 0x180,
		SURFACE_BYTES = 64 * SUPERCHUNK_BYTES,
		DUNGEON_BYTES = 0x600,
		MAP_SIZE = SURFACE_BYTES + DUNGEON_LEVELS * DUNGEON_BYTES,
		CHUNK_COUNT = 1024,
		CHUNK_TILES = 64,
		CHUNKS_SIZE = CHUNK_COUNT * CHUNK_TILES
	};

	bool load(const Bytes &map, const Bytes &chunks);

	/** Chunk number at a tile position; z = 0 is the surface, 1..5 the dungeon levels. Coordinates wrap. */
	int chunkIndex(int z, int x, int y) const;
	/** Tile number at a tile position. */
	uint8_t tile(int z, int x, int y) const;

private:
	Bytes _map, _chunks;
};

} // namespace Ultima6

#endif
