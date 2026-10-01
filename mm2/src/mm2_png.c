#include "mm2_png.h"

#include <stdio.h>
#include <stdlib.h>

static uint32_t crc_table[256];

static void crc_init(void) {
	uint32_t n, k, c;
	if (crc_table[1]) return;
	for (n = 0; n < 256; n++) {
		c = n;
		for (k = 0; k < 8; k++)
			c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
		crc_table[n] = c;
	}
}

static uint32_t crc(uint32_t c, const uint8_t *p, size_t n) {
	size_t i;
	for (i = 0; i < n; i++)
		c = crc_table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
	return c;
}

static void be32(uint8_t *p, uint32_t v) {
	p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

static int chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
	uint8_t hdr[8], tail[4];
	uint32_t c;
	be32(hdr, len);
	hdr[4] = (uint8_t)type[0]; hdr[5] = (uint8_t)type[1]; hdr[6] = (uint8_t)type[2]; hdr[7] = (uint8_t)type[3];
	c = crc(0xFFFFFFFFu, hdr + 4, 4);
	if (len) c = crc(c, data, len);
	be32(tail, c ^ 0xFFFFFFFFu);
	return fwrite(hdr, 1, 8, f) == 8 && (!len || fwrite(data, 1, len, f) == len) && fwrite(tail, 1, 4, f) == 4;
}

int mm2_write_png(const char *path, const uint8_t *indexed, int w, int h, const uint32_t *palette, int n) {
	static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
	FILE *f = fopen(path, "wb");
	uint8_t ihdr[13], pal[768];
	size_t rawLen = (size_t)(w + 1) * h, pos = 0, zlen, i;
	uint8_t *raw, *z;
	uint32_t a = 1, b = 0;
	int ok;
	if (!f) return 0;
	crc_init();
	raw = (uint8_t *)malloc(rawLen);
	z = (uint8_t *)malloc(rawLen + rawLen / 65535 * 5 + 64);
	for (i = 0; i < (size_t)h; i++) {
		raw[i * (w + 1)] = 0;
		for (pos = 0; pos < (size_t)w; pos++)
			raw[i * (w + 1) + 1 + pos] = indexed[i * w + pos];
	}
	zlen = 0;
	z[zlen++] = 0x78; z[zlen++] = 0x01;
	for (pos = 0; pos < rawLen;) {
		size_t blk = rawLen - pos > 65535 ? 65535 : rawLen - pos;
		z[zlen++] = pos + blk >= rawLen;
		z[zlen++] = (uint8_t)blk; z[zlen++] = (uint8_t)(blk >> 8);
		z[zlen++] = (uint8_t)~blk; z[zlen++] = (uint8_t)(~blk >> 8);
		for (i = 0; i < blk; i++) {
			z[zlen++] = raw[pos + i];
			a = (a + raw[pos + i]) % 65521;
			b = (b + a) % 65521;
		}
		pos += blk;
	}
	be32(z + zlen, (b << 16) | a);
	zlen += 4;
	be32(ihdr, (uint32_t)w); be32(ihdr + 4, (uint32_t)h);
	ihdr[8] = 8; ihdr[9] = 3; ihdr[10] = ihdr[11] = ihdr[12] = 0;
	for (i = 0; i < (size_t)n && i < 256; i++) {
		pal[i * 3] = (uint8_t)(palette[i] >> 16); pal[i * 3 + 1] = (uint8_t)(palette[i] >> 8); pal[i * 3 + 2] = (uint8_t)palette[i];
	}
	ok = fwrite(sig, 1, 8, f) == 8 && chunk(f, "IHDR", ihdr, 13) && chunk(f, "PLTE", pal, (uint32_t)(n * 3)) &&
		 chunk(f, "IDAT", z, (uint32_t)zlen) && chunk(f, "IEND", 0, 0);
	free(raw);
	free(z);
	fclose(f);
	return ok;
}
