#include "datastruct/visobjref.h"

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

const wxPoint* TVisObjRef::GetPoint(int /*fieldId*/) const {
    return &m_point;
}

const wxRect* TVisObjRef::GetRect(int /*fieldId*/) const {
    return &m_rect;
}

const std::uint8_t* TVisObjRef::GetId() const {
    return m_id;
}
