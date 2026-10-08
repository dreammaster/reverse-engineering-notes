/* opl_render DRIVER SONG SECONDS OUT.wav [fx ids...]: play a song (and then effects) through the sound driver + OPL emulator to a WAV file. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../opl.h"
#include "../x86.h"

static Opl opl;
static unsigned pit_div = 0x4006, pit_state;
static void out8(void *u, uint16_t port, uint8_t v) {
	(void)u;
	static uint8_t reg;
	if (port == 0x388) reg = v; else if (port == 0x389) opl_write(&opl, reg, v);
	else if (port == 0x43) pit_state = 0;
	else if (port == 0x40) { if (pit_state == 0) { pit_div = v; pit_state = 1; } else { pit_div |= v << 8; pit_state = 0; } }
}
static uint8_t status;
static uint8_t in8(void *u, uint16_t port) { (void)u; (void)port; return status; }

static uint8_t *slurp(const char *p, size_t *n) { FILE *f = fopen(p, "rb"); if (!f) exit(2); uint8_t *b = malloc(65536); *n = fread(b, 1, 65536, f); fclose(f); return b; }

int main(int argc, char **argv) {
	static uint8_t mem[0x110000];
	X86 c;
	size_t n;
	if (argc < 5) return 2;
	memset(&c, 0, sizeof c);
	opl_reset(&opl);
	c.mem = mem; c.out8 = out8; c.in8 = in8;
	{ uint8_t *b = slurp(argv[1], &n); memcpy(mem + 0x20000, b, n); b = slurp(argv[2], &n); memcpy(mem + 0x40000, b, n); }
	mem[0x20] = 0x20; mem[0x21] = 0xFF; mem[0x22] = 0x00; mem[0x23] = 0xF0; mem[0xFFF20] = 0xCF;
	c.ss = 0x3000; c.sp = 0xFFF0; c.ds = 0x1000; c.es = 0x1000;
	uint16_t a0[1] = { 0x388 }, a6[2] = { 0, 0x4000 };
	x86_call_far(&c, 0x2000, 0, a0, 1, 5000000);
	x86_call_far(&c, 0x2000, 6, a6, 2, 5000000);
	uint16_t off = mem[0x20] | (mem[0x21] << 8), seg = mem[0x22] | (mem[0x23] << 8);
	double seconds = atof(argv[3]);
	unsigned total = (unsigned)(seconds * OPL_RATE);
	int16_t *pcm = malloc(total * 2 * 2 + 65536 * 4);
	unsigned done = 0; double next_tick = 0, tick_len = 0; int fxi = 5;
	double fx_every = argc > 5 ? seconds / (argc - 5 + 1) : 1e30; double next_fx = fx_every;
	while (done < total) {
		if (done >= next_tick) {
			X86 save = c;
			c.sp -= 6; mem[0x30000 + c.sp] = 0x00; mem[0x30000 + c.sp + 1] = 0xFF; mem[0x30000 + c.sp + 2] = 0; mem[0x30000 + c.sp + 3] = 0xF0; mem[0x30000 + c.sp + 4] = (uint8_t)c.flags; mem[0x30000 + c.sp + 5] = (uint8_t)(c.flags >> 8);
			c.cs = seg; c.ip = off;
			x86_run(&c, 0xF000, 0xFF00, 2000000);
			c = save;
			tick_len = (double)OPL_RATE * pit_div / 1193182.0;
			next_tick += tick_len;
		}
		if (fxi < argc && done >= next_fx * OPL_RATE) { uint16_t a9[1] = { (uint16_t)atoi(argv[fxi++]) }; x86_call_far(&c, 0x2000, 9, a9, 1, 5000000); next_fx += fx_every; }
		unsigned chunk = (unsigned)(next_tick - done) + 1; if (chunk > total - done) chunk = total - done; if (chunk == 0) chunk = 1;
		opl_samples(&opl, pcm + done, (int)chunk); done += chunk;
	}
	FILE *w = fopen(argv[4], "wb");
	uint32_t datalen = total * 2, rate = OPL_RATE, v;
	fwrite("RIFF", 1, 4, w); v = 36 + datalen; fwrite(&v, 4, 1, w); fwrite("WAVEfmt ", 1, 8, w); v = 16; fwrite(&v, 4, 1, w);
	uint16_t fmt[2] = { 1, 1 }; fwrite(fmt, 2, 2, w); fwrite(&rate, 4, 1, w); v = rate * 2; fwrite(&v, 4, 1, w); uint16_t ba[2] = { 2, 16 }; fwrite(ba, 2, 2, w);
	fwrite("data", 1, 4, w); fwrite(&datalen, 4, 1, w); fwrite(pcm, 2, total, w); fclose(w);
	fprintf(stderr, "pit divisor %04X (%.1f Hz), %u samples\n", pit_div, 1193182.0 / pit_div, total);
	return 0;
}
