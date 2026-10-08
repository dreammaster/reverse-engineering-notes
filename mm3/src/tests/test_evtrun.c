/* usage: test_evtrun [MM3.CUR] -- synthetic scripts for the control flow, then (with MM3.CUR) the real MAZE01 pit event. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cc.h"
#include "../evtrun.h"

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)

typedef struct {
	int yesno;      /* answer for the yes/no test (mode 44) */
	int has_item;   /* answer for mode 21 */
	unsigned rolls; /* value rnd() returns */
	char log[256];
} Mock;

static void logf_(Mock *m, const char *s) { strcat(m->log, s); }

static unsigned m_rnd(void *u, unsigned lo, unsigned hi) { (void)lo; (void)hi; return ((Mock *)u)->rolls; }
static int m_test(void *u, const Mm3Event *ev, const Mm3EventOp *op) {
	Mock *m = u; (void)ev;
	return op->pair[0].mode == 44 ? op->pair[0].value == (unsigned)m->yesno : op->pair[0].mode == 21 ? m->has_item : 0;
}
static int m_action(void *u, const Mm3Event *ev, const Mm3EventOp *op, int *next) {
	Mock *m = u; char b[32]; (void)next;
	switch (ev->opcode) {
	case MM3_OP_DISPLAY1: snprintf(b, sizeof b, "T%d ", op->bytes[0]); logf_(m, b); break;
	case MM3_OP_TELEPORT: snprintf(b, sizeof b, "TP%d,%d,%d ", op->bytes[0], op->bytes[1], op->bytes[2]); logf_(m, b); return MM3_EVT_RESTART;
	default: snprintf(b, sizeof b, "op%d ", ev->opcode); logf_(m, b);
	}
	return MM3_EVT_CONTINUE;
}

/* record builder: len x y facing line op operands... */
static size_t rec(uint8_t *p, int x, int y, int f, int line, int op, const uint8_t *args, int n) {
	p[0] = (uint8_t)(5 + n); p[1] = (uint8_t)x; p[2] = (uint8_t)y; p[3] = (uint8_t)f; p[4] = (uint8_t)line; p[5] = (uint8_t)op;
	for (int i = 0; i < n; i++) p[6 + i] = args[i];
	return 6 + (size_t)n;
}

int main(int argc, char **argv) {
	uint8_t buf[512];
	size_t len = 0;
	Mm3EventList l;
	Mock m;
	Mm3EventHost h = { &m, m_rnd, m_test, m_action };
	Mm3Position pos = { 3, 4, 1 };
	unsigned steps;

	/* script: 0 Display 1 ; 1 If(yesno==1) goto 4 ; 2 Display 2 ; 3 Exit ; 4 CallEvent (9,9) line 0 ; 5 Display 5 ; 6 AlterEvent line 7 -> Exit(18) ; 7 Display 7 ; 8 Exit
	 * sub-script at (9,9): 0 Display 9 ; 1 Return */
	{ uint8_t a[8];
	a[0] = 1; len += rec(buf + len, 3, 4, 4, 0, 1, a, 1);
	a[0] = 44; a[1] = 1; a[2] = 4; len += rec(buf + len, 3, 4, 4, 1, 9, a, 3);
	a[0] = 2; len += rec(buf + len, 3, 4, 4, 2, 1, a, 1);
	len += rec(buf + len, 3, 4, 4, 3, 18, a, 0);
	a[0] = 9; a[1] = 9; a[2] = 0; len += rec(buf + len, 3, 4, 4, 4, 25, a, 3);
	a[0] = 5; len += rec(buf + len, 3, 4, 4, 5, 1, a, 1);
	a[0] = 7; a[1] = 18; len += rec(buf + len, 3, 4, 4, 6, 24, a, 2);
	a[0] = 7; len += rec(buf + len, 3, 4, 4, 7, 1, a, 1);
	len += rec(buf + len, 3, 4, 4, 8, 18, a, 0);
	a[0] = 99; len += rec(buf + len, 9, 9, 4, 0, 1, a, 1);
	len += rec(buf + len, 9, 9, 4, 1, 26, a, 0); }
	CHECK(mm3_events_load(&l, buf, len) == 0 && l.count == 11);

	memset(&m, 0, sizeof m); m.yesno = 0;
	CHECK(mm3_events_run(&l, pos, &h, &steps) == MM3_EVT_STOP);
	CHECK(strcmp(m.log, "T1 T2 ") == 0);         /* test false -> falls through to line 2, Exit at 3 */

	memset(&m, 0, sizeof m); m.yesno = 1;
	CHECK(mm3_events_run(&l, pos, &h, &steps) == MM3_EVT_STOP);
	CHECK(strcmp(m.log, "T1 T99 T5 ") == 0);     /* goto 4, call (9,9), return, line 5, AlterEvent turns line 7 into Exit... */
	/* ...so line 7 (now Exit) ends the script before Display 7 */

	memset(&m, 0, sizeof m); m.yesno = 1;        /* the opcode was rewritten: second run behaves the same, line 7 stays Exit */
	CHECK(mm3_events_run(&l, pos, &h, &steps) == MM3_EVT_STOP && strstr(m.log, "T7") == NULL);

	pos.x = 0; pos.y = 0; pos.facing = 0;        /* (0,0) facing 0 never runs */
	memset(&m, 0, sizeof m);
	CHECK(mm3_events_run(&l, pos, &h, &steps) == MM3_EVT_STOP && steps == 0);
	mm3_events_free(&l);

	{ /* JumpRnd and loop guard */
		uint8_t a[4];
		len = 0;
		a[0] = 5; a[1] = 3; a[2] = 2; len += rec(buf + len, 1, 1, 4, 0, 23, a, 3);
		a[0] = 1; len += rec(buf + len, 1, 1, 4, 1, 1, a, 1);
		len += rec(buf + len, 1, 1, 4, 2, 18, a, 0);
		mm3_events_load(&l, buf, len);
		pos.x = 1; pos.y = 1; pos.facing = 2;
		memset(&m, 0, sizeof m); m.rolls = 3;
		mm3_events_run(&l, pos, &h, &steps);
		CHECK(strcmp(m.log, "") == 0);           /* roll matches: jump to Exit at line 2 */
		memset(&m, 0, sizeof m); m.rolls = 1;
		mm3_events_run(&l, pos, &h, &steps);
		CHECK(strcmp(m.log, "T1 ") == 0);
		mm3_events_free(&l);
		len = 0;
		a[0] = 1; a[1] = 1; a[2] = 0; len += rec(buf + len, 1, 1, 4, 0, 25, a, 3); /* CallEvent to itself, forever */
		mm3_events_load(&l, buf, len);
		CHECK(mm3_events_run(&l, pos, &h, &steps) == -1);
		mm3_events_free(&l);
	}

	if (argc > 1) { /* real data: the MAZE01 pit that needs a rope (docs/data-files.md) */
		Mm3Cc cc;
		size_t elen;
		uint8_t *d;
		CHECK(mm3_cc_open(&cc, argv[1]) == 0);
		d = mm3_cc_read(&cc, "MAZE01.EVT", &elen);
		CHECK(d != NULL);
		mm3_events_load(&l, d, elen);
		for (unsigned i = 0; i < l.count; i++)
			if (l.events[i].opcode == MM3_OP_TELEPORT && l.events[i].args[0] == 6 && l.events[i].args[1] == 1 && l.events[i].args[2] == 13) {
				pos.x = l.events[i].x; pos.y = l.events[i].y; pos.facing = l.events[i].facing == 4 ? 0 : l.events[i].facing;
				printf("pit at (%d,%d) facing %d\n", pos.x, pos.y, pos.facing);
				memset(&m, 0, sizeof m); m.has_item = 1; m.yesno = 0; /* the script tests value 0 for "yes" */
				CHECK(mm3_events_run(&l, pos, &h, &steps) == MM3_EVT_RESTART);
				printf("  with rope, answer yes : %s\n", m.log);
				memset(&m, 0, sizeof m); m.has_item = 0; m.yesno = 1;
				CHECK(mm3_events_run(&l, pos, &h, &steps) == MM3_EVT_STOP);
				printf("  without rope          : %s\n", m.log);
				break;
			}
		mm3_events_free(&l);
		free(d);
		mm3_cc_close(&cc);
	}
	puts(failures ? "FAILED" : "all tests passed");
	return failures != 0;
}
