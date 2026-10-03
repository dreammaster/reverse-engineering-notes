// Original path not yet confirmed; stays alongside type.h in datastruct/.
//
// TVedFile (a Visionaire project/savegame file being read or written) is
// not itself reversed; only the two queries TTypeGroup::AppliesToFileVersion()
// makes of it (Deponia_Linux.asm line 585065) are confirmed call shapes.
#pragma once

class TVedFile {
public:
	bool IsSaveGame() const;
	// Whether a field/type that exists from file version `versionIn` up to
	// (but not including) `versionOut` (-1 = open-ended) is present in this
	// file's format version.
	bool GetVersionOk(int versionIn, int versionOut) const;
};
