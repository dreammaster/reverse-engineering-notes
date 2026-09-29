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

void TGInterface::RemoveSpritesAndAnimations() {
}

void TGInterface::SetObjectsActive(bool /*active*/) {
}

void TGInterface::UpdateItems(const TVList &/*items*/) {
}
