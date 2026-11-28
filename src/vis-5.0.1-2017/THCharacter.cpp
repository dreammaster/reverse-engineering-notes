#include "THCharacter.h"

#include "AppGlobals.h"
#include "TGInterface.h"
#include "TGScene.h"
#include "TSceneControl.h"
#include "datastruct/visionaireobject.h"
#include "datastruct/vlist.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/fieldIds.h"

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

// Confirmed (asm lines 218783-218826)
THCharacter::THCharacter(const TVisObjRef &self, const TVisObjRef &parent) : TGCharacter(self, parent) {
	_objRef.RegisterEventHandler(this, TEventEnum::kChanged);

	TVisObjRef outfit = _objRef.GetLink(kCharacterCurrentOutfit);

	if (!outfit.IsEmpty())
		outfit.RegisterEventHandler(this, TEventEnum::kChanged);
}

// Confirmed (asm lines 217481-217545)
THCharacter::~THCharacter() {
	_objRef.UnRegisterEventHandler(this);

	TVisObjRef outfit = _objRef.GetLink(kCharacterCurrentOutfit);

	if (!outfit.IsEmpty())
		outfit.UnRegisterEventHandler(this);
}

// Confirmed (asm lines 217635-218766), except that the original reads the speed of the outfit for
// kOutfitCharacterSpeed and counts the talk animations for kOutfitTalkAnimations without using what it
// got. Which field changed is all that is given: the new value is read from the data.
void THCharacter::OnEvent(TEventEnum /*event*/, int field, TVisionaireObject *object) {
	switch (field) {
	case kCharacterState:
		if (_objRef.GetInt(kCharacterState) == 2)
			StopWalking(true);
		break;
	case kCharacterScale:
	case kCharacterScaleFactor:
		SetCurrentSize();
		break;
	case kCharacterDestVisibility:
		SetDestAlpha(_objRef.GetInt(kCharacterDestVisibility), _objRef.GetInt(kCharacterTimeToDestVisibility));
		break;
	case kCharacterVisibility:
		SetDestAlpha(_objRef.GetInt(kCharacterVisibility), 0);
		break;
	case kCharacterDirection:
		SetCurrentDirection(_objRef.GetInt(kCharacterDirection));
		break;
	case kCharacterItems:
		// the items of the character that is played are shown in its interfaces
		if (gameControl()->GetCurrentCharacterPointer() == this) {
			TVList items;

			_objRef.GetLinks(kCharacterItems, TypeOrder::kValue0, items);

			for (TGInterface *interface : _interfaces)
				interface->UpdateItems(items);
		}
		break;
	case kCharacterCurrentCommentSet:
		SetCurrentCommentSet(_objRef.GetLink(kCharacterCurrentCommentSet));
		break;
	case kCharacterFollowReachDistance:
		_followReach = _objRef.GetInt(kCharacterFollowReachDistance);
		break;
	case kCharacterActiveCommand: {
		// the game runs the command that the character is to do
		TVisObjRef command = _objRef.GetLink(kCharacterActiveCommand);
		TVisObjRef game = _objRef.GetVisionaire()->GetGame();

		game.SetLink(kGameActiveCommand, command, true);
		break;
	}
	case kOutfitCharacterSpeed: {
		// the steps that were measured for the old speed are not right any more: they are measured again
		TVisObjRef outfit = _objRef.GetLink(kCharacterCurrentOutfit);
		TVList animations;

		outfit.GetLinks(kOutfitWalkAnimations, TypeOrder::kValue0, animations);

		for (TVList::iterator it = animations.begin(); it != animations.end(); ++it) {
			TVisObjRef animation(**it);
			std::vector<float> steps;

			animation.GetFloats(kAnimationWalkSteps, steps);

			if (!steps.empty()) {
				steps.clear();
				animation.SetValue(kAnimationWalkSteps, steps, TSendEventEnum::kNoEvent);
			}
		}
		break;
	}
	case kOutfitTalkAnimations: {
		TVisObjRef outfit = _objRef.GetLink(kCharacterCurrentOutfit);
		TVList animations;

		outfit.GetLinks(kOutfitTalkAnimations, TypeOrder::kValue1, animations);
		break;
	}
	case kCharacterWalkingSound:
		SetCurrentWalkingSound(_objRef.GetPath(kCharacterWalkingSound));
		break;
	case kCharacterCurrentOutfit: {
		// the old outfit is not listened to any more, the new one is
		if (object && !object->IsEmpty())
			object->UnRegisterEventHandler(this);

		TVisObjRef outfit = _objRef.GetLink(kCharacterCurrentOutfit);

		if (!outfit.IsEmpty())
			outfit.RegisterEventHandler(this, TEventEnum::kChanged);

		SetCurrentOutfit(_objRef.GetLink(kCharacterCurrentOutfit));
		break;
	}
	case kCharacterActive:
		SetActive(_objRef.GetBool(kCharacterActive));
		break;
	case kCharacterPosition:
		_position = *_objRef.GetPoint(kCharacterPosition);
		_realPosition.x = (float)_position.x;
		_realPosition.y = (float)_position.y;
		StopStandingAnim();
		StopWalking(true);
		SetCurrentSize();
		gameControl()->ScrollToCharacterIfNeeded(_objRef);
		break;
	case kCharacterDestination:
		CheckCharacterPosition();
		SetFreeDestination(*_objRef.GetPoint(kCharacterDestination), false, false, false);
		break;
	case kCharacterDestinationObject: {
		// the character walks to an object (its position, and where it is to stand: the offset)
		TVisObjRef destination = _objRef.GetLink(kCharacterDestinationObject);

		if (!destination.IsEmpty()) {
			CheckCharacterPosition();

			const wxPoint &offset = *destination.GetPoint(kObjectOffset);
			wxPoint point = *destination.GetPoint(kObjectPosition) + offset;

			_objRef.SetValue(kCharacterDestination, point, TSendEventEnum::kNoEvent);
			SetFreeDestination(point, true, false, false);
		}
		break;
	}
	case kCharacterInterfaces:
		// the interfaces of the character that is played are made again
		if (gameControl()->GetCurrentCharacter()->GetRef() == _objRef) {
			SetInterfaces();
			gameControl()->SetInterfaces();

			TVisObjRef command = _objRef.GetLink(kCharacterActiveCommand);
			bool ownCommand = false;

			if (!command.IsEmpty()) {
				// (a command of one of the interfaces is not to be run again)
				TVisObjRef parent = command.GetParent();

				for (TGInterface *interface : _interfaces) {
					if (interface->GetRef() == parent) {
						ownCommand = true;
						break;
					}
				}
			}

			if (!ownCommand) {
				// the commands of the interfaces are set again, with the event
				for (TGInterface *interface : _interfaces) {
					TVisObjRef interfaceCommand = interface->GetRef().GetLink(kInterfaceActiveCommand);

					if (interfaceCommand.IsEmpty())
						continue;

					interface->GetRef().ClearLink(kInterfaceActiveCommand, false);
					interface->GetRef().SetLink(kInterfaceActiveCommand, interfaceCommand, true);
				}
			}

			gameControl()->AdjustInterfacesOnScreen(false, nullptr);
		}
		break;
	case kCharacterScene: {
		// the character goes to another scene. The data has the new one already: it is put back, because
		// the scene (and the one it leaves) are changed by the game, which sets it.
		TVisObjRef scene = _objRef.GetLink(kCharacterScene);

		if (object)
			_objRef.SetLink(kCharacterScene, TVisObjRef(*object), false);
		else
			_objRef.SetLink(kCharacterScene, TVisObjRef(), false);

		// it stands where it is, or at the first object of the new scene
		wxPoint position = _position;
		TVList objects;

		scene.GetList(kSceneObjects, objects);

		if (objects.size() != 0 && !objects.front()->IsEmpty())
			position = *objects.front()->GetPoint(kObjectPosition);

		TVisObjRef game = _objRef.GetVisionaire()->GetGame();
		bool current = (_objRef == game.GetLink(kGameCurrentCharacter));
		int direction = _objRef.GetInt(kCharacterDirection);

		if (current)
			gameControl()->GetSceneControl()->ChangeScene(_objRef, scene, true, position, direction);
		else
			gameControl()->GetScene()->SetCharacter(_objRef, scene, position, direction);
		break;
	}
	case kCharacterFollowCharacter: {
		TVisObjRef follow = _objRef.GetLink(kCharacterFollowCharacter);

		if (follow.IsEmpty()) {
			StopWalking(true);
			break;
		}

		_objRef.ClearLink(kCharacterActionCharacter, false);

		// in the same scene it walks to where the other one is
		if (_objRef.GetLink(kCharacterScene) == follow.GetLink(kCharacterScene))
			_objRef.SetValue(kCharacterDestination, *follow.GetPoint(kCharacterPosition), TSendEventEnum::kSendEvent);

		_followTimer.SetTime();
		break;
	}
	default:
		break;
	}
}
