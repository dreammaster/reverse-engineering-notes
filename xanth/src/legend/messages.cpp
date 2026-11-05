#include "legend/messages.h"

#include "common/stream.h"

namespace Legend {

bool MessageFile::load(const Common::Bytes &data) {
	_d = data;
	_blocks.clear();
	_tree.clear();
	_phrases.clear();
	Common::ReadStream s(_d);
	unsigned nblocks = s.readUint16LE();
	std::vector<unsigned> counts;
	std::vector<uint32> sizes;
	unsigned total = 0;
	for (unsigned i = 0; i < nblocks; i++) {
		counts.push_back(s.readUint16LE());
		sizes.push_back(s.readUint32LE());
		total += counts.back();
	}
	size_t lenBase = s.pos();
	s.skip(2u * total);
	unsigned n1 = s.readUint16LE();
	for (unsigned i = 0; i < n1; i++)
		_tree.push_back(s.readSint16LE());
	unsigned k = s.readUint16LE();
	if (k) {
		size_t offs = s.pos();
		s.skip(2u * k);
		unsigned poolSize = s.readUint16LE();
		size_t pool = s.pos();
		if (s.err() || pool + poolSize > _d.size())
			return false;
		for (unsigned i = 0; i < k; i++) {
			s.seek(offs + 2 * i);
			unsigned o = s.readUint16LE();
			unsigned e = o;
			while (e < poolSize && _d[pool + e])
				e++;
			_phrases.push_back(std::string((const char *)&_d[pool + o], e - o));
		}
		s.seek(pool + poolSize);
	}
	if (s.err() || _tree.size() < 2)
		return false;
	size_t dataOff = s.pos();
	s.seek(lenBase);
	for (unsigned b = 0; b < nblocks; b++) {
		Block blk;
		blk.dataOffset = (uint32)dataOff;
		uint32 o = 0;
		for (unsigned i = 0; i < counts[b]; i++) {
			uint16 l = s.readUint16LE();
			blk.lengths.push_back(l);
			blk.offsets.push_back(o);
			o += l;
		}
		_blocks.push_back(blk);
		dataOff += sizes[b];
	}
	return !s.err();
}

std::string MessageFile::string(size_t block, size_t index) const {
	if (block >= _blocks.size() || index >= _blocks[block].lengths.size())
		return std::string();
	const Block &b = _blocks[block];
	size_t n = b.lengths[index];
	size_t off = b.dataOffset + b.offsets[index];
	if (off + n > _d.size())
		return std::string();
	const byte *src = n ? &_d[off] : nullptr;
	size_t root = _tree.size() - 2, node = root;
	int last = -1;
	std::string out;
	for (size_t bit = 0; bit < n * 8; bit++) {
		if ((bit + 7) / 8 >= n && last == 0)             // the NUL symbol ended the string
			break;
		int v = (src[bit >> 3] >> (bit & 7)) & 1;
		size_t at = node | (size_t)v;
		if (at >= _tree.size())
			return std::string();
		int t = _tree[at];
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
	while (!out.empty() && out[out.size() - 1] == 0)
		out.erase(out.size() - 1);
	return out;
}

} // namespace Legend
