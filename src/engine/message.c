#include "message.h"
#include "panel.h"
#include <stdio.h>
#include <string.h>

static const uint8_t TOP[2][3] = {{0, 1, 2}, {5, 0x0E, 0x0C}};
static const uint8_t LEFT[8] = {6, 7, 7, 8, 8, 8, 8, 8};
static const uint8_t RIGHT[8] = {0x0B, 0x0A, 0x0A, 9, 9, 9, 9, 9};
static const uint8_t BOTTOM[2][3] = {{0x10, 0x0F, 0x11}, {3, 1, 4}};
#define FILL 0x0D

int message_columns(int n, int cl) {
	int w = cl, dx = n - 4 * w;
	if (dx >= 0)
		do {
			w++;
			dx -= w;
		} while (dx > 0);
	return w;
}

void message_layout(const char *text0, MsgLayout *out) {
	char text[512];
	size_t n0 = strlen(text0);
	if (n0 > sizeof(text) - 2)
		n0 = sizeof(text) - 2;
	memcpy(text, text0, n0);
	text[n0] = ' ';
	text[n0 + 1] = 0;
	int n = (int)n0 + 1;
	int w = message_columns(n, 12);
	out->w = w;
	out->nlines = 0;
	int pos = 0;
	for (;;) {
		int bx = 0, bp = w, scan = pos, done = 0;
		for (;;) {
			while (scan < n && text[scan] != ' ') {
				scan++;
				bx++;
			}
			if (scan >= n)
				done = 1;
			else
				scan++;
			if (bx > w) {
				done = 0;
				break;
			}
			bp = bx;
			bx++;
			if (done)
				break;
		}
		if (out->nlines < MSG_MAX_LINES) {
			int len = bp < MSG_MAX_LEN - 1 ? bp : MSG_MAX_LEN - 1;
			memcpy(out->lines[out->nlines], text + pos, (size_t)len);
			out->lines[out->nlines][len] = 0;
			out->nlines++;
		}
		pos += bp;
		if (pos < n && text[pos] == ' ')
			pos++;
		if (done || pos >= n)
			return;
	}
}

static void tile_blit(uint8_t *canvas, int x, int y, const uint8_t *tiles, size_t len, int idx) {
	if ((size_t)(idx + 1) * 32 > len)
		return;
	const uint8_t *t = tiles + idx * 32;
	for (int r = 0; r < 8; r++)
		for (int b = 0; b < 8; b++) {
			unsigned col = 0;
			for (int k = 0; k < 4; k++)
				col |= (unsigned)((t[r * 4 + k] >> (7 - b)) & 1) << k;
			if (x + b >= 0 && x + b < 320 && y + r >= 0 && y + r < 200)
				canvas[(y + r) * 320 + x + b] = (uint8_t)col;
		}
}

void message_rect(const MsgLayout *l, int slot, int *x, int *y, int *w, int *h) {
	int fill = (l->w * 6) / 8, rows = (6 * l->nlines) / 8;
	*x = 0x20 + 16 * slot;
	*y = 0x14 + 16 * slot;
	*w = 8 * (fill + 2);
	*h = 8 * (rows + 4);
}

void message_draw(uint8_t *canvas, const char *text, int slot, const uint8_t *tiles, size_t tiles_len, const uint8_t *font, size_t font_len,
                  int (*rnd)(void *), void *user) {
	message_draw_at(canvas, text, 0x20 + 16 * slot, 0x14 + 16 * slot, tiles, tiles_len, font, font_len, rnd, user);
}

void message_draw_at(uint8_t *canvas, const char *text, int x, int y, const uint8_t *tiles, size_t tiles_len, const uint8_t *font, size_t font_len,
                     int (*rnd)(void *), void *user) {
	MsgLayout l;
	message_layout(text, &l);
	int fill = (l.w * 6) / 8, rows = (6 * l.nlines) / 8;
	int yy = y;
	for (int s = 0; s < 2; s++) {
		tile_blit(canvas, x, yy, tiles, tiles_len, TOP[s][0]);
		for (int i = 0; i < fill; i++)
			tile_blit(canvas, x + 8 + 8 * i, yy, tiles, tiles_len, TOP[s][1]);
		tile_blit(canvas, x + 8 + 8 * fill, yy, tiles, tiles_len, TOP[s][2]);
		yy += 8;
	}
	for (int r = 0; r < rows; r++) {
		int a = rnd ? rnd(user) & 7 : 0;
		tile_blit(canvas, x, yy, tiles, tiles_len, LEFT[a]);
		for (int i = 0; i < fill; i++)
			tile_blit(canvas, x + 8 + 8 * i, yy, tiles, tiles_len, FILL);
		a = rnd ? rnd(user) & 7 : 0;
		tile_blit(canvas, x + 8 + 8 * fill, yy, tiles, tiles_len, RIGHT[a]);
		yy += 8;
	}
	for (int s = 0; s < 2; s++) {
		tile_blit(canvas, x, yy, tiles, tiles_len, BOTTOM[s][0]);
		for (int i = 0; i < fill; i++)
			tile_blit(canvas, x + 8 + 8 * i, yy, tiles, tiles_len, BOTTOM[s][1]);
		tile_blit(canvas, x + 8 + 8 * fill, yy, tiles, tiles_len, BOTTOM[s][2]);
		yy += 8;
	}
	int width = 8 * (fill + 2), ty = y + 13;
	for (int i = 0; i < l.nlines; i++, ty += 6) {
		int len = (int)strlen(l.lines[i]);
		int xx = x + ((width - 6 * len) >> 1);
		for (const char *c = l.lines[i]; *c; c++, xx += 6) {
			int g = status_glyph_index(*c);
			if (g < 0 || (size_t)(g + 1) * 20 > font_len)
				continue;
			for (int r = 0; r < 5; r++)
				for (int b = 0; b < 8; b++) {
					unsigned col = 0;
					for (int k = 0; k < 4; k++)
						col |= (unsigned)((font[g * 20 + r * 4 + k] >> (7 - b)) & 1) << k;
					if (col && xx + b >= 0 && xx + b < 320 && ty + r >= 0 && ty + r < 200)
						canvas[(ty + r) * 320 + xx + b] = (uint8_t)col;
				}
		}
	}
}

void message_text(uint8_t *canvas, int xx, int y, const char *s, const uint8_t *font, size_t font_len) {
	for (; *s; s++, xx += 6) {
		int g = status_glyph_index(*s);
		if (g < 0 || (size_t)(g + 1) * 20 > font_len)
			continue;
		for (int r = 0; r < 5; r++)
			for (int b = 0; b < 8; b++) {
				unsigned col = 0;
				for (int k = 0; k < 4; k++)
					col |= (unsigned)((font[g * 20 + r * 4 + k] >> (7 - b)) & 1) << k;
				if (col && xx + b >= 0 && xx + b < 320 && y + r >= 0 && y + r < 200)
					canvas[(y + r) * 320 + xx + b] = (uint8_t)col;
			}
	}
}

int rank_table_load(const uint8_t *mp1, size_t len, RankTable *rt) {
	if (len < 120)
		return -1;
	const uint8_t *p = mp1 + len - 120;
	for (int i = 0; i < 20; i++) {
		rt->id[i] = (uint16_t)((p[6 * i] << 8) | p[6 * i + 1]);
		rt->thr[i] = ((uint32_t)((p[6 * i + 2] << 8) | p[6 * i + 3]) << 16) | (uint32_t)((p[6 * i + 4] << 8) | p[6 * i + 5]);
	}
	return 0;
}

int rank_string_id(const RankTable *rt, uint32_t score) {
	for (int i = 0; i < 20; i++)
		if (score <= rt->thr[i])
			return rt->id[i];
	return rt->id[19];
}

int complete_percent(unsigned rooms_visited, unsigned six_fa, unsigned tad_w5) {
	int d = (int)six_fa - (int)tad_w5;
	return d > 0 ? (int)(((0x6400u / (unsigned)d) * rooms_visited) >> 8) : 0;
}

void status_dialog_draw(uint8_t *canvas, const uint8_t *tiles, size_t tiles_len, const uint8_t *font, size_t font_len, const char *labels, int energy, int energy_max,
                        uint32_t money, uint32_t score, int level, int percent, const char *rank_title, int (*rnd)(void *), void *user) {
	/* box: 16 fill columns, 4 middle rows at (64,10) */
	int x = 64, y = 10, fill = 16, rows = 4, yy = y;
	for (int s2 = 0; s2 < 2; s2++) {
		tile_blit(canvas, x, yy, tiles, tiles_len, TOP[s2][0]);
		for (int i = 0; i < fill; i++) tile_blit(canvas, x + 8 + 8 * i, yy, tiles, tiles_len, TOP[s2][1]);
		tile_blit(canvas, x + 8 + 8 * fill, yy, tiles, tiles_len, TOP[s2][2]);
		yy += 8;
	}
	for (int r = 0; r < rows; r++) {
		tile_blit(canvas, x, yy, tiles, tiles_len, LEFT[(rnd ? rnd(user) : 0) & 7]);
		for (int i = 0; i < fill; i++) tile_blit(canvas, x + 8 + 8 * i, yy, tiles, tiles_len, FILL);
		tile_blit(canvas, x + 8 + 8 * fill, yy, tiles, tiles_len, RIGHT[(rnd ? rnd(user) : 0) & 7]);
		yy += 8;
	}
	for (int s2 = 0; s2 < 2; s2++) {
		tile_blit(canvas, x, yy, tiles, tiles_len, BOTTOM[s2][0]);
		for (int i = 0; i < fill; i++) tile_blit(canvas, x + 8 + 8 * i, yy, tiles, tiles_len, BOTTOM[s2][1]);
		tile_blit(canvas, x + 8 + 8 * fill, yy, tiles, tiles_len, BOTTOM[s2][2]);
		yy += 8;
	}
	int ty = y + 13;
	char line[48];
	const char *p = labels;
	while (*p) {
		size_t n = 0;
		while (p[n] && p[n] != '\r' && n < sizeof(line) - 1) { line[n] = p[n]; n++; }
		line[n] = 0;
		message_text(canvas, 78, ty, line, font, font_len);
		ty += 6;
		p += n;
		if (*p == '\r') p++;
		else if (*p) p++;
	}
	char v[6][40];
	snprintf(v[0], 40, "%d/%d", energy, energy_max);
	snprintf(v[1], 40, "%u", money);
	snprintf(v[2], 40, "%u", score);
	snprintf(v[3], 40, "%d", level);
	snprintf(v[4], 40, "%d%%", percent);
	snprintf(v[5], 40, "%s", rank_title);
	ty = y + 13;
	for (int i = 0; i < 6; i++, ty += 6)
		message_text(canvas, 134, ty, v[i], font, font_len);
}

void message_box(uint8_t *canvas, int x, int y, int fill, int rows, const uint8_t *tiles, size_t tiles_len, int (*rnd)(void *), void *user) {
	int yy = y;
	for (int s = 0; s < 2; s++, yy += 8) {
		tile_blit(canvas, x, yy, tiles, tiles_len, TOP[s][0]);
		for (int i = 0; i < fill; i++) tile_blit(canvas, x + 8 + 8 * i, yy, tiles, tiles_len, TOP[s][1]);
		tile_blit(canvas, x + 8 + 8 * fill, yy, tiles, tiles_len, TOP[s][2]);
	}
	for (int r = 0; r < rows; r++, yy += 8) {
		tile_blit(canvas, x, yy, tiles, tiles_len, LEFT[rnd ? rnd(user) & 7 : 0]);
		for (int i = 0; i < fill; i++) tile_blit(canvas, x + 8 + 8 * i, yy, tiles, tiles_len, FILL);
		tile_blit(canvas, x + 8 + 8 * fill, yy, tiles, tiles_len, RIGHT[rnd ? rnd(user) & 7 : 0]);
	}
	for (int s = 0; s < 2; s++, yy += 8) {
		tile_blit(canvas, x, yy, tiles, tiles_len, BOTTOM[s][0]);
		for (int i = 0; i < fill; i++) tile_blit(canvas, x + 8 + 8 * i, yy, tiles, tiles_len, BOTTOM[s][1]);
		tile_blit(canvas, x + 8 + 8 * fill, yy, tiles, tiles_len, BOTTOM[s][2]);
	}
}
