// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 114826-114878, all 3 manifest-listed methods): the
// button of an interface that stands for a place (a slot) where an item of the interface
// is shown. It adds nothing to THButton but its type: the interface finds the place holder
// of an item (TGInterface::GetPlaceHolder()) and gives it the events that were meant for
// the item.
#pragma once

#include "THButton.h"

class TGPlaceHolder : public THButton {
public:
	TGPlaceHolder(const TVisObjRef &ref, TGInterface *owner) : THButton(ref, owner) {
	}
};
