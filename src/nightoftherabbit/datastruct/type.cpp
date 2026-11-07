#include "datastruct/type.h"

#include "Diagnostics.h"

TTypeData::TTypeData(int description, eTypeData type, eVisionaireTable link, int versionIn, int versionOut)
	: _description(description), _link(static_cast<int>(link)), _type(type), _versionIn(versionIn),
	  _versionOut(versionOut) {
}

TTypeData::TTypeData(int description, eTypeData type, int versionIn, int versionOut)
	: _description(description), _link(-2), _type(type), _versionIn(versionIn), _versionOut(versionOut) {
}

// Confirmed (asm lines 668582-668625), including the original's own
// consistency check: a savegame-only field (eSaveGame 1) must not have a
// game-data version range that rules it out - the real assert text is
// "!(savegame == t_SAVEGAME && (versionIn > 1 || versionOut > -1))".
TTypeData::TTypeData(int description, eTypeData type, eSaveGame savegame, int versionIn, int versionOut,
                     int saveVersionIn, int saveVersionOut)
	: _description(description), _link(-2), _type(type), _versionIn(versionIn), _versionOut(versionOut),
	  _saveVersionIn(saveVersionIn), _saveVersionOut(saveVersionOut), _saveGameType(savegame) {
	bool ok = true;
	if (savegame == eSaveGame::kValue1)
		ok = versionOut < 0 && versionIn <= 1;
	x_assert(ok, "!(savegame == t_SAVEGAME && (versionIn > 1 || versionOut > -1))",
	         "/home/simon/Documents/jenkins/branchPillars/src/datastruct/type.cpp", 0x22);
}
