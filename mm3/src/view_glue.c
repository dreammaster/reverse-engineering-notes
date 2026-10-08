#include "view_glue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "view_host.h"

/* DGROUP offsets (IDA linear - 286F0h, names/mm3.tsv) */
enum {
	DG_PARTY_FACING = 0xE8F4, DG_PARTY_X = 0xE8F5, DG_PARTY_Y = 0xE8F6, DG_PARTY_MAP = 0xE8F7,
	DG_MAZE_CUR_SLOT = 0xC53E, DG_SLOT_BASE = 0xC554, DG_SLOT_STRIDE = 0x340, DG_MAZE_SLOT_IDS = 0x274E,
	DG_ENGINE_MODE = 0xBF20, DG_WRAP_MODE = 0x015B, DG_PARTY_LIGHT = 0xEC38,
	DG_ANIM_TOGGLE = 0x28875 - 0x286F0, /* byte_28875: flips each step */
	DG_ENV_TABLE = 0x30E1,              /* environment number by map id (map 1 at +1) */
	DG_ENV_PREFIXES = 0x5A34,           /* near pointers to "twn", "cav", "dun", "cas", "sci" */
	DG_WALL_SHEETS = 0xC4A6,            /* far pointer table, 4 bytes per sheet number */
	DG_BACKGROUND = 0xECC4              /* word_373B4: far pointer to the view background */
};

#define MARKER_BASE 0x4000
#define MAX_SETS 64

typedef struct {
	unsigned dg_off; /* far pointer variable it is bound to */
	Mm3Sprite spr;
	int used;
} SetSlot;

struct Mm3View {
	const Mm3Cc *cc, *cur;
	const Mm3Dgroup *dgroup;
	Mm3Palette pal;
	Mm3Page page;
	unsigned map_id;
	SetSlot sets[MAX_SETS];
	uint8_t *screen;
};

static uint16_t marker_for(unsigned dg_off) { return (uint16_t)(MARKER_BASE + (dg_off >> 1)); }

static SetSlot *find_set(Mm3View *v, unsigned seg) {
	for (int i = 0; i < MAX_SETS; i++)
		if (v->sets[i].used && marker_for(v->sets[i].dg_off) == seg)
			return &v->sets[i];
	return NULL;
}

/* load a sprite resource by name and bind it to the far pointer variable at dg_off */
static int bind_set(Mm3View *v, unsigned dg_off, const char *name) {
	size_t len;
	uint8_t *d = mm3_cc_read(v->cc, name, &len);
	SetSlot *s = NULL;
	for (int i = 0; i < MAX_SETS && !s; i++)
		if (!v->sets[i].used || v->sets[i].dg_off == dg_off)
			s = &v->sets[i];
	if (!s) { free(d); return -1; }
	if (s->used) mm3_sprite_free(&s->spr);
	s->used = 0;
	if (!d || mm3_sprite_decode(&s->spr, d, len)) {
		free(d);
		wr16(DG, (uint16_t)dg_off, 0);
		wr16(DG, (uint16_t)(dg_off + 2), 0);
		fprintf(stderr, "warning: cannot load %s\n", name);
		return -1;
	}
	free(d);
	s->used = 1;
	s->dg_off = dg_off;
	wr16(DG, (uint16_t)dg_off, 0);
	wr16(DG, (uint16_t)(dg_off + 2), marker_for(dg_off));
	return 0;
}

Mm3View *mm3_view_create(const Mm3Cc *mm3cc, const Mm3Cc *cur, const Mm3Dgroup *dg) {
	Mm3View *v = calloc(1, sizeof(*v));
	if (!v) return NULL;
	v->cc = mm3cc; v->cur = cur; v->dgroup = dg;
	if (mm3_palette_load(&v->pal, mm3cc)) { free(v); return NULL; }
	memcpy(DG, dg->data, MM3_DGROUP_SIZE);
	return v;
}

void mm3_view_destroy(Mm3View *v) {
	if (!v) return;
	for (int i = 0; i < MAX_SETS; i++)
		if (v->sets[i].used) mm3_sprite_free(&v->sets[i].spr);
	free(v);
}

int mm3_facing_left(int f) { static const int t[4] = {3, 2, 0, 1}; return t[f & 3]; }
int mm3_facing_right(int f) { static const int t[4] = {2, 3, 1, 0}; return t[f & 3]; }
void mm3_facing_step(int f, int *dx, int *dy) {
	static const int sx[4] = {0, 0, 1, -1}, sy[4] = {1, -1, 0, 0};
	*dx = sx[f & 3]; *dy = sy[f & 3];
}
int mm3_facing_wall_side(int f) { static const int t[4] = {MM3_SIDE_N, MM3_SIDE_S, MM3_SIDE_E, MM3_SIDE_W}; return t[f & 3]; }

int mm3_view_set_map(Mm3View *v, unsigned map_id, int x, int y, int facing) {
	char name[32];
	size_t len;
	uint8_t *dat;
	unsigned env;
	char buf[32];
	static const int sheet_numbers[4] = {1, 2, 4, 3};
	snprintf(name, sizeof name, "MAZE%02u.DAT", map_id);
	dat = mm3_cc_read(v->cur, name, &len);
	if (!dat || len < MM3_PAGE_SIZE || mm3_page_load(&v->page, dat, len)) { free(dat); return -1; }
	v->map_id = map_id;
	memcpy(&DG[DG_SLOT_BASE], dat, DG_SLOT_STRIDE);
	free(dat);
	DG[DG_MAZE_CUR_SLOT] = 0;
	DG[DG_MAZE_SLOT_IDS + 0] = (uint8_t)map_id;
	DG[DG_MAZE_SLOT_IDS + 1] = DG[DG_MAZE_SLOT_IDS + 2] = DG[DG_MAZE_SLOT_IDS + 3] = 0xFF;
	DG[DG_PARTY_MAP] = (uint8_t)map_id;
	DG[DG_ENGINE_MODE] = 1;
	DG[DG_WRAP_MODE] = 0;
	wr16(DG, DG_PARTY_LIGHT, 999);
	mm3_view_set_party(v, x, y, facing);

	/* environment graphics (docs/view.md "Environment graphics"): four wall sheets, background */
	env = DG[DG_ENV_TABLE + map_id];
	snprintf(name, sizeof name, "%s", (const char *)&DG[rd16(DG, (uint16_t)(DG_ENV_PREFIXES + 2 * env))]);
	for (int i = 0; i < 4; i++) {
		snprintf(buf, sizeof buf, "%swl%d.vga", name, sheet_numbers[i]);
		bind_set(v, DG_WALL_SHEETS + 4 * sheet_numbers[i], buf);
	}
	/* background: day/night picture on town maps, else <env>.sky (the day/night rule is not implemented: always day) */
	if (map_id < 6 || (map_id >= 24 && map_id <= 28))
		bind_set(v, DG_BACKGROUND, "DAY.VGA");
	else {
		snprintf(buf, sizeof buf, "%s.sky", name);
		bind_set(v, DG_BACKGROUND, buf);
	}
	/* HUD and effect sprite sets (names stored in the original at DGROUP 2E63h...) */
	bind_set(v, 0xABDA, "global.icn");
	bind_set(v, 0xC536, "gargoyle.brd");
	bind_set(v, 0xE8C6, "grabber.brd");
	bind_set(v, 0xAC18, "bat.brd");
	bind_set(v, 0xA77A, "fecp.brd");
	bind_set(v, 0xAD4A, "protect.icn");
	bind_set(v, 0xACD0, "hpbars.icn");
	bind_set(v, 0xC532, "restore.icn");
	bind_set(v, 0xECDC, "charpow.icn");
	return 0;
}

void mm3_view_set_party(Mm3View *v, int x, int y, int facing) {
	(void)v;
	DG[DG_PARTY_X] = (uint8_t)x; DG[DG_PARTY_Y] = (uint8_t)y; DG[DG_PARTY_FACING] = (uint8_t)facing;
}
int mm3_view_x(const Mm3View *v) { (void)v; return DG[DG_PARTY_X]; }
int mm3_view_y(const Mm3View *v) { (void)v; return DG[DG_PARTY_Y]; }
int mm3_view_facing(const Mm3View *v) { (void)v; return DG[DG_PARTY_FACING]; }
void mm3_view_toggle_animation(Mm3View *v) { (void)v; DG[DG_ANIM_TOGGLE] ^= 1; }
const Mm3Page *mm3_view_page(const Mm3View *v) { return &v->page; }

/* ---- draw list compositing */
static void draw_list(void *user, unsigned addr) {
	Mm3View *v = user;
	SetSlot *cur = NULL;
	for (int n = 0; n < 4000; n++) {
		unsigned w = rd16(DG, (uint16_t)addr);
		if (w == 0xFFFF) {
			unsigned seg = rd16(DG, (uint16_t)(addr + 4));
			addr += 6;
			if (seg == 0) break;
			cur = find_set(v, seg);
		} else {
			int x = (int16_t)rd16(DG, (uint16_t)addr), y = (int16_t)rd16(DG, (uint16_t)(addr + 2));
			unsigned flags = rd16(DG, (uint16_t)(addr + 4)), frame = rd16(DG, (uint16_t)(addr + 6));
			addr += 8;
			/* flag bit 0 = mirrored; bits 8-9 (distance scale) and bit 15 (enlarge) are not implemented yet */
			if (cur && frame < cur->spr.count)
				mm3_blit_layers(v->screen, MM3_SCREEN_W, MM3_SCREEN_H, &cur->spr.frames[frame], x, y, flags & 1);
		}
	}
}

void mm3_view_render(Mm3View *v, uint8_t *screen) {
	v->screen = screen;
	mm3_view_set_list_callback(draw_list, v);
	mm3_view_run();
}

const Mm3Palette *mm3_view_palette(const Mm3View *v) { return &v->pal; }
