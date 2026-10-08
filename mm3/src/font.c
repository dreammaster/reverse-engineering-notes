#include "font.h"

#include <stdlib.h>
#include <string.h>

int mm3_font_load(Mm3Font *f, const Mm3Cc *cc) {
	size_t len;
	uint8_t *d = mm3_cc_read(cc, "FO.T", &len);
	if (!d || len < MM3_FONT_SIZE) {
		free(d);
		return -1;
	}
	for (int c = 0; c < 256; c++)
		for (int r = 0; r < 8; r++)
			f->rows[c][r] = (uint16_t)(d[c * 16 + r * 2] | (d[c * 16 + r * 2 + 1] << 8));
	memcpy(f->width, d + 0x1000, 256);
	free(d);
	return 0;
}

int mm3_font_draw_char(const Mm3Font *f, uint8_t *surf, int sw, int sh, int x, int y, unsigned ch, const uint8_t colors[3]) {
	unsigned base = ch & 0x7F;
	if (base == 'g' || base == 'p' || base == 'q' || base == 'y')
		y++;
	for (int r = 0; r < 8; r++) {
		unsigned w = f->rows[ch & 0xFF][r];
		int py = y + r;
		for (int i = 0; i < 8; i++, w >>= 2) {
			int px = x + i;
			unsigned v = w & 3;
			if (v && px >= 0 && px < sw && py >= 0 && py < sh)
				surf[py * sw + px] = colors[v - 1];
		}
	}
	return f->width[ch & 0xFF];
}

/* number of argument characters following a control code, or -1 for codes that are not skipped by count */
static int ctrl_args(unsigned c) {
	switch (c) {
	case 0x03: return 1;
	case 0x04: case 0x07: case 0x09: case 0x0B: return 3;
	case 0x0C: return 2;
	case 0x05: return 4;
	case 0x08: return 1;
	default: return 0;
	}
}

int mm3_font_draw_text(const Mm3Font *f, uint8_t *surf, int sw, int sh, int x, int y, const char *text, const uint8_t colors[3]) {
	unsigned alt = 0;
	int x0 = x;
	for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
		unsigned c = *p & 0x7F;
		if (c == 0x01) { alt = 0; continue; }
		if (c == 0x02) { alt = 0x80; continue; }
		if (c == 0x0D) { x = x0; y += alt ? 8 : 9; continue; }
		if (c == 0x06) c = ' ';
		if (c < 0x20) {
			for (int n = ctrl_args(c); n > 0 && p[1]; n--)
				p++;
			continue;
		}
		if (c == ' ') { x += alt ? 3 : 4; continue; }
		x += mm3_font_draw_char(f, surf, sw, sh, x, y, c | alt, colors);
	}
	return x;
}

int mm3_font_text_width(const Mm3Font *f, const char *text) {
	int x = 0, best = 0;
	unsigned alt = 0;
	for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
		unsigned c = *p & 0x7F;
		if (c == 0x01) { alt = 0; continue; }
		if (c == 0x02) { alt = 0x80; continue; }
		if (c == 0x0D) { if (x > best) best = x; x = 0; continue; }
		if (c == 0x06) c = ' ';
		if (c < 0x20) {
			for (int n = ctrl_args(c); n > 0 && p[1]; n--)
				p++;
			continue;
		}
		x += (c == ' ') ? (alt ? 3 : 4) : f->width[c | alt];
	}
	return x > best ? x : best;
}
