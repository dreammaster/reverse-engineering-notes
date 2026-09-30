#include "TGScene.h"

#include "TManagedObject.h"
#include "TMSavegame.h"

bool TGScene::IsMenu() const {
	return false;
}

TManagedObject *TGScene::GetObject(const TVisObjRef &/*object*/) const {
	return nullptr;
}

TManagedObject *TGScene::GetObject(const wxPoint &/*pos*/) const {
	return nullptr;
}

TMSavegame *TGScene::GetSelectedSavegame(bool /*flag*/) {
	return nullptr;
}

TMSavegame *TGScene::GetSavegameAt(const wxPoint &/*pos*/) const {
	return nullptr;
}

void TGScene::DeleteSelectedSavegame() {
}

void TGScene::SelectSavegame(const wxPoint &/*pos*/) {
}

std::vector<TGCharacter *> TGScene::GetCharacters() const {
	return {};
}

void TGScene::InitActionAreas() {
}

void TGScene::SortAllObjects() {
}

void TGScene::UpdateSnoopAnimAlpha() {
}
