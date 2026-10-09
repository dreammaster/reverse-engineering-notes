#include "datastruct/visionaire.h"

#include <algorithm>

#include "Diagnostics.h"
#include <EventHandler.h>
#include "LoadSaveProgressEvent.h"
#include "TXMLNames.h"
#include "baselib/xmlWriter.h"
#include "datastruct/link.h"
#include "TComposedFileManager.h"
#include "TTimer.h"
#include "datastruct/table.h"
#include "datastruct/vedfile.h"
#include "datastruct/visionaireobject.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/datastruct/visionaire.cpp";

bool TVisionaire::IsVisPlayerMode = false;
int TVisionaire::ActiveInstances = 0;
wxString TVisionaire::s_tableNameInvalid(L"<invalid table>");

static int s_newGameId = 0;

static bool isEmptyId(const TId &id) {
	return id.getId() == -1 && id.getTable() == 0xFF;
}

// The link table's order: by target, then owner, then field.
static bool linkRefLess(const TLinkRef &a, const TLinkRef &b) {
	if (a.to.getTable() != b.to.getTable())
		return (signed char)a.to.getTable() < (signed char)b.to.getTable();
	if (a.to.getId() != b.to.getId())
		return a.to.getId() < b.to.getId();
	if (a.from.getTable() != b.from.getTable())
		return (signed char)a.from.getTable() < (signed char)b.from.getTable();
	if (a.from.getId() != b.from.getId())
		return a.from.getId() < b.from.getId();
	return a.field < b.field;
}

// Confirmed (asm lines 614163-614495)
TVisionaire::TVisionaire(const TTypeGroup &gameTypeGroup, int version)
	: _gameId(s_newGameId++), _version(version), _projectType(-1), _revision(-1), _linksCount(-1),
	  _hasIdMapping(false), _modified(false), _shuttingDown(false), _destroying(false), _mainObject(nullptr),
	  _gameTypeGroup(&gameTypeGroup), _emptyObject(nullptr), _anyObject(nullptr), _poolSize(0) {
	ActiveInstances++;
	_emptyObject = new TVisionaireObject(this);
	_anyObject = new TVisionaireObject(true, this);
}

// Confirmed (asm lines 613714-614162)
TVisionaire::~TVisionaire() {
	_destroying = true;
	ActiveInstances--;
	Clear();
	_shuttingDown = true;

	for (TTable *table : _tableList)
		delete table;
	_tableList.clear();
	_tables.clear();
	for (TTableNamesEntry *names : _tableNames)
		delete names;
	_tableNames.clear();

	if (_mainObject) {
		_mainObject->Remove(false, true);
		_mainObject = nullptr;
	}
	if (_emptyObject) {
		_emptyObject->Remove(false, true);
		_emptyObject = nullptr;
	}
	if (_anyObject) {
		_anyObject->Remove(false, true);
		_anyObject = nullptr;
	}
}

// Confirmed (asm lines 608930-608955)
void TVisionaire::CleanUp() {
	TXMLNames::CleanUp();
	delete[] g_lookupTable.pEntries;
	g_lookupTable.pEntries = nullptr;
}

// Confirmed (asm lines 608956-609039)
void TVisionaire::Clear() {
	_shuttingDown = true;
	x_assert(_mappedIds.empty(), "MappedIds.empty()", kSourceFile, 0x74);
	x_assert(_tempParentLinks.empty(), "TempParentLinkRefs.empty()", kSourceFile, 0x75);

	if (_mainObject && _mainObject->GetData()) {
		TTypeGroup *group = _mainObject->GetData()->GetTypeGroupPtrNonConst();
		if (group) {
			group->SetupNeededTypes(false, false, false);
			_mainObject->GetData()->Clear();
		}
	}

	for (TTable *table : _tableList) {
		TTypeGroup *group = table->GetTypeGroup();
		if (group) {
			group->SetupNeededTypes(false, false, false);
			table->Clear();
		}
	}

	_links.clear();
	_shuttingDown = false;
}

// Confirmed (asm lines 610941-611004)
bool TVisionaire::NewGame() {
	if (!_mainObject)
		_mainObject = new TVisionaireObject(1, 1, -1, this, _gameTypeGroup, false);

	Clear();
	_path = wxFileName();
	_modified = false;
	return true;
}

// Confirmed (asm lines 609073-609110)
TVisObjRef TVisionaire::GetGame() const {
	x_assert(_mainObject != nullptr, "MainObject != NULL", kSourceFile, 0x99);
	return TVisObjRef(*_mainObject);
}

// Confirmed (asm lines 609126-609186)
TVisObjRef TVisionaire::GetEmptyObject() const {
	return TVisObjRef(_emptyObject);
}

TVisObjRef TVisionaire::GetAnyObject() const {
	x_assert(_anyObject != nullptr, "AnyObject != NULL", kSourceFile, 0xA9);
	return TVisObjRef(_anyObject);
}

// The table a TId's table byte (a signed number) names, or -1.
int TVisionaire::tableIndex(const TId &id) const {
	int index = (signed char)id.getTable();
	if (index < 0 || index >= (int)_tables.size())
		return -1;
	return index;
}

// Confirmed (asm lines 615757-616050)
void TVisionaire::CreateTables(int count) {
	_tables.assign(count, nullptr);
	_tableList.clear();
	for (TTableNamesEntry *names : _tableNames)
		delete names;
	_tableNames.assign(count, nullptr);
}

// Confirmed (asm lines 614808-615477)
bool TVisionaire::AddTable(int description, int identifier, const wxString &visName, const wxString &singular,
                           const wxString &plural, const TTypeGroup &typeGroup, int activeLinkField, bool flag) {
	x_assert(identifier < (int)_tables.size(), "identifier < (int) Tables.size()", kSourceFile, 0x504);
	if (identifier < 0 || identifier >= (int)_tables.size())
		return false;

	if (_tables[identifier]) {
		x_assert(false, "false", kSourceFile, 0x508);
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"TVisionaire::AddTable: table %ls (id: %d) already exists", visName.c_str(),
			                   identifier);
		return false;
	}

	TTable *table = new TTable(this, description, identifier, visName, _gameId,
	                           const_cast<TTypeGroup *>(&typeGroup), activeLinkField, flag);
	_tables[identifier] = table;
	_tableList.push_back(table);

	_visNameIds[visName.ToStdWstring()] = identifier;
	_singularIds[singular.ToStdWstring()] = identifier;
	_pluralIds[plural.ToStdWstring()] = identifier;

	TTableNamesEntry *names = new TTableNamesEntry;
	names->visName = visName;
	names->singular = singular;
	names->plural = plural;
	delete _tableNames[identifier];
	_tableNames[identifier] = names;
	return true;
}

// Confirmed (asm lines 611594-611652)
bool TVisionaire::GetTable(int identifier, TTable **outTable) const {
	if (identifier < 0 || identifier >= (int)_tables.size())
		return false;
	*outTable = _tables[identifier];
	return true;
}

bool TVisionaire::GetTableConst(int identifier, const TTable **outTable) const {
	if (identifier < 0 || identifier >= (int)_tables.size())
		return false;
	*outTable = _tables[identifier];
	return true;
}

// Confirmed (asm lines 609369-609451)
const TTypeGroup *TVisionaire::GetTypeGroup(int identifier, int &description) const {
	const TTypeGroup *group = nullptr;

	if (identifier >= 0 && identifier < (int)_tables.size() && _tables[identifier] &&
	    _tables[identifier]->GetTypeGroup(description, &group))
		return group;

	if (wxLog::loglevel >= 0)
		wxLog::logexpanded(L"TVisionaire::GetTypeGroup: no table %d", identifier);
	return group;
}

// Confirmed (asm lines 612764-612823)
void TVisionaire::CompleteTables() const {
	for (const TTable *table : _tables)
		x_assert(table != nullptr, "*node != NULL", kSourceFile, 0x53A);
	for (const TTable *table : _tableList)
		x_assert(table != nullptr, "*node != NULL", kSourceFile, 0x53E);
}

// Confirmed (asm lines 611653-612074)
const wxString &TVisionaire::GetVisTableName(int identifier, bool warn) const {
	static const wxString game(L"Game");

	if (identifier == -1)
		return game;
	if (identifier >= 0 && identifier < (int)_tableNames.size() && _tableNames[identifier])
		return _tableNames[identifier]->visName;

	if (warn && wxLog::loglevel > 0)
		wxLog::logexpanded(L"TVisionaire::GetVisTableName: invalid table %d", identifier);
	x_assert(false, "false", kSourceFile, 0x48B);
	return s_tableNameInvalid;
}

const wxString &TVisionaire::GetTableNameSingular(int identifier, bool warn) const {
	static const wxString game(L"Game");

	if (identifier == -1)
		return game;
	if (identifier >= 0 && identifier < (int)_tableNames.size() && _tableNames[identifier])
		return _tableNames[identifier]->singular;

	if (warn && wxLog::loglevel > 0)
		wxLog::logexpanded(L"TVisionaire::GetTableNameSingular: invalid table %d", identifier);
	x_assert(false, "false", kSourceFile, 0x48B);
	return s_tableNameInvalid;
}

const wxString &TVisionaire::GetTableNamePlural(int identifier, bool warn) const {
	static const wxString game(L"Game");

	if (identifier == -1)
		return game;
	if (identifier >= 0 && identifier < (int)_tableNames.size() && _tableNames[identifier])
		return _tableNames[identifier]->plural;

	if (warn && wxLog::loglevel > 0)
		wxLog::logexpanded(L"TVisionaire::GetTableNamePlural: invalid table %d", identifier);
	x_assert(false, "false", kSourceFile, 0x48B);
	return s_tableNameInvalid;
}

static int lookupTableName(const std::map<std::wstring, int> &ids, const wxString &name, bool warn) {
	if (name.Cmp(wxString(L"Game")) == 0)
		return -1;

	auto it = ids.find(name.ToStdWstring());
	if (it != ids.end())
		return it->second;

	if (warn && wxLog::loglevel > 0)
		wxLog::logexpanded(L"TVisionaire: no table named %ls", name.c_str());
	return -2;
}

// Confirmed (asm lines 612075-612657)
int TVisionaire::GetTableIdentifierByVisName(const wxString &name, bool warn) const {
	return lookupTableName(_visNameIds, name, warn);
}

int TVisionaire::GetTableIdentifierBySingularName(const wxString &name, bool warn) const {
	return lookupTableName(_singularIds, name, warn);
}

int TVisionaire::GetTableIdentifierByPluralName(const wxString &name, bool warn) const {
	return lookupTableName(_pluralIds, name, warn);
}

// Confirmed (asm lines 612658-612763)
int TVisionaire::GetTableIdentifierByDescription(int description, bool warn) const {
	for (const TTable *table : _tableList) {
		if (table->GetDescription() == description)
			return table->GetIdentifier();
	}

	if (warn && wxLog::loglevel > 0)
		wxLog::logexpanded(L"TVisionaire: no table with description %d", description);
	return -2;
}

// Confirmed (asm lines 609307-609368)
TVisionaireObject *TVisionaire::GetObjectById(const TId &id) const {
	if (_mainObject && id == _mainObject->GetTId())
		return _mainObject;

	int index = tableIndex(id);
	if (index < 0 || !_tables[index])
		return nullptr;
	return _tables[index]->GetObject(id);
}

// Confirmed (asm lines 609221-609306)
bool TVisionaire::GetObjectById(const TId &id, TVisObjRef &out, bool warn) const {
	if (_mainObject && id == _mainObject->GetTId()) {
		out = TVisObjRef(*_mainObject);
		return true;
	}

	int index = tableIndex(id);
	if (index < 0 || !_tables[index])
		return false;
	return _tables[index]->GetObject(id, out, !warn);
}

// Confirmed (asm lines 609187-609220)
bool TVisionaire::GetObjectByName(const wxString &name, int table, TVisObjRef &out) {
	if (table < 0 || table >= (int)_tables.size() || !_tables[table])
		return false;
	return _tables[table]->GetByName(name, out);
}

// Confirmed (asm lines 609452-609514)
TVisObjRef TVisionaire::CreateObject(int table, TVisObjRef &parent, int field) {
	if (table < 0 || table >= (int)_tables.size() || !_tables[table])
		return TVisObjRef();

	_modified = true;
	return TVisObjRef(_tables[table]->CreateObject(parent.GetObjectPointer(), field, true));
}

// Confirmed (asm lines 609515-609549)
TVisionaireObject *TVisionaire::CreateObjectWithId(int table, const TId &id, int order) {
	if (table < 0 || table >= (int)_tables.size() || !_tables[table])
		return nullptr;
	return _tables[table]->CreateObjectWithId(id, order);
}

// Confirmed (asm lines 609550-609715): makes a copy, with the same id, of an
// object of another project and hangs it under its parent.
TVisObjRef TVisionaire::CloneObject(const TVisObjRef &source, bool /*flag*/) {
	if (source.IsEmpty()) {
		x_assert(false, "false", kSourceFile, 0x101);
		return TVisObjRef();
	}

	TVisionaireObject *original = source.GetObjectPointer();
	TId id(PackVisId(source.GetId()), source.GetId()[3]);
	int index = tableIndex(id);
	if (index < 0 || !_tables[index]) {
		x_assert(false, "false", kSourceFile, 0x11C);
		return TVisObjRef();
	}

	TVisionaireObject *copy = _tables[index]->CreateObjectWithId(id, original->GetOrder());
	if (!copy)
		return TVisObjRef();

	copy->SetParent(original->GetParentId(), original->GetParentField());
	copy->GetData()->CopyContent(*original->GetData(), true, nullptr);
	copy->SetName(source.GetName());

	TVisionaireObject *parent = GetObjectById(copy->GetParentId());
	if (parent && parent->GetData() && !(_mainObject && copy->GetParentId() == _mainObject->GetTId())) {
		TId copyId = copy->GetTId();
		parent->GetData()->SetParentLink(copy->GetParentField(), copyId, false);
	}
	return TVisObjRef(copy);
}

// Confirmed (asm lines 608417-608929, 609716-609849): copies an object, its
// content and (recursively) its children under a new parent.
TVisionaireObject *TVisionaire::CopyObject(const TVisionaireObject *source, TVisionaireObject *parent, int field,
                                           bool /*flag*/) {
	x_assert(source != nullptr && parent != nullptr, "srcObject != NULL && parent != NULL", kSourceFile, 0x134);
	if (!source || !parent)
		return nullptr;

	int index = tableIndex(source->GetTId());
	if (index < 0 || !_tables[index])
		return nullptr;

	TVisionaireObject *copy = _tables[index]->CreateObject(parent, field, false);
	copy->SetOrder(source->GetOrder());
	copy->CopyContent(source, true, nullptr);

	TVList children;
	source->GetChildren(children);
	for (TVisionaireObject *child : children)
		CopyObject(child, copy, child->GetParentField(), false);
	return copy;
}

TVisObjRef TVisionaire::CopyObject(const TVisObjRef &source, TVisObjRef &parent, int field) {
	if (source.IsEmpty())
		return TVisObjRef();
	return TVisObjRef(CopyObject(source.GetObjectPointer(), parent.GetObjectPointer(), field, false));
}

// Confirmed (asm lines 609921-610007)
TVisObjRef TVisionaire::CreateActiveObject(int table, const TVisObjRef &source) {
	if (table < 0 || table >= (int)_tables.size() || !_tables[table])
		return TVisObjRef();
	const std::uint8_t *id = source.GetId();
	return TVisObjRef(_tables[table]->CreateActiveObject(TId(PackVisId(id), id[3])));
}

TVisObjRef TVisionaire::GetActiveObject(int table, const TId &id) {
	if (table < 0 || table >= (int)_tables.size() || !_tables[table])
		return TVisObjRef();
	return TVisObjRef(_tables[table]->GetActiveObject(id));
}

// Confirmed (asm lines 610008-610078)
void TVisionaire::ResetActiveData() {
	for (TTable *table : _tableList)
		table->ResetActiveData();
}

void TVisionaire::ResetActiveData(eVisionaireTable table) {
	int index = (int)table;
	if (index < 0 || index >= (int)_tables.size() || !_tables[index]) {
		x_assert(false, "false", kSourceFile, 0x16A);
		return;
	}
	_tables[index]->ResetActiveData();
}

// Confirmed (asm lines 610079-610136)
bool TVisionaire::RemoveObjectWithoutChildren(TVisObjRef &object) {
	std::uint8_t const *idBytes = object.GetId();
	int index = (signed char)idBytes[3];
	if (index >= 0 && index < (int)_tables.size() && _tables[index] && object.GetObjectPointer())
		return _tables[index]->RemoveObject(object.GetObjectPointer());

	x_assert(false, "false", kSourceFile, 0x185);
	return false;
}

// Confirmed (asm lines 610137-610421): the children (through parent links) go
// first, then the object leaves its table.
bool TVisionaire::RemoveObjectByParent(TVisionaireObject *object) {
	x_assert(object != nullptr, "obj != NULL", kSourceFile, 0x1AB);
	if (!object)
		return false;

	if (object->GetData()) {
		for (const TTypeData *type : object->GetData()->GetTypeGroupPtr()->GetTypes()) {
			int field = type->GetDescription();

			if (type->GetType() == eTypeData::kLink) {
				const TLink &link = GetRefLink(object->GetValue(field, eTypeData::kLink));
				if (!link.IsParentLink() || isEmptyId(link.GetId()))
					continue;

				TVisionaireObject *child = GetObjectById(link.GetId());
				if (child)
					RemoveObject(child);
			} else if (type->GetType() == eTypeData::kLinkList) {
				std::vector<TLink> links = GetRefLinks(object->GetValue(field, eTypeData::kLinkList));
				for (const TLink &link : links) {
					if (!link.IsParentLink() || isEmptyId(link.GetId()))
						continue;

					TVisionaireObject *child = GetObjectById(link.GetId());
					if (child)
						RemoveObject(child);
				}
			}
		}
	}

	int index = tableIndex(object->GetTId());
	if (index < 0 || !_tables[index]) {
		x_assert(false, "false", kSourceFile, 0x1C8);
		return false;
	}
	return _tables[index]->RemoveObject(object);
}

// Confirmed (asm lines 610422-610533): an object with a parent is removed by
// removing the parent's link to it (which removes it); one without by
// RemoveObjectByParent().
bool TVisionaire::RemoveObject(TVisionaireObject *object) {
	x_assert(object != nullptr, "obj != NULL", kSourceFile, 0x18B);
	if (!object || object->IsEmpty() || _shuttingDown)
		return false;

	_modified = true;
	TVisionaireObject *parent = object->GetParent();
	if (parent && !parent->IsEmpty())
		return parent->GetData()->RemoveLink(object->GetParentField(), object->GetTId(), false);

	if (object->GetTId().getTable() == 0xFF)
		return false;
	if (tableIndex(object->GetTId()) < 0) {
		x_assert(false, "false", kSourceFile, 0x19D);
		return false;
	}
	return RemoveObjectByParent(object);
}

// Confirmed (asm lines 612824-612923)
bool TVisionaire::RemoveParentLinkedObject(const TLink &link) {
	if (!link.IsParentLink() || isEmptyId(link.GetId()))
		return false;

	TVisionaireObject *object = GetObjectById(link.GetId());
	if (!object)
		return false;
	return RemoveObject(object);
}

// Confirmed (asm lines 610534-610570)
bool TVisionaire::GetList(int table, TVList &out, bool sortByOrder) const {
	if (table < 0 || table >= (int)_tables.size() || !_tables[table])
		return false;
	_tables[table]->GetList(out, sortByOrder);
	return true;
}

long TVisionaire::GetListSize(int table) const {
	if (table < 0 || table >= (int)_tables.size() || !_tables[table])
		return -1;
	return (long)_tables[table]->GetCount();
}

// Confirmed (asm lines 610604-610748): 0 up, 1 down, 2 first, 3 last.
bool TVisionaire::ChangeOrder(const TVisObjRef &object, TMoveOrderEnum move) {
	if (object.IsEmpty() || object.GetParent().IsEmpty())
		return false;

	const std::uint8_t *id = object.GetId();
	int index = (signed char)id[3];
	if (index < 0 || index >= (int)_tables.size() || !_tables[index]) {
		x_assert(false, "false", kSourceFile, 0x1EA);
		return false;
	}

	TTable *table = _tables[index];
	bool moved;
	switch (move) {
	case TMoveOrderEnum::kUp:
		moved = table->MoveOrderUp(object.GetObjectPointer());
		break;
	case TMoveOrderEnum::kDown:
		moved = table->MoveOrderDown(object.GetObjectPointer());
		break;
	case TMoveOrderEnum::kFirst:
		moved = table->MoveOrderFirst(object.GetObjectPointer());
		break;
	case TMoveOrderEnum::kLast:
		moved = table->MoveOrderLast(object.GetObjectPointer());
		break;
	default:
		return false;
	}

	if (moved)
		_modified = true;
	return moved;
}

// Confirmed (asm lines 610749-610859)
unsigned long TVisionaire::GetSizeMemory(int table) const {
	if (table < 0 || table >= (int)_tables.size() || !_tables[table])
		return 0;
	return _tables[table]->GetSizeMemory();
}

void TVisionaire::RemoveTempData() {
	for (TTable *table : _tableList)
		table->RemoveTempData();
}

void TVisionaire::CreateTempData() {
	for (TTable *table : _tableList)
		table->CreateTempData();
}

// Confirmed (asm lines 610860-610940): the original reserves 0x88 bytes per
// object up front (an object and its record in one block).
void TVisionaire::InitMemory(int count) {
	_poolSize = count;
}

void *TVisionaire::Reserve() {
	return nullptr;
}

// Confirmed (asm lines 611040-611081)
bool TVisionaire::InitAsMasterProject() {
	if (_projectType == 1)
		return false;

	_projectType = 0;
	InitVersionedIds();
	return true;
}

void TVisionaire::InitAsSlaveProject() {
	_projectType = 1;
	InitVersionedIds();
}

// Confirmed (asm lines 611169-611189)
void TVisionaire::IncreaseRevision() {
	_revision = (_revision > 0) ? _revision + 1 : 1;
}

void TVisionaire::InitVersionedIds() {
	for (TTable *table : _tableList)
		table->InitVersionedId();
}

bool TVisionaire::HasPath() const {
	return _path.IsOk();
}

// Confirmed (asm lines 616124-616470). Player mode keeps no link table.
void TVisionaire::AddLink(const TId &from, const TId &to, int field, bool sorted) {
	if (IsVisPlayerMode)
		return;

	TLinkRef ref;
	ref.from = from;
	ref.to = to;
	ref.field = field;

	if (!sorted || _links.empty()) {
		_links.push_back(ref);
		return;
	}

	auto position = std::lower_bound(_links.begin(), _links.end(), ref, linkRefLess);
	if (position != _links.end() && !linkRefLess(ref, *position)) {
		x_assert(false, "false", kSourceFile, 0x5AD);
		return;
	}
	_links.insert(position, ref);
}

// Confirmed (asm lines 612941-613165): finds the entry in the sorted table.
bool TVisionaire::RemoveLink(const TId &from, const TId &to, int field) {
	if (IsVisPlayerMode)
		return true;

	TLinkRef ref;
	ref.from = from;
	ref.to = to;
	ref.field = field;

	auto position = std::lower_bound(_links.begin(), _links.end(), ref, linkRefLess);
	if (position == _links.end() || linkRefLess(ref, *position)) {
		x_assert(false, "false", kSourceFile, 0x5EC);
		return false;
	}
	_links.erase(position);
	return true;
}

// Confirmed (asm lines 616051-616123)
void TVisionaire::AddParentLink(const TId &parent, const TId &child, int field) {
	TLinkRef ref;
	ref.from = parent;
	ref.to = child;
	ref.field = field;
	_tempParentLinks.push_back(ref);
}

// Confirmed (asm lines 615478-615756): every entry whose target is `id`.
void TVisionaire::GetObjectsLinkedTo(const TId &id, std::vector<TLinkRef> &out) const {
	for (const TLinkRef &ref : _links) {
		if (ref.to == id)
			out.push_back(ref);
	}
}

// Confirmed (asm lines 613166-613186)
void TVisionaire::StoreLinksCount() {
	_linksCount = (long)_links.size();
}

// Confirmed (asm lines 613187-613264): the child learns its parent and the
// field it hangs under.
void TVisionaire::SetupParents() {
	for (const TLinkRef &ref : _tempParentLinks) {
		TVisionaireObject *child = GetObjectById(ref.to);
		if (child)
			child->SetParent(ref.from, ref.field);
	}
	_tempParentLinks.clear();
}

// Confirmed in shape (asm lines 619851-619960): puts the link table in order
// after it was filled unsorted by the loader.
void TVisionaire::SortLinks() {
	std::sort(_links.begin(), _links.end(), linkRefLess);
}

// Confirmed (asm lines 613265-613300)
void TVisionaire::BeginPaste() {
	for (TTable *table : _tableList)
		table->BeginPaste();
}

// Confirmed (asm lines 613460-613496)
void TVisionaire::EndPaste() {
	for (TTable *table : _tableList)
		table->EndPaste();
	_mappedIds.clear();
}

// Approximate (asm lines 613497-613713).
void TVisionaire::EndMerge() {
	for (TTable *table : _tableList)
		table->EndPaste();
	_mappedIds.clear();
	_hasIdMapping = false;
}

// Confirmed (asm lines 613301-613459): a binary search of the mappings.
TId TVisionaire::GetMappedId(const TId &id) const {
	for (const TMappedId &mapping : _mappedIds) {
		if (mapping.from == id)
			return mapping.to;
	}
	return TId(-1, -1);
}

// Confirmed in shape (asm lines 616471-616653).
TVisionaireObject *TVisionaire::PasteObject(const TVisionaireObject *source, const TId &parentId, int parentField) {
	int index = tableIndex(source->GetTId());
	if (index < 0 || !_tables[index])
		return nullptr;
	return _tables[index]->PasteObject(source, parentId, parentField);
}

TVisObjRef TVisionaire::PasteObject(const TVisObjRef &source, const TId &parentId, int parentField) {
	if (source.IsEmpty())
		return TVisObjRef();
	return TVisObjRef(PasteObject(source.GetObjectPointer(), parentId, parentField));
}

// Confirmed in shape (asm lines 611225-611563): the merge/import helpers
// ask every table.
void TVisionaire::GetNewObjects(TVList &out) const {
	for (const TTable *table : _tableList)
		table->GetNewObjects(out);
}

void TVisionaire::GetDeletedObjects(const TVisionaire &other, TVList &out) const {
	for (const TTable *table : _tableList) {
		const TTable *otherTable = nullptr;
		if (other.GetTableConst(table->GetIdentifier(), &otherTable) && otherTable)
			table->GetDeletedObjects(otherTable, out);
	}
}

void TVisionaire::GetChangedObjects(TVList &out) const {
	for (const TTable *table : _tableList)
		table->GetChangedObjects(out);
}

int TVisionaire::GetConflictedObjectsCount(const TVisionaire &other) const {
	int count = 0;
	for (const TTable *table : _tableList) {
		const TTable *otherTable = nullptr;
		if (other.GetTableConst(table->GetIdentifier(), &otherTable) && otherTable)
			count += table->GetConflictedObjectsCount(otherTable);
	}
	return count;
}

void TVisionaire::ImportNewObjects(const TVisionaire &source) {
	for (TTable *table : _tableList) {
		const TTable *sourceTable = nullptr;
		if (source.GetTableConst(table->GetIdentifier(), &sourceTable) && sourceTable)
			table->ImportNewObjects(source, const_cast<TTable *>(sourceTable));
	}
}

// Not reconstructed (asm lines 614496-614807): the editor's merge of two
// projects.
void TVisionaire::MergeObjects(const TVisionaire &/*other*/, TVisObjectCompareInfo &/*info*/) {
}

// Confirmed (asm lines 612924-612940)
void TVisionaire::SetVisPlayerMode(bool playerMode) {
	IsVisPlayerMode = playerMode;
}

/** The order in which the tables are written when the data are scrambled (asm ScrambledSortOrder, 0x25 table numbers). */
static const int ScrambledSortOrder[0x25] = {3, 16, -1, 0, 25, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 31, 14,
                                             15, 17, 18, 19, 20, 21, 34, 22, 23, 24, 26, 35, 27, 28, 29, 30, 32, 33};

// Confirmed (asm lines 608353-608417): table `first` goes before `second` in the order above.
static bool ScrambledSorting(TTable *first, TTable *second) {
	int firstPosition = -1;
	int secondPosition = -1;

	for (int i = 0; i < 0x25; i++) {
		if (ScrambledSortOrder[i] == first->GetIdentifier())
			firstPosition = i;

		if (ScrambledSortOrder[i] == second->GetIdentifier())
			secondPosition = i;
	}

	x_assert(secondPosition != -1 && firstPosition != -1, "newPosT1 != -1 && newPosT2 != -1", kSourceFile, 0x55C);
	return firstPosition < secondPosition;
}

// Confirmed (asm lines 616654-616818): the tables are put in the scrambled order and every type group scrambles its fields
// (with the description of its table as the seed). Used when the data are written as XML.
void TVisionaire::SetScrambled() {
	std::sort(_tableList.begin(), _tableList.end(), ScrambledSorting);

	for (TTable *table : _tables) {
		if (table && table->GetTypeGroup())
			table->GetTypeGroup()->SetScrambled(table->GetDescription());
	}
}

// Confirmed (asm lines 616818-617211 and its constant-argument clone 618630-618973). Writes the project or a savegame to
// `writer`. A savegame (and not `forceXml`) is written in the binary format (BinarySave(), into the buffer of the writer);
// else it is XML: the tag of the file (kSaveGame, or kVisionaireAdventure with the project type and the revision), the
// version, the main object ("Game"), then every table. `handler` gets the progress events of the editor.
bool TVisionaire::SaveData(TProjectFileWriter &writer, EventHandler *handler, bool forceXml) {
	if (writer.IsSaveGame() && !forceXml)
		return BinarySave(wxFileName(), handler, true, &writer);

	int rootTag = writer.IsSaveGame() ? kSaveGame : kVisionaireAdventure;

	writer.StartTag(rootTag);

	if (!writer.IsSaveGame()) {
		writer.AddAttribute(kProjectType, _projectType);
		writer.AddAttribute(kRevision, _revision);
	}

	InitWithVersion(_version);

	if (TXMLNames::IsScrambled())
		SetScrambled();

	writer.SetVersion(_version, _version);
	writer.AddAttribute(kVersion, _version);
	writer.FinishAttributes(true);

	x_assert(!_mainObject->IsEmpty(), "MainObject->IsEmpty() == false", kSourceFile, 0x32F);

	if (_mainObject->IsEmpty()) {
		x_assert(false, "false", kSourceFile, 0x341);
		return false;
	}

	TTypeGroup *typeGroup = _mainObject->GetData()->GetTypeGroupPtrNonConst();

	if (!typeGroup) {
		x_assert(false, "false", kSourceFile, 0x341);
		return false;
	}

	if (handler)
		handler->AddPendingEvent(new LoadSaveProgressEvent(-1, true));

	typeGroup->SetupNeededTypes(writer.IsSaveGame(), false, true);

	if (!_mainObject->GetData()->Serialize(writer)) {
		x_assert(false, "false", kSourceFile, 0x33B);
		return false;
	}

	for (TTable *table : _tableList) {
		if (handler)
			handler->AddPendingEvent(new LoadSaveProgressEvent(table->GetIdentifier(), true));

		TTypeGroup *tableTypeGroup = table->GetTypeGroup();

		if (!tableTypeGroup) {
			x_assert(false, "false", kSourceFile, 0x352);
			continue;
		}

		tableTypeGroup->SetupNeededTypes(writer.IsSaveGame(), false, true);
		table->Serialize(writer);
	}

	writer.FinishTag(rootTag);
	return true;
}

// Confirmed (asm lines 619088-619106 and 618630). A savegame: the writer is set to write one, the data are written (binary, into
// the buffer of the writer) and the writer finishes (TXMLStringWriter::FinishWrite() compresses and encrypts the buffer as the
// content flags of the file say).
bool TVisionaire::SaveSaveGame(TProjectFileWriter &writer) {
	writer.SetSaveGame(true);

	if (!SaveData(writer, nullptr, false))
		return false;

	writer.FinishWrite();
	return true;
}

// Confirmed (asm lines 620666-620683): nothing to do before saving (TVisionaireGame::BeforeSave() is the editor's).
bool TVisionaire::BeforeSave() {
	return true;
}

// Confirmed (asm lines 618973-619088): the game data as XML text (empty if BeforeSave() fails or the data cannot be written).
wxString TVisionaire::SaveDataGameToString() {
	if (!BeforeSave())
		return wxString();

	TXMLStringWriter writer;

	writer.SetSaveGame(false);

	if (!SaveData(writer, nullptr, false))
		return wxString();

	writer.FinishWrite();
	writer.GetBufferNonConst().AppendByte(0);
	wxString text;

	toUTF(&text, reinterpret_cast<const char *>(writer.GetBuffer().GetData()));
	return text;
}

// Confirmed (asm lines 619961-620665). `saveGame` is 0 for game data and 1 for a
// savegame (which is only ever loaded in one go, type 2); `type` is described
// at TLoadingTypeEnum. Game data (but not a savegame) clears the project first;
// a savegame resets the active objects instead. Binary files go to
// BinaryLoad(); the XML project reader (TXMLProjectReader) is not
// reconstructed yet, so an XML file fails to load.
bool TVisionaire::Load(const wxFileName &file, const wxString &extra, eSaveGame saveGame,
                       TLoadingTypeEnum type, int *outFlag, TSignalSlot *slot, EventHandler *handler) {
	int gameType = (int)saveGame;
	int loadType = (int)type;

	x_assert(gameType == 0 || gameType == 1, "gameType == t_DATAGAME || gameType == t_SAVEGAME", kSourceFile, 0x212);
	x_assert(loadType == 2 || gameType != 1, "gameType != t_SAVEGAME || loadingType == t_ALL", kSourceFile, 0x213);

	bool isSaveGame = gameType == 1;

	if (file.IsOk())
		_loadedFile = file;
	_path = file;

	TTimer timer;
	timer.SetTime();

	int xmlRoot;
	if (loadType == 0 || loadType == 2) {
		if (isSaveGame) {
			for (TTable *table : _tableList)
				table->ResetActiveData();
			xmlRoot = 0x21;
		} else {
			Clear();
			xmlRoot = 0x64;
		}
	} else {
		xmlRoot = isSaveGame ? 0x21 : 0x64;
	}

	x_assert(!_mainObject->IsEmpty(), "!MainObject->IsEmpty()", kSourceFile, 0x23F);

	TVedFile ved;
	if (!TComposedFileManager::Open(ved, file))
		return false;

	ved.CheckBinary();
	ved.SetSaveGame(isSaveGame);
	_linksCount = (long)_links.size();

	if (ved.IsBinary()) {
		if (!BinaryLoad(ved, type, slot, outFlag, handler)) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Failed to load ved file %ls", file.GetName().c_str());
			return false;
		}
	} else {
		// FIXME: TXMLProjectReader::ReadXMLDoc (asm lines 636222-642078) is not reconstructed.
		(void)extra;
		(void)xmlRoot;
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Failed to load ved file %ls", file.GetName().c_str());
		return false;
	}

	if (wxLog::loglevel > 1)
		wxLog::logexpanded(L"Serialization finished. Needed time: %ld ms", timer.GetTime());
	timer.SetTime();

	if (loadType != 0) {
		SetupParents();
		if (wxLog::loglevel > 1)
			wxLog::logexpanded(L"SetupParents finished. Needed time: %ld ms", timer.GetTime());
		timer.SetTime();

		SortLinks();
		if (wxLog::loglevel > 1)
			wxLog::logexpanded(L"SortLinks finished. Needed time: %ld ms", timer.GetTime());
		timer.SetTime();
	}

	return true;
}

