// Not yet assert-confirmed to a specific file; stays at the top level.
// A single savegame slot's metadata/IO. Confirmed constructor signature and
// call shapes only (TGameControl::SavegameExists/DeleteSavegame,
// Deponia_Linux.asm lines 462562-462773): constructed with (isNumberedSlot,
// slot, b, c, game) - b/c always passed 0 at every call site seen so far,
// meaning unresolved. SavegameExists() is called without a real "this"
// object at its one call site, so modeled as static (matching the
// TGAction::AddRunningAction/ClearActions pattern elsewhere).
#pragma once

#include "WxStub.h"
#include "datastruct/visobjref.h"

class TVisionaireGame;

class TMSavegame {
public:
	TMSavegame(bool isNumberedSlot, int slot, int b, int c, TVisionaireGame *game);
	virtual ~TMSavegame() = default;

	bool Exists() const;
	bool Delete();
	static bool SavegameExists();

	// Confirmed call shape only (TGameControl::LoadGame(int), asm lines
	// 478385-478469) - not reversed beyond that.
	void CheckVisPaths();

	// Confirmed static (TGameControl::Save, asm lines 462781-462971) -
	// builds a save name from a scene reference; not reversed beyond that
	// call shape.
	static wxString MakeSaveGameName(const TVisObjRef &scene);
	// Confirmed call shape only (TGameControl::SaveGame, asm line 463047).
	int GetSavegameNr() const;
};
