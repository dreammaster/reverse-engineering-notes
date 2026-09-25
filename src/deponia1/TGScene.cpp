#include "TGScene.h"

bool TGScene::IsMenu() const {
    return false;
}

void* TGScene::GetObject(const TVisObjRef& /*object*/) const {
    return nullptr;
}
