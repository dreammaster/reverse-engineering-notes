/* usage: test_walk DATADIR -- walk across the page boundaries of the outdoor map 41 and check the party's map / position / slot
 * bookkeeping (mazeUpdateSlot, translated) and that every frame renders. */
#include <stdio.h>
#include <stdlib.h>

#include "../recomp.h"
#include "../view_glue.h"

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)

int main(int argc, char **argv) {
	char path[512];
	Mm3Cc cc, cur;
	Mm3Dgroup dg;
	Mm3View *v;
	static uint8_t screen[MM3_RAW_SIZE];
	if (argc < 2) return 2;
	snprintf(path, sizeof path, "%s/MM3.CC", argv[1]);
	if (mm3_cc_open(&cc, path)) return 2;
	snprintf(path, sizeof path, "%s/MM3.CUR", argv[1]);
	if (mm3_cc_open(&cur, path)) return 2;
	snprintf(path, sizeof path, "%s/DGROUP.BIN", argv[1]);
	if (mm3_dgroup_load(&dg, path)) return 2;
	v = mm3_view_create(&cc, &cur, &dg);
	CHECK(v && mm3_view_set_map(v, 41, 14, 5, 2) == 0);
	CHECK(mm3_view_outdoor(v) && mm3_view_map(v) == 41);
	/* header of map 41: east neighbour 45 (0x2D) */
	for (int step = 0; step < 4; step++) {
		int x = mm3_view_x(v);
		mm3_view_set_party(v, x + 1, mm3_view_y(v), 2);
		mm3_view_after_move(v);
		mm3_view_render(v, screen);
		printf("step %d: map %d x=%d y=%d\n", step + 1, mm3_view_map(v), (int8_t)mm3_view_x(v), (int8_t)mm3_view_y(v));
	}
	CHECK(mm3_view_map(v) == 45);
	CHECK(mm3_view_x(v) < 16);
	/* and back west into 41 */
	for (int step = 0; step < 6; step++) {
		mm3_view_set_party(v, mm3_view_x(v) - 1, mm3_view_y(v), 3);
		mm3_view_after_move(v);
		mm3_view_render(v, screen);
	}
	printf("back: map %d x=%d\n", mm3_view_map(v), (int8_t)mm3_view_x(v));
	CHECK(mm3_view_map(v) == 41);
	mm3_view_destroy(v);
	puts(failures ? "FAILED" : "all tests passed");
	return failures != 0;
}
