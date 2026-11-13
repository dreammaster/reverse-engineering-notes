#ifndef ULTIMA6_ENGINE_CPU8086_H
#define ULTIMA6_ENGINE_CPU8086_H

#include <cstdint>
#include <vector>

namespace Ultima6 {

/**
 * A small interpreter of the 16-bit real-mode instructions of the 8086 (plus the immediate shifts and pushes of the 80186 that DOS compilers use), without ports, interrupts, the coprocessor
 * or the segment-size checks. It runs the drawing drivers (*.DRV) that Martian Dreams loads, the code of which is data of the game's own files (see md_driver.h); the memory is one megabyte.
 */
class Cpu8086 {
public:
	Cpu8086();

	uint8_t *memory() {
		return _mem.data();
	}
	static uint32_t linear(uint16_t seg, uint16_t off) {
		return (((uint32_t)seg << 4) + off) & 0xFFFFF;
	}
	uint8_t read8(uint16_t seg, uint16_t off) const {
		return _mem[linear(seg, off)];
	}
	uint16_t read16(uint16_t seg, uint16_t off) const {
		return (uint16_t)(read8(seg, off) | (read8(seg, (uint16_t)(off + 1)) << 8));
	}
	void write8(uint16_t seg, uint16_t off, uint8_t v) {
		_mem[linear(seg, off)] = v;
	}
	void write16(uint16_t seg, uint16_t off, uint16_t v) {
		write8(seg, off, (uint8_t)v);
		write8(seg, (uint16_t)(off + 1), (uint8_t)(v >> 8));
	}

	// the registers
	uint16_t ax = 0, cx = 0, dx = 0, bx = 0, sp = 0, bp = 0, si = 0, di = 0;
	uint16_t es = 0, cs = 0, ss = 0, ds = 0, ip = 0;
	bool cf = false, pf = false, af = false, zf = false, sf = false, df = false, of = false;

	void push16(uint16_t v);
	uint16_t pop16();

	/**
	 * Runs from cs:ip until the instruction pointer is at the given linear address (checked before each instruction) or `maxSteps` instructions were run. Returns false if an instruction
	 * is not known, a division fails or the steps ran out; error() then says which.
	 */
	bool run(uint32_t stopAt, uint64_t maxSteps);
	const char *error() const {
		return _error;
	}
	uint64_t steps() const {
		return _steps;
	}

private:
	uint16_t &reg16(int i);
	uint8_t &reg8(int i);
	uint16_t &seg(int i);
	struct Operand {
		bool isReg;
		int reg;
		uint16_t segment, offset;
	};
	uint8_t fetch8();
	uint16_t fetch16();
	void modrm(int &mod, int &reg, int &rm);
	Operand decodeRM(int mod, int rm);
	uint16_t load(const Operand &o, bool wide);
	void store(const Operand &o, bool wide, uint16_t v);
	uint16_t alu(int op, uint16_t a, uint16_t b, bool wide);
	uint16_t shift(int op, uint16_t v, int count, bool wide);
	void setSzp(uint16_t r, bool wide);
	bool condition(int cc) const;
	bool step();
	bool fail(const char *msg);

	std::vector<uint8_t> _mem;
	int _segOverride = -1; // 0 es, 1 cs, 2 ss, 3 ds
	const char *_error = nullptr;
	uint64_t _steps = 0;
};

} // namespace Ultima6

#endif
