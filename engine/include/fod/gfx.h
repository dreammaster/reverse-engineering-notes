#pragma once
#include "fod/bytes.h"
#include "fod/map.h"

namespace fod {

struct Rgb { uint8_t r, g, b; };

// The 16-colour master palette (FOD MASTER_PALETTE, 6-bit VGA values scaled to 8 bit).
const Rgb *masterPalette();

// Linear RGB image.
class Image {
public:
	Image(int w, int h) : _w(w), _h(h), _px(size_t(w) * h * 3, 0) {}
	int width() const { return _w; }
	int height() const { return _h; }
	void set(int x, int y, Rgb c) { uint8_t *p = &_px[(size_t(y) * _w + x) * 3]; p[0] = c.r; p[1] = c.g; p[2] = c.b; }
	const Bytes &pixels() const { return _px; }
	void writePng(const std::string &path) const;
private:
	int _w, _h;
	Bytes _px;
};

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
