#include "shared/image.h"
#include <algorithm>
#include <fstream>

namespace shared {

// ---- minimal PNG writer (stored deflate blocks, no external dependency) ----
static uint32_t crc32(const uint8_t *d, size_t n, uint32_t crc = 0) {
	static uint32_t table[256];
	static bool init = false;
	if (!init) {
		for (uint32_t i = 0; i < 256; ++i) {
			uint32_t c = i;
			for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
			table[i] = c;
		}
		init = true;
	}
	crc = ~crc;
	for (size_t i = 0; i < n; ++i) crc = table[(crc ^ d[i]) & 0xFF] ^ (crc >> 8);
	return ~crc;
}

static void be32(Bytes &b, uint32_t v) { for (int s = 24; s >= 0; s -= 8) b.push_back(uint8_t(v >> s)); }

static void pngChunk(std::ofstream &f, const char *type, const Bytes &data) {
	Bytes buf;
	be32(buf, uint32_t(data.size()));
	buf.insert(buf.end(), type, type + 4);
	buf.insert(buf.end(), data.begin(), data.end());
	const uint32_t c = crc32(buf.data() + 4, buf.size() - 4);
	be32(buf, c);
	f.write(reinterpret_cast<const char *>(buf.data()), std::streamsize(buf.size()));
}

void Image::writePng(const std::string &path) const {
	Bytes raw;
	for (int y = 0; y < _h; ++y) {
		raw.push_back(0);
		raw.insert(raw.end(), _px.begin() + size_t(y) * _w * 3, _px.begin() + size_t(y + 1) * _w * 3);
	}
	Bytes z = {0x78, 0x01};
	uint32_t a = 1, b = 0;
	for (size_t pos = 0; pos < raw.size();) {
		const size_t n = std::min<size_t>(65535, raw.size() - pos);
		z.push_back(pos + n == raw.size());
		z.push_back(uint8_t(n)); z.push_back(uint8_t(n >> 8));
		z.push_back(uint8_t(~n)); z.push_back(uint8_t(~n >> 8));
		z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
		for (size_t i = 0; i < n; ++i) { a = (a + raw[pos + i]) % 65521; b = (b + a) % 65521; }
		pos += n;
	}
	be32(z, (b << 16) | a);
	std::ofstream f(path, std::ios::binary);
	if (!f) throw std::runtime_error("cannot write " + path);
	f.write("\x89PNG\r\n\x1a\n", 8);
	Bytes ihdr;
	be32(ihdr, uint32_t(_w)); be32(ihdr, uint32_t(_h));
	ihdr.push_back(8); ihdr.push_back(2); ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0);
	pngChunk(f, "IHDR", ihdr);
	pngChunk(f, "IDAT", z);
	pngChunk(f, "IEND", {});
}

} // namespace shared
