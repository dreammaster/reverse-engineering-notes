// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/datastruct/type.cpp - see manifest/source_layout.tsv.
//
// Implemented in full (Deponia_Linux.asm lines 668535-668960, all 19
// manifest-listed methods). TTypeData describes one field of a data-record
// type (see TTypeGroup): which field id it is, what kind of value it holds,
// which linked table it points into (if it's a link), the file-format
// versions it exists in (for game data and, separately, for savegames),
// whether it's stored in the file or only held temporarily, and its byte
// offset in the record's storage.
//
// Real layout (0x28 bytes): description (+0x00, the field id), link table
// (+0x04, default -2 = "not a link"), data type (+0x08), version in/out
// (+0x0C/0x10), savegame version in/out (+0x14/0x18, default -1 = open),
// savegame type (+0x1C, an eSaveGame value, default 0), data offset (+0x20)
// and two flags (+0x24 temporary, +0x25 stored in the file).
#pragma once

#include "datastruct/visenums.h"

// The 19 kinds of value a field can hold (0-18). The kind decides how the
// field's storage in a record is laid out and handled: see the switch tables
// in TData (data.h), which this numbering and the names below were read off
// (type 5 has no storage and no behavior anywhere; its name is invented).
// Link fields (kLink/kLinkList) are the ones TTypeGroup::SetupNeededTypes()
// treats specially.
enum class eTypeData : int {
	kBool = 0,
	kInt = 1,
	kString = 2,
	kPath = 3,
	kFloat = 4,
	kUnused5 = 5,
	kRectList = 6,
	kSpriteList = 7,
	kPointList = 8,
	kStringList = 9,
	kIntList = 10,
	kPathList = 11,
	kFloatList = 12,
	kPoint = 13,
	kRect = 14,
	kSprite = 15,
	kLink = 16,
	kLinkList = 17,
	kTextList = 18
};

class TTypeData {
public:
	// A link field: `link` is the table it points into.
	TTypeData(int description, eTypeData type, eVisionaireTable link, int versionIn, int versionOut);
	// A plain value field (link = -2).
	TTypeData(int description, eTypeData type, int versionIn, int versionOut);
	// A field with its own savegame version range. (The original asserts that
	// a savegame-only field has versionIn <= 1 and an open versionOut; see
	// the .cpp.)
	TTypeData(int description, eTypeData type, eSaveGame savegame, int versionIn, int versionOut,
	          int saveVersionIn, int saveVersionOut);
	TTypeData(const TTypeData &other) = default;
	TTypeData &operator=(const TTypeData &other) = default;
	~TTypeData() = default;

	int GetDescription() const {
		return _description;
	}
	eTypeData GetType() const {
		return _type;
	}
	int GetTypeLink() const {
		return _link;
	}
	bool IsTempType() const {
		return _isTemp;
	}
	bool IsInFile() const {
		return _inFile;
	}
	int GetOffset() const {
		return _offset;
	}
	// Confirmed (asm lines 668776-668820): savegame fields (type 1) fit a
	// savegame request, game fields (type 0) fit a game-data request, and
	// "both" fields (type 2) fit either.
	bool IsFittingSaveGameType(bool forSaveGame) const {
		int type = static_cast<int>(_saveGameType);
		return forSaveGame ? (type == 1 || type == 2) : (type == 0 || type == 2);
	}
	eSaveGame GetSaveGameType() const {
		return _saveGameType;
	}
	int GetVersionIn() const {
		return _versionIn;
	}
	int GetVersionOut() const {
		return _versionOut;
	}
	int GetVersionSaveGameIn() const {
		return _saveVersionIn;
	}
	int GetVersionSaveGameOut() const {
		return _saveVersionOut;
	}
	// Confirmed (asm lines 668906-668932): two fields are the same field if
	// they have the same data type and field id.
	bool operator==(const TTypeData &other) const {
		return _type == other._type && _description == other._description;
	}

	// Set by TTypeGroup::AddType() as it lays the record's storage out.
	void SetOffset(int offset) {
		_offset = offset;
	}
	void SetTemp(bool temp) {
		_isTemp = temp;
	}
	void SetInFile(bool inFile) {
		_inFile = inFile;
	}

private:
	int _description;
	int _link = -2;
	eTypeData _type;
	int _versionIn;
	int _versionOut;
	int _saveVersionIn = -1;
	int _saveVersionOut = -1;
	eSaveGame _saveGameType = eSaveGame::kValue0;
	int _offset = 0;
	bool _isTemp = false;
	bool _inFile = false;
};
