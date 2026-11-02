#include "files/tile_data.h"

#include <cstring>

namespace Ultima6 {

bool AnimData::load(const Bytes &data) {
	if (data.size() != FILE_SIZE)
		return false;
	const uint8_t *p = &data[0];
	count = get16(p);
	p += 2;
	for (int i = 0; i < MAX; ++i, p += 2)
		tileToAnimate[i] = get16(p);
	for (int i = 0; i < MAX; ++i, p += 2)
		firstFrame[i] = get16(p);
	std::memcpy(andMasks, p, MAX);
	p += MAX;
	std::memcpy(shiftValues, p, MAX);
	return count <= MAX;
}

bool TileFlags::load(const Bytes &data) {
	if (data.size() != FILE_SIZE)
		return false;
	const uint8_t *p = &data[0];
	std::memcpy(flags1, p, TILES);
	p += TILES;
	std::memcpy(flags2, p, TILES);
	p += TILES;
	std::memcpy(block3, p, BLOCK3);
	p += BLOCK3;
	std::memcpy(flags3, p, TILES);
	return true;
}

bool BaseTile::load(const Bytes &data) {
	if (data.size() != FILE_SIZE)
		return false;
	for (int i = 0; i < TYPES; ++i)
		firstTile[i] = get16(&data[i * 2]);
	return true;
}

bool Palette::load(const Bytes &data) {
	if (data.size() < USED_SIZE)
		return false;
	std::memcpy(rgb, &data[0], USED_SIZE);
	for (int i = 0; i < COLORS; ++i)
		for (int c = 0; c < 3; ++c)
			if (rgb[i][c] > 63)
				return false;
	return true;
}

} // namespace Ultima6
