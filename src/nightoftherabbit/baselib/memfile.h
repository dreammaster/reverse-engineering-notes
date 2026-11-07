// Original path confirmed via x_assert() calls: src/baselib/memfile.cpp -
// see manifest/source_layout.tsv.
//
// Confirmed layout (ctor/dtor/DetachBuffer/GetBuffer/GetSize/ReleaseMemory/
// Reserve, Deponia_Linux.asm lines 551869-552091+): a plain owned
// new[]-allocated byte buffer plus its size - not a TMemoryBuffer as
// previously modeled (nothing else in this codebase depended on that, so
// corrected outright). The original's own member name, `m_pData`, is
// recovered byte-for-byte from GetBuffer()'s x_assert() string.
#pragma once

class TMemoryFile {
public:
	TMemoryFile() = default;
	~TMemoryFile();

	TMemoryFile(const TMemoryFile &) = delete;
	TMemoryFile &operator=(const TMemoryFile &) = delete;

	// Confirmed (asm lines 551923-552038): asserts m_pData is non-null,
	// logging a (real, if terse - just "A") warning first if it isn't and
	// logging is verbose; returns it either way.
	unsigned char *GetBuffer();
	unsigned long GetSize() const {
		return _size;
	}
	// Frees the buffer without reallocating (asm lines 552062-552081).
	void ReleaseMemory();
	// Confirmed (asm lines 552089-552136): grows the buffer to at least
	// `size` bytes, discarding any existing contents; returns whether a
	// buffer exists afterward (always true after a successful grow, or
	// reflects the existing buffer's presence if already large enough).
	bool Reserve(long size);
	// Gives up ownership of the buffer without freeing it (asm lines
	// 551908-551917) - the caller is expected to have already taken the
	// pointer via GetBuffer().
	void DetachBuffer();

private:
	unsigned char *_data = nullptr;
	unsigned long _size = 0;
};
