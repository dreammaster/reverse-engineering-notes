// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/datastruct/visionaire.cpp - see manifest/source_layout.tsv.
//
// TVisionaire is the engine's root game-data object; only the one accessor
// TMasterControl needs is stubbed here.
#pragma once

#include "TSignalSlot.h"
#include "TXMLWriter.h"
#include "datastruct/vlist.h"
#include "datastruct/visobjref.h"

class EventHandler;

// Confirmed one value, 0x22 (TGameControl::SaveGame, asm line 463095) - real
// meaning/other values not resolved (named for its raw value like TypeOrder,
// rather than guessing a table name).
enum class eVisionaireTable { kValue34 = 0x22 };

// Confirmed 2 values, 0 and 1 (TGameControl::PreLoad/LoadAndInitGame, asm
// lines 464227, 467804) - real meaning/other values not resolved.
enum class TLoadingTypeEnum { kValue0 = 0, kValue1 = 1 };

// Confirmed one value, 0 (TGameControl::PreLoad, Deponia_Linux.asm line
// 464228) - real meaning/other values not resolved.
enum class eSaveGame { kValue0 = 0 };

class TVisionaire {
public:
	TVisObjRef GetGame() const;

	// Confirmed call shape only (TGameControl::SaveGame, asm line 463089) -
	// writes the game's current state out; not reversed beyond that.
	void SaveSaveGame(TProjectFileWriter &writer);
	// Confirmed call shape only (TGameControl::SaveGame, asm line 463096).
	void ResetActiveData(eVisionaireTable table);

	// Confirmed call shape only (TGameControl::LoadAndInitGame, asm lines
	// 467799-467810) - returns a success bool (tested with a plain bool
	// check, not compared against a specific value). IDA resolves the real
	// symbol as `TVisionaireGame::LoadDataGame`, but the confirmed `this`
	// pointer at that call site is _visionaire (TVisionaire*), not
	// _visionaireGame (TVisionaireGame*) - the same kind of not-yet-
	// integrated-class gap as THGameControl (see NOTES.md); placed on
	// TVisionaire, matching the confirmed pointer, rather than guessed onto
	// TVisionaireGame.
	bool LoadDataGame(const wxFileName &file, const wxString &extra, TLoadingTypeEnum type, bool flag,
	                  TSignalSlot *slot, EventHandler *handler);
	// Confirmed call shape only (TGameControl::PreLoad, Deponia_Linux.asm
	// lines 464222-464231) - distinct from LoadDataGame (an extra eSaveGame
	// parameter, and an extra int* out-param); not reversed beyond that.
	bool Load(const wxFileName &file, const wxString &extra, eSaveGame saveGame, TLoadingTypeEnum type,
	          int *outFlag, TSignalSlot *slot, EventHandler *handler);

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
