/* The two ROM fonts the driver draws text with (8x8 and 8x14, 256 glyphs, one byte
 * per pixel row, bit 7 = leftmost). The original reads them from the PC BIOS; here
 * they are external data: FONT8X8.BIN and FONT8X14.BIN in a font directory
 * (see src1/tools/fon2raw.py for producing stand-ins for local development). */

#ifndef YENDOR1_VID_FONTS_H
#define YENDOR1_VID_FONTS_H

#include "shared/Blob.h"
#include "vid/Video.h"

namespace Yendor1 {
namespace Vid {

class Fonts {
public:
	/** Load FONT8X8.BIN / FONT8X14.BIN from `dir`. Returns true if both were found. */
	bool load(const char *dir);
	/** Hand the loaded fonts to a video driver (they stay owned by this object). */
	void attach(Video &video) const;
	bool have8x8() const { return _f8.size() == 256 * 8; }
	bool have8x14() const { return _f14.size() == 256 * 14; }

private:
	Shared::Blob _f8;
	Shared::Blob _f14;
};

} // namespace Vid
} // namespace Yendor1

#endif
