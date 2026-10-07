#include "vsplayer/animationGame.h"

#include <algorithm>
#include <cstring>
#include <string>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "TCharHolder.h"
#include "TGAction.h"
#include "THAnimation.h"
#include "TManagedObject.h"
#include "TSoundFFMPEG.h"
#include "TSprite.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "graphicslib/graphics.h"
#include "graphicslib/picture.h"
#include "graphicslib/preloadedPicManager.h"
#include "vscommon/scripting/lua.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/animationGame.cpp";

// The table of the TSAnimation records (the running state of an animation) in the
// game data; its name is not resolved.
static const int kAnimationStateTable = 0x1A;

std::list<TGAnimation *> TGAnimation::RunningAnimations;
wxString TGAnimation::EventHandlerAnimStarted;
wxString TGAnimation::EventHandlerAnimStopped;

static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

// Whether the animation trace messages are switched on (bit 0 of g_traceFlags and
// a log level above 1).
static bool tracing() {
	return (g_traceFlags & 1) && wxLog::loglevel > 1;
}

static bool sameId(const TVisObjRef &first, const TVisObjRef &second) {
	return std::memcmp(first.GetId(), second.GetId(), 4) == 0;
}

// The trace messages name an animation by the name and id of its data object; some
// add the id of its running state ("active id").
static void traceAnimation(const wchar_t *format, const TVisObjRef &data) {
	if (tracing())
		wxLog::logexpanded(format, data.GetName().c_str().c_str(), PackVisId(data.GetId()));
}

static void traceAnimation(const wchar_t *format, const TVisObjRef &state, const TVisObjRef &data) {
	if (tracing())
		wxLog::logexpanded(format, data.GetName().c_str().c_str(), PackVisId(state.GetId()),
		                   PackVisId(data.GetId()));
}

static void callEventHandler(const char *debugName, const wxString &handler, const TVisObjRef &state) {
	LuaDebugName(debugName);
	LuaExecuteEventHandler(std::string((const char *)handler.mb_str()), state);
}

// The frame/shader a picture is drawn with: the object's own shader, or the
// default one when it has none.
static int shaderOf(int shader) {
	if (shader == -1 && defaultShader != 0)
		return defaultShader;
	return shader;
}

// The running animation (in the list) that plays the data object, or the end.
static std::list<TGAnimation *>::iterator findByData(std::list<TGAnimation *> &animations,
                                                     const TVisObjRef &dataObject) {
	for (auto it = animations.begin(); it != animations.end(); ++it) {
		if (sameId((*it)->GetDataObject(), dataObject))
			return it;
	}
	return animations.end();
}

// TODO (low priority, see /TODO.md): the model/Spine branches of this class
// (ctor, NextSpriteSelected(), Prepare(), Draw(), DrawMixed(), DrawWithLightMap()).
//
// Confirmed (asm lines 156727-156900; the model part is left out)
TGAnimation::TGAnimation(const TVisObjRef &active, const TVisObjRef &animation)
	: TCAnimation(active, animation) {
	_data.GetLinks(kAnimationPropertyFrames, TypeOrder::kValue0, _frames);

	// the object the animation belongs to; an outfit's animations belong to its
	// character
	_ownerObject = _data.GetParent();
	if (_ownerObject.GetId()[3] == 0x11)
		_ownerObject = _ownerObject.GetParent();
}

// Confirmed (asm lines 156619-156706)
TGAnimation::~TGAnimation() {
	graphics->GetPreloadedPicManager()->StopPreloading(_sprites);
	NotifyOwnersAnimationFinished();
}

// Confirmed (asm lines 149433-149495)
void TGAnimation::Start(bool reverse, float scale) {
	TCAnimation::Start(reverse, scale);

	if (!EventHandlerAnimStarted.IsEmpty())
		callEventHandler("AnimationStartedHook", EventHandlerAnimStarted, _state);
}

// Confirmed (asm lines 148778-148836)
bool TGAnimation::CanRemoveCurrentSprite() const {
	if (_state.GetBool(kAnimationPreloaded))
		return false;
	if (_state.GetInt(kAnimationLoops) != 1)
		return false;
	if (_state.GetInt(kAnimationCurrentSpriteIndex) == -1)
		return false;
	return !IsLoopRandom();
}

// Confirmed (asm lines 148837-149420; the model/Spine matching of the frames is
// left out). Every frame of the animation that is for the sprite just selected
// plays its sound and starts its action.
void TGAnimation::NextSpriteSelected() const {
	int index = _state.GetInt(kAnimationCurrentSpriteIndex);

	for (TVisionaireObject *frame : _frames) {
		if (frame->GetInt(kAnimationFrameIndex) != index)
			continue;

		wxFileName sound = frame->GetPath(kAnimationFrameSound);
		TVisObjRef scene = gameControl()->GetGameSystem()->GetGame().GetLink(kGameCurrentScene);

		// The sound is only played when what the animation belongs to is on the
		// shown scene: a character that is on it, an object that is not on a scene
		// (or on the shown one); anything else always.
		bool playSound = true;
		int ownerType = _ownerObject.GetId()[3];

		if (ownerType == 0) {
			playSound = _ownerObject.GetLink(kCharacterScene) == scene;
		} else if (ownerType == 6) {
			TVisObjRef parent = _ownerObject.GetParent();

			playSound = parent.GetId()[3] != 4 || parent == scene;
		}

		if (sound.IsOk() && playSound) {
			int volume = frame->GetInt(kAnimationFrameSoundVolume);
			int balance = frame->GetInt(kAnimationFrameSoundBalance);

			gameControl()->GetSoundManager()->Play(sound, volume, balance, false, TSoundTypeEnum::kValue1, true, 0);
		}

		TVisObjRef action(frame->GetLink(kAnimationFrameAction));

		if (!action.IsEmpty()) {
			// the action is named after the animation and the frame it is started for
			std::wstring name = std::wstring(_state.GetName().c_str().c_str()) + L": Frame #" +
			                    std::to_wstring(index + 1);

			action.SetName(TCharHolder(wxString(name.c_str())));
			TGAction::AddRunningAction(action);
		}
	}
}

// Confirmed (asm lines 149631-149670)
void TGAnimation::NotifyOwnersAnimationFinished() {
	// (an owner may detach itself from the animation while it is told)
	std::vector<TAnimationOwner *> owners = _owners;

	for (TAnimationOwner *owner : owners)
		owner->AnimationStopped(this);
	_owners.clear();
}

// Confirmed (asm lines 149671-149738)
void TGAnimation::DetachOwner(TAnimationOwner *owner) {
	auto it = std::find(_owners.begin(), _owners.end(), owner);

	if (it == _owners.end()) {
		x_assert(false, "false", kSourceFile, 0x72);
		return;
	}
	_owners.erase(it);
}

// Confirmed (asm lines 149739-149783)
void TGAnimation::SetParallax(int x, int y) {
	for (TPictureIO *sprite : _sprites)
		sprite->SetParallax(x, y);
}

// Confirmed (asm lines 149784-149810; a model animation has nothing to prepare)
void TGAnimation::Prepare() {
	if (_currentSprite)
		_currentSprite->RefreshSprite(false);
}

// Confirmed (asm lines 149811-150132; the model part is left out)
void TGAnimation::Draw(float alpha, unsigned int color, int /*frame*/) {
	if (!_currentSprite)
		return;

	wxPoint savedPosition = _currentSprite->GetPosition();

	// The pictures that follow are to be preloaded next, in order, once the one
	// after the shown one is being preloaded at all.
	int index = _state.GetInt(kAnimationCurrentSpriteIndex);

	if (index >= 0 && (size_t)index + 1 < _sprites.size()) {
		TPictureIO *next = _sprites[index + 1];

		if (next->GetPreloadingStatus() != TPictureIO::ePreloadingStatus::NotPreloading) {
			next->SetPreloadPriority(0);
			if ((size_t)index + 2 < _sprites.size())
				_sprites[index + 2]->SetPreloadPriority(1);
		}
	}

	wxPoint position = GetCurrentSpritePosition();

	// What the sprite is drawn with comes from the object the animation belongs to.
	switch (_ownerObject.GetId()[3]) {
	case 6: {
		// a scene object
		_currentSprite->SetRotation(_ownerObject.GetFloat(kObjectRotation));
		_currentSprite->SetRotationCenter(*_ownerObject.GetPoint(kObjectRotationCenter));
		_currentSprite->SetScale(_ownerObject.GetFloat(kObjectScaleX), _ownerObject.GetFloat(kObjectScaleY));

		int shader = shaderOf(_ownerObject.GetInt(kObjectShaderSet));

		_currentSprite->SetShader(shader);
		ShaderCallback(shader, &_ownerObject);
		_currentSprite->SetMatrixId(_ownerObject.GetInt(kObjectMatrixId));
		position += *_ownerObject.GetPoint(kObjectOffset);
		break;
	}

	case 2: {
		// a button
		_currentSprite->SetRotation(_ownerObject.GetFloat(kButtonRotation));
		_currentSprite->SetRotationCenter(*_ownerObject.GetPoint(kButtonRotationCenter));
		_currentSprite->SetScale(_ownerObject.GetFloat(kButtonScaleX), _ownerObject.GetFloat(kButtonScaleY));
		_currentSprite->SetMatrixId(_ownerObject.GetInt(kButtonMatrixId));

		int shader = shaderOf(_ownerObject.GetInt(kButtonShaderSet));

		_currentSprite->SetShader(shader);
		ShaderCallback(shader, &_ownerObject);
		break;
	}

	case 0: {
		// a character
		int shader = shaderOf(_ownerObject.GetInt(kCharacterShaderSet));

		_currentSprite->SetShader(shader);
		ShaderCallback(shader, &_ownerObject);
		break;
	}

	default:
		break;
	}

	_currentSprite->SetPosition(position, _state.GetFloat(kAnimationSize));
	_currentSprite->Draw(alpha, color);
	_currentSprite->SetPosition(savedPosition, -1.0f);
}

// Confirmed (asm lines 150133-150228): only Spine animations are drawn mixed.
void TGAnimation::DrawMixed(float /*alpha*/, unsigned int /*color*/, const std::vector<TGAnimation *> &/*overlays*/) {
}

// Confirmed (asm lines 150237-150355; the model part is left out)
void TGAnimation::DrawWithLightMap(float alpha, void *lightMap, int /*frame*/) {
	if (!_currentSprite)
		return;

	wxPoint savedPosition = _currentSprite->GetPosition();

	// (The original reads the size as an int although the field is a float: that read
	// fails and gives -1, which leaves the sprite's own scale alone. Reproduced.)
	float scale = (float)_state.GetInt(kAnimationSize);
	wxPoint position = GetCurrentSpritePosition();

	_currentSprite->SetPosition(position, scale);
	_currentSprite->DrawWithLightMap(alpha, 0xFFFFFF, lightMap);
	_currentSprite->SetPosition(savedPosition, -1.0f);
}

// Confirmed (asm lines 150366-150529)
void TGAnimation::ContinueStoppedAnimation() {
	if (!_stopped)
		return;

	_stopped = false;
	traceAnimation(L"\"%s\" (active id: %d, data id: %d)", _state, _data);

	_timer.AdjustTimer(TGameControl::GetStopTime().GetTime());
	if (_timer.GetTime() < 0)
		_timer.SetTime();
}

// Confirmed (asm lines 150530-150547)
bool TGAnimation::IsPreloaded() const {
	return _state.GetBool(kAnimationPreloaded);
}

// Confirmed (asm lines 150548-150568)
void TGAnimation::SetStartedByUser(bool startedByUser) {
	_state.SetValue(kAnimationStartedByUser, startedByUser, TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 153487-153582)
void TGAnimation::Load() {
	_timer.SetTime();
	_timer.AdjustTimer(-(long)_state.GetInt(kAnimationCalledTime));

	int index = _state.GetInt(kAnimationCurrentSpriteIndex);

	if (index == -1) {
		_state.SetValue(kAnimationCurrentSpriteIndex, 0, TSendEventEnum::kSendEvent);
		index = 0;
	}

	if (index < 0 || index >= (int)_sprites.size()) {
		_currentSprite = nullptr;
		_state.SetValue(kAnimationCurrentSpriteIndex, -1, TSendEventEnum::kNoEvent);
	} else {
		_currentSprite = _sprites[index];
	}

	// (the animations of type 0xF objects are not running after a load)
	if (_ownerObject.GetId()[3] == 0x0F)
		_state.SetValue(kAnimationActive, false, TSendEventEnum::kNoEvent);

	_stopped = false;
}

// Confirmed (asm lines 153999-154102)
void TGAnimation::Save() {
	bool keep = IsPreloaded() || (_state.GetBool(kAnimationActive) && _state.GetBool(kAnimationStartedByUser));

	if (keep) {
		TTimer timer = _timer;

		if (_stopped)
			timer.AdjustTimer(TGameControl::GetStopTime().GetTime());
		_state.SetValue(kAnimationCalledTime, (int)timer.GetTime(), TSendEventEnum::kSendEvent);
		_state.SetTemporary(false);
	} else {
		_state.SetTemporary(true);
	}
}

// Confirmed (asm lines 150782-150831)
void TGAnimation::ClearAnimations() {
	for (TGAnimation *animation : RunningAnimations)
		delete animation;
	RunningAnimations.clear();
}

// Confirmed (asm lines 154103-154133)
void TGAnimation::SaveAnimations() {
	for (TGAnimation *animation : RunningAnimations)
		animation->Save();
}

// Confirmed (asm lines 153583-153998): every TSAnimation record of the game data
// (they are saved with the game) becomes a running animation again, as long as
// the animation it was made for still exists.
void TGAnimation::LoadAnimations() {
	ClearAnimations();

	TVList records;

	gameControl()->GetVisionaire()->GetList(kAnimationStateTable, records, false);
	for (TVisionaireObject *record : records) {
		TVisObjRef data(record->GetLink(kAnimationSavedObject));

		if (data.IsEmpty()) {
			if (tracing()) {
				TVisObjRef state(record);

				wxLog::logexpanded(L"Failed to load animation: \"%s\" (active id: %d). Could not find original "
				                   L"animation.",
				                   state.GetName().c_str().c_str(), PackVisId(state.GetId()));
			}
			continue;
		}

		TVisObjRef state(record);
		TGAnimation *animation = new THAnimation(state, data);

		animation->Load();
		RunningAnimations.push_back(animation);

		int parentType = data.GetParent().GetId()[3];

		if (graphics->GetPreloadedPicManager()->HasQueuedPictures())
			animation->PreloadSprites(parentType == 2 || parentType == 0x11);
		if (tracing())
			wxLog::logexpanded(L"Animation loaded: ");
	}
}

// Confirmed (asm lines 150569-150781): the per-frame update. Animations that are
// stopped, paused or already over are left alone; one that ends now tells its
// owners (if it is kept for being preloaded; otherwise its destructor does),
// starts its action and calls the "animation stopped" hook. Ended animations that
// are not preloaded are then deleted.
void TGAnimation::ContinueAnimations() {
	for (TGAnimation *animation : RunningAnimations) {
		if (animation->_stopped || animation->_paused)
			continue;
		if (!animation->_state.GetBool(kAnimationActive))
			continue;

		animation->SetCurrentSprite(false);
		if (animation->_state.GetBool(kAnimationActive))
			continue;

		if (animation->_state.GetBool(kAnimationPreloaded))
			animation->NotifyOwnersAnimationFinished();

		TVisObjRef action = animation->_data.GetLink(kAnimationAction);

		if (!action.IsEmpty())
			TGAction::AddRunningAction(action);

		if (!EventHandlerAnimStopped.IsEmpty())
			callEventHandler("AnimationStoppedHook", EventHandlerAnimStopped, animation->_state);
	}

	auto it = RunningAnimations.begin();

	while (it != RunningAnimations.end()) {
		TGAnimation *animation = *it;

		if (!animation->_state.GetBool(kAnimationActive) && !animation->_state.GetBool(kAnimationPreloaded)) {
			it = RunningAnimations.erase(it);
			delete animation;
		} else {
			++it;
		}
	}
}

// Confirmed (asm lines 152083-152372): every animation but those of a menu scene
// (the scene that is the data object's grandparent) is stopped.
void TGAnimation::StopRunningAnimations() {
	if (tracing())
		wxLog::logexpanded(L"The following animations are stopped because changing from scene to a menu:");

	for (TGAnimation *animation : RunningAnimations) {
		TVisObjRef scene = animation->_data.GetParent().GetParent();

		if (scene.GetId()[3] == 4 && scene.GetBool(kSceneIsMenu))
			continue;

		animation->_stopped = true;
		traceAnimation(L"\"%s\" (active id: %d, data id: %d)", animation->_state, animation->_data);
	}

	if (tracing())
		wxLog::logexpanded(L"--end of stopped animations---");
}

// Confirmed (asm lines 152373-152489)
void TGAnimation::ContinueStoppedAnimations() {
	if (tracing())
		wxLog::logexpanded(L"The following animations are continued because changing from menu to a scene:");

	for (TGAnimation *animation : RunningAnimations)
		animation->ContinueStoppedAnimation();

	if (tracing())
		wxLog::logexpanded(L"--end of continued animations---");
}

// Confirmed (asm lines 158721-158832)
void TGAnimation::ReattachAnimations(TManagedObject &object) {
	for (TGAnimation *animation : RunningAnimations) {
		if (!animation->_state.GetBool(kAnimationActive))
			continue;
		if (!(animation->_ownerObject == object.GetRef()))
			continue;
		if (std::find(animation->_owners.begin(), animation->_owners.end(), &object) != animation->_owners.end())
			continue;

		animation->_owners.push_back(&object);
		object.SetAnimation(animation);
	}
}

// Confirmed (asm lines 157743-158720; the model part is left out)
TGAnimation *TGAnimation::StartAnimation(const TVisObjRef &dataObject, TAnimationOwner *owner, bool reverse,
                                         float scale, int frame) {
	if (dataObject.IsEmpty())
		return nullptr;

	std::vector<TSprite> sprites;

	dataObject.GetSprites(kAnimationSprites, sprites);
	if (sprites.empty() && !TTAnimation::IsModelAnimation(dataObject) &&
	    !TTAnimation::IsBonesAnimation(dataObject)) {
		traceAnimation(L"Animation has no frames: \"%s\" (Id: %d)", dataObject);
		return nullptr;
	}

	auto found = findByData(RunningAnimations, dataObject);

	if (found == RunningAnimations.end()) {
		// a new animation
		TVisObjRef state = gameControl()->GetVisionaire()->CreateActiveObject(kAnimationStateTable, dataObject);
		TGAnimation *animation = new THAnimation(state, dataObject);

		animation->Start(reverse, scale);

		int parentType = animation->_state.GetLink(kAnimationSavedObject).GetParent().GetId()[3];

		if (graphics->GetPreloadedPicManager()->HasQueuedPictures())
			animation->PreloadSprites(parentType == 2 || parentType == 0x11);
		animation->SetCurrentSprite(false);
		RunningAnimations.push_back(animation);
		traceAnimation(L"Animation added: \"%s\" (active id: %d, data id: %d)", animation->_state, dataObject);

		if (owner)
			animation->_owners.push_back(owner);
		return animation;
	}

	TGAnimation *animation = *found;

	if (animation->IsPreloaded()) {
		if (animation->_state.GetBool(kAnimationActive)) {
			traceAnimation(L"Preloaded animation is already running: \"%s\" (active id: %d, data id: %d)",
			               animation->_state, dataObject);
		} else {
			traceAnimation(L"Showing preloaded animation: \"%s\" (active id: %d, data id: %d)",
			               animation->_state, dataObject);
			animation->Start(reverse, scale);
			if (frame >= 0 && frame < animation->GetFrameCount())
				animation->_state.SetValue(kAnimationCurrentSpriteIndex, frame - 1, TSendEventEnum::kSendEvent);
			animation->SetCurrentSprite(true);
		}
	} else {
		// an animation that has ended and only waits to be deleted
		if (!animation->_state.GetBool(kAnimationActive))
			return nullptr;
		traceAnimation(L"Animation is already running: \"%s\" (active id: %d, data id: %d)",
		               animation->_state, dataObject);
	}

	if (owner && std::find(animation->_owners.begin(), animation->_owners.end(), owner) == animation->_owners.end())
		animation->_owners.push_back(owner);
	return animation;
}

// Confirmed (asm lines 157331-157742; the model part is left out)
TGAnimation *TGAnimation::StartLuaAnimation(const TVisObjRef &dataObject, bool reverse, float scale) {
	if (dataObject.IsEmpty())
		return nullptr;

	std::vector<TSprite> sprites;

	dataObject.GetSprites(kAnimationSprites, sprites);
	if (sprites.empty() && !TTAnimation::IsModelAnimation(dataObject) &&
	    !TTAnimation::IsBonesAnimation(dataObject)) {
		traceAnimation(L"Animation has no frames: \"%s\" (Id: %d)", dataObject);
		return nullptr;
	}

	TVisObjRef state = gameControl()->GetVisionaire()->CreateActiveObject(kAnimationStateTable, dataObject);
	TGAnimation *animation = new THAnimation(state, dataObject);

	animation->Start(reverse, scale);
	if (graphics->GetPreloadedPicManager()->HasQueuedPictures())
		animation->PreloadSprites(true);
	animation->SetCurrentSprite(false);
	RunningAnimations.push_back(animation);
	traceAnimation(L"Animation added: \"%s\" (active id: %d, data id: %d)", animation->_state, dataObject);
	return animation;
}

// Confirmed (asm lines 150832-151163)
void TGAnimation::hideAnimation(std::list<TGAnimation *>::iterator position, const TVisObjRef &dataObject) {
	TGAnimation *animation = *position;

	animation->_state.SetValue(kAnimationActive, false, TSendEventEnum::kNoEvent);

	if (!EventHandlerAnimStopped.IsEmpty())
		callEventHandler("AnimationStoppedHook", EventHandlerAnimStopped, animation->_state);

	if (animation->_state.GetBool(kAnimationPreloaded)) {
		// a preloaded animation stays, ready to be shown again
		traceAnimation(L"Preloaded animation hidden: \"%s\" (Id: %d)", dataObject);
		animation->NotifyOwnersAnimationFinished();
	} else {
		traceAnimation(L"Animation removed: \"%s\" (active id: %d, data id: %d)", animation->_state, dataObject);
		delete animation;
		RunningAnimations.erase(position);
	}
}

// Confirmed (asm lines 151164-151330)
void TGAnimation::HideAnimation(const TVisObjRef &dataObject) {
	auto found = findByData(RunningAnimations, dataObject);

	if (found != RunningAnimations.end())
		hideAnimation(found, dataObject);
	else
		traceAnimation(L"Cannot hide animation \"%s\" (Id: %d) because it is not running.", dataObject);
}

// Confirmed (asm lines 151331-151722): `owner` stops using the animation; it is
// hidden when nobody else does.
void TGAnimation::HideAnimation(TGAnimation *animation, TAnimationOwner *owner) {
	std::list<TGAnimation *>::iterator found = RunningAnimations.begin();

	while (found != RunningAnimations.end() && !((*found)->_state == animation->_state))
		++found;

	if (found == RunningAnimations.end()) {
		traceAnimation(L"Cannot hide animation \"%s\" (id: %d) because it is not running.",
		               animation->GetDataObject());
		return;
	}

	std::vector<TAnimationOwner *> &owners = animation->_owners;

	if (!owners.empty()) {
		auto ownerIt = std::find(owners.begin(), owners.end(), owner);

		if (ownerIt != owners.end())
			owners.erase(ownerIt);
		if (!owners.empty()) {
			traceAnimation(L"Animation \"%s\" (id: %d) not hidden because there are still other owners of this "
			               L"animation.",
			               animation->GetDataObject());
			return;
		}
	}

	hideAnimation(found, animation->GetDataObject());
}

void TGAnimation::HideAnimation(TGAnimation *animation, TManagedObject *owner) {
	HideAnimation(animation, static_cast<TAnimationOwner *>(owner));
}

// Confirmed (asm lines 151723-152003): only a preloaded animation can be
// unloaded; it is deleted even when it is running.
void TGAnimation::UnloadAnimation(const TVisObjRef &dataObject) {
	if (dataObject.IsEmpty())
		return;

	auto found = findByData(RunningAnimations, dataObject);

	if (found == RunningAnimations.end()) {
		traceAnimation(L"Animation could not be unloaded because it is not preloaded: \"%s\" (id: %d)", dataObject);
		return;
	}

	TGAnimation *animation = *found;

	if (!animation->IsPreloaded())
		return;

	traceAnimation(L"Animation unloaded: \"%s\" (id: %d)", dataObject);
	delete animation;
	RunningAnimations.erase(found);
}

// Confirmed (asm lines 156908-157330)
void TGAnimation::PreloadAnimation(const TVisObjRef &dataObject) {
	if (dataObject.IsEmpty())
		return;

	auto found = findByData(RunningAnimations, dataObject);

	if (found != RunningAnimations.end()) {
		traceAnimation(L"Already existing animation preloaded: \"%s\" (Id: %d)", dataObject);
		(*found)->_state.SetValue(kAnimationPreloaded, true, TSendEventEnum::kNoEvent);
		return;
	}

	traceAnimation(L"Animation preloaded: \"%s\" (id: %d)", dataObject);

	TVisObjRef state = gameControl()->GetVisionaire()->CreateActiveObject(kAnimationStateTable, dataObject);
	TGAnimation *animation = new TGAnimation(state, dataObject);

	animation->_state.SetValue(kAnimationPreloaded, true, TSendEventEnum::kNoEvent);
	animation->_state.SetValue(kAnimationActive, false, TSendEventEnum::kNoEvent);
	graphics->GetPreloadedPicManager()->PreloadPictures(animation->_sprites);
	RunningAnimations.push_back(animation);
}

// Confirmed (asm lines 152004-152082)
bool TGAnimation::AnimationIsRunning(const TVisObjRef &dataObject) {
	auto found = findByData(RunningAnimations, dataObject);

	return found != RunningAnimations.end() && (*found)->_state.GetBool(kAnimationActive);
}

// Confirmed (asm lines 152490-152546)
TGAnimation *TGAnimation::GetAnimationByObject(const TVisObjRef &state) {
	for (TGAnimation *animation : RunningAnimations) {
		if (sameId(animation->_state, state))
			return animation;
	}
	return nullptr;
}

// Confirmed (asm lines 152547-152621)
TGAnimation *TGAnimation::GetAnimation(const TVisObjRef &dataObject) {
	auto found = findByData(RunningAnimations, dataObject);

	return (found != RunningAnimations.end()) ? *found : nullptr;
}

// Confirmed (asm lines 152622-152780): the object is a scene; the animation is
// looked up in the animations of all its objects.
TVisObjRef TGAnimation::GetObjectAnimationByName(const TVisObjRef &object, const wxString &name) {
	TVList sceneObjects;
	TCharHolder wanted(name);

	object.GetLinks(kSceneObjects, TypeOrder::kValue0, sceneObjects);
	for (TVisionaireObject *sceneObject : sceneObjects) {
		TVList animations;

		sceneObject->GetLinks(kObjectAnimations, TypeOrder::kValue0, animations);
		for (TVisionaireObject *animation : animations) {
			if (animation->GetName().Cmp(wanted) == 0)
				return TVisObjRef(*animation);
		}
	}
	return TVisObjRef();
}

// Confirmed (asm lines 152781-152977): the animation `subName` of the object `name`.
TVisObjRef TGAnimation::GetObjectAnimationByName(const TVisObjRef &object, const wxString &name,
                                                 const wxString &subName) {
	TVList sceneObjects;
	TCharHolder wantedObject(name);
	TCharHolder wantedAnimation(subName);

	object.GetLinks(kSceneObjects, TypeOrder::kValue0, sceneObjects);
	for (TVisionaireObject *sceneObject : sceneObjects) {
		if (sceneObject->GetName().Cmp(wantedObject) != 0)
			continue;

		TVList animations;

		sceneObject->GetLinks(kObjectAnimations, TypeOrder::kValue0, animations);
		for (TVisionaireObject *animation : animations) {
			if (animation->GetName().Cmp(wantedAnimation) == 0)
				return TVisObjRef(*animation);
		}
	}
	return TVisObjRef();
}

// Confirmed (asm lines 152978-153296): the character's outfits' random, standing,
// talk and walk animations, in that order.
TVisObjRef TGAnimation::GetCharacterAnimationByName(TVisObjRef &character, wxString &name) {
	if (character.IsEmpty())
		return TVisObjRef();

	static const int kAnimationFields[] = {kOutfitRandomAnimations, kOutfitStandingAnimations,
	                                       kOutfitTalkAnimations, kOutfitWalkAnimations};
	TVList outfits;
	TCharHolder wanted(name);

	character.GetLinks(kCharacterOutfits, TypeOrder::kValue0, outfits);
	for (TVisionaireObject *outfitObject : outfits) {
		TVisObjRef outfit(*outfitObject);

		for (int field : kAnimationFields) {
			TVList animations;

			outfit.GetLinks(field, TypeOrder::kValue0, animations);
			for (TVisionaireObject *animation : animations) {
				if (animation->GetName().Cmp(wanted) == 0)
					return TVisObjRef(*animation);
			}
		}
	}
	return TVisObjRef();
}

// Confirmed (asm lines 153297-153355)
TVisObjRef TGAnimation::GetAnimationByName(const wxString &name) {
	TCharHolder wanted(name);

	for (TGAnimation *animation : RunningAnimations) {
		if (animation->_state.GetName().Cmp(wanted) == 0)
			return animation->_state;
	}
	return TVisObjRef();
}

// Confirmed (asm lines 153356-153447)
TVisObjRef TGAnimation::GetAnimationById(int id) {
	for (TGAnimation *animation : RunningAnimations) {
		if (PackVisId(animation->_data.GetId()) == id)
			return animation->_state;
	}
	return TVisObjRef();
}

// Confirmed (asm lines 153448-153486)
void TGAnimation::GetAnimations(TVList &list) {
	list.clear();
	for (TGAnimation *animation : RunningAnimations)
		list.push_back(animation->_state);
}

// Confirmed (asm lines 154134-154171)
void TGAnimation::RegisterEventHandlerAnimStarted(const wxString &handler) {
	EventHandlerAnimStarted = handler;
}

void TGAnimation::RegisterEventHandlerAnimStopped(const wxString &handler) {
	EventHandlerAnimStopped = handler;
}

// Confirmed (asm lines 154172-154251)
wxString TGAnimation::GetEventHandlerAnimStarted() {
	return EventHandlerAnimStarted;
}

wxString TGAnimation::GetEventHandlerAnimStopped() {
	return EventHandlerAnimStopped;
}

// Confirmed (asm lines 156224-156254)
unsigned long TGAnimation::GetAnimationCount() {
	return RunningAnimations.size();
}

// Confirmed (asm lines 156549-156601)
long TGAnimation::GetSizeOfAllAnimations() {
	long size = 0;

	for (TGAnimation *animation : RunningAnimations) {
		for (TPictureIO *sprite : animation->_sprites)
			size += (int)sprite->GetSpriteMemSize();
	}
	return size;
}

// Confirmed (asm lines 156602-156618)
std::list<TGAnimation *> &TGAnimation::GetRunningAnimations() {
	return RunningAnimations;
}
