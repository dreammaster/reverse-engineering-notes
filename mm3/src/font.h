/* The text font: .CC member FO.T (id 8D92h, 4352 bytes), used by the video module's text printer
 * (vdrv_printTextEngine / sub_61B42).  256 glyphs of 8 rows x u16 (2 bits per pixel, leftmost pixel = low bits),
 * glyph = char | 80h for the alternate font, then a 256-byte advance-width table at +1000h (+16 unused bytes). */
#ifndef MM3_FONT_H
#define MM3_FONT_H

#include <stddef.h>
#include <stdint.h>

#include "cc.h"

#define MM3_FONT_SIZE 0x1100

typedef struct {
	uint16_t rows[256][8];
	uint8_t width[256];
} Mm3Font;

int mm3_font_load(Mm3Font *f, const Mm3Cc *cc);

/* Draw one glyph with its top-left at (x, y); `colors` = palette indices for pixel values 1-3 (0 is transparent).
 * g, p, q and y are drawn one row lower, as the original does.  Returns the advance in pixels. */
int mm3_font_draw_char(const Mm3Font *f, uint8_t *surf, int surf_w, int surf_h, int x, int y, unsigned ch, const uint8_t colors[3]);

/* Draw a string.  Understands: 01h/02h normal/alternate font, 06h (space), 0Dh (new line; 9 rows, 8 for the alternate font),
 * and skips the argument digits of the other control codes (03h+letter, 07h/09h/0Bh+3 digits, 0Ch+2 digits, 04h+3 digits);
 * colours and alignment are not interpreted yet.  Returns the x position after the last character. */
int mm3_font_draw_text(const Mm3Font *f, uint8_t *surf, int surf_w, int surf_h, int x, int y, const char *text, const uint8_t colors[3]);

/* Width in pixels of the text on one line. */
int mm3_font_text_width(const Mm3Font *f, const char *text);

#endif
