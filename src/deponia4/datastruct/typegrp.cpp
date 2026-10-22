#include "datastruct/typegrp.h"

#include "Diagnostics.h"
#include "datastruct/data.h"
#include "datastruct/vedfile.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/datastruct/typegrp.cpp";

TLookupTable g_lookupTable;

TTypeGroup::TTypeGroup(int description, eVisionaireTable table, int versionIn, int versionOut)
	: _table(static_cast<int>(table)), _description(description), _versionIn(versionIn),
	  _versionOut(versionOut) {
}

TTypeGroup::TTypeGroup(int description, eVisionaireTable table, eSaveGame saveGameType, int versionIn,
                       int versionOut, int saveVersionIn, int saveVersionOut)
	: _table(static_cast<int>(table)), _saveGameType(saveGameType), _description(description),
	  _versionIn(versionIn), _versionOut(versionOut), _saveVersionIn(saveVersionIn),
	  _saveVersionOut(saveVersionOut) {
}

TTypeGroup::TTypeGroup(const TTypeGroup &/*other*/) : _table(0), _description(0), _versionIn(0), _versionOut(0) {
	x_assert(false, "false", kSourceFile, 0xC6);
}

TTypeGroup &TTypeGroup::operator=(const TTypeGroup &/*other*/) {
	x_assert(false, "false", kSourceFile, 0xC6);
	return *this;
}

TTypeGroup::~TTypeGroup() {
	for (TTypeData *type : _types)
		delete type;
}

void TTypeGroup::OnCreate(TVisionaireObject *object) const {
	if (_onCreate)
		_onCreate(object);
}

void TTypeGroup::OnInit(TVisionaireObject *object) const {
	if (_onInit)
		_onInit(object);
}

bool TTypeGroup::GetNameInList(const TVisionaireObject *object, wxString &outName) const {
	if (!_nameInList)
		return false;
	outName = _nameInList(object);
	return true;
}

void TTypeGroup::ClearTypes(int versionLow, int versionHigh) {
	for (TTypeData *type : _types)
		delete type;
	_types.clear();
	_versionLow = versionLow;
	_versionHigh = versionHigh;
	_sizeData = 0;
	_sizeTempData = 0;
	_scrambled = false;
}

bool TTypeGroup::IsFittingSaveGameType(bool forSaveGame) const {
	int type = static_cast<int>(_saveGameType);
	return forSaveGame ? (type == 1 || type == 2) : (type == 0 || type == 2);
}

bool TTypeGroup::AppliesToFileVersion(const TVedFile &file) const {
	bool isSaveGame = file.IsSaveGame();
	if (!IsFittingSaveGameType(isSaveGame))
		return false;
	return isSaveGame ? file.GetVersionOk(_saveVersionIn, _saveVersionOut)
	                  : file.GetVersionOk(_versionIn, _versionOut);
}

// Confirmed (asm lines 585963-586100). The version range a field is in
// (its savegame range for a savegame-only field; for a "both" field, the
// game range widened to its savegame range wherever the savegame one is
// open or starts later/ends later) decides three things against the range
// this group is currently set up for ([_versionLow, _versionHigh]):
//
//  - whether it's added at all: a field that starts at or before the low
//    end must still be in force past it (or be open-ended); one that starts
//    later must reach past the high end (or be open-ended);
//  - whether it's a temporary field (not stored in the file): it ends
//    before the high end;
//  - whether it's stored in the file: it starts at or before the low end
//    and is in force past it (or open-ended).
//
// Persistent fields take their storage offset from _sizeData, temporary
// ones from _sizeTempData, each then grown by the field's size (TData::
// GetDataSize()). The original asserts g_lookupTable.pEntries is non-null
// and dereferences it unchecked; this reports and gives up instead.
void TTypeGroup::AddType(const TTypeData &typeData) {
	TTypeData data(typeData);
	int versionIn = data.GetVersionIn();
	int versionOut = data.GetVersionOut();

	if (static_cast<int>(data.GetSaveGameType()) == 1) {
		versionIn = data.GetVersionSaveGameIn();
		versionOut = data.GetVersionSaveGameOut();
	} else if (static_cast<int>(data.GetSaveGameType()) == 2) {
		if (versionIn != -1 && (data.GetVersionSaveGameIn() == -1 || versionIn < data.GetVersionSaveGameIn()))
			versionIn = data.GetVersionSaveGameIn();
		if (versionOut != -1 && (data.GetVersionSaveGameOut() == -1 || versionOut < data.GetVersionSaveGameOut()))
			versionOut = data.GetVersionSaveGameOut();
	}

	// The high end of the range decides when the low end doesn't settle it.
	bool fitsHighEnd = versionIn <= _versionHigh && (versionOut > _versionHigh || versionOut == -1);
	bool add;
	if (versionIn <= _versionLow && (versionOut > _versionLow || versionOut == -1))
		add = true;
	else
		add = fitsHighEnd;
	if (!add)
		return;

	x_assert(g_lookupTable.pEntries != nullptr, "g_lookupTable.pEntries != NULL", kSourceFile, 0x9C);
	if (!g_lookupTable.pEntries)
		return;

	TLookupEntry &entry = g_lookupTable.pEntries[data.GetDescription()];
	entry.index = static_cast<int>(_types.size());
	entry.valid = true;
	entry.typeElement = _table;

	if (versionOut <= _versionHigh && versionOut != -1) {
		data.SetOffset(_sizeTempData);
		data.SetTemp(true);
		data.SetInFile(true);
		_sizeTempData += TData::GetDataSize(data.GetType());
	} else {
		bool inFile = versionIn <= _versionLow && (versionOut > _versionLow || versionOut == -1);
		data.SetOffset(_sizeData);
		data.SetTemp(false);
		data.SetInFile(inFile);
		_sizeData += TData::GetDataSize(data.GetType());
	}
	_types.push_back(new TTypeData(data));
}

// Confirmed (asm lines 585762-585963).
void TTypeGroup::SetupNeededTypes(bool forSaveGame, bool linkTypesOnly, bool skipTemp) {
	_neededTypes.clear();
	if (!IsFittingSaveGameType(forSaveGame))
		return;

	for (TTypeData *type : _types) {
		if (linkTypesOnly && type->GetType() != eTypeData::kLink && type->GetType() != eTypeData::kLinkList)
			continue;
		if (!type->IsFittingSaveGameType(forSaveGame) || !type->IsInFile())
			continue;
		if (skipTemp && type->IsTempType())
			continue;
		_neededTypes.push_back(type);
	}
}

// Confirmed (asm lines 585525-585639): each field's description plus the
// seed picks a partner slot in 1..n-1; where that slot is later than the
// field's own, the two fields swap places (and their lookup entries'
// indices follow). Done only once per group, and only with at least two
// fields.
void TTypeGroup::SetScrambled(int seed) {
	if (!_scrambled && _types.size() > 1) {
		for (int i = 0; i < static_cast<int>(_types.size()); i++) {
			int count = static_cast<int>(_types.size());
			int partner = (_types[i]->GetDescription() + seed) % (count - 1) + 1;
			if (partner > i && count > partner) {
				TTypeData *first = _types[i];
				TTypeData *second = _types[partner];
				g_lookupTable.pEntries[first->GetDescription()].index = partner;
				g_lookupTable.pEntries[second->GetDescription()].index = i;
				_types[i] = second;
				_types[partner] = first;
			}
		}
	}
	_scrambled = true;
}

// Confirmed (asm lines 585134-585293). `descr`'s lookup entry must be in
// range (the original asserts; a missing/foreign entry logs "T" - the
// original's own, unexplained message text - and returns the shared
// placeholder).
const TTypeData &TTypeGroup::GetTypeData(int description) const {
	static const TTypeData noType(-1, static_cast<eTypeData>(-1), 0, 0);

	x_assert(description >= 0 && description < g_lookupTable.numEntries,
	         "descr >= 0 && descr < g_lookupTable.numEntries", kSourceFile, 0x131);
	if (description >= 0 && description < g_lookupTable.numEntries) {
		const TLookupEntry &entry = g_lookupTable.pEntries[description];
		if (entry.valid && entry.typeElement == _table)
			return *_types[entry.index];
	}

	x_assert(false, "false", kSourceFile, 0x13A);
	if (wxLog::loglevel > 0)
		wxLog::logexpanded(L"T");
	return noType;
}

// Confirmed (asm lines 585293-585339).
TTypeData *TTypeGroup::GetTypeDataPtr(int description) const {
	if (description < 0 || description >= g_lookupTable.numEntries)
		return nullptr;
	const TLookupEntry &entry = g_lookupTable.pEntries[description];
	if (!entry.valid || entry.typeElement != _table)
		return nullptr;
	return _types[entry.index];
}

// Confirmed (asm lines 585339-585473).
eTypeData TTypeGroup::GetType(int description, bool logMissing) const {
	x_assert(description >= 0 && description < g_lookupTable.numEntries,
	         "descr >= 0 && descr < g_lookupTable.numEntries", kSourceFile, 0x153);
	if (description >= 0 && description < g_lookupTable.numEntries) {
		const TLookupEntry &entry = g_lookupTable.pEntries[description];
		if (entry.valid && entry.typeElement == _table)
			return _types[entry.index]->GetType();
	}

	if (logMissing) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"T");
		x_assert(false, "false", kSourceFile, 0x15E);
	}
	return static_cast<eTypeData>(-1);
}

// Confirmed (asm lines 585473-585525): linear search by description.
int TTypeGroup::GetTypeLink(int description) const {
	for (const TTypeData *type : _types) {
		if (type->GetDescription() == description)
			return type->GetTypeLink();
	}
	return -2;
}
