#include "mm2_events.h"

/* DGROUP:15E6 */
static const uint8_t OP_LEN[EV_OPCODES] = {0, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 3, 3, 2, 2, 1, 2, 2, 13, 11, 1, 4, 3, 3, 5, 5, 3, 2, 2, 2,
										   2, 7, 7, 4, 3, 3, 3, 2, 1, 1, 3, 1, 15, 2, 2, 3, 3, 1, 11, 4, 2};

int mm2_event_op_len(int op) {
	if (op <= 0 || op >= EV_OPCODES) return 0;
	/* opcode 37 reads a word although the table says 2; it is never used in the shipped data */
	return op == EV_PAY_GEMS ? 3 : OP_LEN[op];
}

/* Returns the offset of the n-th script, or (size_t)-1. */
static size_t find_script(const uint8_t *s, size_t len, int n) {
	size_t p = 0;
	while (n > 0) {
		while (p < len && s[p] != 0xFF) {
			int l = mm2_event_op_len(s[p]);
			if (!l) return (size_t)-1;
			p += (size_t)l;
		}
		if (p >= len) return (size_t)-1;
		p++;
		n--;
	}
	return p < len ? p : (size_t)-1;
}

static void skip_commands(Mm2Vm *vm, int n) {
	while (n-- > 0 && vm->pc < vm->len && vm->code[vm->pc] != 0xFF) {
		int l = mm2_event_op_len(vm->code[vm->pc]);
		if (!l) {
			vm->pc = vm->len;
			return;
		}
		vm->pc += (size_t)l;
	}
}

/* TODO(review): control-flow ops follow ovl/2PLAY.asm evt_op15/16/17/23/26/27/28/43/44 (IDA 0x198C8..0x1A202).  Unverified:
 *  - opcode 43 tests byte_1DD59, which I labelled "night" earlier; elsewhere that byte is set to 0 when the party moves and
 *    tested after fights, so it may mean "a fight happened"; the VM exposes it as vm->night;
 *  - opcode 44 adds to word_1DC18 (not part of the saved state table; kept in vm->counter);
 *  - whether a host-handled op such as teleport really ends the script in the original (evt_op12_teleport, 0x194D4). */
int mm2_vm_run(Mm2Vm *vm, const uint8_t *scripts, size_t len, int script) {
	size_t start = find_script(scripts, len, script);
	if (start == (size_t)-1) return -1;
	vm->code = scripts;
	vm->len = len;
	vm->pc = start;
	vm->steps = 0;
	while (vm->pc < vm->len) {
		int op = vm->code[vm->pc], l;
		const uint8_t *args;
		if (op == 0xFF) break;
		l = mm2_event_op_len(op);
		if (!l || vm->pc + (size_t)l > vm->len) return -1;
		args = vm->code + vm->pc + 1;
		vm->pc += (size_t)l;
		vm->steps++;
		switch (op) {
		case EV_END:
		case EV_STOP:
			return vm->steps;
		case EV_SKIP_IF:
			if (vm->cond) skip_commands(vm, args[0]);
			break;
		case EV_SKIP_IF_NOT:
			if (!vm->cond) skip_commands(vm, args[0]);
			break;
		case EV_SKIP_IF_NIGHT:
			if (vm->night) skip_commands(vm, args[0]);
			break;
		case EV_VAR_COND: {
			unsigned dg = mm2_event_var_dgroup(args[0]);
			uint8_t *p = dg ? mm2_state_ptr(vm->state, dg) : 0;
			vm->cond = p ? *p : 0;
			break;
		}
		case EV_SET_VAR: {
			unsigned dg = mm2_event_var_dgroup(args[0]);
			uint8_t *p = dg ? mm2_state_ptr(vm->state, dg) : 0;
			if (p) *p = args[1];
			break;
		}
		case EV_COND_GT:
			if (args[0] > vm->cond) vm->cond = 0;
			break;
		case EV_COND_RAND:
			vm->cond = vm->host && vm->host->rand_range ? vm->host->rand_range(vm->host->ud, 1, args[0]) : 1;
			break;
		case EV_ADD_VALUE:
			vm->counter += args[0];
			break;
		default:
			if (vm->host && vm->host->exec) {
				if (!vm->host->exec(vm->host->ud, vm, op, args)) return vm->steps;
			}
			break;
		}
	}
	return vm->steps;
}

int mm2_event_find_trigger(const Mm2EventChunk *c, int x, int y, char facing) {
	int i, bit = facing == 'N' ? 0x80 : facing == 'E' ? 0x40 : facing == 'S' ? 0x20 : 0x10;
	unsigned cell = (unsigned)((y << 4) | x);
	for (i = 0; i < c->nTriggers; i++) {
		const uint8_t *t = c->triggers + i * 3;
		if (t[0] == cell && (t[2] & bit)) return t[1];
	}
	return -1;
}
