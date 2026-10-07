#include "THObject.h"

#include <cwchar>
#include <string>

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
	for (TVisionaireObject *item : _conditions)
		TVisObjRef(item).UnRegisterEventHandler(this);

	_conditions.clear();
}

// Adds "'name' (id: n)" and the suffix to the text of the way down to a condition.
static void appendCondition(std::wstring &text, const TVisObjRef &condition, const wchar_t *suffix) {
	std::wstring name = condition.GetName().c_str().ToStdWstring();
	wchar_t number[16];

	swprintf(number, 16, L"%d", PackVisId(condition.GetId()));
	text += L"'" + name + L"' (id: " + number + L")" + suffix;
}

// Confirmed (asm lines 113666-114205): a condition that is a variable is listened to; one
// that is made of two others lists them (one level at a time: a condition that is already
// on the way down is a cycle, which is logged).
void THObject::RegisterConditions(TVisObjRef &condition, TVList &path) {
	if (condition.GetBool(kConditionIsVariable)) {
		condition.RegisterEventHandler(this, TEventEnum::kChanged);
		_conditions.push_back(condition);
		return;
	}

	for (TVisionaireObject *item : path) {
		if (condition == *item) {
			if (wxLog::loglevel > 0) {
				// the way down to it
				std::wstring chain;

				for (TVisionaireObject *step : path)
					appendCondition(chain, TVisObjRef(step), L" - ");

				appendCondition(chain, condition, L"");

				TVisObjRef first(path.front());

				wxLog::logexpanded(L"Condition '%s' (id: %d) has a cyclic reference: %s", first.GetName().c_str().wc_str(),
				                   PackVisId(first.GetId()), wxString(chain).wc_str());
			}
			return;
		}
	}

	path.push_back(condition);

	TVisObjRef first = condition.GetLink(kConditionCondition1);
	TVisObjRef second = condition.GetLink(kConditionCondition2);

	if (!first.IsEmpty())
		RegisterConditions(first, path);
	if (!second.IsEmpty())
		RegisterConditions(second, path);

	path.pop_back();
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
		_scaleY = _objRef.GetFloat(kObjectScaleY);
		break;
	case kObjectScaleX:
		_scaleX = _objRef.GetFloat(kObjectScaleX);
		break;
	case kObjectRotationCenter:
		_rotationCenter = *_objRef.GetPoint(kObjectRotationCenter);
		// (the original goes on to read the matrix id too)
		_matrixId = _objRef.GetInt(kObjectMatrixId);
		break;
	case kObjectMatrixId:
		_matrixId = _objRef.GetInt(kObjectMatrixId);
		break;
	case kObjectRotation:
		_rotation = _objRef.GetFloat(kObjectRotation);
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
