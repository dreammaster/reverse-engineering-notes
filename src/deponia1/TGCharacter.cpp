#include "TGCharacter.h"

wxPoint TGCharacter::GetScreenPosition() const {
	return wxPoint{-1, -1};
}

wxRect TGCharacter::GetVisibleRect() const {
	return wxRect();
}

void TGCharacter::Save() {
}

void TGCharacter::Load() {
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

bool TGCharacter::IsWalking() const {
	return false;
}

bool TGCharacter::IsWalkingSoundPlaying() const {
	return false;
}

wxFileName TGCharacter::GetWalkingSound() const {
	return wxFileName();
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

void TGCharacter::ShowComment(const TVisObjRef &/*comment*/) {
}

void TGCharacter::StopWalking(bool /*flag*/) {
}

void TGCharacter::CheckWalkingSound() {
}

void TGCharacter::InitWaySystem(int spriteHeight) {
	_waySystem.SetNoLines(spriteHeight);

	// the character's size: scaled by where it stands, if it is to be, and by its own
	// scale factor (percent)
	float size = 100.0f;

	if (_objRef.GetBool(kCharacterScale)) {
		size = _waySystem.GetCalculatedSize(_position);

		int factor = _objRef.GetInt(kCharacterScaleFactor);

		if (factor != 100)
			size = size * (float)factor / 100.0f;
	}
	_objRef.SetValue(kCharacterSize, size, TSendEventEnum::kNoEvent);
}

void TGCharacter::CheckCharacterPosition() {
	_position = _waySystem.CheckPosition(_position);
}

void TGCharacter::StopWalkingSound() {
}

void TGCharacter::PreloadAnimations() {
}

void TGCharacter::StartFittingAnimation() {
}

void TGCharacter::UnloadAnimations() {
}

void TGCharacter::StartStandingAnim() {
}

void TGCharacter::AdjustTimers() {
}
