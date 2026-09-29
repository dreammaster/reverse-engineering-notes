#include "TGInterface.h"

#include "TManagedObject.h"

TManagedObject *TGInterface::GetObject(const TVisObjRef &/*object*/) const {
	return nullptr;
}

void TGInterface::RemoveSpritesAndAnimations() {
}

void TGInterface::SetObjectsActive(bool /*active*/) {
}

void TGInterface::UpdateItems(const TVList &/*items*/) {
}
