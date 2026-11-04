/*
 * test_formats - checks the C decoders against vectors produced by the
 * independent Python reference implementation (tools/gen_test_vectors.py).
 *
 *   test_formats <repo root>      exit 77 (skip) if the retail data is absent
 */
#include "../engine/mjm.h"
#include "../engine/pc1.h"
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

	if (!checked) {
		printf("SKIP: no retail data under %s\n", root);
		return 77;
	}
	printf("%d files checked, %d failed\n", checked, failed);
	return failed != 0;
}
