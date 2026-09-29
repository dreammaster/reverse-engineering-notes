#include "TSceneControl.h"

TGScene *TSceneControl::GetScene() const {
	return const_cast<TGScene *>(&_scene);
}

void TSceneControl::Draw() {
}

void TSceneControl::ShowScene(TVisObjRef &/*scene*/, bool /*immediate*/, bool /*flag*/) {
}

void TSceneControl::Set(const TVisObjRef &/*scene*/) {
}

void TSceneControl::GetLastPlayableSceneParams(TVisObjRef &/*outScene*/, wxPoint &/*outPos*/) const {
}

bool TSceneControl::FadingToNewScene() const {
	return false;
}
