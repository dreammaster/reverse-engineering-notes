#ifndef YENDOR23_FONT_H
#define YENDOR23_FONT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * The bitmap fonts: writeChar (yendor2.asm:46473, yendor3.asm identical). A glyph is 6 bytes, one per pixel row, bit 7 the
 * leftmost of 6 pixels (bits 1-0 unused); glyph index = character - 0x20 (96 glyphs, ' ' .. DEL), and the pen advances 6
 * pixels. There are four fonts, picked by `fontOffset` (0, 2, 4, 6 -> font 0-3); font 0 is the normal upper-case text (lower
 * case holds symbols), fonts 1-3 are the pictographic scripts that DrawIndentedTextColumn substitutes for words the reader's
 * Linguistics cannot read (document.h). The two games' tables are identical except for the thinner ':' and ';' of Chapter 3
 * (ida_scripts/dump_fonts.py). Drawing is `FontOpaque` (off pixels take the background colour) or `FontTransparent` (the
 * original's _font_bgTransparent != 0: they are skipped).
 */
enum { FontCount = 4, FontCharFirst = 0x20, FontCharCount = 96, FontGlyphSize = 6, FontAdvance = 6, FontHeight = 6 };

typedef enum { FontOpaque, FontTransparent } FontBackground;

/* The 6 row bytes of `ch` in `font` (0-3), or NULL if out of range. */
const uint8_t *fontGlyph(GameKind game, unsigned font, unsigned ch);

/*
 * Draws one character into a `stride`-wide 8-bit surface with its top-left at (x, y) and returns the new pen x. No clipping
 * beyond the caller's surface: (x, y) must leave room for 6 x 6 pixels.
 */
int fontDrawChar(GameKind game, unsigned font, uint8_t *surface, size_t stride, int x, int y, unsigned ch, uint8_t foreground,
                 uint8_t background, FontBackground mode);

/* Draws a NUL-terminated string; returns the pen x after it. */
int fontDrawString(GameKind game, unsigned font, uint8_t *surface, size_t stride, int x, int y, const char *text, uint8_t foreground,
                   uint8_t background, FontBackground mode);

#endif
