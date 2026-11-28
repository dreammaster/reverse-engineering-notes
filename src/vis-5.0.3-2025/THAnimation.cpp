#include "THAnimation.h"

#include "AppGlobals.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 260077-260106)
THAnimation::THAnimation(const TVisObjRef &active, const TVisObjRef &animation)
	: TGAnimation(active, animation) {
	_state.RegisterEventHandler(this, TEventEnum::kChanged);
}

// Confirmed (asm lines 259729-259812)
THAnimation::~THAnimation() {
	_state.UnRegisterEventHandler(this);
}

// Confirmed (asm lines 259813-260059)
void THAnimation::OnEvent(TEventEnum /*event*/, int field, TVisionaireObject * /*object*/) {
	int count = (int)_sprites.size();

	switch (field) {
	case kAnimationActive:
		if (!_state.GetBool(kAnimationActive))
			return;

		if ((g_traceFlags & 1) && wxLog::loglevel > 1)
			wxLog::logexpanded(L"Activating preloaded animation: \"%s\" (Id: %d)", _state.GetName().c_str().c_str(),
			                   PackVisId(_data.GetId()));

		Start(false, 100.0f);
		SetCurrentSprite(false);
		break;

	case kAnimationFirstFrame: {
		int first = _state.GetInt(kAnimationFirstFrame);

		if (first <= 0)
			_state.SetValue(kAnimationFirstFrame, 1, TSendEventEnum::kNoEvent);
		else if (first > count)
			_state.SetValue(kAnimationFirstFrame, count, TSendEventEnum::kNoEvent);
		break;
	}

	case kAnimationLastFrame: {
		int last = _state.GetInt(kAnimationLastFrame);

		if (last > count)
			_state.SetValue(kAnimationLastFrame, count, TSendEventEnum::kNoEvent);
		else if (last <= 0)
			_state.SetValue(kAnimationLastFrame, 1, TSendEventEnum::kNoEvent);
		break;
	}

	default:
		break;
	}
}
