/* Lockstep recorder (build with -DMM3_TAPE, run with MM3_TAPE=file): logs every call from translated code into a hand-written host
 * routine -- name, arguments, registers, and the memory the host and the translated code changed in between -- so that
 * tools/mm3_lockstep.py can run the *original* machine code on the same inputs and report where the two first disagree.
 *
 * File: "MM3TAPE1", u32 memsize, the initial memory image, then events:
 *   'T' top-level call of a translated routine   name, regs, delta     'U' its return: ax dx, delta
 *   'H' host call (before the host runs)         name, nargs, ret, regs, 12 stack words, delta
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
	w8('H'); name(n); w8((unsigned)nargs); w8((unsigned)ret); regs(c);
	for (int i = 0; i < 12; i++) w16(rd16(STK, (uint16_t)(c->sp + 2 * i)));
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
	w16((uint16_t)lo); w16((uint16_t)hi); for (int i = 2; i < 12; i++) w16(0);
	delta();
	w8('h'); w16((uint16_t)value); w16(0); w16(0); w16(0);
	delta();
}

/* entry of a translated routine: the original has the same stack pointer there (the callers push placeholder return addresses) */
void tape_fn(Cpu *c, const char *n) {
	if (!tf) return;
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
