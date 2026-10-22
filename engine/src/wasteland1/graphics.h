#pragma once
// Wasteland 1 graphics files: ALLHTDS (tilesets), ALLPICS (96-pixel wide portraits with animation data).
#include "wasteland1/codec.h"

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

// IC0_9.WLF: ten 16x16 planar sprites (4 bit planes of 32 bytes each); MASKS.WLF: ten 16x16 1bpp masks (1 = keep background).
std::vector<Pic> loadSprites(const Bytes &wlf);
std::vector<Bytes> loadMasks(const Bytes &wlf);   // each mask: 256 bytes, one 0/1 per pixel

// COLORF.FNT: 172 glyphs of 8x8, four 8-byte bit planes each.
std::vector<Pic> loadFont(const Bytes &fnt);

struct PicAnimation { Pic base; Bytes animData; };
std::vector<PicAnimation> loadPics(const Bytes &allpics, int width = 96);

} // namespace wl
