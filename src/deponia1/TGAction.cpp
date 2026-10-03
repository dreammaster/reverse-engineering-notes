#include "TGAction.h"

#include "vsplayer/control/masterControl.h"

TGAction *TGAction::AddRunningAction(const TVisObjRef &/*action*/) {
	return nullptr;
}

void TGAction::Execute(bool /*flag*/, t_SkipCutsceneInfo */*skipInfo*/) {
}

void TGAction::StopRunningActions() {
}

void TGAction::ContinueStoppedActions() {
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
