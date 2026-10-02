#include "TGAnimation.h"

void TGAnimation::ClearAnimations() {
}

void TGAnimation::SaveAnimations() {
}

wxString TGAnimation::GetEventHandlerAnimStarted() {
	return wxString();
}

wxString TGAnimation::GetEventHandlerAnimStopped() {
	return wxString();
}

void TGAnimation::LoadAnimations() {
}

void TGAnimation::ContinueAnimations() {
}

void TGAnimation::HideAnimation(TGAnimation */*animation*/, TManagedObject */*owner*/) {
}

void TGAnimation::HideAnimation(TGAnimation */*animation*/, TAnimationOwner */*owner*/) {
}

TGAnimation *TGAnimation::StartAnimation(const TVisObjRef &/*dataObject*/, TAnimationOwner */*owner*/,
                                          bool /*flag*/, float /*scale*/, int /*frame*/) {
	return nullptr;
}

void TGAnimation::HideAnimation(const TVisObjRef &/*dataObject*/) {
}

void TGAnimation::UnloadAnimation(const TVisObjRef &/*dataObject*/) {
}

void TGAnimation::PreloadAnimation(const TVisObjRef &/*dataObject*/) {
}
