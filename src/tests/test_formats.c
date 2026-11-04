/*
 * test_formats - checks the C decoders against vectors produced by the
 * independent Python reference implementation (tools/gen_test_vectors.py).
 *
 *   test_formats <repo root>      exit 77 (skip) if the retail data is absent
 */
#include "../engine/mjm.h"
#include "../engine/pc1.h"
#include "../engine/lng.h"
#include "../engine/sprites.h"
#include "../engine/mp2.h"
#include "../engine/rooms.h"
#include "../engine/intro.h"
#include "../engine/objsprites.h"
#include "../engine/roomobjs.h"
#include "../engine/anim.h"
#include "vectors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned fnv(const uint8_t *b, size_t n) {
	unsigned h = 0x811C9DC5u;
	for (size_t i = 0; i < n; i++)
		h = (h ^ b[i]) * 0x01000193u;
	return h;
}

static uint8_t *slurp(const char *path, size_t *len) {
	FILE *f = fopen(path, "rb");
	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t *d = (uint8_t *)malloc(n ? n : 1);
	if (fread(d, 1, n, f) != (size_t)n) { free(d); fclose(f); return NULL; }
	fclose(f);
	*len = (size_t)n;
	return d;
}

int main(int argc, char **argv) {
	const char *root = argc > 1 ? argv[1] : ".";
	int checked = 0, failed = 0;
	char path[1024];

	for (size_t i = 0; i < sizeof(mjm_vectors) / sizeof(mjm_vectors[0]); i++) {
		const MjmVector *v = &mjm_vectors[i];
		snprintf(path, sizeof(path), "%s/%s", root, v->path);
		size_t n, un;
		uint8_t *in = slurp(path, &n), *out = NULL;
		if (!in)
			continue;
		int rc = mjm_unpack(in, n, &out, &un);
		if (v->size < 0) {
			if (rc == 0) { printf("FAIL %s: expected decode failure\n", v->path); failed++; }
		} else if (rc != 0 || (long)un != v->size || fnv(out, un) != v->hash) {
			printf("FAIL %s: rc=%d size=%zu\n", v->path, rc, rc ? 0 : un);
			failed++;
		}
		checked++;
		free(in);
		free(out);
	}

	for (size_t i = 0; i < sizeof(pic_vectors) / sizeof(pic_vectors[0]); i++) {
		snprintf(path, sizeof(path), "%s/%s", root, pic_vectors[i].path);
		size_t n;
		uint8_t *in = slurp(path, &n);
		if (!in)
			continue;
		static Pc1Image img;
		int rc = pc1_decode(in, n, &img);
		if (rc || fnv(img.pixels, sizeof(img.pixels)) != pic_vectors[i].pixhash) {
			printf("FAIL %s: rc=%d\n", pic_vectors[i].path, rc);
			failed++;
		}
		checked++;
		free(in);
	}

	for (size_t i = 0; i < sizeof(lng_vectors) / sizeof(lng_vectors[0]); i++) {
		snprintf(path, sizeof(path), "%s/%s", root, lng_vectors[i].path);
		size_t n, un;
		uint8_t *in = slurp(path, &n), *raw = NULL;
		if (!in)
			continue;
		LngTable t;
		int rc = mjm_unpack(in, n, &raw, &un);
		if (rc == 0)
			rc = lng_parse(raw, un, &t);
		unsigned h = 0x811C9DC5u;
		if (rc == 0) {
			for (int k = 0; k < t.count; k++) {
				for (const char *c = t.strings[k]; ; c++) {
					h = (h ^ (uint8_t)*c) * 0x01000193u;
					if (!*c)
						break;
				}
			}
		}
		if (rc || t.count != lng_vectors[i].count || h != lng_vectors[i].hash) {
			printf("FAIL %s: rc=%d\n", lng_vectors[i].path, rc);
			failed++;
		}
		if (rc == 0)
			lng_free(&t);
		checked++;
		free(in);
		free(raw);
	}

	for (size_t i = 0; i < sizeof(sprite_vectors) / sizeof(sprite_vectors[0]); i++) {
		snprintf(path, sizeof(path), "%s/%s", root, sprite_vectors[i].path);
		size_t n, un;
		uint8_t *in = slurp(path, &n), *raw = NULL;
		if (!in)
			continue;
		static SpriteSet set;
		int rc = mjm_unpack(in, n, &raw, &un);
		if (rc == 0)
			rc = pcman_load(raw, un, &set);
		unsigned h = 0x811C9DC5u;
		if (rc == 0) {
			for (int k = 0; k < set.count; k++) {
				const Sprite *sp = &set.sprites[k];
				int hdr[4] = {sp->hot_x, sp->hot_y, sp->w, sp->h};
				uint8_t b[12];
				/* hot_x/hot_y are int16 little endian, w/h int32 little endian (matches the Python vector) */
				b[0] = (uint8_t)hdr[0]; b[1] = (uint8_t)(hdr[0] >> 8);
				b[2] = (uint8_t)hdr[1]; b[3] = (uint8_t)(hdr[1] >> 8);
				for (int j = 0; j < 4; j++) { b[4 + j] = (uint8_t)(hdr[2] >> (8 * j)); b[8 + j] = (uint8_t)(hdr[3] >> (8 * j)); }
				for (int j = 0; j < 12; j++)
					h = (h ^ b[j]) * 0x01000193u;
				for (int j = 0; j < sp->w * sp->h; j++)
					h = (h ^ sp->pixels[j]) * 0x01000193u;
			}
		}
		if (rc || set.count != sprite_vectors[i].count || h != sprite_vectors[i].hash) {
			printf("FAIL %s: rc=%d\n", sprite_vectors[i].path, rc);
			failed++;
		}
		if (rc == 0)
			spriteset_free(&set);
		checked++;
		free(in);
		free(raw);
	}

	for (size_t i = 0; i < sizeof(mp2_vectors) / sizeof(mp2_vectors[0]); i++) {
		snprintf(path, sizeof(path), "%s/%s", root, mp2_vectors[i].path);
		size_t n, un;
		uint8_t *in = slurp(path, &n), *raw = NULL;
		if (!in)
			continue;
		Mp2 m;
		int rc = mjm_unpack(in, n, &raw, &un);
		if (rc == 0)
			rc = mp2_parse(raw, un, &m);
		unsigned h = 0x811C9DC5u;
#define HASH32(v) do { uint32_t x_ = (uint32_t)(v); for (int b_ = 0; b_ < 4; b_++) h = (h ^ ((x_ >> (8 * b_)) & 0xFF)) * 0x01000193u; } while (0)
		if (rc == 0) {
			for (int s = 0; s < MP2_SECTIONS; s++) {
				HASH32(m.sec[s].count); HASH32(m.sec[s].data_len); HASH32(m.sec[s].alloc_len);
				for (int k = 0; k < m.sec[s].count; k++) {
					HASH32(m.sec[s].len[k]);
					for (int j = 0; j < m.sec[s].len[k]; j++)
						h = (h ^ (m.sec[s].rec[k] ? m.sec[s].rec[k][j] : 0)) * 0x01000193u;
				}
			}
			mp2_free(&m);
		}
		if (rc || h != mp2_vectors[i].hash) {
			printf("FAIL %s: rc=%d\n", mp2_vectors[i].path, rc);
			failed++;
		}
		checked++;
		free(in);
		free(raw);
	}

	for (size_t i = 0; i < sizeof(room_vectors) / sizeof(room_vectors[0]); i++) {
		const RoomVector *rv = &room_vectors[i];
		const char *names[3] = {"LEV?.MP1", "LEV?.MP2", "LEV?.MP3"};
		uint8_t *raw[3] = {0, 0, 0};
		size_t rl[3] = {0, 0, 0};
		int rc = 0, have = 1;
		char lev = rv->dir[strlen(rv->dir) - 1];
		for (int f = 0; f < 3 && rc == 0; f++) {
			char nm[64];
			snprintf(nm, sizeof(nm), "LEV%c.MP%d", lev, f + 1);
			snprintf(path, sizeof(path), "%s/%s/%s", root, rv->dir, nm);
			size_t n;
			uint8_t *in = slurp(path, &n);
			if (!in) { have = 0; break; }
			rc = mjm_unpack(in, n, &raw[f], &rl[f]);
			free(in);
		}
		(void)names;
		if (have && rc == 0) {
			TileSet ts;
			FloorMaps fm, wr;
			Mp2 m;
			TileSet wt;
			ObjSprites os;
			memset(&os, 0, sizeof(os));
			rc = tiles_load(raw[2], rl[2], &ts);
			if (rc == 0) { rc = walltiles_load(raw[2], rl[2], &wt); if (rc) tiles_free(&ts); }
			if (rc == 0) rc = floormaps_parse(raw[0], rl[0], &fm);
			if (rc == 0) { rc = wallrecs_parse(raw[0], rl[0], &wr); if (rc) floormaps_free(&fm); }
			if (rc == 0) rc = mp2_parse(raw[1], rl[1], &m);
			if (rc == 0) rc = objsprites_load(raw[0], rl[0], &os);
			unsigned h = 0x811C9DC5u;
			int nrooms = 0;
			if (rc == 0) {
				for (int j = 0; j < ts.count * TILE_W * TILE_H; j++)
					h = (h ^ ts.pixels[j]) * 0x01000193u;
				for (int j = 0; j < wt.count * wt.w * wt.h; j++)
					h = (h ^ wt.pixels[j]) * 0x01000193u;
				uint16_t pal[16];
				if (mp1_palette(raw[0], rl[0], pal) != 0)
					rc = -98;
				for (int j = 0; j < 16; j++) {
					h = (h ^ (pal[j] & 0xFF)) * 0x01000193u;
					h = (h ^ (pal[j] >> 8)) * 0x01000193u;
				}
				for (int r = 0; r < fm.count && r < m.sec[0].count; r++) {
					int rw = 0, rh = 0;
					if (m.sec[0].len[r] >= 6 && m.sec[0].rec[r]) { rw = m.sec[0].rec[r][4]; rh = m.sec[0].rec[r][5]; }
					if (!(rw * rh == fm.len[r] && fm.len[r]))
						continue;
					int cw, chh;
					floor_canvas_size(rw, rh, &cw, &chh);
					uint8_t *cv = (uint8_t *)malloc((size_t)cw * chh);
					memset(cv, TILE_TRANSPARENT, (size_t)cw * chh);
					floor_render(&ts, fm.map[r], rw, rh, cv);
					uint32_t hdr[3] = {(uint32_t)r, (uint32_t)cw, (uint32_t)chh};
					for (int a = 0; a < 3; a++)
						for (int b = 0; b < 4; b++)
							h = (h ^ ((hdr[a] >> (8 * b)) & 0xFF)) * 0x01000193u;
					for (int j = 0; j < cw * chh; j++)
						h = (h ^ cv[j]) * 0x01000193u;
					free(cv);
					if (rw >= 3 && rw <= 10 && rh >= 3 && rh <= 10) {
						static uint8_t shell[VIEW_W * VIEW_H];
						memset(shell, TILE_TRANSPARENT, sizeof(shell));
						room_render_shell(&ts, &wt, fm.map[r], r < wr.count ? wr.map[r] : NULL, r < wr.count ? wr.len[r] : 0, rw, rh, shell);
						for (int j = 0; j < VIEW_W * VIEW_H; j++)
							h = (h ^ shell[j]) * 0x01000193u;
						int gx, gy;
						floor_origin(rw, rh, &gx, &gy);
						room_render_objects(&m, &os, r, gx, gy, shell);
						for (int j = 0; j < VIEW_W * VIEW_H; j++)
							h = (h ^ shell[j]) * 0x01000193u;
					}
					nrooms++;
				}
				if (ts.count != rv->tiles || nrooms != rv->rooms || h != rv->hash)
					rc = -99;
				tiles_free(&ts); tiles_free(&wt); floormaps_free(&fm); floormaps_free(&wr); mp2_free(&m); objsprites_free(&os);
			}
			if (rc) { printf("FAIL %s: rc=%d (rooms %d)\n", rv->dir, rc, nrooms); failed++; }
			checked++;
		}
		for (int f = 0; f < 3; f++) free(raw[f]);
	}

	for (size_t i = 0; i < sizeof(obj_vectors) / sizeof(obj_vectors[0]); i++) {
		const ObjVector *ov = &obj_vectors[i];
		char lev = ov->dir[strlen(ov->dir) - 1];
		snprintf(path, sizeof(path), "%s/%s/LEV%c.MP1", root, ov->dir, lev);
		size_t n, un;
		uint8_t *in = slurp(path, &n), *raw = NULL;
		if (!in)
			continue;
		ObjSprites os;
		int rc = mjm_unpack(in, n, &raw, &un);
		if (rc == 0)
			rc = objsprites_load(raw, un, &os);
		unsigned h = 0x811C9DC5u;
		if (rc == 0) {
			for (int k = 0; k < os.count; k++) {
				const ObjSprite *sp = &os.sprites[k];
				uint32_t hd[3] = {(uint32_t)sp->block, (uint32_t)sp->nframes, (uint32_t)sp->extra_len};
				for (int a = 0; a < 3; a++)
					for (int b = 0; b < 4; b++)
						h = (h ^ ((hd[a] >> (8 * b)) & 0xFF)) * 0x01000193u;
				for (int j = 0; j < 32; j++)
					h = (h ^ sp->header[j]) * 0x01000193u;
				for (int j = 0; j < sp->extra_len; j++)
					h = (h ^ sp->extra[j]) * 0x01000193u;
				for (int f = 0; f < sp->nframes; f++) {
					uint32_t fd[2] = {(uint32_t)sp->frames[f].w, (uint32_t)sp->frames[f].h};
					for (int a = 0; a < 2; a++)
						for (int b = 0; b < 4; b++)
							h = (h ^ ((fd[a] >> (8 * b)) & 0xFF)) * 0x01000193u;
					for (int j = 0; j < sp->frames[f].w * sp->frames[f].h; j++)
						h = (h ^ sp->frames[f].pixels[j]) * 0x01000193u;
				}
				if (sp->extra_len) {
					AnimState as = {0, 0, 0, 0, 1, 1, 0, 0};
					for (int s = 0; s < 300; s++) {
						anim_step(&as, sp, NULL);
						int32_t v[6] = {as.pos, as.frame, as.delay, as.dir, as.dx, as.dy};
						for (int a = 0; a < 6; a++)
							for (int b = 0; b < 4; b++)
								h = (h ^ (((uint32_t)v[a] >> (8 * b)) & 0xFF)) * 0x01000193u;
					}
				}
			}
			if (os.count != ov->sprites || h != ov->hash)
				rc = -99;
			objsprites_free(&os);
		}
		if (rc) { printf("FAIL objsprites %s: rc=%d\n", ov->dir, rc); failed++; }
		checked++;
		free(in);
		free(raw);
	}

	for (size_t i = 0; i < sizeof(intro_vectors) / sizeof(intro_vectors[0]); i++) {
		size_t n1, n2, n3, n4;
		char pp[1024];
		snprintf(pp, sizeof(pp), "%s/%s/LOADING2.PC1", root, intro_vectors[i].dir);
		uint8_t *b1 = slurp(pp, &n1);
		snprintf(pp, sizeof(pp), "%s/%s/FONT2.SPR", root, intro_vectors[i].dir);
		uint8_t *b2 = slurp(pp, &n2);
		snprintf(pp, sizeof(pp), "%s/%s/FLAME.DAT", root, intro_vectors[i].dir);
		uint8_t *b3 = slurp(pp, &n3);
		(void)n4;
		if (b1 && b2 && b3) {
			static Pc1Image bg;
			static Font font;
			static Flames fl;
			static Screen scr;
			int rc = pc1_decode(b1, n1, &bg);
			if (rc == 0) rc = font_load(b2, n2, &font);
			if (rc == 0) rc = flames_load(b3, n3, &fl);
			unsigned h = 0x811C9DC5u;
			if (rc == 0)
				for (int lf = 0; lf < 6; lf++) {
					intro_render_title(&scr, &bg, &font, &fl, strstr(intro_vectors[i].dir, "2") != NULL, lf);
					for (int j = 0; j < SCREEN_W * SCREEN_H; j++)
						h = (h ^ scr.pixels[j]) * 0x01000193u;
				}
			if (rc || h != intro_vectors[i].hash) { printf("FAIL intro %s: rc=%d\n", intro_vectors[i].dir, rc); failed++; }
			checked++;
		}
		free(b1); free(b2); free(b3);
	}

	{
		int x, y;
		if (floor_origin(10, 10, &x, &y) != 0 || x != 144 || y != 29 || floor_origin(3, 3, &x, &y) != 0 || x != 144 || y != 80 ||
		    floor_origin(2, 5, &x, &y) == 0) {
			printf("FAIL floor_origin\n");
			failed++;
		}
		checked++;
	}

	if (!checked) {
		printf("SKIP: no retail data under %s\n", root);
		return 77;
	}
	printf("%d files checked, %d failed\n", checked, failed);
	return failed != 0;
}
