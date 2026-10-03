#include "TGText.h"

TGCharacter *TGText::GetSpeaker() const {
	return nullptr;
}

void TGText::SetOwner(TManagedObject */*owner*/) {
}

void TGText::Draw(float /*scale*/) {
}

void TGText::Save() {
}

void TGText::Load() {
}

void TGText::StopRunningTexts() {
}

void TGText::ContinueStoppedTexts() {
}

void TGText::RestartTalkAnimations(const TVisObjRef &/*scene*/) {
}

wxString TGText::GetEventHandlerTextStarted() {
	return wxString();
}

wxString TGText::GetEventHandlerTextStopped() {
	return wxString();
}
