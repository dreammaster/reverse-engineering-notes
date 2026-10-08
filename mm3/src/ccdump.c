/* ccdump ARCHIVE [OUTDIR] -- list the members of a .CC archive, or extract them as <ID>.bin (as mm3_cc.py does
 * without an EXE).  Also the cross-check tool against the Python reference: see `make crosscheck`. */
#include <stdio.h>
#include <stdlib.h>

#include "cc.h"

int main(int argc, char **argv) {
	Mm3Cc cc;
	if (argc < 2 || mm3_cc_open(&cc, argv[1]) != 0) {
		fprintf(stderr, "usage: %s ARCHIVE [OUTDIR]\n", argv[0]);
		return 2;
	}
	for (unsigned i = 0; i < cc.count; i++) {
		size_t len = 0;
		uint8_t *d = mm3_cc_read_index(&cc, (int)i, &len);
		if (!d) {
			fprintf(stderr, "member %04X unreadable\n", cc.entries[i].id);
			return 1;
		}
		if (argc > 2) {
			char path[1024];
			FILE *f;
			snprintf(path, sizeof path, "%s/%04X.bin", argv[2], cc.entries[i].id);
			f = fopen(path, "wb");
			if (!f || fwrite(d, 1, len, f) != len) {
				fprintf(stderr, "cannot write %s\n", path);
				return 1;
			}
			fclose(f);
		} else {
			printf("%04X off=%7u size=%5u -> %5zu\n", cc.entries[i].id, (unsigned)cc.entries[i].offset, cc.entries[i].size, len);
		}
		free(d);
	}
	mm3_cc_close(&cc);
	return 0;
}
