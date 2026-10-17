#include "shanstr.h"

namespace shannara {

ShanStr::ShanStr(Bytes data) : _data(std::move(data)) {
	Cursor c(_data);
	int nblocks = c.u16();
	_blocks.resize(nblocks);
	for (auto &b : _blocks) {
		b.count = c.u16();
		b.bytes = c.u32();
	}
	for (auto &b : _blocks) {
		b.lengths.resize(b.count);
		b.prefix.resize(b.count);
		uint32_t sum = 0;
		for (int i = 0; i < b.count; i++) {
			b.lengths[i] = c.u16();
			b.prefix[i] = sum;
			sum += b.lengths[i];
		}
	}
	int n1 = c.u16();
	_tree.resize(n1);
	for (int i = 0; i < n1; i++)
		_tree[i] = (int16_t)c.u16();
	int k = c.u16();
	if (k) {
		std::vector<uint16_t> offs(k);
		for (int i = 0; i < k; i++)
			offs[i] = c.u16();
		size_t poolSize = c.u16();
		const uint8_t *pool = c.ptr();
		c.skip(poolSize);
		for (int i = 0; i < k; i++) {
			size_t o = offs[i], e = o;
			while (e < poolSize && pool[e])
				e++;
			_phrases.emplace_back((const char *)pool + o, e - o);
		}
	}
	size_t off = c.pos();
	for (auto &b : _blocks) {
		b.dataOffset = off;
		off += b.bytes;
	}
	_end = off;
}

std::string ShanStr::string(int block, int index) const {
	if (block < 0 || block >= (int)_blocks.size())
		throw FormatError("bad string block");
	const Block &b = _blocks[block];
	if (index < 0 || index >= b.count)
		throw FormatError("bad string index");
	size_t start = b.dataOffset + b.prefix[index];
	size_t n = b.lengths[index];
	if (start + n > _data.size())
		throw FormatError("string data past end of file");
	return decode(&_data[start], n);
}

std::string ShanStr::decode(const uint8_t *src, size_t n) const {
	std::string out;
	if (_tree.size() < 2)
		return out;
	const int root = (int)_tree.size() - 2;
	int node = root, last = -1;
	size_t bit = 0;
	const size_t nbits = n * 8;
	for (;;) {
		// engine stops once all bytes were consumed and the last symbol was the NUL terminator
		if ((bit + 7) / 8 >= n && last == 0)
			break;
		if (bit >= nbits)
			break;
		int b = (src[bit >> 3] >> (bit & 7)) & 1;
		bit++;
		node = _tree[node | b];
		if (node < 0) {
			int sym = -node - 1;
			last = sym;
			if (sym >= 0x80 && sym - 0x80 < (int)_phrases.size())
				out += _phrases[sym - 0x80];
			else
				out.push_back((char)sym);
			node = root;
		}
	}
	return out;
}

} // namespace shannara
