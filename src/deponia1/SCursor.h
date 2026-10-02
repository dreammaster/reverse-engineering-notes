// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed fields, approximate names (TCursorControl::~TCursorControl()/
// Clear()/SetCursor(bool,int,bool)/SetActiveCursor()/SetInactiveCursor()/
// ReleaseMoveObject(), Deponia_Linux.asm lines 483999-485255): one entry
// per loadable cursor, heap-allocated and owned individually by
// TCursorControl's own `std::vector<SCursor *>`. `downImage`/`upImage` are
// written by GetSCursor() from fields 0x287/0x288 respectively - which one
// is "active" vs "inactive" isn't confirmed beyond SetActiveCursor() using
// the first and SetInactiveCursor() the second. `linkedIds` is populated by
// LinkButtonCursor() (not reversed); `id` and `active` are both read/
// written directly by SetCursor()'s family.
#pragma once

#include <vector>

#include "datastruct/visobjref.h"

class SCursor {
public:
	TVisObjRef downImage;
	TVisObjRef upImage;
	bool active = false;
	std::vector<int> linkedIds;
	int id = 0;
};
