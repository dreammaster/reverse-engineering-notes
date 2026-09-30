#include "TGObjectManager.h"

wxString TGObjectManager::GetActionText() const {
	return wxString();
}

void TGObjectManager::ResetCurrentObject() {
}

void TGObjectManager::ResetEventInfo() {
}

void TGObjectManager::RemoveItem(bool /*flag*/) {
}

void TGObjectManager::MouseMove(TManagedObject */*object*/) {
}

void TGObjectManager::HandleEvent(TMouseEventEnum /*event*/) {
}

bool TGObjectManager::IsCurrentObjectEmpty() const {
	return true;
}

bool TGObjectManager::IsCurrentObjectWalkable() const {
	return false;
}

void TGObjectManager::SavedObjectChanged() {
}

bool TGObjectManager::IsCurrentObjectDetectable() const {
	return false;
}
