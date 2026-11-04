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

	if (!checked) {
		printf("SKIP: no retail data under %s\n", root);
		return 77;
	}
	printf("%d files checked, %d failed\n", checked, failed);
	return failed != 0;
}
