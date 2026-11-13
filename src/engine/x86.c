#include "x86.h"
#include <string.h>

enum { CF = 1, PF = 4, AF = 16, ZF = 64, SF = 128, IFL = 512, DFL = 1024, OFL = 2048 };

static uint16_t *R16(X86 *c, int n) {
	switch (n) {
	case 0: return &c->ax;
	case 1: return &c->cx;
	case 2: return &c->dx;
	case 3: return &c->bx;
	case 4: return &c->sp;
	case 5: return &c->bp;
	case 6: return &c->si;
	default: return &c->di;
	}
}
static uint16_t *SEG(X86 *c, int n) {
	switch (n & 3) {
	case 0: return &c->es;
	case 1: return &c->cs;
	case 2: return &c->ss;
	default: return &c->ds;
	}
}

#define PHYS(seg, off) ((((unsigned)(seg) << 4) + ((off) & 0xFFFF)) & 0xFFFFF)
uint8_t x86_rb(const X86 *c, unsigned seg, unsigned off) { return c->mem[PHYS(seg, off)]; }
uint16_t x86_rw(const X86 *c, unsigned seg, unsigned off) { return (uint16_t)(x86_rb(c, seg, off) | (x86_rb(c, seg, off + 1) << 8)); }
void x86_wb(X86 *c, unsigned seg, unsigned off, unsigned v) { c->mem[PHYS(seg, off)] = (uint8_t)v; }
void x86_ww(X86 *c, unsigned seg, unsigned off, unsigned v) { x86_wb(c, seg, off, v); x86_wb(c, seg, off + 1, v >> 8); }

void x86_init(X86 *c, uint8_t *mem) {
	memset(c, 0, sizeof(*c));
	c->mem = mem;
	c->fl = 2;
	c->hook_cs = 0xFFFF;
}

static unsigned fetch8(X86 *c) { unsigned v = x86_rb(c, c->cs, c->ip); c->ip++; return v; }
static unsigned fetch16(X86 *c) { unsigned v = fetch8(c); return v | (fetch8(c) << 8); }
void x86_push(X86 *c, unsigned v) { c->sp -= 2; x86_ww(c, c->ss, c->sp, v); }
unsigned x86_pop(X86 *c) { unsigned v = x86_rw(c, c->ss, c->sp); c->sp += 2; return v; }

static unsigned reg(X86 *c, int n, int w) {
	if (w) return *R16(c, n);
	unsigned r = *R16(c, n & 3);
	return (n & 4) ? (r >> 8) & 0xFF : r & 0xFF;
}
static void setreg(X86 *c, int n, int w, unsigned v) {
	if (w) { *R16(c, n) = (uint16_t)v; return; }
	uint16_t *k = R16(c, n & 3);
	if (n & 4) *k = (uint16_t)((*k & 0x00FF) | ((v & 0xFF) << 8));
	else *k = (uint16_t)((*k & 0xFF00) | (v & 0xFF));
}

typedef struct { int is_reg; int n; uint16_t *seg; unsigned off; } Loc;

static int modrm(X86 *c, uint16_t *seg_override, Loc *loc) {
	unsigned m = fetch8(c);
	int mod = m >> 6, r = (m >> 3) & 7, rm = m & 7;
	if (mod == 3) { loc->is_reg = 1; loc->n = rm; return r; }
	loc->is_reg = 0;
	unsigned off;
	uint16_t *bseg = &c->ds;
	if (mod == 0 && rm == 6) {
		off = fetch16(c);
	} else {
		switch (rm) {
		case 0: off = c->bx + c->si; break;
		case 1: off = c->bx + c->di; break;
		case 2: off = c->bp + c->si; bseg = &c->ss; break;
		case 3: off = c->bp + c->di; bseg = &c->ss; break;
		case 4: off = c->si; break;
		case 5: off = c->di; break;
		case 6: off = c->bp; bseg = &c->ss; break;
		default: off = c->bx; break;
		}
		if (mod == 1) { int d = (int8_t)fetch8(c); off += (unsigned)d; }
		else if (mod == 2) off += fetch16(c);
	}
	loc->seg = seg_override ? seg_override : bseg;
	loc->off = off & 0xFFFF;
	return r;
}
static unsigned rd(X86 *c, const Loc *l, int w) {
	if (l->is_reg) return reg(c, l->n, w);
	return w ? x86_rw(c, *l->seg, l->off) : x86_rb(c, *l->seg, l->off);
}
static void wr(X86 *c, const Loc *l, int w, unsigned v) {
	if (l->is_reg) { setreg(c, l->n, w, v); return; }
	if (w) x86_ww(c, *l->seg, l->off, v); else x86_wb(c, *l->seg, l->off, v);
}

static void setf(X86 *c, unsigned mask, int on) { c->fl = (uint16_t)(on ? (c->fl | mask) : (c->fl & ~mask)); }
static int parity8(unsigned v) { v &= 0xFF; v ^= v >> 4; v ^= v >> 2; v ^= v >> 1; return !(v & 1); }
static void szp(X86 *c, unsigned v, int w) {
	unsigned bits = w ? 16 : 8, mask = (1u << bits) - 1;
	v &= mask;
	setf(c, ZF, v == 0);
	setf(c, SF, v >> (bits - 1));
	setf(c, PF, parity8(v));
}

static unsigned alu(X86 *c, int op, unsigned a, unsigned b, int w) {
	unsigned bits = w ? 16 : 8, mask = (1u << bits) - 1, top = 1u << (bits - 1);
	unsigned cf = (c->fl & CF) ? 1 : 0;
	long r;
	if (op == 0 || op == 2) {
		unsigned cin = op == 2 ? cf : 0;
		r = (long)a + b + cin;
		setf(c, CF, r > (long)mask);
		setf(c, OFL, ((a ^ (unsigned)r) & (b ^ (unsigned)r) & top) != 0);
		setf(c, AF, ((a ^ b ^ (unsigned)r) & 0x10) != 0);
	} else if (op == 5 || op == 7 || op == 3) {
		unsigned cin = op == 3 ? cf : 0;
		r = (long)a - b - cin;
		setf(c, CF, r < 0);
		setf(c, OFL, ((a ^ b) & (a ^ (unsigned)r) & top) != 0);
		setf(c, AF, ((a ^ b ^ (unsigned)r) & 0x10) != 0);
	} else {
		r = op == 1 ? (a | b) : op == 4 ? (a & b) : (a ^ b);
		setf(c, CF, 0);
		setf(c, OFL, 0);
	}
	unsigned res = (unsigned)r & mask;
	szp(c, res, w);
	return res;
}

static int cond(const X86 *c, int cc) {
	unsigned f = c->fl;
	int zf = !!(f & ZF), sf = !!(f & SF), cf = !!(f & CF), of = !!(f & OFL), pf = !!(f & PF), t;
	switch (cc >> 1) {
	case 0: t = of; break;
	case 1: t = cf; break;
	case 2: t = zf; break;
	case 3: t = cf || zf; break;
	case 4: t = sf; break;
	case 5: t = pf; break;
	case 6: t = sf != of; break;
	default: t = zf || (sf != of); break;
	}
	return (cc & 1) ? !t : t;
}

static void fail(X86 *c, int op) { c->err = 1; c->err_op = op; }

static void far_call(X86 *c, unsigned s, unsigned o) {
	x86_push(c, c->cs);
	x86_push(c, c->ip);
	c->cs = (uint16_t)s;
	c->ip = (uint16_t)o;
}

static void shift(X86 *c, int kind, const Loc *l, int w, unsigned cnt) {
	unsigned bits = w ? 16 : 8, mask = (1u << bits) - 1;
	unsigned v = rd(c, l, w);
	for (unsigned i = 0; i < cnt; i++) {
		switch (kind) {
		case 4: setf(c, CF, v >> (bits - 1)); v = (v << 1) & mask; break;
		case 5: setf(c, CF, v & 1); v >>= 1; break;
		case 7: setf(c, CF, v & 1); v = (v >> 1) | (v & (1u << (bits - 1))); break;
		case 0: { unsigned cc = v >> (bits - 1); v = ((v << 1) | cc) & mask; setf(c, CF, cc); break; }
		case 1: { unsigned cc = v & 1; v = (v >> 1) | (cc << (bits - 1)); setf(c, CF, cc); break; }
		default: fail(c, 0xD0 + kind); return;
		}
	}
	if (cnt && (kind == 4 || kind == 5 || kind == 7)) {
		szp(c, v, w);
		setf(c, OFL, 0);
	}
	wr(c, l, w, v);
}

static void group3(X86 *c, int r, const Loc *l, int w) {
	unsigned bits = w ? 16 : 8, mask = (1u << bits) - 1;
	if (r == 0 || r == 1) {
		unsigned imm = w ? fetch16(c) : fetch8(c);
		alu(c, 4, rd(c, l, w), imm, w);
	} else if (r == 2) {
		wr(c, l, w, ~rd(c, l, w) & mask);
	} else if (r == 3) {
		unsigned v = rd(c, l, w);
		unsigned res = alu(c, 5, 0, v, w);
		setf(c, CF, v != 0);
		wr(c, l, w, res);
	} else if (r == 4) {
		unsigned v = rd(c, l, w);
		if (w) {
			uint32_t p = (uint32_t)c->ax * v;
			c->ax = (uint16_t)p; c->dx = (uint16_t)(p >> 16);
			setf(c, CF | OFL, (p >> 16) != 0);
		} else {
			unsigned p = (c->ax & 0xFF) * v;
			c->ax = (uint16_t)p;
			setf(c, CF | OFL, (p >> 8) != 0);
		}
	} else if (r == 6) {
		unsigned v = rd(c, l, w);
		if (v == 0) { fail(c, 0xF6); return; }
		if (w) {
			uint32_t n = ((uint32_t)c->dx << 16) | c->ax;
			uint32_t q = n / v, rem = n % v;
			if (q > 0xFFFF) { fail(c, 0xF7); return; }
			c->ax = (uint16_t)q; c->dx = (uint16_t)rem;
		} else {
			unsigned q = c->ax / v, rem = c->ax % v;
			if (q > 0xFF) { fail(c, 0xF6); return; }
			c->ax = (uint16_t)(q | (rem << 8));
		}
	} else fail(c, 0xF6);
}

static void group5(X86 *c, int r, const Loc *l, int w) {
	if (r == 0 || r == 1) {
		unsigned cf = c->fl & CF;
		unsigned v = alu(c, r == 0 ? 0 : 5, rd(c, l, w), 1, w);
		c->fl = (uint16_t)((c->fl & ~CF) | cf);
		wr(c, l, w, v);
	} else if (r == 2 && w) {
		unsigned t = rd(c, l, 1);
		x86_push(c, c->ip);
		c->ip = (uint16_t)t;
	} else if (r == 3 && w && !l->is_reg) {
		unsigned o = x86_rw(c, *l->seg, l->off), s = x86_rw(c, *l->seg, l->off + 2);
		far_call(c, s, o);
	} else if (r == 4 && w) {
		c->ip = (uint16_t)rd(c, l, 1);
	} else if (r == 6 && w) {
		x86_push(c, rd(c, l, 1));
	} else fail(c, 0xFF);
}

static void string_op(X86 *c, int op, uint16_t *seg, int rep) {
	int w = op & 1;
	int step = (c->fl & DFL) ? (w ? -2 : -1) : (w ? 2 : 1);
	unsigned n = rep ? c->cx : 1;
	uint16_t *sg = seg ? seg : &c->ds;
	while (n) {
		if (op == 0xA4 || op == 0xA5) {
			unsigned v = w ? x86_rw(c, *sg, c->si) : x86_rb(c, *sg, c->si);
			if (w) x86_ww(c, c->es, c->di, v); else x86_wb(c, c->es, c->di, v);
			c->si += step; c->di += step;
		} else if (op == 0xAA || op == 0xAB) {
			if (w) x86_ww(c, c->es, c->di, c->ax); else x86_wb(c, c->es, c->di, c->ax & 0xFF);
			c->di += step;
		} else {
			unsigned v = w ? x86_rw(c, *sg, c->si) : x86_rb(c, *sg, c->si);
			setreg(c, 0, w, v);
			c->si += step;
		}
		n--;
		if (rep) c->cx--;
	}
}

void x86_step(X86 *c) {
	if (c->hook && c->cs == c->hook_cs && c->ip == c->hook_ip) {
		c->hook(c, c->hook_user);
		return;
	}
	uint16_t *seg = NULL;
	int rep = 0;
	unsigned op = fetch8(c);
	for (;;) {
		if (op == 0x26) seg = &c->es;
		else if (op == 0x2E) seg = &c->cs;
		else if (op == 0x36) seg = &c->ss;
		else if (op == 0x3E) seg = &c->ds;
		else if (op == 0xF2 || op == 0xF3) rep = 1;
		else break;
		op = fetch8(c);
	}
	Loc l;
	if (op < 0x40 && (op & 7) < 6) {
		int kind = op >> 3, form = op & 7;
		if (form < 4) {
			int w = form & 1;
			int r = modrm(c, seg, &l);
			if (form < 2) {
				unsigned res = alu(c, kind, rd(c, &l, w), reg(c, r, w), w);
				if (kind != 7) wr(c, &l, w, res);
			} else {
				unsigned res = alu(c, kind, reg(c, r, w), rd(c, &l, w), w);
				if (kind != 7) setreg(c, r, w, res);
			}
		} else {
			int w = form & 1;
			unsigned imm = w ? fetch16(c) : fetch8(c);
			unsigned res = alu(c, kind, reg(c, 0, w), imm, w);
			if (kind != 7) setreg(c, 0, w, res);
		}
	} else if (op >= 0x40 && op < 0x50) {
		int n = op & 7;
		uint16_t *p = R16(c, n);
		unsigned cf = c->fl & CF;
		unsigned res = alu(c, op < 0x48 ? 0 : 5, *p, 1, 1);
		c->fl = (uint16_t)((c->fl & ~CF) | cf);
		*p = (uint16_t)res;
	} else if (op == 0x06 || op == 0x0E || op == 0x16 || op == 0x1E) {
		x86_push(c, *SEG(c, (op >> 3) & 3));
	} else if (op == 0x07 || op == 0x17 || op == 0x1F) {
		*SEG(c, (op >> 3) & 3) = (uint16_t)x86_pop(c);
	} else if (op == 0xE4 || op == 0xE5 || op == 0xEC || op == 0xED) {
		int w = op & 1;
		unsigned port = op < 0xE8 ? fetch8(c) : c->dx;
		setreg(c, 0, w, c->io_in ? c->io_in(c->io_user, port, w) : 0);
	} else if (op == 0xE6 || op == 0xE7 || op == 0xEE || op == 0xEF) {
		int w = op & 1;
		unsigned port = op < 0xE8 ? fetch8(c) : c->dx;
		if (c->io_out) c->io_out(c->io_user, port, reg(c, 0, w), w);
	} else if (op >= 0x50 && op < 0x58) {
		x86_push(c, (op & 7) != 4 ? *R16(c, op & 7) : (uint16_t)(c->sp - 2));
	} else if (op >= 0x58 && op < 0x60) {
		*R16(c, op & 7) = (uint16_t)x86_pop(c);
	} else if (op >= 0x70 && op < 0x80) {
		int d = (int8_t)fetch8(c);
		if (cond(c, op & 15)) c->ip = (uint16_t)(c->ip + d);
	} else if (op == 0x80 || op == 0x81 || op == 0x83) {
		int w = op & 1;
		int r = modrm(c, seg, &l);
		unsigned imm;
		if (op == 0x81) imm = fetch16(c);
		else { imm = fetch8(c); if (op == 0x83 && imm >= 128) imm |= 0xFF00; }
		unsigned a = rd(c, &l, w);
		unsigned res = alu(c, r, a, imm & (w ? 0xFFFF : 0xFF), w);
		if (r != 7) wr(c, &l, w, res);
	} else if (op == 0x84 || op == 0x85) {
		int w = op & 1;
		int r = modrm(c, seg, &l);
		alu(c, 4, rd(c, &l, w), reg(c, r, w), w);
	} else if (op == 0x86 || op == 0x87) {
		int w = op & 1;
		int r = modrm(c, seg, &l);
		unsigned a = rd(c, &l, w), b = reg(c, r, w);
		wr(c, &l, w, b);
		setreg(c, r, w, a);
	} else if (op == 0x88 || op == 0x89) {
		int w = op & 1;
		int r = modrm(c, seg, &l);
		wr(c, &l, w, reg(c, r, w));
	} else if (op == 0x8A || op == 0x8B) {
		int w = op & 1;
		int r = modrm(c, seg, &l);
		setreg(c, r, w, rd(c, &l, w));
	} else if (op == 0x8C) {
		int r = modrm(c, seg, &l);
		wr(c, &l, 1, *SEG(c, r));
	} else if (op == 0x8E) {
		int r = modrm(c, seg, &l);
		*SEG(c, r) = (uint16_t)rd(c, &l, 1);
	} else if (op == 0x8D) {
		int r = modrm(c, seg, &l);
		if (l.is_reg) { fail(c, op); return; }
		setreg(c, r, 1, l.off);
	} else if (op == 0x90) {
	} else if (op >= 0x91 && op < 0x98) {
		uint16_t *p = R16(c, op & 7), t = c->ax;
		c->ax = *p; *p = t;
	} else if (op == 0x98) {
		c->ax = (uint16_t)((c->ax & 0xFF) | ((c->ax & 0x80) ? 0xFF00 : 0));
	} else if (op == 0x99) {
		c->dx = (c->ax & 0x8000) ? 0xFFFF : 0;
	} else if (op == 0x9C) {
		x86_push(c, c->fl | 2);
	} else if (op == 0x9D) {
		c->fl = (uint16_t)((x86_pop(c) & 0x0FD5) | 2);
	} else if (op == 0x9A) {
		unsigned o = fetch16(c), s = fetch16(c);
		far_call(c, s, o);
	} else if (op >= 0xA0 && op <= 0xA3) {
		int w = op & 1;
		unsigned off = fetch16(c);
		uint16_t sg = seg ? *seg : c->ds;
		if (op < 0xA2) setreg(c, 0, w, w ? x86_rw(c, sg, off) : x86_rb(c, sg, off));
		else if (w) x86_ww(c, sg, off, c->ax);
		else x86_wb(c, sg, off, c->ax);
	} else if (op == 0xA4 || op == 0xA5 || op == 0xAA || op == 0xAB || op == 0xAC || op == 0xAD) {
		string_op(c, (int)op, seg, rep);
	} else if (op == 0xA8 || op == 0xA9) {
		int w = op & 1;
		unsigned imm = w ? fetch16(c) : fetch8(c);
		alu(c, 4, reg(c, 0, w), imm, w);
	} else if (op >= 0xB0 && op < 0xB8) {
		setreg(c, op & 7, 0, fetch8(c));
	} else if (op >= 0xB8 && op < 0xC0) {
		setreg(c, op & 7, 1, fetch16(c));
	} else if (op == 0xC0 || op == 0xC1 || (op >= 0xD0 && op <= 0xD3)) {
		int w = op & 1;
		int r = modrm(c, seg, &l);
		unsigned cnt = op < 0xC2 ? fetch8(c) : op < 0xD2 ? 1 : (c->cx & 0xFF);
		shift(c, r, &l, w, cnt & 31);
	} else if (op == 0xC3) {
		c->ip = (uint16_t)x86_pop(c);
	} else if (op == 0xC2) {
		unsigned n = fetch16(c);
		c->ip = (uint16_t)x86_pop(c);
		c->sp += n;
	} else if (op == 0xCB) {
		c->ip = (uint16_t)x86_pop(c);
		c->cs = (uint16_t)x86_pop(c);
	} else if (op == 0xC6 || op == 0xC7) {
		int w = op & 1;
		modrm(c, seg, &l);
		wr(c, &l, w, w ? fetch16(c) : fetch8(c));
	} else if (op == 0xF6 || op == 0xF7) {
		int w = op & 1;
		int r = modrm(c, seg, &l);
		group3(c, r, &l, w);
	} else if (op == 0xFE || op == 0xFF) {
		int w = op & 1;
		int r = modrm(c, seg, &l);
		group5(c, r, &l, w);
	} else if (op == 0xE2) {
		int d = (int8_t)fetch8(c);
		c->cx--;
		if (c->cx) c->ip = (uint16_t)(c->ip + d);
	} else if (op == 0xE3) {
		int d = (int8_t)fetch8(c);
		if (!c->cx) c->ip = (uint16_t)(c->ip + d);
	} else if (op == 0xE8) {
		unsigned d = fetch16(c);
		x86_push(c, c->ip);
		c->ip = (uint16_t)(c->ip + d);
	} else if (op == 0xE9) {
		unsigned d = fetch16(c);
		c->ip = (uint16_t)(c->ip + d);
	} else if (op == 0xEB) {
		int d = (int8_t)fetch8(c);
		c->ip = (uint16_t)(c->ip + d);
	} else if (op == 0xF8) setf(c, CF, 0);
	else if (op == 0xF9) setf(c, CF, 1);
	else if (op == 0xFA) setf(c, IFL, 0);
	else if (op == 0xFB) setf(c, IFL, 1);
	else if (op == 0xFC) setf(c, DFL, 0);
	else if (op == 0xFD) setf(c, DFL, 1);
	else if (op == 0xD7) setreg(c, 0, 0, x86_rb(c, seg ? *seg : c->ds, c->bx + (c->ax & 0xFF)));
	else fail(c, (int)op);
}

int x86_run(X86 *c, unsigned stop_cs, unsigned stop_ip, int max_steps) {
	int n = 0;
	while (n < max_steps) {
		if (c->cs == stop_cs && c->ip == stop_ip)
			return n;
		x86_step(c);
		if (c->err)
			return -1;
		n++;
	}
	return -1;
}
