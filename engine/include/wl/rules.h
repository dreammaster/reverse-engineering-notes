#pragma once
// Wasteland's random numbers and its "dice" accumulators (WL.EXE 0x18E41..0x19CCC).
//
// Every skill/attribute check builds a target in `seed1` and a roll in `seed2` (both 16 bit, clamped at 0 / FFFFh)
// and succeeds when seed2 >= seed1. The IDB calls them _randomSeed1/_randomSeed2. All state lives in the data segment.
#include "wl/machine.h"

namespace wl {

class Rules {
public:
	explicit Rules(Machine &m) : _m(m) {}

	// sub_18E6B: mixes `al` into the five byte generator state (ds:465C..4660) with an add-with-carry chain
	uint8_t rngByte(uint8_t al);
	// sub_18E41: uniformly distributed 1..n (rejection sampling on a bit mask); `n <= 1` returns n.
	uint8_t random(uint8_t n);
	// sub_18E5F: one d6 (the incoming al is the generator's carry-over value)
	uint8_t d6(uint8_t al);
	// sub_19C84: two d6 added, and rolled again (added) while both dice are equal
	uint8_t rollDice();

	uint16_t seed1() const { return _m.d16(0x46C0); }   // target accumulator
	uint16_t seed2() const { return _m.d16(0x46BE); }   // roll accumulator
	void setSeed1(uint16_t v) { _m.setD16(0x46C0, v); }
	void setSeed2(uint16_t v) { _m.setD16(0x46BE, v); }
	// sub_19C2C / sub_19C04: signed add to seed1 / seed2, clamped to 0..FFFFh
	void addTarget(int32_t delta);
	void addRoll(int32_t delta);
	// sub_19CAC: base difficulty target 15 + 5 * count
	static uint32_t targetBase(unsigned count) { return 15 + 5 * count; }
	bool rollBeatsTarget() const { return seed2() >= seed1(); }   // sub_19C69 (CF clear = success)

private:
	Machine &_m;
};

} // namespace wl
