// The 8086 interpreter (engine/cpu8086.h): arithmetic and flags, shifts, multiplication and division, string moves, calls.

#include "engine/cpu8086.h"

#include <cstdio>
#include <vector>

using namespace Ultima6;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

// runs the bytes at 0x1000:0 until they reach the 0xF4 behind them
static bool run(Cpu8086 &c, const std::vector<uint8_t> &code) {
	for (size_t i = 0; i < code.size(); ++i)
		c.write8(0x1000, (uint16_t)i, code[i]);
	c.cs = c.ds = c.es = c.ss = 0x1000;
	c.sp = 0xF000;
	c.ip = 0;
	return c.run(Cpu8086::linear(0x1000, (uint16_t)code.size()), 100000);
}

int main() {
	{ // mov ax, 0xFFFF; add ax, 2: carry, result 1
		Cpu8086 c;
		CHECK(run(c, {0xB8, 0xFF, 0xFF, 0x05, 0x02, 0x00}));
		CHECK(c.ax == 1 && c.cf && !c.zf && !c.sf);
	}
	{ // mov al, 0x7F; add al, 1: signed overflow
		Cpu8086 c;
		CHECK(run(c, {0xB0, 0x7F, 0x04, 0x01}));
		CHECK((c.ax & 0xFF) == 0x80 && c.of && c.sf && !c.cf);
	}
	{ // sub: borrow and zero
		Cpu8086 c;
		CHECK(run(c, {0xB8, 0x03, 0x00, 0x2D, 0x04, 0x00}));
		CHECK(c.ax == 0xFFFF && c.cf && c.sf);
	}
	{ // shl ax, 1 (rcl and shr too); mul bx; div bx
		Cpu8086 c;
		CHECK(run(c, {0xB8, 0x00, 0x81, 0xD1, 0xE0, 0xD1, 0xD2}));
		CHECK(c.ax == 0x0200 && c.dx == 1 && !c.cf); // 0x8100 << 1 = 0x0200, carry 1 into dx by rcl dx
		Cpu8086 d;
		CHECK(run(d, {0xB8, 0x34, 0x12, 0xBB, 0x10, 0x00, 0xF7, 0xE3, 0xF7, 0xF3}));
		CHECK(d.ax == 0x1234 && d.dx == 0);
	}
	{ // rep movsb: copies cx bytes forwards; rep stosw fills
		Cpu8086 c;
		for (int i = 0; i < 8; ++i)
			c.write8(0x1000, (uint16_t)(0x100 + i), (uint8_t)(i + 1));
		CHECK(run(c, {0xBE, 0x00, 0x01, 0xBF, 0x00, 0x02, 0xB9, 0x08, 0x00, 0xFC, 0xF3, 0xA4}));
		CHECK(c.read8(0x1000, 0x200) == 1 && c.read8(0x1000, 0x207) == 8 && c.cx == 0);
		Cpu8086 d;
		CHECK(run(d, {0xB8, 0x34, 0x12, 0xBF, 0x00, 0x03, 0xB9, 0x03, 0x00, 0xFC, 0xF3, 0xAB}));
		CHECK(d.read16(0x1000, 0x300) == 0x1234 && d.read16(0x1000, 0x304) == 0x1234 && d.di == 0x306);
	}
	{ // call near / ret; push and pop; loop
		Cpu8086 c;
		// 0: call 7; 3: jmp to the end (11); 5, 6: nops; 7: mov bx, 5; 10: ret
		CHECK(run(c, {0xE8, 0x04, 0x00, 0xEB, 0x06, 0x90, 0x90, 0xBB, 0x05, 0x00, 0xC3}));
		CHECK(c.bx == 5 && c.sp == 0xF000);
		Cpu8086 d;
		CHECK(run(d, {0xB9, 0x04, 0x00, 0x31, 0xC0, 0x40, 0xE2, 0xFD})); // cx = 4; ax = 0; loop: inc ax; loop
		CHECK(d.ax == 4 && d.cx == 0);
	}
	{ // a division by zero stops the run
		Cpu8086 c;
		CHECK(!run(c, {0xB8, 0x01, 0x00, 0x31, 0xDB, 0xF7, 0xF3}));
		CHECK(c.error() != nullptr);
	}
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
