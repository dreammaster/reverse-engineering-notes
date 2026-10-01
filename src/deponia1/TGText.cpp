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

wxString TGText::GetEventHandlerTextStarted() {
	return wxString();
}

wxString TGText::GetEventHandlerTextStopped() {
	return wxString();
}
