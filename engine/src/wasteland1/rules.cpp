#include "wasteland1/rules.h"

namespace wl {

uint8_t Rules::rngByte(uint8_t al) {
	uint8_t &s0 = _m.d8(0x465C);
	++s0;
	unsigned sum = unsigned(al) + s0;
	unsigned carry = sum > 0xFF;
	for (uint16_t i = 1; i < 5; ++i) {
		sum = (sum & 0xFF) + _m.d8(uint16_t(0x465C + i)) + carry;
		_m.d8(uint16_t(0x465C + i)) = uint8_t(sum);
		carry = sum > 0xFF;
	}
	return _m.d8(0x4660);
}

uint8_t Rules::random(uint8_t n) {
	if (n <= 1) return n;
	unsigned mask = 0;
	for (unsigned a = n; a != 0; a >>= 1) mask = (mask << 1) | 1;
	uint8_t al = 0;
	for (;;) {
		al = rngByte(al);
		al = uint8_t(al & mask);
		if (al < n) break;
	}
	return uint8_t(al + 1);
}

uint8_t Rules::d6(uint8_t al) {
	for (;;) {
		al = rngByte(al);
		al &= 7;
		if (al < 6) return uint8_t(al + 1);
	}
}

uint8_t Rules::rollDice() {
	uint8_t total = 0, al = 0;
	for (;;) {
		const uint8_t first = d6(al);
		const uint8_t second = d6(first);
		total = uint8_t(first + second + total);
		if (second != first) return total;
		al = second;
	}
}

uint16_t Rules::sumOfD6(uint8_t base, uint8_t count, uint8_t &al) {
	uint8_t total = base, hi = 0;
	al = base;
	while (count--) {
		const unsigned sum = unsigned(d6(al)) + total;
		al = uint8_t(sum);
		total = al;
		if (sum > 0xFF && ++hi == 0) { hi = 0xFF; total = 0xFF; }
	}
	return uint16_t((hi << 8) | total);
}

void Rules::addTarget(int32_t delta) {
	const int32_t v = int32_t(seed1()) + delta;
	setSeed1(uint16_t(v < 0 ? 0 : v > 0xFFFF ? 0xFFFF : v));
}

void Rules::addRoll(int32_t delta) {
	const int32_t v = int32_t(seed2()) + delta;
	setSeed2(uint16_t(v < 0 ? 0 : v > 0xFFFF ? 0xFFFF : v));
}

} // namespace wl
