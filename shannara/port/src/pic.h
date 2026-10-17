// SHANxxx.PIC picture containers (verified on the real retail files, see analysis section 12/22).
//   directory: 12-byte entries {u32 offset (0 = end), u8 frames, u8 flags, u16 w, u16 h, u8 b10, u8 b11}
//   data at offset: [frames * {s16 x, s16 y}] [768-byte palette if flags & 0x10] [w*h*max(1,frames) bytes of 8-bpp pixels]
//   flags: 0x10 palette present, 0x40 XOR sprite (index 0 = unchanged), 0x20 transparent index 254 (unvalidated)
#pragma once
#include "common.h"

namespace shannara {

struct PicEntry {
	int index = 0;
	uint32_t offset = 0, end = 0;
	uint8_t frames = 0, flags = 0, b10 = 0, b11 = 0;
	uint16_t w = 0, h = 0;
	bool hasPalette() const { return flags & 0x10; }
	bool isXorSprite() const { return flags & 0x40; }
};

struct PicImage {
	bool hasPalette = false;
	uint8_t palette[768] = {}; // 6-bit VGA components as stored; scale with <<2 if 8-bit output is wanted
	struct Frame { int16_t x, y; };
	std::vector<Frame> offsets;
	Bytes pixels; // w*h bytes per frame, frames stored back to back
};

class PicFile {
public:
	explicit PicFile(Bytes data);
	const std::vector<PicEntry> &entries() const { return _entries; }
	// Frame count 0 means one static image. The payload must have exactly w*h*max(1,frames) bytes.
	PicImage load(int entry) const;
	// Flag 0x40 sprites: destination ^= sprite pixel inside the w x h rectangle at (sx, sy).
	static void compositeXor(Bytes &bg, int bgWidth, const uint8_t *spr, int sx, int sy, int sw, int sh);

private:
	Bytes _data;
	std::vector<PicEntry> _entries;
};

} // namespace shannara
