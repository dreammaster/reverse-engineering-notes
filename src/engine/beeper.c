#include "beeper.h"
#include <stdlib.h>
#include <string.h>

#define BSEG 0x3000
#define RET_CS 0x0100
#define CMD_REQ 0x0A
#define CMD_STOP 0x0C

/* ds:4064: sound id -> effect number; bit 7 = looping effect (always starts, remembered in the flag table). Identical in both games. */
static const uint8_t sndtab[0x8C] = {
	0x01, 0x82, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x8c, 0x8d, 0x0e, 0x0f, 0x90, 0x91, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
	0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x9e, 0x1f, 0x20, 0x21, 0x22, 0xa3, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0xaa, 0x2b, 0x2c, 0xad, 0x2e, 0x2f, 0x30,
	0x31, 0x32, 0x33, 0xb4, 0xb5, 0x36, 0xb7, 0x38, 0x39, 0xba, 0x3b, 0x3c, 0x3d, 0x3e, 0xbf, 0xc0, 0x41, 0xc2, 0xc3, 0x44, 0x45, 0x46, 0x00, 0x01,
	0x37, 0x7f, 0x5a, 0x5a, 0x40, 0x6e, 0x32, 0x32, 0x64, 0x7f, 0x21, 0x46, 0x64, 0x0a, 0x46, 0x02, 0x6e, 0x65, 0x64, 0x0a, 0x00, 0x00, 0x00, 0x0a,
	0x5a, 0x5a, 0x5a, 0x7f, 0x1f, 0x64, 0x4f, 0x78, 0x7f, 0x3c, 0x46, 0x5a, 0x1e, 0x40, 0x64, 0x64, 0x46, 0x65, 0x64, 0x46, 0x64, 0x64, 0x6e, 0x65,
	0x65, 0x64, 0x02, 0x02, 0x78, 0x1f, 0x7f, 0x64, 0x02, 0x7f, 0x64, 0x32, 0x65, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x00, 0x00};

static void io_out(void *user, unsigned port, unsigned v, int w) {
	Beeper *b = user;
	(void)w;
	if (port == 0x61) b->port61 = (uint8_t)v;
	else if (port == 0x43) b->npit = 0;
	else if (port == 0x42) {
		b->pit[b->npit & 1] = (uint8_t)v;
		if (++b->npit == 2) { b->divisor = (unsigned)(b->pit[0] | (b->pit[1] << 8)); b->npit = 0; }
	}
}

static unsigned io_in(void *user, unsigned port, int w) {
	Beeper *b = user;
	(void)w;
	return port == 0x61 ? b->port61 : 0;
}

static int call_far(Beeper *b, unsigned off) {
	X86 *c = &b->cpu;
	c->cs = BSEG;
	c->ss = 0x0200;
	c->ds = 0x1F17;
	c->sp = 0xF000;
	x86_push(c, RET_CS);
	x86_push(c, 0);
	c->ip = (uint16_t)off;
	return x86_run(c, RET_CS, 0, 200000);
}

int beeper_init(Beeper *b, const uint8_t *bin, size_t len, int rate) {
	memset(b, 0, sizeof(*b));
	if (len < 8 || len > 0x10000) return -1;
	b->mem = calloc(1, X86_MEM);
	if (!b->mem) return -1;
	memcpy(b->mem + (BSEG << 4), bin, len);
	x86_init(&b->cpu, b->mem);
	b->cpu.io_out = io_out;
	b->cpu.io_in = io_in;
	b->cpu.io_user = b;
	b->rate = rate > 0 ? rate : 44100;
	if (call_far(b, 4) < 0 || (b->cpu.ax & 0xFF) == 0) { beeper_free(b); return -1; }
	return 0;
}

void beeper_free(Beeper *b) {
	free(b->mem);
	b->mem = NULL;
}

unsigned beeper_tick(Beeper *b) {
	if (call_far(b, 0) < 0) return 0;
	return ((b->port61 & 3) == 3) ? b->divisor : 0;
}

static unsigned rdw(const Beeper *b, unsigned off) { return b->mem[(BSEG << 4) + off] | (b->mem[(BSEG << 4) + off + 1] << 8); }

int beeper_busy(const Beeper *b) { return rdw(b, CMD_REQ) != 0; }

void beeper_play(Beeper *b, int id) {
	unsigned e = (unsigned)id < sizeof(sndtab) ? sndtab[id] : 0;
	if (e & 0x80) {
		e &= 0x7F;
		b->flags[e] = (uint8_t)e;
	} else if (rdw(b, CMD_REQ) != 0) {
		return;
	}
	b->mem[(BSEG << 4) + CMD_REQ] = (uint8_t)e;
}

void beeper_stop(Beeper *b, int id) {
	unsigned e = ((unsigned)id + 1) & 0xFF;
	b->flags[e & 0x7F] = 0;
	b->mem[(BSEG << 4) + CMD_STOP] = (uint8_t)e;
	/* 1000:1685: re-issue every looping sound that is still on */
	for (int i = 0; i < 0x80; i++)
		if (b->flags[i]) beeper_play(b, b->flags[i] - 1);
}

void beeper_silence(Beeper *b) {
	memset(b->flags, 0, sizeof(b->flags));
	b->mem[(BSEG << 4) + CMD_REQ] = 0x46;          /* an effect number above 45h: the driver switches both voices off (BEEPER.BIN 05AB) */
	b->mem[(BSEG << 4) + CMD_REQ + 1] = 0;
}

void beeper_render(Beeper *b, int16_t *out, int n) {
	for (int i = 0; i < n; i++) {
		if (b->left <= 0) {
			b->cur_div = beeper_tick(b);
			b->left = b->rate / 50;
		}
		b->left--;
		if (!b->cur_div) { out[i] = 0; continue; }
		double step = (1193182.0 / b->cur_div) / b->rate;
		b->phase += step;
		b->phase -= (long)b->phase;
		out[i] = b->phase < 0.5 ? 6000 : -6000;
	}
}
