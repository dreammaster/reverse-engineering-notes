#include "data/Tiles.h"

#include <stdio.h>

namespace Yendor1 {
namespace Data {

bool TileFile::load(const char *gameDir, int fileNumber) {
	if (fileNumber < 0 || fileNumber >= kTileFileCount)
		return false;
	char name[16], path[512];
	snprintf(name, sizeof(name), "FILE%02d.16", fileNumber);
	if (!Shared::joinPath(path, sizeof(path), gameDir, name))
		return false;
	if (!_blob.load(path))
		return false;
	return _blob.size() % kTileBytes == 0;
}

void TileFile::assign(const byte *data, size_t size) {
	_blob.assign(data, size);
}

const byte *TileFile::raw(size_t index) const {
	if (index >= count())
		return 0;
	return _blob.data() + index * kTileBytes;
}

bool TileFile::decode(size_t index, byte *out1024) const {
	const byte *r = raw(index);
	if (!r)
		return false;
	decodeRaw(r, out1024);
	return true;
}

void TileFile::decodeRaw(const byte *raw512, byte *out1024) {
	for (int i = 0; i < kTileWidth * kTileHeight; i++)
		out1024[i] = 0;
	// plane order in the file: 3, 2, 1, 0
	for (int p = 0; p < 4; p++) {
		const int plane = 3 - p;
		const byte *src = raw512 + p * kTilePlaneBytes;
		for (int y = 0; y < kTileHeight; y++) {
			for (int bx = 0; bx < 4; bx++) {
				const byte v = src[y * 4 + bx];
				for (int bit = 0; bit < 8; bit++) {
					if (v & (0x80 >> bit))
						out1024[y * kTileWidth + bx * 8 + bit] |= (byte)(1 << plane);
				}
			}
		}
	}
}

} // namespace Data
} // namespace Yendor1
