#include "TGActionExecutor.h"

#include <string>

#include "AppGlobals.h"
#include "TGCharacter.h"
#include "TGInterface.h"
#include "TGScene.h"
#include "TSceneControl.h"
#include "TSoundFFMPEG.h"
#include "TTText.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vscommon/fontManager.h"
#include "vsplayer/animationGame.h"
#include "vsplayer/control/gameControl.h"
#include "vsplayer/control/masterControl.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

static int dataId(const TVisObjRef &ref) {
	return PackVisId(ref.GetId());
}

TGActionPartExecutor::TGActionPartExecutor(TGAction &action, const TVisObjRef &part, const TVisObjRef &game,
        const TVList &parts, bool skip, t_SkipCutsceneInfo *skipInfo,
        bool &dialogEnded)
	: _wait(false), _stay(false), _quit(false), _action(action), _part(part), _game(game), _parts(parts),
	  _skip(skip), _skipInfo(skipInfo), _dialogEnded(dialogEnded) {
}

TVisObjRef TGActionPartExecutor::link() const {
	return _part.GetLink(kActionPartLink);
}

TVisObjRef TGActionPartExecutor::altLink() const {
	return _part.GetLink(kActionPartAltLink);
}

TVisObjRef TGActionPartExecutor::currentCharacter() const {
	return gameControl()->GetCurrentCharacter()->GetRef();
}

TVisObjRef TGActionPartExecutor::orCurrentCharacter(const TVisObjRef &ref) const {
	return ref.IsEmpty() ? currentCharacter() : ref;
}

bool TGActionPartExecutor::started() const {
	return _action._active.GetBool(kActionActionPartStarted);
}

void TGActionPartExecutor::setStarted(bool started) {
	_action._active.SetValue(kActionActionPartStarted, started, TSendEventEnum::kSendEvent);
}

// The message of a part that is not done: the action's name and id are in it.
void TGActionPartExecutor::logPartNotDone(const wchar_t *format) {
	TVisObjRef data = _action.GetDataObject();

	wxLog::logexpanded(format, data.GetName().c_str().wc_str(), dataId(data));
}

void TGActionPartExecutor::ifFalse() {
	_action._active.SetValue(kActionContinueWaitForEndIfElse,
	                         _action._active.GetInt(kActionContinueWaitForEndIfElse) + 1, TSendEventEnum::kSendEvent);
}

// What each case of the switch does (Deponia_Linux.asm line 199444 on: the jump table is
// jpt_562CD8, the command minus 5 is the index; the commands that are not in it, and the
// ones whose entry is the table's default, do nothing).
void TGActionPartExecutor::run(int command) {
	switch (command) {
	case kCommandElse:
		cmdElse();
		break;
	case kCommandChangeScene:
		cmdChangeScene();
		break;
	case kCommandChangeCharacter:
		cmdChangeCharacter();
		break;
	case kCommandStartDialog:
		cmdStartDialog();
		break;
	case kCommandEndDialog:
		cmdEndDialog();
		break;
	case kCommandEndAction:
		cmdEndAction();
		break;
	case kCommandShowAnimation:
		cmdShowAnimation();
		break;
	case kCommandWaitAnimation:
		cmdWaitAnimation();
		break;
	case kCommandShowText:
		cmdShowText();
		break;
	case kCommandPlaySound:
		cmdPlaySound(false);
		break;
	case kCommandWait:
		cmdWait();
		break;
	case kCommandCharacterGoToObject:
		cmdCharacterGoToObject();
		break;
	case kCommandWaitCharacter:
		cmdWaitCharacter();
		break;
	case kCommandSetCommand:
		cmdSetCommand();
		break;
	case kCommandShowScene:
		cmdShowScene();
		break;
	case kCommandCharacterGoTo:
	case kCommandCharacterGoTo2:
		cmdCharacterGoTo();
		break;
	case kCommandScrollToObject:
		cmdScrollToObject();
		break;
	case kCommandScrollToPoint:
		cmdScrollToPoint();
		break;
	case kCommandPlaceCharacter:
		cmdPlaceCharacter();
		break;
	case kCommandChangeOutfit:
		cmdChangeOutfit();
		break;
	case kCommandSetCommentSet:
		cmdSetCommentSet();
		break;
	case kCommandSetLanguage:
		cmdSetLanguage();
		break;
	case kCommandShowCharacter:
		cmdShowCharacter();
		break;
	case kCommandQuit:
		gameControl()->QuitGame();
		_quit = true;
		break;
	case kCommandShowHideInterface:
		cmdShowHideInterface();
		break;
	case kCommandTurnCharacter:
		cmdTurnCharacter();
		break;
	case kCommandSceneMusic:
		cmdSceneMusic();
		break;
	case kCommandScrollToCharacter:
		cmdScrollToCharacter();
		break;
	case kCommandPlayVideo:
		cmdPlayVideo();
		break;
	case kCommandIfValue:
		cmdIfValue();
		break;
	case kCommandSetValue:
		cmdSetValue();
		break;
	case kCommandShowTextAt:
		cmdShowTextAt();
		break;
	case kCommandLoopSound:
		cmdPlaySound(true);
		break;
	case kCommandStopSound:
		cmdStopSound();
		break;
	case kCommandIfCharacterDirection:
		cmdIfCharacterDirection();
		break;
	default:
		break;
	}
}

// Confirmed (asm lines 202218-202227): an else that is come to from a branch that was done: what
// follows is skipped up to the end of the if.
void TGActionPartExecutor::cmdElse() {
	ifFalse();
}

// Confirmed (asm lines 202202-202217, 205966-206034, 207131-207159, 205795-205853)
void TGActionPartExecutor::cmdChangeScene() {
	if (gameControl()->GetSceneControl()->FadingToNewScene()) {
		if (wxLog::loglevel >= 0) {
			logPartNotDone(L"'Change scene' action part of action '%s' (data id: %d) was not executed because scene is currently faded.");
		}

		return;
	}

	TVisObjRef target = link();
	int direction = _part.GetInt(kActionPartAltInt);

	if (_skip) {
		// the scene the cutscene ends in
		_skipInfo->scene = target.GetParent();
		gameControl()->GetScene()->SetCharacter(altLink(), target, direction);
	} else {
		gameControl()->GetSceneControl()->ChangeScene(altLink(), target, _part.GetInt(kActionPartInt) != 0, direction);
	}

	_action._stopped = false;
}

// Confirmed (asm lines 200778-200822, 205362-205425, 206809-206820, 207408-207436, 209356-209372)
void TGActionPartExecutor::cmdChangeCharacter() {
	if (gameControl()->GetSceneControl()->FadingToNewScene()) {
		if (wxLog::loglevel > 0) {
			logPartNotDone(L"'Change character' action part of action '%s' (data id: %d) was not executed because scene is currently faded.");
		}

		return;
	}

	if (_skip) {
		// the character the cutscene ends with, in the scene that character is in
		_skipInfo->character = link();
		_skipInfo->scene = gameControl()->GetCharacter(_skipInfo->character)->GetRef().GetLink(kCharacterScene);
		return;
	}

	// (a part that comes while a text is shown is not done)
	if (!gameControl()->IsNoTextDisplayed())
		return;

	gameControl()->ChangeCharacter(link(), _part.GetInt(kActionPartInt) != 0, TVisObjRef());
}

// Confirmed (asm lines 200754-200775)
void TGActionPartExecutor::cmdStartDialog() {
	if (_dialogEnded)
		return;

	gameControl()->StartDialog(link());
}

// Confirmed (asm lines 201587-201594)
void TGActionPartExecutor::cmdEndDialog() {
	gameControl()->EndDialog();
	_dialogEnded = true;
}

// Confirmed (asm lines 201573-201586, 205033-205097): the action is over; the loop is left.
void TGActionPartExecutor::cmdEndAction() {
	_action._finished = true;

	if ((g_traceFlags & 2) && wxLog::loglevel > 1) {
		wxLog::logexpanded(L"Current action stopped: \"%s\" (active id: %d, data id: %d)",
		                   _action._active.GetName().c_str().wc_str(), dataId(_action._active), dataId(_action._data));
	}

	_wait = true;
}

// Confirmed (asm lines 200092-200136, 206700-206727, 207577-207581, 209167-209229, 210343-210373):
// shows an animation (and waits for it to end, when the part says so), or hides it.
void TGActionPartExecutor::cmdShowAnimation() {
	TVisObjRef animation = link();

	if (_part.GetInt(kActionPartInt) != 0) {
		// the flag: hide it (an animation of an object is table 9, one of a character table 0)
		int table = animation.GetId()[3];

		if (table == 9) {
			TGAction::HideAnimation(animation);
		} else if (table == 0 && !animation.IsEmpty()) {
			TGCharacter *character = gameControl()->GetCharacter(animation);

			if (character->IsCharacterAnimRunning())
				character->StopCharacterAnim(TCharacterAnimEnum::kNone, true);
		}

		return;
	}

	if (_skip)
		return;

	bool waitForEnd = (_part.GetInt(kActionPartAltInt2) != 0);

	if (!waitForEnd || !started()) {
		TGAction::ShowAnimation(animation, _part.GetInt(kActionPartAltInt) == 1);

		if (!waitForEnd)
			return;

		setStarted(true);
	}

	if (TGAction::WaitOnAnimation(animation))
		_wait = true;
	else
		setStarted(false);
}

// Confirmed (asm lines 200068-200089)
void TGActionPartExecutor::cmdWaitAnimation() {
	if (_skip)
		return;

	TVisObjRef animation = link();

	_wait = TGAction::WaitOnAnimation(animation);
}

// Confirmed (asm lines 203206-203276, 205540-205628, 206887-206895, 207199-207208, 207731-207754):
// a character says the text, the player reads it (the part waits for it to end) - or, with the
// flag, it is shown without waiting.
void TGActionPartExecutor::cmdShowText() {
	TextAlignmentEnum alignment = (TextAlignmentEnum)_game.GetInt(kGameTextAlignment);
	TGameControl *control = gameControl();

	if (_part.GetInt(kActionPartInt) != 0) {
		if (_skip)
			return;

		TTText text(link());
		TGCharacter *character = control->GetCharacterPointer(altLink());

		control->StartBackgroundText(text, character, alignment, TVisObjRef(), wxPoint());
		return;
	}

	if (_skip) {
		if (started()) {
			control->ClearText(link());
			setStarted(false);
		}

		return;
	}

	TTText text(link());

	if (!started()) {
		TGCharacter *character = control->GetCharacterPointer(altLink());

		control->StartText(text, character, alignment, TVisObjRef(), wxPoint());
		setStarted(true);
	}

	if (started() && control->IsTextActive(text))
		_wait = true;
	else
		setStarted(false);
}

// Confirmed (asm lines 206177-206221, 208197-208218, 206065-206110, 210197-210218): plays a sound
// (`loop`: the command that plays it again and again); the sound is kept in the sound manager
// when the part says so.
void TGActionPartExecutor::cmdPlaySound(bool loop) {
	if (_skip)
		return;

	TSoundFFMPEG *sounds = gameControl()->GetSoundManager();

	if (!sounds)
		return;

	wxFileName path = _part.GetPath(kActionPartPath);
	int sound = sounds->Play(path, _part.GetInt(kActionPartInt), _part.GetInt(kActionPartAltInt), loop,
	                         TSoundTypeEnum::kValue1, true, 0);

	if (sound == -1)
		return;

	if (_part.GetInt(kActionPartAltInt2) == 0)
		sounds->Keep(path);
}

// Confirmed (asm lines 202038-202056, 207314-207360, 207388-207399, 207850-207852): waits for a
// time (seconds, minutes or milliseconds by kActionPartAltInt; the time is a variable's value,
// or the part's own number).
void TGActionPartExecutor::cmdWait() {
	if (_skip)
		return;

	TSAction &record = _action._active;

	if (!record.GetBool(kActionTimerStarted)) {
		record.SetValue(kActionTimerStarted, true, TSendEventEnum::kSendEvent);
		_action._timer.SetTime();
		_wait = true;
		return;
	}

	TVisObjRef variable = link();
	long time = variable.IsEmpty() ? _part.GetInt(kActionPartInt) : variable.GetInt(kValueInt);
	int unit = _part.GetInt(kActionPartAltInt);

	if (unit == 1)
		time *= 1000;
	else if (unit == 2)
		time *= 60000;

	if (_action._timer.GetTime() > time)
		record.SetValue(kActionTimerStarted, false, TSendEventEnum::kSendEvent);
	else
		_wait = true;
}

// Confirmed (asm lines 201966-202035, 206968-207020, 207447-207465): the character walks to an
// object (the position of the object and its offset); with the flag the part waits until it is there.
void TGActionPartExecutor::cmdCharacterGoToObject() {
	TGCharacter *character = gameControl()->GetCharacter(link());
	bool waitForEnd = (_part.GetInt(kActionPartInt) != 0);

	if (!waitForEnd || !started()) {
		TVisObjRef object = altLink();
		wxPoint target = *object.GetPoint(kObjectPosition) + *object.GetPoint(kObjectOffset);

		// (skipping: it is there already)
		character->GetRef().SetValue(_skip ? kCharacterPosition : kCharacterDestination, target, TSendEventEnum::kSendEvent);

		if (waitForEnd)
			setStarted(true);

		if (!waitForEnd)
			return;
	}

	if (!_skip && character->IsWalking())
		_wait = true;
	else
		setStarted(false);
}

// Confirmed (asm lines 201941-201963, 206342-206349)
void TGActionPartExecutor::cmdWaitCharacter() {
	TGCharacter *character = gameControl()->GetCharacter(link());

	if (!_skip && character->IsWalking())
		_wait = true;
}

// Confirmed (asm lines 201909-201938, 206585-206685): the active command of the interface is set
// (0), or put back to the standard one (1), or is the next of its interface (2).
void TGActionPartExecutor::cmdSetCommand() {
	switch (_part.GetInt(kActionPartInt)) {
	case 0:
		_game.SetLink(kGameActiveCommand, link(), true);
		break;
	case 1: {
		TVisObjRef interfaceRef = _game.GetLink(kGameActiveCommand).GetParent();

		interfaceRef.SetLink(kInterfaceActiveCommand, interfaceRef.GetLink(kInterfaceStandardCommand), true);
		break;
	}
	case 2:
		TTInterface(_game.GetLink(kGameActiveCommand).GetParent()).SetNextCommand();
		break;
	default:
		break;
	}
}

// Confirmed (asm lines 201893-201906, 205676-205733, 205913-205937, 207211-207289)
void TGActionPartExecutor::cmdShowScene() {
	if (gameControl()->GetSceneControl()->FadingToNewScene()) {
		if (wxLog::loglevel >= 0) {
			logPartNotDone(L"'Show scene' action part of action '%s' (data id: %d) was not executed because scene is currently faded.");
		}

		return;
	}

	TVisObjRef scene = link();

	if (_skip)
		_skipInfo->scene = scene;
	else
		gameControl()->GetSceneControl()->ShowScene(scene, _part.GetInt(kActionPartInt) != 0, false);

	_action._stopped = false;
}

// Confirmed (asm lines 201844-201890, 206563-206582, 207298-207311, 207662-207669): the character
// walks (or, skipping, is put) to a point; with the flag the part waits until it is there.
void TGActionPartExecutor::cmdCharacterGoTo() {
	TGCharacter *character = gameControl()->GetCharacter(link());
	bool waitForEnd = (_part.GetInt(kActionPartAltInt2) != 0);

	if (!waitForEnd || !started()) {
		wxPoint target;

		target.x = _part.GetInt(kActionPartInt);
		target.y = _part.GetInt(kActionPartAltInt);
		character->GetRef().SetValue(_skip ? kCharacterPosition : kCharacterDestination, target, TSendEventEnum::kSendEvent);

		if (!waitForEnd)
			return;

		setStarted(true);
	}

	if (!_skip && character->IsWalking())
		_wait = true;
	else
		setStarted(false);
}

// Confirmed (asm lines 202823-202863): the view scrolls to have an object in its middle.
void TGActionPartExecutor::cmdScrollToObject() {
	TVisObjRef object = link();
	const wxSize &size = gameControl()->GetScene()->GetVisibleSize();
	const wxPoint *position = object.GetPoint(kObjectPosition);
	wxPoint target;

	target.x = position->x - (size.width >> 1);
	target.y = position->y - (size.height >> 1);
	_game.SetValue(kGameScrollToPoint, target, TSendEventEnum::kForce);
}

// Confirmed (asm lines 202802-202820)
void TGActionPartExecutor::cmdScrollToPoint() {
	wxPoint target;

	target.x = _part.GetInt(kActionPartInt);
	target.y = _part.GetInt(kActionPartAltInt);
	_game.SetValue(kGameScrollToPoint, target, TSendEventEnum::kForce);
}

// Confirmed (asm lines 202746-202799, 206113-206120): a character is put in a scene (at an object,
// facing the direction of the part - the object's own direction when it is -1).
void TGActionPartExecutor::cmdPlaceCharacter() {
	TVisObjRef character = orCurrentCharacter(link());
	TVisObjRef target = altLink();
	int direction = _part.GetInt(kActionPartInt);

	if (direction == -1)
		direction = target.GetInt(kObjectDirection);

	gameControl()->GetScene()->SetCharacter(character, target, direction);
}

// Confirmed (asm lines 202695-202743): the outfit of the character (the parent of the outfit link)
// changes; the flag keeps the animations of the old one from being unloaded meanwhile.
void TGActionPartExecutor::cmdChangeOutfit() {
	TVisObjRef outfit = link();
	TVisObjRef owner = outfit.GetParent();
	TGCharacter *character = gameControl()->GetCharacter(owner);

	if (_part.GetInt(kActionPartInt) != 0)
		character->AllowUnloadingOutfitAnimations(false);

	owner.SetLink(kCharacterCurrentOutfit, outfit, true);
	character->AllowUnloadingOutfitAnimations(true);
}

// Confirmed (asm lines 203019-203068)
void TGActionPartExecutor::cmdSetCommentSet() {
	TVisObjRef character = orCurrentCharacter(altLink());
	TVisObjRef commentSet = link();

	if (!commentSet.IsEmpty())
		character.SetLink(kCharacterCurrentCommentSet, commentSet, true);
}

// Confirmed (asm lines 202986-203016)
void TGActionPartExecutor::cmdSetLanguage() {
	_game.SetLink(kGameStandardLanguage, link(), true);
}

// Confirmed (asm lines 203071-203113, 205098-205158, 206224-206267, 207187-207196)
void TGActionPartExecutor::cmdShowCharacter() {
	if (gameControl()->GetSceneControl()->FadingToNewScene()) {
		if (wxLog::loglevel >= 0) {
			logPartNotDone(L"'Show character' action part of action '%s' (data id: %d) was not executed because scene is currently faded.");
		}

		return;
	}

	TVisObjRef scene = gameControl()->GetCharacter(link())->GetRef().GetLink(kCharacterScene);

	if (_skip)
		_skipInfo->scene = scene;
	else
		gameControl()->GetSceneControl()->ShowScene(scene, _part.GetInt(kActionPartInt) != 0, false);
}

// Confirmed (asm lines 203116-203203, 204690-204733, 206429-206527): the part shows, hides or
// toggles (0, 1, 2) the interface of the class in the link, of the current character.
void TGActionPartExecutor::cmdShowHideInterface() {
	TVisObjRef interfaceClass = link();
	TGCharacter *current = gameControl()->GetCurrentCharacter();
	bool found = false;

	for (TGInterface *candidate : current->GetInterfaces()) {
		if (!(candidate->GetRef().GetLink(kInterfaceInterfaceClass) == interfaceClass))
			continue;

		TVisObjRef interfaceRef = candidate->GetRef();

		found = true;

		if (interfaceRef.IsEmpty())
			continue;

		switch (_part.GetInt(kActionPartInt)) {
		case 0:
			interfaceRef.SetValue(kInterfaceVisible, true, TSendEventEnum::kSendEvent);
			break;
		case 1:
			interfaceRef.SetValue(kInterfaceVisible, false, TSendEventEnum::kSendEvent);
			break;
		case 2:
			interfaceRef.SetValue(kInterfaceVisible, !interfaceRef.GetBool(kInterfaceVisible), TSendEventEnum::kSendEvent);
			break;
		default:
			break;
		}
	}

	if (!found && wxLog::loglevel > 2) {
		TVisObjRef data = _action.GetDataObject();
		TVisObjRef characterRef = current->GetRef();
		wxString className = interfaceClass.IsEmpty() ? wxString(L"Empty") : interfaceClass.GetName().c_str();

		wxLog::logexpanded(L"Could not find interface with class '%s' for action part SHOW_HIDE_INTERFACE in action '%s' (data id: %d). Current character is '%s' (id: %d).",
		                   className.wc_str(), data.GetName().c_str().wc_str(), dataId(data),
		                   characterRef.GetName().c_str().wc_str(), dataId(characterRef));
	}
}

// Confirmed (asm lines 202866-202900, 206282-206328, 207122-207128): a character that stands turns
// to an object, or to the direction of the part.
void TGActionPartExecutor::cmdTurnCharacter() {
	TVisObjRef character = orCurrentCharacter(link());

	if (character.GetInt(kCharacterState) != 2)
		return;

	TVisObjRef object = altLink();
	int direction;

	if (object.IsEmpty()) {
		direction = _part.GetInt(kActionPartInt);
	} else {
		wxPoint target = *object.GetPoint(kObjectPosition) + *object.GetPoint(kObjectOffset);
		const wxPoint *position = character.GetPoint(kCharacterPosition);

		direction = GetAngle((float)(target.x - position->x), (float)(target.y - position->y));
	}

	character.SetValue(kCharacterDirection, direction, TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 202914-202983, 205161-205177, 207073-207119, 208437-208443): the music of a
// scene (the current one when there is none in the link) changes; when the cutscene is skipped the
// scene is also told to `t_SkipCutsceneInfo` (the music is changed without a fade).
void TGActionPartExecutor::cmdSceneMusic() {
	TVisObjRef scene = link();

	if (scene.IsEmpty())
		scene = gameControl()->GetScene()->GetRef();

	scene.SetValue(kSceneMusicVolume, _part.GetInt(kActionPartInt), TSendEventEnum::kNoEvent);
	scene.SetValue(kSceneMusicBalance, _part.GetInt(kActionPartAltInt), TSendEventEnum::kNoEvent);

	if (!_skip) {
		scene.SetValue(kSceneBackgroundMusic, _part.GetPath(kActionPartPath), TSendEventEnum::kSendEvent);
		return;
	}

	if (_part.GetPath(kActionPartPath).SameAs(scene.GetPath(kSceneBackgroundMusic)))
		return;

	bool listed = false;

	for (TVisionaireObject *object : _skipInfo->objects) {
		if (scene == *object) {
			listed = true;
			break;
		}
	}

	if (!listed)
		_skipInfo->objects.push_back(scene);

	scene.SetValue(kSceneBackgroundMusic, _part.GetPath(kActionPartPath), TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 201463-201505)
void TGActionPartExecutor::cmdScrollToCharacter() {
	TVisObjRef character = orCurrentCharacter(link());

	_game.SetValue(kGameScrollCenterCharacter, _part.GetInt(kActionPartInt) == 0, TSendEventEnum::kSendEvent);
	_game.SetLink(kGameScrollCharacter, character, true);
}

// Confirmed (asm lines 201454-201460, 206123-206174, 207034-207070): plays a video; what happens
// to the sounds meanwhile is the part's (0: by what comes next - a part that changes the scene or
// the character after it makes it 3, anything else 1). The next part is then the one that
// is looked at, but not before the next frame.
void TGActionPartExecutor::cmdPlayVideo() {
	if (_skip)
		return;

	int handleSounds = _part.GetInt(kActionPartAltInt);

	if (handleSounds == 0) {
		handleSounds = 1;

		int next = _action._active.GetInt(kActionActionPartIndex) + 1;

		if (next >= 0 && (size_t)next < _parts.size()) {
			int command = TVisObjRef(_parts.at(next)).GetInt(kActionPartCommand);

			if (command == kCommandChangeScene || command == kCommandShowScene || command == kCommandChangeCharacter ||
			        command == kCommandShowCharacter)
				handleSounds = 3;
		}
	}

	gameControl()->PlayAVI(_part.GetPath(kActionPartPath), _part.GetInt(kActionPartInt) != 0,
	                       static_cast<HandleSoundsEnum>(handleSounds));
	_action._active.SetValue(kActionActionPartIndex, _action._active.GetInt(kActionActionPartIndex) + 1,
	                         TSendEventEnum::kSendEvent);
	_wait = true;
}

// Confirmed (asm lines 201396-201451, 204307-204372, 205268-205274): IF a variable (the link) and
// a number (a variable of the second link, or the part's own) are in the relation of the part
// (0 equal, 1 not equal, 2 less or equal, 3 less, 4 greater or equal, 5 greater).
void TGActionPartExecutor::cmdIfValue() {
	TTValue variable(link());
	int left = variable.GetInt(kValueInt);
	TTValue other(altLink());
	int right = other.IsEmpty() ? _part.GetInt(kActionPartAltInt) : other.GetInt(kValueInt);
	bool holds;

	switch (_part.GetInt(kActionPartInt)) {
	case 0:
		holds = (left == right);
		break;
	case 1:
		holds = (left != right);
		break;
	case 2:
		holds = (right >= left);
		break;
	case 3:
		holds = (right > left);
		break;
	case 4:
		holds = (right <= left);
		break;
	case 5:
		holds = (right < left);
		break;
	default:
		return;
	}

	if (!holds)
		ifFalse();
}

// Confirmed (asm lines 201262-201393, 206688-206697, 210380-210438): a variable (the link) is set
// (0), added to (1), subtracted from (2), multiplied by (3) or divided by (4) a number (a variable
// of the second link, or the part's own).
void TGActionPartExecutor::cmdSetValue() {
	TTValue variable(link());
	int left = variable.GetInt(kValueInt);
	TTValue other(altLink());
	int right = other.IsEmpty() ? _part.GetInt(kActionPartAltInt) : other.GetInt(kValueInt);

	if (variable.IsEmpty()) {
		if (wxLog::loglevel > 0) {
			TVisObjRef data = _action.GetDataObject();

			wxLog::logexpanded(L"Value '%s' in 'Set value' action part of action '%s' (data id: %d) cannot be set because it is empty",
			                   variable.GetName().c_str().wc_str(), data.GetName().c_str().wc_str(), dataId(data));
		}

		return;
	}

	switch (_part.GetInt(kActionPartInt)) {
	case 0:
		variable.SetValue(kValueInt, right, TSendEventEnum::kSendEvent);
		break;
	case 1:
		variable.SetValue(kValueInt, right + left, TSendEventEnum::kSendEvent);
		break;
	case 2:
		variable.SetValue(kValueInt, left - right, TSendEventEnum::kSendEvent);
		break;
	case 3:
		variable.SetValue(kValueInt, right * left, TSendEventEnum::kSendEvent);
		break;
	case 4:
		// (a division by zero stops the original; here the variable stays as it is)
		if (right != 0)
			variable.SetValue(kValueInt, left / right, TSendEventEnum::kSendEvent);
		break;
	default:
		break;
	}
}

// Confirmed (asm lines 201197-201259, 205444-205526, 206898-207012, 207650-207659): a text is shown
// at a point of the scene; the same as kCommandShowText, but without a speaker.
void TGActionPartExecutor::cmdShowTextAt() {
	TextAlignmentEnum alignment = (TextAlignmentEnum)_game.GetInt(kGameSpeakerTextAlignment);
	TGameControl *control = gameControl();
	wxPoint position;

	position.x = _part.GetInt(kActionPartInt);
	position.y = _part.GetInt(kActionPartAltInt);

	if (_part.GetInt(kActionPartAltInt2) != 0) {
		if (_skip)
			return;

		TTText text(link());

		control->StartBackgroundText(text, nullptr, alignment, altLink(), position);
		return;
	}

	if (_skip) {
		if (started()) {
			control->ClearText(link());
			setStarted(false);
		}

		return;
	}

	TTText text(link());

	if (!started()) {
		control->StartText(text, nullptr, alignment, altLink(), position);
		setStarted(true);
	}

	if (started() && control->IsTextActive(text))
		_wait = true;
	else
		setStarted(false);
}

// Confirmed (asm lines 202518-202545)
void TGActionPartExecutor::cmdStopSound() {
	TSoundFFMPEG *sounds = gameControl()->GetSoundManager();

	sounds->Stop(_part.GetPath(kActionPartPath));
}

// Confirmed (asm lines 199448-199523, 205354-205359): IF a character's direction (the link, or the
// current character) is within the part's numbers: kActionPartAltInt 0 within kActionPartAltInt2
// of kActionPartInt, 1 from kActionPartInt up to kActionPartAltInt2 above it, and any other is
// always true.
void TGActionPartExecutor::cmdIfCharacterDirection() {
	TVisObjRef character = link();

	if (character.IsEmpty())
		character = _game.GetLink(kGameCurrentCharacter);

	int direction = character.GetInt(kCharacterDirection);
	int center = _part.GetInt(kActionPartInt);
	int mode = _part.GetInt(kActionPartAltInt);
	int range = _part.GetInt(kActionPartAltInt2);
	bool holds;

	if (mode == 0)
		holds = (direction >= center - range && direction <= center + range);
	else if (mode == 1)
		holds = (direction >= center && direction <= center + range);
	else
		holds = true;

	if (!holds)
		ifFalse();
}
