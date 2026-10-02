// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full (Deponia_Linux.asm lines 159199-159323, all 6
// manifest-listed methods) except the real source of its own `_active`
// flag (see below): a TManagedObject directly (no intermediate class) -
// a clickable rectangle over one savegame slot, confirmed constructed from
// TGScene::SetScene() with a plain screen-space wxRect. Overrides
// GetActionList()/ExecuteEvent() as no-ops (it doesn't participate in the
// action-execution engine at all - no scripted actions, no events) and
// IsInside() with a plain rect test instead of the base's polygon one.
//
// The constructor also confirms, independently, two of the three opaque
// TManagedObject bool fields found during the action-execution-subsystem
// pass: it sets `_bypassReachCheck` true (makes sense - a UI slot needs no
// walking/reach check) and `_skipFinalPostExecution` true (also sensible,
// since ExecuteEvent() is a no-op here anyway); `_hasActionTypeFallback`
// is explicitly left false.
//
// `_active` gates IsInside() entirely (false skips the rect test outright)
// but is never written anywhere in this class's own 6 methods, including
// the constructor - presumably set by TGScene's own savegame-slot-picker
// methods (GetSelectedSavegame()/GetSavegameAt()/SelectSavegame(), see
// TGScene.h) once this area is actually shown, but no confirmed call site
// for that was found; defaults to false here (unconfirmed) rather than
// guessing a setter into existence.
#pragma once

#include "TManagedObject.h"

class TMSavegameArea : public TManagedObject {
public:
	explicit TMSavegameArea(const wxRect &rect) : _rect(rect) {
		_bypassReachCheck = true;
		_skipFinalPostExecution = true;
	}

	void GetActionList(TVList &/*outActions*/) const override {
	}
	void ExecuteEvent(TGEventInfo &/*info*/) override {
	}
	bool IsInside(const wxPoint &pt) const override {
		if (!_active)
			return false;
		return _rect.Contains(pt);
	}

private:
	wxRect _rect;
	bool _active = false;
};
