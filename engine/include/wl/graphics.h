#pragma once
// Wasteland 1 graphics files: ALLHTDS (tilesets), ALLPICS (96-pixel wide portraits with animation data).
#include "wl/codec.h"

namespace wl {

// 4bpp image stored one palette index per byte.
struct Pic {
	int width = 0, height = 0;
	Bytes px;
	uint8_t at(int x, int y) const { return px[size_t(y) * size_t(width) + size_t(x)]; }
};

struct MsqHeaderInfo { bool ok = false, compressed = false; size_t size = 0; int disk = 0; size_t next = 0; };
MsqHeaderInfo readMsqHeader(const Bytes &f, size_t pos);   // 8 bytes: u32 size, "msq", disk

const uint32_t *egaPalette();                              // 16 ARGB colours

std::vector<std::vector<Pic>> loadTilesets(const Bytes &allhtds, int w = 16, int h = 16);

struct PicAnimation { Pic base; Bytes animData; };
std::vector<PicAnimation> loadPics(const Bytes &allpics, int width = 96);

} // namespace wl
