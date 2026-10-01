/* Event script interpreter (docs/events.md).  Control flow, the condition register, event variables
 * and a few pure operations are implemented here; everything that touches the user interface or the
 * party is delegated to the host callback. */
#ifndef MM2_EVENTS_H
#define MM2_EVENTS_H

#include "mm2_files.h"
#include "mm2_state.h"

enum {
	EV_MSG = 1, EV_MSG_ROW13, EV_MSG_FRAME, EV_TITLE, EV_MSG_BOX, EV_MSG_FRAMED, EV_WAIT_KEY, EV_WAIT_KEY_MUSIC,
	EV_ASK_YN, EV_ASK_YN2, EV_SHOW_MONSTER, EV_TELEPORT, EV_SOUND, EV_LOCATION, EV_END, EV_SKIP_IF, EV_SKIP_IF_NOT,
	EV_FIGHT, EV_FIGHT2, EV_CLEAR_TRIGGER, EV_CHAR_CHECK, EV_HAS_ITEM, EV_VAR_COND, EV_CHAR_MODIFY, EV_GIVE_ITEM,
	EV_SET_VAR, EV_COND_GT, EV_COND_RAND, EV_WAIT, EV_WAIT_KEY_ABORT, EV_MODIFY_CHAR, EV_MODIFY_PARTY, EV_SET_CELL,
	EV_TIME_CHECK, EV_DATE_CHECK, EV_PAY_GOLD, EV_PAY_GEMS, EV_CHOOSE_CHAR, EV_CHOOSE_CHAR2, EV_REMOVE_ITEM, EV_STOP,
	EV_PLACE_TREASURE, EV_SKIP_IF_NIGHT, EV_ADD_VALUE, EV_CHECK_CLASS, EV_TEACH, EV_READ_STRING, EV_COMPARE_STRING,
	EV_AWARD_EXP, EV_PARTY_SKILL, EV_OPCODES
};

/* Total command size in bytes (opcode included); 0 for opcode 0 / unknown. */
int mm2_event_op_len(int op);

typedef struct Mm2Vm Mm2Vm;

typedef struct {
	/* Called for every opcode the VM does not handle itself.  args = the operand bytes (len - 1).
	 * May set vm->cond.  Return 0 to stop the script. */
	int (*exec)(void *ud, Mm2Vm *vm, int op, const uint8_t *args);
	/* Random number in [lo, hi] (the game's rand_range). */
	int (*rand_range)(void *ud, int lo, int hi);
	void *ud;
} Mm2EventHost;

struct Mm2Vm {
	Mm2State *state;
	const Mm2EventHost *host;
	int cond;               /* byte_1DC7F: result of the last check */
	unsigned counter;       /* word_1DC18, increased by opcode 44 */
	int night;              /* byte_1DD59: tested by opcode 43 */
	const uint8_t *code;    /* script area */
	size_t len, pc;
	int steps;
};

/* Runs script number `script` (the n-th FFh-terminated block).  Returns the number of commands executed,
 * or -1 if the script does not exist / is malformed. */
int mm2_vm_run(Mm2Vm *vm, const uint8_t *scripts, size_t len, int script);

/* Looks up the script for a trigger at (x, y) facing `facing` ('N','E','S','W'); returns the script number
 * or -1 (docs/events.md: facing bits W = 10h, S = 20h, E = 40h, N = 80h). */
int mm2_event_find_trigger(const Mm2EventChunk *c, int x, int y, char facing);

#endif
