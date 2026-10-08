#include "gfx.h"

#include <stdlib.h>
#include <string.h>

#define MAX_FRAME_PIXELS (1 << 22)

typedef struct {
	int x_off, y_off, w, h;
	uint8_t *rows; /* w*h */
} Layer;

static unsigned rd16(const uint8_t *d, size_t o) { return d[o] | (d[o + 1] << 8); }

int mm3_palette_load(Mm3Palette *pal, const Mm3Cc *cc) {
	size_t len;
	uint8_t *mod;
	int idx = -1;
	for (unsigned i = 0; i < cc->count; i++)
		if (cc->entries[i].id == 0x8F99)
			idx = (int)i;
	if (idx < 0)
		return -1;
	mod = mm3_cc_read_index(cc, idx, &len);
	if (!mod || len < 0x39C + 768) {
		free(mod);
		return -1;
	}
	for (int i = 0; i < 256; i++)
		for (int k = 0; k < 3; k++)
			pal->rgb[i][k] = (uint8_t)(mod[0x39C + 3 * i + k] * 4);
	free(mod);
	return 0;
}

static int layer_decode(const uint8_t *d, size_t len, size_t off, Layer *L) {
	size_t p;
	if (off + 8 > len)
		return -1;
	L->x_off = (int)rd16(d, off);
	L->w = (int)rd16(d, off + 2);
	L->y_off = (int)rd16(d, off + 4);
	L->h = (int)rd16(d, off + 6);
	if ((size_t)L->w * L->h > MAX_FRAME_PIXELS)
		return -1;
	L->rows = calloc((size_t)L->w * L->h + 1, 1);
	if (!L->rows)
		return -1;
	p = off + 8;
	for (int y = 0; y < L->h; y++) {
		uint8_t *row = L->rows + (size_t)y * L->w;
		size_t ln, end, q;
		int x;
		if (p + 2 > len) goto fail;
		ln = rd16(d, p);
		if (ln == 0) { p += 2; continue; }
		end = p + 2 + ln;
		if (p + 4 > len) goto fail;
		x = (int)rd16(d, p + 2);
		q = p + 4;
		while (q < end) {
			unsigned op = d[q++];
			if (q > len) goto fail;
			if (op < 0x80) {
				int n = (int)op + 1;
				if (q + n > len) goto fail;
				for (int k = 0; k < n; k++)
					if (x + k < L->w)
						row[x + k] = d[q + k];
				q += n;
				x += n;
			} else if (op < 0xC0) {
				x += (int)(op & 0x3F) + 1;
			} else {
				int n = (int)(op & 0x3F) + 3;
				uint8_t v;
				if (q >= len) goto fail;
				v = d[q++];
				for (int k = 0; k < n; k++)
					if (x + k < L->w)
						row[x + k] = v;
				x += n;
			}
		}
		p = end;
	}
	return 0;
fail:
	free(L->rows);
	L->rows = NULL;
	return -1;
}

int mm3_sprite_decode(Mm3Sprite *spr, const uint8_t *d, size_t len) {
	unsigned n;
	spr->frames = NULL;
	spr->count = 0;
	if (len < 2)
		return -1;
	n = rd16(d, 0);
	if (n == 0 || 2 + (size_t)n * 4 > len)
		return -1;
	spr->frames = calloc(n, sizeof(Mm3Frame));
	if (!spr->frames)
		return -1;
	spr->count = n;
	for (unsigned i = 0; i < n; i++) {
		Layer L[2];
		int nl = 1, x0, y0, x1, y1;
		unsigned o1 = rd16(d, 2 + 4 * i), o2 = rd16(d, 4 + 4 * i);
		Mm3Frame *f = &spr->frames[i];
		memset(L, 0, sizeof(L));
		if (layer_decode(d, len, o1, &L[0]))
			goto fail;
		if (o2) {
			nl = 2;
			if (layer_decode(d, len, o2, &L[1])) {
				free(L[0].rows);
				goto fail;
			}
		}
		x0 = L[0].x_off; y0 = L[0].y_off; x1 = x0 + L[0].w; y1 = y0 + L[0].h;
		if (nl == 2) {
			if (L[1].x_off < x0) x0 = L[1].x_off;
			if (L[1].y_off < y0) y0 = L[1].y_off;
			if (L[1].x_off + L[1].w > x1) x1 = L[1].x_off + L[1].w;
			if (L[1].y_off + L[1].h > y1) y1 = L[1].y_off + L[1].h;
		}
		f->w = x1 - x0;
		f->h = y1 - y0;
		if ((size_t)f->w * f->h > MAX_FRAME_PIXELS) {
			for (int k = 0; k < nl; k++) free(L[k].rows);
			goto fail;
		}
		f->pixels = calloc((size_t)f->w * f->h + 1, 1);
		f->x0 = x0; f->y0 = y0; f->nlayers = nl;
		for (int k = 0; k < nl; k++) {
			for (int y = 0; y < L[k].h; y++)
				for (int x = 0; x < L[k].w; x++) {
					uint8_t v = L[k].rows[(size_t)y * L[k].w + x];
					if (v)
						f->pixels[(size_t)(L[k].y_off - y0 + y) * f->w + (L[k].x_off - x0 + x)] = v;
				}
			f->layer[k].x_off = L[k].x_off; f->layer[k].y_off = L[k].y_off; f->layer[k].w = L[k].w; f->layer[k].h = L[k].h;
			f->layer[k].pixels = L[k].rows; /* ownership moves to the frame */
		}
	}
	return 0;
fail:
	mm3_sprite_free(spr);
	return -1;
}

void mm3_sprite_free(Mm3Sprite *spr) {
	for (unsigned i = 0; i < spr->count; i++) {
		free(spr->frames[i].pixels);
		for (int k = 0; k < spr->frames[i].nlayers; k++)
			free(spr->frames[i].layer[k].pixels);
	}
	free(spr->frames);
	spr->frames = NULL;
	spr->count = 0;
}

void mm3_blit(uint8_t *surf, int sw, int sh, const Mm3Frame *f, int x, int y) {
	for (int j = 0; j < f->h; j++) {
		int dy = y + j;
		if (dy < 0 || dy >= sh)
			continue;
		for (int i = 0; i < f->w; i++) {
			int dx = x + i;
			uint8_t v = f->pixels[(size_t)j * f->w + i];
			if (v && dx >= 0 && dx < sw)
				surf[dy * sw + dx] = v;
		}
	}
}

void mm3_blit_layers(uint8_t *surf, int sw, int sh, const Mm3Frame *f, int x, int y, int mirror) {
	for (int k = 0; k < f->nlayers; k++) {
		const Mm3Layer *L = &f->layer[k];
		for (int j = 0; j < L->h; j++) {
			int dy = y + L->y_off + j;
			if (dy < 0 || dy >= sh)
				continue;
			for (int i = 0; i < L->w; i++) {
				uint8_t v = L->pixels[(size_t)j * L->w + i];
				int dx = x + L->x_off + (mirror ? L->w - 1 - i : i);
				if (v && dx >= 0 && dx < sw)
					surf[dy * sw + dx] = v;
			}
		}
	}
}

int mm3_scale_patterns_load(uint16_t out[4], const Mm3Cc *cc) {
	size_t len;
	int idx = -1;
	uint8_t *mod;
	for (unsigned i = 0; i < cc->count; i++)
		if (cc->entries[i].id == 0x8F99)
			idx = (int)i;
	if (idx < 0 || !(mod = mm3_cc_read_index(cc, idx, &len)))
		return -1;
	if (len < 0xB6A + 8) { free(mod); return -1; }
	for (int i = 0; i < 4; i++)
		out[i] = (uint16_t)(mod[0xB6A + 2 * i] | (mod[0xB6A + 2 * i + 1] << 8));
	free(mod);
	return 0;
}

/* One step of the pixel thinning register (sub_620D8 / the literal-run loop): the source pixel is kept when a 1 bit is shifted out
 * of the register or when the register becomes empty, in which case it is reloaded. */
static int thin_step(unsigned *reg, unsigned reload) {
	unsigned carry = (*reg >> 15) & 1;
	*reg = (*reg << 1) & 0xFFFF;
	if (*reg == 0) {
		*reg = reload;
		return 1;
	}
	return (int)carry;
}

/* number of output pixels (or rows, sub_62136) for `count` source pixels, advancing the register */
static int thin_count(unsigned *reg, unsigned reload, int count) {
	int n = 0;
	for (int i = 0; i < count; i++)
		n += thin_step(reg, reload);
	return n;
}

void mm3_blit_ex(uint8_t *surf, int sw, int sh, const Mm3Frame *f, int x, int y, unsigned flags, const uint16_t pat[4]) {
	int mirror = flags & 1, clip = (flags & 2) != 0, scale = (flags >> 8) & 3;
	for (int k = 0; k < f->nlayers; k++) {
		const Mm3Layer *L = &f->layer[k];
		unsigned P = scale ? pat[scale] : 0, R = (P << 1) & 0xFFFF;
		unsigned vreg = P, hreg = P, hstart;
		int base_x, out_y;
		if (scale) {
			base_x = x + thin_count(&hreg, R, mirror ? L->x_off + L->w : L->x_off);
			out_y = y + thin_count(&vreg, R, L->y_off);
		} else {
			base_x = x + L->x_off;
			out_y = y + L->y_off;
		}
		hstart = hreg; /* the horizontal register restarts from here on every row */
		for (int r = 0; r < L->h; r++) {
			const uint8_t *row = L->pixels + (size_t)r * L->w;
			unsigned reg = hstart;
			int dy, cnt = 0;
			if (scale) { /* vertical thinning: a row is drawn when a 1 bit is shifted out; the register reloads when it runs empty */
				unsigned carry = (vreg >> 15) & 1;
				vreg = (vreg << 1) & 0xFFFF;
				if (!carry)
					continue;
				if (vreg == 0)
					vreg = R;
			}
			dy = out_y++;
			if (dy < 0 || dy >= sh)
				continue;
			for (int c = 0; c < L->w; c++) {
				int dx;
				if (scale && !thin_step(&reg, R))
					continue;
				if (scale)
					dx = mirror ? base_x - cnt : base_x + cnt;
				else
					dx = mirror ? base_x + (L->w - 1 - c) : base_x + c;
				cnt++;
				if (row[c] && dx >= 0 && dx < sw && (!clip || (dx >= 8 && dx < 224)))
					surf[dy * sw + dx] = row[c];
			}
		}
	}
}
