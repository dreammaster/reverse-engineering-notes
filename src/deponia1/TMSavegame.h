// Not yet assert-confirmed to a specific file; stays at the top level.
// A single savegame slot's metadata/IO. Confirmed constructor signature and
// call shapes only (TGameControl::SavegameExists/DeleteSavegame,
// Deponia_Linux.asm lines 462562-462773): constructed with (isNumberedSlot,
// slot, b, c, game) - b/c always passed 0 at every call site seen so far,
// meaning unresolved. SavegameExists() is called without a real "this"
// object at its one call site, so modeled as static (matching the
// TGAction::AddRunningAction/ClearActions pattern elsewhere).
#pragma once

#include <vector>

#include "TManagedObject.h"
#include "TXMLWriter.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"

class TVisionaireGame;

// Confirmed a TManagedObject subclass (its recovered RTTI/vtable overrides
// ExecuteEvent/SetActive/Draw/GetActionList; 0x1E8 bytes in all, per the
// `operator new` size at TGScene::GetSelectedSavegame()'s call site) - the
// base is all this pass confirms; the rest of the class is still stubbed.
class TMSavegame : public TManagedObject {
public:
	TMSavegame(bool isNumberedSlot, int slot, int b, int c, TVisionaireGame *game);
	~TMSavegame() override = default;

	// Confirmed call shapes only (TGScene::Prepare()/Draw(), Deponia_Linux.asm
	// lines 166435-166997): called each frame with the on-screen slot's
	// bounding rect (and, for DrawText(), after Draw()); not reversed beyond
	// that.
	void SetScreenshotRect(const wxRect &rect);
	void DrawText();
	// Confirmed static (TGScene::SetSavegames(), asm line 170896+ - called
	// with no object, a vector of slot numbers to fill, and the result
	// tested as a bool).
	static bool GetExistingSaveGames(std::vector<int> &outSlots);

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
	// Confirmed call shape only (TGameControl::SaveGame, asm line 463092) -
	// writes this slot's data out via writer; not reversed beyond that.
	void SaveGame(const TBufferedProjectFileWriter &writer);
	// Confirmed call shape only (TGameControl::LoadGame(TMSavegame*),
	// Deponia_Linux.asm lines 477503-477522, 477754, 477864 and elsewhere) -
	// the composed/container file this savegame lives in (possibly empty for
	// a loose-file savegame) and its own on-disk file name, respectively; not
	// reversed beyond that.
	wxString GetSavegameComposedFile() const;
	wxString GetFileName() const;
};
