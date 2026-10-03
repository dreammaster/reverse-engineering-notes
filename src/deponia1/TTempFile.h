// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Tracks temp files the engine creates and cleans them up. Confirmed call
// shape only (TGameControl::SaveGame,
// Deponia_Linux.asm line 463093): a static, no-argument cleanup entry
// point, matching the TGAction/TGAnimation static-entry-point pattern.
#pragma once

#include "WxStub.h"

class TTempFile {
public:
	static void DeleteTempFiles();
	// Confirmed call shape only (TMSavegame::SaveGame, Deponia_Linux.asm line
	// 163500+) - registers a temp file named `name` with extension `ext` for
	// later cleanup by DeleteTempFiles() and returns its full path; not
	// reversed beyond that call shape.
	static wxString AddTempFile(const wxString &name, const wxString &ext);
};
