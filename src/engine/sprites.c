#include "sprites.h"

#include <stdlib.h>
#include <string.h>

const uint16_t game_palette_st[16] = {0x0000, 0x0243, 0x0400, 0x0032, 0x0310, 0x0420, 0x0111, 0x0775,
                                      0x0555, 0x0200, 0x0752, 0x0221, 0x0333, 0x0444, 0x0531, 0x0642};

void st_word_to_rgb(uint16_t w, uint8_t rgb[3]) {
	rgb[0] = (uint8_t)(((w >> 8) & 7) * 255 / 7);
	rgb[1] = (uint8_t)(((w >> 4) & 7) * 255 / 7);
	rgb[2] = (uint8_t)((w & 7) * 255 / 7);
}

#define AREA2_SIZE 24576u

int pcman_load(const uint8_t *data, size_t len, SpriteSet *set) {
	memset(set, 0, sizeof(*set));
	if (len < AREA2_SIZE + 96)
		return -1;
	const uint8_t *area[2] = {data, data + AREA2_SIZE};
	size_t area_len[2] = {AREA2_SIZE, len - AREA2_SIZE};

	for (int i = 0; i < PCMAN_SPRITE_COUNT; i++) {
		unsigned off = data[i * 2] | (data[i * 2 + 1] << 8);
		int which = (off >> 8) >= 6;
		if (which)
			off -= 0x600;
		size_t pos = ((size_t)off + 6) * 16;
		if (pos + 8 > area_len[which])
			goto bad;
		const uint8_t *p = area[which] + pos;
		Sprite *s = &set->sprites[i];
		s->hot_x = (int16_t)(p[0] | (p[1] << 8));
		s->hot_y = (int16_t)(p[2] | (p[3] << 8));
		int w = p[4] | (p[5] << 8), h = p[6] | (p[7] << 8);
		size_t bw = 2 * (size_t)w;
		if (w <= 0 || h <= 0 || w > 8 || h > 256 || pos + 8 + 5 * bw * h > area_len[which])
			goto bad;
		s->w = w * 16;
		s->h = h;
		s->pixels = (uint8_t *)malloc((size_t)s->w * h);
		if (!s->pixels)
			goto bad;
		const uint8_t *pl = p + 8;
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < s->w; x++) {
				size_t idx = (size_t)y * bw + x / 8;
				int sh = 7 - x % 8;
				if ((pl[idx] >> sh) & 1) {
					int c = 0;
					for (int k = 0; k < 4; k++)
						c |= ((pl[(size_t)(1 + k) * h * bw + idx] >> sh) & 1) << k;
					s->pixels[y * s->w + x] = (uint8_t)c;
				} else {
					s->pixels[y * s->w + x] = SPRITE_TRANSPARENT;
				}
			}
		}
		set->count = i + 1;
	}
	return 0;
bad:
	spriteset_free(set);
	return -2;
}

void spriteset_free(SpriteSet *set) {
	for (int i = 0; i < PCMAN_SPRITE_COUNT; i++)
		free(set->sprites[i].pixels);
	memset(set, 0, sizeof(*set));
}
