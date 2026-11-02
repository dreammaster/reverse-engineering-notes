#include "engine/framebuffer.h"

#include <cstdio>

namespace Ultima6 {

void Framebuffer::setPalette(const uint8_t rgb6[256][3]) {
	for (int i = 0; i < 256; ++i)
		for (int c = 0; c < 3; ++c)
			_palette[i * 3 + c] = rgb6[i][c] & 63;
}

void Framebuffer::colour(int index, int &r, int &g, int &b) const {
	// 6-bit VGA component to 8 bits
	auto expand = [](int v) { return (v << 2) | (v >> 4); };
	r = expand(_palette[index * 3]);
	g = expand(_palette[index * 3 + 1]);
	b = expand(_palette[index * 3 + 2]);
}

void Framebuffer::fillRect(int x, int y, int w, int h, uint8_t value) {
	for (int j = 0; j < h; ++j) {
		int yy = y + j;
		if (yy < 0 || yy >= HEIGHT)
			continue;
		for (int i = 0; i < w; ++i) {
			int xx = x + i;
			if (xx >= 0 && xx < WIDTH)
				_pixels[yy * WIDTH + xx] = value;
		}
	}
}

void Framebuffer::blit(int x, int y, const uint8_t *src, int w, int h, int srcPitch, int transparent) {
	for (int j = 0; j < h; ++j) {
		int yy = y + j;
		if (yy < 0 || yy >= HEIGHT)
			continue;
		for (int i = 0; i < w; ++i) {
			int xx = x + i;
			if (xx < 0 || xx >= WIDTH)
				continue;
			uint8_t v = src[j * srcPitch + i];
			if (v != transparent || transparent < 0)
				_pixels[yy * WIDTH + xx] = v;
		}
	}
}

namespace {

uint32_t crcTable[256];
bool crcReady = false;

uint32_t crc32(uint32_t crc, const uint8_t *p, size_t n) {
	if (!crcReady) {
		for (uint32_t i = 0; i < 256; ++i) {
			uint32_t c = i;
			for (int k = 0; k < 8; ++k)
				c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
			crcTable[i] = c;
		}
		crcReady = true;
	}
	crc = ~crc;
	while (n--)
		crc = crcTable[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	return ~crc;
}

void put32(std::vector<uint8_t> &v, uint32_t x) {
	v.push_back(x >> 24);
	v.push_back(x >> 16);
	v.push_back(x >> 8);
	v.push_back(x);
}

void chunk(FILE *f, const char *type, const std::vector<uint8_t> &data) {
	std::vector<uint8_t> out;
	put32(out, (uint32_t)data.size());
	std::vector<uint8_t> body(type, type + 4);
	body.insert(body.end(), data.begin(), data.end());
	out.insert(out.end(), body.begin(), body.end());
	put32(out, crc32(0, &body[0], body.size()));
	std::fwrite(&out[0], 1, out.size(), f);
}

} // namespace

bool Framebuffer::savePng(const std::string &path, int scale) const {
	FILE *f = std::fopen(path.c_str(), "wb");
	if (!f)
		return false;
	static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
	std::fwrite(sig, 1, 8, f);

	int w = WIDTH * scale, h = HEIGHT * scale;
	std::vector<uint8_t> ihdr;
	put32(ihdr, w);
	put32(ihdr, h);
	ihdr.push_back(8); // bit depth
	ihdr.push_back(2); // RGB
	ihdr.push_back(0);
	ihdr.push_back(0);
	ihdr.push_back(0);
	chunk(f, "IHDR", ihdr);

	// raw scan lines, each preceded by filter byte 0
	std::vector<uint8_t> raw;
	raw.reserve((size_t)h * (w * 3 + 1));
	for (int y = 0; y < h; ++y) {
		raw.push_back(0);
		for (int x = 0; x < w; ++x) {
			int r, g, b;
			colour(_pixels[(y / scale) * WIDTH + x / scale], r, g, b);
			raw.push_back(r);
			raw.push_back(g);
			raw.push_back(b);
		}
	}
	// zlib stream of stored blocks
	std::vector<uint8_t> z;
	z.push_back(0x78);
	z.push_back(0x01);
	size_t pos = 0;
	while (pos < raw.size()) {
		size_t n = raw.size() - pos;
		if (n > 65535)
			n = 65535;
		z.push_back(pos + n == raw.size() ? 1 : 0);
		z.push_back(n & 0xFF);
		z.push_back(n >> 8);
		z.push_back(~n & 0xFF);
		z.push_back((~n >> 8) & 0xFF);
		z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
		pos += n;
	}
	uint32_t a = 1, b = 0;
	for (uint8_t c : raw) {
		a = (a + c) % 65521;
		b = (b + a) % 65521;
	}
	put32(z, (b << 16) | a);
	chunk(f, "IDAT", z);
	chunk(f, "IEND", std::vector<uint8_t>());
	std::fclose(f);
	return true;
}

} // namespace Ultima6
