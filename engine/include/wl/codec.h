#pragma once
// Wasteland 1 data codecs: bit reader, Huffman trees, rotating-XOR "msq" blocks (formats from the Wasteland Suite
// sources and WL.EXE).
#include "fod/bytes.h"
#include <memory>

namespace wl {

using fod::Bytes;

class BitReader {
public:
	BitReader(const uint8_t *d, size_t n, size_t pos = 0) : _d(d), _n(n), _p(pos) {}
	int bit(bool lsbFirst = false);                 // -1 at the end
	int bits(int count, bool lsbFirst = false);
	int byte() { return bits(8); }
	size_t bytePos() const { return _p; }           // next unread byte (current partial byte counts as read)
private:
	const uint8_t *_d;
	size_t _n, _p;
	int _cur = 0, _bit = 7;
};

class HuffmanTree {
public:
	// Reads the serialised tree (bit 0 = inner node, 1 = leaf + 8 bit payload) from the bit reader.
	explicit HuffmanTree(BitReader &r);
	int decode(BitReader &r) const;                 // one payload byte, -1 on EOF
private:
	struct Node { int left = -1, right = -1, payload = -1; };
	int load(BitReader &r);
	std::vector<Node> _nodes;
	int _root = -1;
};

// Rotating XOR (WL.EXE block cipher): key = b0 ^ b1, key += 0x1F per byte.
Bytes xorDecrypt(const uint8_t *data, size_t n, size_t count);

struct MsqBlock { size_t offset, size; int disk; };
std::vector<MsqBlock> splitMsq(const Bytes &file);     // blocks of GAME1 / GAME2

} // namespace wl
