#include "panel.h"

#include <stdio.h>
#include <string.h>

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

/* ---- status text ---- */
static const uint8_t status_punct[32] = {0x00, 0x2a, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x28, 0x24, 0x26, 0x1a,
                                         0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x00, 0x00, 0x2b, 0x2c, 0x2d, 0x29, 0x00};

int status_glyph_index(char ch) {
	unsigned c = (unsigned char)ch;
	if (c == 0x20)
		return -1;
	if (c >= 0x41)
		return (int)(c - 0x41);
	if (c < 0x21)
		return -1;
	return status_punct[c - 0x21];
}

void status_draw_string(uint8_t *canvas, int x, int y, const char *s, const uint8_t *font, size_t font_len) {
	for (; *s; s++, x += STATUS_ADVANCE) {
		int g = status_glyph_index(*s);
		if (g < 0 || (size_t)(g + 1) * 24 > font_len)
			continue;
		for (int r = 0; r < STATUS_GLYPH_H; r++) {
			const uint8_t *p = font + g * 24 + r * 4;
			for (int b = 0; b < 8; b++) {
				unsigned col = 0;
				for (int k = 0; k < 4; k++)
					col |= (unsigned)((p[k] >> (7 - b)) & 1) << k;
				int xx = x + b, yy = y + r;
				if (col && xx >= 0 && xx < 320 && yy >= 0 && yy < 200)
					canvas[yy * 320 + xx] = (uint8_t)col;
			}
		}
	}
}

void status_draw_centred(uint8_t *canvas, int bx, int by, int bw, const char *s, const uint8_t *font, size_t font_len) {
	status_draw_string(canvas, bx + ((bw - STATUS_ADVANCE * (int)strlen(s)) >> 1), by, s, font, font_len);
}

int status_name_table(const uint8_t *mp1, size_t len, uint16_t table[20]) {
	if (len < 1790)
		return -1;
	const uint8_t *p = mp1 + len - 1790 + 1630;
	for (int i = 0; i < 20; i++)
		table[i] = (uint16_t)be16(p + 2 * i);
	return 0;
}

size_t status_room_name(const uint8_t *rec, size_t rec_len, const uint16_t table[20], char *const *strings, int count,
                        char *out, size_t outsz) {
	size_t n = 0;
	if (!outsz)
		return 0;
	out[0] = 0;
	if (rec_len < 0x1B)
		return 0;
	unsigned pre = be16(rec + 0x18);
	if (!(pre & 0x8000) && (int)pre < count)
		for (const char *s = strings[pre]; *s && n + 1 < outsz; s++)
			out[n++] = *s;
	if (n + 1 < outsz)
		out[n++] = ' ';
	if (!(rec[0x1A] & 0x80) && rec[0x1A] < 20 && table[rec[0x1A]] < count)
		for (const char *s = strings[table[rec[0x1A]]]; *s && n + 1 < outsz; s++)
			out[n++] = *s;
	if (n > STATUS_NAME_MAX)
		n = STATUS_NAME_MAX;
	out[n] = 0;
	return n;
}

void status_draw(uint8_t *canvas, const uint8_t *font, size_t font_len, const char *room_name, int day) {
	char buf[32];
	for (int r = 0; r < 7; r++)
		memset(canvas + (157 + r) * 320 + 252, 0, 60);
	snprintf(buf, sizeof(buf), "DAY %d", day);
	status_draw_centred(canvas, 252, 157, 60, buf, font, font_len);
	status_draw_centred(canvas, 202, 186, 108, room_name, font, font_len);
}

/* ---- verb icons ---- */
static const int verb_slots[VERB_SLOTS][2] = {{9, 145}, {36, 152}, {9, 169}, {36, 177}, {63, 175}, {90, 177}};
static const uint8_t class_verb[10] = {0x00, 0x04, 0x09, 0xFF, 0x07, 0x03, 0x08, 0xFF, 0xFF, 0xFF};

int verbs_list(const uint8_t hdr[32], int flags2, int ds703, uint8_t out[VERB_SLOTS]) {
	int n = 0;
	int h12 = hdr[12], h16 = hdr[0x16], h17 = hdr[0x17], h1d = hdr[0x1D];
	uint8_t all[8];
	int m = 0;
	if ((h12 & 2) && !(flags2 & 4))
		all[m++] = 2;
	if (h17 && !(flags2 & 0x40)) {
		int c = h17 < 10 ? class_verb[h17] : 0xFF;
		if (!(c & 0x80))
			all[m++] = (uint8_t)c;
	}
	if (h1d == 0x12)
		all[m++] = 0x0E;
	if ((h12 & 1) && !(flags2 & 4) && !(ds703 & 0x20))
		all[m++] = 0x0A;
	if (h17 != 6 && !(h16 & 0x80))
		all[m++] = 0x0B;
	all[m++] = 6;
	for (int i = 0; i < m && n < VERB_SLOTS; i++)
		out[n++] = all[i];
	return n;
}

static void icon_draw(uint8_t *canvas, int x, int y, const uint8_t *icons, size_t len, int n) {
	if ((size_t)(n + 1) * 112 > len)
		return;
	for (int r = 0; r < 14; r++) {
		const uint8_t *p = icons + n * 112 + r * 8;
		for (int b = 0; b < 16; b++) {
			unsigned col = 0;
			for (int k = 0; k < 4; k++)
				col |= (unsigned)((be16(p + 2 * k) >> (15 - b)) & 1) << k;
			if (col && x + b < 320 && y + r < 200)
				canvas[(y + r) * 320 + x + b] = (uint8_t)col;
		}
	}
}

void verbs_draw(uint8_t *canvas, const uint8_t *verbs, int n, const uint8_t *icons, size_t icons_len) {
	for (int i = 0; i < VERB_SLOTS; i++)
		icon_draw(canvas, verb_slots[i][0], verb_slots[i][1], icons, icons_len, i < n ? verbs[i] : 0);
}
