// Not yet assert-confirmed to a specific file; stays at the top level.
#pragma once

#include "WxStub.h"

class TManagedObject;
enum class TMouseEventEnum;

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
	// Confirmed call shape only (TGameControl::HandleMouseUp, Deponia_Linux.
	// asm lines 472812-472813 and elsewhere) - dispatches a converted mouse
	// event to whatever object is currently under/held by the cursor; not
	// reversed beyond that.
	void HandleEvent(TMouseEventEnum event);
	// Confirmed call shape only (TGameControl::HandleMouseUp, asm line
	// 472796) - not reversed beyond that.
	bool IsCurrentObjectEmpty() const;
	// Confirmed call shape only (TGameControl::HandleMouseUp, asm line
	// 472998) - not reversed beyond that.
	bool IsCurrentObjectWalkable() const;
};
