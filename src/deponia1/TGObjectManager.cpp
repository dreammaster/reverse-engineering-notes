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
#include "vstables/fieldIds.h"

// TGameControl implements every one of these accessors, but g_pGameControl
// is only declared as TMasterControl* (AppGlobals.h) - same cast already
// established at TManagedObject::ClickedWithoutReach's own call site.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

void TGObjectManager::ResetEventInfo() {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	game.ClearLink(kGameUsedItem, true);
	g_pGameControl->GetCursorControl()->ReleaseMoveObject();
	game.SetValue(kGameUsedItemPicked, false, TSendEventEnum::kNoEvent);
}

void TGObjectManager::ResetCurrentObject() {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	game.ClearLink(kGameUsedItem, true);
}

void TGObjectManager::RemoveItem(const TVisObjRef &item) {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	if (!(game.GetLink(kGameUsedItem) == item))
		return;
	game.ClearLink(kGameUsedItem, true);
	if (!game.GetBool(kGameUsedItemPicked))
		return;
	g_pGameControl->GetCursorControl()->ReleaseMoveObject();
	game.SetValue(kGameUsedItemPicked, false, TSendEventEnum::kNoEvent);
}

void TGObjectManager::RemoveItem(bool keepIfNotHeld) {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	bool held = game.GetBool(kGameUsedItemPicked);
	if (!held && keepIfNotHeld)
		return;
	game.ClearLink(kGameUsedItem, true);
	if (!held)
		return;
	g_pGameControl->GetCursorControl()->ReleaseMoveObject();
	game.SetValue(kGameUsedItemPicked, false, TSendEventEnum::kNoEvent);
}

void TGObjectManager::SetItem(const TVisObjRef &item, bool held) {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	game.SetLink(kGameUsedItem, item, true);
	game.SetValue(kGameUsedItemPicked, held && !item.IsEmpty(), TSendEventEnum::kNoEvent);
	if (held)
		g_pGameControl->GetCursorControl()->SetMoveObject(item);
	else
		g_pGameControl->GetCursorControl()->ReleaseMoveObject();
}

void TGObjectManager::HandleEvent(TMouseEventEnum event) {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();

	TGEventInfo info;
	info.action = game.GetLink(kGameUsedItem);
	info.command = TTButton(game.GetLink(kGameActiveCommand));
	info.flag8 = game.GetBool(kGameUsedItemPicked);
	info.mouseEvent = static_cast<int>(event);
	info.character = GetEventCharacter();

	if (_currentObject)
		_currentObject->ExecuteEvent(info);
}

void TGObjectManager::ObjectReached(TManagedObject *object) {
	if (!object)
		return;

	TVisObjRef game = gameControl()->GetVisionaire()->GetGame();

	TGEventInfo info;
	info.flag8 = game.GetBool(kGameDestinationItemPicked);
	info.action = game.GetLink(kGameDestinationItem);
	info.command = TTButton(game.GetLink(kGameDestinationCommand));
	info.mouseEvent = game.GetInt(kGameDestinationEvent);
	info.character = GetEventCharacter();

	object->ExecuteEvent(info);
}

void TGObjectManager::ObjectReached(TGCharacter &character, TVisObjRef &target) {
	TGCharacter *currentCharacter = gameControl()->GetCurrentCharacter();
	if (!currentCharacter)
		return;
	if (!(currentCharacter->GetRef().GetLink(kCharacterScene) == gameControl()->GetScene()->GetRef()))
		return;
	if (!(character.GetRef() == currentCharacter->GetRef()))
		return;
	TManagedObject *object = gameControl()->GetScene()->GetObject(target);
	ObjectReached(object);
}

void TGObjectManager::SaveEventInfo(TMouseEventEnum event) {
	TVisObjRef game = gameControl()->GetVisionaire()->GetGame();
	TVisObjRef gameSystemGame = gameControl()->GetGameSystem()->GetGame();
	game.SetLink(kGameDestinationCommand, gameSystemGame.GetLink(kGameActiveCommand), true);
	game.SetLink(kGameDestinationItem, game.GetLink(kGameUsedItem), true);
	game.SetValue(kGameDestinationItemPicked, game.GetBool(kGameUsedItemPicked), TSendEventEnum::kNoEvent);
	game.SetValue(kGameDestinationEvent, static_cast<int>(event), TSendEventEnum::kNoEvent);
}

void TGObjectManager::MouseMove(TManagedObject *object) {
	if (_currentObject == object)
		return;

	// Confirmed (asm lines 188427-188451): a "mouse left" notification to
	// the object being replaced, with an empty action/command and no held
	// flag - only fired if its own game-data reference resolves to a real
	// TVisionaire (GetVisionaire() non-null).
	if (_currentObject && _currentObject->GetRef().GetVisionaire()) {
		TGEventInfo info;
		info.mouseEvent = 6;
		info.character = GetEventCharacter();
		_currentObject->ExecuteEvent(info);
	}

	if (object) {
		// Confirmed (asm lines 188500-188524): the "mouse entered"
		// counterpart, fired on the new object before it becomes
		// _currentObject.
		TGEventInfo info;
		info.mouseEvent = 5;
		info.character = GetEventCharacter();
		object->ExecuteEvent(info);

		_currentObject = object;
		if (!object->GetRef().IsEmpty())
			gameControl()->GetGameSystem()->GetGame().SetLink(kGameCurrentObject, object->GetRef(), true);
	} else {
		_currentObject = nullptr;
		gameControl()->GetGameSystem()->GetGame().ClearLink(kGameCurrentObject, true);
	}
}

void TGObjectManager::NotifyObjectRemoved(const TVisObjRef &item) {
	if (_savedObject && item == _savedObject->GetRef()) {
		_savedObject = nullptr;
		gameControl()->GetGameSystem()->GetGame().ClearLink(kGameSavedObject, true);
	}
	if (_currentObject && item == _currentObject->GetRef()) {
		_currentObject = nullptr;
		gameControl()->GetGameSystem()->GetGame().ClearLink(kGameCurrentObject, true);
	}
}

void TGObjectManager::SaveCurrentObject() {
	_savedObject = _currentObject;
	TVisObjRef game = gameControl()->GetVisionaire()->GetGame();
	if (_savedObject)
		game.SetLink(kGameSavedObject, _savedObject->GetRef(), false);
	else
		game.ClearLink(kGameSavedObject, false);
}

void TGObjectManager::ExecuteSavedObject() {
	if (_currentObject && (!_savedObject || !(_currentObject->GetRef() == _savedObject->GetRef()))) {
		// Confirmed (asm lines 188924-188980): the same "mouse left"
		// notification MouseMove() fires on the object being replaced.
		TGEventInfo info;
		info.mouseEvent = 6;
		info.character = GetEventCharacter();
		_currentObject->ExecuteEvent(info);
	}
	_currentObject = _savedObject;
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	if (!_savedObject || _savedObject->GetRef().IsEmpty())
		game.ClearLink(kGameCurrentObject, true);
	else
		game.SetLink(kGameCurrentObject, _savedObject->GetRef(), true);
	HandleEvent(TMouseEventEnum::kValue1);
}

void TGObjectManager::SavedObjectChanged() {
	TVisObjRef game = gameControl()->GetVisionaire()->GetGame();
	TVisObjRef link = game.GetLink(kGameSavedObject);
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
	return _currentObject->GetRef().GetInt(kButtonType) != 0;
}

bool TGObjectManager::IsCurrentObjectWalkable() const {
	if (!_currentObject)
		return false;
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	TTButton button(game.GetLink(kGameActiveCommand));
	if (!_currentObject->IsWalkable())
		return false;
	if (!button.IsStandardCommand())
		return false;
	return !game.GetBool(kGameUsedItemPicked);
}

TVisObjRef TGObjectManager::GetCurrentObject() const {
	if (_currentObject)
		return _currentObject->GetRef();
	return gameControl()->GetGameSystem()->GetEmptyObject();
}

void TGObjectManager::GetDetectInfo(TGDetectInfo &info) const {
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	if (game.GetLink(kGameActiveCommand).IsEmpty()) {
		info.flagA = true;
		info.flagB = false;
		return;
	}
	int type = game.GetInt(kButtonUse);
	if (type == 2) {
		info.flagA = true;
		info.flagB = false;
		return;
	}
	if (type != 0 && type != 1)
		return;
	info.flagA = (type == 0);
	info.flagB = true;
	if (!game.GetBool(kButtonUseOnCurrentCharacter))
		info.character = gameControl()->GetCurrentCharacter()->GetRef();
}

TGCharacter *TGObjectManager::GetEventCharacter() const {
	TGCharacter *character = gameControl()->GetCurrentCharacter();
	if (!character)
		return nullptr;
	if (character->GetRef().GetLink(kCharacterScene) == gameControl()->GetScene()->GetRef())
		return character;
	return nullptr;
}

TVisObjRef TGObjectManager::GetEventCommand() const {
	return gameControl()->GetGameSystem()->GetGame().GetLink(kGameActiveCommand);
}

wxString TGObjectManager::GetActionText() const {
	return wxString();
}
