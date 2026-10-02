// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed a real, named, pure-abstract interface (3 pure virtuals; its own
// vtable/RTTI sit immediately after TManagedObject's own in the binary,
// Deponia_Linux.asm, vtable dump near line 2998940) - the interface
// TGAnimation::HideAnimation()/StartAnimation() dispatch an animation's
// "stopped"/"who owns this" queries through. TManagedObject implements it
// as one of its own two confirmed mixins (modeled as ordinary virtuals
// directly on TManagedObject, not through this interface - see
// TManagedObject.h's own header comment); TCursorControl implements it
// independently (confirmed via a `_ZThn72_`-style this-adjusting thunk at
// its own, different offset) for its own cursor animation, since a cursor
// isn't a TManagedObject at all.
#pragma once

class TGAnimation;
class TId;
class wxString;

class TAnimationOwner {
public:
	virtual ~TAnimationOwner() = default;

	virtual void AnimationStopped(TGAnimation *animation) = 0;
	virtual wxString GetOwnerName() const = 0;
	virtual TId GetOwnerId() const = 0;
};
