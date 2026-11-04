/*
 * intro.h - the title scroll of INTRO.EXE: LOADING2.PC1 backdrop, FONT2.SPR text, FLAME.DAT candle flames.
 * Verified pixel-exact against INTRO.EXE running in tools/dosemu (see tools/intro.py, docs/formats.md).
 */
#ifndef CADAVER_INTRO_H
#define CADAVER_INTRO_H

#include "pc1.h"
#include "screen.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint8_t glyph[31][36];     /* 6x6 opaque glyphs: A-Z . , ? - ' */
} Font;

typedef struct {
	uint8_t frame[6][24 * 16]; /* 16x24, opaque (0 = black) */
} Flames;

int font_load(const uint8_t *data, size_t len, Font *f);
int flames_load(const uint8_t *data, size_t len, Flames *fl);

/* Draw `text` (case-insensitive; unknown characters and spaces draw nothing) with a 6 px advance. */
void font_draw_text(Screen *scr, const Font *f, int x, int y, const char *text);

/* Render the title page. `payoff` selects the publisher line; `left_flame` is the left flame frame (the right
 * flame is (left + 2) mod 6). Copies the PC1 palette. */
void intro_render_title(Screen *scr, const Pc1Image *bg, const Font *f, const Flames *fl, int payoff, int left_flame);

#endif
