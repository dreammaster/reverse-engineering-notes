#pragma once
#include "shared/bytes.h"

namespace shared {

struct Rgb { uint8_t r, g, b; };


// Linear RGB image with a PNG writer (used by the tools of both games).
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

} // namespace shared
