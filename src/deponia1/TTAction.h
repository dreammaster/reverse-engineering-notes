// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed one static method only (TManagedObject::ExecuteMatchingAction,
// Deponia_Linux.asm lines 1478245-1478291) - distinct from TGAction (the
// game-action/cutscene-scripting subsystem, TGAction.h), same "TG vs TT"
// naming split as TGCharacter/THCharacter or TGObject/TTObject elsewhere
// in this project. Nothing else about this class is modeled.
#pragma once

#include "datastruct/typegrp.h"

#include "TGActionInfo.h"

class TTAction {
public:
	// Recovered from the binary's schema (vstables/records.cpp).
	static TTypeGroup &GetTypeGroup();
	static void InitType(int versionLow, int versionHigh);
	static void OnCreate(TVisionaireObject *object);
	static void OnInit(TVisionaireObject *object);

	// Confirmed (asm lines 1478536-1478560): the names a new action gets by where
	// it hangs.
	static wxString strAction;
	static wxString strRightClickAction;
	static wxString strStartAction;

	// Confirmed in full: true for exactly the 16 literal values this
	// enumerates (none individually named - see TypeActionExecution's own
	// header comment); false for everything else.
	static bool IsImmediateExecutionType(TypeActionExecution type) {
		switch (static_cast<int>(type)) {
		case 1:
		case 2:
		case 7:
		case 13:
		case 15:
		case 16:
		case 18:
		case 19:
		case 20:
		case 22:
		case 24:
		case 26:
		case 28:
		case 34:
		case 35:
		case 36:
			return true;
		default:
			return false;
		}
	}
};
