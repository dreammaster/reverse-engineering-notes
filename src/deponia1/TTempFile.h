// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Tracks temp files the engine creates and cleans them up. Confirmed call
// shape only (TGameControl::SaveGame,
// Deponia_Linux.asm line 463093): a static, no-argument cleanup entry
// point, matching the TGAction/TGAnimation static-entry-point pattern.
#pragma once

class TTempFile {
public:
	static void DeleteTempFiles();
};
