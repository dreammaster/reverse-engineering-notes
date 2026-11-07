#include "message.h"
#include "panel.h"
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
	MsgLayout l;
	message_layout(text, &l);
	int fill = (l.w * 6) / 8, rows = (6 * l.nlines) / 8;
	int x = 0x20 + 16 * slot, y = 0x14 + 16 * slot, yy = y;
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
