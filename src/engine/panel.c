#include "panel.h"

static unsigned be16(const uint8_t *p) { return (unsigned)((p[0] << 8) | p[1]); }

int andata_block(const uint8_t *data, size_t len, int index, const uint8_t **block, size_t *blen) {
	if (len < 2)
		return -1;
	unsigned n = be16(data);
	if (index < 0 || (unsigned)index >= n || len < 2 + 2 * n)
		return -1;
	size_t pos = 2 + 2 * (size_t)n;
	for (int i = 0; i < index; i++)
		pos += be16(data + 2 + 2 * i);
	size_t sz = be16(data + 2 + 2 * (size_t)index);
	if (pos + sz > len)
		return -1;
	*block = data + pos;
	*blen = sz;
	return 0;
}

static void put_group(uint8_t *canvas, int row, int g, const uint8_t *src, int keep) {
	unsigned ws[4];
	for (int p = 0; p < 4; p++)
		ws[p] = be16(src + 2 * p);
	for (int b = 0; b < 16; b++) {
		unsigned col = 0;
		for (int p = 0; p < 4; p++)
			col |= ((ws[p] >> (15 - b)) & 1u) << p;
		uint8_t *o = &canvas[row * 320 + g * 16 + b];
		if (keep < 0)
			*o = (uint8_t)col;
		else
			*o = (uint8_t)(((keep >> (15 - b)) & 1 ? *o : 0) | col);
	}
}

int panel_apply_wings(const uint8_t *b1, size_t len, uint8_t *canvas) {
	for (int i = 0; i < 320 * 200; i++)
		if (canvas[i] == 0xFF)
			canvas[i] = 0;
	size_t si = 0;
	int row = 115, cl = -1;
	unsigned bp = 0xFFFF, bx = 0xFFFF;
	while (row < 200) {
		size_t need = 8 * (size_t)(2 * (cl + 1) + 2);
		if (si + need > len)
			return -1;
		for (int g = 0; g <= cl; g++, si += 8)
			put_group(canvas, row, g, b1 + si, -1);
		put_group(canvas, row, cl + 1, b1 + si, (int)bx); si += 8;
		put_group(canvas, row, 18 - cl, b1 + si, (int)bp); si += 8;
		for (int g = 0; g <= cl; g++, si += 8)
			put_group(canvas, row, 19 - cl + g, b1 + si, -1);
		row++;
		bp = (bp << 2) & 0xFFFF;
		bx >>= 2;
		if (bx == 0) {
			cl++;
			bp = bx = 0xFFFF;
		}
	}
	return 0;
}
