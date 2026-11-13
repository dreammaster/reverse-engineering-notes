#include "acl.h"
#include <stdlib.h>
#include <string.h>

static const int8_t OPLEN[94] = {
	2, 2, 0, 2, 2, 1, 1, 2, 2, 1, 1, 2, 2, 1, -1, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 2, 3, 2, 0, 0, 0,
	0, 0, 2, 2, 6, 4, 2, 2, 3, 6, 2, 3, 6, 2, 0, 2, -1, 2, 2, 1, 2, 2, 2, 2, 9, 2, -1, -1, 4, 1, 4, 4,
	5, 3, 3, 2, 2, 5, 1, 0, 2, 4, 2, 2, 1, 2, 2, 2, 1, 1, 1, 1, 8, 2, 2, 1, 1, 2, 3, 1, 2, 2};

int acl_oplen(int op) { return op >= 0 && op < 94 ? OPLEN[op] : -1; }

static int cond_len(int type) {
	switch (type) {
	case 1: case 12: case 13: case 15: case 17: case 19: case 20: case 24: return 1;
	case 4: case 9: case 10: case 18: case 26: return 2;
	default: return 0;
	}
}

void acl_init(AclVM *vm, const Mp2 *mp2) {
	memset(vm, 0, sizeof(*vm));
	vm->timer_delay = -1;
	vm->energy = vm->energy_max = 100;
	for (int t = 0; t < ACL_THINGS; t++) {
		int len;
		const uint8_t *r = mp2_thing(mp2, t, &len);
		if (r && len >= 16) {
			vm->present[t] = 1;
			vm->flags[t] = r[3];
			vm->room[t] = r[10];
			if (r[12] >= 16 && (size_t)r[12] + 6 <= (size_t)len) memcpy(vm->tail[t], r + r[12], 6);
			vm->pos[3 * t] = r[0]; vm->pos[3 * t + 1] = r[1]; vm->pos[3 * t + 2] = r[2];
			vm->flags2[t] = r[15];
		}
	}
}

static void test_res(AclVM *vm, int ok) { vm->c605 = ok ? ((vm->c605 + 1) & 255) : 0; }

static uint8_t *tflag(AclVM *vm, int id, int second) {
	if (id == 0xFFFF)
		id = vm->cur;
	if (id < 0 || id >= ACL_THINGS || !vm->present[id])
		return NULL;
	return second ? &vm->flags2[id] : &vm->flags[id];
}

static void do_op(AclVM *vm, int op, const uint8_t *a, int n) {
	int w = n >= 2 ? (a[0] << 8) | a[1] : 0;
	uint8_t *f = n >= 2 ? tflag(vm, w, 0) : NULL, *f2 = n >= 2 ? tflag(vm, w, 1) : NULL;
	uint8_t *cur = tflag(vm, vm->cur, 0);
	int tid = w == 0xFFFF ? vm->cur : w;
	switch (op) {
	case 1:
		if (f) {
			int had = *f & 0x80;
			*f &= 0x7F;
			if (had && vm->room[tid] != (vm->room_no & 255))
				*f |= 0x08;
		}
		break;
	case 26: if (f) *f |= 0x80; break;
	case 15: case 16: test_res(vm, f && (*f & 1)); break;
	case 17: if (f) *f |= 1; break;
	case 24: if (f) *f ^= 1; break;
	case 18: if (f) *f &= 0xFE; break;
	case 19: test_res(vm, f && (*f & 2)); break;
	case 20: if (f) *f |= 2; break;
	case 25: if (f) *f ^= 2; break;
	case 21: if (f) *f &= 0xFD; break;
	case 54: if (f2) *f2 |= 4; break;
	case 55: if (f2) *f2 &= 0xFB; break;
	case 68: if (f2) *f2 |= 0x40; break;
	case 67: if (f2) *f2 &= 0xBF; break;
	case 30: test_res(vm, cur && (*cur & 1)); break;
	case 31: if (cur) *cur &= 0xFE; break;
	case 32: if (cur) *cur |= 1; break;
	case 33: if (cur) *cur ^= 1; break;
	case 29: vm->disarm = 1; break;
	case 38: vm->vars[a[0] & 0x3F] = a[1]; break;
	case 39: vm->vars[a[0] & 0x3F] = (uint8_t)(vm->vars[a[0] & 0x3F] + a[1]); break;
	case 40: {
		int v = vm->vars[a[0] & 0x3F], mode = a[1], k = a[2];
		test_res(vm, mode == 0 ? v > k : mode == 1 ? v < k : mode == 2 ? v == k : v != k);
		break;
	}
	case 43: {
		int room = a[2] == 0xFE ? vm->room_no : a[2];
		test_res(vm, f && vm->room[tid] == room);
		break;
	}
	case 47: test_res(vm, f && (w == 0xFFFF ? vm->cur != 0 : w != 0)); break;
	case 79: vm->reg = vm->rand_range ? vm->rand_range(a[0], a[1], vm->user) : a[0]; break;
	case 80: test_res(vm, vm->reg == a[0]); break;
	case 81: vm->reg = vm->vars[a[0] & 0x3F]; break;
	case 9: if (vm->queue_n < 16) { vm->queue[vm->queue_n].type = 19; vm->queue[vm->queue_n++].arg = a[0]; } break;
	case 42: vm->timer_arg = a[0]; vm->timer_delay = a[1]; break;
	case 49: case 90: {
		int n = op == 49 ? 1 : 2;
		if (op == 90) {
			vm->g[0x6A9 - 0x590] = a[0];                    /* ds:6A9 = first operand (the TAD routines read it) */
			if (vm->msg_n < 16) vm->msg[vm->msg_n++] = 0x1E;  /* event 8, message 30 */
		}
		for (int k = 0; k < n; k++) {
			int idx = op == 49 ? a[0] : k, dly = op == 49 ? a[1] : a[1 + k];
			if (idx + 0x1A < 0x40) {
				if (vm->vars[0x1A + idx] == 0)
					vm->vars[0x19]++;
				vm->vars[0x1A + idx] = (uint8_t)dly;
				vm->g[0x81 + 0x1A + 0x34 + idx] = (uint8_t)dly;      /* reload copy at ds:62B+0x34+idx */
			}
		}
		break;
	}
	case 51:                                            /* change level: operand = current level index (0..3); next level number = operand + 2 */
		if (a[0] <= 9) vm->next_level = a[0] + 2;
		break;
	case 37:                                            /* change room: [room dx dy z]; landing = (dx + room rec[1], dy + room rec[3], z) (verified) */
		vm->go.pending = 1;
		vm->go.room = a[0]; vm->go.x = a[1]; vm->go.y = a[2]; vm->go.z = a[3];
		break;
	case 85: {                                          /* score += signed word (negative: subtract the raw word, clamp at 0) */
		unsigned wv = (unsigned)w & 0xFFFF;
		if (wv < 0x8000) vm->score += wv;
		else vm->score = vm->score > wv ? vm->score - wv : 0;
		break;
	}
	case 36:                                            /* spawn [id16] x y z room: the original copies the thing record and queues a spawn request; here the thing is placed */
		if (f && n >= 6 && tid >= 0 && tid < ACL_THINGS) {
			vm->pos[3 * tid] = a[2]; vm->pos[3 * tid + 1] = a[3]; vm->pos[3 * tid + 2] = a[4];
			vm->room[tid] = a[5] == 0xFE ? (uint8_t)vm->room_no : a[5];
		}
		break;
	case 41:                                            /* move a thing: [id16] room x y z (0xFE = current room); another room: flag 0x08 */
		if (f && n >= 6 && tid >= 0 && tid < ACL_THINGS) {
			int room = a[2] == 0xFE ? vm->room_no : a[2];
			vm->pos[3 * tid] = a[3]; vm->pos[3 * tid + 1] = a[4]; vm->pos[3 * tid + 2] = a[5];
			if (room != vm->room_no) *f |= 8;
			vm->room[tid] = (uint8_t)room;
		}
		break;
	case 5: vm->score += a[0]; break;
	case 6: vm->money += a[0]; vm->score += a[0] >> 2; break;       /* verified: ds:248 += a, ds:24C += a >> 2 */
	case 2: if (cur) *cur |= 0x80; break;                           /* the current object is removed (its effect on the engine's entries is not modelled) */
	case 45: {
		int e = vm->energy + (int16_t)w;
		vm->energy = e <= 0 ? 0 : e > vm->energy_max ? vm->energy_max : e;
		break;
	}
	case 28: if (vm->msg_n < 16) vm->msg[vm->msg_n++] = w; break;
	case 72: break;               /* describe */
	default: vm->skipped++; break;
	}
}

/* Execute (execute != 0) or just skip an op sequence up to the terminator `end`; returns the position after it. */
static size_t exec_ops(AclVM *vm, const uint8_t *b, size_t len, size_t p, int end, int execute, int *bad) {
	while (p < len) {
		int op = b[p++];
		if (op == end)
			return p;
		if (op >= 94 && bad)
			*bad = 1;
		int n = op < 94 ? OPLEN[op] : 0;
		if (n < 0) {                                  /* IF-like block */
			int extra = (op == 58 || op == 59) ? 1 : 0;
			uint8_t hdr = extra && p < len ? b[p] : 0;
			p += extra + 1;
			int cond = 0;
			if (execute) {
				cond = op == 14 ? vm->c605 != 0 : op == 48 ? vm->c605 == 0 : op == 58 ? vm->c605 == hdr : vm->c605 != hdr;
				vm->c605 = 0;
			}
			p = exec_ops(vm, b, len, p, 0x16, execute && cond, bad);
			if (p < len && b[p] == 0x0F) {
				p++;
				p++;
				p = exec_ops(vm, b, len, p, 0x16, execute && !cond, bad);
			}
			continue;
		}
		if (op == 62 || op == 63 || op == 64) {
			if (p < len && b[p] >= 0x80)
				n--;
		}
		if (execute && p + (size_t)n <= len) {
			if (op == 62 || op == 63 || op == 64)
				vm->skipped++;
			else
				do_op(vm, op, b + p, n);
		}
		p += (size_t)n;
	}
	return p;
}

static int cond_match(int type, const uint8_t *cond, int arg) {
	switch (type) {
	case 2: return 0;
	case 4: return ((cond[0] << 8) | cond[1]) == (arg & 0xFFFF);
	case 9: case 18: case 26: {
		int w = (cond[0] << 8) | cond[1];
		return w >= 0x8000 || w == (arg & 0xFFFF);
	}
	case 10: return cond[1] == (arg & 0xFF);
	case 12: case 13: case 15: case 17: case 19: case 20: case 24: return cond[0] == (arg & 0xFF);
	default: return 1;
	}
}

void acl_room_event(AclVM *vm, const Mp2 *mp2, int room, int type, int arg) {
	if (room < 0 || room >= mp2->sec[0].count || !mp2->sec[0].rec[room])
		return;
	const uint8_t *r = mp2->sec[0].rec[room];
	int len = mp2->sec[0].len[room];
	if (len <= 0x20 || r[0x1F] == 0 || room >= 128)
		return;
	struct { int type, arg; } pend[64];
	int head = 0, tail = 0;
	pend[tail].type = type; pend[tail++].arg = arg;
	while (head < tail) {
		int t = pend[head].type, a = pend[head].arg;
		head++;
		size_t p = 0x20;
		for (int i = 0; i < r[0x1F] && i < 64 && p + 2 <= (size_t)len; i++) {
			int ln = r[p], ty = r[p + 1] & 0x7F, keep = r[p + 1] & 0x80, cl = cond_len(ty);
			if (ln < 2)
				break;
			if (ty == t && !(vm->armed[room][i / 8] & (1 << (i % 8))) && p + 2 + (size_t)cl <= (size_t)len &&
			    cond_match(ty, r + p + 2, a)) {
				vm->disarm = 0;
				vm->c605 = 0;
				vm->queue_n = 0;
				exec_ops(vm, r, (size_t)len, p + 2 + (size_t)cl, 0x17, 1, NULL);
				for (int q = 0; q < vm->queue_n && tail < 64; q++) {
					pend[tail].type = vm->queue[q].type;
					pend[tail++].arg = vm->queue[q].arg;
				}
				if (!keep || vm->disarm)
					vm->armed[room][i / 8] |= (uint8_t)(1 << (i % 8));
			}
			p += (size_t)ln;
		}
	}
}

void acl_thing_event(AclVM *vm, const Mp2 *mp2, int thing, int type, int arg) {
	int len;
	const uint8_t *r = mp2_thing(mp2, thing, &len);
	if (!r || len <= 16 || r[11] == 0 || thing < 0 || thing >= ACL_THINGS)
		return;
	vm->cur = thing;
	size_t p = 16;
	for (int i = 0; i < r[11] && i < 64 && p + 2 <= (size_t)len; i++) {
		int ln = r[p], ty = r[p + 1] & 0x7F, keep = r[p + 1] & 0x80, cl = cond_len(ty);
		uint8_t *arm = &vm->armed[128 + thing][i / 8];
		if (ln < 2)
			break;
		if (ty == type && !(*arm & (1 << (i % 8))) && p + 2 + (size_t)cl <= (size_t)len && cond_match(ty, r + p + 2, arg)) {
			vm->disarm = 0;
			vm->c605 = 0;
			vm->queue_n = 0;
			exec_ops(vm, r, (size_t)len, p + 2 + (size_t)cl, 0x17, 1, NULL);
			if (!keep || vm->disarm)
				*arm |= (uint8_t)(1 << (i % 8));
		}
		p += (size_t)ln;
	}
}

void acl_fire_room(AclVM *vm, const Mp2 *mp2, int room, int event) { acl_room_event(vm, mp2, room, event, 0); }

static int list_parse_ok(const uint8_t *r, size_t len, size_t p, int count) {
	if (len <= p)
		return 0;
	AclVM *dummy = (AclVM *)calloc(1, sizeof(AclVM));
	int ok = 1;
	for (int i = 0; i < count && ok; i++) {
		if (p + 2 > len) { ok = 0; break; }
		size_t ln = r[p];
		int bad = 0;
		size_t q = exec_ops(dummy, r, len, p + 2 + (size_t)cond_len(r[p + 1] & 0x7F), 0x17, 0, &bad);
		if (bad || ln == 0 || (q != p + ln && !(q + 1 == p + ln && ln % 2 == 0)))
			ok = 0;
		p += ln;
	}
	free(dummy);
	return ok;
}

int acl_room_parse_ok(const Mp2 *mp2, int room) {
	if (room < 0 || room >= mp2->sec[0].count || !mp2->sec[0].rec[room])
		return 0;
	return list_parse_ok(mp2->sec[0].rec[room], (size_t)mp2->sec[0].len[room], 0x20, mp2->sec[0].rec[room][0x1F]);
}

int acl_thing_parse_ok(const Mp2 *mp2, int thing) {
	int len;
	const uint8_t *r = mp2_thing(mp2, thing, &len);
	return r && len > 16 && list_parse_ok(r, (size_t)len, 16, r[11]);
}

int acl_chest_toggle(AclVM *vm, const Mp2 *mp2, int thing, int *content) {
	(void)mp2;
	*content = 0;
	if (thing < 0 || thing >= ACL_THINGS || !vm->present[thing])
		return 0;
	uint8_t *tl = vm->tail[thing];
	if (!(tl[4] & 1)) {                                  /* closed: trap, key, content */
		if (tl[5]) {
			int e = vm->energy - tl[5];
			vm->energy = e <= 0 ? 0 : e;
			if (vm->msg_n < 16) vm->msg[vm->msg_n++] = 0x32;
			return 0;
		}
		int key = (tl[2] << 8) | tl[3];
		if (key) {
			if (key >= ACL_THINGS || vm->room[key] != 0xFF)
				return 0;                                /* the key is not carried */
			tl[2] = tl[3] = 0;
			if (tl[4] & 2) vm->flags[key] |= 0x80;       /* the key is used up */
		}
		if (tl[4] & 4) vm->flags[thing] |= 0x80;         /* the chest disappears */
		int c = (tl[0] << 8) | tl[1];
		if (c) { *content = c; tl[0] = tl[1] = 0; }
	}
	tl[4] ^= 1;
	return 1;
}
