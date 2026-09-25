#include "TSceneControl.h"

TGScene* TSceneControl::GetScene() const {
    return const_cast<TGScene*>(&m_scene);
}

void TSceneControl::Draw() {
}
