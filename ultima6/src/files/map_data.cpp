#include "files/map_data.h"

namespace Ultima6 {

bool MapData::load(const Bytes &map, const Bytes &chunks) {
	if (map.size() != MAP_SIZE || chunks.size() != CHUNKS_SIZE)
		return false;
	_map = map;
	_chunks = chunks;
	return true;
}

int MapData::chunkIndex(int z, int x, int y) const {
	int base, index;
	if (z == 0) {
		x &= SURFACE_SIZE - 1;
		y &= SURFACE_SIZE - 1;
		base = ((y >> 7) * 8 + (x >> 7)) * SUPERCHUNK_BYTES;
		index = ((y >> 3) & 15) * 16 + ((x >> 3) & 15);
	} else {
		x &= DUNGEON_SIZE - 1;
		y &= DUNGEON_SIZE - 1;
		base = SURFACE_BYTES + (z - 1) * DUNGEON_BYTES;
		index = ((y >> 3) & 31) * 32 + ((x >> 3) & 31);
	}
	int offset = base + index + (index >> 1);
	int word = _map[offset] | (_map[offset + 1] << 8);
	return (index & 1) ? word >> 4 : word & 0xFFF;
}

uint8_t MapData::tile(int z, int x, int y) const {
	return _chunks[chunkIndex(z, x, y) * CHUNK_TILES + (y & 7) * 8 + (x & 7)];
}

} // namespace Ultima6
