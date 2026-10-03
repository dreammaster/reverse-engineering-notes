// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Implemented in full (Deponia_Linux.asm lines 553440-553700, both
// manifest-listed methods). Tracks the temp files the engine creates (a
// static list of their full paths) so they can all be removed together:
// AddTempFile() names one under the system temp directory and records it,
// DeleteTempFiles() removes every recorded file and forgets them. Both are
// called without an object (the manifest's `this` is not used).
#pragma once

#include <string>
#include <vector>

#include "WxStub.h"

class TTempFile {
public:
	// The name is the temp directory plus `name` plus `ext`, concatenated with
	// nothing between them (so callers choose what the pieces contain), made
	// absolute/normalized. The file itself isn't created.
	static wxFileName AddTempFile(const wxString &name, const wxString &ext);
	// Removes every file AddTempFile() recorded (ignoring failures).
	static void DeleteTempFiles();

private:
	static std::vector<std::wstring> s_tempFiles;
};
