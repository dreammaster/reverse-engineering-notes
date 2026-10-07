#include "TMButton.h"

#include <algorithm>

#include "datastruct/vlist.h"
#include "vsplayer/animationGame.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"

// Confirmed (asm lines 165837-166013): the pictures take the sprites of the two sprite
// records of the button, with the draw settings of the button (rotation, rotation centre,
// scale, matrix and shader); the button is shown by what its condition says.
TMButton::TMButton(const TVisObjRef &ref) : TManagedObject(ref) {
	std::vector<wxPoint> polygon;

	_objRef.GetPoints(kButtonPolygon, polygon);
	SetPolygon(polygon);

	_bypassReachCheck = true;
	_skipFinalPostExecution = true;
	_hasActionTypeFallback = false;

	_activePicture.Set(_objRef.GetLink(kButtonActiveSprite).GetSprite(kSpriteSprite));
	_inactivePicture.Set(_objRef.GetLink(kButtonInactiveSprite).GetSprite(kSpriteSprite));

	TPictureIO *pictures[2] = {&_inactivePicture, &_activePicture};

	for (TPictureIO *picture : pictures) {
		picture->SetRotation(_objRef.GetFloat(kButtonRotation));
		picture->SetRotationCenter(*_objRef.GetPoint(kButtonRotationCenter));
		picture->SetScaleX(_objRef.GetFloat(kButtonScaleX));
		picture->SetScaleY(_objRef.GetFloat(kButtonScaleY));
		picture->SetMatrixId(_objRef.GetInt(kButtonMatrixId));
		picture->SetShader(_objRef.GetInt(kButtonShaderSet));
	}

	_picture = &_inactivePicture;
	_active = false;

	TTCondition condition(_objRef.GetLink(kButtonCondition));

	TMButton::SetActive(_objRef.GetBool(kButtonConditionNegate) != condition.IsTrue());
}

// Confirmed (asm lines 165577-165586)
void TMButton::SetActiveSprite(bool active) {
	_picture = active ? &_activePicture : &_inactivePicture;
}

// Confirmed (asm lines 165598-165616)
void TMButton::RemoveSprites() {
	_activePicture.RemoveSprite();
	_inactivePicture.RemoveSprite();

	if (_currentAnimation)
		TGAnimation::HideAnimation(_currentAnimation, this);

	_currentAnimation = nullptr;
}

// Confirmed (asm lines 165628-165697)
void TMButton::SetActive(bool active) {
	if (_active == active)
		return;

	if (active) {
		if (_currentAnimation)
			TGAnimation::HideAnimation(_currentAnimation, this);

		_currentAnimation = nullptr;

		TVisObjRef animation = _objRef.GetLink(kButtonAnimation);

		if (!animation.IsEmpty())
			_currentAnimation = TGAnimation::StartAnimation(animation, this, false, 100.0f, -1);
	} else {
		if (_currentAnimation)
			TGAnimation::HideAnimation(_currentAnimation, this);

		_activePicture.RemoveSprite();
		_inactivePicture.RemoveSprite();
		_currentAnimation = nullptr;
	}

	_active = active;
}

// Confirmed (asm lines 165712-165719)
void TMButton::GetActionList(TVList &actions) const {
	_objRef.GetList(kButtonActions, actions);
}

// Confirmed (asm lines 166172-166290): TManagedObject::SetAnimation() but for the button's
// own animation field.
void TMButton::SetAnimation(TGAnimation *animation) {
	if (!animation)
		return;

	if (_objRef.GetLink(kButtonAnimation) == animation->GetDataObject()) {
		if (_currentAnimation && _currentAnimation != animation)
			TGAnimation::HideAnimation(_currentAnimation, this);

		_currentAnimation = animation;
		return;
	}

	if (std::find(_animations.begin(), _animations.end(), animation) == _animations.end())
		_animations.push_back(animation);
}

// Confirmed (asm lines 166091-166153)
void TMButton::StartAnimation() {
	if (!IsActive())
		return;

	TVisObjRef animation = _objRef.GetLink(kButtonAnimation);

	if (animation.IsEmpty() || _currentAnimation)
		return;

	SetActive(false);
	SetActive(true);
}
