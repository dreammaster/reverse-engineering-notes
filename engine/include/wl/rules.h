#pragma once
// Wasteland's random numbers and its "dice" accumulators (WL.EXE 0x18E41..0x19CCC).
//
// Every skill/attribute check builds a target in `seed1` and a roll in `seed2` (both 16 bit, clamped at 0 / FFFFh)
// and succeeds when seed2 >= seed1. The IDB calls them _randomSeed1/_randomSeed2.
#include <cstdint>

namespace wl {

class Rules {
public:
	// five byte state of the generator (ds:465C..4660); one `rngByte` call advances it
	uint8_t rngState[5] = {};
	uint16_t seed1 = 0;                // target accumulator
	uint16_t seed2 = 0;                // roll accumulator

	// sub_18E6B: mixes `al` into the state with an add-with-carry chain and returns the new value
	uint8_t rngByte(uint8_t al);
	// sub_18E41: uniformly distributed 1..n (rejection sampling on a bit mask); `n <= 1` returns n.
	uint8_t random(uint8_t n);
	// sub_18E5F: one d6 (the incoming al is the generator's carry-over value)
	uint8_t d6(uint8_t al);
	// sub_19C84: two d6 added, and rolled again (added) while both dice are equal
	uint16_t rollDice();

	// sub_19C2C / sub_19C04: signed add to seed1 / seed2, clamped to 0..FFFFh
	void addTarget(int32_t delta);
	void addRoll(int32_t delta);
	// sub_19CAC: base difficulty target 15 + 5 * count
	static uint32_t targetBase(unsigned count) { return 15 + 5 * count; }
	bool rollBeatsTarget() const { return seed2 >= seed1; }   // sub_19C69 (CF clear = success)
};

} // namespace wl
