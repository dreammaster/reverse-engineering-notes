#include "intro.h"

#include <string.h>

static const char glyph_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.,?-'";

int font_load(const uint8_t *data, size_t len, Font *f) {
	if (len < 31 * 32)
		return -1;
	for (int g = 0; g < 31; g++)
		for (int y = 0; y < 6; y++)
			for (int x = 0; x < 6; x++) {
				const uint8_t *pl = data + g * 32 + y * 4;
				int c = 0;
				for (int p = 0; p < 4; p++)
					c |= ((pl[p] >> (7 - x)) & 1) << p;
				f->glyph[g][y * 6 + x] = (uint8_t)c;
			}
	return 0;
}

int flames_load(const uint8_t *data, size_t len, Flames *fl) {
	if (len < 6 * 192)
		return -1;
	for (int k = 0; k < 6; k++)
		for (int y = 0; y < 24; y++)
			for (int g = 0; g < 1; g++) {
				const uint8_t *w = data + k * 192 + y * 8;   /* four big-endian plane words */
				for (int b = 0; b < 16; b++) {
					int c = 0;
					for (int p = 0; p < 4; p++) {
						unsigned word = (unsigned)((w[p * 2] << 8) | w[p * 2 + 1]);
						c |= (int)((word >> (15 - b)) & 1) << p;
					}
					fl->frame[k][y * 16 + b] = (uint8_t)c;
				}
			}
	return 0;
}

void font_draw_text(Screen *scr, const Font *f, int x, int y, const char *text) {
	for (int i = 0; text[i]; i++) {
		char ch = text[i];
		if (ch >= 'a' && ch <= 'z')
			ch = (char)(ch - 'a' + 'A');
		const char *p = ch ? strchr(glyph_chars, ch) : NULL;
		if (!p)
			continue;
		const uint8_t *g = f->glyph[p - glyph_chars];
		for (int gy = 0; gy < 6; gy++)
			for (int gx = 0; gx < 6; gx++) {
				int px = x + 6 * i + gx, py = y + gy;
				if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H)
					scr->pixels[py * SCREEN_W + px] = g[gy * 6 + gx];
			}
	}
}

static void draw_flame(Screen *scr, const uint8_t *fr, int x, int y) {
	for (int fy = 0; fy < 24; fy++)
		for (int fx = 0; fx < 16; fx++)
			scr->pixels[(y + fy) * SCREEN_W + x + fx] = fr[fy * 16 + fx];
}

void intro_render_title(Screen *scr, const Pc1Image *bg, const Font *f, const Flames *fl, int payoff, int left_flame) {
	memset(scr, 0, sizeof(*scr));
	memcpy(scr->pixels, bg->pixels, sizeof(bg->pixels));
	for (int i = 0; i < 16; i++)
		memcpy(scr->palette[i], bg->palette[i], 3);
	font_draw_text(scr, f, 68, 76, "          cadaver");
	font_draw_text(scr, f, 68, 82, "          -------");
	font_draw_text(scr, f, 68, 100, "copyright nineteen ninety-one");
	font_draw_text(scr, f, 68, 106, "    the bitmap brothers");
	font_draw_text(scr, f, 68, 118, "          -------");
	font_draw_text(scr, f, 68, 130, payoff ? "   published by renagade" : "  published by imageworks");
	int l = ((left_flame % 6) + 6) % 6;
	draw_flame(scr, fl->frame[l], 14, 20);
	draw_flame(scr, fl->frame[(l + 2) % 6], 296, 20);
}
