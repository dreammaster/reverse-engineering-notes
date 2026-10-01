// Not yet assert-confirmed to a specific file; stays at the top level.
// The character/scene animation subsystem. Most of the static entry points
// TGameControl calls are declared here (confirmed call shapes, not
// reversed bodies): ClearAnimations (~TGameControl, Deponia_Linux.asm line
// 474247) and SaveAnimations (TGameControl::Save, asm line 462896). Both
// are called without an object of this type ever being constructed at
// their call sites, so modeled as static methods (same pattern as
// TGAction's entry points).
//
// Confirmed to derive from TCAnimation (TManagedObject::Prepare/
// RemoveSprites/IsMovingObject call TCAnimation's own methods directly on
// a TGAnimation*). The instance methods below are the leaf interface
// TManagedObject's own pass needed (HideAnimation/Prepare/Draw/DrawMixed) -
// confirmed call shapes only, not reversed bodies.
#pragma once

#include <vector>

#include "TCAnimation.h"
#include "WxStub.h"

class TManagedObject;

class TGAnimation : public TCAnimation {
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

	// Confirmed static call shape only (TManagedObject::SetAnimation/
	// ClearAnimations/RemoveAnimations/SetActive, Deponia_Linux.asm lines
	// 192657, 190934, 191252, 191550, 191563) - stops `animation` being
	// displayed on behalf of `owner` (a null owner is confirmed distinct
	// from a real one at one call site); not reversed beyond that call
	// shape. Takes a TManagedObject* directly rather than modeling the
	// original's separate TAnimationOwner mixin interface, since nothing
	// here needs to dispatch through that narrower type.
	static void HideAnimation(TGAnimation *animation, TManagedObject *owner);

	// Confirmed call shape only (TManagedObject::Prepare, asm lines 191034,
	// 191071) - not reversed.
	void Prepare() {
	}
	// Confirmed call shape only (TManagedObject::Draw, asm line 192792) -
	// not reversed.
	void Draw(float alpha, unsigned int color, int frame) {
		(void)alpha;
		(void)color;
		(void)frame;
	}
	// Confirmed call shape only (TManagedObject::Draw, asm line 192862) -
	// not reversed.
	void DrawMixed(float alpha, unsigned int color, const std::vector<TGAnimation *> &overlays) {
		(void)alpha;
		(void)color;
		(void)overlays;
	}
};
