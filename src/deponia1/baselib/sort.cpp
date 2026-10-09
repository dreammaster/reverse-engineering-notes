#include "baselib/sort.h"

#include <string.h>

#include "Diagnostics.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/baselib/sort.cpp";

// Confirmed (asm lines 2987143-2987152)
RadixSort::RadixSort() : _size(0), _currentSize(0), _stride(0), _ranks(nullptr), _ranks2(nullptr) {
	memset(_histogram, 0, sizeof(_histogram));
	memset(_offset, 0, sizeof(_offset));
}

// Confirmed (asm lines 2987162-2987195)
RadixSort::~RadixSort() {
	delete[] _ranks;
	delete[] _ranks2;
}

// Confirmed (asm lines 2987195-2987297)
void RadixSort::Resize(unsigned size, unsigned maxUsed, unsigned stride) {
	if (_size == size && _currentSize == maxUsed && _stride == stride)
		return;

	delete[] _ranks;
	_ranks = nullptr;
	delete[] _ranks2;
	_ranks2 = nullptr;

	_size = size;
	_stride = stride;
	_ranks = new uint32_t[size];
	_ranks2 = new uint32_t[size];

	if (_size && _ranks) {
		x_assert(maxUsed <= _size, "maxused <= Size", kSourceFile, 0x59);
		_currentSize = maxUsed;

		for (unsigned i = 0; i < _currentSize; i++)
			_ranks[i] = _stride * i;
	}
}

// Confirmed (asm lines 2987297-2987397)
bool RadixSort::Grow(unsigned size, unsigned maxUsed, unsigned stride) {
	if (size == 0)
		return false;

	if (size > _size) {
		Resize(size, maxUsed, stride);
		return true;
	}

	if (_stride != stride) {
		Resize(size >= _size ? size : _size, maxUsed, stride);
		return true;
	}

	if (_currentSize == maxUsed)
		return true;

	if (_size && _ranks) {
		x_assert(maxUsed <= _size, "maxused <= Size", kSourceFile, 0x59);
		_currentSize = maxUsed;

		for (unsigned i = 0; i < _currentSize; i++)
			_ranks[i] = _stride * i;
	}

	return true;
}

// Confirmed (asm lines 2987397-2987454)
void RadixSort::ResetIndices(unsigned used) {
	if (_size && _ranks) {
		x_assert(used <= _size, "maxused <= Size", kSourceFile, 0x59);
		_currentSize = used;

		for (unsigned i = 0; i < _currentSize; i++)
			_ranks[i] = _stride * i;
	}
}

// Confirmed (asm lines 2987454-2987919). The input is looked at as the bytes of the keys: a pass is left out when all
// of the keys have the same byte there, and the last pass (the sign) puts the negative numbers - which have a larger
// magnitude with a larger byte - first, with their order reversed.
bool RadixSort::Sort(const float *input, unsigned count) {
	if (count <= 1)
		return true;

	if (_currentSize != count) {
		if (_size && _ranks) {
			x_assert(count <= _size, "maxused <= Size", kSourceFile, 0x59);
			_currentSize = count;

			for (unsigned i = 0; i < _currentSize; i++)
				_ranks[i] = _stride * i;
		}
	}

	memset(_histogram, 0, sizeof(_histogram));

	// the histograms, and whether the keys are in order as the ranks have them
	const uint8_t *bytes = reinterpret_cast<const uint8_t *>(input);
	size_t step = static_cast<size_t>(_stride) * 4;
	bool sorted = true;
	float previous = input[_ranks[0]];

	for (unsigned i = 0; i < count; i++) {
		const uint8_t *key = bytes + i * step;

		_histogram[0][key[0]]++;
		_histogram[1][key[1]]++;
		_histogram[2][key[2]]++;
		_histogram[3][key[3]]++;

		if (sorted && i > 0) {
			float value = input[_ranks[i]];

			if (previous > value)
				sorted = false;
			else
				previous = value;
		}
	}

	if (sorted)
		return true;

	unsigned negatives = 0;

	for (int i = 128; i < 256; i++)
		negatives += _histogram[3][i];

	for (int pass = 0; pass < 3; pass++) {
		const uint8_t *passBytes = bytes + pass;

		if (_histogram[pass][passBytes[0]] == count)
			continue;

		_offset[0] = 0;

		for (int i = 0; i < 255; i++)
			_offset[i + 1] = _offset[i] + _histogram[pass][i];

		const uint32_t *ranks = _ranks;
		const uint32_t *end = _ranks + count;

		for (; ranks != end; ranks++) {
			uint32_t rank = *ranks;
			uint8_t value = passBytes[static_cast<size_t>(rank) * 4];

			_ranks2[_offset[value]++] = rank;
		}

		uint32_t *swap = _ranks;

		_ranks = _ranks2;
		_ranks2 = swap;
	}

	const uint8_t *signBytes = bytes + 3;
	uint8_t first = signBytes[0];

	if (_histogram[3][first] == count) {
		// all of the keys have the same sign byte: the negative ones are in the wrong order
		if (first & 0x80) {
			for (unsigned i = 0; i < count; i++)
				_ranks2[i] = _ranks[count - 1 - i];

			uint32_t *swap = _ranks;

			_ranks = _ranks2;
			_ranks2 = swap;
		}

		return true;
	}

	_offset[0] = negatives;

	for (int i = 0; i < 127; i++)
		_offset[i + 1] = _offset[i] + _histogram[3][i];

	_offset[255] = 0;

	for (int i = 254; i >= 128; i--)
		_offset[i] = _offset[i + 1] + _histogram[3][i + 1];

	for (int i = 128; i < 256; i++)
		_offset[i] += _histogram[3][i];

	for (unsigned i = 0; i < count; i++) {
		uint32_t rank = _ranks[i];
		uint8_t value = signBytes[static_cast<size_t>(rank) * 4];

		if (value < 0x80)
			_ranks2[_offset[value]++] = rank;
		else
			_ranks2[--_offset[value]] = rank;
	}

	uint32_t *swap = _ranks;

	_ranks = _ranks2;
	_ranks2 = swap;
	return true;
}
