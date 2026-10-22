#include "wasteland1/exe.h"

namespace wl {

namespace {
uint16_t w16(const Bytes &b, size_t o) { return uint16_t(b.at(o) | (b.at(o + 1) << 8)); }
}

ExeImage loadExePack(const Bytes &f, uint16_t loadSegment) {
	if (f.size() < 0x20 || f[0] != 'M' || f[1] != 'Z') throw std::runtime_error("not an MZ executable");
	const size_t headerSize = size_t(w16(f, 8)) * 16;
	const uint16_t initCs = w16(f, 0x16), initIp = w16(f, 0x14);
	const size_t base = headerSize + size_t(initCs) * 16;           // EXEPACK header (and decompressor stub) segment
	if (w16(f, base + 16) != 0x4252) throw std::runtime_error("EXEPACK signature not found");
	ExeImage out;
	out.entryIp = w16(f, base);
	out.entryCs = uint16_t(w16(f, base + 2) + loadSegment);
	out.initialSp = w16(f, base + 8);
	out.initialSs = uint16_t(w16(f, base + 10) + loadSegment);
	const size_t destLen = size_t(w16(f, base + 12)) * 16;
	out.image.assign(destLen, 0);

	// The compressed data ends at the header; it is decoded backwards. Trailing 0xFF bytes are padding.
	long src = long(base) - 1;
	while (src > long(headerSize) && f[size_t(src)] == 0xFF) --src;
	long dst = long(destLen) - 1;
	for (;;) {
		if (src < long(headerSize) + 3) throw std::runtime_error("EXEPACK data corrupt");
		const uint8_t op = f[size_t(src)];
		const size_t count = f[size_t(src) - 1] << 8 | f[size_t(src) - 2];
		src -= 3;
		const uint8_t kind = op & 0xFE;
		if (kind == 0xB0) {
			const uint8_t fill = f[size_t(src)];
			--src;
			for (size_t i = 0; i < count && dst >= 0; ++i) out.image[size_t(dst--)] = fill;
		} else if (kind == 0xB2) {
			for (size_t i = 0; i < count && dst >= 0 && src >= long(headerSize); ++i) out.image[size_t(dst--)] = f[size_t(src--)];
		} else {
			throw std::runtime_error("EXEPACK bad command");
		}
		if (op & 1) break;
	}

	// relocation table: 16 blocks of {count, count * offset}; block i covers segment i*0x1000
	size_t p = base + 18;
	while (p + 22 <= f.size() && std::memcmp(&f[p], "Packed file is corrupt", 22) != 0) ++p;
	p += 22;
	for (int block = 0; block < 16; ++block) {
		const size_t n = w16(f, p);
		p += 2;
		for (size_t i = 0; i < n; ++i, p += 2) {
			const size_t at = size_t(block) * 0x10000 + w16(f, p);
			if (at + 2 <= out.image.size()) {
				const uint16_t v = uint16_t(w16(out.image, at) + loadSegment);
				out.image[at] = uint8_t(v);
				out.image[at + 1] = uint8_t(v >> 8);
			}
		}
	}
	return out;
}

} // namespace wl
