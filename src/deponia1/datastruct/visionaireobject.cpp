#include "datastruct/visionaireobject.h"

int TVisionaireObject::GetInt(int /*fieldId*/) const {
	return 0;
}

TCharHolder TVisionaireObject::GetName() const {
	return TCharHolder();
}

TVisionaireObject *TVisionaireObject::GetLink(int /*fieldId*/) const {
	return nullptr;
}

const std::uint8_t *TVisionaireObject::GetId() const {
	return _id;
}

wxString TVisionaireObject::GetStr(int /*fieldId*/) const {
	return wxString();
}
