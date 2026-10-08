#include "evtrun.h"

static Mm3Event *find_line(Mm3EventList *list, int x, int y, int facing, int line) {
	/* original: skip the (0,0) square facing 0 */
	if ((x | y | facing) == 0)
		return NULL;
	for (unsigned i = 0; i < list->count; i++) {
		Mm3Event *e = &list->events[i];
		if (e->x == x && e->y == y && (e->facing == facing || e->facing == 4) && e->line == line)
			return e;
	}
	return NULL;
}

int mm3_events_run(Mm3EventList *list, Mm3Position party, const Mm3EventHost *host, unsigned *steps) {
	int cur_x = party.x, cur_y = party.y, line = 0;
	struct { int x, y, line; } stack[MM3_EVT_MAX_CALL_DEPTH];
	int depth = -1;
	unsigned n = 0;

	for (;;) {
		Mm3Event *ev = find_line(list, cur_x, cur_y, party.facing, line);
		Mm3EventOp op;
		int next = -1, rc = MM3_EVT_CONTINUE;
		if (!ev)
			break; /* no record for this line: the script ends */
		if (++n > MM3_EVT_STEP_LIMIT) {
			if (steps) *steps = n;
			return -1;
		}
		mm3_event_decode(ev, &op); /* a malformed record (MAZE60) keeps whatever decoded */
		switch (ev->opcode) {
		case MM3_OP_NOP:
			break;
		case MM3_OP_EXIT:
			rc = MM3_EVT_STOP;
			break;
		case MM3_OP_IF_GE: case MM3_OP_IF_EQ: case MM3_OP_IF_LE:
			if (host->test && host->test(host->user, ev, &op))
				next = op.goto_line;
			break;
		case MM3_OP_JUMPRND:
			if (host->rnd(host->user, 1, op.bytes[0]) == op.bytes[1])
				next = op.bytes[2];
			break;
		case MM3_OP_CALLEVENT:
			if (depth + 1 >= MM3_EVT_MAX_CALL_DEPTH) {
				if (steps) *steps = n;
				return -1;
			}
			depth++;
			stack[depth].x = cur_x; stack[depth].y = cur_y; stack[depth].line = line;
			cur_x = op.bytes[0]; cur_y = op.bytes[1];
			next = op.bytes[2];
			break;
		case MM3_OP_RETURN:
			if (depth >= 0) { /* the original does not check; an empty stack would read garbage */
				cur_x = stack[depth].x; cur_y = stack[depth].y; line = stack[depth].line;
				depth--;
			}
			break;
		case MM3_OP_ALTEREVENT: {
			/* rewrites the opcode of line bytes[0] of the party's own square (not of the CallEvent target) */
			Mm3Event *t = find_line(list, party.x, party.y, party.facing, op.bytes[0]);
			if (t)
				t->opcode = op.bytes[1];
			break;
		}
		default:
			if (host->action)
				rc = host->action(host->user, ev, &op, &next);
			break;
		}
		if (rc == MM3_EVT_STOP || rc == MM3_EVT_RESTART) {
			if (steps) *steps = n;
			return rc;
		}
		line = (next >= 0 ? next : line + 1);
	}
	if (steps) *steps = n;
	return MM3_EVT_STOP;
}
