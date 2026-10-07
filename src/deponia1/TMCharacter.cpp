#include "TMCharacter.h"

#include <cstring>

#include "Diagnostics.h"
#include "TGDetectInfo.h"
#include "TTText.h"
#include "graphicslib/picture.h"
#include "vscommon/canimation.h"
#include "vsplayer/animationGame.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/characterManaged.cpp";

// Confirmed (asm lines 138489-138501)
TMCharacter::TMCharacter(const TVisObjRef &ref) : TManagedObject(ref) {
	_bypassReachCheck = false;
	_skipFinalPostExecution = false;
}

// Confirmed (asm lines 137965-137980)
int TMCharacter::GetCenter() const {
	return _center;
}

// Confirmed (asm lines 137981-137999)
void TMCharacter::GetActionList(TVList &actions) const {
	_objRef.GetList(kCharacterActions, actions);
}

// Confirmed (asm lines 138000-138017)
int TMCharacter::GetDirection() const {
	return _objRef.GetInt(kCharacterDirection);
}

// Confirmed (asm lines 138422-138488)
wxString TMCharacter::GetLanguageName() const {
	TTText name(_objRef.GetLink(kCharacterName));

	return name.GetTextString();
}

// Confirmed (asm lines 138018-138063)
void TMCharacter::SetAnimation(TGAnimation *animation) {
	x_assert(_currentAnimation == nullptr, "m_pAnimationLink == NULL", kSourceFile, 0x32);

	if (_currentAnimation && _currentAnimation != animation)
		TGAnimation::HideAnimation(_currentAnimation, this);
	_currentAnimation = animation;
}

// Confirmed (asm lines 138081-138143)
bool TMCharacter::IsInside(const wxPoint &position, const TGDetectInfo &info) const {
	if (!info.flagB)
		return false;
	if (std::memcmp(info.character.GetId(), _objRef.GetId(), 4) == 0)
		return false;
	return IsInside(position);
}

// Confirmed (asm lines 138144-138399). TODO (low priority, see /TODO.md): while the
// scene is drawn through a matrix (a character with kCharacterMatrixId 1 and an inverse
// matrix set), the original first moves the point back through that matrix; not
// reconstructed.
bool TMCharacter::IsInside(const wxPoint &position) const {
	if (!_active)
		return false;

	wxRect rect = GetCurrentSpriteRect();

	if (!rect.Contains(position))
		return false;

	// the sprite shown: the animation's current one, or the character's own picture
	TPictureIO *sprite = _picture;

	if (_currentAnimation && _currentAnimation->IsSpriteIndexValid())
		sprite = _currentAnimation->GetCurrentSprite();

	// (a character without a sprite, or an empty one, is hit anywhere in its rectangle)
	if (!sprite || sprite->IsEmpty())
		return true;

	// the point in the unscaled sprite, to look at its pixel
	wxPoint delta = position - wxPoint{rect.GetLeft(), rect.GetTop()};
	float scale = sprite->GetSize() / 100.0f;
	wxPoint inSprite = {(int)((float)delta.x / scale), (int)((float)delta.y / scale)};

	return !sprite->IsTransparent(inSprite);
}
