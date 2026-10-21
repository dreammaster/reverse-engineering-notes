#include "wl/graphics.h"

namespace wl {

MsqHeaderInfo readMsqHeader(const Bytes &f, size_t pos) {
	MsqHeaderInfo h;
	if (pos + 8 > f.size()) return h;
	h.size = f[pos] | (f[pos + 1] << 8) | (f[pos + 2] << 16) | (size_t(f[pos + 3]) << 24);
	if (std::memcmp(&f[pos + 4], "msq", 3) != 0) return h;
	h.ok = true; h.compressed = true; h.disk = f[pos + 7]; h.next = pos + 8;
	return h;
}

const uint32_t *egaPalette() {
	static const uint32_t pal[16] = {0xff000000, 0xff0000aa, 0xff00aa00, 0xff00aaaa, 0xffaa0000, 0xffaa00aa, 0xffaa5500, 0xffaaaaaa,
	                                 0xff555555, 0xff5555ff, 0xff55ff55, 0xff55ffff, 0xffff5555, 0xffff55ff, 0xffffff55, 0xffffffff};
	return pal;
}

namespace {

// Decodes a Pic from huffman-decoded bytes using the vertical XOR filter (each row is XORed with the previous one).
Pic readPic(const HuffmanTree &t, BitReader &r, int w, int h) {
	Pic p;
	p.width = w; p.height = h; p.px.resize(size_t(w) * size_t(h));
	std::vector<int> last(size_t(w / 2), 0);
	for (int y = 0; y < h; ++y)
		for (int x = 0; x < w; x += 2) {
			int b = t.decode(r);
			if (b < 0) throw std::runtime_error("pic data truncated");
			if (y > 0) b ^= last[size_t(x / 2)];
			last[size_t(x / 2)] = b;
			p.px[size_t(y) * size_t(w) + size_t(x)] = uint8_t(b >> 4);
			p.px[size_t(y) * size_t(w) + size_t(x) + 1] = uint8_t(b & 15);
		}
	return p;
}

} // namespace

std::vector<std::vector<Pic>> loadTilesets(const Bytes &f, int w, int h) {
	std::vector<std::vector<Pic>> sets;
	size_t pos = 0;
	for (;;) {
		const MsqHeaderInfo hd = readMsqHeader(f, pos);
		if (!hd.ok) break;
		BitReader r(f.data(), f.size(), hd.next);
		HuffmanTree t(r);
		const size_t quantity = hd.size * 2 / size_t(w) / size_t(h);
		std::vector<Pic> tiles;
		for (size_t i = 0; i < quantity; ++i) tiles.push_back(readPic(t, r, w, h));
		sets.push_back(std::move(tiles));
		pos = r.bytePos();
	}
	return sets;
}

std::vector<Pic> loadSprites(const Bytes &f) {
	std::vector<Pic> out;
	for (size_t base = 0; base + 128 <= f.size(); base += 128) {
		Pic p;
		p.width = 16; p.height = 16; p.px.assign(256, 0);
		for (int plane = 0; plane < 4; ++plane)
			for (int y = 0; y < 16; ++y)
				for (int bx = 0; bx < 2; ++bx) {
					const uint8_t b = f[base + size_t(plane) * 32 + size_t(y) * 2 + size_t(bx)];
					for (int i = 0; i < 8; ++i) p.px[size_t(y) * 16 + size_t(bx * 8 + i)] |= uint8_t(((b >> (7 - i)) & 1) << plane);
				}
		out.push_back(std::move(p));
	}
	return out;
}

std::vector<Bytes> loadMasks(const Bytes &f) {
	std::vector<Bytes> out;
	for (size_t base = 0; base + 32 <= f.size(); base += 32) {
		Bytes m(256, 0);
		for (int y = 0; y < 16; ++y)
			for (int bx = 0; bx < 2; ++bx) {
				const uint8_t b = f[base + size_t(y) * 2 + size_t(bx)];
				for (int i = 0; i < 8; ++i) m[size_t(y) * 16 + size_t(bx * 8 + i)] = (b >> (7 - i)) & 1;
			}
		out.push_back(std::move(m));
	}
	return out;
}

std::vector<Pic> loadFont(const Bytes &f) {
	std::vector<Pic> out;
	for (size_t base = 0; base + 32 <= f.size(); base += 32) {
		Pic p;
		p.width = 8; p.height = 8; p.px.assign(64, 0);
		for (int plane = 0; plane < 4; ++plane)
			for (int y = 0; y < 8; ++y) {
				const uint8_t b = f[base + size_t(plane) * 8 + size_t(y)];
				for (int i = 0; i < 8; ++i) p.px[size_t(y) * 8 + size_t(i)] |= uint8_t(((b >> (7 - i)) & 1) << plane);
			}
		out.push_back(std::move(p));
	}
	return out;
}

std::vector<PicAnimation> loadPics(const Bytes &f, int width) {
	std::vector<PicAnimation> pics;
	size_t pos = 0;
	for (;;) {
		MsqHeaderInfo hd = readMsqHeader(f, pos);
		if (!hd.ok) break;
		PicAnimation a;
		{
			BitReader r(f.data(), f.size(), hd.next);
			HuffmanTree t(r);
			a.base = readPic(t, r, width, int(hd.size * 2 / size_t(width)));
			pos = r.bytePos();
		}
		hd = readMsqHeader(f, pos);
		if (!hd.ok) { pics.push_back(std::move(a)); break; }
		{
			BitReader r(f.data(), f.size(), hd.next);
			HuffmanTree t(r);
			for (size_t i = 0; i < hd.size; ++i) { const int b = t.decode(r); a.animData.push_back(uint8_t(b < 0 ? 0 : b)); }
			pos = r.bytePos();
		}
		pics.push_back(std::move(a));
	}
	return pics;
}

} // namespace wl
