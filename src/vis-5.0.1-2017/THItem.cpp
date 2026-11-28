#include "THItem.h"

#include "vstables/fieldIds.h"

// Confirmed (asm lines 219148-219190)
THItem::THItem(const TVisObjRef &ref, bool animated) : TGItem(ref, animated) {
	if (!_objRef.IsEmpty())
		_objRef.RegisterEventHandler(this, TEventEnum::kChanged);
}

// Confirmed (asm lines 219048-219083)
THItem::~THItem() {
	if (!_objRef.IsEmpty())
		_objRef.UnRegisterEventHandler(this);
}

// Confirmed (asm lines 218884-219020). Which field changed is all that is given: the new
// value is read from the data.
void THItem::OnEvent(TEventEnum /*event*/, int field, TVisionaireObject * /*object*/) {
	switch (field) {
	case kObjectScale:
		// (one scale for both directions)
		_objRef.SetValue(kObjectScaleX, _objRef.GetFloat(kObjectScale), TSendEventEnum::kSendEvent);
		_objRef.SetValue(kObjectScaleY, _objRef.GetFloat(kObjectScale), TSendEventEnum::kSendEvent);
		break;
	case kObjectScaleX:
		_sprite.SetScaleX(_objRef.GetFloat(kObjectScaleX));
		break;
	case kObjectScaleY:
		_sprite.SetScaleY(_objRef.GetFloat(kObjectScaleY));
		break;
	case kObjectVisibility: {
		// the visibility is shown at once (a fade that was going on ends); it is kept
		// between 0 and 100, also in the data
		int visibility = _objRef.GetInt(kObjectVisibility);

		if (visibility < 0) {
			visibility = 0;
			_objRef.SetValue(kObjectVisibility, 0, TSendEventEnum::kNoEvent);
		} else if (visibility > 100) {
			visibility = 100;
			_objRef.SetValue(kObjectVisibility, 100, TSendEventEnum::kNoEvent);
		}

		_alpha = (float)visibility / 100.0f;
		_alphaTarget = _alpha;
		break;
	}
	case kObjectDestVisibility:
		SetDestAlpha(_objRef.GetInt(kObjectDestVisibility), _objRef.GetInt(kObjectTimeToDestVisibility));
		break;
	default:
		break;
	}
}
