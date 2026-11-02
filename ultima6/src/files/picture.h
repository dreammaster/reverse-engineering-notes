#ifndef ULTIMA6_FILES_PICTURE_H
#define ULTIMA6_FILES_PICTURE_H

#include "files/data_file.h"

namespace Ultima6 {

/**
 * The game's *.BMP files (PAPER.BMP, NEWMAGIC.BMP, WORLDMAP.BMP) are not Windows bitmaps: they are
 * LZW streams (see lzw.h) holding a u16 width, a u16 height and the 8-bit pixels, row by row, in
 * the game's palette. PAPER.BMP is the whole 320 x 200 play screen's frame, NEWMAGIC.BMP (144 x 82)
 * the spell book's page and WORLDMAP.BMP (128 x 128) the overview map.
 */
struct Picture {
	int width = 0, height = 0;
	Bytes pixels;

	bool load(const std::string &path);
};

} // namespace Ultima6

#endif
