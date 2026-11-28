#include "TGScrollButton.h"

#include "vstables/fieldIds.h"

// Confirmed (asm lines 220036-220063)
TGScrollButton::TGScrollButton(const TVisObjRef &ref, TGInterface *owner) : THButton(ref, owner) {
	_scrollsBack = (_objRef.GetInt(kButtonType) == 1);
}

// Confirmed (asm lines 219895-219975): for a click (mouse event 1, 3 or 4) the scroll position
// of the interface the button is in moves by one step, forward or back.
void TGScrollButton::ExecuteEvent(TGEventInfo &info) {
	if (info.mouseEvent == 1 || info.mouseEvent == 3 || info.mouseEvent == 4) {
		TVisObjRef panel = _objRef.GetParent();
		int position = panel.GetInt(kInterfaceItemsScrollPosition);
		int step = panel.GetInt(kInterfaceScrollStepSize);

		panel.SetValue(kInterfaceItemsScrollPosition, _scrollsBack ? position - step : position + step,
		               TSendEventEnum::kSendEvent);
	}

	TManagedObject::ExecuteEvent(info);
}

// Confirmed (asm lines 219879-219883): empty.
void TGScrollButton::HandlePostExecution(TGEventInfo &/*info*/, const TGActionInfo &/*actionInfo*/) {
}
