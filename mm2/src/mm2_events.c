#include "mm2_events.h"
#include "mm2_party.h"
#include "mm2_evfields_gen.inc"

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

/* ---- character opcodes -------------------------------------------------------------------------------------- */

typedef struct {
	uint8_t *p;
	int width;
} FieldRef;

/* TODO(review): field ids come from the jump table of ovl/2PLAY.asm loc_1AA00 (IDA 0x1AA00..0x1B0B1, jpt_1AA56): id -> byte offset
 * of the character record, with +1/+2/+3 for the higher bytes of word/dword fields (def_1AA56 at 0x1AFF0) and widths 1, 2
 * (ids 20h 28h 35h 38h 3Ah 3Ch) or 4 (31h, 3Eh); an id whose offset was adjusted is a single byte.  Ids 0 and 1 (roster id
 * of the character, loc_1B0B2) are read-only here. */
static FieldRef field_ref(Mm2Roster *r, int slot0, int id) {
	static uint8_t dummy;
	FieldRef f;
	Mm2Char *c = &r->chars[mm2_party_member(r, slot0)];
	int off, width = 1;
	id &= 0x7F;
	if (id < 2) {
		dummy = (uint8_t)mm2_party_member(r, slot0);
		if (id == 1 && dummy >= 0x18) dummy = 0x80;
		f.p = &dummy;
		f.width = 1;
		return f;
	}
	off = FIELD_OFFSET[id];
	if (id == 0x31 || id == 0x3E) width = 4;
	if (id == 0x20 || id == 0x28 || id == 0x35 || id == 0x38 || id == 0x3A || id == 0x3C) width = 2;
	if (id == 0x21 || id == 0x29 || id == 0x32 || id == 0x35 || id == 0x39 || id == 0x3B || id == 0x3D || id == 0x3F || id == 0x78) { off += 1; width = 1; }
	if (id == 0x33 || id == 0x40) { off += 2; width = 1; }
	if (id == 0x34) { off += 3; width = 1; }
	f.p = c->raw + off;
	f.width = width;
	return f;
}

static uint32_t field_get(const FieldRef *f) {
	uint32_t v = 0;
	int i;
	for (i = 0; i < f->width; i++)
		v |= (uint32_t)f->p[i] << (8 * i);
	return v;
}

/* sub_19CB8: add (mode 0) or subtract (mode 1) an amount from a character field.  Returns the new cond. */
static void modify_field(Mm2Vm *vm, int slot1, int mode, int id, int nbytes, uint32_t amount) {
	FieldRef f = field_ref(vm->roster, slot1 - 1, id);
	uint32_t cur = field_get(&f), v = 0;
	if (mode == 0) {
		if (f.width == 1) { v = (amount & 0xFF) + cur; if (v >= 0x100) v = 0xFF; }
		else if (f.width == 2) { v = (amount & 0xFFFF) + cur; if (v >= 0x10000) v = 0; }
		else { v = amount + cur; if (v < amount) v = 0xFFFFFFFFu; }
	} else {
		uint32_t a = f.width == 1 ? (amount & 0xFF) : f.width == 2 ? (amount & 0xFFFF) : amount;
		if (a > cur) { v = 0; vm->cond = 0; }
		else v = cur - a;
	}
	if (nbytes == 3) nbytes++;
	if (vm->cond && nbytes > 0) {
		int i;
		for (i = 0; i < nbytes && i < 4; i++)
			f.p[i] = (uint8_t)(v >> (8 * i));
	}
}

static int rd24(const uint8_t *a) { return a[0] | (a[1] << 8) | (a[2] << 16); }

/* TODO(review): opcodes 31/32 (ovl/2PLAY.asm evt_op31_modify_char IDA 0x19E40..0x19F37 and sub_19CB8 0x19CB8): 31 adds, 32
 * subtracts (the original names say "modify one / all characters" but the only difference is the mode passed to sub_19CB8).
 * Overflow behaviour is copied: 8-bit saturates at 255, 16-bit wraps to 0 (as the code reads), 32-bit saturates; a failed
 * subtraction clears cond and stops later writes. */
static void op_modify(Mm2Vm *vm, int mode, const uint8_t *a) {
	int who = a[0], fromCond = 0, i, party = mm2_party_size(vm->roster), field = a[1], n = a[2];
	uint32_t amount = (uint32_t)rd24(a + 3);
	if (who >= 0x80) { who &= 0x7F; fromCond = 1; amount = (uint32_t)(vm->cond & 0xFF); }
	if (who == 9) {
		who = vm->sel;
		if (!who) who = vm->cond;
	}
	vm->cond = 1;
	if (party < who) return;
	if (who) {
		modify_field(vm, who, mode, field, n, amount);
	} else {
		for (i = 1; i <= party; i++)
			modify_field(vm, i, mode, field, n, amount);
	}
	(void)fromCond;
}

/* TODO(review): opcodes 21 / 24 (ovl/2PLAY.asm evt_op21_check_char IDA 0x19A02..0x19ABB, field reader sub_199B8): 21 ORs
 * (field & mask) of the addressed characters into cond, 24 writes (field & mask) | value back; only the low byte of the field
 * is used; who = 0 all, 1-8 one, 9 the selected one, +80h = value is the previous cond. */
static void op_check(Mm2Vm *vm, int modify, const uint8_t *a) {
	int who = a[0], field = a[1], mask = a[2], orv = modify ? a[3] : 0, count = 1, party = mm2_party_size(vm->roster);
	int prev = vm->cond;
	vm->prevCond = prev;
	vm->cond = 0;
	if (who >= 0x80) orv = prev;
	who &= 0x7F;
	if (who != 0 && who != 9 && party < who) who = 1;
	if (who == 0) count = who = party;
	if (party == 0) return;
	do {
		int idx = who--, slot = idx;
		unsigned v;
		FieldRef f;
		count--;
		if (idx == 9) {
			slot = vm->sel;
			if (!slot) { slot = prev; vm->sel = slot; }
		}
		if (slot < 1 || slot > party) continue;
		f = field_ref(vm->roster, slot - 1, field);
		v = f.p[0];
		if (modify) {
			f.p[0] = (uint8_t)((v & (unsigned)mask) | (unsigned)orv);
		} else {
			if (mask) v &= (unsigned)mask;
			vm->cond |= (int)v;
		}
	} while (count != 0);
}

static void op_has_item(Mm2Vm *vm, int item) {
	int i, k;
	vm->cond = 0;
	for (i = 0; i < mm2_party_size(vm->roster) && !vm->cond; i++) {
		const Mm2Char *c = &vm->roster->chars[mm2_party_member(vm->roster, i)];
		for (k = 0; k < 6; k++)
			if (c->raw[MC_PACK_ID + k] == item || c->raw[MC_EQUIP_ID + k] == item) vm->cond++;
	}
}

static void op_take_item(Mm2Vm *vm, int item) {
	int i, k;
	vm->cond = 0;
	for (i = 0; i < mm2_party_size(vm->roster) && !vm->cond; i++) {
		Mm2Char *c = &vm->roster->chars[mm2_party_member(vm->roster, i)];
		for (k = 0; k < 6 && !vm->cond; k++)
			if (c->raw[MC_PACK_ID + k] == item) {
				int j;
				vm->cond++;
				for (j = k; j < 5; j++) {
					c->raw[MC_PACK_ID + j] = c->raw[MC_PACK_ID + j + 1];
					c->raw[0x40 + j] = c->raw[0x40 + j + 1];
					c->raw[MC_PACK_FLAGS + j] = c->raw[MC_PACK_FLAGS + j + 1];
				}
				c->raw[MC_PACK_ID + 5] = c->raw[0x40 + 5] = c->raw[MC_PACK_FLAGS + 5] = 0;
			}
	}
}

/* TODO(review): opcode 25 (evt_op25_give_item IDA 0x19B44..0x19C19): operands who (only its high bit matters: item id from cond),
 * item, charges, bonus; first character with a free backpack slot; if nobody has room the item is put on the floor
 * (first free of the two treasure item slots, DGROUP:6950) - that floor case is NOT ported (cond stays 0). */
static void op_give_item(Mm2Vm *vm, const uint8_t *a) {
	int item = a[1], i, k;
	if (a[0] >= 0x80) item = vm->cond;
	vm->cond = 0;
	for (i = 0; i < mm2_party_size(vm->roster) && !vm->cond; i++) {
		Mm2Char *c = &vm->roster->chars[mm2_party_member(vm->roster, i)];
		for (k = 0; k < 6; k++)
			if (!c->raw[MC_PACK_ID + k]) {
				c->raw[MC_PACK_ID + k] = (uint8_t)item;
				c->raw[0x40 + k] = a[2];
				c->raw[MC_PACK_FLAGS + k] = a[3];
				vm->cond = 1;
				break;
			}
	}
}

/* party_pay_gold / party_pay_gems (mm2.asm 0x15190 / 0x15270): pooled over the non-hireling members; the remainder is put on the first
 * member and then shared evenly (party_share_gold 0x1618E, party_share_gems 0x16296). */
static int pool_pay(Mm2Roster *r, int offset, int bytes, uint32_t amount) {
	uint32_t total = 0, avg, rem;
	int i, n = mm2_party_size(r), chars = 0, first = -1;
	for (i = 0; i < n; i++) {
		Mm2Char *c = &r->chars[mm2_party_member(r, i)];
		if (mm2_party_member(r, i) >= MM2_FIRST_HIRELING) continue;
		chars++;
		if (first < 0) first = i;
		total += bytes == 4 ? mm2_c32(c, offset) : mm2_c16(c, offset);
	}
	if (total < amount || !chars) return 0;
	total -= amount;
	avg = total / (uint32_t)chars;
	rem = total - avg * (uint32_t)chars;
	for (i = 0; i < n; i++) {
		Mm2Char *c = &r->chars[mm2_party_member(r, i)];
		uint32_t v;
		int k;
		if (mm2_party_member(r, i) >= MM2_FIRST_HIRELING) continue;
		v = avg + (i == first ? rem : 0);
		for (k = 0; k < bytes; k++) c->raw[offset + k] = (uint8_t)(v >> (8 * k));
	}
	return 1;
}

static int party_skill_total(Mm2Roster *r, int skill) {
	int i, t = 0;
	for (i = 0; i < mm2_party_size(r); i++) {
		const Mm2Char *c = &r->chars[mm2_party_member(r, i)];
		if (c->raw[MC_CONDITION] < 0x81) t += mm2_char_skill_count(c, skill);
	}
	return t;
}

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
		case EV_CHAR_CHECK:
			if (vm->roster) { op_check(vm, 0, args); break; }
			goto host;
		case EV_CHAR_MODIFY:
			if (vm->roster) { op_check(vm, 1, args); break; }
			goto host;
		case EV_MODIFY_CHAR:
			if (vm->roster) { op_modify(vm, 0, args); break; }
			goto host;
		case EV_MODIFY_PARTY:
			if (vm->roster) { op_modify(vm, 1, args); break; }
			goto host;
		case EV_HAS_ITEM:
			if (vm->roster) { op_has_item(vm, args[1]); break; }
			goto host;
		case EV_REMOVE_ITEM:
			if (vm->roster) { op_take_item(vm, args[1]); break; }
			goto host;
		case EV_GIVE_ITEM:
			if (vm->roster) { op_give_item(vm, args); break; }
			goto host;
		case EV_PAY_GOLD:
			if (vm->roster) { vm->cond = pool_pay(vm->roster, MC_GOLD, 4, (uint32_t)(args[0] | (args[1] << 8))); break; }
			goto host;
		case EV_PAY_GEMS:
			if (vm->roster) { vm->cond = pool_pay(vm->roster, MC_GEMS, 2, (uint32_t)(args[0] | (args[1] << 8))); break; }
			goto host;
		case EV_PARTY_SKILL:
			if (vm->roster) { vm->cond = party_skill_total(vm->roster, args[0]); break; }
			goto host;
		case EV_SET_CELL:
			if (vm->map) {
				int cell = ((args[0] >> 4) << 4) | (args[0] & 15);
				vm->map[cell] = args[1];
				vm->map[256 + cell] = args[2];
				break;
			}
			goto host;
		case EV_TIME_CHECK: {   /* named "check hour" in the disassembly but it compares the ERA byte with the range */
			unsigned era = vm->state ? mm2_state_era(vm->state) & 0xFF : 0;
			vm->cond = era >= args[0] && era <= args[1];
			break;
		}
		case EV_DATE_CHECK: {
			unsigned era = vm->state ? mm2_state_era(vm->state) : 0;
			uint8_t *dp = vm->state ? mm2_state_ptr(vm->state, 0x3A2u + 2u * era) : 0;
			unsigned day = dp ? dp[0] : 0;
			if (args[0] == 0xB5) vm->cond = (day & 1) != 0;
			else if (args[0] == 0xB6) vm->cond = (day & 1) == 0;
			else vm->cond = day >= args[0] && day <= args[1];
			break;
		}
		case EV_CHOOSE_CHAR:
		case EV_CHOOSE_CHAR2:
			if (vm->roster && !vm->sel) {
				vm->sel = mm2_party_size(vm->roster) ? 1 : 0;   /* default: the first member (the host may ask the player) */
				vm->cond = vm->sel;
			}
			goto host;
		host:
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
