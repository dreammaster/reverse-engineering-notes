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
	// Confirmed call shape only (TGameControl::LoadAndInitGame,
	// Deponia_Linux.asm lines 468170-468174).
	bool operator==(const wxString &other) const {
		return _value.ToStdWstring() == other.ToStdWstring();
	}
	// Confirmed call shape only (TGameControl::InitScripts, Deponia_Linux.asm
	// line 458534).
	const wchar_t *c_str() const {
		return _value.c_str();
	}

private:
	wxString _value;
};
