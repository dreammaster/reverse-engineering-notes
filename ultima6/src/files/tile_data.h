#ifndef ULTIMA6_FILES_TILE_DATA_H
#define ULTIMA6_FILES_TILE_DATA_H

#include "files/data_file.h"

namespace Ultima6 {

/**
 * ANIMDATA (194 bytes), as read by GAME.EXE's load_tile_attribs: a count, then
 * four parallel arrays of 32 entries.
 */
struct AnimData {
	enum { MAX = 32, FILE_SIZE = 2 + MAX * 2 + MAX * 2 + MAX + MAX };

	uint16_t count;
	uint16_t tileToAnimate[MAX];
	uint16_t firstFrame[MAX];
	uint8_t andMasks[MAX];
	uint8_t shiftValues[MAX];

	bool load(const Bytes &data);
};

/**
 * TILEFLAG (7168 bytes), read by load_tile_attribs into four buffers, one per tile
 * (2048 tiles) except the third, which has 1024 entries: the weight of each object type
 * (found through the conversation weight operator).
 */
struct TileFlags {
	enum { TILES = 2048, BLOCK3 = 1024, FILE_SIZE = TILES + TILES + BLOCK3 + TILES };

	uint8_t flags1[TILES];  // +0x0000
	uint8_t flags2[TILES];  // +0x0800
	uint8_t block3[BLOCK3]; // +0x1000, weight per object type
	uint8_t flags3[TILES];  // +0x1400, top two bits = article

	bool load(const Bytes &data);
};

/**
 * BASETILE (2048 bytes): the first tile of every object type, indexed by the
 * 10-bit object type. A tile of an object is basetile[type] + frame.
 */
struct BaseTile {
	enum { TYPES = 1024, FILE_SIZE = TYPES * 2 };

	uint16_t firstTile[TYPES];

	bool load(const Bytes &data);
};

/**
 * U6PAL: the VGA palette (256 RGB triples, 6 bits per component). The game reads
 * only the first 0x300 bytes of the 1024-byte file.
 */
struct Palette {
	enum { COLORS = 256, USED_SIZE = COLORS * 3 };

	uint8_t rgb[COLORS][3];

	bool load(const Bytes &data);
};

/**
 * U6.CH (2048 bytes): the text font, 256 glyphs of 8 rows, one byte per row with the
 * leftmost pixel in the most significant bit.
 */
struct Font {
	enum { GLYPHS = 256, ROWS = 8, FILE_SIZE = GLYPHS * ROWS };

	uint8_t glyph[GLYPHS][ROWS];

	bool load(const Bytes &data);

	/**
	 * The proportional fonts of Martian Dreams (FONTS.LZC) and Savage Empire (SAVAGE.FNT): byte 0 the height, byte 2 the value of a pixel that is set, the widths at +4, the low and high
	 * bytes of each glyph's offset at +0x104 and +0x204, and there width * height pixel bytes (see Nuvie's wou_font.cpp). The 8 x 8 `glyph` bitmaps stay unused then.
	 */
	bool loadWou(const Bytes &data);
	bool wou = false;
	int wouHeight = 8;
	uint8_t wouWidth[GLYPHS] = {};
	Bytes wouPixels[GLYPHS]; // width * height bytes, non-zero where the glyph has a pixel

	int width(uint8_t c) const {
		return wou ? wouWidth[c] : 8;
	}
	int height() const {
		return wou ? wouHeight : 8;
	}
};

/**
 * PALETTES.INT (4608 bytes): eight palettes of 576 bytes for the introduction and character
 * creation. U.EXE's routine at 1BFE2 loads one by number (offset n * 576) and unpacks it
 * from the end: every 3 bytes b0 b1 b2 hold four 6-bit components,
 *   b0 & 3F,  (b1 << 2 & 3C) + (b0 >> 6),  (b2 << 4 & 30) + (b1 >> 4),  b2 >> 2,
 * so 192 groups make the 256 colours of a VGA palette.
 */
struct IntroPalettes {
	enum { COUNT = 8, PACKED_SIZE = 576, FILE_SIZE = COUNT * PACKED_SIZE };

	Palette palette[COUNT];

	bool load(const Bytes &data);
};

} // namespace Ultima6

#endif
