#include "TGScene.h"

#include "TManagedObject.h"
#include "TMSavegame.h"

bool TGScene::IsMenu() const {
    return false;
}

TManagedObject* TGScene::GetObject(const TVisObjRef& /*object*/) const {
    return nullptr;
}

TMSavegame* TGScene::GetSelectedSavegame(bool /*flag*/) {
    return nullptr;
}

TMSavegame* TGScene::GetSavegameAt(const wxPoint& /*pos*/) const {
    return nullptr;
}

void TGScene::DeleteSelectedSavegame() {
}

std::vector<TGCharacter*> TGScene::GetCharacters() const {
    return {};
}
