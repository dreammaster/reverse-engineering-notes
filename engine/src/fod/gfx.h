#pragma once
#include "fod/types.h"
#include "fod/map.h"

namespace fod {

// The 16-colour master palette (FOD MASTER_PALETTE, 6-bit VGA values scaled to 8 bit).
const Rgb *masterPalette();

// TILES: LZSS -> 1000 tiles of 16x16, 4bpp (high nibble = left pixel).
class Tiles {
public:
	explicit Tiles(const Bytes &file);
	size_t count() const { return _d.size() / 128; }
	void blit(Image &img, int x, int y, int tile) const;
private:
	Bytes _d;
};

// FONT: u16 count then 8x8 4bpp glyphs (32 bytes each).
class Font {
public:
	explicit Font(const Bytes &file);
	size_t count() const { return _glyphs.size() / 32; }
	void blit(Image &img, int x, int y, int ch, Rgb fg) const;
private:
	Bytes _glyphs;
};

Image renderMap(const Map &map, const Tiles &tiles);

} // namespace fod
