#include "mm2_monpic.h"

#include <string.h>

static const uint8_t CODE_TO_COLOUR[16] = {0, 1, 2, 9, 6, 8, 10, 3, 4, 5, 7, 11, 12, 13, 14, 15};

static uint32_t rd32(const uint8_t *p) {
	return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

int mm2_monpic_load(const Mm2Game *g, int id, int cga, Mm2MonPic *p) {
	Mm2Blob d = mm2_read_file(g, cga ? "MONSTERS.4" : "MONSTERS.16");
	uint32_t off, nxt;
	int i;
	memset(p, 0, sizeof(*p));
	if (!d.data || id < 0 || id >= MM2_MONPIC_COUNT || d.size < 300) {
		mm2_blob_free(&d);
		return 0;
	}
	off = rd32(d.data + 4 * id);
	if (!off) {
		mm2_blob_free(&d);
		return 0;
	}
	nxt = (uint32_t)d.size;
	for (i = 0; i < MM2_MONPIC_COUNT; i++) {
		uint32_t o = rd32(d.data + 4 * i);
		if (o > off && o < nxt) nxt = o;
	}
	p->bank = mm2_lzw_blob(d.data + off, nxt - off);
	mm2_blob_free(&d);
	if (!p->bank.data) return 0;
	p->frames = p->bank.data[0] | (p->bank.data[1] << 8);
	p->cga = cga;
	return 1;
}

void mm2_monpic_free(Mm2MonPic *p) {
	mm2_blob_free(&p->bank);
	p->frames = 0;
}

static void paint(uint8_t *frame, const uint8_t *b, size_t len, unsigned off, int cga) {
	int x = b[off], y = b[off + 1], w = b[off + 2], h = b[off + 3];
	int px = x + 4, py = y + 6, col = 0, row = 0;
	size_t pos = off + 4;
	while (row < h && pos < len) {
		int v = b[pos++], n = (v >> 4) + 1, code = v & 15, k;
		for (k = 0; k < n && row < h; k++) {
			int fx = px + col, fy = py + row;
			int inside = fx >= 0 && fx < MM2_MONPIC_W && fy >= 0 && fy < MM2_MONPIC_H;
			if (cga) {
				if (code != 8 && inside) frame[fy * MM2_MONPIC_W + fx] = (uint8_t)(code & 3);
			} else if (code != 5 && inside) {
				frame[fy * MM2_MONPIC_W + fx] = CODE_TO_COLOUR[code];
			}
			col++;
			if (col == w) {
				col = 0;
				row++;
			}
		}
	}
}

void mm2_monpic_frame(const Mm2MonPic *p, int k, uint8_t background, uint8_t *out) {
	const uint8_t *b = p->bank.data;
	unsigned off0 = b[2] | (b[3] << 8);
	memset(out, background, MM2_MONPIC_W * MM2_MONPIC_H);
	paint(out, b, p->bank.size, off0, p->cga);
	if (k > 0 && k < p->frames)
		paint(out, b, p->bank.size, b[2 + 2 * k] | (b[3 + 2 * k] << 8), p->cga);
}
