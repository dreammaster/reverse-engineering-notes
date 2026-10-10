#include "ui_text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- helpers on the screen */


/* the screen is a 64 KB segment (A000h) and all offsets wrap at 16 bits, as the original's do */
static unsigned seg_off(unsigned off) { return off & 0xFFFF; }

/* rep stosw / stosb: `width` pixels from a linear offset, alternating the two bytes of the pattern word */
static void stos_pattern(Mm3Ui *ui, unsigned off, unsigned width, uint16_t pattern) {
	unsigned words = width >> 1;
	for (unsigned i = 0; i < words; i++) {
		ui->screen[seg_off(off)] = (uint8_t)pattern;
		ui->screen[seg_off(off + 1)] = (uint8_t)(pattern >> 8);
		off += 2;
	}
	if (width & 1)
		ui->screen[seg_off(off)] = (uint8_t)pattern;
}

static void fill_rect(Mm3Ui *ui, unsigned x, unsigned y, unsigned w, unsigned h, uint16_t pattern) {
	for (unsigned r = 0; r < h; r++)
		stos_pattern(ui, (y + r) * MM3_UI_W + x, w, pattern);
}

/* ---- the module's small routines */

static void update_flags(Mm3Ui *ui) {
	/* sub_6371C: the screen update; the only state it changes is that it consumes one of the update-region flags */
	if (ui->flag_q) ui->flag_q = 0;
	else if (ui->flag_k) ui->flag_k = 0;
	else if (ui->flag_b) ui->flag_b = 0;
	else if (ui->abs_d) ui->abs_d = 0;
}

static void select_colour(Mm3Ui *ui, unsigned index) { /* sub_61AF0 */
	const uint8_t *e = &ui->colour_entries[(index * 4) % sizeof(ui->colour_entries)];
	/* the original reads a word pair at 0B78h + 4*index with a 16-bit index; out-of-table values read beyond the table */
	if (index * 4 + 4 <= sizeof(ui->colour_entries))
		memcpy(ui->glyph_colours, e, 4);
}

typedef struct { const uint8_t *p; } Cursor; /* the text pointer (dword_60B2C) */

static unsigned next_char(Cursor *c) { return *c->p++ & 0x7F; } /* sub_61B08 */

/* sub_61B16: up to n decimal digits (a space counts as 0); returns 1 and a garbage value in *out when a non-digit is met */
static int parse_digits(Cursor *c, int n, unsigned *out) {
	unsigned v = 0, ch = 0;
	for (int i = 0; i < n; i++) {
		ch = next_char(c);
		if (ch == 0x20) ch = 0x30;
		ch = (ch - 0x30) & 0xFF;
		if (ch > 9) { *out = ch; return 1; }
		v = (v * 10 + ch) & 0xFFFF;
	}
	*out = v;
	return 0;
}

/* sub_619C4: add the width of the next character to *bx; returns 1 (carry) when the character ends the measured text */
static int measure_char(Mm3Ui *ui, Cursor *c, uint16_t *bx, unsigned *last) {
	unsigned ch = next_char(c);
	*last = ch;
	if (ch > 0x20) {
		*bx = (uint16_t)(*bx + ui->font.width[ch | ui->alt_font]);
		return 0;
	}
	if (ch == 0x20) {
		*bx = (uint16_t)(*bx + ((ui->alt_font & 0x80) ? 3 : 4));
		return 0;
	}
	if (ch == 8) {
		unsigned c2 = next_char(c);
		if (c2 == 0x20) { *bx = (uint16_t)(*bx - 2); return 0; }
		c->p--; /* un-read both characters */
		c->p--;
		return 1;
	}
	if (ch == 0x0C) {
		if (next_char(c) != 'd') next_char(c);
		return 0;
	}
	c->p--;
	return 1;
}

/* sub_61B42: draw the glyph for `ch` (without the alternate-font bit) at the cursor and advance */
static void draw_glyph(Mm3Ui *ui, unsigned ch) {
	unsigned y = ui->win.cy;
	unsigned idx = (ch | ui->alt_font) & 0xFF;
	if (getenv("MM3_GLYPHLOG") && ch == 'F') fprintf(stderr, "glyph F colours %02X %02X %02X alt %02X\n", ui->glyph_colours[1], ui->glyph_colours[2], ui->glyph_colours[3], ui->alt_font);
	if (ch >= 'g' && (ch == 'g' || ch == 'p' || ch == 'q' || ch == 'y'))
		y++;
	for (int r = 0; r < 8; r++) {
		unsigned off = (y + r) * MM3_UI_W + ui->win.cx;
		unsigned w = ui->font.rows[idx][r];
		for (int i = 0; i < 8; i++, w >>= 2, off++) {
			unsigned v = w & 3;
			if (v) ui->screen[seg_off(off)] = ui->glyph_colours[v];
		}
	}
	ui->win.cx = (uint16_t)(ui->win.cx + ui->font.width[idx]);
}

/* sub_61A1C: move to the next line.  Returns 1 (carry) when the text area was full ('f' mode) or had to scroll. */
static int new_line(Mm3Ui *ui, Cursor *c) {
	Mm3Window *w = &ui->win;
	int dx, ax;
	while ((*c->p & 0x7F) == 0x20) c->p++;
	w->cx = ui->abs_d ? 0x74 : w->left;
	dx = (ui->alt_font & 0x80) ? 9 : 10;
	w->cy = (uint16_t)(w->cy + dx);
	if (ui->abs_d)
		return 0;
	ax = (int16_t)(w->bottom - w->cy);
	dx--;
	if (ax >= dx)
		return 0;
	if (w->stop_flag & 0x80)
		return 1;
	{
		int scroll = dx - ax;                 /* rows to scroll up */
		int rows = (int16_t)(w->bottom - (w->top + scroll));
		unsigned width = (uint16_t)(w->right - w->left);
		update_flags(ui);
		for (int i = 0; i < rows; i++) {
			unsigned dst = (w->top + i) * MM3_UI_W + w->left, src = (w->top + scroll + i) * MM3_UI_W + w->left;
			for (unsigned k = 0; k < width; k++)
				ui->screen[seg_off(dst + k)] = ui->screen[seg_off(src + k)];
		}
		w->cy = (uint16_t)(w->bottom - dx);
		for (int i = 0; i < scroll; i++)
			stos_pattern(ui, (unsigned)(w->top + rows + i) * MM3_UI_W + w->left, width, w->fill);
	}
	return 1;
}

void mm3_ui_print(Mm3Ui *ui, const char *text) {
	Mm3Window *w = &ui->win;
	Cursor cur = { (const uint8_t *)text };
	unsigned B38 = 0, last;
	uint16_t bx, B3A;
	const uint8_t *save, *end;
	int carry;

	ui->abs_d = 0;
	B3A = w->right;

restart:
	B38 = 0;
	save = cur.p;
	bx = w->align == 0 ? w->cx : w->left;
	/* measure the text up to the end of the line (an overflow, or a character that ends the measurement) */
	for (;;) {
		carry = measure_char(ui, &cur, &bx, &last);
		if (carry) break;
		if (ui->abs_d) continue;
		if (bx <= B3A) continue;
		cur.p--;
		B38++;
		break;
	}
	end = cur.p;
	cur.p = save;
	if (!carry && w->align != 1 && bx >= B3A) {
		/* the line overflowed: look back for a character to break it at; the character at the overflow is only tested when it is a space */
		const uint8_t *cx = end;
		int skip_first = (*cx != 0x20), found = 0;
		for (;;) {
			if (!skip_first) {
				unsigned ch = *cx & 0x7F;
				for (int di = 9; di >= 0; di--)
					if (ui->break_chars[di] == ch) { found = 1; break; }
				if (found) break;
			}
			skip_first = 0;
			cx--;
			if (!(save < cx)) break;
		}
		if (!found) {
			cx = end - 1;
			if (!(w->align == 1 || w->cx == w->left)) {
				int c2 = new_line(ui, &cur);
				if (c2 && (w->stop_flag & 0x80)) goto done;
				goto restart;
			}
		}
		end = cx;
	}
	/* right / centre alignment: measure the line [save, end] and move the cursor */
	if (w->align >= 1) {
		const uint8_t *keep = cur.p;
		uint16_t width = 0;
		for (;;) {
			carry = measure_char(ui, &cur, &width, &last);
			if (carry) break;
			if (cur.p <= end) continue;
			if (last == 0x20) {
				width = (uint16_t)(width - 4);
				if (ui->alt_font & 0x80) width++;
			}
			break;
		}
		cur.p = keep;
		if (w->align == 2) {
			unsigned ax = (w->cx == w->left) ? (unsigned)(w->left + B3A + 1) : (unsigned)w->cx * 2;
			w->cx = (uint16_t)(((ax - width) & 0xFFFF) >> 1);
		} else {
			w->cx = (uint16_t)(w->cx - (uint16_t)(width - 1));
		}
	}
	/* print the characters of the line */
	for (;;) {
		unsigned ch;
		if (cur.p > end) {
			if (w->align != 1 && B38 != 0) {
				int c2 = new_line(ui, &cur);
				if (c2 && (w->stop_flag & 0x80)) goto done;
			}
			goto restart;
		}
		ch = next_char(&cur);
		if (ch > 0x20) { draw_glyph(ui, ch); continue; }
		if (ch == 0x20) { w->cx += 4; if (ui->alt_font & 0x80) w->cx--; continue; }
		if (ch > 0x0D) continue;
		switch (ch) {
		case 0x0D:
			fill_rect(ui, w->left, w->top, (uint16_t)(w->right - w->left), (uint16_t)(w->bottom - w->top), w->fill);
			w->cx = w->left;
			w->cy = w->top;
			break;
		case 1: ui->alt_font = 0; break;
		case 2: ui->alt_font = 0x80; break;
		case 3: {
			unsigned l = next_char(&cur);
			if (l == 'q') ui->flag_q = 1;
			else if (l == 'k') ui->flag_k = 1;
			else if (l == 'b') ui->flag_b = 1;
			else if (l == 'd') ui->abs_d = 2;
			else if (l == 't') w->stop_flag = 0;
			else if (l == 'f') w->stop_flag = 0x80;
			else if (l == 'm') next_char(&cur); /* window background pattern: argument skipped */
			else w->align = (l == 'c') ? 2 : (l == 'r') ? 1 : 0;
			break;
		}
		case 4: { /* bar */
			unsigned width, rows = (ui->alt_font & 0x80) ? 8 : 9;
			unsigned off;
			parse_digits(&cur, 3, &width);
			off = w->cy * MM3_UI_W + w->cx;
			if (w->align == 1) off -= width;
			for (unsigned r = 0; r < rows; r++)
				stos_pattern(ui, off + r * MM3_UI_W, width, w->fill);
			break;
		}
		case 5: { /* draw list: four hex digits, the DGROUP offset of the list */
			unsigned addr = 0;
			for (int i = 0; i < 4; i++) {
				unsigned d = cur.p[i] & 0x7F;
				d = d <= '9' ? d - '0' : (d & 0xDF) - 'A' + 10;
				addr = addr * 16 + (d & 15);
			}
			cur.p += 4;
			ui->draw_ox = (ui->abs_d || ui->flag_b || ui->flag_k) ? 0 : w->left;
			ui->draw_oy = (ui->abs_d || ui->flag_b || ui->flag_k) ? 0 : w->top;
			if (ui->draw_list) ui->draw_list(ui->draw_list_user, addr);
			ui->draw_ox = ui->draw_oy = 0;
			break;
		}
		case 6: draw_glyph(ui, 0x20); break;
		case 7: {
			unsigned v;
			if (parse_digits(&cur, 3, &v)) v = w->default_fill;
			else v = (v & 0xFF) | ((v & 0xFF) << 8);
			w->fill = (uint16_t)v;
			break;
		}
		case 8: {
			unsigned c2 = next_char(&cur);
			if (c2 == 0x20) {
				c2 = 0;
				w->cx -= 2;
				if (ui->alt_font & 0x80) w->cx++;
			} else {
				if (c2 == 6) c2 = 0x20;
				w->cx = (uint16_t)(w->cx - ui->font.width[(c2 | ui->alt_font) & 0xFF]);
			}
			if (!(w->left < w->cx)) w->cx = w->left;
			if (c2) {
				uint16_t keep_x = w->cx;
				uint8_t keep[4];
				memcpy(keep, ui->glyph_colours, 4);
				ui->glyph_colours[1] = (uint8_t)w->fill;
				ui->glyph_colours[2] = (uint8_t)w->fill;
				ui->glyph_colours[3] = (uint8_t)(w->fill >> 8);
				draw_glyph(ui, c2);
				memcpy(ui->glyph_colours, keep, 4);
				w->cx = keep_x;
			}
			break;
		}
		case 9: {
			unsigned v;
			parse_digits(&cur, 3, &v);
			if (!ui->abs_d) { v = (v + w->left) & 0xFFFF; if (v > w->right) v = w->right; }
			w->cx = (uint16_t)v;
			break;
		}
		case 0x0A: {
			int c2 = new_line(ui, &cur);
			if (c2 && (w->stop_flag & 0x80)) goto done;
			break;
		}
		case 0x0B: {
			unsigned v;
			parse_digits(&cur, 3, &v);
			if (!ui->abs_d) { v = (v + w->top) & 0xFFFF; if (v > w->bottom) v = w->bottom; }
			w->cy = (uint16_t)v;
			break;
		}
		case 0x0C: {
			unsigned v;
			if (parse_digits(&cur, 2, &v)) v = w->default_colour;
			w->colour = (uint16_t)v;
			select_colour(ui, w->colour);
			break;
		}
		default: goto done; /* 00: end of the string */
		}
	}
done:
	ui->in_window_text >>= 1;
	update_flags(ui);
}

/* ---- windows */

static void draw_tile(Mm3Ui *ui, unsigned tile) { /* sub_635D4: an 8x8 frame piece at the cursor, then advance 8 */
	unsigned off = ui->win.cy * MM3_UI_W + ui->win.cx;
	ui->win.cx += 8;
	for (int r = 0; r < 8; r++, off += MM3_UI_W)
		for (int i = 0; i < 8; i++) {
			uint8_t v = ui->tiles[tile][r * 8 + i];
			if (v) ui->screen[seg_off(off + i)] = v;
		}
}

void mm3_ui_open_window(Mm3Ui *ui, int x, int y, int w, int h, int colour, const char *text) {
	Mm3Window *win = &ui->win;
	size_t size;
	uint8_t *bg;
	if (ui->depth >= MM3_UI_MAX_WINDOWS)
		return;
	ui->stack[ui->depth] = *win;
	win->x = (uint16_t)x; win->y = (uint16_t)y; win->width = (uint16_t)w; win->height = (uint16_t)h;
	win->colour = (uint16_t)colour;
	win->fill = ui->window_fill;
	win->default_colour = win->colour;
	win->default_fill = win->fill;
	/* save the background under the window */
	size = (size_t)w * h;
	bg = malloc(size ? size : 1);
	for (int r = 0; r < h; r++)
		for (int i = 0; i < w; i++) {
			unsigned off = (unsigned)(y + r) * MM3_UI_W + (unsigned)(x + i);
			bg[(size_t)r * w + i] = ui->screen[seg_off(off)];
		}
	free(ui->saved_bg[ui->depth]);
	ui->saved_bg[ui->depth] = bg;
	ui->depth++;
	/* text area and frame */
	win->align = 0;
	win->stop_flag = 0;
	win->left = (uint16_t)(win->x + 8);
	win->right = (uint16_t)(win->x + win->width - 8);
	win->top = (uint16_t)(win->y + 8);
	win->bottom = (uint16_t)(win->y + win->height - 8);
	/* sub_636CA starts with clc, so even the caller's stc variant fills only the text area; the frame tiles cover the border */
	fill_rect(ui, win->left, win->top, (uint16_t)(win->right - win->left), (uint16_t)(win->bottom - win->top), win->fill);
	win->cx = win->x; win->cy = win->y;
	draw_tile(ui, 0);
	for (unsigned n = (unsigned)(win->width - 9) >> 3, t = 1; n; n--) { draw_tile(ui, t); if (++t == 5) t = 1; }
	win->cx = (uint16_t)(win->x + win->width - 8);
	draw_tile(ui, 5);
	for (unsigned n = (unsigned)(win->height - 9) >> 3, t = 6; n; n--) {
		win->cy += 8;
		win->cx = win->x;
		draw_tile(ui, t);
		win->cx = (uint16_t)(win->x + win->width - 8);
		draw_tile(ui, t + 4);
		if (++t == 10) t = 6;
	}
	win->cx = win->x;
	win->cy = (uint16_t)(win->y + win->height - 8);
	draw_tile(ui, 0x0E);
	for (unsigned n = (unsigned)(win->width - 9) >> 3, t = 0x0F; n; n--) { draw_tile(ui, t); if (++t == 0x13) t = 0x0F; }
	win->cx = (uint16_t)(win->x + win->width - 8);
	draw_tile(ui, 0x13);
	win->cx = win->left;
	win->cy = win->top;
	select_colour(ui, win->colour);
	if (text) {
		ui->in_window_text = 1;
		mm3_ui_print(ui, text);
	} else {
		update_flags(ui);
	}
}

void mm3_ui_close_windows(Mm3Ui *ui, int n) {
	while (n-- > 0) {
		Mm3Window *win = &ui->win;
		if (--ui->depth < 0) {
			ui->depth = 0;
			return;
		}
		if (ui->saved_bg[ui->depth]) {
			for (int r = 0; r < win->height; r++)
				for (int i = 0; i < win->width; i++) {
					unsigned off = (unsigned)(win->y + r) * MM3_UI_W + (unsigned)(win->x + i);
					ui->screen[seg_off(off)] = ui->saved_bg[ui->depth][(size_t)r * win->width + i];
				}
			free(ui->saved_bg[ui->depth]);
			ui->saved_bg[ui->depth] = NULL;
		}
		update_flags(ui);
		*win = ui->stack[ui->depth];
	}
}

void mm3_ui_select_colour(Mm3Ui *ui, unsigned index) { select_colour(ui, index); }

/* ---- setup */

Mm3Ui *mm3_ui_create(const Mm3Cc *cc) {
	Mm3Ui *ui = calloc(1, sizeof(*ui));
	size_t len;
	uint8_t *mod;
	int idx = -1;
	if (!ui || mm3_font_load(&ui->font, cc)) { free(ui); return NULL; }
	for (unsigned i = 0; i < cc->count; i++)
		if (cc->entries[i].id == 0x8F99) idx = (int)i;
	mod = idx >= 0 ? mm3_cc_read_index(cc, idx, &len) : NULL;
	if (!mod || len < 0x37D2 + 0x500) { free(mod); free(ui); return NULL; }
	memcpy(ui->tiles, mod + 0x37D2, sizeof(ui->tiles));
	memcpy(ui->colour_entries, mod + 0xB78, sizeof(ui->colour_entries));
	memcpy(ui->break_chars, mod + 0xE6B, sizeof(ui->break_chars));
	ui->window_fill = (uint16_t)(mod[0xC20] | (mod[0xC21] << 8));
	free(mod);
	/* the module's initial state: one full-screen "window" */
	ui->owned_screen = calloc(1, 65536);
	ui->screen = ui->owned_screen;
	ui->win.width = MM3_UI_W; ui->win.height = MM3_UI_H;
	ui->win.colour = 1; ui->win.fill = 0x8888;
	ui->win.right = MM3_UI_W; ui->win.bottom = MM3_UI_H;
	ui->win.default_colour = 1; ui->win.default_fill = 0x8080;
	ui->glyph_colours[0] = 0; ui->glyph_colours[1] = 0x40; ui->glyph_colours[2] = 0x30; ui->glyph_colours[3] = 0x20;
	return ui;
}

void mm3_ui_set_screen(Mm3Ui *ui, uint8_t *screen) { ui->screen = screen; }

void mm3_ui_destroy(Mm3Ui *ui) {
	if (!ui) return;
	free(ui->owned_screen);
	for (int i = 0; i < MM3_UI_MAX_WINDOWS; i++) free(ui->saved_bg[i]);
	free(ui);
}
