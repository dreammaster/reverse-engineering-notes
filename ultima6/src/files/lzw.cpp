#include "files/lzw.h"

namespace Ultima6 {

namespace {

enum { CODE_RESET = 0x100, CODE_END = 0x101, FIRST_ENTRY = 0x102, MIN_BITS = 9, MAX_BITS = 12 };

class BitReader {
public:
	BitReader(const uint8_t *data, size_t size) : _data(data), _size(size), _bitPos(0) {}

	bool read(int bits, int &value) {
		if (_bitPos + bits > _size * 8)
			return false;
		value = 0;
		for (int i = 0; i < bits; ++i, ++_bitPos)
			value |= ((_data[_bitPos >> 3] >> (_bitPos & 7)) & 1) << i;
		return true;
	}

private:
	const uint8_t *_data;
	size_t _size;
	size_t _bitPos;
};

} // namespace

bool lzwDecompress(const Bytes &src, size_t offset, Bytes &out) {
	out.clear();
	if (src.size() < offset + 4)
		return false;
	const uint8_t *p = &src[offset];
	size_t size = p[0] | (p[1] << 8) | (p[2] << 16) | ((size_t)p[3] << 24);
	out.reserve(size);

	BitReader in(p + 4, src.size() - offset - 4);
	uint16_t prefix[1 << MAX_BITS];
	uint8_t suffix[1 << MAX_BITS];
	uint8_t stack[1 << MAX_BITS];

	int width = MIN_BITS, next = FIRST_ENTRY, limit = 1 << MIN_BITS;
	int prev = 0, last = 0, code;

	while (in.read(width, code)) {
		if (code == CODE_END)
			break;
		if (code == CODE_RESET) {
			width = MIN_BITS;
			next = FIRST_ENTRY;
			limit = 1 << MIN_BITS;
			if (!in.read(width, prev) || prev > 0xFF)
				return false;
			out.push_back((uint8_t)prev);
			last = prev;
			continue;
		}

		int cur = code, sp = 0;
		if (code >= next) { // the entry about to be defined: previous string + its own first byte
			if (code > next)
				return false;
			stack[sp++] = (uint8_t)last;
			code = prev;
		}
		while (code > 0xFF) {
			if (code < FIRST_ENTRY || code >= next)
				return false;
			stack[sp++] = suffix[code];
			code = prefix[code];
		}
		stack[sp++] = (uint8_t)code;
		int first = code;
		while (sp > 0)
			out.push_back(stack[--sp]);

		if (next < (1 << MAX_BITS)) {
			prefix[next] = (uint16_t)prev;
			suffix[next] = (uint8_t)first;
			++next;
		}
		prev = cur;
		last = first;
		if (next >= limit && width != MAX_BITS) {
			++width;
			limit <<= 1;
		}
		if (out.size() > size)
			return false;
	}
	return out.size() == size;
}

} // namespace Ultima6
