// Not yet assert-confirmed to a specific file; stays at the top level.
// A lightweight string handle used throughout the engine (SLoadingScreen's
// image-path fields, TTempFile's tracked paths). Modeled as a thin wxString
// wrapper; real internal representation (possibly an interned/ref-counted
// string, going by patterns seen elsewhere - see NOTES.md) not reversed.
#pragma once

#include "WxStub.h"

class TCharHolder {
public:
	TCharHolder() = default;
	TCharHolder(const TCharHolder &) = default;
	TCharHolder &operator=(const TCharHolder &) = default;

	operator wxString() const {
		return _value;
	}
	operator wxFileName() const {
		return wxFileName(_value.ToStdWstring());
	}
	wxString GetFullPath() const {
		return _value;
	}

private:
	wxString _value;
};
