#include "datastruct/visobjref.h"

#include "Diagnostics.h"
#include "TSprite.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "datastruct/vlist.h"

// Every method below is the one-line forward the asm shows (lines 597612-
// 600388), with the default for an empty reference.

TVisObjRef::TVisObjRef(const TVisionaireObject *object)
	: _object(object ? const_cast<TVisionaireObject *>(object)->GetReference() : nullptr) {
}

TVisObjRef::TVisObjRef(const TVisionaireObject &object)
	: _object(const_cast<TVisionaireObject &>(object).GetReference()) {
}

TVisObjRef::TVisObjRef(const TVisObjRef &other) : _object(other._object ? other._object->GetReference() : nullptr) {
}

TVisObjRef::~TVisObjRef() {
	Clear();
}

TVisObjRef &TVisObjRef::operator=(const TVisObjRef &other) {
	if (this != &other) {
		Clear();
		if (other._object)
			_object = other._object->GetReference();
	}
	return *this;
}

void TVisObjRef::Clear() {
	if (_object) {
		_object->Release();
		_object = nullptr;
	}
}

void TVisObjRef::Remove() {
	if (_object) {
		_object->Remove(true, false);
		_object = nullptr;
	}
}

void TVisObjRef::Set(TVisionaireObject *object) {
	Clear();
	if (object)
		_object = object->GetReference();
}

TVisionaireObject *TVisObjRef::GetReference() const {
	return _object ? _object->GetReference() : nullptr;
}

bool TVisObjRef::operator==(const TVisObjRef &other) const {
	if (this == &other)
		return true;
	if (_object)
		return other._object ? *_object == *other._object : false;
	return other._object == nullptr;
}

bool TVisObjRef::operator==(const TVisionaireObject &other) const {
	if (_object)
		return *_object == other;
	return other.IsEmpty();
}

bool TVisObjRef::IsEmpty() const {
	return _object ? _object->IsEmpty() : true;
}

bool TVisObjRef::IsAnyObject() const {
	return _object ? _object->IsAnyObject() : false;
}

int TVisObjRef::GetTypeLink(int fieldId) const {
	return _object ? _object->GetTypeLink(fieldId) : -2;
}

int TVisObjRef::GetTypeField(int fieldId) const {
	return _object ? _object->GetTypeField(fieldId) : -1;
}

TVisObjRef TVisObjRef::GetParent() const {
	TVisObjRef parent;
	if (_object)
		parent.Set(_object->GetParent());
	return parent;
}

int TVisObjRef::GetParentField() const {
	return _object ? _object->GetParentField() : -1;
}

const std::uint8_t *TVisObjRef::GetId() const {
	static const TId emptyId(-1, -1);

	if (_object)
		return _object->GetId();
	return reinterpret_cast<const std::uint8_t *>(&emptyId);
}

long TVisObjRef::GetOrder() const {
	return _object ? _object->GetOrder() : -1;
}

bool TVisObjRef::ChangeOrder(TMoveOrderEnum move) {
	return _object ? _object->ChangeOrder(move) : false;
}

void TVisObjRef::SetName(const TCharHolder &name) {
	if (_object)
		_object->SetName(name);
}

TCharHolder TVisObjRef::GetName() const {
	static TCharHolder empty;
	return _object ? _object->GetName() : empty;
}

wxString TVisObjRef::GetNameWithParents(int levels) const {
	return _object ? _object->GetNameWithParents(levels) : wxString();
}

void TVisObjRef::GetChildren(TVList &outChildren) const {
	if (_object)
		_object->GetChildren(outChildren);
}

bool TVisObjRef::GetLinkByNameIgnoreCase(const wxString &name, int fieldId, TVisObjRef &outLink) {
	if (!_object)
		return false;
	TVisionaireObject *found = _object->GetLinkByNameIgnoreCase(name, fieldId);
	if (!found)
		return false;
	outLink.Set(found);
	return true;
}

bool TVisObjRef::GetLinkByName(const wxString &name, int fieldId, TVisObjRef &outLink) {
	if (!_object)
		return false;
	TVisionaireObject *found = _object->GetLinkByName(name, fieldId);
	if (!found)
		return false;
	outLink.Set(found);
	return true;
}

int TVisObjRef::GetInt(int fieldId) const {
	return _object ? _object->GetInt(fieldId) : -1;
}

float TVisObjRef::GetFloat(int fieldId) const {
	return _object ? _object->GetFloat(fieldId) : 0.0f;
}

bool TVisObjRef::GetBool(int fieldId) const {
	return _object ? _object->GetBool(fieldId) : false;
}

wxString TVisObjRef::GetStr(int fieldId) const {
	return _object ? _object->GetStr(fieldId) : wxString();
}

const TCharHolder &TVisObjRef::GetStrHolder(int fieldId) const {
	static TCharHolder empty;
	return _object ? _object->GetStrHolder(fieldId) : empty;
}

wxFileName TVisObjRef::GetPath(int fieldId) const {
	return _object ? _object->GetPath(fieldId) : wxFileName();
}

const wxPoint *TVisObjRef::GetPoint(int fieldId) const {
	static wxPoint empty;
	return _object ? _object->GetPoint(fieldId) : &empty;
}

const wxRect *TVisObjRef::GetRect(int fieldId) const {
	static wxRect empty;
	return _object ? _object->GetRect(fieldId) : &empty;
}

const TSprite &TVisObjRef::GetSprite(int fieldId) const {
	static TSprite empty;
	return _object ? _object->GetSprite(fieldId) : empty;
}

TVisObjRef TVisObjRef::GetLink(int fieldId) const {
	TVisObjRef link;
	if (_object)
		link.Set(_object->GetLink(fieldId));
	return link;
}

void TVisObjRef::GetLinks(int fieldId, TypeOrder order, TVList &outLinks) const {
	if (_object)
		_object->GetLinks(fieldId, order, outLinks);
}

void TVisObjRef::GetPoints(int fieldId, std::vector<wxPoint> &outPoints) const {
	if (_object)
		_object->GetPoints(fieldId, outPoints);
}

void TVisObjRef::GetRects(int fieldId, std::vector<wxRect> &outRects) const {
	if (_object)
		_object->GetRects(fieldId, outRects);
}

void TVisObjRef::GetSprites(int fieldId, std::vector<TSprite> &outSprites) const {
	if (_object)
		_object->GetSprites(fieldId, outSprites);
}

void TVisObjRef::GetStrings(int fieldId, std::vector<TCharHolder> &outStrings) const {
	if (_object)
		_object->GetStrings(fieldId, outStrings);
}

void TVisObjRef::GetPaths(int fieldId, std::vector<TCharHolder> **outPaths) const {
	if (_object)
		_object->GetPaths(fieldId, outPaths);
}

void TVisObjRef::GetPaths(int fieldId, std::vector<TCharHolder> &outPaths) const {
	if (_object)
		_object->GetPaths(fieldId, outPaths);
}

void TVisObjRef::GetInts(int fieldId, std::vector<int> &outInts) const {
	if (_object)
		_object->GetInts(fieldId, outInts);
}

void TVisObjRef::GetFloats(int fieldId, std::vector<float> &outFloats) const {
	if (_object)
		_object->GetFloats(fieldId, outFloats);
}

void TVisObjRef::GetTexts(int fieldId, std::vector<TTextLanguage> **outTexts) const {
	if (_object)
		_object->GetTexts(fieldId, outTexts);
}

void TVisObjRef::GetTextsCopy(int fieldId, std::vector<TTextLanguage> &outTexts) const {
	if (_object)
		_object->GetTextsCopy(fieldId, outTexts);
}

void TVisObjRef::GetList(int fieldId, TVList &outList) const {
	if (_object)
		_object->GetList(fieldId, outList);
}

unsigned long TVisObjRef::GetLinksListSize(int fieldId) const {
	return _object ? _object->GetLinksListSize(fieldId) : 0;
}

int TVisObjRef::GetLinksHighestOrder(int fieldId) const {
	return _object ? _object->GetLinksHighestOrder(fieldId) : -1;
}

void TVisObjRef::SetValue(int fieldId, bool value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, int value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, float value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const TCharHolder &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const wxPoint &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const wxRect &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const wxFileName &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const wxString &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const TSprite &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const TVList &value, bool notify) {
	if (_object)
		_object->SetValue(fieldId, value, notify);
}

void TVisObjRef::SetValue(int fieldId, const std::vector<wxRect> &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const std::vector<TSprite> &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const std::vector<wxPoint> &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const std::vector<TCharHolder> &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetPathList(int fieldId, const std::vector<TCharHolder> &value, TSendEventEnum event) {
	if (_object)
		_object->SetPathList(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const std::vector<int> &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const std::vector<float> &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetValue(int fieldId, const std::vector<TTextLanguage> &value, TSendEventEnum event) {
	if (_object)
		_object->SetValue(fieldId, value, event);
}

void TVisObjRef::SetLinkAnyObject(int fieldId, bool notify) {
	if (_object)
		_object->SetLinkAnyObject(fieldId, notify);
}

void TVisObjRef::SetLink(int fieldId, const TVisObjRef &value, bool notify) {
	static const TId emptyId(-1, -1);

	if (_object)
		_object->SetLink(fieldId, value._object ? value._object->GetTId() : emptyId, notify);
}

void TVisObjRef::RemoveLink(int fieldId, const TId &id, bool notify) {
	if (_object)
		_object->RemoveLink(fieldId, id, notify);
}

void TVisObjRef::ClearLink(int fieldId, bool notify) {
	if (_object)
		_object->ClearLink(fieldId, notify);
}

void TVisObjRef::GetObjectsLinkedTo(TVList &out, std::vector<int> &fields) const {
	if (_object)
		_object->GetObjectsLinkedTo(out, fields);
}

bool TVisObjRef::ContainsLinkTo(const TVisObjRef &other, std::vector<int> &fields, std::vector<bool> &isList,
                                std::vector<bool> &isParent) {
	return _object && other._object && _object->ContainsLinkTo(*other._object, fields, isList, isParent);
}

unsigned long TVisObjRef::GetSizeMemory(bool deep, bool unused) const {
	return _object ? _object->GetSizeMemory(deep, unused) : 0;
}

void TVisObjRef::CopyContent(const TVisObjRef &source, bool copyParents, std::vector<int> *skip) {
	if (_object)
		_object->CopyContent(source._object, copyParents, skip);
}

void TVisObjRef::SetTemporary(bool temporary) {
	if (_object)
		_object->SetTemporary(temporary);
}

long TVisObjRef::GetLastModified() const {
	return _object ? _object->GetLastModified() : -1;
}

void TVisObjRef::SetLastModified(long stamp) {
	if (_object)
		_object->SetLastModified(stamp);
}

void TVisObjRef::SetLastModified() {
	if (_object)
		_object->SetLastModified();
}

void TVisObjRef::RegisterEventHandler(TEventHandlerInterface *handler, TEventEnum event) {
	if (_object)
		_object->RegisterEventHandler(handler, event);
}

void TVisObjRef::UnRegisterEventHandler(TEventHandlerInterface *handler) {
	if (_object)
		_object->UnRegisterEventHandler(handler);
}

TVisionaire *TVisObjRef::GetVisionaire() const {
	x_assert(_object != nullptr, "m_pObject != NULL", "/home/simon/Documents/jenkins/branchPillars/src/datastruct/visobjref.cpp", 0);
	return _object ? _object->GetVisionaire() : nullptr;
}
