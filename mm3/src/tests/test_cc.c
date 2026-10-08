/* usage: test_cc DIR  -- DIR from gen_testdata.py.  Checks the C LZHUF/.CC reader against the expected contents,
 * then the maze/event/text parsers on small synthetic buffers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cc.h"
#include "../maze.h"

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)

static uint8_t *slurp(const char *path, size_t *len) {
	FILE *f = fopen(path, "rb");
	uint8_t *b;
	long n;
	if (!f) return NULL;
	fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
	b = malloc(n ? n : 1);
	if (fread(b, 1, n, f) != (size_t)n) { free(b); fclose(f); return NULL; }
	fclose(f);
	*len = n;
	return b;
}

static void check_member(const Mm3Cc *cc, const char *dir, const char *name) {
	char path[512];
	size_t elen, glen;
	uint8_t *exp, *got;
	snprintf(path, sizeof path, "%s/%s.expected", dir, name);
	exp = slurp(path, &elen);
	got = mm3_cc_read(cc, name, &glen);
	CHECK(exp && got);
	if (exp && got) {
		CHECK(elen == glen);
		CHECK(elen == glen && memcmp(exp, got, elen) == 0);
	}
	free(exp); free(got);
}

int main(int argc, char **argv) {
	Mm3Cc cc;
	char path[512];
	if (argc < 2) { fprintf(stderr, "usage: %s DIR\n", argv[0]); return 2; }
	snprintf(path, sizeof path, "%s/test.cc", argv[1]);
	CHECK(mm3_cc_open(&cc, path) == 0);
	CHECK(cc.count == 4);
	check_member(&cc, argv[1], "TEXT.MAZ");
	check_member(&cc, argv[1], "SKEW.DAT");
	check_member(&cc, argv[1], "NOISE.BIN");
	check_member(&cc, argv[1], "MAZE.NAM");
	CHECK(mm3_cc_find(&cc, "text.maz") >= 0); /* case-insensitive */
	CHECK(mm3_cc_find(&cc, "NOPE.BIN") < 0);
	mm3_cc_close(&cc);

	{ /* pages */
		uint8_t d[MM3_PAGE_SIZE] = {0};
		Mm3Page p;
		d[(3 * 16 + 5) * 2 + 1] = 0x31; d[(3 * 16 + 5) * 2] = 0x20; /* word 3120: N=3 E=1 S=2 W=0 */
		d[0x200 + 3 * 16 + 5] = 0x08;
		d[0x300 + MM3_HDR_NEIGHBOUR_EAST] = 7;
		CHECK(mm3_page_load(&p, d, sizeof d) == 0);
		CHECK(mm3_page_wall(&p, 5, 3, MM3_SIDE_N) == 3);
		CHECK(mm3_page_wall(&p, 5, 3, MM3_SIDE_E) == 1);
		CHECK(mm3_page_wall(&p, 5, 3, MM3_SIDE_S) == 2);
		CHECK(mm3_page_wall(&p, 5, 3, MM3_SIDE_W) == 0);
		CHECK(p.flags[3 * 16 + 5] == 8);
		CHECK(p.header[MM3_HDR_NEIGHBOUR_EAST] == 7);
		CHECK(mm3_page_load(&p, d, 10) != 0);
	}
	{ /* events: two lines on (2,4) facing any, one on (2,4) facing 1, padding byte */
		static const uint8_t evt[] = {
			7, 2, 4, 4, 2, 18, 0, 0,          /* line 2: Exit (n=7: x y f line op + 2 operand bytes) */
			6, 2, 4, 4, 1, 1, 9,              /* line 1: Display1 text 9 */
			6, 2, 4, 1, 1, 7, 3,              /* facing 1 only */
			0
		};
		Mm3EventList l;
		const Mm3Event *found[8];
		unsigned n;
		CHECK(mm3_events_load(&l, evt, sizeof evt) == 0);
		CHECK(l.count == 3);
		n = mm3_events_at(&l, 2, 4, 0, found, 8);
		CHECK(n == 2 && found[0]->line == 1 && found[0]->opcode == 1 && found[0]->args[0] == 9 && found[1]->opcode == 18);
		n = mm3_events_at(&l, 2, 4, 1, found, 8);
		CHECK(n == 3);
		CHECK(mm3_events_at(&l, 9, 9, 0, found, 8) == 0);
		mm3_events_free(&l);
	}
	{
		static const uint8_t txt[] = "first\0second\0third";
		CHECK(strcmp(mm3_text_string(txt, sizeof txt, 1), "second") == 0);
		CHECK(strcmp(mm3_text_string(txt, sizeof txt, 2), "third") == 0);
		CHECK(mm3_text_string(txt, sizeof txt, 5) == NULL);
	}
	puts(failures ? "FAILED" : "all tests passed");
	return failures != 0;
}
