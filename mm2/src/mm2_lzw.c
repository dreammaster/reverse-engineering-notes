#include "mm2_lzw.h"

#define MAX_CODES 4096

long mm2_lzw_decode(const uint8_t *in, size_t inlen, uint8_t *out, size_t outcap) {
	uint16_t prefix[MAX_CODES];
	uint8_t suffix[MAX_CODES];
	uint8_t stack[MAX_CODES + 1];
	size_t bitpos = 0, outlen = 0;
	int nbits = 9, nextCode = 0x102, limit = 0x200;
	int prev = -1;
	uint8_t firstOfPrev = 0;

	for (;;) {
		size_t byte = bitpos >> 3;
		uint32_t chunk;
		int code;
		if (byte >= inlen)
			break;
		chunk = in[byte];
		if (byte + 1 < inlen) chunk |= (uint32_t)in[byte + 1] << 8;
		if (byte + 2 < inlen) chunk |= (uint32_t)in[byte + 2] << 16;
		code = (int)((chunk >> (bitpos & 7)) & ((1u << nbits) - 1));
		bitpos += nbits;

		if (code == 0x101)
			break;
		if (code == 0x100) {
			nbits = 9; nextCode = 0x102; limit = 0x200; prev = -1;
			continue;
		}

		{
			int sp = 0, cur = code;
			uint8_t first;
			if (code >= nextCode) {
				if (prev < 0 || code != nextCode)
					return -1;
				cur = prev;
				stack[sp++] = firstOfPrev;   /* entry = prev + prev[0] */
			}
			while (cur >= 0x102) {
				if (sp >= MAX_CODES) return -1;
				stack[sp++] = suffix[cur];
				cur = prefix[cur];
			}
			stack[sp++] = (uint8_t)cur;
			first = (uint8_t)cur;
			if (outlen + (size_t)sp > outcap)
				return -1;
			while (sp > 0)
				out[outlen++] = stack[--sp];
			if (prev >= 0) {
				prefix[nextCode] = (uint16_t)prev;
				suffix[nextCode] = first;
				nextCode++;
				if (nextCode >= limit && nbits < 12) {
					nbits++;
					limit <<= 1;
				}
			}
			prev = code;
			firstOfPrev = first;
		}
	}
	return (long)outlen;
}
