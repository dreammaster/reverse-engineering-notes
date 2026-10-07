// The second half of the commands of TGAction::Execute() (see TGActionCommands.cpp).
#include "TGActionExecutor.h"

#include <string>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "Easing.h"
#include "TGCharacter.h"
#include "TGInterface.h"
#include "TGObjectManager.h"
#include "TGScene.h"
#include "TSceneControl.h"
#include "TSoundFFMPEG.h"
#include "TTAction.h"
#include "TTScene.h"
#include "TTText.h"
#include "Tween.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vscommon/fontManager.h"
#include "vscommon/scripting/id.h"
#include "vscommon/scripting/lua.h"
#include "vsplayer/animationGame.h"
#include "vsplayer/control/cursorControl.h"
#include "vsplayer/control/gameControl.h"
#include "vsplayer/control/masterControl.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/actionGame.cpp";

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

static int dataId(const TVisObjRef &ref) {
	return PackVisId(ref.GetId());
}

// The id of a data object, as the TId the tables want.
static TId tableId(const TVisObjRef &ref) {
	return TId(PackVisId(ref.GetId()), ref.GetId()[3]);
}

// Confirmed (asm lines 202436-202515, 204869-204913): the volume of a sound the action part
// names is set; if it is the music of the scene it has the volume of the music, else the one of the
// sounds.
void TGActionPartExecutor::cmdSetMusicVolume() {
	TSoundFFMPEG *sounds = gameControl()->GetSoundManager();
	wxFileName path = _part.GetPath(kActionPartPath);
	wxFileName sceneMusic = gameControl()->GetScene()->GetRef().GetPath(kSceneBackgroundMusic);
	int volume = _part.GetInt(kActionPartInt);
	int balance = _part.GetInt(kActionPartAltInt);

	if (path.GetFullPath().Cmp(sceneMusic.GetFullPath()) == 0)
		sounds->SetStats(path, volume * sounds->GetMusicVolume() / 100, balance, TSoundTypeEnum::kMusic, false, 0);
	else
		sounds->SetStats(path, volume * sounds->GetSoundVolume() / 100, balance, TSoundTypeEnum::kSound, false, 0);
}

// Confirmed (asm lines 202395-202431, 204844-204866): the action goes on at another part: the one
// with the number of the part (counted from 1) or the one that many parts on (or back); a part that
// does not exist ends the action.
void TGActionPartExecutor::cmdGoto() {
	int target;

	if (_part.GetInt(kActionPartInt) != 0)
		target = _part.GetInt(kActionPartAltInt) - 1;
	else
		target = active().GetInt(kActionActionPartIndex) + _part.GetInt(kActionPartAltInt);

	if (target < 0 || (size_t)target >= _parts.size()) {
		active().SetValue(kActionActionPartIndex, -1, TSendEventEnum::kSendEvent);
		_newPosition = (int)_parts.size();
		_wait = true;
		return;
	}

	active().SetValue(kActionActionPartIndex, target, TSendEventEnum::kSendEvent);
	active().SetValue(kActionContinueWaitForEndIfElse, 0, TSendEventEnum::kSendEvent);
	_newPosition = target;
	_stay = true;
}

// Confirmed (asm lines 202663-202692)
void TGActionPartExecutor::cmdIfCurrentCharacter() {
	if (!(link() == currentCharacter()))
		ifFalse();
}

// Confirmed (asm lines 202559-202620): the character (the current one when there is none) has no items;
// what is held with the mouse is let go.
void TGActionPartExecutor::cmdClearItems() {
	TVisObjRef character = link();

	if (character.IsEmpty())
		character = _game.GetLink(kGameCurrentCharacter);

	gameControl()->GetObjectManager()->RemoveItem(false);

	if (!character.IsEmpty())
		character.SetValue(kCharacterItems, TVList(), true);
}

// Confirmed (asm lines 202623-202660)
void TGActionPartExecutor::cmdGiveAllItems() {
	TGCharacter *from = gameControl()->GetCharacter(link());
	TGCharacter *to = gameControl()->GetCharacter(altLink());

	from->GiveAllItemsTo(to->GetRef());
}

// Confirmed (asm lines 199991-200036): how fast the current outfit of a character walks.
void TGActionPartExecutor::cmdSetOutfitSpeed() {
	TVisObjRef character = orCurrentCharacter(link());
	TVisObjRef outfit = character.GetLink(kCharacterCurrentOutfit);

	outfit.SetValue(kOutfitCharacterSpeed, _part.GetInt(kActionPartInt), TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 203941-203976)
void TGActionPartExecutor::cmdRandomValue() {
	TTValue variable(link());

	variable.SetRandomValue(_part.GetInt(kActionPartInt), _part.GetInt(kActionPartAltInt));
}

// Confirmed (asm lines 203536-203601, 205299-205316, 206834-206842): IF a character is in a scene
// (the scene of the second link, or the one that is shown).
void TGActionPartExecutor::cmdIfCharacterInScene() {
	TGCharacter *character = gameControl()->GetCharacter(link());
	TTScene scene(altLink());
	TVisObjRef characterScene = character->GetRef().GetLink(kCharacterScene);
	bool inScene;

	if (scene.IsEmpty())
		inScene = (characterScene == gameControl()->GetScene()->GetRef());
	else
		inScene = (characterScene == scene);

	if (!inScene)
		ifFalse();
}

// Confirmed (asm lines 202127-202199, 206779-206806): a character follows another (the current
// one follows the one of the link when the integer is 1, else the one of the link follows the
// current one) to within a distance, and does an action when it is there.
void TGActionPartExecutor::cmdFollowCharacter() {
	TVisObjRef other = orCurrentCharacter(link());
	TTAction action(altLink());
	int distance = _part.GetInt(kActionPartAltInt);

	if (_part.GetInt(kActionPartInt) == 1) {
		TVisObjRef follower = currentCharacter();

		follower.SetValue(kCharacterFollowReachDistance, distance, TSendEventEnum::kSendEvent);
		follower.SetLink(kCharacterFollowAction, action, true);
		follower.SetLink(kCharacterFollowCharacter, other, true);
	} else {
		other.SetValue(kCharacterFollowReachDistance, distance, TSendEventEnum::kSendEvent);
		other.SetLink(kCharacterFollowAction, action, true);
		other.SetLink(kCharacterFollowCharacter, currentCharacter(), true);
	}
}

// Confirmed (asm lines 202091-202124)
void TGActionPartExecutor::cmdStopFollowing() {
	TVisObjRef character = gameControl()->GetCharacter(link())->GetRef();

	character.SetLink(kCharacterFollowCharacter, TVisObjRef(), true);
}

// Confirmed (asm lines 203512-203533)
void TGActionPartExecutor::cmdStopWalking() {
	gameControl()->GetCharacter(link())->StopWalking(true);
}

// Confirmed (asm lines 200706-200743)
void TGActionPartExecutor::cmdSetWalkingSound() {
	TVisObjRef character = orCurrentCharacter(link());

	character.SetValue(kCharacterWalkingSound, _part.GetPath(kActionPartPath), TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 200659-200703)
void TGActionPartExecutor::cmdSetFont() {
	TVisObjRef character = orCurrentCharacter(altLink());

	character.SetLink(kCharacterFont, link(), true);
}

// Confirmed (asm lines 200565-200656, 204784-204841): the interface of the link is put among the
// interfaces of the current character instead of the ones of its class (it takes the place of the
// last, and the scroll position of the items that the old one had).
void TGActionPartExecutor::cmdSetInterface() {
	TVisObjRef interfaceRef = link();
	TVisObjRef interfaceClass;

	if (!interfaceRef.IsEmpty())
		interfaceClass = interfaceRef.GetLink(kInterfaceInterfaceClass);

	TGCharacter *character = gameControl()->GetCurrentCharacter();
	TVList kept;
	int scrollPosition = -1;

	for (TGInterface *candidate : character->GetInterfaces()) {
		if (candidate->GetRef().GetLink(kInterfaceInterfaceClass) == interfaceClass)
			scrollPosition = candidate->GetRef().GetInt(kInterfaceItemsScrollPosition);
		else
			kept.push_back(candidate->GetRef());
	}

	if (!interfaceRef.IsEmpty()) {
		if (scrollPosition >= 0)
			interfaceRef.SetValue(kInterfaceItemsScrollPosition, scrollPosition, TSendEventEnum::kSendEvent);

		kept.push_back(interfaceRef);
	}

	character->GetRef().SetValue(kCharacterInterfaces, kept, true);
}

// Confirmed (asm lines 200459-200499)
void TGActionPartExecutor::cmdIfLanguage() {
	if (TTText::GetLanguageId() != dataId(link()))
		ifFalse();
}

// Confirmed (asm lines 200395-200456, 205328-205351, 206352-206392): the cursor is set: the one of
// the link, or the one of a menu scene, or the one of the active command.
void TGActionPartExecutor::cmdSetCursor() {
	TVisObjRef cursor = link();
	TCursorControl *cursors = gameControl()->GetCursorControl();

	if (!cursor.IsEmpty()) {
		cursors->SetCursor(dataId(cursor), true);
		return;
	}

	if (gameControl()->GetScene()->IsMenu()) {
		TVisObjRef sceneCursor = gameControl()->GetScene()->GetRef().GetLink(kSceneCursor);

		cursors->SetCursor(dataId(sceneCursor), true);
		return;
	}

	TVisObjRef command = _game.GetLink(kGameActiveCommand);

	if (!command.IsEmpty())
		cursors->SetCursor(dataId(command), false);
}

// Confirmed (asm lines 200524-200562)
void TGActionPartExecutor::cmdIfCommand() {
	if (!(link() == _game.GetLink(kGameActiveCommand)))
		ifFalse();
}

// Confirmed (asm lines 200355-200392)
void TGActionPartExecutor::cmdCharacterActive() {
	TVisObjRef character = orCurrentCharacter(link());

	character.SetValue(kCharacterActive, _part.GetInt(kActionPartInt) != 1, TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 200513-200521)
void TGActionPartExecutor::cmdSaveObject() {
	gameControl()->GetObjectManager()->SaveCurrentObject();
}

// Confirmed (asm lines 200502-200510)
void TGActionPartExecutor::cmdExecuteSavedObject() {
	gameControl()->GetObjectManager()->ExecuteSavedObject();
}

// Confirmed (asm lines 200334-200352, 204231-204304, 209839-209849): IF the object that the mouse is
// on (0 there is one, 1 there is none, 2 it is an object that is not an item, 3 it is an item, 4 it is
// a character).
void TGActionPartExecutor::cmdIfCurrentObject() {
	TVisObjRef object = gameControl()->GetObjectManager()->GetCurrentObject();
	bool holds;

	switch (_part.GetInt(kActionPartInt)) {
	case 0:
		holds = !object.IsEmpty();
		break;
	case 1:
		holds = object.IsEmpty();
		break;
	case 2:
		holds = !object.IsEmpty() && object.GetId()[3] == 6 && !object.GetBool(kObjectIsItem);
		break;
	case 3:
		holds = !object.IsEmpty() && object.GetId()[3] == 6 && object.GetBool(kObjectIsItem);
		break;
	case 4:
		holds = !object.IsEmpty() && object.GetId()[3] == 0;
		break;
	default:
		return;
	}

	if (!holds)
		ifFalse();
}

// Confirmed (asm lines 200285-200331, 206395-206426): a command button (the link) is shown active,
// or is shown as not active (unless it is the active command of its interface).
void TGActionPartExecutor::cmdSetObjectActive() {
	TVisObjRef button = link();
	TManagedObject *object = gameControl()->GetObject(button);

	if (!object)
		return;

	bool active = (_part.GetInt(kActionPartInt) != 0);

	if (!active) {
		TVisObjRef interfaceRef = button.GetParent();

		if (button == interfaceRef.GetLink(kInterfaceActiveCommand))
			return;
	}

	object->SetActive(active);
}

// Confirmed (asm lines 203828-203836)
void TGActionPartExecutor::cmdClearSavedObject() {
	_game.ClearLink(kGameSavedObject, true);
}

// Confirmed (asm lines 203819-203825)
void TGActionPartExecutor::cmdSkipText() {
	gameControl()->SkipCurrentText();
}

// Confirmed (asm lines 203796-203816)
void TGActionPartExecutor::cmdFade() {
	_game.SetValue(kGameFadeEffect, _part.GetInt(kActionPartInt), TSendEventEnum::kSendEvent);
	_game.SetValue(kGameFadeDelay, _part.GetInt(kActionPartAltInt), TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 203753-203793, 205642-205648, 207684-207728): an object (or a character)
// goes to a visibility in the time of the second integer; the visibility is a variable's value or
// the part's first integer.
void TGActionPartExecutor::cmdSetVisibility(bool character) {
	TVisObjRef variable = altLink();
	int visibility = variable.IsEmpty() ? _part.GetInt(kActionPartInt) : variable.GetInt(kValueInt);
	TVisObjRef target = link();

	if (character) {
		target = orCurrentCharacter(target);
		target.SetValue(kCharacterTimeToDestVisibility, _part.GetInt(kActionPartAltInt), TSendEventEnum::kNoEvent);
		target.SetValue(kCharacterDestVisibility, visibility, TSendEventEnum::kSendEvent);
	} else {
		target.SetValue(kObjectTimeToDestVisibility, _part.GetInt(kActionPartAltInt), TSendEventEnum::kNoEvent);
		target.SetValue(kObjectDestVisibility, visibility, TSendEventEnum::kSendEvent);
	}
}

// Confirmed (asm lines 204076-204180, 204511-204530, 204714-204723): the interfaces of the class of
// the link that are active fade to a visibility in the time of the second integer.
void TGActionPartExecutor::cmdFadeInterface() {
	TVisObjRef interfaceClass = link();

	for (TGInterface *candidate : gameControl()->GetActiveInterfaces()) {
		if (!(candidate->GetRef().GetLink(kInterfaceInterfaceClass) == interfaceClass))
			continue;

		TVisObjRef interfaceRef = candidate->GetRef();

		if (!interfaceRef.IsEmpty()) {
			interfaceRef.SetValue(kInterfaceTimeToDestVisibility, _part.GetInt(kActionPartAltInt), TSendEventEnum::kNoEvent);
			interfaceRef.SetValue(kInterfaceDestVisibility, _part.GetInt(kActionPartInt), TSendEventEnum::kSendEvent);
		} else if (wxLog::loglevel > 2) {
			TVisObjRef data = _action.GetDataObject();
			wxString className = interfaceClass.IsEmpty() ? wxString(L"Empty") : interfaceClass.GetName().c_str();

			wxLog::logexpanded(L"Could not find interface with class '%s' for action part FADE_INTERFACE in action '%s' (data id: %d). ",
			                   className.wc_str(), data.GetName().c_str().wc_str(), dataId(data));
		}
	}
}

// Confirmed (asm lines 203994-204036, 205631-205639)
void TGActionPartExecutor::cmdSetLightMap() {
	TVisObjRef scene = orCurrentScene(link());

	scene.SetValue(kSceneLightMap, _part.GetPath(kActionPartPath), TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 201538-201570, 205665-205673)
void TGActionPartExecutor::cmdSetBrightness() {
	TVisObjRef scene = orCurrentScene(link());

	scene.SetValue(kSceneBrightness, _part.GetInt(kActionPartInt), TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 201508-201535)
void TGActionPartExecutor::cmdSetItem() {
	gameControl()->GetObjectManager()->SetItem(link(), _part.GetInt(kActionPartInt) == 1);
}

// Confirmed (asm lines 204039-204073, 206530-206560, 206770-206776, 207584-207594): the character
// walks (or, skipping, is put) to the position of an object; the part waits until it is there.
void TGActionPartExecutor::cmdCharacterGoToPoint() {
	TGCharacter *character = gameControl()->GetCharacter(link());

	if (!started()) {
		TVisObjRef object = altLink();

		character->GetRef().SetValue(_skip ? kCharacterPosition : kCharacterDestination,
		                             *object.GetPoint(kObjectPosition), TSendEventEnum::kSendEvent);
		setStarted(true);
	}

	if (!_skip && character->IsWalking())
		_wait = true;
	else
		setStarted(false);
}

// Confirmed (asm lines 203979-203991, 205651-205662): a savegame (the one of the second integer, or
// the one that was used last) is deleted.
void TGActionPartExecutor::cmdDeleteSavegame() {
	if (_part.GetInt(kActionPartInt) == 0)
		gameControl()->DeleteSavegame(-1);
	else
		gameControl()->DeleteSavegame(_part.GetInt(kActionPartAltInt));
}

// Confirmed (asm lines 203316-203336, 206270-206279, 207023-207031, 207174-207184, 207550-207574,
// 208707-208717, 209201-209211, 210973-210983): IF a savegame exists (0 the last one, 1 the autosave,
// 2 the second autosave, 3 the one of the second integer).
void TGActionPartExecutor::cmdIfSavegame() {
	int slot;

	switch (_part.GetInt(kActionPartInt)) {
	case 0:
		slot = -1;
		break;
	case 1:
		slot = -2;
		break;
	case 2:
		slot = -3;
		break;
	case 3:
		slot = _part.GetInt(kActionPartAltInt);
		break;
	default:
		x_assert(false, "false", kSourceFile, 0x1FC);
		return;
	}

	if (!gameControl()->SavegameExists(slot))
		ifFalse();
}

// Confirmed (asm lines 203279-203313)
void TGActionPartExecutor::cmdSetAnimationIndex() {
	TVisObjRef character = orCurrentCharacter(link());

	character.SetValue(kCharacterAnimIndex, _part.GetInt(kActionPartInt), TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 203393-203401, 206730-206757): waits for a sound to end.
void TGActionPartExecutor::cmdWaitSound() {
	if (_skip)
		return;

	TSoundFFMPEG *sounds = gameControl()->GetSoundManager();

	if (!sounds)
		return;

	if (sounds->IsPlaying(_part.GetPath(kActionPartPath)))
		_wait = true;
}

// Confirmed (asm lines 203339-203390, 203404-203455): the area of the scene that can be scrolled to
// gets another left and right (`horizontal`) or top and bottom.
void TGActionPartExecutor::cmdSetScrollableArea(bool horizontal) {
	TVisObjRef scene = orCurrentScene(link());
	wxRect area = *scene.GetRect(kSceneScrollableArea);
	int first = _part.GetInt(kActionPartInt);
	int second = _part.GetInt(kActionPartAltInt);

	if (horizontal) {
		area.SetLeft(first);
		area.SetRight(second);
	} else {
		area.SetTop(first);
		area.SetBottom(second);
	}

	scene.SetValue(kSceneScrollableArea, area, TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 200039-200065, 205529-205537)
void TGActionPartExecutor::cmdSetTextOutput() {
	int output = _part.GetInt(kActionPartInt);

	if ((unsigned int)output > 2) {
		x_assert(false, "false", kSourceFile, 0x305);
		return;
	}

	_game.SetValue(kGameTextOutput, output, TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 203680-203750): a character (the current one) is put at a point of a scene
// (the second link, or the one that is shown) with a direction.
void TGActionPartExecutor::cmdPlaceCharacterAt() {
	TVisObjRef character = orCurrentCharacter(link());
	TVisObjRef scene = orCurrentScene(altLink());
	wxPoint position;

	position.x = _part.GetInt(kActionPartInt);
	position.y = _part.GetInt(kActionPartAltInt);
	gameControl()->GetScene()->SetCharacter(character, scene, position, _part.GetInt(kActionPartAltInt2));
}

// Confirmed (asm lines 203646-203677): the way system of a scene (the parent of the way).
void TGActionPartExecutor::cmdSetWaySystem() {
	TVisObjRef way = link();
	TVisObjRef scene = way.GetParent();

	scene.SetLink(kSceneCurrentWaySystem, way, true);
}

// Confirmed (asm lines 203899-203938, 203604-203643): the first and the last frame of a running
// animation.
void TGActionPartExecutor::cmdSetAnimationFrame(bool last) {
	TVisObjRef animation = link();
	TVisObjRef running = gameControl()->GetVisionaire()->GetActiveObject(26, tableId(animation));

	running.SetValue(last ? kAnimationLastFrame : kAnimationFirstFrame, _part.GetInt(kActionPartInt),
	                 TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 202059-202088, 205319-205325)
void TGActionPartExecutor::cmdSetTextSpeed() {
	TVisObjRef variable = link();
	int speed = variable.IsEmpty() ? _part.GetInt(kActionPartInt) : variable.GetInt(kValueInt);

	_game.SetValue(kGameTextSpeed, speed, TSendEventEnum::kSendEvent);
}

// A script of the game's own Lua is run: `currentAction` is the action while it runs. The
// scripts are the text with a '<' for each new line.
static void runScript(const TVisObjRef &action, wxString code, const std::uint8_t *id) {
	LuaSetCurrentAction(action);
	code.Replace(wxString(L"<"), wxString(L"\n"), true);
	LuaDoString(std::string(code.mb_str()), IdStrStd(id));
	LuaSetCurrentAction(TVisObjRef());
}

// Confirmed (asm lines 201745-201841): the script of a script object (the link).
void TGActionPartExecutor::cmdRunScript() {
	TVisObjRef script = link();

	if (script.IsEmpty())
		return;

	runScript(active(), script.GetStr(kScriptScript), script.GetId());
}

// Confirmed (asm lines 201636-201742): the script that is the action part's own string.
void TGActionPartExecutor::cmdRunPartScript() {
	runScript(active(), _part.GetStr(kActionPartString), _part.GetId());
}

// Confirmed (asm lines 201597-201633, 206931-206941): a variable (the link) is made true (0), false
// (1) or the other (2).
void TGActionPartExecutor::cmdSetCondition() {
	TTCondition condition(link());
	int mode = _part.GetInt(kActionPartInt);

	if (mode == 2)
		condition.SetTo(!condition.GetBool(kConditionValue));
	else
		condition.SetTo(mode == 0);
}

// Confirmed (asm lines 203856-203896): IF a variable (the link) is true (the integer 0) or false.
void TGActionPartExecutor::cmdIfCondition() {
	TTCondition condition(link());
	bool expected = (_part.GetInt(kActionPartInt) == 0);

	if (condition.IsTrue() != expected)
		ifFalse();
}

// Confirmed (asm lines 203839-203853)
void TGActionPartExecutor::cmdHideCursor() {
	_game.SetValue(kGameHideCursor, _part.GetInt(kActionPartInt) == 1, TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 203497-203509)
void TGActionPartExecutor::cmdHideInterfaces() {
	_game.SetValue(kGameHideInterfaces, _part.GetInt(kActionPartInt) == 1, TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 203458-203492, 205286-205296): the screen shakes (with a force and a speed),
// or stops (with the integer).
void TGActionPartExecutor::cmdEarthquake() {
	if (_part.GetInt(kActionPartInt) != 0) {
		_game.SetValue(kGameQuake, false, TSendEventEnum::kForce);
		return;
	}

	int force = _part.GetInt(kActionPartAltInt);

	if (force <= 0)
		return;

	_game.SetValue(kGameQuakeForce, force, TSendEventEnum::kNoEvent);
	_game.SetValue(kGameQuakeSpeed, _part.GetInt(kActionPartAltInt2), TSendEventEnum::kNoEvent);
	_game.SetValue(kGameQuake, true, TSendEventEnum::kForce);
}

// Confirmed (asm lines 200246-200270, 204994-205030, 206337-206339): a cutscene starts (the cursor
// and the interfaces go away, the action is the game's cutscene action) or ends (and when it is skipped
// the action is over with it).
void TGActionPartExecutor::cmdCutscene() {
	if (_part.GetInt(kActionPartInt) == 0) {
		_game.SetValue(kGameHideCursor, true, TSendEventEnum::kSendEvent);
		_game.SetValue(kGameHideInterfaces, true, TSendEventEnum::kSendEvent);
		_game.SetLink(kGameCutsceneAction, active(), false);
		return;
	}

	_game.SetValue(kGameHideCursor, false, TSendEventEnum::kSendEvent);
	_game.SetValue(kGameHideInterfaces, false, TSendEventEnum::kSendEvent);

	if (_skip) {
		int next = active().GetInt(kActionActionPartIndex) + 1;

		active().SetValue(kActionActionPartIndex, next, TSendEventEnum::kSendEvent);
		_wait = (next < (int)_parts.size());
	}

	_game.ClearLink(kGameCutsceneAction, false);
}

// The state of the action is saved with the game by the one that saves it: only the index of the
// part that is next is changed for the time of the save.
void TGActionPartExecutor::saveGame(int slot, bool saveAfter) {
	TGAction::s_saveAction = active();

	if (!saveAfter) {
		_action._finished = true;
		gameControl()->SaveGame(slot);
		return;
	}

	int index = active().GetInt(kActionActionPartIndex);

	active().SetValue(kActionActionPartIndex, index + 1, TSendEventEnum::kSendEvent);

	if (index + 1 >= (int)_parts.size())
		_action._finished = true;

	gameControl()->SaveGame(slot);
	active().SetValue(kActionActionPartIndex, index, TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 200230-200243, 205540-205601, 205736-205792, 205940-205955, 207372-207385,
// 207855-207901): loads (0) or saves one of the autosaves (the second integer is its number, up to 99).
void TGActionPartExecutor::cmdAutosave() {
	if (gameControl()->GetSceneControl()->FadingToNewScene()) {
		if (wxLog::loglevel > 0) {
			logPartNotDone(L"'Load/Save autosave' action part of action '%s' (data id: %d) was not executed because scene is currently faded.");
		}

		return;
	}

	int slot = _part.GetInt(kActionPartAltInt);

	if (_part.GetInt(kActionPartInt) == 0) {
		_action._finished = true;

		if ((unsigned int)slot <= 99)
			gameControl()->LoadGame(slot);

		_wait = true;
		return;
	}

	TGAction::s_saveAction = active();

	if ((unsigned int)slot > 99)
		return;

	saveGame(slot, _part.GetInt(kActionPartAltInt2) != 0);
	_action._finished = false;
}

// Confirmed (asm lines 200214-200227, 205854-205910, 206035-206045, 207229-207277): the same for the
// game that is saved over (the slot -1).
void TGActionPartExecutor::cmdGameSave() {
	if (gameControl()->GetSceneControl()->FadingToNewScene()) {
		if (wxLog::loglevel > 0) {
			logPartNotDone(L"'Load/Save game' action part of action '%s' (data id: %d) was not executed because scene is currently faded.");
		}

		return;
	}

	if (_part.GetInt(kActionPartInt) == 0) {
		_action._finished = true;
		gameControl()->LoadGame(-1);
		_wait = true;
		return;
	}

	saveGame(-1, _part.GetInt(kActionPartAltInt) != 0);
}

// Confirmed (asm lines 200139-200210, 205958-205963, 206048-206053, 206760-206884, 207363-207539):
// sets a volume (0 the sounds, 1 the music, 2 the speech, 3 the movies, 4 all) to a number
// (the third integer), or by a number more or less than it is now (the second integer 1, 2).
void TGActionPartExecutor::cmdSetVolume() {
	int kind = _part.GetInt(kActionPartInt);
	TSoundFFMPEG *sounds = gameControl()->GetSoundManager();
	int volume = 0;

	switch (kind) {
	case 0:
		volume = sounds->GetSoundVolume();
		break;
	case 1:
		volume = sounds->GetMusicVolume();
		break;
	case 2:
		volume = sounds->GetSpeechVolume();
		break;
	case 3:
		volume = sounds->GetMovieVolume();
		break;
	case 4:
		volume = sounds->GetGlobalVolume();
		break;
	default:
		x_assert(false, "false", kSourceFile, 0x260);
		break;
	}

	switch (_part.GetInt(kActionPartAltInt)) {
	case 0:
		volume = _part.GetInt(kActionPartAltInt2);
		break;
	case 1:
		volume += _part.GetInt(kActionPartAltInt2);
		break;
	case 2:
		volume -= _part.GetInt(kActionPartAltInt2);
		break;
	default:
		break;
	}

	if (volume > 100)
		volume = 100;

	if (volume < 0)
		volume = 0;

	// (a volume that is -1 is not changed)
	switch (kind) {
	case 0:
		sounds->SetVolume(-1, volume, -1, -1, -1);
		break;
	case 1:
		sounds->SetVolume(volume, -1, -1, -1, -1);
		break;
	case 2:
		sounds->SetVolume(-1, -1, volume, -1, -1);
		break;
	case 3:
		sounds->SetVolume(-1, -1, -1, volume, -1);
		break;
	case 4:
		sounds->SetVolume(-1, -1, -1, -1, volume);
		break;
	default:
		break;
	}
}

// Confirmed (asm lines 202357-202392, 206821-206831, 206959-206965): another action (the link) is
// started - and does its first parts at once when that is how the game is set
// (kGameExecuteCalledActionImmediately) or the cutscene is skipped - or (the integer) is ended.
void TGActionPartExecutor::cmdStartAction() {
	TVisObjRef called = link();

	if (_part.GetInt(kActionPartInt) != 0) {
		TGAction::DeleteRunningAction(called, true);
		return;
	}

	if (!_skip && !_game.GetBool(kGameExecuteCalledActionImmediately)) {
		TGAction::AddRunningAction(called);
		return;
	}

	TGAction *action = TGAction::AddRunningAction(called);

	if (action)
		action->Execute(_skip, _skipInfo);
}

// Confirmed (asm lines 202329-202354, 206944-206956): an animation is kept loaded (the integer 0,
// not while skipping) or let go.
void TGActionPartExecutor::cmdPreloadAnimation() {
	TVisObjRef animation = link();

	if (_part.GetInt(kActionPartInt) != 0)
		TGAnimation::UnloadAnimation(animation);
	else if (!_skip)
		TGAnimation::PreloadAnimation(animation);
}

// Confirmed (asm lines 202248-202326, 205426-205441, 207162-207171): an item (the link) is given to
// a character (the second link, the current one when there is none), or (the integer) taken from it.
void TGActionPartExecutor::cmdCharacterItem() {
	TVisObjRef item = link();
	TTCharacter character(altLink());

	if (character.IsEmpty())
		character = TTCharacter(_game.GetLink(kGameCurrentCharacter));

	if (_part.GetInt(kActionPartInt) != 0) {
		gameControl()->GetObjectManager()->RemoveItem(item);

		if (!character.IsEmpty())
			character.RemoveItem(item);
	} else if (!character.IsEmpty()) {
		character.AddItem(item, _part.GetInt(kActionPartAltInt) != 0);
	}
}

// Confirmed (asm lines 202230-202245)
void TGActionPartExecutor::cmdScrollSavegames() {
	gameControl()->GetScene()->ScrollSavegames(_part.GetInt(kActionPartInt) != 0);
}

// Confirmed (asm lines 201105-201194, 204916-204991, 207605-207637): a running animation (the one
// of the link, or the one this action is a frame action of) goes to another position: the one in
// the part's two integers (the third integer 0), or moved by them (1: more, 2: less).
void TGActionPartExecutor::cmdMoveAnimation() {
	TVisObjRef running;
	TVisObjRef animation = link();

	if (!animation.IsEmpty()) {
		running = gameControl()->GetVisionaire()->GetActiveObject(26, tableId(animation));
	} else if (_part.GetParent().GetParent().GetId()[3] == 0x1C) {
		TVisObjRef owner = _part.GetParent().GetParent().GetParent();

		running = gameControl()->GetVisionaire()->GetActiveObject(26, tableId(owner));
	}

	if (running.IsEmpty())
		return;

	wxPoint position = *running.GetPoint(kAnimationCurrentPosition);
	int x = _part.GetInt(kActionPartInt);
	int y = _part.GetInt(kActionPartAltInt);

	switch (_part.GetInt(kActionPartAltInt2)) {
	case 0:
		position.x = x;
		position.y = y;
		break;
	case 1:
		position.x += x;
		position.y += y;
		break;
	case 2:
		position.x -= x;
		position.y -= y;
		break;
	default:
		x_assert(false, "false", kSourceFile, 0x444);
		break;
	}

	running.SetValue(kAnimationCurrentPosition, position, TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 201079-201102, 206851-206869): the snoop animations of the scene (the ones that
// show when the mouse is over an object) fade in (the first integer 0, not while skipping) or out
// in the time of the second integer.
void TGActionPartExecutor::cmdSnoopAnimations() {
	TGScene *scene = gameControl()->GetScene();
	int time = _part.GetInt(kActionPartAltInt);

	if (_part.GetInt(kActionPartInt) != 0)
		scene->ShowSnoopAnimations(false, time);
	else if (!_skip)
		scene->ShowSnoopAnimations(true, time);

	scene->UpdateSnoopAnimAlpha();
}

// Confirmed (asm lines 201025-201076, 205180-205265, 207542-207602): the animations of a character
// are loaded or let go (the integer): not those of a character that is in the scene.
void TGActionPartExecutor::cmdPreloadCharacter() {
	TGCharacter *character = gameControl()->GetCharacter(link());
	bool unload = (_part.GetInt(kActionPartInt) != 0);

	if (character->GetRef().GetLink(kCharacterScene) == gameControl()->GetScene()->GetRef()) {
		if ((g_traceFlags & 2) && wxLog::loglevel > 1) {
			TVisObjRef data = _action.GetDataObject();
			TVisObjRef characterRef = character->GetRef();

			wxLog::logexpanded(L"'Preload/Unload character' action part of action '%s' (data id: %d) was not executed because character '%s' (id: %d) is on current scene.",
			                   data.GetName().c_str().wc_str(), dataId(data), characterRef.GetName().c_str().wc_str(),
			                   dataId(characterRef));
		}

		return;
	}

	if (unload)
		character->UnloadAnimations();
	else if (!_skip)
		character->PreloadAnimations();
}

// Confirmed (asm lines 200823-201022, 207640-207647): a text is shown at an object (the second
// link), with the game's object font, at a point (the integers), left (0), right (1) or centered
// (2) - when the alignment is none of these it is left and a message is logged.
void TGActionPartExecutor::cmdShowObjectText() {
	TTText text(link());
	TVisObjRef object = altLink();
	TVisObjRef font = _game.GetLink(kGameObjectFont);
	wxPoint position;
	TextAlignmentEnum alignment = TextAlignmentEnum::kLeft;

	position.x = _part.GetInt(kActionPartInt);
	position.y = _part.GetInt(kActionPartAltInt);

	switch (_part.GetInt(kActionPartAltInt2)) {
	case 0:
		break;
	case 1:
		alignment = TextAlignmentEnum::kRight;
		break;
	case 2:
		alignment = TextAlignmentEnum::kCenter;
		break;
	default:
		if (wxLog::loglevel > 0) {
			TVisObjRef data = _action.GetDataObject();

			wxLog::logexpanded(L"Invalid alignment for text in 'Display object text' action part of action '%s' (data id: %d).",
			                   data.GetNameWithParents(3).wc_str(), dataId(data));
		}

		break;
	}

	if (font.IsEmpty() && wxLog::loglevel > 0) {
		TVisObjRef data = _action.GetDataObject();

		wxLog::logexpanded(L"No object font set. The text in 'Display object text' action part of action '%s' (data id: %d) will not be visible.",
		                   data.GetNameWithParents(3).wc_str(), dataId(data));
	}

	gameControl()->StartObjectText(text, object, alignment, font, position);
}

// Confirmed (asm lines 199970-199988)
void TGActionPartExecutor::cmdClearObjectText() {
	gameControl()->ClearObjectText(link());
}

// Confirmed (asm lines 199945-199967, 206915-206922): waits for a character to be done talking.
void TGActionPartExecutor::cmdWaitTalking() {
	TGCharacter *character = gameControl()->GetCharacter(link());

	if (!_skip && gameControl()->IsTalking(character->GetRef()))
		_wait = true;
}

// Confirmed (asm lines 199738-199790 and the tween of the next commands): an object's offset
// moves by the two integers in the time of the third (`by`), or goes to the point where the object is shown
// at the two integers (a position of the scene).
void TGActionPartExecutor::cmdMoveObject(bool by) {
	TVisObjRef object = link();
	int first = _part.GetInt(kActionPartInt);
	int second = _part.GetInt(kActionPartAltInt);
	double duration = _part.GetInt(kActionPartAltInt2);
	const wxPoint offset = *object.GetPoint(kObjectOffset);
	double endX;
	double endY;

	if (by) {
		endX = offset.x + first;
		endY = offset.y + second;
	} else {
		const wxPoint position = *object.GetPoint(kObjectPosition);

		endX = first - position.x;
		endY = second - position.y;
	}

	Tween x(offset.x, endX, duration, Easing::LinearIn, false, false);
	Tween y(offset.y, endY, duration, Easing::LinearIn, false, false);

	gameControl()->StartTween(TVisObjTween(x, object, kObjectOffset, y));
}
