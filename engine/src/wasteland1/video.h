#pragma once
// 320x200 indexed (16 colour) frame buffer and the glyph primitives the original program draws with.
#include "wasteland1/graphics.h"
#include <vector>

namespace wl {

class Video {
public:
	static constexpr int W = 320, H = 200;
	Video() : _px(size_t(W) * H, 0) {}

	void clear(uint8_t c = 0) { std::fill(_px.begin(), _px.end(), c); }
	void put(int x, int y, uint8_t c) { if (x >= 0 && y >= 0 && x < W && y < H) _px[size_t(y) * W + size_t(x)] = c; }
	uint8_t get(int x, int y) const { return _px[size_t(y) * W + size_t(x)]; }
	const Bytes &pixels() const { return _px; }

	// 8x8 glyphs at character cell (cx, cy)
	void colorGlyph(int cx, int cy, const Pic &glyph);
	void monoGlyph(int cx, int cy, const uint8_t *rows8, bool inverse);   // white on black

	void blit(const Pic &p, int x, int y);                              // opaque
	void blitMasked(const Pic &p, const Bytes &keepMask, const Pic &under, int x, int y);   // sprite over `under`

private:
	Bytes _px;
};

} // namespace wl
