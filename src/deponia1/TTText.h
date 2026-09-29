// Not yet assert-confirmed to a specific file; stays at the top level.
// Not to be confused with TSText/TGText (scene text objects) - "TT" here
// matches the mangled symbol exactly.
//
// Confirmed static (TGameControl::LoadAndInitGame, Deponia_Linux.asm lines
// 468197, 468212: called with no object of this type ever constructed at
// either call site, matching the TGAction/TGAnimation static-entry-point
// pattern elsewhere) - not reversed beyond that call shape.
#pragma once

#include "datastruct/visobjref.h"

class TTText {
public:
	static void SetLanguage(const TVisObjRef &language);
};
