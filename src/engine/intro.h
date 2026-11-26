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

/* The 6x6 glyph of a character (case-insensitive), NULL for characters the font does not have. */
const uint8_t *font_glyph(const Font *f, char ch);
void intro_draw_flame(Screen *scr, const uint8_t *frame, int x, int y);

/* Render the title page. `payoff` selects the publisher line; `left_flame` is the left flame frame (the right
 * flame is (left + 2) mod 6). Copies the PC1 palette. */
void intro_render_title(Screen *scr, const Pc1Image *bg, const Font *f, const Flames *fl, int payoff, int left_flame);


/* ---- the rolling story of INTRO.EXE (docs/formats.md section 4) ---- */
typedef struct IntroScript IntroScript;
/* `exe` = the packed or plain INTRO.EXE. Runs its start-up code on the interpreter, cuts the text script (title page, story, credits) out of the image. 0 on success. */
IntroScript *introscript_load(const uint8_t *exe, size_t len);
void introscript_free(IntroScript *s);
/* The screen `t` seconds after the roll started: background, the text window (x 64..255, y 40..159) scrolling up 6 pixels per line at the 70 Hz retrace, a
 * page break (!) waiting 540 timer ticks of 50.16 Hz, the flames advancing once per retrace; the script loops. */
void intro_render_roll(Screen *scr, const Pc1Image *bg, const Font *f, const Flames *fl, const IntroScript *s, double t);

#endif
