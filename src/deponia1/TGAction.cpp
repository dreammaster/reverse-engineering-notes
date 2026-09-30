#include "TGAction.h"

#include "vsplayer/control/masterControl.h"

void TGAction::AddRunningAction(const TVisObjRef &/*action*/) {
}

void TGAction::ContinueRunningActions(bool /*flag*/) {
}

void TGAction::ClearActions() {
}

void TGAction::SaveActions() {
}

void TGAction::SkipCutscene() {
}

TMouseEventEnum TGAction::ConvertToEvent(TMouseMessageEnum /*msg*/) {
	return TMouseEventEnum{};
}

void TGAction::LoadActions() {
}

void TGAction::DeleteFinishedActions() {
}
