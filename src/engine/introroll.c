#include "intro.h"
#include "exeimg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RETRACE_HZ 70.086      /* VGA mode 0Dh: one scroll step per vertical retrace (1000:10EBE waits for it) */
#define TIMER_HZ 50.16         /* the intro programs the PIT to 5CECh; INT 1Ch counts the page wait up from -540 */
#define WAIT_TICKS 540
#define LINE_H 6
#define WIN_X 64
#define WIN_Y 40
#define WIN_W 192
#define WIN_H 120
#define TEXT_X 68
#define TEXT_Y 160            /* a line is typed just below the window and scrolls up into it */

typedef struct { int off, len; int wait_after; } Line;   /* one text line; wait_after: a page break (!) follows its CR */

struct IntroScript {
	uint8_t *text;
	size_t len;
	Line *lines;
	int nlines;
	/* time line of one pass: the CR of line i scrolls during [cr_t[i], cr_t[i] + LINE_H / RETRACE_HZ), cr_t[nlines] = the end of the pass */
	double *cr_t;
	double cycle;
};

static void build_lines(IntroScript *s) {
	/* every 0Dh ends a line (0Ah is skipped); `!` is a command, not a glyph: the interpreter waits there and goes on in the same line */
	int cap = 512;
	s->lines = (Line *)calloc((size_t)cap, sizeof(Line));
	int start = 0, n = 0, pend_wait = 0;
	for (size_t i = 0; i <= s->len && n < cap - 1; i++) {
		if (i == s->len && (int)i == start) break;           /* the script ended with a CR: no unterminated line */
		uint8_t c = i < s->len ? s->text[i] : 0x0D;
		if (c == 0x0A) { if ((int)i == start) start++; continue; }
		if (c == '!' || c == '#') {
			/* only a line that holds nothing else is a page break in the shipped scripts; a `!` in the middle of text keeps the text typed so far */
			pend_wait |= c == '!';
		}
		if (c == 0x0D) {
			Line *l = &s->lines[n++];
			l->off = start; l->len = (int)i - start; l->wait_after = 0;
			start = (int)i + 1;
			if (pend_wait) { l->wait_after = 1; pend_wait = 0; }
		}
	}
	s->nlines = n;
}

IntroScript *introscript_load(const uint8_t *exe, size_t len) {
	uint8_t *mem = NULL;
	if (exeimg_unpack(exe, len, "copyright owners.", &mem) != 0) return NULL;
	/* the marker is the last text of the script: the stub writes the program front to back */
	static const char title[] = "          cadaver";
	size_t at = 0, tl = sizeof(title) - 1;
	for (size_t i = 0x10100; i + tl < EXEIMG_MEM; i++)
		if (mem[i] == ' ' && !memcmp(mem + i, title, tl)) { at = i; break; }
	if (!at) { free(mem); return NULL; }
	size_t end = at;
	while (end < EXEIMG_MEM && mem[end] != '#') end++;
	if (end >= EXEIMG_MEM || end - at > 20000) { free(mem); return NULL; }
	IntroScript *s = (IntroScript *)calloc(1, sizeof(*s));
	if (!s) { free(mem); return NULL; }
	s->len = end - at;
	s->text = (uint8_t *)malloc(s->len ? s->len : 1);
	memcpy(s->text, mem + at, s->len);
	free(mem);
	build_lines(s);
	s->cr_t = (double *)calloc((size_t)s->nlines + 1, sizeof(double));
	double t = 0;
	for (int i = 0; i < s->nlines; i++) {
		if (s->lines[i].wait_after) t += WAIT_TICKS / TIMER_HZ;      /* the page break waits before the CR of its line */
		s->cr_t[i] = t;
		t += LINE_H / RETRACE_HZ;
	}
	s->cr_t[s->nlines] = t;
	s->cycle = t;
	if (getenv("CAD_INTRO_DBG")) { int w = 0; for (int i = 0; i < s->nlines; i++) w += s->lines[i].wait_after; fprintf(stderr, "INTRO script %zu bytes, %d lines, %d page breaks, cycle %.2f s\n", s->len, s->nlines, w, s->cycle); }
	return s;
}

void introscript_free(IntroScript *s) {
	if (!s) return;
	free(s->text); free(s->lines); free(s->cr_t); free(s);
}

static void draw_line(Screen *scr, const Font *f, const IntroScript *s, const Line *l, int y) {
	if (y <= WIN_Y - LINE_H || y >= WIN_Y + WIN_H) return;
	int x = TEXT_X;
	for (int i = 0; i < l->len; i++, x += LINE_H) {
		uint8_t c = s->text[l->off + i];
		if (c == '!' || c == '#') { x -= LINE_H; continue; }
		const uint8_t *g = font_glyph(f, (char)c);
		if (!g) continue;
		for (int gy = 0; gy < LINE_H; gy++) {
			int py = y + gy;
			if (py < WIN_Y || py >= WIN_Y + WIN_H) continue;
			for (int gx = 0; gx < LINE_H; gx++) {
				int px = x + gx;
				if (px >= WIN_X && px < WIN_X + WIN_W) scr->pixels[py * SCREEN_W + px] = g[gy * LINE_H + gx];
			}
		}
	}
}

void intro_render_roll(Screen *scr, const Pc1Image *bg, const Font *f, const Flames *fl, const IntroScript *s, double t) {
	memset(scr, 0, sizeof(*scr));
	memcpy(scr->pixels, bg->pixels, sizeof(bg->pixels));
	for (int i = 0; i < 16; i++) memcpy(scr->palette[i], bg->palette[i], 3);
	if (s && s->nlines > 0 && t >= 0) {
		int cyc = (int)(t / s->cycle);
		double tc = t - cyc * s->cycle;
		int n = 0;                                    /* lines whose scroll has started in this pass */
		while (n < s->nlines && s->cr_t[n] <= tc) n++;
		/* line n-1 is the one being scrolled now (or the last finished); j = steps done of its scroll */
		int cur = n - 1, j = LINE_H;
		if (cur >= 0) {
			double d = (tc - s->cr_t[cur]) * RETRACE_HZ;
			j = d >= LINE_H ? LINE_H : (int)d;
		}
		int done = cur < 0 ? 0 : (j >= LINE_H ? cur + 1 : cur);        /* CRs completed in this pass */
		int global_done = cyc * s->nlines + done;
		/* every line typed so far (the current one included) lies at TEXT_Y - LINE_H * (CRs completed after it) - steps of the running scroll */
		for (int g = global_done - 2 * s->nlines; g <= global_done; g++) {
			if (g < 0) continue;
			const Line *l = &s->lines[g % s->nlines];
			int y = TEXT_Y + LINE_H * (g - global_done) - (j >= LINE_H ? 0 : j);
			if (j >= LINE_H && g > global_done) continue;
			draw_line(scr, f, s, l, y);
		}
	}
	long step = t > 0 ? (long)(t * RETRACE_HZ) : 0;
	int l = (int)(step % 6);
	intro_draw_flame(scr, fl->frame[l], 14, 20);
	intro_draw_flame(scr, fl->frame[(l + 2) % 6], 296, 20);
}
