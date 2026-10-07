// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 219879-220063, all 5 manifest-listed methods): the
// button of an interface that scrolls its items (an arrow). It is a THButton (0x2B8 bytes)
// that knows which way it scrolls (button type 1 is the one that scrolls back); a click on
// it moves the interface's scroll position (kInterfaceItemsScrollPosition) by the interface's
// step (kInterfaceScrollStepSize).
#pragma once

#include "THButton.h"

class TGScrollButton : public THButton {
public:
	TGScrollButton(const TVisObjRef &ref, TGInterface *owner);

	/** A click moves the scroll position of the interface (then the event goes on as
	 *  usual). */
	void ExecuteEvent(TGEventInfo &info) override;
	/** Does nothing (the scrolling is all that is done for a click). */
	void HandlePostExecution(TGEventInfo &info, const TGActionInfo &actionInfo) override;

private:
	bool _scrollsBack;  // +0x2B0
};
