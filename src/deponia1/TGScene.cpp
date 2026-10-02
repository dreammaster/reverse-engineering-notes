#include "TGScene.h"

#include "TMSavegame.h"
#include "TMSavegameArea.h"

bool TGScene::IsMenu() const {
	return false;
}

void TGScene::SetScene() {
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
