#include "vscommon/canimation.h"

#include <cstdlib>

#include "Diagnostics.h"
#include "TSprite.h"
#include "datastruct/visionaireobject.h"
#include "graphicslib/graphics.h"
#include "graphicslib/picture.h"
#include "graphicslib/preloadedPicManager.h"
#include "vstables/fieldIds.h"

// TODO (low priority, see /TODO.md): the model/Spine branches of this ctor, the
// dtor, Start(), FirstSprite()/NextSprite()/EofSprite(), SetCurrentSprite(),
// GetCurrentSpriteIndexOrTick(), GetFrameCount() and
// GetCurrentSpritePosition() are all left out.
//
// Confirmed (asm lines 1383514-1384913; the sprite part only). The first
// reference is the TSAnimation that holds the running state, the second the
// TTAnimation to play. The state record takes the name of the animation; the
// pictures are made from the data's sprite list (the sprites of an outfit's
// animations are owned by their pictures).
TCAnimation::TCAnimation(const TVisObjRef &active, const TVisObjRef &animation)
	: _state(active), _data(animation), _currentSprite(nullptr), _isOutfit(false), _paused(false), _stopped(true) {
	_state.SetName(_data.GetName());

	TVisObjRef parent = _data.GetParent();
	if (parent.GetId()[3] == 0x11) {
		_isOutfit = true;
	}

	std::vector<TSprite> sprites;
	_data.GetSprites(kAnimationSprites, sprites);
	for (const TSprite &sprite : sprites)
		_sprites.push_back(new TPictureIO(sprite, _isOutfit));
}

// Confirmed (asm lines 1383360-1383513)
TCAnimation::~TCAnimation() {
	for (TPictureIO *sprite : _sprites)
		delete sprite;
	_sprites.clear();

	_state.Remove();
}

// The wait before the next loop of a random-loop animation: 5 to 20 seconds.
void TCAnimation::startRandomWait() {
	_state.SetValue(kAnimationCurrentSpriteIndex, -1, TSendEventEnum::kNoEvent);
	_state.SetValue(kAnimationWaitingTime, rand() % 15000 + 5000, TSendEventEnum::kNoEvent);
	_state.SetValue(kAnimationWaiting, true, TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 1381238-1381502)
void TCAnimation::Start(bool reverse, float scale) {
	int loops = _data.GetInt(kAnimationNumberOfLoops);

	if (loops < 0) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"The animation '%ls' (id: %d) has a negative loop count (%d)",
			                   _data.GetName().c_str().c_str(), _data.GetObjectPointer()->GetId24(), loops);
		loops = 0;
	}

	_state.SetValue(kAnimationLoops, loops, TSendEventEnum::kNoEvent);
	_state.SetValue(kPlayOppositeDirection, (_data.GetInt(kAnimationReplay) == 1) != reverse, TSendEventEnum::kNoEvent);
	_state.SetValue(kAnimationCurrentPosition, *_data.GetPoint(kAnimationPosition), TSendEventEnum::kNoEvent);
	_state.SetValue(kAnimationSize, scale, TSendEventEnum::kNoEvent);
	_state.SetValue(kAnimationCurrentSpriteIndex, -1, TSendEventEnum::kNoEvent);

	_currentSprite = nullptr;
	_stopped = false;
	_state.SetValue(kAnimationActive, true, TSendEventEnum::kNoEvent);

	if (_data.GetBool(kAnimationLoopRandom)) {
		_state.SetValue(kAnimationWaitingTime, rand() % 15000 + 5000, TSendEventEnum::kNoEvent);
		_state.SetValue(kAnimationWaiting, true, TSendEventEnum::kNoEvent);
	} else {
		_state.SetValue(kAnimationWaiting, false, TSendEventEnum::kNoEvent);
	}

	_state.SetValue(kFrameCount, 0, TSendEventEnum::kNoEvent);
	_state.SetValue(kAnimationFirstFrame, 1, TSendEventEnum::kNoEvent);
	_state.SetValue(kAnimationLastFrame, (int)_sprites.size(), TSendEventEnum::kNoEvent);

	_timer.SetTime();
}

// Confirmed (asm lines 1381192-1381237)
bool TCAnimation::CanRemoveCurrentSprite() const {
	if (!_currentSprite)
		return false;
	if (GetAnimationLoops() != 1)
		return false;
	if (_state.GetInt(kAnimationCurrentSpriteIndex) == -1)
		return false;
	return _data.GetInt(kAnimationReplay) != 2;
}

// Confirmed (asm lines 1384914-1384917)
void TCAnimation::NextSpriteSelected() const {
}

// Confirmed (asm lines 1381174-1381191)
int TCAnimation::GetAnimationLoops() const {
	return _state.GetInt(kAnimationLoops);
}

// Confirmed (asm lines 158833-158837)
bool TCAnimation::WaitBetweenLoops() const {
	return true;
}

// Confirmed (asm lines 1381542-1381577)
void TCAnimation::RefreshSprites() {
	for (TPictureIO *sprite : _sprites)
		sprite->RefreshSprite(false);
}

// Confirmed (asm lines 1381578-1381644): each picture gets a priority (the first
// one only, as 999, when not all of them are wanted) and is handed to the
// preloader.
void TCAnimation::PreloadSprites(bool all) {
	if (_sprites.empty())
		return;

	if (all) {
		int priority = 0;
		for (TPictureIO *sprite : _sprites) {
			sprite->SetPreloadPriority(priority++);
			graphics->GetPreloadedPicManager()->PreloadPicture(sprite);
		}
	} else {
		for (TPictureIO *sprite : _sprites) {
			sprite->SetPreloadPriority(999);
			graphics->GetPreloadedPicManager()->PreloadPicture(sprite);
		}
	}
}

// Confirmed (asm lines 1381645-1381680)
void TCAnimation::RemoveSprites() {
	for (TPictureIO *sprite : _sprites)
		sprite->RemoveSprite();
}

// Confirmed (asm lines 1381681-1381908; the sprite part)
void TCAnimation::FirstSprite() {
	int first = _state.GetInt(kAnimationFirstFrame);
	int last = _state.GetInt(kAnimationLastFrame);
	int lower = (last <= first) ? last : first;
	int index;

	if (_data.GetInt(kAnimationReplay) == 2) {
		// random order
		index = -1;
		if (!_sprites.empty()) {
			index += last;
			if (last != lower)
				index = lower + rand() % (last - lower) - 1;
		}
		_state.SetValue(kFrameCount, 0, TSendEventEnum::kNoEvent);
	} else {
		index = last - 1;
		if (!_state.GetBool(kPlayOppositeDirection))
			index = lower - 1;
	}

	if (index < 0)
		index = -1;
	_state.SetValue(kAnimationCurrentSpriteIndex, index, TSendEventEnum::kNoEvent);

	NextSpriteSelected();
}

// Confirmed (asm lines 1381789-1381908; the sprite part)
void TCAnimation::NextSprite() {
	int current = _state.GetInt(kAnimationCurrentSpriteIndex);
	int index;

	if (_data.GetInt(kAnimationReplay) == 2) {
		// random order: a different sprite than the current one
		int first = _state.GetInt(kAnimationFirstFrame);
		int last = _state.GetInt(kAnimationLastFrame);
		int lower = (last <= first) ? last : first;
		int count = last - lower + 1;

		if (count <= 1) {
			index = lower - 1;
		} else {
			do {
				index = lower + rand() % count - 1;
			} while (current == index);
		}
		_state.SetValue(kFrameCount, _state.GetInt(kFrameCount) + 1, TSendEventEnum::kNoEvent);
	} else {
		index = _state.GetBool(kPlayOppositeDirection) ? current - 1 : current + 1;
	}

	_state.SetValue(kAnimationCurrentSpriteIndex, index, TSendEventEnum::kNoEvent);

	NextSpriteSelected();
}

// Confirmed (asm lines 1381909-1382017; the sprite part)
bool TCAnimation::EofSprite() const {
	int first = _state.GetInt(kAnimationFirstFrame);
	int last = _state.GetInt(kAnimationLastFrame);
	int lower = (last <= first) ? last : first;

	if (_data.GetInt(kAnimationReplay) == 2)
		return _state.GetInt(kFrameCount) == last - lower + 1;

	int index = _state.GetInt(kAnimationCurrentSpriteIndex);
	if (last <= index)
		return true;
	return index < lower - 1;
}

// The pause of the current sprite: its own if the animation uses individual
// pauses and the sprite has one, else the animation's.
int TCAnimation::currentPause() const {
	int index = _state.GetInt(kAnimationCurrentSpriteIndex);

	if (_data.GetBool(kAnimationUseIndividualPause) && index >= 0 && index < (int)_sprites.size()) {
		int pause = _sprites[index]->GetPause();

		if (pause != -1)
			return pause;
	}
	return _data.GetInt(kAnimationPause);
}

// Confirmed (asm lines 1382018-1382080)
int TCAnimation::GetCurrentPause() const {
	return currentPause();
}

// Confirmed (asm lines 1382081-1382161)
float TCAnimation::GetPauseCompletion() const {
	int pause = currentPause();
	long elapsed = _timer.GetTime();

	if (pause == 0)
		return 0.0f;
	if (elapsed > pause)
		return 1.0f;
	return (float)elapsed / (float)pause;
}

// Confirmed (asm lines 1382162-1382689; the sprite part). Called every frame
// for a running animation.
void TCAnimation::SetCurrentSprite(bool force) {
	if (!_state.GetBool(kAnimationActive))
		return;

	// between the loops of a random-loop animation: wait
	if (_data.GetBool(kAnimationLoopRandom) && WaitBetweenLoops() && _state.GetBool(kAnimationWaiting)) {
		if (_timer.GetTime() < _state.GetInt(kAnimationWaitingTime)) {
			_currentSprite = nullptr;
			return;
		}

		_state.SetValue(kAnimationWaiting, false, TSendEventEnum::kNoEvent);
		_timer.SetTime();
	}

	int index = _state.GetInt(kAnimationCurrentSpriteIndex);
	int pause = currentPause();

	// the current sprite is still shown (a pause of -1 holds it for good)
	if (index != -1 && (_timer.GetTime() < pause || pause < 0) && !force)
		return;

	int loops = GetAnimationLoops();

	// (The original does not check for a current sprite here; TGAnimation's
	// CanRemoveCurrentSprite() does not either, so guard it.)
	if (CanRemoveCurrentSprite() && _currentSprite)
		_currentSprite->RemoveSprite();

	_timer.SetTime();
	if (index == -1)
		FirstSprite();
	else
		NextSprite();

	if (EofSprite()) {
		if (loops == 0) {
			// endless
			if (_data.GetBool(kAnimationLoopRandom)) {
				startRandomWait();
				_state.SetValue(kAnimationWaiting, true, TSendEventEnum::kNoEvent);
				_timer.SetTime();
			} else {
				FirstSprite();
			}
		} else if (loops == 1) {
			// the last loop is over
			_state.SetValue(kAnimationActive, false, TSendEventEnum::kNoEvent);
		} else {
			if (_data.GetBool(kAnimationLoopRandom)) {
				startRandomWait();
				_state.SetValue(kAnimationWaiting, true, TSendEventEnum::kNoEvent);
				_timer.SetTime();
			} else {
				FirstSprite();
			}
			_state.SetValue(kAnimationLoops, loops - 1, TSendEventEnum::kNoEvent);
		}
	}

	_currentSprite = nullptr;
	if (_state.GetBool(kAnimationActive)) {
		int current = _state.GetInt(kAnimationCurrentSpriteIndex);

		if (current >= 0 && current < (int)_sprites.size())
			_currentSprite = _sprites[current];
	}
}

// Confirmed (asm lines 1382707-1382723)
int TCAnimation::GetCurrentSpriteIndex() const {
	return _state.GetInt(kAnimationCurrentSpriteIndex);
}

// Confirmed (asm lines 1382724-1382749): (a model animation's tick instead)
int TCAnimation::GetCurrentSpriteIndexOrTick() const {
	return _state.GetInt(kAnimationCurrentSpriteIndex);
}

// Confirmed (asm lines 1382750-1382794): the number of sprites (a model's ticks /
// a skeleton's duration otherwise)
int TCAnimation::GetFrameCount() const {
	return (int)_sprites.size();
}

// Confirmed (asm lines 1382795-1382839)
void TCAnimation::SetPosition(const wxPoint &position, float scale) {
	_state.SetValue(kAnimationCurrentPosition, position, TSendEventEnum::kNoEvent);
	if (scale != -1.0f)
		_state.SetValue(kAnimationSize, scale, TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 1382840-1383015; the sprite part): the offset of the
// sprite (relative to the animation's centre; a mirrored one is mirrored around
// it), scaled and moved to the animation's position.
wxPoint TCAnimation::GetCurrentSpritePosition() const {
	wxPoint result;

	x_assert(_currentSprite != nullptr, "m_pSprite != NULL", "/home/simon/Documents/jenkins/branchPillars/src/vscommon/canimation.cpp", 0x246);
	if (!_currentSprite)
		return result;

	_currentSprite->RefreshSprite(false);

	if (_currentSprite->IsMirrored()) {
		const wxPoint *center = _data.GetPoint(kAnimationCenter);
		wxPoint position = _currentSprite->GetPosition();

		result.x = center->x - (position.x + _currentSprite->GetWidth());
		result.y = position.y - center->y;
	} else {
		const wxPoint *center = _data.GetPoint(kAnimationCenter);

		result.x = -center->x;
		result.y = -center->y;
		wxPoint position = _currentSprite->GetPosition();
		result.x += position.x;
		result.y += position.y;
	}

	float scale = _state.GetFloat(kAnimationSize);
	if (scale != 100.0f) {
		scale /= 100.0f;
		result.x = (int)(result.x * scale + (result.x < 0 ? -0.5 : 0.5));
		result.y = (int)(result.y * scale + (result.y < 0 ? -0.5 : 0.5));
	}

	const wxPoint *current = _state.GetPoint(kAnimationCurrentPosition);
	result.x += current->x;
	result.y += current->y;
	return result;
}

// Confirmed (asm lines 1383016-1383031)
long TCAnimation::GetCalledTime() const {
	return _timer.GetTime();
}

// Confirmed (asm lines 1383032-1383080)
bool TCAnimation::IsSpriteIndexValid() const {
	int index = _state.GetInt(kAnimationCurrentSpriteIndex);

	return index >= 0 && index < (int)_sprites.size();
}

bool TCAnimation::IsRandomOrder() const {
	return _data.GetInt(kAnimationReplay) == 2;
}

bool TCAnimation::IsLoopRandom() const {
	return _data.GetBool(kAnimationLoopRandom);
}

bool TCAnimation::IsFinished() const {
	return !_state.GetBool(kAnimationActive);
}

bool TCAnimation::IsWaiting() const {
	return _state.GetBool(kAnimationWaiting);
}

bool TCAnimation::IsEndless() const {
	return _state.GetInt(kAnimationLoops) == 0;
}

bool TCAnimation::IsMoveAnimation() const {
	return _data.GetBool(kAnimationMove);
}
