#include "roomobjs.h"
#include "rooms.h"
#include <string.h>

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

void object_trimmed_box(int x1, int y1, int z0, const uint8_t hdr[32], int8_t out[6]) {
	int x0, y0, zt;
	object_box(x1, y1, z0, hdr, &x0, &y0, &zt);
	int zb = z0;
	if (hdr[0x0E]) { zt -= hdr[0x0E] & 15; zb += hdr[0x0E] >> 4; }
	if (hdr[0x1A]) { x1 -= hdr[0x1A] & 15; x0 += hdr[0x1A] >> 4; }
	if (hdr[0x1B]) { y1 -= hdr[0x1B] & 15; y0 += hdr[0x1B] >> 4; }
	out[0] = (int8_t)x1; out[1] = (int8_t)y1; out[2] = (int8_t)x0; out[3] = (int8_t)y0; out[4] = (int8_t)zt; out[5] = (int8_t)zb;
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

int objsprite_frame_offset(const ObjSprite *sp, int k, int *dx, int *dy) {
	*dx = *dy = 0;
	if (sp->extra_len < 8 || k < 0 || k >= sp->nframes)
		return 0;
	size_t sl = (size_t)((sp->extra[2] << 8) | sp->extra[3]), o = 8 + sl + 8 * (size_t)k + 2;
	if (o + 4 > (size_t)sp->extra_len)
		return 0;
	*dx = (int16_t)((sp->extra[o] << 8) | sp->extra[o + 1]);
	*dy = (int16_t)((sp->extra[o + 2] << 8) | sp->extra[o + 3]);
	return 1;
}

int objsprite_initial_frame(const ObjSprite *sp) {
	if (sp->extra_len < 8)
		return 0;
	size_t sl = (size_t)((sp->extra[2] << 8) | sp->extra[3]);
	for (size_t i = 0; i + 1 < sl && 8 + i + 1 < (size_t)sp->extra_len; i += 2) {
		unsigned w = (unsigned)((sp->extra[8 + i] << 8) | sp->extra[8 + i + 1]);
		if (w < (unsigned)sp->nframes)
			return (int)w;
	}
	return 0;
}

void roomobj_moved(RoomObj *o, int gx, int gy) {
	object_project(o->x1, o->y1, o->z0, o->sp->header, gx, gy, &o->sx, &o->sy);
	object_box(o->x1, o->y1, o->z0, o->sp->header, &o->x0, &o->y0, &o->zt);
	int8_t tb[6];
	object_trimmed_box(o->x1, o->y1, o->z0, o->sp->header, tb);
	o->cx1 = tb[0]; o->cy1 = tb[1]; o->cx0 = tb[2]; o->cy0 = tb[3]; o->czt = tb[4]; o->czb = tb[5];
}

int roomobjs_build(RoomObjs *ro, const Mp2 *mp2, const ObjSprites *objs, int room, int gx, int gy, const uint8_t *thing_flags) {
	ro->n = 0;
	if (room < 0 || room >= mp2->sec[2].count || !mp2->sec[2].rec[room])
		return 0;
	const uint8_t *list = mp2->sec[2].rec[room];
	int n = mp2->sec[2].len[room] / 2;
	for (int i = 0; i < n && ro->n < ROOMOBJS_MAX; i++) {
		int id = (list[2 * i] << 8) | list[2 * i + 1], len;
		const uint8_t *t = mp2_thing(mp2, id, &len);
		int fl = (thing_flags && id < 1000) ? thing_flags[id] : (t ? t[3] : 0);
		if (!t || len < 8 || (fl & 0x80))
			continue;
		const ObjSprite *sp = objsprites_find(objs, (t[6] << 8) | t[7]);
		if (!sp)
			continue;
		RoomObj *o = &ro->o[ro->n++];
		memset(o, 0, sizeof(*o));
		o->thing = id;
		o->x1 = t[0];
		o->y1 = t[1];
		o->z0 = t[2];
		o->sp = sp;
		object_project(o->x1, o->y1, o->z0, sp->header, gx, gy, &o->sx, &o->sy);
		/* animation state = the thing record's tail at +rec[14]: delay, frame, dir, pos, (dx, dy), cnt, cad */
		o->an.cnt = o->an.cad = 0;
		int so = t[14];
		if (anim_script_len(sp) && so >= 16 && so + 10 <= len) {
			o->an.delay = t[so];
			o->an.frame = t[so + 1];
			o->an.dir = t[so + 2];
			o->an.pos = t[so + 3];
			o->an.cnt = t[so + 8];
			o->an.cad = t[so + 9];
		} else if (anim_script_len(sp)) {
			o->an.cnt = o->an.cad = 1;
		}
		object_box(o->x1, o->y1, o->z0, sp->header, &o->x0, &o->y0, &o->zt);
		{
			int8_t tb[6];
			object_trimmed_box(o->x1, o->y1, o->z0, sp->header, tb);
			o->cx1 = tb[0]; o->cy1 = tb[1]; o->cx0 = tb[2]; o->cy0 = tb[3]; o->czt = tb[4]; o->czb = tb[5];
		}
		o->solid = !(sp->header[12] & 1);
		o->weight = sp->header[13];
		o->xs = hw(sp->header, 8); o->ys = hw(sp->header, 9); o->zs = hw(sp->header, 10);
		o->animated = anim_script_len(sp) > 0;
		o->first = objsprite_initial_frame(sp);
	}
	return ro->n;
}

void roomobjs_step(RoomObjs *ro) {
	for (int i = 0; i < ro->n; i++)
		if (ro->o[i].animated)
			anim_step(&ro->o[i].an, ro->o[i].sp, NULL);
}

static void draw_obj(const RoomObj *o, uint8_t *canvas, int use_state) {
	int fk = use_state && o->animated ? o->an.frame : o->first, dx = 0, dy = 0;
	if (o->sp->extra_len)
		objsprite_frame_offset(o->sp, fk, &dx, &dy);
	if (fk < 0 || fk >= o->sp->nframes)
		return;
	const ObjFrame *f = &o->sp->frames[fk];
	int sx = o->sx + dx, sy = o->sy + dy;
	for (int y = 0; y < f->h; y++)
		for (int x = 0; x < f->w; x++) {
			uint8_t c = f->pixels[y * f->w + x];
			int px = sx + x, py = sy + y;
			if (c && px >= 0 && px < VIEW_W && py >= 0 && py < VIEW_H)
				canvas[py * VIEW_W + px] = c;
		}
}

void roomobjs_draw(const RoomObjs *ro, uint8_t *canvas, int use_state) {
	for (int i = 0; i < ro->n; i++)
		draw_obj(&ro->o[i], canvas, use_state);
}

void roomobjs_draw_rel(const RoomObjs *ro, uint8_t *canvas, int use_state, int px1, int py1, int pz0, int pz1, int front) {
	(void)pz0;
	for (int i = 0; i < ro->n; i++) {
		const RoomObj *o = &ro->o[i];
		int infront = o->x0 > px1 || o->y0 > py1 || o->z0 > pz1;
		if (infront == !!front)
			draw_obj(o, canvas, use_state);
	}
}

const uint8_t *objsprites_header(const ObjSprites *o, int block) {
	return (block >= 0 && block < o->nhdr && o->has_hdr[block]) ? o->hdr[block] : NULL;
}

int room_render_objects(const Mp2 *mp2, const ObjSprites *objs, int room, int gx, int gy, uint8_t *canvas) {
	static RoomObjs ro;
	int n = roomobjs_build(&ro, mp2, objs, room, gx, gy, NULL);
	roomobjs_draw(&ro, canvas, 0);
	return n;
}
