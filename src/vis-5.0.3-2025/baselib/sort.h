// Reconstructed from Deponia_Linux.asm, RadixSort (asm 2987143-2987919); the path of the original, from its x_assert():
// src/baselib/sort.cpp.
//
// A radix sort of floats with the ranks kept between sorts (the "Radix Sort Revisited" of Pierre Terdiman: a
// sort that finds out that the keys are in order already, and else sorts the ranks by the four bytes of the keys, with
// the sign of the floats taken care of). The keys are `stride` floats apart in the input, and a rank is the number of
// floats from the beginning of the input to its key (so it is the index times `stride`). The ranks of the sort before
// are the order that the next one starts from, which is what decides where equal keys go.
#pragma once

#include <stdint.h>

class RadixSort {
public:
	RadixSort();
	~RadixSort();

	/** Makes room for `size` keys, `maxUsed` of which are used, `stride` floats apart (the ranks are made again). */
	void Resize(unsigned size, unsigned maxUsed, unsigned stride);
	/** As Resize(), but only when the room has to grow or the stride changes; false for no keys. */
	bool Grow(unsigned size, unsigned maxUsed, unsigned stride);
	/** The ranks are the order of the input again, for `used` keys. */
	void ResetIndices(unsigned used);
	/** Sorts `count` keys (smallest first); the ranks are in GetRanks(). */
	bool Sort(const float *input, unsigned count);

	/** The ranks of the last sort. */
	const uint32_t *GetRanks() const {
		return _ranks;
	}

private:
	unsigned _size;           // +0x00, the keys that there is room for
	unsigned _currentSize;    // +0x04, the keys that the ranks are for
	unsigned _stride;         // +0x08, floats from one key to the next
	uint32_t *_ranks;         // +0x10
	uint32_t *_ranks2;        // +0x18
	uint32_t _histogram[4][256];  // +0x20, how many keys have each value of each of their four bytes
	uint32_t _offset[256];    // +0x1020
};
