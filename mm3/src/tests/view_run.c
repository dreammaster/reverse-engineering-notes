/* view_run SNAPSHOT.dg -- run the translated view pipeline on a DGROUP snapshot and print the draw lists it produces
 * as "L" (list start), "S off seg" (sprite set) and "R x y flags frame" lines (see tools/mm3_viewcheck.py). */
#include <stdio.h>

#include "../view_host.h"

static void dump_list(void *user, unsigned addr) {
	(void)user;
	printf("L\n");
	for (int n = 0; n < 4000; n++) {
		unsigned w = rd16(DG, (uint16_t)addr);
		if (w == 0xFFFF) {
			unsigned off = rd16(DG, (uint16_t)(addr + 2)), seg = rd16(DG, (uint16_t)(addr + 4));
			addr += 6;
			if (seg == 0) break;
			printf("S %u %u\n", off, seg);
		} else {
			printf("R %d %d %u %u\n", (int16_t)rd16(DG, (uint16_t)addr), (int16_t)rd16(DG, (uint16_t)(addr + 2)),
				rd16(DG, (uint16_t)(addr + 4)), rd16(DG, (uint16_t)(addr + 6)));
			addr += 8;
		}
	}
}

int main(int argc, char **argv) {
	FILE *f;
	if (argc < 2) return 2;
	f = fopen(argv[1], "rb");
	if (!f || fread(DG, 1, 65536, f) != 65536) return 2;
	fclose(f);
	mm3_view_set_list_callback(dump_list, NULL);
	if (DG[0x15B]) mm3_view_run_outdoor(); else mm3_view_run(); /* Maze_wrapMode: outdoor map */
	return 0;
}
