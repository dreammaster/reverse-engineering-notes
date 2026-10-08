/* usage: test_real MM3.CUR -- parse every MAZEnn.DAT / .EVT in the archive and print per-map statistics.
 * Checks that the event records tile each file exactly (no trailing garbage) apart from padding. */
#include <stdio.h>
#include <stdlib.h>

#include "../cc.h"
#include "../maze.h"

int main(int argc, char **argv) {
	Mm3Cc cc;
	int bad = 0;
	if (argc < 2 || mm3_cc_open(&cc, argv[1]) != 0) return 2;
	for (int n = 1; n < 100; n++) {
		char name[32];
		size_t len;
		uint8_t *d;
		snprintf(name, sizeof name, "MAZE%02d.DAT", n);
		if ((d = mm3_cc_read(&cc, name, &len))) {
			Mm3Page p;
			unsigned walls = 0;
			if (mm3_page_load(&p, d, len)) { printf("%s: bad size %zu\n", name, len); bad++; }
			else {
				for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) for (int s = 0; s < 4; s++) walls += (mm3_page_wall(&p, x, y, s) & 7) != 0;
				printf("%s: %zu bytes, %u wall sides, neighbours S%d E%d N%d W%d, can_rest %d\n", name, len, walls,
					p.header[MM3_HDR_NEIGHBOUR_SOUTH], p.header[MM3_HDR_NEIGHBOUR_EAST], p.header[MM3_HDR_NEIGHBOUR_NORTH],
					p.header[MM3_HDR_NEIGHBOUR_WEST], p.header[MM3_HDR_CAN_REST]);
			}
			free(d);
		}
		snprintf(name, sizeof name, "MAZE%02d.EVT", n);
		if ((d = mm3_cc_read(&cc, name, &len))) {
			Mm3EventList l;
			size_t used = 0;
			mm3_events_load(&l, d, len);
			for (unsigned i = 0; i < l.count; i++) used += 6 + l.events[i].nargs;
			/* each record = 1 length byte + 5 header bytes + operands */
			printf("%s: %zu bytes, %u events (%zu bytes of records)\n", name, len, l.count, used);
			if (used > len || len - used > 4) { printf("  !! records do not tile the file\n"); bad++; }
			mm3_events_free(&l);
			free(d);
		}
	}
	mm3_cc_close(&cc);
	return bad != 0;
}
