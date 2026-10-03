// Original path confirmed via x_assert() calls: src/datastruct/typegrp.cpp.
//
// Implemented in full (Deponia_Linux.asm lines 584546-586100, all 30
// manifest-listed methods). A TTypeGroup is one "record type" of the game
// data's schema: the ordered list of TTypeData fields a record of that type
// has, which tables/versions they belong to, the byte sizes the record's
// persistent and temporary storage need, and the create/init callbacks
// (and optional list-name callback) that give a fresh record its defaults.
// Every TT* data-record class (TTScene, TTEvent, ...) owns one static
// TTypeGroup, built in its static initializer and (re)filled by its
// InitType().
//
// Real layout (0x80 bytes): the field list (+0x00, a vector of TTypeData*
// owned by the group), the "needed" subset of it (+0x18, see
// SetupNeededTypes() - a second vector whose begin/end/capacity are at
// +0x18/0x20/0x28), the persistent and temporary storage sizes (+0x30/0x34),
// the file-version range the group was last set up for (+0x38/0x3C), the
// table it describes (+0x40), its eSaveGame type (+0x44, default 2 = both),
// its description id (+0x48), its game-data version in/out (+0x4C/0x50, -1
// = open) and savegame version in/out (+0x54/0x58, default -1), the three
// callbacks (+0x60/0x68/0x70) and a "fields have been scrambled" flag
// (+0x78).
//
// Fields are found by their description id through the global lookup table
// (g_lookupTable): one 12-byte entry per possible field id naming the group
// that currently owns it and its index in that group's list.
#pragma once

#include <vector>

#include "WxStub.h"
#include "datastruct/type.h"
#include "datastruct/visionaire.h"

class TVedFile;
class TVisionaireObject;

// Confirmed layout (TTypeGroup::GetTypeData(), asm lines 585134-585293;
// the entry stride of 12 and `numEntries`/`pEntries` names come from the
// original's own assert text).
struct TLookupEntry {
	int typeElement = 0;
	int index = 0;
	bool valid = false;
};

struct TLookupTable {
	TLookupEntry *pEntries = nullptr;
	int numEntries = 0;
};

// Confirmed a real, named global (recovered symbol; the original keeps the
// pointer and count as two adjacent globals).
extern TLookupTable g_lookupTable;

class TTypeGroup {
public:
	typedef void (*CreateFunction)(TVisionaireObject *);
	typedef wxString (*NameFunction)(const TVisionaireObject *);

	TTypeGroup(int description, eVisionaireTable table, int versionIn, int versionOut);
	TTypeGroup(int description, eVisionaireTable table, eSaveGame saveGameType, int versionIn, int versionOut,
	           int saveVersionIn, int saveVersionOut);
	// Confirmed to be an intentional trap (asm lines 584849, 585714): both
	// assert(false) - a group is never copied.
	TTypeGroup(const TTypeGroup &other);
	TTypeGroup &operator=(const TTypeGroup &other);
	~TTypeGroup();

	void SetCreate(CreateFunction create) {
		_onCreate = create;
	}
	void SetInit(CreateFunction init) {
		_onInit = init;
	}
	void SetNameInList(NameFunction name) {
		_nameInList = name;
	}
	// Run the matching callback on a record, if one is set.
	void OnCreate(TVisionaireObject *object) const;
	void OnInit(TVisionaireObject *object) const;
	// Fills `outName` from the list-name callback; false if there is none.
	bool GetNameInList(const TVisionaireObject *object, wxString &outName) const;

	// Frees every field and starts the group over for the file-version range
	// [versionLow, versionHigh]; storage sizes and the scrambled flag reset.
	void ClearTypes(int versionLow, int versionHigh);
	// Adds a field if its version range suits the current one, laying out
	// its storage (see the .cpp for the exact rules) and registering it in
	// g_lookupTable.
	void AddType(const TTypeData &type);
	// Rebuilds the "needed" list - the stored (and optionally non-temporary,
	// optionally link-only) fields that fit a savegame or game-data request.
	void SetupNeededTypes(bool forSaveGame, bool linkTypesOnly, bool skipTemp);
	// Deterministically shuffles the field order using `seed` (once only),
	// keeping g_lookupTable in step - used to hide the file layout.
	void SetScrambled(int seed);

	const std::vector<TTypeData *> &GetTypes() const {
		return _types;
	}
	const std::vector<TTypeData *> &GetNeededTypes() const {
		return _neededTypes;
	}
	eVisionaireTable GetTypeElement() const {
		return static_cast<eVisionaireTable>(_table);
	}
	int GetDescription() const {
		return _description;
	}
	int GetVersionIn() const {
		return _versionIn;
	}
	int GetVersionOut() const {
		return _versionOut;
	}
	eSaveGame GetSaveGameType() const {
		return _saveGameType;
	}
	int GetSizeData() const {
		return _sizeData;
	}
	int GetSizeTempData() const {
		return _sizeTempData;
	}
	bool IsFittingSaveGameType(bool forSaveGame) const;
	bool AppliesToFileVersion(const TVedFile &file) const;

	// Field lookup by description id. GetTypeData() logs and returns a shared
	// "no type" placeholder (description -1) when this group has no such
	// field; GetTypeDataPtr() returns null instead; GetType() returns -1
	// (logging only when `logMissing`); GetTypeLink() returns -2 (not a
	// link / not found).
	const TTypeData &GetTypeData(int description) const;
	TTypeData *GetTypeDataPtr(int description) const;
	eTypeData GetType(int description, bool logMissing) const;
	int GetTypeLink(int description) const;

private:
	std::vector<TTypeData *> _types;
	std::vector<TTypeData *> _neededTypes;
	int _sizeData = 0;
	int _sizeTempData = 0;
	int _versionLow = 0;
	int _versionHigh = 0;
	int _table;
	eSaveGame _saveGameType = eSaveGame::kValue2;
	int _description;
	int _versionIn;
	int _versionOut;
	int _saveVersionIn = -1;
	int _saveVersionOut = -1;
	CreateFunction _onCreate = nullptr;
	CreateFunction _onInit = nullptr;
	NameFunction _nameInList = nullptr;
	bool _scrambled = false;
};
