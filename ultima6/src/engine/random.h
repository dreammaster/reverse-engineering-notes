#ifndef ULTIMA6_ENGINE_RANDOM_H
#define ULTIMA6_ENGINE_RANDOM_H

#include <cstdint>

namespace Ultima6 {

/**
 * The Borland C run-time generator all the executables use (U.EXE's _srand at
 * 2195A and _rand at 2196B): a 32-bit LCG, seed = seed * 0x015A4E35 + 1, returning
 * bits 16..30 of the new seed.
 */
class BorlandRandom {
public:
	explicit BorlandRandom(uint16_t seed = 1) : _seed(seed) {}

	void seed(uint16_t seed) {
		_seed = seed;
	}
	/** 0 to 32767 */
	int next() {
		_seed = _seed * 0x015A4E35u + 1;
		return (int)((_seed >> 16) & 0x7FFF);
	}

private:
	uint32_t _seed;
};

} // namespace Ultima6

#endif
