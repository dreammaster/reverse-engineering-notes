#include "mm2_gfx.h"

#include <stdlib.h>
#include <string.h>

const uint32_t MM2_EGA_PALETTE[16] = {
	0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
	0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF};

int mm2_bank_load(const Mm2Game *g, const char *file, int bpp, Mm2Bank *b) {
	memset(b, 0, sizeof(*b));
	b->raw = mm2_load_lzw_file(g, file);
	if (!b->raw.data || b->raw.size < 2) {
		mm2_blob_free(&b->raw);
		return 0;
	}
	b->bpp = bpp;
	b->count = b->raw.data[0] | (b->raw.data[1] << 8);
	b->cache = (Mm2Image *)calloc((size_t)b->count, sizeof(Mm2Image));
	return b->cache != NULL;
}

void mm2_bank_free(Mm2Bank *b) {
	int i;
	for (i = 0; b->cache && i < b->count; i++) {
		free(b->cache[i].pix);
		free(b->cache[i].mask);
	}
	free(b->cache);
	mm2_blob_free(&b->raw);
	memset(b, 0, sizeof(*b));
}

const Mm2Image *mm2_bank_image(Mm2Bank *b, int idx) {
	const uint8_t *d = b->raw.data;
	Mm2Image *im;
	unsigned io, mo;
	int w, h, per, rowb, x, y;
	if (idx < 0 || idx >= b->count) return NULL;
	im = &b->cache[idx];
	if (im->pix) return im;
	io = d[2 + 4 * idx] | (d[3 + 4 * idx] << 8);
	mo = d[4 + 4 * idx] | (d[5 + 4 * idx] << 8);
	w = d[io] | (d[io + 1] << 8);
	h = d[io + 2] | (d[io + 3] << 8);
	per = 8 / b->bpp;
	rowb = (w + per - 1) / per;
	im->w = w;
	im->h = h;
	im->pix = (uint8_t *)malloc((size_t)w * h);
	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++) {
			uint8_t byte = d[io + 4 + y * rowb + x / per];
			im->pix[y * w + x] = (byte >> (8 - b->bpp * (x % per + 1))) & ((1 << b->bpp) - 1);
		}
	if (mo) {
		int mrow = (w + 7) / 8;
		im->mask = (uint8_t *)malloc((size_t)w * h);
		for (y = 0; y < h; y++)
			for (x = 0; x < w; x++)
				im->mask[y * w + x] = (d[mo + y * mrow + x / 8] >> (7 - x % 8)) & 1;
	}
	return im;
}

void mm2_blit(uint8_t *canvas, const Mm2Image *img, int px, int py) {
	int x, y;
	if (!img) return;
	for (y = 0; y < img->h; y++) {
		int cy = py + y;
		if (cy < 0 || cy >= MM2_SCREEN_H) continue;
		for (x = 0; x < img->w; x++) {
			int cx = px + x;
			if (cx < 0 || cx >= MM2_SCREEN_W) continue;
			if (img->mask && !img->mask[y * img->w + x]) continue;
			canvas[cy * MM2_SCREEN_W + cx] = img->pix[y * img->w + x];
		}
	}
}
