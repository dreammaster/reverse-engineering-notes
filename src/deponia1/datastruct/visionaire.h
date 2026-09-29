// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/datastruct/visionaire.cpp - see manifest/source_layout.tsv.
//
// TVisionaire is the engine's root game-data object; only the one accessor
// TMasterControl needs is stubbed here.
#pragma once

#include "TXMLWriter.h"
#include "datastruct/vlist.h"
#include "datastruct/visobjref.h"

// Confirmed one value, 0x22 (TGameControl::SaveGame, asm line 463095) - real
// meaning/other values not resolved (named for its raw value like TypeOrder,
// rather than guessing a table name).
enum class eVisionaireTable { kValue34 = 0x22 };

class TVisionaire {
public:
	TVisObjRef GetGame() const;

	// Confirmed call shape only (TGameControl::SaveGame, asm line 463089) -
	// writes the game's current state out; not reversed beyond that.
	void SaveSaveGame(TProjectFileWriter &writer);
	// Confirmed call shape only (TGameControl::SaveGame, asm line 463096).
	void ResetActiveData(eVisionaireTable table);

	// Confirmed called with a field id, an out-param list, and a bool flag
	// (TGameControl::InitFonts, asm lines 458293-458322) - not reversed
	// beyond that call shape.
	void GetList(int fieldId, TVList &outList, bool flag) const;

	// Confirmed called with a type id and a source TVisObjRef, returning a
	// new TVisObjRef by value (TGameControl::StartObjectText, asm lines
	// 462233-462396) - "creates a new game-data object of this type,
	// presumably linked to/copied from the source" is a reasonable guess
	// from the name and call shape, but not confirmed beyond that.
	TVisObjRef CreateActiveObject(int typeId, const TVisObjRef &source);

	// Confirmed to return a TVisObjRef by value, presumably a fixed "empty"
	// sentinel object (TGameControl::StartBackgroundText, asm lines
	// 461420-461589) - not reversed beyond that call shape.
	TVisObjRef GetEmptyObject() const;
};
