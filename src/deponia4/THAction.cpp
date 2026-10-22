#include "THAction.h"

// Confirmed (asm lines 187127-187150)
THAction::THAction(const TVisObjRef &active, const TVisObjRef &data) : TGAction(active, data) {
	_active.RegisterEventHandler(this, TEventEnum::kChanged);
}

// Confirmed (asm lines 187043-187065)
THAction::~THAction() {
	_active.UnRegisterEventHandler(this);
}

// Confirmed (asm lines 187010-187014): nothing is done.
void THAction::OnEvent(TEventEnum /*event*/, int /*field*/, TVisionaireObject * /*object*/) {
}
