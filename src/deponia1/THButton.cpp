#include "THButton.h"

#include "ConditionListener.h"
#include "TGInterface.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 115901-115969)
THButton::THButton(const TVisObjRef &ref, TGInterface *owner) : TMButton(ref), _owner(owner) {
	if (!_objRef.IsEmpty())
		_objRef.RegisterEventHandler(this, TEventEnum::kChanged);

	TVisObjRef condition = _objRef.GetLink(kButtonCondition);

	if (!condition.IsEmpty()) {
		TVList path;

		RegisterConditions(condition, path);
	}
}

// Confirmed (asm lines 114929-114987)
THButton::~THButton() {
	if (!_objRef.IsEmpty())
		_objRef.UnRegisterEventHandler(this);

	UnRegisterConditions();
}

// Confirmed (asm lines 115101-115640): see ConditionListener.h.
void THButton::RegisterConditions(TVisObjRef &condition, TVList &path) {
	RegisterConditionHandlers(this, condition, path, _conditions);
}

// Confirmed (asm lines 116008-116039)
void THButton::UnRegisterConditions() {
	UnRegisterConditionHandlers(this, _conditions);
}

// Confirmed (asm lines 115647-115870). Which field changed is all that is given: the new
// value is read from the data and given to both pictures.
void THButton::OnEvent(TEventEnum /*event*/, int field, TVisionaireObject * /*object*/) {
	TPictureIO *pictures[2] = {&_activePicture, &_inactivePicture};

	switch (field) {
	case kButtonShaderSet:
		for (TPictureIO *picture : pictures)
			picture->SetShader(_objRef.GetInt(kButtonShaderSet));
		break;
	case kButtonRotation:
		for (TPictureIO *picture : pictures)
			picture->SetRotation(_objRef.GetFloat(kButtonRotation));
		break;
	case kButtonRotationCenter:
		for (TPictureIO *picture : pictures)
			picture->SetRotationCenter(*_objRef.GetPoint(kButtonRotationCenter));
		// (the original goes on to read the matrix id too)
		for (TPictureIO *picture : pictures)
			picture->SetMatrixId(_objRef.GetInt(kButtonMatrixId));
		break;
	case kButtonMatrixId:
		for (TPictureIO *picture : pictures)
			picture->SetMatrixId(_objRef.GetInt(kButtonMatrixId));
		break;
	case kButtonScale:
		// (one scale for both directions)
		_objRef.SetValue(kButtonScaleX, _objRef.GetFloat(kButtonScale), TSendEventEnum::kSendEvent);
		_objRef.SetValue(kButtonScaleY, _objRef.GetFloat(kButtonScale), TSendEventEnum::kSendEvent);
		break;
	case kButtonScaleX:
		for (TPictureIO *picture : pictures)
			picture->SetScaleX(_objRef.GetFloat(kButtonScaleX));
		break;
	case kButtonScaleY:
		for (TPictureIO *picture : pictures)
			picture->SetScaleY(_objRef.GetFloat(kButtonScaleY));
		break;
	case kConditionValue:
		// the value of a variable of the condition changed: the interface has to show or hide it
		if (_owner)
			_owner->SetObjectsActive(true);
		break;
	case kButtonCondition: {
		// the condition itself was replaced
		UnRegisterConditions();

		TVisObjRef condition = _objRef.GetLink(kButtonCondition);

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
