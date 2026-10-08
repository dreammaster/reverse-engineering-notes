/* gfxsum ARCHIVE -- one line per graphics member: id, frame count, FNV-1a of (w,h,pixels) of all frames.
 * tests/gfxsum.py prints the same from the Python decoder; `make gfxcheck` diffs them. */
#include <stdio.h>
#include <stdlib.h>

#include "gfx.h"

static uint32_t fnv(uint32_t h, const uint8_t *p, size_t n) {
	while (n--) { h ^= *p++; h *= 16777619u; }
	return h;
}

int main(int argc, char **argv) {
	Mm3Cc cc;
	if (argc < 2 || mm3_cc_open(&cc, argv[1])) return 2;
	for (unsigned i = 0; i < cc.count; i++) {
		size_t len;
		uint8_t *d = mm3_cc_read_index(&cc, (int)i, &len);
		Mm3Sprite s;
		if (!d) continue;
		if (len == MM3_RAW_SIZE) {
			printf("%04X raw %08x\n", cc.entries[i].id, fnv(2166136261u, d, len));
		} else if (mm3_sprite_decode(&s, d, len) == 0) {
			uint32_t h = 2166136261u;
			for (unsigned k = 0; k < s.count; k++) {
				uint8_t wh[4] = { s.frames[k].w & 255, s.frames[k].w >> 8, s.frames[k].h & 255, s.frames[k].h >> 8 };
				h = fnv(h, wh, 4);
				h = fnv(h, s.frames[k].pixels, (size_t)s.frames[k].w * s.frames[k].h);
			}
			printf("%04X sprite %u %08x\n", cc.entries[i].id, s.count, h);
			mm3_sprite_free(&s);
		}
		free(d);
	}
	mm3_cc_close(&cc);
	return 0;
}
