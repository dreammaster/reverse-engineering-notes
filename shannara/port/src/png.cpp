#include "png.h"

#include <algorithm>
#include <cstdio>

namespace shannara {

static uint32_t crc32(const uint8_t *p, size_t n, uint32_t c = 0) {
	static uint32_t table[256];
	if (!table[1]) {
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t v = i;
			for (int k = 0; k < 8; k++)
				v = v & 1 ? 0xEDB88320u ^ (v >> 1) : v >> 1;
			table[i] = v;
		}
	}
	c = ~c;
	for (size_t i = 0; i < n; i++)
		c = table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
	return ~c;
}

static void be32(Bytes &b, uint32_t v) {
	for (int s = 24; s >= 0; s -= 8)
		b.push_back((uint8_t)(v >> s));
}

static void chunk(FILE *f, const char *type, const Bytes &data) {
	Bytes b;
	be32(b, (uint32_t)data.size());
	fwrite(b.data(), 1, 4, f);
	Bytes td(type, type + 4);
	td.insert(td.end(), data.begin(), data.end());
	fwrite(td.data(), 1, td.size(), f);
	Bytes c;
	be32(c, crc32(td.data(), td.size()));
	fwrite(c.data(), 1, 4, f);
}

void writePng(const std::string &path, const Image &img) {
	Bytes raw;
	for (int y = 0; y < img.h; y++) {
		raw.push_back(0); // filter: none
		raw.insert(raw.end(), img.rgb.begin() + (size_t)y * img.w * 3, img.rgb.begin() + (size_t)(y + 1) * img.w * 3);
	}
	Bytes z = {0x78, 0x01};
	size_t pos = 0;
	uint32_t a = 1, b = 0;
	for (uint8_t v : raw) {
		a = (a + v) % 65521;
		b = (b + a) % 65521;
	}
	while (pos < raw.size() || raw.empty()) {
		size_t n = std::min<size_t>(65535, raw.size() - pos);
		bool last = pos + n >= raw.size();
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
	be32(z, (b << 16) | a);
	FILE *f = fopen(path.c_str(), "wb");
	if (!f)
		throw FormatError("cannot write " + path);
	static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
	fwrite(sig, 1, 8, f);
	Bytes ihdr;
	be32(ihdr, (uint32_t)img.w);
	be32(ihdr, (uint32_t)img.h);
	ihdr.push_back(8);
	ihdr.push_back(2); // RGB
	ihdr.push_back(0);
	ihdr.push_back(0);
	ihdr.push_back(0);
	chunk(f, "IHDR", ihdr);
	chunk(f, "IDAT", z);
	chunk(f, "IEND", {});
	fclose(f);
}

} // namespace shannara
