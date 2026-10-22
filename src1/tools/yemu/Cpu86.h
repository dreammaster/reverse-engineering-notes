/* A small 8086 real-mode CPU core for the development tool yemu: enough of the instruction set to run the
 * Chapter 1 executables (66 distinct mnemonics, plain 8086 plus a few 186 forms). Not part of the engine. */

#ifndef YEMU_CPU86_H
#define YEMU_CPU86_H

#include <stdint.h>
#include <string>

namespace Yemu {

enum { AX, CX, DX, BX, SP, BP, SI, DI };
enum { ES, CS, SS, DS };
enum { CF = 0x0001, PF = 0x0004, AF = 0x0010, ZF = 0x0040, SF = 0x0080, TF = 0x0100, IF = 0x0200, DF = 0x0400, OF = 0x0800 };

class Cpu;

/** Everything outside the CPU: ports, memory-mapped video and software interrupts. */
class Bus {
public:
	virtual ~Bus() {}
	virtual uint8_t in8(uint16_t port) = 0;
	virtual void out8(uint16_t port, uint8_t v) = 0;
	virtual uint8_t vramRead(uint32_t offset) = 0;          // offset within A000:0000..FFFF
	virtual void vramWrite(uint32_t offset, uint8_t v) = 0;
	/** INT n raised. Return true if handled (the CPU continues after the INT), false to vector through the IVT. */
	virtual bool softInt(Cpu &cpu, uint8_t n) = 0;
	/** Called when execution reaches an address in the HLE stub segment F000h; the handler must pop the return itself or leave it to the CPU. */
	virtual void hleStub(Cpu &cpu, uint16_t offset) = 0;
};

class Cpu {
public:
	Cpu(uint8_t *mem1M, Bus *bus);

	uint16_t r[8];
	uint16_t s[4];
	uint16_t ip;
	uint16_t flags;
	uint64_t instructions;
	bool halted;
	bool irqInhibit;        // set after loading SS: the next instruction cannot be interrupted
	std::string fault;      // set on an undefined/unsupported opcode

	uint8_t *mem;
	Bus *bus;

	// memory (with the A000 hook)
	uint8_t rb(uint16_t seg, uint16_t off);
	uint16_t rw(uint16_t seg, uint16_t off);
	void wb(uint16_t seg, uint16_t off, uint8_t v);
	void ww(uint16_t seg, uint16_t off, uint16_t v);
	static uint32_t lin(uint16_t seg, uint16_t off) { return (((uint32_t)seg << 4) + off) & 0xFFFFF; }

	void push(uint16_t v);
	uint16_t pop();
	/** Raise a software interrupt through the IVT (pushes flags, cs, ip). */
	void interrupt(uint8_t n);

	/** Execute one instruction (or one repeat iteration batch). Returns false when halted or faulted. */
	bool step();

private:
	// decode state
	int segOverride;       // -1 none, else ES..DS index
	int repPrefix;         // 0 none, 0xF2, 0xF3
	uint8_t modrm;
	uint16_t eaSeg, eaOff;
	bool eaIsReg;
	int eaReg;

	uint8_t fetch8();
	uint16_t fetch16();
	void decodeModrm(bool w);
	uint8_t getRm8();
	uint16_t getRm16();
	void setRm8(uint8_t v);
	void setRm16(uint16_t v);
	uint8_t getReg8(int i) const;
	void setReg8(int i, uint8_t v);

	uint16_t alu(int op, uint16_t a, uint16_t b, bool word);
	uint16_t shiftOp(int op, uint16_t v, int count, bool word);
	void setSzp(uint16_t v, bool word);
	bool condition(int cc) const;
	bool parity(uint8_t v) const;
	void stringOp(uint8_t op);
	int segFor(int dflt) const { return segOverride >= 0 ? segOverride : dflt; }
	void fail(const char *what, uint8_t op);
};

} // namespace Yemu

#endif
