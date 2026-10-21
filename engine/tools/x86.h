#pragma once
// Minimal 8086 interpreter used as a reference oracle for the Wasteland 1 executable (not part of the engine).
// Covers the instruction subset used by WL.EXE; interrupts and port I/O are forwarded to the embedding machine.
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace x86 {

enum Flag : uint16_t { CF = 1, PF = 4, AF = 0x10, ZF = 0x40, SF = 0x80, TF = 0x100, IF = 0x200, DF = 0x400, OF = 0x800 };

class Cpu {
public:
	Cpu() : mem(1 << 20, 0) {}

	uint16_t ax = 0, cx = 0, dx = 0, bx = 0, sp = 0, bp = 0, si = 0, di = 0;
	uint16_t es = 0, cs = 0, ss = 0, ds = 0, ip = 0, flags = 0x0002;
	std::vector<uint8_t> mem;
	uint64_t steps = 0;
	bool halted = false;
	std::string fault;                                   // set when an unsupported opcode is hit

	// hooks
	std::function<bool(Cpu &, int)> onInt;               // return true if handled (the instruction acts like iret)
	std::function<uint16_t(Cpu &, uint16_t, bool)> onIn; // port, wide
	std::function<void(Cpu &, uint16_t, uint16_t, bool)> onOut;
	std::function<void(Cpu &)> onStep;                   // called before each instruction (trace/breakpoints)

	static uint32_t lin(uint16_t seg, uint16_t off) { return ((uint32_t(seg) << 4) + off) & 0xFFFFF; }
	uint8_t rb(uint16_t seg, uint16_t off) const {
		const uint32_t a = lin(seg, off);
		if (a >= 0xA0000 && a < 0xB0000 && onRead) return onRead(a);
		return mem[a];
	}
	uint16_t rw(uint16_t seg, uint16_t off) const { return uint16_t(rb(seg, off) | (rb(seg, uint16_t(off + 1)) << 8)); }
	void wb(uint16_t seg, uint16_t off, uint8_t v);
	void ww(uint16_t seg, uint16_t off, uint16_t v) { wb(seg, off, uint8_t(v)); wb(seg, uint16_t(off + 1), uint8_t(v >> 8)); }

	std::function<void(uint32_t, uint8_t)> onWrite;      // video memory writes (A0000-AFFFF); the byte is not stored in mem
	std::function<uint8_t(uint32_t)> onRead;             // video memory reads

	bool step();                                         // false when halted or faulted
	void push(uint16_t v) { sp = uint16_t(sp - 2); ww(ss, sp, v); }
	uint16_t pop() { const uint16_t v = rw(ss, sp); sp = uint16_t(sp + 2); return v; }
	void interrupt(int n, bool software = true);         // push flags/cs/ip and jump through the IVT

	bool flag(Flag f) const { return flags & f; }
	void setFlag(Flag f, bool v) { if (v) flags |= f; else flags &= uint16_t(~f); }

private:
	struct ModRM { int mod, reg, rm; uint16_t seg, off; bool isReg; };
	int segOverride = -1;
	uint8_t fetch8() { const uint8_t v = rb(cs, ip); ++ip; return v; }
	uint16_t fetch16() { const uint16_t lo = fetch8(); return uint16_t(lo | (fetch8() << 8)); }
	uint16_t &reg16(int i);
	uint8_t get8(int i);
	void set8(int i, uint8_t v);
	uint16_t &sreg(int i);
	ModRM decode(uint8_t modrm);
	uint16_t readRm(const ModRM &m, bool wide);
	void writeRm(const ModRM &m, bool wide, uint16_t v);
	uint16_t alu(int op, uint16_t a, uint16_t b, bool wide);
	uint16_t shiftRot(int op, uint16_t v, int count, bool wide);
	void szp(uint16_t r, bool wide);
	bool cond(int cc) const;
	void stringOp(uint8_t op, bool wide, int rep);
	uint16_t segFor(uint16_t def) const;
};

} // namespace x86
