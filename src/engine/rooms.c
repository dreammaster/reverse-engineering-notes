#include "rooms.h"

#include <stdlib.h>
#include <string.h>

static uint32_t be32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }

/* Convert n tiles of `rows` rows (16 bytes per row: two groups of four big-endian plane words). */
static int tiles_convert(const uint8_t *src, int n, int rows, TileSet *ts) {
	ts->pixels = (uint8_t *)malloc((size_t)n * TILE_W * rows);
	if (!ts->pixels)
		return -3;
	ts->count = n;
	ts->w = TILE_W;
	ts->h = rows;
	for (int k = 0; k < n; k++) {
		const uint8_t *t = src + (size_t)k * 16 * rows;
		uint8_t *out = ts->pixels + (size_t)k * TILE_W * rows;
		for (int y = 0; y < rows; y++) {
			for (int g = 0; g < 2; g++) {
				const uint8_t *w = t + y * 16 + g * 8;       /* four big-endian plane words */
				unsigned pl[4];
				for (int p = 0; p < 4; p++)
					pl[p] = (unsigned)((w[p * 2] << 8) | w[p * 2 + 1]);
				for (int b = 0; b < 16; b++) {
					unsigned c = 0;
					for (int p = 0; p < 4; p++)
						c |= ((pl[p] >> (15 - b)) & 1u) << p;
					int any = ((pl[0] | pl[1] | pl[2] | pl[3]) >> (15 - b)) & 1;
					out[y * TILE_W + g * 16 + b] = any ? (uint8_t)c : TILE_TRANSPARENT;
				}
			}
		}
	}
	return 0;
}

int tiles_load(const uint8_t *mp3, size_t len, TileSet *ts) {
	memset(ts, 0, sizeof(*ts));
	if (len < 4)
		return -1;
	size_t bytes = be32(mp3);
	int n = (int)(bytes / 288);
	if (4 + (size_t)n * 288 > len || n <= 0)
		return -2;
	return tiles_convert(mp3 + 4, n, TILE_H, ts);
}

int walltiles_load(const uint8_t *mp3, size_t len, TileSet *ts) {
	memset(ts, 0, sizeof(*ts));
	if (len < 8)
		return -1;
	size_t first = be32(mp3);
	if (4 + first + 4 > len)
		return -2;
	size_t bytes = be32(mp3 + 4 + first);
	int n = (int)(bytes / 512);
	if (8 + first + (size_t)n * 512 > len || n <= 0)
		return -2;
	return tiles_convert(mp3 + 8 + first, n, 32, ts);
}

void tiles_free(TileSet *ts) {
	free(ts->pixels);
	memset(ts, 0, sizeof(*ts));
}

static int mp1_records(const uint8_t *mp1, size_t len, int section, FloorMaps *fm) {
	memset(fm, 0, sizeof(*fm));
	size_t p = 0x48;
	for (int sec = 0; sec <= section; sec++) {
		if (p + 10 > len)
			return -1;
		size_t dl = ((size_t)mp1[p] << 24) | (mp1[p + 1] << 16) | (mp1[p + 2] << 8) | mp1[p + 3];
		int cnt = (mp1[p + 8] << 8) | mp1[p + 9];
		size_t dir = p + 10, data = dir + 4 * (size_t)cnt;
		if (sec == section) {
			if (cnt <= 0 || data > len)
				return -2;
			fm->buf = (uint8_t *)malloc(len);
			fm->map = (const uint8_t **)calloc((size_t)cnt, sizeof(uint8_t *));
			fm->len = (int *)calloc((size_t)cnt, sizeof(int));
			if (!fm->buf || !fm->map || !fm->len) {
				floormaps_free(fm);
				return -3;
			}
			memcpy(fm->buf, mp1, len);
			fm->count = cnt;
			for (int i = 0; i < cnt; i++) {
				size_t size = ((size_t)mp1[dir + 4 * i] << 8) | mp1[dir + 4 * i + 1];
				size_t off = ((size_t)mp1[dir + 4 * i + 2] << 8) | mp1[dir + 4 * i + 3];
				if (data + off + size > len) {
					floormaps_free(fm);
					return -2;
				}
				fm->len[i] = (int)size;
				fm->map[i] = size ? fm->buf + data + off : NULL;
			}
			return 0;
		}
		p = (data + dl + 1) & ~(size_t)1;
	}
	return -1;
}

int floormaps_parse(const uint8_t *mp1, size_t len, FloorMaps *fm) { return mp1_records(mp1, len, 0, fm); }
int wallrecs_parse(const uint8_t *mp1, size_t len, FloorMaps *fm) { return mp1_records(mp1, len, 1, fm); }

void floormaps_free(FloorMaps *fm) {
	free(fm->buf);
	free(fm->map);
	free(fm->len);
	memset(fm, 0, sizeof(*fm));
}

void floor_render(const TileSet *ts, const uint8_t *map, int w, int h, uint8_t *canvas) {
	int cw, ch;
	floor_canvas_size(w, h, &cw, &ch);
	for (int row = 0; row < h; row++) {
		for (int col = 0; col < w; col++) {
			unsigned tid = map[row * w + col];
			if (tid == 0 || (int)tid >= ts->count)
				continue;
			int ox = 16 * h + 16 * (col - row), oy = 8 * (col + row);
			const uint8_t *t = ts->pixels + (size_t)tid * TILE_W * TILE_H;  /* floor tiles are 32x18 */
			for (int y = 0; y < TILE_H; y++) {
				for (int x = 0; x < TILE_W; x++) {
					uint8_t c = t[y * TILE_W + x];
					if (c != TILE_TRANSPARENT && oy + y < ch && ox + x < cw)
						canvas[(oy + y) * cw + ox + x] = c;
				}
			}
		}
	}
}

int mp1_palette(const uint8_t *mp1, size_t len, uint16_t pal[16]) {
	size_t p = 0x48;
	for (int s = 0; s < 3; s++) {
		if (p + 10 > len)
			return -1;
		size_t dl = ((size_t)mp1[p] << 24) | (mp1[p + 1] << 16) | (mp1[p + 2] << 8) | mp1[p + 3];
		size_t cnt = ((size_t)mp1[p + 8] << 8) | mp1[p + 9];
		p = (p + 10 + 4 * cnt + dl + 1) & ~(size_t)1;
	}
	if (p + 32 > len)
		return -1;
	for (int i = 0; i < 16; i++)
		pal[i] = (uint16_t)((mp1[p + 2 * i] << 8) | mp1[p + 2 * i + 1]);
	return 0;
}

static const uint16_t floor_origin_table[64] = {3218,2898,2896,2576,2574,2254,1932,1930,2898,2898,2576,2576,2254,2254,1932,1932,2900,2578,2578,2256,2256,1934,1934,1932,2900,2900,2578,2258,2256,1936,1934,1614,2582,2260,2260,1938,1938,1616,1616,1614,2264,1942,1942,1940,1940,1618,1618,1296,1944,1944,1942,1622,1620,1620,1618,1178,1946,1944,1944,1622,1622,1300,1180,1178};

int floor_origin(int w, int h, int *x, int *y) {
	if (w < 3 || w > 10 || h < 3 || h > 10)
		return -1;
	unsigned v = floor_origin_table[(h - 3) * 8 + (w - 3)];
	*x = (int)(v % 40) * 8;
	*y = (int)(v / 40);
	return 0;
}

static void blit_clip(uint8_t *canvas, const uint8_t *px, int tw, int th, int ox, int oy) {
	for (int y = 0; y < th; y++) {
		int yy = oy + y;
		if (yy < 0 || yy >= VIEW_H)
			continue;
		for (int x = 0; x < tw; x++) {
			int xx = ox + x;
			uint8_t c = px[y * tw + x];
			if (xx >= 0 && xx < VIEW_W && c != TILE_TRANSPARENT)
				canvas[yy * VIEW_W + xx] = c;
		}
	}
}

void room_render_shell(const TileSet *floor, const TileSet *walls, const uint8_t *fmap, const uint8_t *wrec, int wrec_len,
                       int w, int h, uint8_t *canvas) {
	int gx, gy;
	if (floor_origin(w, h, &gx, &gy) != 0)
		return;
	for (int row = 0; row < h; row++)
		for (int col = 0; col < w; col++) {
			unsigned tid = fmap[row * w + col];
			if (tid && (int)tid < floor->count)
				blit_clip(canvas, floor->pixels + (size_t)tid * TILE_W * TILE_H, TILE_W, TILE_H, gx + 16 * (col - row),
				          gy + 8 * (col + row));
		}
	if (!wrec || wrec_len < 2)
		return;
	int slots = wrec[0];
	for (int c = 0; c < w + h; c++) {
		int x, ybase;
		if (c < w) {
			x = gx + 16 * c;
			ybase = gy - 17 + 8 * c;
		} else {
			int j = c - w + 1;
			x = gx - 16 * j;
			ybase = gy - 17 + 8 * j;
		}
		for (int k = 0; k < slots; k++) {
			int idx = 2 + c * slots + k;
			if (idx >= wrec_len)
				break;
			unsigned tid = wrec[idx];
			if (tid && tid < 0x80 && (int)tid < walls->count)
				blit_clip(canvas, walls->pixels + (size_t)tid * walls->w * walls->h, walls->w, walls->h, x, ybase - 16 * k);
		}
	}
}
