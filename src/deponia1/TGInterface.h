// Not yet assert-confirmed to a specific file; stays at the top level.
// A single interface panel/widget. Confirmed to derive from TPaintControl
// (TMasterControl::DrawInterfaces, masterControl.cpp: each list element
// exposes a Draw-like virtual at vtable slot 1, matching the Prepare@0/
// Draw@1 pattern also seen for TCursorControl/TLoadingControl/TGScene) and
// to hold a TVisObjRef identifying it (TGameControl::GetInterface,
// Deponia_Linux.asm lines 456603-456648, compares this field against a
// lookup key) plus a GetObject() lookup (TGameControl::GetObject, asm line
// 456719) - nothing else about this class has been reversed.
#pragma once

#include "TPaintControl.h"
#include "datastruct/visobjref.h"

class TManagedObject;

class TGInterface : public TPaintControl {
public:
	const TVisObjRef &GetRef() const {
		return _ref;
	}
	// Confirmed TManagedObject* (TGameControl::StartObjectText calls
	// TManagedObject::SetText() directly on the result, same as
	// TGScene::GetObject() - asm lines 462233-462396) - the earlier void*
	// was a placeholder guess.
	TManagedObject *GetObject(const TVisObjRef &object) const;

	// Confirmed call shapes only (TGameControl::SetInterfaces, asm lines
	// 465533-465671): called on an interface leaving the active set before
	// it's dropped, and on every interface entering it, respectively.
	void RemoveSpritesAndAnimations();
	void SetObjectsActive(bool active);

private:
	TVisObjRef _ref;
};
