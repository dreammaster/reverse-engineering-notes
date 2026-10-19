/* Lockstep recorder (build with -DMM3_TAPE, run with MM3_TAPE=file): logs every call from translated code into a hand-written host
 * routine -- name, arguments, registers, and the memory the host and the translated code changed in between -- so that
 * tools/mm3_lockstep.py can run the *original* machine code on the same inputs and report where the two first disagree.
 *
 * File: "MM3TAPE1", u32 memsize, the initial memory image, then events:
 *   'T' top-level call of a translated routine   name, regs, delta     'U' its return: ax dx, delta
 *   'H' host call (before the host runs)         name, nargs, ret, regs, stack word count + words, delta
 *   'h' host return                              ax dx es sp, delta
 *   'F' entry of a translated routine            name, regs (no delta)
 * `delta` = the bytes of MEM that changed since the previous event: u32 count, then (u32 offset, u32 length, bytes) runs. */
#ifdef MM3_TAPE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"


#define TAPE_MEM (MEM_SIZE + 0x10000)
static FILE *tf;
static volatile int calltest_expired;          /* set by the watchdog; acted on at the next event boundary (tape_check_abort) */
static void tape_check_abort(void);
static uint8_t *prev;
static int host_depth;

static void w8(unsigned v) { fputc((int)v, tf); }
static void w16(unsigned v) { fputc((int)(v & 255), tf); fputc((int)((v >> 8) & 255), tf); }
static void w32(unsigned v) { w16(v & 0xFFFF); w16(v >> 16); }

static void delta(void) {
	/* the image below the program's data never changes: start at DGROUP */
	const size_t start = 0x286F0;
	size_t i = start, count = 0;
	long count_at = ftell(tf);
	w32(0);
	while (i < TAPE_MEM) {
		while (i + 8 <= TAPE_MEM && *(const uint64_t *)(MEM + i) == *(const uint64_t *)(prev + i)) i += 8;
		while (i < TAPE_MEM && MEM[i] == prev[i]) i++;
		if (i >= TAPE_MEM) break;
		size_t s = i, last = i;
		while (i < TAPE_MEM && i - last < 16) { if (MEM[i] != prev[i]) last = i; i++; }
		size_t len = last + 1 - s;
		w32((unsigned)s); w32((unsigned)len);
		fwrite(MEM + s, 1, len, tf);
		memcpy(prev + s, MEM + s, len);
		count++;
		i = last + 1;
	}
	long end = ftell(tf);
	fseek(tf, count_at, SEEK_SET); w32((unsigned)count); fseek(tf, end, SEEK_SET);
}

static void regs(const Cpu *c) {
	w16(c->ax); w16(c->bx); w16(c->cx); w16(c->dx); w16(c->si); w16(c->di); w16(c->bp); w16(c->es); w16(c->ds); w16(c->sp);
}
static void name(const char *n) { size_t l = strlen(n); w8((unsigned)l); fwrite(n, 1, l, tf); }

static void tape_open(void) {
	static int tried;
	if (tried) return;
	tried = 1;
	const char *path = getenv("MM3_TAPE");
	if (!path) return;
	tf = fopen(path, "wb");
	if (!tf) { perror(path); exit(1); }
	setvbuf(tf, NULL, _IOFBF, 1 << 20);
	fwrite("MM3TAPE1", 1, 8, tf);
	w32(TAPE_MEM);
	fwrite(MEM, 1, TAPE_MEM, tf);
	prev = malloc(TAPE_MEM);
	memcpy(prev, MEM, TAPE_MEM);
}

void tape_close(void) { if (tf) { fflush(tf); } }

void tape_host(Cpu *c, const char *n, int nargs, int ret, void (*fn)(Cpu *)) {
	tape_open();
	if (!tf) { fn(c); return; }
	tape_check_abort();
	w8('H'); name(n); w8((unsigned)nargs); w8((unsigned)ret); regs(c);
	int nw = nargs < 4 ? 4 : nargs > 80 ? 80 : nargs;
	w8((unsigned)nw);
	for (int i = 0; i < nw; i++) w16(rd16(STK, (uint16_t)(c->sp + 2 * i)));
	delta();
	host_depth++;
	fn(c);
	host_depth--;
	w8('h'); w16(c->ax); w16(c->dx); w16(c->es); w16(c->sp);
	delta();
	fflush(tf);
}

/* rnd() called by readable C: the original code would have called the rnd routine here */
void tape_rnd(int lo, int hi, int value) {
	if (!tf || !host_depth) return;
	Cpu c; memset(&c, 0, sizeof c);
	c.sp = 0; /* unknown: the call comes from readable C */
	w8('H'); name("rnd"); w8(2); w8(4); regs(&c);
	w8(4); w16((uint16_t)lo); w16((uint16_t)hi); w16(0); w16(0);
	delta();
	w8('h'); w16((uint16_t)value); w16(0); w16(0); w16(0);
	delta();
}

/* every label of a translated routine (build with -DMM3_TAPE_LABELS: `make mm3game_labels`): register state, to find the first instruction
 * where a routine goes wrong (tools/mm3_lockstep.py --labels NAME) */
void tape_label(Cpu *c, const char *n) {
	if (!tf) return;
	w8('L'); name(n); regs(c);
}

/* entry of a translated routine: the original has the same stack pointer there (the callers push placeholder return addresses) */
void tape_fn(Cpu *c, const char *n) {
	if (!tf) return;
	tape_check_abort();
	w8('F'); name(n); regs(c);
}

void tape_top_begin(Cpu *c, const char *n) {
	tape_open();
	if (!tf || host_depth) return;
	w8('T'); name(n); regs(c);
	delta();
}
void tape_top_end(Cpu *c) {
	if (!tf || host_depth) return;
	w8('U'); w16(c->ax); w16(c->dx);
	delta();
	fflush(tf);
}
#endif

/* ---- per-routine fuzzing for the lockstep tool: every translated routine is called with mutated game state and random arguments */
#ifdef MM3_TAPE
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

extern const RecompEntry recomp_entries_game_gen[];
static sigjmp_buf calltest_jmp;
static int calltest_active;
static void calltest_alarm(int sig) { (void)sig; calltest_expired = 1; }
static void tape_check_abort(void) { if (calltest_expired && calltest_active) { calltest_expired = 0; siglongjmp(calltest_jmp, 1); } }
static uint64_t ct_state;
static unsigned ct_rand(void) { ct_state ^= ct_state << 13; ct_state ^= ct_state >> 7; ct_state ^= ct_state << 17; return (unsigned)(ct_state >> 11); }

void tape_abort(void) { if (tf) { w8('X'); fflush(tf); } }

/* mutate some words of the data segment: mostly small values, in the areas that hold game state more often than elsewhere */
static void ct_mutate(void) {
	static const struct { unsigned lo, hi; } areas[] = {
		{ 0xB9D6, 0xB9D6 + 6 * 0x12F }, { 0xE8EA, 0xE8F9 }, { 0xEC30, 0xEC60 }, { 0xAD54, 0xAF00 }, { 0xECC0, 0xECE0 },
		{ 0xC4A2, 0xC4B0 }, { 0xB2C0, 0xB2E0 }, { 0xB6BC, 0xB6F0 }, { 0xC520, 0xC560 }, { 0x100, 0x200 }, { 0x2800, 0x2900 } };
	unsigned count = 4 + ct_rand() % 28;
	for (unsigned i = 0; i < count; i++) {
		unsigned a;
		if (ct_rand() % 10 < 7) { unsigned k = ct_rand() % (sizeof areas / sizeof *areas); a = areas[k].lo + ct_rand() % (areas[k].hi - areas[k].lo); }
		else a = ct_rand() % 0xF000;
		unsigned v = ct_rand() % 4 == 0 ? 0 : ct_rand() % 3 == 0 ? ct_rand() & 0xFFFF : ct_rand() % 40;
		if (a >= 0xB9D6 && a < 0xB9D6 + 6 * 0x12F) {   /* characters: perturb, and keep class and level plausible (level 0 / class 0 are not game states) */
			unsigned f = (a - 0xB9D6) % 0x12F;
			if (f == 0x13 || f == 0x23 || f == 0x24) continue;
			v = (DG[a] + (ct_rand() % 7) - 3) & 0xFF;
		}
		if (ct_rand() & 1) { DG[a] = (uint8_t)v; } else { DG[a] = (uint8_t)v; DG[a + 1] = (uint8_t)(v >> 8); }
	}
	memset(DG + 0xF200, 0, 0xDF0);       /* no stale stack contents: uninitialised locals read as zero in both worlds */
}

int game_calltest(const char *only, unsigned n, unsigned seed) {
	tape_open();
	if (!tf) { fprintf(stderr, "calltest: set MM3_TAPE=file\n"); return 2; }
	static uint8_t *snap;
	snap = malloc(TAPE_MEM);
	memcpy(snap, MEM, TAPE_MEM);
	signal(SIGALRM, calltest_alarm);
	for (const RecompEntry *e = recomp_entries_game_gen; e->name; e++) {
		if (only && strcmp(only, e->name)) continue;
		unsigned h = 0;
		for (const char *p = e->name; *p; p++) h = h * 31 + (unsigned char)*p;
		for (unsigned i = 0; i < n; i++) {
			memcpy(MEM, snap, TAPE_MEM);
			ct_state = ((uint64_t)seed << 32) ^ ((uint64_t)h << 8) ^ (i * 0x9E3779B97F4A7C15ull) ^ 0x5DEECE66Dull;
			if (!ct_state) ct_state = 1;
			ct_mutate();
			Cpu c;
			memset(&c, 0, sizeof c);
			c.sp = STACK_TOP; c.ds = DSEG;
			for (int k = 5; k >= 0; k--) {
				unsigned pick = ct_rand() % 8, v;
				v = pick < 4 ? ct_rand() % 13 : pick < 6 ? 0xB9D6 + (ct_rand() % 6) * 0x12F : ct_rand() & 0xFFFF;
				PUSH(&c, v);
			}
			PUSH(&c, 0); PUSH(&c, 0);
			calltest_expired = 0;
			if (sigsetjmp(calltest_jmp, 1)) { calltest_active = 0; tape_abort(); continue; }
			calltest_active = 1;
			alarm(2);
			tape_top_begin(&c, e->name);
			e->fn(&c);
			tape_top_end(&c);
			alarm(0);
			calltest_active = 0;
		}
	}
	return 0;
}
#endif
