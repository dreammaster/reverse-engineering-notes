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

	explicit Machine(const LeExe &exe);

	// registers and flags (public: generated code uses them directly)
	uint32_t eax = 0, ecx = 0, edx = 0, ebx = 0, esp = kStackTop, ebp = 0, esi = 0, edi = 0;
	bool zf = false, sf = false, cf = false, of = false;

	// memory
	uint8_t r8(uint32_t a) const { check(a, 1); return mem[a]; }
	uint16_t r16(uint32_t a) const { check(a, 2); return mem[a] | (mem[a + 1] << 8); }
	uint32_t r32(uint32_t a) const { check(a, 4); return mem[a] | (mem[a + 1] << 8) | (mem[a + 2] << 16) | ((uint32_t)mem[a + 3] << 24); }
	bool wlogOn = false;                 // debugging: record every byte written to the data object / BSS (address << 8 | value) in wlog
	std::vector<uint64_t> wlog;
	void logw(uint32_t a, uint8_t v) { if (a >= 0xA0000 && a < 0xF0000) wlog.push_back(((uint64_t)a << 8) | v); }
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
	void call(uint32_t addr);      // transpiled function or shim (the caller already pushed the dummy return address)
	void callShim(uint32_t addr);  // boundary function
	bool tracing = false;
	int64_t budget = INT64_MAX; // instructions left before a runaway loop is aborted (reset per command by Engine)
	std::vector<uint8_t> coverage; // optional: one byte per code address (allocated by enableCoverage)
	void enableCoverage() { coverage.assign(0xA0000, 0); }
	void tick(uint32_t ea) {
		if (!coverage.empty() && ea < coverage.size()) coverage[ea] = 1;
		if (tracing) traceInsn(ea);
		if (--budget < 0) throw FormatError("instruction budget exhausted");
	}
	std::vector<std::string> traceLog; // "ea eax ecx edx ebx esi edi ebp esp" per executed transpiled instruction (when tracing)
	void traceInsn(uint32_t ea) {
		char b[160];
		snprintf(b, sizeof b, "%x %x %x %x %x %x %x %x %x", ea, eax, ecx, edx, ebx, esi, edi, ebp, esp);
		traceLog.push_back(b);
	}
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

	// string instructions (direction flag is always clear in this code)
	void movsd(bool rep) { uint32_t n = rep ? ecx : 1; for (uint32_t i = 0; i < n; i++) { w32(edi, r32(esi)); esi += 4; edi += 4; } if (rep) ecx = 0; }
	void movsw(bool rep) { uint32_t n = rep ? ecx : 1; for (uint32_t i = 0; i < n; i++) { w16(edi, r16(esi)); esi += 2; edi += 2; } if (rep) ecx = 0; }
	void movsb(bool rep) { uint32_t n = rep ? ecx : 1; for (uint32_t i = 0; i < n; i++) { w8(edi, r8(esi)); esi++; edi++; } if (rep) ecx = 0; }
	void scasb(bool repne) {
		if (!repne) { sub<uint8_t>((uint8_t)eax, r8(edi)); edi++; return; }
		while (ecx) { uint8_t b = r8(edi); edi++; ecx--; sub<uint8_t>((uint8_t)eax, b); if (zf) break; }
	}
	void cmpsb(bool repe) {
		if (!repe) { sub<uint8_t>(r8(esi), r8(edi)); esi++; edi++; return; }
		while (ecx) { uint8_t a = r8(esi), b = r8(edi); esi++; edi++; ecx--; sub<uint8_t>(a, b); if (!zf) break; }
	}

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
