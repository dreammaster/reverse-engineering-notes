// Minimal 8086 register/flag/memory model for the lifted handlers (generated code in lifted_handlers.cpp, see reversing/scripts/lift_handlers.py).
// The host (the engine) provides the 1 MB memory image, fills ds/es/ss with real paragraphs, and implements the H_* functions declared in lifted_api.h:
// a host function reads its cdecl arguments from the emulated stack (c.arg(n) = the n-th word above the return address) and returns in ax / dx:ax.
#pragma once
#include <cstdint>
#include <cstring>

struct Reg { union { uint16_t x; struct { uint8_t l, h; }; }; };

// thrown by longjmp; the function that called setjmp for this buffer catches it (generated code) and resumes after its _setjmp call
struct LongJmp { uint16_t seg, off, val; };

struct Cpu {
	Reg ax, bx, cx, dx;
	uint16_t si = 0, di = 0, bp = 0, sp = 0, es = 0, ds = 0, ss = 0, cs = 0;
	bool CF = false, ZF = false, SF = false, OF = false, DF = false;
	uint8_t *mem = nullptr;                  // 1 MB flat image, address = (seg << 4) + off (mod 1 MB)
	const char *hist[64] = {nullptr};        // ring of the last labels passed (diagnostics)
	unsigned hpos = 0;
	void (*irqHook)(struct Cpu &) = nullptr;                 // called every 1024 labels: the host delivers hardware interrupts (timer) here
	void (*onLabel)(struct Cpu &, const char *) = nullptr;   // optional per-label probe (diagnostics)
	const char *where = "";                // last label passed (diagnostics for runaway loops)
	int popBytes = 4;                        // bytes the last return popped (return address + `ret N` argument bytes)
	long steps = 0;                          // instruction-budget counter, see STEP()
	int retWords = 2;                        // words pushed as a return address by the current call (1 near, 2 far)

	static uint32_t lin(uint16_t seg, uint16_t off) { return (((uint32_t)seg << 4) + off) & 0x1FFFFF; }
	uint8_t rd8(uint16_t s, uint16_t o) const { return mem[lin(s, o)]; }
	uint16_t rd16(uint16_t s, uint16_t o) const { return (uint16_t)(mem[lin(s, o)] | (mem[lin(s, o + 1)] << 8)); }
	void wr8(uint16_t s, uint16_t o, uint8_t v) { mem[lin(s, o)] = v; }
	void wr16(uint16_t s, uint16_t o, uint16_t v) { mem[lin(s, o)] = (uint8_t)v; mem[lin(s, o + 1)] = (uint8_t)(v >> 8); }
	void push(uint16_t v) { sp -= 2; wr16(ss, sp, v); }
	uint16_t pop() { uint16_t v = rd16(ss, sp); sp += 2; return v; }
	// host-side access to the arguments of the function being called (call site has pushed retWords return words)
	uint16_t arg(int n) const { return rd16(ss, (uint16_t)(sp + 2 * (retWords + n))); }

	// computed far/near calls (call dword ptr [..]); the host maps seg:off to a function
	void (*callFarHook)(Cpu &, uint16_t seg, uint16_t off) = nullptr;
	// optional tracing: called before every direct call with the callee's original name (arguments readable with arg(n))
	void (*onCall)(Cpu &, const char *name) = nullptr;
	// machine interface: software interrupts (DOS, BIOS, mouse, EMS), port input/output; the host emulates the PC under the original program
	void (*onInt)(Cpu &, int number) = nullptr;
	uint16_t (*onIn)(Cpu &, uint16_t port, int bits) = nullptr;
	void (*onOut)(Cpu &, uint16_t port, uint16_t value, int bits) = nullptr;
};

template <class T> struct Sz;
template <> struct Sz<uint8_t> { typedef int8_t S; static const int bits = 8; };
template <> struct Sz<uint16_t> { typedef int16_t S; static const int bits = 16; };

template <class T> inline void setZS(Cpu &c, T r) { c.ZF = r == 0; c.SF = (r >> (Sz<T>::bits - 1)) & 1; }

template <class T> inline T opAdd(Cpu &c, T a, T b, bool cin = false) {
	uint32_t w = (uint32_t)a + b + (cin ? 1 : 0);
	T r = (T)w;
	c.CF = (w >> Sz<T>::bits) & 1;
	c.OF = ((~(a ^ b) & (a ^ r)) >> (Sz<T>::bits - 1)) & 1;
	setZS(c, r);
	return r;
}
template <class T> inline T opSub(Cpu &c, T a, T b, bool bin = false) {
	uint32_t w = (uint32_t)a - b - (bin ? 1 : 0);
	T r = (T)w;
	c.CF = (w >> Sz<T>::bits) & 1;
	c.OF = (((a ^ b) & (a ^ r)) >> (Sz<T>::bits - 1)) & 1;
	setZS(c, r);
	return r;
}
template <class T> inline T opLogic(Cpu &c, T r) { c.CF = false; c.OF = false; setZS(c, r); return r; }
template <class T> inline T opInc(Cpu &c, T a) { bool cf = c.CF; T r = opAdd<T>(c, a, 1); c.CF = cf; return r; }
template <class T> inline T opDec(Cpu &c, T a) { bool cf = c.CF; T r = opSub<T>(c, a, 1); c.CF = cf; return r; }
template <class T> inline T opNeg(Cpu &c, T a) { T r = opSub<T>(c, 0, a); c.CF = a != 0; return r; }
template <class T> inline T opShl(Cpu &c, T a, int n) {
	n &= 31;
	if (!n) return a;
	uint32_t w = n >= 32 ? 0 : ((uint32_t)a << n);
	T r = (T)w;
	c.CF = (w >> Sz<T>::bits) & 1;
	c.OF = (((r >> (Sz<T>::bits - 1)) & 1) != (T)c.CF);
	setZS(c, r);
	return r;
}
template <class T> inline T opShr(Cpu &c, T a, int n) {
	n &= 31;
	if (!n) return a;
	c.CF = n > Sz<T>::bits ? false : (((uint32_t)a >> (n - 1)) & 1);
	T r = (T)((uint32_t)a >> n);
	c.OF = (a >> (Sz<T>::bits - 1)) & 1;
	setZS(c, r);
	return r;
}
template <class T> inline T opSar(Cpu &c, T a, int n) {
	n &= 31;
	if (!n) return a;
	typename Sz<T>::S s = (typename Sz<T>::S)a;
	c.CF = ((int32_t)s >> (n - 1)) & 1;
	T r = (T)((int32_t)s >> n);
	c.OF = false;
	setZS(c, r);
	return r;
}
template <class T> inline T opRol(Cpu &c, T a, int n) {
	n %= Sz<T>::bits;
	if (!n) return a;
	T r = (T)((a << n) | (a >> (Sz<T>::bits - n)));
	c.CF = r & 1;
	return r;
}
template <class T> inline T opRor(Cpu &c, T a, int n) {
	n %= Sz<T>::bits;
	if (!n) return a;
	T r = (T)((a >> n) | (a << (Sz<T>::bits - n)));
	c.CF = (r >> (Sz<T>::bits - 1)) & 1;
	return r;
}
inline void unimpl(const char *) {}
// decimal adjust AL after addition (auxiliary carry derived from the low nibble)
inline void opDaa(Cpu &c) {
	uint8_t al = c.ax.l;
	bool oldCf = c.CF;
	if ((al & 0x0F) > 9) { al = (uint8_t)(al + 6); }
	if (oldCf || c.ax.l > 0x99) { al = (uint8_t)(al + 0x60); c.CF = true; } else { c.CF = false; }
	c.ax.l = al;
	setZS(c, al);
}

// STEP(label) is executed at every label of the lifted code: it counts labels (for the timer interrupt below) and, when LIFT_STEP_LIMIT is defined, abandons runaway
// loops and keeps a ring of the last labels for diagnostics
#ifdef LIFT_STEP_LIMIT
#define STEP(w_) do { if (++c.steps > LIFT_STEP_LIMIT) return; c.where = (w_); c.hist[c.hpos++ & 63] = (w_); if (c.onLabel) c.onLabel(c, (w_)); \
	if (c.irqHook && !(c.steps & 0x3FF)) c.irqHook(c); } while (0)
#else
#define STEP(w_) do { ++c.steps; if (c.irqHook && !(c.steps & 0x3FF)) c.irqHook(c); } while (0)
#endif

#define SGN(c) ((int16_t)(c))

// rotate through carry
template <class T> inline T opRcl(Cpu &c, T a, int n) {
	n &= 31;
	for (int i = 0; i < n; i++) {
		bool top = (a >> (Sz<T>::bits - 1)) & 1;
		a = (T)((a << 1) | (c.CF ? 1 : 0));
		c.CF = top;
	}
	return a;
}
template <class T> inline T opRcr(Cpu &c, T a, int n) {
	n &= 31;
	for (int i = 0; i < n; i++) {
		bool low = a & 1;
		a = (T)((a >> 1) | ((T)(c.CF ? 1 : 0) << (Sz<T>::bits - 1)));
		c.CF = low;
	}
	return a;
}
