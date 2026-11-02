#ifndef ULTIMA6_FILES_TILE_SET_H
#define ULTIMA6_FILES_TILE_SET_H

#include "files/data_file.h"

namespace Ultima6 {

/**
 * The 2048 tiles of Ultima VI (16 x 16 pixels, 256 colours), from four VGA files:
 *
 *   MAPTILES.VGA   LZW stream with the terrain tiles
 *   OBJTILES.VGA   raw, follows MAPTILES in the same address space
 *   TILEINDX.VGA   u16[2048]: the position of each tile, in 16-byte units, in
 *                  MAPTILES followed by OBJTILES
 *   MASKTYPE.VGA   LZW stream; the first 2048 bytes give each tile's kind
 *
 * Kinds: PLAIN is 256 pixels; TRANSPARENT is 256 pixels in which colour 255 shows
 * what is behind; PIXEL_BLOCKS is a byte followed by runs (u16 offset, u8 length,
 * pixels; length 0 ends) whose offsets are screen-line based: an offset is k * 176 + c
 * with c below 16 and means "k rows and c pixels on from the end of the previous run"
 * (a screen line is 320 = 176 + 144 bytes). Pixels no run covers are transparent.
 *
 * The kinds and the pixel-block decoding are those of the MCGA driver's tile blit
 * (U6MCGA.DRV entry 15, sub_2458: kind & 3 = 0 plain, 1 skip colour 255, 2 pixel blocks);
 * all 2048 tiles were also rendered and inspected.
 */
struct TileSet {
	enum { COUNT = 2048, SIZE = 16 * 16, TRANSPARENT_PIXEL = 255 };
	enum Kind { PLAIN = 0x00, TRANSPARENT = 0x05, PIXEL_BLOCKS = 0x0A };

	uint8_t kind[COUNT];
	uint8_t pixels[COUNT][SIZE];

	bool transparent(int tile) const {
		return kind[tile] != PLAIN;
	}
	bool load(const Bytes &mapTiles, const Bytes &maskType, const Bytes &objTiles, const Bytes &tileIndex);
};

/**
 * LOOK.LZD (LZW stream): the descriptions of the tiles. Entries are a u16 tile number
 * and NUL-terminated text; an entry describes the tiles after the previous entry's
 * number up to and including its own. An entry numbered 2048 or more ends the list and
 * tiles after the last entry read "Nothing" (entry 0). Text may contain `\plural` and
 * `/singular` alternatives.
 */
class LookData {
public:
	bool load(const Bytes &decompressed);
	/** Description of a tile (0..2047), with the plural markers left in. */
	const std::string &description(int tile) const {
		return _text[_entryFor[tile]];
	}

private:
	std::vector<std::string> _text;
	int _entryFor[TileSet::COUNT];
};

} // namespace Ultima6

#endif
