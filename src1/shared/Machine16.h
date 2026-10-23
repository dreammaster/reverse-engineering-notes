/* Shared::Machine16 -- the environment the programs that were translated from the disassembly run in (src1/tools/asm2cpp.py generates
 * their code). It is not a CPU emulator: the generated code is straight C++, one function per original routine, but it keeps the
 * original's register file, flags, stack and segment:offset addressing so that every routine behaves exactly like the original
 * (the register-based calling conventions, flag results and table-driven jumps need that). What the machine provides:
 *   - the register file with the 8086's flag semantics (Regs, and the alu helpers below),
 *   - memory: the program's data segment (a Shared::DataSegment loaded from the exe), the stack, the PSP, blocks allocated with
 *     DOS function 48h, and the video driver's context block (segment `kSegCtx`), which maps onto Vid::Context,
 *   - the interrupts the programs use: 62h (the video driver), 21h (DOS: files, memory, EXEC, keyboard, time), 1Ah, 10h, 33h, and
 *     the ports for the speaker.
 * Programs are started with a parameter block like the original's command tail (Params), the inherited file handles of which are
 * numbered by their offset in the block (0x5C party file, 0x5E.. tile files, ...) and resolved to files by the machine. */

#ifndef YENDOR1_SHARED_MACHINE16_H
#define YENDOR1_SHARED_MACHINE16_H

#include "data/ExeImage.h"
#include "shared/DataSegment.h"
#include "shared/Launcher.h"
#include "shared/Params.h"
#include "shared/Runtime.h"
#include "shared/Types.h"

#include <stdio.h>
#include <string>
#include <type_traits>
#include <vector>

namespace Yendor1 {
namespace Shared {

struct Regs {
	union { uint16_t ax; struct { uint8_t al, ah; }; };
	union { uint16_t bx; struct { uint8_t bl, bh; }; };
	union { uint16_t cx; struct { uint8_t cl, ch; }; };
	union { uint16_t dx; struct { uint8_t dl, dh; }; };
	uint16_t si, di, bp, sp;
	uint16_t es, ds, ss;
	bool cf, zf, sf, of, pf, af, df;
	Regs() : ax(0), bx(0), cx(0), dx(0), si(0), di(0), bp(0), sp(0), es(0), ds(0), ss(0), cf(false), zf(false), sf(false), of(false), pf(false), af(false), df(false) {}
};

/** A DOS program ended (int 21h / 4Ch). */
struct ProgramExit {
	int code;
	explicit ProgramExit(int c) : code(c) {}
};

// ---- flag-setting arithmetic of the 8086 ----
namespace alu {

inline bool parity(uint8_t v) {
	v ^= v >> 4;
	v ^= v >> 2;
	v ^= v >> 1;
	return !(v & 1);
}

template <typename T> struct Bits;
template <> struct Bits<uint8_t> { enum { n = 8, msb = 0x80 }; };
template <> struct Bits<uint16_t> { enum { n = 16, msb = 0x8000 }; };

template <typename T> inline void szp(Regs &r, T res) {
	r.zf = res == 0;
	r.sf = (res & Bits<T>::msb) != 0;
	r.pf = parity((uint8_t)res);
}

template <typename T> inline T add(Regs &r, T a, T b, unsigned carry = 0) {
	const uint32_t sum = (uint32_t)a + b + carry;
	const T res = (T)sum;
	r.cf = sum > (uint32_t)((Bits<T>::msb << 1) - 1);
	r.af = (((a & 0x0F) + (b & 0x0F) + carry) & 0x10) != 0;
	r.of = (((a ^ res) & (b ^ res)) & Bits<T>::msb) != 0;
	szp(r, res);
	return res;
}

template <typename T> inline T sub(Regs &r, T a, T b, unsigned borrow = 0) {
	const uint32_t diff = (uint32_t)a - b - borrow;
	const T res = (T)diff;
	r.cf = (uint32_t)a < (uint32_t)b + borrow;
	r.af = ((a & 0x0F) < (unsigned)((b & 0x0F) + borrow));
	r.of = (((a ^ b) & (a ^ res)) & Bits<T>::msb) != 0;
	szp(r, res);
	return res;
}

template <typename T> inline T logic(Regs &r, T res) {
	r.cf = false;
	r.of = false;
	r.af = false;
	szp(r, res);
	return res;
}

template <typename T> inline T inc(Regs &r, T a) {
	const bool cf = r.cf;
	const T res = add<T>(r, a, 1);
	r.cf = cf;
	return res;
}

template <typename T> inline T dec(Regs &r, T a) {
	const bool cf = r.cf;
	const T res = sub<T>(r, a, 1);
	r.cf = cf;
	return res;
}

template <typename T> inline T neg(Regs &r, T a) {
	const T res = sub<T>(r, 0, a);
	r.cf = a != 0;
	return res;
}

template <typename T> inline T shl(Regs &r, T a, unsigned n) {
	n &= 0x1F;
	if (n == 0)
		return a;
	uint32_t v = a;
	v <<= n;
	const T res = (T)v;
	r.cf = (n <= (unsigned)Bits<T>::n) ? ((a >> (Bits<T>::n - n)) & 1) != 0 : false;
	r.of = (((res & Bits<T>::msb) != 0) != r.cf);
	szp(r, res);
	return res;
}

template <typename T> inline T shr(Regs &r, T a, unsigned n) {
	n &= 0x1F;
	if (n == 0)
		return a;
	const T res = (n >= (unsigned)Bits<T>::n) ? (T)0 : (T)(a >> n);
	r.cf = (n <= (unsigned)Bits<T>::n) ? ((a >> (n - 1)) & 1) != 0 : false;
	r.of = (a & Bits<T>::msb) != 0;
	szp(r, res);
	return res;
}

template <typename T> inline T sar(Regs &r, T a, unsigned n) {
	n &= 0x1F;
	if (n == 0)
		return a;
	typedef typename std::conditional<sizeof(T) == 1, int8_t, int16_t>::type S;
	const S s = (S)a;
	const T res = (T)(n >= (unsigned)Bits<T>::n ? (s < 0 ? -1 : 0) : (s >> n));
	r.cf = ((s >> (n >= (unsigned)Bits<T>::n ? Bits<T>::n - 1 : n - 1)) & 1) != 0;
	r.of = false;
	szp(r, res);
	return res;
}

template <typename T> inline T rol(Regs &r, T a, unsigned n) {
	n %= Bits<T>::n;
	const T res = n ? (T)((a << n) | (a >> (Bits<T>::n - n))) : a;
	r.cf = (res & 1) != 0;
	return res;
}

template <typename T> inline T ror(Regs &r, T a, unsigned n) {
	n %= Bits<T>::n;
	const T res = n ? (T)((a >> n) | (a << (Bits<T>::n - n))) : a;
	r.cf = (res & Bits<T>::msb) != 0;
	return res;
}


inline void mul8(Regs &r, uint8_t v) {
	r.ax = (uint16_t)((uint16_t)r.al * v);
	r.cf = r.of = r.ah != 0;
}

inline void mul16(Regs &r, uint16_t v) {
	const uint32_t p = (uint32_t)r.ax * v;
	r.ax = (uint16_t)p;
	r.dx = (uint16_t)(p >> 16);
	r.cf = r.of = r.dx != 0;
}

inline void imul8(Regs &r, uint8_t v) {
	r.ax = (uint16_t)(int16_t)((int8_t)r.al * (int8_t)v);
	r.cf = r.of = (int16_t)r.ax != (int8_t)r.al;
}

inline void imul16(Regs &r, uint16_t v) {
	const int32_t p = (int32_t)(int16_t)r.ax * (int16_t)v;
	r.ax = (uint16_t)p;
	r.dx = (uint16_t)((uint32_t)p >> 16);
	r.cf = r.of = p != (int16_t)r.ax;
}

inline void div8(Regs &r, uint8_t v) {
	if (v == 0)
		return;
	const uint16_t q = (uint16_t)(r.ax / v), rem = (uint16_t)(r.ax % v);
	r.al = (uint8_t)q;
	r.ah = (uint8_t)rem;
}

inline void div16(Regs &r, uint16_t v) {
	if (v == 0)
		return;
	const uint32_t n = ((uint32_t)r.dx << 16) | r.ax;
	r.ax = (uint16_t)(n / v);
	r.dx = (uint16_t)(n % v);
}

inline void idiv8(Regs &r, uint8_t v) {
	if (v == 0)
		return;
	const int16_t n = (int16_t)r.ax;
	r.al = (uint8_t)(n / (int8_t)v);
	r.ah = (uint8_t)(n % (int8_t)v);
}

inline void idiv16(Regs &r, uint16_t v) {
	if (v == 0)
		return;
	const int32_t n = (int32_t)(((uint32_t)r.dx << 16) | r.ax);
	r.ax = (uint16_t)(n / (int16_t)v);
	r.dx = (uint16_t)(n % (int16_t)v);
}

inline uint8_t daa(Regs &r, uint8_t al) {
	const uint8_t oldAl = al;
	const bool oldCf = r.cf;
	if ((al & 0x0F) > 9 || r.af) {
		const unsigned t = (unsigned)al + 6;
		r.cf = oldCf || t > 0xFF;
		al = (uint8_t)t;
		r.af = true;
	} else {
		r.af = false;
	}
	if (oldAl > 0x99 || oldCf) {
		al = (uint8_t)(al + 0x60);
		r.cf = true;
	} else {
		r.cf = false;
	}
	szp<uint8_t>(r, al);
	return al;
}

inline uint16_t getFlags(const Regs &r) {
	return (uint16_t)((r.cf ? 1 : 0) | (r.pf ? 4 : 0) | (r.af ? 0x10 : 0) | (r.zf ? 0x40 : 0) | (r.sf ? 0x80 : 0) | (r.df ? 0x400 : 0) | (r.of ? 0x800 : 0) | 0x2);
}

inline void setFlags(Regs &r, uint16_t f) {
	r.cf = f & 1; r.pf = f & 4; r.af = f & 0x10; r.zf = f & 0x40; r.sf = f & 0x80; r.df = f & 0x400; r.of = f & 0x800;
}

} // namespace alu

class Machine16 {
public:
	enum {
		kSegData = 0x1D60,      ///< the program's data segment (also what the original's CS-relative data accesses mean)
		kSegCtx = 0x2000,       ///< the video driver's context block
		kSegStack = 0x3000,
		kSegPsp = 0x3100,
		kSegParentPsp = 0x3200,
		kSegJft = 0x3300,
		kFirstBlock = 0x4000
	};

	Machine16(Runtime &rt, Launcher *launcher, const Params &params);
	~Machine16();

	/** Load the initial data segment from the program's exe; false when the file is missing. */
	bool loadProgram(const std::string &exeName, uint32_t dataSegmentImageOffset);

	Regs r;
	DataSegment ds;

	/** Reached a line of the original that tools/asm2cpp.py could not translate: report it and stop the program. */
	void unsupported(const char *what);

	// ---- memory ----
	uint8_t &b(uint16_t seg, uint16_t off);
	uint16_t rw(uint16_t seg, uint16_t off) { return (uint16_t)(b(seg, off) | (b(seg, (uint16_t)(off + 1)) << 8)); }
	void ww(uint16_t seg, uint16_t off, uint16_t v) { b(seg, off) = (uint8_t)v; b(seg, (uint16_t)(off + 1)) = (uint8_t)(v >> 8); }

	// ---- stack ----
	void push(uint16_t v) { r.sp = (uint16_t)(r.sp - 2); ww(r.ss, r.sp, v); }
	uint16_t pop() { const uint16_t v = rw(r.ss, r.sp); r.sp = (uint16_t)(r.sp + 2); return v; }

	// ---- interrupts, ports ----
	void intr(uint8_t n);
	void out8(uint16_t port, uint8_t v);
	uint8_t in8(uint16_t port);

	// ---- string instructions (the translator emits these for the movs/stos/lods/scas/cmps family) ----
	void movs(int size);
	void stos(int size);
	uint16_t lods(int size);
	void scas(int size);
	void cmps(int size);

	/** Called for `rep`: how many times a string instruction runs. */
	uint16_t repCount() const { return r.cx; }

	Runtime &rt;
	Launcher *launcher;
	Params params;

	/** The pseudo-segment for a block allocated with DOS function 48h, for tests. */
	uint8_t *blockPtr(uint16_t seg);

	/** How many bytes a program pushed beyond its initial stack pointer (for sanity checks). */

	// ---- tile operations (the tile cache is native: see Machine16.cpp) ----

	// ---- palette helper (VIDP16): the command mailbox ----
	void paletteCommand();                         ///< AL = command

	int lastExitCode;                              ///< of the last EXEC'ed child

private:
	friend class DosFiles;
	uint8_t &ctxByte(uint16_t off);
	void int62();
	void int21();
	void int1A();
	void int10();
	void int33();
	void bridgeImage();
	void unbridgeImage();
	uint8_t *dosPtr(uint16_t seg, uint16_t off, uint32_t length);
	void execChild();
	std::string asciz(uint16_t seg, uint16_t off);
	FILE *handleFile(uint16_t h);
	uint16_t openFile(const std::string &path);
	std::string resolvePath(const std::string &dosName) const;

	uint8_t _stack[65536];
	uint8_t _psp[256];
	uint8_t _parentPsp[256];
	uint8_t _jft[256];
	uint8_t _ctxExtra[0x1000];
	uint64_t _polls, _pollsAtCall;    // INT 21h/06h polls made so far, and when the last driver call was made
	uint16_t _tileTableSeg;
	unsigned _tileSlots;
	void initTileStore();
	uint8_t *_ctxMap[0x1000];
	std::vector<std::vector<uint8_t> > _blocks;
	std::vector<FILE *> _files;                    ///< handle - 0x100
	uint16_t _cursorX, _cursorY;
	uint8_t _dummy;
	uint16_t _speakerDivisor;
};

} // namespace Shared
} // namespace Yendor1

#endif
