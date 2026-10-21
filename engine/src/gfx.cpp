#include "fod/gfx.h"
#include "fod/lzss.h"
#include <fstream>

namespace fod {

static const uint8_t kPal6[16][3] = {
	{0, 0, 0},       {0, 0, 0x27},    {3, 0x25, 3},    {0, 0x1B, 0x1B},
	{0x20, 0, 0},    {0x1B, 0, 0x1B}, {0x20, 0x1B, 5}, {0x26, 0x26, 0x26},
	{0x10, 0x10, 0x10}, {0, 0, 0x38}, {0, 0x3F, 0},    {0, 0x3F, 0x3F},
	{0x3A, 0x20, 0x10}, {0x3F, 0, 0x3F}, {0x3F, 0x3F, 0}, {0x3F, 0x3F, 0x3F}};

const Rgb *masterPalette() {
	static Rgb pal[16];
	static bool init = false;
	if (!init) {
		for (int i = 0; i < 16; ++i)
			pal[i] = {uint8_t(kPal6[i][0] * 255 / 63), uint8_t(kPal6[i][1] * 255 / 63), uint8_t(kPal6[i][2] * 255 / 63)};
		init = true;
	}
	return pal;
}

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

Tiles::Tiles(const Bytes &file) : _d(lzssDecode(file)) {}

void Tiles::blit(Image &img, int x, int y, int tile) const {
	if (tile < 0 || size_t(tile) >= count()) return;
	const Rgb *pal = masterPalette();
	const uint8_t *t = &_d[size_t(tile) * 128];
	for (int row = 0; row < 16; ++row)
		for (int col = 0; col < 8; ++col) {
			const uint8_t v = t[row * 8 + col];
			img.set(x + col * 2, y + row, pal[v >> 4]);
			img.set(x + col * 2 + 1, y + row, pal[v & 15]);
		}
}

Font::Font(const Bytes &file) {
	Reader r(file);
	const unsigned n = r.u16();
	_glyphs.resize(size_t(n) * 32);
	r.bytes(_glyphs.data(), _glyphs.size());
}

void Font::blit(Image &img, int x, int y, int ch, Rgb fg) const {
	if (ch < 0 || size_t(ch) >= count()) return;
	const uint8_t *g = &_glyphs[size_t(ch) * 32];
	for (int row = 0; row < 8; ++row)
		for (int col = 0; col < 4; ++col) {
			const uint8_t v = g[row * 4 + col];
			if (v >> 4) img.set(x + col * 2, y + row, fg);
			if (v & 15) img.set(x + col * 2 + 1, y + row, fg);
		}
}

Image renderMap(const Map &map, const Tiles &tiles) {
	Image img(map.width() * 16, map.height() * 16);
	for (int y = 0; y < map.height(); ++y)
		for (int x = 0; x < map.width(); ++x) tiles.blit(img, x * 16, y * 16, map.cell(x, y).tileIndex());
	return img;
}

} // namespace fod
