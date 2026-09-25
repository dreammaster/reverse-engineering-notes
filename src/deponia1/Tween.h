// Not yet assert-confirmed to a specific file; stays at the top level.
// An animation tween/interpolation descriptor; used by
// TGameControl::StartTween. Not reversed - fields unknown.
#pragma once

struct Tween {};

// TVisObjTween pairs a Tween with the TVisObjRef it animates - guessed from
// the StartTween(TVisObjTween const&) overload existing alongside
// StartTween(Tween const&, string const&).
#include "datastruct/visobjref.h"

struct TVisObjTween {
    TVisObjRef target;
    Tween tween;
};
