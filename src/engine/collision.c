#include "collision.h"

static int s8(int v) { return (int8_t)(v & 0xFF); }

/* the game's inclusive interval test on signed bytes: mover [lo, hi] against [e0, e1] */
static int overlap(int hi, int lo, int e1, int e0) {
	hi = s8(hi); lo = s8(lo); e1 = s8(e1); e0 = s8(e0);
	if (hi >= e1)
		return lo <= e1;
	return hi >= e0;
}

static int exit_lookup(const ColWorld *w, int is_player, int dl, int dh, int cl, int bl, int bh, int ch, int *di) {
	*di = 0;
	if (w->nexits <= 0)
		return 0xFF;
	for (int i = 0; i <= w->nexits; i++) {
		if (i == w->nexits)
			break;                          /* the terminator slot never matches */
		const ColBox *e = &w->exits[i];
		if (!overlap(dl, cl, e->x1, e->x0) || !overlap(dh, bl, e->y1, e->y0) || !overlap(bh, ch, e->zt, e->zb))
			continue;
		int hit = 1;
		if (is_player) {
			int zok = s8(ch) >= e->zb && s8(bh) <= e->zt;
			if (w->dirbits)
				hit = (cl & 0xFF) >= (e->x0 & 0xFF) && (dl & 0xFF) < (e->x1 & 0xFF) && zok;
			else
				hit = (bl & 0xFF) >= (e->y0 & 0xFF) && (dh & 0xFF) < (e->y1 & 0xFF) && zok;
		}
		if (hit) {
			*di = e->id;
			return 0xFE;
		}
	}
	return 0xFF;
}

void collision_test(const ColWorld *w, int x1, int y1, int z, int xs, int ys, int zs, int self, int is_player, ColResult *r) {
	int dl = x1 & 0xFF, dh = y1 & 0xFF, ch = z & 0xFF;
	int cl = (dl - xs + 1) & 0xFF, bl = (dh - ys + 1) & 0xFF, bh = (ch + zs - 1) & 0xFF;
	r->zf = 1; r->count = 0; r->flag = 0; r->n = 0; r->exit_id = 0;
	int out = s8(ch) < 0 || bh >= 0xF0 || s8(dl) >= w->w8 || s8(dh) >= w->h8 ||
	          (!(s8(bl) > w->wally) && s8(cl) <= w->wallx) || s8(cl) < w->thick_x || !(s8(bl) >= w->thick_y);
	if (out) {
		r->zf = 0;
		r->count = exit_lookup(w, is_player, dl, dh, cl, bl, bh, ch, &r->exit_id);
		return;
	}
	if (w->nzones && !(s8(cl) >= 8 && s8(bl) >= 8))
		for (int z4 = 0; z4 < w->nzones; z4++) {
			const ColRect *q = &w->zones[z4];
			if (overlap(dl, cl, q->x1, q->x0) && overlap(dh, bl, q->y1, q->y0)) {
				r->zf = 0;
				r->count = 0xFF;
				return;
			}
		}
	for (int i = 0; i < w->nobjs; i++) {
		const ColBox *o = &w->objs[i];
		if (o->id == self)
			continue;
		if (overlap(dl, cl, o->x1, o->x0) && overlap(dh, bl, o->y1, o->y0) && overlap(bh, ch, o->zt, o->zb)) {
			if (r->n < COL_MAX)
				r->ids[r->n++] = o->id;
			if (o->solid)
				r->flag = 0xFD;
		}
	}
	r->count = r->n;
	r->zf = r->n == 0;
}
