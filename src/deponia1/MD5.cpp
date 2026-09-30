#include "MD5.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

std::uint32_t leftRotate(std::uint32_t x, std::uint32_t c) {
	return (x << c) | (x >> (32 - c));
}

const std::uint32_t kShift[64] = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
};

// K[i] = floor(abs(sin(i + 1)) * 2^32), per RFC 1321.
const std::uint32_t kSine[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

void processBlock(const unsigned char *block, std::uint32_t &a0, std::uint32_t &b0, std::uint32_t &c0,
                  std::uint32_t &d0) {
	std::uint32_t m[16];
	for (int i = 0; i < 16; i++) {
		m[i] = static_cast<std::uint32_t>(block[i * 4]) | (static_cast<std::uint32_t>(block[i * 4 + 1]) << 8) |
		       (static_cast<std::uint32_t>(block[i * 4 + 2]) << 16) |
		       (static_cast<std::uint32_t>(block[i * 4 + 3]) << 24);
	}

	std::uint32_t a = a0, b = b0, c = c0, d = d0;
	for (int i = 0; i < 64; i++) {
		std::uint32_t f;
		int g;
		if (i < 16) {
			f = (b & c) | (~b & d);
			g = i;
		} else if (i < 32) {
			f = (d & b) | (~d & c);
			g = (5 * i + 1) % 16;
		} else if (i < 48) {
			f = b ^ c ^ d;
			g = (3 * i + 5) % 16;
		} else {
			f = c ^ (b | ~d);
			g = (7 * i) % 16;
		}
		f += a + kSine[i] + m[g];
		a = d;
		d = c;
		c = b;
		b += leftRotate(f, kShift[i]);
	}

	a0 += a;
	b0 += b;
	c0 += c;
	d0 += d;
}

}  // namespace

unsigned char *MD5String(const char *data, unsigned long length) {
	std::uint32_t a0 = 0x67452301;
	std::uint32_t b0 = 0xefcdab89;
	std::uint32_t c0 = 0x98badcfe;
	std::uint32_t d0 = 0x10325476;

	std::uint64_t bitLength = static_cast<std::uint64_t>(length) * 8;
	unsigned long paddedLength = ((length + 8) / 64 + 1) * 64;
	std::vector<unsigned char> padded(paddedLength, 0);
	std::memcpy(padded.data(), data, length);
	padded[length] = 0x80;
	for (int i = 0; i < 8; i++)
		padded[paddedLength - 8 + i] = static_cast<unsigned char>(bitLength >> (8 * i));

	for (unsigned long offset = 0; offset < paddedLength; offset += 64)
		processBlock(padded.data() + offset, a0, b0, c0, d0);

	unsigned char *digest = static_cast<unsigned char *>(std::malloc(16));
	std::uint32_t words[4] = {a0, b0, c0, d0};
	for (int i = 0; i < 4; i++) {
		digest[i * 4] = static_cast<unsigned char>(words[i]);
		digest[i * 4 + 1] = static_cast<unsigned char>(words[i] >> 8);
		digest[i * 4 + 2] = static_cast<unsigned char>(words[i] >> 16);
		digest[i * 4 + 3] = static_cast<unsigned char>(words[i] >> 24);
	}
	return digest;
}
