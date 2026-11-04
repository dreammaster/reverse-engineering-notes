#include "objsprites.h"

#include <stdlib.h>
#include <string.h>

static unsigned be16(const uint8_t *p) { return (unsigned)((p[0] << 8) | p[1]); }

/* Try to parse one block. Returns the number of frames (0 = not a sprite block). */
static int parse_block(const uint8_t *b, size_t n, ObjSprite *sp) {
	if (n < 36)
		return 0;
	size_t p = 32;
	int cap = 0, cnt = 0;
	ObjFrame *fr = NULL;
	while (p + 4 <= n) {
		unsigned w = be16(b + p), h = be16(b + p + 2);
		if (w < 1 || w > 4 || h < 1 || h > 128 || p + 4 + (size_t)h * w * 8 > n)
			break;
		if (cnt == cap) {
			cap = cap ? cap * 2 : 4;
			fr = (ObjFrame *)realloc(fr, (size_t)cap * sizeof(ObjFrame));
			if (!fr)
				return 0;
		}
		ObjFrame *f = &fr[cnt];
		f->w = (int)w * 16;
		f->h = (int)h;
		f->pixels = (uint8_t *)malloc((size_t)f->w * f->h);
		if (!f->pixels)
			goto fail;
		const uint8_t *d = b + p + 4;
		for (unsigned y = 0; y < h; y++)
			for (unsigned g = 0; g < w; g++) {
				const uint8_t *ws = d + y * w * 8 + g * 8;
				unsigned pl[4];
				for (int k = 0; k < 4; k++)
					pl[k] = be16(ws + k * 2);
				for (int bit = 0; bit < 16; bit++) {
					unsigned c = 0;
					for (int k = 0; k < 4; k++)
						c |= ((pl[k] >> (15 - bit)) & 1u) << k;
					f->pixels[y * f->w + g * 16 + bit] = (uint8_t)c;
				}
			}
		cnt++;
		p += 4 + (size_t)h * w * 8;
	}
	if (cnt == 0 || p != n)
		goto fail;
	memcpy(sp->header, b, 32);
	sp->nframes = cnt;
	sp->frames = fr;
	return cnt;
fail:
	for (int i = 0; i < cnt; i++)
		free(fr[i].pixels);
	free(fr);
	return 0;
}

int objsprites_load(const uint8_t *mp1, size_t len, ObjSprites *out) {
	memset(out, 0, sizeof(*out));
	size_t p = 0x48;
	for (int s = 0; s < 2; s++) {
		if (p + 10 > len)
			return -1;
		size_t dl = ((size_t)mp1[p] << 24) | (mp1[p + 1] << 16) | (mp1[p + 2] << 8) | mp1[p + 3];
		size_t cnt = ((size_t)mp1[p + 8] << 8) | mp1[p + 9];
		p = (p + 10 + 4 * cnt + dl + 1) & ~(size_t)1;
	}
	if (p + 10 > len)
		return -1;
	int cnt = (int)be16(mp1 + p + 8);
	size_t dir = p + 10, data = dir + 4 * (size_t)cnt;
	if (cnt <= 0 || data > len)
		return -2;
	out->sprites = (ObjSprite *)calloc((size_t)cnt, sizeof(ObjSprite));
	if (!out->sprites)
		return -3;
	size_t pos = 0;
	for (int i = 0; i < cnt; i++) {
		unsigned sz = be16(mp1 + dir + 4 * i);
		size_t stored = sz > 1 ? (sz + 1) / 2 : 0;
		if (data + pos + stored > len) {
			objsprites_free(out);
			return -2;
		}
		ObjSprite sp;
		memset(&sp, 0, sizeof(sp));
		if (stored && parse_block(mp1 + data + pos, stored, &sp)) {
			sp.block = i;
			out->sprites[out->count++] = sp;
		}
		pos += stored;
	}
	return 0;
}

void objsprites_free(ObjSprites *o) {
	for (int i = 0; i < o->count; i++) {
		for (int k = 0; k < o->sprites[i].nframes; k++)
			free(o->sprites[i].frames[k].pixels);
		free(o->sprites[i].frames);
	}
	free(o->sprites);
	memset(o, 0, sizeof(*o));
}
