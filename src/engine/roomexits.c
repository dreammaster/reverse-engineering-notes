#include "roomexits.h"

static int s8(int v) { return (int8_t)(v & 0xFF); }
static int hw(const uint8_t h[32], int i) { return (h[2 * i] << 8) | h[2 * i + 1]; }

int room_build_exits(const Mp2 *mp2, const ObjSprites *objs, int room, ColBox out[ROOM_MAX_EXITS]) {
	if (room < 0 || room >= mp2->sec[0].count || !mp2->sec[0].rec[room] || mp2->sec[0].len[room] < 22)
		return 0;
	const uint8_t *rr = mp2->sec[0].rec[room];
	int thick_x = rr[21], thick_y = rr[20], n = 0;
	for (int i = 0; i < 7; i++) {
		int d = (rr[6 + 2 * i] << 8) | rr[7 + 2 * i];
		if (d == 0xFFFF)
			continue;
		if (d >= mp2->sec[1].count || mp2->sec[1].len[d] < 6)
			continue;
		uint8_t zero[8] = {0};
		const uint8_t *r = mp2->sec[1].rec[d] ? mp2->sec[1].rec[d] : zero;
		int dl = r[0], dh = r[1], ln = r[4], ori = r[5], ch;
		if (ori == 0) {
			ch = 2;
			dl = s8(dl - rr[1]);
			if (dl < 0) { dl++; ch = 0; }
			dh = s8(dh - rr[3]);
		} else {
			ch = 3;
			dh = s8(dh - rr[3]);
			if (dh < 0) { dh++; ch = 1; }
			dl = s8(dl - rr[1]);
		}
		if (ln < 1)
			continue;
		const ObjSprite *sp = objsprites_find(objs, (ln - 1) * 4 + ch);
		if (!sp)
			continue;
		int xs = hw(sp->header, 8), ys = hw(sp->header, 9), zs = hw(sp->header, 10);
		int x0 = (dl * 8) & 0xFF, y0 = (dh * 8) & 0xFF, ddl = x0, ddh = y0, zb = 0, bx = xs, by = ys;
		if (xs == 8) { bx = thick_x; y0 = (y0 - 4) & 0xFF; ddh = (ddh + 4) & 0xFF; }
		if (ys == 8) { by = thick_y; x0 = (x0 - 4) & 0xFF; ddl = (ddl + 4) & 0xFF; }
		ColBox *e = &out[n++];
		e->x1 = (int8_t)(ddl + bx - 1); e->y1 = (int8_t)(ddh + by - 1);
		e->x0 = (int8_t)x0; e->y0 = (int8_t)y0;
		e->zt = (int8_t)(zb + zs - 1); e->zb = (int8_t)zb;
		e->solid = 0;
		e->id = d;
	}
	return n;
}
