// Not yet assert-confirmed to a specific file; stays at the top level.
// The game-action/cutscene-scripting subsystem. Only the entry points
// TGameControl calls are declared here (confirmed call shapes, not
// reversed bodies): AddRunningAction (TGameControl::HandleMouseHolding,
// Deponia_Linux.asm lines 455671-455746) and ClearActions
// (~TGameControl, asm line 474249). Both are called without an object of
// this type ever being constructed at the call site, so modeled as static
// methods rather than instance methods.
#pragma once

#include "datastruct/visobjref.h"

class TGAction {
public:
    static void AddRunningAction(const TVisObjRef& action);
    static void ContinueRunningActions(bool flag);
    static void ClearActions();
};
