#include "pc1.h"

#include <stdlib.h>
#include <string.h>

int pc1_lz(const uint8_t *in, size_t len, uint8_t **out, size_t *out_len) {
	size_t cap = 40000, n = 0, p = 0;
	uint8_t *o = (uint8_t *)malloc(cap);
	if (!o)
		return -3;

#define NEED(k) do { if (p + (k) > len) goto bad; } while (0)
#define EMIT(v) do { if (n == cap) { cap *= 2; uint8_t *t = (uint8_t *)realloc(o, cap); if (!t) goto bad; o = t; } o[n++] = (uint8_t)(v); } while (0)

	for (;;) {
		NEED(2);
		size_t blen = in[p] | (in[p + 1] << 8);
		p += 2;
		if (blen == 0)
			break;
		size_t end = p + blen, base = n;
		if (end > len)
			goto bad;
		while (p < end) {
			unsigned c = in[p++];
			if (c < 0x80) {
				NEED(c);
				for (unsigned i = 0; i < c; i++)
					EMIT(in[p++]);
			} else {
				size_t cnt;
				if (c == 0xFF) {
					NEED(2);
					cnt = in[p] | (in[p + 1] << 8);
					p += 2;
				} else {
					cnt = c & 0x7F;
				}
				NEED(2);
				size_t off = in[p] | (in[p + 1] << 8);
				p += 2;
				if (base + off >= n && cnt)
					goto bad;
				for (size_t i = 0; i < cnt; i++)
					EMIT(o[base + off + i]);
			}
		}
	}
	*out = o;
	*out_len = n;
	return 0;
bad:
	free(o);
	return -2;
#undef NEED
#undef EMIT
}

int pc1_decode(const uint8_t *data, size_t len, Pc1Image *img) {
	uint8_t *raw;
	size_t n;
	int rc = pc1_lz(data, len, &raw, &n);
	if (rc)
		return rc;
	if (n < 34 + 32000) {
		free(raw);
		return -2;
	}
	for (int i = 0; i < 16; i++) {
		unsigned w = (raw[2 + i * 2] << 8) | raw[3 + i * 2];
		img->palette[i][0] = (uint8_t)(((w >> 8) & 7) * 255 / 7);
		img->palette[i][1] = (uint8_t)(((w >> 4) & 7) * 255 / 7);
		img->palette[i][2] = (uint8_t)((w & 7) * 255 / 7);
	}
	for (int g = 0; g < 4000; g++) {
		const uint8_t *s = raw + 34 + g * 8;
		int y = g / 20, xg = g % 20;
		unsigned pl[4];
		for (int k = 0; k < 4; k++)
			pl[k] = (s[k * 2] << 8) | s[k * 2 + 1];
		for (int b = 0; b < 16; b++) {
			unsigned c = 0;
			for (int k = 0; k < 4; k++)
				c |= ((pl[k] >> (15 - b)) & 1) << k;
			img->pixels[y * PC1_WIDTH + xg * 16 + b] = (uint8_t)c;
		}
	}
	free(raw);
	return 0;
}
