#include "TGScene.h"

#include "TManagedObject.h"

bool TGScene::IsMenu() const {
    return false;
}

TManagedObject* TGScene::GetObject(const TVisObjRef& /*object*/) const {
    return nullptr;
}
