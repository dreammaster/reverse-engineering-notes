/* Event script runner: the control flow of runMazeEvent (19608).  Game effects (messages, teleports, item and stat
 * changes, the `If` tests) are delegated to a host so they can be implemented separately. */
#ifndef MM3_EVTRUN_H
#define MM3_EVTRUN_H

#include "events.h"

/* Result of a host action (and of a whole run). */
enum {
	MM3_EVT_CONTINUE = 0, /* go on with the next line */
	MM3_EVT_STOP = 1,     /* end the script (Exit, or the host aborts: combat starting, party dead ...) */
	MM3_EVT_RESTART = 2   /* the party moved to another square or map: the caller reloads and runs again from line 0 */
};

typedef struct {
	void *user;
	/* Uniform random number in [lo, hi]. */
	unsigned (*rnd)(void *user, unsigned lo, unsigned hi);
	/* Opcodes 8-10 (If>=, If==, If<=): evaluate op->pair[0] (mode, value) against the party; return non-zero when true. */
	int (*test)(void *user, const Mm3Event *ev, const Mm3EventOp *op);
	/* Every other opcode with side effects (text, teleport, give/take, damage, town buildings ...).
	 * Return MM3_EVT_CONTINUE / STOP / RESTART.  ConfirmWord and similar opcodes that branch return CONTINUE after
	 * setting *next_line to the line to run next (left at -1 for "the line after this one"). */
	int (*action)(void *user, const Mm3Event *ev, const Mm3EventOp *op, int *next_line);
} Mm3EventHost;

typedef struct {
	uint8_t x, y, facing;
} Mm3Position;

#define MM3_EVT_MAX_CALL_DEPTH 5
#define MM3_EVT_STEP_LIMIT 100000

/* Run the script of the party's square.  Returns MM3_EVT_STOP or MM3_EVT_RESTART, or -1 when the script loops
 * for more than MM3_EVT_STEP_LIMIT records or nests CallEvent too deep.  `steps` (optional) receives the number of
 * records executed.  AlterEvent rewrites opcodes in `list`. */
int mm3_events_run(Mm3EventList *list, Mm3Position party, const Mm3EventHost *host, unsigned *steps);

#endif
