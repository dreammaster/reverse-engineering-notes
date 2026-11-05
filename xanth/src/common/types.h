/* Basic types and helpers shared by all code. C++11 only, no exceptions (see ../README.md).
 *
 * ScummVM port: these map to common/scummsys.h (byte, uint16, ... and ARRAYSIZE); this header is the only place that defines them.
 */
#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

typedef uint8_t byte;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) ((int)(sizeof(a) / sizeof((a)[0])))
#endif

namespace Common {

/** A block of bytes, e.g. a whole data file. */
typedef std::vector<byte> Bytes;

/** Minimal rectangle with exclusive right/bottom edges (like Common::Rect). */
struct Rect {
	int16 left, top, right, bottom;
	Rect() : left(0), top(0), right(0), bottom(0) {}
	Rect(int16 l, int16 t, int16 r, int16 b) : left(l), top(t), right(r), bottom(b) {}
	int16 width() const { return (int16)(right - left); }
	int16 height() const { return (int16)(bottom - top); }
	bool isEmpty() const { return left >= right || top >= bottom; }
	bool contains(int x, int y) const { return x >= left && x < right && y >= top && y < bottom; }
};

struct Point {
	int16 x, y;
	Point() : x(0), y(0) {}
	Point(int16 px, int16 py) : x(px), y(py) {}
};

} // namespace Common

#endif
