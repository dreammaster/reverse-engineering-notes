#include "datastruct/visobjref.h"

#include "datastruct/vlist.h"

TVisObjRef::TVisObjRef(const TVisionaireObject &/*object*/) {
}

TVisObjRef::TVisObjRef(const TVisionaireObject */*object*/) {
}

bool TVisObjRef::operator==(const TVisObjRef &other) const {
	return _id[0] == other._id[0] && _id[1] == other._id[1] && _id[2] == other._id[2] &&
	       _id[3] == other._id[3];
}

bool TVisObjRef::GetBool(int /*fieldId*/) const {
	return false;
}

int TVisObjRef::GetInt(int /*fieldId*/) const {
	return 0;
}

wxString TVisObjRef::GetStr(int /*fieldId*/) const {
	return wxString();
}

TCharHolder TVisObjRef::GetName() const {
	return TCharHolder();
}

std::wstring TVisObjRef::GetPath(int /*fieldId*/) const {
	return std::wstring();
}

TVisObjRef TVisObjRef::GetLink(int /*fieldId*/) const {
	return TVisObjRef();
}

TVisObjRef TVisObjRef::GetParent() const {
	return TVisObjRef();
}

void TVisObjRef::ClearLink(int /*fieldId*/, bool /*flag*/) {
}

void TVisObjRef::SetLink(int /*fieldId*/, const TVisObjRef &/*value*/, bool /*flag*/) {
}

void TVisObjRef::SetValue(int /*fieldId*/, const wxPoint &/*value*/, TSendEventEnum /*event*/) {
}

void TVisObjRef::SetValue(int /*fieldId*/, bool /*value*/, TSendEventEnum /*event*/) {
}

void TVisObjRef::SetValue(int /*fieldId*/, int /*value*/, TSendEventEnum /*event*/) {
}

void TVisObjRef::SetValue(int /*fieldId*/, const wxString &/*value*/, TSendEventEnum /*event*/) {
}

const wxPoint *TVisObjRef::GetPoint(int /*fieldId*/) const {
	return &_point;
}

const wxRect *TVisObjRef::GetRect(int /*fieldId*/) const {
	return &_rect;
}

void TVisObjRef::GetLinks(int /*fieldId*/, TypeOrder /*order*/, TVList &/*outLinks*/) const {
}

void TVisObjRef::GetList(int /*fieldId*/, TVList &/*outList*/) const {
}

const std::uint8_t *TVisObjRef::GetId() const {
	return _id;
}
