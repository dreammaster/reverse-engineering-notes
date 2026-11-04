/*
 * cadaver_unpack - decompress one or more Cadaver "mjm" files.
 *
 *   cadaver_unpack <in> <out>          one file
 *   cadaver_unpack -t <in>...          test only, print sizes
 */
#include "../engine/mjm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *slurp(const char *path, size_t *len) {
	FILE *f = fopen(path, "rb");
	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t *d = (uint8_t *)malloc(n ? n : 1);
	if (d && fread(d, 1, n, f) != (size_t)n) {
		free(d);
		d = NULL;
	}
	fclose(f);
	*len = (size_t)n;
	return d;
}

int main(int argc, char **argv) {
	if (argc >= 3 && strcmp(argv[1], "-t") == 0) {
		int bad = 0;
		for (int i = 2; i < argc; i++) {
			size_t n, un;
			uint8_t *in = slurp(argv[i], &n), *out = NULL;
			if (!in) { fprintf(stderr, "%s: cannot read\n", argv[i]); bad++; continue; }
			int rc = mjm_unpack(in, n, &out, &un);
			if (rc) { printf("%s: ERROR %d\n", argv[i], rc); bad++; }
			else printf("%s: %zu -> %zu\n", argv[i], n, un);
			free(in);
			free(out);
		}
		return bad != 0;
	}
	if (argc != 3) {
		fprintf(stderr, "usage: %s <in> <out> | -t <in>...\n", argv[0]);
		return 2;
	}
	size_t n, un;
	uint8_t *in = slurp(argv[1], &n), *out;
	if (!in) { perror(argv[1]); return 1; }
	int rc = mjm_unpack(in, n, &out, &un);
	if (rc) { fprintf(stderr, "%s: decode error %d\n", argv[1], rc); return 1; }
	FILE *f = fopen(argv[2], "wb");
	if (!f) { perror(argv[2]); return 1; }
	fwrite(out, 1, un, f);
	fclose(f);
	return 0;
}
