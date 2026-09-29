// Not yet assert-confirmed to a specific file; stays at the top level.
#pragma once

#include "WxStub.h"

class TManagedObject;

class TGObjectManager {
public:
	wxString GetActionText() const;

	// Confirmed called in this order from TGameControl::ResetState
	// (Deponia_Linux.asm lines 460910-460953) - not reversed beyond that
	// call shape.
	void ResetCurrentObject();
	void ResetEventInfo();
	void RemoveItem(bool flag);
	// Confirmed call shape only (TGameControl::HandleMouseMove,
	// Deponia_Linux.asm lines 472298, 472356) - called with whatever object
	// (if any) is under the cursor; not reversed beyond that.
	void MouseMove(TManagedObject *object);
};
