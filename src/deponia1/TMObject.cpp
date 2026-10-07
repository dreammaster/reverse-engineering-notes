#include "TMObject.h"

#include "Diagnostics.h"
#include "TSprite.h"
#include "TTText.h"
#include "datastruct/vlist.h"
#include "vsplayer/animationGame.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/objectManaged.cpp";

// Confirmed (asm lines 116424-116523): the sprite of the object's sprite record is
// shown at its own position moved by the object's offset; an object starts inactive.
TMObject::TMObject(const TVisObjRef &ref, bool animated) : TManagedObject(ref) {
	TVisObjRef spriteRecord = _objRef.GetLink(kObjectSprite);

	_sprite.Set(spriteRecord.GetSprite(kSpriteSprite));
	_spritePosition = _sprite.GetPosition();
	_sprite.SetPosition(_spritePosition + *_objRef.GetPoint(kObjectOffset), -1.0f);

	_picture = &_sprite;
	_active = false;
	_currentAnimation = nullptr;
	_animated = animated || spriteRecord.IsEmpty();

	int visibility = std::max(std::min(_objRef.GetInt(kObjectVisibility), 100), 0);

	_alpha = (float)visibility / 100.0f;
	SetDestAlpha(_objRef.GetInt(kObjectDestVisibility), _objRef.GetInt(kObjectTimeToDestVisibility));
}

// Confirmed (asm lines 116072-116081)
void TMObject::SetAlpha() {
	_objRef.SetValue(kObjectVisibility, (int)(100.0f * _alpha), TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 116093-116100)
void TMObject::GetActionList(TVList &actions) const {
	_objRef.GetList(kObjectActions, actions);
}

// Confirmed (asm lines 116315-116349)
wxString TMObject::GetLanguageName() const {
	TTText name(_objRef.GetLink(kObjectName));

	return name.GetTextString();
}

// Confirmed (asm lines 116112-116263)
void TMObject::SetActive(bool active) {
	if (_active == active)
		return;

	if (!active) {
		if (_currentAnimation)
			TGAnimation::HideAnimation(_currentAnimation, this);

		_sprite.RemoveSprite();
		_currentAnimation = nullptr;
		_active = active;
		return;
	}

	x_assert(_currentAnimation == nullptr, "m_pAnimationLink == NULL", kSourceFile, 0x47);

	if (_currentAnimation)
		TGAnimation::HideAnimation(_currentAnimation, this);

	_currentAnimation = nullptr;

	// the animation the object shows is the one of its list that its data names
	TVisObjRef animation = _objRef.GetLink(kObjectAnimation);

	if (_animated && !animation.IsEmpty()) {
		TVList animations;

		_objRef.GetLinks(kObjectAnimations, TypeOrder::kValue0, animations);

		for (TVisionaireObject *item : animations) {
			if (animation == *item) {
				_currentAnimation = TGAnimation::StartAnimation(animation, this, false, 100.0f, -1);
				break;
			}
		}
	}

	_active = active;
}
