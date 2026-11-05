/* XANTH_nn.PIC: a container of pictures.
 *
 * File layout: a directory of 12-byte entries {u32 dataOffset; u8 frames; u8 flags; u16 width; u16 height; u8, u8} ending at the first data offset (offset 0 =
 * unused). An entry's data is `frames` offset pairs {i16 x, i16 y} (the screen position of each frame; 0 pairs = position (0,0)), then, if flags & 0x10, a 768-byte
 * VGA palette (6-bit components), then the pixels: width*height palette indices, PKWARE-DCL compressed in Xanth (stored raw in Shannara).
 * Flags: 0x10 = palette present; 0x20 / 0x40 are picture-kind flags: the original sets the image kind from 0x40 (Image_LoadAlt: `field_1 = 3` if set, else 0), which
 * selects the blit path of Image_Draw; the semantics are documented where the drawing code implements them.
 */
#ifndef LEGEND_PIC_H
#define LEGEND_PIC_H

#include "common/types.h"

namespace Legend {

/** One 6-bit VGA palette entry set (768 bytes): expands a component to 8 bits. */
inline byte palette8(byte v6) {
	v6 &= 63;
	return (byte)(v6 * 4 + (v6 >> 4));
}

struct PicImage {
	int16 x, y;                      // screen position of the (first) frame
	uint16 width, height;
	byte flags;
	uint8 frames;                    // number of position pairs (animation frames); 0 for a plain picture
	std::vector<Common::Point> framePos;
	bool hasPalette;
	byte palette[768];               // 6-bit VGA components (valid if hasPalette)
	Common::Bytes pixels;            // width*height palette indices

	PicImage() : x(0), y(0), width(0), height(0), flags(0), frames(0), hasPalette(false) { memset(palette, 0, sizeof(palette)); }
};

class PicFile {
public:
	struct Entry {
		uint32 offset;               // start of the entry's data (0 = no picture)
		uint32 end;                  // start of the next entry's data (or the file size)
		uint8 frames;
		byte flags;
		uint16 width, height;
	};

	/** Takes ownership of the file's bytes; false if the directory is corrupt. */
	bool load(const Common::Bytes &data, bool compressed = true);
	size_t size() const { return _entries.size(); }
	const Entry &entry(size_t i) const { return _entries[i]; }
	/** Decodes entry i; false for an empty entry or corrupt data. */
	bool decode(size_t i, PicImage &out) const;

private:
	Common::Bytes _d;
	std::vector<Entry> _entries;
	bool _compressed;
};

} // namespace Legend

#endif
