#include "datastruct/visionaireobject.h"

#include <algorithm>

#include "Diagnostics.h"
#include "datastruct/table.h"
#include "datastruct/visionaire.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/datastruct/visionaireobject.cpp";
static const char *const kRefCountAssert = "RefCount > 0 && RefCount < 3000";

static TCharHolder &emptyName() {
	static TCharHolder empty;
	return empty;
}

// Confirmed (asm lines 587318-587352)
void TVisionaireObject::SetNameNewObject(const TCharHolder &name) {
	GetNameNewObject() = name;
}

TCharHolder &TVisionaireObject::GetNameNewObject() {
	static TCharHolder s_nameNewObject;
	return s_nameNewObject;
}

// Confirmed (asm lines 587353-587386)
TVisionaireObject::TVisionaireObject(TVisionaire *visionaire)
	: _id(-1, -1), _parentId(-1, -1), _order(-1), _flags(0), _data(nullptr), _visionaire(visionaire),
	  _refCount(1), _parentField(-1), _luaObject(0) {
}

// Confirmed (asm lines 587387-587420)
TVisionaireObject::TVisionaireObject(bool anyObject, TVisionaire *visionaire)
	: _id(-1, -1), _parentId(-1, -1), _order(-1), _flags(anyObject ? kAnyObject : 0), _data(nullptr),
	  _visionaire(visionaire), _refCount(1), _parentField(-1), _luaObject(0) {
}

// Confirmed (asm lines 587421-587507). The record is always allocated
// separately here; `inPlace` only sets the flag (the original constructs it
// in the memory block that follows the object).
TVisionaireObject::TVisionaireObject(int id, int order, int table, TVisionaire *visionaire,
                                     const TTypeGroup *typeGroup, bool inPlace)
	: _id(id, table), _parentId(-1, -1), _flags(0), _data(nullptr), _visionaire(visionaire), _refCount(1),
	  _parentField(-1), _luaObject(0) {
	SetOrder(order);
	_data = new TDataGroup(typeGroup, this);
	if (inPlace)
		_flags |= kMemoryBlocked;
}

// Confirmed (asm lines 593292-593464)
TVisionaireObject::~TVisionaireObject() {
	x_assert(_refCount == 0, "RefCount == 0", kSourceFile, 0x11C);

	if (_luaObject)
		LuaObjectUnref(_id, _luaObject);

	if (TVisionaire::ActiveInstances > 0 && _visionaire && !_visionaire->IsLinkRemovalSuppressed())
		unlinkFromObjectsLinkedTo();

	delete _data;
	_data = nullptr;
}

// The part of Remove() and the destructor that tells the objects linking to
// this one to drop those links.
void TVisionaireObject::unlinkFromObjectsLinkedTo() {
	if (_id.getId() == -1 || _id.getTable() == 0xFF)
		return;

	std::vector<TLinkRef> linked;
	_visionaire->GetObjectsLinkedTo(_id, linked);
	for (const TLinkRef &ref : linked) {
		TVisionaireObject *object = _visionaire->GetObjectById(ref.from);
		if (object && object->isValid())
			object->_data->RemoveLink(ref.field, _id, false);
	}
}

// Confirmed (asm lines 587916-587961): the original's assignments assert.
TVisionaireObject &TVisionaireObject::operator=(const TVisionaireObject &/*other*/) {
	x_assert(false, "false", kSourceFile, 0);
	return *this;
}

TVisionaireObject &TVisionaireObject::operator=(const TVisionaireObject * /*other*/) {
	x_assert(false, "false", kSourceFile, 0);
	return *this;
}

// Confirmed (asm lines 587962-588074): "any" objects equal each other, a
// removed object equals another removed (or empty) one, otherwise the ids
// (24 bits) and tables are compared.
bool TVisionaireObject::operator==(const TVisionaireObject &other) const {
	if (_flags & kAnyObject)
		return (other._flags & kAnyObject) != 0;
	if (other._flags & kAnyObject)
		return false;

	bool thisEmpty = (_flags & kRemoved) || _id.getId() == -1 || !_data;
	bool otherEmpty = (other._flags & kRemoved) || other._id.getId() == -1 || !other._data;
	if (thisEmpty || otherEmpty)
		return thisEmpty && otherEmpty;

	return _id.getId() == other._id.getId() && _id.getTable() == other._id.getTable();
}

// Confirmed (asm lines 587508-587559)
void TVisionaireObject::Init(TVisionaireObject *parent, int field) {
	_parentField = (short)field;
	if (!parent)
		return;

	_parentId = parent->_id;
	if (!parent->isValid())
		return;
	parent->_data->SetParentLink(field, _id, false);
}

// Confirmed (asm lines 587595-587612)
TVisionaireObject *TVisionaireObject::GetReference() {
	_refCount++;
	return this;
}

// Confirmed (asm lines 587613-587651)
void TVisionaireObject::Release() {
	x_assert(_refCount - 1 >= 0 && _refCount - 1 <= 0xBB6, kRefCountAssert, kSourceFile, 0x142);
	if (--_refCount == 0)
		delete this;
}

// Confirmed (asm lines 593103-593291)
void TVisionaireObject::Remove(bool fromVisionaire, bool unlink) {
	if (fromVisionaire)
		_visionaire->RemoveObject(this);

	if (_luaObject) {
		LuaObjectUnref(_id, _luaObject);
		_luaObject = 0;
	}

	if (unlink || fromVisionaire) {
		if (_visionaire && !_visionaire->IsLinkRemovalSuppressed())
			unlinkFromObjectsLinkedTo();

		delete _data;
		_data = nullptr;
		_id = TId(-1, -1);
		_parentId = TId(-1, -1);
		_parentField = -1;
	}

	_flags |= kRemoved;
	Release();
}

// Confirmed (asm lines 587669-587684)
const TTypeGroup *TVisionaireObject::GetTypeGroup() {
	return _data->GetTypeGroupPtr();
}

// Confirmed (asm lines 587685-587726)
TVisionaireObject *TVisionaireObject::GetParent() const {
	if (!isValid())
		return nullptr;
	return _visionaire->GetObjectById(_parentId);
}

// Confirmed (asm lines 587780-587816)
bool TVisionaireObject::IsEmpty() const {
	if (_flags & kRemoved)
		return true;
	if (_id.getId() == -1)
		return true;
	return _data == nullptr;
}

// Confirmed (asm lines 587835-587915)
bool TVisionaireObject::IsTemporary() const {
	return isValid() && _data->IsTemporary();
}

void TVisionaireObject::SetTemporary(bool temporary) {
	if (isValid())
		_data->SetTemporary(temporary);
}

bool TVisionaireObject::ChangeOrder(TMoveOrderEnum move) {
	TVisObjRef self(*this);
	return _visionaire->ChangeOrder(self, move);
}

// Confirmed (asm lines 588075-588169)
int TVisionaireObject::GetTypeLink(int field) const {
	if (!isValid())
		return -2;
	return _data->GetTypeGroupPtr()->GetTypeLink(field);
}

int TVisionaireObject::GetTypeField(int field) const {
	if (!isValid())
		return -1;
	return (int)_data->GetTypeGroupPtr()->GetType(field, true);
}

// Confirmed (asm lines 588170-588354): renaming also tells the object's
// table, which keeps its objects in name order.
bool TVisionaireObject::SetName(const TCharHolder &name) {
	if (!isValid())
		return false;

	TTable *table = nullptr;
	int position = -1;
	if (_visionaire->GetTable(_id.getTable() > 127 ? _id.getTable() - 256 : _id.getTable(), &table)) {
		x_assert(table != nullptr, "pTable != NULL", kSourceFile, 0);
		unsigned long found = 0;
		if (table && table->GetObjectPosition(wxString(_data->GetName()), _id, found))
			position = (int)found;
	}

	if (!_data->SetName(name))
		return false;

	if (table)
		table->ObjectNameChanged(this, position);
	_visionaire->SetModified(true);
	return true;
}

// Confirmed (asm lines 588355-588414)
const TCharHolder &TVisionaireObject::GetName() const {
	if (!isValid())
		return emptyName();
	return _data->GetName();
}

// Confirmed (asm lines 588415-588744)
wxString TVisionaireObject::GetNameWithParents(int depth) const {
	wxString result(GetName());
	if (!isValid())
		return result;

	const TVisionaireObject *current = this;
	for (int level = 0; level < depth; level++) {
		if (!current->isValid())
			break;
		TVisionaireObject *parent = _visionaire->GetObjectById(current->_parentId);
		if (!parent || !parent->isValid() || parent->_id.getTable() == 0xFF)
			break;

		result = wxString(parent->_data->GetName()) + wxString(L": ") + result;
		current = parent;
	}
	return result;
}

// Confirmed (asm lines 588745-588888)
const wxString &TVisionaireObject::GetNameInList() const {
	static wxString name;

	if (isValid() && _data->GetTypeGroupPtr()->GetNameInList(this, name))
		return name;
	name = wxString(GetName());
	return name;
}

// Confirmed (asm lines 588990-589119)
long TVisionaireObject::GetLastModified() const {
	if (!isValid())
		return -1;
	return _data->GetLastModified();
}

void TVisionaireObject::SetLastModified(long stamp) {
	if (!isValid())
		return;
	_data->Serialize(_id.getId(), _order, (int)stamp, _data->GetName());
}

void TVisionaireObject::SetLastModified() {
	SetLastModified(_visionaire->GetModifiedStamp());
}

// Confirmed (asm lines 589120-589203)
bool TVisionaireObject::SetValue(int field, const void *value, eTypeData type, TSendEventEnum event) {
	if (!isValid())
		return false;

	if (type == eTypeData::kLinkList)
		UnrefLuaFieldsCache(_data->GetId(), field);

	if (!_data->SetValue(field, value, type, event))
		return false;
	_visionaire->SetModified(true);
	return true;
}

// Confirmed (asm lines 589204-589249)
bool TVisionaireObject::IsFieldEmpty(int field) const {
	if (!isValid())
		return true;
	return GetRefVPoint(_data->GetValue(field, eTypeData::kPointList)).empty();
}

// Confirmed (asm lines 589250-589291)
void *TVisionaireObject::GetValue(int field, eTypeData type) const {
	if (!isValid())
		return nullptr;
	return _data->GetValue(field, type);
}

// Confirmed (asm lines 589292-589397 and 589398-589459)
void TVisionaireObject::GetTexts(int field, std::vector<TTextLanguage> **out) const {
	static std::vector<TTextLanguage> empty;

	if (!isValid()) {
		*out = &empty;
		return;
	}
	*out = &GetRefVText(_data->GetValue(field, eTypeData::kTextList));
}

void TVisionaireObject::GetPaths(int field, std::vector<TCharHolder> **out) const {
	if (!isValid())
		return;
	*out = &GetRefVPath(_data->GetValue(field, eTypeData::kPathList));
}

// The simple getters (asm lines 589460-590250) all follow one shape.
int TVisionaireObject::GetInt(int field) const {
	if (!isValid())
		return -1;
	return GetRefInt(_data->GetValue(field, eTypeData::kInt));
}

float TVisionaireObject::GetFloat(int field) const {
	if (!isValid())
		return 0.0f;
	return GetRefFloat(_data->GetValue(field, eTypeData::kFloat));
}

bool TVisionaireObject::GetBool(int field) const {
	if (!isValid())
		return false;
	return GetRefBool(_data->GetValue(field, eTypeData::kBool));
}

wxString TVisionaireObject::GetStr(int field) const {
	if (!isValid())
		return wxString();
	return wxString(GetRefString(_data->GetValue(field, eTypeData::kString)));
}

const TCharHolder &TVisionaireObject::GetStrHolder(int field) const {
	static TCharHolder empty;

	if (!isValid())
		return empty;
	return GetRefString(_data->GetValue(field, eTypeData::kString));
}

wxFileName TVisionaireObject::GetPath(int field) const {
	static wxFileName empty;

	if (!isValid())
		return empty;
	return (wxFileName)GetRefPath(_data->GetValue(field, eTypeData::kPath));
}

const wxPoint *TVisionaireObject::GetPoint(int field) const {
	static wxPoint empty;

	if (!isValid())
		return &empty;
	return &GetRefPoint(_data->GetValue(field, eTypeData::kPoint));
}

const wxRect *TVisionaireObject::GetRect(int field) const {
	static wxRect empty;

	if (!isValid())
		return &empty;
	return &GetRefRect(_data->GetValue(field, eTypeData::kRect));
}

const TSprite &TVisionaireObject::GetSprite(int field) const {
	static TSprite empty;

	if (!isValid())
		return empty;
	return GetRefSprite(_data->GetValue(field, eTypeData::kSprite));
}

// Confirmed (asm lines 590251-590356)
TVisionaireObject *TVisionaireObject::GetLink(int field) const {
	if (!isValid())
		return nullptr;

	const TLink &link = GetRefLink(_data->GetValue(field, eTypeData::kLink));
	if (link.IsAnyLink())
		return nullptr;
	if (link.GetId().getId() == -1 && link.GetId().getTable() == 0xFF)
		return nullptr;
	return _visionaire->GetObjectById(link.GetId());
}

// Confirmed (asm lines 590357-590453): the fallback is an "any" link.
const TLink &TVisionaireObject::GetLinkId(int field) const {
	static TLink empty(true);

	if (!isValid())
		return empty;
	return GetRefLink(_data->GetValue(field, eTypeData::kLink));
}

// Confirmed (asm lines 590454-590761)
TVisionaireObject *TVisionaireObject::GetLinkByNameIgnoreCase(const wxString &name, int field) {
	if (!isValid())
		return nullptr;

	TCharHolder wanted(name);
	for (const TLink &link : GetRefLinks(_data->GetValue(field, eTypeData::kLinkList))) {
		TVisionaireObject *object = _visionaire->GetObjectById(link.GetId());
		if (object && object->GetName().CmpNoCase(wanted) == 0)
			return object;
	}
	return nullptr;
}

TVisionaireObject *TVisionaireObject::GetLinkByName(const wxString &name, int field) {
	if (!isValid())
		return nullptr;

	TCharHolder wanted(name);
	for (const TLink &link : GetRefLinks(_data->GetValue(field, eTypeData::kLinkList))) {
		TVisionaireObject *object = _visionaire->GetObjectById(link.GetId());
		if (object && object->GetName().Cmp(wanted) == 0)
			return object;
	}
	return nullptr;
}

// Confirmed (asm lines 590762-590852)
bool TVisionaireObject::GetList(int field, TVList &out) const {
	if (!isValid())
		return false;

	out.clear();
	return _data->GetList(field, out);
}

// Confirmed (asm lines 595390-595620)
bool TVisionaireObject::GetLinks(int field, TypeOrder order, TVList &out) const {
	if (!isValid())
		return false;

	out.clear();
	for (const TLink &link : GetRefLinks(_data->GetValue(field, eTypeData::kLinkList))) {
		TVisionaireObject *object = _visionaire->GetObjectById(link.GetId());
		if (object)
			out.push_back(object);
	}

	if (order == TypeOrder::kValue1)
		std::sort(out.begin(), out.end(), cmpOrder);
	return !out.empty();
}

// Confirmed (asm lines 590853-591028): the number of elements of any list
// kind.
unsigned long TVisionaireObject::GetLinksListSize(int field) const {
	if (!isValid())
		return 0;

	switch (_data->GetTypeGroupPtr()->GetType(field, true)) {
	case eTypeData::kRectList:
		return GetRefVRect(_data->GetValue(field, eTypeData::kRectList)).size();
	case eTypeData::kSpriteList:
		return GetRefVSprite(_data->GetValue(field, eTypeData::kSpriteList)).size();
	case eTypeData::kPointList:
		return GetRefVPoint(_data->GetValue(field, eTypeData::kPointList)).size();
	case eTypeData::kStringList:
		return GetRefVString(_data->GetValue(field, eTypeData::kStringList)).size();
	case eTypeData::kIntList:
		return GetRefVInt(_data->GetValue(field, eTypeData::kIntList)).size();
	case eTypeData::kFloatList:
		return GetRefVFloat(_data->GetValue(field, eTypeData::kFloatList)).size();
	case eTypeData::kLinkList:
		return GetRefLinks(_data->GetValue(field, eTypeData::kLinkList)).size();
	default:
		return 0;
	}
}

// Confirmed (asm lines 591029-591137): the highest order among the linked
// objects, -1 if there are none.
int TVisionaireObject::GetLinksHighestOrder(int field) const {
	if (!isValid())
		return -1;

	int highest = -1;
	for (const TLink &link : GetRefLinks(_data->GetValue(field, eTypeData::kLinkList))) {
		TVisionaireObject *object = _visionaire->GetObjectById(link.GetId());
		if (object && object->GetOrder() > highest)
			highest = object->GetOrder();
	}
	return highest;
}

// The typed setters (asm lines 591138-592145) all forward to SetValue() with
// the value's address and its kind.
bool TVisionaireObject::SetValue(int field, bool value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kBool, event);
}

bool TVisionaireObject::SetValue(int field, int value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kInt, event);
}

bool TVisionaireObject::SetValue(int field, float value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kFloat, event);
}

bool TVisionaireObject::SetValue(int field, const TCharHolder &value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kString, event);
}

bool TVisionaireObject::SetValue(int field, const wxPoint &value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kPoint, event);
}

bool TVisionaireObject::SetValue(int field, const wxRect &value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kRect, event);
}

bool TVisionaireObject::SetValue(int field, const wxFileName &value, TSendEventEnum event) {
	TCharHolder holder(value);
	return SetValue(field, &holder, eTypeData::kPath, event);
}

bool TVisionaireObject::SetValue(int field, const wxString &value, TSendEventEnum event) {
	TCharHolder holder(value);
	return SetValue(field, &holder, eTypeData::kString, event);
}

bool TVisionaireObject::SetValue(int field, const TSprite &value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kSprite, event);
}

bool TVisionaireObject::SetValue(int field, const std::vector<wxRect> &value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kRectList, event);
}

bool TVisionaireObject::SetValue(int field, const std::vector<TSprite> &value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kSpriteList, event);
}

bool TVisionaireObject::SetValue(int field, const std::vector<wxPoint> &value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kPointList, event);
}

bool TVisionaireObject::SetValue(int field, const std::vector<TCharHolder> &value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kStringList, event);
}

bool TVisionaireObject::SetPathList(int field, const std::vector<TCharHolder> &value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kPathList, event);
}

bool TVisionaireObject::SetValue(int field, const std::vector<int> &value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kIntList, event);
}

bool TVisionaireObject::SetValue(int field, const std::vector<float> &value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kFloatList, event);
}

bool TVisionaireObject::SetValue(int field, const std::vector<TTextLanguage> &value, TSendEventEnum event) {
	return SetValue(field, &value, eTypeData::kTextList, event);
}

// Confirmed (asm lines 592316-592507)
bool TVisionaireObject::SetValue(int field, const TVList &objects, bool notify) {
	if (!isValid())
		return false;

	UnrefLuaFieldsCache(_data->GetId(), field);
	_data->ClearLinks(field, false);
	for (const TVisionaireObject *object : objects) {
		if (object->isValid())
			_data->SetLink(field, object->_id, false);
	}
	if (notify)
		_data->NotifyEvent(field, TEventEnum::kChanged, nullptr);
	return true;
}

// Confirmed (asm lines 592146-592315): plain forwards to the record.
bool TVisionaireObject::ClearLink(int field, bool notify) {
	return _data->ClearLink(field, notify);
}

bool TVisionaireObject::SetLinkAnyObject(int field, bool notify) {
	return _data->SetLinkAnyObject(field, notify);
}

bool TVisionaireObject::SetLink(int field, const TId &id, bool notify) {
	return _data->SetLink(field, id, notify);
}

bool TVisionaireObject::RemoveLink(int field, const TId &id, bool notify) {
	return _data->RemoveLink(field, id, notify);
}

// Confirmed (asm lines 592508-592739)
bool TVisionaireObject::IsLinked(int field, const TVisionaireObject *other) const {
	x_assert(other != nullptr, "other != NULL", kSourceFile, 0x47E);
	if (!isValid() || !other || !other->isValid())
		return false;

	switch (_data->GetTypeGroupPtr()->GetType(field, true)) {
	case eTypeData::kLink: {
		const TId &id = GetRefLink(_data->GetValue(field, eTypeData::kLink)).GetId();
		return id.getId() == other->_id.getId() && id.getTable() == other->_id.getTable();
	}
	case eTypeData::kLinkList:
		for (const TLink &link : GetRefLinks(_data->GetValue(field, eTypeData::kLinkList))) {
			if (link.GetId().getId() == other->_id.getId() && link.GetId().getTable() == other->_id.getTable())
				return true;
		}
		return false;
	default:
		return false;
	}
}

// Confirmed (asm lines 592740-592877): copies the record, then the name
// through SetName() so the table notices.
void TVisionaireObject::CopyContent(const TVisionaireObject *source, bool copyParents, std::vector<int> *skip) {
	x_assert(source != nullptr, "srcObject != NULL", kSourceFile, 0x49D);
	if (!isValid() || !source || !source->isValid())
		return;

	_data->CopyContent(*source->_data, copyParents, skip);
	SetName(_data->GetName());
}

// Confirmed (asm lines 592878-592920)
unsigned long TVisionaireObject::GetSizeMemory(bool deep, bool /*unused*/) const {
	if (!isValid())
		return 0;
	return _data->GetSizeMemory(deep);
}

// Confirmed (asm lines 592921-593022)
unsigned long TVisionaireObject::GetLinksSizeMemory(int field) const {
	unsigned long size = 0;

	if (!isValid())
		return 0;
	for (const TLink &link : GetRefLinks(_data->GetValue(field, eTypeData::kLinkList))) {
		TVisionaireObject *object = _visionaire->GetObjectById(link.GetId());
		if (object && object->isValid())
			size += object->_data->GetSizeMemory(false);
	}
	return size;
}

// Confirmed (asm lines 593023-593102)
void TVisionaireObject::RegisterEventHandler(TEventHandlerInterface *handler, TEventEnum event) {
	if (isValid())
		_data->RegisterEventHandler(handler, event);
}

void TVisionaireObject::UnRegisterEventHandler(TEventHandlerInterface *handler) {
	if (isValid())
		_data->UnRegisterEventHandler(handler);
}

// Confirmed (asm lines 593465-593860, 593849-594289): the "copy a list out"
// getters leave the output alone for an invalid object.
void TVisionaireObject::GetRects(int field, std::vector<wxRect> &out) const {
	if (isValid())
		out = GetRefVRect(_data->GetValue(field, eTypeData::kRectList));
}

void TVisionaireObject::GetPoints(int field, std::vector<wxPoint> &out) const {
	if (isValid())
		out = GetRefVPoint(_data->GetValue(field, eTypeData::kPointList));
}

void TVisionaireObject::GetSprites(int field, std::vector<TSprite> &out) const {
	if (isValid())
		out = GetRefVSprite(_data->GetValue(field, eTypeData::kSpriteList));
}

void TVisionaireObject::GetPaths(int field, std::vector<TCharHolder> &out) const {
	if (isValid())
		out = GetRefVPath(_data->GetValue(field, eTypeData::kPathList));
}

void TVisionaireObject::GetStrings(int field, std::vector<TCharHolder> &out) const {
	if (isValid())
		out = GetRefVString(_data->GetValue(field, eTypeData::kStringList));
}

void TVisionaireObject::GetInts(int field, std::vector<int> &out) const {
	if (isValid())
		out = GetRefVInt(_data->GetValue(field, eTypeData::kIntList));
}

void TVisionaireObject::GetFloats(int field, std::vector<float> &out) const {
	if (isValid())
		out = GetRefVFloat(_data->GetValue(field, eTypeData::kFloatList));
}

void TVisionaireObject::GetTextsCopy(int field, std::vector<TTextLanguage> &out) const {
	if (isValid())
		out = GetRefVText(_data->GetValue(field, eTypeData::kTextList));
}

// Confirmed (asm lines 594290-594475)
void TVisionaireObject::GetObjectsLinkedTo(TVList &out, std::vector<int> &fields) const {
	out.clear();
	if (!isValid())
		return;

	std::vector<TLinkRef> linked;
	_visionaire->GetObjectsLinkedTo(_id, linked);
	for (const TLinkRef &ref : linked) {
		TVisionaireObject *object = _visionaire->GetObjectById(ref.from);
		if (!object)
			continue;
		out.push_back(object);
		fields.push_back(ref.field);
	}
}

// Confirmed (asm lines 594476-594888): the objects behind the parent links.
void TVisionaireObject::GetChildren(TVList &out) const {
	if (!isValid())
		return;

	for (const TTypeData *type : _data->GetTypeGroupPtr()->GetTypes()) {
		int field = type->GetDescription();

		if (type->GetType() == eTypeData::kLink) {
			const TLink &link = GetRefLink(_data->GetValue(field, eTypeData::kLink));
			if (!link.IsParentLink() || (link.GetId().getId() == -1 && link.GetId().getTable() == 0xFF))
				continue;

			TVisionaireObject *object = _visionaire->GetObjectById(link.GetId());
			if (object)
				out.push_back(object);
		} else if (type->GetType() == eTypeData::kLinkList) {
			const std::vector<TLink> &links = GetRefLinks(_data->GetValue(field, eTypeData::kLinkList));
			if (links.empty() || !links.front().IsParentLink())
				continue;

			for (const TLink &link : links) {
				TVisionaireObject *object = _visionaire->GetObjectById(link.GetId());
				if (object)
					out.push_back(object);
			}
		}
	}
}

// Confirmed in shape (asm lines 594889-595325); the meaning of the two flag
// vectors (list or single link, parent link or not) is read from how they are
// filled, not verified.
bool TVisionaireObject::ContainsLinkTo(const TVisionaireObject &other, std::vector<int> &fields,
                                       std::vector<bool> &isList, std::vector<bool> &isParent) {
	if (!isValid())
		return false;

	bool found = false;
	for (const TTypeData *type : _data->GetTypeGroupPtr()->GetTypes()) {
		int field = type->GetDescription();

		if (type->GetType() == eTypeData::kLink) {
			const TLink &link = GetRefLink(_data->GetValue(field, eTypeData::kLink));
			if (link.GetId().getId() != other._id.getId() || link.GetId().getTable() != other._id.getTable())
				continue;

			fields.push_back(field);
			isList.push_back(false);
			isParent.push_back(link.IsParentLink());
			found = true;
		} else if (type->GetType() == eTypeData::kLinkList) {
			const std::vector<TLink> &links = GetRefLinks(_data->GetValue(field, eTypeData::kLinkList));
			for (const TLink &link : links) {
				if (link.GetId().getId() != other._id.getId() || link.GetId().getTable() != other._id.getTable())
					continue;

				fields.push_back(field);
				isList.push_back(true);
				isParent.push_back(link.IsParentLink());
				found = true;
			}
		}
	}
	return found;
}

// Confirmed (asm lines 586479-586512)
bool cmpOrder(const TVisionaireObject *a, const TVisionaireObject *b) {
	return a->GetOrder() < b->GetOrder();
}

bool cmpId(const TVisionaireObject *a, const TVisionaireObject *b) {
	return a->GetId24() < b->GetId24();
}
