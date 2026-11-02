#include "files/tile_set.h"

#include "files/lzw.h"

#include <cstring>

namespace Ultima6 {

static bool decodePixelBlocks(const uint8_t *p, size_t avail, uint8_t *out) {
	std::memset(out, TileSet::TRANSPARENT_PIXEL, TileSet::SIZE);
	if (avail < 1)
		return false;
	size_t pos = 1; // the first byte is the block count, which the end marker makes redundant
	int dst = 0;
	for (;;) {
		if (pos + 3 > avail)
			return false;
		int disp = p[pos] | (p[pos + 1] << 8);
		int len = p[pos + 2];
		if (len == 0)
			return true;
		// What the MCGA driver's blit (U6MCGA.DRV sub_2458) does with a displacement: it is added to
		// the screen pointer in full, then 144 more is added and 176 subtracted per remaining row;
		// 176 + 144 is the 320 pixels of a screen line. For a tile that is 16 per row.
		int rest = disp;
		int delta = 0;
		while (rest >= 16) {
			delta += 16;
			rest -= 176;
		}
		dst += delta + rest;
		if (dst < 0)
			return false;
		if (pos + 3 + len > avail || dst + len > TileSet::SIZE)
			return false;
		std::memcpy(out + dst, p + pos + 3, len);
		dst += len;
		pos += 3 + len;
	}
}

bool TileSet::load(const Bytes &mapTiles, const Bytes &maskType, const Bytes &objTiles, const Bytes &tileIndex) {
	if (maskType.size() < COUNT || tileIndex.size() != COUNT * 2)
		return false;
	Bytes data = mapTiles;
	data.insert(data.end(), objTiles.begin(), objTiles.end());

	for (int i = 0; i < COUNT; ++i) {
		size_t offset = (size_t)get16(&tileIndex[i * 2]) * 16;
		kind[i] = maskType[i];
		if (offset >= data.size())
			return false;
		size_t avail = data.size() - offset;
		switch (kind[i]) {
		case PLAIN:
		case TRANSPARENT:
			if (avail < SIZE)
				return false;
			std::memcpy(pixels[i], &data[offset], SIZE);
			break;
		case PIXEL_BLOCKS:
			if (!decodePixelBlocks(&data[offset], avail, pixels[i]))
				return false;
			break;
		default:
			return false;
		}
	}
	return true;
}

bool LookData::load(const Bytes &d) {
	_text.clear();
	int tile = 0, entry = -1;
	size_t pos = 0;
	for (int t = 0; t < TileSet::COUNT; ++t)
		_entryFor[t] = 0;
	while (pos + 2 <= d.size()) {
		int first = get16(&d[pos]);
		if (first >= TileSet::COUNT)
			break;
		size_t end = pos + 2;
		while (end < d.size() && d[end] != 0)
			++end;
		if (end >= d.size())
			return false;
		_text.push_back(std::string((const char *)&d[pos + 2], end - pos - 2));
		++entry;
		for (; tile <= first; ++tile)
			_entryFor[tile] = entry;
		pos = end + 1;
	}
	for (; tile < TileSet::COUNT; ++tile)
		_entryFor[tile] = 0; // beyond the last entry: "Nothing"
	return !_text.empty();
}

} // namespace Ultima6
