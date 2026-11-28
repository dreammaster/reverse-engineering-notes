// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full (Deponia_Linux.asm lines 159199-159323, all 6
// manifest-listed methods): a TManagedObject directly (no intermediate
// class) - a clickable rectangle over one savegame slot, confirmed
// constructed from TGScene::SetScene() with a plain screen-space wxRect.
// Overrides GetActionList()/ExecuteEvent() as no-ops (it doesn't
// participate in the action-execution engine at all - no scripted actions,
// no events).
//
// The constructor stores its wxRect straight into TManagedObject's own
// _boundingRect (+0x28, the same field TManagedObject::GetBoundingRect()
// reads - TGScene::Prepare()/Draw() call exactly that on these areas to
// place each slot's screenshot) and IsInside() tests TManagedObject's own
// _active flag (+0x38, default true) before a plain rect test instead of
// the base's polygon one. An earlier version of this class modeled both as
// its own private fields - the `_active` flag it called "never written
// anywhere" is simply TManagedObject::SetActive()'s own flag, which
// TGScene's slot-picker methods flip through the normal virtual.
//
// The constructor also confirms, independently, two of the three opaque
// TManagedObject bool fields found during the action-execution-subsystem
// pass: it sets `_bypassReachCheck` true (makes sense - a UI slot needs no
// walking/reach check) and `_skipFinalPostExecution` true (also sensible,
// since ExecuteEvent() is a no-op here anyway), and explicitly clears
// `_hasActionTypeFallback` (the base constructor sets it, so a plain
// TManagedObject has it on).
#pragma once

#include "TManagedObject.h"

class TMSavegameArea : public TManagedObject {
public:
	explicit TMSavegameArea(const wxRect &rect) {
		_bypassReachCheck = true;
		_boundingRect = rect;
		_skipFinalPostExecution = true;
		_hasActionTypeFallback = false;
	}

	void GetActionList(TVList &/*outActions*/) const override {
	}
	void ExecuteEvent(TGEventInfo &/*info*/) override {
	}
	bool IsInside(const wxPoint &pt) const override {
		if (!_active)
			return false;
		return _boundingRect.Contains(pt);
	}
};
