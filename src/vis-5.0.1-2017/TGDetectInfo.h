// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed present as a small out-param struct (TGObjectManager::
// GetDetectInfo, Deponia_Linux.asm lines 189352-189567; TManagedObject::
// IsInside(wxPoint const&, TGDetectInfo const&) takes one but ignores it in
// the base, Deponia_Linux.asm lines 190420-190430) - two flag bytes plus a
// character link. GetDetectInfo() only ever writes 3 of the 4 possible
// (flagA, flagB) combinations - (true, false), (false, true), (true, true) -
// and only fills in `character` alongside the (false, true) and (true, true)
// cases, and then only when the game's own "held" flag (field 0x265) is
// false. Real meaning of the two flags isn't resolved, so they're named by
// position rather than guessed, the same as this project's TypeOrder/
// eVisionaireTable enums.
#pragma once

#include "datastruct/visobjref.h"

class TGDetectInfo {
public:
	bool flagA = false;
	bool flagB = false;
	TVisObjRef character;
};
