#include "legend/surface.h"

#include <cstdio>

#include "legend/pic.h"

namespace Legend {

Surface::Surface(int width, int height) : _w(width), _h(height), _pix((size_t)width * height, 0) {
	memset(palette, 0, sizeof(palette));
}

void Surface::clear(byte color) {
	memset(&_pix[0], color, _pix.size());
}

void Surface::fillRect(const Common::Rect &r, byte color) {
	int x0 = r.left < 0 ? 0 : r.left, y0 = r.top < 0 ? 0 : r.top;
	int x1 = r.right > _w ? _w : r.right, y1 = r.bottom > _h ? _h : r.bottom;
	for (int y = y0; y < y1; y++)
		for (int x = x0; x < x1; x++)
			_pix[(size_t)y * _w + x] = color;
}

void Surface::frameRect(const Common::Rect &r, byte color) {
	for (int x = r.left; x < r.right; x++) {
		put(x, r.top, color);
		put(x, r.bottom - 1, color);
	}
	for (int y = r.top; y < r.bottom; y++) {
		put(r.left, y, color);
		put(r.right - 1, y, color);
	}
}

void Surface::blit(int x, int y, const byte *src, int sw, int sh, int transparent) {
	for (int row = 0; row < sh; row++) {
		int dy = y + row;
		if (dy < 0 || dy >= _h)
			continue;
		for (int col = 0; col < sw; col++) {
			int dx = x + col;
			if (dx < 0 || dx >= _w)
				continue;
			byte c = src[(size_t)row * sw + col];
			if (transparent < 0 || c != (byte)transparent)
				_pix[(size_t)dy * _w + dx] = c;
		}
	}
}

void Surface::copyFrom(const Surface &src, const Common::Rect &r, int x, int y) {
	for (int row = r.top; row < r.bottom; row++)
		for (int col = r.left; col < r.right; col++)
			put(x + col - r.left, y + row - r.top, src.get(col, row));
}

void Surface::setPalette(const byte *vga768) {
	memcpy(palette, vga768, 768);
}

void Surface::toRgb(byte *rgb) const {
	for (size_t i = 0; i < _pix.size(); i++) {
		const byte *p = &palette[_pix[i] * 3];
		rgb[i * 3] = palette8(p[0]);
		rgb[i * 3 + 1] = palette8(p[1]);
		rgb[i * 3 + 2] = palette8(p[2]);
	}
}

bool Surface::savePpm(const std::string &path) const {
	FILE *f = fopen(path.c_str(), "wb");
	if (!f)
		return false;
	fprintf(f, "P6\n%d %d\n255\n", _w, _h);
	std::vector<byte> rgb(_pix.size() * 3);
	toRgb(&rgb[0]);
	fwrite(&rgb[0], 1, rgb.size(), f);
	fclose(f);
	return true;
}

} // namespace Legend
