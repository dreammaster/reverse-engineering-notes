#include "TGAction.h"

#include <cstdarg>
#include <cwchar>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "THAction.h"
#include "TGCharacter.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vsplayer/animationGame.h"
#include "vsplayer/control/gameControl.h"
#include "vsplayer/control/masterControl.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/actionGame.cpp";

// The scale (percent) an animation of a part is started with.
static const float kAnimationScale = 100.0f;

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

std::vector<TGAction *> TGAction::s_runningActions;
std::unordered_map<int, int> TGAction::s_actionIdToRunningAction;
bool TGAction::s_deletedActions = false;

// The actions' trace messages: bit 1 of g_traceFlags and the log level above 1.
static bool traceActions() {
	return (g_traceFlags & 2) && wxLog::loglevel > 1;
}

static wxString format(const wchar_t *fmt, ...) {
	wchar_t buffer[1024];
	va_list args;

	va_start(args, fmt);
	vswprintf(buffer, 1024, fmt, args);
	va_end(args);
	return wxString(buffer);
}

static int dataId(const TVisObjRef &ref) {
	return PackVisId(ref.GetId());
}

// Confirmed (asm lines 194941-194987)
TGAction::TGAction() : _paused(false), _stopped(false), _finished(false), _executing(false) {
}

// Confirmed (asm lines 194999-195073): the record is named like the action it runs.
TGAction::TGAction(const TVisObjRef &active, const TVisObjRef &data)
	: _active(active), _paused(false), _stopped(false), _finished(false), _executing(false), _data(data) {
	_active.SetName(_data.GetName());
}

// Confirmed (asm lines 194786-194840): the record is removed with the action.
TGAction::~TGAction() {
	_active.Remove();
}

// Confirmed (asm lines 195085-195108): the assignment is not valid.
TGAction &TGAction::operator=(TVisObjRef &ref) {
	x_assert(false, "false", kSourceFile, 0x4C);
	_active = TSAction(ref);
	return *this;
}

// Confirmed (asm lines 195120-195159): the same action is the one with the same part index and record.
bool TGAction::operator==(const TGAction &other) const {
	if (_active.GetInt(kActionActionPartIndex) != other._active.GetInt(kActionActionPartIndex))
		return false;

	return _active == other._active;
}

bool TGAction::operator==(const TVisObjRef &ref) const {
	return _active == ref;
}

TVisObjRef TGAction::GetDataObject() const {
	return _data;
}

void TGAction::SetPaused(bool paused) {
	_paused = paused;
}

// Confirmed (asm lines 195225-195375): an action that was stopped for a menu goes on, and the time
// it was stopped is taken from its timer (a timer started before it never runs backwards).
void TGAction::ContinueStoppedAction() {
	if (!_stopped || _finished)
		return;

	_stopped = false;

	if (traceActions()) {
		wxLog::logexpanded(L"\"%s\" (active id: %d, data id: %d)", _data.GetName().c_str().wc_str(), dataId(_active), dataId(_data));
	}

	if (_active.GetBool(kActionTimerStarted)) {
		_timer.AdjustTimer(TGameControl::GetStopTime().GetTime());

		if (_timer.GetTime() < 0)
			_timer.SetTime();
	}
}

// Confirmed (asm lines 195399-195565): an animation of an object is started for the object, an
// animation of a character is the character's own animation (4).
TGAnimation *TGAction::ShowAnimation(TVisObjRef &animation, bool reverse) {
	TVisObjRef owner = animation.GetParent();

	if (owner.GetId()[3] == 0x11) {
		// the animation of a character's outfit
		TGCharacter *character = gameControl()->GetCharacter(owner.GetParent().GetParent());

		if (character->IsCharacterAnimRunning())
			return nullptr;

		TGAnimation *started = character->StartCharacterAnim(animation, TCharacterAnimEnum::kCharacterAnim, reverse);

		if (started)
			started->SetStartedByUser(true);

		return started;
	}

	TVisObjRef parent = animation.GetParent();

	if (TGAnimation::AnimationIsRunning(animation))
		return nullptr;

	TManagedObject *object = gameControl()->GetObject(parent);
	TGAnimation *started = TGAnimation::StartAnimation(animation, object, reverse, kAnimationScale, -1);

	if (!started)
		return nullptr;

	started->SetStartedByUser(true);

	if (object)
		object->SetAnimation(started);

	return started;
}

// Confirmed (asm lines 195570-195722)
void TGAction::HideAnimation(const TVisObjRef &animation) {
	if (animation.IsEmpty())
		return;

	if (animation.GetParent().GetId()[3] != 0x11) {
		TGAnimation::HideAnimation(animation);
		return;
	}

	TVisObjRef character = animation.GetParent().GetParent();

	if (character.IsEmpty())
		return;

	TGCharacter *tgCharacter = gameControl()->GetCharacter(character);

	if (tgCharacter->IsCharacterAnimRunning() && tgCharacter->GetCharacterAnim() == animation)
		tgCharacter->StopCharacterAnim(TCharacterAnimEnum::kNone, true);
}

// Confirmed (asm lines 195728-195770): an animation of an object (table 9) is running when
// the animation system says so, one of a character (0) when the character's animation is.
bool TGAction::WaitOnAnimation(TVisObjRef &animation) {
	if (animation.IsEmpty())
		return false;

	if (animation.GetId()[3] == 9)
		return TGAnimation::AnimationIsRunning(animation);

	if (animation.GetId()[3] != 0)
		return false;

	return gameControl()->GetCharacter(animation)->IsCharacterAnimRunning();
}

std::vector<TGAction *> &TGAction::GetRunningActions() {
	return s_runningActions;
}

// Confirmed (asm lines 195778-196100)
void TGAction::DeleteRunningAction(const TVisObjRef &id, bool byData) {
	for (TGAction *action : s_runningActions) {
		if (action->_finished)
			continue;

		bool found = byData ? (dataId(action->_data) == dataId(id)) : (dataId(action->_active) == dataId(id));

		if (!found)
			continue;

		action->_stopped = true;
		action->_finished = true;

		if (traceActions()) {
			wxLog::logexpanded(L"Action stopped: \"%s\" (active id: %d, data id: %d)", action->_data.GetName().c_str().wc_str(),
			                   dataId(action->_active), dataId(action->_data));
		}

		return;
	}

	if (traceActions()) {
		wxLog::logexpanded(L"Action was not stopped because it is not running: \"%s\" (data id: %d)",
		                   id.GetName().c_str().wc_str(), dataId(id));
	}
}

// Confirmed (asm lines 196116-196360): the table of the actions is made again when one was deleted.
void TGAction::DeleteFinishedActions() {
	x_assert(s_runningActions.size() == s_actionIdToRunningAction.size(),
	         "RunningActions.size() == ActionIdToRunningAction.size()", kSourceFile, 0x960);

	s_deletedActions = true;

	bool deleted = false;

	for (size_t i = 0; i < s_runningActions.size();) {
		TGAction *action = s_runningActions[i];

		if (action->_finished && !action->_executing) {
			s_runningActions.erase(s_runningActions.begin() + i);
			delete action;
			deleted = true;
		} else {
			i++;
		}
	}

	if (!deleted && s_actionIdToRunningAction.size() == s_runningActions.size())
		return;

	s_actionIdToRunningAction.clear();

	for (size_t i = 0; i < s_runningActions.size(); i++)
		s_actionIdToRunningAction[dataId(s_runningActions[i]->_data)] = (int)i;

	x_assert(s_runningActions.size() == s_actionIdToRunningAction.size(),
	         "RunningActions.size() == ActionIdToRunningAction.size()", kSourceFile, 0x976);
}

// Confirmed (asm lines 196366-196431)
void TGAction::ClearActions() {
	for (TGAction *action : s_runningActions)
		delete action;

	s_actionIdToRunningAction.clear();
	s_runningActions.clear();
}

// Confirmed (asm lines 196443-196460)
void TGAction::FinishAllActions() {
	for (TGAction *action : s_runningActions)
		action->_finished = true;
}

// Confirmed (asm lines 196472-196720)
void TGAction::StopRunningActions() {
	if (traceActions())
		wxLog::logexpanded(L"The following actions are stopped because changing from scene to a menu:");

	for (TGAction *action : s_runningActions) {
		if (action->_finished)
			continue;

		action->_stopped = true;

		if (traceActions()) {
			wxLog::logexpanded(L"\"%s\" (active id: %d, data id: %d)", action->_data.GetName().c_str().wc_str(),
			                   dataId(action->_active), dataId(action->_data));
		}
	}

	if (traceActions())
		wxLog::logexpanded(L"---end of stopped actions---");
}

// Confirmed (asm lines 196731-196855)
void TGAction::ContinueStoppedActions() {
	if (traceActions())
		wxLog::logexpanded(L"The following actions are continued because changing from menu to a scene:");

	for (size_t i = 0; i < s_runningActions.size(); i++)
		s_runningActions[i]->ContinueStoppedAction();

	if (traceActions())
		wxLog::logexpanded(L"---end of continued actions---");
}

// Confirmed (asm lines 196863-196913)
TVisObjRef TGAction::GetActionByName(const wxString &name) {
	for (TGAction *action : s_runningActions) {
		if (action->_active.GetName() == name)
			return action->_active;
	}

	return TVisObjRef();
}

// Confirmed (asm lines 196923-196980)
TVisObjRef TGAction::GetActionById(int id) {
	for (TGAction *action : s_runningActions) {
		if (dataId(action->_active) == id)
			return action->_active;
	}

	return TVisObjRef();
}

// Confirmed (asm lines 196990-197017)
void TGAction::GetActions(TVList &outActions) {
	outActions.clear();

	for (TGAction *action : s_runningActions)
		outActions.push_back(action->_active);
}

// Confirmed (asm lines 197029-197760): what the console shows of the running actions.
void TGAction::PrintRunningActions(std::list<wxString> &outLines) {
	outLines.push_back(wxString(L"Running Actions:"));

	if (s_runningActions.empty()) {
		outLines.push_back(wxString(L"No actions running"));
		return;
	}

	for (TGAction *action : s_runningActions) {
		TVList parts;

		action->_data.GetList(kActionActionParts, parts);

		int index = action->_active.GetInt(kActionActionPartIndex);

		outLines.push_back(format(L"Action \"%s\" (active id: %d, data id: %d)", action->_data.GetName().c_str().wc_str(),
		                          dataId(action->_active), dataId(action->_data)));

		if (index >= 0 && (size_t)index < parts.size()) {
			TVisObjRef part(parts.at(index));

			outLines.push_back(format(L"Current action part: \"%s\" (id: %d)", part.GetName().c_str().wc_str(),
			                          dataId(part)));
		}

		outLines.push_back(wxString(L""));
	}
}

// Confirmed (asm lines 197770-197781): the actions of a mouse message are the event numbers of
// the table (message 2 ... 13), none for the others.
TMouseEventEnum TGAction::ConvertToEvent(TMouseMessageEnum msg) {
	static const TMouseEventEnum kEvents[12] = {
		TMouseEventEnum::kValue3, TMouseEventEnum::kValue1, TMouseEventEnum::kValue1, TMouseEventEnum::kValue4,
		TMouseEventEnum::kValue0, TMouseEventEnum::kValue0, TMouseEventEnum::kValue2, TMouseEventEnum::kValue2,
		TMouseEventEnum::kValue7, TMouseEventEnum::kValue7, TMouseEventEnum::kValue8, TMouseEventEnum::kValue9
	};
	unsigned int index = (unsigned int)msg - 2;

	return (index > 0xB) ? TMouseEventEnum::kValue0 : kEvents[index];
}

// Confirmed (asm lines 197793-197823): the paused time that was saved is taken off the timer.
void TGAction::Load(const TVisObjRef &active) {
	_stopped = false;
	_timer.SetTime();
	_timer.AdjustTimer(-(long)active.GetInt(kActionPauseTime));
	_finished = false;
}

// Confirmed (asm lines 197835-197920): a finished action is kept in the saved game only when it is
// not empty; the time it has been running is what is saved of an action that has its timer going.
void TGAction::Save() {
	if (_finished && !_active.IsEmpty()) {
		_active.SetTemporary(true);
		return;
	}

	if (_stopped && _active.GetBool(kActionTimerStarted)) {
		TTimer elapsed = _timer;

		elapsed.AdjustTimer(TGameControl::GetStopTime().GetTime());
		_active.SetValue(kActionPauseTime, (int)elapsed.GetTime(), TSendEventEnum::kSendEvent);
	}

	_active.SetTemporary(false);
}

// Confirmed (asm lines 197928-197946)
void TGAction::SaveActions() {
	for (TGAction *action : s_runningActions)
		action->Save();
}

// Confirmed (asm lines 197958-198480): an action is started once; its record is made new.
TGAction *TGAction::AddRunningAction(const TVisObjRef &action) {
	if (action.IsEmpty()) {
		if ((g_traceFlags & 2) && wxLog::loglevel > 0) {
			wxLog::logexpanded(L"Action not added because it is empty: \"%s\" (data id: %d)", action.GetName().c_str().wc_str(),
			                   dataId(action));
		}

		return nullptr;
	}

	auto found = s_actionIdToRunningAction.find(dataId(action));

	if (found != s_actionIdToRunningAction.end()) {
		if (traceActions()) {
			wxLog::logexpanded(L"Action not added because it is already running: \"%s\" (data id: %d)",
			                   action.GetName().c_str().wc_str(), dataId(action));
		}

		return s_runningActions[found->second];
	}

	TVisObjRef active = gameControl()->GetVisionaire()->CreateActiveObject(0x19, action);

	if (traceActions()) {
		wxLog::logexpanded(L"Action added: \"%s\" (active id: %d, data id: %d)", action.GetName().c_str().wc_str(), dataId(active),
		                   dataId(action));
	}

	TGAction *added = new THAction(active, action);

	registerAction(added);
	return added;
}

void TGAction::registerAction(TGAction *action) {
	s_runningActions.push_back(action);
	s_actionIdToRunningAction[dataId(action->_data)] = (int)s_runningActions.size() - 1;
}

// Confirmed (asm lines 211541-212169): the records of the saved game are made into actions
// again; the ones that were running before are ended.
void TGAction::LoadActions() {
	for (TGAction *action : s_runningActions)
		action->_finished = true;

	TVList records;

	gameControl()->GetVisionaire()->GetList(0x19, records, false);

	for (TVisionaireObject *record : records) {
		TVisObjRef data(record->GetLink(kActionSavedObject));

		if (data.IsEmpty()) {
			if (traceActions()) {
				wxLog::logexpanded(L"Failed to load action: \"%s\" (active id: %d). Could not find original action.",
				                   record->GetName().c_str(), PackVisId(record->GetId()));
			}

			continue;
		}

		TGAction *action = new THAction(TVisObjRef(*record), data);

		registerAction(action);
		action->Load(TVisObjRef(*record));

		if (traceActions()) {
			wxLog::logexpanded(L"Action loaded: \"%s\" (active id: %d, data id: %d)", action->_data.GetName().c_str().wc_str(),
			                   dataId(action->_active), dataId(action->_data));
		}
	}
}
