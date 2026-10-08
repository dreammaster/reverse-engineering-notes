#include "x86.h"

#include <stdio.h>
#include <string.h>

enum { CF = 1, PF = 4, AF = 0x10, ZF = 0x40, SF = 0x80, TF = 0x100, IF = 0x200, DF = 0x400, OF = 0x800 };

#define MEMSIZE 0x110000u
static uint32_t lin(uint16_t seg, uint16_t off) { return ((uint32_t)seg << 4) + off; }
static uint8_t rd8(X86 *c, uint16_t seg, uint16_t off) { return c->mem[lin(seg, off) % MEMSIZE]; }
static uint16_t rd16(X86 *c, uint16_t seg, uint16_t off) { return (uint16_t)(rd8(c, seg, off) | (rd8(c, seg, (uint16_t)(off + 1)) << 8)); }
static void wr8(X86 *c, uint16_t seg, uint16_t off, uint8_t v) { c->mem[lin(seg, off) % MEMSIZE] = v; }
static void wr16(X86 *c, uint16_t seg, uint16_t off, uint16_t v) { wr8(c, seg, off, (uint8_t)v); wr8(c, seg, (uint16_t)(off + 1), (uint8_t)(v >> 8)); }

static uint16_t *reg16p(X86 *c, int i) { uint16_t *r[8] = { &c->ax, &c->cx, &c->dx, &c->bx, &c->sp, &c->bp, &c->si, &c->di }; return r[i]; }
static uint8_t get_reg8(X86 *c, int i) { uint16_t v = *reg16p(c, i & 3); return (i & 4) ? (uint8_t)(v >> 8) : (uint8_t)v; }
static void set_reg8(X86 *c, int i, uint8_t v) { uint16_t *p = reg16p(c, i & 3); *p = (i & 4) ? (uint16_t)((*p & 0xFF) | (v << 8)) : (uint16_t)((*p & 0xFF00) | v); }
static uint16_t *segp(X86 *c, int i) { uint16_t *s[4] = { &c->es, &c->cs, &c->ss, &c->ds }; return s[i & 3]; }

static void push(X86 *c, uint16_t v) { c->sp -= 2; wr16(c, c->ss, c->sp, v); }
static uint16_t pop(X86 *c) { uint16_t v = rd16(c, c->ss, c->sp); c->sp += 2; return v; }

static uint8_t fetch8(X86 *c) { return rd8(c, c->cs, c->ip++); }
static uint16_t fetch16(X86 *c) { uint16_t v = rd16(c, c->cs, c->ip); c->ip += 2; return v; }

typedef struct { int is_reg; int reg; uint16_t seg, off; } Operand;
typedef struct { int seg_override; /* -1 none, else index in es,cs,ss,ds */ int rep; /* 0, 0xF2, 0xF3 */ } Prefix;

static Operand decode_rm(X86 *c, uint8_t modrm, const Prefix *p) {
	Operand o = { 0, 0, 0, 0 };
	int mod = modrm >> 6, rm = modrm & 7;
	if (mod == 3) { o.is_reg = 1; o.reg = rm; return o; }
	uint16_t off;
	int defseg = 3; /* ds */
	switch (rm) {
	case 0: off = c->bx + c->si; break;
	case 1: off = c->bx + c->di; break;
	case 2: off = c->bp + c->si; defseg = 2; break;
	case 3: off = c->bp + c->di; defseg = 2; break;
	case 4: off = c->si; break;
	case 5: off = c->di; break;
	case 6: off = c->bp; defseg = 2; break;
	default: off = c->bx; break;
	}
	if (mod == 0 && rm == 6) { off = fetch16(c); defseg = 3; }
	else if (mod == 1) off += (uint16_t)(int16_t)(int8_t)fetch8(c);
	else if (mod == 2) off += fetch16(c);
	o.off = off;
	o.seg = *segp(c, p->seg_override >= 0 ? p->seg_override : defseg);
	return o;
}

static uint16_t get_op(X86 *c, const Operand *o, int w) {
	if (o->is_reg) return w ? *reg16p(c, o->reg) : get_reg8(c, o->reg);
	return w ? rd16(c, o->seg, o->off) : rd8(c, o->seg, o->off);
}
static void set_op(X86 *c, const Operand *o, int w, uint16_t v) {
	if (o->is_reg) { if (w) *reg16p(c, o->reg) = v; else set_reg8(c, o->reg, (uint8_t)v); }
	else if (w) wr16(c, o->seg, o->off, v);
	else wr8(c, o->seg, o->off, (uint8_t)v);
}

static void setf(X86 *c, unsigned mask, int on) { if (on) c->flags |= mask; else c->flags &= (uint16_t)~mask; }
static int parity(uint8_t v) { v ^= v >> 4; v ^= v >> 2; v ^= v >> 1; return !(v & 1); }
static void set_szp(X86 *c, uint32_t r, int w) {
	uint32_t m = w ? 0xFFFF : 0xFF, sign = w ? 0x8000 : 0x80;
	setf(c, ZF, (r & m) == 0); setf(c, SF, (r & sign) != 0); setf(c, PF, parity((uint8_t)r));
}

/* ALU: op 0 add 1 or 2 adc 3 sbb 4 and 5 sub 6 xor 7 cmp */
static uint16_t alu(X86 *c, int op, uint16_t a, uint16_t b, int w) {
	uint32_t m = w ? 0xFFFF : 0xFF, sign = w ? 0x8000 : 0x80, r;
	unsigned carry = c->flags & CF ? 1 : 0;
	a &= m; b &= m;
	switch (op) {
	case 0: case 2: {
		unsigned cin = op == 2 ? carry : 0;
		r = (uint32_t)a + b + cin;
		setf(c, CF, r > m); setf(c, AF, ((a ^ b ^ r) & 0x10) != 0);
		setf(c, OF, (~(a ^ b) & (a ^ r) & sign) != 0);
		break;
	}
	case 5: case 7: case 3: {
		unsigned cin = op == 3 ? carry : 0;
		r = (uint32_t)a - b - cin;
		setf(c, CF, (uint32_t)a < (uint32_t)b + cin); setf(c, AF, ((a ^ b ^ r) & 0x10) != 0);
		setf(c, OF, ((a ^ b) & (a ^ r) & sign) != 0);
		break;
	}
	case 1: r = a | b; setf(c, CF, 0); setf(c, OF, 0); setf(c, AF, 0); break;
	case 4: r = a & b; setf(c, CF, 0); setf(c, OF, 0); setf(c, AF, 0); break;
	default: r = a ^ b; setf(c, CF, 0); setf(c, OF, 0); setf(c, AF, 0); break;
	}
	set_szp(c, r, w);
	return (uint16_t)(r & m);
}

static uint16_t incdec(X86 *c, uint16_t v, int dec, int w) {
	unsigned cf = c->flags & CF;
	uint16_t r = alu(c, dec ? 5 : 0, v, 1, w);
	setf(c, CF, cf);
	return r;
}

static uint16_t shift(X86 *c, int op, uint16_t v, unsigned n, int w) {
	uint32_t m = w ? 0xFFFF : 0xFF, sign = w ? 0x8000 : 0x80;
	unsigned bits = w ? 16 : 8;
	uint32_t r = v & m;
	n &= 31;
	for (unsigned i = 0; i < n; i++) {
		uint32_t out;
		switch (op) {
		case 0: out = (r & sign) != 0; r = ((r << 1) | out) & m; setf(c, CF, out); break;                                   /* rol */
		case 1: out = r & 1; r = ((r >> 1) | (out ? sign : 0)) & m; setf(c, CF, out); break;                               /* ror */
		case 2: out = (r & sign) != 0; r = ((r << 1) | (c->flags & CF ? 1 : 0)) & m; setf(c, CF, out); break;             /* rcl */
		case 3: out = r & 1; r = ((r >> 1) | (c->flags & CF ? sign : 0)) & m; setf(c, CF, out); break;                    /* rcr */
		case 4: case 6: out = (r & sign) != 0; r = (r << 1) & m; setf(c, CF, out); break;                                    /* shl */
		case 5: out = r & 1; r = r >> 1; setf(c, CF, out); break;                                                            /* shr */
		default: out = r & 1; r = (r >> 1) | (r & sign); setf(c, CF, out); break;                                           /* sar */
		}
	}
	if (n) {
		if (op >= 4) { set_szp(c, r, w); setf(c, AF, 0); }
		/* OF (defined for 1-bit shifts only; kept simple) */
		if (op == 0 || op == 2) setf(c, OF, ((r & sign) != 0) != ((c->flags & CF) != 0));
		else if (op == 1 || op == 3) setf(c, OF, ((r & sign) != 0) != ((r & (sign >> 1)) != 0));
		else if (op == 4 || op == 6) setf(c, OF, ((r & sign) != 0) != ((c->flags & CF) != 0));
		else if (op == 5) setf(c, OF, ((v & m) & sign) != 0);
		else setf(c, OF, 0);
	}
	(void)bits;
	return (uint16_t)r;
}

static int cond(X86 *c, int cc) {
	int cf = c->flags & CF, zf = c->flags & ZF, sf = (c->flags & SF) != 0, of = (c->flags & OF) != 0, pf = c->flags & PF, r;
	switch (cc >> 1) {
	case 0: r = of; break;
	case 1: r = cf != 0; break;
	case 2: r = zf != 0; break;
	case 3: r = cf || zf; break;
	case 4: r = sf; break;
	case 5: r = pf != 0; break;
	case 6: r = sf != of; break;
	default: r = zf || (sf != of); break;
	}
	return (cc & 1) ? !r : r;
}

static void do_interrupt(X86 *c, uint8_t vec) {
	if (c->intr) { c->intr(c, vec); return; }
	push(c, c->flags); push(c, c->cs); push(c, c->ip);
	c->ip = rd16(c, 0, (uint16_t)(vec * 4)); c->cs = rd16(c, 0, (uint16_t)(vec * 4 + 2));
	setf(c, IF | TF, 0);
}

/* one string operation (with rep handling by the caller) */
static void string_op(X86 *c, uint8_t op, const Prefix *p, int w) {
	int step = (c->flags & DF) ? -(w ? 2 : 1) : (w ? 2 : 1);
	uint16_t srcseg = *segp(c, p->seg_override >= 0 ? p->seg_override : 3);
	switch (op) {
	case 0xA4: case 0xA5: { uint16_t v = w ? rd16(c, srcseg, c->si) : rd8(c, srcseg, c->si); if (w) wr16(c, c->es, c->di, v); else wr8(c, c->es, c->di, (uint8_t)v); c->si += step; c->di += step; break; }
	case 0xAA: case 0xAB: if (w) wr16(c, c->es, c->di, c->ax); else wr8(c, c->es, c->di, (uint8_t)c->ax); c->di += step; break;
	case 0xAC: case 0xAD: if (w) c->ax = rd16(c, srcseg, c->si); else set_reg8(c, 0, rd8(c, srcseg, c->si)); c->si += step; break;
	case 0xA6: case 0xA7: { uint16_t a = w ? rd16(c, srcseg, c->si) : rd8(c, srcseg, c->si), b = w ? rd16(c, c->es, c->di) : rd8(c, c->es, c->di); alu(c, 7, a, b, w); c->si += step; c->di += step; break; }
	default: { uint16_t b = w ? rd16(c, c->es, c->di) : rd8(c, c->es, c->di); alu(c, 7, w ? c->ax : (uint8_t)c->ax, b, w); c->di += step; break; } /* scas */
	}
}

static int step(X86 *c) {
	Prefix p = { -1, 0 };
	uint8_t op;
	c->steps++;
	for (;;) {
		op = fetch8(c);
		if (op == 0x26) p.seg_override = 0; else if (op == 0x2E) p.seg_override = 1; else if (op == 0x36) p.seg_override = 2; else if (op == 0x3E) p.seg_override = 3;
		else if (op == 0xF2 || op == 0xF3) p.rep = op; else if (op == 0xF0) {} else break;
	}
	if (op < 0x40 && (op & 7) < 6) { /* ALU group */
		int aop = op >> 3, form = op & 7, w = form & 1;
		if (form < 4) {
			uint8_t modrm = fetch8(c);
			Operand o = decode_rm(c, modrm, &p);
			int r = (modrm >> 3) & 7;
			uint16_t rv = w ? *reg16p(c, r) : get_reg8(c, r);
			uint16_t res;
			if (form < 2) { res = alu(c, aop, get_op(c, &o, w), rv, w); if (aop != 7) set_op(c, &o, w, res); }
			else { res = alu(c, aop, rv, get_op(c, &o, w), w); if (aop != 7) { if (w) *reg16p(c, r) = res; else set_reg8(c, r, (uint8_t)res); } }
		} else {
			uint16_t imm = w ? fetch16(c) : fetch8(c), res = alu(c, aop, w ? c->ax : (uint8_t)c->ax, imm, w);
			if (aop != 7) { if (w) c->ax = res; else set_reg8(c, 0, (uint8_t)res); }
		}
		return 0;
	}
	switch (op) {
	case 0x06: case 0x0E: case 0x16: case 0x1E: push(c, *segp(c, (op >> 3) & 3)); return 0;
	case 0x07: case 0x17: case 0x1F: *segp(c, (op >> 3) & 3) = pop(c); return 0;
	case 0x27: case 0x2F: { /* daa / das */
		uint8_t al = (uint8_t)c->ax, old = al; int oc = c->flags & CF;
		if ((al & 15) > 9 || (c->flags & AF)) { al += op == 0x27 ? 6 : -6; setf(c, AF, 1); } else setf(c, AF, 0);
		if (old > 0x99 || oc) { al += op == 0x27 ? 0x60 : -0x60; setf(c, CF, 1); } else setf(c, CF, 0);
		set_reg8(c, 0, al); set_szp(c, al, 0); return 0;
	}
	case 0x37: case 0x3F: { /* aaa / aas */
		if ((c->ax & 15) > 9 || (c->flags & AF)) {
			if (op == 0x37) { c->ax += 0x106; } else { c->ax -= 6; c->ax = (uint16_t)(c->ax - 0x100); }
			setf(c, AF, 1); setf(c, CF, 1);
		} else { setf(c, AF, 0); setf(c, CF, 0); }
		c->ax &= 0xFF0F; return 0;
	}
	case 0x60: { uint16_t t = c->sp; push(c, c->ax); push(c, c->cx); push(c, c->dx); push(c, c->bx); push(c, t); push(c, c->bp); push(c, c->si); push(c, c->di); return 0; }
	case 0x61: c->di = pop(c); c->si = pop(c); c->bp = pop(c); pop(c); c->bx = pop(c); c->dx = pop(c); c->cx = pop(c); c->ax = pop(c); return 0;
	case 0x68: push(c, fetch16(c)); return 0;
	case 0x6A: push(c, (uint16_t)(int16_t)(int8_t)fetch8(c)); return 0;
	case 0x69: case 0x6B: {
		uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p);
		int16_t imm = op == 0x69 ? (int16_t)fetch16(c) : (int8_t)fetch8(c);
		int32_t r = (int16_t)get_op(c, &o, 1) * (int32_t)imm;
		*reg16p(c, (modrm >> 3) & 7) = (uint16_t)r; setf(c, CF, r != (int16_t)r); setf(c, OF, r != (int16_t)r); return 0;
	}
	case 0x84: case 0x85: { int w = op & 1; uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); int r = (modrm >> 3) & 7; alu(c, 4, get_op(c, &o, w), w ? *reg16p(c, r) : get_reg8(c, r), w); return 0; }
	case 0x86: case 0x87: { int w = op & 1; uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); int r = (modrm >> 3) & 7; uint16_t a = get_op(c, &o, w), b = w ? *reg16p(c, r) : get_reg8(c, r); set_op(c, &o, w, b); if (w) *reg16p(c, r) = a; else set_reg8(c, r, (uint8_t)a); return 0; }
	case 0x88: case 0x89: { int w = op & 1; uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); int r = (modrm >> 3) & 7; set_op(c, &o, w, w ? *reg16p(c, r) : get_reg8(c, r)); return 0; }
	case 0x8A: case 0x8B: { int w = op & 1; uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); int r = (modrm >> 3) & 7; uint16_t v = get_op(c, &o, w); if (w) *reg16p(c, r) = v; else set_reg8(c, r, (uint8_t)v); return 0; }
	case 0x8C: { uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); set_op(c, &o, 1, *segp(c, (modrm >> 3) & 3)); return 0; }
	case 0x8D: { uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); *reg16p(c, (modrm >> 3) & 7) = o.off; return 0; }
	case 0x8E: { uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); *segp(c, (modrm >> 3) & 3) = get_op(c, &o, 1); return 0; }
	case 0x8F: { uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); uint16_t v = pop(c); set_op(c, &o, 1, v); return 0; }
	case 0x90: return 0;
	case 0x98: c->ax = (uint16_t)(int16_t)(int8_t)c->ax; return 0;
	case 0x99: c->dx = (c->ax & 0x8000) ? 0xFFFF : 0; return 0;
	case 0x9A: { uint16_t off = fetch16(c), seg = fetch16(c); push(c, c->cs); push(c, c->ip); c->ip = off; c->cs = seg; return 0; }
	case 0x9B: return 0;
	case 0x9C: push(c, c->flags); return 0;
	case 0x9D: c->flags = (uint16_t)((pop(c) & 0x0FD5) | 2); return 0;
	case 0x9E: c->flags = (uint16_t)((c->flags & 0xFF00) | ((c->ax >> 8) & 0xD5) | 2); return 0;
	case 0x9F: set_reg8(c, 4, (uint8_t)((c->flags & 0xD5) | 2)); return 0;
	case 0xA0: set_reg8(c, 0, rd8(c, *segp(c, p.seg_override >= 0 ? p.seg_override : 3), fetch16(c))); return 0;
	case 0xA1: c->ax = rd16(c, *segp(c, p.seg_override >= 0 ? p.seg_override : 3), fetch16(c)); return 0;
	case 0xA2: wr8(c, *segp(c, p.seg_override >= 0 ? p.seg_override : 3), fetch16(c), (uint8_t)c->ax); return 0;
	case 0xA3: wr16(c, *segp(c, p.seg_override >= 0 ? p.seg_override : 3), fetch16(c), c->ax); return 0;
	case 0xA8: alu(c, 4, (uint8_t)c->ax, fetch8(c), 0); return 0;
	case 0xA9: alu(c, 4, c->ax, fetch16(c), 1); return 0;
	case 0xA4: case 0xA5: case 0xA6: case 0xA7: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF: {
		int w = op & 1;
		if (!p.rep) { string_op(c, op, &p, w); return 0; }
		while (c->cx) {
			string_op(c, op, &p, w);
			c->cx--;
			if (op == 0xA6 || op == 0xA7 || op == 0xAE || op == 0xAF) {
				int zf = (c->flags & ZF) != 0;
				if ((p.rep == 0xF3) != zf) break;
			}
		}
		return 0;
	}
	case 0xC0: case 0xC1: case 0xD0: case 0xD1: case 0xD2: case 0xD3: {
		int w = op & 1; uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p);
		unsigned n = (op == 0xD0 || op == 0xD1) ? 1 : (op >= 0xD2 ? (uint8_t)c->cx : fetch8(c));
		set_op(c, &o, w, shift(c, (modrm >> 3) & 7, get_op(c, &o, w), n, w)); return 0;
	}
	case 0xC2: { uint16_t n = fetch16(c); c->ip = pop(c); c->sp += n; return 0; }
	case 0xC3: c->ip = pop(c); return 0;
	case 0xC4: case 0xC5: { uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); *reg16p(c, (modrm >> 3) & 7) = rd16(c, o.seg, o.off); *(op == 0xC4 ? &c->es : &c->ds) = rd16(c, o.seg, (uint16_t)(o.off + 2)); return 0; }
	case 0xC6: case 0xC7: { int w = op & 1; uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); uint16_t imm = w ? fetch16(c) : fetch8(c); set_op(c, &o, w, imm); return 0; }
	case 0xC8: { uint16_t size = fetch16(c); uint8_t lvl = fetch8(c); push(c, c->bp); c->bp = c->sp; (void)lvl; c->sp -= size; return 0; }
	case 0xC9: c->sp = c->bp; c->bp = pop(c); return 0;
	case 0xCA: { uint16_t n = fetch16(c); c->ip = pop(c); c->cs = pop(c); c->sp += n; return 0; }
	case 0xCB: c->ip = pop(c); c->cs = pop(c); return 0;
	case 0xCC: do_interrupt(c, 3); return 0;
	case 0xCD: do_interrupt(c, fetch8(c)); return 0;
	case 0xCF: c->ip = pop(c); c->cs = pop(c); c->flags = (uint16_t)((pop(c) & 0x0FD5) | 2); return 0;
	case 0xD4: { uint8_t b = fetch8(c); uint8_t al = (uint8_t)c->ax; c->ax = (uint16_t)(((al / b) << 8) | (al % b)); set_szp(c, c->ax & 0xFF, 0); return 0; }
	case 0xD5: { uint8_t b = fetch8(c); c->ax = (uint16_t)(((c->ax >> 8) * b + (uint8_t)c->ax) & 0xFF); set_szp(c, c->ax, 0); return 0; }
	case 0xD7: set_reg8(c, 0, rd8(c, *segp(c, p.seg_override >= 0 ? p.seg_override : 3), (uint16_t)(c->bx + (uint8_t)c->ax))); return 0;
	case 0xE0: case 0xE1: case 0xE2: case 0xE3: {
		int8_t d = (int8_t)fetch8(c); int take;
		if (op == 0xE3) take = c->cx == 0;
		else { c->cx--; take = c->cx != 0; if (op == 0xE0) take = take && !(c->flags & ZF); else if (op == 0xE1) take = take && (c->flags & ZF); }
		if (take) c->ip += d;
		return 0;
	}
	case 0xE4: { uint8_t port = fetch8(c); set_reg8(c, 0, c->in8 ? c->in8(c->io_user, port) : 0); return 0; }
	case 0xE5: { uint8_t port = fetch8(c); uint8_t lo = c->in8 ? c->in8(c->io_user, port) : 0; c->ax = lo; return 0; }
	case 0xE6: { uint8_t port = fetch8(c); if (c->out8) c->out8(c->io_user, port, (uint8_t)c->ax); return 0; }
	case 0xE7: { uint8_t port = fetch8(c); if (c->out8) { c->out8(c->io_user, port, (uint8_t)c->ax); c->out8(c->io_user, (uint16_t)(port + 1), (uint8_t)(c->ax >> 8)); } return 0; }
	case 0xE8: { int16_t d = (int16_t)fetch16(c); push(c, c->ip); c->ip += d; return 0; }
	case 0xE9: { int16_t d = (int16_t)fetch16(c); c->ip += d; return 0; }
	case 0xEA: { uint16_t off = fetch16(c), seg = fetch16(c); c->ip = off; c->cs = seg; return 0; }
	case 0xEB: { int8_t d = (int8_t)fetch8(c); c->ip += d; return 0; }
	case 0xEC: set_reg8(c, 0, c->in8 ? c->in8(c->io_user, c->dx) : 0); return 0;
	case 0xED: c->ax = c->in8 ? c->in8(c->io_user, c->dx) : 0; return 0;
	case 0xEE: if (c->out8) c->out8(c->io_user, c->dx, (uint8_t)c->ax); return 0;
	case 0xEF: if (c->out8) { c->out8(c->io_user, c->dx, (uint8_t)c->ax); c->out8(c->io_user, (uint16_t)(c->dx + 1), (uint8_t)(c->ax >> 8)); } return 0;
	case 0xF4: c->halted = 1; return 0;
	case 0xF5: c->flags ^= CF; return 0;
	case 0xF6: case 0xF7: {
		int w = op & 1; uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); int sub = (modrm >> 3) & 7;
		uint16_t v = get_op(c, &o, w);
		switch (sub) {
		case 0: case 1: alu(c, 4, v, w ? fetch16(c) : fetch8(c), w); break;
		case 2: set_op(c, &o, w, (uint16_t)~v); break;
		case 3: { uint16_t r = alu(c, 5, 0, v, w); set_op(c, &o, w, r); setf(c, CF, v != 0); break; }
		case 4: if (w) { uint32_t r = (uint32_t)c->ax * v; c->ax = (uint16_t)r; c->dx = (uint16_t)(r >> 16); setf(c, CF, c->dx != 0); setf(c, OF, c->dx != 0); } else { uint16_t r = (uint16_t)((uint8_t)c->ax * (uint8_t)v); c->ax = r; setf(c, CF, (r >> 8) != 0); setf(c, OF, (r >> 8) != 0); } break;
		case 5: if (w) { int32_t r = (int32_t)(int16_t)c->ax * (int16_t)v; c->ax = (uint16_t)r; c->dx = (uint16_t)(r >> 16); setf(c, CF, r != (int16_t)r); setf(c, OF, r != (int16_t)r); } else { int16_t r = (int16_t)((int8_t)c->ax * (int8_t)v); c->ax = (uint16_t)r; setf(c, CF, r != (int8_t)r); setf(c, OF, r != (int8_t)r); } break;
		case 6: if (w) { uint32_t n = ((uint32_t)c->dx << 16) | c->ax; if (!v) { do_interrupt(c, 0); break; } c->ax = (uint16_t)(n / v); c->dx = (uint16_t)(n % v); } else { if (!v) { do_interrupt(c, 0); break; } uint16_t n = c->ax; set_reg8(c, 0, (uint8_t)(n / v)); set_reg8(c, 4, (uint8_t)(n % v)); } break;
		default: if (w) { int32_t n = (int32_t)(((uint32_t)c->dx << 16) | c->ax); if (!(int16_t)v) { do_interrupt(c, 0); break; } c->ax = (uint16_t)(n / (int16_t)v); c->dx = (uint16_t)(n % (int16_t)v); } else { int16_t n = (int16_t)c->ax; if (!(int8_t)v) { do_interrupt(c, 0); break; } set_reg8(c, 0, (uint8_t)(n / (int8_t)v)); set_reg8(c, 4, (uint8_t)(n % (int8_t)v)); } break;
		}
		return 0;
	}
	case 0xF8: setf(c, CF, 0); return 0;
	case 0xF9: setf(c, CF, 1); return 0;
	case 0xFA: setf(c, IF, 0); return 0;
	case 0xFB: setf(c, IF, 1); return 0;
	case 0xFC: setf(c, DF, 0); return 0;
	case 0xFD: setf(c, DF, 1); return 0;
	case 0xFE: case 0xFF: {
		int w = op & 1; uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); int sub = (modrm >> 3) & 7;
		if (sub < 2) { set_op(c, &o, w, incdec(c, get_op(c, &o, w), sub, w)); return 0; }
		if (!w) return -1;
		switch (sub) {
		case 2: { uint16_t t = get_op(c, &o, 1); push(c, c->ip); c->ip = t; return 0; }
		case 3: { uint16_t off = rd16(c, o.seg, o.off), seg = rd16(c, o.seg, (uint16_t)(o.off + 2)); push(c, c->cs); push(c, c->ip); c->ip = off; c->cs = seg; return 0; }
		case 4: c->ip = get_op(c, &o, 1); return 0;
		case 5: { uint16_t off = rd16(c, o.seg, o.off), seg = rd16(c, o.seg, (uint16_t)(o.off + 2)); c->ip = off; c->cs = seg; return 0; }
		case 6: { uint16_t v = get_op(c, &o, 1); push(c, v); return 0; }
		default: return -1;
		}
	}
	default: break;
	}
	if (op >= 0x40 && op <= 0x47) { uint16_t *r = reg16p(c, op & 7); *r = incdec(c, *r, 0, 1); return 0; }
	if (op >= 0x48 && op <= 0x4F) { uint16_t *r = reg16p(c, op & 7); *r = incdec(c, *r, 1, 1); return 0; }
	if (op >= 0x50 && op <= 0x57) { push(c, *reg16p(c, op & 7)); return 0; }
	if (op >= 0x58 && op <= 0x5F) { *reg16p(c, op & 7) = pop(c); return 0; }
	if (op >= 0x70 && op <= 0x7F) { int8_t d = (int8_t)fetch8(c); if (cond(c, op & 15)) c->ip += d; return 0; }
	if (op >= 0x80 && op <= 0x83) {
		int w = op & 1; uint8_t modrm = fetch8(c); Operand o = decode_rm(c, modrm, &p); int aop = (modrm >> 3) & 7;
		uint16_t imm = op == 0x81 ? fetch16(c) : (op == 0x83 ? (uint16_t)(int16_t)(int8_t)fetch8(c) : fetch8(c));
		uint16_t r = alu(c, aop, get_op(c, &o, w), imm, w);
		if (aop != 7) set_op(c, &o, w, r);
		return 0;
	}
	if (op >= 0x91 && op <= 0x97) { uint16_t *r = reg16p(c, op & 7); uint16_t t = c->ax; c->ax = *r; *r = t; return 0; }
	if (op >= 0xB0 && op <= 0xB7) { set_reg8(c, op & 7, fetch8(c)); return 0; }
	if (op >= 0xB8 && op <= 0xBF) { *reg16p(c, op & 7) = fetch16(c); return 0; }
	if (op >= 0xD8 && op <= 0xDF) { uint8_t modrm = fetch8(c); decode_rm(c, modrm, &p); return 0; } /* x87 escape: skipped */
	return -1;
}

int x86_run(X86 *c, uint16_t stop_cs, uint16_t stop_ip, unsigned long max_steps) {
	unsigned long n = 0;
	while (!(c->cs == stop_cs && c->ip == stop_ip)) {
		if (c->halted || ++n > max_steps) return -1;
		uint16_t cs = c->cs, ip = c->ip;
		if (step(c)) { fprintf(stderr, "x86: unknown opcode at %04X:%04X (%02X)\n", cs, ip, rd8(c, cs, ip)); return -1; }
	}
	return 0;
}

int x86_call_far(X86 *c, uint16_t seg, uint16_t off, const uint16_t *args, int nargs, unsigned long max_steps) {
	const uint16_t stop_cs = 0xF000, stop_ip = 0xFF00;
	for (int i = nargs - 1; i >= 0; i--) push(c, args[i]);
	push(c, stop_cs); push(c, stop_ip);
	c->cs = seg; c->ip = off;
	int rc = x86_run(c, stop_cs, stop_ip, max_steps);
	c->sp += (uint16_t)(2 * nargs);
	return rc;
}
