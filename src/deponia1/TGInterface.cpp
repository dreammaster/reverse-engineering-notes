#include "TGInterface.h"

#include "TManagedObject.h"

TManagedObject *TGInterface::GetObject(const TVisObjRef &/*object*/) const {
	return nullptr;
}

TManagedObject *TGInterface::GetObject(const wxPoint &/*pos*/) const {
	return nullptr;
}

bool TGInterface::IsInside(const wxPoint &/*pos*/) const {
	return false;
}

TGPlaceHolder *TGInterface::GetPlaceHolder(const TManagedObject */*object*/) const {
	return nullptr;
}

void TGInterface::RemoveSpritesAndAnimations() {
}

void TGInterface::SetObjectsActive(bool /*active*/) {
}

void TGInterface::UpdateItems(const TVList &/*items*/) {
}

void TGInterface::RemoveAllItems() {
}

void TGInterface::Load() {
}
