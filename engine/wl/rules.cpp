#include "wl/rules.h"

namespace wl {

uint8_t Rules::rngByte(uint8_t al) {
	++rngState[0];
	unsigned sum = unsigned(al) + rngState[0];
	unsigned carry = sum > 0xFF;
	for (int i = 1; i < 5; ++i) {
		sum = (sum & 0xFF) + rngState[i] + carry;
		rngState[i] = uint8_t(sum);
		carry = sum > 0xFF;
	}
	return rngState[4];
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

uint16_t Rules::rollDice() {
	uint8_t total = 0, al = 0;
	for (;;) {
		const uint8_t first = d6(al);
		const uint8_t second = d6(first);
		total = uint8_t(first + second + total);
		if (second != first) return total;
		al = second;
	}
}

void Rules::addTarget(int32_t delta) {
	const int32_t v = int32_t(seed1) + delta;
	seed1 = uint16_t(v < 0 ? 0 : v > 0xFFFF ? 0xFFFF : v);
}

void Rules::addRoll(int32_t delta) {
	const int32_t v = int32_t(seed2) + delta;
	seed2 = uint16_t(v < 0 ? 0 : v > 0xFFFF ? 0xFFFF : v);
}

} // namespace wl
