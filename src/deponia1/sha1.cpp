#include "sha1.h"

#include <cstdint>
#include <cstring>

static inline std::uint32_t rotateLeft(std::uint32_t value, int bits) {
	return (value << bits) | (value >> (32 - bits));
}

static void processBlock(const unsigned char *block, std::uint32_t *state) {
	std::uint32_t w[80];

	for (int i = 0; i < 16; i++) {
		w[i] = (static_cast<std::uint32_t>(block[4 * i]) << 24) | (static_cast<std::uint32_t>(block[4 * i + 1]) << 16) |
		       (static_cast<std::uint32_t>(block[4 * i + 2]) << 8) | static_cast<std::uint32_t>(block[4 * i + 3]);
	}

	for (int i = 16; i < 80; i++)
		w[i] = rotateLeft(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

	std::uint32_t a = state[0];
	std::uint32_t b = state[1];
	std::uint32_t c = state[2];
	std::uint32_t d = state[3];
	std::uint32_t e = state[4];

	for (int i = 0; i < 80; i++) {
		std::uint32_t f;
		std::uint32_t k;

		if (i < 20) {
			f = (b & c) | (~b & d);
			k = 0x5A827999;
		} else if (i < 40) {
			f = b ^ c ^ d;
			k = 0x6ED9EBA1;
		} else if (i < 60) {
			f = (b & c) | (b & d) | (c & d);
			k = 0x8F1BBCDC;
		} else {
			f = b ^ c ^ d;
			k = 0xCA62C1D6;
		}

		std::uint32_t temp = rotateLeft(a, 5) + f + e + k + w[i];

		e = d;
		d = c;
		c = rotateLeft(b, 30);
		b = a;
		a = temp;
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
}

void sha1::calc(const void *src, int bytelength, unsigned char *hash) {
	std::uint32_t state[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
	const unsigned char *data = static_cast<const unsigned char *>(src);
	std::uint64_t bits = static_cast<std::uint64_t>(bytelength) * 8;
	int offset = 0;

	for (; bytelength - offset >= 64; offset += 64)
		processBlock(data + offset, state);

	unsigned char tail[128];
	int rest = bytelength - offset;

	std::memset(tail, 0, sizeof(tail));
	std::memcpy(tail, data + offset, rest);
	tail[rest] = 0x80;

	int tailLength = (rest < 56) ? 64 : 128;

	for (int i = 0; i < 8; i++)
		tail[tailLength - 1 - i] = static_cast<unsigned char>(bits >> (8 * i));

	for (int i = 0; i < tailLength; i += 64)
		processBlock(tail + i, state);

	for (int i = 0; i < 5; i++) {
		hash[4 * i] = static_cast<unsigned char>(state[i] >> 24);
		hash[4 * i + 1] = static_cast<unsigned char>(state[i] >> 16);
		hash[4 * i + 2] = static_cast<unsigned char>(state[i] >> 8);
		hash[4 * i + 3] = static_cast<unsigned char>(state[i]);
	}
}

void sha1::toHexString(const unsigned char *hash, char *hexstring) {
	static const char kHex[] = "0123456789abcdef";

	for (int i = 0; i < 20; i++) {
		hexstring[2 * i] = kHex[hash[i] >> 4];
		hexstring[2 * i + 1] = kHex[hash[i] & 0xF];
	}

	hexstring[40] = '\0';
}
