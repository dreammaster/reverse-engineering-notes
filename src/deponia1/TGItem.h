// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed a TManagedObject subclass (its ctor takes a TVisObjRef plus a
// bool, and it overrides Draw()/HandlePreExecution()/HandlePostExecution(),
// matching TManagedObject's own virtuals - Deponia_Linux.asm lines
// 260668-261228). Only the two methods TCursorControl needs are modeled
// here (SetCenteredPosition() in full; GetPositionNextToItem() as a
// confirmed-call-shape stub, since its real body digs into TManagedObject's
// private animation/picture state that isn't exposed and a TPictureIO
// member this project doesn't model yet); the rest of TGItem's own real
// surface (manifest: 9 methods) isn't reversed yet.
#pragma once

#include "TManagedObject.h"

class TGItem : public TManagedObject {
public:
	// Confirmed call shape only (Deponia_Linux.asm line 6237C7) - the bool
	// parameter's purpose isn't resolved.
	explicit TGItem(const TVisObjRef &ref, bool /*flag*/) : TManagedObject(ref) {
	}

	// Confirmed in full (Deponia_Linux.asm lines 261209-261218): a plain
	// field setter.
	void SetCenteredPosition(const wxPoint &pos) {
		_centeredPosition = pos;
	}

	// Confirmed call shape only (Deponia_Linux.asm lines 261228-261289) -
	// not reversed; see this class's own header comment.
	wxPoint GetPositionNextToItem() const {
		return wxPoint();
	}

private:
	wxPoint _centeredPosition;
};
