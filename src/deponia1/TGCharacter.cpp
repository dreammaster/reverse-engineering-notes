#include "TGCharacter.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "AppGlobals.h"
#include "TGAction.h"
#include "TGInterface.h"
#include "TGObjectManager.h"
#include "TGScene.h"
#include "TSceneActionArea.h"
#include "TSoundFFMPEG.h"
#include "TSprite.h"
#include "TTText.h"
#include "datastruct/visionaire.h"
#include "datastruct/vlist.h"
#include "graphicslib/picture.h"
#include "vsplayer/animationGame.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

// Field ids that are shared by several record types, named here for what they are
// in a comment set (see fieldIds.h for kButtonType/kButtonGroup).
static const int kCommentSetKind = 0x129;      // 5 and 6: the set holds the comments of a list of commands
static const int kCommentSetCommands = 0x248;  // the commands such a set is for

// The action execution types of an area action (see TGActionInfo.h): 29 when the
// character enters the area, 30 when it leaves it.
static const int kExecuteEnterArea = 0x1D;
static const int kExecuteLeaveArea = 0x1E;

wxPoint failedPoint;

wxString TGCharacter::HookFunctionGetCharacterAnimationIndex;

// Confirmed (asm lines 176344-176365): the smallest of the three ways round the circle.
int diff(int first, int second) {
	int delta = first - second;
	int best = std::abs(delta);

	best = std::min(best, std::abs(delta - 360));
	best = std::min(best, std::abs(delta + 360));
	return best;
}

// Confirmed (asm lines 176320-176332).
int mirror(int direction) {
	int mirrored = 180 - direction;

	if (mirrored < 0)
		mirrored = 540 - direction;
	return mirrored;
}

// Confirmed (asm lines 178316-178323).
void TGCharacter::RegisterHookFunctionGetCharacterAnimationIndex(const wxString &name) {
	HookFunctionGetCharacterAnimationIndex = name;
}

// Confirmed (asm lines 178993-179152)
TGCharacter::TGCharacter(const TVisObjRef &ref, const TVisObjRef &scene) : TMCharacter(ref) {
	_state = 2;
	_previousState = 2;
	_directionIndex = -1;
	_hasWalkingSound = false;
	_walkingSoundPlaying = false;
	_walkFrame = 0;
	_allowUnloadingOutfitAnimations = true;
	_randomTime = 0;
	_followReach = 0;
	_hasWaySystem = false;
	_lastWalkIndex = 0;
	_lastWalkIndex2 = 0;
	_harmonizeWalk = false;

	_objRef.SetValue(kCharacterState, _state, TSendEventEnum::kNoEvent);
	_objRef.SetLink(kCharacterScene, scene, false);
	_objRef.SetValue(kCharacterAnimState, 0, TSendEventEnum::kNoEvent);

	// the kind of the animation shown (the first one: there is one in this version)
	_animKinds.push_back(TCharacterAnimEnum::kNone);
}

// Confirmed (asm lines 181953-182175)
TGCharacter::~TGCharacter() {
	_currentAnimation = nullptr;
	StopCharacterAnim(TCharacterAnimEnum::kNone, false);
	ClearSprites();
	UnloadAnimations();

	_interfaces.clear();

	for (SCommentSetEntries *entries : _commentSets)
		delete entries;
	_commentSets.clear();
	_activeAreas.clear();
}

// Confirmed (asm lines 175318-175327)
void TGCharacter::SetAlpha() {
	_objRef.SetValue(kCharacterVisibility, (int)(100.0f * _alpha), TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 175338-175363)
void TGCharacter::Save() {
	_objRef.SetValue(kCharacterFollowReachDistance, _followReach, TSendEventEnum::kNoEvent);
	_objRef.SetValue(kCharacterPosition, _position, TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 175374-175550). TODO (low priority, see /TODO.md): the rectangle
// of a model animation or of a Spine skeleton is not reconstructed.
wxRect TGCharacter::GetCurrentSpriteRect() const {
	if (_currentAnimation) {
		TPictureIO *sprite = _currentAnimation->GetCurrentSprite();

		if (sprite)
			return sprite->GetDestRect();
	}

	if (_picture)
		return _picture->GetDestRect();

	return wxRect();
}

// Confirmed (asm lines 175561-175566)
void TGCharacter::ExecuteEvent(TGEventInfo &info) {
	TManagedObject::ExecuteEvent(info);
}

// Confirmed (asm lines 175577-175617): turns another character to face this one.
void TGCharacter::AlignCharacter(TVisObjRef &character) const {
	if (_objRef == character)
		return;

	wxPoint delta = _position - *character.GetPoint(kCharacterPosition);

	character.SetValue(kCharacterDirection, GetAngle((float)delta.x, (float)delta.y), TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 175626-175731). TODO (low priority, see /TODO.md): the
// original moves its animation sprite through a matrix (kCharacterMatrixId) here.
void TGCharacter::Draw() {
	SetSpritePosition();

	// a character that is not shown through a matrix is drawn without them
	bool savedMatrices = false;

	if (_objRef.GetInt(kCharacterMatrixId) == 0) {
		savedMatrices = matricesActive;
		matricesActive = false;
	}

	int tint = _objRef.GetInt(kCharacterTint);

	if (tint & 0xFF000000) {
		if (_objRef.GetBool(kCharacterLightmapActive))
			_color = gameControl()->GetScene()->GetTint(_objRef, _position);
		else
			_color = 0xFFFFFFFF;
	} else {
		_color = tint;
	}

	TManagedObject::Draw();

	if (_objRef.GetInt(kCharacterMatrixId) == 0)
		matricesActive = savedMatrices;
}

// Confirmed (asm lines 175742-175880): the item is taken from the character that
// brings it, when this character has an interface to show it in.
bool TGCharacter::ReceiveItem(TGEventInfo &info) {
	if (_objRef == info.character->GetRef())
		return false;

	TVList interfaces;

	_objRef.GetLinks(kCharacterInterfaces, TypeOrder::kValue0, interfaces);

	if (interfaces.empty())
		return false;

	if (_objRef.IsEmpty() || info.action.IsEmpty())
		return false;

	TTCharacter giver(info.character->GetRef());
	TTCharacter receiver(_objRef);

	giver.RemoveItem(info.action);
	receiver.AddItem(info.action, false);
	return true;
}

// Confirmed (asm lines 175893-175947): sets the way system's number of size lines
// and works out the character's size at its position.
void TGCharacter::InitWaySystem(int spriteHeight) {
	_waySystem.SetNoLines(spriteHeight);
	SetCurrentSize();
}

// Confirmed (asm lines 175958-175980)
void TGCharacter::AlignToObject(const TVisObjRef &object) {
	int direction = object.GetInt(kObjectDirection);

	if (direction != -1)
		_objRef.SetValue(kCharacterDirection, direction, TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 175991-176011)
void TGCharacter::CheckCharacterPosition() {
	_position = _waySystem.CheckPosition(_position);
}

// Confirmed (asm lines 176022-176029)
void TGCharacter::AllowUnloadingOutfitAnimations(bool allow) {
	_allowUnloadingOutfitAnimations = allow;
}

// Confirmed (asm lines 176040-176132): keeps the pictures of the outfit's walk, talk
// and standing animations loaded.
void TGCharacter::PreloadAnimations() {
	TVList *lists[3] = {&_walkAnimations, &_talkAnimations, &_standAnimations};

	for (TVList *list : lists) {
		for (size_t i = 0; i < list->size(); i++) {
			TVisObjRef animation(*list->at((int)i));

			TGAnimation::PreloadAnimation(animation);
		}
	}
}

// Confirmed (asm lines 176164-176261): the state is zeroed meanwhile (the animations
// that are unloaded stop, which must not make the character start others).
void TGCharacter::UnloadAnimations() {
	int savedState = _state;

	_state = 0;

	TVList *lists[3] = {&_walkAnimations, &_talkAnimations, &_standAnimations};

	for (TVList *list : lists) {
		for (size_t i = 0; i < list->size(); i++) {
			TVisObjRef animation(*list->at((int)i));

			TGAnimation::UnloadAnimation(animation);
		}
	}

	_state = savedState;
}

// Confirmed (asm lines 176293-176308)
bool TGCharacter::IsSpineAnimation() const {
	return _currentAnimation && _currentAnimation->IsBonesAnimation();
}

// Confirmed (asm lines 176376-176440)
bool TGCharacter::GiveItemTo(TVisObjRef &receiver, const TVisObjRef &item) {
	if (receiver.IsEmpty() || item.IsEmpty())
		return false;

	TTCharacter giver(_objRef);
	TTCharacter taker(receiver);

	giver.RemoveItem(item);
	taker.AddItem(item, false);
	return true;
}

// Confirmed (asm lines 176467-176529)
void TGCharacter::GiveAllItemsTo(TVisObjRef &receiver) {
	if (receiver.IsEmpty())
		return;

	TTCharacter taker(receiver);
	TVList items;

	_objRef.GetList(kCharacterItems, items);
	taker.AddItems(items);

	items.clear();
	_objRef.SetValue(kCharacterItems, items, true);
}

// Confirmed (asm lines 176555-176666): the character says one of the texts of the
// comment set for the command (the first set when none is for it).
void TGCharacter::ShowComment(const TVisObjRef &command) {
	if (_commentSets.empty())
		return;

	SCommentSetEntries *chosen = nullptr;

	for (SCommentSetEntries *entries : _commentSets) {
		if (entries->set == command) {
			chosen = entries;
			break;
		}

		// a set of the kinds 5 and 6 is for a list of commands
		int kind = entries->set.GetInt(kCommentSetKind);

		if (kind == 5 || kind == 6) {
			TVList commands;
			bool found = false;

			entries->set.GetList(kCommentSetCommands, commands);

			for (TVisionaireObject *item : commands) {
				if (command == *item) {
					found = true;
					break;
				}
			}

			if (found) {
				chosen = entries;
				break;
			}
		}
	}

	if (!chosen)
		chosen = _commentSets.front();

	int count = (int)chosen->entries.size();

	if (count <= 0)
		return;

	TVisObjRef text(chosen->entries.at(std::rand() % count));
	int alignment = gameControl()->GetGameSystem()->GetGame().GetInt(kGameTextAlignment);

	gameControl()->StartText(text, this, (TextAlignmentEnum)alignment, TVisObjRef(), wxPoint());
}

// Confirmed (asm lines 176778-176827): the size (percent) of the character at its
// position.
void TGCharacter::SetCurrentSize() {
	float size = 100.0f;

	if (_objRef.GetBool(kCharacterScale)) {
		size = _waySystem.GetCalculatedSize(_position);

		int factor = _objRef.GetInt(kCharacterScaleFactor);

		if (factor != 100)
			size = size * (float)factor / 100.0f;
	}

	_objRef.SetValue(kCharacterSize, size, TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 176838-176905)
wxSize TGCharacter::GetCurrentDimension() const {
	const TSprite *sprite = nullptr;

	if (_currentAnimation && _currentAnimation->IsSpriteIndexValid())
		sprite = _currentAnimation->GetCurrentSprite();

	if (!sprite)
		sprite = _picture;

	wxSize size;

	if (sprite) {
		size.width = (int)sprite->GetSizedWidth();
		size.height = (int)sprite->GetSizedHeight();
	}
	return size;
}

// Confirmed (asm lines 176915-176933)
int TGCharacter::GetStandingDirection() const {
	if (_directionIndex < 0 || _directionIndex >= (int)_standDirections.size())
		return -1;

	return _standDirections[_directionIndex];
}

// Confirmed (asm lines 176944-176962)
TTPoint TGCharacter::GetPointNextTo(wxPoint &position) {
	return TTPoint(_waySystem.GetPointNextTo(position));
}

// Confirmed (asm lines 176982-177005)
bool TGCharacter::StandingAt(const wxPoint &position) const {
	if (_state == 0)
		return true;
	if (_state == 2)
		return _position == position;
	return false;
}

// Confirmed (asm lines 177015-177021): walking or playing an animation, not standing.
bool TGCharacter::IsWalking() const {
	return (_state & ~2) != 0;
}

// Confirmed (asm lines 177202-177251). TODO (low priority, see /TODO.md): the
// original also positions model and Spine animations here.
void TGCharacter::SetSpritePosition() {
	_picture = nullptr;

	float size = _objRef.GetFloat(kCharacterSize);

	if (_currentAnimation && _currentAnimation->IsSpriteIndexValid())
		_currentAnimation->SetPosition(_position, size);
}

// Confirmed (asm lines 177033-177190): in the shown scene the animation gets its
// position (and the scene becomes the one painted to).
void TGCharacter::UpdateSpriteRect() {
	TVisObjRef shownScene = gameControl()->GetVisionaire()->GetGame().GetLink(kGameCurrentScene);

	if (!(_objRef.GetLink(kCharacterScene) == shownScene))
		return;

	gameControl()->GetScene()->SetCurrent();
	SetSpritePosition();
}

// Confirmed (asm lines 177262-177267)
bool TGCharacter::IsWalkingToWayPoint() const {
	return _waySystem.IsWalkingToWayPoint();
}

// Confirmed (asm lines 177278-177304)
wxFileName TGCharacter::GetWalkingSound() const {
	wxFileName sound = _walkingSound;

	sound.NormalizePath();
	return sound;
}

// Confirmed (asm lines 177329-177336)
bool TGCharacter::IsWalkingSoundPlaying() const {
	return _walkingSoundPlaying;
}

// Confirmed (asm lines 177347-177570): while the character walks in the scene that
// is shown, its walking sound is played, looping, with the volume of its size and
// panned to where it is on the screen.
void TGCharacter::CheckWalkingSound() {
	if (_state != 3 || !_hasWalkingSound || _walkingSoundPlaying)
		return;

	TVisObjRef shownScene = gameControl()->GetVisionaire()->GetGame().GetLink(kGameCurrentScene);

	if (!(_objRef.GetLink(kCharacterScene) == shownScene))
		return;

	TSoundFFMPEG *sounds = g_pGameControl->GetSoundManager();

	if (!sounds)
		return;

	int scrollX = gameControl()->GetScene()->GetScrollPos().x;
	int width = 0, height = 0;

	g_pGameControl->GetWindowSize(&width, &height);

	int x = _position.x - scrollX;

	if (x < 0)
		x = 0;
	else
		x = std::min(x, width);

	int pan = (int)(((double)((float)x / (float)width)) * 200.0 - 100.0);

	float size = _objRef.GetFloat(kCharacterSize);
	int volume = 100;

	if (!(size > 100.0f))
		volume = (0.0f > size) ? 0 : (int)size;

	sounds->Play(_walkingSound, volume, pan, true, TSoundTypeEnum::kSound2, true, 0);
	_walkingSoundPlaying = true;
}

// Confirmed (asm lines 177570-177619)
void TGCharacter::SetCurrentWalkingSound(const wxFileName &file) {
	if (_walkingSoundPlaying) {
		TSoundFFMPEG *sounds = g_pGameControl->GetSoundManager();

		if (sounds)
			sounds->Stop(_walkingSound);
		_walkingSoundPlaying = false;
	}

	_walkingSound = file;
	_hasWalkingSound = _walkingSound.IsOk();
	CheckWalkingSound();
}

// Confirmed (asm lines 177629-177656)
void TGCharacter::StopWalkingSound() {
	if (!_walkingSoundPlaying)
		return;

	TSoundFFMPEG *sounds = g_pGameControl->GetSoundManager();

	if (sounds)
		sounds->Stop(_walkingSound);
	_walkingSoundPlaying = false;
}

// Confirmed (asm lines 177667-177679)
bool TGCharacter::IsCharacterAnimRunning() const {
	return _objRef.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kCharacterAnim;
}

// Confirmed (asm lines 177690-177719)
TVisObjRef TGCharacter::GetCharacterAnim() {
	TVisObjRef anim;

	if (_currentAnimation)
		anim = _currentAnimation->GetState();
	return anim;
}

// Confirmed (asm lines 177737-177742): always false.
bool TGCharacter::TurnCharacter() {
	return false;
}

// Confirmed (asm lines 177753-177807): the time until the random animation is picked
// between the minimum and maximum of the outfit (10 and 30 seconds when it has none).
void TGCharacter::SetRandomTime() {
	TVisObjRef outfit = _objRef.GetLink(kCharacterCurrentOutfit);
	int maxTime = outfit.GetInt(kOutfitRandomMaxTime);
	int minTime = outfit.GetInt(kOutfitRandomMinTime);

	if (maxTime <= 0)
		maxTime = 30000;
	if (minTime <= 0)
		minTime = 10000;

	int span = maxTime + (maxTime == minTime ? 1 : 0) - minTime;

	// (the original divides by zero for a maximum one below the minimum)
	if (span == 0)
		span = 1;

	_randomTime = minTime + std::rand() % span;
	_randomTimer.SetTime();
}

// Confirmed (asm lines 177825-178015): whether the character stands near enough to
// another one (in the same scene). `strict` is for the characters that follow one:
// then the distance depends on the sizes of both and the follow reach.
bool TGCharacter::IsReached(const TVisObjRef &target, bool strict) const {
	if (!(target.GetLink(kCharacterScene) == _objRef.GetLink(kCharacterScene)))
		return false;

	const wxPoint *targetPosition = target.GetPoint(kCharacterPosition);
	int distance = std::abs(_position.x - targetPosition->x) + std::abs(_position.y - targetPosition->y);
	int reach;

	if (!strict) {
		// a character that is to be reached at an exact place
		const wxPoint *destination = _objRef.GetPoint(kCharacterActionDestPosition);

		if (*destination != wxPoint{-1, -1})
			return *destination == *targetPosition;

		// else it is near enough when it is within one and a half sprite widths
		const TSprite *sprite = _picture;

		if (!sprite && _currentAnimation && _currentAnimation->IsSpriteIndexValid())
			sprite = _currentAnimation->GetCurrentSprite();

		reach = sprite ? (int)(sprite->GetSizedWidth() * 1.5f) : 50;
	} else {
		float followReach = (float)_followReach;
		float mySize = _objRef.GetFloat(kCharacterSize);
		float itsSize = target.GetFloat(kCharacterSize);

		reach = (int)((itsSize + mySize) * followReach / 200.0f);
	}

	if (reach < distance)
		return false;

	return !_waySystem.CheckCrossBorders(_position, _position);
}

// Confirmed (asm lines 178026-178032)
bool TGCharacter::IsReached(const TVisObjRef &target) const {
	return IsReached(target, false);
}

// Confirmed (asm lines 178043-178150): this character is to act on another one: it
// walks to it (when it is in the same scene), and the event is kept to be run on arrival.
void TGCharacter::SetActionCharacter(TGCharacter &other, TMouseEventEnum event) {
	_objRef.SetLink(kCharacterActionCharacter, other._objRef, false);
	_objRef.ClearLink(kCharacterFollowCharacter, true);

	if (other._objRef.GetLink(kCharacterScene) == _objRef.GetLink(kCharacterScene)) {
		wxPoint destination = *other._objRef.GetPoint(kCharacterActionDestPosition);

		if (destination == wxPoint{-1, -1})
			destination = other.GetPosition();

		_objRef.SetValue(kCharacterDestination, destination, TSendEventEnum::kSendEvent);
		gameControl()->GetObjectManager()->SaveEventInfo(event);
	}

	_followTimer.SetTime();
}

// Confirmed (asm lines 178188-178196)
void TGCharacter::ClickedWithoutReach(TGCharacter *character, TMouseEventEnum event) {
	character->SetActionCharacter(*this, event);
}

// Confirmed (asm lines 178207-178285): after the game was stopped (a menu) the
// timers are moved on by the time it was stopped.
void TGCharacter::AdjustTimers() {
	long stopped = TGameControl::GetStopTime().GetTime();

	_idleTimer.AdjustTimer(stopped);
	_walkTimer.AdjustTimer(stopped);
	_timer.AdjustTimer(stopped);

	if (_idleTimer.GetTime() < 0)
		_idleTimer.SetTime();
	if (_walkTimer.GetTime() < 0)
		_walkTimer.SetTime();
	if (_timer.GetTime() < 0)
		_timer.SetTime();
}

// Confirmed (asm lines 178334-178470): the interfaces the character's data lists, as
// the ones the game has.
void TGCharacter::SetInterfaces() {
	std::list<TGInterface *> all = gameControl()->GetAllInterfaces();
	TVList links;

	_objRef.GetLinks(kCharacterInterfaces, TypeOrder::kValue1, links);
	_interfaces.clear();

	for (TVisionaireObject *link : links) {
		for (TGInterface *candidate : all) {
			if (candidate->GetRef() == *link) {
				_interfaces.push_back(candidate);
				break;
			}
		}
	}
}

// Confirmed (asm lines 178480-178524)
std::list<TGInterface *> TGCharacter::GetInterfaces() const {
	return _interfaces;
}

// Runs the actions of an area (with the execution type given) that are for this character.
static void runAreaActions(TSceneActionArea &area, const TVisObjRef &character, int executionType) {
	TVList actions;

	area.GetLinks(kActionAreaActions, TypeOrder::kValue0, actions);

	for (TVisionaireObject *item : actions) {
		TVisObjRef areaAction(item);
		TVisObjRef actionCharacter = areaAction.GetLink(kAreaActionCharacter);

		// (an action for any character, or for this one)
		if (actionCharacter.IsAnyObject() || actionCharacter.IsEmpty() || actionCharacter == character) {
			TVisObjRef action = areaAction.GetLink(kAreaActionAction);

			if (action.GetInt(kActionExecutionType) == executionType)
				TGAction::AddRunningAction(action);
		}
	}
}

// Confirmed (asm lines 178540-178990): the actions of the action areas of the scene that
// the character enters or leaves by walking run.
void TGCharacter::UpdateActionAreas() {
	std::list<TSceneActionArea *> *areas = TGScene::GetActionAreas(_objRef.GetLink(kCharacterScene));

	if (!areas)
		return;

	std::list<TSceneActionArea *> inside;

	for (TSceneActionArea *area : *areas) {
		if (area->CanTrigger(_objRef) && area->IsInside(_position))
			inside.push_back(area);
	}

	// the areas it is still in stay; the ones it has left run their "leave" actions
	for (std::list<TSceneActionArea *>::iterator it = _activeAreas.begin(); it != _activeAreas.end();) {
		std::list<TSceneActionArea *>::iterator found = std::find(inside.begin(), inside.end(), *it);

		if (found != inside.end()) {
			inside.erase(found);
			++it;
		} else {
			runAreaActions(**it, _objRef, kExecuteLeaveArea);
			it = _activeAreas.erase(it);
		}
	}

	// the ones it has entered
	for (TSceneActionArea *area : inside) {
		runAreaActions(*area, _objRef, kExecuteEnterArea);
		_activeAreas.push_back(area);
	}
}

// Confirmed (asm lines 179260-179616): the comments of the set, sorted by the command
// they are for (the ones without a command go to the first, default, entry).
void TGCharacter::SetCurrentCommentSet(const TVisObjRef &commentSet) {
	TVList entries;

	commentSet.GetLinks(kCommentSetEntries, TypeOrder::kValue0, entries);

	for (SCommentSetEntries *old : _commentSets)
		delete old;
	_commentSets.clear();

	// the default
	_commentSets.push_back(new SCommentSetEntries());

	for (TVisionaireObject *item : entries) {
		TVisObjRef entry(item);
		TVisObjRef command = entry.GetLink(kCommentSetEntryCommand);
		TTText text(entry.GetLink(kCommentSetEntryText));
		bool found = false;

		for (SCommentSetEntries *set : _commentSets) {
			if (set->set == command) {
				set->entries.push_back(text);
				found = true;
				break;
			}
		}

		if (!found) {
			SCommentSetEntries *set = new SCommentSetEntries();

			set->set = command;
			set->entries.push_back(text);
			_commentSets.push_back(set);
		}
	}
}

// The nearest-direction search of GetDirectionIndex(): the index of the first
// animation of the group that has `variants` further animations in it (the animation
// of a variant is chosen by the character's animation index), or of the group's start.
static int groupIndex(int start, int size, int variants) {
	return (size > variants) ? start + variants : start;
}

// How many of the values from `index` on are equal to the one at `index`.
static int countEqualUp(const std::vector<int> &values, int index) {
	int count = 0;

	for (int i = index; i < (int)values.size() && values[i] == values[index]; i++)
		count++;
	return count;
}

// How many of the values from `index` down are equal to the one at `index`.
static int countEqualDown(const std::vector<int> &values, int index) {
	int count = 0;

	for (int i = index; i >= 0 && values[i] == values[index]; i--)
		count++;
	return count;
}

// Confirmed (asm lines 179616-180316): the index of the animation (of the kind; 0 is the
// kind that is shown) whose direction is nearest to `direction`. The directions of an
// animation list are in ascending order and the same direction can be there several
// times (variants, one of which is chosen by kCharacterAnimIndex); the search wraps
// round at 360. TODO: the original first asks the Lua function registered by
// RegisterHookFunctionGetCharacterAnimationIndex() ("CharacterDirectionHook", with the
// character, the kind and the direction) and uses its result unless it is -1 - the
// same standing Lua-bridge gap as for the other hooks (see TGObjectManager.h).
int TGCharacter::GetDirectionIndex(int direction, TCharacterAnimEnum kind) {
	if (kind == TCharacterAnimEnum::kNone)
		kind = (TCharacterAnimEnum)_objRef.GetInt(kCharacterAnimState);

	const std::vector<int> *list = &_walkDirections;

	if (kind == TCharacterAnimEnum::kTalk)
		list = &_talkDirections;
	else if (kind == TCharacterAnimEnum::kStand)
		list = &_standDirections;

	const std::vector<int> &directions = *list;
	int count = (int)directions.size();
	int variants = std::max(0, _objRef.GetInt(kCharacterAnimIndex));

	if (count == 0)
		return -1;

	// the group of equal directions at the start
	if (direction < directions[0]) {
		// (below the first: the nearer of the first and, round the circle, the last)
		int toFirst = directions[0] - direction;
		int toLast = direction + 360 - directions[count - 1];

		if (toFirst >= toLast) {
			int size = countEqualDown(directions, count - 1);

			return groupIndex(count - size, size, variants);
		}
		return groupIndex(0, countEqualUp(directions, 0), variants);
	}

	// the last direction that is not above
	int below = 0;

	while (below + 1 < count && directions[below + 1] <= direction)
		below++;

	if (below + 1 < count) {
		int above = below + 1;
		int toAbove = directions[above] - direction;
		int toBelow = direction - directions[below];

		if (toAbove < toBelow)
			return groupIndex(above, countEqualUp(directions, above), variants);

		int size = countEqualDown(directions, below);

		return groupIndex(below - size + 1, size, variants);
	}

	// above the last: the nearer of the last and, round the circle, the first
	int toLast = direction - directions[below];
	int toFirst = directions[0] + 360 - direction;

	if (toLast >= toFirst)
		return groupIndex(0, countEqualUp(directions, 0), variants);

	if (count == 1 || directions[count - 2] != directions[count - 1])
		return count - 1;

	// (the original compares with one less than the size of this group: the last
	// variant of it is never chosen)
	int size = countEqualDown(directions, count - 1);
	int start = count - size;

	return (variants < size - 1) ? start + variants : start;
}

// Confirmed (asm lines 180324-180463): the walk animation for the direction it is walking
// in. With harmonized walk animations a change only starts after the direction was the
// same for three updates.
void TGCharacter::StartWalkAnim() {
	int index = GetDirectionIndex(_objRef.GetInt(kCharacterDirection), TCharacterAnimEnum::kWalk);
	bool start = true;

	if (_harmonizeWalk)
		start = (_lastWalkIndex2 == _lastWalkIndex && index == _lastWalkIndex);

	if (start && index >= 0 && index < (int)_walkAnimations.size()) {
		TVisObjRef animation(_walkAnimations.at(index));

		if (!_currentAnimation || std::memcmp(animation.GetId(), _currentAnimation->GetDataObject().GetId(), 4) != 0) {
			StopCharacterAnim(TCharacterAnimEnum::kNone, false);
			StartCharacterAnim(animation, TCharacterAnimEnum::kWalk, false);
			_walkFrame = 0;
		}
	}

	_lastWalkIndex2 = _lastWalkIndex;
	_lastWalkIndex = index;
}

// Confirmed (asm lines 180474-180596): the walk is over where the character is.
void TGCharacter::StopWalking(bool restartAnim) {
	_state = 2;
	_objRef.SetValue(kCharacterState, 2, TSendEventEnum::kNoEvent);
	_objRef.SetValue(kCharacterDestination, *_objRef.GetPoint(kCharacterPosition), TSendEventEnum::kNoEvent);
	_objRef.ClearLink(kCharacterDestinationObject, false);

	if (_objRef.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kWalk || IsSpineAnimation())
		StopCharacterAnim(TCharacterAnimEnum::kWalk, true);

	_harmonizeWalk = false;
	_walkFrame = 0;

	StopWalkingSound();

	_objRef.ClearLink(kCharacterActionCharacter, false);

	if (restartAnim) {
		if (_state == 2) {
			if (gameControl()->IsTalking(_objRef))
				StartTalkAnim();
			else
				StartStandingAnim();
		} else if (_state == 3) {
			StartWalkAnim();
		}
	}
}

// Confirmed (asm lines 180607-181012), without the branches for the characters shown
// as Spine skeletons (see TODO.md). Starts the animation `data` as one of `kind` and
// makes it the one shown; `flag` plays it backwards. Not in the scene that is shown,
// only the animations a script starts play.
TGAnimation *TGCharacter::StartCharacterAnim(TVisObjRef &data, TCharacterAnimEnum kind, bool flag) {
	TVisObjRef shownScene = gameControl()->GetVisionaire()->GetGame().GetLink(kGameCurrentScene);
	bool inShownScene = (_objRef.GetLink(kCharacterScene) == shownScene);

	if (!inShownScene && kind != TCharacterAnimEnum::kCharacterAnim)
		return nullptr;

	TCharacterAnimEnum previousKind = _animKinds[0];

	if (_objRef.GetInt(kCharacterAnimState) != 0)
		StopCharacterAnim(TCharacterAnimEnum::kNone, false);

	if (_currentAnimation) {
		TGAnimation::HideAnimation(_currentAnimation, this);
		_currentAnimation = nullptr;
	}

	data.SetValue(kAnimationPosition, _position, TSendEventEnum::kSendEvent);

	// a walk animation that follows another one goes on from its frame
	int frame = -1;

	if (kind == TCharacterAnimEnum::kWalk && previousKind == TCharacterAnimEnum::kWalk &&
	        _objRef.GetBool(kCharacterHarmonizeWalkAnimations))
		frame = _walkFrame;

	TGAnimation *animation = TGAnimation::StartAnimation(data, this, flag, _objRef.GetFloat(kCharacterSize), frame);

	UpdateSpriteRect();

	// (a state other than standing, walking and playing an animation: back to standing)
	if (_state < 2 || _state > 4)
		StopWalking(false);

	if (animation)
		_objRef.SetValue(kCharacterAnimState, (int)kind, TSendEventEnum::kNoEvent);

	_animKinds[0] = kind;
	_currentAnimation = animation;
	return animation;
}

// Confirmed (asm lines 181022-181140): in its scene the standing character shows the
// standing animation for the direction it looks to.
void TGCharacter::StartStandingAnim() {
	if (_state != 2)
		return;

	TVisObjRef shownScene = gameControl()->GetVisionaire()->GetGame().GetLink(kGameCurrentScene);

	if (!(_objRef.GetLink(kCharacterScene) == shownScene))
		return;

	int index = GetDirectionIndex(_objRef.GetInt(kCharacterDirection), TCharacterAnimEnum::kStand);

	_directionIndex = index;

	if (index < 0 || index >= (int)_standAnimations.size())
		return;

	TVisObjRef animation(_standAnimations.at(index));

	StartCharacterAnim(animation, TCharacterAnimEnum::kStand, false);
}

// Confirmed (asm lines 181222-181494), without the branches for the characters shown
// as Spine skeletons (see TODO.md): ends the animation that is shown; with `restart` a
// fitting one for the state starts (the standing animation is not restarted by it).
void TGCharacter::StopCharacterAnim(TCharacterAnimEnum /*kind*/, bool restart) {
	_randomTimer.SetTime();

	if (_currentAnimation && !_currentAnimation->GetState().IsEmpty()) {
		TGAnimation *animation = _currentAnimation;

		_currentAnimation = nullptr;
		TGAnimation::HideAnimation(animation, this);
	}

	int previous = _objRef.GetInt(kCharacterAnimState);

	_objRef.SetValue(kCharacterAnimState, 0, TSendEventEnum::kNoEvent);

	if (previous == (int)TCharacterAnimEnum::kStand || !restart)
		return;

	if (_state == 2)
		StartStandingAnim();
	else if (_state == 3)
		StartWalkAnim();
}

// Confirmed (asm lines 181501-181534)
void TGCharacter::StopTalkAnim() {
	if (_objRef.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kTalk || IsSpineAnimation())
		StopCharacterAnim(TCharacterAnimEnum::kTalk, true);
}

// Confirmed (asm lines 181544-181577)
void TGCharacter::StopWalkAnim() {
	if (_objRef.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kWalk || IsSpineAnimation())
		StopCharacterAnim(TCharacterAnimEnum::kWalk, true);

	_harmonizeWalk = false;
	_walkFrame = 0;
}

// Confirmed (asm lines 181912-181942)
void TGCharacter::StopStandingAnim() {
	if (_objRef.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kStand || IsSpineAnimation())
		StopCharacterAnim(TCharacterAnimEnum::kNone, true);
}

// Confirmed (asm lines 181588-181725): an animation has ended. The one that is shown is
// replaced by a fitting one; after a turn (state 4) the character goes on with what it
// did before.
void TGCharacter::AnimationStopped(TGAnimation *animation) {
	if (gameControl()->IsClearingAnimations() || !animation) {
		if (_currentAnimation == animation)
			_currentAnimation = nullptr;
	} else if (_currentAnimation && animation == _currentAnimation) {
		_currentAnimation = nullptr;
		StopCharacterAnim(_animKinds[0], true);
	}

	// (the animations shown besides the first one, each with its kind)
	for (size_t i = 0; i < _animations.size(); i++) {
		if (_animations[i] == animation) {
			_animations.erase(_animations.begin() + i);

			if (i + 1 < _animKinds.size())
				_animKinds.erase(_animKinds.begin() + i + 1);
			break;
		}
	}

	if (_state == 4) {
		_idleTimer.SetTime();

		if (_previousState == 3) {
			_walkTimer.SetTime();
			_state = 3;
			_objRef.SetValue(kCharacterState, 3, TSendEventEnum::kNoEvent);
			StartWalkAnim();
			CheckWalkingSound();
		}

		_previousState = 2;
	}
}

// Confirmed (asm lines 181879-181901): forgets the outfit's animations (turn animations
// stay).
void TGCharacter::ClearSprites() {
	StopCharacterAnim(TCharacterAnimEnum::kNone, false);

	_walkAnimations.clear();
	_talkAnimations.clear();
	_talkDirections.clear();
	_standAnimations.clear();
	_standDirections.clear();
	_picture = nullptr;
	_walkDirections.clear();
}

// Confirmed (asm lines 182343-182404)
void TGCharacter::StartTalkAnim() {
	if (_objRef.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kTalk)
		return;

	int index = GetDirectionIndex(_objRef.GetInt(kCharacterDirection), TCharacterAnimEnum::kTalk);

	if (index < 0 || index >= (int)_talkAnimations.size())
		return;

	TVisObjRef animation(_talkAnimations.at(index));

	StartCharacterAnim(animation, TCharacterAnimEnum::kTalk, false);
}

// Confirmed (asm lines 182419-182456)
void TGCharacter::StartFittingAnimation() {
	if (_state == 3) {
		StartWalkAnim();
	} else if (_state == 2) {
		if (gameControl()->IsTalking(_objRef))
			StartTalkAnim();
		else
			StartStandingAnim();
	}
}

// Confirmed (asm lines 182467-182625): one of the random animations of the outfit that
// is for the direction the character stands to (or for any), picked at random.
void TGCharacter::StartRandomAnim() {
	TVisObjRef outfit = _objRef.GetLink(kCharacterCurrentOutfit);
	TVList candidates;

	outfit.GetLinks(kOutfitRandomAnimations, TypeOrder::kValue1, candidates);

	if (candidates.empty())
		return;

	int standing = GetStandingDirection();
	int fitting = 0;

	for (TVisionaireObject *item : candidates) {
		int direction = item->GetInt(kAnimationDirection);

		if (direction == -1 || direction == standing)
			fitting++;
	}

	if (fitting == 0)
		return;

	int pick = std::rand() % fitting;

	for (TVisionaireObject *item : candidates) {
		int direction = item->GetInt(kAnimationDirection);

		if (direction != standing && direction != -1)
			continue;

		if (pick == 0) {
			TVisObjRef animation(item);

			StartCharacterAnim(animation, TCharacterAnimEnum::kRandom, false);
			return;
		}
		pick--;
	}
}

// Confirmed (asm lines 182666-182770): while the character does nothing the random timer
// runs; when it is out an animation starts.
void TGCharacter::CheckRandomTimer() {
	int kind = _objRef.GetInt(kCharacterAnimState);

	if (kind == (int)TCharacterAnimEnum::kRandom)
		return;

	bool idle = (_state != 3 && gameControl()->IsNoTextDisplayed() &&
	             (kind == (int)TCharacterAnimEnum::kStand || kind == (int)TCharacterAnimEnum::kNone));

	if (!idle)
		_randomTimer.SetTime();

	if (_randomTimer.GetTime() > _randomTime) {
		StartRandomAnim();
		SetRandomTime();
	}
}

// Confirmed (asm lines 182784-183210): the character looks to a new direction; one of
// the outfit's turn animations is played when it has one from the old to the new
// direction (within 30 degrees of both). The original lists each turn animation four
// times (the combinations of mirrored and replayed) but with the same directions each
// time, so only the first of them can match.
void TGCharacter::CheckTurning(int direction) {
	int oldDirection = _objRef.GetInt(kCharacterDirection);

	_objRef.SetValue(kCharacterDirection, direction, TSendEventEnum::kNoEvent);

	size_t turns = _turnFrom.size();

	for (size_t i = 0; i < turns * 4; i++) {
		size_t turn = i / 4;

		if (diff(_turnFrom[turn], oldDirection) > 29 || diff(_turnTo[turn], direction) > 29)
			continue;

		_state = 4;

		TVisObjRef animation(_turnAnimations.at((int)turn));
		std::vector<TSprite> sprites;
		bool mirrored = (i & 3) > 1;

		animation.GetSprites(kAnimationSprites, sprites);

		for (TSprite &sprite : sprites)
			sprite.SetMirrored(mirrored);

		animation.SetValue(kAnimationSprites, sprites, TSendEventEnum::kSendEvent);
		animation.SetValue(kAnimationReplay, (int)(i & 1), TSendEventEnum::kSendEvent);
		StartCharacterAnim(animation, TCharacterAnimEnum::kTurn, false);
		return;
	}
}

// Confirmed (asm lines 183217-183500): per frame: a character that is to act on another
// one does so when it has reached it; a character that follows another one stays near it.
void TGCharacter::UpdateCharacter() {
	TVisObjRef actionCharacter = _objRef.GetLink(kCharacterActionCharacter);

	if (!actionCharacter.IsEmpty() && *actionCharacter.GetPoint(kCharacterActionDestPosition) == wxPoint{-1, -1}) {
		TGCharacter *other = gameControl()->GetCharacterPointerEx(actionCharacter);

		if (other && other->IsReached(_objRef, false)) {
			gameControl()->GetObjectManager()->ObjectReached(other);
			StopWalking(true);
		}
	}

	TVisObjRef followed = _objRef.GetLink(kCharacterFollowCharacter);

	if (followed.IsEmpty())
		return;

	// (only in the same scene)
	if (!(followed.GetLink(kCharacterScene) == _objRef.GetLink(kCharacterScene)))
		return;

	if (IsReached(followed, true)) {
		StopWalking(true);

		TVisObjRef followAction = _objRef.GetLink(kCharacterFollowAction);

		if (!followAction.IsEmpty())
			TGAction::AddRunningAction(_objRef.GetLink(kCharacterFollowAction));

		_followTimer.SetTime();
	} else if (_followTimer.GetTime() > 999) {
		// (once a second the way is searched again)
		_objRef.SetValue(kCharacterDestination, *followed.GetPoint(kCharacterPosition), TSendEventEnum::kSendEvent);
		_followTimer.SetTime();
	}
}

// Confirmed (asm lines 183506-183625): the character is put on the end of its way at once.
void TGCharacter::SetOnDestination() {
	if (_state == 3) {
		_position = _waySystem.GetDestination();
		_objRef.SetValue(kCharacterPosition, _position, TSendEventEnum::kNoEvent);
		_realPosition.x = (float)_position.x;
		_realPosition.y = (float)_position.y;
	}

	TVisObjRef destinationObject = _objRef.GetLink(kCharacterDestinationObject);

	if (!destinationObject.IsEmpty()) {
		int direction = destinationObject.GetInt(kObjectDirection);

		if (direction != -1)
			_objRef.SetValue(kCharacterDirection, direction, TSendEventEnum::kSendEvent);

		_objRef.ClearLink(kCharacterDestinationObject, true);
	}

	SetCurrentSize();
	StopWalking(true);
}

// Confirmed (asm lines 183635-184055): one step along the way (the step: the speed of the
// outfit, or of the animation's frame, scaled with the character's size).
void TGCharacter::WalkWay() {
	if (_state != 3)
		return;

	float elapsed = (float)_walkTimer.GetTime();

	if (elapsed > 500.0f)
		elapsed = 500.0f;

	float speed = (float)_outfit.GetInt(kOutfitCharacterSpeed);
	bool perSecond = true;

	if (_currentAnimation) {
		std::vector<float> steps;
		int frame = _currentAnimation->GetCurrentSpriteIndexOrTick();

		_currentAnimation->GetDataObject().GetFloats(kAnimationWalkSteps, steps);

		if (_outfit.GetBool(kOutfitSlideWalkAnimation)) {
			// (the steps are speeds per second)
			if (frame >= 0 && frame < (int)steps.size())
				speed = steps[frame];
			_walkFrame = frame;
		} else {
			// (the steps are the distance to move when the animation goes on to the next frame)
			if (frame == _walkFrame)
				return;

			int previous = _walkFrame;

			_walkFrame = frame;

			if (previous < 0 || previous >= (int)steps.size())
				return;

			speed = steps[previous];

			if (speed == 0.0f)
				return;

			perSecond = false;
		}
	}

	float step = _objRef.GetFloat(kCharacterSize) / 100.0f * speed;

	if (perSecond)
		step = step / 1000.0f * elapsed;

	_walkTimer.SetTime();

	int angle = -1;
	bool arrived = _waySystem.GetNextPosition(_realPosition, &angle, step);

	_position.x = (int)_realPosition.x;
	_position.y = (int)_realPosition.y;
	_objRef.SetValue(kCharacterPosition, _position, TSendEventEnum::kNoEvent);

	if (!arrived) {
		if (angle != -1) {
			_objRef.SetValue(kCharacterDirection, angle, TSendEventEnum::kNoEvent);
			StartWalkAnim();
		}
	} else {
		TVisObjRef destinationObject = _objRef.GetLink(kCharacterDestinationObject);

		if (!destinationObject.IsEmpty()) {
			// (an object to walk to: it is reached, and the character looks to its direction)
			int direction = destinationObject.GetInt(kObjectDirection);

			if (direction != -1)
				_objRef.SetValue(kCharacterDirection, direction, TSendEventEnum::kSendEvent);

			StopWalking(true);
			gameControl()->GetObjectManager()->ObjectReached(*this, destinationObject);
			_objRef.ClearLink(kCharacterDestinationObject, true);
		} else {
			// (a character to act on)
			TVisObjRef actionCharacter = _objRef.GetLink(kCharacterActionCharacter);

			if (!actionCharacter.IsEmpty() && *actionCharacter.GetPoint(kCharacterActionDestPosition) != wxPoint{-1, -1}) {
				TGCharacter *other = gameControl()->GetCharacterPointerEx(actionCharacter);

				if (other && other->IsReached(_objRef, false))
					gameControl()->GetObjectManager()->ObjectReached(other);
			}

			StopWalking(true);
		}
	}

	SetCurrentSize();
	UpdateActionAreas();
}

// Confirmed (asm lines 184065-184210): takes the scene's way system; a walk that was
// going on ends. `init` also sets the way system's number of size lines again and the
// character's size.
void TGCharacter::SetWaySystem(const TVisObjRef &waySystem, bool init) {
	_hasWaySystem = _waySystem.SetWaySystem(waySystem);

	if (init) {
		_waySystem.SetNoLines(_waySystem.GetNoLines());
		SetCurrentSize();
	}

	if (IsWalking())
		StopWalking(true);
}

// Confirmed (asm lines 184219-184301): the character looks to a new direction; the
// animation that is shown changes to the one for it.
void TGCharacter::SetCurrentDirection(int direction) {
	int previous = _directionIndex;

	_directionIndex = GetDirectionIndex(direction, TCharacterAnimEnum::kNone);

	if (previous == _directionIndex)
		return;

	switch ((TCharacterAnimEnum)_objRef.GetInt(kCharacterAnimState)) {
	case TCharacterAnimEnum::kRandom:
		StopCharacterAnim(TCharacterAnimEnum::kRandom, true);
		break;
	case TCharacterAnimEnum::kWalk:
		StopWalking(true);
		break;
	case TCharacterAnimEnum::kTalk:
		StopCharacterAnim(TCharacterAnimEnum::kTalk, false);
		StartTalkAnim();
		break;
	case TCharacterAnimEnum::kStand:
		StopCharacterAnim(TCharacterAnimEnum::kStand, false);
		StartStandingAnim();
		break;
	default:
		break;
	}
}

// Confirmed (asm lines 184312-184615): puts the character in a scene, at a position,
// looking to `direction` (unless that is -1), and takes the scene's way system. The
// action areas that it is in count as entered.
void TGCharacter::AssignToScene(const TVisObjRef &scene, const wxPoint &position, int direction) {
	_objRef.SetLink(kCharacterScene, scene, false);

	_position = position;
	_objRef.SetValue(kCharacterPosition, _position, TSendEventEnum::kNoEvent);
	_realPosition.x = (float)_position.x;
	_realPosition.y = (float)_position.y;

	if (direction != -1) {
		_directionIndex = GetDirectionIndex(direction, TCharacterAnimEnum::kNone);
		_objRef.SetValue(kCharacterDirection, direction, TSendEventEnum::kNoEvent);
	}

	StopWalking(false);
	_objRef.SetValue(kCharacterState, _state, TSendEventEnum::kNoEvent);
	StopCharacterAnim(TCharacterAnimEnum::kNone, true);

	TVisObjRef waySystem = scene.GetLink(kSceneCurrentWaySystem);

	_hasWaySystem = _waySystem.SetWaySystem(waySystem);

	if (IsWalking())
		StopWalking(true);

	_activeAreas.clear();

	std::list<TSceneActionArea *> *areas = TGScene::GetActionAreas(scene);

	if (!areas)
		return;

	for (TSceneActionArea *area : *areas) {
		if (area->CanTrigger(_objRef) && area->IsInside(_position)) {
			_activeAreas.push_back(area);
			runAreaActions(*area, _objRef, kExecuteEnterArea);
		}
	}
}

// Confirmed (asm lines 184624-184980): walks to `destination`. `keepDestinationObject`
// keeps the object that the walk is to (it is cleared otherwise); `noWayPoints` does not
// accept a way along the way points; `useTriangles` finds the way with the triangulation
// of the outline (CalculateWayTriangles()) instead.
void TGCharacter::SetFreeDestination(wxPoint destination, bool keepDestinationObject, bool noWayPoints,
                                     bool useTriangles) {
	if (!keepDestinationObject)
		_objRef.ClearLink(kCharacterDestinationObject, false);

	// (a scene without a way system: the way point that is nearest)
	if (!_hasWaySystem) {
		TTPoint nearest(_waySystem.GetPointNextTo(destination));

		destination = *nearest.GetPoint(kPointPosition);
		_objRef.SetValue(kCharacterDestination, destination, TSendEventEnum::kNoEvent);
	}

	if (_position == destination) {
		StopWalking(true);
		return;
	}

	int previousDirection = _directionIndex;

	if (useTriangles) {
		if (_waySystem.CalculateWayTriangles(_position, destination)) {
			_objRef.SetValue(kCharacterDestination, _waySystem.GetDestination(), TSendEventEnum::kNoEvent);
			CheckTurning(_waySystem.GetDirectionToNextDestination(_position));
		} else {
			// no way: the character turns to the destination and stays
			int angle = GetAngle((float)(destination.x - _position.x), (float)(destination.y - _position.y));

			_directionIndex = GetDirectionIndex(angle, TCharacterAnimEnum::kNone);
			_objRef.SetValue(kCharacterDirection, angle, TSendEventEnum::kNoEvent);
			_objRef.SetValue(kCharacterDestination, _position, TSendEventEnum::kNoEvent);
			StopWalking(true);
			return;
		}
	} else if (!_waySystem.CheckCrossBorders(_position, destination)) {
		// the way is a straight line
		_waySystem.SetDestination(destination);

		int angle = _waySystem.GetDirectionToNextDestination(_position);

		_directionIndex = GetDirectionIndex(angle, TCharacterAnimEnum::kNone);
		CheckTurning(angle);
	} else {
		// the way along the way points
		TTPoint wayPoint;

		wayPoint = TTPoint(_waySystem.GetPointNextTo(destination));

		if (!wayPoint.IsEmpty() && _waySystem.CalculateWay(wayPoint, _position, destination) && !noWayPoints) {
			_objRef.SetValue(kCharacterDestination, _waySystem.GetDestination(), TSendEventEnum::kNoEvent);

			int angle = _waySystem.GetDirectionToNextDestination(_position);

			_directionIndex = GetDirectionIndex(angle, TCharacterAnimEnum::kNone);
			CheckTurning(angle);
		} else {
			// no way
			failedPoint = destination;
			_objRef.SetValue(kCharacterDestination, _position, TSendEventEnum::kNoEvent);
			StopWalking(true);
			return;
		}
	}

	_idleTimer.SetTime();

	if (_state == 4) {
		// (a turn animation is playing: the walk starts after it)
		_previousState = 3;
	} else if (_state == 3) {
		if (_directionIndex != previousDirection)
			StartWalkAnim();
	} else {
		_walkTimer.SetTime();
		_state = 3;
		_objRef.SetValue(kCharacterState, 3, TSendEventEnum::kNoEvent);
		StartWalkAnim();
		CheckWalkingSound();
	}
}

// Confirmed (asm lines 184990-185670): the character wears an outfit; its walk, talk and
// standing animations are collected with the directions they are for (and are set to
// move the character and to play once), and so are the turn animations (with the
// directions they turn from and to). The turn animations are only ever added to, also
// by a second outfit.
void TGCharacter::SetCurrentOutfit(const TVisObjRef &outfit) {
	if (outfit == _outfit)
		return;

	StopCharacterAnim(TCharacterAnimEnum::kNone, false);

	if (_allowUnloadingOutfitAnimations)
		UnloadAnimations();

	_outfit = outfit;

	ClearSprites();

	// (the lists of walk, talk and standing animations with the directions they are for;
	// each animation moves the character and runs once)
	TVList links;
	TVisObjRef animation;

	outfit.GetLinks(kOutfitTalkAnimations, TypeOrder::kValue1, links);

	for (TVisionaireObject *item : links) {
		animation.Set(item);
		_talkDirections.push_back(animation.GetInt(kAnimationDirection));
		animation.SetValue(kAnimationMove, true, TSendEventEnum::kSendEvent);
		animation.SetValue(kAnimationNumberOfLoops, 0, TSendEventEnum::kSendEvent);
		_talkAnimations.push_back(animation);
	}

	links.clear();
	outfit.GetLinks(kOutfitStandingAnimations, TypeOrder::kValue1, links);

	for (TVisionaireObject *item : links) {
		animation.Set(item);
		_standDirections.push_back(animation.GetInt(kAnimationDirection));
		animation.SetValue(kAnimationMove, true, TSendEventEnum::kSendEvent);
		animation.SetValue(kAnimationNumberOfLoops, 0, TSendEventEnum::kSendEvent);
		_standAnimations.push_back(animation);
	}

	links.clear();
	outfit.GetLinks(kOutfitWalkAnimations, TypeOrder::kValue1, links);

	for (TVisionaireObject *item : links) {
		animation.Set(item);
		_walkDirections.push_back(animation.GetInt(kAnimationDirection));
		animation.SetValue(kAnimationMove, true, TSendEventEnum::kSendEvent);
		animation.SetValue(kAnimationNumberOfLoops, 0, TSendEventEnum::kSendEvent);
		_walkAnimations.push_back(animation);
	}

	// the turn animations play once
	links.clear();
	outfit.GetLinks(kOutfitTurnAnimations, TypeOrder::kValue1, links);

	for (TVisionaireObject *item : links) {
		animation.Set(item);
		animation.SetValue(kAnimationNumberOfLoops, 1, TSendEventEnum::kSendEvent);
		_turnFrom.push_back(animation.GetInt(kAnimationDirection));
		_turnTo.push_back(animation.GetInt(kAnimationEndDirection));
		_turnAnimations.push_back(animation);
	}

	// in the scene that is shown the new animations are loaded and the character shows
	// the one that fits
	TVisObjRef shownScene = gameControl()->GetVisionaire()->GetGame().GetLink(kGameCurrentScene);
	bool inShownScene = (_objRef.GetLink(kCharacterScene) == shownScene);

	if (inShownScene)
		PreloadAnimations();

	_directionIndex = GetDirectionIndex(_objRef.GetInt(kCharacterDirection), TCharacterAnimEnum::kNone);

	if (inShownScene) {
		if (_state == 2) {
			if (gameControl()->IsTalking(_objRef))
				StartTalkAnim();
			else
				StartStandingAnim();
		} else if (_state == 3) {
			StartWalkAnim();
		}
	}
}

// Confirmed (asm lines 185680-185780)
void TGCharacter::Init() {
	SetCurrentOutfit(_objRef.GetLink(kCharacterCurrentOutfit));
	SetCurrentCommentSet(_objRef.GetLink(kCharacterCurrentCommentSet));

	_walkingSound = _objRef.GetPath(kCharacterWalkingSound);
	_hasWalkingSound = _walkingSound.IsOk();

	SetRandomTime();
	_idleTimer.SetTime();
	_walkTimer.SetTime();
}

// Confirmed (asm lines 185834-186239): restores the character from its saved data.
void TGCharacter::Load() {
	SetLifetime(0);

	TVisObjRef outfit = _objRef.GetLink(kCharacterCurrentOutfit);

	if (_outfit == outfit) {
		_directionIndex = GetDirectionIndex(_objRef.GetInt(kCharacterDirection), TCharacterAnimEnum::kNone);
		TGAnimation::ReattachAnimations(*this);
	} else {
		SetCurrentOutfit(outfit);
	}

	SetCurrentCommentSet(_objRef.GetLink(kCharacterCurrentCommentSet));

	_active = _objRef.GetBool(kCharacterActive);

	_position = *_objRef.GetPoint(kCharacterPosition);
	_realPosition.x = (float)_position.x;
	_realPosition.y = (float)_position.y;

	TVisObjRef scene = _objRef.GetLink(kCharacterScene);

	_hasWaySystem = _waySystem.SetWaySystem(scene.GetLink(kSceneCurrentWaySystem));
	_waySystem.SetNoLines((int)scene.GetSprite(kSceneSprite).GetSizedHeight());

	_followReach = _objRef.GetInt(kCharacterFollowReachDistance);
	_state = _objRef.GetInt(kCharacterState);

	_walkingSound = _objRef.GetPath(kCharacterWalkingSound);
	_hasWalkingSound = _walkingSound.IsOk();
	_walkingSoundPlaying = false;

	// a walk goes on to where it was going
	if (_state == 3)
		SetFreeDestination(*_objRef.GetPoint(kCharacterDestination), false, false, false);

	// (a random animation does not come back: the standing one does)
	if (_objRef.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kRandom)
		_objRef.SetValue(kCharacterAnimState, (int)TCharacterAnimEnum::kStand, TSendEventEnum::kSendEvent);

	_alpha = (float)_objRef.GetInt(kCharacterVisibility) / 100.0f;
	_timer.SetTime();
	SetDestAlpha(_objRef.GetInt(kCharacterDestVisibility), _objRef.GetInt(kCharacterTimeToDestVisibility));

	TVList items;

	_objRef.GetLinks(kCharacterItems, TypeOrder::kValue0, items);

	for (TGInterface *shown : _interfaces)
		shown->UpdateItems(items);

	_activeAreas.clear();
	UpdateActionAreas();

	SetRandomTime();
}
