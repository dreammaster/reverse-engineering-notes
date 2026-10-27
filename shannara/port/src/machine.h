// Runtime for the transpiled game logic (port/gen/*.cpp, produced by reversing/scripts/transpile_handlers.py): a flat memory image of the
// executable, the 32-bit registers and flags, a stack, and the shims that stand in for the engine/UI/library code outside the transpiled
// region. The generated functions manipulate this state exactly as the original x86 code does.
#pragma once
#include "common.h"
#include "le.h"

#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace shannara {

class Machine;
typedef void (*GenFn)(Machine &);
typedef std::function<void(Machine &)> Shim; // called with esp pointing at the (dummy) return address slot; must pop it (and callee-cleaned args)

class Machine {
public:
	static const uint32_t kMemSize = 0x1000000; // 16 MB flat image
	static const uint32_t kStackTop = 0x900000;
	static const uint32_t kHeapBase = 0xC00000;

	explicit Machine(const LeExe &exe, size_t memSize = kMemSize, uint32_t delta = 0);

	// registers and flags (public: generated code uses them directly)
	uint32_t eax = 0, ecx = 0, edx = 0, ebx = 0, esp = kStackTop, ebp = 0, esi = 0, edi = 0;
	bool zf = false, sf = false, cf = false, of = false;

	// memory
	uint8_t r8(uint32_t a) const { check(a, 1); return mem[a]; }
	uint16_t r16(uint32_t a) const { check(a, 2); return mem[a] | (mem[a + 1] << 8); }
	uint32_t r32(uint32_t a) const { check(a, 4); return mem[a] | (mem[a + 1] << 8) | (mem[a + 2] << 16) | ((uint32_t)mem[a + 3] << 24); }
	bool wlogOn = false;                 // debugging: record every byte written to the data object / BSS (address << 8 | value) in wlog
	std::vector<uint64_t> wlog;
	uint32_t watchLo = 0, watchHi = 0;   // debugging: onWatch(address, value) is called for every byte written into [watchLo, watchHi)
	std::function<void(uint32_t, uint8_t)> onWatch;
	bool wlogStore = true;
	void logw(uint32_t a, uint8_t v) {
		if (watchHi && a >= watchLo && a < watchHi && onWatch) onWatch(a, v);
		if (wlogStore && a >= 0xA0000 && a < 0xF0000) wlog.push_back(((uint64_t)a << 8) | v);
	}
	void w8(uint32_t a, uint8_t v) { check(a, 1); mem[a] = v; if (wlogOn) logw(a, v); }
	void w16(uint32_t a, uint16_t v) { check(a, 2); mem[a] = v & 0xFF; mem[a + 1] = v >> 8; if (wlogOn) { logw(a, v & 0xFF); logw(a + 1, v >> 8); } }
	void w32(uint32_t a, uint32_t v) { check(a, 4); mem[a] = v & 0xFF; mem[a + 1] = (v >> 8) & 0xFF; mem[a + 2] = (v >> 16) & 0xFF; mem[a + 3] = v >> 24; if (wlogOn) for (int k = 0; k < 4; k++) logw(a + k, (v >> (8 * k)) & 0xFF); }
	std::string cstr(uint32_t a, size_t max = 4096) const;
	void writeBytes(uint32_t a, const void *p, size_t n) { check(a, n); memcpy_(mem.data() + a, p, n); if (wlogOn) for (size_t k = 0; k < n; k++) logw(a + (uint32_t)k, ((const uint8_t *)p)[k]); }
	uint8_t *ptr(uint32_t a) { check(a, 1); return mem.data() + a; }

	// stack
	void push(uint32_t v) { esp -= 4; w32(esp, v); }
	uint32_t pop() { uint32_t v = r32(esp); esp += 4; return v; }
	uint32_t stackArg(int i) const { return r32(esp + 4 + 4 * i); } // inside a shim: i-th argument above the return slot

	// calls
	void registerFunction(uint32_t addr, GenFn fn) { _fns[addr] = fn; }
	void registerShim(uint32_t addr, const std::string &name, int retn) { _shimNames[addr] = name; _retn[addr] = retn; _retnByName[name] = retn; }
	// pops the return slot and the callee-cleaned arguments of the named boundary function (custom shims call this)
	void finishShim(const std::string &name) { auto r = _retnByName.find(name); esp += 4 + (r != _retnByName.end() ? (uint32_t)r->second : 0); }
	void setShim(const std::string &name, Shim s) { _shimByName[name] = std::move(s); }
	void clearShim(const std::string &name) { _shimByName.erase(name); }
	void call(uint32_t addr);      // transpiled function or shim (the caller already pushed the dummy return address)
	std::function<void(Machine &, uint32_t)> callHook; // optional: called with the target address of every call (diagnostics)
	void callShim(uint32_t addr);  // boundary function
	bool tracing = false;
	int64_t budget = INT64_MAX; // instructions left before a runaway loop is aborted (reset per command by Engine)
	std::vector<uint8_t> coverage; // optional: one byte per code address (allocated by enableCoverage)
	void enableCoverage() { coverage.assign(0xA0000, 0); }
	// emulated interrupts: when irqEvery is set, irqHook runs every irqEvery executed instructions (at an instruction boundary)
	int irqEvery = 0, irqCountdown = 0;
	std::function<void(Machine &)> irqHook;
	void tick(uint32_t ea) {
		if (irqEvery && --irqCountdown <= 0) {
			irqCountdown = irqEvery;
			irqHook(*this);
		}
		if (!coverage.empty() && ea < coverage.size()) coverage[ea] = 1;
		if (tracing) traceInsn(ea);
		if (--budget < 0) throw FormatError("instruction budget exhausted");
	}
	std::vector<std::string> traceLog; // "ea eax ecx edx ebx esi edi ebp esp" per executed transpiled instruction (when tracing)
	void traceInsn(uint32_t ea) {
		char b[160];
		snprintf(b, sizeof b, "%x %x %x %x %x %x %x %x %x", ea, eax, ecx, edx, ebx, esi, edi, ebp, esp);
		traceLog.push_back(b);
		if (traceRing && traceLog.size() > 8192)
			traceLog.erase(traceLog.begin(), traceLog.begin() + 4096); // keep only the most recent instructions
	}
	bool traceRing = false;
	std::vector<uint32_t> trace; // addresses of the functions currently executing (kept after an exception for diagnosis)
	std::map<std::string, uint64_t> unknownShimCalls; // boundary functions that had no specific shim (no-ops) -> call counts
	bool logEvents = false;                           // record every no-op shim call (dialogs, music, journal, events...) in `events`
	std::vector<std::string> events;

	// flag helpers: results are written back by the generated code
	template <typename T> T add(T a, T b) { T r = (T)(a + b); setAdd(a, b, r); return r; }
	template <typename T> T sub(T a, T b) { T r = (T)(a - b); setSub(a, b, r); return r; }
	template <typename T> T adc(T a, T b) { T c = cf; T r = (T)(a + b + c); cf = (uint64_t)a + b + c > (T)~(T)0; setZS(r); of = ((~(a ^ b) & (a ^ r)) >> (sizeof(T) * 8 - 1)) & 1; return r; }
	template <typename T> T sbb(T a, T b) { T c = cf; T r = (T)(a - b - c); cf = (uint64_t)a < (uint64_t)b + c; setZS(r); of = (((a ^ b) & (a ^ r)) >> (sizeof(T) * 8 - 1)) & 1; return r; }
	template <typename T> T and_(T a, T b) { T r = a & b; cf = of = false; setZS(r); return r; }
	template <typename T> T or_(T a, T b) { T r = a | b; cf = of = false; setZS(r); return r; }
	template <typename T> T xor_(T a, T b) { T r = a ^ b; cf = of = false; setZS(r); return r; }
	template <typename T> T inc(T a) { bool c = cf; T r = add<T>(a, 1); cf = c; return r; }
	template <typename T> T dec(T a) { bool c = cf; T r = sub<T>(a, 1); cf = c; return r; }
	template <typename T> T neg(T a) { T r = sub<T>(0, a); cf = a != 0; return r; }
	template <typename T> T shl(T a, unsigned n) {
		n &= 31;
		if (!n) return a;
		T r = n >= sizeof(T) * 8 ? 0 : (T)(a << n);
		cf = n <= sizeof(T) * 8 ? ((a >> (sizeof(T) * 8 - n)) & 1) : false;
		setZS(r);
		of = ((r >> (sizeof(T) * 8 - 1)) & 1) != cf;
		return r;
	}
	template <typename T> T shr(T a, unsigned n) {
		n &= 31;
		if (!n) return a;
		T r = n >= sizeof(T) * 8 ? 0 : (T)(a >> n);
		cf = n <= sizeof(T) * 8 ? ((a >> (n - 1)) & 1) : false;
		setZS(r);
		of = (a >> (sizeof(T) * 8 - 1)) & 1;
		return r;
	}
	template <typename T> T sar(T a, unsigned n) {
		typedef typename std::make_signed<T>::type S;
		n &= 31;
		if (!n) return a;
		S s = (S)a;
		T r = (T)(n >= sizeof(T) * 8 ? (s < 0 ? -1 : 0) : (s >> n));
		cf = n <= sizeof(T) * 8 ? (((S)a >> (n - 1)) & 1) : (s < 0);
		setZS(r);
		of = false;
		return r;
	}
	template <typename T> void setZS(T r) { zf = r == 0; sf = (r >> (sizeof(T) * 8 - 1)) & 1; }
	template <typename T> void setAdd(T a, T b, T r) { cf = r < a; of = ((~(a ^ b) & (a ^ r)) >> (sizeof(T) * 8 - 1)) & 1; setZS(r); }
	template <typename T> void setSub(T a, T b, T r) { cf = a < b; of = (((a ^ b) & (a ^ r)) >> (sizeof(T) * 8 - 1)) & 1; setZS(r); }

	// string instructions (rep prefix passed as a flag; the direction flag is honoured)
	bool df = false;
	uint32_t vram = 0xA0000;                 // linear address of the 64 KB VGA/VESA window (the transpiled gx code loads it as an immediate)
	int step(int sz) const { return df ? -sz : sz; }
	void movsd(bool rep) { uint32_t n = rep ? ecx : 1; int s = step(4); for (uint32_t i = 0; i < n; i++) { w32(edi, r32(esi)); esi += s; edi += s; } if (rep) ecx = 0; }
	void movsw(bool rep) { uint32_t n = rep ? ecx : 1; int s = step(2); for (uint32_t i = 0; i < n; i++) { w16(edi, r16(esi)); esi += s; edi += s; } if (rep) ecx = 0; }
	void movsb(bool rep) { uint32_t n = rep ? ecx : 1; int s = step(1); for (uint32_t i = 0; i < n; i++) { w8(edi, r8(esi)); esi += s; edi += s; } if (rep) ecx = 0; }
	void stosd(bool rep) { uint32_t n = rep ? ecx : 1; int s = step(4); for (uint32_t i = 0; i < n; i++) { w32(edi, eax); edi += s; } if (rep) ecx = 0; }
	void stosw(bool rep) { uint32_t n = rep ? ecx : 1; int s = step(2); for (uint32_t i = 0; i < n; i++) { w16(edi, (uint16_t)eax); edi += s; } if (rep) ecx = 0; }
	void stosb(bool rep) { uint32_t n = rep ? ecx : 1; int s = step(1); for (uint32_t i = 0; i < n; i++) { w8(edi, (uint8_t)eax); edi += s; } if (rep) ecx = 0; }
	void lodsb(bool rep) { uint32_t n = rep ? ecx : 1; int s = step(1); for (uint32_t i = 0; i < n; i++) { eax = (eax & 0xFFFFFF00u) | r8(esi); esi += s; } if (rep) ecx = 0; }
	void lodsd(bool rep) { uint32_t n = rep ? ecx : 1; int s = step(4); for (uint32_t i = 0; i < n; i++) { eax = r32(esi); esi += s; } if (rep) ecx = 0; }
	void scasb(bool rep, bool whileEqual = false) { // `repne scasb` stops at the first match, `repe scasb` at the first mismatch
		int s = step(1);
		if (!rep) { sub<uint8_t>((uint8_t)eax, r8(edi)); edi += s; return; }
		while (ecx) { uint8_t b = r8(edi); edi += s; ecx--; sub<uint8_t>((uint8_t)eax, b); if (whileEqual ? !zf : zf) break; }
	}
	void cmpsb(bool repe) {
		int s = step(1);
		if (!repe) { sub<uint8_t>(r8(esi), r8(edi)); esi += s; edi += s; return; }
		while (ecx) { uint8_t a = r8(esi), b = r8(edi); esi += s; edi += s; ecx--; sub<uint8_t>(a, b); if (!zf) break; }
	}

	// rotates and double shifts
	template <typename T> T rol(T a, unsigned n) {
		n &= 31;
		if (!n) return a;
		unsigned b = sizeof(T) * 8;
		n %= b;
		T r = n ? (T)((a << n) | (a >> (b - n))) : a;
		cf = r & 1;
		of = ((r >> (b - 1)) & 1) != cf;
		return r;
	}
	template <typename T> T ror(T a, unsigned n) {
		n &= 31;
		if (!n) return a;
		unsigned b = sizeof(T) * 8;
		n %= b;
		T r = n ? (T)((a >> n) | (a << (b - n))) : a;
		cf = (r >> (b - 1)) & 1;
		of = ((r >> (b - 1)) & 1) != ((r >> (b - 2)) & 1);
		return r;
	}
	template <typename T> T rcl(T a, unsigned n) {
		n &= 31;
		unsigned b = sizeof(T) * 8;
		n %= (b + 1);
		for (unsigned i = 0; i < n; i++) { bool c = (a >> (b - 1)) & 1; a = (T)((a << 1) | (T)cf); cf = c; }
		of = n ? (((a >> (b - 1)) & 1) != cf) : of;
		return a;
	}
	template <typename T> T rcr(T a, unsigned n) {
		n &= 31;
		unsigned b = sizeof(T) * 8;
		n %= (b + 1);
		for (unsigned i = 0; i < n; i++) { bool c = a & 1; a = (T)((a >> 1) | ((T)cf << (b - 1))); cf = c; }
		of = n ? (((a >> (b - 1)) & 1) != ((a >> (b - 2)) & 1)) : of;
		return a;
	}
	uint32_t shld(uint32_t a, uint32_t b, unsigned n) {
		n &= 31;
		if (!n) return a;
		uint32_t r = (a << n) | (b >> (32 - n));
		cf = (a >> (32 - n)) & 1;
		setZS(r);
		of = ((r >> 31) & 1) != ((a >> 31) & 1);
		return r;
	}
	uint32_t shrd(uint32_t a, uint32_t b, unsigned n) {
		n &= 31;
		if (!n) return a;
		uint32_t r = (a >> n) | (b << (32 - n));
		cf = (a >> (n - 1)) & 1;
		setZS(r);
		of = ((r >> 31) & 1) != ((a >> 31) & 1);
		return r;
	}
	uint32_t flags() const { return 0x202u | (cf ? 1u : 0u) | (zf ? 0x40u : 0u) | (sf ? 0x80u : 0u) | (df ? 0x400u : 0u) | (of ? 0x800u : 0u); }
	void setFlags(uint32_t f) { cf = f & 1; zf = f & 0x40; sf = f & 0x80; df = f & 0x400; of = f & 0x800; }

	// the hardware boundary: software interrupts (DOS / BIOS / DPMI services) and port I/O are implemented by the host
	std::function<void(Machine &, int)> onInterrupt;
	std::function<uint32_t(Machine &, uint16_t, int)> onPortIn;           // port, bytes -> value
	std::function<void(Machine &, uint16_t, uint32_t, int)> onPortOut;    // port, value, bytes
	void intr(int n) { if (!onInterrupt) throw FormatError("int without a handler"); onInterrupt(*this, n); }
	uint32_t portIn(uint16_t p, int bytes) { return onPortIn ? onPortIn(*this, p, bytes) : 0; }
	void portOut(uint16_t p, uint32_t v, int bytes) { if (onPortOut) onPortOut(*this, p, v, bytes); }

	bool cond_b() const { return cf; }
	bool cond_be() const { return cf || zf; }
	bool cond_l() const { return sf != of; }
	bool cond_le() const { return zf || sf != of; }

	std::vector<uint8_t> mem;
	uint32_t heapPtr = kHeapBase;

private:
	void check(uint32_t a, size_t n) const {
		if ((uint64_t)a + n > mem.size())
			throw FormatError("memory access outside the image");
	}
	static void memcpy_(void *d, const void *s, size_t n);
	std::unordered_map<uint32_t, GenFn> _fns;
	std::map<uint32_t, std::string> _shimNames;
	std::map<uint32_t, int> _retn;
	std::map<std::string, int> _retnByName;
	std::map<std::string, Shim> _shimByName;
};

} // namespace shannara
