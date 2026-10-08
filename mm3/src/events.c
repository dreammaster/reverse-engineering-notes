#include "events.h"

#include <string.h>

/* shape: 'p' pairs (count in npairs), 'g' pairs followed by a goto line, 'f' fixed number of plain bytes */
static const struct { const char *name; char shape; uint8_t n; } OPS[34] = {
	[0] = {"Nop", 'f', 0}, [1] = {"Display1", 'f', 1}, [2] = {"DoorTextSml", 'f', 1}, [3] = {"DoorTextLrg", 'f', 1}, [4] = {"SignText", 'f', 1},
	[5] = {"NPC", 'f', 5}, [6] = {"PlayFX", 'f', 1}, [7] = {"Teleport", 'f', 3},
	[8] = {"If>=", 'g', 1}, [9] = {"If==", 'g', 1}, [10] = {"If<=", 'g', 1},
	[11] = {"MoveObj", 'f', 3}, [12] = {"TakeOrGive", 'p', 2}, [14] = {"Remove", 'f', 0}, [15] = {"SetChar", 'f', 1},
	[16] = {"Spawn", 'f', 4}, [17] = {"DoTownEvent", 'f', 1}, [18] = {"Exit", 'f', 0}, [19] = {"AlterMap", 'f', 4},
	[20] = {"GiveMulti", 'p', 3}, [21] = {"ConfirmWord", 'f', 4}, [22] = {"Damage", 'f', 3}, [23] = {"JumpRnd", 'f', 3},
	[24] = {"AlterEvent", 'f', 2}, [25] = {"CallEvent", 'f', 3}, [26] = {"Return", 'f', 0}, [27] = {"SetVar", 'p', 1},
	[28] = {"TakeOrGive2", 'p', 2}, [29] = {"TakeOrGive3", 'p', 3}, [30] = {"CutsceneEnd", 'f', 0},
	[31] = {"Teleport2", 'f', 3}, [32] = {"WhoWill", 'f', 1}, [33] = {"TakeOrGive4", 'p', 2},
};

unsigned mm3_mode_width(unsigned mode) {
	return (mode == 16 || mode == 34) ? 4 : (mode == 25 || mode == 35) ? 2 : 1;
}

const char *mm3_op_name(unsigned opcode) {
	return (opcode < 34 && OPS[opcode].name) ? OPS[opcode].name : "?";
}

int mm3_event_decode(const Mm3Event *ev, Mm3EventOp *op) {
	unsigned pos = 0, n;
	memset(op, 0, sizeof(*op));
	op->opcode = ev->opcode;
	if (ev->opcode >= 34 || !OPS[ev->opcode].name)
		return -1;
	n = OPS[ev->opcode].n;
	if (OPS[ev->opcode].shape == 'f') {
		if (ev->nargs != n || n > sizeof(op->bytes))
			return -1;
		memcpy(op->bytes, ev->args, n);
		op->nbytes = (uint8_t)n;
		return 0;
	}
	for (unsigned i = 0; i < n; i++) {
		unsigned w;
		uint32_t v = 0;
		if (pos >= ev->nargs)
			return -1;
		op->pair[i].mode = ev->args[pos++];
		w = mm3_mode_width(op->pair[i].mode);
		if (pos + w > ev->nargs)
			return -1;
		for (unsigned k = 0; k < w; k++)
			v |= (uint32_t)ev->args[pos + k] << (8 * k);
		pos += w;
		op->pair[i].value = v;
	}
	op->npairs = (uint8_t)n;
	if (OPS[ev->opcode].shape == 'g') {
		if (pos >= ev->nargs)
			return -1;
		op->has_goto = 1;
		op->goto_line = ev->args[pos++];
	}
	return pos == ev->nargs ? 0 : -1;
}
