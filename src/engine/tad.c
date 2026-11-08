#include "tad.h"
#include <stdlib.h>
#include <string.h>

#define DSG 0x1000
#define SSG 0x2100
#define TADG 0x3000
#define HKG 0x4000
#define QBASE 0xF000
#define SVC_BASE 0xE000          /* fake near addresses stored in the service table: SVC_BASE + byte index */
#define SENT 1

int tad_load(TadRt *t, const uint8_t *raw, size_t len) {
	memset(t, 0, sizeof(*t));
	if (len < 0x40 || len > 0xF000)
		return -1;
	t->mem = (uint8_t *)calloc(1, X86_MEM);
	if (!t->mem)
		return -1;
	memcpy(t->mem + TADG * 16, raw, len);
	t->len = len;
	unsigned p = (unsigned)(raw[0x2C + 6] | (raw[0x2C + 7] << 8));
	while (p + 2 <= len && t->ntimers < TAD_TIMERS) {
		unsigned w = (unsigned)(raw[p] | (raw[p + 1] << 8));
		if (w == 0 || w > 0x1000)
			break;
		t->timer_entry[t->ntimers++] = (uint16_t)w;
		p += 2;
	}
	/* far pointer ds:78E -> hook, service table at ss:[ds:B6] */
	uint8_t *ds = t->mem + DSG * 16;
	ds[0x78E] = SENT; ds[0x78F] = 0; ds[0x790] = HKG & 0xFF; ds[0x791] = HKG >> 8;
	ds[0xB6] = 0; ds[0xB7] = 1;
	for (int i = 0; i < 0x40; i++) {
		unsigned v = SVC_BASE + 2 * i;
		t->mem[SSG * 16 + 0x100 + 2 * i] = (uint8_t)v;
		t->mem[SSG * 16 + 0x101 + 2 * i] = (uint8_t)(v >> 8);
	}
	t->loaded = 1;
	return 0;
}

void tad_free(TadRt *t) {
	free(t->mem);
	memset(t, 0, sizeof(*t));
}

static void energy_change(AclVM *vm, int delta) {
	int e = vm->energy + delta;
	vm->energy = e <= 0 ? 0 : e > vm->energy_max ? vm->energy_max : e;
}

static void svc_hook(X86 *c, void *user) {
	TadRt *t = (TadRt *)user;
	unsigned addr = x86_rw(c, DSG, 0x792);
	int idx = (int)(addr - SVC_BASE);
	switch (idx) {
	case 0x06:                                   /* 81C8: change energy by ax (signed) */
		energy_change(t->vm, (int16_t)c->ax);
		break;
	case 0x20: {                                 /* 8840: random(ax .. dx) */
		int lo = (int16_t)c->ax, hi = (int16_t)c->dx;
		c->ax = (uint16_t)(t->vm->rand_range ? t->vm->rand_range(lo, hi, t->vm->user) : lo);
		break;
	}
	case 0x24:                                   /* 164E: start a sound */
		break;
	default:
		t->unsupported++;
		t->last_unsupported = (uint16_t)idx;
		break;
	}
	c->ip = (uint16_t)x86_pop(c);                /* retf */
	c->cs = (uint16_t)x86_pop(c);
}

int tad_run_timer(TadRt *t, AclVM *vm, int idx) {
	if (!t->loaded || idx < 0 || idx >= t->ntimers)
		return -1;
	uint8_t *ds = t->mem + DSG * 16;
	memcpy(ds + 0x590, vm->g, sizeof(vm->g));
	ds[0x5A] = QBASE & 0xFF; ds[0x5B] = QBASE >> 8;
	ds[0x84] = QBASE & 0xFF; ds[0x85] = QBASE >> 8;
	ds[0x226] = ds[0x227] = 0;
	ds[0x232] = (uint8_t)vm->room_no; ds[0x233] = 0;
	t->vm = vm;
	X86 *c = &t->cpu;
	x86_init(c, t->mem);
	c->ax = (uint16_t)idx;
	c->sp = 0xFF00 - 2;
	c->ds = DSG; c->es = DSG; c->ss = SSG; c->cs = TADG;
	x86_ww(c, SSG, 0xFF00 - 2, SENT);
	c->ip = t->timer_entry[idx];
	c->hook_cs = HKG; c->hook_ip = SENT; c->hook = svc_hook; c->hook_user = t;
	int n = x86_run(c, TADG, SENT, 200000);
	memcpy(vm->g, ds + 0x590, sizeof(vm->g));
	/* queued events: 6-byte entries (type, object/argument, argument) from QBASE to ds:84 */
	unsigned q = (unsigned)(ds[0x84] | (ds[0x85] << 8));
	for (unsigned a = QBASE; a + 6 <= q; a += 6) {
		unsigned type = (unsigned)(ds[a] | (ds[a + 1] << 8)) & 0x3FFF, arg = (unsigned)(ds[a + 2] | (ds[a + 3] << 8));
		if (type == 8 && vm->msg_n < 16)
			vm->msg[vm->msg_n++] = (int)arg;
	}
	return n < 0 ? -1 : 0;
}

void tad_timer_unit(TadRt *t, AclVM *vm) {
	if (!vm->vars[0x19])
		return;
	for (int i = 0; i < 0x26; i++) {
		if (!vm->vars[0x1A + i])
			continue;
		if (--vm->vars[0x1A + i] != 0)
			continue;
		if (t && t->loaded)
			tad_run_timer(t, vm, i);
		if (vm->vars[0x1A + i] == 0)
			vm->vars[0x19]--;                                 /* done: leave the list */
		else
			vm->vars[0x1A + i] = vm->g[0x81 + 0x1A + 0x34 + i];   /* the routine re-armed it: reload */
	}
}
