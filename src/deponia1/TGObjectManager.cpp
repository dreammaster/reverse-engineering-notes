#include "TGObjectManager.h"

#include "AppGlobals.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TGScene.h"
#include "TManagedObject.h"
#include "TTButton.h"
#include "vsplayer/control/cursorControl.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/visionaireGame.h"

// TGameControl implements every one of these accessors, but g_pGameControl
// is only declared as TMasterControl* (AppGlobals.h) - same cast already
// established at TManagedObject::ClickedWithoutReach's own call site.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

void TGObjectManager::ResetEventInfo() {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	game.ClearLink(0x2AE, true);
	g_pGameControl->GetCursorControl()->ReleaseMoveObject();
	game.SetValue(0x2DA, false, TSendEventEnum::kSendEvent);
}

void TGObjectManager::ResetCurrentObject() {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	game.ClearLink(0x2AE, true);
}

void TGObjectManager::RemoveItem(const TVisObjRef &item) {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	if (!(game.GetLink(0x2AE) == item))
		return;
	game.ClearLink(0x2AE, true);
	if (!game.GetBool(0x2DA))
		return;
	g_pGameControl->GetCursorControl()->ReleaseMoveObject();
	game.SetValue(0x2DA, false, TSendEventEnum::kSendEvent);
}

void TGObjectManager::RemoveItem(bool keepIfNotHeld) {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	bool held = game.GetBool(0x2DA);
	if (!held && keepIfNotHeld)
		return;
	game.ClearLink(0x2AE, true);
	if (!held)
		return;
	g_pGameControl->GetCursorControl()->ReleaseMoveObject();
	game.SetValue(0x2DA, false, TSendEventEnum::kSendEvent);
}

void TGObjectManager::SetItem(const TVisObjRef &item, bool held) {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	game.SetLink(0x2AE, item, true);
	game.SetValue(0x2DA, held && !item.IsEmpty(), TSendEventEnum::kSendEvent);
	if (held)
		g_pGameControl->GetCursorControl()->SetMoveObject(item);
	else
		g_pGameControl->GetCursorControl()->ReleaseMoveObject();
}

void TGObjectManager::HandleEvent(TMouseEventEnum /*event*/) {
}

void TGObjectManager::ObjectReached(TManagedObject */*object*/) {
}

void TGObjectManager::ObjectReached(TGCharacter &character, TVisObjRef &target) {
	TGCharacter *currentCharacter = gameControl()->GetCurrentCharacter();
	if (!currentCharacter)
		return;
	if (!(currentCharacter->GetRef().GetLink(0x1F7) == gameControl()->GetScene()->GetRef()))
		return;
	if (!(character.GetRef() == currentCharacter->GetRef()))
		return;
	TManagedObject *object = gameControl()->GetScene()->GetObject(target);
	ObjectReached(object);
}

void TGObjectManager::SaveEventInfo(TMouseEventEnum event) {
	TVisObjRef game = gameControl()->GetVisionaire()->GetGame();
	TVisObjRef gameSystemGame = gameControl()->GetGameSystem()->GetGame();
	game.SetLink(0x1E3, gameSystemGame.GetLink(0x262), true);
	game.SetLink(0x1E4, game.GetLink(0x2AE), true);
	game.SetValue(0x1E5, game.GetBool(0x2DA), TSendEventEnum::kSendEvent);
	game.SetValue(0x267, static_cast<int>(event), TSendEventEnum::kSendEvent);
}

void TGObjectManager::MouseMove(TManagedObject *object) {
	if (_currentObject == object)
		return;

	// Confirmed call shape only, skipped here: before updating the hover
	// object below, the real method fires TManagedObject::ExecuteEvent()
	// on the old object being left (if any) and the new one being entered
	// (if any) - a "mouse left"/"mouse entered" notification, each with a
	// freshly-built TGEventInfo. Same action-execution-subsystem gap
	// flagged on TManagedObject and HandleEvent()/ObjectReached() above.

	_currentObject = object;
	if (!object) {
		gameControl()->GetGameSystem()->GetGame().ClearLink(0x2AF, true);
		return;
	}
	if (!object->GetRef().IsEmpty())
		gameControl()->GetGameSystem()->GetGame().SetLink(0x2AF, object->GetRef(), true);
}

void TGObjectManager::NotifyObjectRemoved(const TVisObjRef &item) {
	if (_savedObject && item == _savedObject->GetRef()) {
		_savedObject = nullptr;
		gameControl()->GetGameSystem()->GetGame().ClearLink(0x1E6, true);
	}
	if (_currentObject && item == _currentObject->GetRef()) {
		_currentObject = nullptr;
		gameControl()->GetGameSystem()->GetGame().ClearLink(0x2AF, true);
	}
}

void TGObjectManager::SaveCurrentObject() {
	_savedObject = _currentObject;
	TVisObjRef game = gameControl()->GetVisionaire()->GetGame();
	if (_savedObject)
		game.SetLink(0x1E6, _savedObject->GetRef(), false);
	else
		game.ClearLink(0x1E6, false);
}

void TGObjectManager::ExecuteSavedObject() {
	if (_currentObject && (!_savedObject || !(_currentObject->GetRef() == _savedObject->GetRef()))) {
		// Confirmed call shape only, skipped here: fires TManagedObject::
		// ExecuteEvent() on the object we're about to replace, with a
		// freshly-built TGEventInfo - same action-execution-subsystem gap
		// as MouseMove()/HandleEvent()/ObjectReached() above.
	}
	_currentObject = _savedObject;
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	if (!_savedObject || _savedObject->GetRef().IsEmpty())
		game.ClearLink(0x2AF, true);
	else
		game.SetLink(0x2AF, _savedObject->GetRef(), true);
	HandleEvent(TMouseEventEnum::kValue1);
}

void TGObjectManager::SavedObjectChanged() {
	TVisObjRef game = gameControl()->GetVisionaire()->GetGame();
	TVisObjRef link = game.GetLink(0x1E6);
	if (link.IsEmpty())
		_savedObject = nullptr;
	else
		_savedObject = gameControl()->GetObject(link);
}

bool TGObjectManager::IsCurrentObjectEmpty() const {
	return !_currentObject || _currentObject->GetRef().IsEmpty();
}

bool TGObjectManager::IsCurrentObjectDetectable() const {
	if (!_currentObject)
		return false;
	if (_currentObject->GetRef().GetId()[3] != 2)
		return false;
	return _currentObject->GetRef().GetInt(0x129) != 0;
}

bool TGObjectManager::IsCurrentObjectWalkable() const {
	if (!_currentObject)
		return false;
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	TTButton button(game.GetLink(0x262));
	if (!_currentObject->IsWalkable())
		return false;
	if (!button.IsStandardCommand())
		return false;
	return !game.GetBool(0x2DA);
}

TVisObjRef TGObjectManager::GetCurrentObject() const {
	if (_currentObject)
		return _currentObject->GetRef();
	return gameControl()->GetGameSystem()->GetEmptyObject();
}

void TGObjectManager::GetDetectInfo(TGDetectInfo &info) const {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	if (game.GetLink(0x262).IsEmpty()) {
		info.flagA = true;
		info.flagB = false;
		return;
	}
	int type = game.GetInt(0x150);
	if (type == 2) {
		info.flagA = true;
		info.flagB = false;
		return;
	}
	if (type != 0 && type != 1)
		return;
	info.flagA = (type == 0);
	info.flagB = true;
	if (!game.GetBool(0x265))
		info.character = gameControl()->GetCurrentCharacter()->GetRef();
}

TGCharacter *TGObjectManager::GetEventCharacter() const {
	TGCharacter *character = gameControl()->GetCurrentCharacter();
	if (!character)
		return nullptr;
	if (character->GetRef().GetLink(0x1F7) == gameControl()->GetScene()->GetRef())
		return character;
	return nullptr;
}

TVisObjRef TGObjectManager::GetEventCommand() const {
	return gameControl()->GetGameSystem()->GetGame().GetLink(0x262);
}

wxString TGObjectManager::GetActionText() const {
	return wxString();
}
