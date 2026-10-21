#pragma once
// EGA planar video memory (mode 0Dh, 320x200x16) with the VGA write/read pipeline used by WL.EXE.
// The layout of this struct is written verbatim into wlemu state files (the native tests read them back).
#include <cstddef>
#include <cstdint>

namespace wl {

struct Ega {
	uint8_t planes[4][65536] = {};
	uint8_t mapMask = 0xF, seqIdx = 0, gcIdx = 0, gc[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0xFF}, latch[4] = {}, crtcIdx = 0, crtc[32] = {};
	int status = 0;

	uint8_t read(uint32_t addr) {
		const size_t off = (addr - 0xA0000) & 0xFFFF;
		for (int p = 0; p < 4; ++p) latch[p] = planes[p][off];
		return planes[gc[4] & 3][off];
	}

	void write(uint32_t addr, uint8_t v) {
		const size_t off = (addr - 0xA0000) & 0xFFFF;
		const int mode = gc[5] & 3, rot = gc[3] & 7, func = (gc[3] >> 3) & 3;
		const uint8_t bitmask = gc[8], sr = gc[0], esr = gc[1];
		const uint8_t rotated = uint8_t((v >> rot) | (v << (8 - rot)));
		for (int p = 0; p < 4; ++p) {
			if (!(mapMask & (1 << p))) continue;
			uint8_t val = 0, bm = bitmask;
			switch (mode) {
			case 0: val = rotated; if (esr & (1 << p)) val = (sr & (1 << p)) ? 0xFF : 0; break;
			case 1: planes[p][off] = latch[p]; continue;
			case 2: val = (v & (1 << p)) ? 0xFF : 0; break;
			default: val = (sr & (1 << p)) ? 0xFF : 0; bm = uint8_t(bitmask & rotated); break;
			}
			switch (func) {
			case 1: val &= latch[p]; break;
			case 2: val |= latch[p]; break;
			case 3: val ^= latch[p]; break;
			default: break;
			}
			planes[p][off] = uint8_t((val & bm) | (latch[p] & ~bm));
		}
	}

	// the 4-bit colour of pixel (x, y) of the visible page
	int pixel(int x, int y) const {
		const size_t start = (size_t(crtc[0xC]) << 8) | crtc[0xD];
		const size_t off = (start + size_t(y) * 40 + size_t(x >> 3)) & 0xFFFF;
		int col = 0;
		for (int p = 0; p < 4; ++p) col |= ((planes[p][off] >> (7 - (x & 7))) & 1) << p;
		return col;
	}
};

} // namespace wl
