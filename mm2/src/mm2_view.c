#include "mm2_view.h"

#include <stdio.h>
#include <string.h>

/* DGROUP:153E.. tables (x, y per depth index 0..3) */
static const int A_X[4] = {32, 64, 88, 104}, A_Y[4] = {22, 40, 54, 62};
static const int BL_X[4] = {8, 32, 64, 88}, BL_Y[4] = {8, 22, 40, 54};
static const int BO_IMG[4] = {12, 14, 2, 3}, BO_X[4] = {8, 8, 40, 88}, BO_Y[4] = {22, 40, 54, 62};
static const int CL_X[4] = {192, 160, 136, 120}, CL_Y[4] = {8, 22, 40, 54};
static const int CO_IMG[4] = {13, 15, 2, 3}, CO_X[4] = {192, 160, 136, 120}, CO_Y[4] = {22, 40, 54, 62};

int mm2_wall_side(uint8_t w, char side) {
	switch (side) {
	case 'N': return (w >> 6) & 3;
	case 'E': return (w >> 4) & 3;
	case 'S': return (w >> 2) & 3;
	default: return w & 3;
	}
}

void mm2_facing_delta(char f, int *dx, int *dy) {
	*dx = *dy = 0;
	switch (f) {
	case 'N': *dy = 1; break;
	case 'S': *dy = -1; break;
	case 'E': *dx = 1; break;
	default: *dx = -1; break;
	}
}

typedef struct {
	int fm, fs;   /* front mask / shift */
	int lm, ls;   /* left side */
	int rm, rs;   /* right side */
	int sx, sy;   /* step */
	int lx, ly;   /* left lane offset */
	int rx, ry;   /* right lane offset */
} Face;

static void face(char f, Face *o) {
	static const Face N = {0xC0, 6, 0x03, 0, 0x30, 4, 0, 1, -1, 0, 1, 0};
	static const Face S = {0x0C, 2, 0x30, 4, 0x03, 0, 0, -1, 1, 0, -1, 0};
	static const Face E = {0x30, 4, 0xC0, 6, 0x0C, 2, 1, 0, 0, 1, 0, -1};
	static const Face W = {0x03, 0, 0x0C, 2, 0xC0, 6, -1, 0, 0, -1, 0, 1};
	switch (f) {
	case 'N': *o = N; break;
	case 'S': *o = S; break;
	case 'E': *o = E; break;
	default: *o = W; break;
	}
}

/* wall variables: kind 0 L, 1 LO (left-lane front), 2 R, 3 RO (right-lane front), 4 F; depth 1..4 */
typedef struct {
	uint8_t v[5][6];
} Walls;

static uint8_t cell(const uint8_t *w, int x, int y) {
	return w[((y & 15) << 4) | (x & 15)];
}

static void collect(const uint8_t *walls, int x, int y, char facing, Walls *out) {
	Face f;
	uint8_t ctr[4], lft[4], rgt[4];
	int d, k;
	face(facing, &f);
	memset(out, 0, sizeof(*out));
	for (d = 0; d < 4; d++) {
		ctr[d] = cell(walls, x + f.sx * d, y + f.sy * d);
		lft[d] = cell(walls, x + f.lx + f.sx * d, y + f.ly + f.sy * d);
		rgt[d] = cell(walls, x + f.rx + f.sx * d, y + f.ry + f.sy * d);
	}
	for (d = 0; d < 4; d++) {
		int n = d + 1, v, o;
		v = (ctr[d] & f.lm) >> f.ls;
		if (v)
			out->v[0][n] = (uint8_t)v;
		else if ((o = (lft[d] & f.fm) >> f.fs) != 0)
			out->v[1][n] = (uint8_t)o;
		v = (ctr[d] & f.rm) >> f.rs;
		if (v)
			out->v[2][n] = (uint8_t)v;
		else if ((o = (rgt[d] & f.fm) >> f.fs) != 0)
			out->v[3][n] = (uint8_t)o;
		v = (ctr[d] & f.fm) >> f.fs;
		if (v) {
			out->v[4][n] = (uint8_t)v;
			if (d < 3) {
				if (!out->v[0][n] && !out->v[1][n] && (o = (lft[d + 1] & f.fm) >> f.fs) != 0)
					out->v[1][n + 1] = (uint8_t)o;
				if (!out->v[2][n] && !out->v[3][n] && (o = (rgt[d + 1] & f.fm) >> f.fs) != 0)
					out->v[3][n + 1] = (uint8_t)o;
			}
			break;
		}
	}
	for (k = 0; k < 4; k += 2)
		for (d = 1; d <= 2; d++)
			if (out->v[k][d] && out->v[k + 1][d + 1] == 3)
				out->v[k + 1][d + 1] = 1;
}

static int load(Mm2Bank *b, const Mm2Game *g, const char *name, int bpp) {
	char file[64];
	snprintf(file, sizeof(file), "%s.%s", name, bpp == 4 ? "16" : "4");
	return mm2_bank_load(g, file, bpp, b);
}

int mm2_view_load_indoor(Mm2View *v, const Mm2Game *g, const char *style) {
	char name[32];
	memset(v, 0, sizeof(*v));
	snprintf(name, sizeof(name), "%sF", style);
	return load(&v->walls, g, style, 4) && load(&v->floor, g, name, 4) && load(&v->sky, g, "SKY", 4);
}

int mm2_view_load_outdoor(Mm2View *v, const Mm2Game *g, const char *special) {
	memset(v, 0, sizeof(*v));
	return load(&v->tiles[0], g, "OUTDOOR1", 4) && load(&v->tiles[1], g, "OUTDOOR2", 4) &&
		   load(&v->tiles[2], g, "OUTDOOR3", 4) && load(&v->special, g, special, 4) &&
		   load(&v->ground, g, "OUTF", 4) && load(&v->sky, g, "SKY", 4);
}

void mm2_view_free(Mm2View *v) {
	int i;
	mm2_bank_free(&v->walls);
	mm2_bank_free(&v->floor);
	mm2_bank_free(&v->sky);
	for (i = 0; i < 3; i++)
		mm2_bank_free(&v->tiles[i]);
	mm2_bank_free(&v->special);
	mm2_bank_free(&v->ground);
}

static int door(int value) {
	return value == 2 ? 0x10 : 0;
}

/* TODO(review): sky image: view_indoor_daylight (ovl/2PLAY.asm IDA 0x18510) decides between SKY images 0 and 1 from a bit
 * table at DGROUP:59A6 and the party position; I always draw image 1.  Night stars (sub_14FB2) and the queued wall sprites for
 * wall value 3 (view_queue_sprite_a..e, 0x1830E..0x18510, TOWNT/TOWNB banks) are not drawn. */
void mm2_view_render_indoor(Mm2View *v, uint8_t *canvas, const uint8_t *walls, int x, int y, char facing) {
	Walls w;
	int n;
	memset(canvas, 0, MM2_SCREEN_W * MM2_SCREEN_H);
	mm2_blit(canvas, mm2_bank_image(&v->sky, 1), 8, 8);
	mm2_blit(canvas, mm2_bank_image(&v->floor, 0), 8, 0x44);
	collect(walls, x, y, facing, &w);
	for (n = 4; n >= 1; n--) {
		int i = n - 1;
		if (w.v[0][n]) mm2_blit(canvas, mm2_bank_image(&v->walls, i + 4 + door(w.v[0][n])), BL_X[i], BL_Y[i]);
		if (w.v[1][n]) mm2_blit(canvas, mm2_bank_image(&v->walls, BO_IMG[i] + door(w.v[1][n])), BO_X[i], BO_Y[i]);
		if (w.v[2][n]) mm2_blit(canvas, mm2_bank_image(&v->walls, i + 8 + door(w.v[2][n])), CL_X[i], CL_Y[i]);
		if (w.v[3][n]) mm2_blit(canvas, mm2_bank_image(&v->walls, CO_IMG[i] + door(w.v[3][n])), CO_X[i], CO_Y[i]);
		if (w.v[4][n]) mm2_blit(canvas, mm2_bank_image(&v->walls, i + door(w.v[4][n])), A_X[i], A_Y[i]);
	}
}

/* ---- outdoors (docs/view.md) ---- */
static const uint8_t TERRAIN[32] = {0, 1, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0};
static const int H_Y[4] = {20, 35, 50, 60}, H_RX[4] = {184, 160, 136, 112};
static const int C_IMG[4] = {0, 0, 1, 2}, C_X[4] = {40, 40, 64, 88}, C_Y[4] = {21, 21, 42, 50};
static const int L_IMG[4] = {4, 5, 2, 3}, L_X[4] = {8, 16, 32, 88}, L_Y[4] = {36, 46, 50, 58};
static const int L_ALT_IMG[3] = {4, 5, 2}, L_ALT_X[3] = {8, 16, 32};
static const int R_IMG[4] = {6, 7, 2, 3}, R_X[4] = {176, 152, 136, 120}, R_Y[4] = {36, 46, 50, 58};
static const int R_ALT_IMG[3] = {6, 7, 2}, R_ALT_X[3] = {176, 152, 136};
static const int ALT_Y[4] = {21, 21, 42, 50};

static int terrain(const uint8_t *w, int x, int y) {
	return TERRAIN[cell(w, x, y) & 0x1F];
}

/* TODO(review): transcribed from ovl/2PLAY.asm sub_189B8 (horizon strips, 0x189B8), sub_18AD0/18B0C/18BEC (tiles) and sub_18CC6
 * (0x18CC6); the two x overrides keyed on byte_22D04 / byte_22D08 (0x18B90, 0x18C8F) are not ported, the sky image is always 0
 * and the terrain bank is chosen by the caller. */
void mm2_view_render_outdoor(Mm2View *v, uint8_t *canvas, const uint8_t *walls, int x, int y, char facing) {
	Face f;
	int cen[4], lef[4], rig[4], d, n, nearest, side;
	memset(canvas, 0, MM2_SCREEN_W * MM2_SCREEN_H);
	mm2_blit(canvas, mm2_bank_image(&v->sky, 0), 8, 8);
	mm2_blit(canvas, mm2_bank_image(&v->ground, 0), 8, 0x44);
	face(facing, &f);
	for (d = 0; d < 4; d++) {
		cen[d] = terrain(walls, x + f.sx * d, y + f.sy * d);
		lef[d] = terrain(walls, x + f.lx + f.sx * d, y + f.ly + f.sy * d);
		rig[d] = terrain(walls, x + f.rx + f.sx * d, y + f.ry + f.sy * d);
	}
	for (d = 0; d < 4; d++) {
		int yy = 0x80 - H_Y[d], c4 = cen[d] > 3, l4 = lef[d] > 3, r4 = rig[d] > 3;
		if (c4) {
			mm2_blit(canvas, mm2_bank_image(&v->special, d), 8, yy);
			if (l4) mm2_blit(canvas, mm2_bank_image(&v->special, d + 12), 8, yy);
			if (r4) mm2_blit(canvas, mm2_bank_image(&v->special, d + 16), H_RX[d], yy);
		} else {
			if (l4) mm2_blit(canvas, mm2_bank_image(&v->special, d + 4), 8, yy);
			if (r4) mm2_blit(canvas, mm2_bank_image(&v->special, d + 8), 0x70, yy);
		}
		if (c4) cen[d] = 0;
		if (l4) lef[d] = 0;
		if (r4) rig[d] = 0;
	}
	for (d = 0; d < 4; d++) {
		cen[d]--;
		lef[d]--;
		rig[d]--; /* -1 = empty */
	}
	n = 4;
	for (d = 0; d < 4; d++)
		if (cen[d] >= 0) {
			n = d;
			break;
		}
	nearest = n == 4 ? 3 : n;
	if (n != 4)
		mm2_blit(canvas, mm2_bank_image(&v->tiles[cen[n]], C_IMG[n]), C_X[n], C_Y[n]);
	for (d = nearest; d >= 0; d--)
		for (side = 0; side < 2; side++) {
			int *arr = side == 0 ? lef : rig;
			int hide = d != 0 && d == nearest && cen[d] >= 0;
			int img, px, py;
			if (hide && arr[d - 1] >= 0) arr[d] = -1;
			if (arr[d] < 0) continue;
			if (side == 0) {
				img = L_IMG[d]; px = L_X[d]; py = L_Y[d];
				if (hide) { img = L_ALT_IMG[d - 1]; px = L_ALT_X[d - 1]; py = ALT_Y[d]; }
				if (d == 1 || (d == 2 && nearest == 2)) px = 8;
			} else {
				img = R_IMG[d]; px = R_X[d]; py = R_Y[d];
				if (hide) { img = R_ALT_IMG[d - 1]; px = R_ALT_X[d - 1]; py = ALT_Y[d]; }
			}
			mm2_blit(canvas, mm2_bank_image(&v->tiles[arr[d]], img), px, py);
		}
}
