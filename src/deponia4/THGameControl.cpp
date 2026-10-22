#include "THGameControl.h"

#include "vsplayer/control/cursorControl.h"
#include "AppGlobals.h"
#include "TTText.h"
#include "datastruct/visionaire.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 258028-258049)
THGameControl::THGameControl() {
}

// Confirmed (asm lines 257854-257939): unregisters from the game object (if
// there still is one), then the TGameControl destructor runs.
THGameControl::~THGameControl() {
	TVisObjRef game = _visionaire->GetGame();

	if (!game.IsEmpty())
		game.UnRegisterEventHandler(this);
}

// Confirmed (asm lines 258050-258120)
void THGameControl::RegisterEventHandler() {
	TVisObjRef game = _visionaire->GetGame();

	if (!game.IsEmpty())
		game.RegisterEventHandler(this, TEventEnum::kChanged);
}

// Confirmed (asm lines 256950-257836): a change of one of the game object's
// fields. The settings that something running has to be told about are handled
// here (the others are read when they are used).
void THGameControl::OnEvent(TEventEnum /*event*/, int field, TVisionaireObject * /*object*/) {
	TVisObjRef game = _visionaire->GetGame();

	switch (field) {
	case kGameScrollSpeed:
		_timingValueSeconds = (float)game.GetInt(kGameScrollSpeed) / 1000.0f;
		break;

	case kGameQuake:
	case kGameQuakeForce:
	case kGameQuakeSpeed:
		if (game.GetBool(kGameQuake))
			StartEarthquake(game.GetInt(kGameQuakeForce), game.GetInt(kGameQuakeSpeed));
		else
			StopEarthquake();
		break;

	case kGameCurrentCharacter:
		ChangeCharacter(game.GetLink(kGameCurrentCharacter), true, TVisObjRef());
		break;

	case kGameCurrentScene: {
		TVisObjRef scene = game.GetLink(kGameCurrentScene);

		_sceneControl->ShowScene(scene, true, false);
		break;
	}

	case kGameScrollPosition: {
		const wxPoint *position = game.GetPoint(kGameScrollPosition);

		_sceneControl->GetScene()->AdjustWindowHorizontal((float)position->x);
		_sceneControl->GetScene()->AdjustWindowVertical((float)position->y);
		break;
	}

	case kGameScrollToPoint:
		game.SetValue(kGameScrollTo, true, TSendEventEnum::kNoEvent);
		GetMainControl()->SetIsScrollable(false);
		break;

	case kGameCurrentText:
	case kGameHideCursor:
		// (the cursor is hidden while a text is shown, unless the settings hide it)
		if (!GetSceneControl()->FadingToNewScene()) {
			TVisObjRef text = game.GetLink(kGameCurrentText);

			if (text.IsEmpty())
				GetCursorControl()->SetActive(!game.GetBool(kGameHideCursor));
			else
				GetCursorControl()->SetActive(false);
		}
		break;

	case kGameSavedObject:
		GetObjectManager()->SavedObjectChanged();
		break;

	case kGameStandardLanguage:
		TTText::SetLanguage(game.GetLink(kGameStandardLanguage));
		break;

	case kGameSpeechLanguage:
		TTText::SetSpeechLanguage(game.GetLink(kGameSpeechLanguage));
		break;

	case kGameHoldTime:
		GameMinDownTime = game.GetInt(kGameHoldTime);
		break;

	case kGameHideInterfaces:
		AdjustInterfacesOnScreen(false, nullptr);
		break;

	case kGameScrollCharacter: {
		TVisObjRef character = game.GetLink(kGameScrollCharacter);

		_previousCharacter = GetCharacterPointer(character);
		ScrollToCharacterIfNeeded(character);
		break;
	}

	case kGameActiveCommand: {
		TVisObjRef command = game.GetLink(kGameActiveCommand);

		if (!command.IsEmpty()) {
			TVisObjRef parentInterface = command.GetParent();

			parentInterface.SetLink(kInterfaceActiveCommand, command, true);
			GetCursorControl()->SetCursor(command.GetObjectPointer()->GetId24(), false);
		}

		// a command that is held up (dragged) keeps the event info
		if (!game.GetBool(kGameUsedItemPicked) || !command.GetBool(kButtonDraggable))
			GetObjectManager()->ResetEventInfo();
		break;
	}

	case kGameUsedItem:
	case kGameUsedItemPicked:
		GetObjectManager()->SetItem(game.GetLink(kGameUsedItem), game.GetBool(kGameUsedItemPicked));
		break;

	default:
		break;
	}
}
