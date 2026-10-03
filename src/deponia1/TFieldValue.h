// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 522229-522260, 528207-529250, all 9
// manifest-listed methods): a small tagged value used to write one XML
// attribute - nothing, an integer, a string, a bool, a float or a file path.
// The tag values (0-5) are read off the constructors; the enum names and the
// member names are invented. The original lays all payloads out side by side
// (the integer at +0, the string at +8, the bool at +0x10, the float at +0x14,
// the path at +0x18, the tag at +0x20) rather than in a union.
#pragma once

#include "WxStub.h"

class TFieldValue {
public:
	enum eKind {
		kNone = 0,
		kInt = 1,
		kString = 2,
		kBool = 3,
		kFloat = 4,
		kPath = 5
	};

	TFieldValue() = default;
	explicit TFieldValue(bool value);
	explicit TFieldValue(long value);
	explicit TFieldValue(float value);
	explicit TFieldValue(const wxString &value);
	/** Stores the path unless it is not valid (then it stays empty). */
	explicit TFieldValue(const wxFileName &value);
	/** Stores `value` made relative to `base` (as is, if either isn't valid). */
	TFieldValue(const wxFileName &value, const wxFileName &base);

	/** The value as XML attribute text: strings and paths have &, <, >, " and
	 *  ' replaced by their entities; the other kinds are formatted plainly
	 *  (booleans as "T"/"F", floats with "%f" after switching the C locale's
	 *  numeric category to "C"). */
	wxString ToString() const;

private:
	long _int = 0;
	wxString _string;
	bool _bool = false;
	float _float = 0.0f;
	wxFileName _path;
	eKind _kind = kNone;
};
