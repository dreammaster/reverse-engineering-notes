/* Tile graphics: FILE00.16 .. FILE06.16.
 *
 * Each file is an array of 512-byte tiles. A tile is 32x32 pixels, 16 colours,
 * stored as 4 bit planes in the order 3,2,1,0, each plane 32 rows of 4 bytes,
 * bit 7 = leftmost pixel. See docs1/file-formats.md. */

#ifndef YENDOR1_DATA_TILES_H
#define YENDOR1_DATA_TILES_H

#include "shared/Blob.h"
#include "shared/Types.h"

namespace Yendor1 {
namespace Data {

using Shared::byte;

enum {
	kTileWidth = 32,
	kTileHeight = 32,
	kTileBytes = 512,
	kTilePlaneBytes = 128,
	kTileFileCount = 7
};

class TileFile {
public:
	/** Load `FILE0<n>.16` from the game directory. */
	bool load(const char *gameDir, int fileNumber);
	/** Use an already loaded image (tests). */
	void assign(const byte *data, size_t size);

	size_t count() const { return _blob.size() / kTileBytes; }
	/** The 512 raw bytes of tile `index`, or 0 when out of range. */
	const byte *raw(size_t index) const;

	/** Decode tile `index` to 32x32 colour indices (0-15), row-major. */
	bool decode(size_t index, byte *out1024) const;

	/** Decode raw tile bytes (static form, used by the video layer). */
	static void decodeRaw(const byte *raw512, byte *out1024);

private:
	Shared::Blob _blob;
};

} // namespace Data
} // namespace Yendor1

#endif
