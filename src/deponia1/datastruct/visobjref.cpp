#include "datastruct/visobjref.h"

bool TVisObjRef::operator==(const TVisObjRef& other) const {
    return m_id[0] == other.m_id[0] && m_id[1] == other.m_id[1] && m_id[2] == other.m_id[2] &&
           m_id[3] == other.m_id[3];
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

std::wstring TVisObjRef::GetPath(int /*fieldId*/) const {
    return std::wstring();
}

TVisObjRef TVisObjRef::GetLink(int /*fieldId*/) const {
    return TVisObjRef();
}

void TVisObjRef::ClearLink(int /*fieldId*/, bool /*flag*/) {
}

void TVisObjRef::SetLink(int /*fieldId*/, const TVisObjRef& /*value*/, bool /*flag*/) {
}

void TVisObjRef::SetValue(int /*fieldId*/, const wxPoint& /*value*/, TSendEventEnum /*event*/) {
}

void TVisObjRef::SetValue(int /*fieldId*/, bool /*value*/, TSendEventEnum /*event*/) {
}

const wxPoint* TVisObjRef::GetPoint(int /*fieldId*/) const {
    return &m_point;
}

const wxRect* TVisObjRef::GetRect(int /*fieldId*/) const {
    return &m_rect;
}

const std::uint8_t* TVisObjRef::GetId() const {
    return m_id;
}
