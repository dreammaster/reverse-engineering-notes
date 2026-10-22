#include "fod/gfx.h"
#include "fod/lzss.h"
#include <fstream>

namespace fod {

static const uint8_t kPal6[16][3] = {
	{0, 0, 0},       {0, 0, 0x27},    {3, 0x25, 3},    {0, 0x1B, 0x1B},
	{0x20, 0, 0},    {0x1B, 0, 0x1B}, {0x20, 0x1B, 5}, {0x26, 0x26, 0x26},
	{0x10, 0x10, 0x10}, {0, 0, 0x38}, {0, 0x3F, 0},    {0, 0x3F, 0x3F},
	{0x3A, 0x20, 0x10}, {0x3F, 0, 0x3F}, {0x3F, 0x3F, 0}, {0x3F, 0x3F, 0x3F}};

const Rgb *masterPalette() {
	static Rgb pal[16];
	static bool init = false;
	if (!init) {
		for (int i = 0; i < 16; ++i)
			pal[i] = {uint8_t(kPal6[i][0] * 255 / 63), uint8_t(kPal6[i][1] * 255 / 63), uint8_t(kPal6[i][2] * 255 / 63)};
		init = true;
	}
	return pal;
}

Tiles::Tiles(const Bytes &file) : _d(lzssDecode(file)) {}

void Tiles::blit(Image &img, int x, int y, int tile) const {
	if (tile < 0 || size_t(tile) >= count()) return;
	const Rgb *pal = masterPalette();
	const uint8_t *t = &_d[size_t(tile) * 128];
	for (int row = 0; row < 16; ++row)
		for (int col = 0; col < 8; ++col) {
			const uint8_t v = t[row * 8 + col];
			img.set(x + col * 2, y + row, pal[v >> 4]);
			img.set(x + col * 2 + 1, y + row, pal[v & 15]);
		}
}

Font::Font(const Bytes &file) {
	Reader r(file);
	const unsigned n = r.u16();
	_glyphs.resize(size_t(n) * 32);
	r.bytes(_glyphs.data(), _glyphs.size());
}

void Font::blit(Image &img, int x, int y, int ch, Rgb fg) const {
	if (ch < 0 || size_t(ch) >= count()) return;
	const uint8_t *g = &_glyphs[size_t(ch) * 32];
	for (int row = 0; row < 8; ++row)
		for (int col = 0; col < 4; ++col) {
			const uint8_t v = g[row * 4 + col];
			if (v >> 4) img.set(x + col * 2, y + row, fg);
			if (v & 15) img.set(x + col * 2 + 1, y + row, fg);
		}
}

Image renderMap(const Map &map, const Tiles &tiles) {
	Image img(map.width() * 16, map.height() * 16);
	for (int y = 0; y < map.height(); ++y)
		for (int x = 0; x < map.width(); ++x) tiles.blit(img, x * 16, y * 16, map.cell(x, y).tileIndex());
	return img;
}

} // namespace fod
