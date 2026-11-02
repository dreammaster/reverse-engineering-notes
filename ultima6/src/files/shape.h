#ifndef ULTIMA6_FILES_SHAPE_H
#define ULTIMA6_FILES_SHAPE_H

#include "files/data_file.h"

namespace Ultima6 {

/**
 * A graphic in Ultima VI's shape format (portraits, intro and ending pictures,
 * cursors, borders):
 *
 *   u16 left, u16 right, u16 up, u16 down   extents around the hot spot
 *   runs, each: u16 n (0 ends), i16 x, i16 y relative to the hot spot, then
 *       n >> 1 pixels (bit 0 of n clear), or
 *       n >> 1 pixels in sub-runs (bit 0 set), each introduced by a byte c:
 *       c >> 1 pixels follow, or with bit 0 of c set one byte repeated c >> 1 times.
 *
 * Pixels no run covers are transparent (255). The format is checked by rendering
 * every shape in the shipped files; the original draws shapes from the graphics
 * drivers, which have not been disassembled yet.
 */
struct Shape {
	enum { TRANSPARENT = 255 };

	int width, height;
	int hotX, hotY;
	Bytes pixels; // width * height, row by row

	bool load(const uint8_t *data, size_t size);
};

/**
 * A *.SHP file: an LZW stream (see lzw.h) holding an archive with a size header
 * (see LibN) of shapes. Absent items are left as empty shapes (width 0).
 */
bool loadShapeFile(const Bytes &fileData, std::vector<Shape> &shapes);

} // namespace Ultima6

#endif
