#include "fod/lzss.h"

namespace fod {

Bytes lzssDecode(const uint8_t *src, size_t srcSize) {
	Reader in(src, srcSize);
	const uint32_t outSize = in.u32();
	Bytes out;
	out.reserve(outSize);
	uint8_t ring[4096];
	std::memset(ring, 0x20, sizeof(ring));
	unsigned r = 0xFEE;
	unsigned flags = 0;
	int bits = 0;
	while (out.size() < outSize) {
		if (bits == 0) {
			flags = in.u8();
			bits = 8;
		}
		const bool literal = flags & 1;
		flags >>= 1;
		--bits;
		if (literal) {
			const uint8_t b = in.u8();
			out.push_back(b);
			ring[r] = b;
			r = (r + 1) & 0xFFF;
		} else {
			const unsigned lo = in.u8(), hi = in.u8();
			unsigned off = lo | ((hi & 0xF0) << 4);
			for (unsigned n = (hi & 0x0F) + 3; n && out.size() < outSize; --n) {
				const uint8_t b = ring[off];
				off = (off + 1) & 0xFFF;
				out.push_back(b);
				ring[r] = b;
				r = (r + 1) & 0xFFF;
			}
		}
	}
	return out;
}

} // namespace fod
