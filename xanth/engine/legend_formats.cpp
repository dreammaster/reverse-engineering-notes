#include "legend_formats.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace Legend {

static uint16_t rd16(const Bytes &d, size_t p) {
	if (p + 2 > d.size())
		throw FormatError("read past end of data");
	return (uint16_t)(d[p] | (d[p + 1] << 8));
}

static uint32_t rd32(const Bytes &d, size_t p) {
	if (p + 4 > d.size())
		throw FormatError("read past end of data");
	return (uint32_t)d[p] | ((uint32_t)d[p + 1] << 8) | ((uint32_t)d[p + 2] << 16) | ((uint32_t)d[p + 3] << 24);
}

Bytes readFile(const std::string &path) {
	FILE *f = fopen(path.c_str(), "rb");
	if (!f)
		throw FormatError("cannot open " + path);
	Bytes b;
	uint8_t buf[65536];
	size_t n;
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
		b.insert(b.end(), buf, buf + n);
	fclose(f);
	return b;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// PKWARE DCL "implode" decoder (after Mark Adler's blast.c; reference: xanth/reversing/scripts/dcl_explode.py)

namespace {

const int kMaxBits = 13;

const uint8_t kLitLen[98] = {
	11, 124, 8, 7, 28, 7, 188, 13, 76, 4, 10, 8, 12, 10, 12, 10, 8, 23, 8, 9, 7, 6, 7, 8, 7, 6, 55, 8, 23, 24, 12, 11,
	7, 9, 11, 12, 6, 7, 22, 5, 7, 24, 6, 11, 9, 6, 7, 22, 7, 11, 38, 7, 9, 8, 25, 11, 8, 11, 9, 12, 8, 12, 5, 38, 5, 38,
	5, 11, 7, 5, 6, 21, 6, 10, 53, 8, 7, 24, 10, 27, 44, 253, 253, 253, 252, 252, 252, 13, 12, 45, 12, 45, 12, 61, 12,
	45, 44, 173};
const uint8_t kLenLen[6] = {2, 35, 36, 53, 38, 23};
const uint8_t kDistLen[7] = {2, 20, 53, 230, 247, 151, 248};
const int16_t kBase[16] = {3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264};
const uint8_t kExtra[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8};

struct Huffman {
	int16_t count[kMaxBits + 1];
	std::vector<int16_t> symbol;

	Huffman(const uint8_t *rep, int n) {
		std::vector<int> length;
		for (int i = 0; i < n; i++) {
			int left = (rep[i] >> 4) + 1;
			int len = rep[i] & 15;
			while (left--)
				length.push_back(len);
		}
		memset(count, 0, sizeof(count));
		for (int len : length)
			count[len]++;
		int16_t offs[kMaxBits + 1] = {0};
		for (int len = 1; len < kMaxBits; len++)
			offs[len + 1] = (int16_t)(offs[len] + count[len]);
		symbol.assign(length.size(), 0);
		for (size_t sym = 0; sym < length.size(); sym++)
			if (length[sym] != 0)
				symbol[offs[length[sym]]++] = (int16_t)sym;
	}
};

struct BlastState {
	const uint8_t *in;
	size_t left;
	const uint8_t *start;
	int bitbuf = 0;
	int bitcnt = 0;

	int bits(int need) {
		int val = bitbuf;
		while (bitcnt < need) {
			if (left == 0)
				throw FormatError("DCL: out of input");
			val |= (int)(*in++) << bitcnt;
			left--;
			bitcnt += 8;
		}
		bitbuf = val >> need;
		bitcnt -= need;
		return val & ((1 << need) - 1);
	}

	int decode(const Huffman &h) {
		int code = 0, first = 0, index = 0, len = 1;
		int b = bitbuf, lft = bitcnt;
		const int16_t *next = h.count + 1;
		for (;;) {
			while (lft--) {
				code |= (b & 1) ^ 1;
				b >>= 1;
				int count = *next++;
				if (code < first + count) {
					bitbuf = b;
					bitcnt = (bitcnt - len) & 7;
					return h.symbol[index + (code - first)];
				}
				index += count;
				first += count;
				first <<= 1;
				code <<= 1;
				len++;
			}
			lft = (kMaxBits + 1) - len;
			if (lft == 0)
				break;
			if (left == 0)
				throw FormatError("DCL: out of input");
			b = *in++;
			left--;
			if (lft > 8)
				lft = 8;
		}
		throw FormatError("DCL: bad code");
	}
};

} // namespace

Bytes dclExplode(const uint8_t *data, size_t size, size_t *consumed) {
	static const Huffman litCode(kLitLen, sizeof(kLitLen)), lenCode(kLenLen, sizeof(kLenLen)), distCode(kDistLen, sizeof(kDistLen));
	BlastState s;
	s.in = data;
	s.start = data;
	s.left = size;
	int lit = s.bits(8);
	if (lit > 1)
		throw FormatError("DCL: bad literal flag");
	int dictBits = s.bits(8);
	if (dictBits < 4 || dictBits > 6)
		throw FormatError("DCL: bad dictionary size");
	Bytes out;
	for (;;) {
		if (s.bits(1)) {
			int sym = s.decode(lenCode);
			int len = kBase[sym] + s.bits(kExtra[sym]);
			if (len == 519)
				break;
			int sh = len == 2 ? 2 : dictBits;
			size_t dist = ((size_t)s.decode(distCode) << sh) + s.bits(sh) + 1;
			if (dist > out.size())
				throw FormatError("DCL: distance too far");
			for (int i = 0; i < len; i++)
				out.push_back(out[out.size() - dist]);
		} else {
			out.push_back((uint8_t)(lit ? s.decode(litCode) : s.bits(8)));
		}
		if (out.size() > (1u << 26))
			throw FormatError("DCL: output too large");
	}
	if (consumed)
		*consumed = (size_t)(s.in - s.start);
	return out;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// PIC: 12-byte directory {u32 offset; u8 count; u8 flags; u16 w; u16 h; u8; u8} until the first data offset; entry data = count*{i16 x,y}, optional
// 768-byte palette (flags & 0x10), pixels (DCL-compressed in Xanth, raw in Shannara)

PicFile::PicFile(Bytes data) : _d(std::move(data)) {
	for (size_t i = 0; i * 12 + 12 <= _d.size(); i++) {
		uint32_t off = rd32(_d, i * 12);
		if (off == 0)
			break;
		Entry e;
		e.offset = off;
		e.count = _d[i * 12 + 4];
		e.flags = _d[i * 12 + 5];
		e.width = rd16(_d, i * 12 + 6);
		e.height = rd16(_d, i * 12 + 8);
		e.end = 0;
		_entries.push_back(e);
		if ((i + 1) * 12 >= _entries[0].offset)
			break;
	}
	for (size_t k = 0; k < _entries.size(); k++)
		_entries[k].end = k + 1 < _entries.size() ? _entries[k + 1].offset : (uint32_t)_d.size();
}

PicFile::Image PicFile::decode(size_t i, bool compressed) const {
	const Entry &e = _entries.at(i);
	Image im;
	im.width = e.width;
	im.height = e.height;
	size_t p = e.offset;
	if (e.count) {
		im.x = (int16_t)rd16(_d, p);
		im.y = (int16_t)rd16(_d, p + 2);
		p += 4u * e.count;
	}
	if (e.flags & 0x10) {
		if (p + 768 > _d.size())
			throw FormatError("PIC: palette past end");
		memcpy(im.palette, &_d[p], 768);
		im.hasPalette = true;
		p += 768;
	}
	size_t want = (size_t)e.width * e.height;
	if (compressed) {
		im.pixels = dclExplode(&_d[p], e.end - p);
		if (im.pixels.size() != want)
			throw FormatError("PIC: decompressed size does not match width*height");
	} else {
		if (p + want > _d.size())
			throw FormatError("PIC: pixels past end");
		im.pixels.assign(_d.begin() + p, _d.begin() + p + want);
	}
	return im;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// RGN: directory of {u16 dataOffset; u16 baseId; u16 count} per PIC entry; regions {i16 id; i16 type; ...}: type 2 = rectangle, else polygon with `type` vertices

RgnFile::RgnFile(const Bytes &d) {
	size_t limit = d.size();
	for (size_t i = 0; i * 6 + 6 <= limit; i++) {
		uint16_t off = rd16(d, i * 6);
		Scene s;
		s.baseId = rd16(d, i * 6 + 2);
		uint16_t cnt = rd16(d, i * 6 + 4);
		if (off) {
			limit = std::min<size_t>(limit, off);
			size_t p = off;
			for (unsigned r = 0; r < cnt; r++) {
				Region rg;
				rg.id = (int16_t)rd16(d, p);
				int16_t type = (int16_t)rd16(d, p + 2);
				p += 4;
				int n = type == 2 ? 4 : 2 * type;
				rg.rect = type == 2;
				for (int k = 0; k < n; k++, p += 2)
					rg.pts.push_back((int16_t)rd16(d, p));
				s.regions.push_back(rg);
			}
		}
		_scenes.push_back(std::move(s));
	}
}

bool RgnFile::Region::contains(int x, int y) const {
	if (rect) {
		int x1 = std::min(pts[0], pts[2]), x2 = std::max(pts[0], pts[2]);
		int y1 = std::min(pts[1], pts[3]), y2 = std::max(pts[1], pts[3]);
		return x >= x1 && x <= x2 && y >= y1 && y <= y2;
	}
	bool in = false;
	size_t n = pts.size() / 2;
	for (size_t i = 0, j = n - 1; i < n; j = i++) {
		int xi = pts[2 * i], yi = pts[2 * i + 1], xj = pts[2 * j], yj = pts[2 * j + 1];
		if (((yi > y) != (yj > y)) && x < (xj - xi) * (y - yi) / (double)(yj - yi) + xi)
			in = !in;
	}
	return in;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// XANTHSTR.DAT / SHANSTR.DAT: u16 nblocks; nblocks*{u16 count; u32 nbytes}; u16 length per string; Huffman tree (u16 n, n words, negative = leaf -(sym+1)); phrase table
// (u16 k, k offsets, u16 poolSize, pool of NUL-terminated strings; symbols 0x80+ expand to a phrase); then the blocks' bit-packed strings (LSB first, NUL ends)

MessageFile::MessageFile(Bytes data) : _d(std::move(data)) {
	size_t p = 0;
	unsigned nblocks = rd16(_d, p);
	p += 2;
	std::vector<std::pair<unsigned, uint32_t>> bl;
	for (unsigned i = 0; i < nblocks; i++, p += 6)
		bl.push_back({rd16(_d, p), rd32(_d, p + 2)});
	size_t lenBase = p;
	unsigned total = 0;
	for (auto &b : bl)
		total += b.first;
	p += 2u * total;
	unsigned n1 = rd16(_d, p);
	p += 2;
	for (unsigned i = 0; i < n1; i++)
		_tree.push_back((int16_t)rd16(_d, p + 2 * i));
	p += 2u * n1;
	unsigned k = rd16(_d, p);
	p += 2;
	if (k) {
		size_t offs = p;
		p += 2u * k;
		unsigned poolSize = rd16(_d, p);
		p += 2;
		const uint8_t *pool = &_d.at(p);
		for (unsigned i = 0; i < k; i++) {
			unsigned o = rd16(_d, offs + 2 * i);
			unsigned e = o;
			while (e < poolSize && pool[e])
				e++;
			_phrases.push_back(std::string((const char *)pool + o, e - o));
		}
		p += poolSize;
	}
	size_t dataOff = p;
	size_t q = lenBase;
	for (auto &b : bl) {
		Block blk;
		blk.dataOffset = (uint32_t)dataOff;
		uint32_t o = 0;
		for (unsigned i = 0; i < b.first; i++, q += 2) {
			uint16_t l = rd16(_d, q);
			blk.lengths.push_back(l);
			blk.offsets.push_back(o);
			o += l;
		}
		_blocks.push_back(std::move(blk));
		dataOff += b.second;
	}
}

std::string MessageFile::string(size_t block, size_t index) const {
	const Block &b = _blocks.at(block);
	size_t n = b.lengths.at(index);
	size_t off = b.dataOffset + b.offsets[index];
	if (off + n > _d.size())
		throw FormatError("message data past end");
	const uint8_t *src = &_d[off];
	size_t root = _tree.size() - 2, node = root;
	int last = -1;
	std::string out;
	for (size_t bit = 0; bit < n * 8; bit++) {
		if ((bit + 7) / 8 >= n && last == 0)
			break;
		int v = (src[bit >> 3] >> (bit & 7)) & 1;
		int t = _tree.at(node | v);
		if (t < 0) {
			int sym = -t - 1;
			last = sym;
			if (sym >= 0x80 && (size_t)(sym - 0x80) < _phrases.size())
				out += _phrases[sym - 0x80];
			else
				out.push_back((char)sym);
			node = root;
		} else {
			node = (size_t)t;
		}
	}
	while (!out.empty() && out.back() == 0)
		out.pop_back();
	return out;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// MUS: 12-byte directory {u32 offset; u16 size, a, b, c}; each record is a standard MIDI file

MusFile::MusFile(Bytes data) : _d(std::move(data)) {
	uint32_t first = rd32(_d, 0);
	for (uint32_t i = 0; i < first / 12; i++) {
		Record r;
		r.offset = rd32(_d, i * 12);
		r.size = rd16(_d, i * 12 + 4);
		r.a = rd16(_d, i * 12 + 6);
		r.b = rd16(_d, i * 12 + 8);
		r.c = rd16(_d, i * 12 + 10);
		_records.push_back(r);
	}
}

const MusFile::Record *MusFile::tune(unsigned t, unsigned arrangement) const {
	size_t i = 2 * (size_t)t - 2 + (arrangement & 1);
	if (t == 0 || i >= _records.size() || _records[i].offset == 0 || _records[i].offset >= _d.size())
		return nullptr;
	return &_records[i];
}

size_t MusFile::smfLength(const Bytes &d, size_t off) {
	if (off + 14 > d.size() || memcmp(&d[off], "MThd", 4) != 0)
		return 0;
	auto be32 = [&](size_t p) { return ((uint32_t)d[p] << 24) | (d[p + 1] << 16) | (d[p + 2] << 8) | d[p + 3]; };
	size_t p = off + 8 + be32(off + 4);
	unsigned ntr = (d[off + 10] << 8) | d[off + 11];
	for (unsigned i = 0; i < ntr; i++) {
		if (p + 8 > d.size() || memcmp(&d[p], "MTrk", 4) != 0)
			return 0;
		p += 8 + be32(p + 4);
	}
	return p - off;
}

Bytes MusFile::midi(const Record &r) const {
	size_t n = smfLength(_d, r.offset);
	if (!n)
		throw FormatError("MUS: record is not a standard MIDI file");
	return Bytes(_d.begin() + r.offset, _d.begin() + r.offset + n);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// VOC: u16 count; count * u32 absolute offsets (last = file size)

VocFile::VocFile(const Bytes &dir) {
	unsigned n = rd16(dir, 0);
	for (unsigned i = 0; i < n; i++)
		_offsets.push_back(rd32(dir, 2 + 4 * i));
	if (_offsets.size() < 2)
		throw FormatError("VOC: empty directory");
}

size_t VocFile::trimmedSize(const uint8_t *samples, size_t size) {
	while (size && samples[size - 1] == 0x80)
		size--;
	return size;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// RS: "STEVE" kind scale u32-length(at +7) 21-byte constant table, samples from +0x20

RsFile::RsFile(const Bytes &d) {
	if (d.size() < 32 || memcmp(d.data(), "STEVE", 5) != 0)
		throw FormatError("RS: bad header");
	_kind = d[5];
	_scale = d[6];
	uint32_t len = rd32(d, 7);
	if (!_scale || 32 + (size_t)len > d.size())
		throw FormatError("RS: bad length or scale");
	_samples.assign(d.begin() + 32, d.begin() + 32 + len);
}

Bytes RsFile::toPcm8() const {
	Bytes out(_samples.size());
	for (size_t i = 0; i < out.size(); i++)
		out[i] = (uint8_t)std::min<unsigned>(255, _samples[i] * 255u / _scale);
	return out;
}

} // namespace Legend
