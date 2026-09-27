#include "TGCharacter.h"

wxPoint TGCharacter::GetScreenPosition() const {
    return wxPoint{-1, -1};
}

wxRect TGCharacter::GetVisibleRect() const {
    return wxRect();
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

std::list<TGInterface*> TGCharacter::GetInterfaces() const {
    return {};
}
