/* Runtime for code translated mechanically from the original 16-bit program (tools/asm2c.py): a register file with
 * flags, the game's data segment (DGROUP) as a 64 KB array and a small stack array. */
#ifndef MM3_RECOMP_H
#define MM3_RECOMP_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	uint16_t ax, bx, cx, dx, si, di, bp, sp;
	uint16_t es, ds; /* segment registers: ds is the data segment unless a routine loads another one */
	int cf, zf, sf, of;
} Cpu;

#define DSEG 0x286F      /* DGROUP */

/* Machine memory: the 8086 address space, linear address = segment * 16 + offset.  The program image (root code, the 13 overlays and
 * the initialised data) lives at 10000h-52165h exactly where IDA has it; DGROUP is segment 286Fh and the stack segment 9000h. */
#define MEM_SIZE 0x120000
extern uint8_t MEM[MEM_SIZE + 0x10000]; /* 64 KB slack: a segment near the end can still be addressed with a 16-bit offset */
#define DG (MEM + 0x286F0)    /* DGROUP, segment 286Fh */
#ifdef RECOMP_STACK_IN_DG
/* the whole game: as in the original SS = DS, the stack lives at the top of DGROUP (near pointers to locals work with DS) */
#define STK DG
#define STACK_SEG DSEG
#define STACK_TOP 0xFFF0
#else
#define STK (MEM + 0x90000)   /* the program stack, segment 9000h (the verified view / rules bundles keep it apart) */
#define STACK_SEG 0x9000
#define STACK_TOP 0xFF00
#endif
#define SEGP(seg) (MEM + ((uint32_t)(seg) << 4))
/* read-only access to code-segment data (tables the compiler put after the code): segment constant + offset */
#define CS8(seg, a) rd8(SEGP(seg), a)
#define CS16(seg, a) rd16(SEGP(seg), a)

static inline uint8_t rd8(const uint8_t *m, uint16_t a) { return m[a]; }
static inline uint16_t rd16(const uint8_t *m, uint16_t a) { return (uint16_t)(m[a] | (m[(uint16_t)(a + 1)] << 8)); }
#ifdef RECOMP_TRACE
#define TRACE_STK(m, a, v) do { if ((m) == STK) fprintf(stderr, "W %04X %02X\n", (unsigned)(a), (unsigned)((v) & 0xFF)); } while (0)
#else
#define TRACE_STK(m, a, v) ((void)0)
#endif
static inline void wr8(uint8_t *m, uint16_t a, uint32_t v) { TRACE_STK(m, a, v); m[a] = (uint8_t)v; }
static inline void wr16(uint8_t *m, uint16_t a, uint32_t v) { TRACE_STK(m, a, v); TRACE_STK(m, (uint16_t)(a + 1), v >> 8); m[a] = (uint8_t)v; m[(uint16_t)(a + 1)] = (uint8_t)(v >> 8); }

/* Memory behind a segment value (far pointers): DGROUP, the stack, and buffers registered by the host (roster, loaded files ...). */
static inline uint8_t *recomp_seg_mem(uint16_t seg) { return SEGP(seg); }
#define ES8(c, a) rd8(recomp_seg_mem((c)->es), a)
#define ES16(c, a) rd16(recomp_seg_mem((c)->es), a)
#define ES8_SET(c, a, v) wr8(recomp_seg_mem((c)->es), a, v)
#define ES16_SET(c, a, v) wr16(recomp_seg_mem((c)->es), a, v)

#define DG8(a) rd8(DG, a)
#define DG16(a) rd16(DG, a)
#define DG8_SET(a, v) wr8(DG, a, v)
#define DG16_SET(a, v) wr16(DG, a, v)
#define ST8(a) rd8(STK, a)
#define ST16(a) rd16(STK, a)
#define ST8_SET(a, v) wr8(STK, a, v)
#define ST16_SET(a, v) wr16(STK, a, v)

static inline void PUSH(Cpu *c, uint32_t v) { c->sp = (uint16_t)(c->sp - 2); wr16(STK, c->sp, v); }
static inline uint16_t POP(Cpu *c) { uint16_t v = rd16(STK, c->sp); c->sp = (uint16_t)(c->sp + 2); return v; }

/* n-th cdecl word argument of a routine, seen from inside a host_ function (no return address is pushed for host routines) */
static inline uint16_t host_arg(const Cpu *c, int n) { return rd16(STK, (uint16_t)(c->sp + 2 * n)); }

static inline uint32_t msk(int bits) { return bits == 8 ? 0xFFu : 0xFFFFu; }
static inline void set_zs(Cpu *c, uint32_t r, int bits) { c->zf = (r & msk(bits)) == 0; c->sf = (r >> (bits - 1)) & 1; }

static inline uint32_t alu_add(Cpu *c, uint32_t a, uint32_t b, int bits) {
	uint32_t r = (a & msk(bits)) + (b & msk(bits));
	c->cf = r > msk(bits);
	c->of = (~(a ^ b) & (a ^ r) & (1u << (bits - 1))) != 0;
	set_zs(c, r, bits);
	return r & msk(bits);
}
static inline uint32_t alu_adc(Cpu *c, uint32_t a, uint32_t b, int bits) {
	uint32_t cin = c->cf, r = (a & msk(bits)) + (b & msk(bits)) + cin;
	c->cf = r > msk(bits);
	c->of = (~(a ^ b) & (a ^ r) & (1u << (bits - 1))) != 0;
	set_zs(c, r, bits);
	return r & msk(bits);
}
static inline uint32_t alu_sub(Cpu *c, uint32_t a, uint32_t b, int bits) {
	uint32_t r = (a & msk(bits)) - (b & msk(bits));
	c->cf = (a & msk(bits)) < (b & msk(bits));
	c->of = ((a ^ b) & (a ^ r) & (1u << (bits - 1))) != 0;
	set_zs(c, r, bits);
	return r & msk(bits);
}
static inline uint32_t alu_sbb(Cpu *c, uint32_t a, uint32_t b, int bits) {
	uint32_t cin = c->cf, r = (a & msk(bits)) - (b & msk(bits)) - cin;
	c->cf = (a & msk(bits)) < (b & msk(bits)) + cin;
	c->of = ((a ^ b) & (a ^ r) & (1u << (bits - 1))) != 0;
	set_zs(c, r, bits);
	return r & msk(bits);
}
static inline uint32_t alu_and(Cpu *c, uint32_t a, uint32_t b, int bits) { uint32_t r = a & b & msk(bits); c->cf = c->of = 0; set_zs(c, r, bits); return r; }
static inline uint32_t alu_or(Cpu *c, uint32_t a, uint32_t b, int bits) { uint32_t r = (a | b) & msk(bits); c->cf = c->of = 0; set_zs(c, r, bits); return r; }
static inline uint32_t alu_xor(Cpu *c, uint32_t a, uint32_t b, int bits) { uint32_t r = (a ^ b) & msk(bits); c->cf = c->of = 0; set_zs(c, r, bits); return r; }
static inline uint32_t alu_inc(Cpu *c, uint32_t a, int bits) {
	uint32_t r = (a + 1) & msk(bits);
	c->of = (r == (1u << (bits - 1)));
	set_zs(c, r, bits);
	return r;
}
static inline uint32_t alu_dec(Cpu *c, uint32_t a, int bits) {
	uint32_t r = (a - 1) & msk(bits);
	c->of = ((a & msk(bits)) == (1u << (bits - 1)));
	set_zs(c, r, bits);
	return r;
}
static inline uint32_t alu_shl(Cpu *c, uint32_t a, unsigned n, int bits) {
	uint32_t r;
	n &= 31;
	if (!n) return a & msk(bits);
	r = (n >= (unsigned)bits) ? 0 : (a << n) & msk(bits);
	c->cf = n <= (unsigned)bits ? ((a >> (bits - n)) & 1) : 0;
	c->of = (int)((r >> (bits - 1)) & 1) != c->cf;
	set_zs(c, r, bits);
	return r;
}
static inline uint32_t alu_shr(Cpu *c, uint32_t a, unsigned n, int bits) {
	uint32_t r;
	n &= 31;
	if (!n) return a & msk(bits);
	a &= msk(bits);
	r = n >= (unsigned)bits ? 0 : a >> n;
	c->cf = n <= (unsigned)bits ? ((a >> (n - 1)) & 1) : 0;
	c->of = (a >> (bits - 1)) & 1;
	set_zs(c, r, bits);
	return r;
}
static inline uint32_t alu_sar(Cpu *c, uint32_t a, unsigned n, int bits) {
	int32_t v = bits == 8 ? (int8_t)a : (int16_t)a;
	uint32_t r;
	n &= 31;
	if (!n) return a & msk(bits);
	if (n >= (unsigned)bits) n = bits - 1 + 1;
	c->cf = (uint32_t)(v >> (n - 1)) & 1;
	r = (uint32_t)(v >> (n > 31 ? 31 : n)) & msk(bits);
	c->of = 0;
	set_zs(c, r, bits);
	return r;
}

static inline uint32_t alu_rcl(Cpu *c, uint32_t a, unsigned n, int bits) {
	a &= msk(bits);
	for (n &= 31; n; n--) { uint32_t out = (a >> (bits - 1)) & 1; a = ((a << 1) | (uint32_t)c->cf) & msk(bits); c->cf = (int)out; }
	return a;
}
static inline uint32_t alu_rcr(Cpu *c, uint32_t a, unsigned n, int bits) {
	a &= msk(bits);
	for (n &= 31; n; n--) { uint32_t out = a & 1; a = (a >> 1) | ((uint32_t)c->cf << (bits - 1)); c->cf = (int)out; }
	return a;
}
static inline uint32_t alu_rol(Cpu *c, uint32_t a, unsigned n, int bits) {
	a &= msk(bits);
	for (n &= 31; n; n--) { uint32_t out = (a >> (bits - 1)) & 1; a = ((a << 1) | out) & msk(bits); c->cf = (int)out; }
	return a;
}
static inline uint32_t alu_ror(Cpu *c, uint32_t a, unsigned n, int bits) {
	a &= msk(bits);
	for (n &= 31; n; n--) { uint32_t out = a & 1; a = (a >> 1) | (out << (bits - 1)); c->cf = (int)out; }
	return a;
}

#ifdef RECOMP_TRACE_CALLS
#define FNTRACE(name) fprintf(stderr, "> %s\n", name)
#else
#define FNTRACE(name) ((void)0)
#endif

#ifdef RECOMP_TRACE
#include <stdio.h>
/* debugging aid: register state at every label, to diff against the emulator (tools/mm3_trace_diff.py) */
#define RTRACE(c, name) fprintf(stderr, "%s %04X %04X %04X %04X %04X %04X\n", name, (c)->ax, (c)->bx, (c)->cx, (c)->dx, (c)->si, (c)->di)
#else
#define RTRACE(c, name) ((void)0)
#endif

/* a host routine that has not been written yet (see the weak definitions in generated code) */
void recomp_bad_jump(const char *fn, unsigned seg, unsigned off); /* computed jump to a target that has no label */
void recomp_unimplemented(const char *name, Cpu *c);

typedef struct { const char *name; void (*fn)(Cpu *); } RecompEntry;

#endif
