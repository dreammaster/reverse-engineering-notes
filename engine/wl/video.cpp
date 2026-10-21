#include "wl/video.h"

namespace wl {

void Video::colorGlyph(int cx, int cy, const Pic &g) {
	for (int y = 0; y < 8; ++y)
		for (int x = 0; x < 8; ++x) put(cx * 8 + x, cy * 8 + y, g.at(x, y));
}

void Video::monoGlyph(int cx, int cy, const uint8_t *rows, bool inverse) {
	for (int y = 0; y < 8; ++y)
		for (int x = 0; x < 8; ++x) {
			bool on = (rows[y] >> (7 - x)) & 1;
			if (inverse) on = !on;
			put(cx * 8 + x, cy * 8 + y, on ? 15 : 0);
		}
}

void Video::blit(const Pic &p, int x0, int y0) {
	for (int y = 0; y < p.height; ++y)
		for (int x = 0; x < p.width; ++x) put(x0 + x, y0 + y, p.at(x, y));
}

void Video::blitMasked(const Pic &sprite, const Bytes &keepMask, const Pic &under, int x0, int y0) {
	for (int y = 0; y < sprite.height; ++y)
		for (int x = 0; x < sprite.width; ++x) {
			const size_t i = size_t(y) * size_t(sprite.width) + size_t(x);
			put(x0 + x, y0 + y, uint8_t((keepMask[i] ? under.at(x, y) : 0) | sprite.at(x, y)));
		}
}

} // namespace wl
