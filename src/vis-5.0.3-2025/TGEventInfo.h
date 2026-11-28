// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed a small, real data-only struct built fresh at each mouse-event
// call site (TGObjectManager::HandleEvent()/MouseMove()/ObjectReached()/
// ExecuteSavedObject(), TManagedObject::ExecuteEvent(), Deponia_Linux.asm)
// and passed around the action-execution subsystem by reference. Built
// from two independently-constructed TVisObjRef-shaped slots (confirmed:
// each site constructs a `TTObject`-shaped value at this struct's own
// start and a separate `TTButton`-shaped value 0x10 bytes in, rather than
// a single inherited base), plus a bool, an int, and a character pointer.
//
// Confirmed (TGObjectManager::HandleEvent(), Deponia_Linux.asm lines
// 187629-188249): `action` is built from the game's own "reached object"
// link (field 0x2AE) and `command` from its "current action" link (field
// 0x262) - matching TGObjectManager::SaveEventInfo()'s own mirroring of
// those same two fields into 0x1E4/0x1E3 respectively, which
// HandleEvent()/ObjectReached() read back to rebuild this same shape.
#pragma once

#include "TTButton.h"
#include "datastruct/visobjref.h"

class TGCharacter;

class TGEventInfo {
public:
	TGEventInfo() = default;

	// Confirmed a TVisObjRef-shaped slot (built via a TTObject ctor at
	// every call site) - see this class's own header comment for which
	// game-data link plausibly feeds it.
	TVisObjRef action;
	// Confirmed a bool (TManagedObject::GetActionsToTest()/
	// HandlePostExecution(), TGObjectManager::HandleEvent()'s own local
	// construction) - real meaning not resolved.
	bool flag8 = false;
	// Confirmed an int (TManagedObject::GetActionsToTest()/ExecuteEvent(),
	// TGObjectManager::SaveEventInfo()/HandleEvent()) - the mouse event
	// that triggered this (compared against small literal values
	// throughout, e.g. 1-6); not a recovered TMouseEventEnum value since
	// that enum itself has no named values yet.
	int mouseEvent = 0;
	// Confirmed a TVisObjRef-shaped slot, specifically built via a
	// TTButton ctor at every call site (TManagedObject::
	// ExecuteMatchingAction() calls TTButton-only methods through it
	// indirectly via the command candidates it's compared against) - see
	// this class's own header comment for which game-data link plausibly
	// feeds it.
	TTButton command;
	// Confirmed a TGCharacter* (TManagedObject::HandlePostExecution()
	// passes it straight to ClickedWithoutReach(); ExecuteEvent() calls
	// IsReached()/AlignCharacter() with its own GetRef()) - the character
	// involved in this event, or null.
	TGCharacter *character = nullptr;
};
