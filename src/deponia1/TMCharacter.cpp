#include "TMCharacter.h"

#include <cstring>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "TGDetectInfo.h"
#include "TGScene.h"
#include "TTText.h"
#include "graphicslib/picture.h"
#include "vscommon/canimation.h"
#include "vsplayer/animationGame.h"
#include "vsplayer/control/gameControl.h"
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

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

// Confirmed (asm lines 138081-138143)
bool TMCharacter::IsInside(const wxPoint &position, const TGDetectInfo &info) const {
	if (!info.flagB)
		return false;
	if (std::memcmp(info.character.GetId(), _objRef.GetId(), 4) == 0)
		return false;
	return IsInside(position);
}

// Confirmed (asm lines 138144-138399). A character that is drawn through a matrix (kCharacterMatrixId 1,
// while invMatrix1 is set) has the point moved back through it first, round the scroll position of the scene.
bool TMCharacter::IsInside(const wxPoint &point) const {
	if (!_active)
		return false;

	wxPoint position = point;

	if (hasInverseMatrix() && _objRef.GetInt(kCharacterMatrixId) == 1) {
		position -= gameControl()->GetScene()->GetScrollPos();
		position = transformByInverseMatrix(position);
		position += gameControl()->GetScene()->GetScrollPos();
	}

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
