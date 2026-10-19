/* The text engine and windows of the original video module (MM3.CC member 8F99h): vdrv_printTextEngine, vdrv_1E_openWindow and
 * vdrv_06_closeWindows, ported to C from the disassembly and checked against the original module running in the emulator
 * (tools/mm3_vdrv_oracle.py, tests/test_ui_text.py).
 *
 * Text strings use the control codes of docs/view.md:
 *   01 / 02            normal / alternate font
 *   03 + letter        c centre, r right, l left alignment; t / f (f: stop at the end of the window instead of scrolling);
 *                      d absolute positions for this call; q k b: screen-update region flags (no pixel effect here); m + digit:
 *                      window background pattern (not implemented)
 *   04 + 3 digits      filled bar of that width, 9 rows high (8 in the alternate font), pattern = the fill word
 *   05 + 4 hex digits  draw list (not handled here, see view_glue.c)
 *   06                 a space glyph
 *   07 + 3 digits      fill colour (both bytes of the fill word)
 *   08 + char          overlay glyph: steps back one glyph and draws it in the fill colour (outline effect)
 *   09 + 3 digits      column;  0B + 3 digits  row  (relative to the text area unless 'd')
 *   0A                 new line (scrolls the text area when it is full)
 *   0C + 2 digits      text colour index (colour table at module offset 0B78h)
 *   0D                 clear the text area and go home
 */
#ifndef MM3_UI_TEXT_H
#define MM3_UI_TEXT_H

#include <stddef.h>
#include <stdint.h>

#include "cc.h"
#include "font.h"

#define MM3_UI_W 320
#define MM3_UI_H 200
#define MM3_UI_MAX_WINDOWS 7

typedef struct {
	uint16_t w02;                 /* 0A02h: unknown, saved with the window */
	uint16_t x, y, width, height; /* the window rectangle (0A04h-0A0Ah) */
	uint16_t colour;              /* text colour index (0A0Ch) */
	uint16_t fill;                /* fill pattern word, two pixels (0A0Eh) */
	uint16_t cx, cy;              /* text cursor (0A10h, 0A12h) */
	uint8_t align;                /* 0 left, 1 right, 2 centre (0A14h) */
	uint8_t stop_flag;            /* 0 or 80h: 'f' = do not scroll, stop (0A15h) */
	uint16_t left, right, top, bottom; /* text area (0A16h-0A1Ch) */
	uint16_t default_colour;      /* 0A1Eh */
	uint16_t default_fill;        /* 0A20h */
} Mm3Window;

typedef struct Mm3Ui {
	uint8_t *screen;              /* segment A000h: the visible 320x200 pixels are the first 64000 bytes (owned unless set by the host) */
	uint8_t *owned_screen;
	void (*draw_list)(void *user, unsigned dgroup_offset); /* control code 05: a draw list at that DGROUP offset (see view_glue.c) */
	void *draw_list_user;
	int draw_ox, draw_oy;         /* origin added to the coordinates of the list being drawn (the text area corner, or 0 after 03 'd'/'b'/'k') */
	Mm3Font font;
	uint8_t tiles[20][64];        /* window frame pieces (module offset 37D2h) */
	uint8_t colour_entries[0xAA]; /* module offset 0B78h: 4 bytes per colour index */
	uint8_t break_chars[10];      /* module offset 0E6Bh: characters a line may be broken after */
	uint16_t window_fill;         /* module word at 0C20h: fill pattern of new windows */
	Mm3Window win;                /* the current window state */
	Mm3Window stack[MM3_UI_MAX_WINDOWS];
	uint8_t *saved_bg[MM3_UI_MAX_WINDOWS];
	int depth;                    /* 60A00h */
	uint8_t alt_font;             /* 0 or 80h (0B72h) */
	uint8_t glyph_colours[4];     /* 0B74h: pixel values 1-3 -> palette index */
	uint8_t abs_d;                /* low byte of 0E67h: set by 03 'd' during one call */
	uint8_t flag_b, flag_k, flag_q;
	uint8_t in_window_text;       /* 0E66h */
} Mm3Ui;

/* Loads the font and the module's tables from MM3.CC; the screen starts black. */
Mm3Ui *mm3_ui_create(const Mm3Cc *mm3cc);
void mm3_ui_destroy(Mm3Ui *ui);
/* Draw into a screen buffer owned by the caller (e.g. video memory of the recompiled game) instead of the built-in one. */
void mm3_ui_set_screen(Mm3Ui *ui, uint8_t *screen);

void mm3_ui_print(Mm3Ui *ui, const char *text);
/* vdrv_1E_openWindow: x, y, width, height in pixels, text colour index, text printed in the new window (may be NULL). */
/* glyph colours of text colour index `index` (what opening a window of that colour leaves behind, even after it is closed) */
void mm3_ui_select_colour(Mm3Ui *ui, unsigned index);
void mm3_ui_open_window(Mm3Ui *ui, int x, int y, int w, int h, int colour, const char *text);
void mm3_ui_close_windows(Mm3Ui *ui, int n);

#endif
