#include "mm2_text.h"
#include "mm2_gfx.h"

#include <string.h>

int mm2_font_load(const Mm2Game *g, Mm2Font *f) {
	Mm2Blob b = mm2_read_file(g, "MM2.CH");
	if (!b.data || b.size < sizeof(f->glyphs)) {
		mm2_blob_free(&b);
		return 0;
	}
	memcpy(f->glyphs, b.data, sizeof(f->glyphs));
	mm2_blob_free(&b);
	return 1;
}

void mm2_draw_char(uint8_t *canvas, const Mm2Font *f, int x, int y, int ch, int fg, int bg) {
	const uint8_t *gl = f->glyphs + (ch & 0x7F) * 8;
	int row, col;
	for (row = 0; row < 8; row++)
		for (col = 0; col < 8; col++) {
			int px = x + col, py = y + row;
			int on = gl[row] & (0x80 >> col);
			if (px < 0 || px >= MM2_SCREEN_W || py < 0 || py >= MM2_SCREEN_H) continue;
			if (on)
				canvas[py * MM2_SCREEN_W + px] = (uint8_t)fg;
			else if (bg >= 0)
				canvas[py * MM2_SCREEN_W + px] = (uint8_t)bg;
		}
}

void mm2_draw_text(uint8_t *canvas, const Mm2Font *f, int col, int row, const char *s, int fg, int bg) {
	for (; *s; s++, col++)
		mm2_draw_char(canvas, f, col * 8, row * 8, (unsigned char)*s, fg, bg);
}
