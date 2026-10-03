// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed call shape only (TGScene::SetScene(), Deponia_Linux.asm lines
// 172600-172880): the 0x340-byte TManagedObject subclass this scene creates
// for each of its own object links (field 0x88); not reversed beyond that
// constructor.
#pragma once

#include "TManagedObject.h"

class THObject : public TManagedObject {
public:
	explicit THObject(const TVisObjRef &ref) : TManagedObject(ref) {
	}
};
