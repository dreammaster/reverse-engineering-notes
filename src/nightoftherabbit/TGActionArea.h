// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 247238-247290, all 3 manifest-listed methods): the
// button of an interface that is an action area (a region of the interface that runs the
// actions of its own when the player acts on it). It adds nothing to THButton but its type,
// which the interface uses to tell it from its other buttons.
#pragma once

#include "THButton.h"

class TGActionArea : public THButton {
public:
	TGActionArea(const TVisObjRef &ref, TGInterface *owner) : THButton(ref, owner) {
	}
};
