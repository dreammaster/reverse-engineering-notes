#include "zlibShim.h"

#include <cstring>

namespace {
const int kMaxBits = 15;
const int kMaxLitLenCodes = 286;
const int kMaxDistCodes = 30;
const int kFixedLitLenCodes = 288;

const int kDataError = -3;
const int kBufError = -5;

struct State {
	const unsigned char *in;
	unsigned long inLen;
	unsigned long inPos;
	unsigned long bitBuf;
	int bitCount;
	unsigned char *out;
	unsigned long outLen;
	unsigned long outPos;
};

struct Huffman {
	short count[kMaxBits + 1];
	short symbol[kFixedLitLenCodes];
};

// Returns the next `need` bits, or -1 when the input runs out.
int getBits(State &s, int need) {
	unsigned long value = s.bitBuf;
	while (s.bitCount < need) {
		if (s.inPos >= s.inLen)
			return -1;
		value |= static_cast<unsigned long>(s.in[s.inPos++]) << s.bitCount;
		s.bitCount += 8;
	}
	s.bitBuf = value >> need;
	s.bitCount -= need;
	return static_cast<int>(value & ((1UL << need) - 1));
}

int decode(State &s, const Huffman &h) {
	int code = 0, first = 0, index = 0;
	for (int len = 1; len <= kMaxBits; len++) {
		int bit = getBits(s, 1);
		if (bit < 0)
			return -1;
		code |= bit;
		int count = h.count[len];
		if (code - count < first)
			return h.symbol[index + (code - first)];
		index += count;
		first += count;
		first <<= 1;
		code <<= 1;
	}
	return -10;
}

// Builds the canonical decoding tables; returns 0 for a complete code, > 0
// for an incomplete one, < 0 for an over-subscribed one.
int construct(Huffman &h, const short *length, int n) {
	for (int len = 0; len <= kMaxBits; len++)
		h.count[len] = 0;
	for (int symbol = 0; symbol < n; symbol++)
		h.count[length[symbol]]++;
	if (h.count[0] == n)
		return 0;

	int left = 1;
	for (int len = 1; len <= kMaxBits; len++) {
		left <<= 1;
		left -= h.count[len];
		if (left < 0)
			return left;
	}

	short offs[kMaxBits + 1];
	offs[1] = 0;
	for (int len = 1; len < kMaxBits; len++)
		offs[len + 1] = offs[len] + h.count[len];
	for (int symbol = 0; symbol < n; symbol++) {
		if (length[symbol] != 0)
			h.symbol[offs[length[symbol]]++] = static_cast<short>(symbol);
	}
	return left;
}

int codes(State &s, const Huffman &lencode, const Huffman &distcode) {
	static const short lens[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
	                               35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
	static const short lext[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
	                               3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
	static const short dists[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
	                                257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
	                                8193, 12289, 16385, 24577};
	static const short dext[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
	                               7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

	for (;;) {
		int symbol = decode(s, lencode);
		if (symbol < 0)
			return kDataError;
		if (symbol < 256) {
			if (s.outPos >= s.outLen)
				return kBufError;
			s.out[s.outPos++] = static_cast<unsigned char>(symbol);
		} else if (symbol == 256) {
			return 0;
		} else {
			symbol -= 257;
			if (symbol >= 29)
				return kDataError;
			int extra = getBits(s, lext[symbol]);
			if (extra < 0)
				return kDataError;
			int len = lens[symbol] + extra;

			symbol = decode(s, distcode);
			if (symbol < 0)
				return kDataError;
			extra = getBits(s, dext[symbol]);
			if (extra < 0)
				return kDataError;
			unsigned long dist = static_cast<unsigned long>(dists[symbol] + extra);
			if (dist > s.outPos)
				return kDataError;
			if (s.outPos + static_cast<unsigned long>(len) > s.outLen)
				return kBufError;
			for (; len > 0; len--, s.outPos++)
				s.out[s.outPos] = s.out[s.outPos - dist];
		}
	}
}

int stored(State &s) {
	s.bitBuf = 0;
	s.bitCount = 0;
	if (s.inPos + 4 > s.inLen)
		return kDataError;
	unsigned len = s.in[s.inPos] | (s.in[s.inPos + 1] << 8);
	unsigned nlen = s.in[s.inPos + 2] | (s.in[s.inPos + 3] << 8);
	s.inPos += 4;
	if (len != (~nlen & 0xFFFF))
		return kDataError;
	if (s.inPos + len > s.inLen)
		return kDataError;
	if (s.outPos + len > s.outLen)
		return kBufError;
	std::memcpy(s.out + s.outPos, s.in + s.inPos, len);
	s.inPos += len;
	s.outPos += len;
	return 0;
}

int fixedBlock(State &s) {
	static bool built = false;
	static Huffman lencode, distcode;
	if (!built) {
		short lengths[kFixedLitLenCodes];
		int symbol = 0;
		for (; symbol < 144; symbol++)
			lengths[symbol] = 8;
		for (; symbol < 256; symbol++)
			lengths[symbol] = 9;
		for (; symbol < 280; symbol++)
			lengths[symbol] = 7;
		for (; symbol < kFixedLitLenCodes; symbol++)
			lengths[symbol] = 8;
		construct(lencode, lengths, kFixedLitLenCodes);
		for (symbol = 0; symbol < kMaxDistCodes; symbol++)
			lengths[symbol] = 5;
		construct(distcode, lengths, kMaxDistCodes);
		built = true;
	}
	return codes(s, lencode, distcode);
}

int dynamicBlock(State &s) {
	static const short order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
	short lengths[kMaxLitLenCodes + kMaxDistCodes];
	Huffman lencode, distcode;

	int nlen = getBits(s, 5) + 257;
	int ndist = getBits(s, 5) + 1;
	int ncode = getBits(s, 4) + 4;
	if (nlen > kMaxLitLenCodes || ndist > kMaxDistCodes)
		return kDataError;

	int index = 0;
	for (; index < ncode; index++) {
		int value = getBits(s, 3);
		if (value < 0)
			return kDataError;
		lengths[order[index]] = static_cast<short>(value);
	}
	for (; index < 19; index++)
		lengths[order[index]] = 0;

	if (construct(lencode, lengths, 19) != 0)
		return kDataError;

	index = 0;
	while (index < nlen + ndist) {
		int symbol = decode(s, lencode);
		if (symbol < 0)
			return kDataError;
		if (symbol < 16) {
			lengths[index++] = static_cast<short>(symbol);
		} else {
			int len = 0;
			int repeat;
			if (symbol == 16) {
				if (index == 0)
					return kDataError;
				len = lengths[index - 1];
				repeat = getBits(s, 2) + 3;
			} else if (symbol == 17) {
				repeat = getBits(s, 3) + 3;
			} else {
				repeat = getBits(s, 7) + 11;
			}
			if (index + repeat > nlen + ndist)
				return kDataError;
			while (repeat--)
				lengths[index++] = static_cast<short>(len);
		}
	}

	if (lengths[256] == 0)
		return kDataError;

	int err = construct(lencode, lengths, nlen);
	if (err != 0 && (err < 0 || nlen != lencode.count[0] + lencode.count[1]))
		return kDataError;
	err = construct(distcode, lengths + nlen, ndist);
	if (err != 0 && (err < 0 || ndist != distcode.count[0] + distcode.count[1]))
		return kDataError;

	return codes(s, lencode, distcode);
}

unsigned adler32(const unsigned char *data, unsigned long length) {
	unsigned a = 1, b = 0;
	for (unsigned long i = 0; i < length; i++) {
		a = (a + data[i]) % 65521;
		b = (b + a) % 65521;
	}
	return (b << 16) | a;
}
}  // namespace

int zlibUncompress(unsigned char *dest, unsigned long *destLen, const unsigned char *source, unsigned long sourceLen) {
	if (sourceLen < 6)
		return kDataError;
	// zlib header: deflate (CM 8), window <= 32K, no preset dictionary.
	if ((source[0] & 0x0F) != 8 || ((source[0] << 8) | source[1]) % 31 != 0 || (source[1] & 0x20))
		return kDataError;

	State s = {source, sourceLen, 2, 0, 0, dest, *destLen, 0};
	int last;
	do {
		last = getBits(s, 1);
		int type = getBits(s, 2);
		if (last < 0 || type < 0)
			return kDataError;

		int err;
		switch (type) {
		case 0:
			err = stored(s);
			break;
		case 1:
			err = fixedBlock(s);
			break;
		case 2:
			err = dynamicBlock(s);
			break;
		default:
			err = kDataError;
			break;
		}
		if (err != 0)
			return err;
	} while (!last);

	*destLen = s.outPos;
	return 0;
}

int zlibCompress(unsigned char *dest, unsigned long *destLen, const unsigned char *source, unsigned long sourceLen) {
	unsigned long blocks = sourceLen == 0 ? 1 : (sourceLen + 65534) / 65535;
	unsigned long needed = 2 + blocks * 5 + sourceLen + 4;
	if (needed > *destLen)
		return kBufError;

	unsigned long pos = 0;
	dest[pos++] = 0x78;
	dest[pos++] = 0x01;
	unsigned long remaining = sourceLen;
	const unsigned char *in = source;
	do {
		unsigned long chunk = remaining > 65535 ? 65535 : remaining;
		remaining -= chunk;
		dest[pos++] = remaining == 0 ? 1 : 0;
		dest[pos++] = static_cast<unsigned char>(chunk);
		dest[pos++] = static_cast<unsigned char>(chunk >> 8);
		dest[pos++] = static_cast<unsigned char>(~chunk);
		dest[pos++] = static_cast<unsigned char>(~chunk >> 8);
		std::memcpy(dest + pos, in, chunk);
		pos += chunk;
		in += chunk;
	} while (remaining > 0);

	unsigned checksum = adler32(source, sourceLen);
	dest[pos++] = static_cast<unsigned char>(checksum >> 24);
	dest[pos++] = static_cast<unsigned char>(checksum >> 16);
	dest[pos++] = static_cast<unsigned char>(checksum >> 8);
	dest[pos++] = static_cast<unsigned char>(checksum);
	*destLen = pos;
	return 0;
}
