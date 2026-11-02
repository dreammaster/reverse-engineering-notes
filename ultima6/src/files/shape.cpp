#include "files/shape.h"

#include "files/lib_n.h"
#include "files/lzw.h"

#include <cstring>

namespace Ultima6 {

namespace {

class Reader {
public:
	Reader(const uint8_t *data, size_t size) : _p(data), _end(data + size) {}

	bool u16(int &v) {
		if (_end - _p < 2)
			return false;
		v = _p[0] | (_p[1] << 8);
		_p += 2;
		return true;
	}
	bool s16(int &v) {
		if (!u16(v))
			return false;
		if (v >= 0x8000)
			v -= 0x10000;
		return true;
	}
	bool u8(int &v) {
		if (_end - _p < 1)
			return false;
		v = *_p++;
		return true;
	}
	const uint8_t *take(size_t n) {
		if ((size_t)(_end - _p) < n)
			return nullptr;
		const uint8_t *r = _p;
		_p += n;
		return r;
	}

private:
	const uint8_t *_p, *_end;
};

} // namespace

bool Shape::load(const uint8_t *data, size_t size) {
	Reader in(data, size);
	int a, c, d;
	if (!in.u16(a) || !in.u16(hotX) || !in.u16(c) || !in.u16(d))
		return false;
	// Words: x extent to the left/right of the hot spot, hot spot x, hot spot y, y extent.
	hotY = c;
	width = a + hotX + 1;
	height = hotY + d + 1;
	if (width <= 0 || height <= 0 || width > 1024 || height > 1024)
		return false;
	pixels.assign((size_t)width * height, TRANSPARENT);

	for (;;) {
		int n, x, y;
		if (!in.u16(n))
			return false;
		if (n == 0)
			return true;
		if (!in.s16(x) || !in.s16(y))
			return false;
		int px = hotX + x, py = hotY + y;
		if (px >= width || py >= height)
			return true; // the original stops at a run that starts outside the shape
		if (px < 0 || py < 0)
			return false;
		uint8_t *dst = &pixels[(size_t)py * width + px];
		size_t room = pixels.size() - ((size_t)py * width + px);

		bool encoded = n & 1;
		n >>= 1;
		if (!encoded) {
			const uint8_t *src = in.take(n);
			if (!src || (size_t)n > room)
				return false;
			std::memcpy(dst, src, n);
			continue;
		}
		for (int j = 0; j < n;) {
			int c;
			if (!in.u8(c))
				return false;
			bool repeat = c & 1;
			c >>= 1;
			if ((size_t)(j + c) > room)
				return false;
			if (repeat) {
				int value;
				if (!in.u8(value))
					return false;
				std::memset(dst + j, value, c);
			} else {
				const uint8_t *src = in.take(c);
				if (!src)
					return false;
				std::memcpy(dst + j, src, c);
			}
			j += c;
		}
	}
}

bool loadShapeFile(const Bytes &fileData, std::vector<Shape> &shapes) {
	shapes.clear();
	Bytes plain;
	if (!lzwDecompress(fileData, 0, plain))
		return false;
	LibN lib;
	if (!lib.load(plain, true))
		return false;
	shapes.resize(lib.count());
	for (size_t i = 0; i < lib.count(); ++i) {
		shapes[i].width = shapes[i].height = shapes[i].hotX = shapes[i].hotY = 0;
		size_t size;
		const uint8_t *p = lib.item(i, size);
		if (p && !shapes[i].load(p, size))
			return false;
	}
	return true;
}

} // namespace Ultima6
