/* Palettes: FILE00.PAL .. FILE06.PAL.
 *
 * A file is a run of 17-byte records: 16 EGA palette-register values
 * (6-bit rgbRGB) plus a spare byte. Bytes after count*17 are stale. */

#ifndef YENDOR1_DATA_PALETTE_H
#define YENDOR1_DATA_PALETTE_H

#include "shared/Blob.h"
#include "shared/Types.h"

namespace Yendor1 {
namespace Data {

using Shared::byte;

enum {
	kPaletteRecordBytes = 17,
	kPaletteColors = 16
};

struct Rgb {
	byte r, g, b;
};

/** EGA register value (bits: R' G' B' r g b = bit5..bit0) to 8-bit RGB. */
Rgb egaToRgb(byte reg);

class PaletteFile {
public:
	bool load(const char *gameDir, int fileNumber);
	void assign(const byte *data, size_t size);

	/** Number of valid records: the leading run of 17-byte records whose 16 register
	 *  values are <= 3Fh and whose spare byte is 0 (FILE03.PAL is 425 bytes but only the
	 *  first 12 records are real; the rest is stale directory data). */
	size_t count() const { return _count; }
	/** The 16 register values of record `index`, or 0 when out of range. */
	const byte *record(size_t index) const;

private:
	void countRecords();

	Shared::Blob _blob;
	size_t _count;

public:
	PaletteFile() : _count(0) {}
};

} // namespace Data
} // namespace Yendor1

#endif
