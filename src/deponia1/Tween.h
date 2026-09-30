// Not yet assert-confirmed to a specific file; stays at the top level.
// An animation tween/interpolation descriptor; used by
// TGameControl::StartTween. Real numeric/easing fields still unknown -
// TGameControl::Update (Deponia_Linux.asm lines 469910-470176) confirms an
// 88-byte layout with a name string (used for a dotted "table.field = value"
// Lua assignment on each update) and a completion callback invoked with 3
// args when the tween is removed, but doesn't pin down the interpolated
// value/duration/easing fields themselves - too deep to responsibly guess
// at without a dedicated pass, so Update()/IsFinished() below are stubs.
#pragma once

#include <string>

struct Tween {
	std::string name;

	// Confirmed void, not bool (TGameControl::Update, asm line 469912: the
	// very next instruction reads a field directly, no test/cmp on the
	// return register) - not reversed beyond that call shape.
	void Update(float deltaMs);
	// Confirmed call shape only (TGameControl::Update, asm line 469982) -
	// gates removal from TGameControl's pending-tween vector; not reversed
	// beyond that call shape (stubbed to never finish, so tweens are never
	// auto-removed in this reconstruction).
	bool IsFinished() const;
};

// TVisObjTween pairs a Tween with the TVisObjRef it animates - guessed from
// the StartTween(TVisObjTween const&) overload existing alongside
// StartTween(Tween const&, string const&).
#include "datastruct/visobjref.h"

struct TVisObjTween {
	TVisObjRef target;
	Tween tween;

	// Confirmed bool (TGameControl::Update, Deponia_Linux.asm line 469894:
	// its result gates an early loop exit) - real meaning ("still updating"?
	// "handled"?) not resolved; stubbed to always continue.
	bool update(double deltaMs);
};
