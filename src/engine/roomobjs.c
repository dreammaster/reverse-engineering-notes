#include "roomobjs.h"
#include "rooms.h"

/* CAD.EXE ds:F8CE as pixels (offset*8 + shift), index (y&7)*8 + (x&7). */
static const int SUB[64] = {
	0, 322, 644, 966, 1288, 1610, 1932, 2254, 318, 640, 962, 1284, 1606, 1928, 2250, 2892,
	636, 958, 1280, 1602, 1924, 2246, 2888, 3210, 954, 1276, 1598, 1920, 2242, 2564, 3526, 3528,
	1272, 1594, 1916, 2238, 2560, 3202, 3844, 3846, 1590, 1912, 2234, 2556, 3198, 3520, 4162, 4164,
	1908, 2230, 2872, 3194, 3516, 3838, 4480, 4482, 2226, 2868, 3190, 3512, 3834, 4156, 4478, 4800};

static int hw(const uint8_t hdr[32], int i) { return (hdr[2 * i] << 8) | hdr[2 * i + 1]; }

void object_project(int x1, int y1, int z0, const uint8_t hdr[32], int gx, int gy, int *sx, int *sy) {
	int xs = hw(hdr, 8), ys = hw(hdr, 9), zs = hw(hdr, 10);
	int n = z0 - hw(hdr, 1) + xs + ys + zs;
	int p = gy * 320 + gx + 2576 * (x1 >> 3) + 2544 * (y1 >> 3) + SUB[(y1 & 7) * 8 + (x1 & 7)];
	p -= 320 * n + 16 * (hw(hdr, 2) - 1);
	if (xs & 7)
		p += 2 * (8 - (xs & 7));
	*sx = ((p % 320) + 320) % 320;
	*sy = (p - *sx) / 320;
}

void object_box(int x1, int y1, int z0, const uint8_t hdr[32], int *x0, int *y0, int *zt) {
	*x0 = x1 - hw(hdr, 8) + 1;
	*y0 = y1 - hw(hdr, 9) + 1;
	*zt = z0 + hw(hdr, 10) - 1;
}

const ObjSprite *objsprites_find(const ObjSprites *o, int block) {
	for (int i = 0; i < o->count; i++)
		if (o->sprites[i].block == block && o->sprites[i].nframes > 0)
			return &o->sprites[i];
	return NULL;
}

int room_render_objects(const Mp2 *mp2, const ObjSprites *objs, int room, int gx, int gy, uint8_t *canvas) {
	if (room < 0 || room >= mp2->sec[2].count || !mp2->sec[2].rec[room])
		return 0;
	const uint8_t *list = mp2->sec[2].rec[room];
	int n = mp2->sec[2].len[room] / 2, drawn = 0;
	for (int i = 0; i < n; i++) {
		int id = (list[2 * i] << 8) | list[2 * i + 1], len;
		const uint8_t *t = mp2_thing(mp2, id, &len);
		if (!t || len < 8 || (t[3] & 0x80))
			continue;
		const ObjSprite *sp = objsprites_find(objs, (t[6] << 8) | t[7]);
		if (!sp)
			continue;
		int sx, sy;
		object_project(t[0], t[1], t[2], sp->header, gx, gy, &sx, &sy);
		const ObjFrame *f = &sp->frames[0];
		for (int y = 0; y < f->h; y++)
			for (int x = 0; x < f->w; x++) {
				uint8_t c = f->pixels[y * f->w + x];
				int px = sx + x, py = sy + y;
				if (c && px >= 0 && px < VIEW_W && py >= 0 && py < VIEW_H)
					canvas[py * VIEW_W + px] = c;
			}
		drawn++;
	}
	return drawn;
}
