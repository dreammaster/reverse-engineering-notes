/* usage: test_state MM3.CUR -- checks the party/roster loaders against the documented new-game state and decodes
 * every shipped event record (the only expected mismatch is the stray-byte record documented for MAZE60). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cc.h"
#include "../events.h"
#include "../party.h"

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)

int main(int argc, char **argv) {
	Mm3Cc cc;
	size_t len;
	uint8_t *d;
	Mm3Party p;
	static Mm3Character roster[MM3_ROSTER_SIZE];
	unsigned total = 0, bad = 0;

	if (argc < 2 || mm3_cc_open(&cc, argv[1]) != 0) return 2;

	d = mm3_cc_read(&cc, "MAZE.PTY", &len);
	CHECK(d && mm3_party_load(&p, d, len) == 0);
	free(d);
	/* docs/data-files.md: six members 6,0,14,18,8,11; facing east (2) at (2,5) on map 1; day 1 year 500; 08:00 */
	CHECK(p.count == 6);
	CHECK(p.member[0] == 6 && p.member[1] == 0 && p.member[2] == 14 && p.member[3] == 18 && p.member[4] == 8 && p.member[5] == 11);
	CHECK(p.facing == 2 && p.x == 2 && p.y == 5 && p.map == 1);
	CHECK(p.year == 500 && p.minutes == 480 && p.food == 90 && p.gold == 3000 && p.gems == 30);
	printf("party: %d members, map %d (%d,%d) facing %d, day %d year %d, %u gold\n", p.count, p.map, p.x, p.y, p.facing, p.day, p.year, p.gold);

	d = mm3_cc_read(&cc, "MAZE.CHR", &len);
	CHECK(d && mm3_roster_load(roster, MM3_ROSTER_SIZE, d, len) == 30);
	free(d);
	for (int i = 0; i < p.count; i++) {
		const Mm3Character *c = &roster[p.member[i]];
		printf("  %-10s class %d level %d hp %d sp %d exp %u might %d\n", c->name, c->charClass, c->level, c->hp, c->sp, c->experience, c->stat[0].permanent);
	}
	CHECK(strcmp(roster[6].name, "Kastore") == 0);
	CHECK(strcmp(roster[0].name, "Sir Canegm") == 0);

	for (int n = 1; n < 100; n++) {
		char name[32];
		Mm3EventList l;
		snprintf(name, sizeof name, "MAZE%02d.EVT", n);
		if (!(d = mm3_cc_read(&cc, name, &len))) continue;
		mm3_events_load(&l, d, len);
		for (unsigned i = 0; i < l.count; i++) {
			Mm3EventOp op;
			total++;
			if (mm3_event_decode(&l.events[i], &op)) {
				bad++;
				printf("  %s (%d,%d) line %d %s: operand shape mismatch (%d bytes)\n", name, l.events[i].x, l.events[i].y,
					l.events[i].line, mm3_op_name(l.events[i].opcode), l.events[i].nargs);
			}
		}
		mm3_events_free(&l);
		free(d);
	}
	printf("events decoded: %u, shape mismatches: %u\n", total, bad);
	CHECK(bad <= 2);
	mm3_cc_close(&cc);
	puts(failures ? "FAILED" : "all checks passed");
	return failures != 0;
}
