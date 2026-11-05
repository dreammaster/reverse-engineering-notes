#include "legend/dcl.h"

namespace Legend {

namespace {

const int kMaxBits = 13;

// Code length tables of the DCL format (repetition-encoded as in blast.c: high nibble + 1 = repeat count, low nibble = bit length)
const byte kLitLen[98] = {
	11, 124, 8, 7, 28, 7, 188, 13, 76, 4, 10, 8, 12, 10, 12, 10, 8, 23, 8, 9, 7, 6, 7, 8, 7, 6, 55, 8, 23, 24, 12, 11,
	7, 9, 11, 12, 6, 7, 22, 5, 7, 24, 6, 11, 9, 6, 7, 22, 7, 11, 38, 7, 9, 8, 25, 11, 8, 11, 9, 12, 8, 12, 5, 38, 5, 38,
	5, 11, 7, 5, 6, 21, 6, 10, 53, 8, 7, 24, 10, 27, 44, 253, 253, 253, 252, 252, 252, 13, 12, 45, 12, 45, 12, 61, 12,
	45, 44, 173};
const byte kLenLen[6] = {2, 35, 36, 53, 38, 23};
const byte kDistLen[7] = {2, 20, 53, 230, 247, 151, 248};
const int16 kBase[16] = {3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264};
const byte kExtra[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8};

struct Huffman {
	int16 count[kMaxBits + 1];
	std::vector<int16> symbol;

	Huffman(const byte *rep, int n) {
		std::vector<int> length;
		for (int i = 0; i < n; i++) {
			int left = (rep[i] >> 4) + 1;
			int len = rep[i] & 15;
			while (left--)
				length.push_back(len);
		}
		memset(count, 0, sizeof(count));
		for (size_t i = 0; i < length.size(); i++)
			count[length[i]]++;
		int16 offs[kMaxBits + 1];
		memset(offs, 0, sizeof(offs));
		for (int len = 1; len < kMaxBits; len++)
			offs[len + 1] = (int16)(offs[len] + count[len]);
		symbol.assign(length.size(), 0);
		for (size_t sym = 0; sym < length.size(); sym++)
			if (length[sym] != 0)
				symbol[offs[length[sym]]++] = (int16)sym;
	}
};

struct BitReader {
	const byte *in;
	size_t left;
	int bitbuf;
	int bitcnt;
	bool error;

	BitReader(const byte *data, size_t size) : in(data), left(size), bitbuf(0), bitcnt(0), error(false) {}

	int bits(int need) {
		int val = bitbuf;
		while (bitcnt < need) {
			if (left == 0) {
				error = true;
				return 0;
			}
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
		const int16 *next = h.count + 1;
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
			if (left == 0) {
				error = true;
				return 0;
			}
			b = *in++;
			left--;
			if (lft > 8)
				lft = 8;
		}
		error = true;
		return 0;
	}
};

} // namespace

bool dclExplode(const byte *data, size_t size, Common::Bytes &out, size_t *consumed) {
	static const Huffman litCode(kLitLen, ARRAYSIZE(kLitLen)), lenCode(kLenLen, ARRAYSIZE(kLenLen)), distCode(kDistLen, ARRAYSIZE(kDistLen));
	BitReader s(data, size);
	out.clear();
	int lit = s.bits(8);
	int dictBits = s.bits(8);
	if (s.error || lit > 1 || dictBits < 4 || dictBits > 6)
		return false;
	for (;;) {
		if (s.bits(1)) {
			int sym = s.decode(lenCode);
			int len = kBase[sym] + s.bits(kExtra[sym]);
			if (s.error)
				return false;
			if (len == 519)
				break;                                    // end of stream
			int sh = len == 2 ? 2 : dictBits;
			size_t dist = ((size_t)s.decode(distCode) << sh) + s.bits(sh) + 1;
			if (s.error || dist > out.size())
				return false;
			for (int i = 0; i < len; i++)
				out.push_back(out[out.size() - dist]);
		} else {
			int v = lit ? s.decode(litCode) : s.bits(8);
			if (s.error)
				return false;
			out.push_back((byte)v);
		}
		if (out.size() > (1u << 26))
			return false;
	}
	if (consumed)
		*consumed = (size_t)(s.in - data);
	return true;
}

} // namespace Legend
