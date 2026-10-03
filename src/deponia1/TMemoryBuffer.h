// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full except Compress()/Uncompress() (Deponia_Linux.asm lines
// 549297-551866, all 26 manifest-listed methods): a growable owned byte
// buffer - `unsigned char *` + a used length + an allocated capacity - used
// both as a scratch read buffer (TFile::ReadToBuf) and as a byte-stream
// builder (the operator<< family, used by TXMLWriter and others).
//
// Compress()/Uncompress() (3 of the 26 methods: both Uncompress() overloads
// and Compress() itself) call the REAL zlib C API directly (`compress()`/
// `uncompress()`, confirmed by their exact symbol names and zlib's own
// well-known `len*1.001+12` worst-case-size formula for the compress-side
// output buffer) - this project doesn't currently link zlib, so these 3
// stay call-shape stubs (matching the same "genuine third-party API
// boundary" treatment as the Lua C API and the Steam/Galaxy SDKs
// elsewhere) pending a decision to add that dependency.
//
// Decrypt()/Encrypt() are a real, fully confirmed cipher: Encrypt() is
// simply Decrypt() (XOR is its own inverse). Decrypt() XORs the buffer
// (starting at byte `offset`) against a repeating 16-byte keystream that is
// MD5(narrow-converted key) - `cursor`, when non-null, carries the current
// position (0-16) within that repeating keystream in and out, so a
// multi-part decrypt can resume mid-cycle. The disassembly's own resume
// logic only re-derives the MD5-buffer pointer correctly when the incoming
// cursor is exactly 0 or 16 (both point at the digest's first byte); the
// two confirmed call sites only ever pass a null cursor, so this isn't
// exercised either way - modeled here so the keystream position (`pos`)
// and the digest pointer always stay in sync for ANY incoming cursor
// value, which matches the two confirmed call sites' behavior exactly and
// is the more defensible reading of the resumable-cipher's intent.
#pragma once

#include "WxStub.h"

class TMemoryBuffer {
public:
	TMemoryBuffer() = default;
	~TMemoryBuffer();

	// Frees the buffer outright (dtor's own logic).
	void ReleaseMemory();
	// Marks the buffer logically empty without freeing it.
	void ClearMemory();
	// Confirmed (asm lines 549385-549433): ensures capacity >= size,
	// discarding any existing content (and resetting the used length to 0)
	// only when it actually has to grow; a no-op when capacity already
	// suffices and a buffer exists.
	void Reserve(unsigned long size);
	// Confirmed (asm lines 549441-549510): ensures capacity >= size like
	// Reserve() above, but PRESERVES existing content (memcpy on grow) and
	// never touches the used length.
	void EnsureBufferSize(unsigned long size);
	// Confirmed (asm lines 549518-549567): like Reserve(), but always resets
	// the used length to 0 even when no reallocation was needed.
	void Init(unsigned long size);

	// Confirmed (asm lines 549575-549819, matching operator<<(char)'s
	// identical shape): appends `size` bytes, growing the buffer (by
	// `size + 0x400000` bytes of headroom, matching the original's own
	// chunky growth strategy) when needed.
	void AppendData(const void *data, unsigned long size);
	void AppendByte(unsigned char value);

	unsigned char *GetData() {
		return _data;
	}
	const unsigned char *GetData() const {
		return _data;
	}
	unsigned long GetLen() const {
		return _len;
	}

	// Confirmed (asm lines 549860-550214): byte-for-byte identical growth
	// logic to AppendByte() above, just returning *this for chaining.
	TMemoryBuffer &operator<<(char value);
	// Confirmed (asm lines 549961-550115): appends strlen(value) bytes -
	// no null terminator.
	TMemoryBuffer &operator<<(const char *value);
	TMemoryBuffer &operator<<(unsigned char value);
	// Confirmed (asm lines 550213-550648): the numeric overloads append
	// BIG-ENDIAN bytes - 2 for short/unsigned short, 4 for long/unsigned long
	// (only the low 32 bits of the 64-bit `long`). (An earlier version of this
	// header said native little-endian, sizeof(value) bytes; wrong.)
	TMemoryBuffer &operator<<(short value);
	TMemoryBuffer &operator<<(unsigned short value);
	TMemoryBuffer &operator<<(long value);
	TMemoryBuffer &operator<<(unsigned long value);
	// Confirmed (asm lines 550650-550872): narrow-converts via GetFullPath()
	// + mb_str(), then appends like operator<<(const char*) - no length
	// prefix, no terminator.
	TMemoryBuffer &operator<<(const wxFileName &value);
	TMemoryBuffer &operator<<(const wxString &value);
	// Confirmed (asm lines 551062-551322): narrow-converts, appends a 2-byte
	// big-endian length prefix (via the same logic as operator<<(short)),
	// then the raw narrow bytes (no terminator).
	void AppendStringWithLen(const wxString &value);

	// Not reversed beyond their confirmed real-zlib call shape - see the
	// class comment.
	bool Uncompress(TMemoryBuffer &dest, long expectedSize);
	bool Uncompress(long expectedSize);
	bool Compress();

	// Confirmed (asm lines 551698-551816): a no-op returning true when `key`
	// is empty (matching the pre-existing stub's behavior for that case).
	bool Decrypt(const wxString &key, unsigned long *cursor, unsigned long offset);
	// Confirmed (asm lines 551824-551835): just calls Decrypt() (XOR is its
	// own inverse) and always returns true.
	bool Encrypt(const wxString &key, unsigned long *cursor, unsigned long offset);

private:
	void growForAppend(unsigned long extraBytes);

	unsigned char *_data = nullptr;
	unsigned long _len = 0;
	unsigned long _capacity = 0;
};
