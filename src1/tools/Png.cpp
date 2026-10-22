#include "tools/Png.h"

#include <vector>

namespace Yendor1 {
namespace Tools {

static uint32_t crcTable[256];
static bool crcInit = false;

static uint32_t crc32Update(uint32_t crc, const uint8_t *p, size_t n) {
	if (!crcInit) {
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t c = i;
			for (int k = 0; k < 8; k++)
				c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
			crcTable[i] = c;
		}
		crcInit = true;
	}
	crc = ~crc;
	for (size_t i = 0; i < n; i++)
		crc = crcTable[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
	return ~crc;
}

static void put32(std::vector<uint8_t> &v, uint32_t x) {
	v.push_back((uint8_t)(x >> 24));
	v.push_back((uint8_t)(x >> 16));
	v.push_back((uint8_t)(x >> 8));
	v.push_back((uint8_t)x);
}

static void chunk(FILE *f, const char *type, const std::vector<uint8_t> &data) {
	std::vector<uint8_t> hdr;
	put32(hdr, (uint32_t)data.size());
	fwrite(&hdr[0], 1, 4, f);
	std::vector<uint8_t> body(type, type + 4);
	body.insert(body.end(), data.begin(), data.end());
	fwrite(&body[0], 1, body.size(), f);
	std::vector<uint8_t> crc;
	put32(crc, crc32Update(0, &body[0], body.size()));
	fwrite(&crc[0], 1, 4, f);
}

bool writePng(const char *path, int width, int height, const uint8_t *rgb) {
	FILE *f = fopen(path, "wb");
	if (!f)
		return false;
	static const uint8_t sig[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
	fwrite(sig, 1, 8, f);

	std::vector<uint8_t> ihdr;
	put32(ihdr, (uint32_t)width);
	put32(ihdr, (uint32_t)height);
	ihdr.push_back(8);  // bit depth
	ihdr.push_back(2);  // truecolour
	ihdr.push_back(0);
	ihdr.push_back(0);
	ihdr.push_back(0);
	chunk(f, "IHDR", ihdr);

	// raw scanlines, each prefixed by filter type 0
	std::vector<uint8_t> raw;
	const size_t rowBytes = (size_t)width * 3;
	for (int y = 0; y < height; y++) {
		raw.push_back(0);
		raw.insert(raw.end(), rgb + y * rowBytes, rgb + (y + 1) * rowBytes);
	}
	// zlib stream of stored blocks
	std::vector<uint8_t> z;
	z.push_back(0x78);
	z.push_back(0x01);
	size_t pos = 0;
	for (;;) {
		size_t n = raw.size() - pos;
		if (n > 65535)
			n = 65535;
		const bool last = pos + n >= raw.size();
		z.push_back(last ? 1 : 0);
		z.push_back((uint8_t)(n & 0xFF));
		z.push_back((uint8_t)(n >> 8));
		z.push_back((uint8_t)(~n & 0xFF));
		z.push_back((uint8_t)((~n >> 8) & 0xFF));
		z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
		pos += n;
		if (last)
			break;
	}
	uint32_t a = 1, b = 0;
	for (size_t i = 0; i < raw.size(); i++) {
		a = (a + raw[i]) % 65521;
		b = (b + a) % 65521;
	}
	put32(z, (b << 16) | a);
	chunk(f, "IDAT", z);
	chunk(f, "IEND", std::vector<uint8_t>());
	fclose(f);
	return true;
}

} // namespace Tools
} // namespace Yendor1
