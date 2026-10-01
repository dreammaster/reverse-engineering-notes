/* 8x8 text font (MM2.CH: 128 glyphs x 8 bytes, MSB = leftmost pixel) and text drawing on the
 * 320x200 canvas; the game's text grid is 40 columns x 25 rows of 8x8 cells. */
#ifndef MM2_TEXT_H
#define MM2_TEXT_H

#include "mm2_files.h"

typedef struct {
	uint8_t glyphs[128 * 8];
} Mm2Font;

int mm2_font_load(const Mm2Game *g, Mm2Font *f);
/* Draws one glyph at pixel (x, y); bg < 0 leaves the background untouched. */
void mm2_draw_char(uint8_t *canvas, const Mm2Font *f, int x, int y, int ch, int fg, int bg);
/* Draws a string starting at text cell (col, row). */
void mm2_draw_text(uint8_t *canvas, const Mm2Font *f, int col, int row, const char *s, int fg, int bg);

#endif
