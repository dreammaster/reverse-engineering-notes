#include "tad.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

#define DSG 0x1000
#define SSG 0x2100
#define TADG 0x3000
#define HKG 0x4000
#define QBASE 0xF000
#define SVC_BASE 0xE000          /* fake near addresses stored in the service table: SVC_BASE + byte index */
#define SENT 1
#define ACT_REC 0x3200
#define PROJ_REC 0x3400

int tad_load(TadRt *t, const uint8_t *raw, size_t len) {
	memset(t, 0, sizeof(*t));
	if (len < 0x40 || len > 0xF000 || raw[0x20] != 0xBB || raw[0x23] != 0xB8)
		return -1;           /* init stub: mov bx,<thunk>; mov ax,<data table> (Cadaver 1: 27h/2Ch; the Payoff's new levels: 2Eh/80h) */
	unsigned base = (unsigned)(raw[0x24] | (raw[0x25] << 8));
	if (base + 0x10 > len)
		return -1;
	t->mem = (uint8_t *)calloc(1, X86_MEM);
	if (!t->mem)
		return -1;
	memcpy(t->mem + TADG * 16, raw, len);
	t->len = len;
	unsigned p = (unsigned)(raw[base + 6] | (raw[base + 7] << 8));
	while (p + 2 <= len && t->ntimers < TAD_TIMERS) {
		unsigned w = (unsigned)(raw[p] | (raw[p + 1] << 8));
		if (w == 0 || w > 0x1000)
			break;
		t->timer_entry[t->ntimers++] = (uint16_t)w;
		p += 2;
	}
	t->base = base;
	for (int k = 9; k < 25 && t->nbeh < 16; k++) {
		unsigned off = base + 4u * (unsigned)k;
		if (off + 2 > len) break;
		unsigned w = (unsigned)(raw[off] | (raw[off + 1] << 8));
		if (w == 0xFFFF || w == 0 || w >= 0x1000) { t->beh_entry[t->nbeh++] = 0; continue; }
		t->beh_entry[t->nbeh++] = (uint16_t)(w + 2);
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
	t->cur_thing = t->proj_thing = -1;
	t->loaded = 1;
	return 0;
}

void tad_free(TadRt *t) {
	free(t->mem);
	memset(t, 0, sizeof(*t));
}

static void energy_change(AclVM *vm, int delta) {
	acl_energy_change(vm, delta);
}

static void build_rec(TadRt *t, AclVM *vm, int thing, unsigned addr, const AnimState *an);
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
	case 0x04: {                                 /* 407F: dx = thing id -> di = its record in the image (FFFF: the current thing); ZF set when there is none */
		unsigned id = c->dx & 0xFFFF;
		int th = id == 0xFFFF ? t->cur_thing : (int)id;
		if (th >= 0 && th < ACL_THINGS && t->vm && t->vm->present[th]) {
			if (th == t->cur_thing) c->di = ACT_REC;
			else { build_rec(t, t->vm, th, PROJ_REC, NULL); t->proj_thing = th; c->di = PROJ_REC; }
			c->fl = (uint16_t)(c->fl & ~64);
		} else c->fl = (uint16_t)(c->fl | 64);
		break;
	}
	case 0x22: {                                 /* 6A44 (first part): set the creature's velocity B[3], B[4] from the direction code cl (table ds:FE1F: pairs of +-127 / 0 per direction bits 1 up, 2 down, 4 left, 8 right) */
		static const uint8_t dirv[32] = {0x00, 0x00, 0x00, 0x81, 0x00, 0x7f, 0x00, 0x00, 0x81, 0x00, 0x81, 0x81, 0x81, 0x7f, 0x00, 0x00,
		                                 0x7f, 0x00, 0x7f, 0x81, 0x7f, 0x7f, 0x00, 0x0b, 0x01, 0x8f, 0x00, 0xca, 0x00, 0xba, 0x00, 0x14};
		uint8_t *dsm = t->mem + DSG * 16;
		int cl = c->cx & 0xFF;
		if (cl < 11 && dsm[(c->bx + 4) & 0xFFFF] == 0) {
			unsigned boff = (c->di + dsm[(c->di + 0xD) & 0xFFFF]) & 0xFFFF;
			dsm[(boff + 3) & 0xFFFF] = dirv[2 * cl];
			dsm[(boff + 4) & 0xFFFF] = dirv[2 * cl + 1];
			if (t->vm && t->cur_thing >= 0 && t->vm->nspawn < 4) {      /* 6A9B..: queue the shot (thing ds:57E) at the shooter */
				int id = dsm[0x57E] | (dsm[0x57F] << 8);
				if (id < ACL_THINGS) { t->vm->spawn[t->vm->nspawn].id = id; t->vm->spawn[t->vm->nspawn].from = t->cur_thing; t->vm->spawn[t->vm->nspawn].vx = (int8_t)dirv[2 * cl]; t->vm->spawn[t->vm->nspawn].vy = (int8_t)dirv[2 * cl + 1]; t->vm->spawn[t->vm->nspawn++].dir = cl; }
			}
		}
		break;
	}
	case 0x24:                                   /* 164E: start a sound (al = id) */
		if (t->vm && t->vm->sound) t->vm->sound(c->ax & 0xFF, t->vm->user);
		break;
	default:
		t->unsupported++;
		t->last_unsupported = (uint16_t)idx;
		if (getenv("CAD_OPS")) fprintf(stderr, "SKIPSVC %02x ret %04x dx %04x\n", idx, x86_rw(c, c->ss, c->sp), c->dx);
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
	t->cur_thing = -1; t->proj_thing = -1;
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

#define PLAYER_REC 0x3000
#define CREAT_REC 0x3100

int tad_run_behaviour(TadRt *t, AclVM *vm, int b, int cx, int cy, int px, int py, int *vx, int *vy) {
	if (!t->loaded || b < 0 || b >= t->nbeh || !t->beh_entry[b])
		return -1;
	uint8_t *ds = t->mem + DSG * 16;
	memcpy(ds + 0x590, vm->g, sizeof(vm->g));
	ds[0x5C] = PLAYER_REC & 0xFF; ds[0x5D] = PLAYER_REC >> 8;                  /* ds:5C = the player's record */
	memset(ds + PLAYER_REC, 0, 0x40);
	ds[PLAYER_REC] = (uint8_t)px; ds[PLAYER_REC + 1] = (uint8_t)py;
	memset(ds + CREAT_REC, 0, 0x40);
	ds[CREAT_REC] = (uint8_t)cx; ds[CREAT_REC + 1] = (uint8_t)cy;
	ds[CREAT_REC + 0xD] = 0x10;                                               /* movement state at +0x10 */
	ds[CREAT_REC + 0xE] = 0x20;                                               /* tail at +0x20 */
	ds[0x226] = ds[0x227] = 0;
	ds[0x84] = QBASE & 0xFF; ds[0x85] = QBASE >> 8;
	t->vm = vm;
	t->cur_thing = -1; t->proj_thing = -1;
	X86 *c = &t->cpu;
	x86_init(c, t->mem);
	c->ax = (uint16_t)(b + 9);
	c->di = CREAT_REC; c->bx = CREAT_REC + 0x20; c->bp = CREAT_REC + 0x20; c->si = CREAT_REC + 0x10;
	c->sp = 0xFF00 - 2;
	c->ds = DSG; c->es = DSG; c->ss = SSG; c->cs = TADG;
	x86_ww(c, SSG, 0xFF00 - 2, SENT);
	c->ip = t->beh_entry[b];
	c->hook_cs = HKG; c->hook_ip = SENT; c->hook = svc_hook; c->hook_user = t;
	uint8_t before[0x40];
	memcpy(before, ds + CREAT_REC, 0x40);
	int n = x86_run(c, TADG, SENT, 200000);
	if (n < 0)
		return -1;
	if (getenv("CAD_TADDIFF")) for (int i = 0; i < 0x40; i++) if (before[i] != ds[CREAT_REC + i]) fprintf(stderr, "BEH%d rec[%02x] %02x -> %02x\n", b, i, before[i], ds[CREAT_REC + i]);
	*vx = (int8_t)ds[CREAT_REC + 0x10 + 3];
	*vy = (int8_t)ds[CREAT_REC + 0x10 + 4];
	return 0;
}

/* the thing's record laid out in the data image at `addr` (header 16, A at +20h, B at +30h (+ its table at +3Eh), C at +80h) */
static void build_rec(TadRt *t, AclVM *vm, int thing, unsigned addr, const AnimState *an) {
	uint8_t *rec = t->mem + DSG * 16 + addr;
	memset(rec, 0, 0xA0);
	rec[0] = vm->pos[3 * thing]; rec[1] = vm->pos[3 * thing + 1]; rec[2] = vm->pos[3 * thing + 2];
	rec[3] = vm->flags[thing]; rec[10] = vm->room[thing]; rec[15] = vm->flags2[thing];
	rec[4] = (uint8_t)(thing >> 8); rec[5] = (uint8_t)thing;
	rec[12] = 0x20; rec[13] = 0x30; rec[14] = 0x80;
	memcpy(rec + 0x20, vm->tail[thing], 16);
	memcpy(rec + 0x30, vm->mstate[thing], 10);
	memcpy(rec + 0x3E, vm->mtab[thing], 48);
	if (an) { rec[0x80] = (uint8_t)an->delay; rec[0x81] = (uint8_t)an->frame; rec[0x82] = (uint8_t)an->dir; rec[0x83] = (uint8_t)an->pos; rec[0x88] = (uint8_t)an->cnt; rec[0x89] = (uint8_t)an->cad; }
}
static void commit_rec(TadRt *t, AclVM *vm, int thing, unsigned addr, AnimState *an) {
	const uint8_t *rec = t->mem + DSG * 16 + addr;
	vm->pos[3 * thing] = rec[0]; vm->pos[3 * thing + 1] = rec[1]; vm->pos[3 * thing + 2] = rec[2];
	vm->flags[thing] = rec[3]; vm->flags2[thing] = rec[15];
	memcpy(vm->tail[thing], rec + 0x20, 16);
	memcpy(vm->mstate[thing], rec + 0x30, 10);
	if (an) { an->delay = rec[0x80]; an->frame = rec[0x81]; an->dir = rec[0x82]; an->pos = rec[0x83]; an->cnt = rec[0x88]; an->cad = rec[0x89]; }
}
int tad_run_action(TadRt *t, AclVM *vm, int thing, int arg, AnimState *an) {
	if (!t->loaded || thing < 0 || thing >= ACL_THINGS || !vm->present[thing] || arg < 0 || arg > 15)
		return -1;
	int b = vm->tail[thing][1];
	unsigned eoff = t->base + 4u * (unsigned)(b + 9) + 2;
	if (eoff + 2 > t->len) return -1;
	uint8_t *blob = t->mem + TADG * 16;
	unsigned sub = (unsigned)(blob[eoff] | (blob[eoff + 1] << 8));
	if (sub == 0 || sub + 2u * (unsigned)(arg + 1) > t->len) return -1;
	unsigned routine = (unsigned)(blob[sub + 2 * arg] | (blob[sub + 2 * arg + 1] << 8));
	if (routine == 0 || routine >= t->len) return -1;
	uint8_t *ds = t->mem + DSG * 16;
	memcpy(ds + 0x590, vm->g, sizeof(vm->g));
	ds[0x5C] = PLAYER_REC & 0xFF; ds[0x5D] = PLAYER_REC >> 8;
	memset(ds + PLAYER_REC, 0, 0x40);
	ds[PLAYER_REC] = (uint8_t)vm->px; ds[PLAYER_REC + 1] = (uint8_t)vm->py; ds[PLAYER_REC + 2] = (uint8_t)vm->pz;
	uint8_t *rec = ds + ACT_REC;
	build_rec(t, vm, thing, ACT_REC, an);
	t->cur_thing = thing; t->proj_thing = -1;
	uint8_t before[0xA0];
	memcpy(before, rec, 0xA0);
	ds[0x226] = ds[0x227] = 0;
	ds[0x84] = QBASE & 0xFF; ds[0x85] = QBASE >> 8;
	ds[0x232] = (uint8_t)vm->room_no; ds[0x233] = 0;
	t->vm = vm;
	X86 *c = &t->cpu;
	x86_init(c, t->mem);
	c->ax = (uint16_t)arg; c->dx = (uint16_t)thing;
	c->di = ACT_REC; c->bp = ACT_REC + 0x20; c->bx = ACT_REC + 0x30; c->si = 0; c->cx = (uint16_t)routine;
	c->sp = 0xFF00 - 2;
	c->ds = DSG; c->es = DSG; c->ss = SSG; c->cs = TADG;
	x86_ww(c, SSG, 0xFF00 - 2, SENT);
	c->ip = (uint16_t)routine;
	c->hook_cs = HKG; c->hook_ip = SENT; c->hook = svc_hook; c->hook_user = t;
	int n = x86_run(c, TADG, SENT, 200000);
	if (n < 0) return -1;
	if (getenv("CAD_TADDIFF")) for (int i = 0; i < 0xA0; i++) if (before[i] != rec[i]) fprintf(stderr, "ACT b%d a%d thing %d rec[%02x] %02x -> %02x\n", b, arg, thing, i, before[i], rec[i]);
	memcpy(vm->g, ds + 0x590, sizeof(vm->g));
	commit_rec(t, vm, thing, ACT_REC, an);
	if (t->proj_thing >= 0) commit_rec(t, vm, t->proj_thing, PROJ_REC, NULL);
	t->cur_thing = -1; t->proj_thing = -1;
	return 0;
}
