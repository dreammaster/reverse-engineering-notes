#include "vid/Trace.h"

#include <stdio.h>

namespace Yendor1 {
namespace Vid {

namespace {

uint32_t fnv(const byte *p, size_t n) {
	uint32_t h = 2166136261u;
	for (size_t i = 0; i < n; i++) {
		h ^= p[i];
		h *= 16777619u;
	}
	return h;
}

} // namespace

std::string formatCall(const Video &v, int ah, int al) {
	const Context &c = v.ctx;
	uint32_t hash = 0;
	const ImagePtr &img = c.image;
	if (ah == 0x0A) {
		const uint32_t n = (uint32_t)c.imgRows * c.imgBytes * 4;
		if (n < 0x10000 && img.data && img.offset + n <= img.size)
			hash = fnv(img.data + img.offset, n);
	} else if (ah == 0x0C && img.data && img.offset + 5 <= img.size) {
		const byte *h = img.data + img.offset;
		const uint32_t rows = (uint32_t)(h[0] | (h[1] << 8)), width = (uint32_t)(h[2] | (h[3] << 8));
		const uint32_t n = rows * width * 4;
		if (n < 0x10000 && img.offset + 5 + n <= img.size)
			hash = fnv(h + 5, n);
	}
	char buf[320];
	snprintf(buf, sizeof(buf), "%02X %02X x1=%u y1=%u x2=%u y2=%u x3=%u y3=%u col=%u pg=%u dst=%u key=%u rows=%u w=%u mask=%02X font=%u tx=%u ty=%u ch=%u fg=%u bg=%u img=%08X",
		ah, al, c.x1, c.y1, c.x2, c.y2, c.x3, c.y3, c.colour, c.drawPage, c.destPage, c.keyColour, c.imgRows, c.imgBytes, c.imgMask,
		c.fontSelect, c.textCol, c.textRow, c.textChar, c.textFg, c.textBg, (unsigned)hash);
	return buf;
}

} // namespace Vid
} // namespace Yendor1
