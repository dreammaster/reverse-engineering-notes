// Original path confirmed via x_assert() calls (not yet reconstructed) -
// see manifest/source_layout.tsv.
//
// Confirmed in full (Deponia_Linux.asm lines 529250-531666, all 32
// manifest-listed methods, matching the demangled `TCharHolder` symbol
// byte-for-byte - this is a genuinely recovered class name, not a guess).
// A lightweight string handle used throughout the engine (image/sound
// paths, tracked temp-file paths, object names) - NOT a wxString wrapper as
// an earlier pass here had assumed: the real class is its own owned,
// heap-allocated `char *` buffer plus a byte count (`new[]`/`delete[]`,
// exactly the same "raw owned buffer" shape as TMemoryFile elsewhere in
// this project), always storing narrow (8-bit) bytes - wide (wchar_t*/
// wxString) input is always narrow-converted first via wxString::mb_str().
// `_data == nullptr` iff `_size == 0` is a strict invariant maintained by
// every mutator; comparisons (Cmp/CmpNoCase/operator==/SameAs) treat this
// "null" state as distinct from - and never equal to - any non-null value,
// even an empty one.
#pragma once

#include "WxStub.h"

class TCharHolder {
public:
	TCharHolder() = default;
	TCharHolder(const TCharHolder &other);
	// Confirmed (asm lines 529250-529363): narrow-converts via
	// wxString::mb_str() before copying, exactly like the wxString overload
	// below.
	explicit TCharHolder(const wchar_t *value);
	explicit TCharHolder(const char *value);
	// Confirmed (asm lines 529715-529781): a [first, last) range copy: if
	// the byte at `last[-1]` is already 0, the range is assumed to already
	// be null-terminated and copied as-is (no extra byte added); otherwise
	// a terminator is appended.
	TCharHolder(const char *first, const char *last);
	explicit TCharHolder(const wxFileName &value);
	explicit TCharHolder(const wxString &value);
	~TCharHolder();

	// Confirmed (asm lines 529844-529890): replaces the contents with a copy
	// of the null-terminated string, same shape as operator=(const char*)
	// below.
	void copy(const char *value);
	// Confirmed (asm lines 529898-529954): replaces the contents with an
	// exact `length`-byte copy (no implied/added terminator), same shape as
	// the copy constructor's payload copy.
	void copy(const char *value, int length);
	// Confirmed (asm lines 529962-530012): discards any existing contents
	// and allocates a fresh, uninitialized `size`-byte buffer (or frees and
	// clears outright when `size == 0`) - a raw reservation, not a
	// content-preserving resize.
	void resize(unsigned long size);
	unsigned long size() const {
		return _size;
	}
	// Confirmed (asm lines 530063-530072): true iff a buffer is currently
	// held (see the class's null/empty invariant above).
	bool IsOk() const {
		return _size != 0;
	}

	// Confirmed (asm lines 530080-530116): the stored bytes, UTF-8-decoded
	// into a wxString (empty when unset).
	wxString GetFullPath() const;
	// Confirmed (asm lines 530124-530285): `wxPathFormat` is compared only
	// against wxPATH_UNIX at the one call site reversed (TSprite::
	// ToLuaString) - any other format attempts a wxString::Replace() of an
	// empty search string for a path separator, which real wxString::
	// Replace() no-ops on (searching for "" is undefined/guarded against),
	// so every format observably returns the same value as GetFullPath().
	wxString GetFullPath(int format) const;

	// Confirmed (asm lines 530293-530374): strcmp()-equivalent, except a
	// null value is treated as strictly less than any non-null one (both
	// null compares equal).
	int Cmp(const TCharHolder &other) const;
	// Confirmed (asm lines 530382-530456): case-insensitive counterpart of
	// Cmp() above (tolower() per byte), same null-handling.
	int CmpNoCase(const TCharHolder &other) const;
	// Confirmed (asm lines 530464-530513): despite the name, this returns a
	// UTF-8-decoded wxString (empty when unset) - identical in effect to
	// GetFullPath(), just a separately confirmed call shape.
	wxString c_str() const;
	// Confirmed (asm lines 530521-530532): the raw stored bytes directly (or
	// "" when unset) - already narrow, unlike wxString::mb_str() this needs
	// no conversion.
	const char *mb_str() const;
	// Confirmed (asm lines 530538-530640): a lowercased copy, converting via
	// wxString::MakeLower() (locale-dependent for non-ASCII bytes, matching
	// real wx) - returns a new wxString rather than modifying in place,
	// despite the unprefixed name.
	wxString Lower() const;
	// Confirmed (asm lines 530648-530725): parses the stored bytes (UTF-8-
	// decoded first) as a double via wxString::ToDouble(); false (and *out
	// untouched) when unset or unparseable.
	bool ToDouble(double *out) const;

	// Confirmed (asm lines 530733-530773): moves `other`'s buffer into this
	// (freeing this's own first), leaving `other` empty - a move-assign, not
	// a symmetric swap despite the name.
	void exchange(TCharHolder &other);
	TCharHolder &operator=(const char *value);
	TCharHolder &operator=(const wxFileName &value);
	TCharHolder &operator=(const TCharHolder &other);
	// Confirmed (asm lines 531026-531107): same [first, last) range-copy
	// shape as the two-pointer constructor above.
	void Assign(const char *first, const char *last);
	// Confirmed (asm lines 531115-531196): narrow-converts then copies as a
	// null-terminated string, same shape as operator=(const char*).
	void Assign(const wxString &value);

	// Confirmed (asm lines 531202-531244): equality via Cmp() semantics
	// (fast-pathing on a stored-size mismatch before ever calling strcmp).
	bool operator==(const TCharHolder &other) const;
	// Confirmed (asm lines 531252-531377): narrow-converts `other` first,
	// then compares byte-for-byte with the same null-is-never-equal rule.
	bool operator==(const wxString &other) const;
	bool operator==(const wchar_t *other) const;
	// Confirmed (asm lines 531516-531569): compares directly against a raw
	// C string, where a literal nullptr `other` counts as "no value" too
	// (unlike the TCharHolder/wxString overloads, whose argument can never
	// itself be null).
	bool operator==(const char *other) const;
	bool operator!=(const TCharHolder &other) const;
	// Confirmed (asm lines 531625-531666): byte-for-byte identical to
	// operator==(const TCharHolder&) - a differently-named alias for the
	// same case-sensitive comparison.
	bool SameAs(const TCharHolder &other) const;

private:
	// Shared by every "copy a null-terminated narrow C string" mutator
	// (copy(const char*), operator=(const char*)/(const wxFileName&),
	// Assign(const wxString&), and the matching constructors) - they all
	// disassemble to the exact same free-old/strlen/new[]/memcpy sequence.
	void assignCString(const char *value);

	char *_data = nullptr;
	unsigned long _size = 0;
};
