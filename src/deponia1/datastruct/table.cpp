#include "datastruct/table.h"

#include <algorithm>

#include "Diagnostics.h"
#include "TXMLNames.h"
#include "baselib/xmlWriter.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/datastruct/table.cpp";

static bool isEmptyId(const TId &id) {
	return id.getId() == -1 && id.getTable() == 0xFF;
}

// Confirmed (asm lines 664949-665091)
TTable::TTable(TVisionaire *visionaire, int description, int identifier, const wxString &name, int unknown,
               TTypeGroup *typeGroup, int activeLinkField, bool flag)
	: _description(description), _identifier(identifier), _name(name), _typeGroup(typeGroup),
	  _nameIndexBuilt(false), _activeLinkField(activeLinkField), _flag(flag), _unknown(unknown),
	  _versionedId(-1), _pasteBase(-1), _nextId(0), _visionaire(visionaire) {
}

// Confirmed (asm lines 664827-664948)
TTable::~TTable() {
	Clear();
}

// Confirmed (asm lines 662786-662817)
bool TTable::GetObjectAtPosition(TVisionaireObject **object, int position) const {
	if (position < 0 || (size_t)position >= _objects.size())
		return false;
	*object = _objects[position];
	return true;
}

// Confirmed (asm lines 660697-661059): the hash index first, then a binary
// search of the (id-ordered) list.
TVisionaireObject *TTable::GetObject(const TId &id) const {
	auto it = _index.find(id.getId());
	if (it != _index.end() && it->second >= 0 && (size_t)it->second < _objects.size())
		return _objects[it->second];

	size_t low = 0;
	size_t high = _objects.size();
	while (low < high) {
		size_t middle = (low + high) / 2;
		int found = _objects[middle]->GetId24();
		if (found == id.getId())
			return _objects[middle];
		if (found < id.getId())
			low = middle + 1;
		else
			high = middle;
	}
	return nullptr;
}

// Confirmed (asm lines 660799-661004)
bool TTable::GetObject(const TId &id, TVisObjRef &out, bool quiet) const {
	TVisionaireObject *object = GetObject(id);
	if (!object) {
		if (!quiet && wxLog::loglevel > 0)
			wxLog::logexpanded(L"TTable::GetObject: object %d not found in table %ls", id.getId(),
			                   _name.c_str());
		return false;
	}

	out = TVisObjRef(*object);
	return true;
}

// Confirmed (asm lines 661224-661309): a plain search by name.
TVisionaireObject *TTable::GetObject(const wxString &name) const {
	TCharHolder wanted(name);
	for (TVisionaireObject *object : _objects) {
		if (object->GetName().Cmp(wanted) == 0)
			return object;
	}
	return nullptr;
}

// Confirmed (asm lines 661060-661223)
bool TTable::GetObjectPosition(const TId &id, unsigned long &position) const {
	auto it = _index.find(id.getId());
	if (it != _index.end()) {
		position = it->second;
		return true;
	}

	size_t low = 0;
	size_t high = _objects.size();
	while (low < high) {
		size_t middle = (low + high) / 2;
		int found = _objects[middle]->GetId24();
		if (found == id.getId()) {
			position = middle;
			return true;
		}
		if (found < id.getId())
			low = middle + 1;
		else
			high = middle;
	}
	return false;
}

// The name list is built by the first lookup by name.
void TTable::ensureNameIndex() const {
	if (_nameIndexBuilt)
		return;

	_byName = _objects;
	std::stable_sort(_byName.begin(), _byName.end(), [](const TVisionaireObject *a, const TVisionaireObject *b) {
		return a->GetName().Cmp(b->GetName()) < 0;
	});
	_nameIndexBuilt = true;
}

int TTable::nameInsertPosition(const wxString &name) const {
	TCharHolder key(name);
	int low = 0;
	int high = (int)_byName.size();
	while (low < high) {
		int middle = (low + high) / 2;
		if (_byName[middle]->GetName().Cmp(key) < 0)
			low = middle + 1;
		else
			high = middle;
	}
	return low;
}

// Confirmed (asm lines 661310-661497): the position in the name-ordered list
// of the object with this name and id.
bool TTable::GetObjectPosition(const wxString &name, const TId &id, unsigned long &position) const {
	ensureNameIndex();

	TCharHolder key(name);
	for (int i = nameInsertPosition(name); i < (int)_byName.size(); i++) {
		if (_byName[i]->GetName().Cmp(key) != 0)
			break;
		if (_byName[i]->GetId24() == id.getId()) {
			position = i;
			return true;
		}
	}
	return false;
}

// Confirmed (asm lines 667552-667963)
bool TTable::GetByName(const wxString &name, TVisObjRef &out) {
	ensureNameIndex();

	int position = nameInsertPosition(name);
	TCharHolder key(name);
	if (position >= (int)_byName.size() || _byName[position]->GetName().Cmp(key) != 0)
		return false;

	out.Set(_byName[position]);
	return true;
}

// Confirmed (asm lines 667964-668125)
bool TTable::GetList(TVList &out, bool sortByOrder) const {
	out.clear();
	for (TVisionaireObject *object : _objects)
		out.push_back(object);

	if (sortByOrder)
		std::sort(out.begin(), out.end(), cmpOrder);
	return true;
}

// Confirmed (asm lines 661498-661611)
TVisionaireObject *TTable::GetActiveObject(const TId &id) {
	if (_activeLinkField == -1) {
		x_assert(false, "false", kSourceFile, 0x140);
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"TTable::GetActiveObject: table %ls (id: %d) does not store active objects",
			                   TXMLNames::GetString(_description).c_str(), _description);
		return nullptr;
	}

	for (TVisionaireObject *object : _objects) {
		TVisionaireObject *linked = object->GetLink(_activeLinkField);
		if (linked && linked->GetTId() == id)
			return object;
	}
	return nullptr;
}

// Confirmed (asm lines 666378-666505)
TVisionaireObject *TTable::CreateActiveObject(const TId &id) {
	if (_activeLinkField == -1) {
		x_assert(false, "false", kSourceFile, 0x2A4);
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"TTable::CreateActiveObject: table %ls (id: %d) does not store active objects",
			                   TXMLNames::GetString(_description).c_str(), _description);
		return nullptr;
	}

	TVisionaireObject *object = CreateObject(nullptr, -1, false);
	object->SetLink(_activeLinkField, id, false);
	return object;
}

void TTable::rebuildIndex() {
	_index.clear();
	for (size_t i = 0; i < _objects.size(); i++)
		_index[_objects[i]->GetId24()] = (int)i;
}

// Adds an object to the id list (at the end, or at its sorted place), the
// hash index and, if built, the name list.
void TTable::addObject(TVisionaireObject *object, bool atEnd) {
	if (atEnd) {
		_objects.push_back(object);
		_index[object->GetId24()] = (int)_objects.size() - 1;
	} else {
		auto position = std::lower_bound(_objects.begin(), _objects.end(), object,
		                                 [](const TVisionaireObject *a, const TVisionaireObject *b) {
			                                 return a->GetId24() < b->GetId24();
		                                 });
		_objects.insert(position, object);
		rebuildIndex();
	}

	x_assert(_index.size() == _objects.size(), "ObjectsHash.size() == Objects.size()", kSourceFile, 0x28A);
	if (_index.size() != _objects.size()) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"table %d not in sync, recreating index", _identifier);
		rebuildIndex();
	}

	if (_nameIndexBuilt)
		_byName.insert(_byName.begin() + nameInsertPosition(wxString(object->GetName())), object);
}

int TTable::allocateId() {
	int id = _nextId;
	_nextId = id + 1;
	return id;
}

// Confirmed (asm lines 665317-665519). Objects loaded from a file get the
// byte distance of their slot as a provisional order.
TVisionaireObject *TTable::CreateObjectLoad(int id) {
	TVisionaireObject *object = new TVisionaireObject(id, (int)(_objects.size() * 8), _identifier, _visionaire,
	                                                  _typeGroup, false);
	addObject(object, true);
	return object;
}

// Confirmed (asm lines 665813-666377)
TVisionaireObject *TTable::CreateObject(TVisionaireObject *parent, int field, bool /*flag*/) {
	int id = allocateId();
	int order = id;
	if (parent && parent->GetTypeField(field) == (int)eTypeData::kLinkList)
		order = parent->GetLinksHighestOrder(field) + 1;

	TVisionaireObject *object = new TVisionaireObject(id, order, _identifier, _visionaire, _typeGroup, false);
	object->Init(parent, field);
	addObject(object, true);

	_typeGroup->OnCreate(object);
	return object;
}

// Confirmed (asm lines 666506-667193)
TVisionaireObject *TTable::CreateObjectWithId(const TId &id, int order) {
	if (GetObject(id)) {
		x_assert(false, "false", kSourceFile, 0x2BC);
		return nullptr;
	}

	TVisionaireObject *object = new TVisionaireObject(id.getId(), order, _identifier, _visionaire, _typeGroup,
	                                                  false);
	NotifyNewId(id.getId());
	addObject(object, false);
	return object;
}

// Confirmed (asm lines 665520-665812): the copy gets the next id and the order
// of the original; its parent is the given one (or the original's if none).
TVisionaireObject *TTable::PasteObject(const TVisionaireObject *source, const TId &parentId, int parentField) {
	x_assert(source != nullptr, "srcObj != NULL", kSourceFile, 0x4C5);
	if (!source || source->IsEmpty()) {
		x_assert(false, "false", kSourceFile, 0x4C8);
		return nullptr;
	}

	int id = allocateId();
	TVisionaireObject *object = new TVisionaireObject(id, source->GetOrder(), _identifier, _visionaire, _typeGroup,
	                                                  false);
	object->GetData()->CopyContent(*source->GetData(), true, nullptr);

	if (!isEmptyId(parentId)) {
		object->SetParent(parentId, parentField);
		object->SetOrder(-1);
	} else {
		object->SetParent(source->GetParentId(), -1);
	}

	addObject(object, true);
	return object;
}

// Confirmed (asm lines 664491-664510): remembers where the pasted ids begin.
void TTable::BeginPaste() {
	_pasteBase = _nextId;
}

// Confirmed in shape (asm lines 665092-665316): the objects made since
// BeginPaste() get their parent ids mapped and their links validated.
void TTable::EndPaste() {
	if (_pasteBase >= 0 && _pasteBase < _nextId) {
		for (TVisionaireObject *object : _objects) {
			if (object->GetId24() < _pasteBase)
				continue;

			TId mapped = _visionaire->GetMappedId(object->GetParentId());
			if (!isEmptyId(mapped))
				object->SetParent(mapped, object->GetParentField());
			object->GetData()->ValidateAndAdaptLinks();
		}
	}
	_pasteBase = -1;
}

// Confirmed (asm lines 662837-663263)
bool TTable::RemoveObject(TVisionaireObject *object) {
	x_assert(object != nullptr, "obj != NULL", kSourceFile, 0x321);

	if (_nameIndexBuilt) {
		auto it = std::find(_byName.begin(), _byName.end(), object);
		if (it != _byName.end())
			_byName.erase(it);
	}

	unsigned long position;
	if (!GetObjectPosition(object->GetTId(), position))
		return false;

	TVisionaireObject *held = _objects[position];
	_objects.erase(_objects.begin() + position);
	rebuildIndex();
	x_assert(_index.size() == _objects.size(), "ObjectsHash.size() == Objects.size()", kSourceFile, 0x348);
	held->Release();
	return true;
}

// Confirmed (asm lines 661788-661897)
void TTable::Clear() {
	_nextId = 0;
	_versionedId = -1;

	std::vector<TVisionaireObject *> objects;
	objects.swap(_objects);
	_index.clear();
	_byName.clear();
	_nameIndexBuilt = false;

	for (TVisionaireObject *object : objects)
		object->Remove(false, true);
}

// Confirmed (asm lines 667194-667551)
void TTable::ObjectNameChanged(const TVisionaireObject *object, int oldPosition) {
	x_assert(object != nullptr, "obj != NULL", kSourceFile, 0x360);
	if (!_nameIndexBuilt)
		return;

	TVisionaireObject *held = nullptr;
	if (oldPosition >= 0 && oldPosition < (int)_byName.size() && _byName[oldPosition]->GetTId() == object->GetTId()) {
		held = _byName[oldPosition];
		_byName.erase(_byName.begin() + oldPosition);
	} else {
		for (auto it = _byName.begin(); it != _byName.end(); ++it) {
			if ((*it)->GetTId() == object->GetTId()) {
				held = *it;
				_byName.erase(it);
				break;
			}
		}
		x_assert(held != nullptr, "namedObjRemoved", kSourceFile, 0x38E);
	}

	if (held)
		_byName.insert(_byName.begin() + nameInsertPosition(wxString(held->GetName())), held);
}

// Confirmed (asm lines 661612-661787)
void TTable::ValidateParentLinks() {
	for (TVisionaireObject *object : _objects) {
		TId parentId = object->GetParentId();
		if (!isEmptyId(parentId)) {
			TVisionaireObject *parent = _visionaire->GetObjectById(parentId);
			if (parent && !parent->IsEmpty())
				continue;
		}

		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"TTable::ValidateParentLinks: object '%hs' (id: %d) in table %ls (id: %d) does not have a valid parent",
			                   object->GetName().mb_str(), object->GetId24(),
			                   TXMLNames::GetString(_description).c_str(), _description);
	}
}

// The four order moves (asm lines 661898-662576) renumber the orders of an
// object's siblings (the objects of the parent's link list, in order).
static bool siblingsOf(const TVisionaireObject *object, TVList &siblings, int &position) {
	TVisionaireObject *parent = object->GetParent();
	if (!parent)
		return false;

	parent->GetLinks(object->GetParentField(), TypeOrder::kValue1, siblings);
	position = 0;
	for (TVisionaireObject *sibling : siblings) {
		if (sibling == object)
			return true;
		position++;
	}
	x_assert(false, "false", kSourceFile, 0x1B6);
	return false;
}

// Confirmed (asm lines 661898-662046): every sibling before the object takes
// the next one's order, the object takes the first.
bool TTable::MoveOrderFirst(const TVisionaireObject *object) {
	x_assert(object != nullptr, "obj != NULL", kSourceFile, 0x196);

	TVList siblings;
	int position;
	if (!siblingsOf(object, siblings, position) || position == 0)
		return false;

	for (int i = 0; i < position; i++) {
		int first = siblings.at(i + 1)->GetOrder();
		int second = siblings.at(i)->GetOrder();
		siblings.at(i + 1)->SetOrder(second);
		siblings.at(i)->SetOrder(first);
	}
	return true;
}

// Confirmed (asm lines 662047-662195): the mirror image of MoveOrderFirst().
bool TTable::MoveOrderLast(const TVisionaireObject *object) {
	x_assert(object != nullptr, "obj != NULL", kSourceFile, 0x1C1);

	TVList siblings;
	int position;
	if (!siblingsOf(object, siblings, position) || position == (int)siblings.size() - 1)
		return false;

	for (int i = (int)siblings.size() - 1; i > position; i--) {
		int last = siblings.at(i)->GetOrder();
		int before = siblings.at(i - 1)->GetOrder();
		siblings.at(i)->SetOrder(before);
		siblings.at(i - 1)->SetOrder(last);
	}
	return true;
}

// Confirmed in shape (asm lines 662196-662576): swaps the orders of the
// object and the sibling before / after it.
bool TTable::MoveOrderUp(const TVisionaireObject *object) {
	TVList siblings;
	int position;
	if (!siblingsOf(object, siblings, position) || position == 0)
		return false;

	int mine = siblings.at(position)->GetOrder();
	siblings.at(position)->SetOrder(siblings.at(position - 1)->GetOrder());
	siblings.at(position - 1)->SetOrder(mine);
	return true;
}

bool TTable::MoveOrderDown(const TVisionaireObject *object) {
	TVList siblings;
	int position;
	if (!siblingsOf(object, siblings, position) || position == (int)siblings.size() - 1)
		return false;

	int mine = siblings.at(position)->GetOrder();
	siblings.at(position)->SetOrder(siblings.at(position + 1)->GetOrder());
	siblings.at(position + 1)->SetOrder(mine);
	return true;
}

// Confirmed (asm lines 662577-662616)
unsigned long TTable::GetSizeMemory() const {
	unsigned long size = 0;
	for (const TVisionaireObject *object : _objects)
		size += object->GetSizeMemory(true, false);
	return size;
}

// Confirmed (asm lines 662617-662699): -1 for an object that isn't here.
unsigned long TTable::GetSizeMemory(const TVisionaireObject &object, bool deep) const {
	TVisionaireObject *found = GetObject(object.GetTId());
	if (!found)
		return (unsigned long)-1;
	return found->GetSizeMemory(deep, false);
}

// Confirmed (asm lines 663264-663369)
bool TTable::Serialize(TProjectFileWriter &writer) {
	if (!_typeGroup->AppliesToFileVersion(writer))
		return true;

	writer.StartTag(_description);
	writer.AddAttribute(kNewId, _nextId);
	writer.AddAttribute(kVersionedId, _versionedId);
	writer.AddAttribute(kTableId, _identifier);
	writer.FinishAttributes(true);

	for (TVisionaireObject *object : _objects) {
		if (object->IsTemporary() || !object->GetData())
			continue;

		x_assert(!object->IsEmpty(), "!(*node)->IsEmpty()", kSourceFile, 0x3C6);
		object->GetData()->Serialize(writer);
	}

	writer.FinishTag(_description);
	return true;
}

// Confirmed (asm lines 663370-663420)
bool TTable::IsActiveTable() const {
	return (int)_typeGroup->GetSaveGameType() == 1;
}

void TTable::ResetActiveData() {
	if ((int)_typeGroup->GetSaveGameType() == 1)
		Clear();
}

// Confirmed (asm lines 664407-664490)
void TTable::CreateTempData() {
	if ((int)_typeGroup->GetSaveGameType() != 2)
		return;
	for (TVisionaireObject *object : _objects)
		object->GetData()->CreateTempData();
}

void TTable::RemoveTempData() {
	for (TVisionaireObject *object : _objects)
		object->GetData()->RemoveTempData();
}

// Confirmed (asm lines 663421-663502)
void TTable::NotifyNewId(int id) {
	if (id >= _nextId)
		_nextId = id + 1;
}

void TTable::InitVersionedId() {
	_versionedId = _nextId - 1;
}

// Approximate (asm lines 663503-664216, read in outline): objects newer than
// the last saved version, unless their parent is newer too.
void TTable::GetNewObjects(TVList &out) const {
	for (TVisionaireObject *object : _objects) {
		if (object->GetId24() <= _versionedId)
			continue;

		TId parentId = object->GetParentId();
		if (parentId.getTable() != 0xFF) {
			TTable *parentTable = nullptr;
			if (!_visionaire->GetTable((signed char)parentId.getTable(), &parentTable))
				continue;
			if (parentTable->_versionedId < parentId.getId())
				continue;
		}
		out.push_back(object);
	}
}

// Approximate (asm lines 663606-663724): the objects of `other` that were in
// this table at the last saved version and aren't any more.
void TTable::GetDeletedObjects(const TTable *other, TVList &out) const {
	for (TVisionaireObject *object : other->_objects) {
		if (_versionedId < object->GetId24())
			continue;
		if (!GetObject(object->GetTId()))
			out.push_back(object);
	}
}

// Approximate (asm lines 663725-663796): the objects of the saved version
// modified since.
void TTable::GetChangedObjects(TVList &out) const {
	for (TVisionaireObject *object : _objects) {
		if (_versionedId < object->GetId24())
			continue;
		if (object->GetLastModified() > _visionaire->GetModifiedStamp())
			out.push_back(object);
	}
}

// Approximate (asm lines 663797-663940): the objects both tables changed.
int TTable::GetConflictedObjectsCount(const TTable *other) const {
	int count = 0;
	for (TVisionaireObject *object : _objects) {
		if (_versionedId < object->GetId24())
			continue;

		TVisionaireObject *theirs = other->GetObject(object->GetTId());
		if (theirs && object->GetLastModified() > _visionaire->GetModifiedStamp() &&
		    theirs->GetLastModified() > _visionaire->GetModifiedStamp())
			count++;
	}
	return count;
}

// Not reconstructed (asm lines 663941-664216): the editor's import/merge of
// another project's objects.
void TTable::ImportNewObjects(const TVisionaire &/*source*/, TTable * /*sourceTable*/) {
}

void TTable::MergeObjects(const TTable * /*other*/, TVisObjectCompareInfo &/*info*/) {
}

void TTable::FixOrderOfImportedObject(TVisionaireObject * /*object*/) {
}
