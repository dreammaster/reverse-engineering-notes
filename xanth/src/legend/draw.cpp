#include "legend/draw.h"

namespace Legend {

// word_892EC..892F4 of the original's data segment
static const int kFrameFill = 10, kLighten = 4, kDarken = 3, kRampLow = 0x11, kRampHigh = 0x1F;

void fillArea(Surface &s, int x1, int y1, int x2, int y2, byte color) {
	s.fillRect(Common::Rect((int16)x1, (int16)y1, (int16)(x2 + 1), (int16)(y2 + 1)), color);
}

static void lighten(Surface &s, int x, int y) {
	int v = s.get(x, y) + kLighten;
	s.put(x, y, (byte)(v > kRampHigh ? kRampHigh : v));
}

static void darken(Surface &s, int x, int y) {
	int v = s.get(x, y) - kDarken;
	s.put(x, y, (byte)(v < kRampLow ? kRampLow : v));
}

void bevelFrame(Surface &s, int x1, int y1, int x2, int y2) {
	// the original walks the left edge upwards, the top edge to the right, the right edge downwards and the bottom edge to the left; every edge pixel is changed
	// once (the colour is read before it is written)
	for (int y = y2 - 1; y > y1; y--)
		darken(s, x1, y);
	for (int x = x1; x < x2; x++)
		darken(s, x, y1);
	for (int y = y1; y <= y2; y++)
		lighten(s, x2, y);
	for (int x = x2 - 1; x >= x1; x--)
		lighten(s, x, y2);
}

void buttonFrame(Surface &s, int x1, int y1, int x2, int y2, bool raised) {
	fillArea(s, x1, y1, x2, y2, kFrameFill);
	byte topLeft = raised ? 15 : 6, bottomRight = raised ? 6 : 15;
	for (int y = y2; y >= y1; y--)
		s.put(x1, y, topLeft);
	for (int x = x1; x <= x2; x++)
		s.put(x, y1, topLeft);
	for (int x = x1 + 1; x <= x2; x++)
		s.put(x, y2, bottomRight);
	for (int y = y1 + 1; y <= y2; y++)
		s.put(x2, y, bottomRight);
}

void drawPicture(Surface &s, int x, int y, const byte *pixels, int w, int h, bool xorMode) {
	for (int row = 0; row < h; row++) {
		for (int col = 0; col < w; col++) {
			byte c = pixels[(size_t)row * w + col];
			s.put(x + col, y + row, xorMode ? (byte)(s.get(x + col, y + row) ^ c) : c);
		}
	}
}

} // namespace Legend
