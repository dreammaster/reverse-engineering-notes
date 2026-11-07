#include "push.h"



typedef struct {
	const ColWorld *w;
	const PushObj *objs;
	int dx, dy, budget;
	int moved[PUSH_MAX], n;
} Pusher;

static int find_obj(const ColWorld *w, int id) {
	for (int i = 0; i < w->nobjs; i++)
		if (w->objs[i].id == id)
			return i;
	return -1;
}

static int push_one(Pusher *p, int i, int depth) {
	const PushObj *o = &p->objs[i];
	p->budget -= o->weight;
	if (p->budget < 0 || depth >= 32)
		return 0;
	ColResult r;
	collision_test(p->w, o->x1 + p->dx, o->y1 + p->dy, o->z0, o->xs, o->ys, o->zs, p->w->objs[i].id, 1, &r);
	if (r.count == 0xFF || r.count == 0xFE || r.flag == 0xFD)
		return 0;
	if (p->n < PUSH_MAX)
		p->moved[p->n++] = i;
	for (int k = 0; k < r.n; k++) {
		int j = find_obj(p->w, r.ids[k]);
		if (j >= 0 && !push_one(p, j, depth + 1))
			return 0;
	}
	return 1;
}

int push_move(const ColWorld *w, const PushObj *objs, int px, int py, int pz, int pxs, int pys, int pzs, int pself, int dx, int dy,
              int strength, int moved[], int *nmoved) {
	ColResult r;
	*nmoved = 0;
	collision_test(w, px + dx, py + dy, pz, pxs, pys, pzs, pself, 1, &r);
	if (r.count == 0xFF || r.count == 0xFE)
		return PUSH_BLOCKED;
	if (r.flag == 0xFD)
		return PUSH_SOLID;
	Pusher p = {w, objs, dx, dy, strength, {0}, 0};
	for (int k = 0; k < r.n; k++) {
		int j = find_obj(w, r.ids[k]);
		if (j >= 0 && !push_one(&p, j, 0))
			return PUSH_FAIL;
	}
	for (int k = 0; k < p.n; k++)
		moved[k] = p.moved[k];
	*nmoved = p.n;
	return p.n ? PUSH_MOVED : PUSH_FREE;
}
