#include "TSceneControl.h"

TGScene* TSceneControl::GetScene() const {
    return const_cast<TGScene*>(&m_scene);
}

void TSceneControl::Draw() {
}

void TSceneControl::ShowScene(TVisObjRef& /*scene*/, bool /*immediate*/, bool /*flag*/) {
}

void TSceneControl::Set(const TVisObjRef& /*scene*/) {
}

void TSceneControl::GetLastPlayableSceneParams(TVisObjRef& /*outScene*/, wxPoint& /*outPos*/) const {
}
