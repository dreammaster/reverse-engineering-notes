/* Decoding of event operands (docs/data-files.md "Event operand encodings"; mirrors tools/mm3_events.py). */
#ifndef MM3_EVENTS_H
#define MM3_EVENTS_H

#include <stdint.h>

#include "maze.h"

enum {
	MM3_OP_NOP = 0, MM3_OP_DISPLAY1 = 1, MM3_OP_DOORTEXT_SML = 2, MM3_OP_DOORTEXT_LRG = 3, MM3_OP_SIGNTEXT = 4, MM3_OP_NPC = 5,
	MM3_OP_PLAYFX = 6, MM3_OP_TELEPORT = 7, MM3_OP_IF_GE = 8, MM3_OP_IF_EQ = 9, MM3_OP_IF_LE = 10, MM3_OP_MOVEOBJ = 11,
	MM3_OP_TAKEORGIVE = 12, MM3_OP_REMOVE = 14, MM3_OP_SETCHAR = 15, MM3_OP_SPAWN = 16, MM3_OP_TOWNEVENT = 17,
	MM3_OP_EXIT = 18, MM3_OP_ALTERMAP = 19, MM3_OP_GIVEMULTI = 20, MM3_OP_CONFIRMWORD = 21, MM3_OP_DAMAGE = 22,
	MM3_OP_JUMPRND = 23, MM3_OP_ALTEREVENT = 24, MM3_OP_CALLEVENT = 25, MM3_OP_RETURN = 26, MM3_OP_SETVAR = 27,
	MM3_OP_TAKEORGIVE2 = 28, MM3_OP_TAKEORGIVE3 = 29, MM3_OP_CUTSCENE_END = 30, MM3_OP_TELEPORT2 = 31,
	MM3_OP_WHOWILL = 32, MM3_OP_TAKEORGIVE4 = 33
};

typedef struct {
	uint8_t mode;
	uint32_t value;
} Mm3ModeValue;

typedef struct {
	uint8_t opcode;
	uint8_t npairs;       /* (mode, value) pairs (If, TakeOrGive, GiveMulti, SetVar) */
	Mm3ModeValue pair[3];
	int has_goto;         /* If: line to jump to when the test is true */
	uint8_t goto_line;
	uint8_t nbytes;       /* plain operand bytes for the fixed-shape opcodes (Teleport: map, x, y ...) */
	uint8_t bytes[12];
} Mm3EventOp;

/* Value width in bytes for a mode: 4 for 16 (experience) and 34 (gold), 2 for 25 (minutes) and 35 (gems), else 1. */
unsigned mm3_mode_width(unsigned mode);

/* Decode the operands of an event record.  Returns 0, or -1 when the operand bytes do not fit the opcode's shape
 * (one shipped record in MAZE60 has a stray extra byte; padding after the pairs is tolerated by `lenient`). */
int mm3_event_decode(const Mm3Event *ev, Mm3EventOp *op);

/* Human-readable opcode name ("Exit", "Teleport", ...), "?" for unknown. */
const char *mm3_op_name(unsigned opcode);

#endif
