#include "THObject.h"

#include "ConditionListener.h"

#include "AppGlobals.h"
#include "datastruct/visionaireobject.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

// Confirmed (asm lines 114616-114682)
THObject::THObject(const TVisObjRef &ref) : TGObject(ref) {
	if (!_objRef.IsEmpty())
		_objRef.RegisterEventHandler(this, TEventEnum::kChanged);

	TVisObjRef condition = _objRef.GetLink(kObjectCondition);

	if (!condition.IsEmpty()) {
		TVList path;

		RegisterConditions(condition, path);
	}
}

// Confirmed (asm lines 113494-113553)
THObject::~THObject() {
	if (!_objRef.IsEmpty())
		_objRef.UnRegisterEventHandler(this);

	UnRegisterConditions();
}

// Confirmed (asm lines 114721-114752)
void THObject::UnRegisterConditions() {
	UnRegisterConditionHandlers(this, _conditions);
}

// Confirmed (asm lines 113666-114205): see ConditionListener.h.
void THObject::RegisterConditions(TVisObjRef &condition, TVList &path) {
	RegisterConditionHandlers(this, condition, path, _conditions);
}

// Confirmed (asm lines 114212-114585). Which field changed is all that is given: the new
// value is read from the data.
void THObject::OnEvent(TEventEnum /*event*/, int field, TVisionaireObject * /*object*/) {
	switch (field) {
	case kObjectDestVisibility:
		SetDestAlpha(_objRef.GetInt(kObjectDestVisibility), _objRef.GetInt(kObjectTimeToDestVisibility));
		break;
	case kObjectVisibility:
		SetDestAlpha(_objRef.GetInt(kObjectVisibility), 0);
		break;
	case kObjectPosition:
		_position = *_objRef.GetPoint(kObjectPosition);
		break;
	case kObjectOffset:
		_sprite.SetPosition(_spritePosition + *_objRef.GetPoint(kObjectOffset), -1.0f);
		break;
	case kObjectScrollFactorX:
	case kObjectScrollFactorY:
		// (a negative factor is set to 0, also in the data)
		_scrollX = _objRef.GetInt(kObjectScrollFactorX);

		if (_scrollX < 0) {
			_scrollX = 0;
			_objRef.SetValue(kObjectScrollFactorX, 0, TSendEventEnum::kNoEvent);
		}

		_scrollY = _objRef.GetInt(kObjectScrollFactorY);

		if (_scrollY < 0) {
			_scrollY = 0;
			_objRef.SetValue(kObjectScrollFactorY, 0, TSendEventEnum::kNoEvent);
		}

		_scrolls = (_scrollX != 100 || _scrollY != 100);
		_scrollX -= 100;
		_scrollY -= 100;
		_sprite.SetParallax(_scrollX, _scrollY);

		if (_currentAnimation)
			_currentAnimation->SetParallax(_scrollX, _scrollY);
		break;
	case kObjectScale:
		// (one scale for both directions)
		_objRef.SetValue(kObjectScaleX, _objRef.GetFloat(kObjectScale), TSendEventEnum::kSendEvent);
		_objRef.SetValue(kObjectScaleY, _objRef.GetFloat(kObjectScale), TSendEventEnum::kSendEvent);
		break;
	case kObjectScaleY:
	case kObjectScaleX:
		_sprite.SetScale(_objRef.GetFloat(kObjectScaleX), _objRef.GetFloat(kObjectScaleY));
		break;
	case kObjectRotationCenter:
		_sprite.SetRotationCenter(*_objRef.GetPoint(kObjectRotationCenter));
		// (the original goes on to read the matrix id too)
		_sprite.SetMatrixId(_objRef.GetInt(kObjectMatrixId));
		break;
	case kObjectMatrixId:
		_sprite.SetMatrixId(_objRef.GetInt(kObjectMatrixId));
		break;
	case kObjectRotation:
		_sprite.SetRotation(_objRef.GetFloat(kObjectRotation));
		break;
	case kConditionValue: {
		// the value of a variable of the condition changed
		TTCondition condition(_objRef.GetLink(kObjectCondition));

		SetActive(_objRef.GetBool(kObjectConditionNegate) != condition.IsTrue());
		gameControl()->UpdateCurrentObject();
		break;
	}
	case kObjectCondition: {
		// the condition itself was replaced
		UnRegisterConditions();

		TVisObjRef condition = _objRef.GetLink(kObjectCondition);

		if (!condition.IsEmpty()) {
			TVList path;

			RegisterConditions(condition, path);
		}
		break;
	}
	default:
		break;
	}
}
