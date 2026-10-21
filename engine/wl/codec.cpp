#include "wl/codec.h"

namespace wl {

int BitReader::bit(bool lsbFirst) {
	if (_bit > 6) {
		if (_p >= _n) return -1;
		_cur = _d[_p++];
		_bit = 0;
	} else {
		++_bit;
	}
	return lsbFirst ? (_cur >> _bit) & 1 : (_cur >> (7 - _bit)) & 1;
}

int BitReader::bits(int count, bool lsbFirst) {
	int v = 0;
	for (int i = 0; i < count; ++i) {
		const int b = bit(lsbFirst);
		if (b < 0) return -1;
		v = lsbFirst ? (v | (b << i)) : ((v << 1) | b);
	}
	return v;
}

HuffmanTree::HuffmanTree(BitReader &r) { _root = load(r); }

int HuffmanTree::load(BitReader &r) {
	const int b = r.bit();
	if (b < 0) throw std::runtime_error("huffman tree truncated");
	Node n;
	if (b == 0) {
		const int l = load(r);
		r.bit();   // separator bit before the right subtree
		const int rr = load(r);
		n.left = l; n.right = rr;
	} else {
		n.payload = r.byte();
	}
	_nodes.push_back(n);
	return int(_nodes.size()) - 1;
}

int HuffmanTree::decode(BitReader &r) const {
	int node = _root;
	while (_nodes[size_t(node)].payload == -1) {
		const int b = r.bit();
		if (b < 0) return -1;
		node = b == 0 ? _nodes[size_t(node)].left : _nodes[size_t(node)].right;
	}
	return _nodes[size_t(node)].payload;
}

Bytes xorDecrypt(const uint8_t *data, size_t n, size_t count) {
	Bytes out;
	if (n < 2) return out;
	int enc = data[0] ^ data[1];
	for (size_t i = 0; i < count && 2 + i < n; ++i) {
		out.push_back(uint8_t(data[2 + i] ^ enc));
		enc = (enc + 0x1F) & 0xFF;
	}
	return out;
}

std::vector<MsqBlock> splitMsq(const Bytes &f) {
	std::vector<MsqBlock> blocks;
	if (f.size() < 4 || std::memcmp(f.data(), "msq", 3) != 0) throw std::runtime_error("no msq header");
	const uint8_t disk = f[3];
	size_t start = 0;
	for (size_t i = 4; i + 3 < f.size(); ++i)
		if (f[i] == 'm' && f[i + 1] == 's' && f[i + 2] == 'q' && f[i + 3] == disk) {
			blocks.push_back({start, i - start, disk - '0'});
			start = i;
		}
	blocks.push_back({start, f.size() - start, disk - '0'});
	return blocks;
}

} // namespace wl
