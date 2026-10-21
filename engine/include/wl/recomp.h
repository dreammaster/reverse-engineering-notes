#pragma once
// Runtime support for the statically recompiled WL.EXE code (see tools/recomp/recomp.py).
//
// The generated functions work on a Regs set and the Machine memory image exactly like the original 8086 code does:
// registers are variables, flags are booleans, the stack lives in the machine's stack segment and calls push their
// return address. Only the control flow is native C++.
#include "wl/machine.h"
#include <cstdint>
#include <stdexcept>
#include <string>

namespace wl {
namespace rc {

union Reg16 {
	uint16_t x;
	struct { uint8_t l, h; } b;
};

struct Regs {
	Reg16 a, b, c, d;        // AX BX CX DX
	uint16_t si = 0, di = 0, bp = 0, sp = 0;
	uint16_t es = 0, cs = 0x1000, ss = 0, ds = 0;
	bool cf = false, pf = false, af = false, zf = false, sf = false, of = false, df = false, ifl = true, tf = false;
	Regs() { a.x = b.x = c.x = d.x = 0; }

	uint16_t flags() const {
		return uint16_t(0x0002 | (cf ? 1 : 0) | (pf ? 4 : 0) | (af ? 0x10 : 0) | (zf ? 0x40 : 0) | (sf ? 0x80 : 0) | (tf ? 0x100 : 0) | (ifl ? 0x200 : 0) | (df ? 0x400 : 0) | (of ? 0x800 : 0));
	}
	void setFlags(uint16_t f) {
		cf = f & 1; pf = f & 4; af = f & 0x10; zf = f & 0x40; sf = f & 0x80; tf = f & 0x100; ifl = f & 0x200; df = f & 0x400; of = f & 0x800;
	}
};

struct Fault : std::runtime_error {
	using std::runtime_error::runtime_error;
};

// ---- memory ---------------------------------------------------------------------------------------------------------
inline uint32_t lin(uint16_t seg, uint16_t off) { return ((uint32_t(seg) << 4) + off) & 0xFFFFF; }

inline uint8_t rb(Machine &m, uint16_t seg, uint16_t off) {
	const uint32_t a = lin(seg, off);
	if (a >= 0xA0000 && a < 0xB0000) return m.ega.read(a);
	return m.mem[a];
}
inline uint16_t rw(Machine &m, uint16_t seg, uint16_t off) { return uint16_t(rb(m, seg, off) | (rb(m, seg, uint16_t(off + 1)) << 8)); }
inline void wb(Machine &m, uint16_t seg, uint16_t off, uint8_t v) {
	const uint32_t a = lin(seg, off);
	if (a >= 0xA0000 && a < 0xB0000) { m.ega.write(a, v); return; }
	m.mem[a] = v;
}
inline void ww(Machine &m, uint16_t seg, uint16_t off, uint16_t v) { wb(m, seg, off, uint8_t(v)); wb(m, seg, uint16_t(off + 1), uint8_t(v >> 8)); }

inline void push(Machine &m, Regs &r, uint16_t v) { r.sp = uint16_t(r.sp - 2); ww(m, r.ss, r.sp, v); }
inline uint16_t pop(Machine &m, Regs &r) { const uint16_t v = rw(m, r.ss, r.sp); r.sp = uint16_t(r.sp + 2); return v; }

// ---- ALU ------------------------------------------------------------------------------------------------------------
template <typename T> inline void szp(Regs &r, T v) {
	constexpr unsigned bits = sizeof(T) * 8;
	r.zf = v == 0;
	r.sf = (v >> (bits - 1)) & 1;
	uint8_t p = uint8_t(v);
	p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
	r.pf = !(p & 1);
}

template <typename T> inline T add_(Regs &r, T a, T b, unsigned c = 0) {
	constexpr unsigned bits = sizeof(T) * 8;
	const uint32_t full = uint32_t(a) + b + c;
	const T res = T(full);
	r.cf = (full >> bits) & 1;
	r.af = ((a ^ b ^ res) & 0x10) != 0;
	r.of = (((a ^ res) & (b ^ res)) >> (bits - 1)) & 1;
	szp(r, res);
	return res;
}
template <typename T> inline T sub_(Regs &r, T a, T b, unsigned c = 0) {
	constexpr unsigned bits = sizeof(T) * 8;
	const uint32_t full = uint32_t(a) - b - c;
	const T res = T(full);
	r.cf = uint32_t(a) < uint32_t(b) + c;
	r.af = ((a ^ b ^ res) & 0x10) != 0;
	r.of = (((a ^ b) & (a ^ res)) >> (bits - 1)) & 1;
	szp(r, res);
	return res;
}
template <typename T> inline T and_(Regs &r, T a, T b) { const T res = T(a & b); r.cf = r.of = r.af = false; szp(r, res); return res; }
template <typename T> inline T or_(Regs &r, T a, T b) { const T res = T(a | b); r.cf = r.of = r.af = false; szp(r, res); return res; }
template <typename T> inline T xor_(Regs &r, T a, T b) { const T res = T(a ^ b); r.cf = r.of = r.af = false; szp(r, res); return res; }
template <typename T> inline T inc_(Regs &r, T a) { const bool c = r.cf; const T res = add_<T>(r, a, 1); r.cf = c; return res; }
template <typename T> inline T dec_(Regs &r, T a) { const bool c = r.cf; const T res = sub_<T>(r, a, 1); r.cf = c; return res; }
template <typename T> inline T neg_(Regs &r, T a) { const T res = sub_<T>(r, 0, a); r.cf = a != 0; return res; }

template <typename T> inline T shl_(Regs &r, T v, unsigned n) {
	constexpr unsigned bits = sizeof(T) * 8;
	for (unsigned i = 0; i < n; ++i) {
		const bool top = (v >> (bits - 1)) & 1;
		v = T(v << 1);
		r.cf = top;
		r.of = (((v >> (bits - 1)) & 1) != 0) != top;
		szp(r, v);
	}
	return v;
}
template <typename T> inline T shr_(Regs &r, T v, unsigned n) {
	constexpr unsigned bits = sizeof(T) * 8;
	for (unsigned i = 0; i < n; ++i) {
		r.of = (v >> (bits - 1)) & 1;
		r.cf = v & 1;
		v = T(v >> 1);
		szp(r, v);
	}
	return v;
}
template <typename T> inline T sar_(Regs &r, T v, unsigned n) {
	constexpr unsigned bits = sizeof(T) * 8;
	for (unsigned i = 0; i < n; ++i) {
		const T top = T(v & (T(1) << (bits - 1)));
		r.cf = v & 1;
		v = T((v >> 1) | top);
		r.of = false;
		szp(r, v);
	}
	return v;
}
template <typename T> inline T rol_(Regs &r, T v, unsigned n) {
	constexpr unsigned bits = sizeof(T) * 8;
	for (unsigned i = 0; i < n; ++i) {
		const bool top = (v >> (bits - 1)) & 1;
		v = T((v << 1) | top);
		r.cf = top;
		r.of = (((v >> (bits - 1)) & 1) != 0) != top;
	}
	return v;
}
template <typename T> inline T ror_(Regs &r, T v, unsigned n) {
	constexpr unsigned bits = sizeof(T) * 8;
	for (unsigned i = 0; i < n; ++i) {
		const bool low = v & 1;
		v = T((v >> 1) | (T(low) << (bits - 1)));
		r.cf = low;
		r.of = (((v >> (bits - 1)) ^ (v >> (bits - 2))) & 1) != 0;
	}
	return v;
}
template <typename T> inline T rcl_(Regs &r, T v, unsigned n) {
	constexpr unsigned bits = sizeof(T) * 8;
	for (unsigned i = 0; i < n; ++i) {
		const bool top = (v >> (bits - 1)) & 1;
		v = T((v << 1) | (r.cf ? 1 : 0));
		r.cf = top;
		r.of = (((v >> (bits - 1)) & 1) != 0) != top;
	}
	return v;
}
template <typename T> inline T rcr_(Regs &r, T v, unsigned n) {
	constexpr unsigned bits = sizeof(T) * 8;
	for (unsigned i = 0; i < n; ++i) {
		const bool low = v & 1;
		v = T((v >> 1) | (T(r.cf ? 1 : 0) << (bits - 1)));
		r.cf = low;
		r.of = (((v >> (bits - 1)) ^ (v >> (bits - 2))) & 1) != 0;
	}
	return v;
}

inline void mul8(Regs &r, uint8_t v) { const uint16_t p = uint16_t(r.a.b.l) * v; r.a.x = p; r.cf = r.of = (p >> 8) != 0; }
inline void mul16(Regs &r, uint16_t v) { const uint32_t p = uint32_t(r.a.x) * v; r.a.x = uint16_t(p); r.d.x = uint16_t(p >> 16); r.cf = r.of = r.d.x != 0; }
inline void imul8(Regs &r, uint8_t v) { const int32_t p = int32_t(int8_t(r.a.b.l)) * int8_t(v); r.a.x = uint16_t(p); r.cf = r.of = p != int8_t(p); }
inline void imul16(Regs &r, uint16_t v) { const int32_t p = int32_t(int16_t(r.a.x)) * int16_t(v); r.a.x = uint16_t(p); r.d.x = uint16_t(uint32_t(p) >> 16); r.cf = r.of = p != int16_t(p); }
inline void div8(Regs &r, uint8_t v) {
	const uint32_t n = r.a.x;
	if (v == 0 || n / v > 0xFF) throw Fault("divide error");
	r.a.x = uint16_t(((n % v) << 8) | (n / v));
}
inline void div16(Regs &r, uint16_t v) {
	const uint32_t n = (uint32_t(r.d.x) << 16) | r.a.x;
	if (v == 0 || n / v > 0xFFFF) throw Fault("divide error");
	r.a.x = uint16_t(n / v); r.d.x = uint16_t(n % v);
}
inline void idiv8(Regs &r, uint8_t v) {
	const int32_t d = int8_t(v), n = int16_t(r.a.x);
	if (d == 0) throw Fault("divide error");
	r.a.x = uint16_t((uint8_t(n % d) << 8) | uint8_t(n / d));
}
inline void idiv16(Regs &r, uint16_t v) {
	const int32_t d = int16_t(v), n = int32_t((uint32_t(r.d.x) << 16) | r.a.x);
	if (d == 0) throw Fault("divide error");
	r.a.x = uint16_t(n / d); r.d.x = uint16_t(n % d);
}

// ---- string instructions (rep count handled by the caller through cx) -------------------------------------------
inline int step(const Regs &r, int size) { return r.df ? -size : size; }

} // namespace rc
} // namespace wl
