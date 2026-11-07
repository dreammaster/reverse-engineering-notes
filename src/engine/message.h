/*
 * message.h - message scrolls (parchment boxes with wrapped text), docs/formats.md 6.7a, tools/message.py.
 * Layout (1000:5DCB) and box/text drawing (1000:8A87, 1000:8714) verified against the original's screen.
 */
#ifndef CADAVER_MESSAGE_H
#define CADAVER_MESSAGE_H

#include <stddef.h>
#include <stdint.h>

#define MSG_MAX_LINES 16
#define MSG_MAX_LEN 40            /* longest line kept */
#define MSG_STEPS 75              /* a scroll stays 75 game steps (150 ticks) */

typedef struct {
	int w;                         /* columns the text was wrapped to */
	int nlines;
	char lines[MSG_MAX_LINES][MSG_MAX_LEN];
} MsgLayout;

/* width in characters for a text of n characters (starts at cl = 12, grows until it fits) */
int message_columns(int n, int cl);
/* wrap `text` (a trailing space is added like the game's string buffer) */
void message_layout(const char *text, MsgLayout *out);
/* Draw slot 0..3 onto a 320x200 colour-index canvas. tiles = ANDATA block 2, font = ANDATA block 3. rnd(user) -> 0..7 picks the ragged edge tiles. */
void message_draw(uint8_t *canvas, const char *text, int slot, const uint8_t *tiles, size_t tiles_len, const uint8_t *font, size_t font_len,
                  int (*rnd)(void *), void *user);
/* Screen rectangle of a message in slot `slot` with this layout: x, y, w, h in pixels. */
void message_rect(const MsgLayout *l, int slot, int *x, int *y, int *w, int *h);

#endif
