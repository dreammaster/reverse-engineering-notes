#include "TGCharacter.h"

wxPoint TGCharacter::GetScreenPosition() const {
	return wxPoint{-1, -1};
}

wxRect TGCharacter::GetVisibleRect() const {
	return wxRect();
}

void TGCharacter::Save() {
}

void TGCharacter::SetInterfaces() {
}

void TGCharacter::WalkWay() {
}

void TGCharacter::UpdateCharacter() {
}

void TGCharacter::SetOnDestination() {
}

void TGCharacter::CheckRandomTimer() {
}

void TGCharacter::SetRandomTime() {
}

std::list<TGInterface *> TGCharacter::GetInterfaces() const {
	return {};
}

void TGCharacter::Init() {
}

void TGCharacter::AssignToScene(const TVisObjRef &/*scene*/, const wxPoint &/*pos*/, int /*walkSpeed*/) {
}
