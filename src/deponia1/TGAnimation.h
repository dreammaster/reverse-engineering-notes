// Not yet assert-confirmed to a specific file; stays at the top level.
// The character/scene animation subsystem. Only the entry points
// TGameControl calls are declared here (confirmed call shapes, not
// reversed bodies): ClearAnimations (~TGameControl, Deponia_Linux.asm line
// 474247) and SaveAnimations (TGameControl::Save, asm line 462896). Both
// are called without an object of this type ever being constructed at
// their call sites, so modeled as static methods (same pattern as
// TGAction's entry points).
#pragma once

#include "WxStub.h"

class TGAnimation {
public:
	static void ClearAnimations();
	static void SaveAnimations();
	// Confirmed static call shapes only (TGameControl::SaveEventHandlers,
	// Deponia_Linux.asm lines 457761-457789) - registered handler names for
	// an animation-started/animation-stopped event, stored somewhere in this
	// class rather than in TGameControl's own handler containers (unlike
	// LoadEventHandlers' four regular categories); not reversed beyond that
	// call shape.
	static wxString GetEventHandlerAnimStarted();
	static wxString GetEventHandlerAnimStopped();
	// Confirmed static (TGameControl::Load, Deponia_Linux.asm line 477011) -
	// the load-side counterpart to SaveAnimations() above; not reversed
	// beyond that call shape.
	static void LoadAnimations();
	// Confirmed static (TGameControl::Update, Deponia_Linux.asm line 469657)
	// - called once per frame; not reversed beyond that call shape.
	static void ContinueAnimations();
};
