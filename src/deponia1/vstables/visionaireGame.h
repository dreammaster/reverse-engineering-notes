// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vstables/visionaireGame.cpp - see manifest/source_layout.tsv.
// Heap-allocated and owned by TGameControl (confirmed from its ctor);
// exposed via GetVisionaire()/GetGameSystem().
#pragma once

#include "datastruct/visobjref.h"

class TVList;

class TVisionaireGame {
public:
	TVisionaireGame();

	// Confirmed (Deponia_Linux.asm lines 1480547+): registers the data-field
	// XML names (ids 100 up) with TXMLNames, after TXMLNames::
	// InitXMLNamesIntern() has registered ids 1-99; runs once. Called by the
	// constructor.
	static void InitXMLNames();

	// Confirmed called directly on whatever GetGameSystem()/GetVisionaire()
	// returns, with the exact same call shape TVisionaire::GetGame()/
	// GetEmptyObject() use everywhere else in this codebase (e.g.
	// TGObjectManager::ResetEventInfo/GetCurrentObject, Deponia_Linux.asm
	// lines 187267-187315, 189315-189352) - another data point for the
	// standing "may really be the same underlying object" gap noted above
	// (see LoadDataGame/LoadSaveGame's own comments on TVisionaire),
	// rather than behavior genuinely new to this class.
	TVisObjRef GetGame() const;
	TVisObjRef GetEmptyObject() const;
	// Confirmed call shape only (TGScene::InitActionAreas(), Deponia_Linux.asm
	// line 169215+) - identical to TVisionaire::GetList() (datastruct/
	// visionaire.h), the same standing "may really be the same object" gap
	// as GetGame() above.
	void GetList(int fieldId, TVList &outList, bool flag) const;
	// Confirmed call shape only (TMSavegame::SetActive()/CheckVisPaths(),
	// Deponia_Linux.asm lines 160840-162327) - loads a savegame file into this
	// object; the same standing "same object as TVisionaire" gap as GetList()
	// above (see TVisionaire::LoadSaveGame()).
	bool LoadSaveGame(const wxFileName &file, const wxString &extra);
};

// Confirmed a free function, not a member (TGameControl::Save, asm line
// 462914) - not reversed beyond that call shape.
void SaveGlobalScriptVariables(TVisionaireGame &game);
// The load-side counterpart (TGameControl::Load, Deponia_Linux.asm line
// 477015) - not reversed beyond that call shape. Notable: this call site's
// argument comes from the SAME field offset (TGameControl+0xC8) that every
// other call in TGameControl::Load/LoadGame reads as `_visionaire` (a
// TVisionaire*) via confirmed TVisionaire::GetGame()/GetList() calls - but a
// free function's parameter type is unambiguous from its own mangled name,
// so this one spot is modeled with the already-existing `_visionaireGame`
// member instead (matching TGameControl::Save's own already-shipped
// SaveGlobalScriptVariables(*_visionaireGame) call). Left as two distinct
// members rather than resolved into one, same as the standing TVisionaire/
// TVisionaireGame "not yet integrated" gap (see LoadDataGame/LoadSaveGame's
// own comments) - but this is a second, independent data point suggesting
// they may really be the same underlying object in the original binary.
void LoadGlobalScriptVariables(TVisionaireGame &game);
