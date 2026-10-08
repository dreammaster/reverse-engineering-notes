/* test_x86_sound DRIVER.DRV SONG.M TICKS: run a sound driver in the interpreter (init, start a song, N timer ticks) and print every
 * OPL register write as "R reg value" and every PIT write; tests/x86_sound_ref.py does the same under Unicorn for comparison. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../x86.h"

static uint8_t status;
static unsigned last_reg;
static void out8(void *u, uint16_t port, uint8_t v) {
	(void)u;
	if (port == 0x388) last_reg = v;
	else if (port == 0x389) {
		printf("R %02X %02X\n", last_reg, v);
		if (last_reg == 4) { if (v & 0x80) status = 0; else if (v & 1) status = 0xC0; }
	} else printf("P %04X %02X\n", port, v);
}
static uint8_t in8(void *u, uint16_t port) { (void)u; return port == 0x388 ? status : 0; }

static uint8_t *load(const char *path, size_t *n) {
	FILE *f = fopen(path, "rb");
	if (!f) exit(2);
	uint8_t *b = malloc(65536);
	*n = fread(b, 1, 65536, f);
	fclose(f);
	return b;
}

int main(int argc, char **argv) {
	static uint8_t mem[0x110000];
	X86 c;
	size_t n;
	if (argc < 4) return 2;
	memset(&c, 0, sizeof c);
	c.mem = mem; c.out8 = out8; c.in8 = in8;
	uint8_t *drv = load(argv[1], &n); memcpy(mem + 0x20000, drv, n);
	uint8_t *song = load(argv[2], &n); memcpy(mem + 0x40000, song, n);
	mem[0x20] = 0x20; mem[0x21] = 0xFF; mem[0x22] = 0x00; mem[0x23] = 0xF0; mem[0xFFF20] = 0xCF; /* old INT 08h vector: an iret stub */
	c.ss = 0x3000; c.sp = 0xFFF0; c.ds = 0x1000; c.es = 0x1000;
	uint16_t a0[1] = { 0x388 };
	printf("# init\n");
	if (x86_call_far(&c, 0x2000, 0, a0, 1, 5000000)) return 1;
	printf("# music\n");
	uint16_t a6[2] = { 0, 0x4000 };
	if (x86_call_far(&c, 0x2000, 6, a6, 2, 5000000)) return 1;
	uint16_t off = mem[0x20] | (mem[0x21] << 8), seg = mem[0x22] | (mem[0x23] << 8);
	printf("# ivt08 %04X:%04X\n", seg, off);
	unsigned ticks = (unsigned)atoi(argv[3]);
	for (int phase = 0; phase <= argc - 4; phase++) {
		if (phase > 0) { /* effect ids after the tick count: play each through API 9 and run 100 ticks */
			uint16_t a9[1] = { (uint16_t)atoi(argv[3 + phase]) };
			printf("# fx %u\n", a9[0]);
			if (x86_call_far(&c, 0x2000, 9, a9, 1, 5000000)) return 1;
			ticks = 100;
		}
	for (unsigned t = 0; t < ticks; t++) {
		X86 save = c;
		c.sp -= 2; mem[0x30000 + c.sp] = (uint8_t)c.flags; mem[0x30000 + c.sp + 1] = (uint8_t)(c.flags >> 8);
		c.sp -= 2; mem[0x30000 + c.sp] = 0x00; mem[0x30000 + c.sp + 1] = 0xF0; /* cs */
		c.sp -= 2; mem[0x30000 + c.sp] = 0x00; mem[0x30000 + c.sp + 1] = 0xFF; /* ip */
		c.cs = seg; c.ip = off;
		printf("# tick %u\n", t);
		if (x86_run(&c, 0xF000, 0xFF00, 2000000)) return 1;
		c = save;
	}
	}
	return 0;
}
