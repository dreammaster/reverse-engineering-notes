#include "exeimg.h"
#include "x86.h"

#include <stdlib.h>
#include <string.h>

static void exe_int(void *user, X86 *c, unsigned n) {
	(void)user;
	if (n == 0x21) {
		unsigned ah = c->ax >> 8;
		if (ah == 0x30) c->ax = 0x0005;                 /* DOS version */
		else if (ah == 0x4C) { c->err = 1; c->err_op = 0x4C; }   /* the program ended without unpacking anything */
		c->fl &= (uint16_t)~1u;
	}
}

static int has_marker(const uint8_t *mem, const char *m, size_t ml, size_t from) {
	for (size_t i = from; i + ml <= EXEIMG_MEM; i++)
		if (mem[i] == (uint8_t)m[0] && !memcmp(mem + i, m, ml)) return 1;
	return 0;
}

int exeimg_unpack(const uint8_t *exe, size_t len, const char *marker, uint8_t **out) {
	if (len < 0x1C || exe[0] != 'M' || exe[1] != 'Z') return -1;
	unsigned hdr = (unsigned)(exe[8] | (exe[9] << 8)) * 16u, nrel = (unsigned)(exe[6] | (exe[7] << 8)), relo = (unsigned)(exe[0x18] | (exe[0x19] << 8));
	const unsigned psp = 0x1000, base = psp + 0x10;
	if (hdr >= len || (size_t)base * 16 + (len - hdr) + 0x1000 >= EXEIMG_MEM || relo + 4u * nrel > len) return -1;
	uint8_t *mem = (uint8_t *)calloc(1, EXEIMG_MEM);
	if (!mem) return -1;
	memcpy(mem + base * 16, exe + hdr, len - hdr);
	for (unsigned i = 0; i < nrel; i++) {
		unsigned off = (unsigned)(exe[relo + 4 * i] | (exe[relo + 4 * i + 1] << 8)), seg = (unsigned)(exe[relo + 4 * i + 2] | (exe[relo + 4 * i + 3] << 8));
		size_t a = ((size_t)(base + seg) * 16 + off) & 0xFFFFF;
		unsigned v = (unsigned)(mem[a] | (mem[a + 1] << 8)) + base;
		mem[a] = (uint8_t)v; mem[a + 1] = (uint8_t)(v >> 8);
	}
	mem[psp * 16] = 0xCD; mem[psp * 16 + 1] = 0x20; mem[psp * 16 + 3] = 0xA0;      /* PSP: INT 20h, top of memory A000h */
	X86 c;
	x86_init(&c, mem);
	c.cs = (uint16_t)(base + (exe[0x16] | (exe[0x17] << 8))); c.ip = (uint16_t)(exe[0x14] | (exe[0x15] << 8));
	c.ss = (uint16_t)(base + (exe[0x0E] | (exe[0x0F] << 8))); c.sp = (uint16_t)(exe[0x10] | (exe[0x11] << 8));
	c.ds = c.es = (uint16_t)psp;
	c.int_cb = exe_int;
	size_t ml = strlen(marker);
	for (int i = 0; i < 400; i++) {
		x86_run(&c, 0xFFFF, 0xFFFF, 50000);                /* the stop address never matches: runs the chunk */
		if (has_marker(mem, marker, ml, (size_t)base * 16)) {
			if (!c.err) x86_run(&c, 0xFFFF, 0xFFFF, 50000);   /* a little more: the stub may still be finishing the block that holds the marker */
			*out = mem;
			return 0;
		}
		if (c.err) break;
	}
	free(mem);
	return -1;
}
